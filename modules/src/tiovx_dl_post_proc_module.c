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
#include "tiovx_tidl_module.h"
#include "tiovx_dl_post_proc_module.h"

typedef struct {
    vx_kernel               kernel;
    vx_user_data_object     io_config;
} TIOVXDLPostProcNodePriv;

vx_status tiovx_dl_post_proc_create_io_config(NodeObj *node)
{
    vx_status status = VX_FAILURE;
    TIOVXDLPostProcNodeCfg *node_cfg = (TIOVXDLPostProcNodeCfg *)node->node_cfg;
    TIOVXDLPostProcNodePriv *node_priv = (TIOVXDLPostProcNodePriv *)node->node_priv;

    vx_map_id map_id;
    tivxDLPostProcParams *params;

    node_priv->io_config = vxCreateUserDataObject(node->graph->tiovx_context,
                                                "tivxDLPostProcParams",
                                                sizeof(tivxDLPostProcParams),
                                                NULL);
    status = vxGetStatus((vx_reference)node_priv->io_config);
    if (VX_SUCCESS != status)
    {
        TIOVX_MODULE_ERROR("[DL_POST_PROC] Create io_config failed\n");
        return status;
    }

    vxSetReferenceName((vx_reference)node_priv->io_config,
                        "dl_post_proc_node_io_config");

    vxMapUserDataObject(node_priv->io_config,
                        0,
                        sizeof(tivxDLPostProcParams),
                        &map_id,
                        (void **)&params,
                        VX_WRITE_ONLY,
                        VX_MEMORY_TYPE_HOST,
                        0);

    memcpy(params, &node_cfg->params, sizeof(tivxDLPostProcParams));

    vxUnmapUserDataObject(node_priv->io_config, map_id);

    return status;
}

vx_status tiovx_dl_post_proc_set_cfg(NodeObj *node)
{
    vx_status status = VX_FAILURE;
    TIOVXDLPostProcNodeCfg *node_cfg = (TIOVXDLPostProcNodeCfg *)node->node_cfg;

    vx_user_data_object temp;
    vx_int32 i;

    node_cfg->input_image_cfg.width = node_cfg->width;
    node_cfg->input_image_cfg.height = node_cfg->height;

    node_cfg->output_image_cfg.width = node_cfg->width;
    node_cfg->output_image_cfg.height = node_cfg->height;

    temp = tiovx_tidl_read_io_config(node->graph,
                                     node_cfg->io_config_path,
                                     &node_cfg->io_buf_desc);

    if (NULL == temp)
    {
        TIOVX_MODULE_ERROR("[DL_POST_PROC] Read io_config failed\n");
        return status;
    }

    node_cfg->num_input_tensors = node_cfg->io_buf_desc.numOutputBuf;

    node_cfg->params.num_input_tensors = node_cfg->num_input_tensors;
    node_cfg->params.oc_prms.ioBufDesc = &node_cfg->io_buf_desc;
    node_cfg->params.od_prms.ioBufDesc = &node_cfg->io_buf_desc;
    node_cfg->params.ss_prms.ioBufDesc = &node_cfg->io_buf_desc;
    node_cfg->params.ss_prms.inDataWidth = node_cfg->io_buf_desc.outWidth[0];
    node_cfg->params.ss_prms.inDataHeight = node_cfg->io_buf_desc.outHeight[0];

    for(i=0; i < node_cfg->num_input_tensors; i++)
    {
        node_cfg->input_tensor_cfg[i].datatype =
                get_vx_tensor_datatype(node_cfg->io_buf_desc.outElementType[i]);

        node_cfg->input_tensor_cfg[i].num_dims = 3;

        if(TIDL_LT_NCHW == node_cfg->io_buf_desc.outLayout[i])
        {
            node_cfg->input_tensor_cfg[i].dim_sizes[0] = (node_cfg->io_buf_desc.outWidth[i] +
                                                        node_cfg->io_buf_desc.outPadL[i] +
                                                        node_cfg->io_buf_desc.outPadR[i]);

            node_cfg->input_tensor_cfg[i].dim_sizes[1] = (node_cfg->io_buf_desc.outHeight[i] +
                                                        node_cfg->io_buf_desc.outPadT[i] +
                                                        node_cfg->io_buf_desc.outPadB[i]);

            node_cfg->input_tensor_cfg[i].dim_sizes[2] = node_cfg->io_buf_desc.outNumChannels[i];
        }
        else
        {
            node_cfg->input_tensor_cfg[i].dim_sizes[0] = node_cfg->io_buf_desc.outNumChannels[i];

            node_cfg->input_tensor_cfg[i].dim_sizes[1] = (node_cfg->io_buf_desc.outWidth[i] +
                                                        node_cfg->io_buf_desc.outPadL[i] +
                                                        node_cfg->io_buf_desc.outPadR[i]);

            node_cfg->input_tensor_cfg[i].dim_sizes[2] = (node_cfg->io_buf_desc.outHeight[i] +
                                                        node_cfg->io_buf_desc.outPadT[i] +
                                                        node_cfg->io_buf_desc.outPadB[i]);
        }
    }

    node->num_inputs = 1 + node_cfg->num_input_tensors;
    node->num_outputs = 1;

    vxReleaseUserDataObject(&temp);

    status = VX_SUCCESS;

    return status;
}

