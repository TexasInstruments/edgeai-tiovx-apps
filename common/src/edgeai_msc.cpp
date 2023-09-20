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

    tiovx_multi_scaler_module_delete(&multiScalerObj1);
    if(useSecondaryMsc)
    {
        tiovx_multi_scaler_module_delete(&multiScalerObj2);
    }

    tiovx_multi_scaler_module_deinit(&multiScalerObj1);
    if(useSecondaryMsc)
    {
        tiovx_multi_scaler_module_deinit(&multiScalerObj2);
    }
}

int32_t multiScaler::getConfig(int32_t input_wd,
                               int32_t input_ht,
                               int32_t post_proc_wd,
                               int32_t post_proc_ht,
                               preProc *pre_proc_obj)
{
    int32_t status = 0;
    int32_t crop_start_x = 0;
    int32_t crop_start_y = 0;
    int32_t resizeHeight = pre_proc_obj->resizeHeight;
    int32_t resizeWidth = pre_proc_obj->resizeWidth;
    int32_t cropHeight = pre_proc_obj->cropHeight;
    int32_t cropWidth = pre_proc_obj->cropWidth;

    if ( (input_wd < post_proc_wd) || (input_ht < post_proc_ht) )
    {
        LOG_ERROR("Output width or height invalid. "
                  "MSC cannot handle upscaling.\n");
        status = -1;
        return status;
    }

    if ( (input_wd/post_proc_wd > 4) || (input_ht/post_proc_ht > 4) )
    {
        LOG_ERROR("Output width or height invalid. "
                  "MSC cannot handle scaling down by more than 1/4.\n");
        status = -1;
        return status;
    }

    float cropXPct =
        (((resizeWidth - cropWidth) / 2 ) / float(resizeWidth));
    float cropYPct =
        (((resizeHeight - cropHeight) / 2 ) / float(resizeHeight));
    crop_start_x = cropXPct * input_wd;
    crop_start_y = cropYPct * input_ht;
    resizeWidth  = cropWidth;
    resizeHeight = cropHeight;
    cropWidth    = input_wd - (2 * crop_start_x);
    cropHeight   = input_ht - (2 * crop_start_y);

    if( ((cropWidth/pre_proc_obj->cropWidth) > 4) ||
        ((cropHeight/pre_proc_obj->cropHeight) > 4) )
    {
        multiScalerObj2.num_channels = 1;
        multiScalerObj2.num_outputs = 1;
        multiScalerObj2.input.bufq_depth = 1;
        for(int32_t out = 0; out < multiScalerObj2.num_outputs; out++)
        {
            multiScalerObj2.output[out].bufq_depth = 1;
        }
        multiScalerObj2.interpolation_method = VX_INTERPOLATION_BILINEAR;
        multiScalerObj2.color_format = VX_DF_IMAGE_NV12;

        if ( (cropWidth/4) > pre_proc_obj->cropWidth)
        {
            multiScalerObj2.input.width = cropWidth/4;
        }
        else
        {
            multiScalerObj2.input.width = pre_proc_obj->cropWidth;
        }

        if ( (cropHeight/4) > pre_proc_obj->cropHeight)
        {
            multiScalerObj2.input.height = cropHeight/4;
        }
        else
        {
            multiScalerObj2.input.height =  pre_proc_obj->cropHeight;
        }

        multiScalerObj2.output[0].width = pre_proc_obj->cropWidth;
        multiScalerObj2.output[0].height = pre_proc_obj->cropHeight;

        resizeWidth = multiScalerObj2.input.width;
        resizeHeight = multiScalerObj2.input.height;

        tiovx_multi_scaler_module_crop_params_init(&multiScalerObj2);

        multiScalerObj2.en_multi_scalar_output = 0;

        useSecondaryMsc = true;

    }

    multiScalerObj1.num_channels = 1;
    multiScalerObj1.num_outputs = 2;
    multiScalerObj1.input.bufq_depth = 1;
    for(int32_t out = 0; out < multiScalerObj1.num_outputs; out++)
    {
        multiScalerObj1.output[out].bufq_depth = 1;
    }
    multiScalerObj1.interpolation_method = VX_INTERPOLATION_BILINEAR;
    multiScalerObj1.color_format = VX_DF_IMAGE_NV12;

    multiScalerObj1.input.width = input_wd;
    multiScalerObj1.input.height = input_ht;

    multiScalerObj1.output[0].width = resizeWidth;
    multiScalerObj1.output[0].height = resizeHeight;
    multiScalerObj1.output[1].width = post_proc_wd;
    multiScalerObj1.output[1].height = post_proc_ht;

    tiovx_multi_scaler_module_crop_params_init(&multiScalerObj1);

    multiScalerObj1.crop_params[0].crop_start_x = crop_start_x;
    multiScalerObj1.crop_params[0].crop_start_y = crop_start_y;
    multiScalerObj1.crop_params[0].crop_width = input_wd - (2 * crop_start_x);
    multiScalerObj1.crop_params[0].crop_height = input_ht - (2 * crop_start_y);

    multiScalerObj1.en_multi_scalar_output = 0;

    LOG_DEBUG("multiScalerObj1.input.width = %d, "
              "multiScalerObj1.input.height = %d \n",
              multiScalerObj1.input.width, multiScalerObj1.input.height);
    LOG_DEBUG("multiScalerObj1.output[0].width = %d, "
              "multiScalerObj1.output[0].height = %d \n",
              multiScalerObj1.output[0].width,
              multiScalerObj1.output[0].height);
    LOG_DEBUG("multiScalerObj1.output[1].width = %d, "
              "multiScalerObj1.output[1].height = %d \n",
              multiScalerObj1.output[1].width,
              multiScalerObj1.output[1].height);
    LOG_DEBUG("multiScalerObj1.crop_params[0].crop_start_x = %d, "
              "multiScalerObj1.crop_params[0].crop_start_y = %d \n",
              multiScalerObj1.crop_params[0].crop_start_x,
              multiScalerObj1.crop_params[0].crop_start_y);
    LOG_DEBUG("multiScalerObj1.crop_params[0].crop_width = %d, "
              "multiScalerObj1.crop_params[0].crop_height = %d \n",
              multiScalerObj1.crop_params[0].crop_width,
              multiScalerObj1.crop_params[0].crop_height);

    return status;
}

