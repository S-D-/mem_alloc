#ifndef MEM_ALLOC_H
#define MEM_ALLOC_H
#include <stdlib.h>

void mem_dispose(void);
void *mem_alloc(size_t size);
void *mem_realloc(void *addr, size_t size);
void mem_free(void *addr);
void mem_dump(void);
#endif // MEM_ALLOC_H
