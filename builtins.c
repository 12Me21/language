//this entire file is outdated and bad I think
//EDIT: it's not, actually

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

//untested
// void sort(uint args){
	// if(args!=1)
		// die("wrong # of args\n");
	// struct Value a = pop();
	// pop();
	// if(a.type == tArray){
		// qsort(a.array->pointer, a.array->length, sizeof(struct Variable), compare_vars)
	// }
	
// }

// void seconds(uint args){
	// if(args!=0)
		// die("wrong # of args\n");
	// pop();
	// push((struct Value){.type = tNumber, .number = current_timestamp()-start_time});
// }


void f_floor(uint args){
	if(args!=1)
		die("wrong # of args\n");
	struct Value a = pop();
	pop();
	if(a.type!=tNumber)
		die("wrong type");
	push((struct Value){.type = tNumber, .number = floor(a.number)});
}

void f_ceil(uint args){
	if(args!=1)
		die("wrong # of args\n");
	struct Value a = pop();
	pop();
	if(a.type!=tNumber)
		die("wrong type");
	push((struct Value){.type = tNumber, .number = ceil(a.number)});
}