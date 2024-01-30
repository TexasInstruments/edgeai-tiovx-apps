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

#include "v4l2_capture_module.h"
#include "tiovx_utils.h"

#include <sys/ioctl.h>
#include <errno.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#define V4L2_CAPTURE_DEFAULT_WIDTH 1920
#define V4L2_CAPTURE_DEFAULT_HEIGHT 1080
#define V4L2_CAPTURE_DEFAULT_PIX_FMT V4L2_PIX_FMT_SRGGB8
#define V4L2_CAPTURE_DEFAULT_DEVICE "/dev/video-rpi-cam0"
#define V4L2_CAPTURE_DEFAULT_BUFQ_DEPTH 4
#define V4L2_CAPTURE_MAX_BUFQ_DEPTH 8

void v4l2_capture_init_cfg(v4l2CaptureCfg *cfg)
{
    cfg->width = V4L2_CAPTURE_DEFAULT_WIDTH;
    cfg->height = V4L2_CAPTURE_DEFAULT_HEIGHT;
    cfg->pix_format = V4L2_CAPTURE_DEFAULT_PIX_FMT;
    cfg->bufq_depth = V4L2_CAPTURE_DEFAULT_BUFQ_DEPTH;
    sprintf(cfg->device, V4L2_CAPTURE_DEFAULT_DEVICE);
}

struct _v4l2CaptureHandle {
    v4l2CaptureCfg cfg;
    int fd;
    Buf *bufq[V4L2_CAPTURE_MAX_BUFQ_DEPTH];
    uint8_t head;
    uint8_t tail;
};

static int xioctl(int fh, int request, void *arg)
{
    int r;

    do {
        r = ioctl(fh, request, arg);
    } while (-1 == r && EINTR == errno);

    return r;
}

int v4l2_capture_check_caps(v4l2CaptureHandle *handle)
{
    struct v4l2_capability cap;
    int status = 0;

    if (-1 == xioctl(handle->fd, VIDIOC_QUERYCAP, &cap)) {
        if (EINVAL == errno) {
            TIOVX_MODULE_ERROR("[V4L2_CAPTURE] %s is no V4L2 device\n",
                    handle->cfg.device);
        } else {
            TIOVX_MODULE_ERROR("[V4L2_CAPTURE] VIDIOC_QUERYCAP Failed\n");
        }

        status = -1;
    }

    if (!(cap.capabilities & V4L2_CAP_VIDEO_CAPTURE)) {
        TIOVX_MODULE_ERROR("[V4L2_CAPTURE] %s is not a capture device\n",
                    handle->cfg.device);
        status = -1;
    }

    if (!(cap.capabilities & V4L2_CAP_STREAMING)) {
        TIOVX_MODULE_ERROR("[V4L2_CAPTURE] %s does not support streaming i/o\n",
                    handle->cfg.device);
        status = -1;
    }

    return status;
}

int v4l2_capture_set_fmt(v4l2CaptureHandle *handle)
{
    struct v4l2_format fmt;
    int status = 0;

    CLR(&fmt);
    fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    fmt.fmt.pix.width = handle->cfg.width;
    fmt.fmt.pix.height = handle->cfg.height;
    fmt.fmt.pix.pixelformat = handle->cfg.pix_format;
    fmt.fmt.pix.field = V4L2_FIELD_ANY;

    if (-1 == xioctl(handle->fd, VIDIOC_S_FMT, &fmt)) {
        TIOVX_MODULE_ERROR("[V4L2_CAPTURE] Set format failed\n");
        status = -1;
    }

    return status;
}

int v4l2_capture_request_buffers(v4l2CaptureHandle *handle)
{
    struct v4l2_requestbuffers req;
    int status = 0;

    CLR(&req);
    req.count = handle->cfg.bufq_depth;
    req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    req.memory = V4L2_MEMORY_DMABUF;

    if (-1 == xioctl(handle->fd, VIDIOC_REQBUFS, &req)) {
        if (EINVAL == errno) {
            TIOVX_MODULE_ERROR("[V4L2_CAPTURE] %s does not support DMABUF\n",
                    handle->cfg.device);
        } else {
            TIOVX_MODULE_ERROR("[V4L2_CAPTURE] VIDIOC_REQBUFS failed\n");
        }
        status = -1;

        return status;
    }

    if (req.count < handle->cfg.bufq_depth) {
        TIOVX_MODULE_ERROR("Failed to allocate requested buffers,"
                           " requested %d, allocted %d\n",
                           handle->cfg.bufq_depth, req.count);
        status = -1;
    }

    return status;
}

