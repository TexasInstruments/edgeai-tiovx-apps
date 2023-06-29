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

#ifndef _TI_EDGEAI_TIOVX_TIDL_H_
#define _TI_EDGEAI_TIOVX_TIDL_H_

/* Standard Headers */
#include <string>

#include <TI/tivx.h>
#include <TI/j7_tidl.h>
#include <tiovx_tidl_module.h>

namespace ti::edgeai::common
{
    using namespace std;

    /**
     * \brief Class that wraps the TIDL related configuration
     *
     * \ingroup group_edgeai_common
     */

    class tidlInf
    {
        public:
            /* Default constructor. Use the compiler generated default one. */
            tidlInf();

            /* Destructor. */
            ~tidlInf();

            /**
             * Helper function to dump the configuration information.
             */
            void        dumpInfo();

            /** Helper function to parse TIDL Inference configuration.
             * @param modelBasePath path of model directory
             * @param context OpenVX context
             *
             */
            int32_t     getConfig(const string  &modelBasePath,
                                  vx_context    context);

            /** Function to read and parse IO.bin file from model zoo Path
             * @param context OpenVX context
             *
             * @return OpenVX user data object for tivxTIDLJ7Params
             */
            vx_user_data_object readConfig(vx_context context);

        public:
            /* Data structure passed to TIDL module */
            TIOVXTIDLModuleObj      tidlObj;
            
            /* IO Buffer Descriptor of the model */
            sTIDL_IOBufDesc_t       *ioBufDesc;

        private:
            /* Path to network file of the model */
            string                  network_file_path{""};
    };
    
} // namespace ti::edgeai::common

#endif // _TI_EDGEAI_TIOVX_TIDL_H_