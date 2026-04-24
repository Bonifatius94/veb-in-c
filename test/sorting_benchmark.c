#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <assert.h>
#include <time.h>
#include <string.h>
#include "vebtrees.h"
#include "radix64.h"

/* Two workloads are exercised here:
   - DENSE: num_keys is a permutation of [0, num_keys), universe tight.
   - SPARSE: num_keys distinct keys drawn from a larger universe via a
     coprime multiplicative hash (stride * i mod 2^u with odd stride).
   Each sort_* routine takes an explicit universe_bits so the harness can
   pass 19 (dense fit for 500k) or 24 (sparse ~0.6% density). */

typedef void (*sort_fn_t)(const uint64_t*, size_t, uint64_t*, uint8_t);
typedef void (*keygen_fn_t)(uint64_t*, size_t, uint32_t);

/* ====================================================
 *         V A N   E M D E   B O A S   S O R T
 * ==================================================== */

void sort_veb_succ(const uint64_t keys[], size_t num_keys, uint64_t output[], uint8_t uni_bits)
{
    size_t i; VebTree* tree;

    vebtree_init(&tree, uni_bits, VEBTREE_DEFAULT_FLAGS);

    for (i = 0; i < num_keys; i++)
        vebtree_insert_key(tree, keys[i]);

    output[0] = vebtree_get_min(tree);
    for (i = 1; i < num_keys; i++)
        output[i] = vebtree_successor(tree, output[i-1]);

    vebtree_free(tree);
}

void sort_veb_pred(const uint64_t keys[], size_t num_keys, uint64_t output[], uint8_t uni_bits)
{
    size_t i; VebTree* tree;

    vebtree_init(&tree, uni_bits, VEBTREE_DEFAULT_FLAGS);

    for (i = 0; i < num_keys; i++)
        vebtree_insert_key(tree, keys[i]);

    output[num_keys - 1] = vebtree_get_max(tree);
    for (i = num_keys - 1; i > 0; i--)
        output[i - 1] = vebtree_predecessor(tree, output[i]);

    vebtree_free(tree);
}

/* ====================================================
 *              R A D I X   6 4   S O R T
 * ==================================================== */

void sort_radix_succ(const uint64_t keys[], size_t num_keys, uint64_t output[], uint8_t uni_bits)
{
    size_t i; Radix64* tree;

    radix64_init(&tree, uni_bits);

    for (i = 0; i < num_keys; i++)
        radix64_insert_key(tree, (r64key_t)keys[i]);

    output[0] = radix64_get_min(tree);
    for (i = 1; i < num_keys; i++)
        output[i] = radix64_successor(tree, output[i - 1]);

    radix64_free(tree);
}

void sort_radix_pred(const uint64_t keys[], size_t num_keys, uint64_t output[], uint8_t uni_bits)
{
    size_t i; Radix64* tree;

    radix64_init(&tree, uni_bits);

    for (i = 0; i < num_keys; i++)
        radix64_insert_key(tree, (r64key_t)keys[i]);

    output[num_keys - 1] = radix64_get_max(tree);
    for (i = num_keys - 1; i > 0; i--)
        output[i - 1] = radix64_predecessor(tree, output[i]);

    radix64_free(tree);
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

void quick_sort(const uint64_t array[], size_t num_keys, uint64_t output[], uint8_t uni_bits)
{
    (void)uni_bits;
    memcpy(output, array, sizeof(uint64_t) * num_keys);
    qsort(output, num_keys, sizeof(uint64_t), compare);
}

/* ====================================================
 *              K E Y   G E N E R A T O R S
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

/* Dense keys: permutation of [0, num_keys). */
void gen_dense(uint64_t* keys, size_t num_keys, uint32_t seed)
{
    size_t i;
    for (i = 0; i < num_keys; i++) keys[i] = (uint64_t)i;
    srand(seed);
    linear_shuffle(keys, num_keys);
}

/* Sparse keys: num_keys distinct values in [0, 2^24). Uses i * STRIDE mod
   2^24 with STRIDE odd and coprime to 2^24, so the mapping is a permutation
   of 2^24 keys — taking the first num_keys yields distinct sparse keys. */
#define SPARSE_STRIDE 8392857ULL
#define SPARSE_UNIVERSE_BITS 24

void gen_sparse(uint64_t* keys, size_t num_keys, uint32_t seed)
{
    size_t i;
    uint64_t mask = ((uint64_t)1 << SPARSE_UNIVERSE_BITS) - 1;
    for (i = 0; i < num_keys; i++)
        keys[i] = ((uint64_t)i * SPARSE_STRIDE) & mask;
    srand(seed);
    linear_shuffle(keys, num_keys);
}

/* ====================================================
 *                B E N C H M A R K
 * ==================================================== */

double benchmark_sort_algo_in_ms(
    sort_fn_t sort_func, keygen_fn_t gen_keys,
    size_t num_keys, uint8_t universe_bits, size_t test_runs)
{
    size_t i, t;
    uint64_t *keys, *sorted_keys;
    clock_t start, end; double elapsed = 0;

    keys = malloc(sizeof(vebkey_t) * num_keys);
    assert(keys != NULL && "keys array allocation failed unexpectedly!");

    sorted_keys = malloc(sizeof(vebkey_t) * num_keys);
    assert(sorted_keys != NULL && "sorted keys array allocation failed unexpectedly!");

    for (t = 0; t < test_runs; t++)
    {
        gen_keys(keys, num_keys, (uint32_t)t);

        start = clock();
        (*sort_func)(keys, num_keys, sorted_keys, universe_bits);
        end = clock();

        for (i = 0; i < num_keys-1; i++)
            assert(sorted_keys[i] < sorted_keys[i+1]);

        elapsed += ((double)end - start) / CLOCKS_PER_SEC;
    }

    free(keys); free(sorted_keys);
    return elapsed / test_runs * 1000;
}

void run_suite(const char* label, keygen_fn_t gen, size_t num_keys,
               uint8_t universe_bits, size_t test_runs)
{
    printf("=== %s: %zu keys, universe_bits=%u ===\n",
           label, num_keys, (unsigned)universe_bits);
    printf("  Veb sorting (successor)     took %lf ms\n",
           benchmark_sort_algo_in_ms(&sort_veb_succ, gen, num_keys, universe_bits, test_runs));
    printf("  Veb sorting (predecessor)   took %lf ms\n",
           benchmark_sort_algo_in_ms(&sort_veb_pred, gen, num_keys, universe_bits, test_runs));
    printf("  Radix64 sorting (successor) took %lf ms\n",
           benchmark_sort_algo_in_ms(&sort_radix_succ, gen, num_keys, universe_bits, test_runs));
    printf("  Radix64 sorting (predecessor) took %lf ms\n",
           benchmark_sort_algo_in_ms(&sort_radix_pred, gen, num_keys, universe_bits, test_runs));
    printf("  Quicksort                   took %lf ms\n",
           benchmark_sort_algo_in_ms(&quick_sort, gen, num_keys, universe_bits, test_runs));
}

int main(int argc, char** argv)
{
    (void)argc; (void)argv;
    run_suite("DENSE", &gen_dense, 500000, 19, 100);
    run_suite("SPARSE", &gen_sparse, 100000, SPARSE_UNIVERSE_BITS, 100);
    return 0;
}
