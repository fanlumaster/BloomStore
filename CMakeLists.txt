cmake_minimum_required(VERSION 3.25.0)
project(bloomstore VERSION 1.0)

# specify the C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

include_directories("./includes/")

add_compile_options("$<$<C_COMPILER_ID:MSVC>:/utf-8>")
add_compile_options("$<$<CXX_COMPILER_ID:MSVC>:/utf-8>")

add_executable(bloomstore "./tests/regular_test.cpp" "./src/bloom_filter.cpp" "./src/bloom_store.cpp" "./src/bloom_store_instance.cpp" "./src/hash_funcs.cpp" "./src/log.cpp")
