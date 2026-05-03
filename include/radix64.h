/* MIT License
 *
 * Copyright (c) 2026 Marco Tröster
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

#ifndef RADIX64_H
#define RADIX64_H

#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <assert.h>

/* ===================================== *
 *      T Y P E S   /  S T R U C T S
 * ===================================== */

/**
 * @brief Integer key for the radix64 tree. Keys live in [0, 2^universe_bits)
 * with universe_bits in [1, 32]. The value r64_null is reserved as a sentinel
 * and cannot be inserted.
 */
typedef uint64_t r64key_t;

/**
 * @brief Sentinel key. Returned by lookup ops to mean "no such key" and
 * forbidden as an insert argument (asserted).
 */
#define r64_null 0xFFFFFFFFFFFFFFFFULL

/**
 * @brief One node of the radix64 tree.
 *
 * Uniform 64-way node. `bitmap` is an occupancy summary for the 64 slots;
 * the union's interpretation depends on the node's level:
 *
 *   - At internal levels 0 .. depth - 3: `u.children[i]` points to the
 *     child subtree for slot i (NULL iff absent; matches bitmap).
 *   - At level depth - 2 (leaf-parent): `u.leaves[i]` is a 64-bit bitmap
 *     encoding the final-nibble presence for keys whose leaf-parent
 *     nibble is i. Leaves are inlined here rather than allocated as
 *     separate nodes — a 65× RAM win on dense workloads.
 *   - When depth == 1 (universe fits in 6 bits), the single root node
 *     stores keys directly in `bitmap` and `u` is unused.
 *
 * All allocation is lazy: a node is calloc'd only on the first insert
 * that traverses it and freed the moment its bitmap clears on delete.
 * sizeof(Radix64Node) == 520 B.
 */
typedef struct _RADIX64_NODE {
    uint64_t bitmap;
    /**< Slot-occupancy summary; bit i set iff slot i is populated. */
    union {
        struct _RADIX64_NODE* children[64];
        /**< Internal levels: child pointers, NULL iff slot absent. */
        uint64_t leaves[64];
        /**< Leaf-parent level: inline final-nibble bitmaps. */
    } u;
} Radix64Node;

/**
 * @brief Handle for a radix64 tree. `root` is NULL until the first insert.
 */
typedef struct _RADIX64 {
    uint8_t universe_bits;
    /**< Universe size in bits, in [1, 32]. */
    uint8_t depth;
    /**< Number of 6-bit levels; equals ceil(universe_bits / 6). */
    Radix64Node* root;
    /**< NULL iff the tree has never had a key inserted. May also be NULL
         transiently after the last key is deleted (empty tree). */
} Radix64;

/* ===================================== *
 *          F U N C T I O N S
 * ===================================== */

/**
 * @brief Create an empty radix64 tree for keys in [0, 2^universe_bits).
 *
 * @param tree          output pointer; *tree receives the new handle
 * @param universe_bits key universe bits; asserted to be in [1, 32]
 */
void radix64_init(Radix64** tree, uint8_t universe_bits);

/**
 * @brief Free every allocated node and the tree handle. Safe on empty trees.
 *
 * @param tree the tree to free; accepts NULL-rooted (never-inserted) trees
 */
void radix64_free(Radix64* tree);

/**
 * @brief Test whether a key is present.
 *
 * @param tree the tree to query
 * @param key  the key to look up; asserted != r64_null
 * @return true iff the key has been inserted and not deleted
 */
bool radix64_contains_key(Radix64* tree, r64key_t key);

/**
 * @brief Test whether the tree holds any key.
 *
 * @param tree the tree to query
 * @return true iff no keys are currently stored
 */
bool radix64_is_empty(Radix64* tree);

/**
 * @brief Retrieve the smallest stored key.
 *
 * @param tree the tree to query
 * @return the smallest key, or r64_null if the tree is empty
 */
r64key_t radix64_get_min(Radix64* tree);

