cmake_minimum_required(VERSION 3.9)
project(ikc)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find OpenMP package (required for parallel graph processing)
find_package(OpenMP REQUIRED)

# Add the library directory
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/lib)

# Set compiler flags for optimization
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3 -march=native")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O3 -march=native")

# Add the ikc executable
add_executable(ikc app/ikc.cpp)
target_link_libraries(ikc PUBLIC OpenMP::OpenMP_CXX)
