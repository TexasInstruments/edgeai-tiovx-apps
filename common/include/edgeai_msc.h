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

#ifndef _TI_EDGEAI_MSC_H_
#define _TI_EDGEAI_MSC_H_

#include <tiovx_multi_scaler_module.h>
#include <common/include/edgeai_pre_proc.h>

namespace ti::edgeai::common
{
    using namespace std;

    class multiScaler
    {
        public:
            /* Default constructor. Use the compiler generated default one. */
            multiScaler();

            /* Destructor. */
            ~multiScaler();

            /**
             * Helper function to dump the configuration information.
             */
            void        dumpInfo();

            /** Helper function to parse MSC configuration. */
            int32_t     getConfig(int32_t input_wd, int32_t input_ht, int32_t post_proc_wd, int32_t post_proc_ht, preProc *pre_proc_obj);

        public:
            /* Data structure passed to MSC module */
            TIOVXMultiScalerModuleObj     multiScalerObj1;
            
            TIOVXMultiScalerModuleObj     multiScalerObj2;

            /* True of multiSclaerObj2 is to be used. */
            bool                          useSecondaryMsc{false};
    };
}

#endif // _TI_EDGEAI_MSC_H_