/**
 * @brief Retrieve the largest stored key.
 *
 * @param tree the tree to query
 * @return the largest key, or r64_null if the tree is empty
 */
r64key_t radix64_get_max(Radix64* tree);

/**
 * @brief Smallest stored key strictly greater than `key`.
 *
 * @param tree the tree to query
 * @param key  query key; need not be present in the tree
 * @return the successor, or r64_null if no stored key is greater
 */
r64key_t radix64_successor(Radix64* tree, r64key_t key);

/**
 * @brief Largest stored key strictly less than `key`.
 *
 * @param tree the tree to query
 * @param key  query key; need not be present in the tree
 * @return the predecessor, or r64_null if no stored key is smaller
 */
r64key_t radix64_predecessor(Radix64* tree, r64key_t key);

/**
 * @brief Insert a key. Inserting a key that is already present is a no-op.
 *
 * @param tree the tree to modify
 * @param key  the key to insert; asserted != r64_null and within the universe
 */
void radix64_insert_key(Radix64* tree, r64key_t key);

/**
 * @brief Delete a key. Deleting a key that is not present is a no-op.
 *
 * @param tree the tree to modify
 * @param key  the key to delete; asserted != r64_null
 */
void radix64_delete_key(Radix64* tree, r64key_t key);

/**
 * @brief Universe bits required to represent `max_key`.
 *
 * Thin wrapper over the high-bit position of `max_key`. For max_key == 0,
 * returns 64 (same latent-bug shape as vebtree_required_universe_bits).
 *
 * @param max_key largest key the caller intends to store; must be != 0
 * @return the required universe_bits
 */
uint8_t radix64_required_universe_bits(r64key_t max_key);

/* info: ignore implementation details for Doxygen docs */
#ifndef DOXYGEN_SKIP

/* ===================================== *
 *        B I T S C A N   O P S
 * ===================================== */

/* Reuse the same tri-backend pattern as vebtrees.h. All three return an
   undefined result when the input is 0; callers MUST guard bitmap != 0. */

#ifdef __GNUC__

#define r64_ctz(x) ((uint8_t)__builtin_ctzll((unsigned long long)(x)))
#define r64_clz(x) ((uint8_t)__builtin_clzll((unsigned long long)(x)))

#elif defined(_MSC_VER)

static uint8_t r64_ctz(uint64_t x)
{
    unsigned long idx;
    _BitScanForward64(&idx, x);
    return (uint8_t)idx;
}

static uint8_t r64_clz(uint64_t x)
{
    unsigned long idx;
    _BitScanReverse64(&idx, x);
    return (uint8_t)(63u - idx);
}

#else

static uint8_t r64_ctz(uint64_t x)
{
    uint8_t c = 0;
    while ((x & 1ULL) == 0) { x >>= 1; c++; }
    return c;
}

static uint8_t r64_clz(uint64_t x)
{
    uint8_t c = 0;
    while ((x & (1ULL << 63)) == 0) { x <<= 1; c++; }
    return c;
}

#endif

/* Mask helpers. Both assume n in [0, 63] — n == 64 is UB on uint64_t shifts. */
#define r64_trailing_mask(n) (((uint64_t)1 << (n)) - 1)
#define r64_leading_mask(n)  (~r64_trailing_mask(n))

/* Maximum depth bound. depth = ceil(universe_bits / 6); universe_bits <= 32
   implies depth <= 6. Path/nibble arrays use +2 margin. */
#define R64_MAX_DEPTH 8

/* ===================================== *
 *     N I B B L E   &   K E Y   M A T H
 * ===================================== */

/* Level 0 is the top (bits nearest the MSB of the key). Level depth-1 is
   the leaf level (bits nearest the LSB). Each level consumes up to 6 bits;
   the top level may consume fewer (see radix64_build_key for the exact
   bit budget). */
