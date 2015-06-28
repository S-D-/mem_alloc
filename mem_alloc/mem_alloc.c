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
//static UsedPH* formated_page_lists[BLK_MAX_POW2] = {NULL};
static size_t formated_page_idxs[BLK_MAX_POW2+1] = {[0 ... BLK_MAX_POW2] = SIZE_MAX};
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
 * @return number of bit that was set.
 */
size_t setBit(char* mask, size_t mask_len)
{
    int i;
    for (i = 0; i < mask_len; ++i) {
        if ((mask[i] & 0xFF) != 0xFF) {
            break;
        }
    }
//    int bits_left = bits_num - (bits_num / 8);
//    if (bits_left) {
//        signed char mask2 = 0xFF;
//        mask2 >>= 8 - bits_left;
//        if((mask[i] & mask2) != mask2) {
//            size_t res = i * 8;
//            char m = 1;
//            while((mask[i] & m) == m) {
//                m <<= 1;
//                res++;
//            }
//            mask[i] |= m;
//            return res;
//        }
//    }
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
    mask[i] |= m;
    return res;
}

void clrBit(char* mask, size_t bit_idx)
{
    size_t i = bit_idx / 8;
    int bit_num = bit_idx - i * 8;
    char clr_mask = 1 << bit_num;
    if (mask[i] & clr_mask) {
        mask[i] &= ~clr_mask;
        return;
    }
    printf("ERROR: Double free. %zu\n", bit_idx);
    mask[i] &= ~clr_mask;
}

bool isFree32(char* usage_mask, const size_t bits_num)
{
    char mask = 0;
    int i;
    for (i = 1; i < bits_num / 8; ++i) {
        mask |= usage_mask[i];
    }
    int bits_left = bits_num - (bits_num / 8) * 8;
    if (bits_left) {
        unsigned char mask2 = 0xFF;
        unsigned char mask3 = 0xFF;
        mask2 <<= bits_left;
        mask3 >>= 8 - bits_left;
        mask |= (usage_mask[i] | mask2) & mask3;
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

bool isFree(char* usage_mask, const size_t bits_num)
{
    char mask = 0;
    int i;
    for (i = 0; i < bits_num / 8; ++i) {
        mask |= usage_mask[i];
    }
    int bits_left = bits_num - (bits_num / 8) * 8;
    if (bits_left) {
        char mask2 = 0xFF;
        unsigned char mask3 = 0xFF;
        mask2 <<= bits_left;
        mask3 >>= 8 - bits_left;
        mask |= (usage_mask[i] | mask2) & mask3;
    }
    if (mask) {
        return false;
    }
    return true;
}

bool isFull(char* usage_mask, const size_t bits_num)
{
    char mask = -1;
    int i;
    for (i = 0; i < bits_num / 8; ++i) {
        mask &= usage_mask[i];
    }
    int bits_left = bits_num - (bits_num / 8) * 8;
    if (bits_left) {
        signed char mask2 = 0xFF;
        char mask3 = 0xFF;
        mask2 >>= 8 - bits_left;
        mask3 <<= bits_left;
        mask &= (usage_mask[i] & mask2) | mask3;
    }
    if (mask == -1) {
        return true;
    }
    return false;
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
    bh->info.prev_size = 0;
    bh->info.size = PAGE_NUM*PAGE_SIZE;
    header_ptrs[0] = (intptr_t) bh | FREE_BIG_BH;
    set_rb_root_var(&free_big_bhs_root);
    rbtree_insert(bh);
    return mapping_area;
}

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
    case USED_PH:;
        UsedPH* ph = (UsedPH*) (header_ptrs[pg_idx] & ~ptr_mask);
        ph->prev_size = size;
        break;
    case USED_BIG_BH:
    case FREE_BIG_BH:;
        FreeBigBH* fbh = (FreeBigBH*) (header_ptrs[pg_idx] & ~ptr_mask);
        fbh->info.prev_size = size;
        break;
    default:
        break;
    }
}

