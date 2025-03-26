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
#include "tiovx_capture_module.h"

#include <tiovx_utils.h>

#define TIOVX_MODULES_DEFAULT_CAPTURE_SENSOR "SENSOR_SONY_IMX390_UB953_D3"

typedef struct {
    SensorObj               sensor_obj;
    vx_user_data_object     config;
    tivx_raw_image          error_frame_raw_image;
    vx_uint32               capture_format;
    tivx_capture_params_t   params;
} TIOVXCaptureNodePriv;

vx_status tiovx_capture_configure_params(NodeObj *node)
{
    vx_status status = VX_FAILURE;
    TIOVXCaptureNodeCfg *node_cfg = (TIOVXCaptureNodeCfg *)node->node_cfg;
    TIOVXCaptureNodePriv *node_priv = (TIOVXCaptureNodePriv *)node->node_priv;

    vx_uint32   num_capt_instances = 0;
    vx_int32    id, lane, ch, vcNum;
    int32_t     ch_mask = node_priv->sensor_obj.ch_mask;

    if (ch_mask <= 0xF)
    {
        num_capt_instances = 1;
    }
    else if ((ch_mask > 0xF) && (ch_mask <= 0xFF))
    {
        num_capt_instances = 2;
    }
#if defined(SOC_J784S4)
    else if ((ch_mask > 0xFF) && (ch_mask <= 0xFFF))
    {
        num_capt_instances = 3;
    }
#endif
    else
    {
        TIOVX_MODULE_ERROR("[CAPTURE] - ch_mask parameter is invalid! \n");
        status = VX_ERROR_INVALID_PARAMETERS;
        return status;
    }

    node_priv->capture_format = node_priv->sensor_obj.sensor_out_format;

    tivx_capture_params_init(&node_priv->params);

    if (node_cfg->enable_error_detection)
    {
        node_priv->params.timeout        = 90;
        node_priv->params.timeoutInitial = 500;
    }
    node_priv->params.numInst  = num_capt_instances;
    node_priv->params.numCh    = node_priv->sensor_obj.num_cameras_enabled;

    for(id = 0; id < num_capt_instances; id++)
    {
        node_priv->params.instId[id]                       = id;
        node_priv->params.instCfg[id].enableCsiv2p0Support = (uint32_t)vx_true_e;
        node_priv->params.instCfg[id].numDataLanes         = node_priv->sensor_obj.sensorParams.sensorInfo.numDataLanes;
        node_priv->params.instCfg[id].laneBandSpeed        = node_priv->sensor_obj.sensorParams.sensorInfo.csi_laneBandSpeed;

        for (lane = 0; lane < node_priv->params.instCfg[id].numDataLanes; lane++)
        {
            node_priv->params.instCfg[id].dataLanesMap[lane] = lane + 1;
        }
    }

    ch = 0;     /*Camera Physical Channel Number*/
    vcNum = 0;  /*CSI2 Virtual Channel Number*/
    id = 0;     /*CSI2 Instance ID*/

    while(ch_mask > 0)
    {
        if(ch > 7)
        {
            id = 2;
        }
        else if( (ch > 3) && (ch <= 7) )
        {
            id = 1;
        }
        else
        {
            id = 0;
        }

        if(ch_mask & 0x1)
        {
            node_priv->params.chVcNum[vcNum] = ch % 4;
            node_priv->params.chInstMap[vcNum] = id;
            vcNum++;
        }

        ch++;
        ch_mask = ch_mask >> 1;
    }

    status = VX_SUCCESS;

    return status;
}

vx_status tiovx_capture_create_config(NodeObj *node)
{
    vx_status status = VX_FAILURE;
    TIOVXCaptureNodePriv *node_priv = (TIOVXCaptureNodePriv *)node->node_priv;

    node_priv->config = vxCreateUserDataObject(node->graph->tiovx_context,
                                               "tivx_capture_params_t",
                                               sizeof(tivx_capture_params_t),
                                               &node_priv->params);
    status = vxGetStatus((vx_reference)node_priv->config);
    if (VX_SUCCESS != status)
    {
        TIOVX_MODULE_ERROR("[CAPTURE] Create config failed\n");
        return status;
    }

    vxSetReferenceName((vx_reference)node_priv->config, "capture_config");

    return status;
}

