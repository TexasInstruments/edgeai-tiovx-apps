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

#include "omx_decode_module.h"
#include "codec_input_demuxer.h"
#include "tiovx_utils.h"

#include <OMX_Core.h>
#include <OMX_Component.h>

#include "OMX_Extension_video_TI.h"
#include "OMX_Extension_index_TI.h"

#include <semaphore.h>

#define OMX_DECODE_DEFAULT_INPUT_FILE TIOVX_MODULES_DATA_PATH"/videos/video0_1280_768.h264"
#define OMX_DECODE_DEFAULT_BUFQ_DEPTH 4
#define OMX_DECODE_MAX_BUFQ_DEPTH 16
#define OMX_DECODE_MAX_BUFQ_DEPTH_OFFSET 3 //This is set in wave5 driver

#define MAX_INBUFS 2
#define ALIGN(x,a) (((x) + (a) - 1L) & ~((a) - 1L))
#define HW_ALIGN 64

#define OMX_SPEC_VERSION 0x00000001     // OMX Version
#define SET_OMX_VERSION_SIZE( param, size ) {             \
    param.nVersion.nVersion = OMX_SPEC_VERSION;           \
    param.nSize = size;                                   \
}

void omx_decode_init_cfg(omxDecodeCfg *cfg)
{
    cfg->bufq_depth = OMX_DECODE_DEFAULT_BUFQ_DEPTH;
    sprintf(cfg->file, OMX_DECODE_DEFAULT_INPUT_FILE);
}

struct _omxDecodeHandle {
    omxDecodeCfg cfg;
    int current_frame;
    streamContext str;
    uint32_t str_size;
    FILE *rdfd;
    uint32_t num_inbufs;
    uint32_t num_outbufs;
    uint64_t inbuf_size;
    uint64_t outbuf_size;
    int stream_start;
    OMX_BUFFERHEADERTYPE *omx_bufq[OMX_DECODE_MAX_BUFQ_DEPTH];
    bool queued[OMX_DECODE_MAX_BUFQ_DEPTH];
    int processed_queue[OMX_DECODE_MAX_BUFQ_DEPTH + 1];
    int pq_head;
    int pq_tail;
    OMX_BUFFERHEADERTYPE *in_omx_bufq[MAX_INBUFS];
    OMX_HANDLETYPE compHandle;
    OMX_U32 inPortIndex;
    OMX_U32 outPortIndex;
    sem_t cmd_sem;
    sem_t dq_sem;
};

OMX_ERRORTYPE omx_decode_event_handler(OMX_HANDLETYPE hComponent,
        OMX_PTR pAppData, OMX_EVENTTYPE eEvent, OMX_U32 nData1,
        OMX_U32 nData2, OMX_PTR pEventData )
{
    omxDecodeHandle *handle = (omxDecodeHandle *)pAppData;
    OMX_ERRORTYPE omxErr = OMX_ErrorNone;

    if (handle->compHandle == NULL) {
        TIOVX_MODULE_ERROR("[OMX_DECODE] EventHandler: compHandle is NULL\n" );
        return OMX_ErrorUndefined;
    }

    switch (eEvent) {
        case OMX_EventError:
        {
            if(hComponent == handle->compHandle) {
                if (OMX_ErrorStreamCorrupt == (OMX_ERRORTYPE)nData1) {
                    TIOVX_MODULE_ERROR("[OMX_DECODE] corrupted stream detected; continuing...\n");
                } else {
                    TIOVX_MODULE_ERROR("[OMX_DECODE] err=0x%x\n", nData1);
                }
            }
            break;
        }

        case OMX_EventCmdComplete:
        {
            switch ((OMX_COMMANDTYPE) nData1) {
                case OMX_CommandStateSet:
                {
                    if( hComponent == handle->compHandle ) {
                        sem_post(&handle->cmd_sem);
                    }

                    break;
                }

                case OMX_CommandFlush:
                case OMX_CommandPortDisable:
                case OMX_CommandPortEnable:
                default:
                    break;
            }

            break;
        }

        case OMX_EventBufferFlag:
        case OMX_EventPortSettingsChanged:
        default:
            break;
    }

    return omxErr;
}

