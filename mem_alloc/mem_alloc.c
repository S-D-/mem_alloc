#include "mem_alloc.h"
#include <stdio.h>
#include <assert.h>
#include <memory.h>
#include <sys/mman.h>
#include <unistd.h>

#define PAGE_NUM 100
#define PAGE_SIZE 4096
#define BLK_MAX_POW2 11

struct free_block_header {
    struct free_block_header* next_bh;
};
typedef struct free_block_header FreeBH;

struct free_page_header {
    struct free_page_header* next_ph;
    char page_area[PAGE_SIZE - sizeof(struct free_page_header*)];
};
typedef struct free_page_header FreePH;

struct used_page_header {
    struct used_page_header* next_ph;
    short blocks_free;
    void* first_block;
    char usage_mask[];
};
typedef struct used_page_header UsedPH;

//static void* page_pointers[PAGE_NUM];
static void* mem_start = NULL;
static UsedPH* formated_page_lists[BLK_MAX_POW2] = {NULL};
static FreePH* first_free_page;

u_int32_t roundToPow2(u_int32_t v)
{
    v--;
    v |= v >> 1;
    v |= v >> 2;
    v |= v >> 4;
    v |= v >> 8;
    v |= v >> 16;
    v++;
    return v;
}

uint nextPow2(size_t v)
{
    uint r = 1;
    while (v >>= 1)
    {
      r++;
    }
    return r;
}

/**
 * @brief setBit
 * @param mask
 * @param mask_len
 * @return number of setted bit.
 */
size_t setBit(char* mask, size_t mask_len)
{
    int i;
    for (i = 0; i < mask_len; ++i) {
        if ((mask[i] & 0xFF) != 0xFF) {
            break;
        }
    }
    assert(i != mask_len);
    char m;
    size_t res = i * 8;
    if ((mask[i] & 0xF) != 0xF) {
        m = 1;
    } else {
        m = 0x10;
        res += 4;
    }
    while((mask[i] & m) == m){
        m <<= 1;
        res++;
    }
    mask[i] &= m;
    return res;
}

void* mem_init(void)
{
    assert(PAGE_SIZE == sysconf(_SC_PAGE_SIZE));
    void* mapping_area = mmap(NULL, PAGE_NUM*PAGE_SIZE, PROT_READ|PROT_WRITE, 
         MAP_PRIVATE|MAP_ANONYMOUS, -1, 0);
    if (mapping_area == MAP_FAILED) {
        return NULL;
    }
    FreePH* free_phs = mapping_area;
    for (int i = 1; i < PAGE_NUM; ++i) {
        free_phs[i-1].next_ph = &free_phs[i];
    }
    free_phs[PAGE_NUM].next_ph = NULL;
    return mapping_area;
}

void* mem_alloc(size_t size)
{
    if (mem_start == NULL) {
        if (mem_init() == NULL) {
            return NULL;
        }
    }
    if (size <= PAGE_SIZE / 2) {
        uint pow2 = nextPow2(size);
        UsedPH* ph = formated_page_lists[pow2];
        size_t block_len = 1;
        block_len = block_len << pow2; //in bits
        size_t mask_len = PAGE_SIZE / block_len / 8;
        if (ph != NULL) {
            ph->blocks_free--;
            if (ph->blocks_free == 0) {
                formated_page_lists[pow2] = ph->next_ph;
            }
            size_t block_idx = setBit(ph->usage_mask, mask_len);
            block_len /= 8; // in bytes
            char* first_block = ph->first_block;
            return first_block + block_len * block_idx;
        }
        if (first_free_page == NULL) {
            return NULL; // no memory left
        }
        block_len /= 8; // in bytes
        FreePH* fp = first_free_page;
        first_free_page = fp->next_ph;
        UsedPH* new_ph;
        if (pow2 == 4) {
            new_ph = (UsedPH*)fp;
            new_ph->blocks_free = PAGE_SIZE / block_len - 1;
            new_ph->first_block = (char*) new_ph + sizeof(UsedPH)+16;
        } else {
            new_ph = mem_alloc(sizeof(UsedPH) + mask_len);
            new_ph->blocks_free = PAGE_SIZE / block_len;
            new_ph->first_block = fp;
        }
        new_ph->next_ph = NULL;
        memset(new_ph->usage_mask, 0, mask_len);
        new_ph->usage_mask[0] = 1;
        return new_ph->first_block;
    }
    printf("size: %zu, %zu, %zu\n", sizeof(UsedPH), sizeof(size_t), sizeof(int));
    return NULL;
}
//TODO check pointers cast for UB
