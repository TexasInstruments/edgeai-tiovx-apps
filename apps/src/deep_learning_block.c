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

#include <yaml_parser.h>
#include <edgeai_nv12_drawing_utils.h>

#include <apps/include/deep_learning_block.h>

static char *g_mpu_targets[] = {TIVX_TARGET_MPU_0, TIVX_TARGET_MPU_1,
                                TIVX_TARGET_MPU_2, TIVX_TARGET_MPU_3};

static uint8_t g_mpu_target_idx = 0;

#if defined(SOC_J784S4)
static char *g_c7x_targets[] = {TIVX_TARGET_DSP_C7_1,TIVX_TARGET_DSP_C7_2,
                                TIVX_TARGET_DSP_C7_3,TIVX_TARGET_DSP_C7_4};
#elif defined(SOC_J722S) || defined(SOC_J742S2)
static char *g_c7x_targets[] = {TIVX_TARGET_DSP_C7_1,TIVX_TARGET_DSP_C7_2};
#else
static char *g_c7x_targets[] = {TIVX_TARGET_DSP_C7_1};
#endif

static uint8_t g_c7x_target_idx = 0;

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

void initialize_deep_learning_block(DeepLearningBlock *dl_block)
{
    dl_block->pre_proc_input_pad = NULL;
    dl_block->pre_proc_width = 0;
    dl_block->pre_proc_height = 0;
    dl_block->post_proc_input_pad = NULL;
    dl_block->post_proc_width = 0;
    dl_block->post_proc_height = 0;
    dl_block->output_pad = NULL;
    dl_block->subflow_info = NULL;
    dl_block->num_channels = 1;
}