void tiovx_dl_post_proc_init_cfg(TIOVXDLPostProcNodeCfg *node_cfg)
{
    node_cfg->width = TIOVX_MODULES_DEFAULT_IMAGE_WIDTH;
    node_cfg->height = TIOVX_MODULES_DEFAULT_IMAGE_HEIGHT;
    node_cfg->input_image_cfg.color_format = TIOVX_MODULES_DEFAULT_COLOR_FORMAT;
    node_cfg->output_image_cfg.color_format = TIOVX_MODULES_DEFAULT_COLOR_FORMAT;
    sprintf(node_cfg->target_string, TIVX_TARGET_MPU_0);
    node_cfg->num_channels = 1;
    node_cfg->num_input_tensors = 1;
    return;
}

vx_status tiovx_dl_post_proc_init_node(NodeObj *node)
{
    vx_status status = VX_FAILURE;
    TIOVXDLPostProcNodeCfg *node_cfg = (TIOVXDLPostProcNodeCfg *)node->node_cfg;
    TIOVXDLPostProcNodePriv *node_priv = (TIOVXDLPostProcNodePriv *)node->node_priv;
    vx_reference exemplar;
    vx_size tensor_sizes[TIOVX_MODULES_MAX_TENSOR_DIMS];
    vx_int32 i;

    CLR(node_priv);

    status = tiovx_dl_post_proc_set_cfg(node);
    if(VX_SUCCESS != status)
    {
        TIOVX_MODULE_ERROR("[DL_POST_PROC] Set Post Proc Node cfg failed\n");
        return status;
    }

    status = tiovx_dl_post_proc_create_io_config(node);
    if(VX_SUCCESS != status)
    {
        TIOVX_MODULE_ERROR("[DL_POST_PROC] Create io_config failed\n");
        return status;
    }

    for(i = 0; i < node_cfg->num_input_tensors; i++)
    {

        tensor_sizes[0] = node_cfg->input_tensor_cfg[i].dim_sizes[0];
        tensor_sizes[1] = node_cfg->input_tensor_cfg[i].dim_sizes[1];
        tensor_sizes[2] = node_cfg->input_tensor_cfg[i].dim_sizes[2];

        node->sinks[i].node = node;
        node->sinks[i].pad_index = i;
        node->sinks[i].node_parameter_index = (3 + i);
        node->sinks[i].num_channels = node_cfg->num_channels;
        exemplar = (vx_reference)vxCreateTensor(node->graph->tiovx_context,
                                                node_cfg->input_tensor_cfg[i].num_dims,
                                                tensor_sizes,
                                                node_cfg->input_tensor_cfg[i].datatype,
                                                0);
        status = tiovx_module_create_pad_exemplar(&node->sinks[i], exemplar);
        if (VX_SUCCESS != status)
        {
            TIOVX_MODULE_ERROR("[DL_POST_PROC] Create Tensor %d failed\n", i);
            return status;
        }
        vxReleaseReference(&exemplar);
    }


    // Image pad will always be the last one
    node->sinks[node->num_inputs - 1].node = node;
    node->sinks[node->num_inputs - 1].pad_index = node->num_inputs - 1;
    node->sinks[node->num_inputs - 1].node_parameter_index = 1;
    node->sinks[node->num_inputs - 1].num_channels = node_cfg->num_channels;
    exemplar = (vx_reference)vxCreateImage(node->graph->tiovx_context,
                                           node_cfg->input_image_cfg.width,
                                           node_cfg->input_image_cfg.height,
                                           node_cfg->input_image_cfg.color_format);
    status = tiovx_module_create_pad_exemplar(&node->sinks[node->num_inputs - 1],
                                              exemplar);
    if (VX_SUCCESS != status)
    {
        TIOVX_MODULE_ERROR("[DL_PRE_PROC] Create Input Image failed\n");
        return status;
    }
    vxReleaseReference(&exemplar);

    node->srcs[0].node = node;
    node->srcs[0].pad_index = 0;
    node->srcs[0].node_parameter_index = 2;
    node->srcs[0].num_channels = node_cfg->num_channels;
    exemplar = (vx_reference)vxCreateImage(node->graph->tiovx_context,
                                           node_cfg->output_image_cfg.width,
                                           node_cfg->output_image_cfg.height,
                                           node_cfg->output_image_cfg.color_format);
    status = tiovx_module_create_pad_exemplar(&node->srcs[0], exemplar);
    if (VX_SUCCESS != status)
    {
        TIOVX_MODULE_ERROR("[DL_PRE_PROC] Create Output Image failed\n");
        return status;
    }
    vxReleaseReference(&exemplar);

    sprintf(node->name, "dl_post_proc_node");

    node_priv->kernel = tivxAddKernelDLPostProc(node->graph->tiovx_context,
                                                node_cfg->num_input_tensors);
    status = vxGetStatus((vx_reference)node_priv->kernel);
    if (VX_SUCCESS != status)
        TIOVX_MODULE_ERROR("[DL_POST_PROC] tivxAddKernelDLPostProc failed\n");

    return status;
}

