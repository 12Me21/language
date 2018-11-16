int priority[] = {
	oInvalid,
	
	oConstant,
	oVariable,
	
	//operators
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
	0,
	5,
	11,
	8,
	9,
	8,
	8,
	9,
	8,
	11,
	-1, //?
	//More operators
	oArray,
	oIndex,
	oCall,
	oDiscard,
	
	
	oPrint,
	oTest,
	oHalt,
	oTable, //table literal.
	//<key><value><key><value>...<oTable(# of items)>
	
	oScope,
	
	oReturn,
	oJump,
	oLogicalOr,
	oLogicalAnd,
	oLength,
	oJumpFalse,
	oJumpTrue,
	oFunction_Info,
	
	oReturn_None,
	
	-2, //parsing only
	//...
	
};

struct Token token;
bool read_next;

void next_t(){
	if(read_next)
		token = next_token();
	else
		read_next=true;
	//printf("token t %d\n",token.type);
}

bool read_token(int wanted_type){
	next_t();
	//printf("rt: %d %d\n",token.type,wanted_type);
	if(token.type==wanted_type){
		read_next=true;
		return true;
	}
	read_next=false;
	return false;
}

struct Item group_start = {.operator = oGroup_Start}; //set priority to -2

struct Item * output_stack;
uint output_stack_pointer = 0;

struct Item * output(struct Item item){
	output_stack[output_stack_pointer] = item;
	return &(output_stack[output_stack_pointer++]);
}

struct Item op_stack[256];
uint op_stack_pointer = 0;

void push_op(struct Item item){
	if(op_stack_pointer >= ARRAYSIZE(op_stack))
		parse_error("Operator Stack Overflow\n");
	op_stack[op_stack_pointer++] = item;
}
struct Item pop_op(){
	if(op_stack_pointer <= 0)
		parse_error("Internal Error: Stack Underflow\n");
	return op_stack[--op_stack_pointer];
}

void resurrect_op(){
	if(op_stack_pointer >= ARRAYSIZE(op_stack))
		parse_error("Operator Stack Overflow\n");
	op_stack_pointer++;
}

uint * p_level_stack[256];
uint scope_length = 0;
uint locals_length[256];

void p_push_scope(){
	if(scope_length >= ARRAYSIZE(p_level_stack))
		parse_error("Scope Stack Overflow\n");
	p_level_stack[scope_length] = malloc(256 * sizeof(uint));
	locals_length[scope_length++] = 0;
}

//note: the thing that the parser calls "scope" is called "level" in run.c

void discard_scope(){
	if(scope_length <= 0)
		parse_error("Internal Error: Scope Stack Underflow\n");
	scope_length--;
}

struct Item declare_variable(uint word){
	if(locals_length[scope_length-1] >= 256)
		parse_error("Local Variable Stack Overflow\n");
	p_level_stack[scope_length-1][locals_length[scope_length-1]] = word;
	return (struct Item){.operator = oVariable, .level = scope_length-1, .index = locals_length[scope_length-1]++};
}

struct Item make_var_item(uint word){
	uint i;
	//This will break if ssp1 is 0. Make sure the global scope is created at the beginning!
	//search for variable in existing scopes, starting from the current one and working down towards global.
	printf("scope length %d\n",scope_length);
	for(i = scope_length; i>0; i--){
		uint j;
		for(j = 0; j < locals_length[i-1]; j++){
			if(word == p_level_stack[i-1][j]){
				return (struct Item){.operator = oVariable, .level = i-1, .index = j};
			}
		}
	}
	//new variable that hasn't been used before:
	//todo: throw an error here and add a special VAR statement
	return declare_variable(word);
}

