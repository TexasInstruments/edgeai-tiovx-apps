/*
 *
 * Copyright (c) 2021 Texas Instruments Incorporated
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

#include "tiovx_utils.h"
#include "app_mem.h"

vx_status readRawImage(char* file_name, tivx_raw_image image)
{
    vx_status status;

    status = vxGetStatus((vx_reference)image);

    if((vx_status)VX_SUCCESS == status)
    {
        FILE * fp = fopen(file_name,"rb");
        vx_int32  j;

        if(fp == NULL)
        {
            TIOVX_MODULE_ERROR("Unable to open file %s \n", file_name);
            return (VX_FAILURE);
        }

        {
            vx_uint32 width, height;
            vx_imagepatch_addressing_t image_addr;
            vx_rectangle_t rect;
            vx_map_id map_id;
            void *data_ptr;
            vx_uint32 bpp = 1;
            vx_uint32 num_bytes;
            tivx_raw_image_format_t format[3];
            vx_int32 plane, num_planes, plane_size;
            vx_uint32 num_exposures;
            vx_bool line_interleaved = vx_false_e;

            tivxQueryRawImage(image, TIVX_RAW_IMAGE_WIDTH, &width, sizeof(vx_uint32));
            tivxQueryRawImage(image, TIVX_RAW_IMAGE_HEIGHT, &height, sizeof(vx_uint32));
            tivxQueryRawImage(image, TIVX_RAW_IMAGE_FORMAT, &format, sizeof(format));
            tivxQueryRawImage(image, TIVX_RAW_IMAGE_NUM_EXPOSURES, &num_exposures, sizeof(num_exposures));
            tivxQueryRawImage(image, TIVX_RAW_IMAGE_LINE_INTERLEAVED, &line_interleaved, sizeof(line_interleaved));

            if(line_interleaved == vx_true_e)
            {
                num_planes = 1;
            }
            else
            {
                num_planes = num_exposures;
            }

            if( format[0].pixel_container == TIVX_RAW_IMAGE_16_BIT )
            {
                bpp = 2;
            }
            else if( format[0].pixel_container == TIVX_RAW_IMAGE_8_BIT )
            {
                bpp = 1;
            }
            else if( format[0].pixel_container == TIVX_RAW_IMAGE_P12_BIT )
            {
                bpp = 0;
            }

            rect.start_x = 0;
            rect.start_y = 0;
            rect.end_x = width;
            rect.end_y = height;

            for (plane = 0; plane < num_planes; plane++)
            {
                tivxMapRawImagePatch(image,
                    &rect,
                    plane,
                    &map_id,
                    &image_addr,
                    &data_ptr,
                    VX_WRITE_ONLY,
                    VX_MEMORY_TYPE_HOST,
                    TIVX_RAW_IMAGE_PIXEL_BUFFER
                    );

                uint8_t *pIn = (uint8_t *)data_ptr;
                num_bytes = 0;
                if(line_interleaved == vx_true_e)
                {
                    for (j = 0; j < (image_addr.dim_y * num_exposures); j++)
                    {
                        num_bytes += fread(pIn, 1, image_addr.dim_x * bpp, fp);
                        pIn += image_addr.stride_y;
                    }
                }
                else
                {
                    for (j = 0; j < image_addr.dim_y; j++)
                    {
                        num_bytes += fread(pIn, 1, image_addr.dim_x * bpp, fp);
                        pIn += image_addr.stride_y;
                    }
                }

                plane_size = (image_addr.dim_y * image_addr.dim_x* bpp);

                if(num_bytes != plane_size)
                    TIOVX_MODULE_ERROR("Plane [%d] bytes read = %d, expected = %d\n", plane, num_bytes, plane_size);

                tivxUnmapRawImagePatch(image, map_id);
            }
        }

        fclose(fp);
    }

    return(status);
}

vx_status writeRawImage(char* file_name, tivx_raw_image image)
{
    vx_status status;

    status = vxGetStatus((vx_reference)image);

    if((vx_status)VX_SUCCESS == status)
    {
        FILE * fp = fopen(file_name,"wb");
        vx_int32  j;

        if(fp == NULL)
        {
            TIOVX_MODULE_ERROR("Unable to open file %s \n", file_name);
            return (VX_FAILURE);
        }

        {
            vx_uint32 width, height;
            vx_imagepatch_addressing_t image_addr;
            vx_rectangle_t rect;
            vx_map_id map_id;
            void *data_ptr;
            vx_uint32 bpp = 1;
            vx_uint32 num_bytes;
            tivx_raw_image_format_t format[3];
            vx_int32 plane, num_planes, plane_size;
            vx_uint32 num_exposures;
            vx_bool line_interleaved = vx_false_e;

            tivxQueryRawImage(image, TIVX_RAW_IMAGE_WIDTH, &width, sizeof(vx_uint32));
            tivxQueryRawImage(image, TIVX_RAW_IMAGE_HEIGHT, &height, sizeof(vx_uint32));
            tivxQueryRawImage(image, TIVX_RAW_IMAGE_FORMAT, &format, sizeof(format));
            tivxQueryRawImage(image, TIVX_RAW_IMAGE_NUM_EXPOSURES, &num_exposures, sizeof(num_exposures));
            tivxQueryRawImage(image, TIVX_RAW_IMAGE_LINE_INTERLEAVED, &line_interleaved, sizeof(line_interleaved));

            if(line_interleaved == vx_true_e)
            {
                num_planes = 1;
            }
            else
            {
                num_planes = num_exposures;
            }

            if( format[0].pixel_container == TIVX_RAW_IMAGE_16_BIT )
            {
                bpp = 2;
            }
            else if( format[0].pixel_container == TIVX_RAW_IMAGE_8_BIT )
            {
                bpp = 1;
            }
            else if( format[0].pixel_container == TIVX_RAW_IMAGE_P12_BIT )
            {
                bpp = 0;
            }

            rect.start_x = 0;
            rect.start_y = 0;
            rect.end_x = width;
            rect.end_y = height;

            for (plane = 0; plane < num_planes; plane++)
            {
                tivxMapRawImagePatch(image,
                    &rect,
                    plane,
                    &map_id,
                    &image_addr,
                    &data_ptr,
                    VX_READ_ONLY,
                    VX_MEMORY_TYPE_HOST,
                    TIVX_RAW_IMAGE_PIXEL_BUFFER
                    );

                uint8_t *pIn = (uint8_t *)data_ptr;
                num_bytes = 0;
                if(line_interleaved == vx_true_e)
                {
                    for (j = 0; j < (image_addr.dim_y * num_exposures); j++)
                    {
                        num_bytes += fwrite(pIn, 1, image_addr.dim_x * bpp, fp);
                        pIn += image_addr.stride_y;
                    }
                }
                else
                {
                    for (j = 0; j < image_addr.dim_y; j++)
                    {
                        num_bytes += fwrite(pIn, 1, image_addr.dim_x * bpp, fp);
                        pIn += image_addr.stride_y;
                    }
                }

                plane_size = (image_addr.dim_y * image_addr.dim_x* bpp);

                if(num_bytes != plane_size)
                    TIOVX_MODULE_ERROR("Plane [%d] bytes written = %d, expected = %d\n", plane, num_bytes, plane_size);

                tivxUnmapRawImagePatch(image, map_id);
            }
        }

        fclose(fp);
    }

    return(status);
}

vx_status readImage(char* file_name, vx_image img)
{
    vx_status status;

    status = vxGetStatus((vx_reference)img);

    if((vx_status)VX_SUCCESS == status)
    {
        FILE * fp = fopen(file_name,"rb");
        vx_int32  j;

        if(fp == NULL)
        {
            TIOVX_MODULE_ERROR("Unable to open file %s \n", file_name);
            return (VX_FAILURE);
        }

        {
            vx_rectangle_t rect;
            vx_imagepatch_addressing_t image_addr;
            vx_map_id map_id;
            void * data_ptr;
            vx_uint32  img_width;
            vx_uint32  img_height;
            vx_uint32  num_bytes = 0;
            vx_size    num_planes;
            vx_uint32  plane;
            vx_uint32  plane_size;
            vx_df_image img_format;

            vxQueryImage(img, VX_IMAGE_WIDTH, &img_width, sizeof(vx_uint32));
            vxQueryImage(img, VX_IMAGE_HEIGHT, &img_height, sizeof(vx_uint32));
            vxQueryImage(img, VX_IMAGE_PLANES, &num_planes, sizeof(vx_size));
            vxQueryImage(img, VX_IMAGE_FORMAT, &img_format, sizeof(vx_df_image));

            for (plane = 0; plane < num_planes; plane++)
            {
                rect.start_x = 0;
                rect.start_y = 0;
                rect.end_x = img_width;
                rect.end_y = img_height;
                status = vxMapImagePatch(img,
                                        &rect,
                                        plane,
                                        &map_id,
                                        &image_addr,
                                        &data_ptr,
                                        VX_WRITE_ONLY,
                                        VX_MEMORY_TYPE_HOST,
                                        VX_NOGAP_X);
                TIOVX_MODULE_PRINTF("image_addr.dim_x = %d\n ", image_addr.dim_x);
                TIOVX_MODULE_PRINTF("image_addr.dim_y = %d\n ", image_addr.dim_y);
                TIOVX_MODULE_PRINTF("image_addr.step_x = %d\n ", image_addr.step_x);
                TIOVX_MODULE_PRINTF("image_addr.step_y = %d\n ", image_addr.step_y);
                TIOVX_MODULE_PRINTF("image_addr.stride_y = %d\n ", image_addr.stride_y);
                TIOVX_MODULE_PRINTF("image_addr.stride_x = %d\n ", image_addr.stride_x);
                TIOVX_MODULE_PRINTF("\n");

                num_bytes = 0;
                for (j = 0; j < (image_addr.dim_y/image_addr.step_y); j++)
                {
                    num_bytes += image_addr.stride_x * fread(data_ptr, image_addr.stride_x, (image_addr.dim_x/image_addr.step_x), fp);
                    data_ptr += image_addr.stride_y;
                }

                plane_size = (image_addr.dim_y/image_addr.step_y) * ((image_addr.dim_x * image_addr.stride_x)/image_addr.step_x);

                if(num_bytes != plane_size)
                    TIOVX_MODULE_ERROR("Plane [%d] bytes read = %d, expected = %d\n", plane, num_bytes, plane_size);

                vxUnmapImagePatch(img, map_id);
            }

        }

        fclose(fp);
    }

    return(status);
}

vx_status writeImage(char* file_name, vx_image img)
{
    vx_status status;

    status = vxGetStatus((vx_reference)img);

    if((vx_status)VX_SUCCESS == status)
    {
        FILE * fp = fopen(file_name,"wb");
        vx_int32  j;

        if(fp == NULL)
        {
            TIOVX_MODULE_ERROR("Unable to open file %s \n", file_name);
            return (VX_FAILURE);
        }

        {
            vx_rectangle_t rect;
            vx_imagepatch_addressing_t image_addr;
            vx_map_id map_id;
            void * data_ptr;
            vx_uint32  img_width;
            vx_uint32  img_height;
            vx_uint32  num_bytes = 0;
            vx_size    num_planes;
            vx_uint32  plane;
            vx_uint32  plane_size;
            vx_df_image img_format;

            vxQueryImage(img, VX_IMAGE_WIDTH, &img_width, sizeof(vx_uint32));
            vxQueryImage(img, VX_IMAGE_HEIGHT, &img_height, sizeof(vx_uint32));
            vxQueryImage(img, VX_IMAGE_PLANES, &num_planes, sizeof(vx_size));
            vxQueryImage(img, VX_IMAGE_FORMAT, &img_format, sizeof(vx_df_image));

            for (plane = 0; plane < num_planes; plane++)
            {
                rect.start_x = 0;
                rect.start_y = 0;
                rect.end_x = img_width;
                rect.end_y = img_height;
                status = vxMapImagePatch(img,
                                        &rect,
                                        plane,
                                        &map_id,
                                        &image_addr,
                                        &data_ptr,
                                        VX_READ_ONLY,
                                        VX_MEMORY_TYPE_HOST,
                                        VX_NOGAP_X);

                TIOVX_MODULE_PRINTF("image_addr.dim_x = %d\n ", image_addr.dim_x);
                TIOVX_MODULE_PRINTF("image_addr.dim_y = %d\n ", image_addr.dim_y);
                TIOVX_MODULE_PRINTF("image_addr.step_x = %d\n ", image_addr.step_x);
                TIOVX_MODULE_PRINTF("image_addr.step_y = %d\n ", image_addr.step_y);
                TIOVX_MODULE_PRINTF("image_addr.stride_y = %d\n ", image_addr.stride_y);
                TIOVX_MODULE_PRINTF("image_addr.stride_x = %d\n ", image_addr.stride_x);
                TIOVX_MODULE_PRINTF("\n");

                num_bytes = 0;
                for (j = 0; j < (image_addr.dim_y/image_addr.step_y); j++)
                {
                    num_bytes += image_addr.stride_x * fwrite(data_ptr, image_addr.stride_x, (image_addr.dim_x/image_addr.step_x), fp);
                    data_ptr += image_addr.stride_y;
                }

                plane_size = (image_addr.dim_y/image_addr.step_y) * ((image_addr.dim_x * image_addr.stride_x)/image_addr.step_x);

                if(num_bytes != plane_size)
                    TIOVX_MODULE_ERROR("Plane [%d] bytes written = %d, expected = %d\n", plane, num_bytes, plane_size);

                vxUnmapImagePatch(img, map_id);
            }

        }

        fclose(fp);
    }

    return(status);
}

vx_status resetImage(vx_image img, int32_t value)
{
    vx_status status;

    status = vxGetStatus((vx_reference)img);

    if((vx_status)VX_SUCCESS == status)
    {
        vx_rectangle_t rect;
        vx_imagepatch_addressing_t image_addr;
        vx_map_id map_id;
        void * data_ptr;
        vx_uint32  img_width;
        vx_uint32  img_height;
        vx_size    num_planes;
        vx_uint32  plane;
        vx_df_image img_format;
        vx_int32  j;

        vxQueryImage(img, VX_IMAGE_WIDTH, &img_width, sizeof(vx_uint32));
        vxQueryImage(img, VX_IMAGE_HEIGHT, &img_height, sizeof(vx_uint32));
        vxQueryImage(img, VX_IMAGE_PLANES, &num_planes, sizeof(vx_size));
        vxQueryImage(img, VX_IMAGE_FORMAT, &img_format, sizeof(vx_df_image));

        for (plane = 0; plane < num_planes; plane++)
        {
            rect.start_x = 0;
            rect.start_y = 0;
            rect.end_x = img_width;
            rect.end_y = img_height;
            status = vxMapImagePatch(img,
                                    &rect,
                                    plane,
                                    &map_id,
                                    &image_addr,
                                    &data_ptr,
                                    VX_WRITE_ONLY,
                                    VX_MEMORY_TYPE_HOST,
                                    VX_NOGAP_X);

            TIOVX_MODULE_PRINTF("image_addr.dim_x = %d\n ", image_addr.dim_x);
            TIOVX_MODULE_PRINTF("image_addr.dim_y = %d\n ", image_addr.dim_y);
            TIOVX_MODULE_PRINTF("image_addr.step_x = %d\n ", image_addr.step_x);
            TIOVX_MODULE_PRINTF("image_addr.step_y = %d\n ", image_addr.step_y);
            TIOVX_MODULE_PRINTF("image_addr.stride_y = %d\n ", image_addr.stride_y);
            TIOVX_MODULE_PRINTF("image_addr.stride_x = %d\n ", image_addr.stride_x);
            TIOVX_MODULE_PRINTF("\n");

            for (j = 0; j < (image_addr.dim_y/image_addr.step_y); j++)
            {
                memset(data_ptr, value, image_addr.dim_x * (image_addr.stride_x / image_addr.step_x));
                data_ptr += image_addr.stride_y;
            }

            vxUnmapImagePatch(img, map_id);
        }
    }

    return(status);
}

static vx_uint32 get_bit_depth(vx_enum data_type)
{
    vx_uint32 size = 0;

    if((data_type == VX_TYPE_UINT8) || (data_type == VX_TYPE_INT8))
    {
        size = sizeof(vx_uint8);
    }
    else if((data_type == VX_TYPE_UINT16) || (data_type == VX_TYPE_INT16))
    {
        size = sizeof(vx_uint16);
    }
    else if((data_type == VX_TYPE_UINT32) || (data_type == VX_TYPE_INT32))
    {
        size = sizeof(vx_uint32);
    }
    else if(data_type == VX_TYPE_FLOAT32)
    {
        size = sizeof(vx_float32);
    }

    return size;
}

vx_status readTensor(char* file_name, vx_tensor tensor)
{
    vx_status status = VX_SUCCESS;

    vx_size num_dims;
    void *data_ptr;
    vx_map_id map_id;

    vx_size start[APP_MAX_TENSOR_DIMS];
    vx_size tensor_strides[APP_MAX_TENSOR_DIMS];
    vx_size tensor_sizes[APP_MAX_TENSOR_DIMS];
    vx_char new_name[APP_MAX_FILE_PATH];
    vx_enum data_type;

    vxQueryTensor(tensor, VX_TENSOR_NUMBER_OF_DIMS, &num_dims, sizeof(vx_size));
    if(num_dims != 3)
    {
        TIOVX_MODULE_ERROR("Number of dims are != 3 \n");
        status = VX_FAILURE;
    }

    vxQueryTensor(tensor, VX_TENSOR_DATA_TYPE, &data_type, sizeof(vx_enum));
    vx_uint32 bit_depth = get_bit_depth(data_type);
    if(bit_depth == 0)
    {
        TIOVX_MODULE_ERROR("Incorrect data_type/bit-depth!\n \n");
        status = VX_FAILURE;
    }

    if((vx_status)VX_SUCCESS == status)
    {
        vxQueryTensor(tensor, VX_TENSOR_DIMS, tensor_sizes, num_dims * sizeof(vx_size));

        start[0] = start[1] = start[2] = 0;

        tensor_strides[0] = bit_depth;
        tensor_strides[1] = tensor_sizes[0] * tensor_strides[0];
        tensor_strides[2] = tensor_sizes[1] * tensor_strides[1];

        status = tivxMapTensorPatch(tensor, num_dims, start, tensor_sizes, &map_id, tensor_strides, &data_ptr, VX_WRITE_ONLY, VX_MEMORY_TYPE_HOST);

        snprintf(new_name, APP_MAX_FILE_PATH, "%s_%dx%d.bin", file_name, (uint32_t)tensor_sizes[0], (uint32_t)tensor_sizes[1]);

        FILE *fp = fopen(new_name, "rb");
        if(NULL == fp)
        {
            TIOVX_MODULE_ERROR("Unable to open file %s \n", new_name);
            status = VX_FAILURE;
        }

        if(VX_SUCCESS == status)
        {
            int32_t size = fread(data_ptr, 1, tensor_sizes[0] * tensor_sizes[1] * tensor_sizes[2] * bit_depth, fp);
            if (size != (tensor_sizes[0] * tensor_sizes[1] * tensor_sizes[2] * bit_depth))
            {
                TIOVX_MODULE_ERROR("fread() size %d not matching with expected size! %d \n", size, (int32_t)(tensor_sizes[0] * tensor_sizes[1] * tensor_sizes[2] * bit_depth));
                status = VX_FAILURE;
            }

            tivxUnmapTensorPatch(tensor, map_id);
        }

        if(fp)
        {
            fclose(fp);
        }
    }

    return(status);
}

vx_status writeTensor(char* file_name, vx_tensor tensor)
{
    vx_status status = VX_SUCCESS;

    vx_size num_dims;
    void *data_ptr;
    vx_map_id map_id;

    vx_size start[APP_MAX_TENSOR_DIMS];
    vx_size tensor_strides[APP_MAX_TENSOR_DIMS];
    vx_size tensor_sizes[APP_MAX_TENSOR_DIMS];
    vx_char new_name[APP_MAX_FILE_PATH];
    vx_enum data_type;

    vxQueryTensor(tensor, VX_TENSOR_NUMBER_OF_DIMS, &num_dims, sizeof(vx_size));
    if(num_dims != 3)
    {
        TIOVX_MODULE_ERROR("Number of dims are != 3 \n");
        status = VX_FAILURE;
    }

    vxQueryTensor(tensor, VX_TENSOR_DATA_TYPE, &data_type, sizeof(vx_enum));
    vx_uint32 bit_depth = get_bit_depth(data_type);
    if(bit_depth == 0)
    {
        TIOVX_MODULE_ERROR("Incorrect data_type/bit-depth!\n \n");
        status = VX_FAILURE;
    }

    if((vx_status)VX_SUCCESS == status)
    {
        vxQueryTensor(tensor, VX_TENSOR_DIMS, tensor_sizes, num_dims * sizeof(vx_size));

        start[0] = start[1] = start[2] = 0;

        tensor_strides[0] = bit_depth;
        tensor_strides[1] = tensor_sizes[0] * tensor_strides[0];
        tensor_strides[2] = tensor_sizes[1] * tensor_strides[1];

        status = tivxMapTensorPatch(tensor, num_dims, start, tensor_sizes, &map_id, tensor_strides, &data_ptr, VX_READ_ONLY, VX_MEMORY_TYPE_HOST);

        snprintf(new_name, APP_MAX_FILE_PATH, "%s_%dx%d.bin", file_name, (uint32_t)tensor_sizes[0], (uint32_t)tensor_sizes[1]);

        FILE *fp = fopen(new_name, "wb");
        if(NULL == fp)
        {
            TIOVX_MODULE_ERROR("Unable to open file %s \n", new_name);
            status = VX_FAILURE;
        }

        if(VX_SUCCESS == status)
        {
            int32_t size = fwrite(data_ptr, 1, tensor_sizes[0] * tensor_sizes[1] * tensor_sizes[2] * bit_depth, fp);
            if (size != (tensor_sizes[0] * tensor_sizes[1] * tensor_sizes[2] * bit_depth))
            {
                TIOVX_MODULE_ERROR("fwrite() size %d not matching with expected size! %d \n", size, (int32_t)(tensor_sizes[0] * tensor_sizes[1] * tensor_sizes[2] * bit_depth));
                status = VX_FAILURE;
            }

            tivxUnmapTensorPatch(tensor, map_id);
        }

        if(fp)
        {
            fclose(fp);
        }
    }

    return(status);
}

int getDmaFd(vx_reference ref)
{
    tivx_raw_image image = (tivx_raw_image)ref;
    vx_uint32 width, height;
    vx_imagepatch_addressing_t image_addr;
    vx_rectangle_t rect;
    vx_map_id map_id;
    tivx_raw_image_format_t format[3];
    vx_uint32 num_exposures;
    vx_bool line_interleaved = vx_false_e;
    void *data_ptr;
    uint32_t dmabuf_fd_offset;
    int ret = -1;

    tivxQueryRawImage(image, TIVX_RAW_IMAGE_WIDTH, &width, sizeof(vx_uint32));
    tivxQueryRawImage(image, TIVX_RAW_IMAGE_HEIGHT, &height, sizeof(vx_uint32));
    tivxQueryRawImage(image, TIVX_RAW_IMAGE_FORMAT, &format, sizeof(format));
    tivxQueryRawImage(image, TIVX_RAW_IMAGE_NUM_EXPOSURES, &num_exposures, sizeof(num_exposures));
    tivxQueryRawImage(image, TIVX_RAW_IMAGE_LINE_INTERLEAVED, &line_interleaved, sizeof(line_interleaved));

    rect.start_x = 0;
    rect.start_y = 0;
    rect.end_x = width;
    rect.end_y = height;

    tivxMapRawImagePatch(image,
        &rect,
        0,
        &map_id,
        &image_addr,
        &data_ptr,
        VX_WRITE_ONLY,
        VX_MEMORY_TYPE_HOST,
        TIVX_RAW_IMAGE_PIXEL_BUFFER
        );

    ret = appMemGetDmaBufFd(data_ptr, &dmabuf_fd_offset);

    tivxUnmapRawImagePatch(image, map_id);

    return ret;
}
