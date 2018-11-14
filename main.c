#include "run.c"
#include "token.c"
#include "parser.c"

int main(){
	printf("A");
	FILE * input = fopen("test.prg", "r");
	printf("B");
	struct Item * bytecode = parse(input);
	printf("C");
	run(bytecode);
	printf("D");
	return 0;
}