vx_status tiovx_capture_create_error_detection_frame(NodeObj *node)
{
    vx_status status = VX_FAILURE;
    TIOVXCaptureNodeCfg *node_cfg = (TIOVXCaptureNodeCfg *)node->node_cfg;
    TIOVXCaptureNodePriv *node_priv = (TIOVXCaptureNodePriv *)node->node_priv;

    IssSensor_CreateParams *sensor_params = &node_priv->sensor_obj.sensorParams;
    IssSensor_Info         *sensor_info = &sensor_params->sensorInfo;

    vx_uint32 bytes_read;

    /*Error detection is currently enabled only for RAW input*/
    if(0 != node_priv->capture_format)
    {
        node_cfg->enable_error_detection = 0;
    }

    /* If error detection is enabled, send the test frame */
    if (1 == node_cfg->enable_error_detection)
    {
        node_priv->error_frame_raw_image = tivxCreateRawImage(node->graph->tiovx_context,
                                                              &sensor_info->raw_params);

        status = vxGetStatus((vx_reference)node_priv->error_frame_raw_image);
        if(VX_SUCCESS != status)
        {
            TIOVX_MODULE_ERROR("[CAPTURE] Unable to create error frame RAW image!\n");
            return status;
        }

        vxSetReferenceName((vx_reference)node_priv->error_frame_raw_image,
                           "capture_node_error_frame_raw_image");

        status = readRawImage(node_cfg->error_frame_filename,
                              node_priv->error_frame_raw_image,
                              &bytes_read);

        status = vxGetStatus((vx_reference)node_priv->error_frame_raw_image);
        if(VX_SUCCESS != VX_SUCCESS)
        {
            TIOVX_MODULE_ERROR("[CAPTURE] Unable to populate error frame RAW image!\n");
            return status;
        }

        if(bytes_read < 0)
        {
            TIOVX_MODULE_ERROR("[CAPTURE] Bytes read by error frame for RAW image is < 0!\n");
            tivxReleaseRawImage(&node_priv->error_frame_raw_image);
            node_priv->error_frame_raw_image = NULL;
            status = VX_FAILURE;
            return status;
        }
    }
    else
    {
        node_priv->error_frame_raw_image = NULL;
    }

    status = VX_SUCCESS;
    return status;
}

vx_status tiovx_capture_module_send_error_frame(NodeObj *node)
{
    vx_status status = VX_SUCCESS;
    TIOVXCaptureNodePriv *node_priv = (TIOVXCaptureNodePriv *)node->node_priv;

    if (NULL != node_priv->error_frame_raw_image)
    {
        status = tivxCaptureRegisterErrorFrame(node->tiovx_node,
                                               (vx_reference)node_priv->error_frame_raw_image);
    }

    return status;
}

void tiovx_capture_init_cfg(TIOVXCaptureNodeCfg *node_cfg)
{
    CLR(node_cfg);
    sprintf(node_cfg->sensor_name, TIOVX_MODULES_DEFAULT_CAPTURE_SENSOR);
    node_cfg->ch_mask = 1;
    node_cfg->usecase_option = TIOVX_SENSOR_MODULE_FEATURE_CFG_UC0;
    node_cfg->enable_error_detection = 0;
    sprintf(node_cfg->target_string, TIVX_TARGET_CAPTURE1);
}

