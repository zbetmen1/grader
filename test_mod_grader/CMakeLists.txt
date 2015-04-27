cmake_minimum_required(VERSION 2.8)
project(test_mod_grader)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -Wall -Werror -Wextra -g")

# Find Boost libs
find_package(Boost 1.54 COMPONENTS system filesystem regex unit_test_framework thread REQUIRED)
include_directories( ${Boost_INCLUDE_DIR} )
add_definitions(-DBOOST_LOG_DYN_LINK)

# Include dir
include_directories(../mod_grader/include/core)
add_executable(test_mod_grader main.cpp test_configuration.cpp ../mod_grader/src/core/configuration.cpp ../mod_grader/src/core/logger.cpp)
target_link_libraries(test_mod_grader ${Boost_LIBRARIES})