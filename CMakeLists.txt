cmake_minimum_required(VERSION 3.10.0)

project(edgeai-tiovx-apps)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS}")

add_subdirectory(modules)
add_subdirectory(tests)
