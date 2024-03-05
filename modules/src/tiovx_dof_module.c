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
#include "tiovx_dof_module.h"

#define TIOVX_MODULES_DEFAULT_DOF_WIDTH 1280
#define TIOVX_MODULES_DEFAULT_DOF_HEIGHT 720
#define TIOVX_MODULES_DEFAULT_PYRAMID_LEVELS 3

typedef struct {
    vx_user_data_object         dof_params_obj;
} TIOVXDofNodePriv;

vx_status tiovx_dof_module_configure_params(NodeObj *node)
{
    vx_status status = VX_FAILURE;
    TIOVXDofNodeCfg *node_cfg = (TIOVXDofNodeCfg *)node->node_cfg;
    TIOVXDofNodePriv *node_priv = (TIOVXDofNodePriv *)node->node_priv;

    if(node_cfg->enable_temporal_predicton_flow_vector == 0)
    {
      node_cfg->dof_params.base_predictor[0] = TIVX_DMPAC_DOF_PREDICTOR_DELAY_LEFT;
      node_cfg->dof_params.base_predictor[1] = TIVX_DMPAC_DOF_PREDICTOR_PYR_COLOCATED;

      node_cfg->dof_params.inter_predictor[0] = TIVX_DMPAC_DOF_PREDICTOR_DELAY_LEFT;
      node_cfg->dof_params.inter_predictor[1] = TIVX_DMPAC_DOF_PREDICTOR_PYR_COLOCATED;
    }

    node_priv->dof_params_obj = vxCreateUserDataObject(
                                                node->graph->tiovx_context,
                                                "tivx_dmpac_dof_params_t",
                                                sizeof(tivx_dmpac_dof_params_t),
                                                &node_cfg->dof_params);
    status = vxGetStatus((vx_reference)node_priv->dof_params_obj);
    if(status != VX_SUCCESS)
    {
        TIOVX_MODULE_ERROR("[DOF] Unable to create DOF config object! \n");
    }

    return status;
}

void tiovx_dof_init_cfg(TIOVXDofNodeCfg *node_cfg)
{
    node_cfg->width = TIOVX_MODULES_DEFAULT_DOF_WIDTH;
    node_cfg->height = TIOVX_MODULES_DEFAULT_DOF_HEIGHT;
    node_cfg->num_channels = 1;
    node_cfg->enable_temporal_predicton_flow_vector = 0;
    node_cfg->enable_output_distribution = 0;
    sprintf(node_cfg->target_string, TIVX_TARGET_DMPAC_DOF);

    node_cfg->input_cfg.color_format = VX_DF_IMAGE_U8;
    node_cfg->input_cfg.levels = TIOVX_MODULES_DEFAULT_PYRAMID_LEVELS;
    node_cfg->input_cfg.scale = VX_SCALE_PYRAMID_HALF;

    node_cfg->flow_vector_cfg.color_format = VX_DF_IMAGE_U32;

    node_cfg->distribution_cfg.num_bins = 16;
    node_cfg->distribution_cfg.offset = 0;
    node_cfg->distribution_cfg.range = 16;

    tivx_dmpac_dof_params_init(&node_cfg->dof_params);

    node_cfg->dof_params.vertical_search_range[0] = 48;
    node_cfg->dof_params.vertical_search_range[1] = 48;
    node_cfg->dof_params.horizontal_search_range = 191;
    node_cfg->dof_params.median_filter_enable = 1;
    node_cfg->dof_params.motion_smoothness_factor = 12;
    node_cfg->dof_params.motion_direction = 1; /* 0: for side camera */
}

