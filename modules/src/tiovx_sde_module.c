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
#include "tiovx_sde_module.h"

#define TIOVX_MODULES_DEFAULT_SDE_WIDTH 1280
#define TIOVX_MODULES_DEFAULT_SDE_HEIGHT 720

typedef struct {
    vx_user_data_object         sde_params_obj;
} TIOVXSdeNodePriv;

vx_status tiovx_sde_module_configure_params(NodeObj *node)
{
    vx_status status = VX_FAILURE;
    TIOVXSdeNodeCfg *node_cfg = (TIOVXSdeNodeCfg *)node->node_cfg;
    TIOVXSdeNodePriv *node_priv = (TIOVXSdeNodePriv *)node->node_priv;

    node_priv->sde_params_obj = vxCreateUserDataObject(
                                                node->graph->tiovx_context,
                                                "tivx_dmpac_sde_params_t",
                                                sizeof(tivx_dmpac_sde_params_t),
                                                &node_cfg->sde_params);
    status = vxGetStatus((vx_reference)node_priv->sde_params_obj);
    if(status != VX_SUCCESS)
    {
        TIOVX_MODULE_ERROR("[SDE] Unable to create SDE config object! \n");
    }

    return status;
}

void tiovx_sde_init_cfg(TIOVXSdeNodeCfg *node_cfg)
{
    node_cfg->width = TIOVX_MODULES_DEFAULT_SDE_WIDTH;
    node_cfg->height = TIOVX_MODULES_DEFAULT_SDE_HEIGHT;
    node_cfg->num_channels = 1;
    sprintf(node_cfg->target_string, TIVX_TARGET_DMPAC_SDE);

    node_cfg->input_cfg.color_format = VX_DF_IMAGE_NV12;
    node_cfg->output_cfg.color_format = VX_DF_IMAGE_S16;

    node_cfg->sde_params.median_filter_enable        = 0;
    node_cfg->sde_params.reduced_range_search_enable = 0;
    node_cfg->sde_params.disparity_min               = 0;
    node_cfg->sde_params.disparity_max               = 1;
    node_cfg->sde_params.threshold_left_right        = 0;
    node_cfg->sde_params.texture_filter_enable       = 0;
    node_cfg->sde_params.threshold_texture           = 0;
    node_cfg->sde_params.aggregation_penalty_p1      = 0;
    node_cfg->sde_params.aggregation_penalty_p2      = 0;
    node_cfg->sde_params.confidence_score_map[0]     = 0;
    node_cfg->sde_params.confidence_score_map[1]     = 4;
    node_cfg->sde_params.confidence_score_map[2]     = 9;
    node_cfg->sde_params.confidence_score_map[3]     = 18;
    node_cfg->sde_params.confidence_score_map[4]     = 28;
    node_cfg->sde_params.confidence_score_map[5]     = 43;
    node_cfg->sde_params.confidence_score_map[6]     = 109;
    node_cfg->sde_params.confidence_score_map[7]     = 127;
}

vx_status tiovx_sde_init_node(NodeObj *node)
{
    vx_status status = VX_FAILURE;
    TIOVXSdeNodeCfg *node_cfg = (TIOVXSdeNodeCfg *)node->node_cfg;
    vx_reference exemplar;

    node_cfg->input_cfg.width = node_cfg->width;
    node_cfg->input_cfg.height = node_cfg->height;

    node->num_inputs = 2;

    node->sinks[0].node = node;
    node->sinks[0].pad_index = 0;
    node->sinks[0].node_parameter_index = 1;
    node->sinks[0].num_channels = node_cfg->num_channels;
    exemplar = (vx_reference)vxCreateImage(node->graph->tiovx_context,
                                           node_cfg->input_cfg.width,
                                           node_cfg->input_cfg.height,
                                           node_cfg->input_cfg.color_format);
    status = tiovx_module_create_pad_exemplar(&node->sinks[0], exemplar);
    if (VX_SUCCESS != status) {
        TIOVX_MODULE_ERROR("[SDE] Create Left Input Failed\n");
        return status;
    }
    vxReleaseReference(&exemplar);

    node->sinks[1].node = node;
    node->sinks[1].pad_index = 1;
    node->sinks[1].node_parameter_index = 2;
    node->sinks[1].num_channels = node_cfg->num_channels;
    exemplar = (vx_reference)vxCreateImage(node->graph->tiovx_context,
                                           node_cfg->input_cfg.width,
                                           node_cfg->input_cfg.height,
                                           node_cfg->input_cfg.color_format);
    status = tiovx_module_create_pad_exemplar(&node->sinks[1], exemplar);
    if (VX_SUCCESS != status) {
        TIOVX_MODULE_ERROR("[SDE] Create Right Input Failed\n");
        return status;
    }
    vxReleaseReference(&exemplar);

    node->num_outputs = 1;

    node_cfg->output_cfg.width = node_cfg->width;
    node_cfg->output_cfg.height = node_cfg->height;

    node->srcs[0].node = node;
    node->srcs[0].pad_index = 0;
    node->srcs[0].node_parameter_index = 3;
    node->srcs[0].num_channels = node_cfg->num_channels;
    exemplar = (vx_reference)vxCreateImage(
                                node->graph->tiovx_context,
                                node_cfg->output_cfg.width,
                                node_cfg->output_cfg.height,
                                node_cfg->output_cfg.color_format);
    status = tiovx_module_create_pad_exemplar(&node->srcs[0], exemplar);
    if (VX_SUCCESS != status) {
        TIOVX_MODULE_ERROR("[SDE] Create Output Failed\n");
        return status;
    }
    vxReleaseReference(&exemplar);

    tiovx_sde_module_configure_params(node);

    return status;
}

vx_status tiovx_sde_create_node(NodeObj *node)
{
    vx_status status = VX_FAILURE;
    TIOVXSdeNodeCfg *node_cfg = (TIOVXSdeNodeCfg *)node->node_cfg;
    TIOVXSdeNodePriv *node_priv = (TIOVXSdeNodePriv *)node->node_priv;
    vx_bool replicate[] = {vx_false_e, vx_true_e, vx_true_e,
                           vx_true_e, vx_false_e};

    node->tiovx_node = tivxDmpacSdeNode(node->graph->tiovx_graph,
                                        node_priv->sde_params_obj,
                                        (vx_image)(node->sinks[0].exemplar),
                                        (vx_image)(node->sinks[1].exemplar),
                                        (vx_image)(node->srcs[0].exemplar),
                                        NULL);

    status = vxGetStatus((vx_reference)node->tiovx_node);
    if (VX_SUCCESS != status) {
        TIOVX_MODULE_ERROR("[SDE] Create Node Failed\n");
        return status;
    }

    sprintf(node->name, "sde_node");

    vxSetNodeTarget(node->tiovx_node, VX_TARGET_STRING, node_cfg->target_string);
    vxReplicateNode(node->graph->tiovx_graph, node->tiovx_node, replicate, 5);

    return status;
}

vx_status tiovx_sde_delete_node(NodeObj *node)
{
    vx_status status = VX_FAILURE;
    TIOVXSdeNodePriv *node_priv = (TIOVXSdeNodePriv *)node->node_priv;

    status = vxReleaseNode(&node->tiovx_node);

    status = vxReleaseUserDataObject(&node_priv->sde_params_obj);

    return status;
}

vx_uint32 tiovx_sde_get_cfg_size()
{
    return sizeof(TIOVXSdeNodeCfg);
}

vx_uint32 tiovx_sde_get_priv_size()
{
    return sizeof(TIOVXSdeNodePriv);
}
