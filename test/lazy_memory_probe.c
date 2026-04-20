#include <stdio.h>
#include <stdlib.h>
#include <windows.h>
#include <psapi.h>
#include "vebtrees.h"

static SIZE_T rss_kb(void)
{
    PROCESS_MEMORY_COUNTERS pmc;
    GetProcessMemoryInfo(GetCurrentProcess(), &pmc, sizeof(pmc));
    return pmc.WorkingSetSize / 1024;
}

static void lazy_init_only(uint8_t u)
{
    VebTree* tree;
    SIZE_T before, after;

    before = rss_kb();
    vebtree_init(&tree, u, VEBTREE_FLAG_LAZY);
    after = rss_kb();
    printf("  lazy u=%2u:      RSS %5zu -> %5zu KB  (delta %+zd KB)  global=%p locals=%p\n",
           (unsigned)u, (size_t)before, (size_t)after, (ptrdiff_t)after - (ptrdiff_t)before,
           (void*)tree->global, (void*)tree->locals);
    vebtree_free(tree);
    free(tree);
}

static void nonlazy_init_only(uint8_t u)
{
    VebTree* tree;
    SIZE_T before, after;

    before = rss_kb();
    vebtree_init(&tree, u, 0);
    after = rss_kb();
    printf("  non-lazy u=%2u:  RSS %5zu -> %5zu KB  (delta %+zd KB)\n",
           (unsigned)u, (size_t)before, (size_t)after, (ptrdiff_t)after - (ptrdiff_t)before);
    vebtree_free(tree);
    free(tree);
}

int main(void)
{
    printf("Baseline RSS: %zu KB\n", (size_t)rss_kb());

    /* Lazy init should stay ~constant regardless of u */
    lazy_init_only(24);
    lazy_init_only(28);
    lazy_init_only(32);
    lazy_init_only(40);
    lazy_init_only(64);

    /* Control: non-lazy at u=24 (safe at ~12.5 MB) */
    nonlazy_init_only(24);

    printf("Final RSS:    %zu KB\n", (size_t)rss_kb());
    return 0;
}
