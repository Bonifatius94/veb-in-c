#include "vebtrees.h"

#define LOWER_BITS(uni_bits) ((uni_bits) >> 1) /* div by 2 */
#define UPPER_BITS(uni_bits) ((uni_bits) - LOWER_BITS(uni_bits))
#define LOCAL_MAX_VALUE(uni_bits) ((VebKey)1 << LOWER_BITS(uni_bits))
#define GLOBAL_MAX_VALUE(uni_bits) ((VebKey)1 << UPPER_BITS(uni_bits))
#define LOCAL_ADRESS(key, local_bits) ((((VebKey)1 << (local_bits)) - 1) & (key))
#define GLOBAL_ADRESS(key, local_bits) ((key) >> (local_bits))
#define MATH_MAX(v1, v2) (((v1) >= (v2)) ? (v1) : (v2))
#define EMPTY_VEB_TREE_LEAF(uni_bits) (VebTree){(uni_bits), VEB_NULL, VEB_NULL, NULL, NULL}

void _init_subtrees(VebTree* tree, bool is_lazy);
void _vebtree_init(VebTree* tree, uint8_t universe_bits, bool is_lazy);

void vebtree_init(VebTree** new_tree, uint8_t universe_bits, bool is_lazy)
{
    assert((universe_bits > 0 && universe_bits <= 64) 
        && "invalid amount of universe bits");

    /* allocate memory for the first tree */
    *new_tree = (VebTree*)malloc(sizeof(VebTree));
    _vebtree_init(*new_tree, universe_bits, is_lazy);
}

void _vebtree_init(VebTree* tree, uint8_t universe_bits, bool is_lazy)
{
    assert((universe_bits > 0 && universe_bits <= 64) 
        && "invalid amount of universe bits");

    /* init the tree with an empty leaf */
    *tree = EMPTY_VEB_TREE_LEAF(universe_bits);

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
    size_t i; VebKey num_locals;

    /* recursion anchor for tree leafs */
    if (tree->global == NULL)
        return;

    /* recursion case for child trees */
    vebtree_free(tree->global);
    num_locals = GLOBAL_MAX_VALUE(tree->universe_bits);
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
    upper_bits = UPPER_BITS(tree->universe_bits);
    lower_bits = LOWER_BITS(tree->universe_bits);
    num_locals = GLOBAL_MAX_VALUE(tree->universe_bits);

    /* init global recursively */
    tree->global = (VebTree*)malloc(sizeof(VebTree));
    _vebtree_init(tree->global, upper_bits, is_lazy);

    /* init locals recursively */
    tree->locals = (VebTree*)malloc(num_locals * sizeof(VebTree));
    for (i = 0; i < num_locals; i++)
        _vebtree_init(tree->locals + i, lower_bits, is_lazy);
}

bool vebtree_is_empty(VebTree* tree)
{
    /* TODO: think of converting this into a makro */
    return tree->low == VEB_NULL;
}

VebKey vebtree_get_min(VebTree* tree)
{
    /* TODO: think of converting this into a makro */
    return tree->low;
}

VebKey vebtree_get_max(VebTree* tree)
{
    /* TODO: think of converting this into a makro */
    return tree->high;
}

bool vebtree_contains_key(VebTree* tree, VebKey key)
{
    VebKey local_key, global_key; uint8_t lower_bits;
    assert(key != VEB_NULL && "cannot check for VEB_NULL, invalid key!");

    /* key is the low or high element (base case) */
    if (tree->low == key || tree->high == key)
        return true;

    /* tree is empty, cannot contain the key */
    if (tree->universe_bits == 1 || vebtree_is_empty(tree))
        return false;

    lower_bits = LOWER_BITS(tree->universe_bits);
    local_key = LOCAL_ADRESS(key, lower_bits);
    global_key = GLOBAL_ADRESS(key, lower_bits);

    /* key is part of a local subtree (recursion case) */
    return vebtree_contains_key(tree->global, global_key) &&
           vebtree_contains_key(&(tree->locals[global_key]), local_key);
}

