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

#include "codec_input_demuxer.h"

#include <libavcodec/avcodec.h>
#include <libavcodec/bsf.h>
#include <libavutil/pixdesc.h>

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

static AVFormatContext *open_stream_file(const char *filename)
{
    AVFormatContext *afc = NULL;
    int err = avformat_open_input(&afc, filename, NULL, NULL);

    if (!err)
        err = avformat_find_stream_info(afc, NULL);

    if (err < 0) {
        TIOVX_MODULE_ERROR("[CODEC_DEMUX] %s: lavf error %d\n", filename, err);
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
        TIOVX_MODULE_ERROR("[CODEC_DEMUX] could not open '%s': "
                "unsupported codec %d", filename, cc->codec_id);
        return NULL;
    }

    if (cc->extradata && cc->extradata_size > 0 && cc->extradata[0] == 1) {
        TIOVX_MODULE_ERROR("[CODEC_DEMUX] initializing bitstream filter");
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
            TIOVX_MODULE_ERROR("[CODEC_DEMUX] could not open '%s': failed "
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
                    TIOVX_MODULE_ERROR("[CODEC_DEMUX] bsf_send_packet error: %d",
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
        TIOVX_MODULE_ERROR("[CODEC_DEMUX] could not open demuxer\n");
        return -1;
    }

    inp_buf_len = (inp_width * inp_height);

    inp_buf = malloc(inp_buf_len);
    if(inp_buf == NULL)
    {
        TIOVX_MODULE_ERROR("[CODEC_DEMUX] Memory allocation failed "
                "for inp_buf\n");
        return -1;
    }

    TIOVX_MODULE_PRINTF("[CODEC_DEMUX] demuxer is initialized, "
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
