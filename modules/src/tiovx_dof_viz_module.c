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
#include "tiovx_dof_viz_module.h"
#include <TI/hwa_dmpac_dof.h>

#define TIOVX_MODULES_DEFAULT_DOF_VIZ_WIDTH 1280
#define TIOVX_MODULES_DEFAULT_DOF_VIZ_HEIGHT 720

typedef struct {
    vx_scalar confidence_threshold_obj;
    vx_object_array confidence_output_arr;
    vx_image confidence_output;
} TIOVXDofVizNodePriv;

void tiovx_dof_viz_init_cfg(TIOVXDofVizNodeCfg *node_cfg)
{
    node_cfg->width = TIOVX_MODULES_DEFAULT_DOF_VIZ_WIDTH;
    node_cfg->height = TIOVX_MODULES_DEFAULT_DOF_VIZ_HEIGHT;
    node_cfg->num_channels = 1;
    sprintf(node_cfg->target_string, TIVX_TARGET_DSP1);

    node_cfg->input_cfg.color_format = VX_DF_IMAGE_U32;
    node_cfg->output_cfg.color_format = VX_DF_IMAGE_RGB;
    node_cfg->confidence_output_cfg.color_format = VX_DF_IMAGE_U8;

    node_cfg->confidence_threshold_value = 9;
    node_cfg->enable_confidence_output_pad = 0;
}

vx_status tiovx_dof_viz_init_node(NodeObj *node)
{
    vx_status status = VX_FAILURE;
    TIOVXDofVizNodeCfg *node_cfg = (TIOVXDofVizNodeCfg *)node->node_cfg;
    TIOVXDofVizNodePriv *node_priv = (TIOVXDofVizNodePriv *)node->node_priv;
    vx_reference exemplar;

    CLR(node_priv);

    node_cfg->input_cfg.width = node_cfg->width;
    node_cfg->input_cfg.height = node_cfg->height;

    node->num_inputs = 1;

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
        TIOVX_MODULE_ERROR("[DOF_VIZ] Create Input Failed\n");
        return status;
    }
    vxReleaseReference(&exemplar);

    node->num_outputs = 1;

    node_cfg->output_cfg.width = node_cfg->width;
    node_cfg->output_cfg.height = node_cfg->height;

    node->srcs[0].node = node;
    node->srcs[0].pad_index = 0;
    node->srcs[0].node_parameter_index = 2;
    node->srcs[0].num_channels = node_cfg->num_channels;
    exemplar = (vx_reference)vxCreateImage(
                                node->graph->tiovx_context,
                                node_cfg->output_cfg.width,
                                node_cfg->output_cfg.height,
                                node_cfg->output_cfg.color_format);
    status = tiovx_module_create_pad_exemplar(&node->srcs[0], exemplar);
    if (VX_SUCCESS != status) {
        TIOVX_MODULE_ERROR("[DOF_VIZ] Create Output Failed\n");
        return status;
    }
    vxReleaseReference(&exemplar);

    node_cfg->confidence_output_cfg.width = node_cfg->width;
    node_cfg->confidence_output_cfg.height = node_cfg->height;

    if (node_cfg->enable_confidence_output_pad) {
        node->srcs[1].node = node;
        node->srcs[1].pad_index = 1;
        node->srcs[1].node_parameter_index = 3;
        node->srcs[1].num_channels = node_cfg->num_channels;
        exemplar = (vx_reference)vxCreateImage(
                                  node->graph->tiovx_context,
                                  node_cfg->confidence_output_cfg.width,
                                  node_cfg->confidence_output_cfg.height,
                                  node_cfg->confidence_output_cfg.color_format);
        status = tiovx_module_create_pad_exemplar(&node->srcs[1], exemplar);
        if (VX_SUCCESS != status) {
            TIOVX_MODULE_ERROR("[DOF_VIZ] Create Confidence Output Failed\n");
            return status;
        }
        vxReleaseReference(&exemplar);

        node->num_outputs += 1;
    } else {
        exemplar = (vx_reference)vxCreateImage(
                                  node->graph->tiovx_context,
                                  node_cfg->confidence_output_cfg.width,
                                  node_cfg->confidence_output_cfg.height,
                                  node_cfg->confidence_output_cfg.color_format);

        node_priv->confidence_output_arr = vxCreateObjectArray(
                                                    node->graph->tiovx_context,
                                                    exemplar,
                                                    node_cfg->num_channels);
        node_priv->confidence_output = (vx_image)vxGetObjectArrayItem(
                                            node_priv->confidence_output_arr, 0);
        vxReleaseReference(&exemplar);
    }

    sprintf(node->name, "dof_viz_node");

    return status;
}

vx_status tiovx_dof_viz_create_node(NodeObj *node)
{
    vx_status status = VX_FAILURE;
    TIOVXDofVizNodeCfg *node_cfg = (TIOVXDofVizNodeCfg *)node->node_cfg;
    TIOVXDofVizNodePriv *node_priv = (TIOVXDofVizNodePriv *)node->node_priv;
    vx_bool replicate[] = {vx_true_e, vx_false_e, vx_true_e, vx_true_e};
    vx_image confidence_output = NULL;

    node_priv->confidence_threshold_obj = vxCreateScalar(
                                        node->graph->tiovx_context,
                                        VX_TYPE_UINT32,
                                        &node_cfg->confidence_threshold_value);

    if (node_cfg->enable_confidence_output_pad) {
        confidence_output = (vx_image)node->srcs[1].exemplar;
    } else {
        confidence_output = node_priv->confidence_output;
    }
    node->tiovx_node = tivxDofVisualizeNode(
                                        node->graph->tiovx_graph,
                                        (vx_image)(node->sinks[0].exemplar),
                                        node_priv->confidence_threshold_obj,
                                        (vx_image)(node->srcs[0].exemplar),
                                        confidence_output
                                        );

    status = vxGetStatus((vx_reference)node->tiovx_node);
    if (VX_SUCCESS != status) {
        TIOVX_MODULE_ERROR("[DOF_VIZ] Create Node Failed\n");
        return status;
    }

    vxSetNodeTarget(node->tiovx_node, VX_TARGET_STRING, node_cfg->target_string);
    vxReplicateNode(node->graph->tiovx_graph, node->tiovx_node, replicate, 4);

    return status;
}

vx_status tiovx_dof_viz_delete_node(NodeObj *node)
{
    vx_status status = VX_FAILURE;
    TIOVXDofVizNodeCfg *node_cfg = (TIOVXDofVizNodeCfg *)node->node_cfg;
    TIOVXDofVizNodePriv *node_priv = (TIOVXDofVizNodePriv *)node->node_priv;

    status = vxReleaseNode(&node->tiovx_node);

    status = vxReleaseScalar(&node_priv->confidence_threshold_obj);
    if (!node_cfg->enable_confidence_output_pad) {
        vxReleaseImage(&node_priv->confidence_output);
        vxReleaseObjectArray(&node_priv->confidence_output_arr);
    }

    return status;
}

vx_uint32 tiovx_dof_viz_get_cfg_size()
{
    return sizeof(TIOVXDofVizNodeCfg);
}

vx_uint32 tiovx_dof_viz_get_priv_size()
{
    return sizeof(TIOVXDofVizNodePriv);
}