static uint8_t radix64_nibble(r64key_t key, uint8_t level, uint8_t depth)
{
    uint8_t below_this;
    below_this = (uint8_t)((depth - 1 - level) * 6);
    return (uint8_t)((key >> below_this) & 0x3F);
}

/* Reconstruct a key from a path of nibbles. Top level contributes only
   `universe_bits - (depth - 1) * 6` bits (always in [1, 6]); remaining
   levels contribute the full 6. */
static r64key_t radix64_build_key(const uint8_t* nibbles, uint8_t depth,
                                  uint8_t universe_bits)
{
    r64key_t key;
    uint8_t top_bits;
    uint8_t level;
    top_bits = (uint8_t)(universe_bits - (depth - 1) * 6);
    key = (r64key_t)(nibbles[0] & (((uint64_t)1 << top_bits) - 1));
    for (level = 1; level < depth; level++) {
        key = (key << 6) | nibbles[level];
    }
    return key;
}

/* ===================================== *
 *         F R E E   H E L P E R
 * ===================================== */

/* `children_levels` counts allocated-child levels beneath this node:
     depth == 1 → 0 (root is its own leaf, no children)
     depth >= 2 → depth - 2 at the root (leaf-parents are the last level) */
static void radix64_free_node(Radix64Node* node, uint8_t children_levels)
{
    uint64_t bm;
    uint8_t i;
    /* recursion anchor: NULL pointer (lazy subtree) */
    if (node == NULL) return;
    /* recursion case: internal node — descend into each allocated child.
       leaf-parents (children_levels == 0) have no node children to free;
       their u.leaves[] bitmaps go away with the node itself. */
    if (children_levels >= 1) {
        bm = node->bitmap;
        while (bm != 0) {
            i = r64_ctz(bm);
            radix64_free_node(node->u.children[i], (uint8_t)(children_levels - 1));
            bm &= bm - 1;
        }
    }
    free(node);
}

/* ===================================== *
 *          P U B L I C   A P I
 * ===================================== */

void radix64_init(Radix64** out, uint8_t universe_bits)
{
    Radix64* tree;
    assert((universe_bits > 0 && universe_bits <= 32)
        && "invalid amount of universe bits, needs to be within [1, 32].");
    tree = (Radix64*)malloc(sizeof(Radix64));
    tree->universe_bits = universe_bits;
    tree->depth = (uint8_t)((universe_bits + 5) / 6);
    tree->root = NULL;
    *out = tree;
}

void radix64_free(Radix64* tree)
{
    uint8_t children_levels;
    if (tree == NULL) return;
    children_levels = (tree->depth >= 2) ? (uint8_t)(tree->depth - 2) : (uint8_t)0;
    radix64_free_node(tree->root, children_levels);
    free(tree);
}

bool radix64_is_empty(Radix64* tree)
{
    return tree->root == NULL || tree->root->bitmap == 0;
}

bool radix64_contains_key(Radix64* tree, r64key_t key)
{
    Radix64Node* node;
    uint8_t level;
    uint8_t nibble;
    uint8_t depth;
    uint8_t leaf_parent_nibble;
    uint8_t leaf_nibble;
    assert(key != r64_null && "cannot query r64_null, invalid key!");
    if (tree->root == NULL) return false;
    depth = tree->depth;
    node = tree->root;
    if (depth == 1) {
        nibble = (uint8_t)(key & 0x3F);
        return (node->bitmap & ((uint64_t)1 << nibble)) != 0;
    }
    for (level = 0; level + 2 < depth; level++) {
        nibble = radix64_nibble(key, level, depth);
        if ((node->bitmap & ((uint64_t)1 << nibble)) == 0) return false;
        node = node->u.children[nibble];
    }
    leaf_parent_nibble = radix64_nibble(key, (uint8_t)(depth - 2), depth);
    if ((node->bitmap & ((uint64_t)1 << leaf_parent_nibble)) == 0) return false;
    leaf_nibble = radix64_nibble(key, (uint8_t)(depth - 1), depth);
    return (node->u.leaves[leaf_parent_nibble] & ((uint64_t)1 << leaf_nibble)) != 0;
}

