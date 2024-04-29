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
#include "tiovx_viss_module.h"

#define TIOVX_VISS_MODULE_OUTPUT_NA (0)
#define TIOVX_VISS_MODULE_OUTPUT_EN (1)
#define TIOVX_MODULES_DEFAULT_VISS_DCC_FILE "/opt/imaging/imx219/linear/dcc_viss.bin"
#define TIOVX_MODULES_DEFAULT_VISS_SENSOR "SENSOR_SONY_IMX219_RPI"
#define TIOVX_MODULES_DEFAULT_VISS_WIDTH 1920
#define TIOVX_MODULES_DEFAULT_VISS_HEIGHT 1080

typedef struct {
    vx_user_data_object         viss_params_obj;
    vx_user_data_object         dcc_config_obj;
    SensorObj                   sensor_obj;
} TIOVXVissNodePriv;

vx_status tiovx_viss_module_configure_params(NodeObj *node)
{
    vx_status status = VX_FAILURE;
    TIOVXVissNodeCfg *node_cfg = (TIOVXVissNodeCfg *)node->node_cfg;
    TIOVXVissNodePriv *node_priv = (TIOVXVissNodePriv *)node->node_priv;

    node_cfg->viss_params.sensor_dcc_id       = node_priv->sensor_obj.sensorParams.dccId;
    node_cfg->viss_params.use_case            = 0;
    node_cfg->viss_params.fcp[0].ee_mode      = TIVX_VPAC_VISS_EE_MODE_OFF;
    node_cfg->viss_params.fcp[0].chroma_mode  = TIVX_VPAC_VISS_CHROMA_MODE_420;

    if(node_cfg->output_select[0] == TIOVX_VISS_MODULE_OUTPUT_EN)
    {
#if defined(SOC_AM62A) || defined(SOC_J722S)
        if(node_cfg->viss_params.enable_ir_op)
        {
            if(node_cfg->output_cfgs[0].color_format == VX_DF_IMAGE_U8)
            {
                node_cfg->viss_params.fcp[0].mux_output0  = TIVX_VPAC_VISS_MUX0_IR8;
            }
            else if(node_cfg->output_cfgs[0].color_format == TIVX_DF_IMAGE_P12)
            {
                node_cfg->viss_params.fcp[0].mux_output0  = TIVX_VPAC_VISS_MUX0_IR12_P12;
            }
        }
        else
#endif
        {
            node_cfg->viss_params.fcp[0].mux_output0  = 0;
        }
    }
    if(node_cfg->output_select[1] == TIOVX_VISS_MODULE_OUTPUT_EN)
    {
        node_cfg->viss_params.fcp[0].mux_output1  = 0;
    }
    if(node_cfg->output_select[2] == TIOVX_VISS_MODULE_OUTPUT_EN)
    {
        if(node_cfg->output_cfgs[2].color_format == VX_DF_IMAGE_NV12)
        {
            node_cfg->viss_params.fcp[0].mux_output2  = TIVX_VPAC_VISS_MUX2_NV12;
        }
        else if((node_cfg->output_cfgs[2].color_format == VX_DF_IMAGE_YUYV) ||
                (node_cfg->output_cfgs[2].color_format == VX_DF_IMAGE_UYVY))
        {
            node_cfg->viss_params.fcp[0].mux_output2  = TIVX_VPAC_VISS_MUX2_YUV422;
        }
#if defined(SOC_AM62A) || defined(SOC_J722S)
        else if((node_cfg->output_cfgs[2].color_format == VX_DF_IMAGE_U16) &&
                (node_cfg->viss_params.enable_ir_op))
        {
            node_cfg->viss_params.fcp[0].mux_output2 = TIVX_VPAC_VISS_MUX2_IR12_U16;
        }
#endif
    }
    if(node_cfg->output_select[3] == TIOVX_VISS_MODULE_OUTPUT_EN)
    {
        node_cfg->viss_params.fcp[0].mux_output3  = 0;
    }
    if(node_cfg->output_select[4] == TIOVX_VISS_MODULE_OUTPUT_EN)
    {
        node_cfg->viss_params.fcp[0].mux_output4  = 0;
    }

#if defined(SOC_AM62A) || defined(SOC_J722S)
    if(node_cfg->viss_params.bypass_pcid)
        node_cfg->viss_params.enable_ir_op = TIVX_VPAC_VISS_IR_DISABLE;

    if(node_cfg->viss_params.enable_ir_op)
        node_cfg->viss_params.h3a_in              = TIVX_VPAC_VISS_H3A_IN_LSC;
    else if(node_cfg->viss_params.enable_bayer_op && !(node_cfg->viss_params.bypass_pcid))
        node_cfg->viss_params.h3a_in              = TIVX_VPAC_VISS_H3A_IN_PCID;
    else if(node_cfg->viss_params.enable_bayer_op && node_cfg->viss_params.bypass_pcid)
        node_cfg->viss_params.h3a_in              = TIVX_VPAC_VISS_H3A_IN_LSC;
#else
    node_cfg->viss_params.h3a_in              = TIVX_VPAC_VISS_H3A_IN_LSC; 
#endif
    node_cfg->viss_params.h3a_aewb_af_mode    = TIVX_VPAC_VISS_H3A_MODE_AEWB;
    node_cfg->viss_params.bypass_nsf4         = 0;
    node_cfg->viss_params.enable_ctx          = 1;

    if(node_priv->sensor_obj.sensor_wdr_enabled == 1)
    {
        node_cfg->viss_params.bypass_glbce = 0;
    }
    else
    {
        node_cfg->viss_params.bypass_glbce = 1;
    }

    node_priv->viss_params_obj = vxCreateUserDataObject(node->graph->tiovx_context, "tivx_vpac_viss_params_t", sizeof(tivx_vpac_viss_params_t), &node_cfg->viss_params);
    status = vxGetStatus((vx_reference)node_priv->viss_params_obj);

    if(status != VX_SUCCESS)
    {
        TIOVX_MODULE_ERROR("[VISS] Unable to create VISS config object! \n");
    }

    return status;
}

