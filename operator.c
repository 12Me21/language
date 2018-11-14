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
	if(a.type == tNumber && b.type == tNumber){
		push((struct Value){.type = tNumber, .number = fmod(a.number, b.number)});
	}else{
		die("Type mismatch in %\n");
	}
break;case oExponent:
break;case oBitwise_And:
break;case oMultiply:
break;case oNegative:
break;case oSubtract:
break;case oAdd:;
	b = pop();
	a = pop();
	if(a.type == tNumber && b.type == tNumber){
		push((struct Value){.type = tNumber, .number = a.number + b.number});
	}else{
		die("Type mismatch in +\n");
	}
break;case oEqual:
	b = pop();
	a = pop();
	push(make_boolean(equal(a,b)));
break;case oBitwise_Or:
break;case oFloor_Divide:
break;case oLess_Or_Equal:
break;case oLeft_Shift:
break;case oLess:
	b = pop();
	a = pop();
	if(a.type == tNumber && b.type == tNumber){
		push(make_boolean(a.number < b.number));
	}else{
		die("Type mismatch in <\n");
	}
break;case oGreater_Or_Equal:
break;case oRight_Shift:
break;case oGreater:
break;case oDivide:
break;case oPrint1:
	basic_print(pop());
	printf("\n");
	push((struct Value){.type=tNone});