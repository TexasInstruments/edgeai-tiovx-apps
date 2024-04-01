/*
 *
 * Copyright (c) 2024 Texas Instruments Incorporated
 *
 * All rights reserved not granted herein.
 *
 * Limited License.
 *
 * Texas Instruments Incorporated grants a world-wide, royalty-free, non-exclusive
 * license under copyrights and patents it now or hereafter owns or controls to make,
 * have made, use, import, offer to sell and sell ("Utilize") this software subject to the
 * terms herein.  With respect to the foregoing patent license, such license is granted
 * solely to the extent that any such patent is necessary to Utilize the software alone.
 * The patent license shall not apply to any combinations which include this software,
 * other than combinations with devices manufactured by or for TI ("TI Devices").
 * No hardware patent is licensed hereunder.
 *
 * Redistributions must preserve existing copyright notices and reproduce this license
 * (including the above copyright notice and the disclaimer and (if applicable) source
 * code license limitations below) in the documentation and/or other materials provided
 * with the distribution
 *
 * Redistribution and use in binary form, without modification, are permitted provided
 * that the following conditions are met:
 *
 * *       No reverse engineering, decompilation, or disassembly of this software is
 * permitted with respect to any software provided in binary form.
 *
 * *       any redistribution and use are licensed by TI for use only with TI Devices.
 *
 * *       Nothing shall obligate TI to provide you with source code for the software
 * licensed and provided to you in object code.
 *
 * If software source code is provided to you, modification and redistribution of the
 * source code are permitted provided that the following conditions are met:
 *
 * *       any redistribution and use of the source code, including any resulting derivative
 * works, are licensed by TI for use only with TI Devices.
 *
 * *       any redistribution and use of any object code compiled from the source code
 * and any resulting derivative works, are licensed by TI for use only with TI Devices.
 *
 * Neither the name of Texas Instruments Incorporated nor the names of its suppliers
 *
 * may be used to endorse or promote products derived from this software without
 * specific prior written permission.
 *
 * DISCLAIMER.
 *
 * THIS SOFTWARE IS PROVIDED BY TI AND TI'S LICENSORS "AS IS" AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL TI AND TI'S LICENSORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
 * OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
 * OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

#ifndef _TIOVX_APPS_INFO
#define _TIOVX_APPS_INFO


#include <stdint.h>
#include <string.h>
#include <stdbool.h>


#ifdef TIOVX_APPS_DEBUG
#define TIOVX_APPS_PRINTF(f_, ...) printf("[TIOVX_APPS][DEBUG] %d: %s: " f_, __LINE__, __func__, ##__VA_ARGS__)
#else
#define TIOVX_APPS_PRINTF(f_, ...)
#endif

#define TIOVX_APPS_ERROR(f_, ...) printf("[TIOVX_APPS][ERROR] %d: %s: " f_, __LINE__, __func__, ##__VA_ARGS__)

#ifdef __cplusplus
extern "C" {
#endif


#define DEFAULT_CHAR_ARRAY_SIZE (64)

#define MAX_CHAR_ARRAY_SIZE (128)
#define MAX_FLOWS           (32)
#define MAX_SUBFLOW         (32)
#define MAX_MOSAIC_INPUT    (32)

#define MAX_RAW_IMG         (100)

typedef enum {
    RTOS_CAM = 0,
    LINUX_CAM,
    H264_VID,
    RAW_IMG,
    NUM_INPUT_SOURCES
} InputSource;

typedef enum {
    RTOS_DISPLAY = 0,
    LINUX_DISPLAY,
    H264_ENCODE,
    IMG_DIR,
    NUM_OUTPUT_SINKS
} OutputSink;

/*
 * Input Information 
 */
typedef struct {
    /* Unique input name */
    char            name[DEFAULT_CHAR_ARRAY_SIZE];

    /* Input source. Ex: RTOS_CAM, LINUX_CAM, RAW_IMG etc. */
    InputSource     source;

    /* Source name */
    char            source_name[DEFAULT_CHAR_ARRAY_SIZE];

    /* Width of the input. */
    uint32_t        width;

    /* Height of the input. */
    uint32_t        height;

    /* Loop raw img input */
    bool            loop;

    /* Frame rate for raw img input. */
    float           framerate;

    /* Directory containg raw img. */
    char            raw_img_paths[MAX_RAW_IMG][MAX_CHAR_ARRAY_SIZE];

    /* Number of total raw img */
    uint32_t        num_raw_img;

    /* Video file path */
    char            video_path[MAX_CHAR_ARRAY_SIZE];

    /* Input format [Does not matter in case of RTOS_CAM]. */
    char            format[DEFAULT_CHAR_ARRAY_SIZE];

    /* Name of the camera to be used for RTOS_CAM. */
    char            sensor_name[DEFAULT_CHAR_ARRAY_SIZE];

    /* Channe mask for RTOS_CAM. */
    uint32_t        channel_mask;

    /* Enable LDC */
    bool            ldc_enabled;

    /* Device path for LINUX_CAM. */
    char            device[DEFAULT_CHAR_ARRAY_SIZE];

    /* Subdev path for LINUX_CAM. */
    char            subdev[DEFAULT_CHAR_ARRAY_SIZE];

    /* Number of channels of the input */
    uint32_t        num_channels;

} InputInfo;

