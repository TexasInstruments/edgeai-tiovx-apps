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

uint32_t num_params;
uint32_t max_params;

typedef struct {
    vx_kernel                   kernel;
    tivxTIDLJ7Params            params;
    vx_user_data_object         io_config;
    vx_user_data_object         network;
    vx_user_data_object         createParams;
    vx_object_array             in_args_arr;
    vx_object_array             out_args_arr;
} TIOVXTIDLNodePriv;

void init_param(vx_reference params[], uint32_t _max_params)
{
    num_params  = 0;
    max_params = _max_params;
}

void add_param(vx_reference params[], vx_reference obj)
{
    if(num_params <= max_params)
    {
        params[num_params] = obj;
        num_params++;
    }
    else
    {
        TIOVX_MODULE_ERROR("Error! num_params > max_params!\n");
    }
}

vx_enum get_vx_tensor_datatype(int32_t tidl_datatype)
{
    vx_enum tiovx_datatype = VX_TYPE_INVALID;

    if(tidl_datatype == TIDL_UnsignedChar)
        tiovx_datatype = VX_TYPE_UINT8;
    else if(tidl_datatype == TIDL_SignedChar)
        tiovx_datatype = VX_TYPE_INT8;
    else if(tidl_datatype == TIDL_UnsignedShort)
        tiovx_datatype = VX_TYPE_UINT16;
    else if(tidl_datatype == TIDL_SignedShort)
        tiovx_datatype = VX_TYPE_INT16;
    else if(tidl_datatype == TIDL_UnsignedWord)
        tiovx_datatype = VX_TYPE_UINT32;
    else if(tidl_datatype == TIDL_SignedWord)
        tiovx_datatype = VX_TYPE_INT32;
    else if(tidl_datatype == TIDL_SinglePrecFloat)
        tiovx_datatype = VX_TYPE_FLOAT32;

    return tiovx_datatype;
}

vx_user_data_object tiovx_tidl_read_io_config(GraphObj          *graph,
                                              vx_char           *io_config_path,
                                              sTIDL_IOBufDesc_t *io_buf_desc)
{
    vx_status status = VX_FAILURE;
    tivxTIDLJ7Params *tidlParams = NULL;
    sTIDL_IOBufDesc_t *ioBufDesc = NULL;
    vx_uint32 capacity;
    vx_map_id map_id;
    vx_user_data_object io_config = NULL;

    FILE *fp_io_config;
    vx_size read_count;

    fp_io_config = fopen(io_config_path, "rb");

    if(NULL == fp_io_config)
    {
        TIOVX_MODULE_ERROR("[TIDL] Unable to open %s\n", io_config_path);
        return NULL;
    }

    fseek(fp_io_config, 0, SEEK_END);
    capacity = ftell(fp_io_config);
    fseek(fp_io_config, 0, SEEK_SET);

    if(capacity != sizeof(sTIDL_IOBufDesc_t))
    {
        TIOVX_MODULE_ERROR("[TIDL] Config file size (%d bytes) does not match"
                            "size of sTIDL_IOBufDesc_t (%d bytes)\n", capacity,
                            (vx_uint32)sizeof(sTIDL_IOBufDesc_t));
        fclose(fp_io_config);
        return NULL;
    }

    io_config = vxCreateUserDataObject(graph->tiovx_context,
                                       "tivxTIDLJ7Params",
                                        sizeof(tivxTIDLJ7Params),
                                        NULL);
    status = vxGetStatus((vx_reference)(io_config));
    if (VX_SUCCESS != status)
    {
        TIOVX_MODULE_ERROR("[TIDL] Create io_config failed\n");
        fclose(fp_io_config);
        return NULL;
    }

    vxMapUserDataObject(io_config,
                        0,
                        sizeof(tivxTIDLJ7Params),
                        &map_id,
                        (void **)&tidlParams,
                        VX_WRITE_ONLY,
                        VX_MEMORY_TYPE_HOST,
                        0);

    if(NULL == tidlParams)
    {
        TIOVX_MODULE_ERROR("[TIDL] Map of io_config object failed\n");
        fclose(fp_io_config);
        return NULL;
    }

    tivx_tidl_j7_params_init(tidlParams);

    ioBufDesc = (sTIDL_IOBufDesc_t *)&tidlParams->ioBufDesc;
    read_count = fread(ioBufDesc, capacity, 1, fp_io_config);
    if(read_count != 1)
    {
        TIOVX_MODULE_ERROR("[TIDL] Unable to read io_config file\n");
        fclose(fp_io_config);
        return NULL;
    }

    memcpy(io_buf_desc, ioBufDesc, capacity);

    vxUnmapUserDataObject(io_config, map_id);

    fclose(fp_io_config);

    return io_config;
}

