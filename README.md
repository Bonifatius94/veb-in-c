
# Van Emde Boas Tree Data Structure

## About
This project provides an implementation of the van-Emde-Boas tree data structure in C89 style.
Benchmarks show that it's really efficient, e.g. sorting is ~ 10x faster than stdlib.h qsort().

It's a small, standalone, single-header implementation, not relying on any external dependencies,
highly portable to any operating system or processor architecture of choice.

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

Next, run the build script. This will execute a bunch of property tests and
benchmarks to ensure that the van Emde Boas tree is working as expected.

```sh
./build.sh
```

## Benchmark
For benchmarking, 500k dense indices need to be sorted. The stdlib.h qsort() function
is compared to a sorting procedure using a Veb tree. First, all keys are inserted
into the tree. Then the sorting procedure looks up the smallest key and finds successors
until the highest key is reached. For fairness, the benchmark includes building up the tree.

```sh
build/test/SortingBenchmark
```

Results show that sorting dense indices can be carried out very efficiently with Veb trees.
In fact, the practical performance improves by a quite significant factor of 10x.

```text
Sorting Benchmark (500k keys)
=============================
Veb sorting took 5.796520 milliseconds
Quicksort took 59.823940 milliseconds
```

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
Just copy the [vebtrees.h](./include/vebtrees.h) file into your project's include directory.
As already mentioned, the code is standalone, single-header, pure C89, no crazy dependencies.

## License
This project is available under the terms of the MIT license.
