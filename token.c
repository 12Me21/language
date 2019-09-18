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
		break;case '0':case '1':case '2':case '3':case '4':case '5':case '6':case '7':case '8':case '9':;
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
