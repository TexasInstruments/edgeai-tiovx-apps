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
#include "tiovx_modules.h"
#include <edgeai_tiovx_img_proc.h>
#include <TI/hwa_kernels.h>
#include <TI/dl_kernels.h>
#include <TI/video_io_kernels.h>
#include <stdlib.h>

extern NodeCbs gNodeCbs[TIOVX_MODULES_NUM_MODULES];

vx_status tiovx_modules_initialize_graph(GraphObj *graph)
{
    vx_status status = VX_FAILURE;

    CLR(graph);
    graph->tiovx_context = vxCreateContext();

    tivxHwaLoadKernels(graph->tiovx_context);
    tivxImgProcLoadKernels(graph->tiovx_context);
    tivxEdgeaiImgProcLoadKernels(graph->tiovx_context);
    tivxTIDLLoadKernels(graph->tiovx_context);
    tivxVideoIOLoadKernels(graph->tiovx_context);
#if !defined (SOC_AM62A)
    tivxImagingLoadKernels(graph->tiovx_context);
#endif

    graph->tiovx_graph = vxCreateGraph(graph->tiovx_context);
    graph->schedule_mode = VX_GRAPH_SCHEDULE_MODE_QUEUE_MANUAL;

    status = VX_SUCCESS;

    return status;
}

NodeObj* tiovx_modules_add_node(GraphObj *graph, NODE_TYPES node_type, void *cfg)
{
    vx_status status = VX_FAILURE;
    NodeObj *node = &(graph->node_list[graph->num_nodes]);

    node->graph = graph;
    node->node_index = graph->num_nodes;
    node->cbs = &gNodeCbs[node_type];
    node->node_cfg = malloc(node->cbs->get_cfg_size());

    if (node->cbs->get_priv_size) {
        node->node_priv = malloc(node->cbs->get_priv_size());
    }

    for (uint8_t i = 0; i < TIOVX_MODULES_MAX_NODE_INPUTS; i++) {
        node->sinks[i].direction = SINK;
        node->sinks[i].num_channels = TIOVX_MODULES_DEFAULT_NUM_CHANNELS;
        node->sinks[i].bufq_depth = TIOVX_MODULES_DEFAULT_BUFQ_DEPTH;
        node->sinks[i].graph_parameter_index = -1;
        node->sinks[i].node_parameter_index = -1;
    }

    for (uint8_t i = 0; i < TIOVX_MODULES_MAX_NODE_OUTPUTS; i++) {
        node->srcs[i].direction = SRC;
        node->srcs[i].num_channels = TIOVX_MODULES_DEFAULT_NUM_CHANNELS;
        node->srcs[i].bufq_depth = TIOVX_MODULES_DEFAULT_BUFQ_DEPTH;
        node->srcs[i].graph_parameter_index = -1;
        node->srcs[i].node_parameter_index = -1;
    }

    memcpy(node->node_cfg, cfg, node->cbs->get_cfg_size());

    status = node->cbs->init_node(node);
    if (VX_SUCCESS != status) {
        TIOVX_MODULE_ERROR("Node Init failed, Node Type: %d\n", node_type);
        CLR(node);
        node = NULL;
    } else {
        graph->num_nodes += 1;
    }

    return node;
}

Buf* tiovx_modules_acquire_buf(BufPool *buf_pool)
{
    Buf *buf = NULL;

    buf_pool->free_count--;
    buf = buf_pool->freeQ[buf_pool->free_count];
    buf->acquired = vx_true_e;

    return buf;
}

vx_status tiovx_modules_release_buf(Buf *buf)
{
    vx_status status = VX_FAILURE;

    buf->acquired = vx_false_e;
    buf->pool->freeQ[buf->pool->free_count] = buf;
    buf->pool->free_count++;

    status = VX_SUCCESS;

    return status;
}

BufPool* tiovx_modules_allocate_bufpool(Pad *pad)
{
    vx_status status = VX_FAILURE;
    BufPool* buf_pool = (BufPool *)malloc(sizeof(BufPool));

    buf_pool->pad = pad;
    buf_pool->bufq_depth = pad->bufq_depth;
    buf_pool->free_count = 0;
    buf_pool->enqueue_head = 0;
    buf_pool->enqueue_tail = 0;

    for(uint8_t i = 0; i < pad->bufq_depth ; i++)
    {

        buf_pool->bufs[i].arr = vxCreateObjectArray(
                                               pad->node->graph->tiovx_context,
                                               pad->exemplar,
                                               pad->num_channels);
        status = vxGetStatus((vx_reference)buf_pool->bufs[i].arr);
        if(status != VX_SUCCESS)
        {
            TIOVX_MODULE_ERROR("Creating Bufpool failed\n");
            free(buf_pool);
            buf_pool = NULL;
            break;
        }

        buf_pool->bufs[i].handle = vxGetObjectArrayItem(buf_pool->bufs[i].arr,
                                                        0);
        buf_pool->bufs[i].pool = buf_pool;
        buf_pool->bufs[i].buf_index = i;
        buf_pool->bufs[i].acquired = vx_true_e;
        buf_pool->bufs[i].queued = vx_false_e;
        buf_pool->bufs[i].num_channels = pad->num_channels;
        buf_pool->ref_list[i] = (vx_reference)buf_pool->bufs[i].arr;

        tiovx_modules_release_buf(&buf_pool->bufs[i]);
    }

    return buf_pool;
}

