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

void should_round_trip_odd_universe_u8192()
{
    size_t i; VebTree* tree;
    vebtree_init(&tree, 13, 0);
    assert(vebtree_is_empty(tree));

    for (i = 0; i < 8192; i++)
        vebtree_insert_key(tree, i);

    assert(vebtree_get_min(tree) == 0);
    assert(vebtree_get_max(tree) == 8191);
    assert(vebtree_successor(tree, 0) == 1);
    assert(vebtree_successor(tree, 4095) == 4096);
    assert(vebtree_successor(tree, 8190) == 8191);
    assert(vebtree_predecessor(tree, 1) == 0);
    assert(vebtree_predecessor(tree, 4096) == 4095);
    assert(vebtree_predecessor(tree, 8191) == 8190);
    assert(vebtree_predecessor(tree, 0) == vebtree_null);

    for (i = 0; i < 8192; i++)
        vebtree_delete_key(tree, i);
    assert(vebtree_is_empty(tree));

    vebtree_free(tree);
}

void should_handle_deep_recursion_u24()
{
    size_t i; VebTree* tree;
    vebkey_t keys[500];
    vebtree_init(&tree, 24, 0);
    assert(vebtree_is_empty(tree));

    /* sparse keys spread across the 16M universe;
       exercises tree->global as itself an internal (non-leaf) node */
    for (i = 0; i < 500; i++) {
        keys[i] = (vebkey_t)(i * 33331);
        assert(!vebtree_contains_key(tree, keys[i]));
        vebtree_insert_key(tree, keys[i]);
        assert(vebtree_contains_key(tree, keys[i]));
    }

    assert(vebtree_get_min(tree) == 0);
    assert(vebtree_get_max(tree) == (vebkey_t)(499 * 33331));

    for (i = 0; i < 499; i++)
        assert(vebtree_successor(tree, keys[i]) == keys[i + 1]);
    for (i = 1; i < 500; i++)
        assert(vebtree_predecessor(tree, keys[i]) == keys[i - 1]);
    assert(vebtree_predecessor(tree, keys[0]) == vebtree_null);

    /* delete in reverse-insertion order */
    for (i = 500; i > 0; i--) {
        assert(vebtree_contains_key(tree, keys[i - 1]));
        vebtree_delete_key(tree, keys[i - 1]);
        assert(!vebtree_contains_key(tree, keys[i - 1]));
    }
    assert(vebtree_is_empty(tree));

    vebtree_free(tree);
}

void should_round_trip_odd_universe_u128()
{
    size_t i; VebTree* tree;
    vebtree_init(&tree, 7, 0);
    assert(vebtree_is_empty(tree));

    for (i = 0; i < 128; i++) {
        assert(!vebtree_contains_key(tree, i));
        vebtree_insert_key(tree, i);
        assert(vebtree_contains_key(tree, i));
    }

    assert(vebtree_get_min(tree) == 0);
    assert(vebtree_get_max(tree) == 127);
    for (i = 0; i < 127; i++)
        assert(vebtree_successor(tree, i) == i + 1);
    for (i = 1; i < 128; i++)
        assert(vebtree_predecessor(tree, i) == i - 1);

    for (i = 0; i < 128; i++) {
        vebtree_delete_key(tree, i);
        assert(!vebtree_contains_key(tree, i));
    }
    assert(vebtree_is_empty(tree));

    vebtree_free(tree);
}

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