static vx_status tiovx_viss_module_configure_dcc_params(NodeObj *node)
{
    vx_status status = VX_FAILURE;
    TIOVXVissNodeCfg *node_cfg = (TIOVXVissNodeCfg *)node->node_cfg;
    TIOVXVissNodePriv *node_priv = (TIOVXVissNodePriv *)node->node_priv;
    int32_t dcc_buff_size;
    uint8_t * dcc_buf;
    vx_map_id dcc_buf_map_id;
    FILE *fp = fopen(node_cfg->dcc_config_file, "rb");
    int32_t bytes_read;

    if(fp == NULL)
    {
        TIOVX_MODULE_ERROR("[VISS] Unable to open DCC config file %s!\n", node_cfg->dcc_config_file);
        status = VX_FAILURE;
        return status;
    }

    fseek(fp, 0L, SEEK_END);
    dcc_buff_size = (int32_t)ftell(fp);
    fseek(fp, 0L, SEEK_SET);

    node_priv->dcc_config_obj = vxCreateUserDataObject(node->graph->tiovx_context, "dcc_viss", dcc_buff_size, NULL );
    status = vxGetStatus((vx_reference)node_priv->dcc_config_obj);

    if(status != VX_SUCCESS) {
        TIOVX_MODULE_ERROR("[VISS] Unable to create DCC config object \n");
        return status;
    }

    vxMapUserDataObject(
            node_priv->dcc_config_obj, 0,
            dcc_buff_size,
            &dcc_buf_map_id,
            (void **)&dcc_buf,
            VX_WRITE_ONLY,
            VX_MEMORY_TYPE_HOST, 0);

    bytes_read = fread(dcc_buf, sizeof(uint8_t), dcc_buff_size, fp);

    if(bytes_read != dcc_buff_size)
    {
        TIOVX_MODULE_ERROR("[VISS] DCC config bytes read %d not matching bytes expected %d \n", bytes_read, dcc_buff_size);
        status = VX_FAILURE;
    }

    vxUnmapUserDataObject(node_priv->dcc_config_obj, dcc_buf_map_id);

    if(fp != NULL)
    {
        fclose(fp);
    }

    return status;
}

