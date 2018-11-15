#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <setjmp.h>
#include <string.h>
#include <math.h>
#include "alloc_init.c"
#include "run.c"
#include "token.c"
#include "parser.c"

int main(int argc, char * argv[]){
	if(argc != 2){
		printf("Wrong number of arguments. Try again.\n");
		return 1;
	}
	FILE * input = fopen(argv[1], "r");
	if(!input){
		printf("Wrong number of arguments. Try again.\n"); //evil
		return 1;
	}
	struct Item * bytecode = parse(input);
	run(bytecode);
	return 0;
}