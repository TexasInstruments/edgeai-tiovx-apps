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

#include <tiovx_modules.h>
#include <tiovx_utils.h>
#include <v4l2_capture_module.h>
#include <kms_display_module.h>
#include <linux_aewb_module.h>

#define APP_BUFQ_DEPTH      (4)
#define APP_NUM_CH          (1)
#define APP_NUM_ITERATIONS  (600)

#define INPUT_WIDTH  (1920)
#define INPUT_HEIGHT (1080)
#define PYRAMID_LEVELS (3)

#define SENSOR_NAME "SENSOR_SONY_IMX219_RPI"
#define DCC_VISS "/opt/imaging/imx219/linear/dcc_viss.bin"

vx_status app_modules_linux_capture_dof_test(vx_int32 argc, vx_char* argv[])
{
    vx_status status = VX_FAILURE;
    GraphObj graph;
    NodeObj *viss_node = NULL, *dof_node = NULL, *dof_viz_node = NULL, *mosaic_node = NULL;
    NodeObj *pyramid_node = NULL, *tee_node = NULL, *cc_node = NULL, *msc_node = NULL;
    TIOVXVissNodeCfg viss_cfg;
    TIOVXMultiScalerNodeCfg msc_cfg;
    TIOVXPyramidNodeCfg pyramid_cfg;
    TIOVXTeeNodeCfg tee_cfg;
    TIOVXDofNodeCfg dof_cfg;
    TIOVXDofVizNodeCfg dof_viz_cfg;
    TIOVXDelayNodeCfg delay_cfg;
    TIOVXDLColorConvertNodeCfg cc_cfg;
    TIOVXMosaicNodeCfg mosaic_cfg;
    BufPool *in_buf_pool = NULL, *out_buf_pool = NULL;
    BufPool *h3a_buf_pool = NULL, *aewb_buf_pool = NULL;
    Buf *inbuf = NULL, *outbuf = NULL, *h3a_buf = NULL, *aewb_buf = NULL;
    v4l2CaptureCfg v4l2_capture_cfg;
    v4l2CaptureHandle *v4l2_capture_handle;
    kmsDisplayCfg kms_display_cfg;
    kmsDisplayHandle *kms_display_handle;
    AewbCfg aewb_cfg;
    AewbHandle *aewb_handle;

    status = tiovx_modules_initialize_graph(&graph);

    //VISS
    tiovx_viss_init_cfg(&viss_cfg);

    sprintf(viss_cfg.sensor_name, SENSOR_NAME);
    snprintf(viss_cfg.dcc_config_file, TIVX_FILEIO_FILE_PATH_LENGTH, "%s", DCC_VISS);
    viss_cfg.width = INPUT_WIDTH;
    viss_cfg.height = INPUT_HEIGHT;
    sprintf(viss_cfg.target_string, TIVX_TARGET_VPAC_VISS1);

    viss_cfg.input_cfg.params.format[0].pixel_container = TIVX_RAW_IMAGE_8_BIT;
    viss_cfg.input_cfg.params.format[0].msb = 7;
    viss_cfg.enable_aewb_pad = vx_true_e;
    viss_cfg.enable_h3a_pad = vx_true_e;

    viss_node = tiovx_modules_add_node(&graph, TIOVX_VISS, (void *)&viss_cfg);
    viss_node->sinks[0].bufq_depth = APP_BUFQ_DEPTH;

    //MSC
    tiovx_multi_scaler_init_cfg(&msc_cfg);

    msc_cfg.color_format = VX_DF_IMAGE_NV12;
    msc_cfg.num_outputs = 1;
    msc_cfg.input_cfg.width = INPUT_WIDTH;
    msc_cfg.input_cfg.height = INPUT_HEIGHT;
    msc_cfg.output_cfgs[0].width = 1280;
    msc_cfg.output_cfgs[0].height = 720;
    sprintf(msc_cfg.target_string, TIVX_TARGET_VPAC_MSC1);

    tiovx_multi_scaler_module_crop_params_init(&msc_cfg);

    msc_node = tiovx_modules_add_node(&graph, TIOVX_MULTI_SCALER, (void *)&msc_cfg);

    //Link viss out to msc in
    tiovx_modules_link_pads(&viss_node->srcs[0], &msc_node->sinks[0]);

    //PYRAMID
    tiovx_pyramid_init_cfg(&pyramid_cfg);

    pyramid_cfg.width = 1280;
    pyramid_cfg.height = 720;
    pyramid_cfg.input_cfg.color_format = VX_DF_IMAGE_NV12;
    pyramid_cfg.output_cfg.color_format = VX_DF_IMAGE_U8;
    pyramid_cfg.output_cfg.levels = PYRAMID_LEVELS;
    sprintf(pyramid_cfg.target_string, TIVX_TARGET_VPAC_MSC1);

    pyramid_node = tiovx_modules_add_node(&graph, TIOVX_PYRAMID, (void *)&pyramid_cfg);

    //Link msc out to pyramid in
    tiovx_modules_link_pads(&msc_node->srcs[0], &pyramid_node->sinks[0]);

    //TEE
    tiovx_tee_init_cfg(&tee_cfg);

    tee_cfg.peer_pad = &pyramid_node->srcs[0];
    tee_cfg.num_outputs = 2;

    tee_node = tiovx_modules_add_node(&graph, TIOVX_TEE, (void *)&tee_cfg);

    //DOF
    tiovx_dof_init_cfg(&dof_cfg);

    dof_cfg.width = 1280;
    dof_cfg.height = 720;

    dof_node = tiovx_modules_add_node(&graph, TIOVX_DOF, (void *)&dof_cfg);

    //Link tee pad0 to dof in
    tiovx_modules_link_pads(&tee_node->srcs[0], &dof_node->sinks[0]);

    //DOF Viz
    tiovx_dof_viz_init_cfg(&dof_viz_cfg);

    dof_viz_cfg.width = 1280;
    dof_viz_cfg.height = 720;

    dof_viz_node = tiovx_modules_add_node(&graph, TIOVX_DOF_VIZ, (void *)&dof_viz_cfg);

    //Link tee dof out to dof viz in
    tiovx_modules_link_pads(&dof_node->srcs[0], &dof_viz_node->sinks[0]);

    //DELAY
    tiovx_delay_init_cfg(&delay_cfg);

    delay_cfg.src_pad = &tee_node->srcs[1];
    delay_cfg.sink_pad = &dof_node->sinks[1];
    delay_cfg.delay = 2;

    tiovx_modules_add_node(&graph, TIOVX_DELAY, (void *)&delay_cfg);

    //COLOR CONVERT
    tiovx_dl_color_convert_init_cfg(&cc_cfg);

    cc_cfg.width = 1280;
    cc_cfg.height = 720;
    cc_cfg.input_cfg.color_format = VX_DF_IMAGE_RGB;
    cc_cfg.output_cfg.color_format = VX_DF_IMAGE_NV12;
    sprintf(cc_cfg.target_string, TIVX_TARGET_MPU_0);

    cc_node = tiovx_modules_add_node(&graph, TIOVX_DL_COLOR_CONVERT, (void *)&cc_cfg);

    //Link dof viz out to color convert
    tiovx_modules_link_pads(&dof_viz_node->srcs[0], &cc_node->sinks[0]);

    //MOSAIC
    tiovx_mosaic_init_cfg(&mosaic_cfg);

    mosaic_cfg.color_format = VX_DF_IMAGE_NV12;
    mosaic_cfg.num_inputs = 1;
    mosaic_cfg.input_cfgs[0].width = 1280;
    mosaic_cfg.input_cfgs[0].height = 720;
    mosaic_cfg.output_cfg.width = 1920;
    mosaic_cfg.output_cfg.height = 1080;
    mosaic_cfg.params.num_windows  = 1;
    mosaic_cfg.params.windows[0].startX  = 0;
    mosaic_cfg.params.windows[0].startY  = 0;
    mosaic_cfg.params.windows[0].width   = 1280;
    mosaic_cfg.params.windows[0].height  = 720;
    mosaic_cfg.params.windows[0].input_select   = 0;
    mosaic_cfg.params.windows[0].channel_select = 0;
    mosaic_cfg.params.clear_count  = 4;

    mosaic_node = tiovx_modules_add_node(&graph, TIOVX_MOSAIC,
                                         (void *)&mosaic_cfg);
    //Link cc out to mosaic
    tiovx_modules_link_pads(&cc_node->srcs[0], &mosaic_node->sinks[0]);

    status = tiovx_modules_verify_graph(&graph);

    in_buf_pool = viss_node->sinks[0].buf_pool;
    out_buf_pool = mosaic_node->srcs[0].buf_pool;

    v4l2_capture_init_cfg(&v4l2_capture_cfg);
    v4l2_capture_cfg.width = INPUT_WIDTH;
    v4l2_capture_cfg.height = INPUT_HEIGHT;
    v4l2_capture_cfg.pix_format = V4L2_PIX_FMT_SRGGB8;
    v4l2_capture_cfg.bufq_depth = APP_BUFQ_DEPTH + 1;
    sprintf(v4l2_capture_cfg.device, "/dev/video-rpi-cam0");

    v4l2_capture_handle = v4l2_capture_create_handle(&v4l2_capture_cfg);

    kms_display_init_cfg(&kms_display_cfg);
    kms_display_cfg.width = INPUT_WIDTH;
    kms_display_cfg.height = INPUT_HEIGHT;
    kms_display_cfg.pix_format = DRM_FORMAT_NV12;

    kms_display_handle = kms_display_create_handle(&kms_display_cfg);

    aewb_init_cfg(&aewb_cfg);
    sprintf(aewb_cfg.device, "/dev/v4l-rpi-subdev0");
    sprintf(aewb_cfg.dcc_2a_file, "/opt/imaging/imx219/linear/dcc_2a.bin");

    aewb_handle = aewb_create_handle(&aewb_cfg);

    h3a_buf_pool = viss_node->srcs[viss_node->num_outputs - 1].buf_pool;
    h3a_buf = tiovx_modules_acquire_buf(h3a_buf_pool);
    aewb_buf_pool = viss_node->sinks[1].buf_pool;
    aewb_buf = tiovx_modules_acquire_buf(viss_node->sinks[1].buf_pool);

    for (int i = 0; i < out_buf_pool->bufq_depth; i++) {
        kms_display_register_buf(kms_display_handle, &out_buf_pool->bufs[i]);
    }

    for (int i = 0; i < APP_BUFQ_DEPTH; i++) {
        inbuf = tiovx_modules_acquire_buf(in_buf_pool);
        v4l2_capture_enqueue_buf(v4l2_capture_handle, inbuf);
    }

    v4l2_capture_start(v4l2_capture_handle);

    for (int i = 0; i < APP_NUM_ITERATIONS; i++) {
        inbuf = v4l2_capture_dqueue_buf(v4l2_capture_handle);
        tiovx_modules_enqueue_buf(inbuf);

        outbuf = tiovx_modules_acquire_buf(out_buf_pool);
        tiovx_modules_enqueue_buf(outbuf);

        tiovx_modules_enqueue_buf(h3a_buf);
        tiovx_modules_enqueue_buf(aewb_buf);

        tiovx_modules_schedule_graph(&graph);
        tiovx_modules_wait_graph(&graph);

        inbuf = tiovx_modules_dequeue_buf(in_buf_pool);
        outbuf = tiovx_modules_dequeue_buf(out_buf_pool);

        v4l2_capture_enqueue_buf(v4l2_capture_handle, inbuf);

        kms_display_render_buf(kms_display_handle, outbuf);
        tiovx_modules_release_buf(outbuf);

        aewb_process(aewb_handle, h3a_buf, aewb_buf);
        h3a_buf = tiovx_modules_dequeue_buf(h3a_buf_pool);
        aewb_buf = tiovx_modules_dequeue_buf(aewb_buf_pool);
    }

    v4l2_capture_stop(v4l2_capture_handle);
    v4l2_capture_delete_handle(v4l2_capture_handle);

    tiovx_modules_clean_graph(&graph);

    return status;
}
