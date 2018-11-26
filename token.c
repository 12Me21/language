uint output_length = 0;

struct Line {
	uint line;
	uint column;
};

bool is_digit(char c){
	return c>='0' && c<='9';
}

bool is_alpha(char c){
	return (c>='A' && c<='Z') || (c>='a' && c<='z');
}

bool read_next;
int c;
FILE * stream;
char string_temp[1000];
struct Line line;
struct Line real_line;

void parse_error_1(){
	printf("Error while parsing line %d, character %d\n ", real_line.line, real_line.column);
}

void parse_error_2(){
	longjmp(err_ret, 1);
}

#define parse_error(...) (parse_error_1(), printf(__VA_ARGS__), parse_error_2())

void unexpected_end(char * got, char * expected, char * start, struct Line start_line){
	parse_error("Encountered `%s` while expecting `%s` (to end `%s` on line %d char %d)\n", got, expected, start, start_line.line, start_line.column);
}

Address line_position_in_output[65536];

uint get_line(Address address){
	uint i;
	for(i=0;i<ARRAYSIZE(line_position_in_output);i++)
		if(line_position_in_output[i]>address)
			break;
	return i-1;
}

char * string_input = NULL;

//next character
void next(){
	if(read_next){
		line.column++;
		if(c=='\n'){
			line.column = 1;
			line.line++;
			line_position_in_output[line.line] = output_length;
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

#include "builtins.h"

//put builtin variables here
struct Variable builtins[] = {
	//{.value = {.type = tTable, .table = &f_floor}, .constraint_expression = -1},
	//{.value = {.type = tFunction, .builtin = true, .c_function = &f_floor}, .constraint_expression = -1},
	//{.value = {.type = tFunction, .builtin = true, .c_function = &f_ceil}, .constraint_expression = -1},
	{.value = {.type = tNumber, .number = M_PI}, .constraint_expression = -1},
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

struct Token process_word(char * word){
	//maybe it would be better if each keyword was a separate token...
	//check special reserved words:
	if(!strcmp(word, "true"))
		return (struct Token){.type = tkValue, .value = {.type = tBoolean, .boolean = true}};
	if(!strcmp(word, "false"))
		return (struct Token){.type = tkValue, .value = {.type = tBoolean, .boolean = false}};
	if(!strcmp(word, "none"))
		return (struct Token){.type = tkValue, .value = {.type = tNone}};
	if(!strcmp(word, "or"))
		return (struct Token){.type = tkOr};
	if(!strcmp(word, "in"))
		return (struct Token){.type = tkOperator_2, .operator_2 = oIn};
	uint i;
	//check keywords list
	for(i=0;i<ARRAYSIZE(keywords);i++)
		if(!strcmp(keywords[i],word))
			return (struct Token){.type = tkKeyword, .keyword = i};
	//check variable names list
	for(i=0;i<name_table_pointer;i++){
		//printf("vn comp: %s, %s\n", name_table[i],word);
		if(!strcmp(name_table[i],word)){
			//printf("var name match: %s num %d\n",word,i);
			return (struct Token){.type = tkWord, .word = i};
		}
	}
	//new word
	//printf("new var name: %s\n",word);
	name_table[name_table_pointer] = strdup(word);
	return (struct Token){.type = tkWord, .word = name_table_pointer++};
}

struct Token next_token(){
	while(c == ' ' || c == '\t')
		next();
	
	real_line = line;
	
	switch(c){
		case EOF:
			line_position_in_output[line.line+1] = (uint)-1;
			return (struct Token){.type = tkEof};
		// Number
		case '0':
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
			return (struct Token){.type = tkValue, .value = {.type = tNumber, .number = number}};
		// . operator or number starting with .
		case '.':
			next();
			if(is_digit(c)){
				double number= c - '0';
				next();
				while(is_digit(c)){
					number *= 10;
					number += c - '0';
					next();
				}
				return (struct Token){.type = tkValue, .value = {.type = tNumber, .number = number}};
			}
			return (struct Token){.type = tkDot};
		// String
		case '"':
			next();
			int length=0;
			while(c != '"'){
				if(c == EOF)
					parse_error("Unclosed string");
				string_temp[length++] = c;
				next();
			}
			//next(); //IDK why I need[ed] 2 nexts here
			next();
			return (struct Token){.type = tkValue, .value = {.type = tString, .string = 
				ALLOC_INIT(struct String, {.pointer = memdup(string_temp, length * sizeof(char)), .length = length})
			}};
		case '~':
			next();
			return (struct Token){.type = tkOperator_12, .operator_1 = oBitwise_Not, .operator_2 = oBitwise_Xor};
		case '!':
			next();
			if(c=='='){
				next();
				return (struct Token){.type = tkOperator_2, .operator_2 = oNot_Equal};
			}
			return (struct Token){.type = tkOperator_1, .operator_1 = oNot};
		case '%':
			next();
			return (struct Token){.type = tkOperator_2, .operator_2 = oMod};
		case '^':
			next();
			return (struct Token){.type = tkOperator_2, .operator_2 = oExponent};
		case '&':
			next();
			return (struct Token){.type = tkOperator_2, .operator_2 = oBitwise_And};
		case '*':
			next();
			return (struct Token){.type = tkOperator_2, .operator_2 = oMultiply};
		case '(':
			next();
			return (struct Token){.type = tkLeft_Paren};
		case ')':
			next();
			return (struct Token){.type = tkRight_Paren};
		case '-':
			next();
			return (struct Token){.type = tkOperator_12, .operator_1 = oNegative, .operator_2 = oSubtract};
		case '+':
			next();
			return (struct Token){.type = tkOperator_2, .operator_2 = oAdd};
		case '?':
			next();
			return (struct Token){.type = tkOperator_1, .operator_1 = oPrint};
		case '=':
			next();
			if(c=='='){
				next();
				return (struct Token){.type = tkOperator_2, .operator_2 = oEqual};
			}
			return (struct Token){.type = tkAssign};
		case '[':
			next();
			return (struct Token){.type = tkLeft_Bracket};
		case ']':
			next();
			return (struct Token){.type = tkRight_Bracket};
		case '{':
			next();
			return (struct Token){.type = tkLeft_Brace};
		case '}':
			next();
			return (struct Token){.type = tkRight_Brace};
		case '|':
			next();
			return (struct Token){.type = tkOperator_2, .operator_2 = oBitwise_Or};
		case '\\':
			next();
			return (struct Token){.type = tkOperator_2, .operator_2 = oFloor_Divide};
		case ';':
			next();
			return (struct Token){.type = tkSemicolon};
		case '\n':
			next();
			return (struct Token){.type = tkLine_Break};
		case ':':
			next();
			return (struct Token){.type = tkColon};
		case '@':
			next();
			return (struct Token){.type = tkAt};
		// Comment
		case '\'':
			do{
				next();
			}while(c!='\n' && c!=EOF);
			return next_token();
		case '<':
			next();
			if(c=='='){
				next();
				return (struct Token){.type = tkOperator_2, .operator_2 = oLess_Or_Equal};
			}else if(c=='<'){
				next();
				return (struct Token){.type = tkOperator_2, .operator_2 = oLeft_Shift};
			}
			return (struct Token){.type = tkOperator_2, .operator_2 = oLess};
		case '>':
			next();
			if(c=='='){
				next();
				return (struct Token){.type = tkOperator_2, .operator_2 = oGreater_Or_Equal};
			}else if(c=='>'){
				next();
				return (struct Token){.type = tkOperator_2, .operator_2 = oRight_Shift};
			}
			return (struct Token){.type = tkOperator_2, .operator_2 = oGreater};
		case ',':
			next();
			return (struct Token){.type = tkComma, .operator_2 = oComma};
		case '/':
			next();
			return (struct Token){.type = tkOperator_2, .operator_2 = oDivide};
		default:
			if(is_alpha(c) || c == '_'){
				int length=0;
				do{
					string_temp[length++] = c;
					next();
				}while(is_alpha(c) || is_digit(c) || c == '_');
				string_temp[length] = '\0';
				return process_word(string_temp);
			}
			parse_error("Invalid character");
	}
}