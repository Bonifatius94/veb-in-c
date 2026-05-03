
# Van Emde Boas Tree Data Structure

## About
This project provides an implementation of the van-Emde-Boas tree data structure in C89 style.
Benchmarks show that it's really efficient, e.g. sorting is ~ 10x faster than stdlib.h qsort().

It's a small, standalone, single-header implementation, not relying on any external dependencies,
highly portable to any operating system or processor architecture of choice.

A second header [`include/radix64.h`](./include/radix64.h) ships a uniform 64-way radix trie
with the same public API shape (`radix64_init`, `radix64_insert_key`, `radix64_successor`, …).
It trades vEB's O(log log u) asymptotic for a shorter, cache-friendlier O(⌈u/6⌉) path and
**strictly lazy allocation** — useful as a baseline on dense workloads and as a drop-in when
the vEB memory cliff bites at large universes. Both implementations are exercised side by
side in `SortingBenchmark`.

## Hardware Requirements

The default benchmarks and tests are sized to be safe on any modern laptop, but the vEB
implementation has a known memory cliff that's easy to trip when experimenting at large
universes. Read this before raising `universe_bits`.

| Workload                                      | Peak RAM (vEB) | Peak RAM (radix64) | Safe on |
|-----------------------------------------------|----------------|--------------------|---------|
| `UnitTests` / `RadixUnitTests`                | < 50 MB        | < 10 MB            | anything |
| `SortingBenchmark` dense (500k keys, u = 19)  | ~ 1 MB         | ~ 50 MB            | anything |
| `SortingBenchmark` sparse (100k keys, u = 24) | ~ 12 MB        | ~ 30 MB            | anything |
| Custom: vEB at u = 28                         | ~ 192 MB       | scales with `n`    | 4 GB+   |
| Custom: vEB at u = 30                         | ~ 768 MB       | scales with `n`    | 8 GB+   |
| Custom: vEB at u = 32                         | ~ 3 GB         | scales with `n`    | 16 GB+ — **single malloc may fail; see [#20]** |

**Why the vEB numbers blow up.** With the current allocation strategy, the second distinct
key inserted into a vEB tree of universe `u` triggers a single `malloc` of `2^(u-6) × 48 B`
for the cluster array — driven by the universe size, not the number of keys actually stored.
That doubles with every extra universe bit: 12 MB at u = 24, 192 MB at u = 28, 3 GB at u = 32.
Lazy/sparse allocation is the headline unfinished work — see [`ROADMAP.md`](./ROADMAP.md) §1
and issues [#19] / [#20].

**radix64 has no such cliff.** Allocation is strictly lazy and proportional to the number
of distinct keys inserted (roughly `n × ⌈u/6⌉ × 528 B` worst case, freed back on delete).
If you want to experiment near `u = 32`, prefer `radix64` — it'll comfortably handle a few
million sparse keys in a 32-bit universe on a laptop.

**Rules of thumb.**
- Stay at `u ≤ 24` for vEB unless you've checked the math above against your free RAM.
- Don't run `vebtree_init(u = 32, …)` followed by a second insert on an 8 GB machine. It will
  either OOM or hit the per-chunk malloc cap and crash.
- The shipped tests and benchmarks never exceed ~ 50 MB resident — they're safe out of the box.

[#19]: https://github.com/Bonifatius94/veb-in-c/issues/19
[#20]: https://github.com/Bonifatius94/veb-in-c/issues/20

## Build Toolchain Setup
First, you need to install a minimalistic C compiler toolchain for building the source code.
Additionally, there are a few more packages to generate the Doxygen website (optional).

```sh
sudo apt-get update

# core packages for building the code / running tests
sudo apt-get install -y build-essential cmake git

# additional packages for docs generation (optional)
sudo apt-get install -y doxygen graphviz zip unzip
```

*Note: All commands are tested on Ubuntu 20.04, but should work on any Debian-like system.*

## Build + Run Tests
Clone the source code from this repository (if you haven't done already).

```sh
git clone https://github.com/Bonifatius94/veb-in-c
cd veb-in-c
```

### Git Hooks (one-time)

This repo ships a `commit-msg` hook under `.githooks/` that rejects commits
containing `Co-Authored-By` trailers (commits attribute work to the human
committer only). Activate it once per clone:

```sh
git config core.hooksPath .githooks
```

No global git config is touched — the setting is local to this clone.

### Build

Run the build script. This will execute a bunch of property tests and
benchmarks to ensure that the van Emde Boas tree is working as expected.

```sh
./build.sh
```

## Benchmark
The `SortingBenchmark` target compares three sorters on two workloads:

- **Dense**: 500k keys, universe `u = 19` (a permutation of `[0, 500k)`).
- **Sparse**: 100k distinct keys drawn from a `u = 24` universe (~ 0.6% density)
  via a coprime multiplicative hash.

For each workload the harness builds up the tree, then walks it via successor (and
also predecessor for vEB) until the largest key is reached. Tree construction is
included in the timing for fairness against `qsort`.

```sh
build/test/SortingBenchmark
```

Results show that sorting dense indices can be carried out very efficiently with vEB
trees — roughly a 10× speedup over `qsort` — and that the radix64 trie sits between
the two on dense workloads while remaining the safe choice when the universe is large
relative to RAM.

```text
Sorting Benchmark (500k keys, u = 19)
=====================================
vEB    sorting took  5.80 ms
radix64 sorting took 12.40 ms
qsort                59.82 ms
```

> Numbers are illustrative — actual values vary by machine. Run `/bench` (or
> `build/test/SortingBenchmark`) locally to get your own.

## Doxygen Documentation
If you like to generate the documentation website, run the gen-docs script.

```sh
./gen-docs.sh
```

For viewing the website, unpack the generated zip archive and open the index.html
file inside a web browser of choice (e.g. Firefox).

```sh
unzip vebtree-docs-html.zip
cd vebtree-docs-html/html
firefox index.html
```

## Deployment into Projects
Just copy the [vebtrees.h](./include/vebtrees.h) file (or [radix64.h](./include/radix64.h),
or both) into your project's include directory. As already mentioned, the code is standalone,
single-header, pure C89, no crazy dependencies.

Pick the implementation by workload:
- **`vebtrees.h`** — best raw throughput on dense integer keys with `u ≤ 24`.
- **`radix64.h`** — strictly lazy memory; safe choice when the universe is large
  (`u` near 32) or sparsely populated.

## License
This project is available under the terms of the MIT license.