void radix64_insert_key(Radix64* tree, r64key_t key)
{
    Radix64Node* node;
    uint8_t level;
    uint8_t nibble;
    uint8_t depth;
    uint8_t leaf_parent_nibble;
    uint8_t leaf_nibble;
    assert(key != r64_null && "cannot insert r64_null, invalid key!");
    if (tree->root == NULL) {
        tree->root = (Radix64Node*)calloc(1, sizeof(Radix64Node));
    }
    depth = tree->depth;
    node = tree->root;
    if (depth == 1) {
        nibble = (uint8_t)(key & 0x3F);
        node->bitmap |= (uint64_t)1 << nibble;
        return;
    }
    for (level = 0; level + 2 < depth; level++) {
        nibble = radix64_nibble(key, level, depth);
        node->bitmap |= (uint64_t)1 << nibble;
        if (node->u.children[nibble] == NULL) {
            node->u.children[nibble] = (Radix64Node*)calloc(1, sizeof(Radix64Node));
        }
        node = node->u.children[nibble];
    }
    leaf_parent_nibble = radix64_nibble(key, (uint8_t)(depth - 2), depth);
    leaf_nibble = radix64_nibble(key, (uint8_t)(depth - 1), depth);
    node->bitmap |= (uint64_t)1 << leaf_parent_nibble;
    node->u.leaves[leaf_parent_nibble] |= (uint64_t)1 << leaf_nibble;
}

void radix64_delete_key(Radix64* tree, r64key_t key)
{
    Radix64Node* path[R64_MAX_DEPTH];
    uint8_t nibbles[R64_MAX_DEPTH];
    Radix64Node* node;
    uint8_t level;
    uint8_t nibble;
    uint8_t depth;
    uint8_t leaf_parent_nibble;
    uint8_t leaf_nibble;
    assert(key != r64_null && "cannot delete r64_null, invalid key!");
    if (tree->root == NULL) return;
    depth = tree->depth;
    node = tree->root;
    if (depth == 1) {
        nibble = (uint8_t)(key & 0x3F);
        node->bitmap &= ~((uint64_t)1 << nibble);
    } else {
        /* descend to leaf-parent, recording path */
        for (level = 0; level + 2 < depth; level++) {
            nibble = radix64_nibble(key, level, depth);
            path[level] = node;
            nibbles[level] = nibble;
            if ((node->bitmap & ((uint64_t)1 << nibble)) == 0) return;
            node = node->u.children[nibble];
        }
        /* at leaf-parent (level depth - 2) */
        leaf_parent_nibble = radix64_nibble(key, (uint8_t)(depth - 2), depth);
        path[depth - 2] = node;
        nibbles[depth - 2] = leaf_parent_nibble;
        if ((node->bitmap & ((uint64_t)1 << leaf_parent_nibble)) == 0) return;
        leaf_nibble = radix64_nibble(key, (uint8_t)(depth - 1), depth);
        if ((node->u.leaves[leaf_parent_nibble] & ((uint64_t)1 << leaf_nibble)) == 0) return;
        /* clear the key bit; if the whole leaf cleared, clear the parent bit */
        node->u.leaves[leaf_parent_nibble] &= ~((uint64_t)1 << leaf_nibble);
        if (node->u.leaves[leaf_parent_nibble] == 0) {
            node->bitmap &= ~((uint64_t)1 << leaf_parent_nibble);
        }
        /* ascend: free empty internal nodes */
        for (level = (uint8_t)(depth - 2); level > 0; level--) {
            if (path[level]->bitmap != 0) break;
            free(path[level]);
            path[level - 1]->u.children[nibbles[level - 1]] = NULL;
            path[level - 1]->bitmap &= ~((uint64_t)1 << nibbles[level - 1]);
        }
    }
    if (tree->root != NULL && tree->root->bitmap == 0) {
        free(tree->root);
        tree->root = NULL;
    }
}

