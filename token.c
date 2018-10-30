#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

enum Token_Type {
	tkEOF=256,
	tkNUMBER,
	tkSTRING,
	tkERROR,
	tkNOT_EQUAL,
	tkCOMPARE,
	tkLESS_OR_EQUAL,
	tkGREATER_OR_EQUAL,
	tkLEFT_SHIFT,
	tkRIGHT_SHIFT,
	tkWORD,
	
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


enum Error_Type {
	erNONE=0,
	erUNCLOSED_STRING
};

char * TOKEN_NAME[] = {
	"EOF",
	"Number",
	"String",
	"Error",
	"!=",
	"==",
	"<=",
	">=",
	"<<",
	">>",
	"word",
	"keyword",
};

struct Token {
	enum Token_Type type;
	union {
		double number;
		char * pointer;
		enum Error_Type error;
		int id;
	};
	//value can be:
	//-- double
	//-- pointer to string
	//-- id (for keywords/names)
};

//change this into one big char array (more efficient)
char * name_table[10000] = {
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
			token->type='.';
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
				token->type=tkNOT_EQUAL;
				return;
			}
			token->type='!';
			return;
		case '@':
		case '#':
		case '$':
		case '=':
			if(c=='='){
				next();
				token->type=tkCOMPARE;
				return;
			}
			token->type='=';
			return;
		case '\'':
			while(c && c!='\n')
				next();
			// (don't return here, read another token)
		case '<':
			if(c=='<'){
				next();
				token->type=tkLEFT_SHIFT;
				return;
			}
			if(c=='='){
				next();
				token->type=tkLESS_OR_EQUAL;
				return;
			}
			token->type='<';
			return;
		case '>':
			if(c=='>'){
				next();
				token->type=tkRIGHT_SHIFT;
				return;
			}
			if(c=='='){
				next();
				token->type=tkGREATER_OR_EQUAL;
				return;
			}
			token->type='>';
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
		}
	}
}

void compile(char * code){
	init(code);
	struct Token token;
	printf("Code: %s\n\n",code);
	while(1){
		next_token(&code, &token);
		if(token.type>=tkKEYWORD){
			printf("=== Token Type: Keyword: %s\n", name_table[token.type-tkKEYWORD]);
		}else if(token.type>=256){
			printf("=== Token Type: %s\n", TOKEN_NAME[token.type-256]);
			switch(token.type){
			case tkEOF:
				return;
			case tkNUMBER:
				printf("number: %f\n", token.number);
			break;case tkSTRING:
				printf("string: %s\n", token.pointer);
			break;case tkWORD:
				printf("Word %d: %s\n", token.id, name_table[token.id]);
			}
		}else{
			printf("=== Token Type: %c\n", token.type);
		}
	}
}

//char * code = "12345!=\"TEST\"  TEST TEST THE SAND CAN BE EATEN";
char * code = "FOR I=0 TO 10";

int main(){
	compile(code);
	return 0;
}