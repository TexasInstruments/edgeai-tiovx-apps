cmake_minimum_required(VERSION 3.10.0)

project(edgeai_tiovx_apps_common)

include(${CMAKE_CURRENT_SOURCE_DIR}/../cmake/common.cmake)

set(EDGEAI_COMMON_SRCS
    src/edgeai_cmd_line_parse.cpp
    src/edgeai_demo_config.cpp
    src/edgeai_demo.cpp
    src/edgeai_utils.cpp
    src/edgeai_ovx_wrapper.cpp
    src/edgeai_post_proc.cpp
    src/edgeai_tidl.cpp
    src/edgeai_pre_proc.cpp
    src/edgeai_msc.cpp
    )

build_lib(${PROJECT_NAME} EDGEAI_COMMON_SRCS STATIC)
