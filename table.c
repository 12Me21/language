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

//awful dictionary library

//just use a struct Value as the key, and extract the info from that.
// no I mean the entire value
// except for strings which are special...
// ... need to fix this so it takes the type into account...
struct Entry * table_get(struct Table * table, struct Value key, bool add){
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
		if(current->key_size == key_size && current->key_type==key.type && !memcmp(current->key, key_data, key_size))
			return current;
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

//returns a reference to the Variable stored at an index in a table.
//(Its value can be read/written)
//Used in normal situations.
//Remember that the value contains a pointer back to the original variable :)
struct Value table_lookup(struct Table * table, struct Value key){
	struct Entry * entry = table_get(table, key, false);
	return entry ? entry->variable.value : (struct Value){.type = tNone}; //maybe should be error
}

//creates a new table slot (or overwrites an existing one) with a variable
//(This is the only way to modify the constraint expression)
//Meant to be used by VAR.
//variable must be allocated on the heap (!)
void table_declare(struct Table * table, struct Value key, struct Variable * variable){
	struct Entry * entry = table_get(table, key, true);
	entry->variable = *variable;
	entry->variable.value.variable = variable;
}