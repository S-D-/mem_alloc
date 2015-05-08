#include <stdio.h>
#include "mem_alloc/mem_alloc.h"

void test1() 
{
    void* mem_area1 = mem_alloc(100); 
    printf("Memory dump\n");
    mem_dump();
    void* mem_area2 = mem_alloc(200);
    printf("\nMemory dump2\n");
    mem_dump();
    mem_free(mem_area1);
    printf("\nMemory dump\n");
    mem_dump();
    
    mem_free(mem_area2);
    printf("\nMemory dump\n");
    mem_dump();
    mem_dispose();
}

int main(void)
{
    test1();
    return 0;
}
