cmake_minimum_required(VERSION 3.25)
project(btree)

set(CMAKE_CXX_STANDARD 17)

add_subdirectory(test)

add_executable(main src/bplustree.h src/macros.h src/main.cpp)