void tiovx_modules_free_bufpool(BufPool *buf_pool)
{
    for(uint8_t i = 0; i < buf_pool->bufq_depth ; i++) {
        vxReleaseReference(&buf_pool->bufs[i].handle);
        vxReleaseObjectArray(&buf_pool->bufs[i].arr);
    }
}

vx_bool tiovx_modules_compare_exemplars(vx_reference exempler1, vx_reference exemplar2)
{
    return vx_true_e;
}

vx_status tiovx_modules_link_pads(Pad *src_pad, Pad *sink_pad)
{
    vx_status status = VX_FAILURE;

    if (SRC != src_pad->direction) {
        TIOVX_MODULE_ERROR("Invalid src pad\n");
        return status;
    }

    if (SINK != sink_pad->direction) {
        TIOVX_MODULE_ERROR("Invalid sink pad\n");
        return status;
    }

    if (src_pad->num_channels != sink_pad->num_channels) {
        TIOVX_MODULE_ERROR("Number of channels does not match\n");
        return status;
    }

    if (vx_false_e == tiovx_modules_compare_exemplars(src_pad->exemplar,
                                                      sink_pad->exemplar)) {
        TIOVX_MODULE_ERROR("Pad exemplars do not match\n");
        return status;
    }

    if (NULL != src_pad->peer_pad) {
        TIOVX_MODULE_ERROR("Src pad already linked\n");
        return status;
    }

    if (NULL != sink_pad->peer_pad) {
        TIOVX_MODULE_ERROR("Sink pad already linked\n");
        return status;
    }

    vxRetainReference(src_pad->exemplar);
    vxRetainReference((vx_reference)src_pad->exemplar_arr);
    vxReleaseReference(&sink_pad->exemplar);
    vxReleaseObjectArray(&sink_pad->exemplar_arr);

    sink_pad->exemplar = src_pad->exemplar;
    sink_pad->exemplar_arr = src_pad->exemplar_arr;

    src_pad->peer_pad = sink_pad;
    sink_pad->peer_pad = src_pad;

    status = VX_SUCCESS;

    return status;
}

vx_status tiovx_modules_add_graph_parameter_by_node_index(vx_graph graph, vx_node node, vx_uint32 node_parameter_index)
{
    vx_parameter parameter = vxGetParameterByIndex(node, node_parameter_index);
    vx_status status;
    status = vxAddParameterToGraph(graph, parameter);
    if(status == VX_SUCCESS)
    {
        status = vxReleaseParameter(&parameter);
    }
    return status;
}

vx_status tiovx_modules_add_to_graph_params(Pad *pad)
{
    vx_status status = VX_FAILURE;
    GraphObj *graph = pad->node->graph;
    NodeObj *node = pad->node;
    BufPool *pool = pad->buf_pool;
    status = tiovx_modules_add_graph_parameter_by_node_index(
                                                    graph->tiovx_graph,
                                                    node->tiovx_node,
                                                    pad->node_parameter_index);
    pad->graph_parameter_index = graph->num_graph_params;
    graph->graph_params_list[graph->num_graph_params].graph_parameter_index =
                                                        graph->num_graph_params;
    graph->graph_params_list[graph->num_graph_params].refs_list_size =
                                                               pool->bufq_depth;
    graph->graph_params_list[graph->num_graph_params].refs_list =
                                                               pool->ref_list;
    graph->num_graph_params++;

    status = VX_SUCCESS;

    return status;
}

vx_status tiovx_modules_verify_graph(GraphObj *graph)
{
    vx_status status = VX_FAILURE;
    NodeObj *node = NULL;
    Pad *pad = NULL;

    for (uint8_t i = 0; i < graph->num_nodes; i++) {
        node = &graph->node_list[i];
        status = node->cbs->create_node(node);

        for (uint8_t j = 0; j < node->num_inputs; j++) {
            pad = &node->sinks[j];

            if (NULL == pad->peer_pad) {
                pad->buf_pool = tiovx_modules_allocate_bufpool(pad);
                status = tiovx_modules_add_to_graph_params(pad);
            }
        }

        for (uint8_t j = 0; j < node->num_outputs; j++) {
            pad = &node->srcs[j];

            if (NULL == pad->peer_pad) {
                pad->buf_pool = tiovx_modules_allocate_bufpool(pad);
                status = tiovx_modules_add_to_graph_params(pad);
            }
        }

        if(status != VX_SUCCESS)
        {
            TIOVX_MODULE_ERROR("Creating Node failed: %d\n", i);
            return status;
        }
    }

    status = vxSetGraphScheduleConfig(graph->tiovx_graph,
                                      graph->schedule_mode,
                                      graph->num_graph_params,
                                      graph->graph_params_list);

    status = vxVerifyGraph(graph->tiovx_graph);
    if(status != VX_SUCCESS)
    {
        TIOVX_MODULE_ERROR("Graph Verify failed");
        return status;
    }

    for (uint8_t i = 0; i < graph->num_nodes; i++) {
        node = &graph->node_list[i];

        if (NULL != node->cbs->post_verify_graph) {
            status = node->cbs->post_verify_graph(node);
        }
    }

    return status;
}

