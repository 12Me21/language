struct Token token;
bool read_next;

void next_t(){
	if(read_next)
		token = next_token();
	else
		read_next=true;
	//printf("token t %d\n",token.type);
}

//try to read a specific token type
//returns true if successful.
bool read_token(enum Token_Type wanted_type){
	next_t();
	//printf("rt: %d %d\n",token.type,wanted_type);
	if(token.type==wanted_type){
		read_next=true;
		return true;
	}
	read_next=false;
	return false;
}

// basically, (
struct Item group_start = {.operator = oGroup_Start}; //set priority to -2

struct Item * output_stack;
//output_length is defined in token.c for some reason
struct Item * output(struct Item item){
	//todo:
	//if item is operator:
	//1: check if operator is a constant operator (one that always has the same result for the same inputs)
	//2: check if all of the inputs are constants
	//3: evaluate
	//optimization!!!
	output_stack[output_length] = item;
	return &(output_stack[output_length++]);
}

// Operator stack
// This stores operators temporarily during parsing (using shunting yard algorithm)
struct Item op_stack[256];
uint op_length = 0;
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

//Scope (level) stack
//This keeps track of local variables during parsing.
uint * p_level_stack[256]; //256 = # of levels
uint scope_length = 0;
uint locals_length[256]; //256 = # of local variables per scope

void p_push_scope(){
	if(scope_length >= ARRAYSIZE(p_level_stack))
		parse_error("Level stack overflow. (Too many nested functions)\n");
	p_level_stack[scope_length] = malloc(256 * sizeof(uint));
	locals_length[scope_length++] = 0;
}
void discard_scope(){
	if(scope_length <= 0)
		parse_error("Internal Error: Scope Stack Underflow\n");
	free(p_level_stack[--scope_length]);
}

// char * property_names[65536];
// uint property_names_length = 0;
// uint register_property_name(uint word){
	// uint i;
	// for(i=0;i<property_names_length;i++)
		// if(property_names[i]==word)
			// return i;
	// if(property_names_length >= ARRAYSIZE(property_names))
		// parse_error("Too many property names");
	// property_names[property_names_length] = word;
	// return property_names_length++;
// }

//note: the thing that the parser calls "scope" is called "level" in run.c

struct Item declare_variable(uint word){
	//printf("declare var: %d at level %d\n", word, scope_length);
	if(locals_length[scope_length-1] >= 256)
		parse_error("Local Variable Stack Overflow\n");
	p_level_stack[scope_length-1][locals_length[scope_length-1]] = word;
	return (struct Item){.operator = oVariable, .level = scope_length-1, .index = locals_length[scope_length-1]++};
}

struct Item make_var_item(uint word){
	uint i;
	//printf("found var: %d\n", word);
	//This will break if ssp1 is 0. Make sure the global scope is created at the beginning!
	//search for variable in existing scopes, starting from the current one and working down towards global.
	//printf("scope length %d\n",scope_length);
	for(i = scope_length; i>0; i--){
		uint j;
		for(j = 0; j < locals_length[i-1]; j++){
			if(word == p_level_stack[i-1][j]){
				return (struct Item){.operator = oVariable, .level = i-1, .index = j};
			}
		}
	}
	//new variable that hasn't been used before:
	parse_error("Undefined variable: '%s' (Use `var`)\n", name_table[word]); //remove this to disable "OPTION STRICT"
	return declare_variable(word);
}

