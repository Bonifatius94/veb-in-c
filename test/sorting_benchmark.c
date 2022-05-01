#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <assert.h>
#include <time.h>
#include "vebtrees.h"

int sort_veb_succ(const uint64_t keys[], size_t num_keys, uint64_t output[])
{
    size_t i; VebTree* tree;

    vebtree_init(&tree, 19, VEBTREE_DEFAULT_FLAGS);
    for (i = 0; i < num_keys; i++)
        vebtree_insert_key(tree, keys[i]);

    output[0] = vebtree_get_min(tree);
    for (i = 1; i < num_keys; i++)
        output[i] = vebtree_successor(tree, output[i-1]);

    vebtree_free(tree);
}

int sort_veb_pred(const uint64_t keys[], size_t num_keys, uint64_t output[])
{
    size_t i; VebTree* tree;

    vebtree_init(&tree, 19, VEBTREE_DEFAULT_FLAGS);
    for (i = 0; i < num_keys; i++)
        vebtree_insert_key(tree, keys[i]);

    output[num_keys-1] = vebtree_get_max(tree);
    for (i = num_keys-2; i > 0; i--)
        output[i] = vebtree_predecessor(tree, output[i+1]);

    vebtree_free(tree);
}

void linear_shuffle(uint64_t keys[], size_t num_keys)
{
    size_t i, j; uint64_t temp;

    for (i = 0; i < num_keys-1; i++) {
        j = rand() % (num_keys - i) + i;

        if (i != j) {
            temp = keys[i];
            keys[i] = keys[j];
            keys[j] = temp;
        }
    }
}

int main(int argc, char** argv)
{
    size_t i, t, num_keys = 500000, test_runs = 100;
    uint64_t *keys, *sorted_keys;
    clock_t start, end; double elapsed = 0;

    keys = malloc(sizeof(uint64_t) * num_keys);
    sorted_keys = malloc(sizeof(uint64_t) * num_keys);

    /* generate random distributed array of keys */
    srand(42);
    for (i = 0; i < num_keys; i++)
        keys[i] = i;
    linear_shuffle(keys, num_keys);

    for (t = 0; t < test_runs; t++)
    {
        /* sort keys using insert() / min() / successor() operations */
        start = clock();
        sort_veb_succ(keys, num_keys, sorted_keys);
        end = clock();
        elapsed += (double)(end - start) / CLOCKS_PER_SEC;
        for (i = 0; i < num_keys-1; i++)
            assert(sorted_keys[i] < sorted_keys[i+1]);
    }

    elapsed /= test_runs;
    printf("Sorting took %lf milliseconds\n", elapsed * 1000);

    /* sort keys using insert() / max() / predecessor() operations */
    sort_veb_pred(keys, num_keys, sorted_keys);
    for (i = 0; i < num_keys-1; i++)
        assert(sorted_keys[i] < sorted_keys[i+1]);
    // TODO: enable this test when the predecessor operation is ready for use

    free(keys); free(sorted_keys);
    return 0;
}
