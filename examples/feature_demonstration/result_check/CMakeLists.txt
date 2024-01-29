cmake_minimum_required(VERSION 3.0.0)
project(result_check)

set(CMAKE_CXX_STANDARD 14)

# OpenCL
find_package(OpenCL REQUIRED)

# add example executables
add_executable(result_check result_check.cpp)
target_include_directories(result_check PRIVATE ${OpenCL_INCLUDE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/../../../)
target_link_libraries(result_check ${OpenCL_LIBRARY})