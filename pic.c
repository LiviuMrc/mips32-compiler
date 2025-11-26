#include "pic.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
extern int yyparse(void);
extern FILE *yyin;
extern int yylex_destroy(void);

extern int varCount;
extern t_varNode **variables;
extern t_node *root;

int labelcount;

//Creates variables and assigns them to ports
void createVars(){
	// LEDs (outputs) - all on PORTA bits 0-7
	newVar("LD0", "A", 0x1, 0);   // Port A, bit 0
	newVar("LD1", "A", 0x2, 0);   // Port A, bit 1
	newVar("LD2", "A", 0x4, 0);   // Port A, bit 2
	newVar("LD3", "A", 0x8, 0);   // Port A, bit 3
	newVar("LD4", "A", 0x10, 0);  // Port A, bit 4
	newVar("LD5", "A", 0x20, 0);  // Port A, bit 5
	newVar("LD6", "A", 0x40, 0);  // Port A, bit 6
	newVar("LD7", "A", 0x80, 0);  // Port A, bit 7

	// Switches (inputs)
	newVar("SW0", "F", 0x8, 1);    // Port F, bit 3
	newVar("SW1", "F", 0x20, 1);   // Port F, bit 5
	newVar("SW2", "F", 0x10, 1);   // Port F, bit 4
	newVar("SW3", "D", 0x8000, 1); // Port D, bit 15
	newVar("SW4", "D", 0x4000, 1); // Port D, bit 14
	newVar("SW5", "B", 0x800, 1);  // Port B, bit 11
	newVar("SW6", "B", 0x400, 1);  // Port B, bit 10
	newVar("SW7", "B", 0x200, 1);  // Port B, bit 9

	// RGB LEDs (outputs)
	newVar("R", "D", 0x4, 0);      // Port D, bit 2
	newVar("G", "D", 0x1000, 0);   // Port D, bit 12
	newVar("B", "D", 0x8, 0);      // Port D, bit 3
}

// Creates a label for JUMP statement
void label(FILE *f, int a){
		fprintf(f, ".lbl%d:\n", a);
}

// Code that jumps to label
void go(FILE *f, int a){
		fprintf(f, "\tj\t.lbl%d\n", a);
		fprintf(f, "\tnop\n");
}

// Assigns a bit value to an output type Variable
void writeVarConst(FILE *f, t_varNode *v, int k){
	if(v == NULL){
		printf("Variable missing!\n");
		return;
	}
	fprintf(f, "\t\n");
	fprintf(f, "\t# assign to variable %s\n", v->name);
	fprintf(f, "\tlui\t$8,%%hi(LAT%s%s)\n", v->port, k == 1 ? "SET" : "CLR");
	fprintf(f, "\tli\t$9,0x%x\n", v->mask);
	fprintf(f, "\tsw\t$9,%%lo(LAT%s%s)($8)\n", v->port, k == 1 ? "SET" : "CLR");
}

// Reads from input variable and writes to output variable
void writeVarVar(FILE *f, t_varNode *led, t_varNode *sw){
	if(led == NULL || sw == NULL){
		printf("Variable missing!\n");
		return;
	}
	fprintf(f, "\t\n");
	fprintf(f, "\t# assign %s to %s\n", sw->name, led->name);

	// Read from switch
	fprintf(f, "\tlui\t$8,%%hi(PORT%s)\n", sw->port);
	fprintf(f, "\tlw\t$9,%%lo(PORT%s)($8)\n", sw->port);
	fprintf(f, "\tandi\t$9,$9,0x%x\n", sw->mask);

	// Create labels for if-then-else
	int thenLabel = labelcount++;
	int elseLabel = labelcount++;
	int endLabel = labelcount++;

	// If switch value is not zero, jump to then block
	fprintf(f, "\tbne\t$9,$0,.lbl%d\n", thenLabel);
	fprintf(f, "\tnop\n");

	// Else block: turn LED off
	fprintf(f, "\tlui\t$8,%%hi(LAT%sCLR)\n", led->port);
	fprintf(f, "\tli\t$9,0x%x\n", led->mask);
	fprintf(f, "\tsw\t$9,%%lo(LAT%sCLR)($8)\n", led->port);
	fprintf(f, "\tj\t.lbl%d\n", endLabel);
	fprintf(f, "\tnop\n");

	// Then block: turn LED on
	fprintf(f, ".lbl%d:\n", thenLabel);
	fprintf(f, "\tlui\t$8,%%hi(LAT%sSET)\n", led->port);
	fprintf(f, "\tli\t$9,0x%x\n", led->mask);
	fprintf(f, "\tsw\t$9,%%lo(LAT%sSET)($8)\n", led->port);

	// End label
	fprintf(f, ".lbl%d:\n", endLabel);
}

