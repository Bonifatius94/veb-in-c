
# Van Emde Boas Tree Data Structure

## About
This project provides an implementation of the van-Emde-Boas tree data structure in C.

## Setup

```sh
sudo apt-get update && sudo apt-get install -y build-essential cmake
```

## Build + Run Tests

```sh
#!/bin/bash

# make sure there's an empty build folder
if [ -d "./build" ]; then rm -rf build; fi
mkdir build

# run the build and tests
pushd build
    cmake . .. && cmake --build . && ctest
popd
```
