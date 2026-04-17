/*
 *
 * Copyright (c) 2026 Texas Instruments Incorporated
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

#include <tiovx_modules.h>
#include <tiovx_utils.h>
#include <v4l2_capture_module.h>
#include <kms_display_module.h>
#include <linux_aewb_module.h>

#define APP_BUFQ_DEPTH       (5)
#define APP_NUM_ITERATIONS   (900)

#define CAPTURE_INPUT_WIDTH  1936
#define CAPTURE_INPUT_HEIGHT 1100

#define VISS_INPUT_WIDTH     1936
#define VISS_INPUT_HEIGHT    1100
#define VISS_OUTPUT_WIDTH    (VISS_INPUT_WIDTH)
#define VISS_OUTPUT_HEIGHT   (VISS_INPUT_HEIGHT)

#define LDC_INPUT_WIDTH      VISS_OUTPUT_WIDTH
#define LDC_INPUT_HEIGHT     VISS_OUTPUT_HEIGHT

#define LDC_OUTPUT_WIDTH     1920
#define LDC_OUTPUT_HEIGHT    1080

#define LDC_TABLE_WIDTH     (1920)
#define LDC_TABLE_HEIGHT    (1080)
#define LDC_DS_FACTOR       (2)
#define LDC_BLOCK_WIDTH     (64)
#define LDC_BLOCK_HEIGHT    (32)
#define LDC_PIXEL_PAD       (1)

#define KMS_INPUT_WIDTH     LDC_OUTPUT_WIDTH
#define KMS_INPUT_HEIGHT    LDC_OUTPUT_HEIGHT

#define SENSOR_NAME "SENSOR_SONY_IMX390_UB953_D3"
#define DCC_VISS TIOVX_MODULES_IMAGING_PATH"/imx390/linear/dcc_viss.bin"
#define DCC_2A TIOVX_MODULES_IMAGING_PATH"/imx390/linear/dcc_2a.bin"
#define DCC_LDC TIOVX_MODULES_IMAGING_PATH"/imx390/linear/dcc_ldc.bin"

/* Function to clear frame buffer to black */
static void app_clear_buffer_to_black(Buf *buf, vx_int32 width, vx_int32 height);

