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
#ifndef _TIOVX_VISS_MODULE
#define _TIOVX_VISS_MODULE

#include "tiovx_modules_types.h"
#include <tiovx_sensor_module.h>

#define TIOVX_VISS_MODULE_MAX_OUTPUTS (5)

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    vx_int32                    width;
    vx_int32                    height;
    RawImgCfg                   input_cfg;
    ImgCfg                      output_cfgs[TIOVX_VISS_MODULE_MAX_OUTPUTS];
    vx_int32                    output_select[TIOVX_VISS_MODULE_MAX_OUTPUTS];
    tivx_vpac_viss_params_t     viss_params;
    vx_char                     dcc_config_file[TIVX_FILEIO_FILE_PATH_LENGTH];
    char                        target_string[TIVX_TARGET_MAX_NAME];
    vx_int32                    num_channels;
    char                        sensor_name[ISS_SENSORS_MAX_NAME];
    vx_bool                     enable_h3a_pad;
    vx_bool                     enable_aewb_pad;
    vx_bool                     enable_histogram_pad;
} TIOVXVissNodeCfg;

void tiovx_viss_init_cfg(TIOVXVissNodeCfg *cfg);
vx_status tiovx_viss_init_node(NodeObj *node);
vx_status tiovx_viss_create_node(NodeObj *node);
vx_status tiovx_viss_delete_node(NodeObj *node);
vx_uint32 tiovx_viss_get_cfg_size();
vx_uint32 tiovx_viss_get_priv_size();

#ifdef __cplusplus
}
#endif

#endif
