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
#include "tiovx_ldc_module.h"
#include "tiovx_utils.h"

#define TIOVX_LDC_MODULE_OUTPUT_NA (0)
#define TIOVX_LDC_MODULE_OUTPUT_EN (1)
#define TIOVX_MODULES_DEFAULT_LDC_DCC_FILE "/opt/imaging/imx219/linear/dcc_ldc.bin"
#define TIOVX_MODULES_DEFAULT_LDC_SENSOR "SENSOR_SONY_IMX390_UB953_D3"
#define TIOVX_MODULES_DEFAULT_LDC_INPUT_HEIGHT 1936
#define TIOVX_MODULES_DEFAULT_LDC_INPUT_WIDTH 1096
#define TIOVX_MODULES_DEFAULT_LDC_OUTPUT_HEIGHT 1920
#define TIOVX_MODULES_DEFAULT_LDC_OUTPUT_WIDTH 1080
#define TIOVX_MODULES_DEFAULT_LDC_TABLE_WIDTH     (1920)
#define TIOVX_MODULES_DEFAULT_LDC_TABLE_HEIGHT    (1080)
#define TIOVX_MODULES_DEFAULT_LDC_DS_FACTOR       (2)
#define TIOVX_MODULES_DEFAULT_LDC_BLOCK_WIDTH     (64)
#define TIOVX_MODULES_DEFAULT_LDC_BLOCK_HEIGHT    (32)
#define TIOVX_MODULES_DEFAULT_LDC_PIXEL_PAD       (1)

typedef struct {
    vx_user_data_object           ldc_params_obj;
    vx_user_data_object           mesh_params_obj;
    vx_user_data_object           region_params_obj;
    vx_user_data_object           dcc_config_obj;
    vx_image                      mesh_img;
    vx_matrix                     warp_matrix;
    SensorObj                     sensor_obj;
} TIOVXLdcNodePriv;

static vx_status tiovx_ldc_module_configure_dcc_params(NodeObj *node)
{
    vx_status status = VX_FAILURE;
    TIOVXLdcNodeCfg *node_cfg = (TIOVXLdcNodeCfg *)node->node_cfg;
    TIOVXLdcNodePriv *node_priv = (TIOVXLdcNodePriv *)node->node_priv;
    int32_t dcc_buff_size;
    uint8_t * dcc_buf;
    vx_map_id dcc_buf_map_id;
    FILE *fp = fopen(node_cfg->dcc_config_file, "rb");
    int32_t bytes_read;

    if(fp == NULL)
    {
        TIOVX_MODULE_ERROR("[LDC] Unable to open DCC config file %s!\n", node_cfg->dcc_config_file);
        status = VX_FAILURE;
        return status;
    }

    fseek(fp, 0L, SEEK_END);
    dcc_buff_size = (int32_t)ftell(fp);
    fseek(fp, 0L, SEEK_SET);

    node_priv->dcc_config_obj = vxCreateUserDataObject(node->graph->tiovx_context, "dcc_ldc", dcc_buff_size, NULL );
    status = vxGetStatus((vx_reference)node_priv->dcc_config_obj);

    if(status != VX_SUCCESS) {
        TIOVX_MODULE_ERROR("[LDC] Unable to create DCC config object \n");
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
        TIOVX_MODULE_ERROR("[LDC] DCC config bytes read %d not matching bytes expected %d \n", bytes_read, dcc_buff_size);
        status = VX_FAILURE;
    }

    vxUnmapUserDataObject(node_priv->dcc_config_obj, dcc_buf_map_id);

    if(fp != NULL)
    {
        fclose(fp);
    }

    return status;
}

