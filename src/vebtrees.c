#include "vebtrees.h"

/* TODO: remove those makros, copy the code to the location of usage */
#define vebtree_lower_bits(uni_bits) ((uni_bits) >> 1) /* div by 2 */
#define vebtree_upper_bits(uni_bits) ((uni_bits) - vebtree_lower_bits(uni_bits))

#define vebtree_universe_maxvalue(uni_bits) ((vebkey_t)1 << vebtree_upper_bits(uni_bits))
#define vebtree_local_address(key, local_bits) ((((vebkey_t)1 << (local_bits)) - 1) & (key))
#define vebtree_global_address(key, local_bits) ((key) >> (local_bits))

#define vebtree_new_empty_node(uni_bits, lower_bits, flags) (VebTree){\
    (uni_bits), (lower_bits), (uni_bits) - (lower_bits), (flags),\
    vebtree_null, vebtree_null, NULL, NULL}

#define vebtree_new_empty_bitwise_leaf(uni_bits) (VebTree){\
    (uni_bits), 0, 0, 0, 0, vebtree_null, NULL, NULL}

void _init_subtrees(VebTree* tree, uint8_t flags);
void _vebtree_init(VebTree* tree, uint8_t universe_bits, uint8_t flags);

/***********************************************
 *      B I T W I S E   T R E E   L E A F      *
 ***********************************************/

/* info: use the leaf's "low" attribute as bitboard */

#define trailing_bits_mask(num_bits) (((bitboard_t)1 << (num_bits)) - 1)
#define leading_bits_mask(num_bits) (((bitboard_t)0xFFFFFFFFFFFFFFFF << (num_bits)))

vebkey_t vebtree_bitwise_leaf_successor(VebTree* tree, vebkey_t key)
{
    uint64_t succ_bits, min_succ;
    succ_bits = tree->low & leading_bits_mask((uint8_t)key + 1);
    min_succ = trailing_zeros(succ_bits);
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

/***********************************************
 *    C O R E   I M P L E M E N T A T I O N    *
 ***********************************************/

void vebtree_init(VebTree** new_tree, uint8_t universe_bits, uint8_t flags)
{
    assert((universe_bits > 0 && universe_bits <= 64) 
        && "invalid amount of universe bits, needs to be within [1, 64].");

    /* allocate memory for the first tree */
    *new_tree = (VebTree*)malloc(sizeof(VebTree));
    _vebtree_init(*new_tree, universe_bits, flags);
}

void _vebtree_init(VebTree* tree, uint8_t universe_bits, uint8_t flags)
{
    assert((universe_bits > 0 && universe_bits <= 64) 
        && "invalid amount of universe bits");

    /* init the tree with an empty node */
    if (universe_bits > 6)
        *tree = vebtree_new_empty_node(universe_bits, vebtree_lower_bits(universe_bits), flags);
    else
        *tree = vebtree_new_empty_bitwise_leaf(universe_bits);

    /* recursion anchor for tree leafs */
    if (vebtree_is_leaf(tree) || vebtree_is_lazy(tree)) return;

    /* fully allocate the tree recursively */
    _init_subtrees(tree, flags);

    assert(tree->global != NULL && "global tree init failed!");
    assert(tree->locals != NULL && "locals tree init failed!");
}

void vebtree_free(VebTree* tree)
{
    size_t i; vebkey_t num_locals;

    /* recursion anchor for tree leafs */
    if (vebtree_is_leaf(tree))
        return;

    /* recursion case for child trees */
    vebtree_free(tree->global);
    num_locals = vebtree_universe_maxvalue(tree->universe_bits);
    for (i = 0; i < num_locals; i++)
        vebtree_free(&(tree->locals[i]));

    /* local memory deallocation */
    free(tree->global);
    free(tree->locals);
    tree->global = NULL;
    tree->locals = NULL;
}

void _init_subtrees(VebTree* tree, uint8_t flags)
{
    size_t i, num_locals; VebTree* temp;

    /* determine the sizes of global / locals */
    num_locals = vebtree_universe_maxvalue(tree->universe_bits);

    /* init global recursively */
    tree->global = (VebTree*)malloc(sizeof(VebTree));
    _vebtree_init(tree->global, tree->upper_bits, flags);

    /* init locals recursively */
    tree->locals = (VebTree*)malloc(num_locals * sizeof(VebTree));
    for (i = 0; i < num_locals; i++)
        _vebtree_init(tree->locals + i, tree->lower_bits, flags);
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
    if (tree->locals[global_key].high != vebtree_null &&
            local_key < tree->locals[global_key].high)
        return (global_key << tree->lower_bits) | 
            (vebtree_successor(&(tree->locals[global_key]), local_key));

    /* case where a neighbour contains the successor */
    global_succ = vebtree_successor(tree->global, global_key);
    return global_succ == vebtree_null ? vebtree_null
        : (global_succ << tree->lower_bits) | tree->locals[global_succ].low;
}

vebkey_t vebtree_predecessor(VebTree* tree, vebkey_t key)
{
    /* TODO: implement this analog to the successor function */
    assert(false && "this operation is currently not supported");
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
