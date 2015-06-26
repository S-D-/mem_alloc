#ifndef MEM_RB_TREE_H
#define MEM_RB_TREE_H
#include "mem_types.h"

void set_rb_root_var(FreeBigBH** root_pp);
void rbtree_insert(FreeBigBH* ins_node);
void rbtree_delete(FreeBigBH* del_node);
FreeBigBH* rbtree_lookup(size_t size);
void rbtree_print(FreeBigBH* root_node);

#endif // MEM_RB_TREE_H
