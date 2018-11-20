uint output_stack_pointer = 0;

enum Token_Type {
	tkValue,
	tkDot,
	tkOperator_1, //unary operator
	tkOperator_2, //binary operator
	tkOperator_12, //could be either
	tkLeft_Paren,
	tkRight_Paren,
	tkLeft_Bracket,
	tkRight_Bracket,
	tkLeft_Brace,
	tkRight_Brace,
	tkSemicolon,
	tkColon,
	tkComma,
	tkKeyword,
	tkWord,
	tkEof,
	tkAt,
	tkLine_Break,
	tkAssign,
};

struct Line {
	uint line;
	uint column;
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
};

struct Token {
	enum Token_Type type;
	union {
		struct Value value;
		struct {
			enum Operator operator_1;
			enum Operator operator_2;
		};
		int keyword;
		int word;
	};
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

char * string_input = NULL;

void next(){
	if(read_next){
		line.column++;
		if(c=='\n'){
			line.column = 1;
			line.line++;
			line_position_in_output[line.line] = output_stack_pointer;
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

void init(FILE * new_stream){
	stream = new_stream;
	line.line = 1;
	line.column = 0;
	read_next = true;
	next();
}

void init_string(char * string){
	string_input = string;
	line.line = 1;
	line.column = 0;
	read_next = true;
	next();
}

enum Keyword { //reference to keywords list
	kIf = 0, kThen, kElse, kElseif, kEndif,
	kFor, kNext,
	kWhile, kWend,
	kRepeat, kUntil,
	kVar,
	kDef, kReturn, kEnd,
	//...
};

//typedef uint Word;

char * name_table[63356] = {"seconds"};
uint name_table_pointer = 1;

char * keywords[] = {
	"if", "then", "else", "elseif", "endif",
	"for", "next",
	"while", "wend",
	"repeat", "until",
	"var",
	"def", "return", "end",
	//...
};

struct Token process_word(char * word){
	if(!strcmp(word, "true"))
		return (struct Token){.type = tkValue, .value = {.type = tBoolean, .boolean = true}};
	if(!strcmp(word, "false"))
		return (struct Token){.type = tkValue, .value = {.type = tBoolean, .boolean = false}};
	uint i;
	for(i=0;i<ARRAYSIZE(keywords);i++)
		if(!strcmp(keywords[i],word))
			return (struct Token){.type = tkKeyword, .keyword = i};
	
	for(i=0;i<name_table_pointer;i++)
		if(!strcmp(name_table[i],word))
			return (struct Token){.type = tkWord, .word = i};
	name_table[name_table_pointer] = strdup(word);
	return (struct Token){.type = tkWord, .word = name_table_pointer++};
}

struct Token next_token(){
	while(c == ' ' || c == '\t'){
		next();
	}
	
	real_line = line;
	
	switch(c){
		case EOF:
			line_position_in_output[line.line+1] = (uint)-1;
			return (struct Token){.type = tkEof};
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
		case '"':
			next();
			int length=0;
			while(c != '"'){
				if(c == EOF)
					parse_error("Unclosed string");
				string_temp[length++] = c;
				next();
			}
			//next(); //IDK why I need 2 nexts here
			next();
			return (struct Token){.type = tkValue, .value = {.type = tString, .string = ALLOC_INIT(struct String, {.pointer = memdup(string_temp, length * sizeof(char)), .length = length})}};
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
		case '\'':
			do{
				next();
			}while(c!='\n' && c!=EOF);
			return next_token();
		//case '\n': ...
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
		//case '?': ...
		case ',':
			next();
			return (struct Token){.type = tkOperator_2, .operator_2 = oComma};
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