vx_status tiovx_modules_enqueue_buf(Buf *buf)
{
    vx_status status = VX_FAILURE;
    BufPool *buf_pool = buf->pool;
    Pad *pad = buf_pool->pad;
    GraphObj *graph = pad->node->graph;

    buf_pool->enqueuedQ[buf_pool->enqueue_head] = buf;
    buf_pool->enqueue_head++;
    buf->queued = vx_true_e;

    vxGraphParameterEnqueueReadyRef(graph->tiovx_graph,
                                    pad->graph_parameter_index,
                                    (vx_reference *)&buf->arr, 1);

    status = VX_SUCCESS;

    return status;
}

Buf* tiovx_modules_dequeue_buf(BufPool *buf_pool)
{
    Buf *buf = buf_pool->enqueuedQ[buf_pool->enqueue_tail];
    Pad *pad = buf_pool->pad;
    GraphObj *graph = pad->node->graph;
    vx_object_array ref = NULL;
    vx_uint32 num_refs = 0;

    buf_pool->enqueue_head--;
    buf->queued = vx_false_e;

    vxGraphParameterDequeueDoneRef(graph->tiovx_graph,
                                   pad->graph_parameter_index,
                                   (vx_reference *)&ref, 1, &num_refs);

    return buf;
}

vx_status tiovx_modules_schedule_graph(GraphObj *graph)
{
    vx_status status = VX_SUCCESS;
    if (graph->schedule_mode == VX_GRAPH_SCHEDULE_MODE_QUEUE_MANUAL)
    {
        status = vxScheduleGraph(graph->tiovx_graph);
    }
    return status;
}

vx_status tiovx_modules_wait_graph(GraphObj *graph)
{
    vx_status status = VX_SUCCESS;
    if (graph->schedule_mode == VX_GRAPH_SCHEDULE_MODE_QUEUE_MANUAL)
    {
        status = vxWaitGraph(graph->tiovx_graph);
    }
    return status;
}

vx_status tiovx_modules_delete_node(NodeObj *node)
{
    vx_status status = VX_FAILURE;

    node->cbs->delete_node(node);

    free(node->node_cfg);
    if (node->node_priv) {
        free(node->node_priv);
    }

    return status;
}

vx_status tiovx_modules_release_node(NodeObj *node)
{
    vx_status status = VX_FAILURE;

    status = vxReleaseNode(&node->tiovx_node);

    return status;
}

vx_status tiovx_module_create_pad_exemplar(Pad *pad, vx_reference exemplar)
{
    vx_status status = VX_FAILURE;

    pad->exemplar_arr = vxCreateObjectArray(pad->node->graph->tiovx_context,
                                            exemplar, pad->num_channels);
    status = vxGetStatus((vx_reference)pad->exemplar_arr);
    if(status != VX_SUCCESS)
    {
        TIOVX_MODULE_ERROR("Creating Pad exemplar failed\n");
        return status;
    }

    pad->exemplar = vxGetObjectArrayItem(pad->exemplar_arr, 0);

    return status;
}

vx_status tiovx_modules_clean_graph(GraphObj *graph)
{
    vx_status status = VX_FAILURE;
    NodeObj *node = NULL;
    Pad *pad = NULL;

    for (uint8_t i = 0; i < graph->num_nodes; i++) {
        node = &(graph->node_list[i]);

        for (uint8_t j = 0; j < node->num_inputs; j++) {
            pad = &node->sinks[j];
            vxReleaseReference(&pad->exemplar);
            vxReleaseObjectArray(&pad->exemplar_arr);
            if (pad->peer_pad == NULL) {
                tiovx_modules_free_bufpool(pad->buf_pool);
            }
        }

        for (uint8_t j = 0; j < node->num_outputs; j++) {
            pad = &node->srcs[j];
            vxReleaseReference(&pad->exemplar);
            vxReleaseObjectArray(&pad->exemplar_arr);
            if (pad->peer_pad == NULL) {
                tiovx_modules_free_bufpool(pad->buf_pool);
            }
        }

        tiovx_modules_delete_node(node);
    }

    vxReleaseGraph(&graph->tiovx_graph);

#if !defined (SOC_AM62A)
    tivxImagingUnLoadKernels(graph->tiovx_context);
#endif
    tivxVideoIOUnLoadKernels(graph->tiovx_context);
    tivxTIDLUnLoadKernels(graph->tiovx_context);
    tivxEdgeaiImgProcUnLoadKernels(graph->tiovx_context);
    tivxImgProcUnLoadKernels(graph->tiovx_context);
    tivxHwaUnLoadKernels(graph->tiovx_context);

    vxReleaseContext(&graph->tiovx_context);
    CLR(graph);

    status = VX_SUCCESS;

    return status;
}
