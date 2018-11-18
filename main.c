#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <setjmp.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include "alloc_init.c"

#include "run.c"
#include "token.c"
#include "parser.c"

uint get_line(Address address){
	uint i;
	for(i=0;i<ARRAYSIZE(line_position_in_output);i++)
		if(line_position_in_output[i]>address)
			break;
	return i-1;
}

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
	switch(setjmp(err_ret)){
	case 0:;
		struct Item * bytecode = parse(input);
		run(bytecode);
		return 0;
	break;case 1:
		//parsing error
	break;case 2:
		printf("Error while running line %d\n", get_line(pos));
		uint i;
		for(i = call_stack_pointer-1;i!=-1;i--){
			printf("Also check line %d\n", get_line(call_stack[i]));
		}
	}
	return 1;
}