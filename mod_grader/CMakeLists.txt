cmake_minimum_required(VERSION 2.8)

project(mod_grader)

# Add include from this project
include_directories(include/core include/daemon/ include/plugins/ include/utils/ include/web/)

# Set C++ flags
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++1y -Wall -Werror -g")

# Find Boost library
find_package(Boost 1.54 COMPONENTS log log_setup thread date_time filesystem system iostreams REQUIRED)
include_directories( ${Boost_INCLUDE_DIR} )
add_definitions(-DBOOST_LOG_DYN_LINK) # for stupid log library

find_library(POCO_FOUNDATION PocoFoundation REQUIRED)

# Compile dynamic library
add_library(dynamic_utils SHARED src/utils/dynamic/object.cpp 
                                 src/utils/dynamic/shared_lib.cpp
                                 src/utils/dynamic/register_creators.cpp)
                                 
# Compile process library
add_library(process_utils SHARED src/utils/process/safe_process.cpp
                                 src/utils/process/pipe.cpp
                                 src/utils/process/process.cpp)
target_link_libraries(process_utils ${Boost_LIBRARIES})

add_library(grader SHARED src/core/task.cpp 
                          src/core/grader_base.cpp 
                          src/core/subtest.cpp 
                          src/core/configuration.cpp
                          src/core/log.cpp
                          src/core/shared_memory.cpp)
                          
target_link_libraries(grader ${Boost_LIBRARIES} ${POCO_FOUNDATION} dynamic_utils process_utils)

# Compile Apache module
include_directories("/usr/include/apr-1.0")
include_directories("/usr/include/apache2")
add_library(mod_grader SHARED src/web/mod_grader.cpp src/web/request_parser.cpp)
target_link_libraries(mod_grader grader)
set_target_properties(mod_grader PROPERTIES PREFIX "")
set_target_properties(mod_grader PROPERTIES COMPILE_FLAGS "-pipe -g -O2 -fstack-protector --param=ssp-buffer-size=4 -Wformat -Werror=format-security")

# Deploy Apache module (note that you must implement this script on your own machine)
add_custom_command(TARGET mod_grader POST_BUILD 
                   COMMAND deploy_grader.sh ${PROJECT_SOURCE_DIR}/build/)

# Compile daemon
add_executable(graderd src/core/daemon.cpp)
target_link_libraries(graderd grader process_utils ${Boost_LIBRARIES})
                   
# Compile grader_c
add_library(grader_c SHARED src/plugins/grader_c.cpp)
target_link_libraries(grader_c grader)

# Compile grader_cpp
add_library(grader_cpp SHARED src/plugins/grader_cpp.cpp)
target_link_libraries(grader_cpp grader)

# Compile python grader
add_library(grader_py SHARED src/plugins/grader_py.cpp)
target_link_libraries(grader_py grader ${Boost_LIBRARIES})
                                                
# Compile grader_cpp11
add_library(grader_cpp11 SHARED src/plugins/grader_cpp11.cpp)
target_link_libraries(grader_cpp11 grader)