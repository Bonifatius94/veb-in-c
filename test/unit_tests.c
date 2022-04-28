#include <stdbool.h>
#include <assert.h>
#include "vebtrees.h"

void assert_empty_bitwise_leaf(VebTree* tree)
{
    assert(vebtree_is_leaf(tree));
    assert(tree->low == 0);
    assert(tree->global == NULL);
    assert(tree->locals == NULL);
    assert(tree->universe_bits <= 6);
    assert(tree->high == vebtree_null);
}

void should_create_fully_alloc_tree_u4096()
{
    VebTree* tree; size_t i;
    vebtree_init(&tree, 12, 0);

    assert(vebtree_is_empty(tree));
    assert_empty_bitwise_leaf(tree->global);

    for (i = 0; i < 64; i++)
        assert_empty_bitwise_leaf(&(tree->locals[i]));

    vebtree_free(tree);
}

/* TODO: add test case for trees managing odd universe bits */

void should_insert_into_fully_alloc_tree_u4096()
{
    size_t i; VebTree* tree;
    vebtree_init(&tree, 12, 0);
    assert(vebtree_is_empty(tree));

    for (i = 0; i < 4096; i++) {
        assert(!vebtree_contains_key(tree, i));
        vebtree_insert_key(tree, i);
        assert(vebtree_contains_key(tree, i));
        assert(!vebtree_is_empty(tree));
    }

    assert(!vebtree_is_empty(tree));
    for (i = 0; i < 4096; i++)
        assert(vebtree_contains_key(tree, i));

    vebtree_free(tree);
}

void should_delete_from_fully_alloc_tree_u4096()
{
    size_t i; VebTree* tree;
    vebtree_init(&tree, 12, 0);
    assert(vebtree_is_empty(tree));

    for (i = 0; i < 4096; i++) {
        assert(!vebtree_contains_key(tree, i));
        vebtree_insert_key(tree, i);
        assert(vebtree_contains_key(tree, i));
        assert(!vebtree_is_empty(tree));
    }

    assert(!vebtree_is_empty(tree));
    for (i = 0; i < 4096; i++)
        assert(vebtree_contains_key(tree, i));

    for (i = 0; i < 4096; i++) {
        assert(vebtree_contains_key(tree, i));
        assert(!vebtree_is_empty(tree));
        vebtree_delete_key(tree, i);
        assert(!vebtree_contains_key(tree, i));
    }

    assert(vebtree_is_empty(tree));
    for (i = 0; i < 4096; i++)
        assert(!vebtree_contains_key(tree, i));

    vebtree_free(tree);
}

int main(int argc, char** argv)
{
    should_create_fully_alloc_tree_u4096();
    should_insert_into_fully_alloc_tree_u4096();
    // should_delete_from_fully_alloc_tree_u4096();
    return 0;
}