vx_status tiovx_capture_init_node(NodeObj *node)
{
    vx_status status = VX_FAILURE;
    TIOVXCaptureNodeCfg *node_cfg = (TIOVXCaptureNodeCfg *)node->node_cfg;
    TIOVXCaptureNodePriv *node_priv = (TIOVXCaptureNodePriv *)node->node_priv;
    vx_reference exemplar;
    IssSensor_CreateParams *sensor_params;

    CLR(node_priv);

    status = tiovx_init_sensor_obj(&node_priv->sensor_obj, node_cfg->sensor_name);
    if (VX_SUCCESS != status)
    {
        TIOVX_MODULE_ERROR("[CAPTURE] Init Sensor Obj Failed\n");
        return status;
    }

    node_priv->sensor_obj.ch_mask = node_cfg->ch_mask;
    node_priv->sensor_obj.usecase_option = node_cfg->usecase_option;

    status = tiovx_sensor_query(&node_priv->sensor_obj);
    if (VX_SUCCESS != status)
    {
        TIOVX_MODULE_ERROR("[CAPTURE] Sensor Query Failed\n");
        return status;
    }

    status = tiovx_sensor_init(&node_priv->sensor_obj);
    if (VX_SUCCESS != status)
    {
        TIOVX_MODULE_ERROR("[CAPTURE] Sensor Init Failed\n");
        return status;
    }

    status = tiovx_capture_configure_params(node);
    if (VX_SUCCESS != status)
    {
        TIOVX_MODULE_ERROR("[CAPTURE] Configure params failed\n");
        return status;
    }

    status = tiovx_capture_create_config(node);
    if (VX_SUCCESS != status)
    {
        TIOVX_MODULE_ERROR("[CAPTURE] Create config failed\n");
        return status;
    }

    status = tiovx_capture_create_error_detection_frame(node);
    if (VX_SUCCESS != status)
    {
        TIOVX_MODULE_ERROR("[CAPTURE] Create error detection frame failed\n");
        return status;
    }

    sensor_params = &node_priv->sensor_obj.sensorParams;

    node->num_inputs = 0;
    node->num_outputs = 1;

    node->srcs[0].node = node;
    node->srcs[0].pad_index = 0;
    node->srcs[0].node_parameter_index = 1;
    node->srcs[0].num_channels = node_priv->sensor_obj.num_cameras_enabled;
    node->srcs[0].enqueue_arr = vx_true_e;

    if (0 == node_priv->capture_format) // RAW
    {
        exemplar = (vx_reference)tivxCreateRawImage(node->graph->tiovx_context,
                                                    &sensor_params->sensorInfo.raw_params);
    }
    else                                //YUV
    {
        exemplar = (vx_reference)vxCreateImage(node->graph->tiovx_context,
                                               sensor_params->sensorInfo.raw_params.width,
                                               sensor_params->sensorInfo.raw_params.height,
                                               VX_DF_IMAGE_UYVY);
    }

    status = tiovx_module_create_pad_exemplar(&node->srcs[0], exemplar);
    if (VX_SUCCESS != status)
    {
        TIOVX_MODULE_ERROR("[CAPTURE] Create Output Failed\n");
        return status;
    }
    vxReleaseReference(&exemplar);

    sprintf(node->name, "capture_node");

    return status;
}

vx_status tiovx_capture_create_node(NodeObj *node)
{
    vx_status status = VX_FAILURE;
    TIOVXCaptureNodeCfg *node_cfg = (TIOVXCaptureNodeCfg *)node->node_cfg;
    TIOVXCaptureNodePriv *node_priv = (TIOVXCaptureNodePriv *)node->node_priv;

    node->tiovx_node = tivxCaptureNode(node->graph->tiovx_graph,
                                       node_priv->config,
                                       node->srcs[0].exemplar_arr);
    status = vxGetStatus((vx_reference)node->tiovx_node);
    if (VX_SUCCESS != status)
    {
        TIOVX_MODULE_ERROR("[CAPTURE] Create Node Failed\n");
        return status;
    }

    vxSetNodeTarget(node->tiovx_node,
                    VX_TARGET_STRING, node_cfg->target_string);

    vxSetReferenceName((vx_reference)node->tiovx_node, "capture_node");

    return status;
}

vx_status tiovx_capture_delete_node(NodeObj *node)
{
    vx_status status = VX_FAILURE;
    TIOVXCaptureNodePriv *node_priv = (TIOVXCaptureNodePriv *)node->node_priv;

    status = tiovx_sensor_stop(&node_priv->sensor_obj);

    tiovx_sensor_deinit(&node_priv->sensor_obj);

    status = vxReleaseNode(&node->tiovx_node);

    status = vxReleaseUserDataObject(&node_priv->config);

    if(NULL != node_priv->error_frame_raw_image)
    {
        tivxReleaseRawImage(&node_priv->error_frame_raw_image);
    }

    return status;
}

vx_status tiovx_capture_post_verify_graph(NodeObj *node)
{
    vx_status status = VX_FAILURE;
    TIOVXCaptureNodePriv *node_priv = (TIOVXCaptureNodePriv *)node->node_priv;

    status = tiovx_sensor_start(&node_priv->sensor_obj);

    status = tiovx_capture_module_send_error_frame(node);

    return status;
}

vx_uint32 tiovx_capture_get_cfg_size()
{
    return sizeof(TIOVXCaptureNodeCfg);
}

vx_uint32 tiovx_capture_get_priv_size()
{
    return sizeof(TIOVXCaptureNodePriv);
}
