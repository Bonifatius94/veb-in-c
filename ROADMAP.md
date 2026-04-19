# Roadmap

Work streams for `veb-in-c`, grouped by theme and priority. GitHub issues at <https://github.com/Bonifatius94/veb-in-c/issues> are the source of truth; this doc gives context, ordering, and the reasoning behind it.

## Vision

A single-header, dependency-free vEB tree that you can drop into any C project and use for dense-integer ordered-set workloads — even over large (up to 64-bit) universes, without needing a supercomputer's worth of RAM. The core algorithm already works; the memory story is what's holding the library back from production use.

## 1. Memory efficiency (headline)

The main thing keeping this library from real-world adoption on large universes. Fixing this unlocks the 64-bit key space promised by the type.

- **Lazy tree allocation ([#3])** — allocate clusters/summary on first touch rather than up front. `VEBTREE_FLAG_LAZY` is already reserved in the header but unimplemented. Requires an explicit membership check before every op, since uninitialized malloc bytes can't be trusted. **Top priority.**
- **Sparse-universe hashmaps ([#19])** — back the global summary with a hashmap until it grows past a threshold, then promote to a real vEB summary. Composes with #3; the two together give O(n · log log u) space in practice.
- **Malloc chunk limit ([#20])** — current allocator tops out around ~15 MB per chunk, capping dense universes at ~2^19 keys. Becomes easier to fix once lazy allocation is in, since cluster arrays are the big offenders.

## 2. Correctness / API completeness

- **Predecessor operation ([#15])** — `vebtree_predecessor` is stubbed with `assert(false)`. The bitwise-leaf version already exists; mirror the successor logic for internal nodes. Small, unblocks validation of the sorting benchmark in reverse.
- **Duplicate-key / missing-key safety ([#18])** — guard `insert_key`/`delete_key` with an upfront `contains_key` check. Small, removes a footgun.

## 3. Performance

- **Bigger leaves via SIMD ([#17])** — leaves cap at 64 bits today (a single `uint64_t` bitboard, bounded by ALU width). Move to 256-bit or 512-bit leaves via SSE/AVX intrinsics. Orthogonal perf win that reduces tree depth by 2–3× for common universes.

## 4. Concurrency

- **Concurrent operations ([#22])** — parallel vEB ops are UB today. Two paths on the table: CQRS-style (parallel reads, serialized writes via an op queue) or Kulakowski's lock-free approach ([arXiv:1509.06948](https://arxiv.org/pdf/1509.06948.pdf)). Structural change, lands last.

## 5. Testing + benchmarks

- **Edge-case coverage ([#5])** — current unit tests hit one universe size (u = 4096). Extend to cover odd universe bits, duplicate/missing-key behavior, single-element trees, near-maxkey values, and every code path at least once.
- **Dijkstra pathfinding benchmark ([#8])** — vEB as a priority queue over a Barabási–Albert graph, compared to a standard heap. Validates the library on a realistic workload beyond pure sorting.

## Suggested execution order

The ordering is chosen so each item either unblocks the next or locks in behavior before a disruptive change:

1. **[#15]** predecessor — small, and the sorting benchmark can then be adapted to validate it.
2. **[#18]** dup/missing-key guards — small, removes a class of UB before we start touching allocation.
3. **[#5]** edge-case tests — lock in current behavior before we rewrite the allocator.
4. **[#3]** lazy allocation — the big one.
5. **[#19]** hashmap sparse summary — builds on #3.
6. **[#20]** malloc chunking — largely obviated by #3 + #19, but some cases remain.
7. **[#17]** SIMD leaves — independent perf work, can happen any time after #5.
8. **[#8]** Dijkstra benchmark — exercises everything in a realistic workload.
9. **[#22]** concurrency — last; requires the memory story to be settled first.

## Non-goals

- Becoming a general-purpose ordered-map library. vEB's win comes from dense integer keys; sparse keys or strings belong in a B-tree or hash table.
- External dependencies. The contract is single-header + stdlib. No CMake-fetched third-party libraries, no header-only submodules.
- Breaking the C89 style. Portability across legacy toolchains is a feature.

[#3]: https://github.com/Bonifatius94/veb-in-c/issues/3
[#5]: https://github.com/Bonifatius94/veb-in-c/issues/5
[#8]: https://github.com/Bonifatius94/veb-in-c/issues/8
[#15]: https://github.com/Bonifatius94/veb-in-c/issues/15
[#17]: https://github.com/Bonifatius94/veb-in-c/issues/17
[#18]: https://github.com/Bonifatius94/veb-in-c/issues/18
[#19]: https://github.com/Bonifatius94/veb-in-c/issues/19
[#20]: https://github.com/Bonifatius94/veb-in-c/issues/20
[#22]: https://github.com/Bonifatius94/veb-in-c/issues/22
