include(GNUInstallDirs)

add_compile_options(-Wall)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17")

# Specific compile optios across all targets
#add_compile_definitions(MINIMAL_LOGGING)

IF(NOT CMAKE_BUILD_TYPE)
  SET(CMAKE_BUILD_TYPE Release)
ENDIF()

message(STATUS "CMAKE_BUILD_TYPE = ${CMAKE_BUILD_TYPE} PROJECT_NAME = ${PROJECT_NAME}")

SET(CMAKE_FIND_LIBRARY_PREFIXES "" "lib")
SET(CMAKE_FIND_LIBRARY_SUFFIXES ".a" ".lib" ".so")

if(NOT CMAKE_OUTPUT_DIR)
    set(CMAKE_OUTPUT_DIR ${CMAKE_SOURCE_DIR})
endif()
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_OUTPUT_DIR}/lib/${CMAKE_BUILD_TYPE})
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_OUTPUT_DIR}/lib/${CMAKE_BUILD_TYPE})
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_OUTPUT_DIR}/bin/${CMAKE_BUILD_TYPE})
set(CMAKE_INSTALL_LIBDIR           lib)

if(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
    set(CMAKE_INSTALL_PREFIX /usr CACHE PATH "Installation Prefix" FORCE)
endif()

if (NOT DEFINED ENV{SOC})
    message(FATAL_ERROR "SOC not defined.")
endif()

set(TARGET_SOC_LOWER $ENV{SOC})
set(TARGET_OS        $ENV{TARGET_OS})

if ("${TARGET_OS}" STREQUAL "")
    set(TARGET_OS           LINUX)
endif()

if ("${TARGET_SOC_LOWER}" STREQUAL "j721e")
    set(TARGET_PLATFORM     J7)
    set(TARGET_CPU          A72)
    set(TARGET_SOC          J721E)
elseif ("${TARGET_SOC_LOWER}" STREQUAL "j721s2")
    set(TARGET_PLATFORM     J7)
    set(TARGET_CPU          A72)
    set(TARGET_SOC          J721S2)
elseif ("${TARGET_SOC_LOWER}" STREQUAL "j784s4")
    set(TARGET_PLATFORM     J7)
    set(TARGET_CPU          A72)
    set(TARGET_SOC          J784S4)
elseif ("${TARGET_SOC_LOWER}" STREQUAL "j722s")
    set(TARGET_PLATFORM     J7)
    set(TARGET_CPU          A53)
    set(TARGET_SOC          J722S)
elseif ("${TARGET_SOC_LOWER}" STREQUAL "am62a")
    set(TARGET_PLATFORM     SITARA)
    set(TARGET_CPU          A53)
    set(TARGET_SOC          AM62A)
else()
    message(FATAL_ERROR "SOC ${TARGET_SOC_LOWER} is not supported.")
endif()

set(TARGET_CPU_TEMP $ENV{TARGET_CPU})
if(NOT ("${TARGET_CPU_TEMP}" STREQUAL ""))
    set(TARGET_CPU ${TARGET_CPU_TEMP})
endif()

message("SOC=${TARGET_SOC_LOWER}")
message("TARGET_CPU=${TARGET_CPU}")

add_definitions(
    -DTARGET_CPU_${TARGET_CPU}
    -DTARGET_OS_${TARGET_OS}
    -DSOC_${TARGET_SOC}
)

set(VISION_APPS_LIBS_PATH $ENV{VISION_APPS_LIBS_PATH})
set(TIOVX_LIBS_PATH $ENV{TIOVX_LIBS_PATH})
set(APP_UTILS_LIBS_PATH $ENV{APP_UTILS_LIBS_PATH})
set(IMAGING_LIBS_PATH $ENV{IMAGING_LIBS_PATH})
set(IMAGING_PREBUILT_LIBS_PATH $ENV{IMAGING_PREBUILT_LIBS_PATH})
set(VHWA_C_MODEL_LIBS_PATH $ENV{VHWA_C_MODEL_LIBS_PATH})
set(PREBUILT_LIBS_PATH $ENV{PREBUILT_LIBS_PATH})
set(EDGEAI_LIBS_PATH $ENV{EDGEAI_LIBS_PATH})
link_directories(${TARGET_FS}/usr/lib/aarch64-linux
                 ${TARGET_FS}/usr/lib
                 ${CMAKE_LIBRARY_PATH}/usr/lib
                 ${CMAKE_LIBRARY_PATH}/lib
                 ${VISION_APPS_LIBS_PATH}
                 ${TIOVX_LIBS_PATH}
                 ${APP_UTILS_LIBS_PATH}
                 ${IMAGING_LIBS_PATH}
                 ${IMAGING_PREBUILT_LIBS_PATH}
                 ${VHWA_C_MODEL_LIBS_PATH}
                 ${PREBUILT_LIBS_PATH}
                 ${EDGEAI_LIBS_PATH}
                 )

#message("PROJECT_SOURCE_DIR = ${PROJECT_SOURCE_DIR}")
#message("CMAKE_SOURCE_DIR   = ${CMAKE_SOURCE_DIR}")

set(PSDK_INCLUDE_PATH $ENV{PSDK_INCLUDE_PATH})
if ("${PSDK_INCLUDE_PATH}" STREQUAL "")
    set(PSDK_INCLUDE_PATH ${TARGET_FS}/usr/include/)
endif()

set(EDGEAI_INCLUDE_PATH $ENV{EDGEAI_INCLUDE_PATH})
if ("${EDGEAI_INCLUDE_PATH}" STREQUAL "")
    set(EDGEAI_INCLUDE_PATH ${TARGET_FS}/usr/include/)
endif()


include_directories(${PROJECT_SOURCE_DIR}
                    ${PROJECT_SOURCE_DIR}/modules/include
                    ${PROJECT_SOURCE_DIR}/modules/core/include
                    ${PROJECT_SOURCE_DIR}/utils/include
                    ${PSDK_INCLUDE_PATH}/processor_sdk/ivision
                    ${PSDK_INCLUDE_PATH}/processor_sdk/imaging
                    ${PSDK_INCLUDE_PATH}/processor_sdk/ti-perception-toolkit/include
                    ${PSDK_INCLUDE_PATH}/processor_sdk/imaging/algos/ae/include
                    ${PSDK_INCLUDE_PATH}/processor_sdk/imaging/algos/awb/include
                    ${PSDK_INCLUDE_PATH}/processor_sdk/imaging/algos/dcc/include
                    ${PSDK_INCLUDE_PATH}/processor_sdk/imaging/sensor_drv/include
                    ${PSDK_INCLUDE_PATH}/processor_sdk/imaging/ti_2a_wrapper/include
                    ${PSDK_INCLUDE_PATH}/processor_sdk/imaging/kernels/include
                    ${PSDK_INCLUDE_PATH}/processor_sdk/imaging/utils/itt_server/include/
                    ${PSDK_INCLUDE_PATH}/processor_sdk/tidl_j7/arm-tidl/rt/inc/
                    ${PSDK_INCLUDE_PATH}/processor_sdk/tidl_j7/arm-tidl/tiovx_kernels/include
                    ${PSDK_INCLUDE_PATH}/processor_sdk/tiovx/include
                    ${PSDK_INCLUDE_PATH}/processor_sdk/tiovx/kernels/include
                    ${PSDK_INCLUDE_PATH}/processor_sdk/tiovx/kernels_j7/include
                    ${PSDK_INCLUDE_PATH}/processor_sdk/tiovx/utils/include
                    ${PSDK_INCLUDE_PATH}/processor_sdk/vision_apps
                    ${PSDK_INCLUDE_PATH}/processor_sdk/vision_apps/utils/app_init/include
                    ${PSDK_INCLUDE_PATH}/processor_sdk/vision_apps/kernels/img_proc/include
                    ${PSDK_INCLUDE_PATH}/processor_sdk/vision_apps/kernels/fileio/include
                    ${PSDK_INCLUDE_PATH}/processor_sdk/vision_apps/kernels/stereo/include
                    ${PSDK_INCLUDE_PATH}/processor_sdk/app_utils/
                    ${PSDK_INCLUDE_PATH}/processor_sdk/video_io/kernels/include/
                    ${PSDK_INCLUDE_PATH}/processor_sdk/app_utils/utils/mem/include
                    ${PSDK_INCLUDE_PATH}/drm
                    ${EDGEAI_INCLUDE_PATH}/edgeai-tiovx-kernels
                    ${EDGEAI_INCLUDE_PATH}/edgeai-apps-utils/
                    ${TARGET_FS}/usr/include/
                   )

if ("${TARGET_CPU}" STREQUAL "A72" OR "${TARGET_CPU}" STREQUAL "A53")
    set(SYSTEM_LINK_LIBS
        tivision_apps
        edgeai-tiovx-kernels
        edgeai-apps-utils
        m
        yaml-cpp
        )
else()
    set(SYSTEM_LINK_LIBS
        edgeai-tiovx-kernels
        edgeai-apps-utils
        m
        yaml-cpp
        app_utils_init
        libvx_conformance_engine.a
        vx_conformance_tests
        vx_conformance_tests_testmodule
        vx_framework
        vx_kernels_host_utils
        vx_kernels_openvx_core
        vx_kernels_openvx_ext
        vx_kernels_openvx_ext_tests
        vx_kernels_target_utils
        vx_kernels_test_kernels
        vx_kernels_test_kernels_tests
        vx_platform_pc
        vx_target_kernels_dsp
        vx_target_kernels_ivision_common
        vx_target_kernels_openvx_core
        vx_target_kernels_openvx_ext
        vx_target_kernels_source_sink
        vx_target_kernels_tutorial
        vx_tiovx_internal_tests
        vx_tiovx_tests
        vx_tutorial
        vx_utils
        vx_vxu
        algframework_x86_64
        c6xsim_x86_64_C66
        dmautils_x86_64
        vxlib_bamplugin_x86_64
        vxlib_x86_64
        app_utils_mem
        app_utils_hwa
        app_utils_iss
        ti_imaging_aealg
        ti_imaging_dcc
        vx_kernels_hwa
        vx_kernels_hwa_tests
        vx_kernels_imaging
        vx_kernels_imaging_tests
        vx_target_kernels_dmpac_dof
        vx_target_kernels_dmpac_sde
        vx_target_kernels_imaging_aewb
        vx_target_kernels_j7_arm
        vx_target_kernels_vpac_ldc
        vx_target_kernels_vpac_msc
        vx_target_kernels_vpac_nf
        vx_target_kernels_vpac_viss
        ti_imaging_awbalg
        bl_filter_lib
        cac
        DOF
        ee
        flexcc
        flexcfa_vpac3
        glbce
        h3a
        ldc
        nsf4
        nsf4_wb
        rawfe
        RawHistogram
        scalar
        sde_hw
        utils
        VXLIB_triangulatePoints_i32f_o32f_lib_x86_64
        m
       )
endif()

if ("${TARGET_OS}" STREQUAL "LINUX")
    list(APPEND
         SYSTEM_LINK_LIBS
         drm
         avformat
         avutil
         avcodec)
endif()

if ("${TARGET_OS}" STREQUAL "QNX")
    list(APPEND
         SYSTEM_LINK_LIBS
         sharedmemallocator
         tiudma-usr
         tiipc-usr
         ti-udmalld
         ti-pdk
         ti-sciclient
         c++fs)
    add_definitions(
         -D_QNX_SOURCE
    )
endif()

set(COMMON_LINK_LIBS
    edgeai-tiovx-apps
    )

# Function for building a node:
# app_name: app name
# ${ARGN} expands everything after the last named argument to the end
# usage: build_app(app_name a.c b.c....)
function(build_app app_name)
    add_executable(${app_name} ${ARGN})
    target_link_libraries(${app_name}
                          ${COMMON_LINK_LIBS}
                          ${TARGET_LINK_LIBS}
                          ${SYSTEM_LINK_LIBS}
                         )

    set(BIN_INSTALL_DIR ${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_BINDIR})
    set(BINS ${CMAKE_OUTPUT_DIR}/bin/${CMAKE_BUILD_TYPE}/${app_name})

    install(FILES ${BINS}
            PERMISSIONS OWNER_EXECUTE OWNER_WRITE OWNER_READ
            DESTINATION ${BIN_INSTALL_DIR})

endfunction()

# Function for building a node:
# lib_name: Name of the library
# lib_type: (STATIC, SHARED)
# lib_ver: Version string of the library
# ${ARGN} expands everything after the last named argument to the end
# usage: build_lib(lib_name lib_type lib_ver a.c b.c ....)
function(build_lib lib_name lib_type lib_ver)
    add_library(${lib_name} ${lib_type} ${ARGN})

    SET_TARGET_PROPERTIES(${lib_name}
                          PROPERTIES
                          VERSION ${lib_ver}
                         )

    set(INCLUDE_INSTALL_DIR ${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_INCLUDEDIR}/${PROJECT_NAME})

    FILE(GLOB HDRS
         ${CMAKE_CURRENT_SOURCE_DIR}/include/*.h
         ${CMAKE_CURRENT_SOURCE_DIR}/core/include/*.h
         ${CMAKE_CURRENT_SOURCE_DIR}/core/include/*.h
         ${CMAKE_CURRENT_SOURCE_DIR}/../utils/include/*.h
        )

    install(TARGETS ${lib_name}
            EXPORT ${lib_name}Targets
            LIBRARY DESTINATION ${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_LIBDIR}  # Shared Libs
            ARCHIVE DESTINATION ${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_LIBDIR}  # Static Libs
            RUNTIME DESTINATION ${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_BINDIR}  # Executables, DLLs
           )

    # Specify the header files to install
    install(FILES ${HDRS} DESTINATION ${INCLUDE_INSTALL_DIR})

endfunction()

