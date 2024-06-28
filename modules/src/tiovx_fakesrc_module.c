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
#include "tiovx_fakesrc_module.h"

void tiovx_fakesrc_init_cfg(TIOVXFakesrcNodeCfg *node_cfg)
{
    node_cfg->sink_pad = NULL;
}

vx_status tiovx_fakesrc_init_node(NodeObj *node)
{
    vx_status status = VX_FAILURE;
    TIOVXFakesrcNodeCfg *node_cfg = (TIOVXFakesrcNodeCfg *)node->node_cfg;

    node->num_inputs = 0;
    node->num_outputs = 1;

    node->srcs[0].node = node;
    node->srcs[0].pad_index = 0;
    node->srcs[0].node_parameter_index =
                          node_cfg->sink_pad->node_parameter_index;
    node->srcs[0].num_channels = node_cfg->sink_pad->num_channels;
    vxRetainReference(node_cfg->sink_pad->exemplar);
    vxRetainReference((vx_reference)node_cfg->sink_pad->exemplar_arr);
    node->srcs[0].exemplar = node_cfg->sink_pad->exemplar;
    node->srcs[0].exemplar_arr = node_cfg->sink_pad->exemplar_arr;

    status = tiovx_modules_link_pads(&node->srcs[0], node_cfg->sink_pad);
    if (VX_SUCCESS != status) {
        TIOVX_MODULE_ERROR("[FAKESRC] Failed to link sink pad\n");
        return status;
    }

    sprintf(node->name, "fakesrc_node");

    return status;
}

vx_status tiovx_fakesrc_create_node(NodeObj *node)
{
    vx_status status = VX_SUCCESS;
    TIOVXFakesrcNodeCfg *node_cfg = (TIOVXFakesrcNodeCfg *)node->node_cfg;

    node->tiovx_node = node_cfg->sink_pad->node->tiovx_node;

    return status;
}

vx_status tiovx_fakesrc_delete_node(NodeObj *node)
{
    vx_status status = VX_SUCCESS;

    return status;
}

vx_uint32 tiovx_fakesrc_get_cfg_size()
{
    return sizeof(TIOVXFakesrcNodeCfg);
}
