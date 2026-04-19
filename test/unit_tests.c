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

void should_handle_bit_zero_in_bitwise_leaf_predecessor()
{
    VebTree leaf;

    /* empty leaf: predecessor of any key must be vebtree_null */
    leaf = vebtree_new_empty_bitwise_leaf(6);
    assert(vebtree_bitwise_leaf_predecessor(&leaf, 1) == vebtree_null);
    assert(vebtree_bitwise_leaf_predecessor(&leaf, 5) == vebtree_null);

    /* only bit 5 set: predecessor of 1 has no answer, must be null */
    vebtree_bitwise_leaf_insert_key(&leaf, 5);
    assert(vebtree_bitwise_leaf_predecessor(&leaf, 1) == vebtree_null);
    assert(vebtree_bitwise_leaf_predecessor(&leaf, 5) == vebtree_null);
    assert(vebtree_bitwise_leaf_predecessor(&leaf, 6) == 5);

    /* only bit 0 set: predecessor of 1 is 0 (bit 0 is a valid answer) */
    leaf = vebtree_new_empty_bitwise_leaf(6);
    vebtree_bitwise_leaf_insert_key(&leaf, 0);
    assert(vebtree_bitwise_leaf_predecessor(&leaf, 1) == 0);
    assert(vebtree_bitwise_leaf_predecessor(&leaf, 0) == vebtree_null);
}

void should_return_null_on_empty_tree_u4096()
{
    VebTree* tree;
    vebtree_init(&tree, 12, 0);
    assert(vebtree_is_empty(tree));
    assert(vebtree_predecessor(tree, 100) == vebtree_null);
    assert(vebtree_predecessor(tree, 1) == vebtree_null);
    assert(vebtree_predecessor(tree, 4095) == vebtree_null);
    vebtree_free(tree);
}

void should_delegate_to_leaf_in_small_tree_u64()
{
    VebTree* tree;
    vebtree_init(&tree, 6, 0);
    assert(vebtree_is_leaf(tree));

    vebtree_insert_key(tree, 3);
    vebtree_insert_key(tree, 17);
    vebtree_insert_key(tree, 42);

    assert(vebtree_predecessor(tree, 42) == 17);
    assert(vebtree_predecessor(tree, 18) == 17);
    assert(vebtree_predecessor(tree, 17) == 3);
    assert(vebtree_predecessor(tree, 4) == 3);
    assert(vebtree_predecessor(tree, 3) == vebtree_null);
    assert(vebtree_predecessor(tree, 63) == 42);

    vebtree_free(tree);
}

void should_find_predecessor_in_fully_alloc_tree_u4096()
{
    size_t i; VebTree* tree;
    vebtree_init(&tree, 12, 0);

    for (i = 0; i < 4096; i++)
        vebtree_insert_key(tree, i);

    assert(vebtree_predecessor(tree, 0) == vebtree_null);
    for (i = 1; i < 4096; i++)
        assert(vebtree_predecessor(tree, i) == i - 1);

    vebtree_free(tree);
}

void should_find_predecessor_with_gaps_u4096()
{
    size_t i; VebTree* tree;
    vebtree_init(&tree, 12, 0);

    for (i = 1; i < 4096; i += 2)
        vebtree_insert_key(tree, i);

    assert(vebtree_predecessor(tree, 1) == vebtree_null);
    assert(vebtree_predecessor(tree, 0) == vebtree_null);
    for (i = 2; i < 4096; i += 2)
        assert(vebtree_predecessor(tree, i) == i - 1);
    for (i = 3; i < 4096; i += 2)
        assert(vebtree_predecessor(tree, i) == i - 2);

    vebtree_free(tree);
}

void should_find_predecessor_crossing_low_u4096()
{
    VebTree* tree;
    vebtree_init(&tree, 12, 0);

    vebtree_insert_key(tree, 5);
    vebtree_insert_key(tree, 200);

    assert(vebtree_predecessor(tree, 50) == 5);
    assert(vebtree_predecessor(tree, 6) == 5);
    assert(vebtree_predecessor(tree, 5) == vebtree_null);
    assert(vebtree_predecessor(tree, 200) == 5);
    assert(vebtree_predecessor(tree, 201) == 200);

    vebtree_free(tree);
}

void should_find_predecessor_on_singleton_u4096()
{
    VebTree* tree;
    vebtree_init(&tree, 12, 0);
    vebtree_insert_key(tree, 42);

    assert(vebtree_predecessor(tree, 43) == 42);
    assert(vebtree_predecessor(tree, 4095) == 42);
    assert(vebtree_predecessor(tree, 42) == vebtree_null);
    assert(vebtree_predecessor(tree, 1) == vebtree_null);

    vebtree_free(tree);
}

int main(int argc, char** argv)
{
    should_create_fully_alloc_tree_u4096();
    should_insert_into_fully_alloc_tree_u4096();
    should_delete_from_fully_alloc_tree_u4096();
    should_handle_bit_zero_in_bitwise_leaf_predecessor();
    should_return_null_on_empty_tree_u4096();
    should_delegate_to_leaf_in_small_tree_u64();
    should_find_predecessor_in_fully_alloc_tree_u4096();
    should_find_predecessor_with_gaps_u4096();
    should_find_predecessor_crossing_low_u4096();
    should_find_predecessor_on_singleton_u4096();
    return 0;
}
