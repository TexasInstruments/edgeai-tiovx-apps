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
#include "tiovx_dl_pre_proc_module.h"
#include "tiovx_tidl_module.h"

typedef struct {
    vx_user_data_object io_config;
} TIOVXDLPreProcNodePriv;

vx_status tiovx_dl_pre_proc_create_io_config(NodeObj *node)
{
    vx_status status = VX_FAILURE;
    TIOVXDLPreProcNodeCfg *node_cfg = (TIOVXDLPreProcNodeCfg *)node->node_cfg;
    TIOVXDLPreProcNodePriv *node_priv = (TIOVXDLPreProcNodePriv *)node->node_priv;
    vx_map_id map_id;
    tivxDLPreProcArmv8Params *params;

    node_priv->io_config = vxCreateUserDataObject(node->graph->tiovx_context,
                                                 "tivxDLPreProcArmv8Params",
                                                 sizeof(tivxDLPreProcArmv8Params),
                                                 NULL);
    status = vxGetStatus((vx_reference)node_priv->io_config);
    if (VX_SUCCESS != status)
    {
        TIOVX_MODULE_ERROR("[DL_PRE_PROC] Create IO config failed\n");
        return status;
    }

    vxSetReferenceName((vx_reference)node_priv->io_config, "dl_pre_proc_io_config");

    vxMapUserDataObject(node_priv->io_config,
                        0,
                        sizeof(tivxDLPreProcArmv8Params),
                        &map_id,
                        (void **)&params,
                        VX_WRITE_ONLY,
                        VX_MEMORY_TYPE_HOST,
                        0);

    memcpy(params, &node_cfg->params, sizeof(tivxDLPreProcArmv8Params));

    vxUnmapUserDataObject(node_priv->io_config, map_id);

    return status;
}

vx_status tiovx_dl_pre_proc_set_cfg(NodeObj *node)
{
    vx_status status = VX_FAILURE;
    TIOVXDLPreProcNodeCfg *node_cfg = (TIOVXDLPreProcNodeCfg *)node->node_cfg;

    vx_user_data_object temp;

    node->num_inputs = 1;
    node->num_outputs = 1;

    temp = tiovx_tidl_read_io_config(node->graph,
                                     node_cfg->io_config_path,
                                     &node_cfg->io_buf_desc);
    if (NULL == temp)
    {
        TIOVX_MODULE_ERROR("[DL_PRE_PROC] Read io_config failed\n");
        return status;
    }

    node_cfg->output_cfg.datatype =
                get_vx_tensor_datatype(node_cfg->io_buf_desc.inElementType[0]);

    node_cfg->output_cfg.num_dims = 3;

    if(TIDL_LT_NCHW == node_cfg->io_buf_desc.inLayout[0])
    {
        node_cfg->params.channel_order = 0;

        node_cfg->output_cfg.dim_sizes[0] = node_cfg->io_buf_desc.inWidth[0]+
                                            node_cfg->io_buf_desc.inPadL[0] +
                                            node_cfg->io_buf_desc.inPadR[0];

        node_cfg->output_cfg.dim_sizes[1] = node_cfg->io_buf_desc.inHeight[0]+
                                            node_cfg->io_buf_desc.inPadT[0]  +
                                            node_cfg->io_buf_desc.inPadB[0];

        node_cfg->output_cfg.dim_sizes[2] = node_cfg->io_buf_desc.inNumChannels[0];

        node_cfg->input_cfg.width = node_cfg->output_cfg.dim_sizes[0];
        node_cfg->input_cfg.height = node_cfg->output_cfg.dim_sizes[1];
    }
    else
    {
        node_cfg->params.channel_order = 1;

        node_cfg->output_cfg.dim_sizes[0] = node_cfg->io_buf_desc.inNumChannels[0];

        node_cfg->output_cfg.dim_sizes[1] = node_cfg->io_buf_desc.inWidth[0]+
                                            node_cfg->io_buf_desc.inPadL[0] +
                                            node_cfg->io_buf_desc.inPadR[0];

        node_cfg->output_cfg.dim_sizes[2] = node_cfg->io_buf_desc.inHeight[0]+
                                            node_cfg->io_buf_desc.inPadT[0]  +
                                            node_cfg->io_buf_desc.inPadB[0];

        node_cfg->input_cfg.width = node_cfg->output_cfg.dim_sizes[1];
        node_cfg->input_cfg.height = node_cfg->output_cfg.dim_sizes[2];
    }

    node_cfg->width = node_cfg->input_cfg.width;
    node_cfg->height = node_cfg->input_cfg.height;

    vxReleaseUserDataObject(&temp);

    status = VX_SUCCESS;

    return status;
}

