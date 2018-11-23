break;case oBitwise_Not:;
	struct Value a = pop();
	if(a.type == tNumber)
		push((struct Value){.type = tNumber, .number = (double)~(uint)a.number});
	else
		type_mismatch_1(a, oBitwise_Not);
break;case oBitwise_Xor:;
	struct Value b = pop();
	a = pop();
	if(a.type == tNumber && b.type == tNumber)
		push((struct Value){.type = tNumber, .number = (double)((uint)a.number^(uint)b.number)});
	else
		type_mismatch_2(a, b, oBitwise_Xor);
break;case oNot_Equal:;
	b = pop();
	a = pop();
	push(make_boolean(!equal(a,b)));
break;case oNot:
	push(make_boolean(!truthy(pop())));
break;case oMod:
	b = pop();
	a = pop();
	if(a.type == tNumber && b.type == tNumber)
		push((struct Value){.type = tNumber, .number = fmod(a.number, b.number)});
	else
		die("Type mismatch in %%\n");
break;case oExponent:
	b = pop();
	a = pop();
	if(a.type == tNumber && b.type == tNumber)
		push((struct Value){.type = tNumber, .number = pow(a.number, b.number)});
	else
		die("Type mismatch in ^\n");
break;case oBitwise_And:
	b = pop();
	a = pop();
	if(a.type == tNumber && b.type == tNumber)
		push((struct Value){.type = tNumber, .number = (double)((uint)a.number&(uint)b.number)});
	else
		type_mismatch_2(a, b, oBitwise_And);
break;case oMultiply:
	b = pop();
	a = pop();
	if(b.type != tNumber)
		type_mismatch_2(a, b, oMultiply);
	switch(a.type){
	case tNumber:
		push((struct Value){.type = tNumber, .number = a.number * b.number});
	break;case tString:
		if(b.number<0)
			die("Can't multiply a string by a number less than 0\n");
		uint string_length = a.string->length;
		char * new_string = malloc(string_length * sizeof(char));
		for(i=0;i<(uint)b.number;i++)
			memcpy(new_string + i * string_length, a.string->pointer, string_length * sizeof(char));
		push((struct Value){.type = tString, .string = 
			ALLOC_INIT(struct String, {.pointer = new_string, .length = string_length * (uint)b.number})
		});
	break;default:
		type_mismatch_2(a, b, oMultiply);
	}
break;case oNegative:
	a = pop();
	if(a.type == tNumber)
		push((struct Value){.type = tNumber, .number = -a.number});
	else
		type_mismatch_1(a, oNegative);
break;case oSubtract:
	b = pop();
	a = pop();
	if(a.type == tNumber && b.type == tNumber)
		push((struct Value){.type = tNumber, .number = a.number - b.number});
	else
		die("Type mismatch in -\n");
break;case oAdd:;
	b = pop();
	a = pop();
	if(a.type == tNumber && b.type == tNumber)
		push((struct Value){.type = tNumber, .number = a.number + b.number});
	else
		die("Type mismatch in +\n");
break;case oEqual:
	b = pop_no_lists();
	a = pop_no_lists();
	push(make_boolean(equal(a,b)));
break;case oBitwise_Or:
	die("unimplemented");
break;case oFloor_Divide:
	die("unimplemented");
break;case oLess_Or_Equal:
	b=pop();
	push(make_boolean(compare(pop(),b)<1));
break;case oLeft_Shift:
	b = pop();
	a = pop();
	if(a.type == tNumber && b.type == tNumber)
		push((struct Value){.type = tNumber, .number = (double)((uint)a.number<<(uint)b.number)});
	else
		type_mismatch_2(a, b, oLeft_Shift);
break;case oLess:
	b = pop();
	a = pop();
	if(a.type == tNumber && b.type == tNumber)
		push(make_boolean(a.number < b.number));
	else
		die("Type mismatch in <\n");
break;case oGreater_Or_Equal:
	die("unimplemented");
break;case oRight_Shift:
	b = pop();
	a = pop();
	if(a.type == tNumber && b.type == tNumber)
		push((struct Value){.type = tNumber, .number = (double)((uint)a.number>>(uint)b.number)});
	else
		type_mismatch_2(a, b, oRight_Shift);
