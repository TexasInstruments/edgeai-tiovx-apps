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
#include "tiovx_multi_scaler_module.h"

#include <TI/hwa_vpac_msc.h>

typedef struct {
    vx_user_data_object         coeff_obj;
    vx_user_data_object         crop_obj[TIOVX_MULTI_SCALER_MODULE_MAX_OUTPUTS];
} TIOVXMultiScalerNodePriv;

vx_status tiovx_multi_scaler_module_crop_params_init(TIOVXMultiScalerNodeCfg *cfg)
{
    for (int i = 0; i < cfg->num_outputs; i++)
    {
        cfg->crop_params[i].crop_start_x = 0;
        cfg->crop_params[i].crop_start_y = 0;
        cfg->crop_params[i].crop_width = cfg->input_cfg.width;
        cfg->crop_params[i].crop_height = cfg->input_cfg.height;
    }

    return VX_SUCCESS;
}

void tiovx_multi_scaler_module_set_coeff(tivx_vpac_msc_coefficients_t *coeff, uint32_t interpolation)
{
    uint32_t i;
    uint32_t idx;
    uint32_t weight;

    idx = 0;
    coeff->single_phase[0][idx ++] = 0;
    coeff->single_phase[0][idx ++] = 0;
    coeff->single_phase[0][idx ++] = 256;
    coeff->single_phase[0][idx ++] = 0;
    coeff->single_phase[0][idx ++] = 0;
    idx = 0;
    coeff->single_phase[1][idx ++] = 0;
    coeff->single_phase[1][idx ++] = 0;
    coeff->single_phase[1][idx ++] = 256;
    coeff->single_phase[1][idx ++] = 0;
    coeff->single_phase[1][idx ++] = 0;

    if (VX_INTERPOLATION_BILINEAR == interpolation)
    {
        idx = 0;
        for(i=0; i<32; i++)
        {
            weight = i<<2;
            coeff->multi_phase[0][idx ++] = 0;
            coeff->multi_phase[0][idx ++] = 0;
            coeff->multi_phase[0][idx ++] = 256-weight;
            coeff->multi_phase[0][idx ++] = weight;
            coeff->multi_phase[0][idx ++] = 0;
        }
        idx = 0;
        for(i=0; i<32; i++)
        {
            weight = (i+32)<<2;
            coeff->multi_phase[1][idx ++] = 0;
            coeff->multi_phase[1][idx ++] = 0;
            coeff->multi_phase[1][idx ++] = 256-weight;
            coeff->multi_phase[1][idx ++] = weight;
            coeff->multi_phase[1][idx ++] = 0;
        }
        idx = 0;
        for(i=0; i<32; i++)
        {
            weight = i<<2;
            coeff->multi_phase[2][idx ++] = 0;
            coeff->multi_phase[2][idx ++] = 0;
            coeff->multi_phase[2][idx ++] = 256-weight;
            coeff->multi_phase[2][idx ++] = weight;
            coeff->multi_phase[2][idx ++] = 0;
        }
        idx = 0;
        for(i=0; i<32; i++)
        {
            weight = (i+32)<<2;
            coeff->multi_phase[3][idx ++] = 0;
            coeff->multi_phase[3][idx ++] = 0;
            coeff->multi_phase[3][idx ++] = 256-weight;
            coeff->multi_phase[3][idx ++] = weight;
            coeff->multi_phase[3][idx ++] = 0;
        }
    }
    else /* STR_VX_INTERPOLATION_NEAREST_NEIGHBOR */
    {
        idx = 0;
        for(i=0; i<32; i++)
        {
            coeff->multi_phase[0][idx ++] = 0;
            coeff->multi_phase[0][idx ++] = 0;
            coeff->multi_phase[0][idx ++] = 256;
            coeff->multi_phase[0][idx ++] = 0;
            coeff->multi_phase[0][idx ++] = 0;
        }
        idx = 0;
        for(i=0; i<32; i++)
        {
            coeff->multi_phase[1][idx ++] = 0;
            coeff->multi_phase[1][idx ++] = 0;
            coeff->multi_phase[1][idx ++] = 0;
            coeff->multi_phase[1][idx ++] = 256;
            coeff->multi_phase[1][idx ++] = 0;
        }
        idx = 0;
        for(i=0; i<32; i++)
        {
            coeff->multi_phase[2][idx ++] = 0;
            coeff->multi_phase[2][idx ++] = 0;
            coeff->multi_phase[2][idx ++] = 256;
            coeff->multi_phase[2][idx ++] = 0;
            coeff->multi_phase[2][idx ++] = 0;
        }
        idx = 0;
        for(i=0; i<32; i++)
        {
            coeff->multi_phase[3][idx ++] = 0;
            coeff->multi_phase[3][idx ++] = 0;
            coeff->multi_phase[3][idx ++] = 0;
            coeff->multi_phase[3][idx ++] = 256;
            coeff->multi_phase[3][idx ++] = 0;
        }
    }
}

