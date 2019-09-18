#include "types.h"
#include "alloc_init.c"
jmp_buf err_ret;

char * type_name[] = { "None", "Number", "String", "Table", "Array", "Function", "Boolean", "(List)" };

char * operator_name[] = {
	"Invalid Operator", "Constant", "Variable", "~", "~", "!=", "!", "%", "^", "&", "*", "-", "-", "+", "==", "=", "|",
	"\\", "<=", "<<", "<", ">=", ">>", ">", "/", "Array Literal", "Index", "Call", "Discard", "?", "Halt",
	"Table Literal", "Global Variables", "Jump", "or", "and", "Length", "Jump if false", "Jump if true",
	"Function Info", "Return None", "Group Start", "Assign Discard", "Constrain", "Constrain End", "At", ",",	
};

int compare_vars(struct Variable a, struct Variable b){
	return compare(a.value, b.value);
}

extern uint32_t stack_pointer;

struct Variable * scope_stack[256];
struct Value * at_stack[256]; //this shares the scope stack pointer but maybe it should use callstack instead?
uint scope_stack_pointer = 0;
uint r_scope_length[ARRAYSIZE(scope_stack)];

Address call_stack[255]; //this should be bigger maybe
uint call_stack_pointer = 0;

struct Variable * level_stack[256]; //this doesn't need to be bigger

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
		at_stack[scope_stack_pointer] = get_stack_top_address();
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
			break;case oInit_Global:;
		extern struct Variable builtins[2];
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
	break;case 2:;
		struct Line line = get_line(pos);
		printf("Error while running line %d\n", line.line);
		uint i;
		for(i = call_stack_pointer-1; i!=-1; i--){
			line = get_line(call_stack[i]);
			printf("Also check line %d\n", line.line);
		}
	}
	return 1;
}
