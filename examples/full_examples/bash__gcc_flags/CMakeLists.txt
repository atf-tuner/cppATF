cmake_minimum_required(VERSION 3.0.0)
project(bash__gcc_flags)

set(CMAKE_CXX_STANDARD 14)

# Python
find_package(PythonLibs 2.7 REQUIRED)
include_directories(${PYTHON_INCLUDE_DIR})

# add example executables
add_executable(bash__gcc_flags bash__gcc_flags.cpp)
target_include_directories(bash__gcc_flags PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../../../)
target_link_libraries(bash__gcc_flags ${PYTHON_LIBRARY})
