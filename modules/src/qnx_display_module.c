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


#include "qnx_display_module.h"

screen_window_t screen_win = NULL;

int32_t qnx_display_render_buf(Buf *tiovx_buffer)
{  
    
    static uint32_t init=0;
    int32_t err = 0;
    vx_uint32 width, height;
    vx_df_image df;
    vx_imagepatch_addressing_t image_addr;
    vx_rectangle_t rect;
    vx_map_id map_id1, map_id2;
    void *data_ptr1 = NULL, *data_ptr2 = NULL;
    vx_uint32 num_bytes_per_4pixels;
    vx_uint32 imgaddr_width, imgaddr_height, imgaddr_stride;
    uint32_t i;

    if(init==0)
    {
       qnx_display_init();
       init++;
    }
    else
    {

        int buffer_size[2];
        err = screen_get_window_property_iv(screen_win, SCREEN_PROPERTY_BUFFER_SIZE, buffer_size);
        if(err != 0) {
            printf("Failed to get window buffer size\n");
        }

        screen_buffer_t screen_buf[2];
        err = screen_get_window_property_pv(screen_win, SCREEN_PROPERTY_RENDER_BUFFERS, (void **)&screen_buf);
        if(err != 0) {
            printf("Failed to get window buffer\n");
        }

        /* obtain pointers to the buffers */
        void *ptr1 = NULL;
        err = screen_get_buffer_property_pv(screen_buf[0], SCREEN_PROPERTY_POINTER, (void **)&ptr1);
        if(err != 0) {
            printf("Failed to get buffer pointer\n");
        }

        int buf_stride1 = 0;
        err = screen_get_buffer_property_iv(screen_buf[0], SCREEN_PROPERTY_STRIDE, &buf_stride1);
        if(err != 0) {
           printf("Failed to get buffer stride1\n");
        }

        /* copy frames from OVX buffer to screen buffer*/
        vx_image display_image = (vx_image)(tiovx_buffer->handle);
        vxQueryImage(display_image, VX_IMAGE_WIDTH, &width, sizeof(vx_uint32));
        vxQueryImage(display_image, VX_IMAGE_HEIGHT, &height, sizeof(vx_uint32));
        vxQueryImage(display_image, VX_IMAGE_FORMAT, &df, sizeof(vx_df_image));

        if(VX_DF_IMAGE_NV12 == df)
        {
            num_bytes_per_4pixels = 4;
        }
        else if(TIVX_DF_IMAGE_NV12_P12 == df)
        {
            num_bytes_per_4pixels = 6;
        }
        else
        {
            num_bytes_per_4pixels = 8;
        }

        rect.start_x = 0;
        rect.start_y = 0;
        rect.end_x = width;
        rect.end_y = height;
        
        vxMapImagePatch(display_image,
            &rect,
            0,
            &map_id1,
            &image_addr,
            &data_ptr1,
            VX_WRITE_ONLY,
            VX_MEMORY_TYPE_HOST,
            VX_NOGAP_X
            );

        if(!data_ptr1)
        {
            printf("data_ptr1 is NULL \n");
            return -1;
        }

        imgaddr_width  = image_addr.dim_x;
        imgaddr_height = image_addr.dim_y;
        imgaddr_stride = image_addr.stride_y;

        /*  The current implementation performs memcpy of frame from TIOVX buffer
         *  to screen buffer. This is not the optimal implementation and needs to
         *  be updated going ahead for better G2G latency figures
         */
        for(i=0;i<height;i++)
        {
            memcpy(ptr1, data_ptr1, imgaddr_width*num_bytes_per_4pixels/4);
            data_ptr1 += imgaddr_stride;
            ptr1 += (buf_stride1);
        }
        vxUnmapImagePatch(display_image, map_id1);

        if(VX_DF_IMAGE_NV12 == df || TIVX_DF_IMAGE_NV12_P12 == df)
        {
            vxMapImagePatch(display_image,
                &rect,
                1,
                &map_id2,
                &image_addr,
                &data_ptr2,
                VX_WRITE_ONLY,
                VX_MEMORY_TYPE_HOST,
                VX_NOGAP_X
                );

            if(!data_ptr2)
            {
                printf("data_ptr2 is NULL \n");
                return -1;
            }

            imgaddr_width  = image_addr.dim_x;
            imgaddr_height = image_addr.dim_y;
            imgaddr_stride = image_addr.stride_y;

            for(i=0;i<imgaddr_height/2;i++)
            {
                memcpy(ptr1, data_ptr2, imgaddr_width*num_bytes_per_4pixels/4);
                data_ptr2 += imgaddr_stride;
                ptr1 += buf_stride1;
            }
            vxUnmapImagePatch(display_image, map_id2);
        }

        err = screen_post_window(screen_win, screen_buf[0], 0, NULL, 0);
        if(err != 0) {
            printf("Failed to post window\n");
        }
     
    
    return err;
    }
}

void qnx_display_init()
{
    int32_t err = 0;
    int usage = SCREEN_USAGE_READ | SCREEN_USAGE_WRITE;
    int screenFormat = SCREEN_FORMAT_NV12;
    screen_context_t screen_ctx = NULL;
    /* connect to screen */
    err = screen_create_context(&screen_ctx, SCREEN_APPLICATION_CONTEXT);
    if(err != 0) {
        printf("Failed to create screen context\n");
    }

    /* create a window */
    err = screen_create_window(&screen_win, screen_ctx);
    if(err != 0) {
        printf("Failed to create screen window\n");
    }

    err = screen_set_window_property_iv(screen_win, SCREEN_PROPERTY_USAGE, &usage);
    if(err != 0) {
        printf("Failed to set usage property\n");
    }
    err = screen_set_window_property_iv(screen_win, SCREEN_PROPERTY_FORMAT, &screenFormat);
    if(err != 0) {
        printf("Failed to set format prpoerty\n");
    }

    /* create screen buffers */
    int nbuffers = 2;
    err = screen_create_window_buffers(screen_win, nbuffers);
    if(err != 0) {
        printf("Failed to create window buffer\n");
    }

}
 