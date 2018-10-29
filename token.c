#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

enum Token_Type {
	tkEOF=0,
	tkNUMBER, tkSTRING,
	tkDOT,
	tkBRACKET_LEFT, tkBRACKET_RIGHT, tkPAREN_LEFT, tkPAREN_RIGHT,
	tkERROR,
	tkTILDE,
	tkNOT_EQUAL,
	tkLOGICAL_NOT,
};

enum Error_Type {
	erNONE=0,
	erUNCLOSED_STRING
};

char * TOKEN_NAME[] = {
	"EOF",
	"Number",
	"String",
	"Dot",
	"Left Bracket",
	"Right Bracket",
	"Left Paren",
	"Right Paren",
	"Error",
	"~",
	"!=",
	"!",
};

struct Token {
	enum Token_Type type;
	union {
		double number;
		char * pointer;
		enum Error_Type error;
	};
	//value can be:
	//-- double
	//-- pointer to string
	//-- pointer to name (no duplicates)
	//-- id (for keywords)
};

char * name_table[10000];

bool is_digit(char c){
	return c>='0' && c<='9';
}

#define single(chr, name) case chr: token->type=tk##name; return

void next_token(char * * code, struct Token * token){
	char c=*(*code);
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
			token->type=tkDOT;
			return;
		case '"':
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
		single('~',TILDE);
		case '!':
			if(c=='='){
				next();
				token->type=tkNOT_EQUAL;
				return;
			}
			token->type=tkLOGICAL_NOT;
			return;
		case '@':
		case '#':
		case '$':
		case '%':
		single('^',EXPONENT);
		single('&',BITWISE_AND);
		single('*',MULTIPLY);
		single('(',LEFT_PAREN);
		single(')',RIGHT_PAREN);
		single('-',MINUS);
		single('+',PLUS);
		case '=':
		case '[':
		case ']':
		case '{':
		case '}':
		case '|':
		case '\\':
		case ':':
		case ';':
		case '\'':
		case '<':
		case '>':
		case ',':
		case '?':
		case '/':
		default:
			
		}
	}
}

char * code = "12345!=\"TEST\"";

int main(){
	struct Token token;
	printf("Code: %s\n", code);
	while(1){
		next_token(&code, &token);
		printf("=== Token Type: %s\n", TOKEN_NAME[token.type]);
		switch(token.type){
		case tkEOF:
			return 0;
		case tkNUMBER:
			printf("number: %f\n", token.number);
		break;case tkSTRING:
			printf("string: %s\n", token.pointer);
		//break;case
		}
	}
	//eof:
	return 0;
}