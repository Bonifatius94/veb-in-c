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

#define vebtree_is_leaf(tree) ((tree)->flags & VEBTREE_FLAG_LEAF || (tree)->universe_bits <= 6)
#define vebtree_is_lazy(tree) ((tree)->flags & VEBTREE_FLAG_LAZY)

/**
 * @brief Van Emde Boas tree structure representing a tree with its
 * local child nodes, global registry and high / low pointers.
 */
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

/**
 * @brief Create a new van Emde Boas tree structure.
 * 
 * @param tree a reference pointer that is set to the
 *             newly allocated tree structure's reference.
 * @param universe_bits the universe size to be managed by the tree in bits
 * @param flags a collection of flags adjusting the tree's behavior
 */
void vebtree_init(VebTree** tree, uint8_t universe_bits, uint8_t flags);

/**
 * @brief Free the memory allocated by the given van Emde Boas tree.
 * 
 * @param tree the tree to be freed.
 */
void vebtree_free(VebTree* tree);

/**
 * @brief Inidicates whether a van Emde Boas tree contains the given key.
 * 
 * @param tree the tree to be looked up for the key
 * @param key the key to be looked up
 * @return true if the tree contains the key
 * @return false if the tree doesn't contan the key
 */
bool vebtree_contains_key(VebTree* tree, vebkey_t key);

/**
 * @brief Retrieve the key's successor.
 * 
 * @param tree the tree to be looked up for the key's successor
 * @param key the key to be looked up
 * @return the key's successor or vebtree_null if the key has no successor
 */
vebkey_t vebtree_successor(VebTree* tree, vebkey_t key);

/**
 * @brief Retrieve the key's predecessor.
 * 
 * @param tree the tree to be looked up for the key's predecessor
 * @param key the key to be looked up
 * @return the key's predecessor or vebtree_null if the key has no predecessor
 */
vebkey_t vebtree_predecessor(VebTree* tree, vebkey_t key);

/**
 * @brief Insert the given key into the data structure.
 * 
 * @param tree the tree to be inserted into
 * @param key the key to be inserted
 */
void vebtree_insert_key(VebTree* tree, vebkey_t key);

/**
 * @brief Delete the given key from the data structure.
 * 
 * @param tree the tree to be deleted from
 * @param key the key to be deleted
 */
void vebtree_delete_key(VebTree* tree, vebkey_t key);

/* use intrinsic processor operations for computing the leading / trailing zero bit count */
#ifdef __GNUC__ /* GCC intrinsics for Linux / Mac */

#define leading_zeros(x) __builtin_clzll(x)
#define trailing_zeros(x) __builtin_ctzll(x)

#elif _MSC_VER /* MSVC intrinsics for Windows */

#define leading_zeros(x) __lzcnt64(x)
#define trailing_zeros(x) __tzcnt64(x)

#else /* fallback implementation for other compilers / systems */

int leading_zeros(bitboard_t bits);
int trailing_zeros(bitboard_t bits);

int leading_zeros(bitboard_t bits)
{
    int count = 0;

    if (bits == 0)
        return 64;

    while (bits & (0x800000000000000ull >> count++) == 0) {}
    return count - 1;
}

int trailing_zeros(bitboard_t bits)
{
    int count = 0;

    if (bits == 0)
        return 64;

    while (bits & (0x1ull << count++) == 0) {}
    return count - 1;
}

#endif
/* TODO: use SIMD instructions to support bigger leaf universes */

#define min_bit_set(bits) (trailing_zeros(bits))
#define max_bit_set(bits) (sizeof(bitboard_t) * 8 - leading_zeros(bits) - 1)

#define vebtree_bitwise_leaf_is_empty(tree) ((tree)->low == 0)
#define vebtree_bitwise_leaf_contains_key(tree, key) ((((bitboard_t)1 << (key)) & (tree)->low) > 0)
#define vebtree_bitwise_leaf_get_min(tree) (min_bit_set((tree)->low))
#define vebtree_bitwise_leaf_get_max(tree) (max_bit_set((tree)->low))

/**
 * @brief Indicates whether a tree structure is empty.
 * 
 * @param tree the tree to be searched whether it's empty
 * @return true if the tree structure is empty
 * @return false if the tree structure contains keys
 */
#define vebtree_is_empty(tree) (vebtree_is_leaf(tree) \
    ? vebtree_bitwise_leaf_is_empty(tree) : (tree)->low == vebtree_null)

#define vebtree_get_min_nonempty(tree) (vebtree_is_leaf(tree) \
    ? vebtree_bitwise_leaf_get_min(tree) : (tree)->low)
#define vebtree_get_max_nonempty(tree) (vebtree_is_leaf(tree) \
    ? vebtree_bitwise_leaf_get_max(tree) : (tree)->high)

/**
 * @brief Retrieves the smallest key inserted into the structure.
 * 
 * @param tree the tree to be searched for the smallest key
 * @return the smallest key inserted or vebtree_null if the tree is empty
 */
#define vebtree_get_min(tree) (vebtree_is_empty(tree) \
    ? vebtree_null : vebtree_get_min_nonempty(tree))

/**
 * @brief Retrieves the greatest key inserted into the structure.
 * 
 * @param tree the tree to be searched for the greatest key
 * @return the greatest key inserted or vebtree_null if the tree is empty
 */
#define vebtree_get_max(tree) (vebtree_is_empty(tree) \
    ? vebtree_null : vebtree_get_max_nonempty(tree))

#endif
