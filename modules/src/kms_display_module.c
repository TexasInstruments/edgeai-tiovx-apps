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

#include "kms_display_module.h"
#include "tiovx_utils.h"

#include <drm.h>
#include <xf86drm.h>
#include <xf86drmMode.h>
#include <string.h>
#include <stdio.h>
#include <stdint.h>
#include <errno.h>
#include <unistd.h>
#include <poll.h>

#define KMS_DISPLAY_DEFAULT_WIDTH 1920
#define KMS_DISPLAY_DEFAULT_HEIGHT 1080
#define KMS_DISPLAY_DEFAULT_PIX_FMT DRM_FORMAT_NV12

#if defined(SOC_AM62A) || defined(SOC_J722S) || defined(SOC_J721S2)
    #define KMS_DISPLAY_DEFAULT_CRTC 39
    #define KMS_DISPLAY_DEFAULT_CONNECTOR 41
#elif defined(SOC_J784S4) || defined(SOC_J742S2)
    #define KMS_DISPLAY_DEFAULT_CRTC 49
    #define KMS_DISPLAY_DEFAULT_CONNECTOR 51
#elif defined(SOC_J721E)
    #define KMS_DISPLAY_DEFAULT_CRTC 49
    #define KMS_DISPLAY_DEFAULT_CONNECTOR 51
#endif

#define KMS_DISPLAY_DEFAULT_BUFQ_DEPTH 4
#define KMS_DISPLAY_MAX_BUFQ_DEPTH 8
#define KMS_DISPLAY_PAGE_FLIP_TIMEOUT 50

void kms_display_init_cfg(kmsDisplayCfg *cfg)
{
    CLR(cfg);
    cfg->width = KMS_DISPLAY_DEFAULT_WIDTH;
    cfg->height = KMS_DISPLAY_DEFAULT_HEIGHT;
    cfg->pix_format = KMS_DISPLAY_DEFAULT_PIX_FMT;
    cfg->crtc = KMS_DISPLAY_DEFAULT_CRTC;
    cfg->connector = KMS_DISPLAY_DEFAULT_CONNECTOR;
    cfg->bufq_depth = KMS_DISPLAY_DEFAULT_BUFQ_DEPTH;
}

struct _kmsDisplayHandle {
    kmsDisplayCfg cfg;
    int fd;
    uint32_t fbs[KMS_DISPLAY_MAX_BUFQ_DEPTH];
    drmModeCrtcPtr crtc_info;
};

kmsDisplayHandle *kms_display_create_handle(kmsDisplayCfg *cfg)
{
    kmsDisplayHandle *handle = NULL;

    handle = malloc(sizeof(kmsDisplayHandle));
    handle->fd = -1;
    memcpy(&handle->cfg, cfg, sizeof(kmsDisplayCfg));

    handle->fd = drmOpen("tidss", NULL);
    if (handle->fd <= 0) {
        TIOVX_MODULE_ERROR("[KMS_DISPLAY] Failed to open DRM device: %d, %s\n",
                            errno, strerror(errno));
        goto free_handle;
    }

    handle->crtc_info = drmModeGetCrtc(handle->fd, handle->cfg.crtc);

    return handle;

free_handle:
    free(handle);
    return NULL;
}

int kms_display_register_buf(kmsDisplayHandle *handle, Buf *tiovx_buffer)
{
    int status = 0;
    uint32_t gem_handle[4], pitch[4];
    int fd[4];
    long unsigned int size[4];
    unsigned int offset[4];
    static int modeset_done = 0;

    if (tiovx_buffer->buf_index >= KMS_DISPLAY_MAX_BUFQ_DEPTH) {
        TIOVX_MODULE_ERROR("[KMS_DISPLAY] Registering buf failed, buf index: %d"
                            " >= bufq depth %d\n",
                            tiovx_buffer->buf_index, handle->cfg.bufq_depth);
        status = -1;
        goto ret;
    }

    getImageDmaFd(tiovx_buffer->handle, fd, pitch, size, offset, 2);
    for (int i=0; i<2; i++) {
        drmPrimeFDToHandle(handle->fd, fd[i], &gem_handle[i]);
    }
    drmModeAddFB2(handle->fd, handle->cfg.width, handle->cfg.height,
            handle->cfg.pix_format, gem_handle, pitch, offset,
            &handle->fbs[tiovx_buffer->buf_index], 0);

    if (modeset_done == 0) {
        drmModeSetCrtc(handle->fd, handle->cfg.crtc,
                handle->fbs[tiovx_buffer->buf_index], 0, 0,
                &handle->cfg.connector, 1, &handle->crtc_info->mode);
        modeset_done = 1;
    }
ret:
    return status;
}

void page_flip_handler(int fd, uint32_t dequence, uint32_t tv_sec,
                        uint32_t tv_usec, void *user_data)
{
    *((int *)user_data) = 1;
}

drmEventContext drm_event = {
    .version = DRM_EVENT_CONTEXT_VERSION,
    .page_flip_handler = page_flip_handler,
};

int kms_display_render_buf(kmsDisplayHandle *handle, Buf *tiovx_buffer)
{
    int status = 0;
    struct pollfd pfd;
    int page_flip_done = 0;
    static int first_frame = 1;

    if (first_frame == 1) {
        first_frame = 0;
    } else {
        CLR(&pfd);
        pfd.fd = handle->fd;
        pfd.events = POLLIN;
        pfd.revents = 0;

        while (page_flip_done == 0) {
            poll(&pfd, 1, KMS_DISPLAY_PAGE_FLIP_TIMEOUT);
            status = drmHandleEvent(handle->fd, &drm_event);
        }
    }

    drmModePageFlip(handle->fd, handle->cfg.crtc,
            handle->fbs[tiovx_buffer->buf_index], DRM_MODE_PAGE_FLIP_EVENT,
            &page_flip_done);

    return status;
}

int kms_display_delete_handle(kmsDisplayHandle *handle)
{
    int status = 0;

    close(handle->fd);
    free(handle);

    return status;
}
