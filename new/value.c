#include "types.h"

//check if a Value is truthy
bool truthy(struct Value value){
	if(value.type == tNone)
		return false;
	if(value.type == tBoolean)
		return value.boolean;
	//if(value.type == tNArgs)
	//	die("internal error\n");
	return true;
}

struct Value make_boolean(bool boolean){
	return (struct Value){.type = tBoolean, .boolean = boolean};
}

bool equal(struct Value a, struct Value b){
	if(a.type == b.type){
		switch(a.type){
		case tNumber:
			return a.number == b.number;
		case tBoolean:
			return a.boolean == b.boolean;
		case tArray:
			if(a.array->length!=b.array->length)
				return false;
			uint i;
			for(i=0;i<a.array->length;i++)
				if(!equal(a.array->pointer[i].value,b.array->pointer[i].value))
					return false;
			return true;
			//return a.array == b.array;
		case tTable:
			//return a.table == b.table;
			die("not supported");
		case tString:
			return
				a.string->length == b.string->length && 
				!memcmp(a.string->pointer, b.string->pointer, a.string->length * sizeof(char));
		case tFunction:
			if(a.builtin==b.builtin){
				if(a.builtin)
					return a.c_function == b.c_function;
				else
					return a.user_function == b.user_function;
			}
			break;
		case tNone:
			return true;
		default:
			die("Internal error: Type mismatch in comparison, somehow\n");
		}
	}
	return false;
}

//compare 2 values. a>b -> -1, a==b -> 0, a<b -> 1
//should be 0 iff a==b but I can't guarantee this...
int compare(struct Value a, struct Value b){
	if(a.type != b.type)
		die("Type mismatch in comparison\n");
	switch(a.type){
	case tNumber:
		if(a.number < b.number)
			return -1;
		if(a.number > b.number)
			return 1;
		return 0;
	case tBoolean:
		return a.boolean - b.boolean;
	case tArray:
		die("Tried to compare arrays\n");
	case tTable:
		die("Tried to compare tables\n");
	case tString:;
		uint len1 = a.string->length;
		uint len2 = b.string->length;
		if(len1<len2){
			int x = memcmp(a.string->pointer, b.string->pointer, len1 * sizeof(char));
			return x ? x : -1;
		}
		int x = memcmp(a.string->pointer, b.string->pointer, len2 * sizeof(char));
		return x || len1 == len2 ? x : 1;
	case tFunction:
		die("Tried to compare functions\n");
	case tNone:
		return 0;
	default:
		die("Internal error: Type mismatch in comparison, somehow\n");
	}
}

uint to_string(struct Value value, char * output){
	switch(value.type){
	case tString:
		memcpy(output, value.string->pointer, value.string->length);
		return value.string->length;
	case tBoolean:
		if(value.boolean){
			memcpy(output, "true", 4);
			return 4;
		}
		memcpy(output, "false", 5);
		return 5;
	case tNone:
		memcpy(output, "None", 4);
		return 4;
	case tFunction:
		memcpy(output, "(function)", 10);
		return 10;
	case tTable:
		memcpy(output, "(table)", 7);
		return 7;
	case tArray:;
		char * start = output;
		*(output++) = '[';
		uint i;
		for(i=0;i<value.array->length;i++){
			if(i)
				*(output++) = ',';
			output += to_string(value.array->pointer[i].value, output);
		}
		*(output++) = ']';
		return (output - start);
	case tNumber:
		//NaN
		if(isnan(value.number)){
			memcpy(output, "NaN", 3);
			return 3;
		}
		//+-Infinity
		if(isinf(value.number)){
			if(value.number>0){
				memcpy(output, "Infinity", 8);
				return 8;
			}
			memcpy(output, "-Infinity", 9);
			return 9;
		}
		//should never use scientific notation:
		return sprintf(output, "%.*g", value.number == nearbyintf(value.number) ? 99999 : 15, value.number);
	default:
		die("tried to convert some dumb shit into a string\n");
	}
}

void basic_print(struct Value value){
	switch(value.type){
	case tNumber:
		printf("%g", value.number); //this sucks
	break;case tString:
		fwrite(value.string->pointer, sizeof(char), value.string->length, stdout);
	break;case tFunction:
		printf("function"); //maybe store the function name somewhere...
	break;case tBoolean:
		printf(value.boolean ? "true" : "false");
	break;case tTable:
		printf("table");
	break;case tArray:;
		int i;
		printf("[");
		for(i=0;i<value.array->length;i++){
			if(i)
				printf(",");
			basic_print(value.array->pointer[i].value);
		}
		printf("]");
	break;case tNone:
		printf("None");
	break;case tNArgs:
		die("Internal error: tried to print # of args what\n");
	break;default:
		die("can't print aaaa\n");
	}
}

