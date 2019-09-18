#include "types.h"

// Operator stack
// This stores operators temporarily during parsing (using shunting yard algorithm)
struct Item op_stack[256];
uint op_length = 0;

//higher number = first
// o... values are 'fake' operators that don't have a priority
int priority[] = {
	oInvalid,
	oConstant,
	oVariable,
	// "Real" operators
	99, // ~x
	4, // x ~ y
	7, // !=
	99, // !x
	11, // %
	12, // ^
	6, // &
	11, // *
	99, // -x
	10, // x - y
	10, // +
	7, // ==
	5, // |
	11, // floor div
	8, // <=
	9, // <<
	8, // <
	8, // >=
	9, // >>
	8, // >
	11, // /
	// More operators
	oArray, oIndex, oCall, oDiscard,
	
	-66, // print
	oHalt, oTable, //table literal.
	//<key><value><key><value>...<oTable(# of items)>
	
	oInit_Global, oReturn, oJump, oLogicalOr, oLogicalAnd, oLength, oJumpFalse,
	oJumpTrue, oFunction_Info, oReturn_None,
	
	-99, //parsing only
	//...
	oAssign_Discard, oConstrain, oConstrain_End, oAt,
	-2, //comma
	0, //set_@
	100, //property access
	-4,//in
};

void flush_op_stack(enum Operator op){
	int pri = priority[op];
	while(op_length){
		struct Item top = pop_op();
		if(priority[top.operator] >= pri)
			output(top);
		else{
			resurrect_op();
			break;
		}
	}
}

void flush_op_stack_1(enum Operator op){
	int pri = priority[op];
	while(op_length){
		struct Item top = pop_op();
		if(priority[top.operator] > pri)
			output(top);
		else{
			resurrect_op();
			break;
		}
	}
}

void flush_group(){
	struct Item item;
	while((item = pop_op()).operator != oGroup_Start)
		output(item);
}

void push_op(struct Item item){
	if(op_length >= ARRAYSIZE(op_stack))
		parse_error("Operator Stack Overflow\n");
	op_stack[op_length++] = item;
}
struct Item pop_op(){
	if(op_length <= 0)
		parse_error("Internal Error: Stack Underflow\n");
	return op_stack[--op_length];
}
//Return the previously popped value to the stack
void resurrect_op(){
	if(op_length >= ARRAYSIZE(op_stack))
		parse_error("Operator Stack Overflow\n");
	op_length++;
}
