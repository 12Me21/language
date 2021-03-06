void assign_variable(struct Variable * variable, struct Value value){
	if(!variable)
		die("Tried to set the value of something that isn't a variable\n");
	//Can't set the value of whatever the heck that is\n");
	struct Variable * old_var_ptr = variable;
	variable->value = value;
	variable->value.variable = old_var_ptr; //can't I just use variable instead of oldvarptr?
	//printf("constraint: %d\n",variable->constraint_expression);
}

void type_mismatch_1(struct Value arg, enum Operator operator){
	printf("Type error:\n");
	die(" `%s %s` is not allowed.\n", operator_name[operator], type_name[arg.type])
}

void type_mismatch_2(struct Value arg1, struct Value arg2, enum Operator operator){
	printf("Type error:\n");
	die(" `%s %s %s` is not allowed.\n", type_name[arg1.type], operator_name[operator], type_name[arg2.type])
}

struct Item * code;

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
struct Value pop_no_lists(){
	struct Value a = pop();
	if(a.type == tNArgs)
		die("Illegal List\n");
	return a;
}
struct Value stack_get(int depth){
	if(stack_pointer-depth < 0)
		die("Internal Error: Stack Underflow (in get)\n");
	return stack[stack_pointer-depth];
}
struct Value list_get(uint index){
	if(stack_pointer+index >= ARRAYSIZE(stack))
		die("Internal Error: Stack Overflow (in get)\n");
	return stack[stack_pointer+index];
}
struct Value pop_type(enum Type type){
	struct Value a = pop();
	if(a.type != type)
		die("Wrong type\n");
	return a;
}
void replace(struct Value * value){
	stack[stack_pointer-1] = *value;
}
struct Value arg_get(uint index){
	if(stack_pointer+1+index >= ARRAYSIZE(stack))
		die("Internal Error: Stack Overflow (in get)\n");
	return stack[stack_pointer+1+index];
}

void stack_discard(int amount){
	if(stack_pointer-amount < 0)
		die("Internal Error: Stack Underflow (in discard)\n");
	stack_pointer -= amount;
}
//pop a value, and if it's a list, also discard the items in the list
//which can then be accessed with `list_get(index)`
struct Value pop_l(){
	if(stack_pointer <= 0)
		die("Internal Error: Stack Underflow (in pop_l)\n");
	struct Value a = stack[--stack_pointer];
	if(a.type==tNArgs)
		stack_discard(a.args);
	return a;
}
// struct Value * pop_l(){
	// if(stack_pointer <= 0)
		// die("Internal Error: Stack Underflow (in pop)\n");
	// struct Value * sp = stack + --stack_pointer;
	// if(sp->type==tNArgs)
		// stack_discard(sp->args);
	// return sp;
// }


//calling and scope functions
void make_variable(struct Variable * variable){
	variable->constraint_expression = 0;
	variable->value = (struct Value){.type = tNone};
	variable->value.variable = variable;
}
struct Value allocate_array(int length, struct Array * * new_array){
	*new_array = ALLOC_INIT(struct Array, {.pointer = calloc(sizeof(struct Variable), length), .length = length});//calloc so that constraint is 0 by default!
	struct Value value = (struct Value){.type = tArray, .array = *new_array};
	return value;
}

struct Variable * push_scope(int locals){
	if(scope_stack_pointer >= ARRAYSIZE(scope_stack))
		die("Scope Stack Overflow\n");
	r_scope_length[scope_stack_pointer] = locals;
	struct Variable * scope = scope_stack[scope_stack_pointer++] = malloc(locals * sizeof(struct Variable));
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
		for(i=0;i<inputs;i++)
			assign_variable(scope+i, arg_get(i));
	else //too many args
		die("That's too many!\n");
}
void ret(){
	//todo: delete variable reference of returned value
	if(call_stack_pointer <= 0)
		die("Internal Error: Call Stack Underflow\n");
	pos = call_stack[--call_stack_pointer];
}

// void read_arglist(void(* callback)(struct Value)){
	// struct Value a = pop();
	//write a standard reusable arglist handler
	// if(a.type == tNArgs){
		// uint i;
		// for(i=a.args; i>=1; i--)
			// callback(stack_get(i));
		// stack_discard(a.args);
	// }else
		// callback(a);
// }
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

#include "builtins.c"