/*
 * Pre Proc Information
 */
typedef struct {
    /* Crop width */
    uint32_t   crop_width;

    /* Crop height */
    uint32_t   crop_height;

    /* Resize width */
    uint32_t    resize_width;

    /* Resize height */
    uint32_t    resize_height;

    /* Tensor format */
    uint8_t     tensor_format;

    float       mean[3];

    float       scale[3];
} PreProcInfo;

/*
 * Post Proc Information
 */
typedef struct {
    /* TopN overlay for image classification. */
    uint32_t        top_n;

    /* Vizualisation threshold for object detection. */
    float           viz_threshold;

    /* Alpha value for blending segmentation map. */
    float           alpha;

    /* Normalized detection */
    bool            norm_detect;

    /* Task type. */
    char            task_type[DEFAULT_CHAR_ARRAY_SIZE];

    /* Formatter */
    int32_t         formatter[6];

    /* Label Offset */
    int32_t         label_offset[1000];

    /* Number of label Offset */
    uint32_t        num_label_offset;

    /* Label Index Offset */
    int32_t         label_index_offset;

} PostProcInfo;


/*
 * Model Information 
 */
typedef struct {
    /* Unique model name */
    char            name[DEFAULT_CHAR_ARRAY_SIZE];

    /* Path of model directory. */
    char            model_path[MAX_CHAR_ARRAY_SIZE];

    /* Path of model io_config. */
    char            io_config_path[MAX_CHAR_ARRAY_SIZE];

    /* Path of model network. */
    char            network_path[MAX_CHAR_ARRAY_SIZE];

    /* Pre Proc Information */
    PreProcInfo     pre_proc_info;

    /* Post Proc Information */
    PostProcInfo    post_proc_info;

} ModelInfo;

/*
 * Output Information 
 */
typedef struct {
    /* Unique output name */
    char            name[DEFAULT_CHAR_ARRAY_SIZE];

    /* Name of the source. Ex: RTOS_DISPLAY, LINUX_DISPLAY etc. */
    OutputSink      sink;

    /* Sink name */
    char            sink_name[DEFAULT_CHAR_ARRAY_SIZE];
    
    /* Width of the output. */
    uint32_t        width;

    /* Height of the output. */
    uint32_t        height;

    /* CRTC for kms display */
    uint32_t        crtc;

    /* Connector for kms display */
    uint32_t        connector;

    /* Output directory for saving image/videos */
    char            output_path[MAX_CHAR_ARRAY_SIZE];

    /* Framerate for saving image */
    float           framerate;

    /* Apply performance overlay */
    bool            overlay_perf;

    /* Background title */
    char            title[DEFAULT_CHAR_ARRAY_SIZE];
} OutputInfo;

/*
 * Mosaic Information 
 */
typedef struct {
    /* X pos of the window. */
    uint32_t        pos_x;
    
    /* Y pos of the window. */
    uint32_t        pos_y;

    /* Width of the window. */
    uint32_t        width;

    /* Height of the window. */
    uint32_t        height;
} MosaicInfo;


/*
 * Subflow Information
 */
typedef struct {
    /* Is this subflow used for deep learning*/
    bool        has_model;

    /* Number of channels */
    uint32_t    num_channels;

    /* Model Information. */
    ModelInfo   model_info;

    /* Output Information. */
    OutputInfo  output_info;

    /* Mosaic Information. */
    MosaicInfo  mosaic_info[32];

    /* Max width amongst provided mosaic info. */
    uint32_t    max_mosaic_width;

    /* Max height amongst provided mosaic info. */
    uint32_t    max_mosaic_height;
} SubflowInfo;

/*
 * Flow Information
 */
typedef struct {
    /* Input information. */
    InputInfo   input_info;

    /* Array of Subflow Informations. */
    SubflowInfo subflow_infos[MAX_SUBFLOW];

    /* Number of subflows for the particular input. */
    uint32_t    num_subflows;
} FlowInfo;

/*
 * Command Line Arguments
 */
typedef struct {
    /* Path to config file. */
    char        config_file[64];

    /* Verbose. */
    bool        verbose;

    /* Dump graph as dot */
    bool        dump_dot;
} CmdArgs;

#ifdef __cplusplus
}
#endif

#endif