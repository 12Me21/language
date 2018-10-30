#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h> 

enum Token_Type {
	tkEOF = 0,
	tkNUMBER,
	tkSTRING,
	tkERROR,
	tkOPERATOR,
	tkUNARY,
	tkMAYBE_UNARY,
	tkASSIGN,
	tkVARIABLE,
	tkFUNCTION,
	tkEXPRESSION_LENGTH,
	tkLEFT_PAREN,
	tkRIGHT_PAREN,
	tkLEFT_BRACKET,
	tkRIGHT_BRACKET,
	tkWORD,
	tkCOMMA,
	
	tkKEYWORD,
	tkIF=tkKEYWORD,
	tkTHEN,
	tkELSE,
	tkELSEIF,
	tkENDIF,
	tkFOR,
	tkNEXT,
	tkWHILE,
	tkWEND,
};

// type = tkOPERATOR, tkUNARY, tkMAYBE_UNARY
enum Operator_Type {
	opADD = 0,
	opMINUS, //and negate
	opNEGATE,
	opMULTIPLY,
	opDIVIDE,
	opMOD,
	opFLOOR_DIVIDE,
	opEXPONENT,
	opBITWISE_AND,
	opBITWISE_OR,
	opBITWISE_XOR, //and NOT
	opBITWISE_NOT,
	opLOGICAL_NOT,
	opLOGICAL_AND,
	opLOGICAL_OR,
	opLOGICAL_XOR,
	opEQUAL,
	opNOT_EQUAL,
	opDOT,
	opLEFT_SHIFT,
	opRIGHT_SHIFT,
	opLESS_OR_EQUAL,
	opLESS,
	opGREATER_OR_EQUAL,
	opGREATER,
	opINDEX,
	opARRAY,
	opFOR,
};

char * OPERATOR_NAME[] = {
	"Add",
	"Subtract",
	"Negate",
	"Multiply",
	"Divide",
	"Mod",
	"Floor Divide",
	"Exponent",
	"Bitwise And",
	"Bitwise Or",
	"Bitwise Xor",
	"Bitwise Not",
	"Logical Not",
	"Logical And",
	"Logical Or",
	"Logical Xor",
	"Equal",
	"Not Equal",
	"Dot",
	"Left Shift",
	"Right Shift",
	"Less Than or Equal",
	"Less Than",
	"Greater Than or Equal",
	"Greater Than",
	"Index",
	"Array Literal",
	"For Loop",
};

int PRIORITY[] = {
	11, //add
	11, //subtract
	99, //negate
	12, //mul
	12, //div
	12, //mod
	12, //div
	13, //exp
	6, //band
	4, //bor
	5, //bxor
	99, //bnot
	99, //not
	3, //and
	1, //or
	2, //xor
	7, //==
	7, //!=
	0, //dot
	10, //shift
	10,
	8, //<=
	8,
	8,
	8,
	100, //index (how does !array[x] behave?)
	101, //array literal (idk does priority do anything here??)
};

// type = tkERROR
enum Error_Type {
	erNONE=0,
	erUNCLOSED_STRING
};

//Token type:
//0-255 - single character tokens
//255-tkKEYWORD-1 - regular tokens
//tkKEYWORD+ - keywords (indexed into name_table)
struct Token {
	enum Token_Type type;
	union {
		double number;
		char * pointer;
		enum Error_Type error;
		int length;
		struct {
			int id;
			char args;
		};
	};
};

//allowed tokens:
// type, number (numbers)
// type, pointer (strings)
// type, error (errors)
// type, id (variables, functions)
// type, id, args (operators)
// type, length (expression length indicator)

//change this into one big char array (more efficient)
char * name_table[1000] = {
	//Real keywords
	"IF",
	"THEN",
	"ELSE",
	"ELSEIF",
	"ENDIF",
	"FOR",
	"NEXT",
	"WHILE",
	"WEND",
	//Soft keywords
	"TO",
	"STEP",
	//Names
	//...
};
double variables[1000];

#define KEYWORDS_END 9
int name_table_end;

enum Soft_Keyword {
	nmTO = KEYWORDS_END+1,
	nmSTEP
};

bool is_digit(char c){
	return c>='0' && c<='9';
}

