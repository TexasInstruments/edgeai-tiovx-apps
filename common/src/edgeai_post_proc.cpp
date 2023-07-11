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

/* Standard headers */
#include <filesystem>
#include <iostream>

/* Third-party headers. */
#include <yaml-cpp/yaml.h>

/* Module headers. */
#include <common/include/edgeai_post_proc.h>
#include <common/include/edgeai_utils.h>
#include <utils/include/ti_logger.h>

#include <edgeai_nv12_drawing_utils.h>

namespace ti::edgeai::common
{
using namespace ti::utils;

/* Color Map for semantic segmentation */
uint8_t RGB_COLOR_MAP[26][3] = {{255,0,0},{0,255,0},{73,102,92},
                                {255,255,0},{0,255,255},{0,99,245},
                                {255,127,0},{0,255,100},{235,117,117},
                                {242,15,109},{118,194,167},{255,0,255},
                                {231,200,255},{255,186,239},{0,110,0},
                                {0,0,255},{110,0,0},{110,110,0},
                                {100,0,50},{90,60,0},{255,255,255} ,
                                {170,0,255},{204,255,0},{78,69,128},
                                {133,133,74},{0,0,110}};

postProc::postProc()
{
    LOG_DEBUG("postProc CONSTRUCTOR\n");
}


postProc::~postProc()
{
    if(mYUVColorMap != NULL)
    {
        for(int32_t i = 0; i < mMaxColorClass; ++i)
        {
            delete [] mYUVColorMap[i];
        }
        delete [] mYUVColorMap;
    }

    LOG_DEBUG("postProc DESTRUCTOR\n");
}

int32_t postProc::getConfig(const string        &modelBasePath,
                            sTIDL_IOBufDesc_t   *ioBufDesc,
                            int32_t             imageWidth,
                            int32_t             imageHeight)
{
    const string        &paramFile = modelBasePath + "/param.yaml";
    int32_t             status = 0;
    bool                normDetect = false;

    if (!filesystem::exists(paramFile))
    {
        LOG_ERROR("The file [%s] does not exist.\n",paramFile.c_str());
        status = -1;
        return status;
    }

    string model = modelBasePath;
    if (model.back() == '/')
    {
        model.pop_back();
    }

    const YAML::Node    yaml = YAML::LoadFile(paramFile.c_str());

    const YAML::Node   &session = yaml["session"];
    const YAML::Node   &task = yaml["task_type"];
    const YAML::Node   &postProc = yaml["postprocess"];

    /** Validate the parsed yaml configuration and create the configuration
     *  for the inference object creation
     */
    if (!session)
    {
        LOG_WARN("Inference configuration parameters  missing.\n");
        status = -1;
    }
    else if (!postProc)
    {
        LOG_WARN("Postprocess configuration parameters missing.\n");
        status = -1;
    }
    else if (!task)
    {
        LOG_ERROR("Tasktype configuration parameters missing.\n");
        status = -1;
    }

    if (status == 0)
    {
        string taskType = task.as<string>();
        if(taskType == "classification")
        {
            dlPostProcObj.params.task_type = TIVX_DL_POST_PROC_CLASSIFICATION_TASK_TYPE;
        }
        else if(taskType == "detection")
        {
            dlPostProcObj.params.task_type = TIVX_DL_POST_PROC_DETECTION_TASK_TYPE;
        }
        else if(taskType == "segmentation")
        {
            dlPostProcObj.params.task_type = TIVX_DL_POST_PROC_SEGMENTATION_TASK_TYPE;
        }
        else
        {
            LOG_ERROR("Invalid task type defined! \n");
        }

        for(int32_t i=0; i<6; i++)
        {
            dlPostProcObj.params.od_prms.formatter[i] = i;
        }

        if ( (postProc["formatter"]) && (postProc["formatter"]["src_indices"]) )
        {
            const YAML::Node &formatterNode = postProc["formatter"]["src_indices"];

            /* default is assumed to be [0 1 2 3 4 5] which means
             * [x1y1 x2y2 label score].
             *
             * CASE 1: Only 2 values are specified. These are assumed to
             *         be "label" and "score". Keep [0..3] same as the default
             *         values but overwrite [4,5] with these two values.
             *
             * CASE 2: Only 4 values are specified. These are assumed to
             *         be "x1y1" and "x2y2". Keep [4,5] same as the default
             *         values but overwrite [0..3] with these four values.
             *
             * CASE 3: All 6 values are specified. Overwrite the defaults.
             *
             */
            if (formatterNode.size() == 2)
            {
                dlPostProcObj.params.od_prms.formatter[4] = formatterNode[0].as<int32_t>();
                dlPostProcObj.params.od_prms.formatter[5] = formatterNode[1].as<int32_t>();
            }
            else if ( (formatterNode.size() == 6) ||
                      (formatterNode.size() == 4) )
            {
                for (uint8_t i = 0; i < formatterNode.size(); i++)
                {
                    dlPostProcObj.params.od_prms.formatter[i] = formatterNode[i].as<int32_t>();
                }
            }
            else
            {
                LOG_ERROR("formatter specification incorrect.\n");
                status = -1;
            }
        }

        if (postProc["normalized_detections"])
        {
            normDetect = postProc["normalized_detections"].as<bool>();
        }

        if (normDetect)
        {
            dlPostProcObj.params.od_prms.scaleX = static_cast<float>(imageWidth);
            dlPostProcObj.params.od_prms.scaleY = static_cast<float>(imageHeight);
        }
        else
        {
            dlPostProcObj.params.od_prms.scaleX = static_cast<float>(imageWidth) /
                                                  (ioBufDesc->inWidth[0] +
                                                   ioBufDesc->inPadL[0]  +
                                                   ioBufDesc->inPadR[0]);

            dlPostProcObj.params.od_prms.scaleY = static_cast<float>(imageHeight) /
                                                  (ioBufDesc->inHeight[0] +
                                                   ioBufDesc->inPadT[0]   +
                                                   ioBufDesc->inPadB[0]);
        }

        const YAML::Node   &metric = yaml["metric"];

        if ( (metric) && (metric["label_offset_pred"]) )
        {
            // Read the width and height values
            const YAML::Node &offset = metric["label_offset_pred"];

            if (offset.Type() == YAML::NodeType::Scalar)
            {
                /* Use "0" key to store the value. */
                dlPostProcObj.params.oc_prms.labelOffset = offset.as<int32_t>();
            }
            else if (offset.Type() == YAML::NodeType::Map)
            {
                bool first_iter = true;
                int32_t cntr = 0;
                for (const auto& it : offset)
                {
                    if (it.second.Type() == YAML::NodeType::Scalar)
                    {
                        if(first_iter)
                        {
                            dlPostProcObj.params.od_prms.labelIndexOffset = it.first.as<int32_t>();
                            first_iter = false;
                        }
                        dlPostProcObj.params.od_prms.labelOffset[cntr] = it.second.as<int32_t>();
                        cntr++;
                    }
                }
            }
            else
            {
                LOG_ERROR("label_offset_pred specification incorrect.\n");
                status = -1;
            }
        }

        if(taskType == "classification")
        {
            dlPostProcObj.params.oc_prms.ioBufDesc = ioBufDesc;
            dlPostProcObj.params.oc_prms.num_top_results = 5;
            getClassNames(modelBasePath, dlPostProcObj.params.oc_prms.classnames);
        }
        else if(taskType == "detection")
        {
            dlPostProcObj.params.od_prms.ioBufDesc = ioBufDesc;
            dlPostProcObj.params.od_prms.viz_th = 0.5;
            getClassNames(modelBasePath, dlPostProcObj.params.od_prms.classnames);
        }
        else if(taskType == "segmentation")
        {
            mMaxColorClass = sizeof(RGB_COLOR_MAP)/sizeof(RGB_COLOR_MAP[0]);
            mYUVColorMap = new uint8_t*[mMaxColorClass];
            for(int32_t i = 0; i < mMaxColorClass; ++i)
            {
                mYUVColorMap[i] = new uint8_t[3];
                uint8_t R = RGB_COLOR_MAP[i][0];
                uint8_t G = RGB_COLOR_MAP[i][1];
                uint8_t B = RGB_COLOR_MAP[i][2];
                mYUVColorMap[i][0] = RGB2Y(R,G,B);
                mYUVColorMap[i][1] = RGB2U(R,G,B);
                mYUVColorMap[i][2] = RGB2V(R,G,B);
            }

            dlPostProcObj.params.ss_prms.ioBufDesc = ioBufDesc;
            dlPostProcObj.params.ss_prms.inDataWidth = ioBufDesc->inWidth[0]   +
                                                       ioBufDesc->inPadL[0]    +
                                                       ioBufDesc->inPadR[0];

            dlPostProcObj.params.ss_prms.inDataHeight = ioBufDesc->inHeight[0] +
                                                        ioBufDesc->inPadT[0]   +
                                                        ioBufDesc->inPadB[0];
            dlPostProcObj.params.ss_prms.alpha = 0.4;
            dlPostProcObj.params.ss_prms.YUVColorMap = mYUVColorMap;
            dlPostProcObj.params.ss_prms.MaxColorClass = mMaxColorClass;
        }
        
        dlPostProcObj.params.num_input_tensors = ioBufDesc->numOutputBuf;
        dlPostProcObj.num_input_tensors = ioBufDesc->numOutputBuf;
        dlPostProcObj.num_channels = 1;
        dlPostProcObj.input_image.bufq_depth = 1;
        dlPostProcObj.input_image.color_format = VX_DF_IMAGE_NV12;

        dlPostProcObj.input_image.width = imageWidth;
        dlPostProcObj.input_image.height = imageHeight;

        for(uint32_t i=0; i<dlPostProcObj.num_input_tensors; i++)
        {
            dlPostProcObj.input_tensor[i].bufq_depth = 1;
            dlPostProcObj.input_tensor[i].datatype = getTensorDataType(ioBufDesc->outElementType[i]);
            dlPostProcObj.input_tensor[i].num_dims = 3;
            
            dlPostProcObj.input_tensor[i].dim_sizes[0] = ioBufDesc->outWidth[i]  +
                                                         ioBufDesc->outPadL[i]   +
                                                         ioBufDesc->outPadR[i];

            dlPostProcObj.input_tensor[i].dim_sizes[1] = ioBufDesc->outHeight[i] +
                                                         ioBufDesc->outPadT[i]   +
                                                         ioBufDesc->outPadB[i];

            dlPostProcObj.input_tensor[i].dim_sizes[2] = ioBufDesc->outNumChannels[i];
        }

        dlPostProcObj.en_out_image_write = 0;
        dlPostProcObj.output_image.bufq_depth = 1;
        dlPostProcObj.output_image.color_format = VX_DF_IMAGE_NV12;

        dlPostProcObj.output_image.width = imageWidth;
        dlPostProcObj.output_image.height = imageHeight;

    }

    return status;
}

}