vx_status tiovx_tidl_set_cfg(NodeObj *node)
{
    vx_status status = VX_FAILURE;
    TIOVXTIDLNodeCfg *node_cfg = (TIOVXTIDLNodeCfg *)node->node_cfg;
    TIOVXTIDLNodePriv *node_priv = (TIOVXTIDLNodePriv *)node->node_priv;

    vx_int32 i;

    node_priv->io_config = tiovx_tidl_read_io_config(node->graph,
                                                    node_cfg->io_config_path,
                                                    &node_cfg->io_buf_desc);
    if(NULL == node_priv->io_config)
    {
        TIOVX_MODULE_ERROR("[TIDL] Config object is null\n");
        return status;
    }

    node_cfg->num_input_tensors  = node_cfg->io_buf_desc.numInputBuf;
    node_cfg->num_output_tensors = node_cfg->io_buf_desc.numOutputBuf;

    for(i = 0; i < node_cfg->num_input_tensors; i++)
    {
        vx_char name[VX_MAX_REFERENCE_NAME];
        snprintf(name, VX_MAX_REFERENCE_NAME, "tidl_node_input_tensors_%d", i);

        node_cfg->input_cfg[i].num_dims = 3;
        node_cfg->input_cfg[i].datatype = get_vx_tensor_datatype(node_cfg->io_buf_desc.inElementType[i]);

        if (TIDL_LT_NCHW == node_cfg->io_buf_desc.inLayout[i])
        {
            node_cfg->input_cfg[i].dim_sizes[0] = (node_cfg->io_buf_desc.inWidth[i] +
                                                    node_cfg->io_buf_desc.inPadL[i] +
                                                    node_cfg->io_buf_desc.inPadR[i]);

            node_cfg->input_cfg[i].dim_sizes[1] = (node_cfg->io_buf_desc.inHeight[i] +
                                                    node_cfg->io_buf_desc.inPadT[i] +
                                                    node_cfg->io_buf_desc.inPadB[i]);

            node_cfg->input_cfg[i].dim_sizes[2] = node_cfg->io_buf_desc.inNumChannels[i];
        }
        else
        {
            node_cfg->input_cfg[i].dim_sizes[0] = node_cfg->io_buf_desc.inNumChannels[i];

            node_cfg->input_cfg[i].dim_sizes[1] = (node_cfg->io_buf_desc.inWidth[i] +
                                                    node_cfg->io_buf_desc.inPadL[i] +
                                                    node_cfg->io_buf_desc.inPadR[i]);

            node_cfg->input_cfg[i].dim_sizes[2] = (node_cfg->io_buf_desc.inHeight[i] +
                                                    node_cfg->io_buf_desc.inPadT[i] +
                                                    node_cfg->io_buf_desc.inPadB[i]);
        }
    }

    for(i = 0; i < node_cfg->num_output_tensors; i++)
    {
        vx_char name[VX_MAX_REFERENCE_NAME];
        snprintf(name, VX_MAX_REFERENCE_NAME, "tidl_node_output_tensors_%d", i);

        node_cfg->output_cfg[i].num_dims = 3;
        node_cfg->output_cfg[i].datatype = get_vx_tensor_datatype(node_cfg->io_buf_desc.outElementType[i]);

        if (TIDL_LT_NCHW == node_cfg->io_buf_desc.outLayout[i])
        {
            node_cfg->output_cfg[i].dim_sizes[0] = (node_cfg->io_buf_desc.outWidth[i] +
                                                    node_cfg->io_buf_desc.outPadL[i] +
                                                    node_cfg->io_buf_desc.outPadR[i]);

            node_cfg->output_cfg[i].dim_sizes[1] = (node_cfg->io_buf_desc.outHeight[i] +
                                                    node_cfg->io_buf_desc.outPadT[i] +
                                                    node_cfg->io_buf_desc.outPadB[i]);

            node_cfg->output_cfg[i].dim_sizes[2] = node_cfg->io_buf_desc.outNumChannels[i];
        }
        else
        {
            node_cfg->output_cfg[i].dim_sizes[0] = node_cfg->io_buf_desc.outNumChannels[i];

            node_cfg->output_cfg[i].dim_sizes[1] = (node_cfg->io_buf_desc.outWidth[i] +
                                                    node_cfg->io_buf_desc.outPadL[i] +
                                                    node_cfg->io_buf_desc.outPadR[i]);

            node_cfg->output_cfg[i].dim_sizes[2] = (node_cfg->io_buf_desc.outHeight[i] +
                                                    node_cfg->io_buf_desc.outPadT[i] +
                                                    node_cfg->io_buf_desc.outPadB[i]);
        }
    }

    node->num_inputs = node_cfg->num_input_tensors;
    node->num_outputs = node_cfg->num_output_tensors;

    status = VX_SUCCESS;

    return status;
}

