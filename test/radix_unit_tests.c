#include <stdbool.h>
#include <assert.h>
#include <stdlib.h>
#include "radix64.h"

void should_init_and_free_empty_u32()
{
    Radix64* tree;
    radix64_init(&tree, 32);

    assert(tree->universe_bits == 32);
    assert(tree->depth == 6);
    assert(tree->root == NULL);
    assert(radix64_is_empty(tree));

    radix64_free(tree);
}

void should_init_and_free_empty_u19()
{
    Radix64* tree;
    radix64_init(&tree, 19);

    assert(tree->universe_bits == 19);
    assert(tree->depth == 4);
    assert(tree->root == NULL);
    assert(radix64_is_empty(tree));

    radix64_free(tree);
}

void should_handle_single_key_u32()
{
    Radix64* tree;
    r64key_t k;
    radix64_init(&tree, 32);

    radix64_insert_key(tree, 0xDEADBEEF);

    assert(!radix64_is_empty(tree));
    assert(radix64_contains_key(tree, 0xDEADBEEF));
    assert(!radix64_contains_key(tree, 0xDEADBEF0));
    assert(!radix64_contains_key(tree, 0xDEADBEEE));
    assert(radix64_get_min(tree) == 0xDEADBEEF);
    assert(radix64_get_max(tree) == 0xDEADBEEF);

    k = radix64_successor(tree, 0);
    assert(k == 0xDEADBEEF);
    k = radix64_successor(tree, 0xDEADBEEF);
    assert(k == r64_null);
    k = radix64_successor(tree, 0xDEADBEEE);
    assert(k == 0xDEADBEEF);

    k = radix64_predecessor(tree, 0xFFFFFFFFULL);
    assert(k == 0xDEADBEEF);
    k = radix64_predecessor(tree, 0xDEADBEEF);
    assert(k == r64_null);
    k = radix64_predecessor(tree, 0xDEADBEF0);
    assert(k == 0xDEADBEEF);

    radix64_delete_key(tree, 0xDEADBEEF);
    assert(radix64_is_empty(tree));
    assert(tree->root == NULL);

    radix64_free(tree);
}

void should_round_trip_dense_u16()
{
    Radix64* tree;
    r64key_t k;
    size_t i;
    radix64_init(&tree, 16);

    for (i = 0; i < 65536; i++) radix64_insert_key(tree, (r64key_t)i);
    for (i = 0; i < 65536; i++) assert(radix64_contains_key(tree, (r64key_t)i));

    assert(radix64_get_min(tree) == 0);
    assert(radix64_get_max(tree) == 65535);

    k = radix64_get_min(tree);
    for (i = 0; i < 65535; i++) {
        r64key_t next;
        next = radix64_successor(tree, k);
        assert(next == k + 1);
        k = next;
    }
    assert(radix64_successor(tree, 65535) == r64_null);

    for (i = 0; i < 65536; i++) radix64_delete_key(tree, (r64key_t)i);
    assert(radix64_is_empty(tree));
    assert(tree->root == NULL);

    radix64_free(tree);
}

void should_round_trip_sparse_u32()
{
    Radix64* tree;
    r64key_t k;
    size_t i;
    r64key_t keys[500];
    radix64_init(&tree, 32);

    for (i = 0; i < 500; i++) {
        keys[i] = (r64key_t)((i * 8392857ULL) & 0xFFFFFFFFULL);
        /* stay clear of r64_null. all products here are well under 2^32 */
        radix64_insert_key(tree, keys[i]);
    }
    for (i = 0; i < 500; i++) assert(radix64_contains_key(tree, keys[i]));

    /* successor sweep yields sorted sequence */
    k = radix64_get_min(tree);
    for (i = 0; i < 499; i++) {
        r64key_t next;
        next = radix64_successor(tree, k);
        assert(next != r64_null);
        assert(next > k);
        k = next;
    }
    assert(radix64_successor(tree, k) == r64_null);
    assert(k == radix64_get_max(tree));

    /* delete in reverse insertion order */
    for (i = 500; i > 0; i--) radix64_delete_key(tree, keys[i - 1]);
    assert(radix64_is_empty(tree));
    assert(tree->root == NULL);

    radix64_free(tree);
}

