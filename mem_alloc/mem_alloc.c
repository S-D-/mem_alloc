#include "mem_alloc.h"
#include <stdio.h>
#include <assert.h>
#include <memory.h>
#include <sys/mman.h>
#include <unistd.h>
#include "mem_types.h"
#include "mem_rb_tree.h"
#include <stdint.h>

//static void* page_pointers[PAGE_NUM];
static char* mem_start = NULL;
static UsedPH* formated_page_lists[BLK_MAX_POW2] = {NULL};
static FreeBigBH* free_big_bhs_root = NULL;
static intptr_t header_ptrs[PAGE_NUM];


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

void clrBit(char* mask, size_t bit_idx)
{
    size_t i = bit_idx / 8;
    int bit_num = bit_idx - i * 8;
    char clr_mask = 1 << bit_num;
    mask[i] &= ~clr_mask;
}

bool isFree32(char* usage_mask, size_t mask_len)
{
    char mask = 0;
    for (int i = 1; i < mask_len; ++i) {
        mask |= usage_mask[i];
    }
    if (mask) {
        return false;
    }
    mask |= usage_mask[0];
    if (mask == 1) {
        return true;
    }
    return false;
}

bool isFree(char* usage_mask, size_t mask_len)
{
    char mask = 0;
    for (int i = 0; i < mask_len; ++i) {
        mask |= usage_mask[i];
    }
    if (mask) {
        return false;
    }
    return true;
}

bool isFull(char* usage_mask, size_t mask_len)
{
    char mask = -1;
    for (int i = 0; i < mask_len; ++i) {
        mask &= usage_mask[i];
    }
    if (mask == -1) {
        return true;
    }
    return false;
}

size_t block_size(void* first_block)
{
    
}

size_t roundToPage(size_t size)
{
    return (size + PAGE_SIZE - 1) / PAGE_SIZE * PAGE_SIZE;
}

void* get_page_start(void* addr)
{
    size_t bytes_num = (char*) addr - mem_start;
    return mem_start + bytes_num / PAGE_SIZE * PAGE_SIZE;
}

void* mem_init(void)
{
    assert(PAGE_SIZE == sysconf(_SC_PAGE_SIZE));
    /* create mapping */
    void* mapping_area = mmap(NULL, PAGE_NUM*PAGE_SIZE, PROT_READ|PROT_WRITE, 
         MAP_PRIVATE|MAP_ANONYMOUS, -1, 0);
    if (mapping_area == MAP_FAILED) {
        return NULL;
    }
    mem_start = mapping_area;
    FreeBigBH* bh = mapping_area;
    free_big_bhs_root = mapping_area;
    //bh->info.next = NULL;
    bh->info.prev_size = 0;
    bh->info.size = PAGE_NUM*PAGE_SIZE;
    rbtree_insert(bh);
    
//    /* init list of free pages */
//    FreePage* free_phs = mapping_area;
//    for (int i = 1; i < PAGE_NUM; ++i) {
//        free_phs[i-1].next_ph = &free_phs[i];
//    }
//    free_phs[PAGE_NUM-1].next_ph = NULL;
//    first_free_page = free_phs;
//    /* init header_ptrs */
//    for (int i = 0; i < PAGE_NUM; ++i) {
//        header_ptrs[i].free_page = &free_phs[i];
//    }
//    header_ptrs[PAGE_NUM-1].free_page = (uintptr_t) header_ptrs[PAGE_NUM-1].free_page | 1; // TODO check this
    header_ptrs[0] = (intptr_t) bh | FREE_BIG_BH;
    set_rb_root_var(&free_big_bhs_root); //TODO header_ptrs?
    return mapping_area;
}

//size_t search_free_page(size_t size) {
//    int pages_num = size / PAGE_SIZE;
//    bool b = header_ptrs
//}

size_t page_idx(void* page_addr)
{
    return ((char*) page_addr - mem_start) / PAGE_SIZE;
}

void set_nxt_blk_prev_size(size_t size, void* next_blk)
{
    size_t pg_idx = page_idx(next_blk);
    HeaderType h_type = header_ptrs[pg_idx] & 3;
    intptr_t ptr_mask = 3;
    switch (h_type) {
    case USED_PH32:
    case USED_PH:;
        UsedPH* ph32 = header_ptrs[pg_idx] & (~ptr_mask);
        ph32->prev_size = size;
        break;
    case USED_BIG_BH:
    case FREE_BIG_BH:;
        FreeBigBH* fbh = header_ptrs[pg_idx] & (~ptr_mask);
        fbh->info.prev_size = size;
        break;
    default:
        break;
    }
}

