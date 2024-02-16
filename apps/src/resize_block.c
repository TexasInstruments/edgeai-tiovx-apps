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

#include <apps/include/resize_block.h>

#define MAX(a,b) ((a) > (b) ? (a) : (b))

#if defined(SOC_J784S4)
static char *g_msc_targets[] = {TIVX_TARGET_VPAC_MSC1, TIVX_TARGET_VPAC_MSC2,
                              TIVX_TARGET_VPAC2_MSC1, TIVX_TARGET_VPAC2_MSC2};
#else
static char *g_msc_targets[] = {TIVX_TARGET_VPAC_MSC1, TIVX_TARGET_VPAC_MSC2};
#endif

static uint8_t g_msc_target_idx = 0;

void initialize_resize_block(ResizeBlock *resize_block)
{
    resize_block->input_pad = NULL;
    resize_block->input_width = 0;
    resize_block->input_height = 0;
    resize_block->total_output_group = 0;
    resize_block->num_channels = 1;
    for (uint32_t i = 0; i < 4; i++)
    {
        resize_block->output_group[i].num_total_pads = 0;
        resize_block->output_group[i].num_connected_pads = 0;
        resize_block->output_group[i].num_exposed_pads = 0;
        resize_block->output_group[i].output_width = 0;
        resize_block->output_group[i].output_height = 0;
        resize_block->output_group[i].crop_start_x = 0;
        resize_block->output_group[i].crop_start_y = 0;
        for (uint32_t j = 0; j < TIOVX_MODULES_MAX_NODE_OUTPUTS; j++)
        {
            resize_block->output_group[i].connected_pads[j] = NULL;
            resize_block->output_group[i].exposed_pads[j] = NULL;
        }
    }
}

int32_t add_resize_block_output_group(ResizeBlock *resize_block,
                                      uint32_t width,
                                      uint32_t height,
                                      uint32_t crop_start_x,
                                      uint32_t crop_start_y,
                                      uint32_t num_output)
{
    uint32_t idx;

    if(resize_block->total_output_group >= 4)
    {
        TIOVX_APPS_ERROR("Resize block already has 4 output groups\n");
        return -1;
    }

    idx = resize_block->total_output_group;
    resize_block->output_group[idx].output_width = width;
    resize_block->output_group[idx].output_height = height;
    resize_block->output_group[idx].crop_start_x = crop_start_x;
    resize_block->output_group[idx].crop_start_y = crop_start_y;
    resize_block->output_group[idx].num_total_pads = num_output;
    resize_block->total_output_group++;
    return 0;
}

int32_t get_resize_block_output_group_index(ResizeBlock *resize_block,
                                            uint32_t width,
                                            uint32_t height,
                                            uint32_t crop_start_x,
                                            uint32_t crop_start_y)
{
    uint32_t i;
    for (i = 0; i < resize_block->total_output_group; i++)
    {
        if(resize_block->output_group[i].output_width == width &&
           resize_block->output_group[i].output_height == height &&
           resize_block->output_group[i].crop_start_x == crop_start_x &&
           resize_block->output_group[i].crop_start_y == crop_start_y)
        {
            return i;
        }
    }
    
    return -1;
}

int32_t connect_pad_to_resize_block_output_group(ResizeBlock *resize_block,
                                                 uint32_t group_num,
                                                 Pad *pad)
{
    uint32_t idx;

    if(group_num >= resize_block->total_output_group)
    {
        TIOVX_APPS_ERROR("Invalid output group number specified for linking pad\n");
        return -1;
    }

    idx = resize_block->output_group[group_num].num_connected_pads;

    resize_block->output_group[group_num].connected_pads[idx] = pad;
    
    resize_block->output_group[group_num].num_connected_pads++;
    
    return 0;
}

