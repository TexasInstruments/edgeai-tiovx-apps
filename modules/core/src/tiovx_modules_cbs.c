/*
 *
 * Copyright (c) 2021 Texas Instruments Incorporated
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
#include "tiovx_modules.h"

NodeCbs gNodeCbs[TIOVX_MODULES_NUM_MODULES] =
{
    {
        .init_node = tiovx_multi_scaler_init_node,
        .create_node = tiovx_multi_scaler_create_node,
        .post_verify_graph = tiovx_multi_scaler_post_verify_graph,
        .delete_node = tiovx_multi_scaler_delete_node,
        .get_cfg_size = tiovx_multi_scaler_get_cfg_size,
        .get_priv_size = tiovx_multi_scaler_get_priv_size
    },
    {
        .init_node = tiovx_dl_color_convert_init_node,
        .create_node = tiovx_dl_color_convert_create_node,
        .post_verify_graph = NULL,
        .delete_node = tiovx_modules_release_node,
        .get_cfg_size = tiovx_dl_color_convert_get_cfg_size,
        .get_priv_size = NULL
    },
    {
        .init_node = tiovx_color_convert_init_node,
        .create_node = tiovx_color_convert_create_node,
        .post_verify_graph = NULL,
        .delete_node = tiovx_modules_release_node,
        .get_cfg_size = tiovx_color_convert_get_cfg_size,
        .get_priv_size = NULL
    },
    {
        .init_node = tiovx_viss_init_node,
        .create_node = tiovx_viss_create_node,
        .post_verify_graph = NULL,
        .delete_node = tiovx_viss_delete_node,
        .get_cfg_size = tiovx_viss_get_cfg_size,
        .get_priv_size = tiovx_viss_get_priv_size
    },
    {
        .init_node = tiovx_ldc_init_node,
        .create_node = tiovx_ldc_create_node,
        .post_verify_graph = NULL,
        .delete_node = tiovx_ldc_delete_node,
        .get_cfg_size = tiovx_ldc_get_cfg_size,
        .get_priv_size = tiovx_ldc_get_priv_size
    },
    {
        .init_node = tiovx_tee_init_node,
        .create_node = tiovx_tee_create_node,
        .post_verify_graph = NULL,
        .delete_node = tiovx_tee_delete_node,
        .get_cfg_size = tiovx_tee_get_cfg_size,
        .get_priv_size = NULL
    },
    {
        .init_node = tiovx_tidl_init_node,
        .create_node = tiovx_tidl_create_node,
        .post_verify_graph = NULL,
        .delete_node = tiovx_tidl_delete_node,
        .get_cfg_size = tiovx_tidl_get_cfg_size,
        .get_priv_size = tiovx_tidl_get_priv_size
    },
    {
        .init_node = tiovx_dl_pre_proc_init_node,
        .create_node = tiovx_dl_pre_proc_create_node,
        .post_verify_graph = NULL,
        .delete_node = tiovx_dl_pre_proc_delete_node,
        .get_cfg_size = tiovx_dl_pre_proc_get_cfg_size,
        .get_priv_size = tiovx_dl_pre_proc_get_priv_size
    },
    {
        .init_node = tiovx_dl_post_proc_init_node,
        .create_node = tiovx_dl_post_proc_create_node,
        .post_verify_graph = NULL,
        .delete_node = tiovx_dl_post_proc_delete_node,
        .get_cfg_size = tiovx_dl_post_proc_get_cfg_size,
        .get_priv_size = tiovx_dl_post_proc_get_priv_size
    },
    {
        .init_node = tiovx_mosaic_init_node,
        .create_node = tiovx_mosaic_create_node,
        .post_verify_graph = NULL,
        .delete_node = tiovx_mosaic_delete_node,
        .get_cfg_size = tiovx_mosaic_get_cfg_size,
        .get_priv_size = tiovx_mosaic_get_priv_size
    }
};
