#ifndef MEM_TYPES_H
#define MEM_TYPES_H

#include <stdlib.h>
#include <stdbool.h>

#define PAGE_NUM 100
#define PAGE_SIZE 4096
#define BLK_MAX_POW2 11

struct used_big_bh {
    struct used_big_bh* prev;
    struct used_big_bh* next; // holds isUsed bit
    size_t size;
};
typedef struct used_big_bh UsedBigBH;
typedef struct used_big_bh FreeBigBHInfo;

typedef bool RBColor;

struct free_big_bh {
    FreeBigBHInfo info;
    struct free_big_bh* rb_left;
    struct free_big_bh* rb_right;
    struct free_big_bh* rb_parent;
    RBColor rb_color;

};
typedef struct free_big_bh FreeBigBH;

struct free_block_header {
    struct free_block_header* next_bh;
};
typedef struct free_block_header FreeBH;

struct free_page {
    struct free_page* next_ph;
    char page_area[PAGE_SIZE - sizeof(struct free_page*)];
};
typedef struct free_page FreePage;

struct used_page_header {
    struct used_page_header* next_ph;
    short blocks_free;
    void* first_block;
    char usage_mask[];
};
typedef struct used_page_header UsedPH;

union header_ptr {
    FreePage* free_page;
    UsedPH* used_ph;
};

#endif // MEM_TYPES_H
