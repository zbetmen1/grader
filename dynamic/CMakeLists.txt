cmake_minimum_required(VERSION 2.8.11)

project(dynamic)

IF(CMAKE_COMPILER_IS_GNUCXX)
    SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++1y -Wall -Wextra -Wno-ignored-qualifiers")
ENDIF(CMAKE_COMPILER_IS_GNUCXX)

include_directories("include/")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/lib/)

set(SRC src/any.cpp src/register_methods.cpp src/register_constructor.cpp src/shared_lib.cpp src/object.cpp)

add_library(dynamic SHARED ${SRC})
target_link_libraries(dynamic dl)

add_executable(main main.cpp)
target_link_libraries(main dynamic)

install(TARGETS main RUNTIME DESTINATION bin)
