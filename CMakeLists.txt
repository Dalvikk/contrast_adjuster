cmake_minimum_required(VERSION 3.15)

project(contrast_adjuster)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++14 -fopenmp")

add_executable(contrast_adjuster 
main.cpp 
"image_ppm.h" 
"image_ppm.cpp")