OMX_ERRORTYPE omx_decode_empty_buffer_done(OMX_HANDLETYPE hComponent,
        OMX_PTR pAppData, OMX_BUFFERHEADERTYPE *pBufHdr)
{
    OMX_ERRORTYPE omxErr = OMX_ErrorNone;
    omxDecodeHandle *handle = (omxDecodeHandle *)pAppData;

    TIOVX_MODULE_PRINTF("[OMX_DECODE] Empty Buffer Done\n");

    if (handle->compHandle != hComponent ) {
        TIOVX_MODULE_ERROR("[OMX_DECODE] EmptyBufferDone Unknown Component %p\n", hComponent );
        return omxErr;
    }

    return omxErr;
}

OMX_ERRORTYPE omx_decode_fill_buffer_done(OMX_HANDLETYPE hComponent,
        OMX_PTR pAppData, OMX_BUFFERHEADERTYPE *pBufHdr)
{
    OMX_ERRORTYPE omxErr = OMX_ErrorNone;
    omxDecodeHandle *handle = (omxDecodeHandle *)pAppData;
    Buf *tiovx_buffer = (Buf *)(pBufHdr->pAppPrivate);
    void *addr;
    uint64_t size;


    TIOVX_MODULE_PRINTF("[OMX_DECODE] Fill Buffer Done\n");

    if (handle->compHandle != hComponent ) {
        TIOVX_MODULE_ERROR("[OMX_DECODE] FillBufferDone Unknown Component %p\n", hComponent );
        return omxErr;
    }

    getReferenceAddr(tiovx_buffer->handle, &addr, &size);
    memcpy(addr, pBufHdr->pBuffer, size);

    handle->processed_queue[handle->pq_head] = tiovx_buffer->buf_index;
    handle->pq_head = (handle->pq_head + 1) % (handle->num_outbufs + 1);

    sem_post(&handle->dq_sem);

    return omxErr;
}

int omx_allocate_output_buffer(omxDecodeHandle *handle, Buf *tiovx_buffer)
{
    OMX_ERRORTYPE omxErr = OMX_ErrorNone;

    omxErr = OMX_AllocateBuffer(handle->compHandle,
                                &handle->omx_bufq[tiovx_buffer->buf_index],
                                handle->outPortIndex, tiovx_buffer,
                                handle->outbuf_size);
    if (omxErr != OMX_ErrorNone) {
        TIOVX_MODULE_ERROR("[OMX_DECODE] Port OMX_UseBuffer() returned 0x%08x\n", omxErr);
        return (int)omxErr;
    }

    return 0;
}

int omx_allocate_input_buffers(omxDecodeHandle *handle)
{
    OMX_ERRORTYPE omxErr = OMX_ErrorNone;

    for (int i = 0; i < handle->num_inbufs; i++) {
        omxErr = OMX_AllocateBuffer(handle->compHandle, &handle->in_omx_bufq[i],
                                    handle->inPortIndex, NULL,
                                    handle->inbuf_size);
        if (omxErr != OMX_ErrorNone) {
            TIOVX_MODULE_ERROR("[OMX_DECODE] Port OMX_AllocateBuffer() returned 0x%08x\n", omxErr);
            return (int)omxErr;
        }
    }

    return 0;
}

OMX_CALLBACKTYPE callbacks = {&omx_decode_event_handler,
                              &omx_decode_empty_buffer_done,
                              &omx_decode_fill_buffer_done};

