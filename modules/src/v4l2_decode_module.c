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

#include "v4l2_decode_module.h"
#include "tiovx_utils.h"

#include <sys/ioctl.h>
#include <errno.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <poll.h>

#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libavcodec/bsf.h> 
#include <libavutil/pixdesc.h>

#define V4L2_DECODE_DEFAULT_DEVICE "/dev/video0"
#define V4L2_DECODE_DEFAULT_INPUT_FILE "/opt/edgeai-test-data/videos/video0_1280_768.h264"
#define V4L2_DECODE_DEFAULT_BUFQ_DEPTH 4
#define V4L2_DECODE_MAX_BUFQ_DEPTH 10
#define V4L2_DECODE_MAX_BUFQ_DEPTH_OFFSET 3 //This is set in wave5 driver

#define MAX_FRAMES 1000
#define MAX_OUTBUFS 4
#define ALIGN(x,a) (((x) + (a) - 1L) & ~((a) - 1L))
#define HW_ALIGN 64

void v4l2_decode_init_cfg(v4l2DecodeCfg *cfg)
{
    cfg->bufq_depth = V4L2_DECODE_DEFAULT_BUFQ_DEPTH;
    sprintf(cfg->device, V4L2_DECODE_DEFAULT_DEVICE);
    sprintf(cfg->file, V4L2_DECODE_DEFAULT_INPUT_FILE);
}

struct ESdescriptor {
    int length;
    unsigned char *data;
};

struct demux {
    AVFormatContext *afc;
    AVStream *st;
    AVCodecParameters *cc;
    AVBSFContext *bsfcontext;
    struct ESdescriptor esds;
    /* Used for mpeg4 esds data copy */
    int first_in_buff;
};

typedef struct
{
    size_t frame_sizes[MAX_FRAMES];
    int width;
    int height;
    int bitdepth;
    int num_bytes_per_pix;
    enum AVPixelFormat pix_fmt;
    enum AVCodecID codec;
    int profile;
    int level;
    int num_frames;
} streamContext;

static AVFormatContext *open_stream_file(const char *filename)
{
    AVFormatContext *afc = NULL;
    int err = avformat_open_input(&afc, filename, NULL, NULL);

    if (!err)
        err = avformat_find_stream_info(afc, NULL);

    if (err < 0) {
        TIOVX_MODULE_ERROR("[V4L2_DECODE] %s: lavf error %d", filename, err);
        exit(1);
    }

    av_dump_format(afc, 0, filename, 0);

    return afc;
}

static AVStream * find_stream(AVFormatContext *afc)
{
    AVStream *st = NULL;
    unsigned int i;

    for (i = 0; i < afc->nb_streams; i++) {
        if (afc->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO && !st)
            st = afc->streams[i];
        else
            afc->streams[i]->discard = AVDISCARD_ALL;
    }

    return st;
}

struct demux *demux_init(const char * filename, int *width, int *height,
        int *bitdepth, enum AVPixelFormat *pix_fmt, enum AVCodecID *codec)
{
    struct demux *demux = NULL;
    AVFormatContext *afc = open_stream_file(filename);
    AVStream *st = find_stream(afc);
    AVCodecParameters *cc = st->codecpar;
    const AVBitStreamFilter *bsf = NULL;
    AVBSFContext *bsfcontext = NULL;

    if ((cc->codec_id != AV_CODEC_ID_H264) &&
        (cc->codec_id != AV_CODEC_ID_HEVC)) {
        TIOVX_MODULE_ERROR("[V4L2_DECODE] could not open '%s': "
                "unsupported codec %d", filename, cc->codec_id);
        return NULL;
    }

    if (cc->extradata && cc->extradata_size > 0 && cc->extradata[0] == 1) {
        TIOVX_MODULE_ERROR("[V4L2_DECODE] initializing bitstream filter");
        if(cc->codec_id == AV_CODEC_ID_H264) {
            bsf = av_bsf_get_by_name("h264_mp4toannexb");
            av_bsf_alloc(bsf,&bsfcontext);
            av_bsf_init(bsfcontext);
        } else {
            bsf = av_bsf_get_by_name("hevc_mp4toannexb");
            av_bsf_alloc(bsf,&bsfcontext);
            av_bsf_init(bsfcontext);
        }
        if (!bsf) {
            TIOVX_MODULE_ERROR("[V4L2_DECODE] could not open '%s': failed "
                    "to initialize bitstream filter", filename);
            return NULL;
        }
    }

    *width = cc->width;
    *height = cc->height;
    *bitdepth = cc->bits_per_raw_sample;
    *pix_fmt = cc->format;
    *codec = cc->codec_id;

    demux = calloc(1, sizeof(*demux));

    demux->afc = afc;
    demux->cc = cc;
    demux->st = st;
    demux->bsfcontext = bsfcontext;
    demux->first_in_buff = 0;

    return demux;
}

