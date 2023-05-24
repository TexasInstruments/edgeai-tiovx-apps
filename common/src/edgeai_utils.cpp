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
#include <thread>
#include <unistd.h>
#include <ncurses.h>
#include <cmath>

/* Module headers. */
#include <utils/include/ti_logger.h>
#include <common/include/edgeai_utils.h>

/* OpenVX headers */
#include <tiovx_tidl_module.h>

namespace ti::edgeai::common
{
using namespace ti::utils;

vx_size getTensorDataType(vx_int32 tidl_type)
{
    vx_size openvx_type = VX_TYPE_INVALID;

    if (tidl_type == TIDL_UnsignedChar)
    {
        openvx_type = VX_TYPE_UINT8;
    }
    else if(tidl_type == TIDL_SignedChar)
    {
        openvx_type = VX_TYPE_INT8;
    }
    else if(tidl_type == TIDL_UnsignedShort)
    {
        openvx_type = VX_TYPE_UINT16;
    }
    else if(tidl_type == TIDL_SignedShort)
    {
        openvx_type = VX_TYPE_INT16;
    }
    else if(tidl_type == TIDL_UnsignedWord)
    {
        openvx_type = VX_TYPE_UINT32;
    }
    else if(tidl_type == TIDL_SignedWord)
    {
        openvx_type = VX_TYPE_INT32;
    }
    else if(tidl_type == TIDL_SinglePrecFloat)
    {
        openvx_type = VX_TYPE_FLOAT32;
    }

    return openvx_type;
}

void getClassNames(const string &modelBasePath, char (*classnames)[256])
{
    const string        &datasetFile = modelBasePath + "/dataset.yaml";

    if (!filesystem::exists(datasetFile))
    {
        printf("The file [%s] does not exist.\n", datasetFile.c_str());
        return;
    }

    const YAML::Node    yaml = YAML::LoadFile(datasetFile.c_str());

    const YAML::Node   &categories = yaml["categories"];

    // Validate the parsed yaml configuration
    if (!categories)
    {
        printf("Parameter categories missing in dataset file.\n");
        return;
    }

    string     name, none_str;
    int32_t    id;

    strcpy(classnames[0], const_cast<char*>(string("None").c_str()));

    for (YAML::Node data : categories)
    {
        id = data["id"].as<int32_t>();
        name = data["name"].as<std::string>();

        if (data["supercategory"])
        {
            name = data["supercategory"].as<std::string>() + "/" + name;
        }

        strcpy(classnames[id], const_cast<char*>(name.c_str()));
    }
}

const std::string to_fraction(std::string& num)
{
    if(_is_number<int>(num))
    {
        return num+"/1";
    }
    else if(_is_number<double>(num))
    {
        int dec_pos = num.find(".");
        int dec_length = num.length() - dec_pos - 1;
        num.erase(dec_pos, 1);
        int numerator = stoi(num);
        int denom = pow(10,dec_length);
        string fps = to_string(numerator)+"/"+to_string(denom);
        return fps;
    }
    else
    {
        LOG_ERROR("Framerate is not numeric.\n");
        throw runtime_error("Invalid Framerate.");
    }
}

template<typename Numeric>
bool _is_number(const std::string& s)
{
    Numeric n;
    return((std::istringstream(s) >> n >> std::ws).eof());
}

} // namespace ti::edgeai::common