v4l2CaptureHandle *v4l2_capture_create_handle(v4l2CaptureCfg *cfg)
{
    v4l2CaptureHandle *handle = NULL;

    handle = malloc(sizeof(v4l2CaptureHandle));
    handle->fd = -1;
    memcpy(&handle->cfg, cfg, sizeof(v4l2CaptureHandle));

    handle->fd = open(cfg->device, O_RDWR | O_NONBLOCK, 0);
    if (-1 == handle->fd) {
        TIOVX_MODULE_ERROR("[V4L2_CAPTURE] Cannot open '%s': %d, %s\n",
                            cfg->device, errno, strerror(errno));
        goto free_handle;
    }

    if (0 != v4l2_capture_check_caps(handle)) {
        TIOVX_MODULE_ERROR("[V4L2_CAPTURE] Check caps failed\n");
        goto free_fd;
    }

    if (0 != v4l2_capture_set_fmt(handle)) {
        TIOVX_MODULE_ERROR("[V4L2_CAPTURE] Set format failed\n");
        goto free_fd;
    }

    if (0 != v4l2_capture_request_buffers(handle)) {
        TIOVX_MODULE_ERROR("[V4L2_CAPTURE] request buffers failed\n");
        goto free_fd;
    }

    handle->head = 0;
    handle->tail = handle->cfg.bufq_depth - 1;

    return handle;

free_fd:
    close(handle->fd);
free_handle:
    free(handle);
    return NULL;
}

int v4l2_capture_start(v4l2CaptureHandle *handle)
{
    int status = 0;
    enum v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

    if (-1 == xioctl(handle->fd, VIDIOC_STREAMON, &type)) {
        TIOVX_MODULE_ERROR("[V4L2_CAPTURE] VIDIOC_STREAMON failed\n");
        status = -1;
    }

    return status;
}

int v4l2_capture_enqueue_buf(v4l2CaptureHandle *handle, Buf *tiovx_buffer)
{
    struct v4l2_buffer buf;
    int status = 0;

    if (handle->head == handle->tail) {
        TIOVX_MODULE_ERROR("[V4L2_CAPTURE] Queue Full\n");
        status = -1;
        goto ret;
    }

    CLR(&buf);
    buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    buf.memory = V4L2_MEMORY_DMABUF;
    buf.index = handle->head;
    buf.m.fd = getDmaFd(tiovx_buffer->handle);

    if (-1 == xioctl(handle->fd, VIDIOC_QBUF, &buf)) {
        TIOVX_MODULE_ERROR("[V4L2_CAPTURE] VIDIOC_QBUF failed\n");
        status = -1;
    } else {
        handle->bufq[handle->head] = tiovx_buffer;
        handle->head = (handle->head + 1) % handle->cfg.bufq_depth;
    }

ret:
    return status;
}

Buf *v4l2_capture_dqueue_buf(v4l2CaptureHandle *handle)
{
    Buf *tiovx_buffer = NULL;
    struct v4l2_buffer buf;

    if ((handle->tail + 1) % handle->cfg.bufq_depth == handle->head) {
        TIOVX_MODULE_ERROR("[V4L2_CAPTURE] Queue Empty\n");
        goto ret;
    } else {
        handle->tail = (handle->tail + 1) % handle->cfg.bufq_depth;
        tiovx_buffer = handle->bufq[handle->tail];
        handle->bufq[handle->tail] = NULL;
    }

    CLR(&buf);
    buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    buf.memory = V4L2_MEMORY_DMABUF;

    while (1) {
        if (-1 == xioctl(handle->fd, VIDIOC_DQBUF, &buf)) {
            if (errno == EAGAIN) {
                continue;
            } else {
                TIOVX_MODULE_ERROR("[V4L2_CAPTURE] VIDIOC_DQBUF failed\n");
                break;
            }
        } else {
            break;
        }
    }

ret:
    return tiovx_buffer;
}

int v4l2_capture_stop(v4l2CaptureHandle *handle)
{
    int status = 0;
    enum v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

    if (-1 == xioctl(handle->fd, VIDIOC_STREAMOFF, &type)) {
        TIOVX_MODULE_ERROR("[V4L2_CAPTURE] VIDIOC_STREAMOFF failed\n");
        status = -1;
    }

    return status;
}

int v4l2_capture_delete_handle(v4l2CaptureHandle *handle)
{
    int status = 0;

    close(handle->fd);
    free(handle);

    return status;
}
