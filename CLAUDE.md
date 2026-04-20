# Claude Agent Guide — veb-in-c

Terse guide for Claude sessions working in this repo. Human contributors: see `README.md` (usage) and `ROADMAP.md` (direction).

## Ground rules for agents

- **Do not write to the hidden per-project memory directory** (`~/.claude/projects/.../memory/`). Everything that needs to persist goes into this repo so it's versioned, reviewable, and visible to collaborators.
- **Skills live in this repo** under `.claude/skills/<name>/SKILL.md` — commit them like any other source file.
- **Plans are the issue body, not comments.** Issues exist in two states: a raw draft of the idea, and a refined plan detailed enough for an agent to execute autonomously. When you refine (e.g. via `/refine`), draft the plan in `./plans/<N>.md` for the user to review on disk, then replace the issue body with the same file via `gh issue edit <N> --body-file ./plans/<N>.md`. The `plans/` directory is gitignored — it's a local review workspace, not versioned state, and the ticket remains the canonical home of the plan. Never stash plans in hidden memory or anywhere outside `plans/`. If there's no suitable issue, create one first with `gh issue create`.
- Short-lived working notes in the conversation are fine; anything worth keeping gets checked in, posted to a ticket, or (for plans under review) dropped into `./plans/`.
- **No `Co-Authored-By` trailers in commit messages.** Commits attribute work to the human committer only. A `commit-msg` hook in `.githooks/` enforces this — do not bypass with `--no-verify`. Activate the hook locally once per clone with `git config core.hooksPath .githooks`.
- **Don't pass `-C <path>` to git.** The session's `pwd` is already the repo root. Plain `git status`, `git add`, `git commit` etc. work. `git -C <abs>` is overcautious noise — skip it.

## Purpose

Single-header C89 implementation of a van Emde Boas tree. Goals, in order:

1. Zero dependencies. One header drop-in (`include/vebtrees.h`).
2. Portable across compilers and architectures (GCC, Clang, MSVC; bit-scan intrinsics with a pure-C fallback).
3. Competitive performance (~10× faster than `qsort` on dense-key sorting).
4. **Lazy allocation** so large universes (up to 64-bit) are usable without exhausting RAM. This is the headline unfinished work — see `ROADMAP.md` §1.

Not a general-purpose ordered map. vEB wins on dense integer keys; sparse or string keys belong elsewhere.

## Codebase layout

```
include/vebtrees.h          the whole library (public API + impl)
test/unit_tests.c           property tests across u = 6, 7, 12, 13, 24 (plus leaf-level)
test/sorting_benchmark.c    vEB-sort vs qsort on 500k dense keys
test/lazy_memory_probe.c    Windows-only RSS probe for lazy init (psapi)
test/CMakeLists.txt         targets: UnitTests, SortingBenchmark, LazyMemoryProbe (WIN32-gated)
CMakeLists.txt              registers UnitTests + SortingBenchmark with CTest
build.sh                    clean + configure + build + ctest (what CI runs)
gen-docs.sh                 Doxygen → vebtree-docs-html.zip
.github/workflows/cicd.yaml Build+Test on push/PR (Ubuntu, gcc)
```

## Architecture

### The data structure (recap)

vEB over universe `u = 2^k` stores `min` and `max` at the node directly (min is **not** in any subtree — this is the invariant that gives O(log log u) amortized). The node recurses into one size-√u `global` summary and √u size-√u `locals` clusters. Standard textbook space is O(u); lazy/hash variants bring this to O(n · log log u).

### How this implementation diverges