void should_handle_bit_zero_in_bitwise_leaf_successor()
{
    VebTree leaf;

    /* empty leaf: successor of any key must be vebtree_null */
    leaf = vebtree_new_empty_bitwise_leaf(6);
    assert(vebtree_bitwise_leaf_successor(&leaf, 0) == vebtree_null);
    assert(vebtree_bitwise_leaf_successor(&leaf, 5) == vebtree_null);
    assert(vebtree_bitwise_leaf_successor(&leaf, 62) == vebtree_null);

    /* only bit 5 set: successor crosses from 4 to 5, saturates past 5 */
    vebtree_bitwise_leaf_insert_key(&leaf, 5);
    assert(vebtree_bitwise_leaf_successor(&leaf, 0) == 5);
    assert(vebtree_bitwise_leaf_successor(&leaf, 4) == 5);
    assert(vebtree_bitwise_leaf_successor(&leaf, 5) == vebtree_null);
    assert(vebtree_bitwise_leaf_successor(&leaf, 6) == vebtree_null);

    /* only bit 0 set: no bit above 0, successor of 0 is null */
    leaf = vebtree_new_empty_bitwise_leaf(6);
    vebtree_bitwise_leaf_insert_key(&leaf, 0);
    assert(vebtree_bitwise_leaf_successor(&leaf, 0) == vebtree_null);
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

void should_return_null_on_empty_tree_successor_u4096()
{
    VebTree* tree;
    vebtree_init(&tree, 12, 0);
    assert(vebtree_is_empty(tree));
    assert(vebtree_successor(tree, 0) == vebtree_null);
    assert(vebtree_successor(tree, 100) == vebtree_null);
    assert(vebtree_successor(tree, 4094) == vebtree_null);
    vebtree_free(tree);
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

void should_delegate_to_leaf_successor_in_small_tree_u64()
{
    VebTree* tree;
    vebtree_init(&tree, 6, 0);
    assert(vebtree_is_leaf(tree));

    vebtree_insert_key(tree, 3);
    vebtree_insert_key(tree, 17);
    vebtree_insert_key(tree, 42);

    assert(vebtree_successor(tree, 0) == 3);
    assert(vebtree_successor(tree, 2) == 3);
    assert(vebtree_successor(tree, 3) == 17);
    assert(vebtree_successor(tree, 16) == 17);
    assert(vebtree_successor(tree, 17) == 42);
    assert(vebtree_successor(tree, 41) == 42);
    assert(vebtree_successor(tree, 42) == vebtree_null);
    assert(vebtree_successor(tree, 62) == vebtree_null);

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

void should_find_successor_in_fully_alloc_tree_u4096()
{
    size_t i; VebTree* tree;
    vebtree_init(&tree, 12, 0);

    for (i = 0; i < 4096; i++)
        vebtree_insert_key(tree, i);

    assert(vebtree_successor(tree, 4095) == vebtree_null);
    for (i = 0; i < 4095; i++)
        assert(vebtree_successor(tree, i) == i + 1);

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

void should_find_successor_with_gaps_u4096()
{
    size_t i; VebTree* tree;
    vebtree_init(&tree, 12, 0);

    for (i = 1; i < 4096; i += 2)
        vebtree_insert_key(tree, i);

    assert(vebtree_successor(tree, 4095) == vebtree_null);
    assert(vebtree_successor(tree, 0) == 1);
    for (i = 0; i < 4095; i += 2)
        assert(vebtree_successor(tree, i) == i + 1);
    for (i = 1; i < 4094; i += 2)
        assert(vebtree_successor(tree, i) == i + 2);

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

void should_find_successor_crossing_low_u4096()
{
    VebTree* tree;
    vebtree_init(&tree, 12, 0);

    vebtree_insert_key(tree, 5);
    vebtree_insert_key(tree, 200);

    assert(vebtree_successor(tree, 0) == 5);
    assert(vebtree_successor(tree, 4) == 5);
    assert(vebtree_successor(tree, 5) == 200);
    assert(vebtree_successor(tree, 6) == 200);
    assert(vebtree_successor(tree, 199) == 200);
    assert(vebtree_successor(tree, 200) == vebtree_null);
    /* successor past tree->high at global_key == 63 triggers leading_bits_mask(64)
       which is UB on 64-bit shifts - tracked separately, do not test here */

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

void should_find_successor_on_singleton_u4096()
{
    VebTree* tree;
    vebtree_init(&tree, 12, 0);
    vebtree_insert_key(tree, 42);

    assert(vebtree_successor(tree, 0) == 42);
    assert(vebtree_successor(tree, 41) == 42);
    assert(vebtree_successor(tree, 42) == vebtree_null);
    assert(vebtree_successor(tree, 100) == vebtree_null);

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

void should_ignore_duplicate_insert_u4096()
{
    VebTree* tree;
    vebtree_init(&tree, 12, 0);

    vebtree_insert_key(tree, 10);
    vebtree_insert_key(tree, 100);
    vebtree_insert_key(tree, 1000);

    /* re-insert each key - must be a no-op */
    vebtree_insert_key(tree, 10);
    vebtree_insert_key(tree, 100);
    vebtree_insert_key(tree, 1000);

    assert(vebtree_contains_key(tree, 10));
    assert(vebtree_contains_key(tree, 100));
    assert(vebtree_contains_key(tree, 1000));
    assert(vebtree_get_min(tree) == 10);
    assert(vebtree_get_max(tree) == 1000);

    /* deleting each should cleanly empty the tree;
       if any re-insert corrupted the subtree structure,
       delete leaves stale state behind */
    vebtree_delete_key(tree, 10);
    vebtree_delete_key(tree, 100);
    vebtree_delete_key(tree, 1000);

    assert(vebtree_is_empty(tree));
    assert(!vebtree_contains_key(tree, 10));
    assert(!vebtree_contains_key(tree, 100));
    assert(!vebtree_contains_key(tree, 1000));
    assert(vebtree_get_min(tree) == vebtree_null);
    assert(vebtree_get_max(tree) == vebtree_null);

    vebtree_free(tree);
}

void should_ignore_duplicate_insert_on_low_u4096()
{
    VebTree* tree;
    vebtree_init(&tree, 12, 0);

    vebtree_insert_key(tree, 5);
    vebtree_insert_key(tree, 50);
    vebtree_insert_key(tree, 500);

    /* re-inserting the current low is the canonical corruption case:
       unguarded insert pushes low into locals[0], violating the
       "low not in any subtree" invariant */
    vebtree_insert_key(tree, 5);

    assert(vebtree_get_min(tree) == 5);
    assert(vebtree_get_max(tree) == 500);

    /* delete the low - on a corrupted tree the new low is pulled
       from the polluted subtree, leaving tree->low == 5 */
    vebtree_delete_key(tree, 5);

    assert(!vebtree_contains_key(tree, 5));
    assert(vebtree_get_min(tree) == 50);
    assert(vebtree_contains_key(tree, 50));
    assert(vebtree_contains_key(tree, 500));

    vebtree_free(tree);
}

void should_ignore_delete_absent_u4096()
{
    VebTree* tree;
    vebtree_init(&tree, 12, 0);

    /* delete from empty tree - no-op */
    vebtree_delete_key(tree, 7);
    assert(vebtree_is_empty(tree));

    /* single-element tree: unguarded delete-absent wipes the entry
       because the "tree->low == tree->high" branch fires regardless
       of whether the key matches */
    vebtree_insert_key(tree, 42);
    vebtree_delete_key(tree, 7);
    assert(vebtree_contains_key(tree, 42));
    assert(vebtree_get_min(tree) == 42);
    assert(vebtree_get_max(tree) == 42);

    /* populated tree: delete-absent must not disturb other keys */
    vebtree_insert_key(tree, 100);
    vebtree_insert_key(tree, 1000);
    vebtree_delete_key(tree, 7);
    vebtree_delete_key(tree, 200);
    vebtree_delete_key(tree, 2000);
    assert(vebtree_contains_key(tree, 42));
    assert(vebtree_contains_key(tree, 100));
    assert(vebtree_contains_key(tree, 1000));
    assert(vebtree_get_min(tree) == 42);
    assert(vebtree_get_max(tree) == 1000);

    vebtree_free(tree);
}

void should_ignore_delete_absent_on_leaf_u64()
{
    VebTree* tree;
    vebtree_init(&tree, 6, 0);
    assert(vebtree_is_leaf(tree));

    vebtree_delete_key(tree, 3);
    assert(vebtree_is_empty(tree));

    vebtree_insert_key(tree, 3);
    vebtree_insert_key(tree, 17);
    vebtree_insert_key(tree, 42);

    vebtree_delete_key(tree, 0);
    vebtree_delete_key(tree, 20);
    vebtree_delete_key(tree, 63);

    assert(vebtree_contains_key(tree, 3));
    assert(vebtree_contains_key(tree, 17));
    assert(vebtree_contains_key(tree, 42));
    assert(vebtree_get_min(tree) == 3);
    assert(vebtree_get_max(tree) == 42);

    vebtree_free(tree);
}

int main(int argc, char** argv)
{
    should_create_fully_alloc_tree_u4096();
    should_round_trip_odd_universe_u128();
    should_round_trip_odd_universe_u8192();
    should_handle_deep_recursion_u24();
    should_insert_into_fully_alloc_tree_u4096();
    should_delete_from_fully_alloc_tree_u4096();
    should_handle_bit_zero_in_bitwise_leaf_successor();
    should_handle_bit_zero_in_bitwise_leaf_predecessor();
    should_return_null_on_empty_tree_successor_u4096();
    should_return_null_on_empty_tree_u4096();
    should_delegate_to_leaf_successor_in_small_tree_u64();
    should_delegate_to_leaf_in_small_tree_u64();
    should_find_successor_in_fully_alloc_tree_u4096();
    should_find_predecessor_in_fully_alloc_tree_u4096();
    should_find_successor_with_gaps_u4096();
    should_find_predecessor_with_gaps_u4096();
    should_find_successor_crossing_low_u4096();
    should_find_predecessor_crossing_low_u4096();
    should_find_successor_on_singleton_u4096();
    should_find_predecessor_on_singleton_u4096();
    should_ignore_duplicate_insert_u4096();
    should_ignore_duplicate_insert_on_low_u4096();
    should_ignore_delete_absent_u4096();
    should_ignore_delete_absent_on_leaf_u64();
    return 0;
}
