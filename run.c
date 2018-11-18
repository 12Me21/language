
jmp_buf err_ret;

typedef uint32_t Address; //location in the bytecode
typedef uint32_t uint;

enum Type {
	tNone,
	tNumber,
	tString,
	tTable,
	tArray,
	tFunction,
	tBoolean,
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
		struct {
			union {
				Address user_function;
				void (*c_function)(uint);
			};
			bool builtin;
		};
		bool boolean;
		int args;
	};
	struct Variable * variable;
};

struct Variable {
	struct Value value;
	Address constraint_expression;
};

struct Value * at_value;

// every distinct operator
// subtraction and negation are different operators
// functions are called using the "call function" operator
// which takes as input the function pointer and number of arguments
// <function ptr> <args ...> <# of args> CALL
enum Operator {
	oInvalid,//0
	
	oConstant,
	oVariable,
	
	//operators
	oBitwise_Not,
	oBitwise_Xor,
	oNot_Equal,
	oNot,
	oMod,
	oExponent,
	oBitwise_And,
	oMultiply,//10
	oNegative,
	oSubtract,
	oAdd,
	oEqual,
	oAssign,
	oBitwise_Or,
	oFloor_Divide,
	oLess_Or_Equal,
	oLeft_Shift,
	oLess,
	oGreater_Or_Equal,
	oRight_Shift,
	oGreater,
	oDivide,
	oPrint1,
	//More operators
	oArray,//26
	oIndex,
	oCall,
	oDiscard, //29
	
	
	oPrint,
	oTest,
	oHalt,
	oTable, //table literal.
	//<key><value><key><value>...<oTable(# of items)>
	
	oScope,
	
	oReturn,
	oJump,
	oLogicalOr,
	oLogicalAnd,
	oLength,
	oJumpFalse,
	oJumpTrue,
	oFunction_Info,
	
	oReturn_None,
	
	oGroup_Start, //parsing only
	
	oAssign_Discard,
	oConstrain,
	oConstrain_End,
	oAt,
};

//this takes up a lot of space (at least 40 bytes I think)... perhaps this should just be <less awful>
struct Item {
	enum Operator operator;
	union {
		struct Value value;
		Address address; //index in bytecode
		 //used by variable and functioninfo
		struct {
			uint level; //level of var or func
			union {
				uint index; //index of var
				struct {
					uint locals; //# of local variables in a scope
					uint args; //# of inputs to a function
				};
			};
		}; //simplify this ^
		uint length; //generic length
	};
};

//#define die longjmp(err_ret, 0)
#define die(...) {printf(__VA_ARGS__); longjmp(err_ret, 2);}
struct Value stack[256];
uint32_t stack_pointer = 0;

void assign_variable(struct Variable * variable, struct Value value){
	//printf("var assign");
	if(!variable)
		die("Tried to set the value of something that isn't a variable\n");
	//Can't set the value of whatever the heck that is\n");
	struct Variable * old_var_ptr = variable;
	variable->value = value;
	variable->value.variable = old_var_ptr; //can't I just use variable instead of oldvarptr?
	//printf("constraint: %d\n",variable->constraint_expression);
}

struct Item * code;
Address call_stack[255]; //this should be bigger
uint call_stack_pointer = 0;
struct Variable * scope_stack[256];
uint scope_stack_pointer = 0;
struct Variable * level_stack[256]; //this doesn't need to be bigger

Address pos = 0;
struct Item item;

//Main stack functions
void push(struct Value value){
	if(stack_pointer >= ARRAYSIZE(stack))
		die("Stack Overflow\n");
	stack[stack_pointer++] = value;
}
struct Value pop(){
	if(stack_pointer <= 0)
		die("Internal Error: Stack Underflow (in pop)\n");
	return stack[--stack_pointer];
}
struct Value stack_get(int depth){
	if(stack_pointer-depth < 0)
		die("Internal Error: Stack Underflow (in get)\n");
	return stack[stack_pointer-depth];
}
void stack_discard(int amount){
	if(stack_pointer-amount < 0)
		die("Internal Error: Stack Underflow (in discard)\n");
	stack_pointer -= amount;
}

//calling and scope functions
void make_variable(struct Variable * variable){
	variable->value = (struct Value){.type = tNone};
	variable->value.variable = variable;
}
struct Array * allocate_array(int length){
	return ALLOC_INIT(struct Array, {.pointer = calloc(sizeof(struct Variable), length), .length = length});
}

struct Variable * push_scope(int locals){
	if(scope_stack_pointer >= ARRAYSIZE(scope_stack))
		die("Scope Stack Overflow\n");
	struct Variable * scope = scope_stack[scope_stack_pointer++] = calloc(locals, sizeof(struct Variable)); //calloc so that constraint is 0 by default!
	int i;
	for(i=0;i<locals;i++)
		make_variable(&scope[i]);
	return scope;
}
void pop_scope(){
	if(scope_stack_pointer <= 0)
		die("Internal Error: Scope Stack Underflow\n");
	//garbage collect here
	free(scope_stack[--scope_stack_pointer]);
}

