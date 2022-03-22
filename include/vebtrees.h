#ifndef VEBTREES_H
#define VEBTREES_H

#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <assert.h>

typedef uint64_t VebKey;
#define VEB_NULL 0xFFFFFFFFFFFFFFFF

typedef struct _VEB_TREE_NODE {
    uint8_t universe_bits;
    VebKey low;
    VebKey high;
    struct _VEB_TREE_NODE* global;
    struct _VEB_TREE_NODE* locals;
} VebTree;

typedef struct _VEB_TREE_BITWISE_LEAF {
    uint8_t universe_bits;
    VebKey bitboard;
} VebTreeBitwiseLeaf;

void vebtree_init(VebTree** tree, uint8_t universe_bits, bool is_lazy);

void vebtree_free(VebTree* tree);

bool vebtree_contains_key(VebTree* tree, VebKey key);

bool vebtree_is_empty(VebTree* tree);

VebKey vebtree_get_min(VebTree* tree);

VebKey vebtree_get_max(VebTree* tree);

VebKey vebtree_successor(VebTree* tree, VebKey key);

VebKey vebtree_predecessor(VebTree* tree, VebKey key);

void vebtree_insert_key(VebTree* tree, VebKey key);

void vebtree_delete_key(VebTree* tree, VebKey key);

#endif
