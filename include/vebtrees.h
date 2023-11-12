/* MIT License
 *
 * Copyright (c) 2022 Marco Tröster
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#ifndef VEBTREES_H
#define VEBTREES_H

#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <assert.h>

/* ===================================== *
 *      T Y P E S   /  S T R U C T S
 * ===================================== */

/**
 * @brief Representing a 64-bit integer key managed by van Emde Boas trees.
 * The key space allows any value within [0, u] for a maximum universe u of 2^64.
 * Note that the key 2^64-1 is reserved for vebtree_null, so it cannot be inserted.
 */
typedef uint64_t vebkey_t;

/**
 * @brief This constant is used to represent null keys (e.g. when a key has no successor).
 */
#define vebtree_null 0xFFFFFFFFFFFFFFFF

/**
 * @brief Van Emde Boas tree structure representing a tree with its
 * local child nodes, global registry and high / low pointers.
 */
typedef struct _VEB_TREE_NODE {
    uint8_t universe_bits;
    /**< The universe bits defining the key space managed by the tree node. */
    uint8_t lower_bits;
    /**< The universe bits managed by each local subtree. */
    uint8_t upper_bits;
    /**< The universe bits managed by the global subtree. */
    uint8_t flags;
    /**< A collection of flags adjusting the tree's behavior. */
    vebkey_t low;
    /**< The low pointer representing the smallest key inserted into the tree.
         Note that the low pointer is by definition not inserted into any subtree.
         Moreover, the bitwise tree leafs use the low pointer as bitboards. */
    vebkey_t high;
    /**< The high pointer representing the greatest key inserted into the tree. */
    struct _VEB_TREE_NODE* global;
    /**< The pointer reference to the global subtree. */
    struct _VEB_TREE_NODE* locals;
    /**< The pointer reference to the local subtrees array. */
} VebTree;

/* ===================================== *
 *          F U N C T I O N S
 * ===================================== */

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
 * @brief Indicates whether a tree structure is empty.
 *
 * @param tree the tree to be searched whether it's empty
 * @return true if the tree structure is empty
 * @return false if the tree structure contains keys
 */
bool vebtree_is_empty(VebTree* tree);

/**
 * @brief Retrieves the smallest key inserted into the structure.
 *
 * @param tree the tree to be searched for the smallest key
 * @return the smallest key inserted or vebtree_null if the tree is empty
 */
vebkey_t vebtree_get_min(VebTree* tree);

/**
 * @brief Retrieves the greatest key inserted into the structure.
 *
 * @param tree the tree to be searched for the greatest key
 * @return the greatest key inserted or vebtree_null if the tree is empty
 */
vebkey_t vebtree_get_max(VebTree* tree);

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

/**
 * @brief Retrieve the amount of universe bits required
 * to represent the given maximum key value.
 *
 * @param max_key the maximum key to be inserted into the van Emde Boas tree.
 * @return the universe bits required to represent the max. key
 */
uint8_t vebtree_required_universe_bits(vebkey_t max_key);

/* info: ignore details for Doxygen docs */
#ifndef DOXYGEN_SKIP

/* ===================================== *
 *        B I T S C A N   O P S
 * ===================================== */

typedef uint64_t bitboard_t;

/* use intrinsic processor operations for computing the leading / trailing zero bit count */
#ifdef __GNUC__ /* GCC intrinsics for Linux / Mac */

#define leading_zeros(x) __builtin_clzll(x)
#define trailing_zeros(x) __builtin_ctzll(x)

#define min_bit_set(bits) ((uint8_t)trailing_zeros(bits))
#define max_bit_set(bits) ((uint8_t)(sizeof(bitboard_t) * 8 - leading_zeros(bits) - 1))

#elif _MSC_VER /* MSVC intrinsics for Windows */

uint8_t max_bit_set(bitboard_t bits)
{
    uint32_t idx = 0;
    _BitScanReverse64(&idx, bits);
    return (uint8_t)idx;
}

uint8_t min_bit_set(bitboard_t bits)
{
    uint32_t idx = 0;
    _BitScanForward64(&idx, bits);
    return (uint8_t)idx;
}

#else /* fallback implementation for other compilers / systems */

int leading_zeros(bitboard_t bits)
{
    int count = 0;
    bitboard_t mask = (bitboard_t)1 << (sizeof(bitboard_t) * 8 - 1);

    if (bits == 0)
        return 64;

    while (mask && (bits & mask) == 0) { count++; mask >>= 1; }
    return count;
}