void* mem_alloc(size_t size)
{
    printf("size: %zu, %zu, %zu\n", sizeof(UsedPH), sizeof(size_t), sizeof(int));
    if (mem_start == NULL) {
        if (mem_init() == NULL) {
            return NULL;
        }
    }
    if (size <= PAGE_SIZE / 2) {
        uint pow2 = nextPow2(size);
        UsedPH* ph = formated_page_lists[pow2];
        size_t block_len = 1;
        block_len <<= pow2; //in bits ??
        size_t mask_len = PAGE_SIZE / block_len / 8;
        if (ph != NULL) {
            //ph->blocks_free--;
            size_t block_idx = setBit(ph->usage_mask, mask_len);
            if (isFull(ph->usage_mask, mask_len)){
                formated_page_lists[pow2] = ph->next_ph;
            }
//            if (ph->blocks_free == 0) {
//                formated_page_lists[pow2] = ph->next_ph;
//            }
            //size_t block_idx = setBit(ph->usage_mask, mask_len);
            //block_len /= 8; // in bytes
            char* first_block = ph->first_block;
            return first_block + block_len * block_idx;
        }
        if (free_big_bhs_root == NULL) {
            return NULL; // no memory left
        }
        
        /* Format new page */
       // block_len /= 8; // in bytes
        FreeBigBH* fbh = free_big_bhs_root;
        rbtree_delete(fbh);
        if (fbh->info.size > PAGE_SIZE) {
            FreeBigBH* new_root = (FreeBigBH*)((char*) fbh + PAGE_SIZE);
            //new_root->info.next = fbh->info.next;
            new_root->info.prev_size = fbh->info.prev_size;
            new_root->info.size = fbh->info.size - PAGE_SIZE;
//            if (new_root->info.prev_size != 0) {
//                new_root->info.prev->next = new_root;
//            }
            void* next_block = (char*) new_root + new_root->info.size;
            if (next_block < mem_start + PAGE_NUM*PAGE_SIZE) {
                set_prev_size(new_root->info.size, next_block);
            }
            rbtree_insert(new_root);
            size_t block_idx = page_idx(new_root);
            header_ptrs[block_idx] = (intptr_t) new_root | FREE_BIG_BH;
        }
        //first_free_page = fp->next_ph;
        UsedPH* new_ph;
        size_t prev_sz = fbh->info.prev_size;
        if (pow2 == 5) { // 4?
            new_ph = (UsedPH*)fbh;
            //new_ph->blocks_free = PAGE_SIZE / block_len - 1;
            new_ph->first_block = (char*) new_ph + sizeof(UsedPH)+mask_len;
            new_ph->usage_mask[0] = 3;
            header_ptrs[page_idx(fbh)] = (intptr_t) new_ph | USED_PH32;
        } else {
            new_ph = mem_alloc(sizeof(UsedPH) + mask_len);
            //new_ph->blocks_free = PAGE_SIZE / block_len;
            new_ph->first_block = fbh;
            new_ph->usage_mask[0] = 1;
            header_ptrs[page_idx(fbh)] = (intptr_t) new_ph | USED_PH;
        }
        new_ph->prev_size = prev_sz;
        new_ph->next_ph = NULL;
        memset(&new_ph->usage_mask[1], 0, mask_len-1);
        //new_ph->usage_mask[0] = 1;
        formated_page_lists[pow2] = new_ph;
        return new_ph->first_block;
    }
    /* if big block */
    if (free_big_bhs_root == NULL) {
        return NULL; // no memory left
    }
    FreeBigBH* fbh = rbtree_lookup(size + sizeof(UsedBigBH));
    rbtree_delete(fbh);
    size_t block_size = roundToPage(size + sizeof(UsedBigBH));
    if (block_size < fbh->info.size) {
        FreeBigBH* rem_bh = (FreeBigBH*)((char*) fbh + block_size);
        rem_bh->info.prev_size = block_size;
        rem_bh->info.size = fbh->info.size - block_size;
//        if (rem_bh->info.prev != NULL) {
//            rem_bh->info.prev->next = rem_bh;
//        }
        void* next_block = (char*) rem_bh + rem_bh->info.size;
        if (next_block < mem_start + PAGE_NUM*PAGE_SIZE) {
            set_prev_size(rem_bh->info.size, next_block);
        }
//        if (rem_bh->info.next != NULL) {
//            rem_bh->info.next->prev = rem_bh;
//        }
        rbtree_insert(rem_bh);
        size_t block_idx = page_idx(rem_bh);
        header_ptrs[block_idx] = (intptr_t) rem_bh | FREE_BIG_BH;
    }
    header_ptrs[page_idx(fbh)] = (intptr_t) fbh | USED_BIG_BH;
    return (UsedBigBH*) fbh + 1;
}
//TODO add the checks
// TODO add header_locs handling

void* mem_realloc(void *addr, size_t size)
{
    
}

static void free_if_big_bh(UsedBigBH* bh);
static void free_pages(UsedPH* ph, int page_num)
{
    size_t pg_idx = page_idx((char*)ph - ph->prev_size);
    HeaderType h_type = header_ptrs[pg_idx] & 3;
    switch (h_type) {
    case USED_PH:
    case USED_PH32:
        UsedPH* ph = header_ptrs[pg_idx] & (~ptr_mask);
        ph->
        break;
    default:
        break;
    }
}

void mem_free(void *addr)
{
    intptr_t test = (intptr_t) addr & 3;
    if (test != 0) {
        fprintf(stderr, "ERROR: Incorrect address for mem_free()!\n");
    }
    void* page_start = get_page_start(addr);
    size_t pg_idx = page_idx(page_start);
    HeaderType h_type = header_ptrs[pg_idx] & 3;
    switch (h_type) {
    case USED_PH32:;
        UsedPH* ph32 = page_start;
        int block_idx = ((char*) addr - (char*) page_start) / 32;
        size_t mask_len = PAGE_SIZE / 32 / 8;
        bool was_full = isFull(ph32->usage_mask, mask_len);
        clrBit(ph32->usage_mask, block_idx);
        if (isFree32(ph32->usage_mask, mask_len)) {
            free_pages(ph32, 1);
        } else if (was_full){
            ph32->next_ph = formated_page_lists[5];
            formated_page_lists[5] = ph32;
        }
        break;
    case USED_PH:;
        intptr_t ptr_mask = 3;
        UsedPH* ph = header_ptrs[pg_idx] & (~ptr_mask);
        int block_idx = ((char*) addr - (char*) page_start) / 32;
        clrBit(ph->usage_mask, block_idx);
        break;
    case USED_BIG_BH:
        
        break;
    case FREE_BIG_BH:
        break;
    default:
        break;
    }
}

void free_if_big_bh(UsedBigBH *bh)
{
    if (bh->next != NULL) {
        
    }
}

//TODO check pointers cast for UB
