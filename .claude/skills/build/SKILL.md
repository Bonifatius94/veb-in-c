---
name: build
description: Clean-build the veb-in-c library and run all CTest suites (UnitTests + SortingBenchmark). Use after edits to vebtrees.h or anything under test/, or when the user wants to verify the build is green.
---

# /build

Runs the same `./build.sh` script that CI runs. It wipes `build/`, reconfigures CMake in Release mode, builds, and runs CTest with `CTEST_OUTPUT_ON_FAILURE=1`.

## Steps

1. From the repo root, run `./build.sh`.
2. If CMake/GCC isn't installed, point the user at the README's apt line (`sudo apt-get install -y build-essential cmake git`).

## Report

- Whether configure, build, and tests each succeeded.
- Which CTest targets ran (`UnitTests`, `SortingBenchmark`) and their status.
- On failure, surface the relevant compiler error or the failing test's output. Do not dump the full log.
- On success, a one-line confirmation is enough — the user doesn't need the full build log.

## Variants

- Debug build: uncomment `set(CMAKE_BUILD_TYPE Debug)` in the top-level `CMakeLists.txt`, or pass `-DCMAKE_BUILD_TYPE=Debug` to cmake. Ask before switching — Release is the default and what CI checks.
- Incremental build: if `build/` already exists and the user is iterating, `cmake --build build && ctest --test-dir build --output-on-failure` is faster than a full clean. Still, clarify before skipping the clean step.
