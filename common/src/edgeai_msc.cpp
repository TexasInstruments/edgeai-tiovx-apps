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
#include <common/include/edgeai_msc.h>
#include <utils/include/ti_logger.h>

namespace ti::edgeai::common
{

using namespace ti::utils;

multiScaler::multiScaler()
{
    LOG_DEBUG("multiScaler CONSTRUCTOR\n");
}


multiScaler::~multiScaler()
{
    LOG_DEBUG("multiScaler DESTRUCTOR\n");
}

int32_t multiScaler::getConfig(int32_t inputWidth,
                               int32_t inputHeight,
                               int32_t firstOutputWidth,
                               int32_t firstOutputHeight,
                               preProc *preProcObj)
{
    int32_t status = 0;
    int32_t resizeHeight;
    int32_t resizeWidth;
    int32_t cropHeight;
    int32_t cropWidth;
    float   cropXPct;
    float   cropYPct;
    int32_t crop_start_x;
    int32_t crop_start_y;

    if (inputWidth < firstOutputWidth || inputHeight < firstOutputHeight)
    {
        LOG_ERROR("Output width or height invalid. MSC cannot handle upscaling.\n");
        status = -1;
        return status;
    }

    if (inputWidth/firstOutputWidth > 4 || inputHeight/firstOutputHeight > 4)
    {
        LOG_ERROR("Output width or height invalid. "
                  "MSC cannot handle scaling down by more than 1/4.\n");
        status = -1;
        return status;
    }

    if (preProcObj != NULL)
    {
        resizeHeight = preProcObj->resizeHeight;
        resizeWidth = preProcObj->resizeWidth;
        cropHeight = preProcObj->cropHeight;
        cropWidth = preProcObj->cropWidth;

        cropXPct = (((resizeWidth - cropWidth) / 2 ) / float(resizeWidth));
        cropYPct = (((resizeHeight - cropHeight) / 2 ) / float(resizeHeight));

        crop_start_x = cropXPct * inputWidth;
        crop_start_y = cropYPct * inputHeight;

        resizeWidth  = cropWidth;
        resizeHeight = cropHeight;

        cropWidth    = inputWidth - (2 * crop_start_x);
        cropHeight   = inputHeight - (2 * crop_start_y);

        if((cropWidth/preProcObj->cropWidth) > 4
        ||
        (cropHeight/preProcObj->cropHeight) > 4)
        {
            multiScalerObj2.num_channels = 1;
            multiScalerObj2.num_outputs = 1;
            multiScalerObj2.input.bufq_depth = 1;
            for(int out = 0; out < multiScalerObj2.num_outputs; out++)
            {
                multiScalerObj2.output[out].bufq_depth = 1;
            }
            multiScalerObj2.interpolation_method = VX_INTERPOLATION_BILINEAR;
            multiScalerObj2.color_format = VX_DF_IMAGE_NV12;

            if (cropWidth/4 > preProcObj->cropWidth)
                multiScalerObj2.input.width = cropWidth/4;
            else
                multiScalerObj2.input.width = preProcObj->cropWidth;

            if (cropHeight/4 > preProcObj->cropHeight)
                multiScalerObj2.input.height = cropHeight/4;
            else
                multiScalerObj2.input.height =  preProcObj->cropHeight;

            multiScalerObj2.output[0].width = preProcObj->cropWidth;
            multiScalerObj2.output[0].height = preProcObj->cropHeight;

            resizeWidth = multiScalerObj2.input.width;
            resizeHeight = multiScalerObj2.input.height;

            tiovx_multi_scaler_module_crop_params_init(&multiScalerObj2);

            multiScalerObj2.en_multi_scalar_output = 0;

            useSecondaryMsc = true;

        }

        /** First output is for post-processing and second is for pre-processing. */
        multiScalerObj1.num_outputs = 2;
    }
    else
    {
        multiScalerObj1.num_outputs = 1;
    }

    multiScalerObj1.num_channels = 1;
    multiScalerObj1.input.bufq_depth = 1;
    for(int out = 0; out < multiScalerObj1.num_outputs; out++)
    {
        multiScalerObj1.output[out].bufq_depth = 1;
    }
    multiScalerObj1.interpolation_method = VX_INTERPOLATION_BILINEAR;
    multiScalerObj1.color_format = VX_DF_IMAGE_NV12;

    multiScalerObj1.input.width = inputWidth;
    multiScalerObj1.input.height = inputHeight;

    multiScalerObj1.output[0].width = firstOutputWidth;
    multiScalerObj1.output[0].height = firstOutputHeight;
    if (preProcObj != NULL)
    {
        multiScalerObj1.output[1].width = resizeWidth;
        multiScalerObj1.output[1].height = resizeHeight;
    }

    tiovx_multi_scaler_module_crop_params_init(&multiScalerObj1);

    if (preProcObj != NULL)
    {
        multiScalerObj1.crop_params[1].crop_start_x = crop_start_x;
        multiScalerObj1.crop_params[1].crop_start_y = crop_start_y;
        multiScalerObj1.crop_params[1].crop_width = inputWidth - (2 * crop_start_x);
        multiScalerObj1.crop_params[1].crop_height = inputHeight - (2 * crop_start_y);
    }

    multiScalerObj1.en_multi_scalar_output = 0;

    LOG_DEBUG("multiScalerObj1.input.width = %d, "
              "multiScalerObj1.input.height = %d \n",
              multiScalerObj1.input.width, multiScalerObj1.input.height);
    LOG_DEBUG("multiScalerObj1.output[0].width = %d, "
              "multiScalerObj1.output[0].height = %d \n",
              multiScalerObj1.output[0].width, multiScalerObj1.output[0].height);
    LOG_DEBUG("multiScalerObj1.output[1].width = %d, "
              "multiScalerObj1.output[1].height = %d \n",
              multiScalerObj1.output[1].width, multiScalerObj1.output[1].height);
    LOG_DEBUG("multiScalerObj1.crop_params[1].crop_start_x = %d, "
              "multiScalerObj1.crop_params[1].crop_start_y = %d \n",
              multiScalerObj1.crop_params[1].crop_start_x,
              multiScalerObj1.crop_params[1].crop_start_y);
    LOG_DEBUG("multiScalerObj1.crop_params[1].crop_width = %d, "
              "multiScalerObj1.crop_params[1].crop_height = %d \n",
              multiScalerObj1.crop_params[1].crop_width,
              multiScalerObj1.crop_params[1].crop_height);

    return status;
}

}