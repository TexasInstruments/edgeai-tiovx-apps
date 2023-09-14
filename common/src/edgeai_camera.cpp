/*
 *  Copyright (C) 2023 Texas Instruments Incorporated - http://www.ti.com/
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions
 *  are met:
 *
 *    Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 *    Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the
 *    distribution.
 *
 *    Neither the name of Texas Instruments Incorporated nor the names of
 *    its contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 *  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 *  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 *  A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 *  OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 *  SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 *  LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 *  DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 *  THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 *  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 *  OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/* Module headers */
#include <common/include/edgeai_camera.h>
#include <utils/include/ti_logger.h>

namespace ti::edgeai::common
{

using namespace ti::utils;

camera::camera()
{
    LOG_DEBUG("camera CONSTRUCTOR\n");
}

camera::~camera()
{
    LOG_DEBUG("camera DESTRUCTOR\n");

    /* Delete all camera openVX modules */
    tiovx_capture_module_delete(&captureObj);
    tiovx_viss_module_delete(&vissObj);
#if !defined (SOC_AM62A)
    tiovx_aewb_module_delete(&aewbObj);
#endif
    tiovx_ldc_module_delete(&ldcObj);

    /* Deinit all camera openVX modules */
    tiovx_sensor_module_deinit(&sensorObj);
    tiovx_capture_module_deinit(&captureObj);
    tiovx_viss_module_deinit(&vissObj);
#if !defined (SOC_AM62A)
    tiovx_aewb_module_deinit(&aewbObj);
#endif
    tiovx_ldc_module_deinit(&ldcObj);

}

void getCamChMask(DemoConfig &config, int32_t &chMask, int32_t &numCam,
                    InputInfo*& camInputInfo)
{
    for (auto const &[name,flow] : config.m_flowMap)
    {
        auto const &input = config.m_inputMap[flow->m_inputId];

        /**
         * chMask = 00001101 means cameras connected to port
         *  (or m_cameraId) 0, 2 and 3 of fusion board
         */
        if(input->m_source == "camera")
        {
            chMask = chMask | (1 << input->m_cameraId);
            numCam++;
            camInputInfo = input;
        }
    }
}

int32_t camera::getConfig(InputInfo* camInputInfo, int32_t chMask)
{
    vx_status status = VX_SUCCESS;

    tiovx_sensor_module_params_init(&sensorObj);
    sensorObj.is_interactive = 0;
    sensorObj.ch_mask = chMask;

    tiovx_sensor_module_query(&sensorObj);

    /* App works only for IMX390 2MP cameras */
    if(camInputInfo->m_sen_id == "imx390")
    {
        sensorObj.sensor_index = 0;
    }
    else
    {
        throw runtime_error("App works only for IMX390 2MP cameras now. \n");
    }

    tiovx_capture_module_params_init(&captureObj, &sensorObj);
    captureObj.enable_error_detection = 0; /* or 1 */
    captureObj.out_bufq_depth = 4; /* This must be greater than 3 */

    /* VISS Module params init */
    tivx_vpac_viss_params_init(&vissObj.params);

    snprintf(vissObj.dcc_config_file_path,
             TIVX_FILEIO_FILE_PATH_LENGTH,
             camInputInfo->m_vissDccPath.c_str());

    vissObj.input.bufq_depth = 1;

    memcpy(&vissObj.input.params,
            &sensorObj.sensorParams.sensorInfo.raw_params,
            sizeof(tivx_raw_image_create_params_t));

    vissObj.ae_awb_result_bufq_depth        = 1;

    vissObj.output_select[0] = TIOVX_VISS_MODULE_OUTPUT_NA; /* 0 */
    vissObj.output_select[1] = TIOVX_VISS_MODULE_OUTPUT_NA;
    vissObj.output_select[2] = TIOVX_VISS_MODULE_OUTPUT_EN; /* 1 */
    vissObj.output_select[3] = TIOVX_VISS_MODULE_OUTPUT_NA;
    vissObj.output_select[4] = TIOVX_VISS_MODULE_OUTPUT_NA;

    /* As we are selecting output2, specify output2 image properties */
    vissObj.output2.bufq_depth   = 1;
    vissObj.output2.color_format = VX_DF_IMAGE_NV12;
    vissObj.output2.width        = sensorObj.image_width;
    vissObj.output2.height       = sensorObj.image_height;

    vissObj.h3a_stats_bufq_depth = 1;

    /* LDC Module params init */
    snprintf(ldcObj.dcc_config_file_path,
             TIVX_FILEIO_FILE_PATH_LENGTH,
             camInputInfo->m_ldcDccPath.c_str());

    /* Configuring LDC in DCC mode */
    ldcObj.ldc_mode = TIOVX_MODULE_LDC_OP_MODE_DCC_DATA;
    ldcObj.en_out_image_write = 0;
    ldcObj.en_output1 = 0;

    ldcObj.input.bufq_depth = 1;
    ldcObj.input.color_format = VX_DF_IMAGE_NV12;
    ldcObj.input.width = vissObj.output2.width;
    ldcObj.input.height = vissObj.output2.height;

    if(camInputInfo->m_sen_id == "imx390")
    {
        ldcObj.init_x = 0;
        ldcObj.init_y = 0;
        ldcObj.table_width = 1920;
        ldcObj.table_height = 1080;
    }

    ldcObj.ds_factor = 2;
    ldcObj.out_block_width = 64;
    ldcObj.out_block_height = 32;
    ldcObj.pixel_pad = 1;

    ldcObj.output0.bufq_depth = 1;
    ldcObj.output0.color_format = VX_DF_IMAGE_NV12;
    ldcObj.output0.width = ldcObj.table_width;
    ldcObj.output0.height = ldcObj.table_height;

    return status;
}

int32_t camera::cameraInit(vx_context context, InputInfo* camInputInfo,
                            int32_t chMask)
{
    vx_status status = VX_SUCCESS;

    status = getConfig(camInputInfo, chMask);

    if(status == VX_SUCCESS)
    {
        status = tiovx_sensor_module_init(&sensorObj,
                            const_cast<char*>(string("sensor_obj").c_str()));
    }
    if(status == VX_SUCCESS)
    {
        status = tiovx_capture_module_init(context, &captureObj, &sensorObj);
    }
    if(status == VX_SUCCESS)
    {
        status = tiovx_viss_module_init(context, &vissObj, &sensorObj);
    }
#if !defined (SOC_AM62A)
    if(status == VX_SUCCESS)
    {
        status = tiovx_aewb_module_init(context, &aewbObj, &sensorObj,
                            const_cast<char*>(string("aewb_obj").c_str()),
                            0, sensorObj.num_cameras_enabled);
    }
#endif
    if(status == VX_SUCCESS)
    {
        status = tiovx_ldc_module_init(context, &ldcObj, &sensorObj);
    }

    return status;
}

int32_t camera::cameraCreate(vx_graph graph, InputInfo* camInputInfo)
{
    int32_t status = VX_SUCCESS;

    string viss_target = TIVX_TARGET_VPAC_VISS1;
    string ldc_target = TIVX_TARGET_VPAC_LDC1;

#if defined(SOC_J784S4)
    if(camInputInfo->m_vpac_id == 2)
    {
        viss_target = TIVX_TARGET_VPAC2_VISS1;
        ldc_target = TIVX_TARGET_VPAC2_LDC1;
    }
    else
#endif
    if(camInputInfo->m_vpac_id == 1)
    {
        viss_target = TIVX_TARGET_VPAC_VISS1;
        ldc_target = TIVX_TARGET_VPAC_LDC1;
    }

    if(status == VX_SUCCESS)
    {
        status = tiovx_capture_module_create(graph, &captureObj,
                                             TIVX_TARGET_CAPTURE1);
    }
    if(status == VX_SUCCESS)
    {
        status = vxReleaseObjectArray(&vissObj.ae_awb_result_arr[0]);
        vissObj.ae_awb_result_arr[0] = NULL;
        status = tiovx_viss_module_create(graph, &vissObj,
                                          captureObj.image_arr[0],
                                          NULL,
                                          viss_target.c_str());
    }
#if !defined (SOC_AM62A)
    if(status == VX_SUCCESS)
    {
        status = tiovx_aewb_module_create(graph, &aewbObj,
                                          vissObj.h3a_stats_arr[0]);
    }
#endif
    if(status == VX_SUCCESS)
    {
        status = tiovx_ldc_module_create(graph, &ldcObj,
                                         vissObj.output2.arr[0],
                                         ldc_target.c_str());
    }

    return status;
}

} /* namespace ti::edgeai::common */