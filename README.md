
# Van Emde Boas Tree Data Structure

## About
This project provides an implementation of the van-Emde-Boas tree data structure in C.

If you're interested in some theoretical background, see the [theory](./THEORY.md) section.

## Setup

```sh
# core packages for building the code / running tests
sudo apt-get update
sudo apt-get install -y build-essential cmake git

# additional packages for docs generation
sudo apt-get install -y doxygen graphviz
```

## Build + Run Tests + Generate Docs

Clone the source code from this repository:

```sh
git clone https://github.com/Bonifatius94/veb-in-c
cd veb-in-c
```

Run the build script. This also executes a bunch of unit tests and benchmarks.

```sh
./build.sh
```

If you like to generate the documentation website, run the gen-docs script.

```sh
./gen-docs.sh
```

## Deployment into Projects
Just copy the [vebtrees.h](./include/vebtrees.h) file from the include directory
and drop it into your project's include directory (single-header deployment).

## License
This project is available under the terms of the MIT license.
