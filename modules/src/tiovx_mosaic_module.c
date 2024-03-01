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
#include "tiovx_mosaic_module.h"

typedef struct {
    vx_user_data_object         config;
    vx_kernel                   kernel;
} TIOVXMosaicNodePriv;

vx_status tiovx_mosaic_create_config(NodeObj *node)
{
    vx_status status = VX_FAILURE;
    TIOVXMosaicNodeCfg *node_cfg = (TIOVXMosaicNodeCfg *)node->node_cfg;
    TIOVXMosaicNodePriv *node_priv = (TIOVXMosaicNodePriv *)node->node_priv;

    node_priv->config = vxCreateUserDataObject(node->graph->tiovx_context,
                                               "ImgMosaicConfig", 
                                               sizeof(tivxImgMosaicParams),
                                               NULL);
    status = vxGetStatus((vx_reference)node_priv->config);
    if(VX_SUCCESS != status)
    {
        TIOVX_MODULE_ERROR("[MOSAIC] Create config failed\n");
        return status;
    }

    vxSetReferenceName((vx_reference)node_priv->config, "mosaic_node_config");

    status = vxCopyUserDataObject(node_priv->config,
                                  0,
                                  sizeof(tivxImgMosaicParams),
                                  &(node_cfg->params),
                                  VX_WRITE_ONLY,
                                  VX_MEMORY_TYPE_HOST);
    if(status != VX_SUCCESS)
    {
        TIOVX_MODULE_ERROR("[MOSAIC] Unable to copy mosaic params!\n");
        return status;
    }

    return status;
}

void tiovx_mosaic_init_cfg(TIOVXMosaicNodeCfg *node_cfg)
{
    node_cfg->color_format = TIOVX_MODULES_DEFAULT_COLOR_FORMAT;
    node_cfg->num_inputs = 1;
    for(int32_t i = 0; i < TIVX_IMG_MOSAIC_MAX_INPUTS; i++)
    {
        node_cfg->num_channels[i] = 1;
    }
    node_cfg->input_cfgs[0].width = TIOVX_MODULES_DEFAULT_IMAGE_WIDTH;
    node_cfg->input_cfgs[0].height = TIOVX_MODULES_DEFAULT_IMAGE_HEIGHT;
    node_cfg->output_cfg.width = TIOVX_MODULES_DEFAULT_IMAGE_WIDTH;
    node_cfg->output_cfg.height = TIOVX_MODULES_DEFAULT_IMAGE_HEIGHT;
    sprintf(node_cfg->target_string, TIVX_TARGET_VPAC_MSC1);
    tivxImgMosaicParamsSetDefaults(&node_cfg->params);
    node_cfg->background_img = NULL;
}

vx_status tiovx_mosaic_init_node(NodeObj *node)
{
    vx_status status = VX_FAILURE;
    TIOVXMosaicNodeCfg *node_cfg = (TIOVXMosaicNodeCfg *)node->node_cfg;
    TIOVXMosaicNodePriv *node_priv = (TIOVXMosaicNodePriv *)node->node_priv;
    vx_reference exemplar;
    vx_int32 i;

    for (i = 0; i < node_cfg->num_inputs; i++)
        node_cfg->input_cfgs[i].color_format = node_cfg->color_format;

    node_cfg->output_cfg.color_format = node_cfg->color_format;

    node->num_inputs = node_cfg->num_inputs;
    node->num_outputs = 1;

    for (i = 0; i < node_cfg->num_inputs; i++)
    {
        node->sinks[i].node = node;
        node->sinks[i].pad_index = i;
        node->sinks[i].node_parameter_index = (3+i);
        node->sinks[i].num_channels = node_cfg->num_channels[i];
        exemplar = (vx_reference)vxCreateImage(node->graph->tiovx_context,
                                               node_cfg->input_cfgs[i].width,
                                               node_cfg->input_cfgs[i].height,
                                               node_cfg->input_cfgs[i].color_format);
        status = tiovx_module_create_pad_exemplar(&node->sinks[i], exemplar);
        if (VX_SUCCESS != status)
        {
            TIOVX_MODULE_ERROR("[MOSAIC] Create Input %d Failed\n", i);
            return status;
        }
        vxReleaseReference(&exemplar);
    }

    node->srcs[0].node = node;
    node->srcs[0].pad_index = 0;
    node->srcs[0].node_parameter_index = 1;
    node->srcs[0].num_channels = 1;
    exemplar = (vx_reference)vxCreateImage(node->graph->tiovx_context,
                                           node_cfg->output_cfg.width,
                                           node_cfg->output_cfg.height,
                                           node_cfg->output_cfg.color_format);
    status = tiovx_module_create_pad_exemplar(&node->srcs[0], exemplar);
    if (VX_SUCCESS != status)
    {
        TIOVX_MODULE_ERROR("[MULTI_SCALER] Create Output Failed\n");
        return status;
    }
    vxReleaseReference(&exemplar);

    sprintf(node->name, "mosaic_node");

    status = tiovx_mosaic_create_config(node);
    if(VX_SUCCESS != status)
    {
        TIOVX_MODULE_ERROR("[MOSAIC] Create config failed!\n");
        return status;
    }

    node_priv->kernel = tivxAddKernelImgMosaic(node->graph->tiovx_context,
                                               node_cfg->num_inputs);
    status = vxGetStatus((vx_reference)node_priv->kernel);
    if(VX_SUCCESS != status)
    {
        TIOVX_MODULE_ERROR("[MOSAIC] Unable to create kernel with %d inputs!\n",
                            node_cfg->num_inputs);
        return status;
    }

    return status;
}

vx_status tiovx_mosaic_create_node(NodeObj *node)
{
    vx_status status = VX_FAILURE;
    TIOVXMosaicNodeCfg *node_cfg = (TIOVXMosaicNodeCfg *)node->node_cfg;
    TIOVXMosaicNodePriv *node_priv = (TIOVXMosaicNodePriv *)node->node_priv;

    vx_object_array inputs[TIVX_IMG_MOSAIC_MAX_INPUTS];
    vx_int32 i;

    for (i = 0; i < node_cfg->num_inputs; i++)
        inputs[i] = node->sinks[i].exemplar_arr;

    node->tiovx_node = tivxImgMosaicNode(node->graph->tiovx_graph,
                                         node_priv->kernel,
                                         node_priv->config,
                                         (vx_image)(node->srcs[0].exemplar),
                                         node_cfg->background_img,
                                         inputs,
                                         node_cfg->num_inputs);

    status = vxGetStatus((vx_reference)node->tiovx_node);
    if (VX_SUCCESS != status) {
        TIOVX_MODULE_ERROR("[MOSAIC] Create Node Failed\n");
        return status;
    }

    vxSetNodeTarget(node->tiovx_node,
                    VX_TARGET_STRING, node_cfg->target_string);

    vxSetReferenceName((vx_reference)node->tiovx_node, "mosaic_node");

    return status;
}

vx_status tiovx_mosaic_delete_node(NodeObj *node)
{
    vx_status status = VX_FAILURE;
    TIOVXMosaicNodePriv *node_priv = (TIOVXMosaicNodePriv *)node->node_priv;

    status = vxReleaseNode(&node->tiovx_node);

    status = vxReleaseUserDataObject(&node_priv->config);

    status = vxRemoveKernel(node_priv->kernel);

    return status;
}

vx_uint32 tiovx_mosaic_get_cfg_size()
{
    return sizeof(TIOVXMosaicNodeCfg);
}

vx_uint32 tiovx_mosaic_get_priv_size()
{
    return sizeof(TIOVXMosaicNodePriv);
}
