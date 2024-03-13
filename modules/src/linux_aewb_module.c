/*
 *
 * Copyright (c) 2024 Texas Instruments Incorporated
 *
 * All rights reserved not granted herein.
 *
 * Limited License.
 *
 * Texas Instruments Incorporated grants a world-wide, royalty-free, non-exclusive
 * license under copyrights and patents it now or hereafter owns or controls to make,
 * have made, use, import, offer to sell and sell ("Utilize") this software subject to the
 * terms herein.  With respect to the foregoing patent license, such license is granted
 * solely to the extent that any such patent is necessary to Utilize the software alone.
 * The patent license shall not apply to any combinations which include this software,
 * other than combinations with devices manufactured by or for TI ("TI Devices").
 * No hardware patent is licensed hereunder.
 *
 * Redistributions must preserve existing copyright notices and reproduce this license
 * (including the above copyright notice and the disclaimer and (if applicable) source
 * code license limitations below) in the documentation and/or other materials provided
 * with the distribution
 *
 * Redistribution and use in binary form, without modification, are permitted provided
 * that the following conditions are met:
 *
 * *       No reverse engineering, decompilation, or disassembly of this software is
 * permitted with respect to any software provided in binary form.
 *
 * *       any redistribution and use are licensed by TI for use only with TI Devices.
 *
 * *       Nothing shall obligate TI to provide you with source code for the software
 * licensed and provided to you in object code.
 *
 * If software source code is provided to you, modification and redistribution of the
 * source code are permitted provided that the following conditions are met:
 *
 * *       any redistribution and use of the source code, including any resulting derivative
 * works, are licensed by TI for use only with TI Devices.
 *
 * *       any redistribution and use of any object code compiled from the source code
 * and any resulting derivative works, are licensed by TI for use only with TI Devices.
 *
 * Neither the name of Texas Instruments Incorporated nor the names of its suppliers
 *
 * may be used to endorse or promote products derived from this software without
 * specific prior written permission.
 *
 * DISCLAIMER.
 *
 * THIS SOFTWARE IS PROVIDED BY TI AND TI'S LICENSORS "AS IS" AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL TI AND TI'S LICENSORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
 * OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
 * OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

#include "linux_aewb_module.h"
#include "tiovx_utils.h"
#include "ti_2a_wrapper.h"

#include <sys/ioctl.h>
#include <errno.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <linux/videodev2.h>

#define AEWB_DEFAULT_DEVICE "/dev/v4l-rpi-subdev0"
#define AEWB_DEFAULT_2A_FILE "/opt/imaging/imx219/dcc_2a.bin"
#define AEWB_DEFAULT_SENSOR "SENSOR_SONY_IMX219_RPI"

#define ISS_IMX390_GAIN_TBL_SIZE                (71U)

static const uint16_t gIMX390GainsTable[ISS_IMX390_GAIN_TBL_SIZE][2U] = {
  {1024, 0x20},
  {1121, 0x23},
  {1223, 0x26},
  {1325, 0x28},
  {1427, 0x2A},
  {1529, 0x2C},
  {1631, 0x2E},
  {1733, 0x30},
  {1835, 0x32},
  {1937, 0x33},
  {2039, 0x35},
  {2141, 0x36},
  {2243, 0x37},
  {2345, 0x39},
  {2447, 0x3A},
  {2549, 0x3B},
  {2651, 0x3C},
  {2753, 0x3D},
  {2855, 0x3E},
  {2957, 0x3F},
  {3059, 0x40},
  {3160, 0x41},
  {3262, 0x42},
  {3364, 0x43},
  {3466, 0x44},
  {3568, 0x45},
  {3670, 0x46},
  {3772, 0x46},
  {3874, 0x47},
  {3976, 0x48},
  {4078, 0x49},
  {4180, 0x49},
  {4282, 0x4A},
  {4384, 0x4B},
  {4486, 0x4B},
  {4588, 0x4C},
  {4690, 0x4D},
  {4792, 0x4D},
  {4894, 0x4E},
  {4996, 0x4F},
  {5098, 0x4F},
  {5200, 0x50},
  {5301, 0x50},
  {5403, 0x51},
  {5505, 0x51},
  {5607, 0x52},
  {5709, 0x52},
  {5811, 0x53},
  {5913, 0x53},
  {6015, 0x54},
  {6117, 0x54},
  {6219, 0x55},
  {6321, 0x55},
  {6423, 0x56},
  {6525, 0x56},
  {6627, 0x57},
  {6729, 0x57},
  {6831, 0x58},
  {6933, 0x58},
  {7035, 0x58},
  {7137, 0x59},
  {7239, 0x59},
  {7341, 0x5A},
  {7442, 0x5A},
  {7544, 0x5A},
  {7646, 0x5B},
  {7748, 0x5B},
  {7850, 0x5C},
  {7952, 0x5C},
  {8054, 0x5C},
  {8192, 0x5D}
};

void get_imx219_ae_dyn_params (IssAeDynamicParams *p_ae_dynPrms)
{
    uint8_t count = 0;

    p_ae_dynPrms->targetBrightnessRange.min = 40;
    p_ae_dynPrms->targetBrightnessRange.max = 50;
    p_ae_dynPrms->targetBrightness = 45;
    p_ae_dynPrms->threshold = 1;
    p_ae_dynPrms->enableBlc = 1;
    p_ae_dynPrms->exposureTimeStepSize = 1;

    p_ae_dynPrms->exposureTimeRange[count].min = 100;
    p_ae_dynPrms->exposureTimeRange[count].max = 33333;
    p_ae_dynPrms->analogGainRange[count].min = 1024;
    p_ae_dynPrms->analogGainRange[count].max = 8192;
    p_ae_dynPrms->digitalGainRange[count].min = 256;
    p_ae_dynPrms->digitalGainRange[count].max = 256;
    count++;

    p_ae_dynPrms->numAeDynParams = count;
}

void get_imx390_ae_dyn_params (IssAeDynamicParams *p_ae_dynPrms)
{
    uint8_t count = 0;

    p_ae_dynPrms->targetBrightnessRange.min = 40;
    p_ae_dynPrms->targetBrightnessRange.max = 50;
    p_ae_dynPrms->targetBrightness = 45;
    p_ae_dynPrms->threshold = 1;
    p_ae_dynPrms->enableBlc = 1;
    p_ae_dynPrms->exposureTimeStepSize = 1;

    p_ae_dynPrms->exposureTimeRange[count].min = 100;
    p_ae_dynPrms->exposureTimeRange[count].max = 33333;
    p_ae_dynPrms->analogGainRange[count].min = 1024;
    p_ae_dynPrms->analogGainRange[count].max = 8192;
    p_ae_dynPrms->digitalGainRange[count].min = 256;
    p_ae_dynPrms->digitalGainRange[count].max = 256;
    count++;

    p_ae_dynPrms->numAeDynParams = count;
}

void get_ov2312_ae_dyn_params (IssAeDynamicParams *p_ae_dynPrms)
{
    uint8_t count = 0;

    p_ae_dynPrms->targetBrightnessRange.min = 40;
    p_ae_dynPrms->targetBrightnessRange.max = 50;
    p_ae_dynPrms->targetBrightness = 45;
    p_ae_dynPrms->threshold = 5;
    p_ae_dynPrms->enableBlc = 0;
    p_ae_dynPrms->exposureTimeStepSize = 1;

    p_ae_dynPrms->exposureTimeRange[count].min = 1000;
    p_ae_dynPrms->exposureTimeRange[count].max = 14450;
    p_ae_dynPrms->analogGainRange[count].min = 16;
    p_ae_dynPrms->analogGainRange[count].max = 16;
    p_ae_dynPrms->digitalGainRange[count].min = 1024;
    p_ae_dynPrms->digitalGainRange[count].max = 1024;
    count++;

    p_ae_dynPrms->exposureTimeRange[count].min = 14450;
    p_ae_dynPrms->exposureTimeRange[count].max = 14450;
    p_ae_dynPrms->analogGainRange[count].min = 16;
    p_ae_dynPrms->analogGainRange[count].max = 42;
    p_ae_dynPrms->digitalGainRange[count].min = 1024;
    p_ae_dynPrms->digitalGainRange[count].max = 1024;
    count++;

    p_ae_dynPrms->numAeDynParams = count;
}

void gst_tiovx_isp_map_2A_values (char *sensor_name, int exposure_time,
    int analog_gain, int *exposure_time_mapped, int *analog_gain_mapped)
{
  if (strcmp(sensor_name, "SENSOR_SONY_IMX390_UB953_D3") == 0) {
      int i;
      for (i = 0; i < ISS_IMX390_GAIN_TBL_SIZE - 1; i++) {
          if (gIMX390GainsTable[i][0] >= analog_gain) {
              break;
          }
      }
      *exposure_time_mapped = exposure_time;
      *analog_gain_mapped = gIMX390GainsTable[i][1];
  } else if (strcmp(sensor_name, "SENSOR_SONY_IMX219_RPI") == 0) {
      double multiplier = 0;
      *exposure_time_mapped = (1080 * exposure_time / 33);
      multiplier = analog_gain / 1024.0;
      *analog_gain_mapped = 256.0 - 256.0 / multiplier;
  } else if (strcmp(sensor_name, "SENSOR_OV2312_UB953_LI") == 0) {
      *exposure_time_mapped = (60 * 1300 * exposure_time / 1000000);
      *analog_gain_mapped = analog_gain;
  } else {
      TIOVX_MODULE_ERROR("[AEWB] Unknown sensor: %s", sensor_name);
  }
}

void aewb_init_cfg(AewbCfg *cfg)
{
    sprintf(cfg->device, AEWB_DEFAULT_DEVICE);
    sprintf(cfg->dcc_2a_file, AEWB_DEFAULT_2A_FILE);
    sprintf(cfg->sensor_name, AEWB_DEFAULT_SENSOR);
    cfg->ae_mode = ALGORITHMS_ISS_AE_AUTO;
    cfg->awb_mode = ALGORITHMS_ISS_AWB_AUTO;
    cfg->ae_num_skip_frames = 0;
    cfg->awb_num_skip_frames = 0;
}

struct _AewbHandle {
    AewbCfg             cfg;
    tivx_aewb_config_t  aewb_config;
    SensorObj           sensor_obj;
    uint8_t             *dcc_2a_buf;
    uint32_t            dcc_2a_buf_size;
    TI_2A_wrapper       ti_2a_wrapper;
    sensor_config_get   sensor_in_data;
    sensor_config_set   sensor_out_data;
    int fd;
};

AewbHandle *aewb_create_handle(AewbCfg *cfg)
{
    AewbHandle *handle = NULL;
    vx_status status = VX_FAILURE;
    FILE *dcc_2a_fp = NULL;

    handle = malloc(sizeof(AewbHandle));
    handle->fd = -1;
    memcpy(&handle->cfg, cfg, sizeof(AewbHandle));

    handle->fd = open(cfg->device, O_RDWR | O_NONBLOCK, 0);
    if (-1 == handle->fd) {
        TIOVX_MODULE_ERROR("[AEWB] Cannot open '%s': %d, %s\n",
                            cfg->device, errno, strerror(errno));
        goto free_handle;
    }

    status = tiovx_init_sensor_obj(&handle->sensor_obj, cfg->sensor_name);
    if (VX_SUCCESS != status) {
        TIOVX_MODULE_ERROR("[AEWB] Sensor Init Failed\n");
        goto free_fd;
    }

    handle->aewb_config.sensor_img_phase = 3;
    handle->aewb_config.sensor_dcc_id = handle->sensor_obj.sensorParams.dccId;
    handle->aewb_config.sensor_img_format = 0;
    handle->aewb_config.awb_mode = cfg->awb_mode;
    handle->aewb_config.ae_mode = cfg->ae_mode;
    handle->aewb_config.awb_num_skip_frames = cfg->awb_num_skip_frames;
    handle->aewb_config.ae_num_skip_frames = cfg->ae_num_skip_frames;
    handle->aewb_config.channel_id = 0;

    dcc_2a_fp = fopen (cfg->dcc_2a_file, "rb");

    if (NULL == dcc_2a_fp) {
        TIOVX_MODULE_ERROR("[AEWB] Unable to open dcc 2a file\n");
        goto free_fd;
    }

    fseek(dcc_2a_fp, 0, SEEK_END);
    handle->dcc_2a_buf_size = ftell(dcc_2a_fp);
    fseek (dcc_2a_fp, 0, SEEK_SET);

    if (0 == handle->dcc_2a_buf_size) {
        TIOVX_MODULE_ERROR("[AEWB] dcc 2a file has 0 size\n");
        goto free_2a_file;
    }

    handle->dcc_2a_buf = (uint8_t *) tivxMemAlloc (handle->dcc_2a_buf_size,
                                                   TIVX_MEM_EXTERNAL);
    fread (handle->dcc_2a_buf, 1, handle->dcc_2a_buf_size, dcc_2a_fp);
    fclose (dcc_2a_fp);

    status = TI_2A_wrapper_create(&handle->ti_2a_wrapper, &handle->aewb_config,
                handle->dcc_2a_buf, handle->dcc_2a_buf_size);
    if (status) {
        TIOVX_MODULE_ERROR("[AEWB] Unable to create TI 2A wrapper: %d", status);
        goto free_2a_file;
    }

    if (strcmp(cfg->sensor_name, "SENSOR_SONY_IMX390_UB953_D3") == 0) {
      get_imx390_ae_dyn_params (&handle->sensor_in_data.ae_dynPrms);
    } else if (strcmp(cfg->sensor_name, "SENSOR_OV2312_UB953_LI") == 0) {
      get_ov2312_ae_dyn_params (&handle->sensor_in_data.ae_dynPrms);
    } else {
      get_imx219_ae_dyn_params (&handle->sensor_in_data.ae_dynPrms);
    }

    return handle;

free_2a_file:
    fclose(dcc_2a_fp);
free_fd:
    close(handle->fd);
free_handle:
    free(handle);
    return NULL;
}

int aewb_write_to_sensor(AewbHandle *handle)
{
    int ret = -1;
    int analog_gain = 0;
    int coarse_integration_time = 0;
    struct v4l2_control control;

    gst_tiovx_isp_map_2A_values (handle->cfg.sensor_name,
            handle->sensor_out_data.aePrms.exposureTime[0],
            handle->sensor_out_data.aePrms.analogGain[0],
            &coarse_integration_time, &analog_gain);

    control.id = V4L2_CID_EXPOSURE;
    control.value = coarse_integration_time;
    ret = ioctl (handle->fd, VIDIOC_S_CTRL, &control);
    if (ret < 0) {
        TIOVX_MODULE_ERROR("[AEWB] Unable to call exposure ioctl: %d", ret);
        return ret;
    }

    control.id = V4L2_CID_ANALOGUE_GAIN;
    control.value = analog_gain;
    ret = ioctl (handle->fd, VIDIOC_S_CTRL, &control);
    if (ret < 0) {
        TIOVX_MODULE_ERROR("[AEWB] Unable to call analog gain ioctl: %d", ret);
    }

    return ret;
}

int aewb_process(AewbHandle *handle, Buf *h3a_buf, Buf *aewb_buf)
{
    vx_status status = VX_FAILURE;
    vx_map_id h3a_buf_map_id;
    vx_map_id aewb_buf_map_id;
    tivx_h3a_data_t *h3a_ptr = NULL;
    tivx_ae_awb_params_t *aewb_ptr = NULL;

    vxMapUserDataObject ((vx_user_data_object)h3a_buf->handle, 0,
            sizeof (tivx_h3a_data_t), &h3a_buf_map_id, (void **) &h3a_ptr,
            VX_READ_ONLY, VX_MEMORY_TYPE_HOST, 0);
    vxMapUserDataObject ((vx_user_data_object)aewb_buf->handle, 0,
            sizeof (tivx_ae_awb_params_t), &aewb_buf_map_id,
            (void **)&aewb_ptr, VX_WRITE_ONLY, VX_MEMORY_TYPE_HOST, 0);

    status = TI_2A_wrapper_process(&handle->ti_2a_wrapper, &handle->aewb_config,
            h3a_ptr, &handle->sensor_in_data, aewb_ptr,
            &handle->sensor_out_data);
    if (status) {
        TIOVX_MODULE_ERROR("[AEWB] Process call failed: %d", status);
    }

    vxUnmapUserDataObject ((vx_user_data_object)h3a_buf->handle, h3a_buf_map_id);
    vxUnmapUserDataObject ((vx_user_data_object)aewb_buf->handle, aewb_buf_map_id);

    status = aewb_write_to_sensor(handle);

    return status;
}

int aewb_delete_handle(AewbHandle *handle)
{
    int status = 0;

    TI_2A_wrapper_delete(&handle->ti_2a_wrapper);
    tivxMemFree((void *)handle->dcc_2a_buf, handle->dcc_2a_buf_size,
            TIVX_MEM_EXTERNAL);
    close(handle->fd);
    free(handle);

    return status;
}
