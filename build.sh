#!/bin/bash
set -e

# ===============================
#     D E B U G   B U I L D
# ===============================

# make sure there's an empty build folder
if [ -d "./build" ]; then rm -rf build; fi
mkdir build

# run the build and tests (debug mode)
pushd build
    cmake .. && cmake --build .
    CTEST_OUTPUT_ON_FAILURE=1 ctest
popd

# ===============================
#    R E L E A S E   B U I L D
# ===============================

# make sure there's an empty build folder
if [ -d "./build" ]; then rm -rf build; fi
mkdir build

# run the build and tests (release mode)
pushd build
    cmake -DCMAKE_BUILD_TYPE=Release .. && cmake --build .
    CTEST_OUTPUT_ON_FAILURE=1 ctest
popd