VebKey vebtree_successor(VebTree* tree, VebKey key)
{
    VebKey global_key, local_key, global_succ; uint8_t lower_bits;

    /* base case for universe size 2 */
    if (tree->universe_bits == 1)
        return (key == 0 && tree->high == 1) ? (VebKey)1 : VEB_NULL;

    /* base case for predecessor in neighbour local -> low is the successor */
    if (tree->low != VEB_NULL && key < tree->low)
        return tree->low;

    lower_bits = LOWER_BITS(tree->universe_bits);
    local_key = LOCAL_ADRESS(key, lower_bits);
    global_key = GLOBAL_ADRESS(key, lower_bits);

    /* case where a local contains the successor */
    if (tree->locals[global_key].high != VEB_NULL &&
            local_key < tree->locals[global_key].high)
        return (global_key << lower_bits) | 
            (vebtree_successor(&(tree->locals[global_key]), local_key));

    /* case where a neighbour contains the successor */
    global_succ = vebtree_successor(tree->global, global_key);
    return global_succ == VEB_NULL ? VEB_NULL
        : (global_succ << lower_bits) | tree->locals[global_succ].low;
}

VebKey vebtree_predecessor(VebTree* tree, VebKey key)
{
    /* TODO: implement this analog to the successor function */
    assert(false && "this operation is currently not supported");
}

void vebtree_insert_key(VebTree* tree, VebKey key)
{
    VebKey global_key, local_key, temp; uint8_t lower_bits;
    assert(key != VEB_NULL && "cannot insert VEB_NULL, invalid key!");

    /* base case when tree is empty */
    if (vebtree_is_empty(tree)) { tree->low = tree->high = key; return; }

    /* case when the key becomes the new low -> insert old low instead */
    if (key < tree->low) { temp = tree->low; tree->low = key; key = temp; }

    /* not the base case for universe size 2 */
    if (tree->universe_bits > 1)
    {
        lower_bits = LOWER_BITS(tree->universe_bits);
        local_key = LOCAL_ADRESS(key, lower_bits);
        global_key = GLOBAL_ADRESS(key, lower_bits);

        /* insert the global key if the corresponding local is empty */
        if (vebtree_is_empty(&(tree->locals[global_key])))
            vebtree_insert_key(tree->global, global_key);

        /* insert the local key into local scope */
        vebtree_insert_key(&(tree->locals[global_key]), local_key);
    }

    /* update the tree's high */
    tree->high = MATH_MAX(tree->high, key);
}

void vebtree_delete_key(VebTree* tree, VebKey key)
{
    uint8_t lower_bits; VebKey global_key, local_key, global_high;
    assert(key != VEB_NULL && "cannot delete VEB_NULL, invalid key!");

    /* base case with only one element -> set low and high to null */
    if (tree->low == tree->high) { tree->low = tree->high = VEB_NULL; return; }

    /* base case with universe size 2 -> flip the bit */
    if (tree->universe_bits == 1) { tree->low = tree->high = 1 - key; return; }

    /* case when deleting the low element -> new low needs to be pulled out */
    if (key == tree->low)
        tree->low = key = (tree->global->low << lower_bits)
            | tree->locals[tree->global->low].low;

    lower_bits = LOWER_BITS(tree->universe_bits);
    global_key = GLOBAL_ADRESS(key, lower_bits);
    local_key = LOCAL_ADRESS(key, lower_bits);

    /* delete the local key recursively */
    vebtree_delete_key(&(tree->locals[global_key]), local_key);

    if (vebtree_is_empty(&(tree->locals[global_key])))
    {
        vebtree_delete_key(tree->global, global_key);

        /* in case the maximum was deleted */
        if (key == tree->high) {
            global_high = tree->global->high;
            tree->high = global_high == VEB_NULL ? tree->low
                : ((VebKey)global_high << lower_bits) | tree->locals[global_high].high;
        }

    /* if the maximum was deleted, but the tree was not empty */
    } else if (key == tree->high)
        tree->high = (global_key << lower_bits) | tree->locals[global_key].high;
}
