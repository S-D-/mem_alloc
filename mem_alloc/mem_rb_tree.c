#include "mem_rb_tree.h"
#include <assert.h>
#include <stdio.h>

typedef FreeBigBH* Node;
static const RBColor BLACK = false;
static const RBColor RED = true;
static Node* rb_root_pp;

void set_rb_root_var(FreeBigBH** root_pp)
{
    rb_root_pp = root_pp;
}

static int node_cmp(Node n1, Node n2)
{
    if (n1 == n2) {
        return 0;
    }
    if (n1->info.size == n2->info.size) {
        return n1 > n2? 1: -1;
    }
    return n1->info.size > n2->info.size? 1: -1;
}

static Node grandparent(Node n) {
    assert (n != NULL);
    assert (n->rb_parent != NULL); /* Not the root node */
    assert (n->rb_parent->rb_parent != NULL); /* Not child of root */
    return n->rb_parent->rb_parent;
}

static Node sibling(Node n) {
    assert (n != NULL);
    assert (n->rb_parent != NULL); /* Root node has no sibling */
    if (n == n->rb_parent->rb_left)
        return n->rb_parent->rb_right;
    else
        return n->rb_parent->rb_left;
}

static Node uncle(Node n) {
    assert (n != NULL);
    assert (n->rb_parent != NULL); /* Root node has no uncle */
    assert (n->rb_parent->rb_parent != NULL); /* Children of root have no uncle */
    return sibling(n->rb_parent);
}

//static Node lookup_node(Node key_node) {
//    Node n = *rb_root_pp;
//    while (n != NULL) {
//        int comp_result = node_cmp(key_node, n);
//        if (comp_result == 0) {
//            return n;
//        } else if (comp_result < 0) {
//            n = n->rb_left;
//        } else {
//            assert(comp_result > 0);
//            n = n->rb_right;
//        }
//    }
//    return n;
//}

FreeBigBH *rbtree_lookup(size_t size) 
{
    Node n = *rb_root_pp;
    while (n != NULL) {
        if (size <= n->info.size) {
            return n;
        } else {
            n = n->rb_right;
        }
    }
    return n;
}

static void replace_node(Node oldn, Node newn) {
    if (oldn->rb_parent == NULL) {
        *rb_root_pp = newn;
    } else {
        if (oldn == oldn->rb_parent->rb_left)
            oldn->rb_parent->rb_left = newn;
        else
            oldn->rb_parent->rb_right = newn;
    }
    if (newn != NULL) {
        newn->rb_parent = oldn->rb_parent;
    }
}

static void rotate_left(Node n) {
    Node r = n->rb_right;
    replace_node(n, r);
    n->rb_right = r->rb_left;
    if (r->rb_left != NULL) {
        r->rb_left->rb_parent = n;
    }
    r->rb_left = n;
    n->rb_parent = r;
}

static void rotate_right(Node n) {
    Node L = n->rb_left;
    replace_node(n, L);
    n->rb_left = L->rb_right;
    if (L->rb_right != NULL) {
        L->rb_right->rb_parent = n;
    }
    L->rb_right = n;
    n->rb_parent = L;
}

static void insert_case1(Node n);
static void insert_case2(Node n);
static void insert_case3(Node n);
static void insert_case4(Node n);
static void insert_case5(Node n);

void rbtree_insert(FreeBigBH* ins_node) 
{
    //printf("Inserting %p\n", ins_node);
   // node inserted_node = new_node(key, value, RED, NULL, NULL);
    ins_node->rb_color = RED;
    ins_node->rb_left = ins_node->rb_right = NULL;
    if (*rb_root_pp == NULL) {
        *rb_root_pp = ins_node;
    } else {
        Node n = *rb_root_pp;
        while (1) {
            int comp_result = node_cmp(ins_node, n);
            if (comp_result == 0) {
                n->info = ins_node->info;
                printf("ERROR: RB\n");
                //n->value = value;
                return;
            } else if (comp_result < 0) {
                if (n->rb_left == NULL) {
                    n->rb_left = ins_node;
                    break;
                } else {
                    n = n->rb_left;
                }
            } else {
                assert (comp_result > 0);
                if (n->rb_right == NULL) {
                    n->rb_right = ins_node;
                    break;
                } else {
                    n = n->rb_right;
                }
            }
        }
        ins_node->rb_parent = n;
    }
    insert_case1(ins_node);
}

static void insert_case1(Node n) {
    if (n->rb_parent == NULL)
        n->rb_color = BLACK;
    else
        insert_case2(n);
}

static RBColor node_color(Node n)
{
    return n == NULL? BLACK: n->rb_color;
}

static void insert_case2(Node n) {
    if (node_color(n->rb_parent) == BLACK)
        return; /* Tree is still valid */
    else
        insert_case3(n);
}

static void insert_case3(Node n) {
    if (node_color(uncle(n)) == RED) {
        n->rb_parent->rb_color = BLACK;
        uncle(n)->rb_color = BLACK;
        grandparent(n)->rb_color = RED;
        insert_case1(grandparent(n));
    } else {
        insert_case4(n);
    }
}

