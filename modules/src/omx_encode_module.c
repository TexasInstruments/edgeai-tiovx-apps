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

#include "omx_encode_module.h"
#include "tiovx_utils.h"

#include <OMX_Core.h>
#include <OMX_Component.h>

#include "OMX_Extension_video_TI.h"
#include "OMX_Extension_index_TI.h"

#include <semaphore.h>

#define OMX_ENCODE_DEFAULT_WIDTH 1920
#define OMX_ENCODE_DEFAULT_HEIGHT 1080
#define OMX_ENCODE_DEFAULT_COLOR_FORMAT VX_DF_IMAGE_NV12
#define OMX_ENCODE_DEFAULT_ENCODING AV_CODEC_ID_H264
#define OMX_ENCODE_DEFAULT_OUTPUT_FILE TIOVX_MODULES_DATA_PATH"/output/qnx_encode.h264"
#define OMX_ENCODE_DEFAULT_BUFQ_DEPTH 4
#define OMX_ENCODE_MAX_BUFQ_DEPTH 16

#define MAX_OUTBUFS 4
#define ALIGN(x,a) (((x) + (a) - 1L) & ~((a) - 1L))
#define HW_ALIGN 64

#define OMX_SPEC_VERSION 0x00000001     // OMX Version
#define SET_OMX_VERSION_SIZE( param, size ) {             \
    param.nVersion.nVersion = OMX_SPEC_VERSION;           \
    param.nSize = size;                                   \
}

void omx_encode_init_cfg(omxEncodeCfg *cfg)
{
    CLR(cfg);
    cfg->width = OMX_ENCODE_DEFAULT_WIDTH;
    cfg->height = OMX_ENCODE_DEFAULT_HEIGHT;
    cfg->color_format = OMX_ENCODE_DEFAULT_COLOR_FORMAT;
    cfg->bufq_depth = OMX_ENCODE_DEFAULT_BUFQ_DEPTH;
    cfg->encoding = OMX_ENCODE_DEFAULT_ENCODING;
    sprintf(cfg->file, OMX_ENCODE_DEFAULT_OUTPUT_FILE);
}

struct _omxEncodeHandle {
    omxEncodeCfg cfg;
    FILE *wrfd;
    uint32_t num_inbufs;
    uint32_t num_outbufs;
    uint64_t inbuf_size;
    uint64_t outbuf_size;
    int stream_start;
    OMX_BUFFERHEADERTYPE *omx_bufq[OMX_ENCODE_MAX_BUFQ_DEPTH];
    bool queued[OMX_ENCODE_MAX_BUFQ_DEPTH];
    int processed_queue[OMX_ENCODE_MAX_BUFQ_DEPTH + 1];
    int pq_head;
    int pq_tail;
    OMX_BUFFERHEADERTYPE *out_omx_bufq[MAX_OUTBUFS];
    uint64_t out_processed_queue[MAX_OUTBUFS + 1];
    int out_pq_head;
    int out_pq_tail;
    OMX_HANDLETYPE compHandle;
    OMX_U32 inPortIndex;
    OMX_U32 outPortIndex;
    sem_t cmd_sem;
    sem_t dq_sem;
};