int demux_read(struct demux *demux, unsigned char *input, int size)
{
    AVPacket pk = { };
    int count=-1;
    while (!av_read_frame(demux->afc, &pk)) {
        if (pk.stream_index == demux->st->index) {
            count++;
            uint8_t *buf = NULL;
            int bufsize = 0;

            if (demux->bsfcontext) {
                int ret;
                ret = av_bsf_send_packet(demux->bsfcontext,&pk);
                if (ret < 0) {
                    TIOVX_MODULE_ERROR("[V4L2_DECODE] bsf_send_packet error: %d",
                            ret);
                    return 0;
                }
                while(av_bsf_receive_packet(demux->bsfcontext,&pk) == 0) {
                    bufsize+=pk.size;
                }
            } else {
                buf = pk.data;
                bufsize = pk.size;
            }

            if (bufsize > size){
                bufsize = size;
            }

            if (demux->first_in_buff == 1) {
                memcpy(input, demux->esds.data, demux->esds.length);
                memcpy(input + demux->esds.length, buf, bufsize);
                demux->first_in_buff = 0;
                bufsize = bufsize + demux->esds.length;
            } else {
                memcpy(input, buf, bufsize);
            }

            if (demux->bsfcontext)
                av_free(buf);

            av_packet_unref(&pk);

            return bufsize;
        }
        av_packet_unref(&pk);
    }

    return 0;
}

int demux_rewind(struct demux *demux)
{
    return av_seek_frame(demux->afc, demux->st->index, 0, AVSEEK_FLAG_FRAME);
}

void demux_deinit(struct demux *demux)
{
    avformat_close_input(&demux->afc);
    if (demux->bsfcontext)
         av_bsf_free(&demux->bsfcontext);
    free(demux->esds.data);
    free(demux);
}

int stream_framelevel_parsing(streamContext *str, char *input_file,
        int max_frames)
{
    struct demux *demux;
    int inp_buf_len = 0;
    int inp_width, inp_height;
    int frame_num = 0;
    int frame_size = 0;
    int bitdepth = 0;
    enum AVPixelFormat pix_fmt;
    enum AVCodecID codec;
    unsigned char *inp_buf;

    demux = demux_init(input_file, &inp_width, &inp_height, &bitdepth, &pix_fmt,
                       &codec);
    if (!demux)
    {
        TIOVX_MODULE_ERROR("[V4L2_DECODE] could not open demuxer\n");
        return -1;
    }

    inp_buf_len = (inp_width * inp_height);

    inp_buf = malloc(inp_buf_len);
    if(inp_buf == NULL)
    {
        TIOVX_MODULE_ERROR("[V4L2_DECODE] Memory allocation failed "
                "for inp_buf\n");
        return -1;
    }

    TIOVX_MODULE_PRINTF("[V4L2_DECODE] demuxer is initialized, "
                                "width=%d, height=%d\n", inp_width, inp_height);
    str->width = inp_width;
    str->height = inp_height;
    str->bitdepth = bitdepth;
    str->pix_fmt = pix_fmt;
    str->codec = codec;
    str->profile = demux->cc->profile;
    str->level = demux->cc->level;


    if(bitdepth == 8 || pix_fmt == AV_PIX_FMT_YUV420P ||
            pix_fmt == AV_PIX_FMT_YUV422P)
        str->num_bytes_per_pix = 1;
    if(bitdepth == 10 || pix_fmt == AV_PIX_FMT_YUV420P10LE ||
            pix_fmt == AV_PIX_FMT_YUV422P10LE)
        str->num_bytes_per_pix = 2;

    while(1)
    {
        frame_size = demux_read(demux, inp_buf, inp_buf_len);
        if (frame_size)
        {
            str->frame_sizes[frame_num] = frame_size;
            frame_num++;
        } else {
            break;
        }
        if (max_frames > 0 && frame_num >= max_frames)
            break;
    }

    demux_deinit(demux);
    free(inp_buf);

    str->num_frames = frame_num;

    return frame_num;
}

