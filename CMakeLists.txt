cmake_minimum_required(VERSION 3.9.4)
project(vebtrees LANGUAGES C)

file(GLOB sources "src/*.c")
add_library(vebtrees ${sources})
target_include_directories(vebtrees PRIVATE include)
set_property(TARGET vebtrees PROPERTY C_STANDARD 90)

add_subdirectory(test)
enable_testing()
add_test(NAME UnitTests COMMAND UnitTests)
add_test(NAME SortingBenchmark COMMAND SortingBenchmark)
