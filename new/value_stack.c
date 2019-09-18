#include "types.h"

struct Value stack[256];
uint32_t stack_pointer = 0;

struct Value *get_stack_top_address(){
	return stack + stack_pointer + 1;
}

//Main stack functions
void push(struct Value value){
	if(stack_pointer >= ARRAYSIZE(stack))
		die("Stack Overflow\n");
	stack[stack_pointer++] = value;
}
struct Value pop(){
	if(stack_pointer <= 0)
		die("Internal Error: Stack Underflow (in pop)\n");
	return stack[--stack_pointer];
}
struct Value pop_no_lists(){
	struct Value a = pop();
	if(a.type == tNArgs)
		die("Illegal List\n");
	return a;
}
struct Value stack_get(int depth){
	if(stack_pointer-depth < 0)
		die("Internal Error: Stack Underflow (in get)\n");
	return stack[stack_pointer-depth];
}
struct Value list_get(uint index){
	if(stack_pointer+index >= ARRAYSIZE(stack))
		die("Internal Error: Stack Overflow (in get)\n");
	return stack[stack_pointer+index];
}
struct Value pop_type(enum Type type){
	struct Value a = pop();
	if(a.type != type)
		die("Wrong type\n");
	return a;
}
void replace(struct Value * value){
	stack[stack_pointer-1] = *value;
}
struct Value arg_get(uint index){
	if(stack_pointer+1+index >= ARRAYSIZE(stack))
		die("Internal Error: Stack Overflow (in get)\n");
	return stack[stack_pointer+1+index];
}

void stack_discard(int amount){
	if(stack_pointer-amount < 0)
		die("Internal Error: Stack Underflow (in discard)\n");
	stack_pointer -= amount;
}
//pop a value, and if it's a list, also discard the items in the list
//which can then be accessed with `list_get(index)`
struct Value pop_l(){
	if(stack_pointer <= 0)
		die("Internal Error: Stack Underflow (in pop_l)\n");
	struct Value a = stack[--stack_pointer];
	if(a.type==tNArgs)
		stack_discard(a.args);
	return a;
}
// struct Value * pop_l(){
	// if(stack_pointer <= 0)
		// die("Internal Error: Stack Underflow (in pop)\n");
	// struct Value * sp = stack + --stack_pointer;
	// if(sp->type==tNArgs)
		// stack_discard(sp->args);
	// return sp;
// }