bool is_alpha(char c){
	return (c>='A' && c<='Z') || (c>='a' && c<='z');
}

#define single(chr, name) case chr: token->type=tk##name; return

int add_name(char * new_name, size_t new_length){
	for(int i=0;i<name_table_end;i++){
		if(strlen(name_table[i])==new_length){
			if(!memcmp(name_table[i], new_name, new_length)){
				return i;
			}
		}
	}
	name_table[name_table_end]=memcpy(malloc(new_length+1),new_name,new_length);
	name_table[name_table_end][new_length]='\0';
	return name_table_end++;
}

char c;

void init(char * code){
	name_table_end = KEYWORDS_END;
	c=*code;
}

void next_token(char * * code, struct Token * token){
	//printf("call\n");
	void next(){
		//printf("%c",c);
		c=*++*code;
	}
	
	while(1){
		while(c==' ' || c=='\t')
			next();
		if(!c){
			token->type=tkEOF;
			return;
		}
		char old_c = c;
		next();
		switch(old_c){
		//case '\0':
			//printf("EOF\n");
		//	token->type=tkEOF;
		//	return;
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
			double number=old_c-'0';
			while(is_digit(c)){
				number*=10;
				number+=c-'0';
				//number = ? * 10 + (c-'0')
				next();
			}
			// decimal point, and next char is a digit:
			if(c=='.'){
				if(is_digit(*code[1])){
					next();
					double divider = 10;
					do{
						number+=(c-'0')/divider;
						divider*=10;
						next();
					}while(is_digit(c));
				}
			}
			token->type=tkNUMBER;
			token->number=number;
			return;
		case '.':
			if(is_digit(c)){
				double number=c-'0';
				next();
				while(is_digit(c)){
					number*=10;
					number+=c-'0';
					next();
				}
				token->type=tkNUMBER;
				token->number=number;
				return;
			}
			token->type=tkOPERATOR;
			token->id=opDOT;
			return;
		case '"':;
			char * start = *code;
			while(c && c!='"' && c!='\n')
				next();
			if(c=='"'){
				//printf("str end\n");
				token->type=tkSTRING;
				size_t size = sizeof(char)*(size_t)(*code-start);
				//printf("size: %d\n",size);
				memcpy(token->pointer=malloc(size+1),start,size);
				token->pointer[size]='\0';
				next();
				return;
			}
			token->type=tkERROR;
			token->error=erUNCLOSED_STRING;
			return;
		case '!':
			if(c=='='){
				next();
				token->type=tkOPERATOR;
				token->id=opNOT_EQUAL;
				return;
			}
			token->type=tkUNARY;
			token->id=opLOGICAL_NOT;
			return;
		case '=':
			if(c=='='){
				next();
				token->type=tkOPERATOR;
				token->id=opEQUAL;
				return;
			}
			token->type=tkASSIGN;
			return;
		case '\'':
			while(c && c!='\n')
				next();
			// (don't return here, read another token)
		case '<':
			token->type=tkOPERATOR;
			if(c=='<'){
				next();
				token->id=opLEFT_SHIFT;
				return;
			}
			if(c=='='){
				next();
				token->id=opLESS_OR_EQUAL;
				return;
			}
			token->id=opLESS;
			return;
		case '>':
			token->type=tkOPERATOR;
			if(c=='>'){
				next();
				token->id=opRIGHT_SHIFT;
				return;
			}
			if(c=='='){
				next();
				token->id=opGREATER_OR_EQUAL;
				return;
			}
			token->id=opGREATER;
			return;
		case '~':
			token->type=tkMAYBE_UNARY;
			token->id=opBITWISE_XOR;
			return;
		case '%':
			token->type=tkOPERATOR;
			token->id=opMOD;
			return;
		case '^':
			token->type=tkOPERATOR;
			token->id=opEXPONENT;
			return;
		case '&':
			token->type=tkOPERATOR;
			token->id=opBITWISE_AND;
			return;
		case '*':
			token->type=tkOPERATOR;
			token->id=opMULTIPLY;
			return;
		case '-':
			token->type=tkMAYBE_UNARY;
			token->id=opMINUS;
			return;
		case '+':
			token->type=tkOPERATOR;
			token->id=opADD;
			return;
		case '\\':
			token->type=tkOPERATOR;
			token->id=opFLOOR_DIVIDE;
			return;
		case '|':
			token->type=tkOPERATOR;
			token->id=opBITWISE_OR;
			return;
		case '/':
			token->type=tkOPERATOR;
			token->id=opDIVIDE;
			return;
		case ',':
			token->type=tkCOMMA;
			return;
		default:
			if(is_alpha(old_c) || old_c=='_'){
				char * start = *code-1;
				while(is_alpha(c) || c=='_' || is_digit(c)){
					next();
				}
				int id = add_name(start, *code-start);
				if(id < KEYWORDS_END){
					token->type=tkKEYWORD+id;
				}else{
					token->type=tkWORD;
					token->id=id;
				}
				return;
			}
			//other chars
			token->type=tkERROR;
			return;
		}
	}
}