static vx_status tiovx_ldc_module_configure_mesh_params(NodeObj *node)
{
    vx_status status = VX_FAILURE;
    TIOVXLdcNodeCfg *node_cfg = (TIOVXLdcNodeCfg *)node->node_cfg;
    TIOVXLdcNodePriv *node_priv = (TIOVXLdcNodePriv *)node->node_priv;

    vx_uint32 table_width_ds, table_height_ds;

    table_width_ds = (((node_cfg->table_width / (1 << node_cfg->ds_factor)) + 1u) + 15u) & (~15u);
    table_height_ds = ((node_cfg->table_height / (1 << node_cfg->ds_factor)) + 1u);

    if (node_cfg->lut_file[0] != '\0')
    {
        /* Mesh Image */
        node_priv->mesh_img = vxCreateImage(node->graph->tiovx_context,
                                           table_width_ds, table_height_ds,
                                           VX_DF_IMAGE_U32);
        status = vxGetStatus((vx_reference)node_priv->mesh_img);

        if((vx_status)VX_SUCCESS == status)
        {
            /* Read LUT file */
            status = readImage(node_cfg->lut_file, node_priv->mesh_img);
        } else {
            TIOVX_MODULE_ERROR("[LDC] Unable to create mesh image! \n");
        }
    }

    if (status == VX_SUCCESS)
    {
        /* Mesh Parameters */
        memset(&node_cfg->mesh_params, 0, sizeof(tivx_vpac_ldc_mesh_params_t));

        tivx_vpac_ldc_mesh_params_init(&node_cfg->mesh_params);

        node_cfg->mesh_params.mesh_frame_width  = node_cfg->table_width;
        node_cfg->mesh_params.mesh_frame_height = node_cfg->table_height;
        node_cfg->mesh_params.subsample_factor  = node_cfg->ds_factor;

        node_priv->mesh_params_obj = vxCreateUserDataObject(
                                        node->graph->tiovx_context,
                                        "tivx_vpac_ldc_mesh_params_t",
                                        sizeof(tivx_vpac_ldc_mesh_params_t),
                                        NULL);
        status = vxGetStatus((vx_reference)node_priv->mesh_params_obj);

        if((vx_status)VX_SUCCESS == status)
        {
            status = vxCopyUserDataObject(node_priv->mesh_params_obj, 0,
                                sizeof(tivx_vpac_ldc_mesh_params_t),
                                &node_cfg->mesh_params,
                                VX_WRITE_ONLY,
                                VX_MEMORY_TYPE_HOST);
            if(status != VX_SUCCESS)
            {
                TIOVX_MODULE_ERROR("[LDC] Unable to copy mesh params into buffer! \n");
            }
        }
        else
        {
            TIOVX_MODULE_ERROR("[LDC] Unable to create mesh config object! \n");
        }
    }
    else
    {
        TIOVX_MODULE_ERROR("[LDC] Unable to copy mesh image! \n");
    }

    return status;
}

static vx_status tiovx_ldc_module_configure_region_params(NodeObj *node)
{
    vx_status status = VX_FAILURE;
    TIOVXLdcNodeCfg *node_cfg = (TIOVXLdcNodeCfg *)node->node_cfg;
    TIOVXLdcNodePriv *node_priv = (TIOVXLdcNodePriv *)node->node_priv;

    /* Block Size parameters */
    node_cfg->region_params.out_block_width  = node_cfg->out_block_width;
    node_cfg->region_params.out_block_height = node_cfg->out_block_height;
    node_cfg->region_params.pixel_pad        = node_cfg->pixel_pad;

    node_priv->region_params_obj = vxCreateUserDataObject(
                                        node->graph->tiovx_context,
                                        "tivx_vpac_ldc_region_params_t",
                                        sizeof(tivx_vpac_ldc_region_params_t),
                                        NULL);
    status = vxGetStatus((vx_reference)node_priv->region_params_obj);

    if((vx_status)VX_SUCCESS == status)
    {
        status = vxCopyUserDataObject(node_priv->region_params_obj, 0,
                            sizeof(tivx_vpac_ldc_region_params_t),
                            &node_cfg->region_params,
                            VX_WRITE_ONLY,
                            VX_MEMORY_TYPE_HOST);
        if(status != VX_SUCCESS)
        {
            TIOVX_MODULE_ERROR("[LDC] Unable to copy ldc region params to buffer! \n");
        }
    }
    else
    {
        TIOVX_MODULE_ERROR("[LDC] Unable to create region config object! \n");
    }

    return status;
}