// Generate assembly for DELAY statement
void generateDelay(FILE *f, int delayValue){
	fprintf(f, "\t\n");
	fprintf(f, "\t# delay %d\n", delayValue);

	// Load delay value into register $10
	fprintf(f, "\tli\t$10,0x%x\n", delayValue);

	// Create a label for the delay loop
	int delayLabel = labelcount++;
	fprintf(f, ".lbl%d:\n", delayLabel);

	// Decrement the register
	fprintf(f, "\taddi\t$10,$10,-1\n");

	// If register is not zero, jump back to delay loop
	fprintf(f, "\tbne\t$10,$0,.lbl%d\n", delayLabel);
	fprintf(f, "\tnop\n");
}

// Convert syntax tree to assembly
void translate(FILE *f, t_node *p){
	int i;
	int loopLabel;
	switch(p->type){
		case tAssign:
			if(p->children[0]->type == tConst)
				writeVarConst(f, p->children[1]->var, p->children[0]->value);
			else if(p->children[0]->type == tVar)
				writeVarVar(f, p->children[1]->var, p->children[0]->var);
			break;
		case tBlock:
			for(i = 0; i < p->n; i++)
				translate(f, p->children[i]);
			break;
		case tLoop:
			loopLabel = labelcount++;
			label(f, loopLabel);
			translate(f, p->children[0]);
			go(f, loopLabel);
			break;
		case tDelay:
			generateDelay(f, p->children[0]->value);
			break;
		case tIf:
			{
				t_varNode *sw = p->children[0]->var;
				int thenLabel = labelcount++;
				int elseLabel = labelcount++;
				int endLabel = labelcount++;

				// Read switch value
				fprintf(f, "\t\n");
				fprintf(f, "\t# if (%s)\n", sw->name);
				fprintf(f, "\tlui\t$8,%%hi(PORT%s)\n", sw->port);
				fprintf(f, "\tlw\t$9,%%lo(PORT%s)($8)\n", sw->port);
				fprintf(f, "\tandi\t$9,$9,0x%x\n", sw->mask);

				// If switch is pressed (not zero), go to then block
				fprintf(f, "\tbne\t$9,$0,.lbl%d\n", thenLabel);
				fprintf(f, "\tnop\n");

				// Else block (if exists)
				if(p->n == 3){
					// Has ELSE
					translate(f, p->children[2]);
					fprintf(f, "\tj\t.lbl%d\n", endLabel);
					fprintf(f, "\tnop\n");
				} else {
					// No ELSE, jump to end
					fprintf(f, "\tj\t.lbl%d\n", endLabel);
					fprintf(f, "\tnop\n");
				}

				// Then block
				fprintf(f, ".lbl%d:\n", thenLabel);
				translate(f, p->children[1]);

				// End label
				fprintf(f, ".lbl%d:\n", endLabel);
			}
			break;
		// you should add all the other node types here
	}
}

// First statements of the program, setting up code segment
void init(FILE *f){
	fprintf(f, "\t.global main\n");
	fprintf(f, "\t.text\n");
	fprintf(f, "\t.set noreorder\n");
	fprintf(f, "\t.ent main\n");
	fprintf(f, "main:\n");
}

// Last statements of the program, eternal loop. Also initialization of board parameters. DO NOT CHANGE THESE!
void conclude(FILE *f){
	//v  this is endless loop at end of program
	int a = labelcount++;
	label(f, a);
	go(f, a);
	//^ end this
	
	fprintf(f, "\t\n");
	fprintf(f, "\t.end\tmain\n");
	fprintf(f, "\t\n");
	// Board settings
	fprintf(f, "\t.section\t.config_BFC02FFC, code, keep, address(0xBFC02FFC) \n");
	fprintf(f, "\t.word\t0x7FFFFFFB \n");
	fprintf(f, "\t.section\t.config_BFC02FF8, code, keep, address(0xBFC02FF8)\n");
	fprintf(f, "\t.word\t0xFF74FD5B \n");
	fprintf(f, "\t.section\t.config_BFC02FF4, code, keep, address(0xBFC02FF4) \n");
	fprintf(f, "\t.word\t0xFFF8FFD9 \n");
	fprintf(f, "\t.section\t.config_BFC02FF0, code, keep, address(0xBFC02FF0) \n");
	fprintf(f, "\t.word\t0xCFFFFFFF\n");
}

