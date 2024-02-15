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
#include "tiovx_aewb_module.h"

#define TIOVX_MODULES_DEFAULT_AEWB_SENSOR "SENSOR_SONY_IMX390_UB953_D3"

typedef struct {
    vx_object_array         config_arr;
    vx_user_data_object     dcc_config;
    vx_object_array         histogram_arr;
    vx_object_array         aewb_out_arr;
    tivx_aewb_config_t      params;
    SensorObj               sensor_obj;
} TIOVXAewbNodePriv;

vx_status tiovx_aewb_create_dcc(NodeObj *node)
{
    vx_status status = VX_FAILURE;
    TIOVXAewbNodePriv *node_priv = (TIOVXAewbNodePriv *)node->node_priv;

    if(node_priv->sensor_obj.sensor_dcc_enabled)
    {
        int32_t     dcc_buff_size;
        uint8_t     *dcc_buf;
        vx_map_id   dcc_buf_map_id;

        dcc_buff_size = appIssGetDCCSize2A(node_priv->sensor_obj.sensor_name,
                                        node_priv->sensor_obj.sensor_wdr_enabled);
        if(dcc_buff_size < 0)
        {
            TIOVX_MODULE_ERROR("[AEWB] Invalid DCC size for 2A! \n");
            status = VX_FAILURE;
            return status;
        }

        node_priv->dcc_config = vxCreateUserDataObject(node->graph->tiovx_context,
                                                       "dcc_2a",
                                                       dcc_buff_size,
                                                       NULL);
        status = vxGetStatus((vx_reference)node_priv->dcc_config);
        if(VX_SUCCESS != status)
        {
            TIOVX_MODULE_ERROR("[AEWB] Unable to create DCC config object!\n");
            return status;
        }

        vxSetReferenceName((vx_reference)node_priv->dcc_config,
                            "aewb_node_dcc_config");

        vxMapUserDataObject(node_priv->dcc_config,
                            0,
                            dcc_buff_size,
                            &dcc_buf_map_id,
                            (void **)&dcc_buf,
                            VX_WRITE_ONLY,
                            VX_MEMORY_TYPE_HOST,
                            0);

        status = appIssGetDCCBuff2A(node_priv->sensor_obj.sensor_name,
                                    node_priv->sensor_obj.sensor_wdr_enabled,
                                    dcc_buf,
                                    dcc_buff_size);
        if(status != VX_SUCCESS)
        {
            TIOVX_MODULE_ERROR("[AEWB] Error getting 2A DCC buffer! \n");
            return status;
        }
        vxUnmapUserDataObject(node_priv->dcc_config, dcc_buf_map_id);
    }
    else
    {
        node_priv->dcc_config = NULL;
        status = VX_SUCCESS;
        TIOVX_MODULE_PRINTF("[AEWB-MODULE] Sensor DCC is disabled \n");
    }

    return status;
}

vx_status tiovx_aewb_create_config(NodeObj *node)
{
    vx_status status = VX_FAILURE;
    TIOVXAewbNodeCfg *node_cfg = (TIOVXAewbNodeCfg *)node->node_cfg;
    TIOVXAewbNodePriv *node_priv = (TIOVXAewbNodePriv *)node->node_priv;
    vx_user_data_object config;
    vx_int32 ch;
    vx_int32 ch_mask;
    vx_uint32 array_obj_index;

    node_priv->params.sensor_dcc_id = node_priv->sensor_obj.sensorParams.dccId;
    node_priv->params.sensor_img_format = 0;
    node_priv->params.sensor_img_phase = 3;

    if(node_priv->sensor_obj.sensor_exp_control_enabled ||
       node_priv->sensor_obj.sensor_gain_control_enabled)
    {
        node_priv->params.ae_mode = ALGORITHMS_ISS_AE_AUTO;
    } else {
        node_priv->params.ae_mode = ALGORITHMS_ISS_AE_DISABLED;
    }

    node_priv->params.awb_mode = node_cfg->awb_mode;
    node_priv->params.awb_num_skip_frames = node_cfg->awb_num_skip_frames;
    node_priv->params.ae_num_skip_frames = node_cfg->ae_num_skip_frames;
    node_priv->params.channel_id = node_priv->sensor_obj.starting_channel;

    config = vxCreateUserDataObject(node->graph->tiovx_context,
                                    "tivx_aewb_config_t",
                                    sizeof(tivx_aewb_config_t),
                                    &node_priv->params);
    status = vxGetStatus((vx_reference)config);
    if(VX_SUCCESS !=status)
    {
        TIOVX_MODULE_ERROR("[AEWB] Unable to create AEWB config object! \n");
        return status;
    }

    node_priv->config_arr = vxCreateObjectArray(node->graph->tiovx_context,
                                                (vx_reference)config,
                                                node_priv->sensor_obj.num_cameras_enabled);
    status = vxGetStatus((vx_reference)node_priv->config_arr);
    if(VX_SUCCESS != status)
    {
        TIOVX_MODULE_ERROR("[AEWB] Unable to create AEWB config object array! \n");
        return status;
    }
    vxReleaseUserDataObject(&config);

    vxSetReferenceName((vx_reference)node_priv->config_arr, "aewb_node_config_arr");

    array_obj_index = 0;
    ch = 0;
    ch_mask = node_priv->sensor_obj.ch_mask >> node_priv->sensor_obj.starting_channel;
    while(ch_mask > 0)
    {
        if(ch_mask & 0x1)
        {
            vx_user_data_object config = (vx_user_data_object)vxGetObjectArrayItem(node_priv->config_arr,
                                                                                   array_obj_index);
            array_obj_index++;
            node_priv->params.channel_id = ch + node_priv->sensor_obj.starting_channel;
            vxCopyUserDataObject(config,
                                 0,
                                 sizeof(tivx_aewb_config_t),
                                 &node_priv->params,
                                 VX_WRITE_ONLY,
                                 VX_MEMORY_TYPE_HOST);
            vxReleaseUserDataObject(&config);
        }
        ch++;
        ch_mask = ch_mask >> 1;
        if (ch >= node_priv->sensor_obj.num_cameras_enabled)
        {
            break;
        }
    }

    return status;
}

