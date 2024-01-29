cmake_minimum_required(VERSION 3.0.0)
project(opencl__saxpy)

set(CMAKE_CXX_STANDARD 14)

# OpenCL
find_package(OpenCL REQUIRED)

# add example executables
add_executable(opencl__saxpy opencl__saxpy.cpp)
target_include_directories(opencl__saxpy PRIVATE ${OpenCL_INCLUDE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/../../../)
target_link_libraries(opencl__saxpy ${OpenCL_LIBRARY})