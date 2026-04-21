---
name: theory
description: Theoretical background for veb-in-c's design choices — memeff root, bitwise leaves, the O(u)-bits derivation from issue #2, and how they interact. Use when designing around memory allocation, answering "why is lower_bits pinned at 6 at the root", or making decisions about #19 / #17 that depend on understanding the interlock between the two tricks.
---

# Theory — memory model

Two optimizations work together to keep the struct footprint sane on small-to-mid universes. Neither is a textbook vEB — both come from [#2] and [#4].

**Scope:** `vebtree_init` is publicly capped at `universe_bits ≤ 32` (IPv4 address space). The recursion and bit-math below still reason in terms of arbitrary `k`, but no realistic `k > 32` use case is supported.

## 1. Memeff root

**Where:** `is_memeff_root` branch at `_vebtree_init`, vebtrees.h:368.

Textbook vEB splits universe `u = 2^k` into √u clusters of √u each — so at the root, `lower_bits = k/2` and there are `2^(k/2)` cluster slots. That's catastrophic at large `k`: `u=32` gives `2^16` slots whose *cluster structs* are each themselves non-trivial vEB nodes, recurring a handful of levels. Total bits: `O(u log log u)`.

**Trick from [#2]:** at the root only, pin `lower_bits` to `log u` (in our code: the constant `VEBTREE_LEAF_BITS = 6`). Then:

- Each local holds ≤ `2^6 = 64` keys — representable as a single bitboard.
- Number of locals: `2^(k - 6)` — one flat layer, no recursion *inside* clusters.
- Global summary is a vEB over universe `2^(k - 6)`, recursing normally (√u split) from there.

Storage, per [#2]:

```
  locals: 2^(k-6) structs × 48 B each
  global: O(u / log u × log(u / log u)) bits    = O(u) bits
```

The `O(u) bits` bound holds when `lower_bits ≈ log u`. We freeze it at 6 for ALU reasons (see §2), so for `u ≥ 32` the `locals` array still explodes — that's the cliff #3's lazy allocation and #19's sparse allocation exist to solve. The root trick shifts the cliff from "impossible at u > ~16" to "impossible at u > ~30", which is the difference between "toy" and "usable on moderate universes."

## 2. Bitwise leaves at u ≤ 6

**Where:** vebtrees.h:238–280.

Issue [#2]'s original suggestion was to use binary search trees as the local structure (where each local manages `O(log u)` keys). [#4] replaces that with a 64-bit bitboard plus bit-scan intrinsics (`__builtin_ctzll` / `__builtin_clzll` / MSVC equivalents). All leaf ops — `contains`, `insert`, `delete`, `min`, `max`, `successor`, `predecessor` — become O(1) ALU ops, beating a BST's `O(log log u)` on every dimension: speed, branch-predictability, allocation count (zero per leaf).

## 3. Why they interlock

Since the memeff root already pins local-size to 6 bits = 64 keys, **every locals slot fits in one bitboard**. The two tricks are mutually reinforcing: memeff root caps cluster size at the ALU word width exactly because bitwise leaves efficiently fill that width. Change either and you need to rethink the other. [#17] proposes widening leaves to 256/512 bits via SIMD — would let memeff root expand to `lower_bits = 8` or `9`, raising the cliff by 2–3 bits of universe without touching sparsity.

## Implications for #3 (lazy) and #19 (sparse)

- `tree->locals[i]` is always a leaf at the memeff root. That means *sparse cluster storage* (#19's job) only has to map `cluster_idx → one 48 B leaf`, not a recursively-allocated subtree. No deep recursion inside a cluster ever happens at the root layer. This is a huge simplification for the sparse-allocation design.
- At non-memeff-root layers (the recursion inside `tree->global`), `lower_bits = upper_bits/2`, so clusters there *are* internal nodes. Any sparse design has to handle both — but the leaf-cluster case is the hot path (it's where every key ultimately lives).
- Lazy ([#3]) alone doesn't break the cliff because `_ensure_subtrees` still allocates the full `2^(k-6)` locals array on the first second-key insert. Lazy helps with "never inserted" and "one-key" cases only. Full cliff removal needs sparse locals ([#19]).

[#2]: https://github.com/Bonifatius94/veb-in-c/issues/2
[#3]: https://github.com/Bonifatius94/veb-in-c/issues/3
[#4]: https://github.com/Bonifatius94/veb-in-c/issues/4
[#17]: https://github.com/Bonifatius94/veb-in-c/issues/17
[#19]: https://github.com/Bonifatius94/veb-in-c/issues/19
