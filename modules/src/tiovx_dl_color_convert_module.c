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
#include "tiovx_dl_color_convert_module.h"
#include <edgeai_tiovx_img_proc.h>

void tiovx_dl_color_convert_init_cfg(TIOVXDLColorConvertNodeCfg *node_cfg)
{
    node_cfg->width = TIOVX_MODULES_DEFAULT_IMAGE_WIDTH;
    node_cfg->height = TIOVX_MODULES_DEFAULT_IMAGE_HEIGHT;
    node_cfg->input_cfg.color_format = TIOVX_MODULES_DEFAULT_COLOR_FORMAT;
    node_cfg->output_cfg.color_format = TIOVX_MODULES_DEFAULT_COLOR_FORMAT;
    node_cfg->num_channels = 1;
    sprintf(node_cfg->target_string, TIVX_TARGET_A72_0);
}

vx_status tiovx_dl_color_convert_init_node(NodeObj *node)
{
    vx_status status = VX_FAILURE;
    TIOVXDLColorConvertNodeCfg *node_cfg = (TIOVXDLColorConvertNodeCfg *)node->node_cfg;
    vx_reference exemplar;

    node_cfg->input_cfg.width = node_cfg->width;
    node_cfg->input_cfg.height = node_cfg->height;
    node_cfg->output_cfg.width = node_cfg->width;
    node_cfg->output_cfg.height = node_cfg->height;

    node->num_inputs = 1;
    node->num_outputs = 1;

    node->sinks[0].node = node;
    node->sinks[0].pad_index = 0;
    node->sinks[0].node_parameter_index = 0;
    node->sinks[0].num_channels = node_cfg->num_channels;
    exemplar = (vx_reference)vxCreateImage(node->graph->tiovx_context,
                                           node_cfg->input_cfg.width,
                                           node_cfg->input_cfg.height,
                                           node_cfg->input_cfg.color_format);
    status = tiovx_module_create_pad_exemplar(&node->sinks[0], exemplar);
    if (VX_SUCCESS != status) {
        TIOVX_MODULE_ERROR("[DL_COLOR_CONVERT] Create Input Failed\n");
        return status;
    }
    vxReleaseReference(&exemplar);

    node->srcs[0].node = node;
    node->srcs[0].pad_index = 0;
    node->srcs[0].node_parameter_index = 1;
    node->srcs[0].num_channels = node_cfg->num_channels;
    exemplar = (vx_reference)vxCreateImage(node->graph->tiovx_context,
                                           node_cfg->output_cfg.width,
                                           node_cfg->output_cfg.height,
                                           node_cfg->output_cfg.color_format);
    status = tiovx_module_create_pad_exemplar(&node->srcs[0], exemplar);
    if (VX_SUCCESS != status) {
        TIOVX_MODULE_ERROR("[DL_COLOR_CONVERT] Create Output Failed\n");
        return status;
    }
    vxReleaseReference(&exemplar);

    sprintf(node->name, "dl_color_convert_node");

    return status;
}

vx_status tiovx_dl_color_convert_create_node(NodeObj *node)
{
    vx_status status = VX_FAILURE;
    TIOVXDLColorConvertNodeCfg *node_cfg = (TIOVXDLColorConvertNodeCfg *)node->node_cfg;
    vx_bool replicate[] = { vx_true_e, vx_true_e };

    node->tiovx_node = tivxDLColorConvertArmv8Node(node->graph->tiovx_graph,
                                (vx_image)(node->sinks[0].exemplar),
                                (vx_image)(node->srcs[0].exemplar));
    status = vxGetStatus((vx_reference)node->tiovx_node);
    if (VX_SUCCESS != status) {
        TIOVX_MODULE_ERROR("[DL_COLOR_CONVERT] Create Node Failed\n");
        return status;
    }

    vxSetNodeTarget(node->tiovx_node,
                    VX_TARGET_STRING, node_cfg->target_string);
    vxReplicateNode(node->graph->tiovx_graph,
                    node->tiovx_node, replicate, 2);

    return status;
}

vx_uint32 tiovx_dl_color_convert_get_cfg_size()
{
    return sizeof(TIOVXDLColorConvertNodeCfg);
}
