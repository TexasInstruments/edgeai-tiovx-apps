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
#include <v4l2_capture_module.h>
#include <kms_display_module.h>
#include <linux_aewb_module.h>

#define APP_BUFQ_DEPTH      (4)
#define APP_NUM_ITERATIONS  (600)

#define INPUT_WIDTH  (1920)
#define INPUT_HEIGHT (1080)

#define DISPLAY_WIDTH  (1920)
#define DISPLAY_HEIGHT (1080)

#define WINDOW_WIDTH  (640)
#define WINDOW_HEIGHT (360)

#define SENSOR_NAME "SENSOR_SONY_IMX219_RPI"
#define DCC_VISS TIOVX_MODULES_IMAGING_PATH"/imx219/linear/dcc_viss.bin"

#define DEV "/dev/video-imx219-cam%d"
#define SUBDEV "/dev/v4l-imx219-subdev%d"

#define NUM_CAMS 2

typedef struct {
    Pad *pad;
    Pad *h3a_pad;
    Pad *aewb_pad;
    v4l2CaptureHandle *v4l2_handle;
    AewbHandle *aewb_handle;
    kmsDisplayHandle *kms_display_handle;
} threadParams;

void *viss_enqueue(void *params)
{
    threadParams *thread_params = (threadParams *)params;
    Pad *input_pad = thread_params->pad;
    Pad *h3a_pad = thread_params->h3a_pad;
    Pad *aewb_pad = thread_params->aewb_pad;
    v4l2CaptureHandle *v4l2_handle = thread_params->v4l2_handle;
    BufPool *input_buf_pool = input_pad->buf_pool;
    BufPool *h3a_buf_pool = h3a_pad->buf_pool;
    BufPool *aewb_buf_pool = aewb_pad->buf_pool;
    Buf *buf = NULL;

    for (int i = 0; i < input_pad->bufq_depth; i++) {
        buf = tiovx_modules_acquire_buf(input_buf_pool);
        v4l2_capture_enqueue_buf(v4l2_handle, buf);
    }

    v4l2_capture_start(v4l2_handle);

    for (int i = 0; i < APP_NUM_ITERATIONS; i++) {
        do {
            buf = v4l2_capture_dqueue_buf(v4l2_handle);
        } while (buf == NULL);
        tiovx_modules_enqueue_buf(buf);

        buf = tiovx_modules_acquire_buf(h3a_buf_pool);
        tiovx_modules_enqueue_buf(buf);

        buf = tiovx_modules_acquire_buf(aewb_buf_pool);
        tiovx_modules_enqueue_buf(buf);
    }

    v4l2_capture_stop(v4l2_handle);

    pthread_exit(NULL);
}

void *viss_dequeue(void *params)
{
    threadParams *thread_params = (threadParams *)params;
    Pad *input_pad = thread_params->pad;
    Pad *h3a_pad = thread_params->h3a_pad;
    Pad *aewb_pad = thread_params->aewb_pad;
    v4l2CaptureHandle *v4l2_handle = thread_params->v4l2_handle;
    AewbHandle *aewb_handle = thread_params->aewb_handle;
    BufPool *input_buf_pool = input_pad->buf_pool;
    BufPool *h3a_buf_pool = h3a_pad->buf_pool;
    BufPool *aewb_buf_pool = aewb_pad->buf_pool;
    Buf *buf = NULL, *h3a_buf = NULL, *aewb_buf = NULL;

    for (int i = 0; i < APP_NUM_ITERATIONS; i++) {
        buf = tiovx_modules_dequeue_buf(input_buf_pool);
        v4l2_capture_enqueue_buf(v4l2_handle, buf);

        h3a_buf = tiovx_modules_dequeue_buf(h3a_buf_pool);
        aewb_buf = tiovx_modules_dequeue_buf(aewb_buf_pool);

        aewb_process(aewb_handle, h3a_buf, aewb_buf);

        tiovx_modules_release_buf(h3a_buf);
        tiovx_modules_release_buf(aewb_buf);
    }

    pthread_exit(NULL);
}

