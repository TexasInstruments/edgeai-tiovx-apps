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
 * Capture(IMX390 2MP) --> VISS --> LDC --> MSC0 --> MSC1 --> DL_PRE_PROC --> TIDL
 *                                             |                                  |
 *                                             |                                   --> DL_POST_PROC --> Display
 *                                             |                                  |
 *                                             ---------------------------------->
 */

#include <tiovx_modules.h>
#include <tiovx_utils.h>
#include <stdio.h>
#include <unistd.h>

#define NUM_ITERATIONS   (300)

#define APP_BUFQ_DEPTH   (1)
#define APP_NUM_CH       (1)

#define VISS_INPUT_WIDTH  1936
#define VISS_INPUT_HEIGHT 1100

#define VISS_OUTPUT_WIDTH  (VISS_INPUT_WIDTH)
#define VISS_OUTPUT_HEIGHT (VISS_INPUT_HEIGHT)

#define LDC_INPUT_WIDTH  VISS_OUTPUT_WIDTH
#define LDC_INPUT_HEIGHT VISS_OUTPUT_HEIGHT

#define LDC_OUTPUT_WIDTH  1920
#define LDC_OUTPUT_HEIGHT 1080

#define MSC_INPUT_WIDTH  LDC_OUTPUT_WIDTH
#define MSC_INPUT_HEIGHT LDC_OUTPUT_HEIGHT

#define MSC_OUTPUT_WIDTH  MSC_INPUT_WIDTH/2
#define MSC_OUTPUT_HEIGHT MSC_INPUT_HEIGHT/2

#define LDC_TABLE_WIDTH     (1920)
#define LDC_TABLE_HEIGHT    (1080)
#define LDC_DS_FACTOR       (2)
#define LDC_BLOCK_WIDTH     (64)
#define LDC_BLOCK_HEIGHT    (32)
#define LDC_PIXEL_PAD       (1)

#define SENSOR_NAME "SENSOR_SONY_IMX390_UB953_D3"
#define DCC_VISS TIOVX_MODULES_IMAGING_PATH"/imx390/linear/dcc_viss.bin"
#define DCC_LDC TIOVX_MODULES_IMAGING_PATH"/imx390/linear/dcc_ldc.bin"

#define POST_PROC_OUT_WIDTH (1280)
#define POST_PROC_OUT_HEIGHT (720)

#define TIDL_IO_CONFIG_FILE_PATH "/opt/model_zoo/ONR-OD-8200-yolox-nano-lite-mmdet-coco-416x416/artifacts/detslabels_tidl_io_1.bin"
#define TIDL_NETWORK_FILE_PATH "/opt/model_zoo/ONR-OD-8200-yolox-nano-lite-mmdet-coco-416x416/artifacts/detslabels_tidl_net.bin"

static char imgnet_labels3[TIVX_DL_POST_PROC_MAX_NUM_CLASSNAMES][TIVX_DL_POST_PROC_MAX_SIZE_CLASSNAME] =
{
  "Not Available"
};

static int32_t label_offset3[TIVX_DL_POST_PROC_MAX_NUM_CLASSNAMES] = {0};