/* Descend leftmost via ctz to leaf-parent, then pick leftmost bit in its
   leaf bitmap. Handles depth == 1 (root is the leaf) inline. */
r64key_t radix64_get_min(Radix64* tree)
{
    Radix64Node* node;
    uint8_t nibbles[R64_MAX_DEPTH];
    uint8_t level;
    uint8_t nibble;
    uint8_t depth;
    uint64_t leaf_bm;
    if (tree->root == NULL || tree->root->bitmap == 0) return r64_null;
    depth = tree->depth;
    node = tree->root;
    if (depth == 1) {
        nibbles[0] = r64_ctz(node->bitmap);
        return radix64_build_key(nibbles, depth, tree->universe_bits);
    }
    for (level = 0; level + 2 < depth; level++) {
        nibble = r64_ctz(node->bitmap);
        nibbles[level] = nibble;
        node = node->u.children[nibble];
    }
    nibble = r64_ctz(node->bitmap);
    nibbles[depth - 2] = nibble;
    leaf_bm = node->u.leaves[nibble];
    nibbles[depth - 1] = r64_ctz(leaf_bm);
    return radix64_build_key(nibbles, depth, tree->universe_bits);
}

r64key_t radix64_get_max(Radix64* tree)
{
    Radix64Node* node;
    uint8_t nibbles[R64_MAX_DEPTH];
    uint8_t level;
    uint8_t nibble;
    uint8_t depth;
    uint64_t leaf_bm;
    if (tree->root == NULL || tree->root->bitmap == 0) return r64_null;
    depth = tree->depth;
    node = tree->root;
    if (depth == 1) {
        nibbles[0] = (uint8_t)(63u - r64_clz(node->bitmap));
        return radix64_build_key(nibbles, depth, tree->universe_bits);
    }
    for (level = 0; level + 2 < depth; level++) {
        nibble = (uint8_t)(63u - r64_clz(node->bitmap));
        nibbles[level] = nibble;
        node = node->u.children[nibble];
    }
    nibble = (uint8_t)(63u - r64_clz(node->bitmap));
    nibbles[depth - 2] = nibble;
    leaf_bm = node->u.leaves[nibble];
    nibbles[depth - 1] = (uint8_t)(63u - r64_clz(leaf_bm));
    return radix64_build_key(nibbles, depth, tree->universe_bits);
}

/* Descend leftmost (ctz) from `start_node` at level `start_level + 1` down
   to the leaf, filling nibbles[start_level + 1 .. depth - 1]. Caller has
   already filled nibbles[0 .. start_level]. Only called when start_node
   is at an internal level (start_level + 1 <= depth - 2); the leaf-parent
   case at start_level == depth - 2 is handled inline by the caller. */
static r64key_t radix64_descend_leftmost(Radix64Node* start_node,
                                         uint8_t start_level,
                                         uint8_t* nibbles, uint8_t depth,
                                         uint8_t universe_bits)
{
    Radix64Node* node;
    uint8_t level;
    uint8_t nibble;
    uint64_t leaf_bm;
    node = start_node;
    for (level = (uint8_t)(start_level + 1); level + 2 < depth; level++) {
        nibble = r64_ctz(node->bitmap);
        nibbles[level] = nibble;
        node = node->u.children[nibble];
    }
    /* node is at leaf-parent (level depth - 2) */
    nibble = r64_ctz(node->bitmap);
    nibbles[depth - 2] = nibble;
    leaf_bm = node->u.leaves[nibble];
    nibbles[depth - 1] = r64_ctz(leaf_bm);
    return radix64_build_key(nibbles, depth, universe_bits);
}