typedef struct {
    void *mapped;
    int offset;
    int length;
} outBuf;

struct _v4l2DecodeHandle {
    v4l2DecodeCfg cfg;
    int fd;
    Buf *bufq[V4L2_DECODE_MAX_BUFQ_DEPTH];
    bool queued[V4L2_DECODE_MAX_BUFQ_DEPTH];
    streamContext str;
    uint32_t num_outbufs;
    outBuf outbufs[MAX_OUTBUFS];
    FILE *rdfd;
    int current_frame;
    uint8_t outbuf_head;
    uint8_t outbuf_tail;
};

static int xioctl(int fh, int request, void *arg)
{
    int r;

    do {
        r = ioctl(fh, request, arg);
    } while (-1 == r && EINTR == errno);

    return r;
}

int v4l2_decode_check_caps(v4l2DecodeHandle *handle)
{
    struct v4l2_capability cap;
    int status = 0;

    if (-1 == xioctl(handle->fd, VIDIOC_QUERYCAP, &cap)) {
        if (EINVAL == errno) {
            TIOVX_MODULE_ERROR("[V4L2_DECODE] %s is no V4L2 device\n",
                    handle->cfg.device);
        } else {
            TIOVX_MODULE_ERROR("[V4L2_DECODE] VIDIOC_QUERYCAP Failed\n");
        }

        status = -1;
    }

    if (!(cap.capabilities & V4L2_CAP_VIDEO_M2M_MPLANE)) {
        TIOVX_MODULE_ERROR("[V4L2_DECODE] %s is not a M2M device\n",
                    handle->cfg.device);
        status = -1;
    }

    if (!(cap.capabilities & V4L2_CAP_STREAMING)) {
        TIOVX_MODULE_ERROR("[V4L2_DECODE] %s does not support streaming i/o\n",
                    handle->cfg.device);
        status = -1;
    }

    return status;
}

int v4l2_decode_set_fmt(v4l2DecodeHandle *handle)
{
    struct v4l2_format fmt;
    int status = 0;

    CLR(&fmt);
    fmt.type = V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE;
    fmt.fmt.pix_mp.width = handle->str.width;
    fmt.fmt.pix_mp.height = handle->str.height;
    fmt.fmt.pix_mp.plane_fmt[0].sizeimage = (handle->str.width * handle->str.height);
    fmt.fmt.pix_mp.num_planes = 1;

    if(handle->str.codec == AV_CODEC_ID_H264) {
        fmt.fmt.pix_mp.pixelformat = V4L2_PIX_FMT_H264;
    } else if(handle->str.codec == AV_CODEC_ID_HEVC) {
        fmt.fmt.pix_mp.pixelformat = V4L2_PIX_FMT_HEVC;
    }

    if (0 > xioctl(handle->fd, VIDIOC_S_FMT, &fmt)) {
        TIOVX_MODULE_ERROR("[V4L2_DECODE] Output Set format failed\n");
        status = -1;
        goto ret;
    }

    fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
    fmt.fmt.pix_mp.pixelformat = V4L2_PIX_FMT_NV12;
    fmt.fmt.pix_mp.plane_fmt[0].sizeimage =
                                fmt.fmt.pix_mp.width * fmt.fmt.pix_mp.height *
                                handle->str.num_bytes_per_pix * 3 /2;
    fmt.fmt.pix_mp.width = ALIGN(handle->str.width, HW_ALIGN);
    fmt.fmt.pix_mp.height = ALIGN(handle->str.height, HW_ALIGN);
    fmt.fmt.pix_mp.num_planes = 1;
    fmt.fmt.pix_mp.plane_fmt[0].bytesperline =
                                            ALIGN(handle->str.width, HW_ALIGN) *
                                            handle->str.num_bytes_per_pix;

    if (0 > xioctl(handle->fd, VIDIOC_S_FMT, &fmt)) {
        TIOVX_MODULE_ERROR("[V4L2_DECODE] Capture Set format failed\n");
        status = -1;
        goto ret;
    }

ret:
    return status;
}

