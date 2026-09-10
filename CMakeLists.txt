cmake_minimum_required(VERSION 3.20)
project(search_engine CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

option(ENABLE_POSITIONS "enable position data in postings for phrase queries" ON)
option(ENABLE_COMPRESSION "enable delta+varint posting list compression" ON)

add_compile_options(-Wall -Wextra -Wno-unused-parameter)

if(ENABLE_POSITIONS)
    add_compile_definitions(ENABLE_POSITIONS)
endif()

if(ENABLE_COMPRESSION)
    add_compile_definitions(ENABLE_COMPRESSION)
endif()

# core library - everything except main and benchmarks
set(ENGINE_SOURCES
    src/indexer/tokenizer.cpp
    src/indexer/stemmer.cpp
    src/indexer/stopwords.cpp
    src/indexer/normalizer.cpp
    src/indexer/index_builder.cpp
    src/index/posting_list.cpp
    src/index/inverted_index.cpp
    src/index/forward_index.cpp
    src/query/query_parser.cpp
    src/query/scorer.cpp
    src/query/query_executor.cpp
    src/query/thread_pool.cpp
    src/storage/serializer.cpp
    src/storage/mmap_reader.cpp
)

add_library(engine_core STATIC ${ENGINE_SOURCES})
target_include_directories(engine_core PUBLIC ${CMAKE_SOURCE_DIR}/src)

find_package(Threads REQUIRED)
target_link_libraries(engine_core PUBLIC Threads::Threads)

# main executable
add_executable(search_engine src/main.cpp)
target_link_libraries(search_engine PRIVATE engine_core)

# benchmark executables
add_executable(bench_index src/bench/bench_index.cpp)
target_link_libraries(bench_index PRIVATE engine_core)

add_executable(bench_query src/bench/bench_query.cpp)
target_link_libraries(bench_query PRIVATE engine_core)

add_executable(bench_throughput src/bench/bench_throughput.cpp)
target_link_libraries(bench_throughput PRIVATE engine_core)

# test executable
add_executable(run_tests ${CMAKE_SOURCE_DIR}/tests/test_main.cpp)
target_link_libraries(run_tests PRIVATE engine_core)

# release flags
set(CMAKE_CXX_FLAGS_RELEASE "-O3 -march=native -DNDEBUG")

# debug flags with sanitizers
set(CMAKE_CXX_FLAGS_DEBUG "-g -fsanitize=address -fno-omit-frame-pointer")
set(CMAKE_EXE_LINKER_FLAGS_DEBUG "-fsanitize=address")
