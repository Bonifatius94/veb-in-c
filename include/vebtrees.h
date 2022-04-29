#ifndef VEBTREES_H
#define VEBTREES_H

#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <assert.h>

typedef uint64_t vebkey_t;
#define vebtree_null 0xFFFFFFFFFFFFFFFF

typedef uint64_t bitboard_t;

#define VEBTREE_FLAG_LEAF 1
#define VEBTREE_FLAG_LAZY 2
#define VEBTREE_DEFAULT_FLAGS 0
/* TODO: enable lazy mode once the implementation is ready */

typedef struct _VEB_TREE_NODE {
    uint8_t universe_bits;
    uint8_t lower_bits;
    uint8_t upper_bits;
    uint8_t flags;
    vebkey_t low;
    vebkey_t high;
    struct _VEB_TREE_NODE* global;
    struct _VEB_TREE_NODE* locals;
} VebTree;

#define vebtree_is_leaf(tree) ((tree)->flags & VEBTREE_FLAG_LEAF || (tree)->universe_bits <= 6)
#define vebtree_is_lazy(tree) ((tree)->flags & VEBTREE_FLAG_LAZY)
#define vebtree_bitwise_leaf_is_empty(tree) ((tree)->low == 0)

#define vebtree_is_empty(tree) (vebtree_is_leaf(tree) \
    ? vebtree_bitwise_leaf_is_empty(tree) : (tree)->low == vebtree_null)
#define vebtree_get_min(tree) ((tree)->low)
#define vebtree_get_max(tree) ((tree)->high)

void vebtree_init(VebTree** tree, uint8_t universe_bits, uint8_t flags);

void vebtree_free(VebTree* tree);

bool vebtree_contains_key(VebTree* tree, vebkey_t key);

vebkey_t vebtree_successor(VebTree* tree, vebkey_t key);

vebkey_t vebtree_predecessor(VebTree* tree, vebkey_t key);

void vebtree_insert_key(VebTree* tree, vebkey_t key);

void vebtree_delete_key(VebTree* tree, vebkey_t key);

/* TODO: test intrinsics for other systems / compilers */

/* use intrinsic processor operations for leading / trailing bits count */
#ifdef __GNUC__ /* GCC intrinsics for Linux / Mac */

#define leading_zeros(x) __builtin_clzll((x))
#define trailing_zeros(x) __builtin_ctzll((x))

#elif _MSC_VER /* MSVC intrinsics for Windows */

#define leading_zeros(x) __lzcnt64((x))
#define trailing_zeros(x) __tzcnt64((x))

#else /* fallback implementations for other systems / compilers */

/* TODO: fallback doesn't compile, figure out why and fix this */

int tab64[64] = {
    63,  0, 58,  1, 59, 47, 53,  2,
    60, 39, 48, 27, 54, 33, 42,  3,
    61, 51, 37, 40, 49, 18, 28, 20,
    55, 30, 34, 11, 43, 14, 22,  4,
    62, 57, 46, 52, 38, 26, 32, 41,
    50, 36, 17, 19, 29, 10, 13, 21,
    56, 45, 25, 31, 35, 16,  9, 12,
    44, 24, 15,  8, 23,  7,  6,  5
};

int leading_zeros(uint64_t value)
{
    /* De-Bruijn algorithm with logarithmic time complexity */
    value |= value >> 1;
    value |= value >> 2;
    value |= value >> 4;
    value |= value >> 8;
    value |= value >> 16;
    value |= value >> 32;
    return tab64[((uint64_t)((value - (value >> 1))*0x07EDD5E59A4E28C2)) >> 58];
}

int trailing_zeros(uint64_t value)
{
    /* TODO: find a better implementation, this is pretty bad */

    /* naive bit scan algorithm */
    int count = 0;

    if (value == 0) {
        return 64;
    }

    while (value & 1 == 0) {
        count++;
        value >>= 1;
    }

    return count;
}

#endif

#endif
