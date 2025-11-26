typedef struct _varNode{
	char *name;
	char *port; // address of port
	unsigned int mask; //bitmask of port
	int accessed;
	int isInput; // 1 for input, 0 for output
} t_varNode;

typedef enum {tConst, tVar, tBlock, tAssign, tLoop, tDelay, tIf} t_nodeType;

typedef struct _node{
	t_nodeType type;
	t_varNode *var; //tVar
	int value; //tConst
	int n; //tOp number of operands
	struct _node **children; //tOp operands
} t_node;

t_varNode *findVar(char *name);
t_varNode *newVar(char *name, char *port, unsigned int mask, int isInput);
