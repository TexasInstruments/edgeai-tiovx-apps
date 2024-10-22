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
#include <edgeai_nv12_drawing_utils.h>

#if defined(SOC_AM62A) && defined(TARGET_OS_QNX)
#include "qnx_display_module.h"
#endif

#define NUM_ITERATIONS   (300)

#define APP_BUFQ_DEPTH   (1)
#define APP_NUM_CH       (1)

#if !(defined(SOC_AM62A) && defined(TARGET_OS_QNX))
#define VISS_INPUT_WIDTH  1936
#define VISS_INPUT_HEIGHT 1100

#else
#define VISS_INPUT_WIDTH  1936
#define VISS_INPUT_HEIGHT 1096
#endif

#define VISS_OUTPUT_WIDTH  (VISS_INPUT_WIDTH)
#define VISS_OUTPUT_HEIGHT (VISS_INPUT_HEIGHT)

#define LDC_INPUT_WIDTH  VISS_OUTPUT_WIDTH
#define LDC_INPUT_HEIGHT VISS_OUTPUT_HEIGHT

#define LDC_OUTPUT_WIDTH  1920
#define LDC_OUTPUT_HEIGHT 1080

#define MSC_INPUT_WIDTH  LDC_OUTPUT_WIDTH
#define MSC_INPUT_HEIGHT LDC_OUTPUT_HEIGHT

#if !(defined(SOC_AM62A) && defined(TARGET_OS_QNX))
#define MSC_OUTPUT_WIDTH  MSC_INPUT_WIDTH/2
#define MSC_OUTPUT_HEIGHT MSC_INPUT_HEIGHT/2

#else
#define MSC_OUTPUT_WIDTH  MSC_INPUT_WIDTH
#define MSC_OUTPUT_HEIGHT MSC_INPUT_HEIGHT
#endif

#define LDC_TABLE_WIDTH     (1920)
#define LDC_TABLE_HEIGHT    (1080)
#define LDC_DS_FACTOR       (2)
#define LDC_BLOCK_WIDTH     (64)
#define LDC_BLOCK_HEIGHT    (32)
#define LDC_PIXEL_PAD       (1)

#define SENSOR_NAME "SENSOR_SONY_IMX390_UB953_D3"
#define DCC_VISS TIOVX_MODULES_IMAGING_PATH"/imx390/linear/dcc_viss.bin"
#define DCC_LDC TIOVX_MODULES_IMAGING_PATH"/imx390/linear/dcc_ldc.bin"

#if !(defined(SOC_AM62A) && defined(TARGET_OS_QNX))
#define POST_PROC_OUT_WIDTH (1280)
#define POST_PROC_OUT_HEIGHT (720)

#else
#define POST_PROC_OUT_WIDTH   (1920)
#define POST_PROC_OUT_HEIGHT  (1080)
#endif

#if !(defined(SOC_AM62A) && defined(TARGET_OS_QNX))
#define TIDL_IO_CONFIG_FILE_PATH "/opt/model_zoo/TFL-SS-2580-deeplabv3_mobv2-ade20k32-mlperf-512x512/artifacts/201_tidl_io_1.bin"
#define TIDL_NETWORK_FILE_PATH "/opt/model_zoo/TFL-SS-2580-deeplabv3_mobv2-ade20k32-mlperf-512x512/artifacts/201_tidl_net.bin"

#else 
#define TIDL_IO_CONFIG_FILE_PATH "/ti_fs/model_zoo/TFL-SS-2580-deeplabv3_mobv2-ade20k32-mlperf-512x512/artifacts/201_tidl_io_1.bin"
#define TIDL_NETWORK_FILE_PATH "/ti_fs/model_zoo/TFL-SS-2580-deeplabv3_mobv2-ade20k32-mlperf-512x512/artifacts/201_tidl_net.bin"
#endif

