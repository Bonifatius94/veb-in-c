#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <assert.h>
#include <time.h>
#include <string.h>
#include "vebtrees.h"

/* ====================================================
 *        V A N   E M D E   B O A S   S O R T
 * ==================================================== */

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

/* this is just for testing whether the predecessor() operation works,
   one sorting procedure using the van Emde Boas tree totally suffices */
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

/* ====================================================
 *                Q U I C K   S O R T
 * ==================================================== */

void swap(uint64_t *a, uint64_t *b)
{
    uint64_t temp = *a;
    *a = *b;
    *b = temp;
}

uint64_t partition(uint64_t array[], uint64_t low, uint64_t high)
{
    size_t i = low - 1, j;
    uint64_t pivot = array[high];

    for (j = low; j < high; j++)
        if (array[j] <= pivot)
            swap(&array[++i], &array[j]);

    swap(&array[i + 1], &array[high]);
    return (i + 1);
}

void _quick_sort(uint64_t array[], uint64_t low, uint64_t high)
{
    uint64_t pivot;

    /* recursion anchor */
    if (low >= high) return;

    /* recursion step */
    pivot = partition(array, low, high);
    _quickSort(array, low, pivot - 1);
    _quickSort(array, pivot + 1, high);
}

void quick_sort(uint64_t array[], uint64_t num_keys, uint64_t output[])
{
    memcpy(output, array, sizeof(uint64_t) * num_keys);
    _quick_sort(output, 0, num_keys - 1);
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

double benchmark_sort_algo_in_ms(void* sort_func, uint64_t num_keys, size_t test_runs, size_t rng_seed)
{
    size_t i, t;
    uint64_t *keys, *sorted_keys;
    clock_t start, end; double elapsed = 0;

    keys = malloc(sizeof(uint64_t) * num_keys);
    sorted_keys = malloc(sizeof(uint64_t) * num_keys);
    for (i = 0; i < num_keys; i++) keys[i] = i;
    srand(rng_seed);

    for (t = 0; t < test_runs; t++)
    {
        /* generate a randomly distributed array of keys */
        linear_shuffle(keys, num_keys);

        /* call the sorting routine and measure the time elapsed */
        start = clock();
        (*sort_func)(keys, num_keys, sorted_keys);
        end = clock();

        /* verify that the sorted keys are valid */
        for (i = 0; i < num_keys-1; i++)
            assert(sorted_keys[i] < sorted_keys[i+1]);

        elapsed += (double)(end - start) / CLOCKS_PER_SEC;
    }

    free(keys); free(sorted_keys);
    return elapsed / test_runs * 1000;
}
    
int main(int argc, char** argv)
{
    size_t num_keys = 500000, test_runs = 100, rng_seed = 42;

    printf("Veb sorting took %lf milliseconds\n", 
           benchmark_sort_algo_in_ms(&sort_veb_succ, num_keys, test_runs, rng_seed));

    /* TODO: enable this test to verify that predecessor() works */
    /*printf("Veb sorting (backwards) took %lf milliseconds\n", 
           benchmark_sort_algo_in_ms(&sort_veb_pred, num_keys, test_runs, rng_seed));*/

    printf("Quicksort took %lf milliseconds\n", 
           benchmark_sort_algo_in_ms(&quick_sort, num_keys, test_runs, rng_seed));

    return 0;
}