- **One `VebTree` struct for both leaves and internal nodes.** Distinguished by `universe_bits <= VEBTREE_LEAF_BITS` (6). Leaves repurpose the `low` field as a 64-bit bitboard and leave `global`/`locals` NULL.
- **Memory-efficient root** (`is_memeff_root` in `_vebtree_init`): at the root, `lower_bits` is pinned to `VEBTREE_LEAF_BITS = 6`, so the root's locals are always leaves. This avoids the naive √u split blowing up memory at the top. Non-root nodes use `vebtree_lower_bits(u) = u / 2`. Derivation in §Memory model; closed as [#2].
- **Bit-scan ops have three backends**: GCC `__builtin_clzll`/`_ctzll`, MSVC `_BitScan{Reverse,Forward}64`, and a portable C fallback for unknown compilers.
- **Bitwise leaves up to 6 universe bits.** `VEBTREE_LEAF_BITS = 6` is tied to `sizeof(bitboard_t) * 8 = 64`. If you change one, change both. Closed as [#4]; [#17] tracks widening leaves via SIMD.

### Memory model

Two optimizations work together to keep the struct footprint sane on small-to-mid universes. Neither is a textbook vEB — both come from [#2] and [#4].

**1. Memeff root (`is_memeff_root` branch at `_vebtree_init`, vebtrees.h:368).** Textbook vEB splits universe `u = 2^k` into √u clusters of √u each — so at the root, `lower_bits = k/2` and there are `2^(k/2)` cluster slots. That's catastrophic at large `k`: `u=32` gives `2^16` slots whose *cluster structs* are each themselves non-trivial vEB nodes, recurring a handful of levels. Total bits: `O(u log log u)`.

Trick from [#2]: at the root only, pin `lower_bits` to `log u` (in our code: the constant `VEBTREE_LEAF_BITS = 6`). Then:
- Each local holds ≤ `2^6 = 64` keys — representable as a single bitboard.
- Number of locals: `2^(k - 6)` — one flat layer, no recursion *inside* clusters.
- Global summary is a vEB over universe `2^(k - 6)`, recursing normally (√u split) from there.

Storage, per [#2]:
```
  locals: 2^(k-6) structs × 48 B each
  global: O(u / log u × log(u / log u)) bits    = O(u) bits
```
The `O(u) bits` bound holds when `lower_bits ≈ log u`. We freeze it at 6 for ALU reasons (see below), so for `u ≥ 32` the `locals` array still explodes — that's the cliff #3's lazy allocation and #19's sparse allocation exist to solve. The root trick shifts the cliff from "impossible at u > ~16" to "impossible at u > ~30", which is the difference between "toy" and "usable on moderate universes."

**2. Bitwise leaves at `u ≤ 6` (vebtrees.h:238–280).** Issue [#2]'s original suggestion was to use binary search trees as the local structure (where each local manages `O(log u)` keys). [#4] replaces that with a 64-bit bitboard plus bit-scan intrinsics (`__builtin_ctzll` / `__builtin_clzll` / MSVC equivalents). All leaf ops — `contains`, `insert`, `delete`, `min`, `max`, `successor`, `predecessor` — become O(1) ALU ops, beating a BST's `O(log log u)` on every dimension: speed, branch-predictability, allocation count (zero per leaf).

Critically: since the memeff root already pins local-size to 6 bits = 64 keys, **every locals slot fits in one bitboard**. The two tricks are mutually reinforcing: memeff root caps cluster size at the ALU word width exactly because bitwise leaves efficiently fill that width. Change either and you need to rethink the other. [#17] proposes widening leaves to 256/512 bits via SIMD — would let memeff root expand to `lower_bits = 8` or `9`, raising the cliff by 2–3 bits of universe without touching sparsity.

**Why this matters for ongoing work on [#3] / [#19].**

- `tree->locals[i]` is always a leaf at the memeff root. That means *sparse cluster storage* (#19's job) only has to map `cluster_idx → one 48 B leaf`, not a recursively-allocated subtree. No deep recursion inside a cluster ever happens at the root layer. This is a huge simplification for the sparse-allocation design.
- At non-memeff-root layers (the recursion inside `tree->global`), `lower_bits = upper_bits/2`, so clusters there *are* internal nodes. Any sparse design has to handle both — but the leaf-cluster case is the hot path (it's where every key ultimately lives).

[#2]: https://github.com/Bonifatius94/veb-in-c/issues/2
[#4]: https://github.com/Bonifatius94/veb-in-c/issues/4
[#17]: https://github.com/Bonifatius94/veb-in-c/issues/17

## Design patterns

- **Single-header, `DOXYGEN_SKIP` gate.** Public API sits above the gate and is the Doxygen surface. Everything below is impl — still inlined into callers, just hidden from docs.
- **Macro-based inline helpers** (`vebtree_is_leaf`, `vebtree_local_address`, `vebtree_new_empty_node`, …) for zero-overhead abstraction. There's a `TODO: remove those makros` in the header suggesting a future inline-at-call-site refactor — don't do that without discussing.
- **Explicit recursion anchor / recursion case comments** on every recursive function. Match this convention when extending.
- **Leading underscore = private helper** (`_vebtree_init`, `_init_subtrees`). Public names are `vebtree_<verb>_<noun>`. Macros are `SCREAMING_SNAKE`.

## Coding standards

- **C89.** No `//` comments. No mixed declarations and statements — declare all variables at the top of each block. See existing functions for the pattern.
- **Doxygen `/** … */` blocks on the public API only.** Internal comments are sparse `/* … */` and explain *why* (e.g. "recursion anchor", "base case when tree is empty"), not *what*.
- **Assertions for preconditions**, always with a message: `assert(key != vebtree_null && "cannot insert vebtree_null, invalid key!");`.
- 4-space indent. K&R-ish braces (opening brace on same line for control flow, next line for functions — follow what's there).
- No new external dependencies. Stdlib only: `<stdlib.h>`, `<stdint.h>`, `<stdbool.h>`, `<assert.h>`.

## Invariants (do not break)

1. `low` of an internal node is never present in any subtree. On insert, if the new key `< tree->low`, swap and insert the old `low` instead.
2. `tree->high` equals the max key, or `vebtree_null` when the tree is empty.
3. Empty-check: internal node ⇔ `low == vebtree_null`; leaf ⇔ `low == 0` (bitboard).
4. When a local subtree becomes empty on delete, the corresponding global key must also be deleted (keeps the summary in sync).
5. `VEBTREE_LEAF_BITS` must match `sizeof(bitboard_t) * 8`'s log₂.
6. `vebtree_null = 0xFFFF...FF` is a sentinel — inserting it is forbidden and asserted.

## Known gotchas / half-finished work

- `VEBTREE_FLAG_LAZY` works for init-only and single-key trees at any `u ≤ 64` (O(1) RAM). The 2nd-distinct-key insert still triggers `_init_subtrees` which mallocs `2^(u-6) × 48 B` — fine up to `u ≈ 30`, guaranteed OOM beyond. That's the cliff [#19] is aimed at.
- `vebtree_bitwise_leaf_successor` / `_predecessor` return `vebtree_null` when the answer would be bit 0, conflating "bit 0 is the answer" with "nothing found". Re-check before extending or reusing.
- `malloc` caps at ~15 MB in one chunk on some platforms, which limits universes to ≈ 2^19 keys ([#20]). Partly obviated once sparse cluster allocation lands.

[#19]: https://github.com/Bonifatius94/veb-in-c/issues/19
[#20]: https://github.com/Bonifatius94/veb-in-c/issues/20

## Build + test loops

- `./build.sh` — clean build + CTest. This is what CI runs. Also available as `/build`.
- `build/test/SortingBenchmark` — perf numbers. Also available as `/bench`.
- `./gen-docs.sh` → `vebtree-docs-html.zip`. Also available as `/docs`.
- Manual: `cmake -B build -DCMAKE_BUILD_TYPE=Release && cmake --build build && ctest --test-dir build --output-on-failure`.

## Workflow for picking up work

1. Open `ROADMAP.md` for ordering and context.
2. Pick an issue number from GitHub.
3. If the issue is still a raw draft, run `/refine <N>` to promote it to a refined plan (the plan replaces the issue body).
4. Run `/implement <N>` to branch, code, test, and commit against the refined plan.
5. Iterate with `/build` / `/bench` during implementation as needed.
6. When stopping mid-task, run `/next` to emit a handoff prompt for the next session.
