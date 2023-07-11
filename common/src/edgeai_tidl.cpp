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
#include <cstdio>
#include <iostream>
#include <filesystem>

/* Third-party headers. */
#include <yaml-cpp/yaml.h>

/* Module headers. */
#include <common/include/edgeai_tidl.h>
#include <utils/include/ti_logger.h>
#include <common/include/edgeai_utils.h>

namespace ti::edgeai::common
{

#define MAX_NUM_OF_TIDL_OUTPUT_TENSORS 8

using namespace ti::utils;

/* Helper function to check if string ends with particular substring. */
static inline bool ends_with(std::string const &value, std::string const &ending)
{
    if (ending.size() > value.size())
        return false;

    return std::equal(ending.rbegin(), ending.rend(), value.rbegin());
}

tidlInf::tidlInf()
{
    LOG_DEBUG("tidlInf CONSTRUCTOR\n");
}

tidlInf::~tidlInf()
{
    LOG_DEBUG("tidlInf DESTRUCTOR\n");
}

int32_t tidlInf::getConfig(const string &modelBasePath, vx_context context)
{
    const string        &artifacts_dir_path = modelBasePath + "/artifacts/" ;      
    string              io_config_file_path = "";
    int32_t             status = 0;

    vx_map_id           map_id_config;
    tivxTIDLJ7Params    *tidlParams;

    for(const auto &filename : filesystem::directory_iterator(artifacts_dir_path))
    {
        string x(filename.path());
        if(ends_with(x, "_tidl_io_1.bin"))
        {
            io_config_file_path = x;
        }

        if(ends_with(x, "_tidl_net.bin"))
        {
            network_file_path = x;
        }
    }

    if (!filesystem::exists(io_config_file_path))
    {
        LOG_ERROR("The file [%s] does not exist.\n",io_config_file_path.c_str());
        status = -1;
        return status;
    }

    if (!filesystem::exists(network_file_path))
    {
        LOG_ERROR("The file [%s] does not exist.\n",network_file_path.c_str());
        status = -1;
        return status;
    }

    tidlObj.config_file_path = const_cast<vx_char*>(io_config_file_path.c_str());
    tidlObj.config = readConfig(context);
    status = vxGetStatus((vx_reference) tidlObj.config);

    if(status != 0)
    {
        LOG_ERROR("Read Config failed.\n");
        status = -1;
        return status;
    }

    tidlObj.network_file_path = const_cast<vx_char*>(network_file_path.c_str());
    tidlObj.num_cameras = 1;
    tidlObj.en_out_tensor_write = 0;
    tidlObj.input[0].bufq_depth = 1;

    if(tidlObj.num_output_tensors < MAX_NUM_OF_TIDL_OUTPUT_TENSORS)
    {
        for(uint32_t i=0; i < tidlObj.num_output_tensors; i++)
        {
            tidlObj.output[i].bufq_depth = 1;
        }
    }

    /* Extracting IO Buf Descriptor from config */
    vxMapUserDataObject(tidlObj.config,
                        0,
                        sizeof(tivxTIDLJ7Params),
                        &map_id_config,
                        (void **) &tidlParams,
                        VX_READ_ONLY,
                        VX_MEMORY_TYPE_HOST,
                        0);
    ioBufDesc = (sTIDL_IOBufDesc_t *) &tidlParams->ioBufDesc;
    vxUnmapUserDataObject(tidlObj.config, map_id_config);

    return status;
}

vx_user_data_object tidlInf::readConfig(vx_context context)
{
    vx_status status = VX_SUCCESS;

    vx_user_data_object     config = NULL;
    tivxTIDLJ7Params        *tidlParams;
    vx_uint32               capacity;
    vx_map_id               map_id;
    vx_size                 read_count;

    FILE *fp_config;

    fp_config = fopen(tidlObj.config_file_path, "rb");

    if(fp_config == NULL)
    {
        LOG_ERROR("Unable to open file! %s \n", tidlObj.config_file_path);
        return NULL;
    }

    fseek(fp_config, 0, SEEK_END);
    capacity = ftell(fp_config);
    fseek(fp_config, 0, SEEK_SET);

    if( capacity != sizeof(sTIDL_IOBufDesc_t))
    {
        LOG_ERROR("Config file size (%d bytes) does not match "
                  "size of sTIDL_IOBufDesc_t (%d bytes)\n",
                  capacity,
                  (vx_uint32)sizeof(sTIDL_IOBufDesc_t));
        fclose(fp_config);
        return NULL;
    }

    status = vxGetStatus((vx_reference)context);

    if(VX_SUCCESS == status)
    {
        config = vxCreateUserDataObject(context,
                                        "tivxTIDLJ7Params",
                                        sizeof(tivxTIDLJ7Params),
                                        NULL );
        status = vxGetStatus((vx_reference)config);

        if (VX_SUCCESS == status)
        {
            vxMapUserDataObject(config,
                                0,
                                sizeof(tivxTIDLJ7Params),
                                &map_id,
                                (void **) &tidlParams,
                                VX_WRITE_ONLY,
                                VX_MEMORY_TYPE_HOST,
                                0);

            if(tidlParams == NULL)
            {
                LOG_ERROR("Map of config object failed\n");
                fclose(fp_config);
                return NULL;
            }

            tivx_tidl_j7_params_init(tidlParams);

            ioBufDesc = (sTIDL_IOBufDesc_t *) &tidlParams->ioBufDesc;

            read_count = fread(ioBufDesc, capacity, 1, fp_config);
            if(read_count != 1)
            {
                LOG_ERROR("Unable to read file!\n");
            }

            tidlObj.num_input_tensors  = ioBufDesc->numInputBuf;
            tidlObj.num_output_tensors = ioBufDesc->numOutputBuf;

            tidlParams->compute_config_checksum  = 0;
            tidlParams->compute_network_checksum = 0;

            vxUnmapUserDataObject(config, map_id);
        }
    }
    fclose(fp_config);

    return config;
}

void tidlInf::dumpInfo()
{
    LOG_INFO("ioBufDesc->inDataFormat[0] == %d, "
             "if 1 then BGR, 0 then RGB \n\n",
             ioBufDesc->inDataFormat[0]);

    LOG_INFO("input data type = %ld \n",
             getTensorDataType(ioBufDesc->inElementType[0]));

    LOG_INFO("Input tensor dimensions \n");

    LOG_INFO("ioBufDesc->inWidth[0] + ioBufDesc->inPadL[0] + "
             "ioBufDesc->inPadR[0] = %d \n",
             ioBufDesc->inWidth[0] + ioBufDesc->inPadL[0] + ioBufDesc->inPadR[0]);

    LOG_INFO("ioBufDesc->inHeight[0] + ioBufDesc->inPadT[0] + "
             "ioBufDesc->inPadB[0] = %d \n",
             ioBufDesc->inHeight[0] + ioBufDesc->inPadT[0] + ioBufDesc->inPadB[0]);

    LOG_INFO("ioBufDesc->inNumChannels[0] = %d \n\n", ioBufDesc->inNumChannels[0]);

    LOG_INFO("ioBufDesc->numInputBuf = %d \n", ioBufDesc->numInputBuf);

    LOG_INFO("ioBufDesc->numOutputBuf = %d \n\n", ioBufDesc->numOutputBuf);

    LOG_INFO("Output details\n");
    for(int32_t j=0;j<ioBufDesc->numOutputBuf;j++)
    {
        LOG_INFO("Output data type %d = %ld \n\n",
                j,
                getTensorDataType(ioBufDesc->outElementType[j]));

        LOG_INFO("Output tensor dimensions \n");

        LOG_INFO("ioBufDesc->outWidth[%d] + ioBufDesc->outPadL[%d] + "
                 "ioBufDesc->outPadR[%d] = %d \n",
                 j,
                 j, 
                 j, 
                 ioBufDesc->outWidth[j] + ioBufDesc->outPadL[j] + ioBufDesc->outPadR[j]);

        LOG_INFO("ioBufDesc->outHeight[%d] + ioBufDesc->outPadT[%d] + "
                 "ioBufDesc->outPadB[%d] = %d \n",
                 j,
                 j,
                 j,
                 ioBufDesc->outHeight[j] + ioBufDesc->outPadT[j] + ioBufDesc->outPadB[j]);

        LOG_INFO("ioBufDesc->outNumChannels[%d] = %d \n\n",
                 j,
                 ioBufDesc->outNumChannels[j]);
    }
}

} // namespace ti::edgeai::common