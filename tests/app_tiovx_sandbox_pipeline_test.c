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
 *
 * Input 1(640x480 S16) --> LUT ----> ADD ----> Output
 *                                    /|\
 *                                     |                                          |
 * Input 2(640x480 S16) ----------------
 */

#include <tiovx_modules.h>
#include <tiovx_utils.h>

#define APP_BUFQ_DEPTH   (1)
#define APP_NUM_CH       (1)
#define APP_NUM_ITERATIONS  (600)

#define IMAGE_WIDTH  640
#define IMAGE_HEIGHT 480

vx_status app_modules_sandbox_pipeline_test(vx_int32 argc, vx_char* argv[])
{
    vx_status status = VX_FAILURE;
    GraphObj graph;
    Pad *input_pad[2] = {NULL}, *output_pad = NULL;
    Buf* buf;
    struct timespec start, end;
    uint64_t time_nsec, fps;

    status = tiovx_modules_initialize_graph(&graph);
    graph.schedule_mode = VX_GRAPH_SCHEDULE_MODE_QUEUE_AUTO;

    {
        TIOVXLutNodeCfg lut_cfg;
        NodeObj *lut_node;
        int16_t lut[65536];

        for (int i = 0; i < 65536; i++) {
            lut[i] = i - 65536/2;
        }

        tiovx_lut_init_cfg(&lut_cfg);

        lut_cfg.img_cfg.width = IMAGE_WIDTH;
        lut_cfg.img_cfg.height = IMAGE_HEIGHT;
        lut_cfg.img_cfg.color_format = VX_DF_IMAGE_S16;
        lut_cfg.lut_size = 65536;
        lut_cfg.lut = (void *)lut;
        sprintf(lut_cfg.target_string, TIVX_TARGET_DSP1);

        lut_node = tiovx_modules_add_node(&graph, TIOVX_LUT, (void *)&lut_cfg);
        input_pad[0] = &lut_node->sinks[0];
        output_pad = &lut_node->srcs[0];
    }

    {
        TIOVXPixelwiseAddNodeCfg add_cfg;
        NodeObj *add_node;

        tiovx_pixelwise_add_init_cfg(&add_cfg);

        add_cfg.input_cfg.width = IMAGE_WIDTH;
        add_cfg.input_cfg.height = IMAGE_HEIGHT;
        add_cfg.input_cfg.color_format = VX_DF_IMAGE_S16;
        add_cfg.output_color_format = VX_DF_IMAGE_S16;
        sprintf(add_cfg.target_string, TIVX_TARGET_DSP1);

        add_node = tiovx_modules_add_node(&graph, TIOVX_PIXELWISE_ADD, (void *)&add_cfg);

        input_pad[1] = &add_node->sinks[1];
        tiovx_modules_link_pads(output_pad, &add_node->sinks[0]);
        output_pad = &add_node->srcs[0];
    }

    status = tiovx_modules_verify_graph(&graph);

    for (int i = 0; i < input_pad[0]->bufq_depth; i++) {
        buf = tiovx_modules_acquire_buf(input_pad[0]->buf_pool);
        tiovx_modules_enqueue_buf(buf);
        buf = tiovx_modules_acquire_buf(input_pad[1]->buf_pool);
        tiovx_modules_enqueue_buf(buf);
        buf = tiovx_modules_acquire_buf(output_pad->buf_pool);
        tiovx_modules_enqueue_buf(buf);
    }

    clock_gettime(CLOCK_MONOTONIC, &start);
    for (int i = 0; i < APP_NUM_ITERATIONS; i++) {
        buf = tiovx_modules_dequeue_buf(input_pad[0]->buf_pool);
        tiovx_modules_enqueue_buf(buf);
        buf = tiovx_modules_dequeue_buf(input_pad[1]->buf_pool);
        tiovx_modules_enqueue_buf(buf);
        buf = tiovx_modules_dequeue_buf(output_pad->buf_pool);
        tiovx_modules_enqueue_buf(buf);
    }
    clock_gettime(CLOCK_MONOTONIC, &end);

    for (int i = 0; i < input_pad[0]->bufq_depth; i++) {
        buf = tiovx_modules_dequeue_buf(input_pad[0]->buf_pool);
        tiovx_modules_release_buf(buf);
        buf = tiovx_modules_dequeue_buf(input_pad[1]->buf_pool);
        tiovx_modules_release_buf(buf);
        buf = tiovx_modules_dequeue_buf(output_pad->buf_pool);
        tiovx_modules_release_buf(buf);
    }

    tiovx_modules_clean_graph(&graph);

    time_nsec = (end.tv_sec - start.tv_sec)*1000000000 +
                (end.tv_nsec - start.tv_nsec);

    fps = 1000000000L * APP_NUM_ITERATIONS/time_nsec;

    printf("Avg time (ms): %lu\n", time_nsec/1000000/APP_NUM_ITERATIONS);
    printf("FPS          : %lu\n", fps);

    return status;
}
