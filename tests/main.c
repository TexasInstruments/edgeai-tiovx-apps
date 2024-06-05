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

#include <stdio.h>
#include <TI/tivx.h>
#include <app_init.h>
#include <stdlib.h>
#include <tiovx_modules.h>

#define APP_MODULES_TEST_COLOR_CONVERT (1)
#define APP_MODULES_TEST_DL_COLOR_CONVERT (1)
#define APP_MODULES_TEST_MULTI_SCALER (1)
#define APP_MODULES_TEST_VISS (1)
#define APP_MODULES_TEST_LDC (1)
#define APP_MODULES_TEST_VISS_LDC_MSC (1)
#define APP_MODULES_TEST_TEE (1)
#define APP_MODULES_TEST_MOSAIC (1)
#define APP_MODULES_TEST_CAPTURE (0)
#define APP_MODULES_TEST_TIDL (0)
#define APP_MODULES_TEST_DL_PRE_PROC (0)
#define APP_MODULES_TEST_DL_POST_PROC (0)
#define APP_MODULES_TEST_DL_PIPELINE (0)
#define APP_MODULES_TEST_DISPLAY (0)
#define APP_MODULES_TEST_V4L2_CAPTURE (0)
#define APP_MODULES_TEST_LINUX_CAPTURE_DISPLAY (0)
#define APP_MODULES_TEST_LINUX_DECODE_DISPLAY (0)
#define APP_MODULES_TEST_LINUX_CAPTURE_ENCODE (0)
#define APP_MODULES_TEST_LINUX_MULTI_CAPTURE_DISPLAY (0)
#define APP_MODULES_TEST_CAPTURE_VISS_LDC_MSC_DISPLAY (0)
#define APP_MODULES_TEST_CAPTURE_DL_DISPLAY (0)
#define APP_MODULES_TEST_PYRAMID (1)
#define APP_MODULES_TEST_SDE (1)
#define APP_MODULES_TEST_DOF (1)
#define APP_MODULES_TEST_LINUX_CAPTURE_DOF (0)
#define APP_MODULES_TEST_LINUX_DECODE_SDE (0)
#define APP_MODULES_TEST_PIXELWISE_MULTIPLY (1)
#define APP_MODULES_TEST_PIXELWISE_ADD (1)

char *EDGEAI_DATA_PATH;