vx_status tiovx_tidl_create_io_config(NodeObj *node)
{
    vx_status status = VX_FAILURE;
    TIOVXTIDLNodePriv *node_priv = (TIOVXTIDLNodePriv *)node->node_priv;

    vx_map_id map_id;
    tivxTIDLJ7Params *temp_params;

    vxSetReferenceName((vx_reference)node_priv->io_config, "tidl_node_io_config");

    vxMapUserDataObject(node_priv->io_config,
                        0,
                        sizeof(tivxTIDLJ7Params),
                        &map_id,
                        (void **)&temp_params,
                        VX_WRITE_ONLY,
                        VX_MEMORY_TYPE_HOST,
                        0);

    temp_params->compute_config_checksum  = 0;
    temp_params->compute_network_checksum = 0;

    memcpy(&node_priv->params, temp_params, sizeof(tivxTIDLJ7Params));

    vxUnmapUserDataObject(node_priv->io_config, map_id);

    status = VX_SUCCESS;

    return status;
}

vx_status tiovx_tidl_create_network(NodeObj *node)
{
    vx_status status = VX_FAILURE;
    TIOVXTIDLNodeCfg *node_cfg = (TIOVXTIDLNodeCfg *)node->node_cfg;
    TIOVXTIDLNodePriv *node_priv = (TIOVXTIDLNodePriv *)node->node_priv;

    vx_map_id map_id;
    vx_uint32 capacity;
    FILE *fp_network;
    void *network_buffer = NULL;
    vx_size read_count;

    fp_network = fopen(&node_cfg->network_path[0], "rb");
    if(NULL == fp_network)
    {
        TIOVX_MODULE_ERROR("[TIDL] Unable to open %s\n", node_cfg->network_path);
        return status;
    }

    fseek(fp_network, 0, SEEK_END);
    capacity = ftell(fp_network);
    fseek(fp_network, 0, SEEK_SET);

    node_priv->network = vxCreateUserDataObject(node->graph->tiovx_context,
                                               "TIDL_network",
                                               capacity,
                                               NULL);
    status = vxGetStatus((vx_reference)node_priv->network);
    if (VX_SUCCESS != status)
    {
        TIOVX_MODULE_ERROR("[TIDL] Create network Failed\n");
        fclose(fp_network);
        return status;
    }

    vxSetReferenceName((vx_reference)node_priv->network, "tidl_node_network");

    vxMapUserDataObject(node_priv->network,
                        0,
                        capacity,
                        &map_id,
                        (void **)&network_buffer,
                        VX_WRITE_ONLY,
                        VX_MEMORY_TYPE_HOST,
                        0);

    if(network_buffer)
    {
        read_count = fread(network_buffer, capacity, 1, fp_network);
        if(1 != read_count)
        {
            TIOVX_MODULE_ERROR("[TIDL] Unable to read network\n");
            status = VX_FAILURE;
        }
    }
    else
    {
        TIOVX_MODULE_ERROR("[TIDL] Unable to allocate memory for reading"
                           "network(%d bytes)\n", capacity);
        status = VX_FAILURE;
    }

    vxUnmapUserDataObject(node_priv->network, map_id);

    fclose(fp_network);

    return status;
}

