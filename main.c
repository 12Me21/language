#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <setjmp.h>
#include <string.h>
#include <math.h>
#include <time.h>

#include "alloc_init.c"

typedef uint32_t uint;
jmp_buf err_ret;
#define die(...) {printf(__VA_ARGS__); longjmp(err_ret, 2);}

#include "val_item.c"
#include "token.c"
#include "parser.c"
#include "run.c"

int main(int argc, char * argv[]){
	FILE * input = NULL;
	char * string_input = NULL;
	if(argc == 2){
		input = fopen(argv[1], "r");
		if(!input){
			printf("Could not load file\n");
			return 1;
		}
	}else if(argc == 3){
		string_input = argv[2];
	}else{
		printf("Wrong number of arguments. Try again.\n");
		return 1;
	}
	switch(setjmp(err_ret)){
	case 0:;
		struct Item * bytecode = parse(input, string_input);
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