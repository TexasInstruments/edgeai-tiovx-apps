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

#include <apps/include/output_block.h>

void initialize_output_block(OutputBlock *output_block)
{
    for(uint32_t i = 0; i < TIVX_IMG_MOSAIC_MAX_INPUTS; i++)
    {
        output_block->input_pads[i] = NULL;
    }

    output_block->num_inputs = 0;
    output_block->output_pad = NULL;
    output_block->mosaic_bg_pad = NULL;

    tiovx_mosaic_init_cfg(&output_block->mosaic_cfg);
    output_block->mosaic_cfg.num_inputs = 0;
    output_block->mosaic_cfg.params.num_windows = 0;
}

int32_t connect_pad_to_output_block(OutputBlock *output_block,
                                    Pad *pad,
                                    SubflowInfo *subflow_info)
{
    TIOVXMosaicNodeCfg *mosaic_cfg;
    uint32_t prev_window_index, new_window_index;
    uint32_t i = 0, cnt = 0;

    mosaic_cfg = &output_block->mosaic_cfg;

    mosaic_cfg->input_cfgs[mosaic_cfg->num_inputs].width = subflow_info->max_mosaic_width;
    mosaic_cfg->input_cfgs[mosaic_cfg->num_inputs].height = subflow_info->max_mosaic_height;
    mosaic_cfg->num_channels[mosaic_cfg->num_inputs] = subflow_info->num_channels;

    prev_window_index = mosaic_cfg->params.num_windows;
    mosaic_cfg->params.num_windows += subflow_info->num_channels;
    new_window_index = mosaic_cfg->params.num_windows;

    for (i = prev_window_index; i < new_window_index; i++)
    {
        mosaic_cfg->params.windows[i].startX  = subflow_info->mosaic_info[cnt].pos_x;
        mosaic_cfg->params.windows[i].startY  = subflow_info->mosaic_info[cnt].pos_y;
        mosaic_cfg->params.windows[i].width   = subflow_info->mosaic_info[cnt].width;
        mosaic_cfg->params.windows[i].height  = subflow_info->mosaic_info[cnt].height;
        mosaic_cfg->params.windows[i].input_select   = mosaic_cfg->num_inputs;
        mosaic_cfg->params.windows[i].channel_select = cnt;
        cnt++;
    }

    output_block->input_pads[output_block->num_inputs] = pad;

    mosaic_cfg->num_inputs++;
    output_block->num_inputs++;

    return 0;
}

int32_t create_output_block(GraphObj *graph, OutputBlock *output_block)
{
    int32_t status = 0;
    int32_t i;
    OutputInfo *output_info;
    Pad *output_pad = NULL;

    if(0 == output_block->num_inputs)
    {
        TIOVX_APPS_ERROR("Number of inputs in output block cannot be 0\n");
        return -1;
    }
    if(NULL == output_block->output_info)
    {
        TIOVX_APPS_ERROR("Output info of output block is null\n");
        return -1;
    }

    output_info = output_block->output_info;

    /* Mosaic. */
    {
        TIOVXMosaicNodeCfg mosaic_cfg;
        NodeObj *mosaic_node = NULL;

        tiovx_mosaic_init_cfg(&mosaic_cfg);

        mosaic_cfg = output_block->mosaic_cfg;

        mosaic_cfg.output_cfg.width = output_info->width;
        mosaic_cfg.output_cfg.height = output_info->height;

        mosaic_cfg.params.clear_count  = 4;

        mosaic_node = tiovx_modules_add_node(graph,
                                             TIOVX_MOSAIC,
                                             (void *)&mosaic_cfg);

        /* Link Input pads to Mosaic */
        for (i = 0; i < mosaic_cfg.num_inputs; i++)
        {
            tiovx_modules_link_pads(output_block->input_pads[i], &mosaic_node->sinks[i]);
        }
        
        output_block->mosaic_bg_pad = &mosaic_node->sinks[mosaic_cfg.num_inputs];
        output_pad = &mosaic_node->srcs[0];
    }

#if !defined(SOC_AM62A)
    /* RTOS_DISPLAY */
    if(output_info->sink == RTOS_DISPLAY)
    {
        TIOVXDisplayNodeCfg display_cfg;
        NodeObj *display_node = NULL;

        tiovx_display_init_cfg(&display_cfg);

        display_cfg.width = output_info->width;
        display_cfg.height = output_info->height;
        display_cfg.params.outWidth  = output_info->width;
        display_cfg.params.outHeight = output_info->height;
        display_cfg.params.posX = (1920 - output_info->width)/2;
        display_cfg.params.posY = (1080 - output_info->height)/2;

        display_node = tiovx_modules_add_node(graph,
                                              TIOVX_DISPLAY,
                                              (void *)&display_cfg);

        /* Link Mosaic to Display */
        tiovx_modules_link_pads(output_pad, &display_node->sinks[0]);

        output_pad = NULL;
    }
#endif

    if(output_info->sink == LINUX_DISPLAY)
    {
        kmsDisplayCfg kms_display_cfg;

        kms_display_init_cfg(&kms_display_cfg);

        kms_display_cfg.width = output_info->width;
        kms_display_cfg.height = output_info->height;
        kms_display_cfg.pix_format  = DRM_FORMAT_NV12;
        if(0 != output_info->crtc)
        {
            kms_display_cfg.crtc = output_info->crtc;
        }
        if(0 != output_info->connector)
        {
            kms_display_cfg.connector = output_info->connector;
        }

        output_block->kms_obj.kms_display_handle = kms_display_create_handle(&kms_display_cfg);

    }

    output_block->output_pad = output_pad;

    return status;
}