int trailing_zeros(bitboard_t bits)
{
    int count = 0;
    bitboard_t mask = 1;

    if (bits == 0)
        return 64;

    while (mask && (bits & mask) == 0) { count++; mask <<= 1; }
    return count;
}

#define min_bit_set(bits) ((uint8_t)trailing_zeros(bits))
#define max_bit_set(bits) ((uint8_t)(sizeof(bitboard_t) * 8 - leading_zeros(bits) - 1))

#endif
/* TODO: use SIMD instructions to support even bigger leaf universes */

/* ===================================== *
 *        B I T W I S E   L E A F
 * ===================================== */

#define VEBTREE_LEAF_BITS 6
#define vebtree_new_empty_bitwise_leaf(uni_bits) (VebTree){\
    (uni_bits), 0, 0, 0, 0, vebtree_null, NULL, NULL}

#define vebtree_bitwise_leaf_is_empty(tree) ((tree)->low == 0)
#define vebtree_bitwise_leaf_contains_key(tree, key) ((((bitboard_t)1 << (key)) & (tree)->low) > 0)
#define vebtree_bitwise_leaf_get_min(tree) (min_bit_set((tree)->low))
#define vebtree_bitwise_leaf_get_max(tree) (max_bit_set((tree)->low))

#define trailing_bits_mask(num_bits) (((bitboard_t)1 << (num_bits)) - 1)
#define leading_bits_mask(num_bits) (((bitboard_t)0xFFFFFFFFFFFFFFFF << (num_bits)))

vebkey_t vebtree_bitwise_leaf_successor(VebTree* tree, vebkey_t key)
{
    uint64_t succ_bits, min_succ;
    succ_bits = tree->low & leading_bits_mask((uint8_t)key + 1);
    min_succ = min_bit_set(succ_bits);
    return (min_succ == 0 || succ_bits == 0) ? vebtree_null : min_succ;
}

vebkey_t vebtree_bitwise_leaf_predecessor(VebTree* tree, vebkey_t key)
{
    uint64_t pred_bits, max_pred;
    pred_bits = tree->low & trailing_bits_mask((uint8_t)key);
    max_pred = max_bit_set(pred_bits);
    return (max_pred == 0 && (tree->low & 1) == 0) ? vebtree_null : max_pred;
}

void vebtree_bitwise_leaf_insert_key(VebTree* tree, vebkey_t key)
{
    tree->low |= (bitboard_t)1 << key;
}

void vebtree_bitwise_leaf_delete_key(VebTree* tree, vebkey_t key)
{
    tree->low &= ~((bitboard_t)1 << key);
}

/* ===================================== *
 *          T R E E   F L A G S
 * ===================================== */

#define VEBTREE_FLAG_LEAF 1
#define VEBTREE_FLAG_LAZY 2
#define VEBTREE_DEFAULT_FLAGS 0
/* TODO: enable lazy mode once the implementation is ready */

#define vebtree_is_leaf(tree) ((tree)->universe_bits <= VEBTREE_LEAF_BITS)
#define vebtree_is_lazy(tree) ((tree)->flags & VEBTREE_FLAG_LAZY)

/* ===================================== *
 *           V E B   C O R E
 * ===================================== */

#define vebtree_new_empty_node(uni_bits, lower_bits, flags) (VebTree){\
    (uni_bits), (lower_bits), (uni_bits) - (lower_bits), (flags),\
    vebtree_null, vebtree_null, NULL, NULL}

/* TODO: remove those makros, copy the code to the location of usage */
#define vebtree_lower_bits(uni_bits) ((uni_bits) >> 1) /* div by 2 */
#define vebtree_upper_bits(uni_bits) ((uni_bits) - vebtree_lower_bits(uni_bits))

#define vebtree_universe_maxvalue(uni_bits) ((vebkey_t)1 << (uni_bits))
#define vebtree_local_address(key, local_bits) ((((vebkey_t)1 << (local_bits)) - 1) & (key))
#define vebtree_global_address(key, local_bits) ((key) >> (local_bits))

bool vebtree_is_empty(VebTree* tree)
{
    return vebtree_is_leaf(tree)
        ? vebtree_bitwise_leaf_is_empty(tree)
        : (tree)->low == vebtree_null;
}

