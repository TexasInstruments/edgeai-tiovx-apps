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
#ifndef _TIOVX_MODULES_TYPES
#define _TIOVX_MODULES_TYPES

#include <TI/tivx.h>
#include <TI/tivx_fileio.h>
#include <TI/tivx_obj_desc.h>

#ifdef __cplusplus
extern "C" {
#endif

//#define TIOVX_MODULE_DEBUG

#include <stdio.h>
#include <string.h>
#include <pthread.h>

#ifdef TIOVX_MODULE_DEBUG
#define TIOVX_MODULE_PRINTF(f_, ...) printf("[TIOVX_MODULES][DEBUG] %d: %s: "f_, __LINE__, __func__, ##__VA_ARGS__)
#else
#define TIOVX_MODULE_PRINTF(f_, ...)
#endif

#define TIOVX_MODULE_ERROR(f_, ...) printf("[TIOVX_MODULES][ERROR] %d: %s: "f_, __LINE__, __func__, ##__VA_ARGS__)

#define CLR(o) memset(o, 0, sizeof(*o))

#define TIOVX_MODULES_MAX_BUFQ_DEPTH      (16u)
#define TIOVX_MODULES_MAX_NODE_INPUTS     (16u)
#define TIOVX_MODULES_MAX_NODE_OUTPUTS    (8u)
#define TIOVX_MODULES_MAX_NODES           (256u)
#define TIOVX_MODULES_MAX_NUM_CHANNELS    (16u)
#define TIOVX_MODULES_MAX_GRAPH_PARAMS    (16u)

#define TIOVX_MODULES_MAX_TENSOR_DIMS     (4u)
#define TIOVX_MODULES_MAX_TENSORS         (8u)
#define TIOVX_MODULES_MAX_PARAMS          (16u)

#define TIOVX_MODULES_DEFAULT_IMAGE_WIDTH  (640)
#define TIOVX_MODULES_DEFAULT_IMAGE_HEIGHT (480)
#define TIOVX_MODULES_DEFAULT_COLOR_FORMAT (VX_DF_IMAGE_NV12)

#define TIOVX_MODULES_DEFAULT_BUFQ_DEPTH      (2u)
#define TIOVX_MODULES_DEFAULT_NUM_CHANNELS    (1u)
#define TIOVX_MODULES_MAX_REF_HANDLES     (16u)

typedef enum {
    SRC = 0,
    SINK,
} PAD_DIRECTION;

typedef struct _Pad         Pad;
typedef struct _NodeObj     NodeObj;
typedef struct _GraphObj    GraphObj;
typedef struct _Buf         Buf;
typedef struct _BufPool     BufPool;

struct _Buf {
    BufPool             *pool;
    vx_int32            buf_index;
    vx_bool             acquired;
    vx_bool             queued;
    vx_object_array     arr;
    vx_reference        handle;
    vx_int32            num_channels;
};

struct _BufPool {
    Pad                 *pad;
    Buf                 bufs[TIOVX_MODULES_MAX_BUFQ_DEPTH];
    vx_int32            bufq_depth;
    Buf                 *freeQ[TIOVX_MODULES_MAX_BUFQ_DEPTH];
    vx_int32            free_count;
    Buf                 *enqueuedQ[TIOVX_MODULES_MAX_BUFQ_DEPTH];
    vx_int32            enqueue_head;
    vx_int32            enqueue_tail;
    vx_reference        ref_list[TIOVX_MODULES_MAX_BUFQ_DEPTH];
    pthread_mutex_t     lock;
};

struct _Pad {
    NodeObj             *node;
    Pad                 *peer_pad;
    PAD_DIRECTION       direction;
    vx_int32            pad_index;
    vx_int32            node_parameter_index;
    vx_int32            graph_parameter_index;
    vx_int32            num_channels;
    vx_int32            bufq_depth;
    BufPool             *buf_pool;
    vx_reference        exemplar;
    vx_object_array     exemplar_arr;
};

typedef struct {
    vx_status (*init_node)(NodeObj *node);
    vx_status (*create_node)(NodeObj *node);
    vx_status (*post_verify_graph)(NodeObj *node);
    vx_status (*delete_node)(NodeObj *node);
    vx_uint32 (*get_cfg_size)();
    vx_uint32 (*get_priv_size)();
} NodeCbs;

struct _NodeObj {
    GraphObj            *graph;
    void                *node_cfg;
    vx_int32            node_index;
    vx_node             tiovx_node;
    vx_int32            num_inputs;
    vx_int32            num_outputs;
    Pad                 sinks[TIOVX_MODULES_MAX_NODE_INPUTS];
    Pad                 srcs[TIOVX_MODULES_MAX_NODE_OUTPUTS];
    NodeCbs             *cbs;
    void                *node_priv;
};

struct _GraphObj {
    vx_context                          tiovx_context;
    vx_graph                            tiovx_graph;
    vx_graph_parameter_queue_params_t   graph_params_list[TIOVX_MODULES_MAX_GRAPH_PARAMS];
    vx_int32                            num_graph_params;
    vx_int32                            num_nodes;
    NodeObj                             node_list[TIOVX_MODULES_MAX_NODES];
    vx_enum                             schedule_mode;
    pthread_mutex_t                     lock;
};

typedef struct {
    tivx_raw_image_create_params_t  params;
} RawImgCfg;

typedef struct {
    vx_int32            width;
    vx_int32            height;
    vx_int32            color_format;
} ImgCfg;

typedef struct {
    vx_int32            num_dims;
    vx_int32            dim_sizes[TIVX_CONTEXT_MAX_TENSOR_DIMS];
    vx_int32            datatype;
} TensorCfg;

typedef struct {
    vx_int32            width;
    vx_int32            height;
    vx_int32            color_format;
    vx_int32            levels;
    vx_float32          scale;
} PyramidCfg;

typedef struct {
	vx_size 	        num_bins;
    vx_int32 	        offset;
    vx_uint32 	        range;
} DstCfg;

vx_status tiovx_module_create_pad_exemplar(Pad *pad, vx_reference exemplar);
vx_status tiovx_modules_link_pads(Pad *src_pad, Pad *sink_pad);

#ifdef __cplusplus
}
#endif

#endif //_TIOVX_MODULES_TYPES