vx_status app_modules_v4l2_capture_viss_ldc_display_test(vx_int32 argc, vx_char* argv[])
{
    vx_status status = VX_FAILURE;
    GraphObj graph;
    NodeObj *viss_node = NULL, *ldc_node = NULL;
    v4l2CaptureCfg v4l2_capture_cfg;
    v4l2CaptureHandle *v4l2_capture_handle;
    kmsDisplayCfg kms_display_cfg;
    kmsDisplayHandle *kms_display_handle;
    AewbCfg aewb_cfg;
    AewbHandle *aewb_handle;
    TIOVXVissNodeCfg viss_cfg;
    TIOVXLdcNodeCfg ldc_cfg;
    BufPool *in_buf_pool = NULL, *out_buf_pool = NULL;
    BufPool *h3a_buf_pool = NULL, *aewb_buf_pool = NULL;
    Buf *inbuf = NULL, *outbuf = NULL, *h3a_buf = NULL, *aewb_buf = NULL;

    /* Initialize TIOVX Graph */
    status = tiovx_modules_initialize_graph(&graph);
    graph.schedule_mode = VX_GRAPH_SCHEDULE_MODE_QUEUE_AUTO;

    /* VISS */
    tiovx_viss_init_cfg(&viss_cfg);
    sprintf(viss_cfg.sensor_name, SENSOR_NAME);
    snprintf(viss_cfg.dcc_config_file, TIVX_FILEIO_FILE_PATH_LENGTH, "%s", DCC_VISS);
    viss_cfg.width = VISS_INPUT_WIDTH;
    viss_cfg.height = VISS_INPUT_HEIGHT;
    viss_cfg.enable_aewb_pad = vx_true_e;
    viss_cfg.enable_h3a_pad  = vx_true_e;
    sprintf(viss_cfg.target_string, TIVX_TARGET_VPAC_VISS1);
    viss_cfg.input_cfg.params.format[0].pixel_container = TIVX_RAW_IMAGE_16_BIT;
    viss_cfg.input_cfg.params.format[0].msb = 11;
    viss_node = tiovx_modules_add_node(&graph, TIOVX_VISS, (void *)&viss_cfg);
    viss_node->sinks[0].bufq_depth = APP_BUFQ_DEPTH;

    /* LDC */
    tiovx_ldc_init_cfg(&ldc_cfg);
    sprintf(ldc_cfg.sensor_name, SENSOR_NAME);
    snprintf(ldc_cfg.dcc_config_file, TIVX_FILEIO_FILE_PATH_LENGTH, "%s", DCC_LDC);
    ldc_cfg.ldc_mode = TIOVX_MODULE_LDC_OP_MODE_DCC_DATA;
    ldc_cfg.input_cfg.color_format = VX_DF_IMAGE_NV12;
    ldc_cfg.input_cfg.width = LDC_INPUT_WIDTH;
    ldc_cfg.input_cfg.height = LDC_INPUT_HEIGHT;
    ldc_cfg.output_cfgs[0].color_format = VX_DF_IMAGE_NV12;
    ldc_cfg.output_cfgs[0].width = LDC_OUTPUT_WIDTH;
    ldc_cfg.output_cfgs[0].height = LDC_OUTPUT_HEIGHT;
    ldc_cfg.init_x = 0;
    ldc_cfg.init_y = 0;
    ldc_cfg.table_width = LDC_TABLE_WIDTH;
    ldc_cfg.table_height = LDC_TABLE_HEIGHT;
    ldc_cfg.ds_factor = LDC_DS_FACTOR;
    ldc_cfg.out_block_width = LDC_BLOCK_WIDTH;
    ldc_cfg.out_block_height = LDC_BLOCK_HEIGHT;
    ldc_cfg.pixel_pad = LDC_PIXEL_PAD;
    sprintf(ldc_cfg.target_string, TIVX_TARGET_VPAC_LDC1);
    ldc_node = tiovx_modules_add_node(&graph, TIOVX_LDC, (void *)&ldc_cfg);

    /* Link Nodes */
    tiovx_modules_link_pads(&viss_node->srcs[0], &ldc_node->sinks[0]);

    /* Verify Graph */
    status = tiovx_modules_verify_graph(&graph);

    /* InBuf Pool=VISS SINK OutBuf Pool = LDC SRC */
    in_buf_pool  = viss_node->sinks[0].buf_pool;
    out_buf_pool = ldc_node->srcs[0].buf_pool;

    /* V4l - Linux Capture */
    v4l2_capture_init_cfg(&v4l2_capture_cfg);
    v4l2_capture_cfg.width = CAPTURE_INPUT_WIDTH;
    v4l2_capture_cfg.height = CAPTURE_INPUT_HEIGHT;
    v4l2_capture_cfg.pix_format = V4L2_PIX_FMT_SRGGB12;
    v4l2_capture_cfg.bufq_depth = APP_BUFQ_DEPTH;
    sprintf(v4l2_capture_cfg.device, "/dev/video-imx390-cam0");
    v4l2_capture_handle = v4l2_capture_create_handle(&v4l2_capture_cfg);

    /* KMS - Linux Display */
    kms_display_init_cfg(&kms_display_cfg);
    kms_display_cfg.width = KMS_INPUT_WIDTH;
    kms_display_cfg.height = KMS_INPUT_HEIGHT;
    kms_display_cfg.pix_format = DRM_FORMAT_NV12;
    kms_display_handle = kms_display_create_handle(&kms_display_cfg);

    /* AEWB */
    aewb_init_cfg(&aewb_cfg);
    sprintf(aewb_cfg.device, "/dev/v4l-imx390-subdev0");
    sprintf(aewb_cfg.dcc_2a_file, TIOVX_MODULES_IMAGING_PATH"/imx390/linear/dcc_2a.bin");
    sprintf(aewb_cfg.sensor_name, SENSOR_NAME);
    aewb_handle = aewb_create_handle(&aewb_cfg);

    h3a_buf_pool  = viss_node->srcs[viss_node->num_outputs - 1].buf_pool;
    aewb_buf_pool = viss_node->sinks[1].buf_pool;

    if (status != VX_SUCCESS)
    {
        printf("ERROR: Graph verify failed\n");
        return status;
    }

    /* Clear Output Buffers to black and register with KMS */
    for (int i = 0; i < out_buf_pool->bufq_depth; i++)
    {
        app_clear_buffer_to_black(&out_buf_pool->bufs[i], LDC_OUTPUT_WIDTH, LDC_OUTPUT_HEIGHT);
        kms_display_register_buf(kms_display_handle, &out_buf_pool->bufs[i]);
    }

    for (int i = 0; i < APP_BUFQ_DEPTH; i++)
    {
        inbuf = tiovx_modules_acquire_buf(in_buf_pool);
        v4l2_capture_enqueue_buf(v4l2_capture_handle, inbuf);
    }

    /* Start Camera Capture */
    v4l2_capture_start(v4l2_capture_handle);

    for (vx_int32 i = 0; i < out_buf_pool->bufq_depth; i++)
    {
        outbuf = tiovx_modules_acquire_buf(out_buf_pool);
        tiovx_modules_enqueue_buf(outbuf);

        inbuf = v4l2_capture_dqueue_buf(v4l2_capture_handle);
        tiovx_modules_enqueue_buf(inbuf);

        h3a_buf = tiovx_modules_acquire_buf(h3a_buf_pool);
        tiovx_modules_enqueue_buf(h3a_buf);
        aewb_buf = tiovx_modules_acquire_buf(aewb_buf_pool);
        tiovx_modules_enqueue_buf(aewb_buf);
    }

    /* Run the Application */
    for (vx_int32 i = 0; i < APP_NUM_ITERATIONS; i++)
    {
         inbuf  = tiovx_modules_dequeue_buf(in_buf_pool);
         outbuf = tiovx_modules_dequeue_buf(out_buf_pool);
         h3a_buf = tiovx_modules_dequeue_buf(h3a_buf_pool);
         aewb_buf = tiovx_modules_dequeue_buf(aewb_buf_pool);

         kms_display_render_buf(kms_display_handle, outbuf);

         aewb_process(aewb_handle, h3a_buf, aewb_buf);

         v4l2_capture_enqueue_buf(v4l2_capture_handle, inbuf);
         do
         {
             inbuf = v4l2_capture_dqueue_buf(v4l2_capture_handle);
         } while (inbuf == NULL);

         tiovx_modules_enqueue_buf(outbuf);
         tiovx_modules_enqueue_buf(inbuf);
         tiovx_modules_enqueue_buf(h3a_buf);
         tiovx_modules_enqueue_buf(aewb_buf);
    }

    /* Stop Capture and Clean-UP */
    v4l2_capture_stop(v4l2_capture_handle);
    v4l2_capture_delete_handle(v4l2_capture_handle);
    kms_display_delete_handle(kms_display_handle);

    tiovx_modules_clean_graph(&graph);

    return status;
}

