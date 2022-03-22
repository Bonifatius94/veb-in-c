#!/bin/bash

# make sure there's an empty build folder
if [ -d "./build" ]; then rm -rf build; fi
mkdir build

# run the build and tests
pushd build
    cmake . .. && cmake --build .
    CTEST_OUTPUT_ON_FAILURE=1 ctest
popd
