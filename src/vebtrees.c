#include "vebtrees.h"

#define math_max(v1, v2) (((v1) >= (v2)) ? (v1) : (v2))

#define vebtree_lower_bits(uni_bits) ((uni_bits) >> 1) /* div by 2 */
#define vebtree_upper_bits(uni_bits) ((uni_bits) - vebtree_lower_bits(uni_bits))
#define vebtree_local_max(uni_bits) ((vebkey_t)1 << vebtree_lower_bits(uni_bits))
#define vebtree_global_max(uni_bits) ((vebkey_t)1 << vebtree_upper_bits(uni_bits))
#define vebtree_local_address(key, local_bits) ((((vebkey_t)1 << (local_bits)) - 1) & (key))
#define vebtree_global_address(key, local_bits) ((key) >> (local_bits))
#define vebtree_new_empty_node(uni_bits) (VebTree){(uni_bits), vebtree_null, vebtree_null, NULL, NULL}

void _init_subtrees(VebTree* tree, bool is_lazy);
void _vebtree_init(VebTree* tree, uint8_t universe_bits, bool is_lazy);

/***********************************************
 *    C O R E   I M P L E M E N T A T I O N    *
 ***********************************************/

void vebtree_init(VebTree** new_tree, uint8_t universe_bits, bool is_lazy)
{
    assert((universe_bits > 0 && universe_bits <= 64) 
        && "invalid amount of universe bits, needs to be within [1, 64].");

    /* allocate memory for the first tree */
    *new_tree = (VebTree*)malloc(sizeof(VebTree));
    _vebtree_init(*new_tree, universe_bits, is_lazy);
}

void _vebtree_init(VebTree* tree, uint8_t universe_bits, bool is_lazy)
{
    assert((universe_bits > 0 && universe_bits <= 64) 
        && "invalid amount of universe bits");

    /* init the tree with an empty leaf */
    *tree = vebtree_new_empty_node(universe_bits);

    /* recursion anchor for tree leafs */
    if (universe_bits == 1 || is_lazy)
        return;

    /* fully allocate the tree recursively */
    _init_subtrees(tree, is_lazy);

    assert(tree->global != NULL && "global tree init failed!");
    assert(tree->locals != NULL && "locals tree init failed!");
}

void vebtree_free(VebTree* tree)
{
    size_t i; vebkey_t num_locals;

    /* recursion anchor for tree leafs */
    if (tree->global == NULL)
        return;

    /* recursion case for child trees */
    vebtree_free(tree->global);
    num_locals = vebtree_global_max(tree->universe_bits);
    for (i = 0; i < num_locals; i++)
        vebtree_free(&(tree->locals[i]));

    /* local memory deallocation */
    free(tree->global);
    free(tree->locals);
    tree->global = NULL;
    tree->locals = NULL;
}

void _init_subtrees(VebTree* tree, bool is_lazy)
{
    size_t i, num_locals; uint8_t upper_bits, lower_bits; VebTree* temp;

    /* determine the sizes of global / locals */
    upper_bits = vebtree_upper_bits(tree->universe_bits);
    lower_bits = vebtree_lower_bits(tree->universe_bits);
    num_locals = vebtree_global_max(tree->universe_bits);

    /* init global recursively */
    tree->global = (VebTree*)malloc(sizeof(VebTree));
    _vebtree_init(tree->global, upper_bits, is_lazy);

    /* init locals recursively */
    tree->locals = (VebTree*)malloc(num_locals * sizeof(VebTree));
    for (i = 0; i < num_locals; i++)
        _vebtree_init(tree->locals + i, lower_bits, is_lazy);
}

bool vebtree_contains_key(VebTree* tree, vebkey_t key)
{
    vebkey_t local_key, global_key; uint8_t lower_bits;
    assert(key != vebtree_null && "cannot check for vebtree_null, invalid key!");

    /* key is the low or high element (base case) */
    if (tree->low == key || tree->high == key)
        return true;

    /* tree is empty, cannot contain the key */
    if (tree->universe_bits == 1 || vebtree_is_empty(tree))
        return false;

    lower_bits = vebtree_lower_bits(tree->universe_bits);
    local_key = vebtree_local_address(key, lower_bits);
    global_key = vebtree_global_address(key, lower_bits);

    /* key is part of a local subtree (recursion case) */
    return vebtree_contains_key(tree->global, global_key) &&
           vebtree_contains_key(&(tree->locals[global_key]), local_key);
}

vebkey_t vebtree_successor(VebTree* tree, vebkey_t key)
{
    vebkey_t global_key, local_key, global_succ; uint8_t lower_bits;

    /* base case for universe size 2 */
    if (tree->universe_bits == 1)
        return (key == 0 && tree->high == 1) ? (vebkey_t)1 : vebtree_null;

    /* base case for predecessor in neighbour local -> low is the successor */
    if (tree->low != vebtree_null && key < tree->low)
        return tree->low;

    lower_bits = vebtree_lower_bits(tree->universe_bits);
    local_key = vebtree_local_address(key, lower_bits);
    global_key = vebtree_global_address(key, lower_bits);

    /* case where a local contains the successor */
    if (tree->locals[global_key].high != vebtree_null &&
            local_key < tree->locals[global_key].high)
        return (global_key << lower_bits) | 
            (vebtree_successor(&(tree->locals[global_key]), local_key));

    /* case where a neighbour contains the successor */
    global_succ = vebtree_successor(tree->global, global_key);
    return global_succ == vebtree_null ? vebtree_null
        : (global_succ << lower_bits) | tree->locals[global_succ].low;
}

