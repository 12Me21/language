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

// Multiple things may point to the same String or Table
struct String {
	char * pointer;
	uint32_t length;
	bool checked;
};

struct Variable;

struct Array {
	struct Variable * pointer;
	uint32_t length;
	bool checked;
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
		void * pointer; //
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
	//special constraint values:
	//0 = no constraint
	//-1 (max int) = constant
	//maybe add constant option for optimization?
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
	//errors:
	tkUnclosedString,
	tkInvalidChar,
};

struct Line {
	uint line;
	uint column;
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
	struct Line position_in_file;
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