static vx_status tiovx_ldc_module_configure_ldc_params(NodeObj *node)
{
    vx_status status = VX_FAILURE;
    TIOVXLdcNodeCfg *node_cfg = (TIOVXLdcNodeCfg *)node->node_cfg;
    TIOVXLdcNodePriv *node_priv = (TIOVXLdcNodePriv *)node->node_priv;

    /* LDC Configuration */
    tivx_vpac_ldc_params_init(&node_cfg->ldc_params);
    node_cfg->ldc_params.init_x = node_cfg->init_x;
    node_cfg->ldc_params.init_y = node_cfg->init_y;
    node_cfg->ldc_params.luma_interpolation_type = 1;
    node_cfg->ldc_params.dcc_camera_id = node_priv->sensor_obj.sensorParams.dccId;

    node_priv->ldc_params_obj = vxCreateUserDataObject(node->graph->tiovx_context,
                                            "tivx_vpac_ldc_params_t",
                                            sizeof(tivx_vpac_ldc_params_t),
                                            NULL);
    status = vxGetStatus((vx_reference)node_priv->ldc_params_obj);

    if((vx_status)VX_SUCCESS == status)
    {
        status = vxCopyUserDataObject(node_priv->ldc_params_obj, 0,
                            sizeof(tivx_vpac_ldc_params_t),
                            &node_cfg->ldc_params,
                            VX_WRITE_ONLY,
                            VX_MEMORY_TYPE_HOST);
        if(status != VX_SUCCESS)
        {
            TIOVX_MODULE_ERROR("[LDC] Unable to copy ldc config params into buffer! \n");
        }
    }
    else
    {
        TIOVX_MODULE_ERROR("[LDC] Unable to create ldc params object! \n");
    }

    return status;
}

void tiovx_ldc_init_cfg(TIOVXLdcNodeCfg *node_cfg)
{
    snprintf(node_cfg->dcc_config_file, TIVX_FILEIO_FILE_PATH_LENGTH, "%s", TIOVX_MODULES_DEFAULT_LDC_DCC_FILE);
    node_cfg->ldc_mode = TIOVX_MODULE_LDC_OP_MODE_DCC_DATA;
    node_cfg->output_select[0] = TIOVX_LDC_MODULE_OUTPUT_EN;
    node_cfg->output_select[1] = TIOVX_LDC_MODULE_OUTPUT_NA;

    node_cfg->input_cfg.color_format = VX_DF_IMAGE_NV12;
    node_cfg->input_cfg.width = TIOVX_MODULES_DEFAULT_LDC_INPUT_WIDTH;
    node_cfg->input_cfg.height = TIOVX_MODULES_DEFAULT_LDC_INPUT_HEIGHT;

    node_cfg->output_cfgs[0].color_format = VX_DF_IMAGE_NV12;
    node_cfg->output_cfgs[0].width = TIOVX_MODULES_DEFAULT_LDC_OUTPUT_WIDTH;
    node_cfg->output_cfgs[0].height = TIOVX_MODULES_DEFAULT_LDC_OUTPUT_HEIGHT;

    node_cfg->init_x = 0;
    node_cfg->init_y = 0;
    node_cfg->table_width = TIOVX_MODULES_DEFAULT_LDC_TABLE_WIDTH;
    node_cfg->table_height = TIOVX_MODULES_DEFAULT_LDC_TABLE_HEIGHT;
    node_cfg->ds_factor = TIOVX_MODULES_DEFAULT_LDC_DS_FACTOR;
    node_cfg->out_block_width = TIOVX_MODULES_DEFAULT_LDC_BLOCK_WIDTH;
    node_cfg->out_block_height = TIOVX_MODULES_DEFAULT_LDC_BLOCK_HEIGHT;
    node_cfg->pixel_pad = TIOVX_MODULES_DEFAULT_LDC_PIXEL_PAD;

    node_cfg->num_channels = 1;
    sprintf(node_cfg->target_string, TIVX_TARGET_VPAC_LDC1);
    sprintf(node_cfg->sensor_name, TIOVX_MODULES_DEFAULT_LDC_SENSOR);
}

