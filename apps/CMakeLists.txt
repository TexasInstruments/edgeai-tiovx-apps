cmake_minimum_required(VERSION 3.10.0)

set(PROJ_NAME edgeai-tiovx-apps-main)

include(${CMAKE_CURRENT_SOURCE_DIR}/../cmake/common.cmake)

set(SRC_FILES
    src/main.c
    src/app.c
    src/deep_learning_block.c
    src/resize_block.c
    src/input_block.c
    src/output_block.c
    src/misc.c)

build_app(${PROJ_NAME}
          ${SRC_FILES})