vx_status tiovx_dl_post_proc_create_node(NodeObj *node)
{
    vx_status status = VX_FAILURE;
    TIOVXDLPostProcNodeCfg *node_cfg = (TIOVXDLPostProcNodeCfg *)node->node_cfg;
    TIOVXDLPostProcNodePriv *node_priv = (TIOVXDLPostProcNodePriv *)node->node_priv;

    vx_image input_image, output_image;
    vx_tensor input_tensors[TIOVX_MODULES_MAX_TENSORS];
    vx_int32 i;
    vx_bool replicate[8];


    for(i = 0; i < node_cfg->num_input_tensors; i++)
        input_tensors[i] = (vx_tensor)(node->sinks[i].exemplar);

    input_image = (vx_image)(node->sinks[node->num_inputs - 1].exemplar);
    output_image = (vx_image)(node->srcs[0].exemplar);

    node->tiovx_node = tivxDLPostProcNode(node->graph->tiovx_graph,
                                          node_priv->kernel,
                                          node_priv->io_config,
                                          input_image,
                                          input_tensors,
                                          output_image);
    status = vxGetStatus((vx_reference)node->tiovx_node);
    if (VX_SUCCESS != status)
    {
        TIOVX_MODULE_ERROR("[DL_POST_PROC] Create Node failed\n");
        return status;
    }

    vxSetReferenceName((vx_reference)node->tiovx_node, "dl_post_proc_node");
    vxSetNodeTarget(node->tiovx_node, VX_TARGET_STRING, node_cfg->target_string);

    replicate[TIVX_DL_POST_PROC_CONFIG_IDX] = vx_false_e;
    replicate[TIVX_DL_POST_PROC_INPUT_IMAGE_IDX] = vx_true_e;
    replicate[TIVX_DL_POST_PROC_OUTPUT_IMAGE_IDX] = vx_true_e;

    for(i = 0; i < node_cfg->num_input_tensors; i++)
    {
        replicate[TIVX_DL_POST_PROC_INPUT_TENSOR_START_IDX + i] = vx_true_e;
    }

    vxReplicateNode(node->graph->tiovx_graph,
                    node->tiovx_node, replicate,
                    TIVX_DL_POST_PROC_BASE_PARAMS + node_cfg->num_input_tensors);

    vxReleaseImage(&input_image);

    for(i = 0; i < node_cfg->num_input_tensors; i++)
        vxReleaseTensor(&input_tensors[i]);

    vxReleaseImage(&output_image);

    return status;
}

vx_status tiovx_dl_post_proc_delete_node(NodeObj *node)
{
    vx_status status = VX_FAILURE;
    TIOVXDLPostProcNodePriv *node_priv = (TIOVXDLPostProcNodePriv *)node->node_priv;

    status = vxReleaseNode(&node->tiovx_node);

    status = vxReleaseUserDataObject(&node_priv->io_config);

    vxRemoveKernel(node_priv->kernel);

    return status;
}

vx_uint32 tiovx_dl_post_proc_get_cfg_size()
{
    return sizeof(TIOVXDLPostProcNodeCfg);
}

vx_uint32 tiovx_dl_post_proc_get_priv_size()
{
    return sizeof(TIOVXDLPostProcNodePriv);
}