vx_status tiovx_tidl_update_checksums(NodeObj *node)
{
    vx_status status = VX_FAILURE;
    TIOVXTIDLNodeCfg *node_cfg = (TIOVXTIDLNodeCfg *)node->node_cfg;
    TIOVXTIDLNodePriv *node_priv = (TIOVXTIDLNodePriv *)node->node_priv;

    tivxTIDLJ7Params *tidlParams;
    vx_map_id map_id;

    vxMapUserDataObject(node_priv->io_config,
                        0,
                        sizeof(tivxTIDLJ7Params),
                        &map_id,
                        (void **)&tidlParams,
                        VX_WRITE_ONLY,
                        VX_MEMORY_TYPE_HOST,
                        0);

    if(NULL != tidlParams)
    {
        memcpy(tidlParams->config_checksum,
               &node_cfg->io_config_checksum[0],
               TIVX_TIDL_J7_CHECKSUM_SIZE);

        memcpy(tidlParams->network_checksum,
               &node_cfg->network_checksum[0],
               TIVX_TIDL_J7_CHECKSUM_SIZE);

        status = VX_SUCCESS;
    }
    else
    {
        TIOVX_MODULE_ERROR("[TIDL] Unable to copy checksums\n");
    }

    vxUnmapUserDataObject(node_priv->io_config, map_id);

    return status;
}

vx_status tiovx_tidl_set_createParams(NodeObj *node)
{
    vx_status status = VX_FAILURE;
#if defined(SOC_J784S4) || defined(SOC_J722S)
    TIOVXTIDLNodeCfg *node_cfg = (TIOVXTIDLNodeCfg *)node->node_cfg;
#endif
    TIOVXTIDLNodePriv *node_priv = (TIOVXTIDLNodePriv *)node->node_priv;

    vx_map_id  map_id;
    void *createParams_buffer = NULL;

    node_priv->createParams = vxCreateUserDataObject(node->graph->tiovx_context,
                                                    "TIDL_CreateParams",
                                                    sizeof(TIDL_CreateParams),
                                                    NULL);
    status = vxGetStatus((vx_reference)node_priv->createParams);
    if (VX_SUCCESS != status)
    {
        TIOVX_MODULE_ERROR("[TIDL] Create createParams Failed\n");
        return status;
    }

    vxSetReferenceName((vx_reference)node_priv->createParams, "tidl_node_createParams");

    vxMapUserDataObject(node_priv->createParams,
                        0,
                        sizeof(TIDL_CreateParams),
                        &map_id,
                        (void **)&createParams_buffer,
                        VX_WRITE_ONLY,
                        VX_MEMORY_TYPE_HOST,
                        0);

    if(createParams_buffer)
    {
        TIDL_CreateParams *prms = createParams_buffer;
        TIDL_createParamsInit(prms);
        prms->isInbufsPaded                 = 1;
        prms->quantRangeExpansionFactor     = 1.0;
        prms->quantRangeUpdateFactor        = 0.0;
        prms->traceLogLevel                 = 0;
        prms->traceWriteLevel               = 0;
#if defined(SOC_J784S4)
        if (0 == strcmp(TIVX_TARGET_DSP_C7_2, node_cfg->target_string))
        {
            prms->coreId = 1;
        }
        else if (0 == strcmp(TIVX_TARGET_DSP_C7_3, node_cfg->target_string))
        {
            prms->coreId = 2;
        }
        else if (0 == strcmp(TIVX_TARGET_DSP_C7_4, node_cfg->target_string))
        {
            prms->coreId = 3;
        }
#elif defined(SOC_J722S)
        if (0 == strcmp(TIVX_TARGET_DSP_C7_2, node_cfg->target_string))
        {
            prms->coreId = 1;
        }
#endif

    }
    else
    {
        TIOVX_MODULE_ERROR("[TIDL] Unable to allocate memory for"
                           "createParams(%ld bytes)\n", sizeof(TIDL_CreateParams));
        status = VX_FAILURE;
    }

    vxUnmapUserDataObject(node_priv->createParams, map_id);

    return status;
}

