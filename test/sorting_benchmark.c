#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <assert.h>
#include "vebtrees.h"

int sort_veb(const VebKey keys[], size_t num_keys, VebKey output[])
{
    size_t i; VebTree* tree;

    vebtree_init(&tree, 16, false);

    for (i = 0; i < num_keys; i++)
        vebtree_insert_key(tree, keys[i]);

    output[0] = vebtree_get_min(tree);
    for (i = 1; i < num_keys; i++)
        output[i] = vebtree_successor(tree, output[i-1]);

    vebtree_free(tree);
}

void linear_shuffle(VebKey keys[], size_t num_keys)
{
    size_t i, j; VebKey temp;

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
    size_t i, num_keys = 65536;
    VebKey keys[num_keys], sorted_keys[num_keys];

    srand(42);
    for (i = 0; i < num_keys; i++)
        keys[i] = i;
    linear_shuffle(keys, num_keys);

    sort_veb(keys, num_keys, sorted_keys);
    for (i = 0; i < num_keys-1; i++)
        assert(sorted_keys[i] < sorted_keys[i+1]);

    return 0;
}