int omx_decode_init(omxDecodeHandle *handle)
{
    OMX_ERRORTYPE omxErr;
    OMX_PORT_PARAM_TYPE portParam;
    OMX_VENDOR_TIVPU_PARAM_TYPE vpuParam = {0};
    OMX_PARAM_PORTDEFINITIONTYPE inPortParam;
    OMX_PARAM_PORTDEFINITIONTYPE outPortParam;

    omxErr = OMX_Init();
    if (omxErr != OMX_ErrorNone) {
        TIOVX_MODULE_ERROR("[OMX_DECODE] Component OMX_Init() returned 0x%08x\n", omxErr);
        return (int)omxErr;
    }

    OMX_U32 num_comp = 1;
    OMX_U8 comp_name[128];
    OMX_U8 *pcomp_name[1]={comp_name};
    char role[128];

    if (handle->str.codec == AV_CODEC_ID_H264) {
        sprintf(role, "video_decoder.avc");
    } else if(handle->str.codec == AV_CODEC_ID_HEVC) {
        sprintf(role, "video_decoder.hevc");
    }

    omxErr = OMX_GetComponentsOfRole (role, &num_comp, pcomp_name);
    if (omxErr != OMX_ErrorNone) {
        TIOVX_MODULE_ERROR("[OMX_DECODE ] OMX_GetComponentsOfRole() returned 0x%08x\n", omxErr );
        return (int)omxErr;
    }

    omxErr = OMX_GetHandle(&handle->compHandle, (OMX_STRING)comp_name,
                            (OMX_PTR)handle, &callbacks );
    if ((handle->compHandle == NULL) || (omxErr != OMX_ErrorNone)) {
        TIOVX_MODULE_ERROR("[OMX_DECODE] Component(%s) OMX_GetHandle() returned 0x%08x\n", comp_name, omxErr );
        return (int)omxErr;
    }

    vpuParam.dec_buf_num = handle->cfg.bufq_depth;

    omxErr = OMX_SetParameter(handle->compHandle,
                              (OMX_INDEXTYPE)OMX_VendorTIVPUDecBufCount,
                              &vpuParam);
    if (omxErr != OMX_ErrorNone) {
        TIOVX_MODULE_ERROR("[OMX_DECODE] Component OMX_SetParameter() returned 0x%08x\n", omxErr);
        return (int)omxErr;
    }

    SET_OMX_VERSION_SIZE(portParam, sizeof(OMX_PORT_PARAM_TYPE));
    portParam.nPorts = 0;
    omxErr = OMX_GetParameter(handle->compHandle,
                              (OMX_INDEXTYPE)(OMX_IndexParamVideoInit),
                              &portParam);
    if (omxErr != OMX_ErrorNone) {
        TIOVX_MODULE_ERROR("[OMX_DECODE] Component OMX_GetParameter() returned 0x%08x version %u\n",
                  omxErr, portParam.nVersion.nVersion );
        return (int)omxErr;
    }

    if (portParam.nPorts < 2) {
        TIOVX_MODULE_ERROR("[OMX_DECODE] Invalid number of ports %d\n", portParam.nPorts);
        return (int)omxErr;
    }

    handle->inPortIndex = portParam.nStartPortNumber;
    handle->outPortIndex = portParam.nStartPortNumber + 1;

    SET_OMX_VERSION_SIZE(inPortParam, sizeof(OMX_PARAM_PORTDEFINITIONTYPE));
    inPortParam.nPortIndex = handle->inPortIndex;
    omxErr = OMX_GetParameter(handle->compHandle, OMX_IndexParamPortDefinition,
                              &inPortParam );
    if( omxErr != OMX_ErrorNone ) {
        TIOVX_MODULE_ERROR("[OMX_DECODE] Port OMX_GetParameter() returned 0x%08x\n", omxErr );
        return (int)omxErr;
    }

    inPortParam.format.video.nFrameWidth   = handle->str.width;
    inPortParam.format.video.nFrameHeight  = handle->str.height;
    if (handle->str.codec == AV_CODEC_ID_H264) {
        inPortParam.format.video.eCompressionFormat = OMX_VIDEO_CodingAVC;
    } else if(handle->str.codec == AV_CODEC_ID_HEVC) {
        inPortParam.format.video.eCompressionFormat = OMXQ_VIDEO_CodingHEVC;
    }
    inPortParam.nBufferCountActual = MAX_INBUFS;

    omxErr = OMX_SetParameter(handle->compHandle, OMX_IndexParamPortDefinition,
                              &inPortParam );
    if( omxErr != OMX_ErrorNone ) {
        TIOVX_MODULE_ERROR("[OMX_DECODE] Port OMX_SetParameter() returned 0x%08x\n", omxErr );
        return (int)omxErr;
    }

    omxErr = OMX_GetParameter(handle->compHandle, OMX_IndexParamPortDefinition,
                              &inPortParam );
    if( omxErr != OMX_ErrorNone ) {
        TIOVX_MODULE_ERROR("[OMX_DECODE] Port OMX_GetParameter() returned 0x%08x\n", omxErr );
        return (int)omxErr;
    }

    handle->num_inbufs = inPortParam.nBufferCountActual;
    handle->inbuf_size = inPortParam.nBufferSize;

    SET_OMX_VERSION_SIZE(outPortParam, sizeof(OMX_PARAM_PORTDEFINITIONTYPE));
    outPortParam.nPortIndex = handle->outPortIndex;
    omxErr = OMX_GetParameter(handle->compHandle, OMX_IndexParamPortDefinition,
                              &outPortParam );
    if( omxErr != OMX_ErrorNone ) {
        TIOVX_MODULE_ERROR("[OMX_DECODE] Port OMX_GetParameter() returned 0x%08x\n", omxErr );
        return (int)omxErr;
    }

    outPortParam.format.video.nFrameWidth   = ALIGN(handle->str.width, HW_ALIGN);
    outPortParam.format.video.nFrameHeight  = ALIGN(handle->str.height, HW_ALIGN);
    outPortParam.format.video.nStride = ALIGN(handle->str.width, HW_ALIGN);
    outPortParam.format.video.nSliceHeight = ALIGN(handle->str.height, HW_ALIGN);
    outPortParam.format.video.eColorFormat = OMX_COLOR_FormatYUV420SemiPlanar;
    outPortParam.nBufferCountActual = handle->cfg.bufq_depth;

    omxErr = OMX_SetParameter(handle->compHandle, OMX_IndexParamPortDefinition,
                              &outPortParam );
    if( omxErr != OMX_ErrorNone ) {
        TIOVX_MODULE_ERROR("[OMX_DECODE] Port OMX_SetParameter() returned 0x%08x\n", omxErr );
        return (int)omxErr;
    }

    omxErr = OMX_GetParameter(handle->compHandle, OMX_IndexParamPortDefinition,
                              &outPortParam );
    if( omxErr != OMX_ErrorNone ) {
        TIOVX_MODULE_ERROR("[OMX_DECODE] Port OMX_GetParameter() returned 0x%08x\n", omxErr );
        return (int)omxErr;
    }

    handle->num_outbufs = outPortParam.nBufferCountActual;
    handle->outbuf_size = outPortParam.nBufferSize;

    return omxErr;
}

