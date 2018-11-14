int priority[] = {
	//...
	
};

struct Item * parse(FILE * stream){
	init(stream);
	struct Token token;
	bool read_next=true;
	
	void next(){
		if(read_next)
			token = next_token();
		else
			read_next=true;
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
		while(1){
			struct Item top = pop_op();
			if(priority[top.operator] >= priority[operator])
				output(top)
			else{
				resurrect_op();
				return;
			}
		}
	}
		
	void flush_op_stack(){
		struct Item item
		while((item = pop_op()) != group_start)
			output(top)
	}
	
	bool read_expression(){
		next();
		switch(token.type){
		//Values
		case tkValue:
			push((struct Item){.operator = oConstant, .value = token.value});
		//Variable
		break;case tkWord:
			output(make_var_item(token.word));
		break;case tkOperator_1: case tkOperator_12:
			flush_op_stack(token.operator_1);
			push_op((struct Item){.operator = token.operator_1});
			read_expression() || parse_error("Expected expression\n");
		break;case tkLeft_Paren:
			push_op(group_start);
			read_expression() || parse_error("Expected expression\n");
			read_token(tkRight_Paren) || parse_error("Expected `)`\n");
			flush_op_stack();
		//array literal
		break;case tkLeft_Bracket:
			push_op(group_start);
			int length=0;
			if(read_expression()){
				length++;
				while(read_token(tkCOMMA)){
					read_subexpression() || parse_error("Expected expression\n");
					length++;
				}
			}
			read_token(tkRight_Bracket) || parse_error("Expected `]`\n");
			flush_op_stack();
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
				read_expression() || parse_error("Expected expression\n");
				read_token(tkRight_Bracket) || parse_error("Expected `]`\n");
				flush_op_stack();
				push((struct Item){.operator = oIndex});
			//infix operator
			break;case tkOperator_2: case tkOperator_12:
				flush_op_stack(token.operator_2);
				push_op((struct Item){.operator = token.operator_2});
				read_expression() || parse_error("Expected expression\n");
			//function call
			break;case tkLeft_Paren:
				push_op(group_start);
				int length=0;
				if(read_expression()){
					length++;
					while(read_token(tkCOMMA)){
						read_subexpression() || parse_error("Expected expression\n");
						length++;
					}
				}
				read_token(tkRight_Paren) || parse_error("Expected `)`\n");
				flush_op_stack();
				output((struct Item){.operator = oConstant, .value = {.type = tNArgs, .args = length}});
				output((struct Item){.operator = oCall});
			break;default:
				read_next = false;
				return true;
			}
		}
		return true;
	}
	
	void read_line{
		if(read_expression()){
			while(op_stack_pointer){
				struct Item item = pop_op();
				if(item == group_start)
					parse_error("Internal error: `(` in operator stack after parsing expression\n");
				output(item);
			}
			output((struct Item){.operator = oDiscard})
		}else{
			next();
			switch(token.type){
			case()
				
				
			}
			
		}
		
	}
}