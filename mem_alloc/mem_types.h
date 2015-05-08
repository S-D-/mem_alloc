#ifndef MEM_TYPES_H
#define MEM_TYPES_H

#include <stdlib.h>
#include <stdbool.h>

#define PAGE_NUM 100
#define PAGE_SIZE 4096
#define BLK_MAX_POW2 11

struct used_big_bh {
    size_t prev_size;
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

struct used_page_header {
    size_t prev_size; 
    size_t next_ph_idx;
    unsigned char blk_size_pow2;
    char usage_mask[];
};
typedef struct used_page_header UsedPH;

enum header_type {
    USED_PH,
    USED_BIG_BH,
    FREE_BIG_BH
};
typedef enum header_type HeaderType;

#endif // MEM_TYPES_H
