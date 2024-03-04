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
#include "tiovx_tee_module.h"

void tiovx_tee_init_cfg(TIOVXTeeNodeCfg *node_cfg)
{
    node_cfg->peer_pad = NULL;
    node_cfg->num_outputs = 1;
}

vx_status tiovx_tee_init_node(NodeObj *node)
{
    vx_status status = VX_FAILURE;
    TIOVXTeeNodeCfg *node_cfg = (TIOVXTeeNodeCfg *)node->node_cfg;

    node->num_inputs = 1;
    node->num_outputs = node_cfg->num_outputs;

    node->sinks[0].node = node;
    node->sinks[0].pad_index = 0;
    node->sinks[0].node_parameter_index =
                          node_cfg->peer_pad->node_parameter_index;
    node->sinks[0].num_channels = node_cfg->peer_pad->num_channels;
    vxRetainReference(node_cfg->peer_pad->exemplar);
    vxRetainReference((vx_reference)node_cfg->peer_pad->exemplar_arr);
    node->sinks[0].exemplar = node_cfg->peer_pad->exemplar;
    node->sinks[0].exemplar_arr = node_cfg->peer_pad->exemplar_arr;

    for (int i = 0; i < node_cfg->num_outputs; i++) {
        node->srcs[i].node = node;
        node->srcs[i].pad_index = i;
        node->srcs[i].node_parameter_index =
                             node_cfg->peer_pad->node_parameter_index;
        node->srcs[i].num_channels = node_cfg->peer_pad->num_channels;
        vxRetainReference(node_cfg->peer_pad->exemplar);
        vxRetainReference((vx_reference)node_cfg->peer_pad->exemplar_arr);
        node->srcs[i].exemplar = node_cfg->peer_pad->exemplar;
        node->srcs[i].exemplar_arr = node_cfg->peer_pad->exemplar_arr;
    }

    if (SRC == node_cfg->peer_pad->direction) {
        status = tiovx_modules_link_pads(node_cfg->peer_pad, &node->sinks[0]);
        if (VX_SUCCESS != status) {
            TIOVX_MODULE_ERROR("[TEE] Failed to link sink pad\n");
            return status;
        }
    } else {
        status = tiovx_modules_link_pads(&node->srcs[0], node_cfg->peer_pad);
        if (VX_SUCCESS != status) {
            TIOVX_MODULE_ERROR("[TEE] Failed to link src pad\n");
            return status;
        }
    }

    sprintf(node->name, "tee_node");

    return status;
}

vx_status tiovx_tee_create_node(NodeObj *node)
{
    vx_status status = VX_SUCCESS;
    TIOVXTeeNodeCfg *node_cfg = (TIOVXTeeNodeCfg *)node->node_cfg;

    node->tiovx_node = node_cfg->peer_pad->node->tiovx_node;

    return status;
}

vx_status tiovx_tee_delete_node(NodeObj *node)
{
    vx_status status = VX_SUCCESS;

    return status;
}

vx_uint32 tiovx_tee_get_cfg_size()
{
    return sizeof(TIOVXTeeNodeCfg);
}
