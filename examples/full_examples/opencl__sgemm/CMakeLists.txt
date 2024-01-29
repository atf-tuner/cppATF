cmake_minimum_required(VERSION 3.0.0)
project(opencl__sgemm)

set(CMAKE_CXX_STANDARD 14)

# OpenCL
find_package(OpenCL REQUIRED)

# add example executables
add_executable(opencl__sgemm opencl__sgemm.cpp)
target_include_directories(opencl__sgemm PRIVATE ${OpenCL_INCLUDE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/../../../)
target_link_libraries(opencl__sgemm ${OpenCL_LIBRARY})