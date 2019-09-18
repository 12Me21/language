void * memdup(void * source, size_t size) {
	void * mem = malloc(size);
	return mem ? memcpy(mem, source, size) : NULL;
}
#define ALLOC_INIT(type, ...) memdup((type[]){ __VA_ARGS__ }, sizeof(type))
