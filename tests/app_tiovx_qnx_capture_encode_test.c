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
#include <omx_encode_module.h>

#define APP_BUFQ_DEPTH   (6)
#define APP_NUM_CH       (1)

#define NUM_ITERATIONS   (400)

#define VISS_INPUT_WIDTH  1936
#define VISS_INPUT_HEIGHT 1096

#define VISS_OUTPUT_WIDTH  (VISS_INPUT_WIDTH)
#define VISS_OUTPUT_HEIGHT (VISS_INPUT_HEIGHT)

#define LDC_INPUT_WIDTH  VISS_OUTPUT_WIDTH
#define LDC_INPUT_HEIGHT VISS_OUTPUT_HEIGHT

#define LDC_OUTPUT_WIDTH  1920
#define LDC_OUTPUT_HEIGHT 1080

#define LDC_TABLE_WIDTH     (1920)
#define LDC_TABLE_HEIGHT    (1080)
#define LDC_DS_FACTOR       (2)
#define LDC_BLOCK_WIDTH     (64)
#define LDC_BLOCK_HEIGHT    (32)
#define LDC_PIXEL_PAD       (1)

#define SENSOR_NAME "SENSOR_SONY_IMX390_UB953_D3"
#define DCC_VISS TIOVX_MODULES_IMAGING_PATH"/imx390/linear/dcc_viss.bin"
#define DCC_LDC TIOVX_MODULES_IMAGING_PATH"/imx390/linear/dcc_ldc.bin"

