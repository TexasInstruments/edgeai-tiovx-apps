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

#define APP_BUFQ_DEPTH   (1)
#define APP_NUM_CH       (1)

#define INPUT_WIDTH  (1920)
#define INPUT_HEIGHT (1080)

#define SENSOR_NAME "SENSOR_SONY_IMX219_RPI"
#define DCC_VISS TIOVX_MODULES_IMAGING_PATH"/imx219/linear/dcc_viss.bin"

vx_status app_modules_viss_test(vx_int32 argc, vx_char* argv[])
{
    vx_status status = VX_FAILURE;
    GraphObj graph;
    NodeObj *node = NULL;
    TIOVXVissNodeCfg cfg;
    BufPool *in_buf_pool = NULL, *out_buf_pool = NULL;
    Buf *inbuf = NULL, *out_buf = NULL;
    char input_filename[100];
    char output_filename[100];
    vx_uint32 bytes_read;

    sprintf(input_filename, "%s/raw_images/modules_test/imx219_1920x1080_capture.raw", EDGEAI_DATA_PATH);
    sprintf(output_filename, "%s/output/imx219_1920x1080_capture_nv12.yuv", EDGEAI_DATA_PATH);
#ifndef SOC_J721E
    BufPool *histogram_buf_pool = NULL;
    Buf *histogram_buf = NULL;
    char output_distribution_name[100];
    sprintf(output_distribution_name, "%s/output/imx219_1920x1080_capture_distribution.bin", EDGEAI_DATA_PATH);
#endif

    tiovx_viss_init_cfg(&cfg);

    sprintf(cfg.sensor_name, SENSOR_NAME);
    snprintf(cfg.dcc_config_file, TIVX_FILEIO_FILE_PATH_LENGTH, "%s", DCC_VISS);
    cfg.width = INPUT_WIDTH;
    cfg.height = INPUT_HEIGHT;
#ifndef SOC_J721E
    cfg.enable_histogram_pad = vx_true_e;
#endif
    sprintf(cfg.target_string, TIVX_TARGET_VPAC_VISS1);

    cfg.input_cfg.params.format[0].pixel_container = TIVX_RAW_IMAGE_8_BIT;
    cfg.input_cfg.params.format[0].msb = 7;

    status = tiovx_modules_initialize_graph(&graph);
    node = tiovx_modules_add_node(&graph, TIOVX_VISS, (void *)&cfg);
    status = tiovx_modules_verify_graph(&graph);

    in_buf_pool = node->sinks[0].buf_pool;
    inbuf = tiovx_modules_acquire_buf(in_buf_pool);
    readRawImage(input_filename, (tivx_raw_image)inbuf->handle, &bytes_read);
    tiovx_modules_enqueue_buf(inbuf);

    out_buf_pool = node->srcs[0].buf_pool;
    out_buf = tiovx_modules_acquire_buf(out_buf_pool);
    tiovx_modules_enqueue_buf(out_buf);
#ifndef SOC_J721E
    histogram_buf_pool = node->srcs[1].buf_pool;
    histogram_buf = tiovx_modules_acquire_buf(histogram_buf_pool);
    tiovx_modules_enqueue_buf(histogram_buf);
#endif

    tiovx_modules_schedule_graph(&graph);
    tiovx_modules_wait_graph(&graph);

    inbuf = tiovx_modules_dequeue_buf(in_buf_pool);
    out_buf = tiovx_modules_dequeue_buf(out_buf_pool);
#ifndef SOC_J721E
    histogram_buf = tiovx_modules_dequeue_buf(histogram_buf_pool);
#endif

    writeImage(output_filename, (vx_image)out_buf->handle);
#ifndef SOC_J721E
    writeDistribution(output_distribution_name, (vx_distribution)histogram_buf->handle);
#endif
    tiovx_modules_release_buf(inbuf);
    tiovx_modules_release_buf(out_buf);
#ifndef SOC_J721E
    tiovx_modules_release_buf(histogram_buf);
#endif
    tiovx_modules_clean_graph(&graph);

    return status;
}
