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
#ifndef _TIOVX_APPS_OUTPUT_BLOCK
#define _TIOVX_APPS_OUTPUT_BLOCK

#include <tiovx_modules.h>
#include <kms_display_module.h>
#include <TI/tivx_img_proc.h>

#include <apps/include/info.h>
#include <apps/include/deep_learning_block.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * KMS Information
 */
typedef struct {

    /* KMS Display handle */
    kmsDisplayHandle        *kms_display_handle;

} KMSObjects;

/*
 * Output block information
 */
typedef struct {

    /* Input pad to the block */
    Pad                     *input_pads[TIVX_IMG_MOSAIC_MAX_INPUTS];
    
    /* Output pad from the block */
    Pad                     *output_pad;

    /* Background pad of mosaic. */
    Pad                     *mosaic_bg_pad;

    /* Background pad of perf overlay. */
    Pad                     *perf_overlay_pad;

    /* Number of inputs to this block. */
    uint32_t                num_inputs;

    /* OutputInfo */
    OutputInfo              *output_info;

    /* KMS Objects */
    KMSObjects              kms_obj;

    /* Mosaic Node Config */
    TIOVXMosaicNodeCfg      mosaic_cfg;

} OutputBlock;

void initialize_output_block(OutputBlock *output_block);

int32_t connect_pad_to_output_block(OutputBlock *output_block,
                                    Pad *pad,
                                    SubflowInfo *subflow_info);

int32_t create_output_block(GraphObj *graph, OutputBlock *output_block);

#ifdef __cplusplus
}
#endif

#endif