
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

**Why vEB has a cliff.** The second distinct key inserted into a vEB tree of universe
`u` triggers a single `malloc` of `2^(u-6) × 48 B` for the cluster array — driven by the
universe size, not the number of keys actually stored. The table below is computed
directly from that formula (it is the dominant allocation, not measured RSS):

| `universe_bits` | vEB cluster malloc (`2^(u-6) × 48 B`) | Notes |
|-----------------|----------------------------------------|-------|
| 19 (dense default benchmark)  | 384 KB   | trivial |
| 24 (sparse default benchmark) | 12 MB    | trivial |
| 28                            | 192 MB   | needs ≥ 4 GB free |
| 30                            | 768 MB   | needs ≥ 8 GB free |
| 32                            | 3 GB     | **single malloc may fail; see [#20]** |

Lazy/sparse allocation is the headline unfinished work — see [`ROADMAP.md`](./ROADMAP.md) §1
and issues [#19] / [#20].

**radix64 has no such cliff.** Allocation is strictly lazy and proportional to the number
of distinct keys actually inserted; nodes are `calloc`'d on descent and freed back on the
delete ascent when their bitmap empties. If you want to experiment near `u = 32`, prefer
`radix64`.

**Rules of thumb.**
- Stay at `u ≤ 24` for vEB unless you've checked the math above against your free RAM.
- Don't run `vebtree_init(u = 32, …)` followed by a second insert on a low-RAM machine. It
  will either OOM or hit the per-chunk malloc cap and crash.
- The shipped tests and benchmarks stay in the "trivial" rows above — they're safe out of
  the box.

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

Results show that sorting dense indices can be carried out very efficiently with both
vEB and radix64 — roughly a 10× speedup over `qsort` on the dense workload. On the
sparse workload radix64 takes the lead while vEB and `qsort` end up close together.

```text
=== DENSE: 500000 keys, universe_bits=19 ===
  Veb sorting (successor)       took 4.140000 ms
  Veb sorting (predecessor)     took 4.330000 ms
  Radix64 sorting (successor)   took 4.140000 ms
  Radix64 sorting (predecessor) took 4.320000 ms
  Quicksort                     took 39.060000 ms
=== SPARSE: 100000 keys, universe_bits=24 ===
  Veb sorting (successor)       took 4.910000 ms
  Veb sorting (predecessor)     took 4.910000 ms
  Radix64 sorting (successor)   took 1.820000 ms
  Radix64 sorting (predecessor) took 2.010000 ms
  Quicksort                     took 7.520000 ms
```

> **Test machine.** Numbers above were collected on:
> AMD Ryzen 9 9900X (12C / 24T), 64 GB RAM, Windows 11 Pro 26200, MSVC Release build.
> Three back-to-back runs were within ±5%; the run shown is representative. Your
> numbers will vary by CPU, compiler, and build flags — run `build/test/SortingBenchmark`
> locally for your own.

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
