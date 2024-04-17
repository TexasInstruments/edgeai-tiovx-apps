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
 * Input(NV12 1280x720) --> MSC1 ----> MSC2 ----> DL_PRE_PROC ----> TIDL
 *                              |                                       |
 *                              |                                        ----> DL_POST_PROC ---> Display (NV12 640x360)
 *                              |                                       |
 *                              ---------------------------------------->
 */

#include <tiovx_modules.h>
#include <tiovx_utils.h>
#include <stdio.h>
#include <unistd.h>

#define APP_BUFQ_DEPTH   (1)
#define APP_NUM_CH       (1)

#define INPUT_WIDTH (1280)
#define INPUT_HEIGHT (720)

#define OUTPUT_WIDTH (640)
#define OUTPUT_HEIGHT (360)

#define TIDL_IO_CONFIG_FILE_PATH "/opt/model_zoo/ONR-CL-6360-regNetx-200mf/artifacts/410_tidl_io_1.bin"
#define TIDL_NETWORK_FILE_PATH "/opt/model_zoo/ONR-CL-6360-regNetx-200mf/artifacts/410_tidl_net.bin"

static char imgnet_labels_2[TIVX_DL_POST_PROC_MAX_NUM_CLASSNAMES][TIVX_DL_POST_PROC_MAX_SIZE_CLASSNAME] =
{
  "Not Available"
};

