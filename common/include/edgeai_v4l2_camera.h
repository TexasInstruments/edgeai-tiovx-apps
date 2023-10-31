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

#ifndef _TI_EDGEAI_TIOVX_V4L2_CAMERA_H_
#define _TI_EDGEAI_TIOVX_V4L2_CAMERA_H_

/* Standard Headers */
#include <stdint.h>
#include <vector>

/* Module Headers */
#include <common/include/edgeai_demo_config.h>

/* OpenVX Headers */
#include <tiovx_sensor_module.h>
#include <tiovx_viss_module.h>
#include <tiovx_ldc_module.h>

namespace ti::edgeai::common
{
    using namespace std;

    class v4l2Camera
    {
        public:
            /* Constructor. */
            v4l2Camera(InputInfo* inputInfo);

            /* Destructor. */
            ~v4l2Camera();

            /** Helper function to initialize ISP and LDC.
             *
             * @param context openVX context for reuired graph.
             */
            int32_t     v4l2CameraInit(vx_context context);

            /** Helper function to create ISP and LDC nodes.
             *
             * @param graph openVX graph.
             */
            int32_t     v4l2CameraCreate(vx_graph graph);

            /** Helper function to capture from v4l2 device and enqueue.
             *
             * @param graph openVX graph.
             */
            int32_t     v4l2CameraCapture(vx_graph graph);

        public:
            InputInfo                *m_inputInfo;

            /* Data structure passed to sensor module */
            SensorObj                sensorObj;

            /* Data structure passed to viss module */
            TIOVXVISSModuleObj       vissObj;

            /* Data structure passed to ldc module */
            TIOVXLDCModuleObj        ldcObj;

        private:
            /** Helper function to parse camera input configuration.
             *  Parse and fill camera related module configs.
             *
             * @param camInputInfo Input info of camera input
             * @param chMask channel mask for sensor node.
             */
            int32_t     getConfig(InputInfo* camInputInfo);

    };

} /* namespace ti::edgeai::common */

#endif /* _TI_EDGEAI_TIOVX_V4L2_CAMERA_H_ */