int main(int argc, char *argv[])
{
    int status = 0;

    EDGEAI_DATA_PATH = getenv("EDGEAI_DATA_PATH");
    if (EDGEAI_DATA_PATH == NULL)
    {
      TIOVX_MODULE_ERROR("EDGEAI_DATA_PATH Not Defined!!\n");
    }

    status = appInit();

#if (APP_MODULES_TEST_MULTI_SCALER)
    if(status==0)
    {
        printf("Running Multi Scaler module test\n");
        int app_modules_multi_scaler_test(int argc, char* argv[]);

        status = app_modules_multi_scaler_test(argc, argv);
    }
#endif

#if (APP_MODULES_TEST_DL_COLOR_CONVERT)
    if(status==0)
    {
        printf("Running DL color convert module test\n");
        int app_modules_dl_color_convert_test(int argc, char* argv[]);

        status = app_modules_dl_color_convert_test(argc, argv);
    }
#endif

#if (APP_MODULES_TEST_COLOR_CONVERT)
    if(status==0)
    {
        printf("Running color convert module test\n");
        int app_modules_color_convert_test(int argc, char* argv[]);

        status = app_modules_color_convert_test(argc, argv);
    }
#endif

#if (APP_MODULES_TEST_VISS)
    if(status==0)
    {
        printf("Running viss module test\n");
        int app_modules_viss_test(int argc, char* argv[]);

        status = app_modules_viss_test(argc, argv);
    }
#endif

#if (APP_MODULES_TEST_LDC)
    if(status==0)
    {
        printf("Running ldc module test\n");
        int app_modules_ldc_test(int argc, char* argv[]);

        status = app_modules_ldc_test(argc, argv);
    }
#endif

#if (APP_MODULES_TEST_VISS_LDC_MSC)
    if(status==0)
    {
        printf("Running viss->ldc->msc pipeline test\n");
        int app_modules_viss_ldc_msc_test(int argc, char* argv[]);

        status = app_modules_viss_ldc_msc_test(argc, argv);
    }
#endif

#if (APP_MODULES_TEST_TEE)
    if(status==0)
    {
        printf("Running tee module test\n");
        int app_modules_tee_test(int argc, char* argv[]);

        status = app_modules_tee_test(argc, argv);
    }
#endif
#if (APP_MODULES_TEST_MOSAIC)
    if(status==0)
    {
        printf("Running mosaic module test\n");
        int app_modules_mosaic_test(int argc, char* argv[]);

        status = app_modules_mosaic_test(argc, argv);
    }
#endif
#if (APP_MODULES_TEST_TIDL)
    if(status==0)
    {
        printf("Running tidl module test\n");
        int app_modules_tidl_test(int argc, char* argv[]);

        status = app_modules_tidl_test(argc, argv);
    }
#endif
#if (APP_MODULES_TEST_DL_PRE_PROC)
    if(status==0)
    {
        printf("Running dl pre proc module test\n");
        int app_modules_dl_pre_proc_test(int argc, char* argv[]);

        status = app_modules_dl_pre_proc_test(argc, argv);
    }
#endif
#if (APP_MODULES_TEST_DL_POST_PROC)
    if(status==0)
    {
        printf("Running dl post proc module test\n");
        int app_modules_dl_post_proc_test(int argc, char* argv[]);

        status = app_modules_dl_post_proc_test(argc, argv);
    }
#endif
#if (APP_MODULES_TEST_V4L2_CAPTURE)
    if(status==0)
    {
        printf("Running v4l2 capture module test\n");
        int app_modules_v4l2_capture_test(int argc, char* argv[]);

        status = app_modules_v4l2_capture_test(argc, argv);
    }
#endif
#if (APP_MODULES_TEST_LINUX_CAPTURE_DISPLAY)
    if(status==0)
    {
        printf("Running linux capture module test\n");
        int app_modules_linux_capture_display_test(int argc, char* argv[]);

        status = app_modules_linux_capture_display_test(argc, argv);
    }
#endif
#if (APP_MODULES_TEST_LINUX_DECODE_DISPLAY)
    if(status==0)
    {
        printf("Running linux decode module test\n");
        int app_modules_linux_decode_display_test(int argc, char* argv[]);

        status = app_modules_linux_decode_display_test(argc, argv);
    }
#endif
#if (APP_MODULES_TEST_LINUX_CAPTURE_ENCODE)
    if(status==0)
    {
        printf("Running linux encode module test\n");
        int app_modules_linux_capture_encode_test(int argc, char* argv[]);

        status = app_modules_linux_capture_encode_test(argc, argv);
    }
#endif
#if (APP_MODULES_TEST_LINUX_MULTI_CAPTURE_DISPLAY)
    if(status==0)
    {
        printf("Running linux multi capture display test\n");
        int app_modules_linux_multi_capture_display_test(int argc, char* argv[]);

        status = app_modules_linux_multi_capture_display_test(argc, argv);
    }
#endif
#if (APP_MODULES_TEST_PYRAMID)
    if(status==0)
    {
        printf("Running Pyramid module test\n");
        int app_modules_pyramid_test(int argc, char* argv[]);

        status = app_modules_pyramid_test(argc, argv);
    }
#endif

#if (APP_MODULES_TEST_PIXELWISE_MULTIPLY)
    if(status==0)
    {
        printf("Running Pixelwise Multiply module test\n");
        int app_modules_pixelwise_multiply_test(int argc, char* argv[]);

        status = app_modules_pixelwise_multiply_test(argc, argv);
    }
#endif

#if (APP_MODULES_TEST_PIXELWISE_ADD)
    if(status==0)
    {
        printf("Running Pixelwise Add module test\n");
        int app_modules_pixelwise_add_test(int argc, char* argv[]);

        status = app_modules_pixelwise_add_test(argc, argv);
    }
#endif

#if defined(SOC_J721E) || defined(SOC_J721S2) || defined(SOC_J784S4) || defined(SOC_J722S)
#if (APP_MODULES_TEST_DL_PIPELINE)
    if(status==0)
    {
        printf("Running dl pipeline module test\n");
        int app_modules_dl_pipeline_test(int argc, char* argv[]);

        status = app_modules_dl_pipeline_test(argc, argv);
    }
#endif
#if (APP_MODULES_TEST_DISPLAY)
    if(status==0)
    {
        printf("Running display module test\n");
        int app_modules_display_test(int argc, char* argv[]);

        status = app_modules_display_test(argc, argv);
    }
#endif
#if (APP_MODULES_TEST_CAPTURE)
    if(status==0)
    {
        printf("Running capture module test\n");
        int app_modules_capture_test(int argc, char* argv[]);

        status = app_modules_capture_test(argc, argv);
    }
#endif
#if (APP_MODULES_TEST_SDE)
    if(status==0)
    {
        printf("Running sde module test\n");
        int app_modules_sde_test(int argc, char* argv[]);

        status = app_modules_sde_test(argc, argv);
    }
#endif
#if (APP_MODULES_TEST_DOF)
    if(status==0)
    {
        printf("Running dof module test\n");
        int app_modules_dof_test(int argc, char* argv[]);

        status = app_modules_dof_test(argc, argv);
    }
#endif
#if (APP_MODULES_TEST_CAPTURE_VISS_LDC_MSC_DISPLAY)
    if(status==0)
    {
        printf("Running capture->viss->ldc->msc->display test\n");
        int app_modules_capture_viss_ldc_msc_display_test(int argc, char* argv[]);

        status = app_modules_capture_viss_ldc_msc_display_test(argc, argv);
    }
#endif
#if (APP_MODULES_TEST_CAPTURE_DL_DISPLAY)
    if(status==0)
    {
        printf("Running capture->dl->display test\n");
        int app_modules_capture_dl_display_test(int argc, char* argv[]);

        status = app_modules_capture_dl_display_test(argc, argv);
    }
#endif
#if (APP_MODULES_TEST_LINUX_CAPTURE_DOF)
    if(status==0)
    {
        printf("Running linux capture dof module test\n");
        int app_modules_linux_capture_dof_test(int argc, char* argv[]);

        status = app_modules_linux_capture_dof_test(argc, argv);
    }
#endif
#if (APP_MODULES_TEST_LINUX_DECODE_SDE)
    if(status==0)
    {
        printf("Running linux decode sde module test\n");
        int app_modules_linux_decode_sde_test(int argc, char* argv[]);

        status = app_modules_linux_decode_sde_test(argc, argv);
    }
#endif
#endif

    printf("All tests complete!\n");

    appDeInit();

    return status;
}
