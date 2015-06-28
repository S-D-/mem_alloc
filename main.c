#include <stdio.h>
#include "mem_alloc/mem_alloc.h"
#include "mem_alloc/mem_types.h"
#include <time.h>
#include <stdint.h>

void test1(void) 
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

void test2(void)
{
#define PTRS_NUM 1000
    const int max_size = PAGE_SIZE;
    void* pointers[PTRS_NUM] = {NULL};
    uint seed = time(NULL);
    srand(seed);
    printf("SEED: %u\n", seed);
    for (int i = 0; i < 28; ++i) {
        printf("Memory dump %i\n", i);
        mem_dump();
        int r = rand();
        int idx = rand() % PTRS_NUM;
        if (r > RAND_MAX / 2) {
            if (pointers[idx] == NULL) {
                int size = rand() % max_size;
                pointers[idx] = mem_alloc(size);
//                printf("malloc %zu\n", size);
            } else {
                mem_free(pointers[idx]);
                pointers[idx] = NULL;
//                printf("free\n");
            }
        } else {
            int size = rand() % max_size;
            void* new_ptr = mem_realloc(pointers[idx], size);
            if (new_ptr == NULL) {
                mem_free(pointers[idx]);
                pointers[idx] = NULL;
            } else {
                pointers[idx] = new_ptr;
            }
        }
    }
    printf("Memory dump\n");
    mem_dump();
    for (int i = 0; i < PTRS_NUM; ++i) {
        if (pointers[i] != NULL) {
            printf("%i: %p\n", i, pointers[i]);
            printf("Memory dump, freeing %i\n", i);
            mem_dump();
            printf("\n");
            fflush(stdout);
        }
        mem_free(pointers[i]);
    }
    printf("Memory dump\n");
    mem_dump();
    printf("SEED: %u\n", seed);
}
/* 1435346457 - 4
 * 1435443214 - 10
 * 1435485050 - 50
 * 1435499681 - 300
 * realloc:
 * 1435502300 - 28 
 */
int main(void)
{
    test2();
    return 0;
}