void flush_op_stack(enum Operator operator){
	//printf("flush op\n");
	while(op_stack_pointer){
		struct Item top = pop_op();
		if(priority[top.operator] >= priority[operator])
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

bool read_expression(){
	next_t();
	switch(token.type){
	//Values
	case tkValue:
		output((struct Item){.operator = oConstant, .value = token.value});
	//Variable
	break;case tkWord:
		output(make_var_item(token.word));
	break;case tkOperator_1: case tkOperator_12:
		flush_op_stack(token.operator_1);
		push_op((struct Item){.operator = token.operator_1});
		if(!read_expression())
			parse_error("Expected expression\n");
	break;case tkLeft_Paren:
		push_op(group_start);
		if(!read_expression())
			parse_error("Expected expression\n");
		if(!read_token(tkRight_Paren))
			parse_error("Expected `)`\n");
		flush_group();
	//array literal
	break;case tkLeft_Bracket:
		push_op(group_start);
		int length=0;
		if(read_expression()){
			length++;
			while(read_token(tkComma)){
				if(!read_expression())
					parse_error("Expected expression after comma\n");
				length++;
			}
		}
		if(!read_token(tkRight_Bracket))
			parse_error("Expected `]`\n");
		flush_group();
		output((struct Item){.operator = oArray, .length = length});
	break;case tkKeyword:
		if(token.keyword == kDef){
			printf("p def\n");
			//function def compiles to:
			//jump(@skip) @func functioninfo(level, locals, args) ... x return @skip
			struct Item * start = output((struct Item){.operator = oJump});
			Address start_pos = output_stack_pointer;
			struct Line start_line = real_line;
			//if(!read_token(tkWord))
			//	parse_error("Missing name in function definition\n");
			//declare_variable(token.word);
			if(!read_token(tkLeft_Paren))
				parse_error("Missing `(` in function definition\n");
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
						parse_error("Expected argument\n");
					declare_variable(token.word);
					args++;
				}
			}
			if(!read_token(tkRight_Paren))
				parse_error("Missing `)` in function definition\n");
			struct Item * function_info = output((struct Item){.operator = oFunction_Info, .level = level, .args = args});
			//read function body
			enum Keyword keyword;
			do{
				keyword = read_line();
			}while(!keyword);
			if(keyword!=kEnd)
				unexpected_end(keywords[keyword], keywords[kEnd], keywords[kDef], start_line);
			output((struct Item){.operator = oReturn_None});
			start->address = output_stack_pointer;
			function_info->locals = locals_length[scope_length-1];
			discard_scope();
			output((struct Item){.operator = oConstant, .value = {.type = tFunction, .builtin = false, .user_function = start_pos}});
		}else{
			return read_next = false;
		}
	break;default:
		return read_next = false;
	}
	//suffix/infix operators
	while(1){
		next_t();
		switch(token.type){
		//array/table index
		case tkLeft_Bracket:
			push_op(group_start);
			if(!read_expression())
				parse_error("Expected expression\n");
			if(!read_token(tkRight_Bracket))
				parse_error("Expected `]`\n");
			flush_group();
			output((struct Item){.operator = oIndex});
		//infix operator
		break;case tkOperator_2: case tkOperator_12:
			flush_op_stack(token.operator_2);
			push_op((struct Item){.operator = token.operator_2});
			if(!read_expression())
				parse_error("Expected expression\n");
		//function call
		break;case tkLeft_Paren:
			push_op(group_start);
			int length=0;
			if(read_expression()){
				length++;
				while(read_token(tkComma)){
					if(!read_expression())
						parse_error("Expected expression\n");
					length++;
				}
			}
			if(!read_token(tkRight_Paren))
				parse_error("Expected `)`\n");
			flush_group();
			output((struct Item){.operator = oConstant, .value = {.type = tNArgs, .args = length}});
			output((struct Item){.operator = oCall});
		break;default:
			read_next = false;
			return true;
		}
	}
}

bool read_full_expression(){
	push_op(group_start);
	if(read_expression()){
		flush_group();
		return true;
	}
	flush_group();
	return false;
}

