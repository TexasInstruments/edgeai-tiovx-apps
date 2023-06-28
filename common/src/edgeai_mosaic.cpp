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

extern "C"
{
    #include <edgeai_nv12_drawing_utils.h>
    #include <edgeai_nv12_font_utils.h>
}

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
        m_mosaicInfo  will contain information about mosaics for
        same output across all the flows 
        Each element of mosaic info is a vector of below format
        <start_x, start_y, window_width, window_height, output_width, output_height, is_multi_cam_input>
    */
    m_mosaicInfo = mosaicInfo;
    imgMosaicObj.out_width    = m_mosaicInfo[0][4];
    imgMosaicObj.out_height   = m_mosaicInfo[0][5];
    imgMosaicObj.out_bufq_depth = 1;
    imgMosaicObj.color_format = VX_DF_IMAGE_NV12;

    imgMosaicObj.num_channels = 1;
    imgMosaicObj.num_inputs   = m_mosaicInfo.size();
    for (i = 0; i < m_mosaicInfo.size(); i++)
    {
        imgMosaicObj.inputs[i].width = m_mosaicInfo[i][2];
        imgMosaicObj.inputs[i].height = m_mosaicInfo[i][3];
        imgMosaicObj.inputs[i].color_format = VX_DF_IMAGE_NV12;
        imgMosaicObj.inputs[i].bufq_depth = 1;
    }

    tivxImgMosaicParamsSetDefaults(&imgMosaicObj.params);

    imgMosaicObj.params.num_windows  = m_mosaicInfo.size();
    for (i = 0; i < m_mosaicInfo.size(); i++)
    {
        imgMosaicObj.params.windows[i].startX  = m_mosaicInfo[i][0];
        imgMosaicObj.params.windows[i].startY  = m_mosaicInfo[i][1];
        imgMosaicObj.params.windows[i].width   = m_mosaicInfo[i][2];
        imgMosaicObj.params.windows[i].height  = m_mosaicInfo[i][3];

        /* 
           is_cam_input - If input is camera then for multi channel, input_select remains same
           but channel select changes
        */
        if(m_mosaicInfo[i][6] == 1)
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

void imgMosaic::setBackground(string title, vector<string> mosaicTitle)
{
    vx_rectangle_t              rect;
    vx_imagepatch_addressing_t  image_addr;
    vx_map_id                   map_id;
    vx_uint32                   img_width;
    vx_uint32                   img_height;
    void                        *y_data_ptr;
    void                        *uv_data_ptr;
    Image                       image_holder;
    YUVColor                    title_color;
    YUVColor                    text_color;
    FontProperty                title_font;
    FontProperty                text_font;
    YUVColor                    bg_color;
    uint                        titleX;
    uint                        i;

    vxQueryImage(imgMosaicObj.background_image[0], VX_IMAGE_WIDTH, &img_width, sizeof(vx_uint32));
    vxQueryImage(imgMosaicObj.background_image[0], VX_IMAGE_HEIGHT, &img_height, sizeof(vx_uint32));

    /* Query Y-plane. */
    rect.start_x = 0;
    rect.start_y = 0;
    rect.end_x = img_width;
    rect.end_y = img_height;
    vxMapImagePatch(imgMosaicObj.background_image[0],
                    &rect,
                    0,
                    &map_id,
                    &image_addr,
                    &y_data_ptr,
                    VX_WRITE_ONLY,
                    VX_MEMORY_TYPE_HOST,
                    VX_NOGAP_X);
    vxUnmapImagePatch(imgMosaicObj.background_image[0], map_id);

    /* Query UV-plane. */
    rect.start_x = 0;
    rect.start_y = 0;
    rect.end_x = img_width;
    rect.end_y = img_height;
    vxMapImagePatch(imgMosaicObj.background_image[0],
                    &rect,
                    1,
                    &map_id,
                    &image_addr,
                    &uv_data_ptr,
                    VX_WRITE_ONLY,
                    VX_MEMORY_TYPE_HOST,
                    VX_NOGAP_X);
    vxUnmapImagePatch(imgMosaicObj.background_image[0], map_id);

    image_holder.width = img_width;
    image_holder.height = img_height;
    image_holder.yRowAddr = (uint8_t *)y_data_ptr;
    image_holder.uvRowAddr = (uint8_t *)uv_data_ptr;

    getColor(&title_color, 255, 0, 0);
    getColor(&text_color, 255, 255, 255);
    getFont(&title_font, 0.02 * img_width);
    getFont(&text_font, 0.005 * img_width);
    getColor(&bg_color, 0, 0, 0);

    /* Black Background. */
    drawRect(&image_holder,0,0,img_width,img_height,&bg_color,-1);

    /* Draw Title. */
    titleX = (img_width - (title_font.width * title.length())) / 2;
    drawText(&image_holder,title.c_str(),titleX,10,&title_font,&title_color);

    for (i = 0; i < m_mosaicInfo.size(); i++)
    {
        drawText(&image_holder,
                 mosaicTitle[i].c_str(),
                 m_mosaicInfo[i][0],
                 m_mosaicInfo[i][1] - text_font.height,
                 &text_font,
                 &text_color);
    }
}

}