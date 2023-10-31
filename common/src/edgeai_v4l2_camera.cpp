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
#include <common/include/edgeai_v4l2_camera.h>
#include <utils/include/ti_logger.h>

namespace ti::edgeai::common
{

using namespace ti::utils;

v4l2Camera::v4l2Camera(InputInfo* inputInfo)
{
    LOG_DEBUG("v4l2Camera CONSTRUCTOR\n");
    m_inputInfo = inputInfo;

    tiovx_querry_sensor(&sensorObj);

    /* VISS Module params init */
    tivx_vpac_viss_params_init(&vissObj.params);
    vissObj.input.bufq_depth = 1;
    vissObj.input.params.width  = m_inputInfo->m_width;
    vissObj.input.params.height = m_inputInfo->m_height;
    vissObj.input.params.num_exposures = 1;
    vissObj.input.params.line_interleaved = vx_false_e;
    vissObj.input.params.meta_height_before = 0;
    vissObj.input.params.meta_height_after = 0;
    vissObj.ae_awb_result_bufq_depth = 2;
    vissObj.h3a_stats_bufq_depth = 2;

    if (m_inputInfo->m_sen_id == "imx219")
    {
        char sensor_name[] = "SENSOR_SONY_IMX219_RPI";
        tiovx_init_sensor(&sensorObj, sensor_name);
        vissObj.input.params.format[0].pixel_container = TIVX_RAW_IMAGE_8_BIT;
        vissObj.input.params.format[0].msb = 7;
        snprintf(vissObj.dcc_config_file_path,
                 TIVX_FILEIO_FILE_PATH_LENGTH,
                 inputInfo->m_vissDccPath.c_str());
    }
    else if (m_inputInfo->m_sen_id == "imx390")
    {
        char sensor_name[] = "SENSOR_SONY_IMX390_UB953_D3";
        tiovx_init_sensor(&sensorObj, sensor_name);
        vissObj.input.params.format[0].pixel_container = TIVX_RAW_IMAGE_16_BIT;
        vissObj.input.params.format[0].msb = 9;
        snprintf(vissObj.dcc_config_file_path,
                 TIVX_FILEIO_FILE_PATH_LENGTH,
                 inputInfo->m_vissDccPath.c_str());
    }

    vissObj.output_select[0] = TIOVX_VISS_MODULE_OUTPUT_NA;
    vissObj.output_select[1] = TIOVX_VISS_MODULE_OUTPUT_NA;
    vissObj.output_select[2] = TIOVX_VISS_MODULE_OUTPUT_EN;
    vissObj.output_select[3] = TIOVX_VISS_MODULE_OUTPUT_NA;
    vissObj.output_select[4] = TIOVX_VISS_MODULE_OUTPUT_NA;

    /* As we are selecting output2, specify output2 image properties */
    vissObj.output2.bufq_depth   = 2;
    vissObj.output2.color_format = VX_DF_IMAGE_NV12;
    vissObj.output2.width        = m_inputInfo->m_width;
    vissObj.output2.height       = m_inputInfo->m_height;

    if (m_inputInfo->m_ldc)
    {
        /* LDC Module params init */
        snprintf(ldcObj.dcc_config_file_path,
                 TIVX_FILEIO_FILE_PATH_LENGTH,
                 inputInfo->m_ldcDccPath.c_str());

        /* Configuring LDC in DCC mode */
        ldcObj.ldc_mode = TIOVX_MODULE_LDC_OP_MODE_DCC_DATA;
        ldcObj.en_out_image_write = 0;
        ldcObj.en_output1 = 0;

        ldcObj.input.bufq_depth = 1;
        ldcObj.input.color_format = VX_DF_IMAGE_NV12;
        ldcObj.input.width = vissObj.output2.width;
        ldcObj.input.height = vissObj.output2.height;

        if(inputInfo->m_sen_id == "imx390")
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
    }
}

v4l2Camera::~v4l2Camera()
{
    LOG_DEBUG("v4l2Camera DESTRUCTOR\n");

    tiovx_viss_module_delete(&vissObj);
    tiovx_ldc_module_delete(&ldcObj);

    /* Deinit all camera openVX modules */
    tiovx_sensor_module_deinit(&sensorObj);
    tiovx_viss_module_deinit(&vissObj);

    if (m_inputInfo->m_ldc)
    {
        tiovx_ldc_module_deinit(&ldcObj);
    }

}

int32_t v4l2Camera::v4l2CameraInit(vx_context context)
{
    vx_status status = VX_SUCCESS;

    if(status == VX_SUCCESS)
    {
        status = tiovx_viss_module_init(context, &vissObj, &sensorObj);
    }
    if(status == VX_SUCCESS && m_inputInfo->m_ldc)
    {
        status = tiovx_ldc_module_init(context, &ldcObj, &sensorObj);
    }

    return status;
}

int32_t v4l2Camera::v4l2CameraCreate(vx_graph graph)
{
    int32_t status = VX_SUCCESS;

    string viss_target = TIVX_TARGET_VPAC_VISS1;
    string ldc_target = TIVX_TARGET_VPAC_LDC1;

#if defined(SOC_J784S4)
    if(m_inputInfo->m_vpac_id == 2)
    {
        viss_target = TIVX_TARGET_VPAC2_VISS1;
        ldc_target = TIVX_TARGET_VPAC2_LDC1;
    }
    else
#endif
    if(m_inputInfo->m_vpac_id == 1)
    {
        viss_target = TIVX_TARGET_VPAC_VISS1;
        ldc_target = TIVX_TARGET_VPAC_LDC1;
    }

    if(status == VX_SUCCESS)
    {
        status = tiovx_viss_module_create(graph, &vissObj,
                                          NULL,
                                          NULL,
                                          viss_target.c_str());
    }
    if(status == VX_SUCCESS && m_inputInfo->m_ldc)
    {
        status = tiovx_ldc_module_create(graph, &ldcObj,
                                         vissObj.output2.arr[0],
                                         ldc_target.c_str());
    }

    return status;
}

int32_t v4l2Camera::v4l2CameraCapture(vx_graph graph)
{
    int32_t status = VX_SUCCESS;

    vxGraphParameterEnqueueReadyRef(graph,
                                    vissObj.input.graph_parameter_index,
                                    (vx_reference*) &vissObj.input.image_handle[0],
                                    1);

    return status;
}

} /* namespace ti::edgeai::common */
