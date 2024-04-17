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
 #ifndef _TIOVX_CAPTURE_MODULE
 #define _TIOVX_CAPTURE_MODULE

#include "tiovx_modules_types.h"

#include <TI/video_io_capture.h>
#include <tiovx_sensor_module.h>

#ifdef __cplusplus
extern "C" {
#endif

/*!
 * \brief Structure describing the configuration of tiovx capture module.
 */
typedef struct {
    /*! \brief Sensor name as defined in Imaging repo */
    char                    sensor_name[ISS_SENSORS_MAX_NAME];

    /*! \brief Channel mask describing the cameras selected in case FPD Link fusion board */
    uint32_t                ch_mask;

    /*! \brief Index of sensor to be selected as described in imaging repo */
    vx_int32                usecase_option;
    vx_uint8                enable_error_detection;
    char                    error_frame_filename[256];

    /*! \brief OpenVX target on which to run the new node created using this config */
    char                    target_string[TIVX_TARGET_MAX_NAME];
} TIOVXCaptureNodeCfg;

/*! \brief Function to initialize tiovx capture module config.
 * \param [in,out] cfg \ref TIOVXCaptureNodeCfg.
 * \ingroup tiovx_modules
 */
void tiovx_capture_init_cfg(TIOVXCaptureNodeCfg *cfg);
vx_status tiovx_capture_init_node(NodeObj *node);
vx_status tiovx_capture_create_node(NodeObj *node);
vx_status tiovx_capture_post_verify_graph(NodeObj *node);
vx_status tiovx_capture_delete_node(NodeObj *node);
vx_uint32 tiovx_capture_get_cfg_size();
vx_uint32 tiovx_capture_get_priv_size();

#ifdef __cplusplus
}
#endif

#endif // _TIOVX_CAPTURE_MODULE
