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

#include <tiovx_sensor_module.h>

static char availableSensorNames[ISS_SENSORS_MAX_SUPPORTED_SENSOR][ISS_SENSORS_MAX_NAME];

vx_status tiovx_init_sensor_obj(SensorObj *sensorObj, char *objName)
{
    vx_status status = VX_SUCCESS;
    sensorObj->sensor_dcc_enabled=1;
    sensorObj->sensor_exp_control_enabled=0;
    sensorObj->sensor_gain_control_enabled=0;
    sensorObj->sensor_wdr_enabled=0;
    sensorObj->num_cameras_enabled=1;
    sensorObj->ch_mask=1;
    sensorObj->starting_channel=0;
    snprintf(sensorObj->sensor_name, ISS_SENSORS_MAX_NAME, "%s", objName);
    sensorObj->num_sensors_found = 1;
    sensorObj->sensor_features_enabled = 0;
    sensorObj->sensor_features_supported = 0;
    sensorObj->usecase_option = TIOVX_SENSOR_MODULE_FEATURE_CFG_UC0;
    sensorObj->sensor_index = 0;
    sensorObj->sensor_out_format = 0;

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

vx_status tiovx_sensor_query(SensorObj *sensorObj)
{
    vx_status status = VX_SUCCESS;
    char* sensor_list[ISS_SENSORS_MAX_SUPPORTED_SENSOR];
    vx_uint16 selectedSensor = 0xFFF;
    int32_t i;

    memset(availableSensorNames,
           0,
           (ISS_SENSORS_MAX_SUPPORTED_SENSOR * ISS_SENSORS_MAX_NAME));

    for(i = 0; i < ISS_SENSORS_MAX_SUPPORTED_SENSOR; i++)
    {
        sensor_list[i] = availableSensorNames[i];
    }

    memset(&sensorObj->sensorParams, 0, sizeof(IssSensor_CreateParams));
    status = appEnumerateImageSensor(sensor_list, &sensorObj->num_sensors_found);
    if(VX_SUCCESS != status)
    {
        TIOVX_MODULE_ERROR("[SENSOR-MODULE] appCreateImageSensor failed\n");
        return status;
    }


    selectedSensor = sensorObj->sensor_index;
    if(selectedSensor > (sensorObj->num_sensors_found - 1))
    {
        TIOVX_MODULE_ERROR("[SENSOR-MODULE] Invalid selection %d from app:\n", selectedSensor);
        return -1;
    }
    else
    {
        snprintf(sensorObj->sensor_name,
                 ISS_SENSORS_MAX_NAME,
                 "%s",
                 sensor_list[selectedSensor]);

        TIOVX_MODULE_PRINTF("[SENSOR-MODULE] %s selected : %s\n", sensorObj->sensor_name);

        status = appQueryImageSensor(sensorObj->sensor_name,
                                     &sensorObj->sensorParams);
        if(VX_SUCCESS != status)
        {
            TIOVX_MODULE_ERROR("[SENSOR-MODULE] appQueryImageSensor failed.\n");
            return status;
        }

        if(sensorObj->sensorParams.sensorInfo.raw_params.format[0].pixel_container == VX_DF_IMAGE_UYVY)
        {
            sensorObj->sensor_out_format = 1;
        }
        else
        {
            sensorObj->sensor_out_format = 0;
        }
    }

    if(sensorObj->ch_mask > 0)
    {
        vx_uint32 mask = sensorObj->ch_mask;
        vx_bool start_found = vx_false_e;
        vx_uint32 current_pos = 0;
        sensorObj->num_cameras_enabled = 0;
        while(mask > 0)
        {
            if(mask & 0x1)
            {
                sensorObj->num_cameras_enabled++;
                if (vx_false_e == start_found)
                {
                    start_found = vx_true_e;
                    sensorObj->starting_channel = current_pos;
                }
            }
            current_pos ++;
            mask = mask >> 1;
        }
    }

    /*
    Check for supported sensor features.
    It is upto the application to decide which features should be enabled.
    This demo app enables WDR, DCC and 2A if the sensor supports it.
    */

    sensorObj->sensor_features_supported = sensorObj->sensorParams.sensorInfo.features;

    if(ISS_SENSOR_FEATURE_COMB_COMP_WDR_MODE ==(sensorObj->sensor_features_supported & ISS_SENSOR_FEATURE_COMB_COMP_WDR_MODE))
    {
        TIOVX_MODULE_PRINTF("WDR mode is supported \n");
        sensorObj->sensor_features_enabled |= ISS_SENSOR_FEATURE_COMB_COMP_WDR_MODE;
        sensorObj->sensor_wdr_enabled = 1;
    }else
    {
        TIOVX_MODULE_PRINTF("WDR mode is not supported. Defaulting to linear \n");
        sensorObj->sensor_features_enabled |= ISS_SENSOR_FEATURE_LINEAR_MODE;
        sensorObj->sensor_wdr_enabled = 0;
    }

    if(ISS_SENSOR_FEATURE_MANUAL_EXPOSURE == (sensorObj->sensor_features_supported & ISS_SENSOR_FEATURE_MANUAL_EXPOSURE))
    {
        TIOVX_MODULE_PRINTF("Expsoure control is supported \n");
        sensorObj->sensor_features_enabled |= ISS_SENSOR_FEATURE_MANUAL_EXPOSURE;
        sensorObj->sensor_exp_control_enabled = 1;
    }

    if(ISS_SENSOR_FEATURE_MANUAL_GAIN == (sensorObj->sensor_features_supported & ISS_SENSOR_FEATURE_MANUAL_GAIN))
    {
        TIOVX_MODULE_PRINTF("Gain control is supported \n");
        sensorObj->sensor_features_enabled |= ISS_SENSOR_FEATURE_MANUAL_GAIN;
        sensorObj->sensor_gain_control_enabled = 1;
    }

    if(ISS_SENSOR_FEATURE_CFG_UC1 == (sensorObj->sensor_features_supported & ISS_SENSOR_FEATURE_CFG_UC1))
    {
        if(sensorObj->usecase_option == TIOVX_SENSOR_MODULE_FEATURE_CFG_UC1)
        {
            TIOVX_MODULE_PRINTF("CMS Usecase is supported \n");
            sensorObj->sensor_features_enabled |= ISS_SENSOR_FEATURE_CFG_UC1;
        }
    }

    if(ISS_SENSOR_FEATURE_DCC_SUPPORTED == (sensorObj->sensor_features_supported & ISS_SENSOR_FEATURE_DCC_SUPPORTED))
    {
        sensorObj->sensor_features_enabled |= ISS_SENSOR_FEATURE_DCC_SUPPORTED;
        sensorObj->sensor_dcc_enabled = 1;
        TIOVX_MODULE_PRINTF("Sensor DCC is enabled \n");
    }else
    {
        sensorObj->sensor_dcc_enabled = 0;
        TIOVX_MODULE_PRINTF("Sensor DCC is disabled \n");
    }

    sensorObj->image_width   = sensorObj->sensorParams.sensorInfo.raw_params.width;
    sensorObj->image_height  = sensorObj->sensorParams.sensorInfo.raw_params.height;

    TIOVX_MODULE_PRINTF("Sensor width = %d\n", sensorObj->sensorParams.sensorInfo.raw_params.width);
    TIOVX_MODULE_PRINTF("Sensor height = %d\n", sensorObj->sensorParams.sensorInfo.raw_params.height);
    TIOVX_MODULE_PRINTF("Sensor DCC ID = %d\n", sensorObj->sensorParams.dccId);
    TIOVX_MODULE_PRINTF("Sensor Supported Features = 0x%08X\n", sensorObj->sensor_features_supported);
    TIOVX_MODULE_PRINTF("Sensor Enabled Features = 0x%08X\n", sensorObj->sensor_features_enabled);

    return status;
}

vx_status tiovx_sensor_init(SensorObj *sensorObj)
{
    vx_status status = VX_SUCCESS;
    int32_t sensor_init_status = -1;
    int32_t ch_mask = sensorObj->ch_mask;

    sensor_init_status = appInitImageSensor(sensorObj->sensor_name,
                                            sensorObj->sensor_features_enabled,
                                            ch_mask);
    if(0 != sensor_init_status)
    {
        TIOVX_MODULE_ERROR("Error initializing sensor %s \n", sensorObj->sensor_name);
        status = VX_FAILURE;
    }

    return status;
}

void tiovx_sensor_deinit(SensorObj *sensorObj)
{
    appDeInitImageSensor(sensorObj->sensor_name);
}

vx_status tiovx_sensor_start(SensorObj *sensorObj)
{
    vx_status status = VX_SUCCESS;

    status = appStartImageSensor(sensorObj->sensor_name, sensorObj->ch_mask);

    return status;
}

vx_status tiovx_sensor_stop(SensorObj *sensorObj)
{
    vx_status status = VX_SUCCESS;

    status = appStopImageSensor(sensorObj->sensor_name, sensorObj->ch_mask);

    return status;
}
