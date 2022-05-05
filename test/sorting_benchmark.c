#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <assert.h>
#include <time.h>
#include <string.h>
#include "vebtrees.h"

/* ====================================================
 *         V A N   E M D E   B O A S   S O R T
 * ==================================================== */

void sort_veb_succ(const uint64_t keys[], size_t num_keys, uint64_t output[])
{
    size_t i; VebTree* tree; uint8_t uni_bits;

    uni_bits = vebtree_required_universe_bits(num_keys);
    vebtree_init(&tree, uni_bits, VEBTREE_DEFAULT_FLAGS);

    for (i = 0; i < num_keys; i++)
        vebtree_insert_key(tree, keys[i]);

    output[0] = vebtree_get_min(tree);
    for (i = 1; i < num_keys; i++)
        output[i] = vebtree_successor(tree, output[i-1]);

    vebtree_free(tree);
}

/* this is just for testing whether the predecessor() operation works,
   one sorting procedure using the van Emde Boas tree totally suffices */
void sort_veb_pred(const uint64_t keys[], size_t num_keys, uint64_t output[])
{
    size_t i; VebTree* tree; uint8_t uni_bits;
    
    uni_bits = vebtree_required_universe_bits(num_keys);
    vebtree_init(&tree, uni_bits, VEBTREE_DEFAULT_FLAGS);

    for (i = 0; i < num_keys; i++)
        vebtree_insert_key(tree, keys[i]);

    output[num_keys-1] = vebtree_get_max(tree);
    for (i = num_keys-2; i > 0; i--)
        output[i] = vebtree_predecessor(tree, output[i+1]);

    vebtree_free(tree);
}

/* ====================================================
 *                Q U I C K   S O R T
 * ==================================================== */

int compare(const void* a, const void* b)
{
    if (*(uint64_t*)a < *(uint64_t*)b)
        return -1;
    if (*(uint64_t*)a > *(uint64_t*)b)
        return 1;
    return 0;
}

void quick_sort(const uint64_t array[], size_t num_keys, uint64_t output[])
{
    memcpy(output, array, sizeof(uint64_t) * num_keys);
    qsort(output, num_keys, sizeof(uint64_t), compare);
}

/* ====================================================
 *                B E N C H M A R K
 * ==================================================== */

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

double benchmark_sort_algo_in_ms(void (*sort_func)(const uint64_t*, size_t, uint64_t*),
                                 size_t num_keys, size_t test_runs)
{
    size_t i, t;
    uint64_t *keys, *sorted_keys;
    clock_t start, end; double elapsed = 0;

    keys = malloc(sizeof(vebkey_t) * num_keys);
    assert(keys != NULL && "keys array allocation failed unexpectedly!");

    sorted_keys = malloc(sizeof(vebkey_t) * num_keys);
    assert(sorted_keys != NULL && "sorted keys array allocation failed unexpectedly!");

    for (i = 0; i < num_keys; i++)
        keys[i] = (vebkey_t)i;

    for (t = 0; t < test_runs; t++)
    {
        /* generate a randomly distributed array of keys */
        srand((uint32_t)t);
        linear_shuffle(keys, num_keys);

        /* call the sorting routine and measure the time elapsed */
        start = clock();
        (*sort_func)(keys, num_keys, sorted_keys);
        end = clock();

        /* verify that the sorted keys are valid */
        for (i = 0; i < num_keys-1; i++)
            assert(sorted_keys[i] < sorted_keys[i+1]);

        elapsed += ((double)end - start) / CLOCKS_PER_SEC;
    }

    free(keys); free(sorted_keys);
    return elapsed / test_runs * 1000;
}

int main(int argc, char** argv)
{
    size_t num_keys = 500000, test_runs = 100;

    printf("Veb sorting took %lf milliseconds\n", 
           benchmark_sort_algo_in_ms(&sort_veb_succ, num_keys, test_runs));

    /* TODO: enable this test to verify that predecessor() works */
    /*printf("Veb sorting (backwards) took %lf milliseconds\n", 
           benchmark_sort_algo_in_ms(&sort_veb_pred, num_keys, test_runs, rng_seed));*/

    printf("Quicksort took %lf milliseconds\n", 
           benchmark_sort_algo_in_ms(&quick_sort, num_keys, test_runs));

    return 0;
}
