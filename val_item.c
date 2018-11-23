//this contains structs and enums for types, values, operators, "bytecode" items, etc.

typedef uint32_t Address; //location in the bytecode

enum Type {
	tNone,
	tNumber,
	tString,
	tTable,
	tArray,
	tFunction,
	tBoolean,
	tNArgs, //there could be a "list" type perhaps. implemented as <items ...> <# of items>
};

char * type_name[] = { "None", "Number", "String", "Table", "Array", "Function", "Boolean", "(List)" };

// Multiple things may point to the same String or Table
struct String {
	char * pointer;
	uint32_t length;
	//int references;
};

struct Variable;

struct Array {
	struct Variable * pointer;
	uint32_t length;
};

// A value or variable
// Variables just contain the extra "constraint_expression" field
// Multiple things should not point to the same Value or Variable. EVER. ?
struct Value {
	enum Type type;
	union {
		double number;
		struct String * string;
		struct Table * table;
		struct Array * array;
		struct {
			union {
				Address user_function;
				void (*c_function)(uint);
			};
			bool builtin;
		};
		bool boolean;
		int args;
	};
	struct Variable * variable;
};

struct Variable {
	struct Value value;
	Address constraint_expression;
};

// every distinct operator
// subtraction and negation are different operators
// functions are called using the "call function" operator
// which takes as input the function pointer and number of arguments
// <function ptr> <args ...> <# of args> CALL
enum Operator {
	oInvalid,//0
	
	oConstant,
	oVariable,
	
	//operators
	oBitwise_Not,
	oBitwise_Xor,
	oNot_Equal,
	oNot,
	oMod,
	oExponent,
	oBitwise_And,
	oMultiply,//10
	oNegative,
	oSubtract,
	oAdd,
	oEqual,
	oBitwise_Or,
	oFloor_Divide,
	oLess_Or_Equal,
	oLeft_Shift,
	oLess,
	oGreater_Or_Equal,//20
	oRight_Shift,
	oGreater,
	oDivide,
	//More operators
	oArray,
	oIndex,
	oCall,
	oDiscard,
	
	oPrint,
	oHalt,
	oTable, //30 //table literal.
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
	
	oReturn_None, //40
	
	oGroup_Start, //parsing only
	
	oAssign_Discard,
	oConstrain,
	oConstrain_End,
	oAt,
	oComma,
	oSet_At,
	oProperty, //48
	oIn,
};

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

//this takes up a lot of space (at least 40 bytes I think)... perhaps this should just be <less awful>
struct Item {
	enum Operator operator;
	union {
		struct Value value;
		Address address; //index in bytecode
		 //used by variable and functioninfo
		struct {
			uint level; //level of var or func
			union {
				struct {
					uint index; //index of var
					//Address constraint 'put constraint address here? but that would break table/array things
				};
				struct {
					uint locals; //# of local variables in a scope
					uint args; //# of inputs to a function
				};
			};
		}; //simplify this ^
		uint length; //generic length
	};
};

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
	tkOr,
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

enum Keyword { //reference to keywords list
	kIf = 0, kThen, kElse, kElseif, kEndif,
	kFor, kNext,
	kWhile, kWend,
	kRepeat, kUntil,
	kVar,
	kDef, kReturn, kEnd,
	//...
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
	if(value.type == tNArgs)
		die("internal error\n");
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
			return a.table == b.table;
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

