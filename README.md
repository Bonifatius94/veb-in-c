
# Van Emde Boas Tree Data Structure

## About
This project provides an implementation of the van-Emde-Boas tree data structure in C.

## Setup

```sh
sudo apt-get update
sudo apt-get install -y build-essential cmake git
```

## Build + Run Tests

Clone the source code from this repository:

```sh
git clone https://github.com/Bonifatius94/veb-in-c
cd veb-in-c
```

Run the build script. This also executes a bunch of unit tests and benchmarks.

```sh
./build.sh
```

## Deployment into Projects

1) Copy the file contents from [vebtrees.h](./include/vebtrees.h) and [vebtrees.c](./src/vebtrees.c)
2) Paste the contents into a new header file (e.g. name it vebtrees.h)
3) Put this header file into your project's include folder (no further dependencies required)

TODO: create a deployment mechanism that does the copying for a source distro header file including the implementation details

## License
This project is available under the terms of the MIT license.
