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
#include <common/include/edgeai_display.h>
#include <utils/include/ti_logger.h>

namespace ti::edgeai::common
{

#define RTOS_DISPLAY_WIDTH  1920
#define RTOS_DISPLAY_HEIGHT 1080

using namespace ti::utils;

display::display()
{
    LOG_DEBUG("display CONSTRUCTOR\n");
}


display::~display()
{
    LOG_DEBUG("display DESTRUCTOR\n");

#if !defined(SOC_AM62A)
    appGrpxDeInit();
    tiovx_display_module_delete(&displayObj);
    tiovx_display_module_deinit(&displayObj);
#endif
}

#if !defined (SOC_AM62A)
static void app_draw_perf_graphics(Draw2D_Handle *draw2d_obj,
                                    Draw2D_BufInfo *draw2d_buf_info,
                                    uint32_t update_type)
{
    uint16_t cpuWidth = 0;
    uint16_t cpuHeight = 0;

    uint16_t hwaWidth = 0;
    uint16_t hwaHeight = 0;

    uint16_t ddrWidth = 0;
    uint16_t ddrHeight = 0;

    uint16_t startX, startY;

    if (draw2d_obj == NULL)
    {
        return;
    }

    if(update_type==0)
    {
        appGrpxShowLogo(0, 0);
    }
    else
    {
        /* Get height and width for cpu,hwa & ddr graphs*/
        appGrpxGetDimCpuLoad(&cpuWidth, &cpuHeight);
        appGrpxGetDimHwaLoad(&hwaWidth, &hwaHeight);
        appGrpxGetDimDdrLoad(&ddrWidth, &ddrHeight);

        /* Draw the graphics. */
        startX =
        (draw2d_buf_info->bufWidth - (cpuWidth + hwaWidth + ddrWidth)) / 2;
        startY = draw2d_buf_info->bufHeight - cpuHeight;
        appGrpxShowCpuLoad(startX, startY);

        startX = startX + cpuWidth;
        startY = draw2d_buf_info->bufHeight - hwaHeight;
        appGrpxShowHwaLoad(startX, startY);

        startX = startX + hwaWidth;
        startY = draw2d_buf_info->bufHeight - ddrHeight;
        appGrpxShowDdrLoad(startX, startY);
    }

    return;
}
#endif

int32_t display::getConfig(vx_context context, OutputInfo *output)
{
    int32_t status = VX_SUCCESS;

    tiovx_display_module_params_init(&displayObj);

    displayObj.input_width  = output->m_width;
    displayObj.input_height = output->m_height;
    displayObj.input_color_format = VX_DF_IMAGE_NV12;

    displayObj.params.outWidth  = output->m_width;
    displayObj.params.outHeight = output->m_height;

    /* Rtos display has fixed resolution of 1920x1080.*/
    displayObj.params.posX = (RTOS_DISPLAY_WIDTH - output->m_width)/2;
    displayObj.params.posY = (RTOS_DISPLAY_HEIGHT - output->m_height)/2;

    /* Overlay preformance */
    appGrpxInitParamsInit(&grpx_prms, context);
    /* Rtos display has fixed resolution of 1920x1080.*/
    grpx_prms.width = RTOS_DISPLAY_WIDTH;
    grpx_prms.height = RTOS_DISPLAY_HEIGHT;
    /* Custom callback for perf overlay*/
    grpx_prms.draw_callback = app_draw_perf_graphics;

    return status;
}

int32_t display::displayInit(vx_context context, OutputInfo *output)
{
    int32_t status = VX_SUCCESS;

    /* Get config for multiscaler module. */
    getConfig(context, output);

    /* Init display module. */
    status = tiovx_display_module_init(context, &displayObj);

    /* Init perf overlay display module */
    if(status == VX_SUCCESS)
    {
        status = appGrpxInit(&grpx_prms);
    }

    return status;
}

} /* namespace ti::edgeai::common */