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

#include <tiovx_modules.h>
#include <tiovx_utils.h>
#include <v4l2_capture_module.h>

#define APP_BUFQ_DEPTH      (4)
#define APP_NUM_CH          (1)
#define APP_NUM_ITERATIONS  (5)

#define INPUT_WIDTH  (1920)
#define INPUT_HEIGHT (1080)

#define SENSOR_NAME "SENSOR_SONY_IMX219_RPI"
#define DCC_VISS "/opt/imaging/imx219/linear/dcc_viss.bin"

vx_status app_modules_v4l2_capture_test(vx_int32 argc, vx_char* argv[])
{
    vx_status status = VX_FAILURE;
    GraphObj graph;
    NodeObj *node = NULL;
    TIOVXVissNodeCfg cfg;
    BufPool *in_buf_pool = NULL, *out_buf_pool = NULL;
    Buf *inbuf = NULL, *outbuf = NULL;
    char output_filename[100];
    v4l2CaptureCfg v4l2_capture_cfg;
    v4l2CaptureHandle *v4l2_capture_handle;

    tiovx_viss_init_cfg(&cfg);

    sprintf(cfg.sensor_name, SENSOR_NAME);
    snprintf(cfg.dcc_config_file, TIVX_FILEIO_FILE_PATH_LENGTH, "%s", DCC_VISS);
    cfg.width = INPUT_WIDTH;
    cfg.height = INPUT_HEIGHT;
    sprintf(cfg.target_string, TIVX_TARGET_VPAC_VISS1);

    cfg.input_cfg.params.format[0].pixel_container = TIVX_RAW_IMAGE_8_BIT;
    cfg.input_cfg.params.format[0].msb = 7;

    status = tiovx_modules_initialize_graph(&graph);
    node = tiovx_modules_add_node(&graph, TIOVX_VISS, (void *)&cfg);
    node->sinks[0].bufq_depth = APP_BUFQ_DEPTH;
    status = tiovx_modules_verify_graph(&graph);

    in_buf_pool = node->sinks[0].buf_pool;
    out_buf_pool = node->srcs[0].buf_pool;

    v4l2_capture_init_cfg(&v4l2_capture_cfg);
    v4l2_capture_cfg.width = INPUT_WIDTH;
    v4l2_capture_cfg.height = INPUT_HEIGHT;
    v4l2_capture_cfg.pix_format = V4L2_PIX_FMT_SRGGB8;
    v4l2_capture_cfg.bufq_depth = APP_BUFQ_DEPTH + 1;
    sprintf(v4l2_capture_cfg.device, "/dev/video-imx219-cam0");

    v4l2_capture_handle = v4l2_capture_create_handle(&v4l2_capture_cfg);

    for (int i = 0; i < APP_BUFQ_DEPTH; i++) {
        inbuf = tiovx_modules_acquire_buf(in_buf_pool);
        v4l2_capture_enqueue_buf(v4l2_capture_handle, inbuf);
    }

    v4l2_capture_start(v4l2_capture_handle);

    for (int i = 0; i < APP_NUM_ITERATIONS; i++) {
        inbuf = v4l2_capture_dqueue_buf(v4l2_capture_handle);
        tiovx_modules_enqueue_buf(inbuf);

        outbuf = tiovx_modules_acquire_buf(out_buf_pool);
        tiovx_modules_enqueue_buf(outbuf);

        tiovx_modules_schedule_graph(&graph);
        tiovx_modules_wait_graph(&graph);

        inbuf = tiovx_modules_dequeue_buf(in_buf_pool);
        outbuf = tiovx_modules_dequeue_buf(out_buf_pool);

        v4l2_capture_enqueue_buf(v4l2_capture_handle, inbuf);

        sprintf(output_filename,
                "%s/output/imx219_1920x1080_v4l2_capture_nv12_%d.yuv",
                EDGEAI_DATA_PATH, i);

        writeImage(output_filename, (vx_image)outbuf->handle);
        tiovx_modules_release_buf(outbuf);
    }

    v4l2_capture_stop(v4l2_capture_handle);
    v4l2_capture_delete_handle(v4l2_capture_handle);

    tiovx_modules_clean_graph(&graph);

    return status;
}
