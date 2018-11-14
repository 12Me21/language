#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <setjmp.h>
#include <string.h>
#include "alloc_init.h"
#include <math.h>

jmp_buf err_ret;

typedef uint32_t Address; //location in the bytecode
typedef uint32_t uint;

enum Type {
	tNumber,
	tString,
	tTable,
	tArray,
	tFunction,
	tBoolean,
	tNone,
	tNArgs, //there could be a "list" type perhaps. implemented as <items ...> <# of items>
};

char * type_name[] = { "Number", "String", "Table", "Array", "Function", "Boolean", "None" };

// Multiple things may point to the same String or Table
struct String {
	char * pointer;
	uint32_t length;
	//int references;
};

struct Variable;

struct Array {
	struct Variable * pointer;
	uint32_t length;
};

// A value or variable
// Variables just contain the extra "constraint_expression" field
// Multiple things should not point to the same Value or Variable. EVER. ?
struct Value {
	enum Type type;
	union {
		double number;
		struct String * string;
		struct Table * table;
		struct Array * array;
		Address function;
		bool boolean;
		int args;
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
	oLess,
	oMod,
	oEquals,
	
	oPrint,
	oTest,
	oAssign,
	oHalt,
	oTable, //table literal.
	//<key><value><key><value>...<oTable(# of items)>
	oIndex,
	oScope,
	oArray,
	oCall,
	oDiscard,
	oReturn,
	oMultiAssign,
	oJump,
	oLogicalOr,
	oLogicalAnd,
	oLength,
	oJumpFalse,
	oJumpTrue,
};

//this takes up a lot of space (at least 40 bytes I think)... perhaps this should just be a Value, with a special [type]
struct Item {
	enum Operator operator;
	union {
		struct Value value;
		Address address; //index in bytecode
		struct { //variable
			unsigned int scope;
			unsigned int index;
		};
		unsigned int length; //generic length
		unsigned int locals; //# of local variables in a scope
	};
	//uint line;
	//uint column;
};

struct Array * allocate_array(int length){
	return ALLOC_INIT(struct Array, {.pointer = malloc(sizeof(struct Variable) * length), .length = length});
}
void assign_variable(struct Variable * variable, struct Value value){
	struct Variable * old_var_ptr = variable;
	variable->value = value;
	variable->value.variable = old_var_ptr;
	//TODO: check constraint
}

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
#define SCOPE_STACK_SIZE 256
#define CALL_STACK_SIZE 256
//#define die longjmp(err_ret, 0)
#define die(...) {printf(__VA_ARGS__); longjmp(err_ret, 0);}
struct Value stack[STACK_SIZE];
uint32_t stack_pointer = 0;


//struct Variable variables[256];
struct Item code[65536];
Address call_stack[256];
uint32_t call_stack_pointer = 0;
struct Variable * scope_stack[256];
unsigned int scope_stack_pointer = 0;

Address pos = 0;
struct Item item;

//calling and scope functions
void make_variable(struct Variable * variable){
	variable->value = (struct Value){.type = tNone};
	variable->value.variable = variable;
}
void push_scope(int locals){
	if(scope_stack_pointer >= SCOPE_STACK_SIZE)
		die("Scope Stack Overflow\n");
	struct Variable * scope = scope_stack[scope_stack_pointer++] = malloc(sizeof(struct Variable) * locals);
	int i;
	for(i=0;i<locals;i++)
		make_variable(&scope[i]);
}
void pop_scope(){
	if(scope_stack_pointer <= 0)
		die("Internal Error: Scope Stack Underflow\n");
	free(scope_stack[--scope_stack_pointer]);
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
	pos = call_stack[--call_stack_pointer]+1;
}

//Main stack functions
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

//check if a Value is truthy
bool truthy(struct Value value){
	if(value.type == tNone)
		return false;
	if(value.type == tBoolean)
		return value.boolean;
	return true;
}

void basic_print(struct Value value){
	switch(value.type){
	case tNumber:
		printf("%g", value.number);
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
	}
}

#include "table.c"

int main(){
	uint i=0;
	//DEF TEST(A,B,C):END:TEST(1,2,3) is compiled to
	//1 2 3 3(because 3 arguments were passed) TEST CALL ... @TEST PUSHSCOPE{3} MULTIASSIGN{3}
	
	//VAR X=0:REPEAT:X=X+1:PRINT X:WHILE X<10
	
	//VAR X=0
	code[i++] = (struct Item){.operator = oScope, .locals = 1};
	code[i++] = (struct Item){.operator = oVariable, .scope = 0, .index = 0};
	code[i++] = (struct Item){.operator = oConstant, .value = {.type = tNumber, .number = 0}};
	code[i++] = (struct Item){.operator = oAssign, .scope = 0, .index = 0};
	//X=X+1
	code[i++] = (struct Item){.operator = oVariable, .scope = 0, .index = 0};
	code[i++] = (struct Item){.operator = oVariable, .scope = 0, .index = 0};
	code[i++] = (struct Item){.operator = oConstant, .value = {.type = tNumber, .number = 1}};
	code[i++] = (struct Item){.operator = oAdd};
	code[i++] = (struct Item){.operator = oAssign};
	
	code[i++] = (struct Item){.operator = oVariable, .scope = 0, .index = 0};
	code[i++] = (struct Item){.operator = oConstant, .value = {.type = tNumber, .number = 15}};
	code[i++] = (struct Item){.operator = oMod};
	code[i++] = (struct Item){.operator = oConstant, .value = {.type = tNumber, .number = 0}};
	code[i++] = (struct Item){.operator = oEquals};
	code[i++] = (struct Item){.operator = oJumpFalse, .address = 17};
	code[i++] = (struct Item){.operator = oConstant, .value = {.type = tString, .string = &(struct String){.pointer = &(char[]){'f','i','z','z','b','u','z','z'}, .length=8}}};
	code[i++] = (struct Item){.operator = oJump, .address = 34};
	
	code[i++] = (struct Item){.operator = oVariable, .scope = 0, .index = 0};
	code[i++] = (struct Item){.operator = oConstant, .value = {.type = tNumber, .number = 3}};
	code[i++] = (struct Item){.operator = oMod};
	code[i++] = (struct Item){.operator = oConstant, .value = {.type = tNumber, .number = 0}};
	code[i++] = (struct Item){.operator = oEquals};
	code[i++] = (struct Item){.operator = oJumpFalse, .address = 25};
	code[i++] = (struct Item){.operator = oConstant, .value = {.type = tString, .string = &(struct String){.pointer = &(char[]){'f','i','z','z'},.length=4}}};
	code[i++] = (struct Item){.operator = oJump, .address = 34};
	
	code[i++] = (struct Item){.operator = oVariable, .scope = 0, .index = 0};
	code[i++] = (struct Item){.operator = oConstant, .value = {.type = tNumber, .number = 5}};
	code[i++] = (struct Item){.operator = oMod};
	code[i++] = (struct Item){.operator = oConstant, .value = {.type = tNumber, .number = 0}};
	code[i++] = (struct Item){.operator = oEquals};	
	code[i++] = (struct Item){.operator = oJumpFalse, .address = 33};
	code[i++] = (struct Item){.operator = oConstant, .value = {.type = tString, .string = &(struct String){.pointer = &(char[]){'b','u','z','z'},.length=4}}};
	code[i++] = (struct Item){.operator = oJump, .address = 34};
	
	//PRINT X
	code[i++] = (struct Item){.operator = oVariable, .scope = 0, .index = 0};
	code[i++] = (struct Item){.operator = oPrint, .length = 1};
	
	code[i++] = (struct Item){.operator = oVariable, .scope = 0, .index = 0};
	code[i++] = (struct Item){.operator = oConstant, .value = {.type = tNumber, .number = 100}};
	code[i++] = (struct Item){.operator = oLess};
	code[i++] = (struct Item){.operator = oJumpTrue, .address = 4};
	
	code[i++] = (struct Item){.operator = oHalt};
	
	if(setjmp(err_ret)){
		printf("Error\n");
		//printf("Error on line %d, column %d\n", item.line, item.column);
		return 1;
	}else{
		while(1){
			//printf("working on item: %d\n",pos);
			item = code[pos++];
			switch(item.operator){
			//Constant
			//Output: <value>
			case oConstant:
				push(item.value);
			//Variable
			//Output: <value>
			break;case oVariable:
				if(item.scope) //local var
					push(scope_stack[scope_stack_pointer-item.scope][item.index].value);
				else //global var
					push(scope_stack[0][item.index].value);
			//Addition
			//Input: <arg1> <arg2>
			//Output: <sum>
			break;case oAdd:;
				struct Value a = pop();
				struct Value b = pop();
				if(a.type == tNumber && b.type == tNumber){
					push((struct Value){.type = tNumber, .number = a.number + b.number});
				}else{
					die("Type mismatch in +\n");
				}
			break;case oLess:;
				b = pop();
				a = pop();
				if(a.type == tNumber && b.type == tNumber){
					push((struct Value){.type = tBoolean, .boolean = a.number < b.number});
				}else{
					die("Type mismatch in <\n");
				}
			break;case oMod:;
				b = pop();
				a = pop();
				if(a.type == tNumber && b.type == tNumber){
					push((struct Value){.type = tNumber, .number = fmod(a.number, b.number)});
				}else{
					die("Type mismatch in %\n");
				}
			break;case oEquals:;
				b = pop();
				a = pop();
				if(a.type == tNumber && b.type == tNumber){
					push((struct Value){.type = tBoolean, .boolean = a.number == b.number});
				}else{
					die("Type mismatch in ==\n");
				}
			//Assignment
			//Input: <variable> <value>
			break;case oAssign:;
				struct Value value = pop();
				struct Value variable = pop();
				assign_variable(variable.variable, value);
			//Print
			//Input: <args ...> <# of args>
			break;case oPrint:;
				for(i = item.length; i>0; i--){
					a = stack_get(i);
					basic_print(a);
					if(i!=1)
						printf("\t");
				}
				stack_discard(item.length);
				printf("\n");
			//End of program
			break;case oHalt:
				goto end;
			//Table literal
			//Input: (<key> <value> ...) <# of values>
			//Output: <table>
			break;case oTable:;
				struct Table * table = ALLOC_INIT(struct Table, {.first=NULL, .last=NULL});
				for(i = 0; i<item.length; i++){
					a = pop();
					b = pop();
					table_declare(table, b, ALLOC_INIT(struct Variable, {.value = a}));					
				}
				push((struct Value){.type = tTable, .table = table});
			// Array literal
			//Input: <values ...> <# of values>
			//Output: <array>
			break;case oArray:;
				struct Array * array = allocate_array(item.length);
				for(i = item.length; i>0; i--){
					array->pointer[i-1].value = pop();
				}
				push((struct Value){.type = tArray, .array = array});
			// Array/Table access
			//Input: <table> <index>
			//Output: <value>
			break;case oIndex:; //table, key
				b = pop(); //key
				a = pop(); //table
				switch(a.type){
				case tTable:
					push(table_lookup(a.table, b));
					break;
				case tArray:
					if(b.type!=tNumber)
						die("Expected number for array index, got %s.\n", type_name[a.type]);
					if(b.number<0 || b.number >= a.array->length)
						die("Array index out of bounds (Got %g, Expected 0 to %d)\n", b.number, a.array->length-1);
					push(a.array->pointer[(uint32_t)b.number].value);
					break;
				default:
					die("Expected Array or Table; got %s.\n", type_name[a.type]);
					// etoyr viyr rttpt zrddshrd !
				}
			//Call function.
			//Input: <function> <args> <# of args> |
			break;case oCall:
				a = stack_get(1);
				if(a.type != tNArgs){
					die("Internal error. Function call failed. AAAAAaAAAAAAAAAAAaaaaaaaaaaaaa\n");
				}
				a = stack_get(a.args+2);
				if(a.type != tFunction)
					die("Tried to call a %s as a function\n", type_name[a.type]);
				call(a.function);
			//Discard value from stack
			//Input: <values ...>
			break;case oDiscard: //used after calling functions where the return value is not used.
				pop();
			//Create variable scope
			break;case oScope:
				push_scope(item.locals);
			//Return from function
			break;case oReturn:
				//garbage collect here
				pop_scope();
				ret();
			//Jump
			break;case oJump:
				pos = item.address;
			//Jump if true
			//Input: <condition>
			break;case oJumpTrue:
				if(truthy(pop()))
					pos = item.address;
			//Jump if false
			//Input: <condition>
			break;case oJumpFalse:
				if(!truthy(pop()))
					pos = item.address;
			//Assign values of function input vars
			//Input: <function> <args ...> <# of args>
			break;case oMultiAssign:;
				struct Variable * scope = scope_stack[scope_stack_pointer-1];
				int args = pop().args; // number of inputs which were passed to the function
				if(args<=item.length) //right number of arguments, or fewer
					for(i=0;i<args;i++)
						assign_variable(scope+i, pop());
				else //too many args
					die("That's too many!\n");
				pop(); //remove function from stack
			//Logical OR operator (with shortcutting)
			//Input: <condition 1>
			//Output: ?<condition 1>
			break;case oLogicalOr:
				if(truthy(stack_get(1)))
					pos = item.address;
				else
					stack_discard(1);
			//Logical AND operator (with shortcutting)
			//Input: <condition 1>
			//Output: ?<condition 1>
			break;case oLogicalAnd:
				if(truthy(stack_get(1)))
					stack_discard(1);
				else
					pos = item.address;
			//Length operator
			//Input: <value>
			//Output: <length>
			break;case oLength:
				a = pop();
				unsigned int length;
				switch(a.type){
					case tArray:
						length = a.array->length;
					break;
					case tString:
						length = a.string->length;
					break;
					case tTable:
						length = table_length(a.table);
					break;
					default:
						die("Length operator expected String, Array, or Table; got %s.\n", type_name[a.type]);
				}
				push((struct Value){.type = tNumber, .number = (double)length});
			break;default:
				die("Unsupported operator\n");
			}
		}
	}
	end:
	printf("stopped\n");
	return 0;
}

// calling a function:
// <function> <args> <# of args> CALL 
//function:
// <pushscope # of local vars> <multiassign # of args>

//1: push args to stack
//2: push # of args to stack
//3: push function pointer to stack
//4: CALL:
//4.1: push current pos to call stack
//4.2: pop function pointer and jump
//5: pushscope creates scope
//6: multiassign pops arguments and assigns them to local vars

//where are constraint expressions stored?

// IF(address) - pop and jump if falsey

// IF A THEN B ENDIF
// A IF(@SKIP)	B @SKIP
// IF A THEN B ELSE C ENDIF
// A IF(@ELSE) B GOTO(@SKIP) @ELSE C @SKIP
// IF A THEN B ELSEIF C THEN D ELSE E ENDIF
// A IF(@ELSE1) B GOTO(@SKIP) @ELSE1 C IF(@ELSE2) D GOTO(@SKIP) @ELSE2 E @SKIP

// WHILE A B WEND
// @LOOP A IF(@SKIP) B GOTO(@LOOP) @SKIP

// REPEAT A UNTIL B
// @LOOP A B IFN(@LOOP)

// FOR A = B TO C : D : NEXT
//

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
//1 2 3 3(because 3 arguments were passed) TEST CALL ... @TEST PUSHSCOPE{3} MULTIASSIGN{3}
//CALL pops a function from the stack and calls it.
//PUSHSCOPE pushes a scope to the scope stack, creating the right number of local variables. in this case it's 3 (because arguments are local vars)
//MULTIASSIGN will perform multiple assignments. first it reads the stack entry before the variable list, which is the number of arguments that were passed.
// it then assigns values to the variables, and discards everything on the stack (up to the first value passed to the function)
// The first variables in the function scope are the function arguments. Multiassign can infer their indexes based on the number of parameters to that function.

//the parser must put the function after the args, somehow.

//pushing to the scope stack:
// this will create lots of new variables
// the tFunction type will have a function address

//when a variable is encountered in the bytecode:
//-- use its indexes to find the Variable in the scope stack.
//-- make a copy of its value. set the variable pointer to point at the Variable in the scope stack. (note: it's important that RETURN and other operations do not preserve that pointer)
//-- push that to the stack.

//recursion idea:
// VAR A
// DEF X
 // VAR B
 // DEF Y
  // VAR C
  // B = 1
 // END
 // Y
// END

// X

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

//worry about this later

//There might be a "symbol" type specifically for properties. ex "table.x" compiles to: [table] [.x] [index] not [table] ["x"] [index] to improve efficiency.

//constraints:
//A = B
//A B =
//1: push value A to the stack
//2: push B
//=
//3: CALL constraint expression (don't pop)
//4: constraint expressions end with a special RETURN call which pops a value, throws an error if false, then does the assignment

//idea: some way to access keys in a table with a number. they are stored in an ordered list, after all. perhaps table:key(x) idk. to make iteration etc. easier

//make IF expressions
//print (
	// if x % 15 == 0 then
		// "fizzbuzz"
	// elseif x % 3 == 0 then
		// "fizz"
	// elseif x % 5 == 0 then
		// "buzz"
	// else
		// x
	// endif
// )
