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
#include "tiovx_obj_array_split_module.h"

void tiovx_obj_array_split_init_cfg(TIOVXObjArraySplitNodeCfg *node_cfg)
{
    CLR(node_cfg);
    node_cfg->input_pad = NULL;
    node_cfg->num_outputs = 2;
    node_cfg->num_output_group0 = 1;
    node_cfg->num_output_group1 = 1;
    node_cfg->num_output_group2 = 0;
    node_cfg->num_output_group3 = 0;
}

vx_status tiovx_obj_array_split_init_node(NodeObj *node)
{
    vx_status status = VX_FAILURE;
    TIOVXObjArraySplitNodeCfg *node_cfg = (TIOVXObjArraySplitNodeCfg *)node->node_cfg;
    
    vx_uint32 output_groups[] = {node_cfg->num_output_group0,
                                 node_cfg->num_output_group1,
                                 node_cfg->num_output_group2,
                                 node_cfg->num_output_group3};
    vx_uint32 total = 0;
    vx_int32 i;

    if(node_cfg->num_outputs < 2 || node_cfg->num_outputs > 4)
    {
        TIOVX_MODULE_ERROR("[OBJ_ARRAY_SPLIT] Outputs must be between 2 and 4\n");
        return status;
    }

    for(i = 0; i < node_cfg->num_outputs; i++)
    {
        total += output_groups[i];
    }

    if (total != node_cfg->input_pad->num_channels)
    {
        TIOVX_MODULE_ERROR("[OBJ_ARRAY_SPLIT] Number of channels on input pad"
                           " does not match the sum of output groups\n");
        return status;
    }

    if (SRC != node_cfg->input_pad->direction)
    {
        TIOVX_MODULE_ERROR("[OBJ_ARRAY_SPLIT] Input pad should be a src pad\n");
        return status;
    }

    node->num_inputs = 1;
    node->num_outputs = node_cfg->num_outputs;

    node->sinks[0].node = node;
    node->sinks[0].pad_index = 0;
    node->sinks[0].node_parameter_index =
                          node_cfg->input_pad->node_parameter_index;
    node->sinks[0].num_channels = node_cfg->input_pad->num_channels;
    vxRetainReference(node_cfg->input_pad->exemplar);
    vxRetainReference((vx_reference)node_cfg->input_pad->exemplar_arr);
    node->sinks[0].exemplar = node_cfg->input_pad->exemplar;
    node->sinks[0].exemplar_arr = node_cfg->input_pad->exemplar_arr;

    for(i = 0; i < node_cfg->num_outputs; i++)
    {
        node->srcs[i].node = node;
        node->srcs[i].pad_index = i;
        node->srcs[i].node_parameter_index =
                            node_cfg->input_pad->node_parameter_index;
        node->srcs[i].num_channels = output_groups[i];
        
        status = tiovx_module_create_pad_exemplar(&node->srcs[i],
                                                  (vx_reference) node_cfg->input_pad->exemplar);
        if (VX_SUCCESS != status)
        {
            TIOVX_MODULE_ERROR("[OBJ_ARRAY_SPLIT] Create Output Failed\n");
            return status;
        }
    }

    sprintf(node->name, "obj_arr_split_node");

    status = tiovx_modules_link_pads(node_cfg->input_pad, &node->sinks[0]);
    if (VX_SUCCESS != status) {
        TIOVX_MODULE_ERROR("[OBJ_ARRAY_SPLIT] Failed to link sink pad\n");
        return status;
    }

    return status;
}

vx_status tiovx_obj_array_split_create_node(NodeObj *node)
{
    vx_status status = VX_FAILURE;
    TIOVXObjArraySplitNodeCfg *node_cfg = (TIOVXObjArraySplitNodeCfg *)node->node_cfg;
    vx_object_array outputs[] = {NULL,NULL,NULL,NULL};
    vx_int32 i;

    for (i = 0; i < node_cfg->num_outputs; i++)
        outputs[i] = node->srcs[i].exemplar_arr;

    node->tiovx_node = tivxObjArraySplitNode(node->graph->tiovx_graph,
                                             node->sinks[0].exemplar_arr,
                                             outputs[0],
                                             outputs[1],
                                             outputs[2],
                                             outputs[3]);

    status = vxGetStatus((vx_reference)node->tiovx_node);
    if (VX_SUCCESS != status) {
        TIOVX_MODULE_ERROR("[OBJ_ARRAY_SPLIT] Create Node Failed\n");
        return status;
    }

    vxSetNodeTarget(node->tiovx_node, VX_TARGET_STRING, TIVX_TARGET_MPU_0);
    vxSetReferenceName((vx_reference)node->tiovx_node, "obj_array_split_node");
    return status;
}

vx_status tiovx_obj_array_split_delete_node(NodeObj *node)
{
    vx_status status = VX_FAILURE;
    status = VX_SUCCESS;
    return status;
}

vx_uint32 tiovx_obj_array_split_get_cfg_size()
{
    return sizeof(TIOVXObjArraySplitNodeCfg);
}
