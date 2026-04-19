add_executable(UnitTests unit_tests.c)
target_include_directories(UnitTests PRIVATE ../include)

add_executable(SortingBenchmark sorting_benchmark.c)
target_include_directories(SortingBenchmark PRIVATE ../include)

# Keep assert() active in tests regardless of build type. CMake's Release
# presets set -DNDEBUG, which turns assert() into a no-op — masking every
# precondition failure the tests exist to catch.
set(_test_keep_asserts $<IF:$<C_COMPILER_ID:MSVC>,/UNDEBUG,-UNDEBUG>)
target_compile_options(UnitTests PRIVATE ${_test_keep_asserts})
target_compile_options(SortingBenchmark PRIVATE ${_test_keep_asserts})
