cmake_minimum_required(VERSION 3.25)
project(btree)

enable_testing()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=thread -g")

add_subdirectory(test)

add_executable(main src/bplustree.h src/macros.h src/shared_latch.h src/main.cpp)