break;case oGreater:
	die("unimplemented");
break;case oDivide:
	b = pop();
	a = pop();
	if(a.type == tNumber && b.type == tNumber)
		push((struct Value){.type = tNumber, .number = a.number / b.number});
	else
		type_mismatch_2(a, b, oDivide);
break;case oPrint:
	a = pop();
	if(a.type == tNArgs){
		uint i;
		for(i=a.args; i>=1; i--){
			basic_print(stack_get(i));
			if(i!=1)
				printf("\t");
		}
		stack_discard(a.args);
	}else
		basic_print(a);
	printf("\n");
	push((struct Value){.type = tNone});
break;case oComma:
	b = pop();
	if(b.type == tNArgs){ //?, list
		a = stack_get(b.args+1);
		if(a.type == tNArgs){ //list, list
			die("can't merge 2 lists"); //mm
			memmove(stack+stack_pointer-b.args-1, stack+stack_pointer-b.args, b.args * sizeof(struct Item));
			pop();
			b.args += a.args;
			//printf("ll %d\n",b.args);
			push(b);
		}else{ //value, list
			die("can't merge 2 lists"); //mm
			b.args++;
			push(b);
			//printf("vl\n");
		}
		//die("can't merge 2 lists"); //mm
	}else{ //?, value
		a = pop();
		if(a.type == tNArgs){ //list, value
			//die("can't merge 2 lists"); //mm
			push(b);
			a.args++;
			push(a);
		}else{ //value, value
			push(a);
			push(b);
			push((struct Value){.type = tNArgs, .args = 2});
		}
	}
//Length operator
//Input: <value>
//Output: <length>
break;case oLength:
	a = pop();
	unsigned int length;
	switch(a.type){
		case tArray:
			length = a.array->length;
		break;
		case tString:
			length = a.string->length;
		break;
		case tTable:
			length = table_length(a.table);
		break;
		default:
			die("Length operator expected String, Array, or Table; got %s.\n", type_name[a.type]);
	}
	push((struct Value){.type = tNumber, .number = (double)length});
//Logical OR operator (with shortcutting)
//Input: <condition 1>
//Output: ?<condition 1>
break;case oLogicalOr:
	if(truthy(stack_get(1)))
		pos = item.address;
	else
		stack_discard(1);
//Logical AND operator (with shortcutting)
//Input: <condition 1>
//Output: ?<condition 1>
break;case oLogicalAnd:
	if(truthy(stack_get(1)))
		stack_discard(1);
	else
		pos = item.address;
break;case oIn:
	a = pop();
	switch(a.type){
	//value in list
	case tNArgs:
		stack_discard(a.args);
		b = pop_no_lists();
		for(i=0;i<a.args;i++)
			if(equal(b, stack_get(-1-i))){
				push((struct Value){.type = tNumber, .number = (double)i});
				goto found_in;
			}
		push((struct Value){.type = tNone});
	//value in array
	break;case tArray:
		b = pop_no_lists();
		for(i=0;i<a.array->length;i++)
			if(equal(b, a.array->pointer[i].value)){
				push((struct Value){.type = tNumber, .number = (double)i});
				goto found_in;
			}
		push((struct Value){.type = tNone});
	//todo: value in table, substring in string
	break;case tString:
		b = pop();
		if(b.type!=tString)
			die("expected string\n");
		for(i=0;i<a.string->length - b.string->length;i++)
			if(!memcmp(a.string->pointer, b.string->pointer, b.string->length * sizeof(char))){
				push((struct Value){.type = tNumber, .number = (double)i});
				goto found_in;
			}
		push((struct Value){.type = tNone});
	break;default:
		die("not implemented\n");
	}
	found_in:;

//I feel like lists could be optimized
//like you have A,B,C, that means it has to make A,B into a list and then add C to that
//not very efficient...
//if commas were instead compiled into a "make the last <x> items into a list" operator... idk

//lists are evil

//also implement builtins PLEASE