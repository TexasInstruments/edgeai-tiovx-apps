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

#ifndef _AEWB_MODULE
#define _AEWB_MODULE

#include "tiovx_modules_types.h"
#include <tiovx_sensor_module.h>

#ifdef __cplusplus
extern "C" {
#endif

/*!
 * \brief Structure describing the configuration of linux aewb module.
 */
typedef struct {
    /*! \brief v4l2 subdev of the sensor to apply the gain and exposure result */
    vx_char     device[TIVX_FILEIO_FILE_PATH_LENGTH];

    /*! \brief dcc file for 2a algo configuration */
    vx_char     dcc_2a_file[TIVX_FILEIO_FILE_PATH_LENGTH];

    /*! \brief AE mode */
    vx_enum     ae_mode;

    /*! \brief AWB mode */
    vx_enum     awb_mode;

    /*! \brief AE skip frames */
    vx_uint32   ae_num_skip_frames;

    /*! \brief AWB skip frames */
    vx_uint32   awb_num_skip_frames;

    /*! \brief Sensor name as defined in Imaging repo */
    vx_char     sensor_name[ISS_SENSORS_MAX_NAME];
} AewbCfg;

typedef struct _AewbHandle AewbHandle;

/*! \brief Function to initialize AEWB config.
 * \param [in,out] cfg \ref AewbCfg.
 * \ingroup tiovx_modules
 */
void aewb_init_cfg(AewbCfg *cfg);

/*! \brief Function to create a aewb handle.
 * \param [in] cfg \ref AewbCfg.
 *
 * \return Pointer to new aewb handle \ref AewbHandle *\
 *
 * \ingroup tiovx_modules
 */
AewbHandle *aewb_create_handle(AewbCfg *cfg);

/*! \brief Process function to invoke the algorithm.
 * \param [in] aewb handle \ref AewbHandle.
 * \param [in] h3a_stats from viss out.
 * \param [out] aewb_result for viss in.
 * \ingroup tiovx_modules
 */
int aewb_process(AewbHandle *handle, Buf *h3a_stats, Buf *aewb_result);

/*! \brief Function to free a aewb handle.
 * \param [in] aewb handle \ref AewbHandle.
 * \ingroup tiovx_modules
 */
int aewb_delete_handle(AewbHandle *handle);

#ifdef __cplusplus
}
#endif

#endif