int32_t create_deep_learning_block(GraphObj *graph, DeepLearningBlock *dl_block)
{
    int32_t status = 0;
    uint32_t i;
    ModelInfo *model_info;
    PreProcInfo *pre_proc_info;
    PostProcInfo *post_proc_info;
    Pad *output_pad[4] = {NULL, NULL, NULL, NULL};
    uint32_t output_width;
    uint32_t output_height;

    if(NULL == dl_block->subflow_info)
    {
        TIOVX_APPS_ERROR("SubflowInfo for deep learning block is null\n");
        return -1;
    }

    if(0 == dl_block->post_proc_width || 0 == dl_block->post_proc_height)
    {
        TIOVX_APPS_ERROR("Invalid post process dims for deep learning block\n");
        return -1;
    }

    model_info = &dl_block->subflow_info->model_info;
    pre_proc_info = &model_info->pre_proc_info;
    post_proc_info = &model_info->post_proc_info;

    output_width = dl_block->post_proc_width;
    output_height = dl_block->post_proc_height;

    /* DL Pre Proc*/
    {
        TIOVXDLPreProcNodeCfg dl_pre_proc_cfg;
        NodeObj *dl_pre_proc_node;

        tiovx_dl_pre_proc_init_cfg(&dl_pre_proc_cfg);

        dl_pre_proc_cfg.num_channels = dl_block->num_channels;

        dl_pre_proc_cfg.io_config_path = model_info->io_config_path;
        dl_pre_proc_cfg.params.tensor_format = pre_proc_info->tensor_format;

        sprintf(dl_pre_proc_cfg.target_string, g_mpu_targets[g_mpu_target_idx]);

        dl_pre_proc_node = tiovx_modules_add_node(graph,
                                                  TIOVX_DL_PRE_PROC,
                                                  (void *)&dl_pre_proc_cfg);

        dl_block->pre_proc_input_pad = &dl_pre_proc_node->sinks[0];
        output_pad[0] = &dl_pre_proc_node->srcs[0];

        TIOVXDLPreProcNodeCfg *temp_cfg = (TIOVXDLPreProcNodeCfg *)dl_pre_proc_node->node_cfg;
        dl_block->pre_proc_width = temp_cfg->input_cfg.width;
        dl_block->pre_proc_height = temp_cfg->input_cfg.height;
    }  

    /* TIDL */
    {
        TIOVXTIDLNodeCfg tidl_cfg;
        NodeObj *tidl_node;

        tiovx_tidl_init_cfg(&tidl_cfg);

        tidl_cfg.num_channels = dl_block->num_channels;

        tidl_cfg.io_config_path = model_info->io_config_path;
        tidl_cfg.network_path = model_info->network_path;

        sprintf(tidl_cfg.target_string, g_c7x_targets[g_c7x_target_idx]);

        tidl_node = tiovx_modules_add_node(graph,
                                           TIOVX_TIDL,
                                           (void *)&tidl_cfg);

        /* Link DLPreProc to TIDL */
        tiovx_modules_link_pads(output_pad[0], &tidl_node->sinks[0]);

        for (i = 0; i < tidl_node->num_outputs; i++)
        {
            output_pad[i] = &tidl_node->srcs[i];
        }
    }

    /* DL Post Proc */
    {
        TIOVXDLPostProcNodeCfg dl_post_proc_cfg;
        NodeObj *dl_post_proc_node;

        tiovx_dl_post_proc_init_cfg(&dl_post_proc_cfg);

        dl_post_proc_cfg.num_channels = dl_block->num_channels;
    
        dl_post_proc_cfg.width =  output_width;
        dl_post_proc_cfg.height =  output_height;

        dl_post_proc_cfg.io_config_path = model_info->io_config_path;

        if(0 == strcmp("classification", post_proc_info->task_type))
        {
            dl_post_proc_cfg.params.task_type = TIVX_DL_POST_PROC_CLASSIFICATION_TASK_TYPE;
            dl_post_proc_cfg.params.oc_prms.num_top_results = post_proc_info->top_n;

            /* LabelOffset */
            dl_block->label_offset =  (int32_t*) malloc(TIVX_DL_POST_PROC_MAX_NUM_CLASSNAMES * sizeof(int32_t));
            status = get_label_offset(model_info->model_path,
                                      dl_block->label_offset,
                                      NULL);
            if(0 != status)
            {
                TIOVX_APPS_ERROR("Unable to parse label_offset\n");
                return status;
            }

            dl_post_proc_cfg.params.oc_prms.labelOffset = dl_block->label_offset[0];


            /* Classnames for Image Classification and Object detection*/
            dl_block->classnames = malloc(TIVX_DL_POST_PROC_MAX_SIZE_CLASSNAME * TIVX_DL_POST_PROC_MAX_NUM_CLASSNAMES * sizeof(char));
            status = get_classname(model_info->model_path,
                                   dl_block->classnames,
                                   TIVX_DL_POST_PROC_MAX_NUM_CLASSNAMES);
            if(0 != status)
            {
                TIOVX_APPS_ERROR("Unable to parse classnames\n");
                return status;
            }
            dl_post_proc_cfg.params.oc_prms.classnames =  dl_block->classnames;
        }
        else if(0 == strcmp("detection", post_proc_info->task_type))
        {
            int32_t label_index_offset;

            dl_post_proc_cfg.params.task_type = TIVX_DL_POST_PROC_DETECTION_TASK_TYPE;
            dl_post_proc_cfg.params.od_prms.viz_th = post_proc_info->viz_threshold;

            /* LabelOffset */
            dl_block->label_offset = (int32_t*) malloc(TIVX_DL_POST_PROC_MAX_NUM_CLASSNAMES * sizeof(int32_t));
            status = get_label_offset(model_info->model_path,
                                      dl_block->label_offset,
                                      &label_index_offset);
            if(0 != status)
            {
                TIOVX_APPS_ERROR("Unable to parse label_offset\n");
                return status;
            }

            dl_post_proc_cfg.params.od_prms.labelIndexOffset = label_index_offset;
            dl_post_proc_cfg.params.od_prms.labelOffset = dl_block->label_offset;
            
            for (uint32_t i = 0; i < 6; i++)
            {
                dl_post_proc_cfg.params.od_prms.formatter[i] = post_proc_info->formatter[i];
            }

            if(post_proc_info->norm_detect)
            {
                dl_post_proc_cfg.params.od_prms.scaleX = (float)output_width;
                dl_post_proc_cfg.params.od_prms.scaleY = (float)output_height;
            }
            else
            {
                dl_post_proc_cfg.params.od_prms.scaleX = (float)(output_width / (float)dl_block->pre_proc_width);
                dl_post_proc_cfg.params.od_prms.scaleY = (float)(output_height / (float)dl_block->pre_proc_height);
            }

            /* Classnames for Image Classification and Object detection*/
            dl_block->classnames = malloc(TIVX_DL_POST_PROC_MAX_SIZE_CLASSNAME * TIVX_DL_POST_PROC_MAX_NUM_CLASSNAMES * sizeof(char));

            status = get_classname(model_info->model_path,
                                   dl_block->classnames,
                                   TIVX_DL_POST_PROC_MAX_NUM_CLASSNAMES);
            if(0 != status)
            {
                TIOVX_APPS_ERROR("Unable to parse classnames\n");
                return status;
            }

            dl_post_proc_cfg.params.od_prms.classnames = dl_block->classnames;
        }
        else if(0 == strcmp("segmentation", post_proc_info->task_type))
        {
            uint32_t max_color_class = sizeof(RGB_COLOR_MAP)/sizeof(RGB_COLOR_MAP[0]);
            dl_post_proc_cfg.params.task_type = TIVX_DL_POST_PROC_SEGMENTATION_TASK_TYPE;
            dl_post_proc_cfg.params.ss_prms.alpha = post_proc_info->alpha;
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
        }
        else
        {
            TIOVX_APPS_ERROR("Invalid task-type specified\n");
            return -1;
        }

        dl_post_proc_node = tiovx_modules_add_node(graph,
                                                    TIOVX_DL_POST_PROC,
                                                    (void *)&dl_post_proc_cfg);

        /* Link TIDL to DL Post Proc  */
        for (i = 0; i < dl_post_proc_node->num_inputs - 1; i++)
        {
            tiovx_modules_link_pads(output_pad[i], &dl_post_proc_node->sinks[i]);
        }

        dl_block->post_proc_input_pad = &dl_post_proc_node->sinks[dl_post_proc_node->num_inputs - 1];

        dl_block->output_pad = &dl_post_proc_node->srcs[0];
    }

    g_mpu_target_idx++;
    if(g_mpu_target_idx >= sizeof(g_mpu_targets)/sizeof(g_mpu_targets[0]))
    {
        g_mpu_target_idx = 0;
    }

    g_c7x_target_idx++;
    if(g_c7x_target_idx >= sizeof(g_c7x_targets)/sizeof(g_c7x_targets[0]))
    {
        g_c7x_target_idx = 0;
    }

    return status;
}