void* mem_alloc(size_t size)
{
    if (mem_start == NULL) {
        if (mem_init() == NULL) {
            return NULL;
        }
    }
    uint pow2 = nextPow2(size);
    size_t block_len = 1;
    block_len <<= pow2;
    if (size <= PAGE_SIZE / 2) {
        size_t ph_idx = formated_page_idxs[pow2];
        size_t blocks_num = PAGE_SIZE / block_len;
        size_t mask_len = (blocks_num + 7) / 8;
        if (ph_idx != SIZE_MAX) {
            UsedPH* ph = (UsedPH*) (header_ptrs[ph_idx] & ~(intptr_t)3);
            size_t block_idx = setBit(ph->usage_mask, mask_len);
            if (isFull(ph->usage_mask, blocks_num)){
                formated_page_idxs[pow2] = ph->next_ph_idx;
            }
            void* result = mem_start + ph_idx * PAGE_SIZE + block_len * block_idx;
            return result;
        }
        if (free_big_bhs_root == NULL) {
            return NULL; // no memory left
        }
        
        /* Format new page */
        FreeBigBH* fbh = free_big_bhs_root;
        
        rbtree_delete(fbh);
        if (fbh->info.size > PAGE_SIZE) {
            FreeBigBH* rem_fbh = (FreeBigBH*)((char*) fbh + PAGE_SIZE);
            rem_fbh->info.prev_size = PAGE_SIZE;//fbh->info.prev_size;
            rem_fbh->info.size = fbh->info.size - PAGE_SIZE;
            void* next_block = (char*) rem_fbh + rem_fbh->info.size;
            if ((char*) next_block < mem_start + PAGE_NUM*PAGE_SIZE) {
                set_nxt_blk_prev_size(rem_fbh->info.size, next_block);
            }
            rbtree_insert(rem_fbh);
            size_t block_idx = page_idx(rem_fbh);
            header_ptrs[block_idx] = (intptr_t) rem_fbh | FREE_BIG_BH;
        }
        UsedPH* new_ph;
        size_t prev_sz = fbh->info.prev_size;
        void* first_block;
        if (pow2 == 5) { // 4?
            new_ph = (UsedPH*)fbh;
            new_ph->usage_mask[0] = 3;
            first_block = (char*) fbh + block_len;
        } else {
            new_ph = mem_alloc(sizeof(UsedPH) + mask_len);
            if (new_ph == NULL) {
                return NULL; // TODO rollback
            }
            new_ph->usage_mask[0] = 1;
            first_block = (char*) fbh;
        }
        size_t pg_idx = page_idx(fbh);
        header_ptrs[pg_idx] = (intptr_t) new_ph | USED_PH;
        new_ph->blk_size_pow2 = pow2;
        new_ph->prev_size = prev_sz;
        if (prev_sz == 0) {
            printf("!!!\n");
        }
        new_ph->next_ph_idx = formated_page_idxs[pow2];
        memset(&new_ph->usage_mask[1], 0, mask_len-1);
        formated_page_idxs[pow2] = pg_idx;
        return first_block;
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
        void* next_block = (char*) rem_bh + rem_bh->info.size;
        if ((char*) next_block < mem_start + PAGE_NUM*PAGE_SIZE) {
            set_nxt_blk_prev_size(rem_bh->info.size, next_block);
        }
        rbtree_insert(rem_bh);
        size_t block_idx = page_idx(rem_bh);
        header_ptrs[block_idx] = (intptr_t) rem_bh | FREE_BIG_BH;
        fbh->info.size = block_size;
    }
    header_ptrs[page_idx(fbh)] = (intptr_t) fbh | USED_BIG_BH;
    return (UsedBigBH*) fbh + 1;
}

void* mem_realloc(void *addr, size_t size)
{
    if (addr == NULL) {
        return mem_alloc(size);
    }
    void* pg_start = get_page_start(addr);
    size_t pg_idx = page_idx(pg_start);
    HeaderType h_type = header_ptrs[pg_idx] & 3;
    void* new_addr;
    switch (h_type) {
    case USED_BIG_BH:;
        UsedBigBH* bh = pg_start;
        if (bh->size >= size + PAGE_SIZE) {
            size_t free_bh_sz = (bh->size - size) / PAGE_SIZE * PAGE_SIZE; // rounding
            void* next_blk_addr = (char*) bh + bh->size;
            set_nxt_blk_prev_size(free_bh_sz, next_blk_addr);
            bh->size -= free_bh_sz;
            FreeBigBH* free_bh = (FreeBigBH*) ((char*) bh + bh->size);
            free_bh->info.prev_size = bh->size;
            free_bh->info.size = free_bh_sz;
            rbtree_insert(free_bh);
            size_t free_bh_idx = page_idx(free_bh);
            header_ptrs[free_bh_idx] = (intptr_t) free_bh | FREE_BIG_BH;
            return addr;
        }
        if (bh->size >= size) {
            return addr;
        }
        void* next_addr = (char*) bh + bh->size;
        if ((char*) next_addr < mem_start + PAGE_NUM * PAGE_SIZE) {
            size_t next_idx = page_idx(next_addr);
            HeaderType next_h_type = header_ptrs[next_idx] & 3;
            if (next_h_type == FREE_BIG_BH) {
                FreeBigBH* next_bh = next_addr;
                if (next_bh->info.size + bh->size <= size + sizeof(UsedBigBH)) {
                    rbtree_delete(next_bh);
                    bh->size += next_bh->info.size;
                    void* next_next_addr = (char*) next_bh + next_bh->info.size;
                    if ((char*) next_next_addr < mem_start + PAGE_NUM * PAGE_SIZE) {
                        set_nxt_blk_prev_size(bh->size, next_next_addr);
                    }
                    return addr;
                }
            }
        }
        new_addr = mem_alloc(size);
        if (new_addr == NULL) {
            return NULL;
        }
        memcpy(new_addr, addr, bh->size);
        mem_free(addr);
        return new_addr;
    case USED_PH:;
        UsedPH* old_ph = (UsedPH*) (header_ptrs[pg_idx] & ~(intptr_t)3);
        size_t old_ph_blk_sz = 1 << old_ph->blk_size_pow2;
        if (old_ph_blk_sz >= size) {
            return addr;
        }
        new_addr = mem_alloc(size);
        if (new_addr == NULL) {
            return NULL;
        }
//        if ((void*) old_ph == pg_start) {
//            memcpy(new_addr, addr, PAGE_SIZE - sizeof(UsedPH));
//        } else {
//            memcpy(new_addr, addr, PAGE_SIZE);
//        }
        memcpy(new_addr, addr, old_ph_blk_sz);
        mem_free(addr);
        return new_addr;
    default:
        printf("ERROR: in mem_realloc()\n");
        fflush(stdout);
        return NULL;
    }
    
}

