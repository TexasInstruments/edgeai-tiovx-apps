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
#ifndef _TIOVX_MODULES
#define _TIOVX_MODULES

#include "tiovx_modules_types.h"
#include "tiovx_modules_cbs.h"

#ifdef __cplusplus
extern "C" {
#endif

/*! \brief Function to initialize a graph object.
 * \param [in,out] graph Graph object \ref _GraphObj.
 * \ingroup tiovx_modules
 */
vx_status tiovx_modules_initialize_graph(GraphObj *graph);

/*! \brief Function to add a new node to graph.
 * \param [in,out] graph Graph object \ref _GraphObj.
 * \param [in] node_type Enum associated with the module to add node for \ref NODE_TYPES.
 * \parm [in] cfg Configuration needed for given module
 *            This is module specific structure defined in module header.
 *
 * \return New node object \ref NodeObj
 *
 * \ingroup tiovx_modules
 */
NodeObj* tiovx_modules_add_node(GraphObj *graph, NODE_TYPES node_type, void *cfg);


/*! \brief Function to be called after adding and connecting all the nodes.
 *         This function will created actual OpenVX Nodes, creates graph parameters
 *         for floating pads and created buffer pools for all graph params
 *
 * \param [in,out] graph Graph object \ref _GraphObj.
 * \ingroup tiovx_modules
 */
vx_status tiovx_modules_verify_graph(GraphObj *graph);


/*! \brief Function to export graph as a dot file.
 *
 * \param [in] graph Graph object \ref _GraphObj.
 * \param [in] path To dump the dot files.
 * \param [in] prefix To be added to dot files.
 * \ingroup tiovx_modules
 */
vx_status tiovx_modules_export_graph(GraphObj *graph, char *path, char *prefix);

/*! \brief Function to get access to node object by name.
 *
 * \param [in] graph Graph object \ref _GraphObj.
 * \param [in] name of the node.
 *
 * \return Node object of given name \ref NodeObj
 *
 * \ingroup tiovx_modules
 */
NodeObj* tiovx_modules_get_node_by_name(GraphObj *graph, char *name);

/*! \brief Acquire a free buffer from a buffer pool.
 *
 * \param [in] buf_pool Buffer pool \ref _BufPool.
 *
 * \return Free Buffer \ref _Buf
 *
 * \ingroup tiovx_modules
 */
Buf* tiovx_modules_acquire_buf(BufPool *buf_pool);

/*! \brief Release a previously acquired buffer back to its pool.
 *
 * \param [in] buf Buffer to be released \ref _Buf.
 *
 * \ingroup tiovx_modules
 */
vx_status tiovx_modules_release_buf(Buf *buf);

/*! \brief Function to clean the graph at the end of the application.
 *
 * \param [in] buf Buffer to be released \ref _Buf.
 *
 * \ingroup tiovx_modules
 */
vx_status tiovx_modules_clean_graph(GraphObj *graph);

/*! \brief Function to enqueue a buffer to OpenVX graph.
 *
 * \param [in] buf Buffer to be enqueued \ref _Buf.
 *
 * \ingroup tiovx_modules
 */
vx_status tiovx_modules_enqueue_buf(Buf *buf);

/*! \brief dequeue a buffer from OpenVX graph.
 *
 * \param [in] buf_pool Buffer pool \ref _BufPool.
 *
 * \return Free Buffer \ref _Buf
 *
 * \ingroup tiovx_modules
 */
Buf* tiovx_modules_dequeue_buf(BufPool *buf_pool);

/*! \brief Function to schedule graph, if mode set to manual.
 *
 * \param [in] buf Buffer to be enqueued \ref _Buf.
 *
 * \ingroup tiovx_modules
 */
vx_status tiovx_modules_schedule_graph(GraphObj *graph);

/*! \brief Function to wait for graph exec, if mode set to manual.
 *
 * \param [in] buf Buffer to be enqueued \ref _Buf.
 *
 * \ingroup tiovx_modules
 */
vx_status tiovx_modules_wait_graph(GraphObj *graph);


/*! \brief Function to release a node, to be used only in modules.
 *
 * \param [in] Node object \ref NodeObj
 *
 * \ingroup tiovx_modules
 */
vx_status tiovx_modules_release_node(NodeObj *node);

#ifdef __cplusplus
}
#endif

#endif //_TIOVX_MODULES