vx_status tiovx_ldc_init_node(NodeObj *node)
{
    vx_status status = VX_FAILURE;
    TIOVXLdcNodeCfg *node_cfg = (TIOVXLdcNodeCfg *)node->node_cfg;
    TIOVXLdcNodePriv *node_priv = (TIOVXLdcNodePriv *)node->node_priv;
    vx_reference exemplar;

    CLR(node_priv);

    status = tiovx_init_sensor(&node_priv->sensor_obj, node_cfg->sensor_name);
    if (VX_SUCCESS != status) {
        TIOVX_MODULE_ERROR("[LDC] Sensor Init Failed\n");
        return status;
    }

    node->num_inputs = 1;

    node->sinks[0].node = node;
    node->sinks[0].pad_index = 0;
    node->sinks[0].node_parameter_index = 6;
    node->sinks[0].num_channels = node_cfg->num_channels;
    exemplar = (vx_reference)vxCreateImage(node->graph->tiovx_context,
                                           node_cfg->input_cfg.width,
                                           node_cfg->input_cfg.height,
                                           node_cfg->input_cfg.color_format);
    status = tiovx_module_create_pad_exemplar(&node->sinks[0], exemplar);
    if (VX_SUCCESS != status) {
        TIOVX_MODULE_ERROR("[LDC] Create Input Failed\n");
        return status;
    }
    vxReleaseReference(&exemplar);

    node->num_outputs = 0;

    for (int i = 0; i < TIOVX_LDC_MODULE_MAX_OUTPUTS; i++) {
        if (node_cfg->output_select[i] == TIOVX_LDC_MODULE_OUTPUT_EN) {
            node->srcs[node->num_outputs].node = node;
            node->srcs[node->num_outputs].pad_index = node->num_outputs;
            node->srcs[node->num_outputs].node_parameter_index = i + 7;
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
                        "[LDC] Create Output %d Failed\n", i);
                return status;
            }
            vxReleaseReference(&exemplar);
            node->num_outputs++;
        }
    }

    if (node_cfg->ldc_mode == TIOVX_MODULE_LDC_OP_MODE_DCC_DATA) {
        tiovx_ldc_module_configure_dcc_params(node);
    } else if (node_cfg->ldc_mode == TIOVX_MODULE_LDC_OP_MODE_MESH_IMAGE) {
        tiovx_ldc_module_configure_mesh_params(node);
        tiovx_ldc_module_configure_region_params(node);
    }

    tiovx_ldc_module_configure_ldc_params(node);

    return status;
}

vx_status tiovx_ldc_create_node(NodeObj *node)
{
    vx_status status = VX_FAILURE;
    TIOVXLdcNodeCfg *node_cfg = (TIOVXLdcNodeCfg *)node->node_cfg;
    TIOVXLdcNodePriv *node_priv = (TIOVXLdcNodePriv *)node->node_priv;
    vx_image output[] = {NULL, NULL};
    vx_bool replicate[] = { vx_false_e, vx_false_e, vx_false_e,
                            vx_false_e, vx_false_e, vx_false_e,
                            vx_true_e, vx_false_e, vx_false_e};

    for (int i = 0, j = 0; i < TIOVX_LDC_MODULE_MAX_OUTPUTS; i++) {
        if (node_cfg->output_select[i] == TIOVX_LDC_MODULE_OUTPUT_EN) {
            output[i] = (vx_image)node->srcs[j++].exemplar;
            replicate[i+7] = vx_true_e;
        }
    }

    node->tiovx_node = tivxVpacLdcNode(node->graph->tiovx_graph,
                                node_priv->ldc_params_obj,
                                node_priv->warp_matrix,
                                node_priv->region_params_obj,
                                node_priv->mesh_params_obj,
                                node_priv->mesh_img,
                                node_priv->dcc_config_obj,
                                (vx_image)(node->sinks[0].exemplar),
                                output[0], output[1]);

    status = vxGetStatus((vx_reference)node->tiovx_node);
    if (VX_SUCCESS != status) {
        TIOVX_MODULE_ERROR("[LDC] Create Node Failed\n");
        return status;
    }

    vxSetNodeTarget(node->tiovx_node,
                    VX_TARGET_STRING, node_cfg->target_string);
    vxReplicateNode(node->graph->tiovx_graph,
                    node->tiovx_node, replicate, 9);

    return status;
}

vx_status tiovx_ldc_delete_node(NodeObj *node)
{
    vx_status status = VX_FAILURE;
    TIOVXLdcNodeCfg *node_cfg = (TIOVXLdcNodeCfg *)node->node_cfg;
    TIOVXLdcNodePriv *node_priv = (TIOVXLdcNodePriv *)node->node_priv;

    status = vxReleaseNode(&node->tiovx_node);

    status = vxReleaseUserDataObject(&node_priv->ldc_params_obj);
    if (node_cfg->ldc_mode == TIOVX_MODULE_LDC_OP_MODE_DCC_DATA) {
        status = vxReleaseUserDataObject(&node_priv->dcc_config_obj);
    } else if (node_cfg->ldc_mode == TIOVX_MODULE_LDC_OP_MODE_MESH_IMAGE) {
        status = vxReleaseUserDataObject(&node_priv->region_params_obj);
        status = vxReleaseUserDataObject(&node_priv->mesh_params_obj);
        status = vxReleaseImage(&node_priv->mesh_img);
    }

    return status;
}

vx_uint32 tiovx_ldc_get_cfg_size()
{
    return sizeof(TIOVXLdcNodeCfg);
}

vx_uint32 tiovx_ldc_get_priv_size()
{
    return sizeof(TIOVXLdcNodePriv);
}
