//Bit operators:
break;case oBitwise_Not:;
	struct Value a = pop_type(tNumber);
	push((struct Value){.type = tNumber, .number = (double)~(uint)a.number});
break;case oBitwise_Xor:;
	struct Value b = pop_type(tNumber);
	a = pop_type(tNumber);
	push((struct Value){.type = tNumber, .number = (double)((uint)a.number^(uint)b.number)});
break;case oBitwise_And:
	b = pop_type(tNumber);
	a = pop_type(tNumber);
	push((struct Value){.type = tNumber, .number = (double)((uint)a.number&(uint)b.number)});
break;case oBitwise_Or:
	die("unimplemented");
break;case oRight_Shift:
	b = pop_type(tNumber);
	a = pop_type(tNumber);
	push((struct Value){.type = tNumber, .number = (double)((uint)a.number>>(uint)b.number)});
break;case oLeft_Shift:
	b = pop_type(tNumber);
	a = pop_type(tNumber);
	push((struct Value){.type = tNumber, .number = (double)((uint)a.number<<(uint)b.number)});

//Math operators:
break;case oNegative:
	a = pop_type(tNumber);
	push((struct Value){.type = tNumber, .number = -a.number});
break;case oAdd:;
	b = pop_l();
	a = pop_l();
	if(a.type == tNumber){
		if(b.type != tNumber)
			die("expect number\n");
		push((struct Value){.type = tNumber, .number = a.number + b.number});
	}else if (a.type == tString){
		uint length = to_string(b, to_string_output);
		uint a_length = a.string->length;
		char * new_string = malloc((a_length + length) * sizeof(char));
		memcpy(new_string, a.string->pointer, a_length);
		memcpy(new_string + a_length, to_string_output, length);
		a = (struct Value){.type = tString, .string =
			ALLOC_INIT(struct String, {.pointer = new_string, .length = a_length + length})
		};
		if(!a.string)
			die("memory error\n");
		record_alloc(&a);
		push(a);
	}else{
		die("bad types xd\n");
	}
break;case oSubtract:
	b = pop_type(tNumber);
	a = pop_type(tNumber);
	push((struct Value){.type = tNumber, .number = a.number - b.number});
break;case oMultiply:
	//printf("multiply started");
	b = pop_type(tNumber);
	a = pop_l();
	switch(a.type){
	case tNumber:
		push((struct Value){.type = tNumber, .number = a.number * b.number});
	break;case tString:
		if(b.number<0)
			die("Can't multiply a string by a number less than 0\n");
		uint string_length = a.string->length;
		char * new_string = malloc(string_length * (uint)b.number * sizeof(char));
		for(i=0;i<(uint)b.number;i++)
			memcpy(new_string + i * string_length, a.string->pointer, string_length * sizeof(char));
		//printf("aaa");
		a = (struct Value){.type = tString, .string = 
			ALLOC_INIT(struct String, {.pointer = new_string, .length = string_length * (uint)b.number})
		};
		if(!a.string)
			die("memory error\n");
		//printf("%p",a.string);
		record_alloc(&a);
		push(a);
	break;default:
		type_mismatch_2(a, b, oMultiply);
	}
	//printf("multiply finished");
break;case oDivide:
	b = pop_type(tNumber);
	a = pop_type(tNumber);
	push((struct Value){.type = tNumber, .number = a.number / b.number});
break;case oFloor_Divide:
	b = pop_type(tNumber);
	a = pop_type(tNumber);
	push((struct Value){.type = tNumber, .number = floor(a.number / b.number)});
break;case oMod:
	b = pop_type(tNumber);
	a = pop_type(tNumber);
	push((struct Value){.type = tNumber, .number = fmod(a.number, b.number)});
break;case oExponent:
	b = pop_type(tNumber);
	a = pop_type(tNumber);
	push((struct Value){.type = tNumber, .number = pow(a.number, b.number)});
	
//Comparison operators:
break;case oNot_Equal:;
	b = pop_no_lists();
	push(make_boolean(!equal(pop_no_lists(), b)));
break;case oEqual:
	//maybe should support lists?
	b = pop_no_lists();
	push(make_boolean(equal(pop_no_lists(), b)));
break;case oLess_Or_Equal:
	b = pop_no_lists();
	push(make_boolean(compare(pop_no_lists(), b) <= 0));
break;case oLess:
	b = pop_no_lists();
	push(make_boolean(compare(pop_no_lists(), b) < 0));
break;case oGreater_Or_Equal:
	b = pop_no_lists();
	push(make_boolean(compare(pop_no_lists(), b) >= 0));
break;case oGreater:
	b = pop_no_lists();
	push(make_boolean(compare(pop_no_lists(), b) > 0));
//value in list
break;case oIn:
	a = pop_l();
	switch(a.type){
	//value in list
	case tNArgs:
		b = pop_no_lists();
		for(i=0;i<a.args;i++)
			if(equal(b, list_get(i + 1))){ // + 1 because `b` was popped from the stack after the list was removed
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
			if(!memcmp(a.string->pointer+i, b.string->pointer, b.string->length * sizeof(char))){
				push((struct Value){.type = tNumber, .number = (double)i});
				goto found_in;
			}
		push((struct Value){.type = tNone});
	break;default:
		die("not implemented\n");
	}
	found_in:;

//Logical operators:
break;case oNot:
	push(make_boolean(!truthy(pop_no_lists())));
//Logical OR operator (with shortcutting)
//Input: <condition 1>
//Output: ?<condition 1>
break;case oLogicalOr:
	if(truthy(stack_get(1)))
		pos = item.address;
	else
		pop_l();
//Logical AND operator (with shortcutting)
//Input: <condition 1>
//Output: ?<condition 1>
break;case oLogicalAnd:
	if(truthy(stack_get(1)))
		pop_l();
	else
		pos = item.address;

//Misc.
break;case oPrint:
	a = pop_l();
	if(a.type == tNArgs){
		uint i;
		for(i=0; i<a.args; i++){
			if(i)
				printf("\t");
			basic_print(list_get(i));
		}
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
	a = pop_l();
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

//I feel like lists could be optimized
//like you have A,B,C, that means it has to make A,B into a list and then add C to that
//not very efficient...
//if commas were instead compiled into a "make the last <x> items into a list" operator... idk

//lists are evil

//also implement builtins PLEASE