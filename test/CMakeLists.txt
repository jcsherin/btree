cmake_minimum_required(VERSION 3.25)
project(btree_tests)

set(CMAKE_CXX_STANDARD 17)

include(FetchContent)
FetchContent_Declare(
        googletest
        URL https://github.com/google/googletest/archive/refs/tags/v1.14.0.zip
)
# For Windows: Prevent overriding the parent project's compiler/linker settings
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

enable_testing()

add_executable(btree_insert_test btree_insert_test.cpp)
target_compile_definitions(btree_insert_test PRIVATE ENABLE_LATCH_DEBUGGING)
target_link_libraries(btree_insert_test GTest::gtest_main)

add_executable(btree_delete_test btree_delete_test.cpp)
target_compile_definitions(btree_delete_test PRIVATE ENABLE_LATCH_DEBUGGING)
target_link_libraries(btree_delete_test GTest::gtest_main)

add_executable(btree_concurrent_test btree_concurrent_test.cpp)
target_compile_definitions(btree_concurrent_test PRIVATE ENABLE_LATCH_DEBUGGING)
target_link_libraries(btree_concurrent_test GTest::gtest_main)

add_executable(btree_iterator_test btree_iterator_test.cpp)
target_compile_definitions(btree_iterator_test PRIVATE ENABLE_LATCH_DEBUGGING)
target_link_libraries(btree_iterator_test GTest::gtest_main)

include(GoogleTest)
gtest_discover_tests(btree_insert_test)
gtest_discover_tests(btree_delete_test)
gtest_discover_tests(btree_concurrent_test)
gtest_discover_tests(btree_iterator_test)
