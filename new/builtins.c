//So this is broken, um
//I think the function is not popped from the stack when it should be ??

#include "types.h"

// void f_length(uint args){
	// struct Value * a = pop_l();
	// switch(a->type){
	// case tArray:
		// replace(&(struct Value){.type = tNumber, .number = (double)a->array->length});
	// break;case tTable:
		// die("unsupported\n");
	// break;case tList:
		// replace(&(struct Value){.type = tNumber, .number = (double)a->args});
	// break;case tString:
		// replace(&(struct Value){.type = tNumber, .number = (double)a->string->length});
	// break;default:
		// die("wrong type");
	// }
// }

//list_get is used to get arguments
//0 is the function itself, 1 is the first argument, etc.
//use push to return.

char arg_types[256];

extern char * type_name[8];

void print_args(){
	printf("( ");
	uint i = 1;
	while(1){
		enum Type type = list_get(i).type;
		if(type==tNArgs)
			break;
		if(i>1)
			printf(" , ");
		printf(type_name[type]);
		i++;
	}
	printf(" )");
}

void get_arg(uint index, struct Value * value, uint allowed_types) {
	*value = list_get(index);
	if(!(1<<value->type & allowed_types)){
		printf("Wrong type passed to function %s.\n", variable_pointer_to_name(list_get(0).variable)); //get function name SOMEHOW
		printf("Arguments: ");
		print_args();
		printf("\nArgument %d was expected to be: ", index);
		uint i;
		for(i=0;i<8;i++){
			if(allowed_types & 1<<i){
				printf("%s ", type_name[i]);
			}
		}
		printf(".\n");
		longjmp(err_ret, 2);
	}
}

//maybe have a list of all the valid syntax forms for each function

void f_array_fill(uint args){
	switch(args){
	case 2:;
		struct Value array, value;
		get_arg(1, &array, 1<<tArray);
		get_arg(2, &value, -1);
		uint i;
		for(i=0;i<array.array->length;i++)
			assign_variable(array.array->pointer+i, value);
		push((struct Value){.type=tNone});
	case 4:;
		struct Value number;
		get_arg(1, &array, 1<<tArray);
		get_arg(2, &value, -1);
		get_arg(3, &number, 1<<tNumber);
		uint start = number.number;
		get_arg(4, &number, 1<<tNumber);
		uint length = number.number;
		for(i=start;i<start+length && i<array.array->length;i++)
			assign_variable(array.array->pointer+i, value);
		push((struct Value){.type=tNone});
	break;default:
		die("Wrong number of arguments passed to function. Expected 2 or 4, got %d\n", args);
	}
}


//what if none was replaced with an empty list?
//hmm
//but then it couldn't be inside another list (well it could.... but it would be weird...)

//calling a function:

//[function] [args] ... [# of args] | (stack pointer)
//pop_l
//[function] | [args] ... [# of args]
//pop
//| [function] [args] ... [# of args]
//args are accessed by reading (stack pointer + arg #) (1 indexed)
//push return value(s):
//[result] ... |
