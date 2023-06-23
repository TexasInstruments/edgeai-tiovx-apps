include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

add_compile_options(-std=c++17)

# Specific compile optios across all targets
#add_compile_definitions(MINIMAL_LOGGING)

IF(NOT CMAKE_BUILD_TYPE)
  SET(CMAKE_BUILD_TYPE Release)
ENDIF()

# Turn off output data dumps for testing by default
OPTION(EDGEAI_ENABLE_OUTPUT_FOR_TEST "Enable Output Dumps for test" OFF)

# Check if we got an option from command line
if(EDGEAI_ENABLE_OUTPUT_FOR_TEST)
    message("EDGEAI_ENABLE_OUTPUT_FOR_TEST enabled")
    add_definitions(-DEDGEAI_ENABLE_OUTPUT_FOR_TEST)
endif()

message(STATUS "CMAKE_BUILD_TYPE = ${CMAKE_BUILD_TYPE} PROJECT_NAME = ${PROJECT_NAME}")

SET(CMAKE_FIND_LIBRARY_PREFIXES "" "lib")
SET(CMAKE_FIND_LIBRARY_SUFFIXES ".a" ".lib" ".so")

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/lib/${CMAKE_BUILD_TYPE})
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin/${CMAKE_BUILD_TYPE})
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin/${CMAKE_BUILD_TYPE})

if (NOT DEFINED ENV{SOC})
    message(FATAL_ERROR "SOC not defined.")
endif()

set(TARGET_SOC_LOWER $ENV{SOC})

if ("${TARGET_SOC_LOWER}" STREQUAL "j721e")
    set(TARGET_PLATFORM     J7)
    set(TARGET_CPU          A72)
    set(TARGET_OS           LINUX)
    set(TARGET_SOC          J721E)
elseif ("${TARGET_SOC_LOWER}" STREQUAL "j721s2")
    set(TARGET_PLATFORM     J7)
    set(TARGET_CPU          A72)
    set(TARGET_OS           LINUX)
    set(TARGET_SOC          J721S2)
elseif ("${TARGET_SOC_LOWER}" STREQUAL "j784s4")
    set(TARGET_PLATFORM     J7)
    set(TARGET_CPU          A72)
    set(TARGET_OS           LINUX)
    set(TARGET_SOC          J784S4)
elseif ("${TARGET_SOC_LOWER}" STREQUAL "am62a")
    set(TARGET_PLATFORM     SITARA)
    set(TARGET_CPU          A53)
    set(TARGET_OS           LINUX)
    set(TARGET_SOC          AM62A)
else()
    message(FATAL_ERROR "SOC ${TARGET_SOC_LOWER} is not supported.")
endif()

message("SOC=${TARGET_SOC_LOWER}")

add_definitions(
    -DTARGET_CPU=${TARGET_CPU}
    -DTARGET_OS=${TARGET_OS}
    -DSOC_${TARGET_SOC}
)

set(DLPACK_INSTALL_DIR ${TARGET_FS}/usr/include/dlpack)

link_directories(${TARGET_FS}/usr/lib/aarch64-linux-gnu
                 ${TARGET_FS}/usr/lib/
                 )

#message("PROJECT_SOURCE_DIR =" ${PROJECT_SOURCE_DIR})
#message("CMAKE_SOURCE_DIR =" ${CMAKE_SOURCE_DIR})

include_directories(${PROJECT_SOURCE_DIR}
                    ${PROJECT_SOURCE_DIR}/..
                    ${PROJECT_SOURCE_DIR}/include
                    SYSTEM ${TARGET_FS}/usr/local/include
                    SYSTEM ${TARGET_FS}/usr/include/processor_sdk/vision_apps
                    SYSTEM ${TARGET_FS}/usr/include/edgeai-tiovx-modules
                    SYSTEM ${TARGET_FS}/usr/include/processor_sdk/tiovx/include
                    SYSTEM ${TARGET_FS}/usr/include/processor_sdk/tiovx/kernels/include/
                    SYSTEM ${TARGET_FS}/usr/include/processor_sdk/tiovx/kernels_j7/include/
                    SYSTEM ${TARGET_FS}/usr/include/processor_sdk/vision_apps/kernels/img_proc/include/
                    SYSTEM ${TARGET_FS}/usr/include/processor_sdk/tidl_j7/ti_dl/inc
                    SYSTEM ${TARGET_FS}/usr/include/processor_sdk/ivision
                    SYSTEM ${TARGET_FS}/usr/include/processor_sdk/tiovx/utils/include
                    SYSTEM ${TARGET_FS}/usr/include/edgeai-tiovx-kernels/
                    SYSTEM ${TARGET_FS}/usr/include/processor_sdk/vision_apps/kernels/fileio/include/
                    SYSTEM ${TARGET_FS}/usr/include/processor_sdk/imaging/kernels/include/
                    SYSTEM ${TARGET_FS}/usr/include/processor_sdk/app_utils/
                    SYSTEM ${TARGET_FS}/usr/include/processor_sdk/vision_apps/utils/app_init/include/
                    SYSTEM ${TARGET_FS}/usr/include/edgeai-apps-utils
                    SYSTEM ${TARGET_FS}/usr/include/processor_sdk/vision_apps/kernels/fileio/include/
                    SYSTEM ${TARGET_FS}/usr/include/processor_sdk/video_io/kernels/include/
                    SYSTEM ${TARGET_FS}/usr/include/processor_sdk/imaging
                    SYSTEM ${TARGET_FS}/usr/include/processor_sdk/imaging/kernels/include/
                    )

set(COMMON_LINK_LIBS
    edgeai_utils
    edgeai_tiovx_apps_common
    edgeai-tiovx-kernels
    edgeai-tiovx-modules
    edgeai-apps-utils
    )

set(SYSTEM_LINK_LIBS
    ncurses
    tinfo
    yaml-cpp
    pthread
    tivision_apps
    dl
    )

# Function for building a node:
# ARG0: app name
# ARG1: source list
function(build_app)
    set(app ${ARGV0})
    set(src ${ARGV1})
    add_executable(${app} ${${src}})
    target_link_libraries(${app}
                          -Wl,--start-group
                          ${COMMON_LINK_LIBS}
                          ${TARGET_LINK_LIBS}
                          ${SYSTEM_LINK_LIBS}
                          -Wl,--end-group)
endfunction(build_app)

# Function for building a node:
# ARG0: lib name
# ARG1: source list
# ARG2: type (STATIC, SHARED)
function(build_lib)
    set(lib ${ARGV0})
    set(src ${ARGV1})
    set(type ${ARGV2})
    set(version 1.0.0)

    add_library(${lib} ${type} ${${src}})

    get_filename_component(PROJ_DIR "${CMAKE_CURRENT_SOURCE_DIR}" NAME)

    set(INC_DIR_DST ${CMAKE_INSTALL_LIBDIR}/${CMAKE_INSTALL_INCLUDEDIR}/${PROJ_DIR})

    install(TARGETS ${lib}
            LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}  # Shared Libs
            ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}  # Static Libs
            RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}  # Executables, DLLs
            INCLUDES DESTINATION ${INC_DIR_DST}
    )

    # Specify the header files to install
    install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/include
        DESTINATION ${INC_DIR_DST}
    )

endfunction(build_lib)