vx_status tiovx_tidl_create_inArgs(NodeObj *node)
{
    vx_status status = VX_FAILURE;
    TIOVXTIDLNodeCfg *node_cfg = (TIOVXTIDLNodeCfg *)node->node_cfg;
    TIOVXTIDLNodePriv *node_priv = (TIOVXTIDLNodePriv *)node->node_priv;

    vx_user_data_object inArgs;
    vx_int32 i;

    inArgs = vxCreateUserDataObject(node->graph->tiovx_context,
                                    "TIDL_InArgs",
                                    sizeof(TIDL_InArgs),
                                    NULL);
    node_priv->in_args_arr  = vxCreateObjectArray(node->graph->tiovx_context,
                                                 (vx_reference)inArgs,
                                                 node_cfg->num_channels);
    vxReleaseUserDataObject(&inArgs);

    vxSetReferenceName((vx_reference)node_priv->in_args_arr, "tidl_node_inArgsArray");

    for(i = 0; i < node_cfg->num_channels; i++)
    {
        vx_user_data_object inArgs;
        vx_map_id  map_id;
        void *inArgs_buffer = NULL;

        inArgs = (vx_user_data_object)vxGetObjectArrayItem(node_priv->in_args_arr, i);
        status = vxGetStatus((vx_reference)inArgs);
        if (VX_SUCCESS != status)
        {
            TIOVX_MODULE_ERROR("[TIDL] Create inArgs(%d) Failed\n", i);
            break;
        }

        vxMapUserDataObject(inArgs,
                            0,
                            sizeof(TIDL_InArgs),
                            &map_id,
                            (void **)&inArgs_buffer,
                            VX_WRITE_ONLY,
                            VX_MEMORY_TYPE_HOST,
                            0);

        if(inArgs_buffer)
        {
            TIDL_InArgs *prms                = inArgs_buffer;
            prms->iVisionInArgs.size         = sizeof(TIDL_InArgs);
            prms->iVisionInArgs.subFrameInfo = 0;
        }
        else
        {
            TIOVX_MODULE_ERROR("[TIDL] Unable to allocate memory for"
                               "inArgs(%d)(%ld bytes)\n", i, sizeof(TIDL_InArgs));
            status = VX_FAILURE;
        }

        vxUnmapUserDataObject(inArgs, map_id);

        vxReleaseUserDataObject(&inArgs);

        if (VX_SUCCESS != status)
            break;
    }

    return status;
}