void should_handle_successor_across_gaps_u24()
{
    Radix64* tree;
    radix64_init(&tree, 24);

    radix64_insert_key(tree, 1);
    radix64_insert_key(tree, 64);
    radix64_insert_key(tree, 65);
    radix64_insert_key(tree, 128);
    radix64_insert_key(tree, 4096);
    radix64_insert_key(tree, 4160);

    assert(radix64_get_min(tree) == 1);
    assert(radix64_get_max(tree) == 4160);

    /* successor sweep */
    assert(radix64_successor(tree, 0) == 1);
    assert(radix64_successor(tree, 1) == 64);
    assert(radix64_successor(tree, 2) == 64);
    assert(radix64_successor(tree, 63) == 64);
    assert(radix64_successor(tree, 64) == 65);
    assert(radix64_successor(tree, 65) == 128);
    assert(radix64_successor(tree, 66) == 128);
    assert(radix64_successor(tree, 127) == 128);
    assert(radix64_successor(tree, 128) == 4096);
    assert(radix64_successor(tree, 129) == 4096);
    assert(radix64_successor(tree, 4095) == 4096);
    assert(radix64_successor(tree, 4096) == 4160);
    assert(radix64_successor(tree, 4097) == 4160);
    assert(radix64_successor(tree, 4159) == 4160);
    assert(radix64_successor(tree, 4160) == r64_null);
    assert(radix64_successor(tree, 4161) == r64_null);

    /* predecessor sweep */
    assert(radix64_predecessor(tree, 4160) == 4096);
    assert(radix64_predecessor(tree, 4161) == 4160);
    assert(radix64_predecessor(tree, 4200) == 4160);
    assert(radix64_predecessor(tree, 4096) == 128);
    assert(radix64_predecessor(tree, 4000) == 128);
    assert(radix64_predecessor(tree, 128) == 65);
    assert(radix64_predecessor(tree, 127) == 65);
    assert(radix64_predecessor(tree, 65) == 64);
    assert(radix64_predecessor(tree, 64) == 1);
    assert(radix64_predecessor(tree, 2) == 1);
    assert(radix64_predecessor(tree, 1) == r64_null);
    assert(radix64_predecessor(tree, 0) == r64_null);

    radix64_free(tree);
}

void should_return_null_on_empty_tree_u32()
{
    Radix64* tree;
    radix64_init(&tree, 32);

    assert(radix64_is_empty(tree));
    assert(!radix64_contains_key(tree, 42));
    assert(radix64_get_min(tree) == r64_null);
    assert(radix64_get_max(tree) == r64_null);
    assert(radix64_successor(tree, 0) == r64_null);
    assert(radix64_successor(tree, 0xFFFFFFFFULL) == r64_null);
    assert(radix64_predecessor(tree, 0) == r64_null);
    assert(radix64_predecessor(tree, 0xFFFFFFFFULL) == r64_null);

    /* delete on empty tree is a no-op */
    radix64_delete_key(tree, 123);
    assert(radix64_is_empty(tree));

    radix64_free(tree);
}

void should_ignore_duplicate_insert_u24()
{
    Radix64* tree;
    radix64_init(&tree, 24);

    radix64_insert_key(tree, 12345);
    radix64_insert_key(tree, 12345);
    radix64_insert_key(tree, 12345);

    assert(radix64_contains_key(tree, 12345));
    assert(radix64_get_min(tree) == 12345);
    assert(radix64_get_max(tree) == 12345);

    /* single delete empties the tree */
    radix64_delete_key(tree, 12345);
    assert(radix64_is_empty(tree));
    assert(tree->root == NULL);

    /* re-insert the same key works cleanly */
    radix64_insert_key(tree, 12345);
    assert(radix64_contains_key(tree, 12345));

    radix64_free(tree);
}