vx_status app_modules_capture_dl_display_test(int argc, char* argv[])
{
    vx_status status = VX_FAILURE;
    GraphObj graph;
    Pad *input_pad = NULL, *output_pad[4] = {NULL, NULL, NULL, NULL}, *post_proc_in_img_pad = NULL;
    Buf *inbuf = NULL;

    status = tiovx_modules_initialize_graph(&graph);
    graph.schedule_mode = VX_GRAPH_SCHEDULE_MODE_QUEUE_AUTO;

    /* Capture */
    {
        TIOVXCaptureNodeCfg capture_cfg;
        NodeObj *capture_node;

        tiovx_capture_init_cfg(&capture_cfg);

        capture_cfg.ch_mask = 1;

        capture_node = tiovx_modules_add_node(&graph, TIOVX_CAPTURE, (void *)&capture_cfg);
        input_pad = &capture_node->srcs[0];
    }

    /* TEE */
    {
        TIOVXTeeNodeCfg tee_cfg;
        NodeObj *tee_node;

        tiovx_tee_init_cfg(&tee_cfg);

        tee_cfg.peer_pad = input_pad;
        tee_cfg.num_outputs = 2;

        tee_node = tiovx_modules_add_node(&graph, TIOVX_TEE, (void *)&tee_cfg);

        tee_node->srcs[0].bufq_depth = 4; /* This must be greater than 3 */

        input_pad = &tee_node->srcs[0]; // This is for enqueing
        output_pad[0] = &tee_node->srcs[1]; // This will go to viss
    }

    /* VISS */
    {
        TIOVXVissNodeCfg viss_cfg;
        NodeObj *viss_node;

        tiovx_viss_init_cfg(&viss_cfg);

        sprintf(viss_cfg.sensor_name, SENSOR_NAME);
        snprintf(viss_cfg.dcc_config_file, TIVX_FILEIO_FILE_PATH_LENGTH, "%s", DCC_VISS);
        viss_cfg.width = VISS_INPUT_WIDTH;
        viss_cfg.height = VISS_INPUT_HEIGHT;
        sprintf(viss_cfg.target_string, TIVX_TARGET_VPAC_VISS1);
        viss_cfg.enable_h3a_pad = vx_true_e;

        viss_cfg.input_cfg.params.format[0].pixel_container = TIVX_RAW_IMAGE_16_BIT;
        viss_cfg.input_cfg.params.format[0].msb = 11;

        viss_node = tiovx_modules_add_node(&graph, TIOVX_VISS, (void *)&viss_cfg);

        /* Link TEE0 to VISS1 */
        tiovx_modules_link_pads(output_pad[0], &viss_node->sinks[0]);

        output_pad[0] = &viss_node->srcs[0];
        output_pad[1] = &viss_node->srcs[1];
    }

    /* AEWB */
    {
        TIOVXAewbNodeCfg aewb_cfg;
        NodeObj *aewb_node;

        tiovx_aewb_init_cfg(&aewb_cfg);

        sprintf(aewb_cfg.sensor_name, SENSOR_NAME);
        aewb_cfg.ch_mask = 1;
        aewb_cfg.awb_mode = ALGORITHMS_ISS_AWB_AUTO;
        aewb_cfg.awb_num_skip_frames = 9;
        aewb_cfg.ae_num_skip_frames  = 9;

        aewb_node = tiovx_modules_add_node(&graph, TIOVX_AEWB, (void *)&aewb_cfg);

        /* Link VISS to AEWB */
        tiovx_modules_link_pads(output_pad[1], &aewb_node->sinks[0]);
    }

    /* LDC */
    {
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

    /* MSC0 */
    {
        TIOVXMultiScalerNodeCfg msc_cfg;
        NodeObj *msc_node;

        tiovx_multi_scaler_init_cfg(&msc_cfg);

        msc_cfg.color_format = VX_DF_IMAGE_NV12;
        msc_cfg.num_outputs = 2;

        msc_cfg.input_cfg.width = LDC_OUTPUT_WIDTH;
        msc_cfg.input_cfg.height = LDC_OUTPUT_HEIGHT;

        // This is for pre proc
        msc_cfg.output_cfgs[0].width = LDC_OUTPUT_WIDTH / 2;
        msc_cfg.output_cfgs[0].height = LDC_OUTPUT_HEIGHT / 2;

        msc_cfg.output_cfgs[1].width = POST_PROC_OUT_WIDTH;
        msc_cfg.output_cfgs[1].height = POST_PROC_OUT_HEIGHT;

        sprintf(msc_cfg.target_string, TIVX_TARGET_VPAC_MSC1);
        tiovx_multi_scaler_module_crop_params_init(&msc_cfg);

        msc_node = tiovx_modules_add_node(&graph, TIOVX_MULTI_SCALER, (void *)&msc_cfg);

        /* Link LDC to MSC0 */
        tiovx_modules_link_pads(output_pad[0], &msc_node->sinks[0]);
        output_pad[0] = &msc_node->srcs[0];
        post_proc_in_img_pad = &msc_node->srcs[1];
    }

    /* MSC1 */
    {
        TIOVXMultiScalerNodeCfg msc_cfg;
        NodeObj *msc_node;

        tiovx_multi_scaler_init_cfg(&msc_cfg);

        msc_cfg.color_format = VX_DF_IMAGE_NV12;
        msc_cfg.num_outputs = 1;

        msc_cfg.input_cfg.width = LDC_OUTPUT_WIDTH / 2;
        msc_cfg.input_cfg.height = LDC_OUTPUT_HEIGHT / 2;

        msc_cfg.output_cfgs[0].width = 416;
        msc_cfg.output_cfgs[0].height = 416;

        sprintf(msc_cfg.target_string, TIVX_TARGET_VPAC_MSC1);
        tiovx_multi_scaler_module_crop_params_init(&msc_cfg);

        msc_node = tiovx_modules_add_node(&graph, TIOVX_MULTI_SCALER, (void *)&msc_cfg);

        /* Link MSC0 to MSC1 */
        tiovx_modules_link_pads(output_pad[0], &msc_node->sinks[0]);
        output_pad[0] = &msc_node->srcs[0];
    }

    /* DL Pre Proc */
    {
        TIOVXDLPreProcNodeCfg dl_pre_proc_cfg;
        NodeObj *dl_pre_proc_node;

        tiovx_dl_pre_proc_init_cfg(&dl_pre_proc_cfg);

        dl_pre_proc_cfg.io_config_path = TIDL_IO_CONFIG_FILE_PATH;
        dl_pre_proc_cfg.params.tensor_format = 1; //BGR

        dl_pre_proc_node = tiovx_modules_add_node(&graph, TIOVX_DL_PRE_PROC, (void *)&dl_pre_proc_cfg);

        /* Link MSC1 to DLPreProc */
        tiovx_modules_link_pads(output_pad[0], &dl_pre_proc_node->sinks[0]);
        output_pad[0] = &dl_pre_proc_node->srcs[0];
    }

    /* TIDL */
    {
        TIOVXTIDLNodeCfg tidl_cfg;
        NodeObj *tidl_node;

        tiovx_tidl_init_cfg(&tidl_cfg);

        tidl_cfg.io_config_path = TIDL_IO_CONFIG_FILE_PATH;
        tidl_cfg.network_path = TIDL_NETWORK_FILE_PATH;

        tidl_node = tiovx_modules_add_node(&graph, TIOVX_TIDL, (void *)&tidl_cfg);

        /* Link DLPreProc to TIDL */
        tiovx_modules_link_pads(output_pad[0], &tidl_node->sinks[0]);

        for (int32_t i = 0; i < tidl_node->num_outputs; i++)
        {
            output_pad[i] = &tidl_node->srcs[i];
        }
    }

    /* DL Post Proc */
    {
        TIOVXDLPostProcNodeCfg dl_post_proc_cfg;
        NodeObj *dl_post_proc_node;

        tiovx_dl_post_proc_init_cfg(&dl_post_proc_cfg);

        dl_post_proc_cfg.width = POST_PROC_OUT_WIDTH;
        dl_post_proc_cfg.height = POST_PROC_OUT_HEIGHT;

        dl_post_proc_cfg.params.task_type = TIVX_DL_POST_PROC_DETECTION_TASK_TYPE;
        dl_post_proc_cfg.params.od_prms.viz_th = 0.5;

        dl_post_proc_cfg.params.od_prms.formatter[0] = 0;
        dl_post_proc_cfg.params.od_prms.formatter[1] = 1;
        dl_post_proc_cfg.params.od_prms.formatter[2] = 2;
        dl_post_proc_cfg.params.od_prms.formatter[3] = 3;
        dl_post_proc_cfg.params.od_prms.formatter[4] = 5;
        dl_post_proc_cfg.params.od_prms.formatter[5] = 4;

        dl_post_proc_cfg.params.od_prms.scaleX = POST_PROC_OUT_WIDTH / 416.0;
        dl_post_proc_cfg.params.od_prms.scaleY = POST_PROC_OUT_HEIGHT / 416.0;

        dl_post_proc_cfg.params.od_prms.labelIndexOffset = 0;

        dl_post_proc_cfg.params.od_prms.labelOffset = label_offset3;
        dl_post_proc_cfg.params.od_prms.classnames = imgnet_labels3;

        dl_post_proc_cfg.io_config_path = TIDL_IO_CONFIG_FILE_PATH;

        dl_post_proc_node = tiovx_modules_add_node(&graph, TIOVX_DL_POST_PROC, (void *)&dl_post_proc_cfg);

        /* Link MSC0 and TIDL to DL Post Proc  */
        for (int32_t i = 0; i < dl_post_proc_node->num_inputs - 1; i++)
        {
            tiovx_modules_link_pads(output_pad[i], &dl_post_proc_node->sinks[i]);
        }

        tiovx_modules_link_pads(post_proc_in_img_pad, &dl_post_proc_node->sinks[dl_post_proc_node->num_inputs - 1]);

        output_pad[0] = &dl_post_proc_node->srcs[0];

    }

    /* Display */
    {
        TIOVXDisplayNodeCfg display_cfg;
        NodeObj *display_node;

        tiovx_display_init_cfg(&display_cfg);

        display_cfg.width = POST_PROC_OUT_WIDTH;
        display_cfg.height = POST_PROC_OUT_HEIGHT;
        display_cfg.params.outWidth  = POST_PROC_OUT_WIDTH;
        display_cfg.params.outHeight = POST_PROC_OUT_HEIGHT;
        display_cfg.params.posX = (1920 - POST_PROC_OUT_WIDTH)/2;
        display_cfg.params.posY = (1080 - POST_PROC_OUT_HEIGHT)/2;

        display_node = tiovx_modules_add_node(&graph, TIOVX_DISPLAY, (void *)&display_cfg);

        /* Link DL Post Proc to Display */
        tiovx_modules_link_pads(output_pad[0], &display_node->sinks[0]);
    }

    status = tiovx_modules_verify_graph(&graph);

    for (int32_t i = 0; i < input_pad->buf_pool->bufq_depth; i++)
    {
        inbuf = tiovx_modules_acquire_buf(input_pad->buf_pool);
        tiovx_modules_enqueue_buf(inbuf);
    }

    int frame_count = 0;
    while (frame_count++ < NUM_ITERATIONS)
    {
        inbuf = tiovx_modules_dequeue_buf(input_pad->buf_pool);
        tiovx_modules_enqueue_buf(inbuf);
    }

    for (int32_t i = 0; i < 2; i++)
    {
        inbuf = tiovx_modules_dequeue_buf(input_pad->buf_pool);
        tiovx_modules_release_buf(inbuf);
    }

    tiovx_modules_clean_graph(&graph);

    return status;
}