enum Keyword read_line(){
	printf("parser line\n");
	if(read_full_expression()){
		output((struct Item){.operator = oDiscard});
	}else{
		next_t();
		switch(token.type){
		case tkSemicolon:
			
		break;case tkKeyword:
			switch(token.keyword){
			//WHILE
			case kWhile:;
				Address start_pos = output_stack_pointer;
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
				start->address = output_stack_pointer;
			
			break;case kIf:
				start_pos = output_stack_pointer;
				start_line = real_line;
				//read condition
				if(!read_full_expression())
					parse_error("Missing condition in `if`\n");
				
				start = output((struct Item){.operator = oJumpFalse});
				//read THEN
				next_t();
				if(!(token.type == tkKeyword && token.keyword == kThen))
					parse_error("Missing `then` after `if`\n");
				//Read code inside IF block
				do{
					keyword = read_line();
				}while(!keyword);
				
				if(keyword == kEndif){
					start->address = output_stack_pointer;
				}else if(keyword == kElseif){
					//you need to do lots of stuff here
					parse_error("UNSUPPORTED\n");
				}else if(keyword == kElse){
					
					parse_error("UNSUPPORTED\n");
				}else{
					unexpected_end(keywords[keyword], "Endif/`Elseif`/`Else", keywords[kIf], start_line);
				}
			break;case kRepeat:
				start_pos = output_stack_pointer;
				start_line = real_line;
				do{
					keyword = read_line();
				}while(!keyword);
				if(keyword!=kUntil)
					unexpected_end(keywords[keyword], keywords[kUntil], keywords[kRepeat], start_line);
				if(!read_full_expression())
					parse_error("Missing condition in `until`\n");
				output((struct Item){.operator = oJumpFalse, .address = start_pos});
			break;case kDef:
				//function def compiles to:
				//jump(@skip) @func functioninfo(level, locals, args) ... x return @skip
				parse_error("coming soon\n");
				start_pos = output_stack_pointer;
				start = output((struct Item){.operator = oJump});
				start_line = real_line;
				if(!read_token(tkWord))
					parse_error("Missing name in function definition\n");
				declare_variable(token.word);
				if(!read_token(tkLeft_Paren))
					parse_error("Missing `(` in function definition\n");
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
							parse_error("Expected argument\n");
						declare_variable(token.word);
						args++;
					}
				}
				if(!read_token(tkRight_Paren))
					parse_error("Missing `)` in function definition\n");
				struct Item * function_info = output((struct Item){.operator = oFunction_Info, .level = level, .args = args});
				//read function body
				do{
					keyword = read_line();
				}while(!keyword);
				if(keyword!=kEnd)
					unexpected_end(keywords[keyword], keywords[kEnd], keywords[kDef], start_line);
				output((struct Item){.operator = oReturn_None});
				start->address = output_stack_pointer;
				function_info->locals = locals_length[scope_length-1];
				discard_scope();
			break;case kEndif:case kWend:case kElse:case kElseif:case kUntil:case kEnd:
				//"End" tokens
				return token.keyword;
				//if(token.keyword != expect_end)
				//	parse_error("expected keyword, got a different keyword\n");
				//return false;
			break;default:
				parse_error("Unsupported token\n");
			}
		break;default:
			//if(expect_end)
			//	parse_error("expected keyword, got EOF\n");
			return -1;
		}
	}
	printf("line finished\n");
	return 0;
}

struct Item * parse(FILE * stream){
	//printf("Parser started\n");
	output_stack = malloc(sizeof(struct Item) * 65536);
	
	init(stream);
	read_next=true;
	p_push_scope();
	
	output((struct Item){.operator = oScope});
	
	//printf("Parser started (for real)\n");
	while(read_line()!=-1);
	
	if(scope_length!=1)
		parse_error("internal error: scope mistake\n");
	//printf("Parser finished\n");
	printf("%d global variables\n", locals_length[0]);
	output((struct Item){.operator = oHalt});
	output_stack[0].locals = locals_length[0];
	return output_stack;
}