void should_ignore_delete_absent_u24()
{
    Radix64* tree;
    radix64_init(&tree, 24);

    radix64_insert_key(tree, 1000);
    radix64_insert_key(tree, 2000);

    /* absent key */
    radix64_delete_key(tree, 1500);
    assert(radix64_contains_key(tree, 1000));
    assert(radix64_contains_key(tree, 2000));

    /* absent key that would share a path prefix with 1000 */
    radix64_delete_key(tree, 1001);
    assert(radix64_contains_key(tree, 1000));
    assert(radix64_contains_key(tree, 2000));

    /* absent key far out of range of any inserted prefix */
    radix64_delete_key(tree, 0xFFFFFF);
    assert(radix64_contains_key(tree, 1000));
    assert(radix64_contains_key(tree, 2000));

    radix64_free(tree);
}

void should_free_after_sparse_inserts_u32()
{
    Radix64* tree;
    size_t i;
    r64key_t keys[50];
    radix64_init(&tree, 32);

    for (i = 0; i < 50; i++) {
        keys[i] = (r64key_t)((i * 99991ULL) & 0xFFFFFFFFULL);
        radix64_insert_key(tree, keys[i]);
    }
    for (i = 0; i < 50; i++) radix64_delete_key(tree, keys[i]);
    assert(radix64_is_empty(tree));
    assert(tree->root == NULL);

    /* radix64_free on a NULL-rooted tree must not crash */
    radix64_free(tree);
}

void should_compute_required_universe_bits()
{
    assert(radix64_required_universe_bits(1) == 1);
    assert(radix64_required_universe_bits(2) == 2);
    assert(radix64_required_universe_bits(3) == 2);
    assert(radix64_required_universe_bits(4) == 3);
    assert(radix64_required_universe_bits(7) == 3);
    assert(radix64_required_universe_bits(8) == 4);
    assert(radix64_required_universe_bits(255) == 8);
    assert(radix64_required_universe_bits(256) == 9);
    assert(radix64_required_universe_bits((r64key_t)0x7FFFFFFFULL) == 31);
    assert(radix64_required_universe_bits((r64key_t)0xFFFFFFFFULL) == 32);
}

void should_handle_u1()
{
    Radix64* tree;
    radix64_init(&tree, 1);

    assert(tree->depth == 1);
    assert(radix64_is_empty(tree));

    radix64_insert_key(tree, 0);
    radix64_insert_key(tree, 1);

    assert(radix64_contains_key(tree, 0));
    assert(radix64_contains_key(tree, 1));
    assert(radix64_get_min(tree) == 0);
    assert(radix64_get_max(tree) == 1);
    assert(radix64_successor(tree, 0) == 1);
    assert(radix64_successor(tree, 1) == r64_null);
    assert(radix64_predecessor(tree, 1) == 0);
    assert(radix64_predecessor(tree, 0) == r64_null);

    radix64_delete_key(tree, 0);
    assert(radix64_get_min(tree) == 1);
    radix64_delete_key(tree, 1);
    assert(radix64_is_empty(tree));

    radix64_free(tree);
}

void should_handle_u6()
{
    Radix64* tree;
    size_t i;
    radix64_init(&tree, 6);

    assert(tree->depth == 1);

    /* dense fill of the entire 64-key universe */
    for (i = 0; i < 64; i++) radix64_insert_key(tree, (r64key_t)i);
    for (i = 0; i < 64; i++) assert(radix64_contains_key(tree, (r64key_t)i));
    assert(radix64_get_min(tree) == 0);
    assert(radix64_get_max(tree) == 63);
    assert(radix64_successor(tree, 62) == 63);
    assert(radix64_successor(tree, 63) == r64_null);
    assert(radix64_predecessor(tree, 1) == 0);
    assert(radix64_predecessor(tree, 0) == r64_null);

    for (i = 0; i < 64; i++) radix64_delete_key(tree, (r64key_t)i);
    assert(radix64_is_empty(tree));

    radix64_free(tree);
}

int main(void)
{
    should_init_and_free_empty_u32();
    should_init_and_free_empty_u19();
    should_handle_single_key_u32();
    should_round_trip_dense_u16();
    should_round_trip_sparse_u32();
    should_handle_successor_across_gaps_u24();
    should_return_null_on_empty_tree_u32();
    should_ignore_duplicate_insert_u24();
    should_ignore_delete_absent_u24();
    should_free_after_sparse_inserts_u32();
    should_compute_required_universe_bits();
    should_handle_u1();
    should_handle_u6();
    return 0;
}
