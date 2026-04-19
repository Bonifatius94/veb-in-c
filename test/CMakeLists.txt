add_executable(UnitTests unit_tests.c)
target_include_directories(UnitTests PRIVATE ../include)

add_executable(SortingBenchmark sorting_benchmark.c)
target_include_directories(SortingBenchmark PRIVATE ../include)

# Keep assert() active in UnitTests regardless of build type. CMake's
# Release preset sets -DNDEBUG, which turns assert() into a no-op and
# masks every precondition failure the unit tests exist to catch.
# SortingBenchmark is deliberately left with NDEBUG on release builds —
# asserts inside vebtree_insert_key / vebtree_predecessor / ... land
# inside the timed inner loop and skew the measurement.
target_compile_options(UnitTests PRIVATE
    $<IF:$<C_COMPILER_ID:MSVC>,/UNDEBUG,-UNDEBUG>)