void call(Address address){
	if(call_stack_pointer >= ARRAYSIZE(call_stack))
		die("Call stack overflow. (Too many functions were called without returning)\n");
	call_stack[call_stack_pointer++] = pos;
	pos = address;
}
//Call a user defined function
//stack in: | <function (unused)> <args> |
//stack out: | |
//modifies level_stack, pushed to call_stack and scope_stack, jumps to address.
void call_user_function(Address address, uint inputs){
	call(address);
	pos++;
	//check function info
	if(code[address].operator != oFunction_Info)
		die("Internal error: Could not call function because function info is missing\n");
	//create scope
	struct Variable * scope = push_scope(code[address].locals);
	level_stack[code[address].level] = scope;
	//assign values to input variables
	uint i;
	if(inputs <= code[address].args) //right number of arguments, or fewer
		for(i=inputs-1;i!=-1;i--)
			assign_variable(&scope[i], pop());
	else //too many args
		die("That's too many!\n");
	//jump
	pop(); //remove function itself
}
void ret(){
	//todo: delete variable reference of returned value
	if(call_stack_pointer <= 0)
		die("Internal Error: Call Stack Underflow\n");
	pos = call_stack[--call_stack_pointer];
}

//check if a Value is truthy
bool truthy(struct Value value){
	if(value.type == tNone)
		return false;
	if(value.type == tBoolean)
		return value.boolean;
	if(value.type == tNArgs)
		die("internal error\n");
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
			return a.table == b.table;
		case tString:
			return a.string->length == b.string->length && !memcmp(a.string->pointer, b.string->pointer, a.string->length * sizeof(char));
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


//double current_timestamp(){
	//struct timeval te; 
	//gettimeofday(&te, NULL);
	//return te.tv_sec + te.tv_usec/1000000.0;
//}

double start_time;

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
	break;case tNArgs:
		die("Internal error: tried to print # of args what\n");
	break;default:
		die("can't print aaaa\n");
	}
}

#include "table.c"
#include "builtins.c"

int run(struct Item * new_code){
	//start_time = current_timestamp();
	
	code = new_code;
	pos = 0;
	//todo: reset other things
	uint i = 0;
	printf("Starting \n\n");
	
	while(1){
		item = code[pos++];
		//printf("working on item: %d, op %d\n",pos,item.operator);
		switch(item.operator){
		//Constant
		//Output: <value>
		case oConstant:
			push(item.value);
		//Variable
		//Output: <value>
		break;case oVariable:
			//printf("variable. level %d index %d\n", item.level, item.index);
			push(level_stack[item.level][item.index].value);
		#include "operator.c"
		break;case oAt:
			push(*at_value);
		//Assignment
		//Input: <variable> <value>
		break;case oAssign:;
			struct Value value = pop();
			struct Value variable = pop();
			if(variable.variable->constraint_expression){
				//printf("CCC\n");
				push(variable);
				push(value); //replace with resurrect
				at_value = stack + stack_pointer - 1;
				call(variable.variable->constraint_expression);
				
				//todo: store new value somewhere accessible?
			}else{
				assign_variable(variable.variable, value);
				push(value);
			}
		break;case oAssign_Discard:
			value = pop();
			variable = pop();
			assign_variable(variable.variable, value);
		break;case oConstrain:
			variable = pop();
			variable.variable->constraint_expression = item.address;
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
			for(i = item.length-1; i!=-1; i--)
				assign_variable(&(array->pointer[i]), pop());
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
		break;case oConstrain_End:;
			bool valid = truthy(pop());
			value = pop();
			assign_variable(pop().variable, value);
			if(!valid){
				printf("Validation failed:\n");
				printf(" Variable: <idk>\n");
				printf(" Value: ");
				basic_print(value);
				die("\n");
			}
			ret();
			push(value);
		//Call function.
		//Input: <function> <args> <# of args>
		//Output (builtin): <return value>
		//Output (user): 
		//Output (user, after return): <return value>
		break;case oCall:
			a = pop();
			if(a.type != tNArgs)
				die("Internal error. Function call failed. AAAAAaAAAAAAAAAAAaaaaaaaaaaaaa\n");
			uint args = a.args;
			
			a = stack_get(args+1); //get "function" value
			if(a.type != tFunction)
				die("Tried to call a %s as a function\n", type_name[a.type]);
			if(a.builtin)
				//c functions should pop the inputs + 1 extra item from the stack.
				//and push the return value.
				(*(a.c_function))(args);
			else
				call_user_function(a.user_function, args);
		//Discard value from stack
		//Input: <values ...>
		break;case oDiscard: //used after calling functions where the return value is not used.
			pop();
		//Create variable scope
		break;case oScope:
			level_stack[0] = push_scope(item.locals);
			//assign_variable(level_stack[0]+1, (struct Value){.type = tFunction, .builtin = true, .c_function = &seconds});
		//Return from function
		break;case oReturn:
			//todo: delete variable reference of returned value
			pop_scope();
			ret();
		break;case oReturn_None:
			pop_scope();
			ret();
			push((struct Value){.type = tNone});
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
		//this should never run
		break;case oFunction_Info:
			die("Internal error: Illegal function entry\n");
		break;default:
			die("Unsupported operator\n");
		}
	}
	end:
	printf("\nstopped\n");
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

//bytecode structure:
//<main><functions>
//after bytecode is generated, function addresses must be inserted.

// DEF A
 // DEF B
 // END
// END
//<A> <B>
//how to insert B when A hasn't even finished?

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
//
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

//X=@+1 compiles to
//X <push to @ stack> @ 1 + =
//= discards from @ stack

//now I see why += etc are so common
//not only are they efficient to convert to machine code
//but x+=1 can just be compiled simply to X <dup> 1 +