vx_status tiovx_multi_scaler_module_configure_scaler_coeffs(NodeObj *node)
{
    vx_status status = VX_FAILURE;
    TIOVXMultiScalerNodeCfg *node_cfg = (TIOVXMultiScalerNodeCfg *)node->node_cfg;
    TIOVXMultiScalerNodePriv *node_priv = (TIOVXMultiScalerNodePriv *)node->node_priv;
    tivx_vpac_msc_coefficients_t coeffs;

    tiovx_multi_scaler_module_set_coeff(&coeffs,
                                        node_cfg->interpolation_method);

    /* Set Coefficients */
    node_priv->coeff_obj = vxCreateUserDataObject(node->graph->tiovx_context,
                                "tivx_vpac_msc_coefficients_t",
                                sizeof(tivx_vpac_msc_coefficients_t),
                                NULL);
    status = vxGetStatus((vx_reference)node_priv->coeff_obj);

    if((vx_status)VX_SUCCESS == status)
    {
        vxSetReferenceName((vx_reference)node_priv->coeff_obj, "multi_scaler_node_coeff_obj");
        status = vxCopyUserDataObject(node_priv->coeff_obj, 0,
                                    sizeof(tivx_vpac_msc_coefficients_t),
                                    &coeffs,
                                    VX_WRITE_ONLY,
                                    VX_MEMORY_TYPE_HOST);
    }
    else
    {
        TIOVX_MODULE_ERROR(
                "[MULTI_SCALER] Unable to create scaler coeffs "
                "object! \n");
    }

    return status;
}

vx_status tiovx_multi_scaler_configure_crop_params(NodeObj *node)
{
    vx_status status = VX_FAILURE;
    TIOVXMultiScalerNodeCfg *node_cfg = (TIOVXMultiScalerNodeCfg *)node->node_cfg;
    TIOVXMultiScalerNodePriv *node_priv = (TIOVXMultiScalerNodePriv *)node->node_priv;

    for (int i = 0; i < node->num_outputs; i++)
    {
        node_priv->crop_obj[i] = vxCreateUserDataObject(
                                        node->graph->tiovx_context,
                                        "tivx_vpac_msc_crop_params_t",
                                        sizeof(tivx_vpac_msc_crop_params_t),
                                        NULL);
        status = vxGetStatus((vx_reference)node_priv->crop_obj[i]);

        if((vx_status)VX_SUCCESS == status)
        {
            status = vxCopyUserDataObject(node_priv->crop_obj[i], 0,
                                          sizeof(tivx_vpac_msc_crop_params_t),
                                          node_cfg->crop_params + i,
                                          VX_WRITE_ONLY,
                                          VX_MEMORY_TYPE_HOST);
        }

        if((vx_status)VX_SUCCESS != status)
        {
            TIOVX_MODULE_ERROR(
                    "[MULTI_SCALER] Creating user data objectfor crop "
                    "params failed!, %d\n", i);
            break;
        }
    }

    return status;
}

void tiovx_multi_scaler_init_cfg(TIOVXMultiScalerNodeCfg *node_cfg)
{
    node_cfg->color_format = TIOVX_MODULES_DEFAULT_COLOR_FORMAT;
    node_cfg->num_outputs = 1;
    node_cfg->num_channels = 1;
    node_cfg->input_cfg.width = TIOVX_MODULES_DEFAULT_IMAGE_WIDTH;
    node_cfg->input_cfg.height = TIOVX_MODULES_DEFAULT_IMAGE_HEIGHT;
    node_cfg->output_cfgs[0].width = TIOVX_MODULES_DEFAULT_IMAGE_WIDTH;
    node_cfg->output_cfgs[0].height = TIOVX_MODULES_DEFAULT_IMAGE_HEIGHT;
    sprintf(node_cfg->target_string, TIVX_TARGET_VPAC_MSC1);
}

vx_status tiovx_multi_scaler_init_node(NodeObj *node)
{
    vx_status status = VX_FAILURE;
    TIOVXMultiScalerNodeCfg *node_cfg = (TIOVXMultiScalerNodeCfg *)node->node_cfg;
    vx_reference exemplar;

    node_cfg->input_cfg.color_format = node_cfg->color_format;
    for (int i = 0; i < node_cfg->num_outputs; i++)
        node_cfg->output_cfgs[i].color_format = node_cfg->color_format;

    node->num_inputs = 1;
    node->num_outputs = node_cfg->num_outputs;

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
        TIOVX_MODULE_ERROR("[MULTI_SCALER] Create Input Failed\n");
        return status;
    }
    vxReleaseReference(&exemplar);

    for (int i = 0; i < node_cfg->num_outputs; i++) {
        node->srcs[i].node = node;
        node->srcs[i].pad_index = i;
        node->srcs[i].node_parameter_index = i + 1;
        node->srcs[i].num_channels = node_cfg->num_channels;
        exemplar = (vx_reference)vxCreateImage(node->graph->tiovx_context,
                                               node_cfg->output_cfgs[i].width,
                                               node_cfg->output_cfgs[i].height,
                                               node_cfg->output_cfgs[i].color_format);
        status = tiovx_module_create_pad_exemplar(&node->srcs[i], exemplar);
        if (VX_SUCCESS != status) {
            TIOVX_MODULE_ERROR(
                    "[MULTI_SCALER] Create Output %d Failed\n", i);
            return status;
        }
        vxReleaseReference(&exemplar);
    }

    sprintf(node->name, "msc_node");

    status = tiovx_multi_scaler_configure_crop_params(node);
    if (VX_SUCCESS != status) {
        TIOVX_MODULE_ERROR("[MULTI_SCALER] Failed to set crop params\n");
        return status;
    }

    status = tiovx_multi_scaler_module_configure_scaler_coeffs(node);
    if (VX_SUCCESS != status) {
        TIOVX_MODULE_ERROR("[MULTI_SCALER] Failed to set scaler coeffs\n");
    }

    return status;
}