void print_token(struct Token * token){
	switch(token->type){
	case tkOPERATOR:
		printf("Operator: %s, args: %d\n", OPERATOR_NAME[token->id], token->args);
		break;
	case tkNUMBER:
		printf("Number: %f\n", token->number);
		break;
	case tkVARIABLE:
		printf("Variable: %s\n", name_table[token->id]);
		break;
	default:
		printf("Unknown Token: %d\n",token->type);
	}
}

struct Token left_paren = {.type=tkLEFT_PAREN};

//operator:
//type = tkOPERATOR
//args = args
//id = operation id

void compile(char * code){
	init(code);
	struct Token token;
	printf("Code: %s\n\n",code);
	bool read_next=true;
	struct Token temp;
	
	void next(){
		if(read_next){
			next_token(&code, &token);
			printf("token type: %d\n",token.type);
		}else
			read_next=true;
	}
	
	bool read_token(int wanted_type){
		next();
		printf("rt: %d %d\n",token.type,wanted_type);
		if(token.type==wanted_type){
			read_next=true;
			return true;
		}
		read_next=false;
		return false;
	}
	
	struct Token operator_stack[256];
	struct Token * operator_stack_pointer = operator_stack;
	struct Token bytecode[65536];
	struct Token * bytecode_start = bytecode;
	struct Token * bytecode_pointer = bytecode_start + 1; //first entry is the expression length
	
	void finish_expression(){
		assert(operator_stack_pointer==operator_stack); //make sure operator stack is empty
		bytecode_start->type = tkEXPRESSION_LENGTH;
		bytecode_start->length = bytecode_pointer - bytecode_start; //length of expression (jump distance to start of next. 0 items = "length" 1)
		bytecode_start = bytecode_pointer;
		bytecode_pointer = bytecode_start + 1;
		operator_stack_pointer = operator_stack;
	}
	
	//push to output stack
	void push(struct Token token){
		//check for stack overflow
		*bytecode_pointer++=token;
	}
	
	//push to operator stack
	void push_op(struct Token token){
		//check for stack overflow
		*operator_stack_pointer++=token;
	}
	
	//pop tokens from the operator stack and push them onto the output stack
	//until a ( is found (Don't push that one though)
	void p(){
		operator_stack_pointer--;
		assert(operator_stack_pointer>=operator_stack);
		while(operator_stack_pointer->type!=tkLEFT_PAREN){
			push(*operator_stack_pointer);
			operator_stack_pointer--;
			assert(operator_stack_pointer>=operator_stack);
		}
	}
	
	void flush(int priority){
		while(operator_stack_pointer>operator_stack){
			struct Token top = *(operator_stack_pointer-1);
			if(top.type!=tkLEFT_PAREN && PRIORITY[top.type]>=priority)
				push(*--operator_stack_pointer);
			else
				break;
		}
	}
	
	bool read_expression(){
		printf("READING EXPR\n");
		next();
		switch(token.type){
		//number literal
		case tkNUMBER:
		case tkSTRING:
			push(token);
		//function or variable
			break;
		case tkWORD:
			if(read_token(tkLEFT_PAREN)){
				push(left_paren);
				int args=0;
				if(read_expression()){
					args++;
					while(read_token(tkCOMMA)){
						assert(read_expression());
						args++;
					}
				}
				assert(read_token(tkRIGHT_PAREN));
				temp.type=tkFUNCTION;
				temp.id=token.id;
				temp.args=args;
			}else{
				temp.type=tkVARIABLE;
				temp.id=token.id;
				push(temp);
			}
			break;
		case tkUNARY:
		case tkMAYBE_UNARY:
			flush(PRIORITY[token.id]);
			temp.type=tkOPERATOR;
			temp.id=token.id;
			temp.args=1;
			push_op(temp);
			assert(read_expression());
			break;
		//(
		case tkLEFT_PAREN:
			push(left_paren);
			read_expression();
			assert(read_token(tkRIGHT_PAREN));
			p();
			break;
		//array literal
		case tkLEFT_BRACKET:
			push(left_paren);
			int args=0;
			if(read_expression()){
				args++;
				while(read_token(tkCOMMA)){
					assert(read_expression());
					args++;
				}
			}
			assert(read_token(tkRIGHT_BRACKET));
			p();
			temp.type=tkOPERATOR;
			temp.id=opARRAY;
			temp.args=args;
			push(temp);
			break;
		default:
			read_next=false;
			return false;
		}
		while(1)
			if(read_token(tkLEFT_BRACKET)){
				push(left_paren);
				assert(read_expression());
				assert(read_token(tkRIGHT_BRACKET));
				p();
				temp.type=tkOPERATOR;
				temp.id=opINDEX;
				temp.args=2;
				push(temp);
			}else
				break;
		if(read_token(tkOPERATOR) || read_token(tkMAYBE_UNARY)){
			temp.type=tkOPERATOR;
			temp.id=token.id;
			temp.args=2;
			flush(PRIORITY[temp.id]);
			push_op(temp);
			assert(read_expression);
		}
		return true;
	}
	
	void read_statement(){
		next();
		switch(token.type){
		case tkFOR:
			printf("FOR loop:\n");
			assert(read_expression());
			//printf(" Variable: %s\n",name_table[token.id]);
			printf("RV\n");
			assert(read_token(tkASSIGN));
			assert(read_expression());
			//printf(" Start: %f\n",token.number);
			assert(read_token(tkWORD));
			assert(token.id==nmTO);
			assert(read_expression());
			//printf(" End: %f\n",token.number);
			temp.args = 3;
			if(read_token(tkWORD)){
				if(token.id==nmSTEP){
					assert(read_expression());
					temp.args = 4;
					//printf(" Step: %f\n",token.number);
				}else{
					read_next=false;
				}
			}
			temp.type = tkOPERATOR;
			temp.id = opFOR;
			push(temp);
		break;
		case tkIF:
			printf("IF statement:\n");
			
		}
	}
	
	read_statement();
	
	printf("output:\n");
	for(struct Token * token=bytecode;token<bytecode_pointer;token++){
		print_token(token);
	}
	
	// while(1){
		// next_token(&code, &token);
		// if(token.type>=tkKEYWORD){
			// printf("=== Token Type: Keyword: %s\n", name_table[token.type-tkKEYWORD]);
		// }else if(token.type>=256){
			// printf("=== Token Type: %s\n", TOKEN_NAME[token.type-256]);
			// switch(token.type){
			// case tkEOF:
				// return;
			// case tkNUMBER:
				// printf("number: %f\n", token.number);
			// break;case tkSTRING:
				// printf("string: %s\n", token.pointer);
			// break;case tkWORD:
				// printf("Word %d: %s\n", token.id, name_table[token.id]);
			// }
		// }else{
			// printf("=== Token Type: %c\n", token.type);
		// }
	// }
}

// FOR I=0 TO 10
 // IF I%2 THEN
  // ...
 // ENDIF
// NEXT

// - read FOR statement
// - push bytecode
// - push 'for' and position to block stack
// - read IF statement
// - push bytecode
// - ...
// - push 'if' and position to block stack
// - read ENDIF
// - check block stack (IF is on top, good).
// - modify IF's bytecode: add position of ENDIF
// - read NEXT
// - check block stack
// - modify FOR's bytecode

// final bytecode will mostly be RPN expressions
// IF/FOR/whatever are all operators, maybe
// how are expressions separated?
// idk
// probably it will store them like
// <length><expression>...
// since nuls might exist within the expression

//char * code = "12345!=\"TEST\"  TEST TEST THE SAND CAN BE EATEN";
char * code = "FOR I=0 TO 10";

int main(){
	compile(code);
	return 0;
}