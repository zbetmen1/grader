cmake_minimum_required(VERSION 2.8.11)

set(CMAKE_CXX_COMPILER "/usr/bin/g++-4.8")
SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -Wall -Wextra -Wno-ignored-qualifiers")
project(test_dynamic)
include_directories("../dynamic/include/")
add_library(testdynamic SHARED test_object.cpp)
target_link_libraries(testdynamic "/home/zbetmen/Documents/programming/grader/dynamic/lib/libdynamic.so")

