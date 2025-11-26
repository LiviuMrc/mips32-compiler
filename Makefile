CC := gcc
CFLAGS := -Wall -Wextra -g
OBJS := parser.o scanner.o pic.o
TGT := pic

all: $(TGT)

$(TGT): $(OBJS) pic.h
	$(CC) $^ -o $@ $(CFLAGS)

scanner.c: scanner.l pic.h
	flex -o $@ $<

parser.c: parser.y pic.h
	bison -d -o $@ $<

pic.o: pic.c pic.h
	$(CC) $< -c $(CFLAGS)
	
clean:
	$(RM) $(TGT) $(OBJS) scanner.c parser.c parser.h

rebuild: clean all 