static void free_block(void* addr, size_t block_size, size_t prev_size)
{
//    printf("\nFREEING %p\nMem dump before\n", addr);
//    mem_dump();
//    printf("The tree\n");
//    rbtree_print(free_big_bhs_root);
    size_t blk_idx = page_idx(addr);
    void* prev_addr = (char*) addr - prev_size;
    size_t prev_idx;
    HeaderType prev_h_type;
    bool hasPrev = prev_size != 0;
    if (hasPrev) {
        prev_idx = page_idx(prev_addr);
        prev_h_type = header_ptrs[prev_idx] & 3;
    }
    size_t next_idx;
    void* next_addr = (char*) addr + block_size;
    bool hasNext = (char*) next_addr < mem_start + PAGE_NUM*PAGE_SIZE;
    HeaderType next_h_type;
    if (hasNext) {
        next_idx = page_idx(next_addr);
        next_h_type = header_ptrs[next_idx] & 3;
    }
    size_t new_size = block_size;
    FreeBigBH* new_blk = addr;
    if (hasPrev) {
        switch (prev_h_type) {
        case FREE_BIG_BH:
            rbtree_delete(prev_addr);
            new_size += prev_size;
            new_blk = prev_addr;
            header_ptrs[prev_idx] = (intptr_t) new_blk | FREE_BIG_BH;
            break;
        case USED_BIG_BH:
        case USED_PH:
            new_blk->info.prev_size = prev_size;
            header_ptrs[blk_idx] = (intptr_t) new_blk | FREE_BIG_BH;
            break;
        default:
            break;
        }
    } else {
        header_ptrs[blk_idx] = (intptr_t) new_blk | FREE_BIG_BH;
    }
    new_blk->info.size = new_size;
    if (hasNext) {
        switch (next_h_type) {
        case FREE_BIG_BH:
            rbtree_delete(next_addr);
            new_size += ((FreeBigBH*) next_addr)->info.size;
            char* next_next_blk = (char*) next_addr + ((FreeBigBH*) next_addr)->info.size;
            new_blk->info.size = new_size;
            if (next_next_blk >= mem_start + PAGE_NUM*PAGE_SIZE) {
                break;
            }
            set_nxt_blk_prev_size(new_size, next_next_blk);
            break;
        case USED_BIG_BH:
        case USED_PH:
            set_nxt_blk_prev_size(new_size, next_addr);
            break;
        default:
            break;
        }
    }
    rbtree_insert(new_blk);
//    printf("Mem dump after\n");
//    mem_dump();
//    printf("The tree\n");
//    rbtree_print(free_big_bhs_root);
//    printf("\n");
}

size_t formatted_prev_idx(int pow2, size_t pg_idx)
{
    size_t prev_idx = SIZE_MAX;
    for (size_t i = formated_page_idxs[pow2]; i != SIZE_MAX;
         i = ((UsedPH*) (header_ptrs[i] & ~(intptr_t)3))->next_ph_idx) {
        if (i >= PAGE_NUM) {
            fflush(stdout);
            printf("Prev idx!\n");
        }
        if (pg_idx == i) {
            return prev_idx;
        }
        prev_idx = i;
    }
    assert(prev_idx == SIZE_MAX);
    printf("ERROR? at formatted_prev_idx, line: %i\n", __LINE__);
    mem_dump();
    fflush(stdout);
    return SIZE_MAX-1;
}

