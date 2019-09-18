#include "types.c"

extern char *keywords[15];

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

char * operator_name[] = {
	"Invalid Operator", "Constant", "Variable", "~", "~", "!=", "!", "%", "^", "&", "*", "-", "-", "+", "==", "=", "|",
	"\\", "<=", "<<", "<", ">=", ">>", ">", "/", "Array Literal", "Index", "Call", "Discard", "?", "Halt",
	"Table Literal", "Global Variables", "Jump", "or", "and", "Length", "Jump if false", "Jump if true",
	"Function Info", "Return None", "Group Start", "Assign Discard", "Constrain", "Constrain End", "At", ",",	
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