void tiovx_dl_pre_proc_init_cfg(TIOVXDLPreProcNodeCfg *node_cfg)
{
    node_cfg->width = TIOVX_MODULES_DEFAULT_IMAGE_WIDTH;
    node_cfg->height = TIOVX_MODULES_DEFAULT_IMAGE_HEIGHT;
    node_cfg->input_cfg.color_format = TIOVX_MODULES_DEFAULT_COLOR_FORMAT;
    node_cfg->num_channels = 1;
    sprintf(node_cfg->target_string, TIVX_TARGET_A72_0);
    node_cfg->params.channel_order = 0;
    node_cfg->params.tensor_format = 1;
    node_cfg->params.scale[0] = 1.0;
    node_cfg->params.scale[1] = 1.0;
    node_cfg->params.scale[2] = 1.0;
    node_cfg->params.mean[0] = 0.0;
    node_cfg->params.mean[1] = 0.0;
    node_cfg->params.mean[2] = 0.0;
    node_cfg->params.crop[0] = 0;
    node_cfg->params.crop[1] = 0;
    node_cfg->params.crop[2] = 0;
    node_cfg->params.crop[3] = 0;

}

vx_status tiovx_dl_pre_proc_init_node(NodeObj *node)
{
    vx_status status = VX_FAILURE;
    TIOVXDLPreProcNodeCfg *node_cfg = (TIOVXDLPreProcNodeCfg *)node->node_cfg;
    vx_reference exemplar;
    vx_size tensor_sizes[TIVX_CONTEXT_MAX_TENSOR_DIMS];
    vx_int32 i;

    status = tiovx_dl_pre_proc_set_cfg(node);
    if(VX_SUCCESS != status)
    {
        TIOVX_MODULE_ERROR("[DL_PRE_PROC] Set Pre Proc Node cfg failed\n");
        return status;
    }

    status = tiovx_dl_pre_proc_create_io_config(node);
    if (VX_SUCCESS != status)
    {
        TIOVX_MODULE_ERROR("[DL_PRE_PROC] Create IO config failed\n");
        return status;
    }


    for(i = 0; i < node_cfg->output_cfg.num_dims; i++)
    {
        tensor_sizes[i] = (vx_size) node_cfg->output_cfg.dim_sizes[i];
    }

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
        TIOVX_MODULE_ERROR("[DL_PRE_PROC] Create Input Failed\n");
        return status;
    }
    vxReleaseReference(&exemplar);

    node->srcs[0].node = node;
    node->srcs[0].pad_index = 0;
    node->srcs[0].node_parameter_index = 2;
    node->srcs[0].num_channels = node_cfg->num_channels;
    exemplar = (vx_reference)vxCreateTensor(node->graph->tiovx_context,
                                            node_cfg->output_cfg.num_dims,
                                            tensor_sizes,
                                            node_cfg->output_cfg.datatype,
                                            0);
    status = tiovx_module_create_pad_exemplar(&node->srcs[0], exemplar);
    if (VX_SUCCESS != status)
    {
        TIOVX_MODULE_ERROR("[DL_PRE_PROC] Create Output Failed\n");
        return status;
    }
    vxReleaseReference(&exemplar);

    sprintf(node->name, "dl_pre_proc_node");

    return status;
}

vx_status tiovx_dl_pre_proc_create_node(NodeObj *node)
{
    vx_status status = VX_FAILURE;
    TIOVXDLPreProcNodeCfg *node_cfg = (TIOVXDLPreProcNodeCfg *)node->node_cfg;
    TIOVXDLPreProcNodePriv *node_priv = (TIOVXDLPreProcNodePriv *)node->node_priv;
    vx_bool replicate[] = { vx_false_e, vx_true_e, vx_true_e };

    node->tiovx_node = tivxDLPreProcArmv8Node(node->graph->tiovx_graph,
                                node_priv->io_config,
                                (vx_image)(node->sinks[0].exemplar),
                                (vx_tensor)(node->srcs[0].exemplar));
    status = vxGetStatus((vx_reference)node->tiovx_node);
    if (VX_SUCCESS != status)
    {
        TIOVX_MODULE_ERROR("[DL_PRE_PROC] Create Node Failed\n");
        return status;
    }

    vxSetNodeTarget(node->tiovx_node,
                    VX_TARGET_STRING, node_cfg->target_string);
    vxReplicateNode(node->graph->tiovx_graph,
                    node->tiovx_node, replicate, 3);

    return status;
}

vx_status tiovx_dl_pre_proc_delete_node(NodeObj *node)
{
    vx_status status = VX_FAILURE;
    TIOVXDLPreProcNodePriv *node_priv = (TIOVXDLPreProcNodePriv *)node->node_priv;

    status = vxReleaseNode(&node->tiovx_node);

    status = vxReleaseUserDataObject(&node_priv->io_config);

    return status;
}

vx_uint32 tiovx_dl_pre_proc_get_cfg_size()
{
    return sizeof(TIOVXDLPreProcNodeCfg);
}

vx_uint32 tiovx_dl_pre_proc_get_priv_size()
{
    return sizeof(TIOVXDLPreProcNodePriv);
}