vx_status app_modules_qnx_capture_encode_test(vx_int32 argc, vx_char* argv[])
{
    vx_status status = VX_FAILURE;
    GraphObj graph;
    NodeObj *capture_node = NULL, *tee_node = NULL, *viss_node = NULL, *ldc_node = NULL;
    TIOVXCaptureNodeCfg capture_cfg;
    TIOVXTeeNodeCfg tee_cfg;
    TIOVXVissNodeCfg viss_cfg;
    TIOVXLdcNodeCfg ldc_cfg;
    omxEncodeCfg omx_encode_cfg;
    omxEncodeHandle *omx_encode_handle;
    BufPool *in_buf_pool = NULL, *out_buf_pool = NULL;
    Buf *inbuf = NULL, *outbuf = NULL;
    int32_t frame_count = 0;

    status = tiovx_modules_initialize_graph(&graph);
    graph.schedule_mode = VX_GRAPH_SCHEDULE_MODE_QUEUE_AUTO;

    /* Capture */
    tiovx_capture_init_cfg(&capture_cfg);

    sprintf(capture_cfg.sensor_name, SENSOR_NAME);
    capture_cfg.ch_mask = 1;
    capture_cfg.enable_error_detection = 0;

    capture_node = tiovx_modules_add_node(&graph, TIOVX_CAPTURE, (void *)&capture_cfg);

    /* TEE */
    tiovx_tee_init_cfg(&tee_cfg);

    tee_cfg.peer_pad = &capture_node->srcs[0];
    tee_cfg.num_outputs = 2;

    tee_node = tiovx_modules_add_node(&graph, TIOVX_TEE, (void *)&tee_cfg);

    tee_node->srcs[0].bufq_depth = APP_BUFQ_DEPTH; /* This must be greater than 3 */

    /* VISS */
    tiovx_viss_init_cfg(&viss_cfg);

    sprintf(viss_cfg.sensor_name, SENSOR_NAME);
    snprintf(viss_cfg.dcc_config_file, TIVX_FILEIO_FILE_PATH_LENGTH, "%s", DCC_VISS);
    viss_cfg.width = VISS_INPUT_WIDTH;
    viss_cfg.height = VISS_INPUT_HEIGHT;
    sprintf(viss_cfg.target_string, TIVX_TARGET_VPAC_VISS1);

    viss_cfg.input_cfg.params.format[0].pixel_container = TIVX_RAW_IMAGE_16_BIT;
    viss_cfg.input_cfg.params.format[0].msb = 11;

    viss_node = tiovx_modules_add_node(&graph, TIOVX_VISS, (void *)&viss_cfg);

    /* LDC */
    tiovx_ldc_init_cfg(&ldc_cfg);

    sprintf(ldc_cfg.sensor_name, SENSOR_NAME);
    snprintf(ldc_cfg.dcc_config_file, TIVX_FILEIO_FILE_PATH_LENGTH, "%s", DCC_LDC);
    ldc_cfg.ldc_mode = TIOVX_MODULE_LDC_OP_MODE_DCC_DATA;

    ldc_cfg.input_cfg.color_format = VX_DF_IMAGE_NV12;
    ldc_cfg.input_cfg.width = LDC_INPUT_WIDTH;
    ldc_cfg.input_cfg.height = LDC_INPUT_HEIGHT;

    ldc_cfg.output_cfgs[0].color_format = VX_DF_IMAGE_NV12;
    ldc_cfg.output_cfgs[0].width = LDC_OUTPUT_WIDTH;
    ldc_cfg.output_cfgs[0].height = LDC_OUTPUT_HEIGHT;

    ldc_cfg.init_x = 0;
    ldc_cfg.init_y = 0;
    ldc_cfg.table_width = LDC_TABLE_WIDTH;
    ldc_cfg.table_height = LDC_TABLE_HEIGHT;
    ldc_cfg.ds_factor = LDC_DS_FACTOR;
    ldc_cfg.out_block_width = LDC_BLOCK_WIDTH;
    ldc_cfg.out_block_height = LDC_BLOCK_HEIGHT;
    ldc_cfg.pixel_pad = LDC_PIXEL_PAD;

    sprintf(ldc_cfg.target_string, TIVX_TARGET_VPAC_LDC1);

    ldc_node = tiovx_modules_add_node(&graph, TIOVX_LDC, (void *)&ldc_cfg);
    ldc_node->srcs[0].bufq_depth = APP_BUFQ_DEPTH;

    /* Link Nodes */
    tiovx_modules_link_pads(&tee_node->srcs[1], &viss_node->sinks[0]);
    tiovx_modules_link_pads(&viss_node->srcs[0], &ldc_node->sinks[0]);

    status = tiovx_modules_verify_graph(&graph);

    /* Encode handle */
    omx_encode_init_cfg(&omx_encode_cfg);
    omx_encode_cfg.width = LDC_OUTPUT_WIDTH;
    omx_encode_cfg.height = LDC_OUTPUT_HEIGHT;
    omx_encode_cfg.bufq_depth = APP_BUFQ_DEPTH;
    sprintf(omx_encode_cfg.file, "%s/output/qnx_encode.h264", EDGEAI_DATA_PATH);

    omx_encode_handle = omx_encode_create_handle(&omx_encode_cfg);

    in_buf_pool = tee_node->srcs[0].buf_pool;
    out_buf_pool = ldc_node->srcs[0].buf_pool;

    for (int32_t i = 0; i < out_buf_pool->bufq_depth; i++)
    {
        outbuf = tiovx_modules_acquire_buf(out_buf_pool);
        omx_encode_enqueue_buf(omx_encode_handle, outbuf);
    }

    omx_encode_start(omx_encode_handle);

    for (int32_t i = 0; i < in_buf_pool->bufq_depth; i++)
    {
        inbuf = tiovx_modules_acquire_buf(in_buf_pool);
        outbuf = omx_encode_dqueue_buf(omx_encode_handle);
        tiovx_modules_enqueue_buf(inbuf);
        tiovx_modules_enqueue_buf(outbuf);
    }

    while (frame_count++ < NUM_ITERATIONS)
    {
        inbuf = tiovx_modules_dequeue_buf(in_buf_pool);
        outbuf = tiovx_modules_dequeue_buf(out_buf_pool);
        tiovx_modules_enqueue_buf(inbuf);
        omx_encode_enqueue_buf(omx_encode_handle, outbuf);
        outbuf = omx_encode_dqueue_buf(omx_encode_handle);
        tiovx_modules_enqueue_buf(outbuf);
    }

    for (int32_t i = 0; i < out_buf_pool->bufq_depth - 2; i++)
    {
        tiovx_modules_dequeue_buf(out_buf_pool);
        tiovx_modules_dequeue_buf(in_buf_pool);
    }

    tiovx_modules_clean_graph(&graph);

    return status;
}
