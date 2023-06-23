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
#include <common/include/edgeai_mosaic.h>
#include <utils/include/ti_logger.h>

namespace ti::edgeai::common
{

using namespace ti::utils;

imgMosaic::imgMosaic()
{
    LOG_DEBUG("imgMosaic CONSTRUCTOR\n");
}


imgMosaic::~imgMosaic()
{
    LOG_DEBUG("imgMosaic DESTRUCTOR\n");
}

int32_t imgMosaic::getConfig(vector<vector<int>> mosaicInfo)
{
    int32_t status = 0;
    uint    i;
    int     cam_input_sel = -1;
    int     cam_channel_sel = 0;
    int     file_input_sel = -1;

    /*  
        mosaicInfo  will contain information about mosaics for
        same output across all the flows 
        Each element of mosaic info is a vector of below format
        <start_x, start_y, window_width, window_height, output_width, output_height, is_multi_cam_input>
    */
    imgMosaicObj.out_width    = mosaicInfo[0][4];
    imgMosaicObj.out_height   = mosaicInfo[0][5];
    imgMosaicObj.out_bufq_depth = 1;
    imgMosaicObj.color_format = VX_DF_IMAGE_NV12;

    imgMosaicObj.num_channels = 1;
    imgMosaicObj.num_inputs   = mosaicInfo.size();
    for (i = 0; i < mosaicInfo.size(); i++)
    {
        imgMosaicObj.inputs[i].width = mosaicInfo[i][2];
        imgMosaicObj.inputs[i].height = mosaicInfo[i][3];
        imgMosaicObj.inputs[i].color_format = VX_DF_IMAGE_NV12;
        imgMosaicObj.inputs[i].bufq_depth = 1;
    }

    tivxImgMosaicParamsSetDefaults(&imgMosaicObj.params);

    imgMosaicObj.params.num_windows  = mosaicInfo.size();
    for (i = 0; i < mosaicInfo.size(); i++)
    {
        imgMosaicObj.params.windows[i].startX  = mosaicInfo[i][0];
        imgMosaicObj.params.windows[i].startY  = mosaicInfo[i][1];
        imgMosaicObj.params.windows[i].width   = mosaicInfo[i][2];
        imgMosaicObj.params.windows[i].height  = mosaicInfo[i][3];
        //imgMosaicObj.params.windows[i].input_select   = i;
        //imgMosaicObj.params.windows[i].channel_select = 0;

        /* 
           is_cam_input - If input is camera then for multi channel, input_select remains same
           but channel select changes
        */
        if(mosaicInfo[i][6] == 1)
        {
            if(cam_input_sel == -1)
                cam_input_sel = i;
            imgMosaicObj.params.windows[i].input_select   = cam_input_sel;
            imgMosaicObj.params.windows[i].channel_select = cam_channel_sel;
            cam_channel_sel++;
        }
        else
        {
            if(file_input_sel == -1)
                file_input_sel = i;
            imgMosaicObj.params.windows[i].input_select   = file_input_sel;
            imgMosaicObj.params.windows[i].channel_select = 0;
            file_input_sel++;
        }
    }

    /* Number of time to clear the output buffer before it gets reused */
    imgMosaicObj.params.clear_count  = 4; // CAPTURE BUFQ DEPTH
    
    return status;
}

}