void *mosaic_enqueue(void *params)
{
    threadParams *thread_params = (threadParams *)params;
    Pad *pad = thread_params->pad;
    BufPool *buf_pool = pad->buf_pool;
    Buf *buf = NULL;

    for (int i = 0; i < APP_NUM_ITERATIONS; i++) {
        buf = tiovx_modules_acquire_buf(buf_pool);
        tiovx_modules_enqueue_buf(buf);
    }

    pthread_exit(NULL);
}

void *mosaic_dequeue(void *params)
{
    threadParams *thread_params = (threadParams *)params;
    Pad *pad = thread_params->pad;
    kmsDisplayHandle *kms_display_handle = thread_params->kms_display_handle;
    BufPool *buf_pool = pad->buf_pool;
    Buf *buf = NULL;

    for (int i = 0; i < pad->bufq_depth; i++) {
        kms_display_register_buf(kms_display_handle, &buf_pool->bufs[i]);
    }

    for (int i = 0; i < APP_NUM_ITERATIONS; i++) {
        buf = tiovx_modules_dequeue_buf(buf_pool);
        kms_display_render_buf(kms_display_handle, buf);
        tiovx_modules_release_buf(buf);
    }

    pthread_exit(NULL);
}

vx_status app_modules_linux_multi_capture_display_test(vx_int32 argc, vx_char* argv[])
{
    vx_status status = VX_FAILURE;
    GraphObj graph;
    NodeObj *viss_node[NUM_CAMS], *mosaic_node;
    TIOVXVissNodeCfg viss_cfg;
    TIOVXMosaicNodeCfg mosaic_cfg;
    v4l2CaptureCfg v4l2_capture_cfg;
    v4l2CaptureHandle *v4l2_capture_handle[NUM_CAMS];
    kmsDisplayCfg kms_display_cfg;
    kmsDisplayHandle *kms_display_handle;
    AewbCfg aewb_cfg;
    AewbHandle *aewb_handle[NUM_CAMS];
    threadParams viss_thread_params[NUM_CAMS];
    threadParams mosaic_thread_params;
    pthread_t viss_enqueue_threads[NUM_CAMS], viss_dequeue_threads[NUM_CAMS];
    pthread_t mosaic_enqueue_thread, mosaic_dequeue_thread;
    uint8_t i;

    status = tiovx_modules_initialize_graph(&graph);
    graph.schedule_mode = VX_GRAPH_SCHEDULE_MODE_QUEUE_AUTO;

    tiovx_viss_init_cfg(&viss_cfg);
    sprintf(viss_cfg.sensor_name, SENSOR_NAME);
    snprintf(viss_cfg.dcc_config_file, TIVX_FILEIO_FILE_PATH_LENGTH, "%s", DCC_VISS);
    viss_cfg.width = INPUT_WIDTH;
    viss_cfg.height = INPUT_HEIGHT;
    sprintf(viss_cfg.target_string, TIVX_TARGET_VPAC_VISS1);
    viss_cfg.input_cfg.params.format[0].pixel_container = TIVX_RAW_IMAGE_8_BIT;
    viss_cfg.input_cfg.params.format[0].msb = 7;
    viss_cfg.enable_aewb_pad = vx_true_e;
    viss_cfg.enable_h3a_pad = vx_true_e;

    tiovx_mosaic_init_cfg(&mosaic_cfg);
    mosaic_cfg.color_format = VX_DF_IMAGE_NV12;
    mosaic_cfg.num_inputs = NUM_CAMS;
    mosaic_cfg.output_cfg.width = DISPLAY_WIDTH;
    mosaic_cfg.output_cfg.height = DISPLAY_HEIGHT;
    mosaic_cfg.params.num_windows  = NUM_CAMS;
    mosaic_cfg.params.clear_count  = 4;

    v4l2_capture_init_cfg(&v4l2_capture_cfg);
    v4l2_capture_cfg.width = INPUT_WIDTH;
    v4l2_capture_cfg.height = INPUT_HEIGHT;
    v4l2_capture_cfg.pix_format = V4L2_PIX_FMT_SRGGB8;
    v4l2_capture_cfg.bpp = 1;
    v4l2_capture_cfg.bufq_depth = APP_BUFQ_DEPTH;

    aewb_init_cfg(&aewb_cfg);
    sprintf(aewb_cfg.dcc_2a_file, TIOVX_MODULES_IMAGING_PATH"/imx219/linear/dcc_2a.bin");

    for (i = 0; i < NUM_CAMS; i++) {
        viss_node[i] = tiovx_modules_add_node(&graph, TIOVX_VISS, (void *)&viss_cfg);
        viss_node[i]->sinks[0].bufq_depth = APP_BUFQ_DEPTH;

        mosaic_cfg.input_cfgs[i].width = INPUT_WIDTH;
        mosaic_cfg.input_cfgs[i].height = INPUT_HEIGHT;
        mosaic_cfg.params.windows[i].startX  = (i % 3) * WINDOW_WIDTH;
        mosaic_cfg.params.windows[i].startY  = (i/3) * WINDOW_HEIGHT;
        mosaic_cfg.params.windows[i].width   = WINDOW_WIDTH;
        mosaic_cfg.params.windows[i].height  = WINDOW_HEIGHT;
        mosaic_cfg.params.windows[i].input_select   = i;
        mosaic_cfg.params.windows[i].channel_select = 0;

        sprintf(v4l2_capture_cfg.device, DEV, i);
        v4l2_capture_handle[i] = v4l2_capture_create_handle(&v4l2_capture_cfg);

        sprintf(aewb_cfg.device, SUBDEV, i);
        aewb_handle[i] = aewb_create_handle(&aewb_cfg);
    }

    mosaic_node = tiovx_modules_add_node(&graph, TIOVX_MOSAIC, (void *)&mosaic_cfg);

    for (i = 0; i < NUM_CAMS; i++) {
        tiovx_modules_link_pads(&viss_node[i]->srcs[0], &mosaic_node->sinks[i]);
    }

    kms_display_init_cfg(&kms_display_cfg);
    kms_display_cfg.width = DISPLAY_WIDTH;
    kms_display_cfg.height = DISPLAY_HEIGHT;
    kms_display_cfg.pix_format = DRM_FORMAT_NV12;
    kms_display_handle = kms_display_create_handle(&kms_display_cfg);

    status = tiovx_modules_verify_graph(&graph);

    //create and join threads
    for (i = 0; i < NUM_CAMS; i++) {
        viss_thread_params[i].pad = &viss_node[i]->sinks[0];
        viss_thread_params[i].h3a_pad = &viss_node[i]->srcs[viss_node[i]->num_outputs - 1];
        viss_thread_params[i].aewb_pad = &viss_node[i]->sinks[1];
        viss_thread_params[i].v4l2_handle = v4l2_capture_handle[i];
        viss_thread_params[i].aewb_handle = aewb_handle[i];

        pthread_create(&viss_enqueue_threads[i], NULL, viss_enqueue, (void *)&viss_thread_params[i]);
        pthread_create(&viss_dequeue_threads[i], NULL, viss_dequeue, (void *)&viss_thread_params[i]);
    }

    mosaic_thread_params.pad = &mosaic_node->srcs[0];
    mosaic_thread_params.kms_display_handle = kms_display_handle;

    pthread_create(&mosaic_enqueue_thread, NULL, mosaic_enqueue, (void *)&mosaic_thread_params);
    pthread_create(&mosaic_dequeue_thread, NULL, mosaic_dequeue, (void *)&mosaic_thread_params);

    for (i = 0; i < NUM_CAMS; i++) {
        pthread_join(viss_enqueue_threads[i], NULL);
        pthread_join(viss_dequeue_threads[i], NULL);
    }

    pthread_join(mosaic_enqueue_thread, NULL);
    pthread_join(mosaic_dequeue_thread, NULL);

    for (i = 0; i < NUM_CAMS; i++) {
        v4l2_capture_delete_handle(v4l2_capture_handle[i]);
        aewb_delete_handle(aewb_handle[i]);
    }

    kms_display_delete_handle(kms_display_handle);

    tiovx_modules_clean_graph(&graph);

    return status;
}