// Configure port variables as analog or digital, as input or output.
void initVars(FILE *f){
	fprintf(f, "\t# configure PORTA as digital\n");
	fprintf(f, "\tlui\t$8,%%hi(ANSELA)\n");
	fprintf(f, "\tsw\t$0,%%lo(ANSELA)($8)\n");

	// Configure PORTB as digital
	fprintf(f, "\t# configure PORTB as digital\n");
	fprintf(f, "\tlui\t$8,%%hi(ANSELB)\n");
	fprintf(f, "\tsw\t$0,%%lo(ANSELB)($8)\n");

	// Configure PORTD as digital
	fprintf(f, "\t# configure PORTD as digital\n");
	fprintf(f, "\tlui\t$8,%%hi(ANSELD)\n");
	fprintf(f, "\tsw\t$0,%%lo(ANSELD)($8)\n");

	// Configure PORTF as digital
	fprintf(f, "\t# configure PORTF as digital\n");
	fprintf(f, "\tlui\t$8,%%hi(ANSELF)\n");
	fprintf(f, "\tsw\t$0,%%lo(ANSELF)($8)\n");

	for(int i = 0; i < varCount; i++)
		if(variables[i]->accessed){
			fprintf(f, "\t\n");
			fprintf(f, "\t# initialize variable %s\n", variables[i]->name);
			if(variables[i]->isInput){
				// Configure as input (set TRIS bit)
				fprintf(f, "\tlui\t$8,%%hi(TRIS%sSET)\n", variables[i]->port);
				fprintf(f, "\tli\t$9,0x%x\n", variables[i]->mask);
				fprintf(f, "\tsw\t$9,%%lo(TRIS%sSET)($8)\n", variables[i]->port);
			} else {
				// Configure as output (clear TRIS bit)
				fprintf(f, "\tlui\t$8,%%hi(TRIS%sCLR)\n", variables[i]->port);
				fprintf(f, "\tli\t$9,0x%x\n", variables[i]->mask);
				fprintf(f, "\tsw\t$9,%%lo(TRIS%sCLR)($8)\n", variables[i]->port);
			}
		}
}

// Prints the tree
void printNode(t_node *p, int level){
	if(p == NULL){
		printf("%*c NULL\n", level, ' ');
		return;
	}
	int i;
	switch(p->type){
		case tConst: 	printf("%*c const: %d\n", level, ' ', p->value);
			break;
		case tVar: 	if(p->var == NULL)
				printf("%*c var(NULL)\n", level, ' ');
			else
				printf("%*c var(%s): ? \n", level, ' ', p->var->name);
			break;
		case tAssign: 	printf("%*c op: %d\n", level, ' ', p->type);
			for(i = 0; i < p->n; i++)
				printNode(p->children[i], level + 4);
			break;
		case tBlock: printf("%*c block\n", level, ' ');
			for(i = 0; i < p->n; i++)
				printNode(p->children[i], level + 4);
			break;
		case tLoop: printf("%*c loop\n", level, ' ');
			for(i = 0; i < p->n; i++)
				printNode(p->children[i], level + 4);
			break;
		case tDelay:
			printf("%*c delay: %d\n", level, ' ', p->children[0]->value);
			break;
		case tIf:
			if(p->n == 3)
				printf("%*c if-else (%s)\n", level, ' ', p->children[0]->var ? p->children[0]->var->name : "?");
			else
				printf("%*c if (%s)\n", level, ' ', p->children[0]->var ? p->children[0]->var->name : "?");
			for(i = 1; i < p->n; i++)
				printNode(p->children[i], level + 4);
			break;
	}
}

// main program. It expects 1 or 2 parameters, first one is input file (example.pic), second is output file (prog.s)
int main(int argc, char *argv[]){
	if(argc < 2){
		printf("nothing to compile\n");
		return 0;
	}
	yyin = fopen(argv[1], "r");
	if(yyin == NULL){
		printf("could not open file %s\n", argv[1]);
		return -1;
	}

	labelcount = 1;
	
	varCount = 0;
	variables = NULL;
	createVars();
	
	root = NULL;
	yyparse();
	fclose(yyin);
	yylex_destroy();
	
	printNode(root, 0);
	
	FILE *f = NULL;
	if(argc > 2)
		f = fopen(argv[2], "w");
	if(f == NULL)
		f = fopen("prog.s", "w");
	init(f);
	initVars(f);
	translate(f, root);
	conclude(f);
	fclose(f);
}

