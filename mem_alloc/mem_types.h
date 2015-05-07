#ifndef MEM_TYPES_H
#define MEM_TYPES_H

#include <stdlib.h>
#include <stdbool.h>

#define PAGE_NUM 100
#define PAGE_SIZE 4096
#define BLK_MAX_POW2 11

struct used_big_bh {
    size_t prev_size; // holds isUsed bit
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

//struct free_block_header {
//    struct free_block_header* next_bh;
//};
//typedef struct free_block_header FreeBH;

//struct free_page {
//    struct free_page* next_ph;
//    char page_area[PAGE_SIZE - sizeof(struct free_page*)];
//};
//typedef struct free_page FreePage;

struct used_page_header {
    size_t prev_size; // TODO mb make isUsed here - no
    struct used_page_header* next_ph;
    //short blocks_free;
    void* first_block;
    char usage_mask[];
};
typedef struct used_page_header UsedPH;

enum header_type {
    USED_PH,
    USED_PH32,
    USED_BIG_BH,
    FREE_BIG_BH
};
typedef enum header_type HeaderType;

struct header_location {
    void* addr;
    enum header_type h_type;
};
typedef struct header_location HeaderLocation;

union header_ptr {
    FreeBigBH* free_page;
    UsedBigBH* used_big_bh;
    UsedPH* used_ph;
};

#endif // MEM_TYPES_H
