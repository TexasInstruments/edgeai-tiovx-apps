cmake_minimum_required(VERSION 3.10.0)

set(PROJ_NAME edgeai-tiovx-apps-test)

include(${CMAKE_CURRENT_SOURCE_DIR}/../cmake/common.cmake)

set(SRC_FILES
    app_tiovx_multi_scaler_module_test.c
    app_tiovx_dl_color_convert_module_test.c
    app_tiovx_color_convert_module_test.c
    app_tiovx_viss_module_test.c
    app_tiovx_ldc_module_test.c
    app_tiovx_viss_ldc_msc_test.c
    app_tiovx_tee_module_test.c
    app_tiovx_tidl_module_test.c
    app_tiovx_dl_pre_proc_module_test.c
    app_tiovx_dl_post_proc_module_test.c
    app_tiovx_mosaic_module_test.c
    app_tiovx_v4l2_capture_module_test.c
    app_tiovx_linux_capture_display_test.c
    app_tiovx_linux_decode_display_test.c
    main.c)

if ("${TARGET_SOC}" STREQUAL "J721E" OR "${TARGET_SOC}" STREQUAL "J721S2" OR "${TARGET_SOC}" STREQUAL "J784S4")
    list(APPEND
         SRC_FILES
         app_tiovx_capture_module_test.c
         app_tiovx_capture_viss_ldc_msc_display_test.c
         app_tiovx_dl_pipeline_test.c
         app_tiovx_display_module_test.c
         app_tiovx_capture_dl_display_pipeline.c)
endif()

build_app(${PROJ_NAME}
          ${SRC_FILES})
