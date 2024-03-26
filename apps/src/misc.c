/*
 *
 * Copyright (c) 2024 Texas Instruments Incorporated
 *
 * All rights reserved not granted herein.
 *
 * Limited License.
 *
 * Texas Instruments Incorporated grants a world-wide, royalty-free, non-exclusive
 * license under copyrights and patents it now or hereafter owns or controls to make,
 * have made, use, import, offer to sell and sell ("Utilize") this software subject to the
 * terms herein.  With respect to the foregoing patent license, such license is granted
 * solely to the extent that any such patent is necessary to Utilize the software alone.
 * The patent license shall not apply to any combinations which include this software,
 * other than combinations with devices manufactured by or for TI ("TI Devices").
 * No hardware patent is licensed hereunder.
 *
 * Redistributions must preserve existing copyright notices and reproduce this license
 * (including the above copyright notice and the disclaimer and (if applicable) source
 * code license limitations below) in the documentation and/or other materials provided
 * with the distribution
 *
 * Redistribution and use in binary form, without modification, are permitted provided
 * that the following conditions are met:
 *
 * *       No reverse engineering, decompilation, or disassembly of this software is
 * permitted with respect to any software provided in binary form.
 *
 * *       any redistribution and use are licensed by TI for use only with TI Devices.
 *
 * *       Nothing shall obligate TI to provide you with source code for the software
 * licensed and provided to you in object code.
 *
 * If software source code is provided to you, modification and redistribution of the
 * source code are permitted provided that the following conditions are met:
 *
 * *       any redistribution and use of the source code, including any resulting derivative
 * works, are licensed by TI for use only with TI Devices.
 *
 * *       any redistribution and use of any object code compiled from the source code
 * and any resulting derivative works, are licensed by TI for use only with TI Devices.
 *
 * Neither the name of Texas Instruments Incorporated nor the names of its suppliers
 *
 * may be used to endorse or promote products derived from this software without
 * specific prior written permission.
 *
 * DISCLAIMER.
 *
 * THIS SOFTWARE IS PROVIDED BY TI AND TI'S LICENSORS "AS IS" AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL TI AND TI'S LICENSORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
 * OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
 * OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

#include <apps/include/misc.h>

void *map_image(vx_image image, uint8_t plane, vx_map_id *map_id)
{
    vx_rectangle_t rect;
    vx_imagepatch_addressing_t image_addr;
    vx_uint32 img_width;
    vx_uint32 img_height;
    void *ptr;

    vxQueryImage(image, VX_IMAGE_WIDTH, &img_width, sizeof(vx_uint32));
    vxQueryImage(image, VX_IMAGE_HEIGHT, &img_height, sizeof(vx_uint32));

    /* Y Plane */
    rect.start_x = 0;
    rect.start_y = 0;
    rect.end_x = img_width;
    rect.end_y = img_height;
    vxMapImagePatch(image,
                    &rect,
                    plane,
                    map_id,
                    &image_addr,
                    &ptr,
                    VX_WRITE_ONLY,
                    VX_MEMORY_TYPE_HOST,
                    VX_NOGAP_X);

    return ptr;
}

void unmap_image(vx_image image, vx_map_id map_id)
{
    vxUnmapImagePatch(image, map_id);
}

void set_mosaic_background(vx_image image, char *title)
{
    vx_uint32 img_width;
    vx_uint32 img_height;
    void *y_ptr;
    void *uv_ptr;
    vx_map_id y_ptr_map_id;
    vx_map_id uv_ptr_map_id;
    Image image_holder;
    YUVColor color_black;
    YUVColor color_white;
    YUVColor color_red;
    YUVColor color_green;
    FontProperty big_font;
    FontProperty medium_font;

    vxQueryImage(image, VX_IMAGE_WIDTH, &img_width, sizeof(vx_uint32));
    vxQueryImage(image, VX_IMAGE_HEIGHT, &img_height, sizeof(vx_uint32));

    y_ptr = map_image(image, 0, &y_ptr_map_id);
    uv_ptr = map_image(image, 1, &uv_ptr_map_id);

    image_holder.width = img_width;
    image_holder.height = img_height;
    image_holder.yRowAddr = (uint8_t *)y_ptr;
    image_holder.uvRowAddr = (uint8_t *)uv_ptr;

    getColor(&color_black, 0, 0, 0);
    getColor(&color_white, 255, 255, 255);
    getColor(&color_red, 255, 0, 0);
    getColor(&color_green, 0, 255, 0);

    getFont(&big_font, 0.02*img_width);
    getFont(&medium_font, 0.01*img_width);

    /* Set background to black */
    drawRect(&image_holder,0,0,img_width,img_height,&color_black,-1);

    /* Draw title */
    drawText(&image_holder,
             "EDGEAI TIOVX APPS",
             2,
             2,
             &big_font,
             &color_red);

    drawText(&image_holder,
             title,
             2,
             big_font.height + 2,
             &medium_font,
             &color_green);

    unmap_image(image, y_ptr_map_id);
    unmap_image(image, uv_ptr_map_id);
}

void update_perf_overlay(vx_image image, EdgeAIPerfStats *perf_stats_handle)
{
    vx_uint32 img_width;
    vx_uint32 img_height;
    void *y_ptr;
    void *uv_ptr;
    vx_map_id y_ptr_map_id;
    vx_map_id uv_ptr_map_id;

    vxQueryImage(image, VX_IMAGE_WIDTH, &img_width, sizeof(vx_uint32));
    vxQueryImage(image, VX_IMAGE_HEIGHT, &img_height, sizeof(vx_uint32));

    y_ptr = map_image(image, 0, &y_ptr_map_id);
    uv_ptr = map_image(image, 1, &uv_ptr_map_id);

    perf_stats_handle->overlay.imgYPtr = (uint8_t *)y_ptr;
    perf_stats_handle->overlay.imgUVPtr = (uint8_t *)uv_ptr;
    perf_stats_handle->overlay.imgWidth = img_width;
    perf_stats_handle->overlay.imgHeight = img_height;
    perf_stats_handle->overlay.xPos = 0;
    perf_stats_handle->overlay.yPos = 0;
    perf_stats_handle->overlay.width = img_width;
    perf_stats_handle->overlay.height = img_height;

    update_edgeai_perf_stats(perf_stats_handle);

    unmap_image(image, y_ptr_map_id);
    unmap_image(image, uv_ptr_map_id);
}