/* Function to clear frame buffer to black */
static void app_clear_buffer_to_black(Buf *buf, vx_int32 width, vx_int32 height)
{
    vx_image image = (vx_image)vxGetObjectArrayItem(buf->arr, 0);
    vx_rectangle_t rect = {0, 0, (vx_uint32)width, (vx_uint32)height};

    vx_map_id map_id;
    vx_imagepatch_addressing_t addr;
    void *ptr = NULL;

    /* Map ONLY Plane 0 - We will calculate the rest manually to avoid ID conflicts */
    vxMapImagePatch(image, &rect, 0, &map_id, &addr, &ptr,
                    VX_WRITE_ONLY, VX_MEMORY_TYPE_HOST, 0);

    if (ptr != NULL)
    {
        /* Total Luma size (Full Height) */
        size_t luma_size = addr.stride_y * addr.dim_y;

        /* Total Chroma size (Half Height) */
        size_t chroma_size = luma_size / 2;

        /* Clear the WHOLE TOP (Luma) to Black */
        memset(ptr, 0, luma_size);

        /* Clear the WHOLE BOTTOM (Chroma) to Neutral */
        /* This starts exactly after luma_size */
        memset((uint8_t *)ptr + luma_size, 0x80, chroma_size);
    }
    /* Unmap and Release */
    vxUnmapImagePatch(image, map_id);
    vxReleaseImage(&image);
}