vx_status tiovx_tidl_create_outArgs(NodeObj *node)
{
    vx_status status = VX_FAILURE;
    TIOVXTIDLNodeCfg *node_cfg = (TIOVXTIDLNodeCfg *)node->node_cfg;
    TIOVXTIDLNodePriv *node_priv = (TIOVXTIDLNodePriv *)node->node_priv;

    vx_user_data_object outArgs;
    vx_int32 i;

    outArgs = vxCreateUserDataObject(node->graph->tiovx_context,
                                     "TIDL_outArgs",
                                     sizeof(TIDL_outArgs),
                                     NULL);

    node_priv->out_args_arr  = vxCreateObjectArray(node->graph->tiovx_context,
                                                  (vx_reference)outArgs,
                                                  node_cfg->num_channels);

    vxReleaseUserDataObject(&outArgs);

    vxSetReferenceName((vx_reference)node_priv->out_args_arr, "tidl_node_outArgsArray");

    for(i = 0; i < node_cfg->num_channels; i++)
    {
        vx_user_data_object outArgs;
        void *outArgs_buffer = NULL;
        vx_map_id  map_id;

        outArgs = (vx_user_data_object)vxGetObjectArrayItem(node_priv->out_args_arr, i);
        status = vxGetStatus((vx_reference)outArgs);
        if (VX_SUCCESS != status)
        {
            TIOVX_MODULE_ERROR("[TIDL] Create outArgs(%d) Failed\n", i);
            break;
        }

        vxMapUserDataObject(outArgs,
                            0,
                            sizeof(TIDL_outArgs),
                            &map_id,
                            (void **)&outArgs_buffer,
                            VX_WRITE_ONLY,
                            VX_MEMORY_TYPE_HOST,
                            0);

        if(outArgs_buffer)
        {
            TIDL_outArgs *prms = outArgs_buffer;
            prms->iVisionOutArgs.size  = sizeof(TIDL_outArgs);
        }
        else
        {
            TIOVX_MODULE_ERROR("[TIDL] Unable to allocate memory for"
                               "outArgs(%d)(%ld bytes)\n", i, sizeof(TIDL_outArgs));

            status = VX_FAILURE;
        }

        vxUnmapUserDataObject(outArgs, map_id);

        vxReleaseUserDataObject(&outArgs);

        if (VX_SUCCESS != status)
            break;
    }

    return status;
}

void tiovx_tidl_init_cfg(TIOVXTIDLNodeCfg *node_cfg)
{
    CLR(node_cfg);
    node_cfg->num_input_tensors = 1;
    node_cfg->num_output_tensors = 1;
    node_cfg->num_channels = 1;
    sprintf(node_cfg->target_string, TIVX_TARGET_DSP_C7_1);
    return;
}

