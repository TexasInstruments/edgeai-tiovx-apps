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

#ifndef _TI_EDGEAI_TIOVX_MSC_H_
#define _TI_EDGEAI_TIOVX_MSC_H_

/* Modules Headers */
#include <common/include/edgeai_pre_proc.h>
#include <common/include/edgeai_camera.h>

/* OpenVX Headers */
#include <tiovx_multi_scaler_module.h>

namespace ti::edgeai::common
{
    using namespace std;

    /**
     * \brief Class that wraps the multiscaler related configuration
     *
     * \ingroup group_edgeai_common
     */

    class multiScaler
    {
        public:
            /** Constructor. */
            multiScaler();

            /** Destructor. */
            ~multiScaler();

            /** Helper function to dump the configuration information. */
            void        dumpInfo();

            /** Helper function to init MSC module.
             *
             * @param context OpenVX context
             * @param input_wd Width of the input frame
             * @param input_ht Height of the input frame
             * @param post_proc_wd Width of the output frame (for post-process)
             * @param post_proc_ht Height of the output frame (for post-process)
             * @param pre_proc_obj Pre-Processing object
             * @param srcType Source Type
             * @param cameraObj Camera Object
             *
             */
            int32_t     multiScalerInit(vx_context context,
                                        int32_t input_wd,
                                        int32_t input_ht,
                                        int32_t post_proc_wd,
                                        int32_t post_proc_ht,
                                        preProc *pre_proc_obj,
                                        string &srcType,
                                        camera*& cameraObj);

        private:
            /** Helper function to parse MSC configuration.
             *
             * @param input_wd Width of the input frame
             * @param input_ht Height of the input frame
             * @param post_proc_wd Width of the output frame (for post-process)
             * @param post_proc_ht Height of the output frame (for post-process)
             * @param pre_proc_obj Pre-Processing object
             *
             */
            int32_t     getConfig(int32_t input_wd,
                                  int32_t input_ht,
                                  int32_t post_proc_wd,
                                  int32_t post_proc_ht,
                                  preProc *pre_proc_obj);

        public:
            /** Data structure passed to MSC module */
            TIOVXMultiScalerModuleObj     multiScalerObj1;
            
            /** Optional second MSC to be used when need to scale by a factor
             *  more than 4.
             */
            TIOVXMultiScalerModuleObj     multiScalerObj2;

            /* True if multiSclaerObj2 is to be used. */
            bool                          useSecondaryMsc{false};

            /** Flag to indicate if tiovx MSC node is first in subflow,
             *  where a subflow corresonds to a unique model in a flow.
             */
            bool                          isHeadNode{true};
    };
} /* namespace ti::edgeai::common */

#endif /* _TI_EDGEAI_TIOVX_MSC_H_ */