void tiovx_viss_init_cfg(TIOVXVissNodeCfg *node_cfg)
{
    tivx_vpac_viss_params_init(&node_cfg->viss_params);
    #if defined(SOC_AM62A) || defined(SOC_J722S)
        node_cfg->viss_params.bypass_pcid = 1;
        node_cfg->viss_params.enable_ir_op = TIVX_VPAC_VISS_IR_DISABLE;
        node_cfg->viss_params.enable_bayer_op = TIVX_VPAC_VISS_BAYER_ENABLE;
    #endif

    snprintf(node_cfg->dcc_config_file, TIVX_FILEIO_FILE_PATH_LENGTH, "%s", TIOVX_MODULES_DEFAULT_VISS_DCC_FILE);
    node_cfg->width = TIOVX_MODULES_DEFAULT_VISS_WIDTH;
    node_cfg->height = TIOVX_MODULES_DEFAULT_VISS_HEIGHT;
    node_cfg->num_channels = 1;
    sprintf(node_cfg->target_string, TIVX_TARGET_VPAC_VISS1);
    sprintf(node_cfg->sensor_name, TIOVX_MODULES_DEFAULT_VISS_SENSOR);
    node_cfg->enable_h3a_pad = vx_false_e;
    node_cfg->enable_aewb_pad = vx_false_e;

    node_cfg->input_cfg.params.num_exposures = 1;
    node_cfg->input_cfg.params.line_interleaved = vx_false_e;
    node_cfg->input_cfg.params.meta_height_before = 0;
    node_cfg->input_cfg.params.meta_height_after = 0;
    node_cfg->input_cfg.params.format[0].pixel_container = TIVX_RAW_IMAGE_8_BIT;
    node_cfg->input_cfg.params.format[0].msb = 7;

    node_cfg->output_select[0] = TIOVX_VISS_MODULE_OUTPUT_NA;
    node_cfg->output_select[1] = TIOVX_VISS_MODULE_OUTPUT_NA;
    node_cfg->output_select[2] = TIOVX_VISS_MODULE_OUTPUT_EN;
    node_cfg->output_select[3] = TIOVX_VISS_MODULE_OUTPUT_NA;
    node_cfg->output_select[4] = TIOVX_VISS_MODULE_OUTPUT_NA;

    node_cfg->output_cfgs[0].color_format = VX_DF_IMAGE_U8;
    node_cfg->output_cfgs[2].color_format = VX_DF_IMAGE_NV12;
}

