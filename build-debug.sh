#!/bin/bash
set -e

# Debug build for RGR cycles: keeps asserts enabled (no -DNDEBUG) so unit
# tests actually check preconditions instead of silently no-op'ing.
# Use ./build.sh for release / CI-parity builds.

if [ -d "./build-debug" ]; then rm -rf build-debug; fi
mkdir build-debug

pushd build-debug
    cmake -DCMAKE_BUILD_TYPE=Debug .. && cmake --build .
    CTEST_OUTPUT_ON_FAILURE=1 ctest
popd
