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
    main.c)

build_app(${PROJ_NAME}
          ${SRC_FILES})
