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

#include <apps/include/misc.h>

void *map_image(vx_image image, uint8_t plane, vx_map_id *map_id)
{
    vx_rectangle_t rect;
    vx_imagepatch_addressing_t image_addr;
    vx_uint32 img_width;
    vx_uint32 img_height;
    void *ptr;

    vxQueryImage(image, VX_IMAGE_WIDTH, &img_width, sizeof(vx_uint32));
    vxQueryImage(image, VX_IMAGE_HEIGHT, &img_height, sizeof(vx_uint32));

    /* Y Plane */
    rect.start_x = 0;
    rect.start_y = 0;
    rect.end_x = img_width;
    rect.end_y = img_height;
    vxMapImagePatch(image,
                    &rect,
                    plane,
                    map_id,
                    &image_addr,
                    &ptr,
                    VX_WRITE_ONLY,
                    VX_MEMORY_TYPE_HOST,
                    VX_NOGAP_X);

    return ptr;
}

void unmap_image(vx_image image, vx_map_id map_id)
{
    vxUnmapImagePatch(image, map_id);
}

void set_mosaic_background(vx_image image, char *title)
{
    vx_uint32 img_width;
    vx_uint32 img_height;
    void *y_ptr;
    void *uv_ptr;
    vx_map_id y_ptr_map_id;
    vx_map_id uv_ptr_map_id;
    Image image_holder;
    YUVColor color_black;
    YUVColor color_white;
    YUVColor color_red;
    YUVColor color_green;
    FontProperty big_font;
    FontProperty medium_font;

    vxQueryImage(image, VX_IMAGE_WIDTH, &img_width, sizeof(vx_uint32));
    vxQueryImage(image, VX_IMAGE_HEIGHT, &img_height, sizeof(vx_uint32));

    y_ptr = map_image(image, 0, &y_ptr_map_id);
    uv_ptr = map_image(image, 1, &uv_ptr_map_id);

    image_holder.width = img_width;
    image_holder.height = img_height;
    image_holder.yRowAddr = (uint8_t *)y_ptr;
    image_holder.uvRowAddr = (uint8_t *)uv_ptr;

    getColor(&color_black, 0, 0, 0);
    getColor(&color_white, 255, 255, 255);
    getColor(&color_red, 255, 0, 0);
    getColor(&color_green, 0, 255, 0);

    getFont(&big_font, 0.02*img_width);
    getFont(&medium_font, 0.01*img_width);

    /* Set background to black */
    drawRect(&image_holder,0,0,img_width,img_height,&color_black,-1);

    /* Draw title */
    drawText(&image_holder,
             "EDGEAI TIOVX APPS",
             2,
             2,
             &big_font,
             &color_red);

    drawText(&image_holder,
             title,
             2,
             big_font.height + 2,
             &medium_font,
             &color_green);

    unmap_image(image, y_ptr_map_id);
    unmap_image(image, uv_ptr_map_id);
}

void update_perf_overlay(vx_image image, EdgeAIPerfStats *perf_stats_handle)
{
    vx_uint32 img_width;
    vx_uint32 img_height;
    void *y_ptr;
    void *uv_ptr;
    vx_map_id y_ptr_map_id;
    vx_map_id uv_ptr_map_id;

    if(NULL != image)
    {
        vxQueryImage(image, VX_IMAGE_WIDTH, &img_width, sizeof(vx_uint32));
        vxQueryImage(image, VX_IMAGE_HEIGHT, &img_height, sizeof(vx_uint32));

        y_ptr = map_image(image, 0, &y_ptr_map_id);
        uv_ptr = map_image(image, 1, &uv_ptr_map_id);

        perf_stats_handle->overlayType = OVERLAY_TYPE_GRAPH;
        perf_stats_handle->overlay.imgYPtr = (uint8_t *)y_ptr;
        perf_stats_handle->overlay.imgUVPtr = (uint8_t *)uv_ptr;
        perf_stats_handle->overlay.imgWidth = img_width;
        perf_stats_handle->overlay.imgHeight = img_height;
        perf_stats_handle->overlay.xPos = 0;
        perf_stats_handle->overlay.yPos = 0;
        perf_stats_handle->overlay.width = img_width;
        perf_stats_handle->overlay.height = img_height;

        update_edgeai_perf_stats(perf_stats_handle);

        unmap_image(image, y_ptr_map_id);
        unmap_image(image, uv_ptr_map_id);
    }
    else
    {
        perf_stats_handle->overlayType = OVERLAY_TYPE_NONE;
        update_edgeai_perf_stats(perf_stats_handle);
    }
}