static void insert_case4(Node n) {
    if (n == n->rb_parent->rb_right && n->rb_parent == grandparent(n)->rb_left) {
        rotate_left(n->rb_parent);
        n = n->rb_left;
    } else if (n == n->rb_parent->rb_left && n->rb_parent == grandparent(n)->rb_right) {
        rotate_right(n->rb_parent);
        n = n->rb_right;
    }
    insert_case5(n);
}

static void insert_case5(Node n) {
    n->rb_parent->rb_color = BLACK;
    grandparent(n)->rb_color = RED;
    if (n == n->rb_parent->rb_left && n->rb_parent == grandparent(n)->rb_left) {
        rotate_right(grandparent(n));
    } else {
        assert (n == n->rb_parent->rb_right && n->rb_parent == grandparent(n)->rb_right);
        rotate_left(grandparent(n));
    }
}

static Node maximum_node(Node n) {
    assert (n != NULL);
    while (n->rb_right != NULL) {
        n = n->rb_right;
    }
    return n;
}

static void delete_case1(Node n);
static void delete_case2(Node n);
static void delete_case3(Node n);
static void delete_case4(Node n);
static void delete_case5(Node n);
static void delete_case6(Node n);

// TODO fix dublicated values case.
void rbtree_delete(FreeBigBH *del_node) 
{
    //printf("Deleting %p\n", del_node);
    Node child;
    Node n = del_node; //was Node n = lookup_node(del_node);
    Node pred = NULL;
    if (n == NULL) return;  /* Key not found, do nothing */
    if (n->rb_left != NULL && n->rb_right != NULL) {
        /* Copy key/value from predecessor and then delete it instead */
        pred = maximum_node(n->rb_left);
//        n->key   = pred->key;
//        n->value = pred->value;
        
        //was: n->info = pred->info; // TODO fix it!
        n = pred;
    }

    assert(n->rb_left == NULL || n->rb_right == NULL);
    child = n->rb_right == NULL ? n->rb_left  : n->rb_right;
    if (node_color(n) == BLACK) {
        n->rb_color = node_color(child);
        delete_case1(n);
    }
    replace_node(n, child);
    if (n->rb_parent == NULL && child != NULL) // root should be black
        child->rb_color = BLACK;
    
    //TODO check this:
    if (pred != NULL) {
        replace_node(del_node, pred);
    }
    //free(n);
}

static void delete_case1(Node n) {
    if (n->rb_parent == NULL)
        return;
    else
        delete_case2(n);
}

static void delete_case2(Node n) {
    if (node_color(sibling(n)) == RED) {
        n->rb_parent->rb_color = RED;
        sibling(n)->rb_color = BLACK;
        if (n == n->rb_parent->rb_left)
            rotate_left(n->rb_parent);
        else
            rotate_right(n->rb_parent);
    }
    delete_case3(n);
}

static void delete_case3(Node n) {
    if (node_color(n->rb_parent) == BLACK &&
        node_color(sibling(n)) == BLACK &&
        node_color(sibling(n)->rb_left) == BLACK &&
        node_color(sibling(n)->rb_right) == BLACK)
    {
        sibling(n)->rb_color = RED;
        delete_case1(n->rb_parent);
    }
    else
        delete_case4(n);
}

static void delete_case4(Node n) {
    if (node_color(n->rb_parent) == RED &&
        node_color(sibling(n)) == BLACK &&
        node_color(sibling(n)->rb_left) == BLACK &&
        node_color(sibling(n)->rb_right) == BLACK)
    {
        sibling(n)->rb_color = RED;
        n->rb_parent->rb_color = BLACK;
    }
    else
        delete_case5(n);
}

static void delete_case5(Node n) {
    if (n == n->rb_parent->rb_left &&
        node_color(sibling(n)) == BLACK &&
        node_color(sibling(n)->rb_left) == RED &&
        node_color(sibling(n)->rb_right) == BLACK)
    {
        sibling(n)->rb_color = RED;
        sibling(n)->rb_left->rb_color = BLACK;
        rotate_right(sibling(n));
    }
    else if (n == n->rb_parent->rb_right &&
             node_color(sibling(n)) == BLACK &&
             node_color(sibling(n)->rb_right) == RED &&
             node_color(sibling(n)->rb_left) == BLACK)
    {
        sibling(n)->rb_color = RED;
        sibling(n)->rb_right->rb_color = BLACK;
        rotate_left(sibling(n));
    }
    delete_case6(n);
}

static void delete_case6(Node n) {
    sibling(n)->rb_color = node_color(n->rb_parent);
    n->rb_parent->rb_color = BLACK;
    if (n == n->rb_parent->rb_left) {
        assert (node_color(sibling(n)->rb_right) == RED);
        sibling(n)->rb_right->rb_color = BLACK;
        rotate_left(n->rb_parent);
    }
    else
    {
        assert (node_color(sibling(n)->rb_left) == RED);
        sibling(n)->rb_left->rb_color = BLACK;
        rotate_right(n->rb_parent);
    }
}
