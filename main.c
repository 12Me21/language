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

#include "types.h"

char * type_name[] = { "None", "Number", "String", "Table", "Array", "Function", "Boolean", "(List)" };

//higher number = first
int priority[] = {
	oInvalid,
	
	oConstant,
	oVariable,
	
	// "Real" operators
	99,
	4,
	7,
	99,
	11,
	12,
	6,
	11,
	99,
	10,
	10,
	7,
	5,
	11,
	8,
	9,
	8,
	8,
	9,
	8,
	11,
	// More operators
	oArray,
	oIndex,
	oCall,
	oDiscard,
	
	-66, // print
	oHalt,
	oTable, //table literal.
	//<key><value><key><value>...<oTable(# of items)>
	
	oInit_Global,
	
	oReturn,
	oJump,
	oLogicalOr,
	oLogicalAnd,
	oLength,
	oJumpFalse,
	oJumpTrue,
	oFunction_Info,
	
	oReturn_None,
	
	-99, //parsing only
	//...
	oAssign_Discard,
	oConstrain,
	oConstrain_End,
	oAt,
	-2, //comma
	0, //set_@
	100, //property access
	-4,//in
};

char * operator_name[] = {
	"Invalid Operator", "Constant", "Variable", "~", "~", "!=", "!", "%", "^", "&", "*", "-", "-", "+", "==", "=", "|",
	"\\", "<=", "<<", "<", ">=", ">>", ">", "/", "Array Literal", "Index", "Call", "Discard", "?", "Halt",
	"Table Literal", "Global Variables", "Jump", "or", "and", "Length", "Jump if false", "Jump if true",
	"Function Info", "Return None", "Group Start", "Assign Discard", "Constrain", "Constrain End", "At", ",",	
};

char * token_name[] = {
	"(Value)",
	"`.`",
	"Operator",
	"Operator",
	"Operator",
	"`(`",
	"`)`",
	"`[`",
	"`]`",
	"`{`",
	"`}`",
	"`;`",
	"`:`",
	"`,`",
	"Keyword",
	"(Variable)",
	"(End of program)",
	"@",
	"(Line break)",
	"=",
	"or",
	"(Unclosed string)",
	"(Invalid character)",
};

char * keywords[] = {
	"if", "then", "else", "elseif", "endif",
	"for", "next",
	"while", "wend",
	"repeat", "until",
	"var",
	"def", "return", "end",
	//...
};