vx_status tiovx_tidl_init_node(NodeObj *node)
{
    vx_status status = VX_FAILURE;
    TIOVXTIDLNodeCfg *node_cfg = (TIOVXTIDLNodeCfg *)node->node_cfg;
    TIOVXTIDLNodePriv *node_priv = (TIOVXTIDLNodePriv *)node->node_priv;
    vx_reference exemplar;
    vx_size tensor_sizes[TIOVX_MODULES_MAX_TENSOR_DIMS];
    vx_int32 i;

    CLR(node_priv);

    status = tiovx_tidl_set_cfg(node);
    if(VX_SUCCESS != status)
    {
        TIOVX_MODULE_ERROR("[TIDL] Set TIDL cfg failed\n");
        return status;
    }

    status = tiovx_tidl_create_io_config(node);
    if(VX_SUCCESS != status)
    {
        TIOVX_MODULE_ERROR("[TIDL] Create io_config failed\n");
        return status;
    }

    status = tiovx_tidl_create_network(node);
    if(VX_SUCCESS != status)
    {
        TIOVX_MODULE_ERROR("[TIDL] Create network failed\n");
        return status;
    }

    status = tiovx_tidl_update_checksums(node);
    if(VX_SUCCESS != status)
    {
        TIOVX_MODULE_ERROR("[TIDL] Update checksums failed\n");
        return status;
    }

    status = tiovx_tidl_set_createParams(node);
    if(VX_SUCCESS != status)
    {
        TIOVX_MODULE_ERROR("[TIDL] Set creatParams failed\n");
        return status;
    }

    status = tiovx_tidl_create_inArgs(node);
    if(VX_SUCCESS != status)
    {
        TIOVX_MODULE_ERROR("[TIDL] Create inArgs failed\n");
        return status;
    }

    status = tiovx_tidl_create_outArgs(node);
    if(VX_SUCCESS != status)
    {
        TIOVX_MODULE_ERROR("[TIDL] Create outArgs failed\n");
        return status;
    }

    for(i = 0; i < node_cfg->num_input_tensors; i++)
    {
        tensor_sizes[0] = node_cfg->input_cfg[i].dim_sizes[0];
        tensor_sizes[1] = node_cfg->input_cfg[i].dim_sizes[1];
        tensor_sizes[2] = node_cfg->input_cfg[i].dim_sizes[2];

        node->sinks[i].node = node;
        node->sinks[i].pad_index = i;
        node->sinks[i].node_parameter_index = (6 + i);
        node->sinks[i].num_channels = node_cfg->num_channels;
        exemplar = (vx_reference)vxCreateTensor(node->graph->tiovx_context,
                                                node_cfg->input_cfg[i].num_dims,
                                                tensor_sizes,
                                                node_cfg->input_cfg[i].datatype,
                                                0);
        status = tiovx_module_create_pad_exemplar(&node->sinks[i], exemplar);
        if (VX_SUCCESS != status)
        {
            TIOVX_MODULE_ERROR("[TIDL] Create Input %d Failed\n", i);
            return status;
        }
        vxReleaseReference(&exemplar);
    }

    for(i = 0; i < node_cfg->num_output_tensors; i++)
    {
        tensor_sizes[0] = node_cfg->output_cfg[i].dim_sizes[0];
        tensor_sizes[1] = node_cfg->output_cfg[i].dim_sizes[1];
        tensor_sizes[2] = node_cfg->output_cfg[i].dim_sizes[2];

        node->srcs[i].node = node;
        node->srcs[i].pad_index = i;
        node->srcs[i].node_parameter_index = (6 + node_cfg->num_input_tensors + i);
        node->srcs[i].num_channels = node_cfg->num_channels;
        exemplar = (vx_reference)vxCreateTensor(node->graph->tiovx_context,
                                                node_cfg->output_cfg[i].num_dims,
                                                tensor_sizes,
                                                node_cfg->output_cfg[i].datatype,
                                                0);
        status = tiovx_module_create_pad_exemplar(&node->srcs[i], exemplar);
        if (VX_SUCCESS != status)
        {
            TIOVX_MODULE_ERROR("[TIDL] Create Output %i Failed\n", i);
            return status;
        }
        vxReleaseReference(&exemplar);
    }

    sprintf(node->name, "tidl_node");

    node_priv->kernel = tivxAddKernelTIDL(node->graph->tiovx_context,
                                         node_cfg->num_input_tensors,
                                         node_cfg->num_output_tensors);
    status = vxGetStatus((vx_reference)node_priv->kernel);
    if (VX_SUCCESS != status)
        TIOVX_MODULE_ERROR("[TIDL] tivxAddKernelTIDL Failed\n");

    return status;
}

