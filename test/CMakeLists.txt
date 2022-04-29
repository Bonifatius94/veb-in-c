add_executable(UnitTests unit_tests.c)
target_include_directories(UnitTests PRIVATE ../include)

add_executable(SortingBenchmark sorting_benchmark.c)
target_include_directories(SortingBenchmark PRIVATE ../include)
