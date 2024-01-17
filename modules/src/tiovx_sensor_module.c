/*
 *
 * Copyright (c) 2020 Texas Instruments Incorporated
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

#include <tiovx_sensor_module.h>

vx_status tiovx_init_sensor(SensorObj *sensorObj, char *objName)
{
    vx_status status = VX_SUCCESS;
    sensorObj->sensor_dcc_enabled=1;
    sensorObj->sensor_exp_control_enabled=0;
    sensorObj->sensor_gain_control_enabled=0;
    sensorObj->sensor_wdr_enabled=0;
    sensorObj->num_cameras_enabled=1;
    sensorObj->ch_mask=1;
    snprintf(sensorObj->sensor_name, ISS_SENSORS_MAX_NAME, "%s", objName);

    TIOVX_MODULE_PRINTF("[SENSOR-MODULE] Sensor name = %s\n", sensorObj->sensor_name);

    if(strcmp(sensorObj->sensor_name, "SENSOR_SONY_IMX390_UB953_D3") == 0)
    {
        sensorObj->sensorParams.dccId=390;
    }
    else if(strcmp(sensorObj->sensor_name, "SENSOR_ONSEMI_AR0820_UB953_LI") == 0)
    {
        sensorObj->sensorParams.dccId=820;
    }
    else if(strcmp(sensorObj->sensor_name, "SENSOR_ONSEMI_AR0233_UB953_MARS") == 0)
    {
        sensorObj->sensorParams.dccId=233;
    }
    else if(strcmp(sensorObj->sensor_name, "SENSOR_SONY_IMX219_RPI") == 0)
    {
        sensorObj->sensorParams.dccId=219;
    }
    else if(strcmp(sensorObj->sensor_name, "SENSOR_OV2312_UB953_LI") == 0)
    {
        sensorObj->sensorParams.dccId=2312;
    }
    else
    {
        TIOVX_MODULE_ERROR("[SENSOR-MODULE] Invalid sensor name\n");
        status = VX_FAILURE;
    }

    TIOVX_MODULE_PRINTF("[SENSOR-MODULE] Dcc ID = %d\n", sensorObj->sensorParams.dccId);

    return status;
}
