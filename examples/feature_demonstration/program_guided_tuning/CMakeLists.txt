cmake_minimum_required(VERSION 3.0.0)
project(program_guided_tuning)

set(CMAKE_CXX_STANDARD 14)

# OpenCL
find_package(OpenCL REQUIRED)

# add example executables
add_executable(program_guided_tuning program_guided_tuning.cpp)
target_include_directories(program_guided_tuning PRIVATE ${OpenCL_INCLUDE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/../../../)
target_link_libraries(program_guided_tuning ${OpenCL_LIBRARY})