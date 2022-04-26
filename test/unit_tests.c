#include <stdbool.h>
#include <assert.h>
#include "vebtrees.h"

void assert_empty_fully_alloc_u2(VebTree* tree)
{
    assert(vebtree_is_empty(tree));
    assert(tree->universe_bits == 1 && tree->global == NULL && tree->locals == NULL);
}

void assert_empty_fully_alloc_u4(VebTree* tree)
{
    assert(vebtree_is_empty(tree));
    assert(tree->universe_bits == 2 && tree->global != NULL && tree->locals != NULL);
    assert_empty_fully_alloc_u2(tree->global);
    assert_empty_fully_alloc_u2(&(tree->locals[0]));
    assert_empty_fully_alloc_u2(&(tree->locals[1]));
}

void should_create_fully_alloc_tree_u16()
{
    VebTree* tree; size_t i;
    vebtree_init(&tree, 4, 0);

    assert(vebtree_is_empty(tree));
    assert(tree->universe_bits == 4 && tree->global != NULL && tree->locals != NULL);
    assert_empty_fully_alloc_u4(tree->global);
    assert_empty_fully_alloc_u4(&(tree->locals[0]));
    assert_empty_fully_alloc_u4(&(tree->locals[1]));
    assert_empty_fully_alloc_u4(&(tree->locals[2]));
    assert_empty_fully_alloc_u4(&(tree->locals[3]));

    vebtree_free(tree);
}

/* TODO: add test case for trees managing odd universe bits */

void should_insert_into_fully_alloc_tree_u256()
{
    size_t i; VebTree* tree;
    vebtree_init(&tree, 8, 0);
    assert(vebtree_is_empty(tree));

    for (i = 0; i < 256; i++) {
        assert(!vebtree_contains_key(tree, i));
        vebtree_insert_key(tree, i);
        assert(vebtree_contains_key(tree, i));
        assert(!vebtree_is_empty(tree));
    }

    assert(!vebtree_is_empty(tree));
    for (i = 0; i < 256; i++)
        assert(vebtree_contains_key(tree, i));

    vebtree_free(tree);
}

void should_delete_from_fully_alloc_tree_u256()
{
    size_t i; VebTree* tree;
    vebtree_init(&tree, 8, 0);
    assert(vebtree_is_empty(tree));

    for (i = 0; i < 256; i++) {
        assert(!vebtree_contains_key(tree, i));
        vebtree_insert_key(tree, i);
        assert(vebtree_contains_key(tree, i));
        assert(!vebtree_is_empty(tree));
    }

    assert(!vebtree_is_empty(tree));
    for (i = 0; i < 256; i++)
        assert(vebtree_contains_key(tree, i));

    for (i = 0; i < 256; i++) {
        assert(vebtree_contains_key(tree, i));
        assert(!vebtree_is_empty(tree));
        vebtree_delete_key(tree, i);
        assert(!vebtree_contains_key(tree, i));
    }

    assert(vebtree_is_empty(tree));
    for (i = 0; i < 256; i++)
        assert(!vebtree_contains_key(tree, i));

    vebtree_free(tree);
}

int main(int argc, char** argv)
{
    should_create_fully_alloc_tree_u16();
    should_insert_into_fully_alloc_tree_u256();
    should_delete_from_fully_alloc_tree_u256();
    return 0;
}