vx_status tiovx_viss_init_node(NodeObj *node)
{
    vx_status status = VX_FAILURE;
    TIOVXVissNodeCfg *node_cfg = (TIOVXVissNodeCfg *)node->node_cfg;
    TIOVXVissNodePriv *node_priv = (TIOVXVissNodePriv *)node->node_priv;
    vx_reference exemplar;

    status = tiovx_init_sensor_obj(&node_priv->sensor_obj, node_cfg->sensor_name);
    if (VX_SUCCESS != status) {
        TIOVX_MODULE_ERROR("[VISS] Sensor Init Failed\n");
        return status;
    }

    node_cfg->input_cfg.params.width = node_cfg->width;
    node_cfg->input_cfg.params.height = node_cfg->height
                                - node_cfg->input_cfg.params.meta_height_before
                                - node_cfg->input_cfg.params.meta_height_after;

    node->num_inputs = 1;

    node->sinks[0].node = node;
    node->sinks[0].pad_index = 0;
    node->sinks[0].node_parameter_index = 3;
    node->sinks[0].num_channels = node_cfg->num_channels;
    exemplar = (vx_reference)tivxCreateRawImage(node->graph->tiovx_context,
                                                &node_cfg->input_cfg.params);
    status = tiovx_module_create_pad_exemplar(&node->sinks[0], exemplar);
    if (VX_SUCCESS != status) {
        TIOVX_MODULE_ERROR("[VISS] Create Input Failed\n");
        return status;
    }
    vxReleaseReference(&exemplar);

    #if defined(SOC_AM62A) || defined(SOC_J722S)
        if (node_cfg->viss_params.enable_ir_op == TIVX_VPAC_VISS_IR_ENABLE) {
            node_cfg->output_select[0] = TIOVX_VISS_MODULE_OUTPUT_EN;
        } else {
            node_cfg->output_select[0] = TIOVX_VISS_MODULE_OUTPUT_NA;
        }

        if (node_cfg->viss_params.enable_bayer_op == TIVX_VPAC_VISS_BAYER_ENABLE) {
            node_cfg->output_select[2] = TIOVX_VISS_MODULE_OUTPUT_EN;
        } else {
            node_cfg->output_select[2] = TIOVX_VISS_MODULE_OUTPUT_NA;
        }
    #endif

    node->num_outputs = 0;

    for (int i = 0; i < TIOVX_VISS_MODULE_MAX_OUTPUTS; i++) {
        if (node_cfg->output_select[i] == TIOVX_VISS_MODULE_OUTPUT_EN) {
            node_cfg->output_cfgs[i].width = node_cfg->width;
            node_cfg->output_cfgs[i].height = node_cfg->height
                                - node_cfg->input_cfg.params.meta_height_before
                                - node_cfg->input_cfg.params.meta_height_after;

            node->srcs[node->num_outputs].node = node;
            node->srcs[node->num_outputs].pad_index = node->num_outputs;
            node->srcs[node->num_outputs].node_parameter_index = i + 4;
            node->srcs[node->num_outputs].num_channels = node_cfg->num_channels;
            exemplar = (vx_reference)vxCreateImage(
                                        node->graph->tiovx_context,
                                        node_cfg->output_cfgs[i].width,
                                        node_cfg->output_cfgs[i].height,
                                        node_cfg->output_cfgs[i].color_format);
            status = tiovx_module_create_pad_exemplar(
                                                &node->srcs[node->num_outputs],
                                                exemplar);
            if (VX_SUCCESS != status) {
                TIOVX_MODULE_ERROR(
                        "[VISS] Create Output %d Failed\n", i);
                return status;
            }
            vxReleaseReference(&exemplar);
            node->num_outputs++;
        }
    }

    if (node_cfg->enable_aewb_pad == vx_true_e) {
        node->sinks[1].node = node;
        node->sinks[1].pad_index = 1;
        node->sinks[1].node_parameter_index = 1;
        node->sinks[1].num_channels = node_cfg->num_channels;
        exemplar = (vx_reference)vxCreateUserDataObject(
                                        node->graph->tiovx_context,
                                        "tivx_ae_awb_params_t",
                                        sizeof(tivx_ae_awb_params_t),
                                        NULL);
        status = tiovx_module_create_pad_exemplar(&node->sinks[1], exemplar);
        if (VX_SUCCESS != status) {
            TIOVX_MODULE_ERROR("[VISS] Create AEWB Input Failed\n");
            return status;
        }
        for (int i=0; i < node->sinks[1].num_channels; i++) {
            void *map_ptr;
            vx_map_id map_id;
            vx_user_data_object obj = NULL;

            obj = (vx_user_data_object)vxGetObjectArrayItem(
                                                 node->sinks[1].exemplar_arr, i);
            vxMapUserDataObject(obj, 0, sizeof(tivx_ae_awb_params_t),
                        &map_id, &map_ptr, VX_READ_ONLY, VX_MEMORY_TYPE_HOST, 0);
            vxUnmapUserDataObject(obj, map_id);
            vxReleaseReference((vx_reference *)&obj);
        }
        vxReleaseReference(&exemplar);
        node->num_inputs++;
    }

    if (node_cfg->enable_h3a_pad == vx_true_e) {
        node->srcs[node->num_outputs].node = node;
        node->srcs[node->num_outputs].pad_index = node->num_outputs;
        node->srcs[node->num_outputs].node_parameter_index = 9;
        node->srcs[node->num_outputs].num_channels = node_cfg->num_channels;
        exemplar = (vx_reference)vxCreateUserDataObject(
                                        node->graph->tiovx_context,
                                        "tivx_h3a_data_t",
                                        sizeof(tivx_h3a_data_t),
                                        NULL);
        status = tiovx_module_create_pad_exemplar(&node->srcs[node->num_outputs],
                                                  exemplar);
        if (VX_SUCCESS != status) {
            TIOVX_MODULE_ERROR("[VISS] Create H3A Output Failed\n");
            return status;
        }
        vxReleaseReference(&exemplar);
        node->num_outputs++;
    }

    sprintf(node->name, "viss_node");

    tiovx_viss_module_configure_params(node);
    tiovx_viss_module_configure_dcc_params(node);

    return status;
}

