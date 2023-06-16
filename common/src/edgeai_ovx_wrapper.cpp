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
#include <common/include/edgeai_ovx_wrapper.h>

/* OpenVX headers */
#include <TI/tivx_img_proc_kernels.h>
#include <TI/dl_kernels.h>
#include <TI/hwa_kernels.h>
#include <edgeai_tiovx_target_kernels.h>
#include <TI/video_io_kernels.h>

#include<stdio.h>

namespace ti::edgeai::common
{

/* Constructor */
ovxGraph::ovxGraph()
{
    /* Create OpenVX Context */
    context = vxCreateContext();
    int32_t status = vxGetStatus((vx_reference) context);

    if(status != VX_SUCCESS)
    {
        LOG_ERROR("OpenVX context creation failed \n");
        exit(-1);
    }

    /* Load all the kernels needed for application */
    tivxVideoIOLoadKernels(context);
    tivxHwaLoadKernels(context);
    tivxImgProcLoadKernels(context);
    tivxEdgeaiImgProcLoadKernels(context);
    tivxTIDLLoadKernels(context);
}

/* Destructor */
ovxGraph::~ovxGraph()
{
    LOG_DEBUG("ovxGraph Destructor \n");

    /* Unload all the kernels needed for application */
    tivxVideoIOUnLoadKernels(context);
    tivxHwaUnLoadKernels(context);
    tivxImgProcUnLoadKernels(context);
    tivxEdgeaiImgProcUnLoadKernels(context);
    tivxTIDLUnLoadKernels(context);

    /* Release OpenVX context */
    int32_t status = vxReleaseContext(&context);
    if(status != VX_SUCCESS)
    {
        LOG_ERROR("OpenVX context release failed \n");
    }

}

} // namespace ti::edgeai::common