vx_status tiovx_aewb_create_aewb_out(NodeObj *node)
{
    vx_status status = VX_FAILURE;
    TIOVXAewbNodePriv *node_priv = (TIOVXAewbNodePriv *)node->node_priv;
    vx_reference exemplar;

    exemplar = (vx_reference)vxCreateUserDataObject(node->graph->tiovx_context,
                                                    "tivx_ae_awb_params_t",
                                                    sizeof(tivx_ae_awb_params_t),
                                                    NULL);

    node_priv->aewb_out_arr = vxCreateObjectArray(node->graph->tiovx_context,
                                                 exemplar,
                                                 node_priv->sensor_obj.num_cameras_enabled);
    status = vxGetStatus((vx_reference)node_priv->aewb_out_arr);
    if(VX_SUCCESS != status)
    {
        TIOVX_MODULE_ERROR("[AEWB] Unable to create aewb in object array! \n");
    }

    vxReleaseReference(&exemplar);

    return status;
}

vx_status tiovx_aewb_create_histogram(NodeObj *node)
{
    vx_status status = VX_FAILURE;
    TIOVXAewbNodePriv *node_priv = (TIOVXAewbNodePriv *)node->node_priv;

    vx_distribution histogram;

    histogram = vxCreateDistribution(node->graph->tiovx_context, 256, 0, 256);
    status = vxGetStatus((vx_reference)histogram);
    if(VX_SUCCESS != status)
    {
        TIOVX_MODULE_ERROR("[AEWB] Unable to create Histogram object! \n");
        return status;
    }

    node_priv->histogram_arr = vxCreateObjectArray(node->graph->tiovx_context,
                                                   (vx_reference)histogram,
                                                   node_priv->sensor_obj.num_cameras_enabled);
    status = vxGetStatus((vx_reference)node_priv->histogram_arr);
    if(VX_SUCCESS != status)
    {
        TIOVX_MODULE_ERROR("[AEWB] Unable to create Histogram object array! \n");
        vxReleaseDistribution(&histogram);
        return status;
    }

    vxSetReferenceName((vx_reference)node_priv->histogram_arr, "aewb_node_histogram_arr");
    vxReleaseDistribution(&histogram);

    return status;
}

void tiovx_aewb_init_cfg(TIOVXAewbNodeCfg *node_cfg)
{
    sprintf(node_cfg->sensor_name, TIOVX_MODULES_DEFAULT_AEWB_SENSOR);
    node_cfg->awb_mode = ALGORITHMS_ISS_AWB_AUTO;
    node_cfg->awb_num_skip_frames = 9;
    node_cfg->ae_num_skip_frames  = 9;
    node_cfg->ch_mask  = 1;
}

