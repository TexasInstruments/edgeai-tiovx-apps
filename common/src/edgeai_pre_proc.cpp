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

/* Third-party headers. */
#include <yaml-cpp/yaml.h>

/* Module headers */
#include <common/include/edgeai_pre_proc.h>
#include <utils/include/ti_logger.h>
#include <common/include/edgeai_utils.h>

namespace ti::edgeai::common
{
using namespace ti::utils;

preProc::preProc()
{
    LOG_DEBUG("preProc CONSTRUCTOR\n");
}

preProc::~preProc()
{
    LOG_DEBUG("preProc DESTRUCTOR\n");
}

int32_t preProc::getConfig(const string         &modelBasePath,
                           sTIDL_IOBufDesc_t    *ioBufDesc)
{
    const string        &paramFile = modelBasePath + "/param.yaml";
    int32_t             status = 0;

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
    const YAML::Node   &preProc = yaml["preprocess"];

    /** Validate the parsed yaml configuration and create the configuration
     *  for the inference object creation
     */
    if (!preProc)
    {
        LOG_ERROR("Preprocess configuration parameters missing.\n");
        status = -1;
    }
    else if (!session["input_mean"])
    {
        LOG_ERROR("Mean value specification missing.\n");
        status = -1;
    }
    else if (!session["input_scale"])
    {
        LOG_ERROR("Scale value specification missing.\n");
        status = -1;
    }
    else if (!session["input_details"])
    {
        LOG_ERROR("Input tensor details missing.\n");
        status = -1;
    }
    else if (!preProc["data_layout"])
    {
        LOG_ERROR("Data layout specification missing.\n");
        status = -1;
    }
    else if (!preProc["resize"])
    {
        LOG_ERROR("Resize specification missing.\n");
        status = -1;
    }
    else if (!preProc["crop"])
    {
        LOG_ERROR("Crop specification missing.\n");
        status = -1;
    }
    /* Check if crop information exists. */
    if (status == 0)
    {
        // Read the width and height values
        const YAML::Node &cropNode = preProc["crop"];

        if (cropNode.Type() == YAML::NodeType::Sequence)
        {
            dlPreProcObj.input.width = cropNode[0].as<int32_t>();
            dlPreProcObj.input.height = cropNode[1].as<int32_t>();

            cropHeight = cropNode[0].as<int32_t>();
            cropWidth  = cropNode[1].as<int32_t>();
        }
        else if (cropNode.Type() == YAML::NodeType::Scalar)
        {
            dlPreProcObj.input.width = cropNode.as<int32_t>();
            dlPreProcObj.input.height = cropNode.as<int32_t>();

            cropHeight = cropNode.as<int32_t>();
            cropWidth  = cropNode.as<int32_t>();
        }

        // Read the width and height values
        const YAML::Node &resizeNode = preProc["resize"];

        if (resizeNode.Type() == YAML::NodeType::Sequence)
        {
            resizeHeight = resizeNode[0].as<int32_t>();
            resizeWidth  = resizeNode[1].as<int32_t>();
        }
        else if (resizeNode.Type() == YAML::NodeType::Scalar)
        {
            resizeHeight = resizeNode.as<int32_t>();
            resizeWidth  = resizeHeight;
        }

        // Read the data layout
        if(preProc["data_layout"].as<std::string>() == "NCHW")
        {
            dlPreProcObj.params.channel_order = 0;
        }
        else
        {
            dlPreProcObj.params.channel_order = 1;
        }

        if(preProc["reverse_channels"])
        {
            dlPreProcObj.params.tensor_format = preProc["reverse_channels"].as<bool>();
        }
        else
        {
            dlPreProcObj.params.tensor_format = 0;
        }
        

        // Read the mean values
        const YAML::Node &meanNode = session["input_mean"];
        if (!meanNode.IsNull())
        {
            for (uint32_t i = 0; i < meanNode.size(); i++)
            {
                dlPreProcObj.params.mean[i] = meanNode[i].as<float>();
            }
        }
        else
        {
            dlPreProcObj.params.mean[0] = 0.0;
            dlPreProcObj.params.mean[1] = 0.0;
            dlPreProcObj.params.mean[2] = 0.0;
        }

        // Read the scale values
        const YAML::Node &scaleNode = session["input_scale"];
        if (!scaleNode.IsNull())
        {
            for (uint32_t i = 0; i < scaleNode.size(); i++)
            {
                dlPreProcObj.params.scale[i] = scaleNode[i].as<float>();
            }
        }
        else
        {
            dlPreProcObj.params.scale[0] = 1.0;
            dlPreProcObj.params.scale[1] = 1.0;
            dlPreProcObj.params.scale[2] = 1.0;
        }

        dlPreProcObj.params.crop[0] = 0;
        dlPreProcObj.params.crop[1] = 0;
        dlPreProcObj.params.crop[2] = 0;
        dlPreProcObj.params.crop[3] = 0;

        dlPreProcObj.num_channels = 1;
        dlPreProcObj.input.bufq_depth = 1;
        dlPreProcObj.input.color_format = VX_DF_IMAGE_NV12;

        /* TIDL node input is NCHW so output tensor dimensions */
        if(dlPreProcObj.params.channel_order == 0)
        {
            dlPreProcObj.output.dim_sizes[0] = ioBufDesc->inWidth[0]  +
                                               ioBufDesc->inPadL[0]   +
                                               ioBufDesc->inPadR[0];

            dlPreProcObj.output.dim_sizes[1] = ioBufDesc->inHeight[0] +
                                               ioBufDesc->inPadT[0]   +
                                               ioBufDesc->inPadB[0];

            dlPreProcObj.output.dim_sizes[2] = ioBufDesc->inNumChannels[0];
        }
        else /* TIDL node input is NHWC so output tensor dimensions */
        {
            dlPreProcObj.output.dim_sizes[0] = ioBufDesc->inNumChannels[0];

            dlPreProcObj.output.dim_sizes[1] = ioBufDesc->inWidth[0]  +
                                               ioBufDesc->inPadL[0]   +
                                               ioBufDesc->inPadR[0];

            dlPreProcObj.output.dim_sizes[2] = ioBufDesc->inHeight[0] +
                                               ioBufDesc->inPadT[0]   +
                                               ioBufDesc->inPadB[0];
        }

        dlPreProcObj.output.bufq_depth = 1;
        dlPreProcObj.output.num_dims = 3;
        dlPreProcObj.en_out_tensor_write = 0;

        dlPreProcObj.output.datatype = getTensorDataType(ioBufDesc->inElementType[0]);

    }
    return status;
}

}