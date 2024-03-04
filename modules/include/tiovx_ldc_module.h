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
#ifndef _TIOVX_LDC_MODULE
#define _TIOVX_LDC_MODULE

#include "tiovx_modules_types.h"
#include <tiovx_sensor_module.h>
#include <TI/hwa_vpac_ldc.h>

#define TIOVX_LDC_MODULE_MAX_OUTPUTS (2)

#ifdef __cplusplus
extern "C" {
#endif

/** \brief LDC Mode enumeration
 *
 * Contains different enumeration option for LDC operation
 *
 * 0 - All details are taken from DCC data, no need to provide mesh image,
 *     warp matrix, region params etc. (default)
 * 1 - No DCC data available user to provide all details pertaining to warp matrix,
 *     mesh image, region params etc.
 * 2 - Max enumeration value
 *
 */
typedef enum {
    TIOVX_MODULE_LDC_OP_MODE_DCC_DATA = 0,
    TIOVX_MODULE_LDC_OP_MODE_MESH_IMAGE,
    TIOVX_MODULE_LDC_OP_MODE_MAX,
    TIOVX_MODULE_LDC_OP_MODE_DEFAULT = TIOVX_MODULE_LDC_OP_MODE_DCC_DATA
}eLDCMode;

typedef struct {
    ImgCfg                        input_cfg;
    ImgCfg                        output_cfgs[TIOVX_LDC_MODULE_MAX_OUTPUTS];
    vx_int32                      output_select[TIOVX_LDC_MODULE_MAX_OUTPUTS];
    eLDCMode                      ldc_mode;
    tivx_vpac_ldc_params_t        ldc_params;
    tivx_vpac_ldc_mesh_params_t   mesh_params;
    tivx_vpac_ldc_region_params_t region_params;
    vx_char                       dcc_config_file[TIVX_FILEIO_FILE_PATH_LENGTH];
    vx_uint32                     table_width;
    vx_uint32                     table_height;
    vx_uint32                     ds_factor;
    vx_uint32                     out_block_width;
    vx_uint32                     out_block_height;
    vx_uint32                     pixel_pad;
    vx_uint32                     init_x;
    vx_uint32                     init_y;
    vx_char                       lut_file[TIVX_FILEIO_FILE_PATH_LENGTH];
    char                          target_string[TIVX_TARGET_MAX_NAME];
    vx_int32                      num_channels;
    char                          sensor_name[ISS_SENSORS_MAX_NAME];
} TIOVXLdcNodeCfg;

void tiovx_ldc_init_cfg(TIOVXLdcNodeCfg *cfg);
vx_status tiovx_ldc_init_node(NodeObj *node);
vx_status tiovx_ldc_create_node(NodeObj *node);
vx_status tiovx_ldc_delete_node(NodeObj *node);
vx_uint32 tiovx_ldc_get_cfg_size();
vx_uint32 tiovx_ldc_get_priv_size();

#ifdef __cplusplus
}
#endif

#endif
