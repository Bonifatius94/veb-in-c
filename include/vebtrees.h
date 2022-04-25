#ifndef VEBTREES_H
#define VEBTREES_H

#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <assert.h>

typedef uint64_t vebkey_t;
#define vebtree_null 0xFFFFFFFFFFFFFFFF

typedef struct _VEB_TREE_NODE {
    // TODO: replace universe bits with lower / upper bits
    uint8_t universe_bits;
    // uint8_t lower_bits;
    // uint8_t upper_bits;
    // uint8_t flags;
    vebkey_t low;
    vebkey_t high;
    struct _VEB_TREE_NODE* global;
    struct _VEB_TREE_NODE* locals;
} VebTree;

typedef struct _VEB_TREE_BITWISE_LEAF {
    uint8_t universe_bits;
    vebkey_t bitboard;
} VebTreeBitwiseLeaf;

void vebtree_init(VebTree** tree, uint8_t universe_bits, bool is_lazy);

void vebtree_free(VebTree* tree);

bool vebtree_contains_key(VebTree* tree, vebkey_t key);

bool vebtree_is_empty(VebTree* tree);

vebkey_t vebtree_get_min(VebTree* tree);

vebkey_t vebtree_get_max(VebTree* tree);

vebkey_t vebtree_successor(VebTree* tree, vebkey_t key);

vebkey_t vebtree_predecessor(VebTree* tree, vebkey_t key);

void vebtree_insert_key(VebTree* tree, vebkey_t key);

void vebtree_delete_key(VebTree* tree, vebkey_t key);

#endif
