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
#include "tiovx_display_module.h"

#define DEFAULT_DISPLAY_WIDTH 1920
#define DEFAULT_DISPLAY_HEIGHT 1080

typedef struct {
    vx_user_data_object     config;
} TIOVXDisplayNodePriv;

vx_status tiovx_display_create_config(NodeObj *node)
{
    vx_status status = VX_FAILURE;
    TIOVXDisplayNodeCfg *node_cfg = (TIOVXDisplayNodeCfg *)node->node_cfg;
    TIOVXDisplayNodePriv *node_priv = (TIOVXDisplayNodePriv *)node->node_priv;

    node_priv->config = vxCreateUserDataObject(node->graph->tiovx_context,
                                               "tivx_display_params_t",
                                               sizeof(tivx_display_params_t),
                                               &node_cfg->params);
    status = vxGetStatus((vx_reference)node_priv->config);
    if (VX_SUCCESS != status)
    {
        TIOVX_MODULE_ERROR("[DISPLAY] Create config failed\n");
        return status;
    }

    vxSetReferenceName((vx_reference)node_priv->config, "display_config");

    return status;
}

void tiovx_display_init_cfg(TIOVXDisplayNodeCfg *node_cfg)
{
    node_cfg->width = TIOVX_MODULES_DEFAULT_IMAGE_WIDTH;
    node_cfg->height = TIOVX_MODULES_DEFAULT_IMAGE_HEIGHT;
    node_cfg->input_cfg.color_format = TIOVX_MODULES_DEFAULT_COLOR_FORMAT;
    node_cfg->num_channels = 1;
    sprintf(node_cfg->target_string, TIVX_TARGET_DISPLAY1);
    memset(&node_cfg->params, 0, sizeof(tivx_display_params_t));
    node_cfg->params.opMode = TIVX_KERNEL_DISPLAY_ZERO_BUFFER_COPY_MODE;
    node_cfg->params.pipeId = 0;
    node_cfg->params.outWidth = DEFAULT_DISPLAY_WIDTH;
    node_cfg->params.outHeight = DEFAULT_DISPLAY_HEIGHT;
    node_cfg->params.posX = 0;
    node_cfg->params.posY = 0;
}

vx_status tiovx_display_init_node(NodeObj *node)
{
    vx_status status = VX_FAILURE;
    TIOVXDisplayNodeCfg *node_cfg = (TIOVXDisplayNodeCfg *)node->node_cfg;
    vx_reference exemplar;

    status = tiovx_display_create_config(node);
    if (VX_SUCCESS != status)
    {
        TIOVX_MODULE_ERROR("[DISPLAY] Create config failed\n");
        return status;
    }

    node_cfg->input_cfg.width = node_cfg->width;
    node_cfg->input_cfg.height = node_cfg->height;

    node->num_inputs = 1;
    node->num_outputs = 0;

    node->sinks[0].node = node;
    node->sinks[0].pad_index = 0;
    node->sinks[0].node_parameter_index = 1;
    node->sinks[0].num_channels = node_cfg->num_channels;
    exemplar = (vx_reference)vxCreateImage(node->graph->tiovx_context,
                                           node_cfg->input_cfg.width,
                                           node_cfg->input_cfg.height,
                                           node_cfg->input_cfg.color_format);
    status = tiovx_module_create_pad_exemplar(&node->sinks[0], exemplar);
    if (VX_SUCCESS != status)
    {
        TIOVX_MODULE_ERROR("[DISPLAY] Create Input Failed\n");
        return status;
    }
    vxReleaseReference(&exemplar);

    return status;
}

vx_status tiovx_display_create_node(NodeObj *node)
{
    vx_status status = VX_FAILURE;
    TIOVXDisplayNodeCfg *node_cfg = (TIOVXDisplayNodeCfg *)node->node_cfg;
    TIOVXDisplayNodePriv *node_priv = (TIOVXDisplayNodePriv *)node->node_priv;

    node->tiovx_node = tivxDisplayNode(node->graph->tiovx_graph,
                                       node_priv->config,
                                       (vx_image)(node->sinks[0].exemplar));
    status = vxGetStatus((vx_reference)node->tiovx_node);
    if (VX_SUCCESS != status)
    {
        TIOVX_MODULE_ERROR("[DISPLAY] Create Node Failed\n");
        return status;
    }

    sprintf(node->name, "display_node");

    vxSetNodeTarget(node->tiovx_node,
                    VX_TARGET_STRING, node_cfg->target_string);

    vxSetReferenceName((vx_reference)node->tiovx_node, "display_node");

    return status;
}

vx_status tiovx_display_delete_node(NodeObj *node)
{
    vx_status status = VX_FAILURE;
    TIOVXDisplayNodePriv *node_priv = (TIOVXDisplayNodePriv *)node->node_priv;

    status = vxReleaseNode(&node->tiovx_node);

    status = vxReleaseUserDataObject(&node_priv->config);

    return status;
}

vx_uint32 tiovx_display_get_cfg_size()
{
    return sizeof(TIOVXDisplayNodeCfg);
}

vx_uint32 tiovx_display_get_priv_size()
{
    return sizeof(TIOVXDisplayNodePriv);
}
