cmake_minimum_required(VERSION 3.9.4)
project(vebtrees LANGUAGES C)

# only enable this when VSCode debugging fails
# set(CMAKE_BUILD_TYPE Debug)

add_subdirectory(test)
enable_testing()
add_test(NAME UnitTests COMMAND UnitTests)
add_test(NAME SortingBenchmark COMMAND SortingBenchmark)
