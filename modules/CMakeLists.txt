cmake_minimum_required(VERSION 3.10.0)

include(${CMAKE_CURRENT_SOURCE_DIR}/../cmake/common.cmake)

set(SRC_FILES
    core/src/tiovx_modules.c
    core/src/tiovx_modules_cbs.c
    ../utils/src/tiovx_utils.c
    ../utils/src/yaml_parser.cpp
    src/tiovx_multi_scaler_module.c
    src/tiovx_dl_color_convert_module.c
    src/tiovx_color_convert_module.c
    src/tiovx_viss_module.c
    src/tiovx_sensor_module.c
    src/tiovx_ldc_module.c
    src/tiovx_tee_module.c
    src/tiovx_tidl_module.c
    src/tiovx_dl_pre_proc_module.c
    src/tiovx_dl_post_proc_module.c
    src/tiovx_mosaic_module.c
    src/v4l2_capture_module.c
    src/v4l2_decode_module.c
    src/v4l2_encode_module.c
    src/kms_display_module.c
    src/linux_aewb_module.c
    src/tiovx_obj_array_split_module.c
    src/tiovx_pyramid_module.c
    )

if ("${TARGET_SOC}" STREQUAL "J721E" OR "${TARGET_SOC}" STREQUAL "J721S2" OR "${TARGET_SOC}" STREQUAL "J784S4")
    list(APPEND
         SRC_FILES
         src/tiovx_capture_module.c
         src/tiovx_aewb_module.c
         src/tiovx_sde_module.c
         src/tiovx_sde_viz_module.c
         src/tiovx_dof_module.c
         src/tiovx_display_module.c)
endif()

build_lib(${PROJECT_NAME}  # Named argument: library name
          SHARED           # Named argument: library type
          0.1.0            # Named argument: library version
          # The following all will be clubbed under ${ARGN}
          ${SRC_FILES})