vx_status tiovx_viss_create_node(NodeObj *node)
{
    vx_status status = VX_FAILURE;
    TIOVXVissNodeCfg *node_cfg = (TIOVXVissNodeCfg *)node->node_cfg;
    TIOVXVissNodePriv *node_priv = (TIOVXVissNodePriv *)node->node_priv;
    vx_user_data_object ae_awb_result = NULL;
    vx_image output[] = {NULL, NULL, NULL, NULL, NULL};
    vx_user_data_object h3a_stats = NULL;
    vx_bool replicate[] = {vx_false_e, vx_false_e, vx_false_e,
                           vx_true_e, vx_false_e, vx_false_e,
                           vx_false_e, vx_false_e, vx_false_e,
                           vx_false_e, vx_false_e, vx_false_e,
                           vx_false_e};

    if (node_cfg->enable_aewb_pad == vx_true_e) {
        replicate[1] = vx_true_e;

        ae_awb_result = (vx_user_data_object)node->sinks[1].exemplar;
    }

    if (node_cfg->enable_h3a_pad == vx_true_e) {
        replicate[9] = vx_true_e;

        h3a_stats =
            (vx_user_data_object)node->srcs[node->num_outputs - 1].exemplar;
    }

    for (int i = 0, j = 0; i < TIOVX_VISS_MODULE_MAX_OUTPUTS; i++) {
        if (node_cfg->output_select[i] == TIOVX_VISS_MODULE_OUTPUT_EN) {
            output[i] = (vx_image)node->srcs[j++].exemplar;
            replicate[i+4] = vx_true_e;
        }
    }

    node->tiovx_node = tivxVpacVissNode(node->graph->tiovx_graph,
                                node_priv->viss_params_obj,
                                ae_awb_result,
                                node_priv->dcc_config_obj,
                                (tivx_raw_image)(node->sinks[0].exemplar),
                                output[0], output[1], output[2],
                                output[3], output[4],
                                h3a_stats, NULL, NULL, NULL);

    status = vxGetStatus((vx_reference)node->tiovx_node);
    if (VX_SUCCESS != status) {
        TIOVX_MODULE_ERROR("[VISS] Create Node Failed\n");
        return status;
    }

    vxSetNodeTarget(node->tiovx_node,
                    VX_TARGET_STRING, node_cfg->target_string);
    vxReplicateNode(node->graph->tiovx_graph,
                    node->tiovx_node, replicate, 13);

    return status;
}

vx_status tiovx_viss_delete_node(NodeObj *node)
{
    vx_status status = VX_FAILURE;
    TIOVXVissNodePriv *node_priv = (TIOVXVissNodePriv *)node->node_priv;

    status = vxReleaseNode(&node->tiovx_node);

    status = vxReleaseUserDataObject(&node_priv->viss_params_obj);
    status = vxReleaseUserDataObject(&node_priv->dcc_config_obj);

    return status;
}

vx_uint32 tiovx_viss_get_cfg_size()
{
    return sizeof(TIOVXVissNodeCfg);
}

vx_uint32 tiovx_viss_get_priv_size()
{
    return sizeof(TIOVXVissNodePriv);
}