omxDecodeHandle *omx_decode_create_handle(omxDecodeCfg *cfg,
                                            omxDecodeOutFmt *out_fmt)
{
    omxDecodeHandle *handle = NULL;
    int status = 0;

    handle = calloc(1, sizeof(omxDecodeHandle));
    memcpy(&handle->cfg, cfg, sizeof(omxDecodeCfg));
    handle->current_frame = 0;

    if (stream_framelevel_parsing(&handle->str, cfg->file, MAX_FRAMES) < 0) {
        TIOVX_MODULE_ERROR("[OMX_DECODE] Stream parsing failed\n");
        goto free_handle;
    }

    handle->rdfd = fopen(cfg->file, "r");
    if (handle->rdfd  == NULL) {
        TIOVX_MODULE_ERROR("[OMX_DECODE] Failed to open input file %s\n",
                            cfg->file);
        goto free_handle;
    }

    fseek(handle->rdfd, 0L, SEEK_END);
    handle->str_size = ftell(handle->rdfd);
    rewind(handle->rdfd);

    handle->num_inbufs = MAX_INBUFS;
    handle->num_outbufs = handle->cfg.bufq_depth;

    for (int i=0; i < handle->num_outbufs; i++) {
        handle->queued[i] = false;
    }

    handle->pq_head = 0;
    handle->pq_tail = handle->num_outbufs;

    out_fmt->width = ALIGN(handle->str.width, HW_ALIGN);
    out_fmt->height = ALIGN(handle->str.height, HW_ALIGN);

    status = omx_decode_init(handle);
    if (status) {
        TIOVX_MODULE_ERROR("[OMX_DECODE] Failed to init OMX\n");
        goto free_handle;
    }

    sem_init(&handle->cmd_sem, 0, 0);
    sem_init(&handle->dq_sem, 0, 0);

    omx_allocate_input_buffers(handle);

    return handle;

free_handle:
    free(handle);
    return NULL;
}

int omx_decode_move_to_state(omxDecodeHandle *handle,
        OMX_STATETYPE newState)
{
    OMX_ERRORTYPE omxErr;
    OMX_STATETYPE currState;

    switch(newState) {
        case OMX_StateLoaded:
        case OMX_StateIdle:
        case OMX_StatePause:
        case OMX_StateExecuting:
            break;
        default:
            return OMX_ErrorBadParameter;
    }

    omxErr = OMX_GetState(handle->compHandle, &currState);
    if (omxErr != OMX_ErrorNone)
    {
        TIOVX_MODULE_ERROR("[OMX_DECODE] Failed to get current state!!!\n" );
        return (int)omxErr;
    }

    if (currState == OMX_StateInvalid) {
        TIOVX_MODULE_ERROR("[OMX_DECODE] Transition from Invalid state is not allowed!!!\n" );
        return (int)OMX_ErrorInvalidState;
    }

    if(currState == newState) {
        return (int)OMX_ErrorNone;
    }

    omxErr = OMX_SendCommand(handle->compHandle, OMX_CommandStateSet, newState,
                             NULL);
    if (omxErr != OMX_ErrorNone) {
        TIOVX_MODULE_ERROR("[OMX_DECODE] StateTransition(IDLE) returned 0x%08x\n", omxErr );
        return (int)omxErr;
    }

    sem_wait(&handle->cmd_sem);

    return 0;
}

