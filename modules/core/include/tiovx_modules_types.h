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
#include <semaphore.h>

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
#define TIOVX_MODULES_MAX_NODES           (128u)
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

#if defined(TARGET_OS_QNX)
#define TIOVX_MODULES_DATA_PATH "/ti_fs/edgeai/edgeai-test-data/"
#define TIOVX_MODULES_IMAGING_PATH "/ti_fs/edgeai/imaging/"
#else
#define TIOVX_MODULES_DATA_PATH "/opt/edgeai-test-data/"
#define TIOVX_MODULES_IMAGING_PATH "/opt/imaging/"
#endif

/*!
 * \brief Enum for pad directions.
 */
typedef enum {
    SRC = 0,
    SINK,
} PAD_DIRECTION;

typedef struct _Pad         Pad;
typedef struct _NodeObj     NodeObj;
typedef struct _GraphObj    GraphObj;
typedef struct _Buf         Buf;
typedef struct _BufPool     BufPool;

/*!
 * \brief Structure describing a Buffer object.
 */
struct _Buf {
    /*! \brief Bufferpool to which the buffer belongs to \ref _BufPool */
    BufPool             *pool;

    /*! \brief Index of the buffer with respect to all the buffers in the pool */
    vx_int32            buf_index;

    /*! \brief Object array for the buffer */
    vx_object_array     arr;

    /*! \brief 0th handle of the object array */
    vx_reference        handle;

    /*! \brief Number of channels */
    vx_int32            num_channels;
};

/*!
 * \brief Structure describing a Buffer pool.
 */
struct _BufPool {
    /*! \brief Pad with which the buffer pool is allocated for \ref _Pad */
    Pad                 *pad;

    /*! \brief List of buffers in the pool \ref _Buf */
    Buf                 bufs[TIOVX_MODULES_MAX_BUFQ_DEPTH];

    /*! \brief Number of buffers in the pool */
    vx_int32            bufq_depth;

    /*! \brief Array that contains all free buffers \ref _Buf */
    Buf                 *freeQ[TIOVX_MODULES_MAX_BUFQ_DEPTH];

    /*! \brief Number of buffers that are free */
    vx_int32            free_count;

    /*! \brief List of buffers that are enqueued \ref _Buf */
    Buf                 *enqueuedQ[TIOVX_MODULES_MAX_BUFQ_DEPTH];

    /*! \brief Head pointer for queue management */
    vx_int32            enqueue_head;

    /*! \brief Tail pointer for queue management */
    vx_int32            enqueue_tail;

    /*! \brief Ref list for graph param */
    vx_reference        ref_list[TIOVX_MODULES_MAX_BUFQ_DEPTH];

    /*! \brief Mutex for queue management */
    pthread_mutex_t     lock;

    /*! \brief Semaphore for acquire and release */
    sem_t               sem;
};

/*!
 * \brief Structure describing a Pad.
 */
struct _Pad {
    /*! \brief Node to which the pad belongs to \ref _NodeObj */
    NodeObj             *node;

    /*! \brief Peer pad to which the pad is connected to */
    Pad                 *peer_pad;

    /*! \brief Pad direction \ref PAD_DIRECTION */
    PAD_DIRECTION       direction;

    /*! \brief Index among all the pads in the node with same direction */
    vx_int32            pad_index;

    /*! \brief Node parameter index */
    vx_int32            node_parameter_index;

    /*! \brief Graph parameter index */
    vx_int32            graph_parameter_index;

    /*! \brief Number of channels */
    vx_int32            num_channels;


    /*! \brief Buffer queue depth */
    vx_int32            bufq_depth;

    /*! \brief Buffer pool allocated for this pad */
    BufPool             *buf_pool;

    /*! \brief vx_reference that defines the data that this pad handles */
    vx_reference        exemplar;
    vx_object_array     exemplar_arr;

    /*! \brief vx_bool Enqueue objext array instead of handle if set to true */
    vx_bool             enqueue_arr;
};

/*!
 * \brief Structure of function prototypes to be implemented by each module.
 */
typedef struct {
    vx_status (*init_node)(NodeObj *node);
    vx_status (*create_node)(NodeObj *node);
    vx_status (*post_verify_graph)(NodeObj *node);
    vx_status (*delete_node)(NodeObj *node);
    vx_uint32 (*get_cfg_size)();
    vx_uint32 (*get_priv_size)();
} NodeCbs;

/*!
 * \brief Structure describing a Node.
 */
struct _NodeObj {
    /*! \brief Name of the node */
    vx_char             name[VX_MAX_REFERENCE_NAME];

    /*! \brief Name type \ref NODE_TYPES */
    vx_int32            node_type;

    /*! \brief Pointer to the parent Graph \ref _GraphObj */
    GraphObj            *graph;

    /*! \brief Copy of node cfg supplied during add node */
    void                *node_cfg;

    /*! \brief Index among all nodes in the graph */
    vx_int32            node_index;

    /*! \brief Actual OpenVx node */
    vx_node             tiovx_node;

    /*! \brief Number of inputs to the Node */
    vx_int32            num_inputs;

    /*! \brief Number of outputs to the Node */
    vx_int32            num_outputs;

    /*! \brief List of input pads \ref _Pad */
    Pad                 sinks[TIOVX_MODULES_MAX_NODE_INPUTS];

    /*! \brief List of output pads \ref _Pad */
    Pad                 srcs[TIOVX_MODULES_MAX_NODE_OUTPUTS];

    /*! \brief Module call backs \ref NodeCbs */
    NodeCbs             *cbs;

    /*! \brief Pointer to private data managed by modules */
    void                *node_priv;
};

struct _GraphObj {
    /*! \brief OpenVX context used for creating all OpenVX references */
    vx_context                          tiovx_context;

    /*! \brief Actual OpenVX graph */
    vx_graph                            tiovx_graph;

    /*! \brief Graph params list */
    vx_graph_parameter_queue_params_t   graph_params_list[TIOVX_MODULES_MAX_GRAPH_PARAMS];

    /*! \brief Number of graph params */
    vx_int32                            num_graph_params;

    /*! \brief Number of nodes in the graph */
    vx_int32                            num_nodes;

    /*! \brief List of nodes in the graph */
    NodeObj                             node_list[TIOVX_MODULES_MAX_NODES];

    /* \brief Schedule mode (AUTO or MANUAL) */
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

/*! \brief Function to link two pads.
 *
 * \param [in] src_pad Source pad\ref _Pad.
 * \param [in] sink_pad Sink pad \ref _Pad.
 *
 * \ingroup tiovx_modules
 */
vx_status tiovx_modules_link_pads(Pad *src_pad, Pad *sink_pad);

#ifdef __cplusplus
}
#endif

#endif //_TIOVX_MODULES_TYPES