OMX_ERRORTYPE omx_encode_event_handler(OMX_HANDLETYPE hComponent,
        OMX_PTR pAppData, OMX_EVENTTYPE eEvent, OMX_U32 nData1,
        OMX_U32 nData2, OMX_PTR pEventData )
{
    omxEncodeHandle *handle = (omxEncodeHandle *)pAppData;
    OMX_ERRORTYPE omxErr = OMX_ErrorNone;

    if (handle->compHandle == NULL) {
        TIOVX_MODULE_ERROR("[OMX_ENCODE] EventHandler: compHandle is NULL\n" );
        return OMX_ErrorUndefined;
    }

    switch (eEvent) {
        case OMX_EventError:
        {
            if(hComponent == handle->compHandle) {
                if (OMX_ErrorStreamCorrupt == (OMX_ERRORTYPE)nData1) {
                    TIOVX_MODULE_ERROR("[OMX_ENCODE] corrupted stream detected; continuing...\n");
                } else {
                    TIOVX_MODULE_ERROR("[OMX_ENCODE] err=0x%x\n", nData1);
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

OMX_ERRORTYPE omx_encode_fill_buffer_done(OMX_HANDLETYPE hComponent,
        OMX_PTR pAppData, OMX_BUFFERHEADERTYPE *pBufHdr)
{
    OMX_ERRORTYPE omxErr = OMX_ErrorNone;
    omxEncodeHandle *handle = (omxEncodeHandle *)pAppData;

    TIOVX_MODULE_PRINTF("[OMX_ENCODE] Empty Buffer Done\n");

    if (handle->compHandle != hComponent ) {
        TIOVX_MODULE_ERROR("[OMX_ENCODE] EmptyBufferDone Unknown Component %p\n", hComponent );
        return omxErr;
    }

    fwrite(pBufHdr->pBuffer, pBufHdr->nFilledLen, 1, handle->wrfd);

    handle->out_processed_queue[handle->out_pq_head] =
                                                    (uint64_t)(pBufHdr->pAppPrivate);
    handle->out_pq_head = (handle->out_pq_head + 1) % (handle->num_outbufs + 1);

    return omxErr;
}

OMX_ERRORTYPE omx_encode_empty_buffer_done(OMX_HANDLETYPE hComponent,
        OMX_PTR pAppData, OMX_BUFFERHEADERTYPE *pBufHdr)
{
    OMX_ERRORTYPE omxErr = OMX_ErrorNone;
    omxEncodeHandle *handle = (omxEncodeHandle *)pAppData;
    Buf *tiovx_buffer = (Buf *)(pBufHdr->pAppPrivate);

    TIOVX_MODULE_PRINTF("[OMX_ENCODE] Fill Buffer Done\n");

    if (handle->compHandle != hComponent ) {
        TIOVX_MODULE_ERROR("[OMX_ENCODE] FillBufferDone Unknown Component %p\n", hComponent );
        return omxErr;
    }

    handle->processed_queue[handle->pq_head] = tiovx_buffer->buf_index;
    handle->pq_head = (handle->pq_head + 1) % (handle->num_inbufs + 1);

    sem_post(&handle->dq_sem);

    return omxErr;
}

int omx_allocate_input_buffer(omxEncodeHandle *handle, Buf *tiovx_buffer)
{
    OMX_ERRORTYPE omxErr = OMX_ErrorNone;
    void *addr;
    uint64_t size;

    getReferenceAddr(tiovx_buffer->handle, &addr, &size);

    omxErr = OMX_UseBuffer(handle->compHandle,
                           &handle->omx_bufq[tiovx_buffer->buf_index],
                           handle->inPortIndex,
                           (OMX_PTR)(tiovx_buffer),
                           size, addr);
    if (omxErr != OMX_ErrorNone) {
        TIOVX_MODULE_ERROR("[OMX_DECODE] Port OMX_UseBuffer() returned 0x%08x\n", omxErr);
        return (int)omxErr;
    }

    return 0;
}

int omx_allocate_output_buffers(omxEncodeHandle *handle)
{
    OMX_ERRORTYPE omxErr = OMX_ErrorNone;

    for (uint64_t i = 0; i < handle->num_outbufs; i++) {
        omxErr = OMX_AllocateBuffer(handle->compHandle, &handle->out_omx_bufq[i],
                                    handle->outPortIndex, (OMX_PTR)i,
                                    handle->outbuf_size);
        if (omxErr != OMX_ErrorNone) {
            TIOVX_MODULE_ERROR("[OMX_ENCODE] Port OMX_AllocateBuffer() returned 0x%08x\n", omxErr);
            return (int)omxErr;
        }
    }

    return 0;
}

static OMX_CALLBACKTYPE callbacks = {&omx_encode_event_handler,
                                     &omx_encode_empty_buffer_done,
                                     &omx_encode_fill_buffer_done};

int omx_encode_init(omxEncodeHandle *handle)
{
    OMX_ERRORTYPE omxErr;
    OMX_PORT_PARAM_TYPE portParam;
    OMX_PARAM_PORTDEFINITIONTYPE inPortParam;
    OMX_PARAM_PORTDEFINITIONTYPE outPortParam;

    omxErr = OMX_Init();
    if (omxErr != OMX_ErrorNone) {
        TIOVX_MODULE_ERROR("[OMX_ENCODE] Component OMX_Init() returned 0x%08x\n", omxErr);
        return (int)omxErr;
    }

    OMX_STRING comp_name = "OMX.qnx.video.encoder";

    omxErr = OMX_GetHandle(&handle->compHandle, (OMX_STRING)comp_name,
                            (OMX_PTR)handle, &callbacks );
    if ((handle->compHandle == NULL) || (omxErr != OMX_ErrorNone)) {
        TIOVX_MODULE_ERROR("[OMX_ENCODE] Component(%s) OMX_GetHandle() returned 0x%08x\n", comp_name, omxErr );
        return (int)omxErr;
    }

    SET_OMX_VERSION_SIZE(portParam, sizeof(OMX_PORT_PARAM_TYPE));
    portParam.nPorts = 0;
    omxErr = OMX_GetParameter(handle->compHandle,
                              (OMX_INDEXTYPE)(OMX_IndexParamVideoInit),
                              &portParam);
    if (omxErr != OMX_ErrorNone) {
        TIOVX_MODULE_ERROR("[OMX_ENCODE] Component OMX_GetParameter() returned 0x%08x version %u\n",
                  omxErr, portParam.nVersion.nVersion );
        return (int)omxErr;
    }

    if (portParam.nPorts < 2) {
        TIOVX_MODULE_ERROR("[OMX_ENCODE] Invalid number of ports %d\n", portParam.nPorts);
        return (int)omxErr;
    }

    handle->inPortIndex = portParam.nStartPortNumber;
    handle->outPortIndex = portParam.nStartPortNumber + 1;

    SET_OMX_VERSION_SIZE(inPortParam, sizeof(OMX_PARAM_PORTDEFINITIONTYPE));
    inPortParam.nPortIndex = handle->inPortIndex;
    omxErr = OMX_GetParameter(handle->compHandle, OMX_IndexParamPortDefinition,
                              &inPortParam );
    if( omxErr != OMX_ErrorNone ) {
        TIOVX_MODULE_ERROR("[OMX_ENCODE] Port OMX_GetParameter() returned 0x%08x\n", omxErr );
        return (int)omxErr;
    }

    inPortParam.format.video.nFrameWidth   = handle->cfg.width;
    inPortParam.format.video.nFrameHeight  = handle->cfg.height;
    inPortParam.format.video.nStride = ALIGN(handle->cfg.width, HW_ALIGN);
    inPortParam.format.video.eColorFormat  = OMX_COLOR_FormatYUV420SemiPlanar;
    inPortParam.nBufferCountActual = handle->cfg.bufq_depth;

    omxErr = OMX_SetParameter(handle->compHandle, OMX_IndexParamPortDefinition,
                              &inPortParam );
    if( omxErr != OMX_ErrorNone ) {
        TIOVX_MODULE_ERROR("[OMX_ENCODE] Port OMX_SetParameter() returned 0x%08x\n", omxErr );
        return (int)omxErr;
    }

    omxErr = OMX_GetParameter(handle->compHandle, OMX_IndexParamPortDefinition,
                              &inPortParam );
    if( omxErr != OMX_ErrorNone ) {
        TIOVX_MODULE_ERROR("[OMX_ENCODE] Port OMX_GetParameter() returned 0x%08x\n", omxErr );
        return (int)omxErr;
    }

    handle->num_inbufs = inPortParam.nBufferCountActual;
    handle->inbuf_size = inPortParam.nBufferSize;

    SET_OMX_VERSION_SIZE(outPortParam, sizeof(OMX_PARAM_PORTDEFINITIONTYPE));
    outPortParam.nPortIndex = handle->outPortIndex;
    omxErr = OMX_GetParameter(handle->compHandle, OMX_IndexParamPortDefinition,
                              &outPortParam );
    if( omxErr != OMX_ErrorNone ) {
        TIOVX_MODULE_ERROR("[OMX_ENCODE] Port OMX_GetParameter() returned 0x%08x\n", omxErr );
        return (int)omxErr;
    }

    if (handle->cfg.encoding == AV_CODEC_ID_H264) {
        outPortParam.format.video.eCompressionFormat = OMX_VIDEO_CodingAVC;
    } else if(handle->cfg.encoding == AV_CODEC_ID_HEVC) {
        outPortParam.format.video.eCompressionFormat = OMXQ_VIDEO_CodingHEVC;
    }
    outPortParam.format.video.nFrameWidth   = ALIGN(handle->cfg.width, HW_ALIGN);
    outPortParam.format.video.nFrameHeight  = ALIGN(handle->cfg.height, HW_ALIGN);
    outPortParam.nBufferCountActual = handle->num_outbufs;

    omxErr = OMX_SetParameter(handle->compHandle, OMX_IndexParamPortDefinition,
                              &outPortParam );
    if( omxErr != OMX_ErrorNone ) {
        TIOVX_MODULE_ERROR("[OMX_ENCODE] Port OMX_SetParameter() returned 0x%08x\n", omxErr );
        return (int)omxErr;
    }

    omxErr = OMX_GetParameter(handle->compHandle, OMX_IndexParamPortDefinition,
                              &outPortParam );
    if( omxErr != OMX_ErrorNone ) {
        TIOVX_MODULE_ERROR("[OMX_ENCODE] Port OMX_GetParameter() returned 0x%08x\n", omxErr );
        return (int)omxErr;
    }

    handle->num_outbufs = outPortParam.nBufferCountActual;
    handle->out_pq_tail = handle->num_outbufs;
    handle->outbuf_size = outPortParam.nBufferSize;

    return omxErr;
}

omxEncodeHandle *omx_encode_create_handle(omxEncodeCfg *cfg)
{
    omxEncodeHandle *handle = NULL;
    int status = 0;

    handle = calloc(1, sizeof(omxEncodeHandle));
    memcpy(&handle->cfg, cfg, sizeof(omxEncodeCfg));

    handle->wrfd = fopen(cfg->file, "w");
    if (handle->wrfd  == NULL) {
        TIOVX_MODULE_ERROR("[OMX_ENCODE] Failed to open output file %s\n",
                            cfg->file);
        goto free_handle;
    }

    handle->num_inbufs = handle->cfg.bufq_depth;
    handle->num_outbufs = MAX_OUTBUFS;

    for (int i=0; i < handle->num_inbufs; i++) {
        handle->queued[i] = false;
    }

    handle->pq_head = 0;
    handle->pq_tail = handle->num_inbufs;

    handle->out_pq_head = 0;
    handle->out_pq_tail = handle->num_outbufs;

    status = omx_encode_init(handle);
    if (status) {
        TIOVX_MODULE_ERROR("[OMX_ENCODE] Failed to init OMX\n");
        goto free_handle;
    }

    sem_init(&handle->cmd_sem, 0, 0);
    sem_init(&handle->dq_sem, 0, 0);

    omx_allocate_output_buffers(handle);

    return handle;

free_handle:
    free(handle);
    return NULL;
}

int omx_encode_move_to_state(omxEncodeHandle *handle,
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
        TIOVX_MODULE_ERROR("[OMX_ENCODE] Failed to get current state!!!\n" );
        return (int)omxErr;
    }

    if (currState == OMX_StateInvalid) {
        TIOVX_MODULE_ERROR("[OMX_ENCODE] Transition from Invalid state is not allowed!!!\n" );
        return (int)OMX_ErrorInvalidState;
    }

    if(currState == newState) {
        return (int)OMX_ErrorNone;
    }

    omxErr = OMX_SendCommand(handle->compHandle, OMX_CommandStateSet, newState,
                             NULL);
    if (omxErr != OMX_ErrorNone) {
        TIOVX_MODULE_ERROR("[OMX_ENCODE] StateTransition(IDLE) returned 0x%08x\n", omxErr );
        return (int)omxErr;
    }

    sem_wait(&handle->cmd_sem);

    return 0;
}

int omx_encode_start(omxEncodeHandle *handle)
{
    int status = 0;
    OMX_ERRORTYPE omxErr = OMX_ErrorNone;

    status = omx_encode_move_to_state(handle, OMX_StateIdle);
    if (status) {
        TIOVX_MODULE_ERROR("[OMX_ENCODE] Transition LOADED->IDLE failed 0x%x\n", status);
        return status;
    }

    status = omx_encode_move_to_state(handle, OMX_StateExecuting);
    if (status) {
        TIOVX_MODULE_ERROR("[OMX_ENCODE] Transition IDLE->EXECUTING failed 0x%x\n", status);
        return status;
    }

    for (int i=0; i < handle->num_outbufs; i++) {
        omxErr = OMX_FillThisBuffer(handle->compHandle, handle->out_omx_bufq[i]);
        if (omxErr != OMX_ErrorNone) {
            TIOVX_MODULE_ERROR("[OMX_ENCODE] OMX_FillThisBuffer: return omxErr=%08x\n", omxErr);
            return (int)omxErr;
        }
    }

    for (int i=0; i < handle->num_inbufs; i++) {
        if (handle->queued[i]) {
            handle->omx_bufq[i]->nFilledLen = handle->omx_bufq[i]->nAllocLen;
            handle->omx_bufq[i]->nFlags |= OMX_BUFFERFLAG_ENDOFFRAME;
            omxErr = OMX_EmptyThisBuffer(handle->compHandle, handle->omx_bufq[i]);
            if (omxErr != OMX_ErrorNone) {
                TIOVX_MODULE_ERROR("[OMX_ENCODE] OMX_EmptyThisBuffer: return omxErr=%08x\n", omxErr);
                return (int)omxErr;
            }
        }
    }

    handle->stream_start = 1;

    return status;
}

int omx_encode_enqueue_buf(omxEncodeHandle *handle, Buf *tiovx_buffer)
{
    int status = 0;
    OMX_ERRORTYPE omxErr = OMX_ErrorNone;

    if (handle->stream_start == 0) {
        handle->queued[tiovx_buffer->buf_index] = true;
        status = omx_allocate_input_buffer(handle, tiovx_buffer);
        return status;
    } else {
        handle->queued[tiovx_buffer->buf_index] = true;
        omxErr = OMX_EmptyThisBuffer(handle->compHandle,
                                    handle->omx_bufq[tiovx_buffer->buf_index]);
        if (omxErr != OMX_ErrorNone) {
            TIOVX_MODULE_ERROR("[OMX_ENCODE] OMX_FillThisBuffer() returned 0x%08x\n", omxErr);
            return (int)omxErr;
        }
    }

    return status;
}

Buf *omx_encode_dqueue_buf(omxEncodeHandle *handle)
{
    Buf *tiovx_buffer = NULL;
    int index;
    OMX_ERRORTYPE omxErr = OMX_ErrorNone;

    while (handle->out_pq_head !=
                    ((handle->out_pq_tail + 1) % (handle->num_outbufs + 1))) {
        handle->out_pq_tail = (handle->out_pq_tail + 1) %
                                                (handle->num_outbufs + 1);
        index = handle->out_processed_queue[handle->out_pq_tail];
        omxErr = OMX_FillThisBuffer(handle->compHandle,
                                    handle->out_omx_bufq[index]);
        if (omxErr != OMX_ErrorNone) {
            TIOVX_MODULE_ERROR("[OMX_ENCODE] OMX_FillThisBuffer: return omxErr=%08x\n", omxErr);
            return NULL;
        }
    }

    sem_wait(&handle->dq_sem);

    if ((handle->pq_tail + 1) % (handle->num_inbufs + 1) == handle->pq_head) {
        TIOVX_MODULE_ERROR("[OMX_ENCODE] Queue Empty\n");
        return NULL;
    } else {
        handle->pq_tail = (handle->pq_tail + 1) %
                                            (handle->num_inbufs + 1);
    }

    index = handle->processed_queue[handle->pq_tail];

    handle->queued[index] = false;
    tiovx_buffer = (Buf *)(handle->omx_bufq[index]->pAppPrivate);

    return tiovx_buffer;
}

int omx_encode_stop(omxEncodeHandle *handle)
{
    int status = 0;

    return status;
}

int omx_encode_delete_handle(omxEncodeHandle *handle)
{
    int status = 0;

    fclose(handle->wrfd);
    free(handle);

    return status;
}
