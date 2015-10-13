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

struct mem_area_info {
    void* mem_area;
    size_t size;
    char hash;
};

char hash_array(char* array, size_t size)
{
    char res = 0;
    for (size_t i = 0; i < size; ++i) {
        res ^= array[i];
    }
    return res;
}

void testHash(struct mem_area_info* pointers_info, int idx)
{
    char hash = hash_array(pointers_info[idx].mem_area, pointers_info[idx].size);
    if (hash != pointers_info[idx].hash) {
        printf("TEST FAILED\n");
        fflush(stdout);
        getchar();
    }
}

void fill_area(struct mem_area_info* info)
{
    for (size_t i = 0; i < info->size; ++i) {
        ((char*) info->mem_area)[i] = rand();
    }
}

void test2(void)
{
#define PTRS_NUM 1000
    const int max_size = 3 * PAGE_SIZE / 2;
    struct mem_area_info pointers_info[PTRS_NUM] = {{0}};
    uint seed = time(NULL);
    srand(seed);
    printf("SEED: %u\n", seed);
    fflush(stdout);
    for (int i = 0; i < 10000; ++i) {
        printf("i = %i\n", i);
        if (i > 5200) {
            printf("Memory dump %i\n", i);
            mem_dump();
        }
        int r = rand();
        int idx = rand() % PTRS_NUM;
        if (r > RAND_MAX / 2) {
            if (pointers_info[idx].mem_area == NULL) {
                int size = rand() % max_size;
                printf("mallocing... %i\n", size);
                fflush(stdout);
                pointers_info[idx].mem_area = mem_alloc(size);
                if (pointers_info[idx].mem_area != NULL) {
                    pointers_info[idx].size = size;
                    fill_area(&pointers_info[idx]);
                    pointers_info[idx].hash = hash_array(pointers_info[idx].mem_area, size);
                }
                printf("malloc %i\n", size);
            } else {
                if (pointers_info[idx].mem_area != NULL) {
                    testHash(pointers_info, idx);
                }
                mem_free(pointers_info[idx].mem_area);
                pointers_info[idx].mem_area = NULL;
                printf("free\n");
            }
        } else {
            int size = rand() % max_size;
            if (pointers_info[idx].mem_area != NULL) {
                testHash(pointers_info, idx);
            }
            void* new_ptr = mem_realloc(pointers_info[idx].mem_area, size);
            printf("realloc %p, size = %i\n", pointers_info[idx].mem_area, size);
            fflush(stdout);
            if (new_ptr == NULL) {
                mem_free(pointers_info[idx].mem_area);
                printf("free2\n");
                pointers_info[idx].mem_area = NULL;
            } else {
                pointers_info[idx].mem_area = new_ptr;
                pointers_info[idx].size = size;
                fill_area(&pointers_info[idx]);
                pointers_info[idx].hash = hash_array(new_ptr, size);
            }
        }
        fflush(stdout);
    }
    printf("Memory dump\n");
    mem_dump();
    for (int i = 0; i < PTRS_NUM; ++i) {
        if (pointers_info[i].mem_area != NULL) {
            printf("%i: %p\n", i, pointers_info[i].mem_area);
            printf("Memory dump, freeing %i\n", i);
            mem_dump();
            printf("\n");
            fflush(stdout);
            testHash(pointers_info, i);
        }
        mem_free(pointers_info[i].mem_area);
    }
    printf("Memory dump\n");
    mem_dump();
    printf("SEED: %u\n", seed);
}

int main(void)
{
    test2();
    return 0;
}
