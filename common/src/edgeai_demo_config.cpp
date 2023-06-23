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
/* Standard headers. */
#include <algorithm>
#include <filesystem>
#include <iostream>

/* Module headers. */
#include <utils/include/ti_stl_helpers.h>
#include <utils/include/ti_logger.h>
#include <common/include/edgeai_utils.h>
#include <common/include/edgeai_demo_config.h>

namespace ti::edgeai::common
{
using namespace std;
using namespace ti::edgeai::common;
using namespace ti::utils;

int32_t InputInfo::m_numInstances = 0;

InputInfo::InputInfo(const YAML::Node &node)
{
    string  srcExt;

    m_instId = m_numInstances++;

    m_source    = node["source"].as<string>();
    m_width     = node["width"].as<int32_t>();
    m_height    = node["height"].as<int32_t>();

    if (node["framerate"])
    {
        m_framerate = node["framerate"].as<string>();

        /* Change framerate to string representation of fraction. 0.5 = "1/2". */
        m_framerate = to_fraction(m_framerate);
    }

    if (node["camera-id"])
    {
        m_cameraId = node["camera-id"].as<int32_t>();
    }

    if (node["format"])
    {
        m_format = node["format"].as<string>();
    }

    if (node["loop"])
    {
        m_loop = node["loop"].as<bool>();
    }

    if (node["subdev-id"])
    {
        m_subdev_id = node["subdev-id"].as<int32_t>();
    }

    if (node["ldc"])
    {
        m_ldc = node["ldc"].as<bool>();
    }

    if (node["sen-id"])
    {
        m_sen_id = node["sen-id"].as<string>();
    }

    srcExt = filesystem::path(m_source).extension();

    if(m_source == "camera")
    {
        m_srcType = "camera";
    }
    else if (filesystem::exists(m_source))
    {
        if(filesystem::is_directory(m_source))
        {
            m_srcType = "images_directory";
            for(const auto &filename : filesystem::directory_iterator(m_source))
            {
                string file(filename.path());
                string imgExt = filesystem::path(file).extension();

                if(imgExt == ".nv12" || imgExt == ".yuv")
                {
                    m_multiImageVect.push_back(file);
                }
            }
            if (m_multiImageVect.size() == 0)
            {
                LOG_ERROR("Directory [%s] does not have supported images [.nv12,.yuv]\n", m_source.c_str());
                throw runtime_error("Invalid source.\n");
            }

            std::sort(m_multiImageVect.begin(),m_multiImageVect.end(),[](const string & a, const string & b) -> bool
            {
                return a < b;
            });
        }
        else if (srcExt == ".nv12" || srcExt == ".yuv")
        {
            m_srcType = "image";
        }
        else
        {
            LOG_ERROR("Invalid source.\n");
            throw runtime_error("Invalid source.\n");
        }
    }
    else
    {
        LOG_ERROR("Invalid source.\n");
        throw runtime_error("Invalid source.\n");
    }

}

void InputInfo::dumpInfo(const char *prefix) const
{
    LOG_INFO("%sInputInfo::source        = %s\n", prefix, m_source.c_str());
    LOG_INFO("%sInputInfo::width         = %d\n", prefix, m_width);
    LOG_INFO("%sInputInfo::height        = %d\n", prefix, m_height);
    LOG_INFO("%sInputInfo::framerate     = %s\n", prefix, m_framerate.c_str());
}

InputInfo::~InputInfo()
{
    LOG_DEBUG("DESTRUCTOR\n");
}

int32_t OutputInfo::m_numInstances = 0;

OutputInfo::OutputInfo(const YAML::Node    &node,
                       const string        &title):
    m_title(title)
{
    string sinkExt;

    m_instId = m_numInstances++;

    m_sink   = node["sink"].as<string>();
    m_width  = node["width"].as<int32_t>();
    m_height = node["height"].as<int32_t>();

    if (node["connector"])
    {
        m_connector = node["connector"].as<int32_t>();
    }

    sinkExt = filesystem::path(m_sink).extension();

    if (sinkExt == ".nv12" || sinkExt == ".yuv")
    {
        m_sinkType = "image";
    }
    else if(filesystem::is_directory(m_sink))
    {
        m_sinkType = "images_directory";
    }
    else if(m_sink == "display")
    {
        m_sinkType = "display";
    }
    else
    {
        LOG_ERROR("Invalid sink.\n");
        throw runtime_error("Invalid sink.\n");
    }

    LOG_DEBUG("CONSTRUCTOR\n");
}

void OutputInfo::dumpInfo(const char *prefix) const
{
    LOG_INFO("%sOutputInfo::sink         = %s\n", prefix, m_sink.c_str());
    LOG_INFO("%sOutputInfo::width        = %d\n", prefix, m_width);
    LOG_INFO("%sOutputInfo::height       = %d\n", prefix, m_height);
    LOG_INFO("%sOutputInfo::connector    = %d\n", prefix, m_connector);

    LOG_INFO_RAW("\n");
}

int32_t OutputInfo::registerDispParams(const MosaicInfo  *mosaicInfo,
                                       const string      &modelTitle)
{
    int32_t             status = 0;
    int32_t             id;

    if (!mosaicInfo->m_mosaicEnabled || !m_mosaicEnabled)
    {
        if (m_mosaicCnt == 0)
        {
            m_mosaicEnabled = false;
        }
        else
        {
            LOG_ERROR("Need mosaic to support multiple subflow with same output\n");
            status = -1;
        }
    }

    if (status ==0)
    {
        id = m_mosaicCnt++;

        /* Store the mosaic information in the map. */
        m_instIdMosaicMap[id] = mosaicInfo;

        m_titleMap.push_back(modelTitle);
    }
    else
    {
        id = status;
    }

    return id;
}


OutputInfo::~OutputInfo()
{
    LOG_DEBUG("DESTRUCTOR\n");
}

MosaicInfo::MosaicInfo(vector<int> data)
{
    m_posX     = data[0];
    m_posY     = data[1];
    m_width    = data[2];
    m_height   = data[3];
    m_mosaicEnabled = true;

    LOG_DEBUG("CONSTRUCTOR\n");
}

MosaicInfo::MosaicInfo()
{
    m_width  = 1;
    m_height = 1;
    m_posX   = 0;
    m_posY   = 0;
    m_mosaicEnabled = false;

    LOG_DEBUG("CONSTRUCTOR\n");
}

void MosaicInfo::dumpInfo(const char *prefix) const
{
    LOG_INFO("%sMosaicInfo::width        = %d\n", prefix, m_width);
    LOG_INFO("%sMosaicInfo::height       = %d\n", prefix, m_height);
    LOG_INFO("%sMosaicInfo::pos_x        = %d\n", prefix, m_posX);
    LOG_INFO("%sMosaicInfo::pos_y        = %d\n", prefix, m_posY);
    LOG_INFO_RAW("\n");
}

MosaicInfo::~MosaicInfo()
{
    LOG_DEBUG("DESTRUCTOR\n");
}

ModelInfo::ModelInfo(const YAML::Node &node)
{
    m_modelPath  = node["model_path"].as<string>();

    if (node["alpha"])
    {
        m_alpha = node["alpha"].as<float>();
    }

    if (node["viz_threshold"])
    {
        m_vizThreshold = node["viz_threshold"].as<float>();
    }

    if (node["topN"])
    {
        m_topN = node["topN"].as<int32_t>();
    }

    LOG_DEBUG("CONSTRUCTOR\n");
}

void ModelInfo::dumpInfo(const char *prefix) const
{
    LOG_INFO("%sModelInfo::modelPath     = %s\n", prefix, m_modelPath.c_str());
    LOG_INFO("%sModelInfo::vizThreshold  = %f\n", prefix, m_vizThreshold);
    LOG_INFO("%sModelInfo::alpha         = %f\n", prefix, m_alpha);
    LOG_INFO("%sModelInfo::topN          = %d\n", prefix, m_topN);
    LOG_INFO_RAW("\n");
}

ModelInfo::~ModelInfo()
{
    LOG_DEBUG("DESTRUCTOR\n");
}

int32_t FlowInfo::m_numInstances = 0;

FlowInfo::FlowInfo(FlowConfig &flowConfig)
{
    m_instId = m_numInstances++;

    m_inputId = flowConfig.input;

    m_subFlowConfigs = flowConfig.subflow_configs;

    for (auto const &s : m_subFlowConfigs)
    {
        m_modelIds.push_back(s.model);
        m_outputIds.push_back(s.output);
    }

    LOG_DEBUG("CONSTRUCTOR\n");
}

int32_t FlowInfo::initialize(map<string, ModelInfo*>   &modelMap,
                             map<string, InputInfo*>   &inputMap,
                             map<string, OutputInfo*>  &outputMap,
                             bool                       isMultiCam)
{
    auto                          &inputInfo = inputMap[m_inputId];
    int32_t                       cnt = 0;
    string                        flowStr = "flow" + to_string(m_instId);
    int32_t                       status = 0;

    /* Set up the flows. */
    for (auto &s : m_subFlowConfigs)
    {

        ModelInfo          *model = modelMap[s.model];
        string              inputName;
        int32_t             mosaicWidth{0};
        int32_t             mosaicHeight{0};

        string modelName = model->m_modelPath;

        if (modelName.back() == '/')
        {
            modelName.pop_back();
        }

        modelName = std::filesystem::path(modelName).filename();

        m_mosaicVec.clear();
        if (s.mosaic_info.size() <= 0)
        {
            m_mosaicVec.push_back(new MosaicInfo());
        }
        else
        {
            m_mosaicVec.push_back(new MosaicInfo(s.mosaic_info));
        }

        auto &output     =  s.output;
        auto &outputInfo =  outputMap[output];
        auto &mosaicInfo =  m_mosaicVec[0];
        /* Create the instanceId to Mosaic info mapping. */
        mosaicInfo->m_outputInfo = outputInfo;
        if (!mosaicInfo->m_mosaicEnabled)
        {
            mosaicInfo->m_width = outputInfo->m_width;
            mosaicInfo->m_height = outputInfo->m_height;
        }

        /* Add some necessary checks. */
        if (mosaicInfo->m_width <= 0)
        {
            LOG_ERROR("Invalid mosaic width [%d].\n", mosaicInfo->m_width);
            status = -1;
            break;
        }

        if (mosaicInfo->m_height <= 0)
        {
            LOG_ERROR("Invalid mosaic height [%d].\n", mosaicInfo->m_height);
            status = -1;
            break;
        }

        if (mosaicInfo->m_width > inputInfo->m_width ||
            mosaicInfo->m_height > inputInfo->m_height)
        {
            LOG_ERROR("Flow output resolution cannot be "
                        "greater than input resolution.\n");
            status = -1;
            break;
        }

        if (mosaicInfo->m_mosaicEnabled)
        {
            if ((mosaicInfo->m_posX + mosaicInfo->m_width) > outputInfo->m_width)
            {
                LOG_ERROR("Mosaic (posX + width) [%d + %d] exceeds "
                            "Output width [%d] .\n",
                            mosaicInfo->m_width,
                            mosaicInfo->m_posX,
                            outputInfo->m_width);
                status = -1;
                break;
            }
            if((mosaicInfo->m_posY + mosaicInfo->m_height) > outputInfo->m_height)
            {
                LOG_ERROR("Mosaic (posY + height) [%d + %d] exceeds "
                            "Output height [%d] .\n",
                            mosaicInfo->m_height,
                            mosaicInfo->m_posY,
                            outputInfo->m_height);
                status = -1;
                break;
            }
        }
        /* Check ends here. */

        if (mosaicInfo->m_width > mosaicWidth)
        {
            mosaicWidth = mosaicInfo->m_width;
        }
        if (mosaicInfo->m_height > mosaicHeight)
        {
            mosaicHeight = mosaicInfo->m_height;
        }

        if (status == -1)
        {
            break;
        }

        if(inputInfo->m_source == "camera" && isMultiCam)
        { 
            m_mosaicInfoVec.push_back({mosaicInfo->m_posX, mosaicInfo->m_posY,
                                        mosaicWidth, mosaicHeight,
                                        outputInfo->m_width, outputInfo->m_height, 1});
        }
        else
        { 
            m_mosaicInfoVec.push_back({mosaicInfo->m_posX, mosaicInfo->m_posY,
                                        mosaicWidth, mosaicHeight,
                                        outputInfo->m_width, outputInfo->m_height, 0});
        }

        vector <OutputInfo *> outputs;
        auto    out = mosaicInfo->m_outputInfo;
        outputs.push_back(out);

        /* Increment the model count. */
        cnt++;
    }

    return status;
}

void FlowInfo::dumpInfo(const char *prefix) const
{
    LOG_INFO("%sFlowInfo::input          = %s\n", prefix, m_inputId.c_str());

    LOG_INFO("%sFlowInfo::modelIds       = [ ", prefix);

    for (auto const &m : m_modelIds)
    {
        LOG_INFO_RAW("%s ", m.c_str());
    }

    LOG_INFO_RAW("]\n");

    LOG_INFO("%sFlowInfo::outputIds      = [ ", prefix);

    for (auto const &o : m_outputIds)
    {
        LOG_INFO_RAW("%s ", o.c_str());
    }

    LOG_INFO_RAW("]\n");

    LOG_INFO("%sFlowInfo::mosaic        =\n", prefix);
    for (auto const &d: m_mosaicVec)
    {
        d->dumpInfo("\t\t");
    }
}

FlowInfo::~FlowInfo()
{
    LOG_DEBUG("DESTRUCTOR\n");
    DeleteVec(m_mosaicVec);
}

int32_t DemoConfig::parse(const YAML::Node &yaml)
{
    int32_t status = 0;
    auto const &title = yaml["title"].as<string>();

    for (auto &n : yaml["flows"])
    {
        const string &flow_name = n.first.as<string>();
        auto         &subflow = n.second;

        /* Parse input information. */
        const string &input = subflow[0].as<string>();
        if (m_inputMap.find(input) == m_inputMap.end())
        {
            /* Validate Input */
            if (!yaml["inputs"][input])
            {
                LOG_ERROR("[%s] Invalid input [%s] specified.\n",
                          flow_name.c_str(), input.c_str());
                status = -1;
                break;
            }
            m_inputMap[input] = new InputInfo(yaml["inputs"][input]);
            m_inputMap[input]->m_name = input;
            m_inputOrder.push_back(input);
        }

        /* Parse model information. */
        const string &model = subflow[1].as<string>();
        if (m_modelMap.find(model) == m_modelMap.end())
        {
            /* Validate Model */
            if (!yaml["models"][model])
            {
                LOG_ERROR("[%s] Invalid Model [%s] specified.\n",
                          flow_name.c_str(), model.c_str());
                status = -1;
                break;
            }
            m_modelMap[model] = new ModelInfo(yaml["models"][model]);
        }

        /* Parse output information. */
        const string &output = subflow[2].as<string>();
        if (m_outputMap.find(output) == m_outputMap.end())
        {
            /* Validate Output */
            if (!yaml["outputs"][output])
            {
                LOG_ERROR("[%s] Invalid Output [%s] specified.\n",
                          flow_name.c_str(), output.c_str());
                status = -1;
                break;
            }

            m_outputMap[output] = new OutputInfo(yaml["outputs"][output],title);

            /* Validate Dimension */
            if (m_outputMap[output]->m_width <= 0)
            {
                LOG_ERROR("Invalid Output width [%d].\n",
                          m_outputMap[output]->m_width);
                status = -1;
                break;
            }

            if (m_outputMap[output]->m_height <= 0)
            {
                LOG_ERROR("Invalid Output height [%d].\n",
                          m_outputMap[output]->m_height);
                status = -1;
                break;
            }
        }
    }
    if (status == 0)
    {
        /* Parse flow information. */
        status = parseFlowInfo(yaml);
    }

    return status;
}

DemoConfig::DemoConfig()
{
    LOG_DEBUG("CONSTRUCTOR\n");
}

void DemoConfig::dumpInfo() const
{
    LOG_INFO_RAW("\n");

    LOG_INFO("DemoConfig::Inputs:\n");
    for (auto &[name, obj]: m_inputMap)
    {
        obj->dumpInfo("\t");
    }

    LOG_INFO("DemoConfig::Models:\n");
    for (auto &[name, obj]: m_modelMap)
    {
        obj->dumpInfo("\t");
    }

    LOG_INFO("DemoConfig::Outputs:\n");
    for (auto &[name, obj]: m_outputMap)
    {
        obj->dumpInfo("\t");
    }

    LOG_INFO("DemoConfig::Flows:\n");
    for (auto &[name, obj]: m_flowMap)
    {
        obj->dumpInfo("\t");
    }

    LOG_INFO_RAW("\n");
}

int32_t DemoConfig::parseFlowInfo(const YAML::Node &config)
{
    int32_t status = 0;
    vector<string> visited;
    const YAML::Node &flows = config["flows"];

    for (auto &input_name: m_inputOrder)
    {
        FlowConfig flowConfig;

        flowConfig.input = input_name;
        string flow_name = "";
        for (auto &i : flows)
        {
            const string &s = i.first.as<string>();
            if (find(visited.begin(), visited.end(), s) != visited.end())
            {
                continue;
            }

            string input = flows[s][0].as<string>();
            if (input_name != input)
            {
                continue;
            }

            visited.push_back(s);

            if (flow_name == "")
            {
               flow_name = s;
            }

            string model_name = flows[s][1].as<string>();
            string output     = flows[s][2].as<string>();
            vector<int> mosaic_info{};
            SubFlowConfig subFlowConfig;
            subFlowConfig.model = model_name;
            subFlowConfig.output = output;

            if (flows[s].size() > 3)
            {
                mosaic_info = flows[s][3].as<vector<int>>();
            }
            subFlowConfig.mosaic_info = mosaic_info;

            flowConfig.subflow_configs.push_back(subFlowConfig);      
        }

        auto const &f = new FlowInfo(flowConfig);
        m_flowMap[flow_name] = f;
    }

    return status;
}

DemoConfig::~DemoConfig()
{
    LOG_DEBUG("DESTRUCTOR\n");
    DeleteMap(m_inputMap);
    DeleteMap(m_modelMap);
    DeleteMap(m_outputMap);
    DeleteMap(m_flowMap);
}

} // namespace ti::edgeai::common