//Get readable name of token
char * token_name_2(struct Token token){
	//operators
	if(token.type == tkOperator_1)
		return operator_name[token.operator_1];
	if(token.type == tkOperator_2)
		return operator_name[token.operator_2];
	if(token.type == tkOperator_12)
		return operator_name[token.operator_2]; //_1 and _2 should be the same here
	//keywords
	if(token.type == tkKeyword)
		return keywords[token.keyword];
	//other
	return token_name[token.type];
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Functions
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


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

int compare_vars(struct Variable a, struct Variable b){
	return compare(a.value, b.value);
}

char to_string_output[256];

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

struct Value stack[256];
uint32_t stack_pointer = 0;

struct Variable * scope_stack[256];
struct Value * at_stack[256]; //this shares the scope stack pointer but maybe it should use callstack instead?
uint scope_stack_pointer = 0;
uint r_scope_length[ARRAYSIZE(scope_stack)];

Address call_stack[255]; //this should be bigger maybe
uint call_stack_pointer = 0;

struct Variable * level_stack[256]; //this doesn't need to be bigger
// input: stream or string
// output: tokens

bool read_next;
int c;
FILE * stream;
char * string_input = NULL;
struct Line line;

//next character
void next(){
	if(read_next){
		line.column++;
		if(c=='\n'){
			line.column = 1;
			line.line++;
		}
		if(string_input){
			c = *string_input++;
			if(!c)
				c = EOF;
		}else
			c = getc(stream);
	}else
		read_next = true;
	//printf("char: %c\n",c);
}

//typedef uint Word;

// void f_floor(uint args);
// void f_ceil(uint args);
void f_array_fill(uint args);
//array:count()
//list:count()
//string:count()
//all return a table where the key is the item and value is the count

//function to convert table -> array (throw error if non-numbers are found?)

// def(array)
	// var i=0
	// for number, amount in array:count()
		// array:fill(i,amount, number)
		// i = @ + amount
	// next
// end

//put builtin variables here
struct Variable builtins[] = {
	//{.value = {.type = tTable, .table = &f_floor}, .constraint_expression = -1},
	//{.value = {.type = tFunction, .builtin = true, .c_function = &f_floor}, .constraint_expression = -1},
	//{.value = {.type = tFunction, .builtin = true, .c_function = &f_ceil}, .constraint_expression = -1},
	{.value = {.type = tNumber, .number = 3.14159265358979323846}, .constraint_expression = -1},
	{.value = {.type = tFunction, .builtin = true, .c_function = &f_array_fill}, .constraint_expression = -1},
};
//should pi be a global variable?
//sure it might seem better to put it inside Math or Number, but think about it
//when are you going to use a variable called `pi` which doesn't have a value of 3.14159...?
//same with floor, ceil, etc.
//...
//maybe builtins should insert their value directly during parsing...
//to make precomputing easier?
char * name_table[63356] = {
	//"Number",
	//"floor",
	//"ceil",
	"pi",
	"fill",
	//"random", //random between 2 values. also add random_choice or something that picks from an array/list/table/whatever
};
//if lists could be evaluated lazily somehow... that would be pretty cool.
uint name_table_pointer;

void init(){
	//printf("ASB %d\n",ARRAYSIZE(builtins));
	name_table_pointer = ARRAYSIZE(builtins);
	line.line = 1;
	line.column = 0;
	read_next = true;
	next();
}

void init_stream(FILE * new_stream){
	stream = new_stream;
	init();
}

void init_string(char * string){
	string_input = string;
	init();
}

void process_word(char * word, struct Token * token){
	//maybe it would be better if each keyword was a separate token...
	//check special reserved words:
	if(!strcmp(word, "true")){
		token->type = tkValue;
		token->value = (struct Value){.type = tBoolean, .boolean = true};
	}else if(!strcmp(word, "false")){
		token->type = tkValue;
		token->value = (struct Value){.type = tBoolean, .boolean = false};
	}else if(!strcmp(word, "none")){
		token->type = tkValue;
		token->value = (struct Value){.type = tNone};
	}else if(!strcmp(word, "or")){
		token->type = tkOr;
	}else if(!strcmp(word, "in")){
		token->type = tkOperator_2;
		token->operator_2 = oIn;
	}else{
		uint i;
		//check keywords list
		for(i=0;i<ARRAYSIZE(keywords);i++)
			if(!strcmp(keywords[i],word)){
				token->type = tkKeyword;
				token->keyword = i;
				goto ret;
			}
		//check variable names list
		for(i=0;i<name_table_pointer;i++){
			//printf("vn comp: %s, %s\n", name_table[i],word);
			if(!strcmp(name_table[i],word)){
				//printf("var name match: %s num %d\n",word,i);
				token->type = tkWord;
				token->word = i;
				goto ret;
			}
		}
		//new word
		//printf("new var name: %s\n",word);
		name_table[name_table_pointer] = strdup(word);
		token->type = tkWord;
		token->word = name_table_pointer++;
	}
	ret:;
}

char string_temp[1000];

bool is_digit(char c){
	return c>='0' && c<='9';
}

bool is_alpha(char c){
	return (c>='A' && c<='Z') || (c>='a' && c<='z');
}

struct Line real_line;

struct Token next_token(){
	while(c == ' ' || c == '\t')
		next();
	
	real_line = line;
	struct Token ret = {.position_in_file = real_line};
	
	switch(c){
		case EOF:
			//line_position_in_output[line.line+1] = (uint)-1;
			ret.type = tkEof;
		// Number
		break;case '0':
		case '1':
		case '2':
		case '3':
		case '4':
		case '5':
		case '6':
		case '7':
		case '8':
		case '9':;
			double number = c - '0';
			next();
			while(is_digit(c)){
				number *= 10;
				number += c - '0';
				//number = ? * 10 + (c-'0')
				next();
			}
			// decimal point, and next char is a digit:
			if(c == '.'){
				next();
				if(is_digit(c)){
					//next();
					double divider = 10;
					do{
						number += (c - '0') / divider;
						divider *= 10;
						next();
					}while(is_digit(c));
				}else
					read_next = false;
			}
			ret.type = tkValue;
			ret.value = (struct Value){.type = tNumber, .number = number};
		// . operator or number starting with .
		break;case '.':
			next();
			if(is_digit(c)){
				double number= c - '0';
				next();
				while(is_digit(c)){
					number *= 10;
					number += c - '0';
					next();
				}
				ret.type = tkValue;
				ret.value = (struct Value){.type = tNumber, .number = number};
			}else{
				ret.type = tkDot;
			}
		// String
		break;case '"':
			next();
			int length=0;
			while(c != '"'){
				if(c == EOF){
					ret.type = tkUnclosedString;
					goto exit;
				}else if(c == '\\'){
					next();
					switch(c){
					case '\\':case '"':
					break;case 'n':
						c='\n';
					break;case 'r':
						c='\r';
					break;case 'f':
						c='\f';
					break;case '0':
						c='\0'; //just allow numbers eventually
					break;case 'b':
						c='\b';
					break;case 'a':
						c='\a';
					break;default:;
						//none
					}
				}
				string_temp[length++] = c;
				next();
			}
			//next(); //IDK why I need[ed] 2 nexts here
			next();
			ret.type = tkValue;
			ret.value = (struct Value){.type = tString, .string = 
				ALLOC_INIT(struct String, {.pointer = memdup(string_temp, length * sizeof(char)), .length = length})
			};
		break;case '~':
			next();
			ret.type = tkOperator_12;
			ret.operator_1 = oBitwise_Not;
			ret.operator_2 = oBitwise_Xor;
		break;case '!':
			next();
			if(c=='='){
				next();
				ret.type=tkOperator_2;
				ret.operator_2 = oNot_Equal;
			}else{
				ret.type = tkOperator_1;
				ret.operator_1 = oNot;
			}
		break;case '%':
			next();
			ret.type = tkOperator_2;
			ret.operator_2 = oMod;
		break;case '^':
			next();
			ret.type = tkOperator_2;
			ret.operator_2 = oExponent;
		break;case '&':
			next();
			ret.type = tkOperator_2;
			ret.operator_2 = oBitwise_And;
		break;case '*':
			next();
			ret.type = tkOperator_2;
			ret.operator_2 = oMultiply;
		break;case '(':
			next();
			ret.type = tkLeft_Paren;
		break;case ')':
			next();
			ret.type = tkRight_Paren;
		break;case '-':
			next();
			ret.type = tkOperator_12;
			ret.operator_1 = oNegative;
			ret.operator_2 = oSubtract;
		break;case '+':
			next();
			ret.type = tkOperator_2;
			ret.operator_2 = oAdd;
		break;case '?':
			next();
			ret.type = tkOperator_1;
			ret.operator_1 = oPrint;
		break;case '=':
			next();
			if(c=='='){
				next();
				ret.type = tkOperator_2;
				ret.operator_2 = oEqual;
			}else{
				ret.type = tkAssign;
			}
		break;case '[':
			next();
			ret.type = tkLeft_Bracket;
		break;case ']':
			next();
			ret.type = tkRight_Bracket;
		break;case '{':
			next();
			ret.type = tkLeft_Brace;
		break;case '}':
			next();
			ret.type = tkRight_Brace;
		break;case '|':
			next();
			ret.type = tkOperator_2;
			ret.operator_2 = oBitwise_Or;
		break;case '\\':
			next();
			ret.type = tkOperator_2;
			ret.operator_2 = oFloor_Divide;
		break;case ';':
			next();
			ret.type = tkSemicolon;
		break;case '\n':
			next();
			ret.type = tkLine_Break;
		break;case ':':
			next();
			ret.type = tkColon;
		break;case '@':
			next();
			ret.type = tkAt;
		// Comment
		break;case '\'':
			do {
				next();
			} while(c!='\n' && c!=EOF);
			ret = next_token();
		break;case '<':
			next();
			if(c=='='){
				next();
				ret.type = tkOperator_2;
				ret.operator_2 = oLess_Or_Equal;
			}else if(c=='<'){
				next();
				ret.type = tkOperator_2;
				ret.operator_2 = oLeft_Shift;
			}else{
				ret.type = tkOperator_2;
				ret.operator_2 = oLess;
			}
		break;case '>':
			next();
			if(c=='='){
				next();
				ret.type = tkOperator_2;
				ret.operator_2 = oGreater_Or_Equal;
			}else if(c=='>'){
				next();
				ret.type = tkOperator_2;
				ret.operator_2 = oRight_Shift;
			}else{
				ret.type = tkOperator_2;
				ret.operator_2 = oGreater;
			}
		break;case ',':
			next();
			ret.type = tkComma;
			ret.operator_2 = oComma;
		break;case '/':
			next();
			ret.type = tkOperator_2;
			ret.operator_2 = oDivide;
		break;default:
			if(is_alpha(c) || c == '_'){
				int length=0;
				do{
					string_temp[length++] = c;
					next();
				}while(is_alpha(c) || is_digit(c) || c == '_');
				string_temp[length] = '\0';
				process_word(string_temp, &ret);
			}else{
				ret.type=tkInvalidChar;
			}
	}
	exit:
	return ret;
}
struct Token token;
bool read_next;

void parse_error_1(){
	printf("Error while parsing line %d, character %d\n ", real_line.line, real_line.column);
}

void parse_error_2(){
	longjmp(err_ret, 1);
}

#define parse_error(...) (parse_error_1(), printf(__VA_ARGS__), parse_error_2())

uint output_length = 0;

struct Line address_position_in_code[65536] = {};

struct Line get_line(Address address){
	while(address_position_in_code[address].line==0)
		address++;
	return address_position_in_code[address];
}

void next_t(){
	if(read_next){
		token = next_token();
		//printf("read token %d %d, %d\n",token.position_in_file.line,token.position_in_file.column,output_length);
		address_position_in_code[output_length] = token.position_in_file;
	}else
		read_next=true;
	//printf("token t %d\n",token.type);
}

//try to read a specific token type
//returns true if successful.
bool read_token(enum Token_Type wanted_type){
	next_t();
	//printf("rt: %d %d\n",token.type,wanted_type);
	if(token.type==wanted_type){
		read_next=true;
		return true;
	}
	read_next=false;
	return false;
}

// basically, (
struct Item group_start = {.operator = oGroup_Start}; //set priority to -2

struct Item * output_stack;
//output_length is defined in token.c for some reason
struct Item * output(struct Item item){
	//todo:
	//if item is operator:
	//1: check if operator is a constant operator (one that always has the same result for the same inputs)
	//2: check if all of the inputs are constants
	//3: evaluate
	//optimization!!!
	output_stack[output_length] = item;
	return &(output_stack[output_length++]);
}

// Operator stack
// This stores operators temporarily during parsing (using shunting yard algorithm)
struct Item op_stack[256];
uint op_length = 0;
void push_op(struct Item item){
	if(op_length >= ARRAYSIZE(op_stack))
		parse_error("Operator Stack Overflow\n");
	op_stack[op_length++] = item;
}
struct Item pop_op(){
	if(op_length <= 0)
		parse_error("Internal Error: Stack Underflow\n");
	return op_stack[--op_length];
}
//Return the previously popped value to the stack
void resurrect_op(){
	if(op_length >= ARRAYSIZE(op_stack))
		parse_error("Operator Stack Overflow\n");
	op_length++;
}

//Scope (level) stack
//This keeps track of local variables during parsing.
uint * p_level_stack[256]; //256 = # of levels
uint scope_length = 0;
uint locals_length[256]; //256 = # of local variables per scope

//to find variable name from Variable:
//1: get pointer to variable
//2: search for pointer in scope stack
//3: search level stack for that entry in the scope stack
//(now you have the index and level
//(if that wasn't found, that means the variable is in an array or table or something...)
//4: use the call stack to find the address of the function which that variable is from (this is assuming the call stack and scope stack are aligned, which they are because the language uses function scoped variables)
//5: now you have the function address and var index
//6: during parsing, whenever entering a scope, push the function address to an array, along with the names of all that function's local variables
//7: now, search for the address in that list, and look up the index in the list of vars
//8: now you have the variable name ID which you can look up using the name table!

//find Variable address from name:
//1: look up name in name table
//3: each scope (function) has a list of the names of all its local variables (generated during parsing)
//4: using the level/call stacks, search for the name
// - look at the top item in the level stack
// - use that to look up the right call stack entry
// - use the call stack entry to get the function address
// - look up the address in function_addresses
// - look for the name id in the corresponding function_locals list
// - repeat for each item in the level stack.

// void 

Address function_addresses[65536];
uint * function_locals[65536];
uint function_addresses_length = 0;

char * variable_pointer_to_name(struct Variable * variable){
	//printf("AAAAAAAA\n");
	uint i;
	//printf("- %p\n",variable);
	for(i=scope_stack_pointer-1;i!=-1;i--){
		uint j;
		
		for(j=0;j<r_scope_length[i];j++){
			//printf("scope %i: %p\n",i,&scope_stack[i][j]);
			if(&scope_stack[i][j] == variable){
				Address address = i ? call_stack[i-1] : 0;
				//printf("- %d\n",address);
				for(i=0;i<function_addresses_length;i++){
					//printf("%d\n",function_addresses[i]);
					if(function_addresses[i]==address){
						//printf("OK %d",function_locals[i][j]);
						return name_table[function_locals[i][j]];
					}
				}
				die("Internal error: failed to get variable name");
			}
		}
	}
	return "<unknown variable>";
	die("failed 1\n");
	
	
	// uint i;
	// for(i=0;i<ARRAYSIZE(function_addresses);i++)
		// if(function_addresses[i] == address)
			// return function_locals[i][index];
	// die("Internal error: failure when looking up variable name");
}

void p_push_scope(){
	if(scope_length >= ARRAYSIZE(p_level_stack))
		parse_error("Level stack overflow. (Too many nested functions)\n");
	p_level_stack[scope_length] = malloc(256 * sizeof(uint));
	locals_length[scope_length++] = 0;
}

void discard_scope(uint start_pos){
	if(scope_length <= 0)
		parse_error("Internal Error: Scope Stack Underflow\n");
	--scope_length;
	function_addresses[function_addresses_length] = start_pos;
	function_locals[function_addresses_length++] = realloc(p_level_stack[scope_length], locals_length[scope_length] * sizeof(uint));
}

// char * property_names[65536];
// uint property_names_length = 0;
// uint register_property_name(uint word){
	// uint i;
	// for(i=0;i<property_names_length;i++)
		// if(property_names[i]==word)
			// return i;
	// if(property_names_length >= ARRAYSIZE(property_names))
		// parse_error("Too many property names");
	// property_names[property_names_length] = word;
	// return property_names_length++;
// }

//note: the thing that the parser calls "scope" is called "level" in run.c

struct Item declare_variable(uint word){
	//printf("declare var: %d at level %d\n", word, scope_length);
	if(locals_length[scope_length-1] >= 256)
		parse_error("Local Variable Stack Overflow\n");
	p_level_stack[scope_length-1][locals_length[scope_length-1]] = word;
	return (struct Item){.operator = oVariable, .level = scope_length-1, .index = locals_length[scope_length-1]++};
}

struct Item make_var_item(uint word){
	uint i;
	//printf("found var: %d\n", word);
	//This will break if ssp1 is 0. Make sure the global scope is created at the beginning!
	//search for variable in existing scopes, starting from the current one and working down towards global.
	//printf("scope length %d\n",scope_length);
	for(i = scope_length; i>0; i--){
		uint j;
		for(j = 0; j < locals_length[i-1]; j++){
			if(word == p_level_stack[i-1][j]){
				return (struct Item){.operator = oVariable, .level = i-1, .index = j};
			}
		}
	}
	//new variable that hasn't been used before:
	parse_error("Undefined variable: '%s' (Use `var`)\n", name_table[word]); //remove this to disable "OPTION STRICT"
	return declare_variable(word);
}

void flush_op_stack(int pri){
	//printf("flush op\n");
	while(op_length){
		struct Item top = pop_op();
		if(priority[top.operator] >= pri) //might need to use >= for binary ops idk?
			output(top);
		else{
			resurrect_op();
			break;
		}
	}
}

void flush_group(){
	struct Item item;
	while((item = pop_op()).operator != oGroup_Start)
		output(item);
}

enum Keyword read_line();

void expected(char * expected){
	parse_error("Expected %s, got `%s`\n", expected, token_name_2(token));
}

void unexpected_end(char * got, char * expected, char * start, struct Line start_line){
	parse_error("Encountered `%s` while expecting `%s` (to end `%s` on line %d char %d)\n", got, expected, start, start_line.line, start_line.column);
}

bool read_expression(bool allow_comma){
	next_t();
	switch(token.type){
	// Value
	case tkValue:
		output((struct Item){.operator = oConstant, .value = token.value});
	// Variable
	break;case tkWord:
		output(make_var_item(token.word));
	// Prefix operator
	break;case tkOperator_1: case tkOperator_12:
		flush_op_stack(priority[token.operator_1]+1); //adding 1 here is a dangerous hack to get like, left associativity or something
		//make sure prefix ops have priorities which are spread out so this won't break too much.
		
		push_op((struct Item){.operator = token.operator_1});
		if(!read_expression(true))
			expected("expression");
	// Group
	break;case tkLeft_Paren:
		push_op(group_start);
		if(!read_expression(true))
			output((struct Item){.operator = oConstant, .value = {.type = tNArgs, .args = 0}});
		if(!read_token(tkRight_Paren))
			expected("`)`");
		flush_group();
	// Array literal
	//input: [ list ]
	//output: <list> <oArray>
	break;case tkLeft_Bracket:
		push_op(group_start);
		read_token(tkLine_Break);
		if(!read_expression(true))
			output((struct Item){.operator = oConstant, .value = {.type = tNArgs, .args = 0}});
		read_token(tkLine_Break);
		if(!read_token(tkRight_Bracket))
			expected("`]`");
		flush_group();
		output((struct Item){.operator = oArray});
	// Table literal
	//input: { key = value , ... }
	//output: <key> <value> ... <oTable>
	break;case tkLeft_Brace:
		read_token(tkLine_Break);
		uint length = 0;
		push_op(group_start);
		if(read_expression(false)){
			length++;
			flush_group();
			if(!read_token(tkAssign))
				expected("`=`");
			push_op(group_start);
			if(!read_expression(false))
				expected("value");
			flush_group();
			while(read_token(tkComma)){
				push_op(group_start);
				read_expression(false);
				
				length++;
				flush_group();
				if(!read_token(tkAssign))
					expected("`=`");
				push_op(group_start);
				if(!read_expression(false))
					expected("value");
				flush_group();
			}
		}else
			flush_group();
		if(!read_token(tkRight_Brace))
			expected("`}`");
		output((struct Item){.operator = oTable, .length = length});
	break;case tkAt:
		output((struct Item){.operator = oAt/*meal*/});
	break;case tkKeyword:
		if(token.keyword == kDef){
			struct Item * start = output((struct Item){.operator = oJump});
			Address start_pos = output_length;
			struct Line start_line = real_line;
			if(!read_token(tkLeft_Paren))
				expected("`(` in function definition");
			//create scope
			p_push_scope();
			uint level = scope_length-1;
			//read argument list
			uint args=0;
			if(read_token(tkWord)){
				declare_variable(token.word);
				args++;
				while(read_token(tkComma)){
					if(!read_token(tkWord))
						expected("function parameter name");
					declare_variable(token.word);
					args++;
				}
			}
			if(!read_token(tkRight_Paren))
				expected("`)` in function definition");
			struct Item * function_info = output((struct Item){.operator = oFunction_Info, .level = level, .args = args});
			//read function body
			enum Keyword keyword;
			do{
				keyword = read_line();
			}while(!keyword);
			if(keyword!=kEnd)
				unexpected_end(keywords[keyword], keywords[kEnd], keywords[kDef], start_line);
			output((struct Item){.operator = oReturn_None});
			start->address = output_length;
			function_info->locals = locals_length[scope_length-1];
			discard_scope(start_pos);
			output((struct Item){.operator = oConstant, .value = {.type = tFunction, .builtin = false, .user_function = start_pos}});
		}else{
			return read_next = false;
		}
	break;default:
		return read_next = false;
	}
	//This handles operators that can appear at the "end" of an expression
	//and might continue the expression.
	//for example <expr> + <expr2> or <expr>[index]
	while(1){
		next_t();
		switch(token.type){
		// Index
		case tkLeft_Bracket:
			push_op(group_start);
			if(!read_expression(true))
				expected("expression");
			if(!read_token(tkRight_Bracket))
				expected("`]`");
			flush_group();
			output((struct Item){.operator = oIndex});
		break;
		// Infix operator
		case tkComma:
			if(!allow_comma){
				read_next = false;
				return true;
			}
			flush_op_stack(priority[token.operator_2]);
			push_op((struct Item){.operator = token.operator_2});
			read_token(tkLine_Break);
			if(!read_expression(true))
				pop_op();
		break;case tkOperator_2: case tkOperator_12: //no break
			//don't allow commas. This is for reading table literals where commas are used to separate values.
			//(maybe a better idea would be to use a different symbol but whatever)
			flush_op_stack(priority[token.operator_2]);
			push_op((struct Item){.operator = token.operator_2});
			read_token(tkLine_Break); //this is one case where you're allowed to have line breaks in an expression
			//after a binary infix operator
			//1+1 can be 1+ \n 2
			//mainly useful for things like array literals etc.
			//maybe also allow line breaks after [ and before ] ?
			if(!read_expression(true))
				expected("expression");
		// logical "or" and "and"
		break;case tkOr:;
			struct Item * start = output((struct Item){.operator = oLogicalOr});
			flush_op_stack(priority[oLogicalOr]);
			read_token(tkLine_Break);
			if(!read_expression(true))
				expected("expression");
			start->address = output_length;
		//break;case tkAnd:
			
		// Function call
		break;case tkLeft_Paren:
			push_op(group_start);
			//read arguments
			if(!read_expression(true))
				//handle () 0 arguments
				output((struct Item){.operator = oConstant, .value = {.type = tNArgs, .args = 0}});
				//if there is 1 arg, oCall will see something other than tNArgs on the stack.
			if(!read_token(tkRight_Paren))
				expected("`)`");
			flush_group();
			output((struct Item){.operator = oCall});
		// .
		// break;case tkDot:
			// edit this to allow keywords after . maybe
			// if(!read_token(tkWord))
				// expected("Property Name");
			// push_property_name(name_table[token.word]);
			// push_op((struct Item){.operator = oProperty, .property = property_names_length-1});
		break;default:
			read_next = false;
			return true;
		}
	}
}

bool read_full_expression(){
	push_op(group_start);
	if(read_expression(true)){
		flush_group();
		return true;
	}
	flush_group();
	return false;
}

enum Keyword read_line(){
	//printf("parser line\n");
	if(read_full_expression()){
		if(read_token(tkAssign)){
			//optimize: only insert when @ is actually used in the value expression.
			output((struct Item){.operator = oSet_At});
			read_full_expression();
			output((struct Item){.operator = oAssign_Discard});
		}else
			output((struct Item){.operator = oDiscard});
	}else{
		next_t();
		switch(token.type){
		case tkLine_Break: case tkSemicolon:
			//
		break;case tkKeyword:
			switch(token.keyword){
			//WHILE
			case kWhile:;
				Address start_pos = output_length;
				struct Line start_line = real_line;
				if(!read_full_expression())
					parse_error("Missing condition in `while`\n");
				struct Item * start = output((struct Item){.operator = oJumpFalse});
				enum Keyword keyword;
				do{
					keyword = read_line();
				}while(!keyword);
				if(keyword!=kWend)
					unexpected_end(keywords[keyword], keywords[kWend], keywords[kWhile], start_line);
				output((struct Item){.operator = oJump, .address = start_pos});
				start->address = output_length;
			// IF
			break;case kIf:
				start_line = real_line;
				//read condition
				if(!read_full_expression())
					parse_error("Missing condition in `if`\n");
				
				start = output((struct Item){.operator = oJumpFalse});
				//read THEN
				next_t();
				if(!(token.type == tkKeyword && token.keyword == kThen))
					expected("`then`");
					//parse_error("Missing `then` after `if`\n");
				//Read code inside IF block
				do{
					keyword = read_line();
				}while(!keyword);
				
				if(keyword == kEndif){
					start->address = output_length;
				}else if(keyword == kElseif){
					//you need to do lots of stuff here
					parse_error("ELSEIF UNSUPPORTED\n");
				}else if(keyword == kElse){
					
					parse_error("ELSE UNSUPPORTED\n");
				}else{
					unexpected_end(keywords[keyword], "Endif/`Elseif`/`Else", keywords[kIf], start_line);
				}
			// repeat
			break;case kRepeat:
				start_pos = output_length;
				start_line = real_line;
				do{
					keyword = read_line();
				}while(!keyword);
				if(keyword!=kUntil)
					unexpected_end(keywords[keyword], keywords[kUntil], keywords[kRepeat], start_line);
				if(!read_full_expression())
					parse_error("Missing condition in `until`\n");
				output((struct Item){.operator = oJumpFalse, .address = start_pos});
			// def
			break;case kDef:
				//function def compiles to:
				//jump(@skip) @func functioninfo(level, locals, args) ... x return @skip
				parse_error("coming soon\n");
			//return
			break;case kReturn:
				if(read_full_expression())
					output((struct Item){.operator = oReturn});
				else
					output((struct Item){.operator = oReturn_None});
			// var
			break;case kVar:
				if(!read_token(tkWord))
					parse_error("Missing name in variable declaration\n");
				struct Item variable = declare_variable(token.word);
				if(read_token(tkLeft_Brace)){
					start = output((struct Item){.operator = oJump});
					Address start_pos = output_length;
					if(!read_full_expression())
						parse_error("Missing constraint expression\n");
					output((struct Item){.operator = oConstrain_End});
					start->address = output_length;
					//todo: store `start`... somewhere...
					if(!read_token(tkRight_Brace))
						parse_error("missing `}`");
					output(variable);
					output((struct Item){.operator = oConstrain, .address = start_pos});
				}
				if(read_token(tkAssign)){
					output(variable);
					if(!read_full_expression())
						parse_error("Missing initialization expression\n");
					output((struct Item){.operator = oAssign_Discard});
				}else{
					read_next = false;
				}
			//"End" tokens
			break;case kEndif:case kWend:case kElse:case kElseif:case kUntil:case kEnd:
				return token.keyword;
			break;default:
				parse_error("Unsupported token\n");
			}
		break;default:
			return -1;
		}
	}
	return 0;
}

//parser/tokenizer can take either stream or string as input
//This is an ugly hack but I need to use it because my C compiler doesn't have `fmemopen`
struct Item * parse(FILE * stream, char * string){
	output_stack = malloc(sizeof(struct Item) * 65536);
	
	if(stream)
		init_stream(stream);
	else
		init_string(string);
	read_next=true;
	p_push_scope();
	//declare builtin variables/function
	uint i;
	for(i=0;i<ARRAYSIZE(builtins);i++){
		declare_variable(i);
	}
	
	output((struct Item){.operator = oInit_Global});
	
	while(read_line()!=-1);
	
	if(scope_length!=1)
		parse_error("internal error: scope mistake\n");
	
	output((struct Item){.operator = oHalt});
	output_stack[0].locals = locals_length[0];
	discard_scope(0);
	return output_stack;
}

//parsing tables:
//1: read key expression
//2: read : (or maybe a different separator symbol?)
//3: read value expression (don't allow top level commas)
//4: repeat
//5: push table literal operator
//(how to do constraints though?? maybe there is a more consistant way to implement them for regular variables, arrays, and tables)

//make = a statement rather than an expression
//so that if x=1 then will throw an error during parsing!
//when = is encountered in an expression, finish that expression and then read another expression and push the assign operator

//idea: var <variable_name> declares a variable.
//have an operator that applies a constraint, and after var, go back and read <variable_name> as a line of code to run the constraint and assignment.

//usually {a = 3} sets x["a"] or x.a, and {["a"]=3} sets the same thing
//change that maybe...
//used by JS and Lua

//array[start LENGTH length] -> new array which points to part of the old array.
//might be dangerous if the original array is resized or reallocated
//also might ruin garbage collection
//...
//the sub-array must hold a reference back to the main array, so that the main array's reference count can be modified too
//and to determine if the main array has gotten too small...
//this isn't worth doing, really.
//:(

//syntax for constraints in arrays?

//need to keep track of variable names somehow, for error messages/debugging
//after an error, allow inputting variable names to check value
//or write a full REPL idk
//awful dictionary library

struct Entry {
	struct Variable variable;
	size_t key_size;
	char * key;
	unsigned char key_type;
	struct Entry * next;
};

//rather than storing first and last, just store first, and insert new elements at the start of the linked list
//this would break support for iteration though ...
struct Table {
	struct Entry * first;
	struct Entry * last;
	uint length;
	bool checked;
}; //table_new = {.first = NULL, .last = NULL, /*.references = 0*/};

struct Entry * table_get(struct Table * table, struct Value key, bool add){
	char * key_data;
	size_t key_size;
	//this is very very very very very not very good.
	switch(key.type){
	case tNumber:
		key_data = (char *)&(key.number);
		key_size = sizeof(double);
		break;
	case tString:
		key_data = (char *)key.string->pointer;
		key_size = key.string->length * sizeof(char);
		break;
	case tTable:
		key_data = (char *)&(key.table);
		key_size = sizeof(struct Table *);
		break;
	case tArray:
		key_data = (char *)&(key.array);
		key_size = sizeof(struct Array *);
		break;
	case tNone:
		key_size = 0;
		break;
	case tBoolean:
		key_data = (char *)&(key.boolean);
		key_size = sizeof(bool);
		break;
	case tFunction:
		key_data = (char *)&(key.user_function); //bad
		key_size = sizeof(Address);
		break;
	case tNArgs:
		die("Can't use a list to index a table\n");
	}
	
	struct Entry * current = table->first;
	while(current){
		if(current->key_size == key_size && current->key_type==key.type && !memcmp(current->key, key_data, key_size)){
			return current;
		}
		current=current->next;
	}
	if(add){
		current = ALLOC_INIT(struct Entry, {.key_size = key_size, .key_type = key.type, .key = memdup(key_data, key_size), .next = NULL});
		if(table->last){
			table->last->next = current;
			table->last = current;
			table->length++;
		}else{ //first item added to the table
			table->first = table->last = current;
			table->length = 1;
		}
		return current;
	}
	return NULL;
}

void free_table(struct Table * table){
	struct Entry * current = table->first;
	while(current){
		struct Entry * next = current->next;
		free(current->key);
		free(current);
		current=next;
	}
	free(table);
}

unsigned int table_length(struct Table * table){
	unsigned int length=0;
	struct Entry * current = table->first;
	while(current){
		length++;
		current=current->next;
	}
	return length;
}
//ok so when you access table.key, that should be fast, of course. .key should be a symbol, not a string
//but it is expected that table.key == table["key"] ...
//maybe make strings also check symbols too?

//creates a new table slot (or overwrites an existing one) with a variable
//(This is the only way to modify the constraint expression)
//Meant to be used by VAR.
struct Variable * table_declare(struct Table * table, struct Value key, struct Value value){
	struct Entry * entry = table_get(table, key, true);
	entry->variable = (struct Variable){.value = value};
	return entry->variable.value.variable = &(entry->variable);
}

//returns the value stored at an index in a table.
//(Its value can be read/written)
//Used in normal situations.
//Remember that the value contains a pointer back to the original variable :)
struct Value table_lookup(struct Table * table, struct Value key){
	//printf("looking up\n");
	struct Entry * entry = table_get(table, key, false);
	if(entry)
		return entry->variable.value;
	//die("tried to access undefined table index");
	return table_declare(table, key, (struct Value){.type = tNone})->value;
}
//keep a list of all objects allocated on the heap (arrays/tables/strings)
//garbage collection:
//for each item in the stack, and each variable in the scope stack:
// recursively mark all referenced objects as used.
//for each object in the objects list, free it if it hasn't been marked as used.

struct Heap_Object {
	enum Type type;
	union {
		struct Array * array;
		struct Table * table;
		struct String * string;
		void * pointer;
	};
};

uint used_memory = 0;
struct Heap_Object heap_objects[65536];

//add a flag to array/string/table keeping track of whether it's been checked or not

void check(struct Value * value){
	switch(value->type){
	case tArray:;
		struct Array * array = value->array;
		if(array->checked)
			return;
		array->checked = true;
		uint i;
		for(i=0;i<array->length;i++)
			check(&(array->pointer[i].value));
	break;case tTable:;
		struct Table * table = value->table;
		if(table->checked)
			return;
		table->checked = true;
		struct Entry * current = table->first;
		while(current){
			check(&(current->variable.value));
			current=current->next;
		}
	break;case tString:;
		value->string->checked = true;
	}
}

void garbage_collect(){
	printf("Running garbage collector\n");
	uint i, j;
	
	for(i=0;i<stack_pointer;i++)
		check(stack + i);
	
	for(i=0;i<scope_stack_pointer;i++)
		for(j=0;j<r_scope_length[i];j++)
			check(&(scope_stack[i][j].value));
	
	for(i=0;i<ARRAYSIZE(heap_objects);i++)
		switch(heap_objects[i].type){
		case tArray:
			if(!heap_objects[i].array->checked){
				used_memory -= heap_objects[i].array->length * sizeof(struct Variable);
				free(heap_objects[i].array->pointer);
				free(heap_objects[i].array);
				heap_objects[i].type = tNone;
			}else
				heap_objects[i].array->checked = false;
		break;case tTable:
			if(!heap_objects[i].table->checked){
				used_memory -= heap_objects[i].table->length * sizeof(struct Entry); // + table struct itself
				free_table(heap_objects[i].table);
				heap_objects[i].type = tNone;
			}else
				heap_objects[i].table->checked = false;
		break;case tString:
			if(!heap_objects[i].string->checked){
				used_memory -= heap_objects[i].string->length * sizeof(char);
				free(heap_objects[i].string->pointer);
				free(heap_objects[i].string);
				heap_objects[i].type = tNone;
			}else
				heap_objects[i].string->checked = false;
		}
}

//just make a custom *alloc function which checks if there's enough memory free and adjusts FREEMEM
size_t memory_usage(struct Value * value){
	switch(value->type){
	case tArray:
		return value->array->length * sizeof(struct Variable);
	break;case tTable:
		return value->table->length * sizeof(struct Entry);
	break;case tString:
		return value->string->length * sizeof(char);
	}
	die("ww\n");
}

void record_alloc(struct Value * value){
	used_memory += memory_usage(value);
	//printf("mem: %d\n",used_memory);
	uint i;
	for(i=0;i<ARRAYSIZE(heap_objects);i++)
		if(heap_objects[i].type == tNone){
			//printf("allocated, slot %d\n", i);
			heap_objects[i] = (struct Heap_Object){.type = value->type, .pointer = value->pointer};
			if(used_memory > 64 * 1024 * 1024)
				garbage_collect();
			if(used_memory > 64 * 1024 * 1024)
				die("Used too much memory\n");
			goto success;
		}
	garbage_collect();
	for(i=0;i<ARRAYSIZE(heap_objects);i++)
		if(heap_objects[i].type == tNone){
			//printf("allocated on second attempt, slot %d\n", i);
			heap_objects[i] = (struct Heap_Object){.type = value->type, .pointer = value->pointer};
			goto success;
		}
	die("Out of space\n");
	success:;
}

//consider: https://en.wikipedia.org/wiki/Tracing_garbage_collection#Moving_vs._non-moving
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
