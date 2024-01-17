cmake_minimum_required(VERSION 3.10.0)

include(${CMAKE_CURRENT_SOURCE_DIR}/../cmake/common.cmake)

set(SRC_FILES
    core/src/tiovx_modules.c
    core/src/tiovx_modules_cbs.c
    ../utils/src/tiovx_utils.c
    src/tiovx_multi_scaler_module.c
    src/tiovx_dl_color_convert_module.c
    src/tiovx_color_convert_module.c
    src/tiovx_viss_module.c
    src/tiovx_sensor_module.c
    )

build_lib(${PROJECT_NAME}  # Named argument: library name
          SHARED           # Named argument: library type
          0.1.0            # Named argument: library version
          # The following all will be clubbed under ${ARGN}
          ${SRC_FILES})