vebkey_t vebtree_get_min(VebTree* tree)
{
    if (vebtree_is_empty(tree))
        return vebtree_null;

    return vebtree_is_leaf(tree)
        ? vebtree_bitwise_leaf_get_min(tree) : tree->low;
}

vebkey_t vebtree_get_max(VebTree* tree)
{
    if (vebtree_is_empty(tree))
        return vebtree_null;

    return vebtree_is_leaf(tree)
        ? vebtree_bitwise_leaf_get_max(tree) : tree->high;
}

uint8_t vebtree_required_universe_bits(vebkey_t max_key)
{
    assert(max_key != 0 && "universe has to consist of at least 2 keys");
    return max_bit_set(max_key) == 0 ? 64 : max_bit_set(max_key) + 1;
}

void _init_subtrees(VebTree* tree, uint8_t flags);
void _vebtree_init(VebTree* tree, uint8_t universe_bits, uint8_t flags, bool is_memeff_root);

void vebtree_init(VebTree** new_tree, uint8_t universe_bits, uint8_t flags)
{
    assert((universe_bits > 0 && universe_bits <= 64)
        && "invalid amount of universe bits, needs to be within [1, 64].");

    /* allocate memory for the first tree */
    *new_tree = (VebTree*)malloc(sizeof(VebTree));
    _vebtree_init(*new_tree, universe_bits, flags, true);
}

void _vebtree_init(VebTree* tree, uint8_t universe_bits, uint8_t flags, bool is_memeff_root)
{
    uint8_t lower_bits;

    assert((universe_bits > 0 && universe_bits <= 64)
        && "invalid amount of universe bits, needs to be within [1, 64].");

    /* recursion anchor allocating a tree new leaf */
    if (universe_bits <= VEBTREE_LEAF_BITS) {
        *tree = vebtree_new_empty_bitwise_leaf(universe_bits);
        return;
    }

    /* recursion case allocating a tree node */
    lower_bits = is_memeff_root ? VEBTREE_LEAF_BITS : vebtree_lower_bits(universe_bits);
    *tree = vebtree_new_empty_node(universe_bits, lower_bits, flags);

    /* don't allocate the whole tree upfront in case of lazy allocation */
    if (vebtree_is_lazy(tree)) return;

    /* fully allocate the tree recursively */
    _init_subtrees(tree, flags);

    assert(tree->global != NULL && "global tree init failed unexpectedly!");
    assert(tree->locals != NULL && "locals tree init failed unexpectedly!");
}

void _init_subtrees(VebTree* tree, uint8_t flags)
{
    size_t i, num_locals;

    /* determine the sizes of global / locals */
    num_locals = vebtree_universe_maxvalue(tree->upper_bits);

    /* init global recursively */
    tree->global = (VebTree*)malloc(sizeof(VebTree));
    _vebtree_init(tree->global, tree->upper_bits, flags, false);

    /* init locals recursively */
    tree->locals = (VebTree*)malloc(num_locals * sizeof(VebTree));
    for (i = 0; i < num_locals; i++)
        _vebtree_init(tree->locals + i, tree->lower_bits, flags, false);
}

void vebtree_free(VebTree* tree)
{
    size_t i; vebkey_t num_locals;

    /* recursion anchor for tree leafs */
    if (vebtree_is_leaf(tree))
        return;

    /* recursion case for child trees */
    vebtree_free(tree->global);
    num_locals = vebtree_universe_maxvalue(tree->upper_bits);
    for (i = 0; i < num_locals; i++)
        vebtree_free(&(tree->locals[i]));

    /* local memory deallocation */
    free(tree->global);
    free(tree->locals);
    tree->global = NULL;
    tree->locals = NULL;
}

bool vebtree_contains_key(VebTree* tree, vebkey_t key)
{
    vebkey_t local_key, global_key;
    assert(key != vebtree_null && "cannot check for vebtree_null, invalid key!");

    /* base case: encountered tree leaf */
    if (vebtree_is_leaf(tree))
        return vebtree_bitwise_leaf_contains_key(tree, key);

    /* base case: check if key is low (low is not part of any subtree) */
    if (tree->low == key || tree->high == key)
        return true;

    /* base case: stop recursion when tree is empty */
    if (vebtree_is_empty(tree))
        return false;

    /* local subtree exists and the key is part of it (recursion case) */
    assert(tree->global != NULL && tree->locals != NULL);
    local_key = vebtree_local_address(key, tree->lower_bits);
    global_key = vebtree_global_address(key, tree->lower_bits);

    return vebtree_contains_key(tree->global, global_key) &&
           vebtree_contains_key(&(tree->locals[global_key]), local_key);
}

