/*
 *
 * Copyright (c) 2021 Texas Instruments Incorporated
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

#define INPUT_WIDTH  (1936)
#define INPUT_HEIGHT (1096)

#define OUTPUT_WIDTH  (1920)
#define OUTPUT_HEIGHT (1080)

#define LDC_TABLE_WIDTH     (1920)
#define LDC_TABLE_HEIGHT    (1080)
#define LDC_DS_FACTOR       (2)
#define LDC_BLOCK_WIDTH     (64)
#define LDC_BLOCK_HEIGHT    (32)
#define LDC_PIXEL_PAD       (1)

#define SENSOR_NAME "SENSOR_SONY_IMX390_UB953_D3"
#define DCC_LDC "/opt/imaging/imx390/wdr/dcc_ldc_wdr.bin"

vx_status app_modules_ldc_test(vx_int32 argc, vx_char* argv[])
{
    vx_status status = VX_FAILURE;
    GraphObj graph;
    NodeObj *node = NULL;
    TIOVXLdcNodeCfg cfg;
    BufPool *in_buf_pool = NULL, *out_buf_pool = NULL;
    Buf *inbuf = NULL, *outbuf = NULL;
    char input_filename[100];
    char output_filename[100];

    sprintf(input_filename, "%s/raw_images/modules_test/imx390_fisheye_1936x1096_nv12.yuv", EDGEAI_DATA_PATH);
    sprintf(output_filename, "%s/output/imx390_rectified_1920x1080_nv12.yuv", EDGEAI_DATA_PATH);

    tiovx_ldc_init_cfg(&cfg);

    snprintf(cfg.dcc_config_file, TIVX_FILEIO_FILE_PATH_LENGTH, "%s", DCC_LDC);
    snprintf(cfg.lut_file, TIVX_FILEIO_FILE_PATH_LENGTH,
            "%s/raw_images/modules_test/imx390_ldc_lut_1920x1080.bin",
            EDGEAI_DATA_PATH);

    cfg.ldc_mode = TIOVX_MODULE_LDC_OP_MODE_DCC_DATA; //TIOVX_MODULE_LDC_OP_MODE_MESH_IMAGE

    cfg.input_cfg.color_format = VX_DF_IMAGE_NV12;
    cfg.input_cfg.width = INPUT_WIDTH;
    cfg.input_cfg.height = INPUT_HEIGHT;

    cfg.output_cfgs[0].color_format = VX_DF_IMAGE_NV12;
    cfg.output_cfgs[0].width = OUTPUT_WIDTH;
    cfg.output_cfgs[0].height = OUTPUT_HEIGHT;

    cfg.init_x = 0;
    cfg.init_y = 0;
    cfg.table_width = LDC_TABLE_WIDTH;
    cfg.table_height = LDC_TABLE_HEIGHT;
    cfg.ds_factor = LDC_DS_FACTOR;
    cfg.out_block_width = LDC_BLOCK_WIDTH;
    cfg.out_block_height = LDC_BLOCK_HEIGHT;
    cfg.pixel_pad = LDC_PIXEL_PAD;

    sprintf(cfg.target_string, TIVX_TARGET_VPAC_LDC1);

    status = tiovx_modules_initialize_graph(&graph);
    node = tiovx_modules_add_node(&graph, TIOVX_LDC, (void *)&cfg);
    status = tiovx_modules_verify_graph(&graph);

    in_buf_pool = node->sinks[0].buf_pool;
    inbuf = tiovx_modules_acquire_buf(in_buf_pool);
    readImage(input_filename, (vx_image)inbuf->handle);
    tiovx_modules_enqueue_buf(inbuf);

    out_buf_pool = node->srcs[0].buf_pool;
    outbuf = tiovx_modules_acquire_buf(out_buf_pool);
    tiovx_modules_enqueue_buf(outbuf);

    tiovx_modules_schedule_graph(&graph);
    tiovx_modules_wait_graph(&graph);

    inbuf = tiovx_modules_dequeue_buf(in_buf_pool);
    outbuf = tiovx_modules_dequeue_buf(out_buf_pool);

    writeImage(output_filename, (vx_image)outbuf->handle);

    tiovx_modules_release_buf(inbuf);
    tiovx_modules_release_buf(outbuf);

    tiovx_modules_clean_graph(&graph);

    return status;
}
