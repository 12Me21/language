//awful dictionary library

struct Entry {
	struct Variable variable;
	size_t key_size;
	char * key;
	unsigned char key_type;
	struct Entry * next;
};

struct Table {
	struct Entry * first;
	struct Entry * last;
	//int references;
}; //table_new = {.first = NULL, .last = NULL, /*.references = 0*/};

struct Entry * table_get(struct Table * table, struct Value key, bool add){
	char * key_data;
	size_t key_size;
	//this is very very very very very not very good.
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
		key_data = (char *)&(key.user_function); //bad
		key_size = sizeof(Address);
	}
	
	struct Entry * current = table->first;
	while(current){
		if(current->key_size == key_size && current->key_type==key.type && !memcmp(current->key, key_data, key_size)){
			return current;
		}
		current=current->next;
	}
	if(add){
		current = ALLOC_INIT(struct Entry, {.key_size = key_size, .key_type = key.type, .key = memdup(key_data, key_size), .next = NULL});
		if(table->last){
			table->last->next = current;
			table->last = current;
		}else{ //first item added to the table
			table->first = table->last = current;
		}
		return current;
	}
	return NULL;
}

unsigned int table_length(struct Table * table){
	unsigned int length=0;
	struct Entry * current = table->first;
	while(current){
		length++;
		current=current->next;
	}
	return length;
}
//ok so when you access table.key, that should be fast, of course. .key should be a symbol, not a string
//but it is expected that table.key == table["key"] ...
//maybe make strings also check symbols too?

//creates a new table slot (or overwrites an existing one) with a variable
//(This is the only way to modify the constraint expression)
//Meant to be used by VAR.
struct Variable * table_declare(struct Table * table, struct Value key, struct Value value){
	struct Entry * entry = table_get(table, key, true);
	entry->variable = (struct Variable){.value = value};
	return entry->variable.value.variable = &(entry->variable);
}

//returns the value stored at an index in a table.
//(Its value can be read/written)
//Used in normal situations.
//Remember that the value contains a pointer back to the original variable :)
struct Value table_lookup(struct Table * table, struct Value key){
	//printf("looking up\n");
	struct Entry * entry = table_get(table, key, false);
	if(entry)
		return entry->variable.value;
	//die("tried to access undefined table index");
	return table_declare(table, key, (struct Value){.type = tNone})->value;
}