vx_status tiovx_tidl_create_node(NodeObj *node)
{
    vx_status status = VX_FAILURE;
    TIOVXTIDLNodeCfg *node_cfg = (TIOVXTIDLNodeCfg *)node->node_cfg;
    TIOVXTIDLNodePriv *node_priv = (TIOVXTIDLNodePriv *)node->node_priv;

    vx_user_data_object inArgs;
    vx_user_data_object outArgs;
    vx_reference params[TIOVX_MODULES_MAX_PARAMS];
    vx_tensor input_tensors[TIOVX_MODULES_MAX_TENSORS];
    vx_tensor output_tensors[TIOVX_MODULES_MAX_TENSORS];
    vx_int32 i;
    vx_bool replicate[16];
    vx_uint32 replicate_idx;

    inArgs = (vx_user_data_object)vxGetObjectArrayItem(node_priv->in_args_arr, 0);
    outArgs = (vx_user_data_object)vxGetObjectArrayItem(node_priv->out_args_arr, 0);

    init_param(params, TIOVX_MODULES_MAX_PARAMS);
    add_param(params, (vx_reference)node_priv->io_config);
    add_param(params, (vx_reference)node_priv->network);
    add_param(params, (vx_reference)node_priv->createParams);
    add_param(params, (vx_reference)inArgs);
    add_param(params, (vx_reference)outArgs);
    add_param(params, NULL);

    vxReleaseUserDataObject(&inArgs);
    vxReleaseUserDataObject(&outArgs);

    for(i = 0; i < node_cfg->num_input_tensors; i++)
        input_tensors[i] = (vx_tensor)(node->sinks[i].exemplar);

    for(i = 0; i < node_cfg->num_output_tensors; i++)
        output_tensors[i] = (vx_tensor)(node->srcs[i].exemplar);

    node->tiovx_node = tivxTIDLNode(node->graph->tiovx_graph,
                                    node_priv->kernel,
                                    params,
                                    input_tensors,
                                    output_tensors);
    status = vxGetStatus((vx_reference)node->tiovx_node);
    if (VX_SUCCESS != status)
    {
        TIOVX_MODULE_ERROR("[TIDL] Create Node Failed\n");
        return status;
    }

    vxSetReferenceName((vx_reference)node->tiovx_node, "tidl_node");
    vxSetNodeTarget(node->tiovx_node, VX_TARGET_STRING, node_cfg->target_string);

    replicate[TIVX_KERNEL_TIDL_IN_CONFIG_IDX] = vx_false_e;
    replicate[TIVX_KERNEL_TIDL_IN_NETWORK_IDX] = vx_false_e;
    replicate[TIVX_KERNEL_TIDL_IN_CREATE_PARAMS_IDX] = vx_false_e;
    replicate[TIVX_KERNEL_TIDL_IN_IN_ARGS_IDX] = vx_true_e;
    replicate[TIVX_KERNEL_TIDL_IN_OUT_ARGS_IDX] = vx_true_e;
    replicate[TIVX_KERNEL_TIDL_IN_TRACE_DATA_IDX] = vx_false_e;

    for(i = 0; i < node_cfg->num_input_tensors; i++)
    {
        replicate_idx = TIVX_KERNEL_TIDL_NUM_BASE_PARAMETERS + i;
        replicate[replicate_idx] = vx_true_e;
    }

    for(i = 0; i < node_cfg->num_output_tensors; i++)
    {
        replicate_idx = (TIVX_KERNEL_TIDL_NUM_BASE_PARAMETERS +
                         node_cfg->num_input_tensors +
                         i);
        replicate[replicate_idx] = vx_true_e;
    }

    replicate_idx = (TIVX_KERNEL_TIDL_NUM_BASE_PARAMETERS +
                     node_cfg->num_input_tensors +
                     node_cfg->num_output_tensors);

    vxReplicateNode(node->graph->tiovx_graph,
                    node->tiovx_node, replicate, replicate_idx);

    for(i = 0; i < node_cfg->num_input_tensors; i++)
        vxReleaseTensor(&input_tensors[i]);

    for(i = 0; i < node_cfg->num_output_tensors; i++)
        vxReleaseTensor(&output_tensors[i]);

    return status;
}

vx_status tiovx_tidl_delete_node(NodeObj *node)
{
    vx_status status = VX_FAILURE;
    TIOVXTIDLNodePriv *node_priv = (TIOVXTIDLNodePriv *)node->node_priv;

    status = vxReleaseNode(&node->tiovx_node);

    status = vxReleaseUserDataObject(&node_priv->io_config);
    status = vxReleaseUserDataObject(&node_priv->network);
    status = vxReleaseUserDataObject(&node_priv->createParams);
    status = vxReleaseObjectArray(&node_priv->in_args_arr);
    status = vxReleaseObjectArray(&node_priv->out_args_arr);

    vxRemoveKernel(node_priv->kernel);

    return status;
}

vx_uint32 tiovx_tidl_get_cfg_size()
{
    return sizeof(TIOVXTIDLNodeCfg);
}

vx_uint32 tiovx_tidl_get_priv_size()
{
    return sizeof(TIOVXTIDLNodePriv);
}
