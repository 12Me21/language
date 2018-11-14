void multiply(uint args){
	if(args!=2)
		die("wrong # of args\n");
	struct Value a = pop();
	struct Value b = pop();
	if(a.type == tNumber && b.type == tNumber){
		push((struct Value){.type = tNumber, .number = a.number * b.number});
	}else{
		die("Type mismatch in multiply\n");
	}
}

void divisible_by(uint args){
	if(args!=2)
		die("wrong # of args\n");
	struct Value b = pop();
	struct Value a = pop();
	pop();
	if(a.type == tNumber && b.type == tNumber){
		//printf("%g,%g : %g %d\n",a.number, b.number,fmod(a.number, b.number),(fmod(a.number, b.number) == 0.0));
		push((struct Value){.type = tBoolean, .boolean = (fmod(a.number, b.number) == 0.0)});
	}else{
		die("Type mismatch in divby\n");
	}
}