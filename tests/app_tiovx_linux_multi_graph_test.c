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

/*
 * Pipeline:
 *                 ________      ________
 *                |        |    |        |
 * V4L2 Decode ---|-> MSC -|--->| MOSAIC |---> KMS Display
 *                |________|    |________|
 *                 Graph 1        Graph 2
 */

#include <tiovx_modules.h>
#include <tiovx_utils.h>
#include <v4l2_decode_module.h>
#include <kms_display_module.h>

#define APP_BUFQ_DEPTH      (10)
#define APP_NUM_CH          (1)
#define APP_NUM_ITERATIONS  (600)
#define MSC_OUT_WIDTH       (640)
#define MSC_OUT_HEIGHT      (480)

#define MIN(a, b) a<b?a:b

vx_status app_modules_linux_multi_graph_test(vx_int32 argc, vx_char* argv[])
{
    vx_status status = VX_FAILURE;
    GraphObj graph1, graph2;
    NodeObj *node1, *node2 = NULL;
    TIOVXMosaicNodeCfg mosaic_cfg;
    TIOVXMultiScalerNodeCfg msc_cfg;
    BufPool *in_buf_pool1 = NULL, *out_buf_pool1 = NULL;
    BufPool *in_buf_pool2 = NULL, *out_buf_pool2 = NULL;
    Buf *inbuf1 = NULL, *outbuf1 = NULL;
    Buf *inbuf2 = NULL, *outbuf2 = NULL;
    v4l2DecodeCfg v4l2_decode_cfg;
    v4l2DecodeHandle *v4l2_decode_handle;
    v4l2DecodeOutFmt v4l2_decode_fmt;
    kmsDisplayCfg kms_display_cfg;
    kmsDisplayHandle *kms_display_handle;

    // Decode handle
    v4l2_decode_init_cfg(&v4l2_decode_cfg);
    v4l2_decode_cfg.bufq_depth = APP_BUFQ_DEPTH;
    sprintf(v4l2_decode_cfg.file, "%s/videos/video0_1280_768.h264", EDGEAI_DATA_PATH);
    v4l2_decode_handle = v4l2_decode_create_handle(&v4l2_decode_cfg, &v4l2_decode_fmt);

    // Display handle
    kms_display_init_cfg(&kms_display_cfg);
    kms_display_cfg.width = 1920;
    kms_display_cfg.height = 1080;
    kms_display_cfg.pix_format = DRM_FORMAT_NV12;
    kms_display_handle = kms_display_create_handle(&kms_display_cfg);

    // MSC config
    tiovx_multi_scaler_init_cfg(&msc_cfg);
    msc_cfg.color_format = VX_DF_IMAGE_NV12;
    msc_cfg.num_outputs = 1;
    msc_cfg.input_cfg.width = v4l2_decode_fmt.width;
    msc_cfg.input_cfg.height = v4l2_decode_fmt.height;
    msc_cfg.output_cfgs[0].width = MSC_OUT_WIDTH;
    msc_cfg.output_cfgs[0].height = MSC_OUT_HEIGHT;
    sprintf(msc_cfg.target_string, TIVX_TARGET_VPAC_MSC1);
    tiovx_multi_scaler_module_crop_params_init(&msc_cfg);
    msc_cfg.crop_params[0].crop_start_x = 0;
    msc_cfg.crop_params[0].crop_start_y = 0;
    msc_cfg.crop_params[0].crop_width = 960;
    msc_cfg.crop_params[0].crop_height = 540;

    status = tiovx_modules_initialize_graph(&graph1);
    graph1.schedule_mode = VX_GRAPH_SCHEDULE_MODE_QUEUE_AUTO;
    node1 = tiovx_modules_add_node(&graph1, TIOVX_MULTI_SCALER, (void *)&msc_cfg);
    node1->sinks[0].bufq_depth = APP_BUFQ_DEPTH;
    status = tiovx_modules_verify_graph(&graph1);

    in_buf_pool1 = node1->sinks[0].buf_pool;
    out_buf_pool1 = node1->srcs[0].buf_pool;

    // Mosaic config
    tiovx_mosaic_init_cfg(&mosaic_cfg);
    mosaic_cfg.color_format = VX_DF_IMAGE_NV12;
    mosaic_cfg.num_inputs = 1;
    mosaic_cfg.input_cfgs[0].width = MSC_OUT_WIDTH;
    mosaic_cfg.input_cfgs[0].height = MSC_OUT_HEIGHT;
    mosaic_cfg.output_cfg.width = 1920;
    mosaic_cfg.output_cfg.height = 1080;
    mosaic_cfg.params.num_windows  = 1;
    mosaic_cfg.params.windows[0].startX  = 0;
    mosaic_cfg.params.windows[0].startY  = 0;
    mosaic_cfg.params.windows[0].width   = MIN(1920, v4l2_decode_fmt.width);
    mosaic_cfg.params.windows[0].height  = MIN(1080, v4l2_decode_fmt.height);
    mosaic_cfg.params.windows[0].input_select   = 0;
    mosaic_cfg.params.windows[0].channel_select = 0;
    mosaic_cfg.params.clear_count  = 4;

    status = tiovx_modules_initialize_graph(&graph2);
    graph2.schedule_mode = VX_GRAPH_SCHEDULE_MODE_QUEUE_AUTO;
    node2 = tiovx_modules_add_node(&graph2, TIOVX_MOSAIC, (void *)&mosaic_cfg);
    status = tiovx_modules_verify_graph(&graph2);

    in_buf_pool2 = node2->sinks[0].buf_pool;
    out_buf_pool2 = node2->srcs[0].buf_pool;

    // Start
    for (int i = 0; i < APP_BUFQ_DEPTH; i++) {
        inbuf1 = tiovx_modules_acquire_buf(in_buf_pool1);
        v4l2_decode_enqueue_buf(v4l2_decode_handle, inbuf1);
    }

    for (int i = 0; i < out_buf_pool2->bufq_depth; i++) {
        kms_display_register_buf(kms_display_handle, &out_buf_pool2->bufs[i]);
    }

    v4l2_decode_start(v4l2_decode_handle);

    /*
     * Note that in below loop graph on and 2 are running sequentially
     * to run them in parellel, you need to decuple them by creating
     * seperate threads for enqueue/dequeue of each graph
     */
    for (int i = 0; i < APP_NUM_ITERATIONS; i++) {
        //Graph1 Execution
        inbuf1 = v4l2_decode_dqueue_buf(v4l2_decode_handle);
        outbuf1 = tiovx_modules_acquire_buf(out_buf_pool1);
        tiovx_modules_enqueue_buf(inbuf1);
        tiovx_modules_enqueue_buf(outbuf1);
        inbuf1 = tiovx_modules_dequeue_buf(in_buf_pool1);
        outbuf1 = tiovx_modules_dequeue_buf(out_buf_pool1);
        v4l2_decode_enqueue_buf(v4l2_decode_handle, inbuf1);

        //Graph2 Execution
        inbuf2 = tiovx_modules_acquire_buf(in_buf_pool2);
        outbuf2 = tiovx_modules_acquire_buf(out_buf_pool2);
        //Swap input2 mem with output1 to feed graph1 out to graph2
        tiovx_modules_buf_swap_mem(outbuf1, inbuf2);
        tiovx_modules_release_buf(outbuf1);
        tiovx_modules_enqueue_buf(inbuf2);
        tiovx_modules_enqueue_buf(outbuf2);
        inbuf2 = tiovx_modules_dequeue_buf(in_buf_pool2);
        outbuf2 = tiovx_modules_dequeue_buf(out_buf_pool2);
        kms_display_render_buf(kms_display_handle, outbuf2);
        tiovx_modules_release_buf(inbuf2);
        tiovx_modules_release_buf(outbuf2);
    }

    v4l2_decode_stop(v4l2_decode_handle);
    v4l2_decode_delete_handle(v4l2_decode_handle);
    kms_display_delete_handle(kms_display_handle);

    tiovx_modules_clean_graph(&graph1);
    tiovx_modules_clean_graph(&graph2);

    return status;
}
