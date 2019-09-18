#include "types.h"

// input: stream or string
// output: tokens
static void * memdup(void * source, size_t size) {
	void * mem = malloc(size);
	return mem ? memcpy(mem, source, size) : NULL;
}
#define ALLOC_INIT(type, ...) memdup((type[]){ __VA_ARGS__ }, sizeof(type))

char * keywords[] = {
	"if", "then", "else", "elseif", "endif",
	"for", "next",
	"while", "wend",
	"repeat", "until",
	"var",
	"def", "return", "end",
	//...
};
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
			if(!strcmp(name_table[i],word)){
				token->type = tkWord;
				token->word = i;
				goto ret;
			}
		}
		//new word
		name_table[name_table_pointer] = strdup(word);
		token->type = tkWord;
		token->word = name_table_pointer++;
	}
	ret:;
}

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
}

void f_array_fill(uint args);
//put builtin variables here
struct Variable builtins[] = {
	//{.value = {.type = tTable, .table = &f_floor}, .constraint_expression = -1},
	//{.value = {.type = tFunction, .builtin = true, .c_function = &f_floor}, .constraint_expression = -1},
	//{.value = {.type = tFunction, .builtin = true, .c_function = &f_ceil}, .constraint_expression = -1},
	{.value = {.type = tNumber, .number = 3.14159265358979323846}, .constraint_expression = -1},
	{.value = {.type = tFunction, .builtin = true, .c_function = &f_array_fill}, .constraint_expression = -1},
};

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

struct Line real_line;

struct Token next_token(){
	static char string_temp[1000];
	while(c == ' ' || c == '\t')
		next();
	
	real_line = line;
	struct Token ret = {.position_in_file = real_line};
	
	switch(c){
		case EOF:
			//line_position_in_output[line.line+1] = (uint)-1;
			ret.type = tkEof;
		// Number
		break;case '0':case '1':case '2':case '3':case '4':case '5':case '6':case '7':case '8':case '9':;
			double number = c - '0';
			next();
			while(isdigit(c)){
				number *= 10;
				number += c - '0';
				//number = ? * 10 + (c-'0')
				next();
			}
			// decimal point, and next char is a digit:
			if(c == '.'){
				next();
				if(isdigit(c)){
					//next();
					double divider = 10;
					do{
						number += (c - '0') / divider;
						divider *= 10;
						next();
					}while(isdigit(c));
				}else
					read_next = false;
			}
			ret.type = tkValue;
			ret.value = (struct Value){.type = tNumber, .number = number};
		// . operator or number starting with .
		break;case '.':
			next();
			if(isdigit(c)){
				double number= c - '0';
				next();
				while(isdigit(c)){
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
			if(isalpha(c) || c == '_'){
				int length=0;
				do{
					string_temp[length++] = c;
					next();
				}while(isalpha(c) || isdigit(c) || c == '_');
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
		address_position_in_code[output_length] = token.position_in_file;
	}else
		read_next=true;
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

//Scope (level) stack
//This keeps track of local variables during parsing.
uint * p_level_stack[256]; //256 = # of levels
uint scope_length = 0;
uint locals_length[256]; //256 = # of local variables per scope

Address function_addresses[65536];
uint * function_locals[65536];
uint function_addresses_length = 0;

extern struct Variable * scope_stack[256];
extern uint scope_stack_pointer;
extern uint r_scope_length[ARRAYSIZE(scope_stack)];

extern Address call_stack[255]; //this should be bigger maybe
extern uint call_stack_pointer;

static char * operator_name[] = {
	"Invalid Operator", "Constant", "Variable", "~", "~", "!=", "!", "%", "^", "&", "*", "-", "-", "+", "==", "=", "|",
	"\\", "<=", "<<", "<", ">=", ">>", ">", "/", "Array Literal", "Index", "Call", "Discard", "?", "Halt",
	"Table Literal", "Global Variables", "Jump", "or", "and", "Length", "Jump if false", "Jump if true",
	"Function Info", "Return None", "Group Start", "Assign Discard", "Constrain", "Constrain End", "At", ",",	
};

char * variable_pointer_to_name(struct Variable * variable){
	uint i;
	for(i=scope_stack_pointer-1;i!=-1;i--){
		uint j;
		
		for(j=0;j<r_scope_length[i];j++){
			if(&scope_stack[i][j] == variable){
				Address address = i ? call_stack[i-1] : 0;
				for(i=0;i<function_addresses_length;i++){
					if(function_addresses[i]==address){
						return name_table[function_locals[i][j]];
					}
				}
				die("Internal error: failed to get variable name");
			}
		}
	}
	return "<unknown variable>";
	die("failed 1\n");

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

//note: the thing that the parser calls "scope" is called "level" in run.c

struct Item declare_variable(uint word){
	if(locals_length[scope_length-1] >= 256)
		parse_error("Local Variable Stack Overflow\n");
	p_level_stack[scope_length-1][locals_length[scope_length-1]] = word;
	return (struct Item){.operator = oVariable, .level = scope_length-1, .index = locals_length[scope_length-1]++};
}

struct Item make_var_item(uint word){
	uint i;
	//This will break if ssp1 is 0. Make sure the global scope is created at the beginning!
	//search for variable in existing scopes, starting from the current one and working down towards global.
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

enum Keyword read_line();

char * token_name[] = {
	"(Value)", "`.`",	"Operator",	"Operator", "Operator",
	"`(`", "`)`", "`[`", "`]`","`{`", "`}`", "`;`", "`:`", "`,`",
	"Keyword", "(Variable)", "(End of program)", "@", "(Line break)",
	"=", "or", "(Unclosed string)", "(Invalid character)",
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
		flush_op_stack_1(token.operator_1);
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
			flush_op_stack(token.operator_2);
			push_op((struct Item){.operator = token.operator_2});
			read_token(tkLine_Break);
			if(!read_expression(true))
				pop_op();
		break;case tkOperator_2: case tkOperator_12: //no break
			//don't allow commas. This is for reading table literals where commas are used to separate values.
			//(maybe a better idea would be to use a different symbol but whatever)
			flush_op_stack(token.operator_2);
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
			flush_op_stack(oLogicalOr);
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
