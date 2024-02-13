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

#include <apps/include/deep_learning_block.h>

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
            dl_post_proc_cfg.params.oc_prms.labelOffset = post_proc_info->label_offset[0];
            status = get_classname(model_info->model_path,
                                    dl_post_proc_cfg.params.oc_prms.classnames);
            if(0 != status)
            {
                TIOVX_APPS_ERROR("Unable to parse classnames\n");
                return status;
            }
        }
        else if(0 == strcmp("detection", post_proc_info->task_type))
        {
            dl_post_proc_cfg.params.task_type = TIVX_DL_POST_PROC_DETECTION_TASK_TYPE;
            dl_post_proc_cfg.params.od_prms.viz_th = post_proc_info->viz_threshold;
            dl_post_proc_cfg.params.od_prms.labelIndexOffset = post_proc_info->label_index_offset;
            for(uint32_t i = 0; i < post_proc_info->num_label_offset; i++)
            {
                dl_post_proc_cfg.params.od_prms.labelOffset[i] = post_proc_info->label_offset[i];
            }
            
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

            status = get_classname(model_info->model_path,
                                    dl_post_proc_cfg.params.od_prms.classnames);
            if(0 != status)
            {
                TIOVX_APPS_ERROR("Unable to parse classnames\n");
                return status;
            }
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

    return status;
}