int v4l2_decode_request_capture_buffers(v4l2DecodeHandle *handle)
{
    struct v4l2_requestbuffers req;
    int status = 0;

    CLR(&req);
    req.count = handle->cfg.bufq_depth + V4L2_DECODE_MAX_BUFQ_DEPTH_OFFSET;
    req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
    req.memory = V4L2_MEMORY_DMABUF;

    if (0 > xioctl(handle->fd, VIDIOC_REQBUFS, &req)) {
        if (EINVAL == errno) {
            TIOVX_MODULE_ERROR("[V4L2_DECODE] %s does not support DMABUF\n",
                    handle->cfg.device);
        } else {
            TIOVX_MODULE_ERROR("[V4L2_DECODE] VIDIOC_REQBUFS failed\n");
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

int v4l2_decode_request_output_buffers(v4l2DecodeHandle *handle)
{
    struct v4l2_requestbuffers req;
    int status = 0;

    CLR(&req);
    req.count = MAX_OUTBUFS;
    req.type = V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE;
    req.memory = V4L2_MEMORY_MMAP;

    if (0 > xioctl(handle->fd, VIDIOC_REQBUFS, &req)) {
        TIOVX_MODULE_ERROR("[V4L2_DECODE] VIDIOC_REQBUFS failed\n");
        status = -1;
        return status;
    }

    handle->num_outbufs = req.count;

    return status;
}

int v4l2_decode_map_output_buffers(v4l2DecodeHandle *handle)
{
    struct v4l2_buffer buffer;
    struct v4l2_plane buf_planes[1];
    int status = 0;

    for (int i = 0; i < handle->num_outbufs; i++) {
        CLR(&buffer);
        buffer.type = V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE;
        buffer.index = i;
        buffer.m.planes = buf_planes;
        buffer.length = 1;

        if (0 > xioctl(handle->fd, VIDIOC_QUERYBUF, &buffer)) {
            TIOVX_MODULE_ERROR("[V4L2_DECODE] VIDIOC_QUERYBUF failed "
                               ": buf index = %d\n", i);
            status = -1;
            return status;
        }

        handle->outbufs[i].mapped = mmap(NULL, buffer.m.planes[0].length,
                                PROT_READ | PROT_WRITE, MAP_SHARED, handle->fd,
                                buffer.m.planes[0].m.mem_offset);
        handle->outbufs[i].offset = buffer.m.planes[0].m.mem_offset;
        handle->outbufs[i].length = buffer.m.planes[0].length;

        if (MAP_FAILED == handle->outbufs[i].mapped) {
            TIOVX_MODULE_ERROR("[V4L2_DECODE] mmap buffer failed "
                               ": buf index = %d\n", i);
            status = -1;
            return status;
        }
    }
    return status;
}

int v4l2_decode_enqueue_outbuf(v4l2DecodeHandle *handle)
{
    struct v4l2_buffer buf;
    struct v4l2_plane buf_planes[1];
    int status = 0;
    outBuf *outbuf;

    if (handle->current_frame >= handle->str.num_frames) {
        rewind(handle->rdfd);
        handle->current_frame = 0;
    }

    if (handle->outbuf_head == handle->outbuf_tail) {
        TIOVX_MODULE_ERROR("[V4L2_DECODE] Queue Full\n");
        status = -1;
        goto ret;
    }

    CLR(&buf);

    outbuf = &handle->outbufs[handle->outbuf_head];
    memset(outbuf->mapped, 0, outbuf->length);
    fread(outbuf->mapped, handle->str.frame_sizes[handle->current_frame],
            1, handle->rdfd);

    buf.type = V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE;
    buf.memory = V4L2_MEMORY_MMAP;
    buf.m.planes = buf_planes;
    buf.length = 1;
    buf.index = handle->outbuf_head;

    CLR(&buf_planes[0]);
    buf_planes[0].bytesused = handle->str.frame_sizes[handle->current_frame];
    buf_planes[0].m.mem_offset = outbuf->offset;
    buf_planes[0].data_offset = 0;

    if (-1 == xioctl(handle->fd, VIDIOC_QBUF, &buf)) {
        TIOVX_MODULE_ERROR("[V4L2_DECODE] VIDIOC_QBUF failed\n");
        status = -1;
    } else {
        handle->outbuf_head = (handle->outbuf_head + 1) % handle->num_outbufs;
    }

    handle->current_frame += 1;

ret:
    return status;

}

int v4l2_decode_dqueue_outbuf(v4l2DecodeHandle *handle)
{
    struct v4l2_buffer buf;
    struct v4l2_plane buf_planes[3];
    int status = 0;

    if ((handle->outbuf_tail + 1) % handle->num_outbufs == handle->outbuf_head) {
        TIOVX_MODULE_ERROR("[V4L2_DECODE] Queue Empty\n");
        status = -1;
        return status;
    } else {
        handle->outbuf_tail = (handle->outbuf_tail + 1) %
                                                handle->num_outbufs;
    }

    CLR(&buf);
    buf.type = V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE;
    buf.memory = V4L2_MEMORY_MMAP;
    buf.m.planes = buf_planes;
    buf.length = 1;

    while (1) {
        if (-1 == xioctl(handle->fd, VIDIOC_DQBUF, &buf)) {
            if (errno == EAGAIN) {
                continue;
            } else {
                TIOVX_MODULE_ERROR("[V4L2_DECODE] VIDIOC_DQBUF failed\n");
                status = -1;
                break;
            }
        } else {
            break;
        }
    }

    return status;
}

int v4l2_decode_start_output(v4l2DecodeHandle *handle)
{
    int status = 0;
    enum v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE;

    if (-1 == xioctl(handle->fd, VIDIOC_STREAMON, &type)) {
        TIOVX_MODULE_ERROR("[V4L2_DECODE] output VIDIOC_STREAMON failed\n");
        status = -1;
    }

    return status;
}

v4l2DecodeHandle *v4l2_decode_create_handle(v4l2DecodeCfg *cfg,
                                            v4l2DecodeOutFmt *out_fmt)
{
    v4l2DecodeHandle *handle = NULL;

    handle = malloc(sizeof(v4l2DecodeHandle));
    handle->fd = -1;
    handle->current_frame = 0;
    memcpy(&handle->cfg, cfg, sizeof(v4l2DecodeHandle));

    if (stream_framelevel_parsing(&handle->str, cfg->file, MAX_FRAMES) < 0) {
        TIOVX_MODULE_ERROR("[V4L2_DECODE] Stream parsing failed\n");
        goto free_handle;
    }

    handle->rdfd = fopen(cfg->file, "r");
    if (handle->rdfd  == NULL) {
        TIOVX_MODULE_ERROR("[V4L2_DECODE] Failed to open input file %s\n",
                            cfg->file);
        goto free_handle;
    }

    handle->fd = open(cfg->device, O_RDWR | O_NONBLOCK, 0);
    if (-1 == handle->fd) {
        TIOVX_MODULE_ERROR("[V4L2_DECODE] Cannot open '%s': %d, %s\n",
                            cfg->device, errno, strerror(errno));
        goto free_handle;
    }

    if (0 != v4l2_decode_check_caps(handle)) {
        TIOVX_MODULE_ERROR("[V4L2_DECODE] Check caps failed\n");
        goto free_fd;
    }

    if (0 != v4l2_decode_set_fmt(handle)) {
        TIOVX_MODULE_ERROR("[V4L2_DECODE] Set format failed\n");
        goto free_fd;
    }

    if (0 != v4l2_decode_request_output_buffers(handle)) {
        TIOVX_MODULE_ERROR("[V4L2_DECODE] request output buffers failed\n");
        goto free_fd;
    }

    if (0 != v4l2_decode_map_output_buffers(handle)) {
        TIOVX_MODULE_ERROR("[V4L2_DECODE] mapping output buffers failed\n");
        goto free_fd;
    }

    handle->outbuf_head = 0;
    handle->outbuf_tail = handle->num_outbufs - 1;

    for (int i=0; i < handle->num_outbufs - 1; i++) {
        v4l2_decode_enqueue_outbuf(handle);
    }

    if (0 != v4l2_decode_start_output(handle)) {
        TIOVX_MODULE_ERROR("[V4L2_DECODE] starting output failed\n");
        goto free_fd;
    }

    if (0 != v4l2_decode_request_capture_buffers(handle)) {
        TIOVX_MODULE_ERROR("[V4L2_DECODE] request capture buffers failed\n");
        goto free_fd;
    }

    for (int i=0; i < handle->cfg.bufq_depth; i++) {
        handle->queued[i] = false;
    }

    out_fmt->width = ALIGN(handle->str.width, HW_ALIGN);
    out_fmt->height = ALIGN(handle->str.height, HW_ALIGN);

    return handle;

free_fd:
    close(handle->fd);
free_handle:
    free(handle);
    return NULL;
}

int v4l2_decode_start(v4l2DecodeHandle *handle)
{
    int status = 0;
    enum v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;

    if (-1 == xioctl(handle->fd, VIDIOC_STREAMON, &type)) {
        TIOVX_MODULE_ERROR("[V4L2_DECODE] capture VIDIOC_STREAMON failed\n");
        status = -1;
    }

    return status;
}

int v4l2_decode_enqueue_buf(v4l2DecodeHandle *handle, Buf *tiovx_buffer)
{
    struct v4l2_buffer buf;
    struct v4l2_plane buf_planes[3];
    int status = 0;
    uint32_t pitch[4];
    int fd[4];
    long unsigned int size[4];
    unsigned int offset[4];


    if (handle->queued[tiovx_buffer->buf_index] == true) {
        TIOVX_MODULE_ERROR("[V4L2_DECODE] Buffer alread enqueued\n");
        status = -1;
        goto ret;
    }

    getImageDmaFd(tiovx_buffer->handle, fd, pitch, size, offset, 2);

    CLR(&buf);
    buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
    buf.memory = V4L2_MEMORY_DMABUF;
    buf.index = tiovx_buffer->buf_index;
    buf.m.planes = buf_planes;
    buf.length = 1;

    CLR(buf_planes);
    buf_planes[0].m.fd = fd[0];
    buf_planes[0].length = size[0] + size[1];

    if (-1 == xioctl(handle->fd, VIDIOC_QBUF, &buf)) {
        TIOVX_MODULE_ERROR("[V4L2_DECODE] VIDIOC_QBUF failed\n");
        status = -1;
    } else {
        handle->queued[tiovx_buffer->buf_index] = true;
        handle->bufq[tiovx_buffer->buf_index] = tiovx_buffer;
    }

ret:
    return status;
}

Buf *v4l2_decode_dqueue_buf(v4l2DecodeHandle *handle)
{
    Buf *tiovx_buffer = NULL;
    struct v4l2_buffer buf;
    struct v4l2_plane buf_planes[3];
    struct pollfd pfd;
    int ret = 0;

poll:
    CLR(&pfd);
    pfd.fd = handle->fd;
    pfd.events = POLLIN | POLLOUT | POLLPRI;
    pfd.revents = 0;

    ret = poll(&pfd, 1, 100);
    if (ret < 0) {
        TIOVX_MODULE_ERROR("[V4L2_DECODE] POLL failed\n");
        goto ret;
    }

    if (pfd.revents & POLLPRI) {
        TIOVX_MODULE_ERROR("[V4L2_DECODE] Decoding finished\n");
        goto ret;
    }

    if (pfd.revents & POLLOUT) {
        v4l2_decode_dqueue_outbuf(handle);
        v4l2_decode_enqueue_outbuf(handle);
    }

    if ((pfd.revents & POLLIN) == 0) {
        goto poll;
    }

    CLR(&buf);
    buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
    buf.memory = V4L2_MEMORY_DMABUF;
    buf.m.planes = buf_planes;
    buf.length = 1;

    while (1) {
        if (-1 == xioctl(handle->fd, VIDIOC_DQBUF, &buf)) {
            if (errno == EAGAIN) {
                continue;
            } else {
                TIOVX_MODULE_ERROR("[V4L2_DECODE] VIDIOC_DQBUF failed\n");
                break;
            }
        } else {
            break;
        }
    }

    handle->queued[buf.index] = false;
    tiovx_buffer = handle->bufq[buf.index];

ret:
    return tiovx_buffer;
}

int v4l2_decode_stop(v4l2DecodeHandle *handle)
{
    int status = 0;
    enum v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;

    if (-1 == xioctl(handle->fd, VIDIOC_STREAMOFF, &type)) {
        TIOVX_MODULE_ERROR("[V4L2_DECODE] capture VIDIOC_STREAMOFF failed\n");
        status = -1;
    }

    type = V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE;

    if (-1 == xioctl(handle->fd, VIDIOC_STREAMOFF, &type)) {
        TIOVX_MODULE_ERROR("[V4L2_DECODE] output VIDIOC_STREAMOFF failed\n");
        status = -1;
    }

    return status;
}

int v4l2_decode_delete_handle(v4l2DecodeHandle *handle)
{
    int status = 0;

    fclose(handle->rdfd);
    close(handle->fd);
    free(handle);

    return status;
}