void print_perf(GraphObj *graph, EdgeAIPerfStats *perf_stats_handle)
{

    if(0 == perf_stats_handle->frameCount)
    {
        Stats *stats = &perf_stats_handle->stats;

        printf("================================================\n\n");
        printf ("CPU: mpu: TOTAL LOAD = %.2f\n",
                (float)stats->cpu_load.cpu_load/100);

#if defined(SOC_AM62A) || defined(SOC_J721E) || defined(SOC_J721S2) || defined(SOC_J784S4) || defined(SOC_J722S) || defined(SOC_J742S2)
        for (uint32_t i = 0; i < APP_IPC_CPU_MAX; i++)
        {
            const char *cpu_name = appIpcGetCpuName(i);
            if (appIpcIsCpuEnabled (i) &&
                (NULL != strstr (cpu_name, "c7x") ||
                NULL != strstr (cpu_name, "mcu")))
            {
                printf ("CPU: %6s: TOTAL LOAD = %.2f\n",
                        appIpcGetCpuName (i),
                        (float)stats->cpu_loads[i].cpu_load/100);
            }
        }

        for (uint32_t i = 0; i < stats->hwa_count ; i++)
        {
            app_perf_stats_hwa_load_t *hwaLoad;
            uint64_t load;
            for (uint32_t j = 0; j < APP_PERF_HWA_MAX; j++)
            {
                app_perf_hwa_id_t id = (app_perf_hwa_id_t) j;
                hwaLoad = &stats->hwa_loads[i].hwa_stats[id];

                if (hwaLoad->active_time > 0 &&
                    hwaLoad->pixels_processed > 0 &&
                    hwaLoad->total_time > 0)
                {
                    load = (hwaLoad->active_time * 10000) / hwaLoad->total_time;
                    printf ("HWA: %6s: LOAD = %.2f %% ( %lu MP/s )\n",
                            appPerfStatsGetHwaName (id),
                            (float)load/100,
                            (hwaLoad->pixels_processed / hwaLoad->total_time));
                }
            }
        }
#endif
        printf ("DDR: READ  BW: AVG = %6d MB/s, PEAK = %6d MB/s\n",
                stats->ddr_load.read_bw_avg, stats->ddr_load.read_bw_peak);

        printf ("DDR: WRITE BW: AVG = %6d MB/s, PEAK = %6d MB/s\n",
                stats->ddr_load.write_bw_avg, stats->ddr_load.write_bw_peak);

        printf ("DDR: TOTAL BW: AVG = %6d MB/s, PEAK = %6d MB/s\n",
                stats->ddr_load.read_bw_avg + stats->ddr_load.write_bw_avg,
                stats->ddr_load.write_bw_peak + stats->ddr_load.read_bw_peak);

        for (uint32_t i = 0; i < NUM_THERMAL_ZONE; i++)
        {
            printf ("TEMP: %s = %.2f C\n",
                    stats->soc_temp.thermal_zone_name[i],
                    stats->soc_temp.thermal_zone_temp[i]);
        }

        printf ("FPS: %d\n\n",stats->fps);

        for(uint32_t i = 0; i < graph->num_nodes; i++)
        {
            if(TIOVX_TIDL == graph->node_list[i].node_type)
            {
                vx_perf_t perf;
                vxQueryNode(graph->node_list[i].tiovx_node,
                            VX_NODE_PERFORMANCE,
                            &perf,
                            sizeof(perf));
                printf("%s - %0.3f ms\n", graph->node_list[i].name, perf.avg/1000000.0);
            }
        }

        printf("\n");

        vx_perf_t graph_perf;
        vxQueryGraph(graph->tiovx_graph,
                    VX_GRAPH_PERFORMANCE,
                    &graph_perf,
                    sizeof(graph_perf));
        printf("Graph - %0.3f ms\n", graph_perf.avg/1000000.0);
        printf("================================================\n\n");
    }
}

void generate_datasheet(GraphObj *graph, EdgeAIPerfStats *perf_stats_handle)
{

    if(0 == perf_stats_handle->frameCount)
    {
        FILE *fptr;
        const char* filename = "TIOVX_APP_demo";

        fptr = fopen(filename, "w+");

        if (fptr == NULL) {
            printf("The file is not opened. The program will "
                   "exit now");
            exit(0);
        }

        Stats *stats = &perf_stats_handle->stats;

        fprintf(fptr, "MPU Load (%%) = %.2f\n",
                (float)stats->cpu_load.cpu_load/100);

#if defined(SOC_AM62A) || defined(SOC_J721E) || defined(SOC_J721S2) || defined(SOC_J784S4) || defined(SOC_J722S)
        int c7x_id = 0;
        for (uint32_t i = 0; i < APP_IPC_CPU_MAX; i++)
        {
            const char *cpu_name = appIpcGetCpuName(i);
            if (appIpcIsCpuEnabled (i) &&
                (NULL != strstr (cpu_name, "c7x")))
            {
                fprintf(fptr, "C7x_%d Load (%%) = %.2f\n",
                        c7x_id, (float)stats->cpu_loads[i].cpu_load/100);
                c7x_id++;
            }
        }

        for (uint32_t i = 0; i < stats->hwa_count ; i++)
        {
            app_perf_stats_hwa_load_t *hwaLoad;
            uint64_t load;
            for (uint32_t j = 0; j < APP_PERF_HWA_MAX; j++)
            {
                app_perf_hwa_id_t id = (app_perf_hwa_id_t) j;
                hwaLoad = &stats->hwa_loads[i].hwa_stats[id];

                if (hwaLoad->active_time > 0 &&
                    hwaLoad->pixels_processed > 0 &&
                    hwaLoad->total_time > 0)
                {
                    load = (hwaLoad->active_time * 10000) / hwaLoad->total_time;
                    fprintf(fptr, "%6s Load (%%) = %.2f\n",
                            appPerfStatsGetHwaName (id),
                            (float)load/100);
                }
            }
        }
#endif
        fprintf(fptr, "DDR Read BW (MB/s): %6d\n",
                stats->ddr_load.read_bw_avg);

        fprintf(fptr, "DDR Write BW (MB/s): %6d\n",
                stats->ddr_load.write_bw_avg);

        fprintf(fptr, "DDR Total BW (MB/s): %6d\n",
                stats->ddr_load.read_bw_avg + stats->ddr_load.write_bw_avg);

        fprintf(fptr, "FPS: %d\n", stats->fps);

        for(uint32_t i = 0; i < graph->num_nodes; i++)
        {
            if(TIOVX_TIDL == graph->node_list[i].node_type)
            {
                vx_perf_t perf;
                vxQueryNode(graph->node_list[i].tiovx_node,
                            VX_NODE_PERFORMANCE,
                            &perf,
                            sizeof(perf));
                fprintf(fptr, "Inference time (ms): %0.3f\n", perf.avg/1000000.0);
            }
        }

        fprintf(fptr,"\n");
        fclose(fptr);
    }
}
