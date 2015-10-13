#ifndef MEM_ALLOC_H
#define MEM_ALLOC_H
#include <stdlib.h>
#include "mem_types.h"

void mem_dispose(void);
void *mem_alloc(size_t size);
void *mem_realloc(void *addr, size_t size);
void mem_free(void *addr);
void mem_dump(void);
HeaderType mem_get_type(void* addr);
#endif // MEM_ALLOC_H
