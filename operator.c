break;case oBitwise_Not:
break;case oBitwise_Xor:
break;case oNot_Equal:;
	struct Value b = pop();
	struct Value a = pop();
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
break;case oMultiply:
	b = pop();
	a = pop();
	if(a.type == tNumber && b.type == tNumber)
		push((struct Value){.type = tNumber, .number = a.number * b.number});
	else
		die("Type mismatch in *\n");
break;case oNegative:
	a = pop();
	if(a.type == tNumber){
		push((struct Value){.type = tNumber, .number = -a.number});
	}else{
		type_mismatch_1(a, oNegative);
	}
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
	b = pop();
	a = pop();
	push(make_boolean(equal(a,b)));
break;case oBitwise_Or:
	die("unimplemented");
break;case oFloor_Divide:
	die("unimplemented");
break;case oLess_Or_Equal:
	b=pop();
	push(make_boolean(compare(pop(),b)<1));
break;case oLeft_Shift:
	die("unimplemented");
break;case oLess:
	b = pop();
	a = pop();
	if(a.type == tNumber && b.type == tNumber)
		push(make_boolean(a.number < b.number));
	else
		die("Type mismatch in <\n");
break;case oGreater_Or_Equal:
break;case oRight_Shift:
break;case oGreater:
break;case oDivide:
	b = pop();
	a = pop();
	if(a.type == tNumber && b.type == tNumber)
		push((struct Value){.type = tNumber, .number = a.number / b.number});
	else
		die(type_mismatch(a.type, b.type, oDivide));
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
	b = pop(); //new value
	a = pop(); //list terminator (NArgs) or first value
	if(a.type == tNArgs){
		push(b);
		a.args++;
		push(a);
	}else{
		push(a);
		push(b);
		push((struct Value){.type = tNArgs, .args = 2});
	}