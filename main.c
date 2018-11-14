#include "run.c"
#include "token.c"
#include "parser.c"

int main(){
	printf("A");
	FILE * input = fopen("test.prg", "r");
	struct Item * bytecode = parse(input);
	run(bytecode);
	return 0;
}