/* Symmetric variant of radix64_descend_leftmost for predecessor. */
static r64key_t radix64_descend_rightmost(Radix64Node* start_node,
                                          uint8_t start_level,
                                          uint8_t* nibbles, uint8_t depth,
                                          uint8_t universe_bits)
{
    Radix64Node* node;
    uint8_t level;
    uint8_t nibble;
    uint64_t leaf_bm;
    node = start_node;
    for (level = (uint8_t)(start_level + 1); level + 2 < depth; level++) {
        nibble = (uint8_t)(63u - r64_clz(node->bitmap));
        nibbles[level] = nibble;
        node = node->u.children[nibble];
    }
    nibble = (uint8_t)(63u - r64_clz(node->bitmap));
    nibbles[depth - 2] = nibble;
    leaf_bm = node->u.leaves[nibble];
    nibbles[depth - 1] = (uint8_t)(63u - r64_clz(leaf_bm));
    return radix64_build_key(nibbles, depth, universe_bits);
}

r64key_t radix64_successor(Radix64* tree, r64key_t key)
{
    Radix64Node* path[R64_MAX_DEPTH];
    uint8_t nibbles[R64_MAX_DEPTH];
    Radix64Node* node;
    uint8_t depth;
    uint8_t level;
    uint8_t descend_start;
    uint8_t nibble;
    uint8_t next_nibble;
    uint8_t leaf_parent_nibble;
    uint8_t leaf_nibble;
    uint64_t mask;
    uint64_t bm;
    uint64_t leaf_bm;
    if (tree->root == NULL || tree->root->bitmap == 0) return r64_null;
    depth = tree->depth;
    /* depth == 1: root bitmap IS the key set; a single masked ctz suffices. */
    if (depth == 1) {
        nibble = (uint8_t)(key & 0x3F);
        if (nibble < 63) {
            mask = r64_leading_mask((uint8_t)(nibble + 1));
            bm = tree->root->bitmap & mask;
            if (bm != 0) {
                nibbles[0] = r64_ctz(bm);
                return radix64_build_key(nibbles, depth, tree->universe_bits);
            }
        }
        return r64_null;
    }
    /* depth >= 2: descend internal nodes until leaf-parent or missing slot */
    node = tree->root;
    descend_start = (uint8_t)(depth - 1);  /* sentinel: reached leaf-parent */
    for (level = 0; level + 2 < depth; level++) {
        path[level] = node;
        nibble = radix64_nibble(key, level, depth);
        nibbles[level] = nibble;
        if ((node->bitmap & ((uint64_t)1 << nibble)) == 0) {
            descend_start = level;
            break;
        }
        node = node->u.children[nibble];
    }
    if (descend_start == (uint8_t)(depth - 1)) {
        /* at leaf-parent; try to stay within the same leaf first */
        path[depth - 2] = node;
        leaf_parent_nibble = radix64_nibble(key, (uint8_t)(depth - 2), depth);
        nibbles[depth - 2] = leaf_parent_nibble;
        if (node->bitmap & ((uint64_t)1 << leaf_parent_nibble)) {
            leaf_nibble = radix64_nibble(key, (uint8_t)(depth - 1), depth);
            leaf_bm = node->u.leaves[leaf_parent_nibble];
            if (leaf_nibble < 63) {
                mask = r64_leading_mask((uint8_t)(leaf_nibble + 1));
                bm = leaf_bm & mask;
                if (bm != 0) {
                    nibbles[depth - 1] = r64_ctz(bm);
                    return radix64_build_key(nibbles, depth, tree->universe_bits);
                }
            }
        }
        descend_start = (uint8_t)(depth - 2);
    }
    /* ascend: at each recorded level, find a nibble > the one we took */
    for (level = descend_start; ; level--) {
        nibble = nibbles[level];
        if (nibble < 63) {
            mask = r64_leading_mask((uint8_t)(nibble + 1));
            bm = path[level]->bitmap & mask;
            if (bm != 0) {
                next_nibble = r64_ctz(bm);
                nibbles[level] = next_nibble;
                if (level == (uint8_t)(depth - 2)) {
                    /* sibling leaf at leaf-parent level; take leftmost bit */
                    leaf_bm = path[level]->u.leaves[next_nibble];
                    nibbles[depth - 1] = r64_ctz(leaf_bm);
                    return radix64_build_key(nibbles, depth, tree->universe_bits);
                }
                return radix64_descend_leftmost(path[level]->u.children[next_nibble],
                    level, nibbles, depth, tree->universe_bits);
            }
        }
        if (level == 0) return r64_null;
    }
}

