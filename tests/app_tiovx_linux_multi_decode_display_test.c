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
#include <v4l2_decode_module.h>
#include <kms_display_module.h>
#include <time.h>

#define APP_BUFQ_DEPTH      (10)
#define APP_NUM_CH          (1)
#define APP_NUM_ITERATIONS  (600)
#define NUM_VID  (2)
#define WINDOW_WIDTH  (960)
#define WINDOW_HEIGHT  (540)

vx_status app_modules_linux_multi_decode_display_test(vx_int32 argc, vx_char* argv[])
{
    vx_status status = VX_FAILURE;
    GraphObj graph;
    NodeObj *node = NULL;
    TIOVXMosaicNodeCfg cfg;
    BufPool *in_buf_pool_1 = NULL, *in_buf_pool_2 = NULL, *out_buf_pool = NULL;
    Buf *inbuf_1 = NULL, *inbuf_2 = NULL, *outbuf = NULL;
    v4l2DecodeCfg v4l2_decode_cfg_1, v4l2_decode_cfg_2;
    v4l2DecodeHandle *v4l2_decode_handle[NUM_VID];
    v4l2DecodeOutFmt v4l2_decode_fmt;
    kmsDisplayCfg kms_display_cfg;
    kmsDisplayHandle *kms_display_handle;
    uint8_t i;

    v4l2_decode_init_cfg(&v4l2_decode_cfg_1);
    v4l2_decode_init_cfg(&v4l2_decode_cfg_2);

    v4l2_decode_cfg_1.bufq_depth = APP_BUFQ_DEPTH;
    v4l2_decode_cfg_2.bufq_depth = APP_BUFQ_DEPTH;
    sprintf(v4l2_decode_cfg_1.device, "/dev/video0");
    sprintf(v4l2_decode_cfg_2.device, "/dev/video2");

    sprintf(v4l2_decode_cfg_1.file, "%s/videos/video0_3840_2160.h264", EDGEAI_DATA_PATH);
    sprintf(v4l2_decode_cfg_2.file, "%s/videos/video0_3840_2160.h264", EDGEAI_DATA_PATH);

    tiovx_mosaic_init_cfg(&cfg);
    cfg.color_format = VX_DF_IMAGE_NV12;
    cfg.num_inputs = NUM_VID;
    cfg.output_cfg.width = 1920;
    cfg.output_cfg.height = 1080;
    cfg.params.num_windows  = NUM_VID;
    cfg.params.clear_count  = 4;

    for (i = 0; i < NUM_VID; i++) {
        v4l2_decode_handle[i] = v4l2_decode_create_handle(&v4l2_decode_cfg_1, &v4l2_decode_fmt);
        v4l2_decode_handle[i] = v4l2_decode_create_handle(&v4l2_decode_cfg_2, &v4l2_decode_fmt);

        cfg.input_cfgs[i].width = v4l2_decode_fmt.width;
        cfg.input_cfgs[i].height = v4l2_decode_fmt.height;
        cfg.params.windows[i].startX  = (i % 2) * WINDOW_WIDTH;
        cfg.params.windows[i].startY  = (i/2) * WINDOW_HEIGHT;
        cfg.params.windows[i].width   = WINDOW_WIDTH;
        cfg.params.windows[i].height  = WINDOW_HEIGHT;
        cfg.params.windows[i].input_select   = i;
        cfg.params.windows[i].channel_select = 0;
    }

    status = tiovx_modules_initialize_graph(&graph);
    graph.schedule_mode = VX_GRAPH_SCHEDULE_MODE_QUEUE_AUTO;
    node = tiovx_modules_add_node(&graph, TIOVX_MOSAIC, (void *)&cfg);
    node->sinks[0].bufq_depth = APP_BUFQ_DEPTH;
    node->sinks[1].bufq_depth = APP_BUFQ_DEPTH;
    status = tiovx_modules_verify_graph(&graph);

    in_buf_pool_1 = node->sinks[0].buf_pool;
    in_buf_pool_2 = node->sinks[1].buf_pool;
    out_buf_pool = node->srcs[0].buf_pool;

    kms_display_init_cfg(&kms_display_cfg);
    kms_display_cfg.width = 1920;
    kms_display_cfg.height = 1080;
    kms_display_cfg.pix_format = DRM_FORMAT_NV12;

    kms_display_handle = kms_display_create_handle(&kms_display_cfg);

    for (int i = 0; i < APP_BUFQ_DEPTH; i++) {
        inbuf_1 = tiovx_modules_acquire_buf(in_buf_pool_1);
        inbuf_2 = tiovx_modules_acquire_buf(in_buf_pool_2);
        v4l2_decode_enqueue_buf(v4l2_decode_handle[0], inbuf_1);
        v4l2_decode_enqueue_buf(v4l2_decode_handle[1], inbuf_2);
    }

    v4l2_decode_start(v4l2_decode_handle[0]);
    v4l2_decode_start(v4l2_decode_handle[1]);

    for (int i = 0; i < out_buf_pool->bufq_depth; i++) {
        inbuf_1 = v4l2_decode_dqueue_buf(v4l2_decode_handle[0]);
        inbuf_2 = v4l2_decode_dqueue_buf(v4l2_decode_handle[1]);
        tiovx_modules_enqueue_buf(inbuf_1);
        tiovx_modules_enqueue_buf(inbuf_2);

        outbuf = tiovx_modules_acquire_buf(out_buf_pool);
        tiovx_modules_enqueue_buf(outbuf);
        kms_display_register_buf(kms_display_handle, outbuf);
    }

    struct timespec start, end;
    uint64_t delta_ms = 0;
    for (int i = 0; i < APP_NUM_ITERATIONS; i++) {
        clock_gettime(CLOCK_MONOTONIC_RAW, &start);
        inbuf_1 = tiovx_modules_dequeue_buf(in_buf_pool_1);
        inbuf_2 = tiovx_modules_dequeue_buf(in_buf_pool_2);
        outbuf = tiovx_modules_dequeue_buf(out_buf_pool);

        v4l2_decode_enqueue_buf(v4l2_decode_handle[0], inbuf_1);
        v4l2_decode_enqueue_buf(v4l2_decode_handle[1], inbuf_2);
        kms_display_render_buf(kms_display_handle, outbuf);

        inbuf_1 = v4l2_decode_dqueue_buf(v4l2_decode_handle[0]);
        inbuf_2 = v4l2_decode_dqueue_buf(v4l2_decode_handle[1]);
        tiovx_modules_enqueue_buf(inbuf_1);
        tiovx_modules_enqueue_buf(inbuf_2);
        tiovx_modules_enqueue_buf(outbuf);
        clock_gettime(CLOCK_MONOTONIC_RAW, &end);
        delta_ms += (end.tv_sec - start.tv_sec) * 1000 + (end.tv_nsec - start.tv_nsec) / 1000000;
    }
    printf("FPS = %ld\n", (1000 * APP_NUM_ITERATIONS)/delta_ms);

    for (i = 0; i < NUM_VID; i++) {
        v4l2_decode_stop(v4l2_decode_handle[i]);
        v4l2_decode_delete_handle(v4l2_decode_handle[i]);
    }

    kms_display_delete_handle(kms_display_handle);
    tiovx_modules_clean_graph(&graph);

    return status;
}
