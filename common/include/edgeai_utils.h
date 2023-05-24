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

#ifndef _TI_EDGEAI_TIOVX_UTILS_H_
#define _TI_EDGEAI_TIOVX_UTILS_H_

/* Standard headers. */
#include <string>
#include <thread>

/* Third-party headers. */
#include <yaml-cpp/yaml.h>

/* OpenVX headers */
#include <TI/tivx.h>

using namespace std;

namespace ti::edgeai::common
{
    /**
     * Helper function to convert TIDL data types to OpenVX data types
     *
     * @param tidl_type TIDL data type
     *
     * @return OpenVX data type
     */
    vx_size getTensorDataType(vx_int32 tidl_type);

    /**
     * Helper function to parse classnames from model directory
     *
     * @param modelBasePath Path to model directory
     *
     * @param classnames Classnames will be stored in this buffer
     */
    void getClassNames(const string &modelBasePath, char (*classnames)[256]);

    /**
     * Helper function to convert string to fraction
     *
     * @param num numeric integer or decimal string
     *
     * @return string representation of fraction (Ex: "1/2")
     */
    const string to_fraction(string& num);

    /**
     * Helper function to check if string is numeric
     *
     * @param s string to be checked
     *
     * @return true if string is purely numeric, else false
     */
    template<typename Numeric> bool _is_number(const string& s);

} // namespace ti::edgeai::common

#endif /* _TI_EDGEAI_TIOVX_UTILS_H_ */

