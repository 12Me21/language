#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <setjmp.h>
#include <string.h>

jmp_buf err_ret;

typedef uint32_t Address; //location in the bytecode

enum Type {
	tNumber,
	tString,
	tTable,
	tFunction,
	tBoolean,
	tNone,
};

struct String {
	char * pointer;
	uint32_t length;
	//int references;
};

struct Variable;

// A value or variable
// Variables just contain the extra "constraint_expression" field
struct Value {
	enum Type type;
	union {
		double number;
		struct String * string;
		struct Table * table;
		Address function;
		bool boolean;
	};
	struct Variable * variable;
};

struct Variable {
	struct Value value;
	//Address constraint_expression;
};

// every distinct operator
// subtraction and negation are different operators
// functions are called using the "call function" operator
// which takes as input the function pointer and number of arguments
// <function ptr> <args ...> <# of args> CALL
enum Operator {
	oInvalid,
	
	oConstant,
	oVariable,
	
	oAdd,
	oSubtract,
	oPrint1,
	oTest,
	oAssign,
	oHalt,
};

#define ptr(x) (x *)

//this takes up a lot of space (at least 40 bytes I think)... perhaps this should just be a Value, with a special [type]
struct Item {
	enum Operator operator;
	union {
		struct Value value;
		Address address; //For example, IF uses this to skip past the code when its condition is false.
		struct Variable * variable;
	};
};
//ideally:
// code item:
// - operator type / value type
// - number / string / table / function / boolean / address / variable reference
// stack item:
// - value type
// - number / string / table / function / boolean
// - variable reference
// variable:
// - stack item
// - constraint expression

#define STACK_SIZE 256
//#define die longjmp(err_ret, 0)
#define die(message) {printf(message); longjmp(err_ret, 0);}
struct Value stack[STACK_SIZE];
uint32_t stack_pointer = 0;

struct Item code[65536];
Address pos = 0;

void push(struct Value value){
	if(stack_pointer >= STACK_SIZE)
		die("Stack Overflow\n");
	stack[stack_pointer++] = value;
}

struct Value pop(){
	if(stack_pointer <= 0)
		die("Internal Error: Stack Underflow\n");
	return stack[--stack_pointer];
}

void * memdup(void * source, size_t size) {
	void * mem = malloc(size);
	return mem ? memcpy(mem, source, size) : NULL;
}
#define ALLOC_INIT(type, ...) memdup((type[]){ __VA_ARGS__ }, sizeof(type))

int main(){
	struct Variable * x = ALLOC_INIT(struct Variable, {.value = {.type = tNumber, .number = 1}});
	x->value.variable = x;
	
	code[0] = (struct Item){.operator = oVariable, .variable = x};
	code[1] = (struct Item){.operator = oConstant, .value = {.type = tNumber, .number = 4}};
	code[2] = (struct Item){.operator = oAssign};
	code[3] = (struct Item){.operator = oVariable, .variable = x};
	code[4] = (struct Item){.operator = oPrint1};
	code[5] = (struct Item){.operator = oHalt};
	
	if(setjmp(err_ret)){
		printf("error\n");
		return 1;
	}else{
		while(1){
			struct Item item = code[pos++];
			//if(item == NULL)
			//	break;
			
			switch(item.operator){
			case oConstant:
				push(item.value);
				break;
			case oVariable:
				push(item.variable->value);
				break;
			case oAdd:;
				struct Value a = pop();
				struct Value b = pop();
				if(a.type == tNumber && b.type == tNumber){
					push((struct Value){.type = tNumber, .number = a.number + b.number});
				}else{
					die("Type mismatch in +\n");
				}
				break;
			case oAssign:;
				struct Value value = pop();
				struct Value variable = pop();
				variable.variable->value = value;
				break;
			case oPrint1:;
				a = pop();
				if(a.type == tNumber){
					printf("%f\n",a.number);
				}else{
					die("Type mm\n");
				}
				break;
			case oTest:
				printf("test\n");
				break;
			case oHalt:
				die("End of program\n");
				break;
			default:
				die("Unsupported operator\n");
			}
		}
	}
	printf("stopped\n");
	return 0;
}

//important:
//make sure that values don't contain direct pointers to strings/tables since they might need to be re-allocated.
//don't get rid of the Variable struct since it's needed for keeping track of constraints etc.

//Jobs for the parser:
// - generate a list of all properties (table.property) independantly from other words