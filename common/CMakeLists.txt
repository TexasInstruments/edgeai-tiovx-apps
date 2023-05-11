cmake_minimum_required(VERSION 3.10.0)

project(edgeai_tiovx_apps_common)

include(${CMAKE_CURRENT_SOURCE_DIR}/../cmake/common.cmake)

set(EDGEAI_COMMON_SRCS
    # src/edgeai_inference_pipe.cpp
    )

build_lib(${PROJECT_NAME} EDGEAI_COMMON_SRCS STATIC)