void flush_op_stack(int pri){
	//printf("flush op\n");
	while(op_length){
		struct Item top = pop_op();
		if(priority[top.operator] >= pri) //might need to use >= for binary ops idk?
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

enum Keyword read_line();

void expected(char * expected){
	parse_error("Expected %s, got `%s`\n", expected, token_name_2(token));
}

bool read_expression(bool allow_comma){
	next_t();
	switch(token.type){
	// Value
	case tkValue:
		output((struct Item){.operator = oConstant, .value = token.value});
	// Variable
	break;case tkWord:
		output(make_var_item(token.word));
	// Prefix operator
	break;case tkOperator_1: case tkOperator_12:
		flush_op_stack(priority[token.operator_1]+1); //adding 1 here is a dangerous hack to get like, left associativity or something
		//make sure prefix ops have priorities which are spread out so this won't break too much.
		
		push_op((struct Item){.operator = token.operator_1});
		if(!read_expression(true))
			expected("expression");
	// Group
	break;case tkLeft_Paren:
		push_op(group_start);
		if(!read_expression(true))
			output((struct Item){.operator = oConstant, .value = {.type = tNArgs, .args = 0}});
		if(!read_token(tkRight_Paren))
			expected("`)`");
		flush_group();
	// Array literal
	//input: [ list ]
	//output: <list> <oArray>
	break;case tkLeft_Bracket:
		push_op(group_start);
		read_token(tkLine_Break);
		if(!read_expression(true))
			output((struct Item){.operator = oConstant, .value = {.type = tNArgs, .args = 0}});
		read_token(tkLine_Break);
		if(!read_token(tkRight_Bracket))
			expected("`]`");
		flush_group();
		output((struct Item){.operator = oArray});
	// Table literal
	//input: { key = value , ... }
	//output: <key> <value> ... <oTable>
	break;case tkLeft_Brace:
		read_token(tkLine_Break);
		uint length = 0;
		push_op(group_start);
		if(read_expression(false)){
			length++;
			flush_group();
			if(!read_token(tkAssign))
				expected("`=`");
			push_op(group_start);
			if(!read_expression(false))
				expected("value");
			flush_group();
			while(read_token(tkComma)){
				push_op(group_start);
				read_expression(false);
				
				length++;
				flush_group();
				if(!read_token(tkAssign))
					expected("`=`");
				push_op(group_start);
				if(!read_expression(false))
					expected("value");
				flush_group();
			}
		}else
			flush_group();
		if(!read_token(tkRight_Brace))
			expected("`}`");
		output((struct Item){.operator = oTable, .length = length});
	break;case tkAt:
		output((struct Item){.operator = oAt/*meal*/});
	break;case tkKeyword:
		if(token.keyword == kDef){
			struct Item * start = output((struct Item){.operator = oJump});
			Address start_pos = output_length;
			struct Line start_line = real_line;
			if(!read_token(tkLeft_Paren))
				expected("`(` in function definition");
			//create scope
			p_push_scope();
			uint level = scope_length-1;
			//read argument list
			uint args=0;
			if(read_token(tkWord)){
				declare_variable(token.word);
				args++;
				while(read_token(tkComma)){
					if(!read_token(tkWord))
						expected("function parameter name");
					declare_variable(token.word);
					args++;
				}
			}
			if(!read_token(tkRight_Paren))
				expected("`)` in function definition");
			struct Item * function_info = output((struct Item){.operator = oFunction_Info, .level = level, .args = args});
			//read function body
			enum Keyword keyword;
			do{
				keyword = read_line();
			}while(!keyword);
			if(keyword!=kEnd)
				unexpected_end(keywords[keyword], keywords[kEnd], keywords[kDef], start_line);
			output((struct Item){.operator = oReturn_None});
			start->address = output_length;
			function_info->locals = locals_length[scope_length-1];
			discard_scope();
			output((struct Item){.operator = oConstant, .value = {.type = tFunction, .builtin = false, .user_function = start_pos}});
		}else{
			return read_next = false;
		}
	break;default:
		return read_next = false;
	}
	//This handles operators that can appear at the "end" of an expression
	//and might continue the expression.
	//for example <expr> + <expr2> or <expr>[index]
	while(1){
		next_t();
		switch(token.type){
		// Index
		case tkLeft_Bracket:
			push_op(group_start);
			if(!read_expression(true))
				expected("expression");
			if(!read_token(tkRight_Bracket))
				expected("`]`");
			flush_group();
			output((struct Item){.operator = oIndex});
		break;
		// Infix operator
		case tkComma:
			if(!allow_comma){
				read_next = false;
				return true;
			}
			flush_op_stack(priority[token.operator_2]);
			push_op((struct Item){.operator = token.operator_2});
			read_token(tkLine_Break);
			if(!read_expression(true))
				pop_op();
		break;case tkOperator_2: case tkOperator_12: //no break
			//don't allow commas. This is for reading table literals where commas are used to separate values.
			//(maybe a better idea would be to use a different symbol but whatever)
			flush_op_stack(priority[token.operator_2]);
			push_op((struct Item){.operator = token.operator_2});
			read_token(tkLine_Break); //this is one case where you're allowed to have line breaks in an expression
			//after a binary infix operator
			//1+1 can be 1+ \n 2
			//mainly useful for things like array literals etc.
			//maybe also allow line breaks after [ and before ] ?
			if(!read_expression(true))
				expected("expression");
		// logical "or" and "and"
		break;case tkOr:;
			struct Item * start = output((struct Item){.operator = oLogicalOr});
			flush_op_stack(priority[oLogicalOr]);
			read_token(tkLine_Break);
			if(!read_expression(true))
				expected("expression");
			start->address = output_length;
		//break;case tkAnd:
			
		// Function call
		break;case tkLeft_Paren:
			push_op(group_start);
			//read arguments
			if(!read_expression(true))
				//handle () 0 arguments
				output((struct Item){.operator = oConstant, .value = {.type = tNArgs, .args = 0}});
				//if there is 1 arg, oCall will see something other than tNArgs on the stack.
			if(!read_token(tkRight_Paren))
				expected("`)`");
			flush_group();
			output((struct Item){.operator = oCall});
		// .
		// break;case tkDot:
			// edit this to allow keywords after . maybe
			// if(!read_token(tkWord))
				// expected("Property Name");
			// push_property_name(name_table[token.word]);
			// push_op((struct Item){.operator = oProperty, .property = property_names_length-1});
		break;default:
			read_next = false;
			return true;
		}
	}
}

bool read_full_expression(){
	push_op(group_start);
	if(read_expression(true)){
		flush_group();
		return true;
	}
	flush_group();
	return false;
}

enum Keyword read_line(){
	//printf("parser line\n");
	if(read_full_expression()){
		if(read_token(tkAssign)){
			//optimize: only insert when @ is actually used in the value expression.
			output((struct Item){.operator = oSet_At});
			read_full_expression();
			output((struct Item){.operator = oAssign_Discard});
		}else
			output((struct Item){.operator = oDiscard});
	}else{
		next_t();
		switch(token.type){
		case tkLine_Break: case tkSemicolon:
			
		break;case tkKeyword:
			switch(token.keyword){
			//WHILE
			case kWhile:;
				Address start_pos = output_length;
				struct Line start_line = real_line;
				if(!read_full_expression())
					parse_error("Missing condition in `while`\n");
				struct Item * start = output((struct Item){.operator = oJumpFalse});
				enum Keyword keyword;
				do{
					keyword = read_line();
				}while(!keyword);
				if(keyword!=kWend)
					unexpected_end(keywords[keyword], keywords[kWend], keywords[kWhile], start_line);
				output((struct Item){.operator = oJump, .address = start_pos});
				start->address = output_length;
			// IF
			break;case kIf:
				start_line = real_line;
				//read condition
				if(!read_full_expression())
					parse_error("Missing condition in `if`\n");
				
				start = output((struct Item){.operator = oJumpFalse});
				//read THEN
				next_t();
				if(!(token.type == tkKeyword && token.keyword == kThen))
					expected("`then`");
					//parse_error("Missing `then` after `if`\n");
				//Read code inside IF block
				do{
					keyword = read_line();
				}while(!keyword);
				
				if(keyword == kEndif){
					start->address = output_length;
				}else if(keyword == kElseif){
					//you need to do lots of stuff here
					parse_error("ELSEIF UNSUPPORTED\n");
				}else if(keyword == kElse){
					
					parse_error("ELSE UNSUPPORTED\n");
				}else{
					unexpected_end(keywords[keyword], "Endif/`Elseif`/`Else", keywords[kIf], start_line);
				}
			// repeat
			break;case kRepeat:
				start_pos = output_length;
				start_line = real_line;
				do{
					keyword = read_line();
				}while(!keyword);
				if(keyword!=kUntil)
					unexpected_end(keywords[keyword], keywords[kUntil], keywords[kRepeat], start_line);
				if(!read_full_expression())
					parse_error("Missing condition in `until`\n");
				output((struct Item){.operator = oJumpFalse, .address = start_pos});
			// def
			break;case kDef:
				//function def compiles to:
				//jump(@skip) @func functioninfo(level, locals, args) ... x return @skip
				parse_error("coming soon\n");
			//return
			break;case kReturn:
				if(read_full_expression())
					output((struct Item){.operator = oReturn});
				else
					output((struct Item){.operator = oReturn_None});
			// var
			break;case kVar:
				if(!read_token(tkWord))
					parse_error("Missing name in variable declaration\n");
				struct Item variable = declare_variable(token.word);
				if(read_token(tkLeft_Brace)){
					start = output((struct Item){.operator = oJump});
					Address start_pos = output_length;
					if(!read_full_expression())
						parse_error("Missing constraint expression\n");
					output((struct Item){.operator = oConstrain_End});
					start->address = output_length;
					//todo: store `start`... somewhere...
					if(!read_token(tkRight_Brace))
						parse_error("missing `}`");
					output(variable);
					output((struct Item){.operator = oConstrain, .address = start_pos});
				}
				if(read_token(tkAssign)){
					output(variable);
					if(!read_full_expression())
						parse_error("Missing initialization expression\n");
					output((struct Item){.operator = oAssign_Discard});
				}else{
					read_next = false;
				}
			//"End" tokens
			break;case kEndif:case kWend:case kElse:case kElseif:case kUntil:case kEnd:
				return token.keyword;
			break;default:
				parse_error("Unsupported token\n");
			}
		break;default:
			return -1;
		}
	}
	return 0;
}

// void parse_function(){
	
// }

//parser/tokenizer can take either stream or string as input
//This is an ugly hack but I need to use it because my C compiler doesn't have `fmemopen`
struct Item * parse(FILE * stream, char * string){
	output_stack = malloc(sizeof(struct Item) * 65536);
	
	if(stream)
		init_stream(stream);
	else
		init_string(string);
	read_next=true;
	p_push_scope();
	//declare builtin variables/function
	uint i;
	for(i=0;i<ARRAYSIZE(builtins);i++){
		declare_variable(i);
	}
	
	output((struct Item){.operator = oInit_Global});
	
	while(read_line()!=-1);
	
	if(scope_length!=1)
		parse_error("internal error: scope mistake\n");
	
	output((struct Item){.operator = oHalt});
	output_stack[0].locals = locals_length[0];
	return output_stack;
}

//parsing tables:
//1: read key expression
//2: read : (or maybe a different separator symbol?)
//3: read value expression (don't allow top level commas)
//4: repeat
//5: push table literal operator
//(how to do constraints though?? maybe there is a more consistant way to implement them for regular variables, arrays, and tables)

//make = a statement rather than an expression
//so that if x=1 then will throw an error during parsing!
//when = is encountered in an expression, finish that expression and then read another expression and push the assign operator

//idea: var <variable_name> declares a variable.
//have an operator that applies a constraint, and after var, go back and read <variable_name> as a line of code to run the constraint and assignment.

//usually {a = 3} sets x["a"] or x.a, and {["a"]=3} sets the same thing
//change that maybe...
//used by JS and Lua

//array[start LENGTH length] -> new array which points to part of the old array.
//might be dangerous if the original array is resized or reallocated
//also might ruin garbage collection
//...
//the sub-array must hold a reference back to the main array, so that the main array's reference count can be modified too
//and to determine if the main array has gotten too small...
//this isn't worth doing, really.
//:(