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
#ifndef _TIOVX_MODULES_CBS
#define _TIOVX_MODULES_CBS

#include "tiovx_multi_scaler_module.h"
#include "tiovx_dl_color_convert_module.h"
#include "tiovx_color_convert_module.h"
#include "tiovx_viss_module.h"
#include "tiovx_ldc_module.h"
#include "tiovx_tee_module.h"
#include "tiovx_tidl_module.h"
#include "tiovx_dl_pre_proc_module.h"
#include "tiovx_dl_post_proc_module.h"
#include "tiovx_mosaic_module.h"
#include "tiovx_obj_array_split_module.h"
#include "tiovx_pyramid_module.h"
#include "tiovx_delay_module.h"
#include "tiovx_fakesink_module.h"
#include "tiovx_fakesrc_module.h"
#include "tiovx_pixelwise_multiply_module.h"
#include "tiovx_pixelwise_add_module.h"
#include "tiovx_lut_module.h"

#if !defined(SOC_AM62A)
#include "tiovx_display_module.h"
#endif
#if (defined(SOC_AM62A) && defined(TARGET_OS_QNX)) || !defined(SOC_AM62A)
#include "tiovx_capture_module.h"
#include "tiovx_aewb_module.h"
#endif
#if !defined(SOC_AM62A)
#include "tiovx_sde_module.h"
#include "tiovx_sde_viz_module.h"
#include "tiovx_dof_module.h"
#include "tiovx_dof_viz_module.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif

/*!
 * \brief Enum that lists all available modules.
 */
typedef enum {
    TIOVX_MULTI_SCALER = 0,
    TIOVX_DL_COLOR_CONVERT,
    TIOVX_COLOR_CONVERT,
    TIOVX_VISS,
    TIOVX_LDC,
    TIOVX_TEE,
    TIOVX_TIDL,
    TIOVX_DL_PRE_PROC,
    TIOVX_DL_POST_PROC,
    TIOVX_MOSAIC,
    TIOVX_OBJ_ARRAY_SPLIT,
    TIOVX_PYRAMID,
    TIOVX_DELAY,
    TIOVX_FAKESINK,
    TIOVX_FAKESRC,
    TIOVX_PIXELWISE_MULTIPLY,
    TIOVX_PIXELWISE_ADD,
    TIOVX_LUT,
#if !defined(SOC_AM62A)
    TIOVX_DISPLAY,
#endif
#if (defined(SOC_AM62A) && defined(TARGET_OS_QNX)) || !defined(SOC_AM62A)
    TIOVX_CAPTURE,
    TIOVX_AEWB,
#endif
#if !defined(SOC_AM62A)
    TIOVX_SDE,
    TIOVX_SDE_VIZ,
    TIOVX_DOF,
    TIOVX_DOF_VIZ,
#endif
    TIOVX_MODULES_NUM_MODULES,
} NODE_TYPES;

#ifdef __cplusplus
}
#endif

#endif //_TIOVX_MODULES_CBS