vx_status tiovx_multi_scaler_module_update_filter_coeffs(NodeObj *node)
{
    vx_status status = VX_FAILURE;
    vx_reference refs[1];
    TIOVXMultiScalerNodePriv *node_priv = (TIOVXMultiScalerNodePriv *)node->node_priv;

    refs[0] = (vx_reference)node_priv->coeff_obj;

    status = tivxNodeSendCommand(node->tiovx_node, 0u,
                                 TIVX_VPAC_MSC_CMD_SET_COEFF,
                                 refs, 1u);
    if((vx_status)VX_SUCCESS != status)
    {
        TIOVX_MODULE_ERROR("[MULTI_SCALER] Node send command failed!\n");
    }

    return status;
}

vx_status tiovx_multi_scaler_module_update_crop_params(NodeObj *node)
{
    vx_status status = VX_FAILURE;
    vx_reference refs[TIOVX_MULTI_SCALER_MODULE_MAX_OUTPUTS];
    TIOVXMultiScalerNodeCfg *node_cfg = (TIOVXMultiScalerNodeCfg *)node->node_cfg;
    TIOVXMultiScalerNodePriv *node_priv = (TIOVXMultiScalerNodePriv *)node->node_priv;

    for (int i = 0; i < node_cfg->num_outputs; i++)
    {
        refs[i] = (vx_reference)node_priv->crop_obj[i];
    }

    status = tivxNodeSendCommand(node->tiovx_node, 0u,
                                 TIVX_VPAC_MSC_CMD_SET_CROP_PARAMS,
                                 refs, node->num_outputs);

    if((vx_status)VX_SUCCESS != status)
    {
        TIOVX_MODULE_ERROR(
                "[MULTI-SCALER-MODULE] Node send command "
                "TIVX_VPAC_MSC_CMD_SET_CROP_PARAMS, failed!\n");
    }

    return status;
}

vx_status tiovx_multi_scaler_create_node(NodeObj *node)
{
    vx_status status = VX_FAILURE;
    TIOVXMultiScalerNodeCfg *node_cfg = (TIOVXMultiScalerNodeCfg *)node->node_cfg;
    vx_image output[] = {NULL, NULL, NULL, NULL, NULL};
    vx_bool replicate[] = {vx_true_e, vx_false_e, vx_false_e,
                           vx_false_e, vx_false_e, vx_false_e};

    for (int i = 0; i < node->num_outputs; i++) {
        output[i] = (vx_image)node->srcs[i].exemplar;
        replicate[i+1] = vx_true_e;
    }

    node->tiovx_node = tivxVpacMscScaleNode(node->graph->tiovx_graph,
                                (vx_image)(node->sinks[0].exemplar),
                                output[0], output[1], output[2],
                                output[3], output[4]);

    status = vxGetStatus((vx_reference)node->tiovx_node);
    if (VX_SUCCESS != status) {
        TIOVX_MODULE_ERROR("[MULTI_SCALER] Create Node Failed\n");
        return status;
    }

    vxSetNodeTarget(node->tiovx_node,
                    VX_TARGET_STRING, node_cfg->target_string);
    vxReplicateNode(node->graph->tiovx_graph,
                    node->tiovx_node, replicate, 6);

    return status;
}

vx_status tiovx_multi_scaler_delete_node(NodeObj *node)
{
    vx_status status = VX_FAILURE;
    TIOVXMultiScalerNodePriv *node_priv = (TIOVXMultiScalerNodePriv *)node->node_priv;

    status = vxReleaseNode(&node->tiovx_node);

    status = vxReleaseUserDataObject(&node_priv->coeff_obj);

    for (int i = 0; i < node->num_outputs; i++) {
        status = vxReleaseUserDataObject(&node_priv->crop_obj[i]);
    }

    return status;
}

vx_status tiovx_multi_scaler_post_verify_graph(NodeObj *node)
{
    vx_status status = VX_FAILURE;

    status = tiovx_multi_scaler_module_update_filter_coeffs(node);
    status = tiovx_multi_scaler_module_update_crop_params(node);

    return status;
}

vx_uint32 tiovx_multi_scaler_get_cfg_size()
{
    return sizeof(TIOVXMultiScalerNodeCfg);
}

vx_uint32 tiovx_multi_scaler_get_priv_size()
{
    return sizeof(TIOVXMultiScalerNodePriv);
}
