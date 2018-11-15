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
	oMultiAssign,
	oJump,
	oLogicalOr,
	oLogicalAnd,
	oLength,
	oJumpFalse,
	oJumpTrue,
	oFunctionInfo,
	
	-2, //parsing only
	//...
	
};

struct Item * parse(FILE * stream){
	//printf("Parser started\n");
	init(stream);
	struct Token token;
	bool read_next=true;
	
	void next(){
		if(read_next)
			token = next_token();
		else
			read_next=true;
		//printf("token t %d\n",token.type);
	}
	
	bool read_token(int wanted_type){
		next();
		//printf("rt: %d %d\n",token.type,wanted_type);
		if(token.type==wanted_type){
			read_next=true;
			return true;
		}
		read_next=false;
		return false;
	}
	
	struct Item group_start = {.operator = oGroup_Start}; //set priority to -2
	
	struct Item * output_stack = malloc(sizeof(struct Item) * 65536);
	uint output_stack_pointer = 0;
	
	struct Item * output(struct Item item){
		output_stack[output_stack_pointer] = item;
		return &(output_stack[output_stack_pointer++]);
	}
	
	struct Item op_stack[256];
	uint op_stack_pointer = 0;
	
	void push_op(struct Item item){
		if(op_stack_pointer >= ARRAYSIZE(op_stack))
			parse_error("Stack Overflow\n");
		op_stack[op_stack_pointer++] = item;
	}
	struct Item pop_op(){
		if(op_stack_pointer <= 0)
			parse_error("Internal Error: Stack Underflow\n");
		return op_stack[--op_stack_pointer];
	}
	
	void resurrect_op(){
		if(op_stack_pointer >= ARRAYSIZE(op_stack))
			parse_error("Stack Overflow\n");
		op_stack_pointer++;
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
	
	int globals = 0; //temp
	
	struct Item make_var_item(int word){
		if(word >= globals)
			globals = word+1;
		return (struct Item){.operator = oVariable, .scope = 0, .index = word};
	}
	
	void flush_group(){
		struct Item item;
		while((item = pop_op()).operator != oGroup_Start)
			output(item);
	}
	
	output((struct Item){.operator = oScope});
	
	bool read_expression(){
		next();
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
		break;default:
			return read_next = false;
		}
		//suffix/infix operators
		while(1){
			next();
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
		if(read_expression()){
			while(op_stack_pointer){
				struct Item item = pop_op();
				if(item.operator == oGroup_Start)
					parse_error("Internal error: `(` in operator stack after parsing expression\n");
				output(item);
			}
			return true;
		}
		return false;
	}
	
	enum Keyword read_line(){
		//printf("parser line\n");
		if(read_full_expression()){
			output((struct Item){.operator = oDiscard});
		}else{
			next();
			switch(token.type){
			case tkSemicolon:
				
			break;case tkKeyword:
				switch(token.keyword){
				//WHILE
				case kWhile:;
					Address start_pos = output_stack_pointer;
					struct Line start_line = real_line;
					if(!read_full_expression())
						parse_error("Missing condition in WHILE\n");
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
						parse_error("Missing condition in WHILE\n");
					
					start = output((struct Item){.operator = oJumpFalse});
					//read THEN
					next();
					if(token.type == tkKeyword && token.keyword == kThen)
						parse_error("Missing `then` after `if`\n");
					//Read code inside IF block
					do{
						keyword = read_line();
					}while(!keyword);
					
					if(keyword == kEndif){
						start->address = output_stack_pointer;
					}else if(keyword == kElseif){
						parse_error("UNSUPPORTED\n");
					}else if(keyword == kElse){
						parse_error("UNSUPPORTED\n");
					}else{
						unexpected_end(keywords[keyword], "Endif/`Elseif`/`Else", keywords[kIf], start_line);
					}
				break;case kEndif:case kWend:case kElse:case kElseif:
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
		return 0;
	}
	
	
	//printf("Parser started (for real)\n");
	while(read_line()!=-1);
	//printf("Parser finished\n");
	output((struct Item){.operator = oHalt});
	output_stack[0].locals = globals;
	return output_stack;
}