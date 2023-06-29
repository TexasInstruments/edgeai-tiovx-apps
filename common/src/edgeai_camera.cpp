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
    LOG_DEBUG("multiScaler DESTRUCTOR\n");
}

int32_t camera::getConfig(int32_t chMask)
{
    vx_status status = VX_SUCCESS;

    tiovx_sensor_module_params_init(&sensorObj);
    sensorObj.is_interactive = 0;
    sensorObj.ch_mask = chMask;

    tiovx_sensor_module_query(&sensorObj);
    sensorObj.sensor_index = 0; /* App works only for IMX390 2MP cameras */

    tiovx_capture_module_params_init(&captureObj, &sensorObj);
    captureObj.enable_error_detection = 0; // or 1
    captureObj.out_bufq_depth = 4; // This must be greater than 3

    /* VISS Module params init */
    tivx_vpac_viss_params_init(&vissObj.params);

    snprintf(vissObj.dcc_config_file_path,
             TIVX_FILEIO_FILE_PATH_LENGTH,
             "/opt/imaging/imx390/dcc_viss_wdr.bin");

    vissObj.input.bufq_depth = 1;

    vissObj.input.params.width              = sensorObj.image_width;
    vissObj.input.params.height             = sensorObj.image_height;
    vissObj.input.params.num_exposures      = sensorObj.sensorParams.sensorInfo.raw_params.num_exposures;
    vissObj.input.params.line_interleaved   = sensorObj.sensorParams.sensorInfo.raw_params.line_interleaved;
    vissObj.input.params.meta_height_before = sensorObj.sensorParams.sensorInfo.raw_params.meta_height_before;
    vissObj.input.params.meta_height_after  = sensorObj.sensorParams.sensorInfo.raw_params.meta_height_after;
    vissObj.input.params.format[0].pixel_container = sensorObj.sensorParams.sensorInfo.raw_params.format[0].pixel_container;
    vissObj.input.params.format[0].msb      = sensorObj.sensorParams.sensorInfo.raw_params.format[0].msb;
    vissObj.ae_awb_result_bufq_depth        = 1;

    vissObj.output_select[0] = TIOVX_VISS_MODULE_OUTPUT_NA; // 0
    vissObj.output_select[1] = TIOVX_VISS_MODULE_OUTPUT_NA;
    vissObj.output_select[2] = TIOVX_VISS_MODULE_OUTPUT_EN; // 1
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
             "/opt/imaging/imx390/dcc_ldc_wdr.bin");

    snprintf(ldcObj.lut_file_path,
             TIVX_FILEIO_FILE_PATH_LENGTH,
             "/opt/edgeai-test-data/raw_images/modules_test/imx390_ldc_lut_1920x1080.bin");

    ldcObj.ldc_mode = TIOVX_MODULE_LDC_OP_MODE_DCC_DATA;
    ldcObj.en_out_image_write = 0;
    ldcObj.en_output1 = 0;

    ldcObj.input.bufq_depth = 1;
    ldcObj.input.color_format = VX_DF_IMAGE_NV12;
    ldcObj.input.width = vissObj.output2.width;
    ldcObj.input.height = vissObj.output2.height;

    ldcObj.init_x = 0;
    ldcObj.init_y = 0;
    ldcObj.table_width = 1920;
    ldcObj.table_height = 1080;
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

}