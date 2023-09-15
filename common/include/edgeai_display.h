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

#ifndef _TI_EDGEAI_TIOVX_DISPLAY_H_
#define _TI_EDGEAI_TIOVX_DISPLAY_H_

/* Module Headers */
#include <common/include/edgeai_demo_config.h>

/* OpenVX Headers */
#include <tiovx_display_module.h>

extern "C"
{
#include <utils/grpx/include/app_grpx.h>
}

namespace ti::edgeai::common
{
    using namespace std;

    /**
     * \brief Class that wraps the display related configuration
     *
     * \ingroup group_edgeai_common
     */

    class display
    {
        public:
            /** Constructor. */
            display();

            /** Destructor. */
            ~display();

            /** Helper function to dump the configuration information. */
            void        dumpInfo();

            /** Helper function to init display module.
             *
             * @param context OpenVX context
             *
             */
            int32_t     displayInit(vx_context context, OutputInfo *output);

        private:
            /** Helper function to parse display configuration.
             *
             * @param input_wd Width of the input frame
             *
             */
            int32_t     getConfig(vx_context context, OutputInfo *output);

        public:
            /** Data structure passed to display module */
            TIOVXDisplayModuleObj               displayObj;

        private:
#if !defined (SOC_AM62A)
            /** Performance overlay parameter struct */
            app_grpx_init_prms_t                grpx_prms;
#endif

    };
} /* namespace ti::edgeai::common */

#endif /* _TI_EDGEAI_TIOVX_DISPLAY_H_ */