vebkey_t vebtree_predecessor(VebTree* tree, vebkey_t key)
{
    /* TODO: implement this analog to the successor function */
    assert(false && "this operation is currently not supported");
}

void vebtree_insert_key(VebTree* tree, vebkey_t key)
{
    vebkey_t global_key, local_key, temp; uint8_t lower_bits;
    assert(key != vebtree_null && "cannot insert vebtree_null, invalid key!");

    /* base case when tree is empty */
    if (vebtree_is_empty(tree)) { tree->low = tree->high = key; return; }

    /* case when the key becomes the new low -> insert old low instead */
    if (key < tree->low) { temp = tree->low; tree->low = key; key = temp; }

    /* not the base case for universe size 2 */
    if (tree->universe_bits > 1)
    {
        lower_bits = vebtree_lower_bits(tree->universe_bits);
        local_key = vebtree_local_address(key, lower_bits);
        global_key = vebtree_global_address(key, lower_bits);

        /* insert the global key if the corresponding local is empty */
        if (vebtree_is_empty(&(tree->locals[global_key])))
            vebtree_insert_key(tree->global, global_key);

        /* insert the local key into local scope */
        vebtree_insert_key(&(tree->locals[global_key]), local_key);
    }

    /* update the tree's high */
    tree->high = math_max(tree->high, key);
}

void vebtree_delete_key(VebTree* tree, vebkey_t key)
{
    uint8_t lower_bits; vebkey_t global_key, local_key, global_high;
    assert(key != vebtree_null && "cannot delete vebtree_null, invalid key!");

    /* base case with only one element -> set low and high to null */
    if (tree->low == tree->high) { tree->low = tree->high = vebtree_null; return; }

    /* base case with universe size 2 -> flip the bit */
    if (tree->universe_bits == 1) { tree->low = tree->high = 1 - key; return; }

    lower_bits = vebtree_lower_bits(tree->universe_bits);

    /* case when deleting the low element -> new low needs to be pulled out */
    if (key == tree->low)
        tree->low = key = (tree->global->low << lower_bits)
            | tree->locals[tree->global->low].low;

    global_key = vebtree_global_address(key, lower_bits);
    local_key = vebtree_local_address(key, lower_bits);

    /* delete the local key recursively */
    vebtree_delete_key(&(tree->locals[global_key]), local_key);

    if (vebtree_is_empty(&(tree->locals[global_key])))
    {
        vebtree_delete_key(tree->global, global_key);

        /* in case the maximum was deleted */
        if (key == tree->high) {
            global_high = tree->global->high;
            tree->high = global_high == vebtree_null ? tree->low
                : ((vebkey_t)global_high << lower_bits) | tree->locals[global_high].high;
        }

    /* if the maximum was deleted, but the tree was not empty */
    } else if (key == tree->high)
        tree->high = (global_key << lower_bits) | tree->locals[global_key].high;
}

/***********************************************
 *      B I T W I S E   T R E E   L E A F      *
 ***********************************************/

/* info: use the leaf's "low" attribute as bitboard */

#define trailing_bits_mask(num_bits) (((bitboard_t)1 << (num_bits)) - 1)
#define leading_bits_mask(num_bits) (((bitboard_t)0xFFFFFFFFFFFFFFFF << (num_bits)))
#define u64_log2(x) (63 - leading_zeros((x)))

bool vebtree_bitwise_leaf_contains_key(VebTree* tree, vebkey_t key)
{
    assert(key < 64);
    return (((bitboard_t)1 << key) & tree->low) > 0;
}

bool vebtree_bitwise_leaf_is_empty(VebTree* tree)
{
    return tree->low == 0;
}

vebkey_t vebtree_bitwise_leaf_get_min(VebTree* tree)
{
    return trailing_zeros(tree->low);
}

vebkey_t vebtree_bitwise_leaf_get_max(VebTree* tree)
{
    return u64_log2(tree->low);
}

vebkey_t vebtree_bitwise_leaf_successor(VebTree* tree, vebkey_t key)
{
    uint64_t succ_bits, min_succ;
    assert(key < 64);
    succ_bits = tree->low & leading_bits_mask((uint8_t)key + 1);
    min_succ = trailing_zeros(succ_bits);
    return (min_succ == 0 || succ_bits == 0) ? vebtree_null : min_succ;
}

vebkey_t vebtree_bitwise_leaf_predecessor(VebTree* tree, vebkey_t key)
{
    uint64_t pred_bits, max_pred;
    assert(key < 64);
    pred_bits = tree->low & trailing_bits_mask((uint8_t)key);
    max_pred = u64_log2(pred_bits);
    return (max_pred == 0 && (tree->low & 1) == 0) ? vebtree_null : max_pred;
}

void vebtree_bitwise_leaf_insert_key(VebTree* tree, vebkey_t key)
{
    assert(key < 64);
    tree->low |= (bitboard_t)1 << key;
}

void vebtree_bitwise_leaf_delete_key(VebTree* tree, vebkey_t key)
{
    assert(key < 64);
    tree->low &= ~((bitboard_t)1 << key);
}
