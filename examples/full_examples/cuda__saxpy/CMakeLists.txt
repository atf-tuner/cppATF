cmake_minimum_required(VERSION 3.0.0)
project(cuda__saxpy)

set(CMAKE_CXX_STANDARD 14)

# CUDA
find_package(CUDAToolkit REQUIRED)

# add example executables
add_executable(cuda__saxpy cuda__saxpy.cpp)
target_include_directories(cuda__saxpy PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../../../)
target_link_libraries(cuda__saxpy CUDA::cuda_driver CUDA::cudart CUDA::nvrtc)
