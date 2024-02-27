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

#define APP_BUFQ_DEPTH   (1)
#define APP_NUM_CH       (1)
#define APP_NUM_INPUTS  (1)

#define DISPLAY_WIDTH  (1920)
#define DISPLAY_HEIGHT (1080)
#define IMAGE_WIDTH  (1280)
#define IMAGE_HEIGHT (720)

vx_status app_modules_mosaic_test(vx_int32 argc, vx_char* argv[])
{
    vx_status status = VX_FAILURE;
    GraphObj graph;
    NodeObj *node = NULL;
    TIOVXMosaicNodeCfg cfg;
    BufPool *in_buf_pool[APP_NUM_INPUTS], *out_buf_pool = NULL;
    Buf *inbuf[APP_NUM_INPUTS], *outbuf = NULL;
    char input_filename[100];
    char output_filename[100];
    int i;

    sprintf(input_filename, "%s/raw_images/modules_test/avp3_1280x720_1_nv12.yuv", EDGEAI_DATA_PATH);
    sprintf(output_filename, "%s/output/avp3_1280x720_1_output_mosaic_edgeai_modules_nv12.yuv", EDGEAI_DATA_PATH);

    status = tiovx_modules_initialize_graph(&graph);

    tiovx_mosaic_init_cfg(&cfg);

    cfg.color_format = VX_DF_IMAGE_NV12;
    cfg.num_inputs = APP_NUM_INPUTS;

    cfg.output_cfg.width = DISPLAY_WIDTH;
    cfg.output_cfg.height = DISPLAY_HEIGHT;

    for (i = 0; i < APP_NUM_INPUTS; i++)
    {
        cfg.input_cfgs[i].width = IMAGE_WIDTH;
        cfg.input_cfgs[i].height = IMAGE_HEIGHT;
    }

    cfg.params.num_windows  = APP_NUM_INPUTS;
    for (i = 0; i < APP_NUM_INPUTS; i++)
    {
        cfg.params.windows[i].startX  = (i * 100);
        cfg.params.windows[i].startY  = 200;
        cfg.params.windows[i].width   = IMAGE_WIDTH;
        cfg.params.windows[i].height  = IMAGE_HEIGHT;
        cfg.params.windows[i].input_select   = i;
        cfg.params.windows[i].channel_select = 0;
    }
    cfg.params.clear_count  = 4;

    node = tiovx_modules_add_node(&graph, TIOVX_MOSAIC, (void *)&cfg);
    status = tiovx_modules_verify_graph(&graph);
    for(i = 0; i < APP_NUM_INPUTS; i++)
    {
        in_buf_pool[i] = node->sinks[i].buf_pool;
        inbuf[i] = tiovx_modules_acquire_buf(in_buf_pool[i]);
        readImage(input_filename, (vx_image)inbuf[i]->handle);
        tiovx_modules_enqueue_buf(inbuf[i]);
    }
    out_buf_pool = node->srcs[0].buf_pool;
    outbuf = tiovx_modules_acquire_buf(out_buf_pool);
    tiovx_modules_enqueue_buf(outbuf);

    tiovx_modules_schedule_graph(&graph);
    tiovx_modules_wait_graph(&graph);

    for(i = 0; i < APP_NUM_INPUTS; i++)
    {
        inbuf[i] = tiovx_modules_dequeue_buf(in_buf_pool[i]);
    }

    outbuf = tiovx_modules_dequeue_buf(out_buf_pool);

    writeImage(output_filename, (vx_image)outbuf->handle);

    for(i = 0; i < APP_NUM_INPUTS; i++)
    {
        tiovx_modules_release_buf(inbuf[i]);
    }

    tiovx_modules_release_buf(outbuf);

    tiovx_modules_clean_graph(&graph);

    return status;
}
