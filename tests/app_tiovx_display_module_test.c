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

#define IMAGE_WIDTH  (640)
#define IMAGE_HEIGHT (480)

vx_status app_modules_display_test(vx_int32 argc, vx_char* argv[])
{
    vx_status status = VX_FAILURE;
    GraphObj graph;
    NodeObj *node = NULL;
    TIOVXDisplayNodeCfg cfg;
    BufPool *in_buf_pool = NULL;
    Buf *inbuf = NULL;
    char input_filename[100];

    sprintf(input_filename, "%s/raw_images//baboon_640x480_nv12.yuv", EDGEAI_DATA_PATH);

    tiovx_display_init_cfg(&cfg);

    cfg.width = IMAGE_WIDTH;
    cfg.height = IMAGE_HEIGHT;
    cfg.input_cfg.color_format = VX_DF_IMAGE_NV12;

    cfg.params.outWidth  = IMAGE_WIDTH;
    cfg.params.outHeight = IMAGE_HEIGHT;

    /* Rtos display has fixed resolution of 1920x1080.*/
    cfg.params.posX = (1920 - IMAGE_WIDTH)/2;
    cfg.params.posY = (1080 - IMAGE_HEIGHT)/2;

    status = tiovx_modules_initialize_graph(&graph);
    node = tiovx_modules_add_node(&graph, TIOVX_DISPLAY, (void *)&cfg);
    status = tiovx_modules_verify_graph(&graph);

    in_buf_pool = node->sinks[0].buf_pool;
    inbuf = tiovx_modules_acquire_buf(in_buf_pool);

    readImage(input_filename, (vx_image)inbuf->handle);

    tiovx_modules_enqueue_buf(inbuf);

    tiovx_modules_schedule_graph(&graph);
    tiovx_modules_wait_graph(&graph);

    inbuf = tiovx_modules_dequeue_buf(in_buf_pool);

    tiovx_modules_release_buf(inbuf);

    tiovx_modules_clean_graph(&graph);

    return status;
}
