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

// void seconds(uint args){
	// if(args!=0)
		// die("wrong # of args\n");
	// pop();
	// push((struct Value){.type = tNumber, .number = current_timestamp()-start_time});
// }

void print(uint args){
	uint i;
	for(i = args; i>0; i--){
		basic_print(stack_get(i));
		if(i!=1)
			printf("\t");
	}
	stack_discard(args);
	printf("\n");
}