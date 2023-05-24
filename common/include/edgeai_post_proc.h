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

#ifndef _TI_EDGEAI_POST_PROC_H_
#define _TI_EDGEAI_POST_PROC_H_

#include <stdint.h>
#include <string>

/* OpenVX Headers */
#include <TI/j7_tidl.h>
#include <tiovx_dl_post_proc_module.h>

namespace ti::edgeai::common
{
    using namespace std;

    class postProc
    {
        public:
            /* Default constructor. Use the compiler generated default one. */
            postProc();

            /* Destructor. */
            ~postProc();

            /**
             * Helper function to dump the configuration information.
             */
            void        dumpInfo();

            /** Helper function to parse post process configuration. */
            int32_t     getConfig(const string &modelBasePath, sTIDL_IOBufDesc_t *ioBufDesc, int32_t imageWidth, int32_t imageHeight);

        public:
            /* Data structure passed to post process module */
            TIOVXDLPostProcModuleObj    dlPostProcObj;

            /* YUV color map for semantic segmentation */
            uint8_t                     **mYUVColorMap{NULL};

            int                         mMaxColorClass{0};
            
    };

} // namespace ti::edgeai::common

#endif // _TI_EDGEAI_POST_PROC_H_