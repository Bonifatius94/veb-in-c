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
4. **Lazy allocation** so large universes (up to 32-bit, the IPv4 address space) are usable without exhausting RAM. This is the headline unfinished work — see `ROADMAP.md` §1.

Not a general-purpose ordered map. vEB wins on dense integer keys; sparse or string keys belong elsewhere.

## Codebase layout

```
include/vebtrees.h          vEB tree (public API + impl, u ≤ 32)
include/radix64.h           uniform 64-way radix trie, API-compatible peer (u ≤ 32)
test/unit_tests.c           vEB property tests across u = 6, 7, 12, 13, 24 (plus leaf-level)
test/radix_unit_tests.c     radix64 property tests across u = 1, 6, 16, 19, 24, 32
test/sorting_benchmark.c    vEB-sort + radix64-sort vs qsort on 500k dense keys
test/lazy_memory_probe.c    Windows-only RSS probe for lazy init (psapi)
test/CMakeLists.txt         targets: UnitTests, RadixUnitTests, SortingBenchmark, LazyMemoryProbe (WIN32-gated)
CMakeLists.txt              registers UnitTests + RadixUnitTests + SortingBenchmark with CTest
build.sh                    clean + configure + build + ctest (what CI runs)
gen-docs.sh                 Doxygen → vebtree-docs-html.zip
.github/workflows/cicd.yaml Build+Test on push/PR (Ubuntu, gcc)
```

## Architecture

### The data structure (recap)

vEB over universe `u = 2^k` stores `min` and `max` at the node directly (min is **not** in any subtree — this is the invariant that gives O(log log u) amortized). The node recurses into one size-√u `global` summary and √u size-√u `locals` clusters. Standard textbook space is O(u); lazy/hash variants bring this to O(n · log log u).

### How this implementation diverges

- **One `VebTree` struct for both leaves and internal nodes.** Distinguished by `universe_bits <= VEBTREE_LEAF_BITS` (6). Leaves repurpose the `low` field as a 64-bit bitboard and leave `global`/`locals` NULL.
- **Memory-efficient root** (`is_memeff_root` in `_vebtree_init`): at the root, `lower_bits` is pinned to `VEBTREE_LEAF_BITS = 6`, so the root's locals are always leaves. This avoids the naive √u split blowing up memory at the top. Non-root nodes use `vebtree_lower_bits(u) = u / 2`. Derivation and rationale: run `/theory`.
- **Bit-scan ops have three backends**: GCC `__builtin_clzll`/`_ctzll`, MSVC `_BitScan{Reverse,Forward}64`, and a portable C fallback for unknown compilers.
- **Bitwise leaves up to 6 universe bits.** `VEBTREE_LEAF_BITS = 6` is tied to `sizeof(bitboard_t) * 8 = 64`. If you change one, change both.

### radix64 (peer implementation)

`include/radix64.h` is a separate single-header library with the same public-API shape (`radix64_init`, `radix64_insert_key`, `radix64_successor`, …). It stores keys in a uniform 64-way radix trie of depth `⌈universe_bits / 6⌉`, with a `uint64_t` bitmap at every node summarizing which slots are populated. Nibble extraction is `(key >> ((depth-1-level) * 6)) & 0x3F`; the top level gets the leftover `universe_bits - (depth-1)*6` bits and the rest are full 6-bit slices. Allocation is strictly lazy: `tree->root = NULL` until the first insert, nodes are `calloc`'d on descent, and nodes with bitmap == 0 are freed on the delete ascent. `R64_MAX_DEPTH = 8` sizes the path stacks (real max depth at `u = 32` is 6). Successor/predecessor descend recording the path, then ascend checking for a sibling bit `> nibble` (or `<` for predecessor) in each level's bitmap, then descend leftmost/rightmost via `ctz`/`clz`.

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

## Skills

Skills live in `.claude/skills/<name>/SKILL.md`. Invoke with `/<name>`. An agent should activate the matching skill proactively when the trigger condition is met — don't wait for the user to type the slash command.

| Skill        | When to activate                                                                                                                                                                  |
|--------------|-----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| `/build`     | After any edit to `include/vebtrees.h` or anything under `test/`, or whenever you need to confirm the tree is green. Clean build + CTest.                                          |
| `/bench`     | After a perf-relevant change to `vebtrees.h` (insert/delete/successor/predecessor hot paths), or when the user asks for a timing number. Runs `SortingBenchmark`.                  |
| `/docs`      | After a change to the public API or Doxygen comments in `vebtrees.h`, or when the user asks to regenerate the docs website. Produces `vebtree-docs-html.zip`.                       |
| `/refine`    | When the user hands you a GitHub issue number that's still a raw draft. Produces a refined plan in `./plans/<N>.md` and replaces the issue body with it. **Does not write code.**  |
| `/implement` | When the user hands you a GitHub issue number whose body is a refined plan. Executes strict red-green-refactor TDD, one commit per phase, on main.                                 |
| `/theory`    | When making decisions that depend on the memory model (memeff root, bitwise leaves, sparsity, leaf sizing) or when the user asks why `lower_bits` is pinned at 6, why the cliff is where it is, etc. Explanatory — does not write code. |
| `/next`      | At the end of a work session, especially when stopping mid-task. Produces a self-contained handoff prompt for the next Claude session.                                             |

Manual alternatives to the automation skills:
- `./build.sh` — what `/build` wraps; also what CI runs.
- `build/test/SortingBenchmark` — what `/bench` wraps.
- `./gen-docs.sh` — what `/docs` wraps.
- `cmake -B build -DCMAKE_BUILD_TYPE=Release && cmake --build build && ctest --test-dir build --output-on-failure` — bare equivalent.

## Workflow for picking up work

1. Open `ROADMAP.md` for ordering and context.
2. Pick an issue number from GitHub.
3. If the issue is still a raw draft, run `/refine <N>` to promote it to a refined plan (the plan replaces the issue body).
4. Run `/implement <N>` to branch, code, test, and commit against the refined plan.
5. Iterate with `/build` / `/bench` during implementation as needed.
6. When stopping mid-task, run `/next` to emit a handoff prompt for the next session.
