cmake_minimum_required(VERSION 2.8)

project(mod_grader)

# Add include from this project
include_directories(include/core include/daemon/ include/plugins/ include/utils/ include/web/)

# Set C++ flags
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++1y -Wall -Werror -g")

# Find Boost library
find_package(Boost 1.54 COMPONENTS filesystem system iostreams REQUIRED)
include_directories( ${Boost_INCLUDE_DIR} )

# Compile dynamic library
add_library(dynamic_utils SHARED src/utils/dynamic/object.cpp 
                                 src/utils/dynamic/shared_lib.cpp
                                 src/utils/dynamic/register_creators.cpp)
                                 
# Compile process library
add_library(process_utils SHARED src/utils/process/safe_process.cpp
                                 src/utils/process/pipe.cpp
                                 src/utils/process/process.cpp)
target_link_libraries(process_utils ${Boost_LIBRARIES})

add_library(grader SHARED src/core/logger.cpp
                          src/core/configuration.cpp
                          src/core/plugin_manager.cpp
                          src/core/shared_memory.cpp)
                          
target_link_libraries(grader ${Boost_LIBRARIES} dynamic_utils process_utils)

# Compile daemon
add_executable(graderd src/core/daemon.cpp)
target_link_libraries(graderd grader process_utils ${Boost_LIBRARIES})