vx_status tiovx_dof_init_node(NodeObj *node)
{
    vx_status status = VX_FAILURE;
    TIOVXDofNodeCfg *node_cfg = (TIOVXDofNodeCfg *)node->node_cfg;
    vx_reference exemplar;

    node_cfg->input_cfg.width = node_cfg->width;
    node_cfg->input_cfg.height = node_cfg->height;
    node_cfg->flow_vector_cfg.width = node_cfg->width;
    node_cfg->flow_vector_cfg.height = node_cfg->height;

    node->num_inputs = 2;

    node->sinks[0].node = node;
    node->sinks[0].pad_index = 0;
    node->sinks[0].node_parameter_index = 3;
    node->sinks[0].num_channels = node_cfg->num_channels;
    exemplar = (vx_reference)vxCreatePyramid(node->graph->tiovx_context,
                                             node_cfg->input_cfg.levels,
                                             node_cfg->input_cfg.scale,
                                             node_cfg->input_cfg.width,
                                             node_cfg->input_cfg.height,
                                             node_cfg->input_cfg.color_format);
    status = tiovx_module_create_pad_exemplar(&node->sinks[0], exemplar);
    if (VX_SUCCESS != status) {
        TIOVX_MODULE_ERROR("[DOF] Create Input Failed\n");
        return status;
    }
    vxReleaseReference(&exemplar);

    node->sinks[1].node = node;
    node->sinks[1].pad_index = 1;
    node->sinks[1].node_parameter_index = 4;
    node->sinks[1].num_channels = node_cfg->num_channels;
    exemplar = (vx_reference)vxCreatePyramid(node->graph->tiovx_context,
                                             node_cfg->input_cfg.levels,
                                             node_cfg->input_cfg.scale,
                                             node_cfg->input_cfg.width,
                                             node_cfg->input_cfg.height,
                                             node_cfg->input_cfg.color_format);
    status = tiovx_module_create_pad_exemplar(&node->sinks[1], exemplar);
    if (VX_SUCCESS != status) {
        TIOVX_MODULE_ERROR("[DOF] Create Ref Input Failed\n");
        return status;
    }
    vxReleaseReference(&exemplar);

    node->num_outputs = 1;

    node->srcs[0].node = node;
    node->srcs[0].pad_index = 0;
    node->srcs[0].node_parameter_index = 8;
    node->srcs[0].num_channels = node_cfg->num_channels;
    exemplar = (vx_reference)vxCreateImage(
                                node->graph->tiovx_context,
                                node_cfg->flow_vector_cfg.width,
                                node_cfg->flow_vector_cfg.height,
                                node_cfg->flow_vector_cfg.color_format);
    status = tiovx_module_create_pad_exemplar(&node->srcs[0], exemplar);
    if (VX_SUCCESS != status) {
        TIOVX_MODULE_ERROR("[DOF] Create Output Failed\n");
        return status;
    }
    vxReleaseReference(&exemplar);

    if (node_cfg->enable_temporal_predicton_flow_vector) {
        node->sinks[node->num_inputs].node = node;
        node->sinks[node->num_inputs].pad_index = node->num_inputs;
        node->sinks[node->num_inputs].node_parameter_index = 5;
        node->sinks[node->num_inputs].num_channels = node_cfg->num_channels;
        exemplar = (vx_reference)vxCreateImage(
                                    node->graph->tiovx_context,
                                    node_cfg->flow_vector_cfg.width,
                                    node_cfg->flow_vector_cfg.height,
                                    node_cfg->flow_vector_cfg.color_format);
        status = tiovx_module_create_pad_exemplar(&node->sinks[node->num_inputs],
                                                  exemplar);
        if (VX_SUCCESS != status) {
            TIOVX_MODULE_ERROR("[DOF] Create Flow vector Input Failed\n");
            return status;
        }
        vxReleaseReference(&exemplar);

        node->num_inputs += 1;
    }

    if (node_cfg->enable_output_distribution) {
        node->srcs[node->num_outputs].node = node;
        node->srcs[node->num_outputs].pad_index = node->num_outputs;
        node->srcs[node->num_outputs].node_parameter_index = 9;
        node->srcs[node->num_outputs].num_channels = node_cfg->num_channels;
        exemplar = (vx_reference)vxCreateDistribution(
                                    node->graph->tiovx_context,
                                    node_cfg->distribution_cfg.num_bins,
                                    node_cfg->distribution_cfg.offset,
                                    node_cfg->distribution_cfg.range);
        status = tiovx_module_create_pad_exemplar(&node->srcs[node->num_outputs],
                                                  exemplar);
        if (VX_SUCCESS != status) {
            TIOVX_MODULE_ERROR("[DOF] Create Distribution Output Failed\n");
            return status;
        }
        vxReleaseReference(&exemplar);

        node->num_outputs += 1;
    }

    sprintf(node->name, "dof_node");

    tiovx_dof_module_configure_params(node);

    return status;
}

vx_status tiovx_dof_create_node(NodeObj *node)
{
    vx_status status = VX_FAILURE;
    TIOVXDofNodeCfg *node_cfg = (TIOVXDofNodeCfg *)node->node_cfg;
    TIOVXDofNodePriv *node_priv = (TIOVXDofNodePriv *)node->node_priv;
    vx_image input_flow_vector = NULL;
    vx_distribution output_distribution = NULL;
    vx_bool replicate[] = {
                        vx_false_e, vx_false_e, vx_false_e, vx_true_e, vx_true_e,
                        vx_false_e, vx_false_e, vx_false_e, vx_true_e, vx_false_e
                    };

    if (node_cfg->enable_temporal_predicton_flow_vector) {
        input_flow_vector = (vx_image)(node->sinks[2].exemplar);
        replicate[5] = vx_true_e;
    }

    if (node_cfg->enable_output_distribution) {
        output_distribution = (vx_distribution)(node->srcs[1].exemplar);
        replicate[9] = vx_true_e;
    }

    node->tiovx_node = tivxDmpacDofNode(node->graph->tiovx_graph,
                                        node_priv->dof_params_obj,
                                        NULL,
                                        NULL,
                                        (vx_pyramid)(node->sinks[0].exemplar),
                                        (vx_pyramid)(node->sinks[1].exemplar),
                                        input_flow_vector,
                                        NULL,
                                        NULL,
                                        (vx_image)(node->srcs[0].exemplar),
                                        output_distribution);

    status = vxGetStatus((vx_reference)node->tiovx_node);
    if (VX_SUCCESS != status) {
        TIOVX_MODULE_ERROR("[DOF] Create Node Failed\n");
        return status;
    }

    vxSetNodeTarget(node->tiovx_node, VX_TARGET_STRING, node_cfg->target_string);
    vxReplicateNode(node->graph->tiovx_graph, node->tiovx_node, replicate, 10);

    return status;
}

vx_status tiovx_dof_delete_node(NodeObj *node)
{
    vx_status status = VX_FAILURE;
    TIOVXDofNodePriv *node_priv = (TIOVXDofNodePriv *)node->node_priv;

    status = vxReleaseNode(&node->tiovx_node);

    status = vxReleaseUserDataObject(&node_priv->dof_params_obj);

    return status;
}

vx_uint32 tiovx_dof_get_cfg_size()
{
    return sizeof(TIOVXDofNodeCfg);
}

vx_uint32 tiovx_dof_get_priv_size()
{
    return sizeof(TIOVXDofNodePriv);
}
