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
//    if (array == NULL) {
//        printf("ERROR3\n");
//    } else {
//        printf("array = %p\n", array);
//    }
    
    for (int i = 0; i < size; ++i) {
        res ^= array[i];
    }
    return res;
}

void test2(void)
{
#define PTRS_NUM 1000
    const int max_size = 3*PAGE_SIZE;
    void* pointers[PTRS_NUM] = {NULL};
    uint seed = time(NULL);
    srand(1435522608);
    printf("SEED: %u\n", seed);
    for (int i = 0; i < 1000; ++i) {
        printf("Memory dump %i\n", i);
        mem_dump();
        int r = rand();
        int idx = rand() % PTRS_NUM;
        if (r > RAND_MAX / 2) {
            if (pointers[idx] == NULL) {
                int size = rand() % max_size;
                pointers[idx] = mem_alloc(size);
                printf("malloc %zu\n", size);
            } else {
                mem_free(pointers[idx]);
                pointers[idx] = NULL;
                printf("free\n");
            }
        } else {
            int size = rand() % max_size;
            void* new_ptr = mem_realloc(pointers[idx], size);
            printf("realloc %p\n", pointers[idx]);
            if (new_ptr == NULL) {
                mem_free(pointers[idx]);
                printf("free2\n");
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
 * 1435522608 - 1000, 3*PAGE_SIZE
 * prob 0.5 - 100, 3*PAGE_SIZE
 */

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
    for (int i = 0; i < info->size; ++i) {
        ((char*) info->mem_area)[i] = rand();
    }
}

void test3(void)
{
#define PTRS_NUM 1000
    const int max_size = 3*PAGE_SIZE;
    struct mem_area_info pointers_info[PTRS_NUM] = {{0}};
    uint seed = time(NULL);
    srand(1435522608);
    printf("SEED: %u\n", seed);
    for (int i = 0; i < 1000; ++i) {
        printf("Memory dump %i\n", i);
        mem_dump();
        int r = rand();
        int idx = rand() % PTRS_NUM;
        if (r > RAND_MAX / 2) {
            if (pointers_info[idx].mem_area == NULL) {
                int size = rand() % max_size;
                pointers_info[idx].mem_area = mem_alloc(size);
                pointers_info[idx].size = size;
                fill_area(&pointers_info[idx]);
                pointers_info[idx].hash = hash_array(pointers_info[idx].mem_area, size);
                printf("malloc %zu\n", size);
            } else {
                testHash(pointers_info, idx);
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
            printf("realloc %p, size = %zu\n", pointers_info[idx].mem_area, size);
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
    test3();
    return 0;
}
