#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <setjmp.h>
#include <string.h>
#include "alloc_init.h"

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

// Multiple things may point to the same String or Table
struct String {
	char * pointer;
	uint32_t length;
	//int references;
};

struct Variable;

// A value or variable
// Variables just contain the extra "constraint_expression" field
// Multiple things should not point to the same Value or Variable. EVER.
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
	Address constraint_expression;
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
	oPrint,
	oTest,
	oAssign,
	oHalt,
	oTable, //table literal.
	//<key><value><key><value>...<oTable(# of items)>
	oIndex,
};

//this takes up a lot of space (at least 40 bytes I think)... perhaps this should just be a Value, with a special [type]
struct Item {
	enum Operator operator;
	union {
		struct Value value;
		Address address; //For example, IF uses this to skip past the code when its condition is false.
		//struct Variable * variable; //replace with index into the variables array if you want any hope of recursion.
		struct {
			unsigned int scope;
			unsigned int index;
		};
		unsigned int length;
		unsigned int locals;
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


//struct Variable variables[256];
struct Item code[65536];
Address call_stack[256];
uint32_t call_stack_pointer = 0;
struct Variable * scope_stack[256];
unsigned int scope_pointer = 0;

Address pos = 0;

void push_scope(int locals){
	if(scope_stack_pointer >= SCOPE_STACK_SIZE)
		die("Scope Stack Overflow\n");
	scope_stack[scope_stack_pointer++] = malloc(sizeof(struct Variable), locals);
	//variables will not be initialized. you must use VAR first
	//perhaps they should be set to some known value so error checking can happen.
}

void pop_scope(){
	if(scope_stack_pointer <= 0)
		die("Internal Error: Scope Stack Underflow\n");
	free(scope_stack[--call_stack_pointer]);
}

void call(Address address){
	if(call_stack_pointer >= CALL_STACK_SIZE)
		die("Call Stack Overflow\n");
	call_stack[call_stack_pointer++] = pos;
	pos = address;
}

void ret(){
	if(call_stack_pointer <= 0)
		die("Internal Error: Call Stack Underflow\n");
	void call(Address address){
	pos = call_stack[--call_stack_pointer];
}

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

struct Value stack_get(int depth){
	if(stack_pointer-depth < 0)
		die("Internal Error: Stack Underflow\n");
	return stack[stack_pointer-depth];
}

void stack_discard(int amount){
	if(stack_pointer-amount < 0)
		die("Internal Error: Stack Underflow\n");
	stack_pointer -= amount;
}

#include "table.c"

int main(){
	struct Variable * x = ALLOC_INIT(struct Variable, {.value = {.type = tNumber, .number = 1}});
	x->value.variable = x;
	struct Variable * y = ALLOC_INIT(struct Variable, {.value = {.type = tNumber, .number = 1}});
	y->value.variable = y;
	
	unsigned int i=0;
	
	code[i++] = (struct Item){.operator = oScope, .locals = 2}; //required
	code[i++] = (struct Item){.operator = oConstant, .value = {.type = tNumber, .number = 4}};
	code[i++] = (struct Item){.operator = oConstant, .value = {.type = tNumber, .number = 8}};
	code[i++] = (struct Item){.operator = oTable, .length = 1};
	code[i++] = (struct Item){.operator = oConstant, .value = {.type = tNumber, .number = 4}};
	code[i++] = (struct Item){.operator = oIndex, .length = 1};
	code[i++] = (struct Item){.operator = oPrint, .length = 1};
	code[i++] = (struct Item){.operator = oHalt};
	
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
				if(item.scope) //local var
					push(scope_stack[scope_stack_pointer-item.scope][item.index]);
				else //global var
					push(scope_stack[0][item.index]);
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
			case oPrint:;
				for(i = item.length;i>=1;i--){
					a = stack_get(i);
					switch(a.type){
					case tNumber:
						printf("%g", a.number);
						break;
					case tString:
						fwrite(a.string->pointer, sizeof(char), a.string->length, stdout);
						break;
					case tFunction:
						printf("function"); //maybe store the function name somewhere...
						break;
					case tBoolean:
						if(a.boolean)
							printf("true");
						else
							printf("false");
						break;
					case tTable:
						printf("table");
					}
					if(i!=1)
						printf("\t");
				}
				stack_discard(item.length);
				printf("\n");
				break;
			case oHalt:
				goto end;
			case oTable:;
				//this is for table literals
				//maybe those should just compile to a bunch of VAR statements?
				i=item.length;
				struct Table * table = ALLOC_INIT(struct Table, {.first=NULL, .last=NULL});
				while(i-->0){
					a = pop();
					b = pop();
					table_declare(table, b, ALLOC_INIT(struct Variable, {.value = a}));
				}
				push((struct Value){.type = tTable, .table = table});
				break;
			case oIndex:; //table, key
				b = pop(); //key
				a = pop(); //table
				if(a.type!=tTable)
					die("tried to index something that wasn't a table\n");
				push(table_lookup(a.table, b));
				break;
			case oCall:
				call(item.address);
				break;
			case oRet:
				ret();
				break;
			case oDiscard: //used after calling functions where the return value is not used.
				pop();
				break;
			case oScope:
				push_scope(item.locals);
				break;
			case oReturn:
				//garbage collect here
				pop_scope();
				break;
			default:
				die("Unsupported operator\n");
			}
		}
	}
	end:
	printf("stopped\n");
	return 0;
}