int omx_decode_start(omxDecodeHandle *handle)
{
    int status = 0;
    OMX_ERRORTYPE omxErr = OMX_ErrorNone;

    status = omx_decode_move_to_state(handle, OMX_StateIdle);
    if (status) {
        TIOVX_MODULE_ERROR("[OMX_DECODE] Transition LOADED->IDLE failed 0x%x\n", status);
        return status;
    }

    status = omx_decode_move_to_state(handle, OMX_StateExecuting);
    if (status) {
        TIOVX_MODULE_ERROR("[OMX_DECODE] Transition IDLE->EXECUTING failed 0x%x\n", status);
        return status;
    }

    handle->in_omx_bufq[0]->nFilledLen =
        fread(handle->in_omx_bufq[0]->pBuffer, 1, handle->str_size,
              handle->rdfd);
    handle->in_omx_bufq[1]->nFlags |= OMX_BUFFERFLAG_EOS | OMX_BUFFERFLAG_ENDOFFRAME;
    omxErr = OMX_EmptyThisBuffer(handle->compHandle, handle->in_omx_bufq[0]);
    if (omxErr != OMX_ErrorNone) {
        TIOVX_MODULE_ERROR("[OMX_DECODE] OMX_EmptyThisBuffer: return omxErr=%08x\n", omxErr);
        return (int)omxErr;
    }

    for (int i=0; i < handle->num_outbufs; i++) {
        if (handle->queued[i]) {
            omxErr = OMX_FillThisBuffer(handle->compHandle, handle->omx_bufq[i]);
            if (omxErr != OMX_ErrorNone) {
                TIOVX_MODULE_ERROR("[OMX_DECODE] OMX_FillThisBuffer: return omxErr=%08x\n", omxErr);
                return (int)omxErr;
            }
        }
    }

    handle->stream_start = 1;

    return status;
}

int omx_decode_enqueue_buf(omxDecodeHandle *handle, Buf *tiovx_buffer)
{
    int status = 0;
    OMX_ERRORTYPE omxErr = OMX_ErrorNone;

    if (handle->stream_start == 0) {
        handle->queued[tiovx_buffer->buf_index] = true;
        status = omx_allocate_output_buffer(handle, tiovx_buffer);
        return status;
    } else {
        handle->queued[tiovx_buffer->buf_index] = true;
        omxErr = OMX_FillThisBuffer(handle->compHandle,
                                    handle->omx_bufq[tiovx_buffer->buf_index]);
        if (omxErr != OMX_ErrorNone) {
            TIOVX_MODULE_ERROR("[OMX_DECODE] OMX_FillThisBuffer() returned 0x%08x\n", omxErr);
            return (int)omxErr;
        }
    }

    return status;
}

Buf *omx_decode_dqueue_buf(omxDecodeHandle *handle)
{
    Buf *tiovx_buffer = NULL;
    int index;

    sem_wait(&handle->dq_sem);

    if ((handle->pq_tail + 1) % (handle->num_outbufs + 1) == handle->pq_head) {
        TIOVX_MODULE_ERROR("[OMX_DECODE] Queue Empty\n");
        return NULL;
    } else {
        handle->pq_tail = (handle->pq_tail + 1) %
                                            (handle->num_outbufs + 1);
    }

    index = handle->processed_queue[handle->pq_tail];

    handle->queued[index] = false;
    tiovx_buffer = (Buf *)(handle->omx_bufq[index]->pAppPrivate);

    return tiovx_buffer;
}

int omx_decode_stop(omxDecodeHandle *handle)
{
    int status = 0;

    return status;
}

int omx_decode_delete_handle(omxDecodeHandle *handle)
{
    int status = 0;

    fclose(handle->rdfd);
    free(handle);

    return status;
}