int run(struct Item * new_code){
	//start_time = current_timestamp();
	
	code = new_code;
	pos = 0;
	//todo: reset other things
	uint i = 0;
	printf("Starting \n\n");
	
	while(1){ // while 1 ♥
		item = code[pos++];
		//printf("working on item: %d, op %d. stack height: %d\n",pos,item.operator,stack_pointer);
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
			push(*(at_stack[scope_stack_pointer]));
		break;case oSet_At:
			at_stack[scope_stack_pointer] = stack + stack_pointer - 1;
		//Assignment
		//Input: <variable> <value>
		break;case oAssign_Discard:;
			struct Value value = pop();
			struct Value variable = pop();
			if(value.type==tNArgs || variable.type==tNArgs)
				die("unsupported list operation (coming soon)\n");
			
			assign_variable(variable.variable, value);
			
			if(variable.variable->constraint_expression && variable.variable->constraint_expression!=-1){
				push(variable);
				push_scope(0);
				call(variable.variable->constraint_expression);
			}
				//todo: store new value in @
		break;case oConstrain_End:;
			bool valid = truthy(pop());
			if(!valid){
				printf("Can't set `%s` to `", variable_pointer_to_name(pop().variable));
				basic_print(value);
				die("`\n");
			}else{
				pop();
			}
			ret();
		break;case oConstrain:
			variable = pop();
			variable.variable->constraint_expression = item.address;
		//End of program
		break;case oHalt:
			if(stack_pointer)
				die("Internal error: Stack not empty at end of program\n");
			goto end;
		//Table literal
		//Input: (<key> <value> ...) <# of values>
		//Output: <table>
		break;case oTable:;
			struct Table * table = ALLOC_INIT(struct Table, {.first=NULL, .last=NULL});
			for(i = 0; i<item.length; i++){
				struct Value value = pop();
				struct Value key = pop();
				if(value.type==tNArgs || variable.type==tNArgs)
					die("unsupported list operation (coming soon)\n");
				table_declare(table, key, value);
			}
			a = (struct Value){.type = tTable, .table = table};
			push(a);
			record_alloc(&a);
		// Array literal
		//Input: <values ...> <# of values> OR <value>
		//Output: <array>
		break;case oArray:;
			//printf("s %d\n", stack_pointer);
			a = pop_l();
			//printf("s1 %d\n", stack_pointer);
			struct Array * array;
			if(a.type == tNArgs){
				//printf("making array. length=%d\n",a.args);
				b = allocate_array(a.args, &array);
				for(i = 0; i<a.args; i++)
					assign_variable(&(array->pointer[i]), list_get(i));
			}else{
				b = allocate_array(1, &array);
				assign_variable(&(array->pointer[0]), a);
			}
			push(b);
			record_alloc(&b);
			//printf("s2 %d\n", stack_pointer);
		// Array/Table access
		//Input: <table> <index>
		//Output: <value>
		break;case oIndex:; //table, key
			b = pop(); //key
			if(b.type==tNArgs)
				die("Can't use a list as an index\n");
			a = pop(); //table
			switch(a.type){
			case tTable:
				push(table_lookup(a.table, b));
			break;case tArray:
				if(b.type!=tNumber)
					die("Expected number for array index, got %s.\n", type_name[a.type]);
				if(b.number<0 || b.number >= a.array->length)
					die("Array index out of bounds (Got %g, expected 0 to %d)\n", b.number, a.array->length-1);
				push(a.array->pointer[(uint32_t)b.number].value);
			break;case tNArgs:
				if(b.type!=tNumber)
					die("Expected number for list index, got %s.\n", type_name[a.type]);
				if(b.number<0 || b.number >= a.args)
					die("List index out of bounds (Got %g, expected 0 to %d)\n", b.number, a.args-1);
				stack_discard(a.args);
				push(stack_get(-(uint)b.number));
			break;default:
				die("Expected Array or Table; got %s.\n", type_name[a.type]);
				// etoyr viyr rttpt zrddshrd !
			}
		//Call function.
		//Input: <function> <args> <# of args>
		//Output (builtin): <return value>
		//Output (user): 
		//Output (user, after return): <return value>
		break;case oCall:
			a = pop_l();
			uint args;
			if(a.type == tNArgs)
				args = a.args;
			else
				args = 1;
			a = pop(); //get function
			if(a.type != tFunction)
				die("Tried to call a %s as a function\n", type_name[a.type]);
			if(a.builtin){
				(*(a.c_function))(args);
			}else
				call_user_function(a.user_function, args);
		//Discard value from stack
		//Input: <values ...>
		break;case oDiscard:
			a = pop();
			if(a.type == tNArgs){
				printf("discarding list\n");
				stack_discard(a.args);
			}
		//Create variable scope //this is just used once at the start of the prgram
		break;case oInit_Global:
			level_stack[0] = push_scope(item.locals);
			for(i=0;i<ARRAYSIZE(builtins);i++){
				level_stack[0][i] = builtins[i];
				level_stack[0][i].value.variable = &level_stack[0][i];
			}
				//(struct Variable){.value = {.type = tFunction, .builtin = true, .c_function = builtins[i]}};
			
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
		//table.property
		//property id is stored in operator.
		//break;case oProperty:
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
//lazy solution is to surround all functions with GOTO @SKIP ... @SKIP

//important:
//make sure that values don't contain direct pointers to strings/tables since they might need to be re-allocated.
//don't get rid of the Variable struct since it's needed for keeping track of constraints etc.

//Jobs for the parser:
// - generate a list of all properties (table.property) independantly from other words\

//Hoisting functions
//This is nessesary to preserve the sanity of programmers.
//not sure the best way to handle this.

//when a variable is encountered in the bytecode:
//-- use its indexes to find the Variable in the scope stack.
//-- make a copy of its value. set the variable pointer to point at the Variable in the scope stack. (note: it's important that RETURN and other operations do not preserve that pointer)
//-- push that to the stack.

//solve closures later

//There might be a "symbol" type specifically for properties. ex "table.x" compiles to: [table] [.x] [index] not [table] ["x"] [index] to improve efficiency.

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

//idea: function/whatever that returns all the arguments to a function
//print = def()
// ?args()
//end

//convert the switch statement in run() into a function that takes the operator and stack/pointer
//so it can be used here as well as for precomputing things during parsing
//ensure that, for example, something like `Array.sum` is precomputed so that it doesn't need to do a table access every time.
//also remember to actually implement tables....

//lists:
//lists aren't "real" like all the other types
//they're just multiple values stored in the stack, followed by an item storing the length of the list
//length 0 list: <# of args>
//length 1 list: (1 regular value)
//length 2+ list: <# of args>
//a "real" length 1 list can be created by doing ((),value) but this isn't nessesary. Anything that accepts a list will also allow a single value, which is treated as a length-1 list.