vx_status app_modules_capture_dl_segmentation_display_test(vx_int32 argc, vx_char* argv[])
{
    vx_status status = VX_FAILURE;
    GraphObj graph;
    NodeObj *capture_node = NULL, *tee_node = NULL, *viss_node = NULL, *ldc_node = NULL, *msc_node = NULL, *display_node = NULL, *aewb_node = NULL;
    TIOVXCaptureNodeCfg capture_cfg;
    TIOVXTeeNodeCfg tee_cfg;
    TIOVXVissNodeCfg viss_cfg;
    TIOVXLdcNodeCfg ldc_cfg;
    TIOVXMultiScalerNodeCfg msc_cfg;
    BufPool *in_buf_pool = NULL;
    Buf *inbuf = NULL;
    #if defined(SOC_AM62A) && defined(TARGET_OS_QNX)
    BufPool *out_buf_pool = NULL; 
    Buf *outbuf = NULL;
    #endif
    int32_t frame_count = 0;
    Pad *input_pad =NULL, *output_pad[4] = {NULL, NULL, NULL, NULL};  

    /* RGB Color Map for Semantic Segmentation */
static uint8_t RGB_COLOR_MAP[26][3] = {{255,0,0},{0,255,0},{73,102,92},
                                       {255,255,0},{0,255,255},{0,99,245},
                                       {255,127,0},{0,255,100},{235,117,117},
                                       {242,15,109},{118,194,167},{255,0,255},
                                       {231,200,255},{255,186,239},{0,110,0},
                                       {0,0,255},{110,0,0},{110,110,0},
                                       {100,0,50},{90,60,0},{255,255,255} ,
                                       {170,0,255},{204,255,0},{78,69,128},
                                       {133,133,74},{0,0,110}};
   
    status = tiovx_modules_initialize_graph(&graph);
    graph.schedule_mode = VX_GRAPH_SCHEDULE_MODE_QUEUE_AUTO;

    /* Capture */
    tiovx_capture_init_cfg(&capture_cfg);

    sprintf(capture_cfg.sensor_name, SENSOR_NAME);
    capture_cfg.ch_mask = 1;
    capture_cfg.enable_error_detection = 0;
    #if defined(SOC_AM62A) && defined(TARGET_OS_QNX)
    sprintf(capture_cfg.target_string, TIVX_TARGET_CAPTURE2);
    #endif

    capture_node = tiovx_modules_add_node(&graph, TIOVX_CAPTURE, (void *)&capture_cfg);
    input_pad = &capture_node->srcs[0];

    /* TEE */
    tiovx_tee_init_cfg(&tee_cfg);

    tee_cfg.peer_pad = &capture_node->srcs[0];
    tee_cfg.num_outputs = 2;

    tee_node = tiovx_modules_add_node(&graph, TIOVX_TEE, (void *)&tee_cfg);

    tee_node->srcs[0].bufq_depth = 4; /* This must be greater than 3 */
    input_pad = &tee_node->srcs[0]; // This is for enqueing

    /* VISS */
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

    output_pad[0] = &viss_node->srcs[0];
    output_pad[1] = &viss_node->srcs[1];

    /* AEWB */

    TIOVXAewbNodeCfg aewb_cfg;
    tiovx_aewb_init_cfg(&aewb_cfg);

    sprintf(aewb_cfg.sensor_name, SENSOR_NAME);
    aewb_cfg.ch_mask = 1;
    aewb_cfg.awb_mode = ALGORITHMS_ISS_AWB_AUTO;
    aewb_cfg.awb_num_skip_frames = 0;
    aewb_cfg.ae_num_skip_frames  = 0;

    aewb_node = tiovx_modules_add_node(&graph, TIOVX_AEWB, (void *)&aewb_cfg);


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

    /* DL Pre Proc */

    TIOVXDLPreProcNodeCfg dl_pre_proc_cfg;
    NodeObj *dl_pre_proc_node;

    tiovx_dl_pre_proc_init_cfg(&dl_pre_proc_cfg);

    dl_pre_proc_cfg.io_config_path = TIDL_IO_CONFIG_FILE_PATH;
    dl_pre_proc_cfg.params.tensor_format = 1; //BGR

    dl_pre_proc_node = tiovx_modules_add_node(&graph, TIOVX_DL_PRE_PROC, (void *)&dl_pre_proc_cfg);

    /* TIDL */
    
    TIOVXTIDLNodeCfg tidl_cfg;
    NodeObj *tidl_node;
    tiovx_tidl_init_cfg(&tidl_cfg);
    tidl_cfg.io_config_path = TIDL_IO_CONFIG_FILE_PATH;
    tidl_cfg.network_path = TIDL_NETWORK_FILE_PATH;
    tidl_node = tiovx_modules_add_node(&graph, TIOVX_TIDL, (void *)&tidl_cfg);

    /* DL POST PROC*/

    TIOVXDLPostProcNodeCfg dl_post_proc_cfg;
    NodeObj *dl_post_proc_node;
    tiovx_dl_post_proc_init_cfg(&dl_post_proc_cfg);
    dl_post_proc_cfg.width = POST_PROC_OUT_WIDTH;
    dl_post_proc_cfg.height = POST_PROC_OUT_HEIGHT;
    uint32_t max_color_class = sizeof(RGB_COLOR_MAP)/sizeof(RGB_COLOR_MAP[0]);
    //additional line added
    dl_post_proc_cfg.num_input_tensors=1;
    dl_post_proc_cfg.params.task_type = TIVX_DL_POST_PROC_SEGMENTATION_TASK_TYPE;
    dl_post_proc_cfg.params.ss_prms.alpha = 0.5;
    dl_post_proc_cfg.params.ss_prms.YUVColorMap = (uint8_t **) malloc(sizeof(uint8_t *) * max_color_class);
    for (uint32_t i = 0; i < max_color_class; i++)
            {
                dl_post_proc_cfg.params.ss_prms.YUVColorMap[i] = (uint8_t *) malloc(sizeof(uint8_t) * 3);
                uint8_t R = RGB_COLOR_MAP[i][0];
                uint8_t G = RGB_COLOR_MAP[i][1];
                uint8_t B = RGB_COLOR_MAP[i][2];
                dl_post_proc_cfg.params.ss_prms.YUVColorMap[i][0] = RGB2Y(R,G,B);
                dl_post_proc_cfg.params.ss_prms.YUVColorMap[i][1] = RGB2U(R,G,B);
                dl_post_proc_cfg.params.ss_prms.YUVColorMap[i][2] = RGB2V(R,G,B);
            }
    dl_post_proc_cfg.params.ss_prms.MaxColorClass = max_color_class;
    
    dl_post_proc_cfg.io_config_path = TIDL_IO_CONFIG_FILE_PATH;
    dl_post_proc_node = tiovx_modules_add_node(&graph, TIOVX_DL_POST_PROC, (void *)&dl_post_proc_cfg);

    /* MSC0*/
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

    /* MSC1*/
    TIOVXMultiScalerNodeCfg msc1_cfg;
    NodeObj *msc1_node;
    TIOVXDLPreProcNodeCfg *tempCfg = (TIOVXDLPreProcNodeCfg *)dl_pre_proc_node->node_cfg;

    tiovx_multi_scaler_init_cfg(&msc1_cfg);

    msc1_cfg.color_format = VX_DF_IMAGE_NV12;
    msc1_cfg.num_outputs = 1;

    msc1_cfg.input_cfg.width = LDC_OUTPUT_WIDTH / 2;
    msc1_cfg.input_cfg.height = LDC_OUTPUT_HEIGHT / 2;

    msc1_cfg.output_cfgs[0].width = tempCfg->input_cfg.width;
    msc1_cfg.output_cfgs[0].height = tempCfg->input_cfg.height;

    sprintf(msc1_cfg.target_string, TIVX_TARGET_VPAC_MSC1);
    tiovx_multi_scaler_module_crop_params_init(&msc1_cfg);

    msc1_node = tiovx_modules_add_node(&graph, TIOVX_MULTI_SCALER, (void *)&msc1_cfg);

    /* Link PADS */
    tiovx_modules_link_pads(&tee_node->srcs[1], &viss_node->sinks[0]);
    tiovx_modules_link_pads(&viss_node->srcs[1], &aewb_node->sinks[0]); 
    tiovx_modules_link_pads(&viss_node->srcs[0], &ldc_node->sinks[0]);
    tiovx_modules_link_pads(&ldc_node->srcs[0],&msc_node->sinks[0]);
    tiovx_modules_link_pads(&msc_node->srcs[0],&msc1_node->sinks[0]);
    tiovx_modules_link_pads(&msc1_node->srcs[0],&dl_pre_proc_node->sinks[0]);
    tiovx_modules_link_pads(&dl_pre_proc_node->srcs[0],&tidl_node->sinks[0]);
    tiovx_modules_link_pads(&tidl_node->srcs[0], &dl_post_proc_node->sinks[0]); // Tensor pad
    tiovx_modules_link_pads(&msc_node->srcs[1], &dl_post_proc_node->sinks[1]); // Image Pad

    /* Connect the floating node to Display */
     status = tiovx_modules_verify_graph(&graph);

    #if defined(SOC_AM62A) && defined(TARGET_OS_QNX)
    in_buf_pool = tee_node->srcs[0].buf_pool;
    out_buf_pool = dl_post_proc_node->srcs[0].buf_pool;
    for (int32_t i = 0; i < in_buf_pool->bufq_depth; i++)
    {
        inbuf = tiovx_modules_acquire_buf(in_buf_pool);
        tiovx_modules_enqueue_buf(inbuf);
    }

    for (int32_t i=0; i< out_buf_pool->bufq_depth;i++)
    { 
        outbuf = tiovx_modules_acquire_buf(out_buf_pool);
        tiovx_modules_enqueue_buf(outbuf);
    }

    while (frame_count++ < NUM_ITERATIONS)
    {
        inbuf = tiovx_modules_dequeue_buf(in_buf_pool);
        tiovx_modules_enqueue_buf(inbuf);
        outbuf = tiovx_modules_dequeue_buf(out_buf_pool);
        /* function to render screen support using QNX on AM62A SOC */
        qnx_display_render_buf(outbuf);
        tiovx_modules_enqueue_buf(outbuf);
    }

    for (int32_t i = 0; i < 2; i++)
    {
        inbuf = tiovx_modules_dequeue_buf(in_buf_pool);
        tiovx_modules_release_buf(inbuf);
    }
 
    for(int32_t i =0; i< out_buf_pool->bufq_depth; i++)
    {
        outbuf = tiovx_modules_dequeue_buf(out_buf_pool);
        tiovx_modules_release_buf(outbuf);
    }

    tiovx_modules_clean_graph(&graph);
    free(dl_post_proc_cfg.params.ss_prms.YUVColorMap);

    return status;
    #endif
}