void mem_free(void *addr)
{
    if (addr == NULL) {
        return;
    }
    intptr_t test = (intptr_t) addr & 3;
    if (test != 0) {
        fprintf(stderr, "ERROR: Incorrect address for mem_free()!\n");
    }
    void* page_start = get_page_start(addr);
    size_t pg_idx = page_idx(page_start);
    HeaderType h_type = header_ptrs[pg_idx] & 3;
    switch (h_type) {
    case USED_PH:;
        UsedPH* ph = (UsedPH*) (header_ptrs[pg_idx] & ~(intptr_t)3);
        size_t blk_size = 1;
        blk_size <<= ph->blk_size_pow2;
        int block_idx = ((char*) addr - (char*) page_start) / blk_size;
        size_t blocks_num = PAGE_SIZE / blk_size;
        bool was_full = isFull(ph->usage_mask, blocks_num);
        clrBit(ph->usage_mask, block_idx);
        if ((blk_size == 32 && isFree32(ph->usage_mask, blocks_num))
                || (blk_size != 32 && isFree(ph->usage_mask, blocks_num))) {
            /* remove from formatted pages list */
            size_t f_prev_idx = formatted_prev_idx(ph->blk_size_pow2, pg_idx);
            if (f_prev_idx == SIZE_MAX - 1) {
                formated_page_idxs[ph->blk_size_pow2] = SIZE_MAX;
            } else if (f_prev_idx != SIZE_MAX) {
                UsedPH* list_prev_ph = (UsedPH*) (header_ptrs[f_prev_idx] & ~(intptr_t)3);
                list_prev_ph->next_ph_idx = ph->next_ph_idx;
            } else { // if it's the head of the list
                formated_page_idxs[ph->blk_size_pow2] = ph->next_ph_idx;
            }
            free_block(page_start, PAGE_SIZE, ph->prev_size);
            if (blk_size != 32) {
                mem_free(ph);
            }
        } else if (was_full) {
            ph->next_ph_idx = formated_page_idxs[ph->blk_size_pow2];
            formated_page_idxs[ph->blk_size_pow2] = pg_idx;
        }
        break;
    case USED_BIG_BH:;
        UsedBigBH* bh = (UsedBigBH*) (header_ptrs[pg_idx] & ~(intptr_t)3);
        free_block(bh, bh->size, bh->prev_size);
        break;
    case FREE_BIG_BH:
        printf("ERROR: double free!\n");
        fflush(stdout);
        break;
    default:
        break;
    }
}

void mem_dump(void)
{
    size_t i = 0;
    char* addr = mem_start;
    if (mem_start == NULL) {
        printf("Memory is not initialized\n");
        return;
    }
    while (true) {
        if (i == 115) {
            printf("115...\n");
        }
        HeaderType h_type = header_ptrs[i] & 3;
        switch (h_type) {
        case USED_PH:;
            UsedPH* ph = (UsedPH*) (header_ptrs[i] & ~(intptr_t)3);
            char* used_ph_addr = mem_start+i*PAGE_SIZE;
            size_t block_size = 1 << ph->blk_size_pow2;
            printf("UsedPH:\t%p blk_size: %u usage_mask: ", used_ph_addr, block_size);
            size_t mask_len = (PAGE_SIZE / block_size + 7) /8;
            for (int i = 0; i < mask_len; ++i) {
                printf("0x%1x ", ph->usage_mask[i]);
            }
            printf("\n");
            size_t ph_size = PAGE_SIZE;
            addr += ph_size;
            break;
        case USED_BIG_BH:;
            UsedBigBH* big_bh = (UsedBigBH*) (header_ptrs[i] & ~(intptr_t)3);
            printf("UsedBigBH:\t%p, size: %zu\n", big_bh, big_bh->size);
            addr += big_bh->size;
            assert(big_bh->size != 0);
            break;
        case FREE_BIG_BH:;
            FreeBigBH* free_bh = (FreeBigBH*) (header_ptrs[i] & ~(intptr_t)3);
            printf("FreeBigBH:\t%p, size: %zu\n", free_bh, free_bh->info.size);
            addr += free_bh->info.size;
            assert(free_bh->info.size != 0);
            break;
        default:
            break;
        }
        if (addr >= mem_start + PAGE_NUM * PAGE_SIZE) {
            return;
        }
        i = page_idx(addr);
        fflush(stdout);
    }
}

void mem_dispose(void)
{
    if (mem_start != NULL) {
        munmap(mem_start, PAGE_NUM*PAGE_SIZE);
    }
}