int32_t multiScaler::multiScalerInit(vx_context context, int32_t input_wd,
                                int32_t input_ht, int32_t post_proc_wd,
                                int32_t post_proc_ht, preProc *pre_proc_obj,
                                string &srcType, camera*& cameraObj)
{
    int32_t status = VX_SUCCESS;

    /* Get config for multiscaler module. */
    status = getConfig(input_wd, input_ht, post_proc_wd, post_proc_ht,
                        pre_proc_obj);

    if(srcType == "camera")
    {
        multiScalerObj1.num_channels =
            cameraObj->sensorObj.num_cameras_enabled;
        multiScalerObj2.num_channels =
            cameraObj->sensorObj.num_cameras_enabled;
    }
    if(status == VX_SUCCESS)
    {
        status = tiovx_multi_scaler_module_init(context, &multiScalerObj1);
    }
    if( (useSecondaryMsc) && (status == VX_SUCCESS))
    {
        status = tiovx_multi_scaler_module_init(context, &multiScalerObj2);
    }

    return status;
}

int32_t allMultiScalerCreate(vx_graph graph, camera*& cameraObj,
                             vector<multiScaler*> &multiScalerObjs,
                             vector<int32_t> &camMscIdxMap, DemoConfig &config)
{
    int32_t     status = VX_SUCCESS;
    uint32_t    msc_cnt = 0;

    if(cameraObj != NULL)
    {
        /* Create multiscaler module with appropriate msc connected to ldc node.
        */
        for(auto &iter : camMscIdxMap)
        {
            if(status == VX_SUCCESS)
            {
                status = tiovx_multi_scaler_module_create(graph,
                                        &multiScalerObjs[iter]->multiScalerObj1,
                                        cameraObj->ldcObj.output0.arr[0],
                                        TIVX_TARGET_VPAC_MSC1);

                if( (status == VX_SUCCESS) &&
                    (multiScalerObjs[iter]->useSecondaryMsc) )
                {
                    status = tiovx_multi_scaler_module_create(graph,
                                        &multiScalerObjs[iter]->multiScalerObj2,
                        multiScalerObjs[iter]->multiScalerObj1.output[0].arr[0],
                                        TIVX_TARGET_VPAC_MSC2);
                }

                /* If the input is camera, the multiscaler used with this
                 * subflow is not the first node.
                 */
                multiScalerObjs[iter]->isHeadNode = false;
            }
        }
    }

    for (auto const &[name,flow] : config.m_flowMap)
    {
        uint32_t   msc_ip_used;
        auto const &modelIds = flow->m_modelIds;

        /* For each flow input will be read only once and same input will be
           passed to other multiscaler nodes.
           msc_cnt is the counter for traversing multiscaler object array
           msc_ip_used is the index of the multiscaler object in which the input is read and
           this multiscaler object's input will be used in other multiscaler objects of
           subflows as inputs */
        if( (status == VX_SUCCESS) && (multiScalerObjs[msc_cnt]->isHeadNode) )
        {
            status = tiovx_multi_scaler_module_create(graph,
                                    &multiScalerObjs[msc_cnt]->multiScalerObj1,
                                    NULL, TIVX_TARGET_VPAC_MSC1);

            if( (status == VX_SUCCESS) &&
                (multiScalerObjs[msc_cnt]->useSecondaryMsc) )
            {
                status = tiovx_multi_scaler_module_create(graph,
                                    &multiScalerObjs[msc_cnt]->multiScalerObj2,
                    multiScalerObjs[msc_cnt]->multiScalerObj1.output[0].arr[0],
                                    TIVX_TARGET_VPAC_MSC2);
            }
            msc_ip_used = msc_cnt;
            msc_cnt++;
        }
        /* Starting i from 1 as first index already created */
        for(uint32_t i = 1; i < modelIds.size(); i++)
        {
            if( (status == VX_SUCCESS) &&
                (multiScalerObjs[msc_cnt]->isHeadNode) )
            {
                status = tiovx_multi_scaler_module_create(graph,
                                    &multiScalerObjs[msc_cnt]->multiScalerObj1,
                    multiScalerObjs[msc_ip_used]->multiScalerObj1.input.arr[0],
                                    TIVX_TARGET_VPAC_MSC1);

                if( (status == VX_SUCCESS) &&
                    (multiScalerObjs[msc_cnt]->useSecondaryMsc) )
                {
                    status = tiovx_multi_scaler_module_create(graph,
                                    &multiScalerObjs[msc_cnt]->multiScalerObj2,
                    multiScalerObjs[msc_cnt]->multiScalerObj1.output[0].arr[0],
                                    TIVX_TARGET_VPAC_MSC2);
                }
            }
            msc_cnt++;
        }
    }

    return status;
}

} /* namespace ti::edgeai::common */