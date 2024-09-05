#!/bin/bash

#  Copyright (C) 2021 Texas Instruments Incorporated - http://www.ti.com/
#
#  Redistribution and use in source and binary forms, with or without
#  modification, are permitted provided that the following conditions
#  are met:
#
#    Redistributions of source code must retain the above copyright
#    notice, this list of conditions and the following disclaimer.
#
#    Redistributions in binary form must reproduce the above copyright
#    notice, this list of conditions and the following disclaimer in the
#    documentation and/or other materials provided with the
#    distribution.
#
#    Neither the name of Texas Instruments Incorporated nor the names of
#    its contributors may be used to endorse or promote products derived
#    from this software without specific prior written permission.
#
#  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
#  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
#  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
#  A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
#  OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
#  SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
#  LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
#  DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
#  THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
#  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
#  OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

topdir=/opt/edgeai-tiovx-apps

BRIGHTWHITE='\033[0;37;1m'
RED='\033[0;31m'
GREEN='\033[0;32m'
NOCOLOR='\033[0m'

####################################################################################################
timeout=20
config_file=("$topdir/datasheet/video_config.yaml" \
             "$topdir/datasheet/camera_config.yaml")
parse_script="$topdir/datasheet/convert_to_csv.sh" 
####################################################################################################
cleanup() {
	echo
	echo "[Ctrl-C] Killing the script..."
	ps -eaf | grep "dump_datasheet_stats.sh" | awk -F" " '{print $2}' | xargs kill -9
}

####################################################################################################
usage() {
	echo "./dump_datasheet_stats.sh"
}

####################################################################################################
MODEL_NAME=("ONR-OD-8200-yolox-nano-lite-mmdet-coco-416x416" \
            "TFL-OD-2020-ssdLite-mobDet-DSP-coco-320x320" \
            "ONR-CL-6360-regNetx-200mf" \
            "TFL-CL-0000-mobileNetV1-mlperf")

rm -rf $appdir/datasheet/${SOC}_camera_tiovx_apps_stats.txt

for config in "${config_file[@]}"; do
    if [[ $config =~ "video" ]]; then
        rm -rf $appdir/datasheet/${SOC}_video_tiovx_apps.txt
    else
        rm -rf $appdir/datasheet/${SOC}_camera_tiovx_apps_stats.txt
    fi

    for modelname in "${MODEL_NAME[@]}"; do
        ./dump_engine.sh "TIOVX_APPS-TEST" $config $timeout $modelname 
    done
done

# Convert data to csv format
source $parse_script
