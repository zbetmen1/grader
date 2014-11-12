cmake_minimum_required(VERSION 2.8)

project(mod_grader)

# Add include from this project
include_directories(${PROJECT_SOURCE_DIR}/include)

# Set C++ flags
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -Wall -Werror -Wextra")

# Compile Apache module
include_directories("/usr/include/apr-1.0")
include_directories("/usr/include/apache2")
add_library(mod_grader SHARED ${PROJECT_SOURCE_DIR}/src/mod_grader.cpp ${PROJECT_SOURCE_DIR}/src/request_parser.cpp)
set_target_properties(mod_grader PROPERTIES PREFIX "")
set_target_properties(mod_grader PROPERTIES COMPILE_FLAGS "-pipe -g -O2 -fstack-protector --param=ssp-buffer-size=4 -Wformat -Werror=format-security")

# Deploy Apache module (note that you must implement this script on your own machine)
add_custom_command(TARGET mod_grader POST_BUILD 
                   COMMAND deploy_grader.sh ${PROJECT_SOURCE_DIR}/build/)

# Create grader shared library
find_package(Boost 1.54 COMPONENTS filesystem REQUIRED)
include_directories( ${Boost_INCLUDE_DIR} )
add_library(grader SHARED ${PROJECT_SOURCE_DIR}/src/subtest.cpp ${PROJECT_SOURCE_DIR}/src/configuration.cpp ${PROJECT_SOURCE_DIR}/src/task.cpp)
target_link_libraries(grader ${Boost_LIBRARIES})
                                                
