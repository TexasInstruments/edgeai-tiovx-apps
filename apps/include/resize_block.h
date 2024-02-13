#include <tiovx_modules.h>
#include <apps/include/info.h>

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

#ifndef _TIOVX_APPS_RESIZE_BLOCK
#define _TIOVX_APPS_RESIZE_BLOCK

#include <tiovx_modules.h>

#include <apps/include/info.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Resize Block Output Group */
typedef struct {

    /* Array of all pads to connect to */
    Pad         *connected_pads[TIOVX_MODULES_MAX_NODE_OUTPUTS];

    /* Number of connected pads of this group */
    uint32_t    num_connected_pads;

    /* Array of all the exposed */
    Pad         *exposed_pads[TIOVX_MODULES_MAX_NODE_OUTPUTS];

    /* Subflowinfo for exposed pad */
    SubflowInfo *exposed_pad_info[TIOVX_MODULES_MAX_NODE_OUTPUTS];

    /* Number of exposed pads of this group */
    uint32_t    num_exposed_pads;

    /* Total total output pads of this group (num_connected_pads + num_exposed_pads) */
    uint32_t    num_total_pads;

    /* Crop Start X */
    uint32_t    crop_start_x;

    /* Crop Start Y */
    uint32_t    crop_start_y;

    /* Ouput Width */
    uint32_t    output_width;

    /* Output Height*/
    uint32_t    output_height;   

} ResizeBlockOutputGroup;

/*
 * Resize block information
 */
typedef struct {
    /* Input pad to the block */
    Pad                     *input_pad;

    /* Input Width */
    uint32_t                input_width;

    /* Input Width */
    uint32_t                input_height;

    /* Number of channels */
    uint32_t                num_channels;

    /* Output groups from the block */
    ResizeBlockOutputGroup  output_group[4];

    /* Number of output groups */
    uint32_t                total_output_group;

} ResizeBlock;

void initialize_resize_block(ResizeBlock *resize_block);

int32_t add_resize_block_output_group(ResizeBlock *resize_block,
                                      uint32_t width,
                                      uint32_t height,
                                      uint32_t crop_start_x,
                                      uint32_t crop_start_y,
                                      uint32_t num_output);

int32_t get_resize_block_output_group_index(ResizeBlock *resize_block,
                                            uint32_t width,
                                            uint32_t height,
                                            uint32_t crop_start_x,
                                            uint32_t crop_start_y);

int32_t connect_pad_to_resize_block_output_group(ResizeBlock *resize_block,
                                                 uint32_t group_num,
                                                 Pad *pad);

int32_t create_resize_block_output_group_exposed_pad(ResizeBlock *resize_block,
                                                     uint32_t group_num,
                                                     SubflowInfo *subflow_info);

int32_t create_resize_block(GraphObj *graph, ResizeBlock *resize_block);

#ifdef __cplusplus
}
#endif

#endif