vx_status tiovx_aewb_init_node(NodeObj *node)
{
    vx_status status = VX_FAILURE;
    TIOVXAewbNodeCfg *node_cfg = (TIOVXAewbNodeCfg *)node->node_cfg;
    TIOVXAewbNodePriv *node_priv = (TIOVXAewbNodePriv *)node->node_priv;
    vx_reference exemplar;

    CLR(node_priv);

    status = tiovx_init_sensor_obj(&node_priv->sensor_obj,
                                    node_cfg->sensor_name);
    if (VX_SUCCESS != status) {
        TIOVX_MODULE_ERROR("[AEWB] Sensor Init Failed\n");
        return status;
    }

    node_priv->sensor_obj.ch_mask = node_cfg->ch_mask;
    status = tiovx_sensor_query(&node_priv->sensor_obj);
    if (VX_SUCCESS != status)
    {
        TIOVX_MODULE_ERROR("[AEWB] Sensor Query Failed\n");
        return status;
    }

    status = tiovx_aewb_create_dcc(node);
    if (VX_SUCCESS != status)
    {
        TIOVX_MODULE_ERROR("[AEWB] Create dcc failed\n");
        return status;
    }

    status = tiovx_aewb_create_config(node);
    if (VX_SUCCESS != status)
    {
        TIOVX_MODULE_ERROR("[AEWB] Create config failed\n");
        return status;
    }

    status = tiovx_aewb_create_histogram(node);
    if (VX_SUCCESS != status)
    {
        TIOVX_MODULE_ERROR("[AEWB] Create histogram failed\n");
        return status;
    }

    status = tiovx_aewb_create_aewb_out(node);
    if (VX_SUCCESS != status)
    {
        TIOVX_MODULE_ERROR("[AEWB] Create aewb in failed\n");
        return status;
    }

    node->num_inputs = 1;
    node->num_outputs = 0;

    node->sinks[0].node = node;
    node->sinks[0].pad_index = 0;
    node->sinks[0].node_parameter_index = 2;
    node->sinks[0].num_channels = node_priv->sensor_obj.num_cameras_enabled;

    exemplar = (vx_reference)vxCreateUserDataObject(node->graph->tiovx_context,
                                                    "tivx_h3a_data_t",
                                                    sizeof(tivx_h3a_data_t),
                                                    NULL);
    status = tiovx_module_create_pad_exemplar(&node->sinks[0], exemplar);
    if (VX_SUCCESS != status)
    {
        TIOVX_MODULE_ERROR("[AEWB] Create Input Failed\n");
        return status;
    }

    vxReleaseReference(&exemplar);

    return status;
}

vx_status tiovx_aewb_create_node(NodeObj *node)
{
    vx_status status = VX_FAILURE;
    TIOVXAewbNodePriv *node_priv = (TIOVXAewbNodePriv *)node->node_priv;

    vx_bool replicate[] = { vx_true_e,  vx_true_e, vx_true_e,
                            vx_false_e, vx_true_e, vx_false_e};

    vx_user_data_object config;
    vx_user_data_object aewb_out;
    vx_distribution     histogram;

    config = (vx_user_data_object)vxGetObjectArrayItem(node_priv->config_arr, 0);
    histogram  = (vx_distribution)vxGetObjectArrayItem(node_priv->histogram_arr, 0);
    aewb_out  = (vx_user_data_object)vxGetObjectArrayItem(node_priv->aewb_out_arr, 0);

    node->tiovx_node = tivxAewbNode(
                                node->graph->tiovx_graph,
                                config,
                                histogram,
                                (vx_user_data_object)(node->sinks[0].exemplar),
                                NULL,
                                aewb_out,
                                node_priv->dcc_config);

    vxReleaseUserDataObject(&config);
    vxReleaseDistribution(&histogram);
    vxReleaseUserDataObject(&aewb_out);

    status = vxGetStatus((vx_reference)node->tiovx_node);
    if (VX_SUCCESS != status)
    {
        TIOVX_MODULE_ERROR("[AEWB] Create Node Failed\n");
        return status;
    }

#if defined(SOC_AM62A)
    vxSetNodeTarget(node->tiovx_node, VX_TARGET_STRING, TIVX_TARGET_MPU_0);
#else
    vxSetNodeTarget(node->tiovx_node, VX_TARGET_STRING, TIVX_TARGET_MCU2_0);
#endif

    vxSetReferenceName((vx_reference)node->tiovx_node, "aewb_node");
    vxReplicateNode(node->graph->tiovx_graph, node->tiovx_node, replicate, 6);

    return status;
}

vx_status tiovx_aewb_delete_node(NodeObj *node)
{
    vx_status status = VX_FAILURE;
    TIOVXAewbNodePriv *node_priv = (TIOVXAewbNodePriv *)node->node_priv;

    status = vxReleaseNode(&node->tiovx_node);

    vxReleaseObjectArray(&node_priv->histogram_arr);
    vxReleaseObjectArray(&node_priv->config_arr);
    vxReleaseObjectArray(&node_priv->aewb_out_arr);

    if(node_priv->dcc_config != NULL)
    {
        vxReleaseUserDataObject(&node_priv->dcc_config);
    }
    return status;
}

vx_uint32 tiovx_aewb_get_cfg_size()
{
    return sizeof(TIOVXAewbNodeCfg);
}

vx_uint32 tiovx_aewb_get_priv_size()
{
    return sizeof(TIOVXAewbNodePriv);
}
