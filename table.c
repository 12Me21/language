//awful dictionary library

// entry:
// 1: struct Value value
// 2: size_t size
// 3: char * key

struct Entry {
	struct Variable variable;
	size_t key_size;
	char * key;
	struct Entry * next;
};

struct Table {
	struct Entry * first;
	struct Entry * last;
	//int references;
} table_new = {.first = NULL, .last = NULL, /*.references = 0*/};

//just use a struct Value as the key, and extract the info from that.
// no I mean the entire value
// except for strings which are special...
struct Entry * table_get(struct Table * table, struct Value key){
	char * key_data;
	size_t key_size;
	switch(key.type){
	case tNumber:
		key_data = (char *)&(key.number);
		key_size = sizeof(double);
		break;
	case tString:
		key_data = (char *)key.string->pointer;
		key_size = key.string->length * sizeof(char);
		break;
	case tTable:
		key_data = (char *)&(key.table);
		key_size = sizeof(struct Table *);
		break;
	case tBoolean:
		key_data = (char *)&(key.boolean);
		key_size = sizeof(bool);
		break;
	case tFunction:
		key_data = (char *)&(key.function);
		key_size = sizeof(Address);
	}
	
	struct Entry * current = table->first;
	while(current){
		if(current->key_size == key_size && !memcmp(current->key, key_data, key_size))
			return current;
		current=current->next;
	}
	return NULL;
}

// TODO: support a way of setting the constraint expression
void table_set(struct Table * table, struct Value key, struct Value value){
	char * key_data;
	size_t key_size;
	switch(key.type){
	case tNumber:
		key_data = (char *)&(key.number);
		key_size = sizeof(double);
		break;
	case tString:
		key_data = (char *)key.string->pointer;
		key_size = key.string->length * sizeof(char);
		break;
	case tTable:
		key_data = (char *)&(key.table);
		key_size = sizeof(struct Table *);
		break;
	case tBoolean:
		key_data = (char *)&(key.boolean);
		key_size = sizeof(bool);
		break;
	case tFunction:
		key_data = (char *)&(key.function);
		key_size = sizeof(Address);
	}
		
	if(table->last){
		struct Entry * current = table_get(table, key);
		//key exists in table already
		if(current)
			//fix this to not overwrite the constraint!!!
			current->variable.value = value;
		//new key
		else{
			// create new entry
			current = ALLOC_INIT(struct Entry, {.variable = {.value = value}, .key_size = key_size, .key = memdup(key_data, key_size), .next = NULL});
			// change old last entry to point to new entry
			table->last->next = current;
			// update table's last entry pointer
			table->last = current;
		}
	}else{
		table->first = table->last = ALLOC_INIT(struct Entry, {.variable = {.value = value}, .key_size = key_size, .key = memdup(key_data, key_size), .next = NULL});
	}
}