---
name: bench
description: Run the SortingBenchmark and report the vEB-sort vs qsort timing comparison. Use when the user wants a perf number after changes to vebtrees.h.
---

# /bench

Runs the `SortingBenchmark` test binary, which sorts 500k dense keys (averaged over 100 runs) using both a vEB tree and stdlib `qsort`, and prints both timings.

## Steps

1. Check whether `build/test/SortingBenchmark` exists. If not, run `./build.sh` first (or suggest `/build`).
2. Run `build/test/SortingBenchmark`.
3. Parse the two "took X milliseconds" lines.

## Report

- Both timings, plus the ratio (vEB vs qsort).
- The expected ballpark is ~6 ms vs ~60 ms — roughly a 10× speedup per the README.
- **Flag loudly if vEB is slower than qsort, or if the ratio has regressed meaningfully** vs what the README documents. That's a perf regression and worth investigating before the change lands.
- One-shot numbers are fine for iteration. If the user wants a more stable number, rerun a few times — clock noise on a ~6 ms measurement is non-trivial.

## Notes

- The benchmark also asserts that the sorted output is strictly increasing. If an `assert` fires, the sorting logic is broken — not a perf issue.
- Benchmark binary ignores any CLI args today.
