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

#define APP_BUFQ_DEPTH      (10)
#define APP_NUM_CH          (1)
#define APP_NUM_ITERATIONS  (600)

#define MIN(a, b) a<b?a:b

vx_status app_modules_linux_decode_display_test(vx_int32 argc, vx_char* argv[])
{
    vx_status status = VX_FAILURE;
    GraphObj graph;
    NodeObj *node = NULL;
    TIOVXMosaicNodeCfg cfg;
    BufPool *in_buf_pool = NULL, *out_buf_pool = NULL;
    Buf *inbuf = NULL, *outbuf = NULL;
    v4l2DecodeCfg v4l2_decode_cfg;
    v4l2DecodeHandle *v4l2_decode_handle;
    v4l2DecodeOutFmt v4l2_decode_fmt;
    kmsDisplayCfg kms_display_cfg;
    kmsDisplayHandle *kms_display_handle;


    v4l2_decode_init_cfg(&v4l2_decode_cfg);
    v4l2_decode_cfg.bufq_depth = APP_BUFQ_DEPTH;
    sprintf(v4l2_decode_cfg.file, "/opt/edgeai-test-data/videos/video0_1280_768.h264");
    v4l2_decode_handle = v4l2_decode_create_handle(&v4l2_decode_cfg, &v4l2_decode_fmt);

    tiovx_mosaic_init_cfg(&cfg);

    cfg.color_format = VX_DF_IMAGE_NV12;
    cfg.num_inputs = 1;
    cfg.input_cfgs[0].width = v4l2_decode_fmt.width;
    cfg.input_cfgs[0].height = v4l2_decode_fmt.height;
    cfg.output_cfg.width = 1920;
    cfg.output_cfg.height = 1080;
    cfg.params.num_windows  = 1;
    cfg.params.windows[0].startX  = 0;
    cfg.params.windows[0].startY  = 0;
    cfg.params.windows[0].width   = MIN(1920, v4l2_decode_fmt.width);
    cfg.params.windows[0].height  = MIN(1080, v4l2_decode_fmt.height);
    cfg.params.windows[0].input_select   = 0;
    cfg.params.windows[0].channel_select = 0;
    cfg.params.clear_count  = 4;

    status = tiovx_modules_initialize_graph(&graph);
    graph.schedule_mode = VX_GRAPH_SCHEDULE_MODE_QUEUE_AUTO;
    node = tiovx_modules_add_node(&graph, TIOVX_MOSAIC, (void *)&cfg);
    node->sinks[0].bufq_depth = APP_BUFQ_DEPTH;
    status = tiovx_modules_verify_graph(&graph);

    in_buf_pool = node->sinks[0].buf_pool;
    out_buf_pool = node->srcs[0].buf_pool;

    kms_display_init_cfg(&kms_display_cfg);
    kms_display_cfg.width = 1920;
    kms_display_cfg.height = 1080;
    kms_display_cfg.pix_format = DRM_FORMAT_NV12;

    kms_display_handle = kms_display_create_handle(&kms_display_cfg);

    for (int i = 0; i < APP_BUFQ_DEPTH; i++) {
        inbuf = tiovx_modules_acquire_buf(in_buf_pool);
        v4l2_decode_enqueue_buf(v4l2_decode_handle, inbuf);
    }

    v4l2_decode_start(v4l2_decode_handle);

    for (int i = 0; i < out_buf_pool->bufq_depth; i++) {
        inbuf = v4l2_decode_dqueue_buf(v4l2_decode_handle);
        tiovx_modules_enqueue_buf(inbuf);

        outbuf = tiovx_modules_acquire_buf(out_buf_pool);
        tiovx_modules_enqueue_buf(outbuf);
        kms_display_register_buf(kms_display_handle, outbuf);
    }

    for (int i = 0; i < APP_NUM_ITERATIONS; i++) {
        inbuf = tiovx_modules_dequeue_buf(in_buf_pool);
        outbuf = tiovx_modules_dequeue_buf(out_buf_pool);

        v4l2_decode_enqueue_buf(v4l2_decode_handle, inbuf);
        kms_display_render_buf(kms_display_handle, outbuf);

        inbuf = v4l2_decode_dqueue_buf(v4l2_decode_handle);
        tiovx_modules_enqueue_buf(inbuf);
        tiovx_modules_enqueue_buf(outbuf);
    }

    v4l2_decode_stop(v4l2_decode_handle);
    v4l2_decode_delete_handle(v4l2_decode_handle);

    tiovx_modules_clean_graph(&graph);

    return status;
}