//important:
//make sure that values don't contain direct pointers to strings/tables since they might need to be re-allocated.
//don't get rid of the Variable struct since it's needed for keeping track of constraints etc.

//Jobs for the parser:
// - generate a list of all properties (table.property) independantly from other words\

//when parsing:
// VAR A
// DEF X
 // VAR B
 // DEF Y
  // VAR C
  // B = 1
 // END
 // Y
// END

//VAR A -> add "A" to top of scope stack. push variable pointer (scope = top, index = 0)
//DEF X -> add "X" to top of scope stack. create variable X tFunction function=address. add layer to scope stack, push function things.
// (note: this includes the number of local variables that must be created. which will have filled in after the function has been parsed (use block stack)
//VAR B -> add "B" to top of scope stack. push variable pointer (scope = top, index = 0)
//DEF Y -> "
//VAR C -> add "C". push variable (scope = top, index = 0)
//B -> search downwards in scope stack for "B". will push (scope = top-1, index=0)
//...

//Hoisting functions
//This is nessesary to preserve the sanity of programmers.
//not sure the best way to handle this.

//calling a function
//DEF TEST(A,B,C):END:TEST(1,2,3) is compiled to
//1 2 3 3(because 3 arguments were passed) TEST CALL ... @TEST PUSHSCOPE{3} VAR{A} VAR{B} VAR{C} MULTIASSIGN{3}
//CALL pops a function from the stack and calls it.
//PUSHSCOPE pushes a scope to the scope stack, creating the right number of local variables. in this case it's 3 (because arguments are local vars)
//MULTIASSIGN will perform multiple assignments. first it reads the stack entry before the variable list, which is the number of arguments that were passed.
// it then assigns values to the variables, and discards everything on the stack (up to the first value passed to the function)

//pushing to the scope stack:
// this will create lots of new variables
// the tFunction type will have a function address

//when a variable is encountered in the bytecode:
//-- use its indexes to find the Variable in the scope stack.
//-- make a copy of its value. set the variable pointer to point at the Variable in the scope stack. (note: it's important that RETURN and other operations do not preserve that pointer)
//-- push that to the stack.

//recursion idea:
VAR A
DEF X
 VAR B
 DEF Y
  VAR C
  B = 1
 END
 Y
END

X

// X is called
//  new scope is created
//  B is added to that scope
//  (VAR B) -> B in bytecode points to the 0th item at the top scope.
//  Y is called
//   new scope is created
//   C is added to that scope
//   C points to 0th item at the top scope
//   B points to 0th item in the scope 1 from the top. (note: this is different from before, but it points to the same thing)

//all variables contain 2 indexes:
//- scope (perhaps: 0=global, 1=top, 2=top-1, etc.)
//- index (position in that list)
//The same variable might have different scope values depending on where it's used. (see prev. example))

// big scary problem:

// function whatever(x)
 // return function()
  // print(x)
 // end
// end

//where the heck is X stored?

//solve closures later

//so basically, what should happen is...
//each value that stores a function will keep track of:
//1: the function index (there will be a list of functions, which keeps track of their argument lists and address)
//2: variables

// function whatever(x)
 // function whatever2()
  // var y=3
  // return function(z)
   // print(x,y,z)
  // end
 // end
 // return whatever2()
// end

//ok and better way to call functions:
//<args><number of args><CALL><assignments>
//this way, the assignments are done within the function
//among other things
//don't question this, please.

//anyway
//the returned function in the middle
//during parsing, it is added to the function list just like any other function.
//however, because it is a returned function...
//during runtime its local variables must be captured.
//a list of local variables is stored after the function or something
//and during parsing, each variable which isn't specifically defined in that function is given a new index.
//so rather than x,y,z being in all different scopes, x and y are marked as closure variables, and will be accessed from the value list stored inside the function value.
//so x might be (scope: -1, index: 0) etc.
//whatever() will return a value like:
//{
//	.type = tFunction
//	.function = 2
//	.vars

//check worry about this later