//keep a list of all objects allocated on the heap (arrays/tables/strings)
//garbage collection:
//for each item in the stack, and each variable in the scope stack:
// recursively mark all referenced objects as used.
//for each object in the objects list, free it if it hasn't been marked as used.

struct Heap_Object {
	enum Type type;
	union {
		struct Array * array;
		struct Table * table;
		struct String * string;
		void * pointer;
	};
};

uint used_memory = 0;
struct Heap_Object heap_objects[65536];

//add a flag to array/string/table keeping track of whether it's been checked or not

void check(struct Value * value){
	switch(value->type){
	case tArray:;
		struct Array * array = value->array;
		if(array->checked)
			return;
		array->checked = true;
		uint i;
		for(i=0;i<array->length;i++)
			check(&(array->pointer[i].value));
	break;case tTable:;
		struct Table * table = value->table;
		if(table->checked)
			return;
		table->checked = true;
		struct Entry * current = table->first;
		while(current){
			check(&(current->variable.value));
			current=current->next;
		}
	break;case tString:;
		value->string->checked = true;
	}
}

void garbage_collect(){
	printf("Running garbage collector\n");
	uint i, j;
	
	for(i=0;i<stack_pointer;i++)
		check(stack + i);
	
	for(i=0;i<scope_stack_pointer;i++)
		for(j=0;j<r_scope_length[i];j++)
			check(&(scope_stack[i][j].value));
	
	for(i=0;i<ARRAYSIZE(heap_objects);i++)
		switch(heap_objects[i].type){
		case tArray:
			if(!heap_objects[i].array->checked){
				used_memory -= heap_objects[i].array->length * sizeof(struct Variable);
				free(heap_objects[i].array->pointer);
				free(heap_objects[i].array);
				heap_objects[i].type = tNone;
			}else
				heap_objects[i].array->checked = false;
		break;case tTable:
			if(!heap_objects[i].table->checked){
				used_memory -= heap_objects[i].table->length * sizeof(struct Entry); // + table struct itself
				free_table(heap_objects[i].table);
				heap_objects[i].type = tNone;
			}else
				heap_objects[i].table->checked = false;
		break;case tString:
			if(!heap_objects[i].string->checked){
				used_memory -= heap_objects[i].string->length * sizeof(char);
				free(heap_objects[i].string->pointer);
				free(heap_objects[i].string);
				heap_objects[i].type = tNone;
			}else
				heap_objects[i].string->checked = false;
		}
}

//just make a custom *alloc function which checks if there's enough memory free and adjusts FREEMEM
size_t memory_usage(struct Value * value){
	switch(value->type){
	case tArray:
		return value->array->length * sizeof(struct Variable);
	break;case tTable:
		return value->table->length * sizeof(struct Entry);
	break;case tString:
		return value->string->length * sizeof(char);
	}
	die("ww\n");
}

void record_alloc(struct Value * value){
	used_memory += memory_usage(value);
	//printf("mem: %d\n",used_memory);
	uint i;
	for(i=0;i<ARRAYSIZE(heap_objects);i++)
		if(heap_objects[i].type == tNone){
			//printf("allocated, slot %d\n", i);
			heap_objects[i] = (struct Heap_Object){.type = value->type, .pointer = value->pointer};
			if(used_memory > 64 * 1024 * 1024)
				garbage_collect();
			if(used_memory > 64 * 1024 * 1024)
				die("Used too much memory\n");
			goto success;
		}
	garbage_collect();
	for(i=0;i<ARRAYSIZE(heap_objects);i++)
		if(heap_objects[i].type == tNone){
			//printf("allocated on second attempt, slot %d\n", i);
			heap_objects[i] = (struct Heap_Object){.type = value->type, .pointer = value->pointer};
			goto success;
		}
	die("Out of space\n");
	success:;
}

//consider: https://en.wikipedia.org/wiki/Tracing_garbage_collection#Moving_vs._non-moving