vx_status app_modules_dl_pipeline_test(int argc, char* argv[])
{
    vx_status status = VX_FAILURE;
    GraphObj graph;
    NodeObj *multiScaler1Node = NULL, *multiScaler2Node = NULL, *displayNode = NULL;
    NodeObj *dlPreProcNode = NULL, *tidlNode = NULL, *dlPostProcNode = NULL;

    TIOVXMultiScalerNodeCfg multiScaler1Cfg;
    TIOVXMultiScalerNodeCfg multiScaler2Cfg;
    TIOVXDLPreProcNodeCfg dlPreProcCfg;
    TIOVXTIDLNodeCfg tidlCfg;
    TIOVXDLPostProcNodeCfg dlPostProcCfg;
    TIOVXDisplayNodeCfg displayCfg;

    BufPool *in_buf_pool = NULL;
    Buf *inbuf = NULL;

    char input_filename[100];
    int img_cnt=0;

    status = tiovx_modules_initialize_graph(&graph);

    tiovx_dl_pre_proc_init_cfg(&dlPreProcCfg);
    dlPreProcCfg.io_config_path = TIDL_IO_CONFIG_FILE_PATH;
    dlPreProcCfg.params.tensor_format = 1; //BGR
    dlPreProcNode = tiovx_modules_add_node(&graph, TIOVX_DL_PRE_PROC, (void *)&dlPreProcCfg);

    tiovx_tidl_init_cfg(&tidlCfg);
    tidlCfg.io_config_path = TIDL_IO_CONFIG_FILE_PATH;
    tidlCfg.network_path = TIDL_NETWORK_FILE_PATH;
    tidlNode = tiovx_modules_add_node(&graph, TIOVX_TIDL, (void *)&tidlCfg);

    tiovx_dl_post_proc_init_cfg(&dlPostProcCfg);
    dlPostProcCfg.width = OUTPUT_WIDTH;
    dlPostProcCfg.height = OUTPUT_HEIGHT;
    dlPostProcCfg.params.task_type = TIVX_DL_POST_PROC_CLASSIFICATION_TASK_TYPE;
    dlPostProcCfg.params.oc_prms.labelOffset = 0;
    dlPostProcCfg.params.oc_prms.num_top_results = 5;
    dlPostProcCfg.params.oc_prms.classnames = imgnet_labels_2;
    dlPostProcCfg.io_config_path = TIDL_IO_CONFIG_FILE_PATH;
    dlPostProcNode = tiovx_modules_add_node(&graph, TIOVX_DL_POST_PROC, (void *)&dlPostProcCfg);

    tiovx_multi_scaler_init_cfg(&multiScaler1Cfg);
    multiScaler1Cfg.num_outputs = 2;
    multiScaler1Cfg.input_cfg.width = INPUT_WIDTH;
    multiScaler1Cfg.input_cfg.height = INPUT_HEIGHT;
    multiScaler1Cfg.output_cfgs[0].width = 640; // For pre proc
    multiScaler1Cfg.output_cfgs[0].height = 360;
    multiScaler1Cfg.output_cfgs[1].width = OUTPUT_WIDTH; // For post proc
    multiScaler1Cfg.output_cfgs[1].height = OUTPUT_HEIGHT;
    tiovx_multi_scaler_module_crop_params_init(&multiScaler1Cfg);
    multiScaler1Node = tiovx_modules_add_node(&graph, TIOVX_MULTI_SCALER, (void *)&multiScaler1Cfg);

    tiovx_multi_scaler_init_cfg(&multiScaler2Cfg);
    TIOVXDLPreProcNodeCfg *tempCfg = (TIOVXDLPreProcNodeCfg *)dlPreProcNode->node_cfg;
    multiScaler2Cfg.num_outputs = 1;
    multiScaler2Cfg.input_cfg.width = 640;
    multiScaler2Cfg.input_cfg.height = 360;
    multiScaler2Cfg.output_cfgs[0].width = tempCfg->input_cfg.width; // For pre proc
    multiScaler2Cfg.output_cfgs[0].height = tempCfg->input_cfg.height;
    tiovx_multi_scaler_module_crop_params_init(&multiScaler2Cfg);
    multiScaler2Node = tiovx_modules_add_node(&graph, TIOVX_MULTI_SCALER, (void *)&multiScaler2Cfg);

    tiovx_display_init_cfg(&displayCfg);
    displayCfg.width = OUTPUT_WIDTH;
    displayCfg.height = OUTPUT_HEIGHT;
    displayCfg.params.outWidth  = OUTPUT_WIDTH;
    displayCfg.params.outHeight = OUTPUT_HEIGHT;
    displayCfg.params.posX = (1920 - OUTPUT_WIDTH)/2;
    displayCfg.params.posY = (1080 - OUTPUT_HEIGHT)/2;
    displayNode = tiovx_modules_add_node(&graph, TIOVX_DISPLAY, (void *)&displayCfg);

    tiovx_modules_link_pads(&multiScaler1Node->srcs[0], &multiScaler2Node->sinks[0]);
    tiovx_modules_link_pads(&multiScaler2Node->srcs[0], &dlPreProcNode->sinks[0]);
    tiovx_modules_link_pads(&dlPreProcNode->srcs[0], &tidlNode->sinks[0]);

    tiovx_modules_link_pads(&tidlNode->srcs[0], &dlPostProcNode->sinks[0]); // Tensor pad
    tiovx_modules_link_pads(&multiScaler1Node->srcs[1], &dlPostProcNode->sinks[1]); // Image Pad

    tiovx_modules_link_pads(&dlPostProcNode->srcs[0], &displayNode->sinks[0]);

    status = tiovx_modules_verify_graph(&graph);

    in_buf_pool = multiScaler1Node->sinks[0].buf_pool;

    inbuf = tiovx_modules_acquire_buf(in_buf_pool);

    while(1)
    {
      if (++img_cnt > 14)
        break;

      sprintf(input_filename, "%s/raw_images/tiovx_apps/%04d.nv12", EDGEAI_DATA_PATH, img_cnt);

      readImage(input_filename, (vx_image)inbuf->handle);

      tiovx_modules_enqueue_buf(inbuf);

      tiovx_modules_schedule_graph(&graph);
      tiovx_modules_wait_graph(&graph);

      sleep(2);

      inbuf = tiovx_modules_dequeue_buf(in_buf_pool);
    }

    tiovx_modules_release_buf(inbuf);

    tiovx_modules_clean_graph(&graph);

    return status;
}