int32_t create_resize_block_output_group_exposed_pad(ResizeBlock *resize_block,
                                                     uint32_t group_num,
                                                     SubflowInfo *subflow_info)
{
    uint32_t idx;
    if(group_num >= resize_block->total_output_group)
    {
        TIOVX_APPS_ERROR("Invalid output group number specified for linking pad\n");
        return -1;
    }
    
    idx = resize_block->output_group[group_num].num_exposed_pads;
    resize_block->output_group[group_num].exposed_pad_info[idx] = subflow_info;
    resize_block->output_group[group_num].num_exposed_pads++;
    
    return 0;
}

int32_t create_resize_block(GraphObj *graph, ResizeBlock *resize_block)
{
    int32_t status = 0;
    uint32_t i, j;
    uint32_t sec_msc_target_idx;
    uint32_t input_width, input_height;
    uint32_t output_width, output_height;
    uint32_t crop_start_x, crop_start_y;
    uint32_t crop_width, crop_height;
    Pad *input_pads[4] = {NULL, NULL, NULL, NULL};
    Pad *output_pads[4] = {NULL, NULL, NULL, NULL};

    input_width = resize_block->input_width;
    input_height = resize_block->input_height;

    if(0 == input_width || 0 == input_height)
    {
        TIOVX_APPS_ERROR("Input dims for resize block is invalid\n");
        return -1;
    }
    if(0 == resize_block->total_output_group)
    {
        TIOVX_APPS_ERROR("Number of output groups for resize block is 0\n");
        return -1;
    }
    for (i = 0; i < resize_block->total_output_group; i++)
    {
        output_width = resize_block->output_group[i].output_width;
        output_height = resize_block->output_group[i].output_height;
    
        if(0 == output_width || 0 == output_height)
        {
            TIOVX_APPS_ERROR("Output dims of resize block is invalid\n");
            return -1;
        }
        if(input_width < output_width || input_height < output_height)
        {
            TIOVX_APPS_ERROR("Output dims cannot be greater than input dims for resize block\n");
            return -1;
        }
        if(resize_block->output_group[i].crop_start_x > input_width / 2 ||
           resize_block->output_group[i].crop_start_y > input_height / 2)
        {
            TIOVX_APPS_ERROR("Crop start position should be less than input_dims/2\n");
            return -1;
        }
    }

    /* Primary MSC */
    {
        TIOVXMultiScalerNodeCfg msc_cfg;
        NodeObj *msc_node;

        tiovx_multi_scaler_init_cfg(&msc_cfg);
        msc_cfg.num_channels = resize_block->num_channels;
        msc_cfg.num_outputs = resize_block->total_output_group;
        msc_cfg.input_cfg.width = resize_block->input_width;
        msc_cfg.input_cfg.height = resize_block->input_height;
        sprintf(msc_cfg.target_string, g_msc_targets[g_msc_target_idx]);

        sec_msc_target_idx = (sizeof(g_msc_targets)/sizeof(g_msc_targets[0])) - 1 - g_msc_target_idx;

        tiovx_multi_scaler_module_crop_params_init(&msc_cfg);

        for (i = 0; i < resize_block->total_output_group; i++)
        {
            /* Center Crop */
            crop_start_x = resize_block->output_group[i].crop_start_x;
            crop_start_y = resize_block->output_group[i].crop_start_y;
            crop_width = resize_block->input_width - (2 * crop_start_x);
            crop_height = resize_block->input_height - (2 * crop_start_y);
            
            msc_cfg.crop_params[i].crop_start_x = crop_start_x;
            msc_cfg.crop_params[i].crop_start_y = crop_start_y;
            msc_cfg.crop_params[i].crop_width = crop_width;
            msc_cfg.crop_params[i].crop_height = crop_height;
        
            input_width = crop_width;
            input_height = crop_height;
            output_width = resize_block->output_group[i].output_width;
            output_height = resize_block->output_group[i].output_height;

            if(input_width < output_width || input_height < output_height)
            {
                TIOVX_APPS_ERROR("Crop dims cannot be less than output dims\n");
                return -1;
            }

            else if((input_width / 16) > output_width || (input_height / 16) > output_height)
            {
                TIOVX_APPS_ERROR("resize dims cannot be less than input dims/16\n");
                return -1;
            }

            else if((input_width / 4) > output_width || (input_height / 4) > output_height)
            {
                /* Make secondary MSC */
                TIOVXMultiScalerNodeCfg sec_msc_cfg;
                NodeObj *sec_msc_node;

                tiovx_multi_scaler_init_cfg(&sec_msc_cfg);

                sec_msc_cfg.num_channels = resize_block->num_channels;

                sec_msc_cfg.num_outputs = 1;

                sec_msc_cfg.input_cfg.width = MAX(input_width/4, output_width);
                sec_msc_cfg.input_cfg.height = MAX(input_height/4, output_height);

                sec_msc_cfg.output_cfgs[0].width = output_width;
                sec_msc_cfg.output_cfgs[0].height = output_height;

                sprintf(sec_msc_cfg.target_string, g_msc_targets[sec_msc_target_idx]);

                tiovx_multi_scaler_module_crop_params_init(&sec_msc_cfg);

                sec_msc_node = tiovx_modules_add_node(graph,
                                                      TIOVX_MULTI_SCALER,
                                                      (void *)&sec_msc_cfg);

                input_pads[i] = &sec_msc_node->sinks[0];
                output_pads[i] = &sec_msc_node->srcs[0];

                msc_cfg.output_cfgs[i].width = sec_msc_cfg.input_cfg.width;
                msc_cfg.output_cfgs[i].height = sec_msc_cfg.input_cfg.height;
            }
            else
            {
                msc_cfg.output_cfgs[i].width = output_width;
                msc_cfg.output_cfgs[i].height = output_height;
            }

        }

        /* Create Primary MSC Node */
        msc_node = tiovx_modules_add_node(graph,
                                          TIOVX_MULTI_SCALER,
                                          (void *)&msc_cfg);

        for (i = 0; i < resize_block->total_output_group; i++)
        {
            /* Link Sec MSC to Primary MSC if needed */
            if (NULL != input_pads[i])
            {
                tiovx_modules_link_pads(&msc_node->srcs[i], input_pads[i]);
            }
            else
            {
                output_pads[i] = &msc_node->srcs[i];
            }
        }

        resize_block->input_pad = &msc_node->sinks[0];

        /* Set target for next MSC in round robin fashion */
        g_msc_target_idx++;
        if(g_msc_target_idx >= sizeof(g_msc_targets)/sizeof(g_msc_targets[0]))
        {
            g_msc_target_idx = 0;
        }
    }

    /* TEE */
    {
        for (i = 0; i < resize_block->total_output_group; i++)
        {
            uint32_t tee_pad_idx = 0;

            TIOVXTeeNodeCfg tee_cfg;
            NodeObj *tee_node;

            if(resize_block->output_group[i].num_total_pads !=
               resize_block->output_group[i].num_connected_pads +
               resize_block->output_group[i].num_exposed_pads)
            {
                TIOVX_APPS_ERROR("connected + exposed pads does not match the"
                       "total pads defined for resize block\n");
                return -1;
            }

            tiovx_tee_init_cfg(&tee_cfg);

            tee_cfg.peer_pad = output_pads[i];
            tee_cfg.num_outputs = resize_block->output_group[i].num_total_pads;
            tee_node = tiovx_modules_add_node(graph,
                                              TIOVX_TEE,
                                              (void *)&tee_cfg);
            for (j = 0; j < resize_block->output_group[i].num_connected_pads; j++)
            {
                tiovx_modules_link_pads(&tee_node->srcs[tee_pad_idx],
                                        resize_block->output_group[i].connected_pads[j]);
                tee_pad_idx++;
            }

            for (j = 0; j < resize_block->output_group[i].num_exposed_pads; j++)
            {
                resize_block->output_group[i].exposed_pads[j] = &tee_node->srcs[tee_pad_idx];
                tee_pad_idx++;
            }
        }
    }

    return status;
}