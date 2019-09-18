//this contains structs and enums for types, values, operators, "bytecode" items, etc.
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <setjmp.h>
#include <string.h>
#include <math.h>
#include <ctype.h>

extern jmp_buf err_ret;
#define die(...) {printf(__VA_ARGS__); longjmp(err_ret, 2);}
#define ARRAYSIZE(a) (sizeof(a)/sizeof(a[0]))

typedef uint32_t uint;

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

void record_alloc(struct Value * value);

struct Entry {
	struct Variable variable;
	size_t key_size;
	char * key;
	unsigned char key_type;
	struct Entry * next;
};

//rather than storing first and last, just store first, and insert new elements at the start of the linked list
//this would break support for iteration though ...
struct Table {
	struct Entry * first;
	struct Entry * last;
	uint length;
	bool checked;
}; //table_new = {.first = NULL, .last = NULL, /*.references = 0*/};

void flush_op_stack(enum Operator);
void flush_op_stack_1(enum Operator);
void flush_group();

void push_op(struct Item item);
struct Item pop_op();
void resurrect_op();

struct Item * output(struct Item item);

void parse_error_1();
void parse_error_2();

#define parse_error(...) (parse_error_1(), printf(__VA_ARGS__), parse_error_2())

char * token_name_2(struct Token token);

// value.c
bool truthy(struct Value value);
struct Value make_boolean(bool boolean);
bool equal(struct Value a, struct Value b);
int compare(struct Value a, struct Value b);
uint to_string(struct Value value, char * output);
void basic_print(struct Value value);

//builtins

struct Value list_get(uint index);
char * variable_pointer_to_name(struct Variable * variable);

void assign_variable(struct Variable * variable, struct Value value);

// Stack
struct Value arg_get(uint index);
void push(struct Value value);
struct Value pop_type(enum Type type);
struct Value pop_no_lists();
struct Value pop_l();
struct Value pop();
struct Value stack_get(int depth);
struct Value *get_stack_top_address();
void stack_discard(int amount);

//Table
struct Entry * table_get(struct Table * table, struct Value key, bool add);
unsigned int table_length(struct Table * table);
struct Variable * table_declare(struct Table * table, struct Value key, struct Value value);
struct Value table_lookup(struct Table * table, struct Value key);

//Parser
struct Item * parse(FILE * stream, char * string);
struct Line get_line(Address address);