r64key_t radix64_predecessor(Radix64* tree, r64key_t key)
{
    Radix64Node* path[R64_MAX_DEPTH];
    uint8_t nibbles[R64_MAX_DEPTH];
    Radix64Node* node;
    uint8_t depth;
    uint8_t level;
    uint8_t descend_start;
    uint8_t nibble;
    uint8_t prev_nibble;
    uint8_t leaf_parent_nibble;
    uint8_t leaf_nibble;
    uint64_t mask;
    uint64_t bm;
    uint64_t leaf_bm;
    if (tree->root == NULL || tree->root->bitmap == 0) return r64_null;
    depth = tree->depth;
    if (depth == 1) {
        nibble = (uint8_t)(key & 0x3F);
        if (nibble > 0) {
            mask = r64_trailing_mask(nibble);
            bm = tree->root->bitmap & mask;
            if (bm != 0) {
                nibbles[0] = (uint8_t)(63u - r64_clz(bm));
                return radix64_build_key(nibbles, depth, tree->universe_bits);
            }
        }
        return r64_null;
    }
    node = tree->root;
    descend_start = (uint8_t)(depth - 1);
    for (level = 0; level + 2 < depth; level++) {
        path[level] = node;
        nibble = radix64_nibble(key, level, depth);
        nibbles[level] = nibble;
        if ((node->bitmap & ((uint64_t)1 << nibble)) == 0) {
            descend_start = level;
            break;
        }
        node = node->u.children[nibble];
    }
    if (descend_start == (uint8_t)(depth - 1)) {
        path[depth - 2] = node;
        leaf_parent_nibble = radix64_nibble(key, (uint8_t)(depth - 2), depth);
        nibbles[depth - 2] = leaf_parent_nibble;
        if (node->bitmap & ((uint64_t)1 << leaf_parent_nibble)) {
            leaf_nibble = radix64_nibble(key, (uint8_t)(depth - 1), depth);
            leaf_bm = node->u.leaves[leaf_parent_nibble];
            if (leaf_nibble > 0) {
                mask = r64_trailing_mask(leaf_nibble);
                bm = leaf_bm & mask;
                if (bm != 0) {
                    nibbles[depth - 1] = (uint8_t)(63u - r64_clz(bm));
                    return radix64_build_key(nibbles, depth, tree->universe_bits);
                }
            }
        }
        descend_start = (uint8_t)(depth - 2);
    }
    for (level = descend_start; ; level--) {
        nibble = nibbles[level];
        if (nibble > 0) {
            mask = r64_trailing_mask(nibble);
            bm = path[level]->bitmap & mask;
            if (bm != 0) {
                prev_nibble = (uint8_t)(63u - r64_clz(bm));
                nibbles[level] = prev_nibble;
                if (level == (uint8_t)(depth - 2)) {
                    leaf_bm = path[level]->u.leaves[prev_nibble];
                    nibbles[depth - 1] = (uint8_t)(63u - r64_clz(leaf_bm));
                    return radix64_build_key(nibbles, depth, tree->universe_bits);
                }
                return radix64_descend_rightmost(path[level]->u.children[prev_nibble],
                    level, nibbles, depth, tree->universe_bits);
            }
        }
        if (level == 0) return r64_null;
    }
}

uint8_t radix64_required_universe_bits(r64key_t max_key)
{
    assert(max_key != 0 && "universe has to consist of at least 2 keys");
    return (uint8_t)(64u - r64_clz((uint64_t)max_key));
}

#endif /* DOXYGEN_SKIP */
#endif /* RADIX64_H */
