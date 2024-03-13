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

/*
 * Pipeline:
 *
 * Input(rggb12 1936x1096) --> TEE0 ----> VISS0 ----> LDC ----> MSC0 ---> TEE1 ---> Output0 (NV12 640x480)
 *                                  |                                          |
 *                                  |                                          ---> CC0 ---> Output1 (RGB 640x480)
 *                                  |
 *                                  ----> VISS1 ----> TEE2 ----> MSC1 ---> Output2 (NV12 960x540)
 *                                                         |
 *                                                         ----> CC1 ---> Output3 (I420 1936x1096)
 */

#include <tiovx_modules.h>
#include <tiovx_utils.h>

#define APP_BUFQ_DEPTH   (1)
#define APP_NUM_CH       (1)

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
#define DCC_VISS "/opt/imaging/imx390/dcc_viss.bin"
#define DCC_LDC "/opt/imaging/imx390/dcc_ldc.bin"

vx_status app_modules_tee_test(vx_int32 argc, vx_char* argv[])
{
    vx_status status = VX_FAILURE;
    GraphObj graph;
    Pad *input_pad = NULL, *input_pad2 = NULL, *output_pad[4] = {NULL, NULL, NULL, NULL};
    Buf *inbuf = NULL, *outbuf[4] = {NULL, NULL, NULL, NULL};
    char input_filename[100];
    char output_filename[4][100];
    vx_uint32 bytes_read;

    sprintf(input_filename, "%s/raw_images//imx390_raw_image_1936x1096_16bpp_exp0.raw", EDGEAI_DATA_PATH);
    sprintf(output_filename[0], "%s/output/tee_test_out_nv12_640x480.yuv", EDGEAI_DATA_PATH);
    sprintf(output_filename[1], "%s/output/tee_test_out_rgb_640x480.rgb", EDGEAI_DATA_PATH);
    sprintf(output_filename[2], "%s/output/tee_test_out_nv12_960x540.yuv", EDGEAI_DATA_PATH);
    sprintf(output_filename[3], "%s/output/tee_test_out_i420_1936x1080.yuv", EDGEAI_DATA_PATH);

    status = tiovx_modules_initialize_graph(&graph);

    {
        TIOVXVissNodeCfg viss_cfg;
        NodeObj *viss_node;

        /* VISS0 */
        tiovx_viss_init_cfg(&viss_cfg);

        sprintf(viss_cfg.sensor_name, SENSOR_NAME);
        snprintf(viss_cfg.dcc_config_file, TIVX_FILEIO_FILE_PATH_LENGTH, "%s", DCC_VISS);
        viss_cfg.width = VISS_INPUT_WIDTH;
        viss_cfg.height = VISS_INPUT_HEIGHT;
        sprintf(viss_cfg.target_string, TIVX_TARGET_VPAC_VISS1);

        viss_cfg.input_cfg.params.format[0].pixel_container = TIVX_RAW_IMAGE_16_BIT;
        viss_cfg.input_cfg.params.format[0].msb = 11;

        viss_node = tiovx_modules_add_node(&graph, TIOVX_VISS, (void *)&viss_cfg);
        output_pad[0] = &viss_node->srcs[0];
        input_pad = &viss_node->sinks[0];

        /* VISS1 */
        viss_node = tiovx_modules_add_node(&graph, TIOVX_VISS, (void *)&viss_cfg);
        output_pad[2] = &viss_node->srcs[0];
        input_pad2 = &viss_node->sinks[0];
    }

    {
        TIOVXTeeNodeCfg tee_cfg;
        NodeObj *tee_node;

        /* TEE0 */
        tiovx_tee_init_cfg(&tee_cfg);

        tee_cfg.peer_pad = input_pad;
        tee_cfg.num_outputs = 2;

        /* Create TEE0 and Link TEE0 to VISS0 */
        tee_node = tiovx_modules_add_node(&graph, TIOVX_TEE, (void *)&tee_cfg);
        input_pad = &tee_node->sinks[0];

        /* Link TEE0 to VISS1 */
        tiovx_modules_link_pads(&tee_node->srcs[1], input_pad2);
    }

    {
        /* LDC */
        TIOVXLdcNodeCfg ldc_cfg;
        NodeObj *ldc_node;

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

        /* Link VISS0 to LDC */
        tiovx_modules_link_pads(output_pad[0], &ldc_node->sinks[0]);
        output_pad[0] = &ldc_node->srcs[0];
    }

    {
        /* MSC0 */
        TIOVXMultiScalerNodeCfg msc_cfg;
        NodeObj *msc_node;

        tiovx_multi_scaler_init_cfg(&msc_cfg);

        msc_cfg.color_format = VX_DF_IMAGE_NV12;
        msc_cfg.num_outputs = 1;

        msc_cfg.input_cfg.width = LDC_OUTPUT_WIDTH;
        msc_cfg.input_cfg.height = LDC_OUTPUT_HEIGHT;

        msc_cfg.output_cfgs[0].width = 640;
        msc_cfg.output_cfgs[0].height = 480;

        sprintf(msc_cfg.target_string, TIVX_TARGET_VPAC_MSC1);
        tiovx_multi_scaler_module_crop_params_init(&msc_cfg);

        msc_node = tiovx_modules_add_node(&graph, TIOVX_MULTI_SCALER, (void *)&msc_cfg);

        /* Link LDC to MSC */
        tiovx_modules_link_pads(output_pad[0], &msc_node->sinks[0]);
        output_pad[0] = &msc_node->srcs[0];
    }

    {
        TIOVXTeeNodeCfg tee_cfg;
        NodeObj *tee_node;

        /* TEE1 */
        tiovx_tee_init_cfg(&tee_cfg);

        tee_cfg.peer_pad = output_pad[0];
        tee_cfg.num_outputs = 2;

        tee_node = tiovx_modules_add_node(&graph, TIOVX_TEE, (void *)&tee_cfg);
        output_pad[0] = &tee_node->srcs[0];
        output_pad[1] = &tee_node->srcs[1];
    }

    {
        TIOVXDLColorConvertNodeCfg cc_cfg;
        NodeObj *cc_node;

        /* CC0 */
        tiovx_dl_color_convert_init_cfg(&cc_cfg);

        cc_cfg.width = 640;
        cc_cfg.height = 480;
        cc_cfg.input_cfg.color_format = VX_DF_IMAGE_NV12;
        cc_cfg.output_cfg.color_format = VX_DF_IMAGE_RGB;
        sprintf(cc_cfg.target_string, TIVX_TARGET_A72_0);

        cc_node = tiovx_modules_add_node(&graph, TIOVX_DL_COLOR_CONVERT, (void *)&cc_cfg);

        /* Link TEE1 to CC0 */
        tiovx_modules_link_pads(output_pad[1], &cc_node->sinks[0]);
        output_pad[1] = &cc_node->srcs[0];
    }

    {
        TIOVXTeeNodeCfg tee_cfg;
        NodeObj *tee_node;

        /* TEE2 */
        tiovx_tee_init_cfg(&tee_cfg);

        tee_cfg.peer_pad = output_pad[2];
        tee_cfg.num_outputs = 2;

        tee_node = tiovx_modules_add_node(&graph, TIOVX_TEE, (void *)&tee_cfg);
        output_pad[2] = &tee_node->srcs[0];
        output_pad[3] = &tee_node->srcs[1];
    }

    {
        /* MSC1 */
        TIOVXMultiScalerNodeCfg msc_cfg;
        NodeObj *msc_node;

        tiovx_multi_scaler_init_cfg(&msc_cfg);

        msc_cfg.color_format = VX_DF_IMAGE_NV12;
        msc_cfg.num_outputs = 1;

        msc_cfg.input_cfg.width = VISS_OUTPUT_WIDTH;
        msc_cfg.input_cfg.height = VISS_OUTPUT_HEIGHT;

        msc_cfg.output_cfgs[0].width = 960;
        msc_cfg.output_cfgs[0].height = 540;

        sprintf(msc_cfg.target_string, TIVX_TARGET_VPAC_MSC1);
        tiovx_multi_scaler_module_crop_params_init(&msc_cfg);

        msc_node = tiovx_modules_add_node(&graph, TIOVX_MULTI_SCALER, (void *)&msc_cfg);

        /* Link TEE2 to MSC */
        tiovx_modules_link_pads(output_pad[2], &msc_node->sinks[0]);
        output_pad[2] = &msc_node->srcs[0];
    }

    /* CC1 */
    {
        TIOVXDLColorConvertNodeCfg cc_cfg;
        NodeObj *cc_node;

        tiovx_dl_color_convert_init_cfg(&cc_cfg);

        cc_cfg.width = VISS_OUTPUT_WIDTH;
        cc_cfg.height = VISS_OUTPUT_HEIGHT;
        cc_cfg.input_cfg.color_format = VX_DF_IMAGE_NV12;
        cc_cfg.output_cfg.color_format = VX_DF_IMAGE_IYUV;
        sprintf(cc_cfg.target_string, TIVX_TARGET_A72_0);

        cc_node = tiovx_modules_add_node(&graph, TIOVX_DL_COLOR_CONVERT, (void *)&cc_cfg);

        /* Link TEE2 to CC1 */
        tiovx_modules_link_pads(output_pad[3], &cc_node->sinks[0]);
        output_pad[3] = &cc_node->srcs[0];
    }

    status = tiovx_modules_verify_graph(&graph);

    inbuf = tiovx_modules_acquire_buf(input_pad->buf_pool);
    readRawImage(input_filename, (tivx_raw_image)inbuf->handle, &bytes_read);
    tiovx_modules_enqueue_buf(inbuf);

    outbuf[0] = tiovx_modules_acquire_buf(output_pad[0]->buf_pool);
    outbuf[1] = tiovx_modules_acquire_buf(output_pad[1]->buf_pool);
    outbuf[2] = tiovx_modules_acquire_buf(output_pad[2]->buf_pool);
    outbuf[3] = tiovx_modules_acquire_buf(output_pad[3]->buf_pool);
    tiovx_modules_enqueue_buf(outbuf[0]);
    tiovx_modules_enqueue_buf(outbuf[1]);
    tiovx_modules_enqueue_buf(outbuf[2]);
    tiovx_modules_enqueue_buf(outbuf[3]);

    tiovx_modules_schedule_graph(&graph);
    tiovx_modules_wait_graph(&graph);

    inbuf = tiovx_modules_dequeue_buf(input_pad->buf_pool);
    outbuf[0] = tiovx_modules_dequeue_buf(output_pad[0]->buf_pool);
    outbuf[1] = tiovx_modules_dequeue_buf(output_pad[1]->buf_pool);
    outbuf[2] = tiovx_modules_dequeue_buf(output_pad[2]->buf_pool);
    outbuf[3] = tiovx_modules_dequeue_buf(output_pad[3]->buf_pool);

    writeImage(output_filename[0], (vx_image)outbuf[0]->handle);
    writeImage(output_filename[1], (vx_image)outbuf[1]->handle);
    writeImage(output_filename[2], (vx_image)outbuf[2]->handle);
    writeImage(output_filename[3], (vx_image)outbuf[3]->handle);

    tiovx_modules_release_buf(inbuf);
    tiovx_modules_release_buf(outbuf[0]);
    tiovx_modules_release_buf(outbuf[1]);
    tiovx_modules_release_buf(outbuf[2]);
    tiovx_modules_release_buf(outbuf[3]);

    tiovx_modules_clean_graph(&graph);

    return status;
}
