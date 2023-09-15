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

#ifndef _TI_EDGEAI_TIOVX_MOSAIC_H_
#define _TI_EDGEAI_TIOVX_MOSAIC_H_

/* Standard Headers */
#include <stdint.h>
#include <vector>
#include <string>

/* OpenVX Headers */
#include <tiovx_img_mosaic_module.h>

namespace ti::edgeai::common
{
    using namespace std;

    /**
     * \brief Class that wraps the mosaic related configuration
     *
     * \ingroup group_edgeai_common
     */

    class imgMosaic
    {
        public:
            /* Default constructor. Use the compiler generated default one. */
            imgMosaic();

            /* Destructor. */
            ~imgMosaic();

            /**
             * Helper function to dump the configuration information.
             */
            void        dumpInfo();

            /** Helper function to init mosaic module.
              *
              * @param context OpenVX context
              * @param mosaicInfo vector containing posX, posY, width and
              *                   height of mosaic.
              * @param title Main title text to overlay
              * @param mosaicTitle
              *
            */
           int32_t      imgMosaicInit(vx_context context,
                              vector<vector<int32_t>> &mosaicInfo,
                              string &title, vector<string> &mosaicTitle);

        private:
            /** Helper function to parse mosaic configuration.
              *
              * @param mosaicInfo vector containing posX, posY, width and
              *                   height of mosaic.
              *
            */
            int32_t     getConfig(vector<vector<int32_t>> &mosaicInfo);

            /** Function to set mosaic baground buffer.
              *
              * @param title Main title text to overlay
              * @param mosaicTitle
              *
            */
            void        setBackground(string &title,
                                    vector<string> &mosaicTitle);

        public:
            /* Data structure passed to mosaic module */
            TIOVXImgMosaicModuleObj    imgMosaicObj;

            /**
             * Used to pass input to mosaic nodes from previous post proc outputs.
             * Input to mosaic module is array of vx_object_array.
            */
            vx_object_array mosaic_input_arr[TIVX_IMG_MOSAIC_MAX_INPUTS] = {NULL};

        private:
            /* Vector to store all mosaic co-ordinates and size*/
            vector<vector<int32_t>>        m_mosaicInfo{};
    };

} /* namespace ti::edgeai::common */

#endif /* _TI_EDGEAI_TIOVX_MOSAIC_H_ */