vebkey_t vebtree_successor(VebTree* tree, vebkey_t key)
{
    vebkey_t global_key, local_key, global_succ;

    /* base case for tree leafs */
    if (vebtree_is_leaf(tree))
        return vebtree_bitwise_leaf_successor(tree, key);

    /* base case for predecessor in neighbour local -> low is the successor */
    if (tree->low != vebtree_null && key < tree->low)
        return tree->low;

    local_key = vebtree_local_address(key, tree->lower_bits);
    global_key = vebtree_global_address(key, tree->lower_bits);

    /* case where a local contains the successor */
    if (vebtree_get_max(&(tree->locals[global_key])) != vebtree_null &&
            local_key < vebtree_get_max(&(tree->locals[global_key])))
        return (global_key << tree->lower_bits) |
            (vebtree_successor(&(tree->locals[global_key]), local_key));

    /* case where a neighbour contains the successor */
    global_succ = vebtree_successor(tree->global, global_key);
    return global_succ == vebtree_null ? vebtree_null
        : (global_succ << tree->lower_bits) | vebtree_get_min(&(tree->locals[global_succ]));
}

vebkey_t vebtree_predecessor(VebTree* tree, vebkey_t key)
{
    /* TODO: implement this analog to the successor function */
    assert(false && "this operation is currently not supported");
    return vebtree_null;
}

void vebtree_insert_key(VebTree* tree, vebkey_t key)
{
    vebkey_t global_key, local_key, temp;
    assert(key != vebtree_null && "cannot insert vebtree_null, invalid key!");

    /* base case for tree leafs */
    if (vebtree_is_leaf(tree)) { vebtree_bitwise_leaf_insert_key(tree, key); return; }

    /* base case when tree is empty */
    if (vebtree_is_empty(tree)) { tree->low = tree->high = key; return; }

    /* case when the key becomes the new low -> insert old low instead */
    if (key < tree->low) { temp = tree->low; tree->low = key; key = temp; }

    local_key = vebtree_local_address(key, tree->lower_bits);
    global_key = vebtree_global_address(key, tree->lower_bits);

    /* insert the global key if the corresponding local is empty */
    if (vebtree_is_empty(&(tree->locals[global_key])))
        vebtree_insert_key(tree->global, global_key);

    /* insert the local key into local scope */
    vebtree_insert_key(&(tree->locals[global_key]), local_key);

    /* update the tree's high */
    tree->high = tree->high > key ? tree->high : key;
}

void vebtree_delete_key(VebTree* tree, vebkey_t key)
{
    vebkey_t global_key, local_key, global_high, global_low;
    assert(key != vebtree_null && "cannot delete vebtree_null, invalid key!");

    /* base case for tree leafs */
    if (vebtree_is_leaf(tree)) { vebtree_bitwise_leaf_delete_key(tree, key); return; }

    /* base case with only one element -> set low and high to null */
    if (tree->low == tree->high) { tree->low = tree->high = vebtree_null; return; }

    /* case when deleting the low element -> new low needs to be pulled out */
    if (key == tree->low) {
        global_low = vebtree_get_min(tree->global);
        tree->low = key = (global_low << tree->lower_bits)
            | vebtree_get_min(&(tree->locals[global_low]));
    }

    global_key = vebtree_global_address(key, tree->lower_bits);
    local_key = vebtree_local_address(key, tree->lower_bits);

    /* delete the local key recursively */
    vebtree_delete_key(&(tree->locals[global_key]), local_key);

    if (vebtree_is_empty(&(tree->locals[global_key])))
        vebtree_delete_key(tree->global, global_key);

    /* in case the maximum was deleted -> find new maximum */
    if (key == tree->high) {
        global_high = vebtree_get_max(tree->global);
        tree->high = global_high == vebtree_null ? tree->low
            : (global_high << tree->lower_bits) | vebtree_get_max(&(tree->locals[global_high]));
    }
}

#endif /* DOXYGEN_SKIP */
#endif /* VEBTREES_H */
