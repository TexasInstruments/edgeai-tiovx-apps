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

#include <tiovx_modules.h>
#include <tiovx_utils.h>
#include <omx_decode_module.h>

#define APP_BUFQ_DEPTH      (12)
#define APP_NUM_CH          (1)
#define APP_NUM_ITERATIONS  (200)
#define APP_NUM_PRE_LOAD    (3)

#define MIN(a, b) a<b?a:b

vx_status app_modules_qnx_decode_display_test(vx_int32 argc, vx_char* argv[])
{
    vx_status status = VX_FAILURE;
    GraphObj graph;
    NodeObj *node = NULL;
    TIOVXDisplayNodeCfg display_cfg;
    BufPool *in_buf_pool = NULL;
    Buf *inbuf = NULL;
    omxDecodeCfg omx_decode_cfg;
    omxDecodeHandle *omx_decode_handle;
    omxDecodeOutFmt omx_decode_fmt;

    omx_decode_init_cfg(&omx_decode_cfg);
    omx_decode_cfg.bufq_depth = APP_BUFQ_DEPTH;
    sprintf(omx_decode_cfg.file, "%s/videos/video0_1280_768.h264", EDGEAI_DATA_PATH);
    omx_decode_handle = omx_decode_create_handle(&omx_decode_cfg, &omx_decode_fmt);

    tiovx_display_init_cfg(&display_cfg);

    display_cfg.width = 1280;
    display_cfg.height = 768;
    display_cfg.params.outWidth  = 1280;
    display_cfg.params.outHeight = 768;
    display_cfg.params.posX = (1920 - 1280)/2;
    display_cfg.params.posY = (1080 - 768)/2;

    status = tiovx_modules_initialize_graph(&graph);
    graph.schedule_mode = VX_GRAPH_SCHEDULE_MODE_QUEUE_AUTO;
    node = tiovx_modules_add_node(&graph, TIOVX_DISPLAY, (void *)&display_cfg);
    node->sinks[0].bufq_depth = APP_BUFQ_DEPTH;
    status = tiovx_modules_verify_graph(&graph);

    in_buf_pool = node->sinks[0].buf_pool;

    for (int i = 0; i < APP_BUFQ_DEPTH; i++) {
        inbuf = tiovx_modules_acquire_buf(in_buf_pool);
        omx_decode_enqueue_buf(omx_decode_handle, inbuf);
    }

    omx_decode_start(omx_decode_handle);

    for (int i = 0; i < APP_NUM_PRE_LOAD; i++) {
        inbuf = omx_decode_dqueue_buf(omx_decode_handle);
        tiovx_modules_enqueue_buf(inbuf);
    }

    for (int i = 0; i < APP_NUM_ITERATIONS; i++) {
        inbuf = tiovx_modules_dequeue_buf(in_buf_pool);
        omx_decode_enqueue_buf(omx_decode_handle, inbuf);
        inbuf = omx_decode_dqueue_buf(omx_decode_handle);
        tiovx_modules_enqueue_buf(inbuf);
    }

    for (int i = 0; i < APP_NUM_PRE_LOAD - 1; i++) {
        tiovx_modules_dequeue_buf(in_buf_pool);
    }

    omx_decode_stop(omx_decode_handle);
    omx_decode_delete_handle(omx_decode_handle);

    tiovx_modules_clean_graph(&graph);

    return status;
}
