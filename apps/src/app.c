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

#include <apps/include/info.h>
#include <apps/include/input_block.h>
#include <apps/include/resize_block.h>
#include <apps/include/deep_learning_block.h>
#include <apps/include/output_block.h>
#include <apps/include/misc.h>

#include <tiovx_modules.h>
#include <tiovx_utils.h>

#include <unistd.h>
#include <pthread.h>
#include <signal.h>
#include <math.h>

static volatile int run_loop = 1;
pthread_mutex_t r_thread_lock[MAX_FLOWS];
pthread_mutex_t w_thread_lock[MAX_FLOWS];

/* Data required by read image thread */
struct read_img_thread_data
{
  int32_t       id;
  InputInfo     *input_info;
  Buf           *read_image_buf;
};

/* Data required by write image thread */
struct write_img_thread_data
{
  int32_t       id;
  OutputInfo    *output_info;
  Buf           *write_image_buf;
};

void interrupt_handler(int32_t x)
{
    run_loop = 0;
}

void *read_img_thread(void *arg) 
{
    /* This thread is used to read raw image and fill it in input buffer.
     * The loop then sleeps for some time to maintain user defined fps
     */
    uint32_t counter;
    useconds_t sleep_time;
    struct read_img_thread_data *data;   
    
    data = (struct read_img_thread_data *)arg;
    sleep_time = (useconds_t) (1000000/data->input_info->framerate);
    counter = 0;

    while (run_loop)
    {
        if (counter == data->input_info->num_raw_img)
        {
            if(false == data->input_info->loop)
            {
                break;
            }
            counter = 0;
        }

        pthread_mutex_lock(&r_thread_lock[data->id]);

        readImage(data->input_info->raw_img_paths[counter],
                  (vx_image)data->read_image_buf->handle);

        pthread_mutex_unlock(&r_thread_lock[data->id]);

        counter++;
        usleep(sleep_time);
    }

    return NULL;
}

void *write_img_thread(void *arg)
{
    /* This thread is used to write raw image to input directory.
     * The loop then sleeps for some time to maintain user defined fps
     */
    uint32_t counter;
    useconds_t sleep_time;
    char output_filename[512];
    struct write_img_thread_data *data;

    data = (struct write_img_thread_data *)arg;
    sleep_time = (useconds_t) (1000000/data->output_info->framerate);
    counter = 0;

    while (run_loop)
    {
        /* Max file written will be 9999 and then files will start getting
         * overwritten
         */
        if (counter == 9999)
        {
            counter = 0;
        }

        sprintf(output_filename, "%s/%04d.yuv", data->output_info->output_path, counter);

        pthread_mutex_lock(&w_thread_lock[data->id]);
        writeImage(output_filename, (vx_image)data->write_image_buf->handle);
        pthread_mutex_unlock(&w_thread_lock[data->id]);

        counter++;
        usleep(sleep_time);
    }

    return NULL;
}

int32_t connect_blocks(GraphObj *graph,
                       FlowInfo flow_infos[],
                       uint32_t num_flows,
                       InputBlock input_blocks[],
                       uint32_t *num_input_blocks,
                       OutputBlock output_blocks[],
                       uint32_t *num_output_blocks)
{
    int32_t status;
    uint32_t i, j, k;

    *num_input_blocks = 0;
    *num_output_blocks = 0;

    for(i = 0; i < num_flows; i++)
    {
        for (j = 0; j < flow_infos[i].num_subflows; j++)
        {
            bool found = false;
            char* output_name = flow_infos[i].subflow_infos[j].output_info.name;

            /* Iterate through output_blocks and if sink already present the skip */
            for (k = 0; k < *num_output_blocks; k++)
            {
                if (0 == strcmp(output_name, output_blocks[k].output_info->name))
                {
                    found = true;
                    break;
                }
            }

            if(!found)
            {
                OutputBlock output_block;

                initialize_output_block(&output_block);
                output_block.output_info = &flow_infos[i].subflow_infos[j].output_info;

                output_blocks[*num_output_blocks] = output_block;

                *num_output_blocks = *num_output_blocks + 1;
              }
        }
    }

    for(i = 0; i < num_flows; i++)
    {  
        /* Initialize Input Block */
        InputBlock  input_block;

        /* Array of all resize block required by this Input */
        ResizeBlock resize_blocks[MAX_SUBFLOW];
        uint32_t num_resize_blocks = 0;

        /* Array to contain unique subflow dims
         * [width, height, crop_x, crop_y, num]
         * For example:
         *      [640,360,0,0,3] means that there are 3 subflows with this dims.
         */
        uint32_t unique_dims[2 * MAX_SUBFLOW][5];
        uint32_t num_unique_dims = 0;

        /* Initialize the input block */
        initialize_input_block(&input_block);
        input_block.input_info = &flow_infos[i].input_info;
    
        memset(&unique_dims,0,sizeof(unique_dims));

        /* Iterate through subflow and get unqiue dimensions */
        for (j = 0; j < flow_infos[i].num_subflows; j++)
        {
            bool found = false;
            uint32_t input_width[2], input_height[2];
            uint32_t crop_start_x[2], crop_start_y[2];
            float crop_x_pct, crop_y_pct;
            PreProcInfo pre_proc_info;

            if (flow_infos[i].subflow_infos[j].has_model)
            {
                /* Get resize info for the model */
                pre_proc_info = flow_infos[i].subflow_infos[j].model_info.pre_proc_info;
                crop_x_pct = (float)(((pre_proc_info.resize_width - pre_proc_info.crop_width) / 2 ) / (float)pre_proc_info.resize_width);
                crop_y_pct = (float)(((pre_proc_info.resize_height - pre_proc_info.crop_height) / 2 ) / (float)pre_proc_info.resize_height);
    
                input_width[0] = pre_proc_info.crop_width;
                input_height[0] = pre_proc_info.crop_height;
                input_width[1] = flow_infos[i].subflow_infos[j].max_mosaic_width;
                input_height[1] = flow_infos[i].subflow_infos[j].max_mosaic_height;
                crop_start_x[0] = crop_x_pct * flow_infos[i].input_info.width;
                crop_start_y[0] = crop_y_pct * flow_infos[i].input_info.height;
                crop_start_x[1] = 0;
                crop_start_y[1] = 0;
            }
            else
            {
                /* No model, direct resize is defined */
                input_width[0] = flow_infos[i].subflow_infos[j].max_mosaic_width;
                input_height[0] = flow_infos[i].subflow_infos[j].max_mosaic_height;
                input_width[1] = 0;
                input_height[1] = 0;
                crop_start_x[0] = 0;
                crop_start_y[0] = 0;
                crop_start_x[0] = 0;
                crop_start_y[0] = 0;
            }

            /* Check if dims already exist in unique_dims aray. If yes then 
             * increment the number for this dims.
             */
            for(k = 0; k < num_unique_dims; k++)
            {
                if(input_width[0] == unique_dims[k][0] &&
                   input_height[0] == unique_dims[k][1] &&
                   crop_start_x[0] == unique_dims[k][2] &&
                   crop_start_y[0] == unique_dims[k][3])
                {
                    unique_dims[k][4]++;
                    found = true;
                    break;
                }
            }

            if (false == found)
            {
                unique_dims[num_unique_dims][0] = input_width[0];
                unique_dims[num_unique_dims][1] = input_height[0];
                unique_dims[num_unique_dims][2] = crop_start_x[0];
                unique_dims[num_unique_dims][3] = crop_start_y[0];
                unique_dims[num_unique_dims][4] = 1;
                num_unique_dims++;
            }

            if(input_width[1] != 0 && input_height[1] != 0)
            {
                found = false;
                for(k = 0; k < num_unique_dims; k++)
                {
                    if(input_width[1] == unique_dims[k][0] &&
                       input_height[1] == unique_dims[k][1] &&
                       crop_start_x[1] == unique_dims[k][2] &&
                       crop_start_y[1] == unique_dims[k][3])
                    {
                        unique_dims[k][4]++;
                        found = true;
                        break;
                    }
                }

                if (false == found)
                {
                    unique_dims[num_unique_dims][0] = input_width[1];
                    unique_dims[num_unique_dims][1] = input_height[1];
                    unique_dims[num_unique_dims][2] = crop_start_x[1];
                    unique_dims[num_unique_dims][3] = crop_start_y[1];
                    unique_dims[num_unique_dims][4] = 1;
                    num_unique_dims++;
                }
            }
        }

        /* Initialize required resize block and create needed output groups. 
         * Each resize block can produce 4 different output resolution, hence
         * total required resize block is num_unique_dims/4
         */
        num_resize_blocks = (uint32_t)(ceil(num_unique_dims / 4.0));

        for(j = 0; j < num_resize_blocks; j++)
        {
            ResizeBlock resize_block;
            initialize_resize_block(&resize_block);
            resize_block.input_width = flow_infos[i].input_info.width;
            resize_block.input_height = flow_infos[i].input_info.height;
            resize_block.num_channels = flow_infos[i].input_info.num_channels;
            
            /* For each output group of resize block, set the appropriate
             * parameters like width, height, crop, and number of output needed
             * from that group
             */
            for (k = 0; k < 4 && (j*4)+k < num_unique_dims; k++)
            {
                uint32_t width = unique_dims[(j*4)+k][0];
                uint32_t height = unique_dims[(j*4)+k][1];
                uint32_t crop_start_x = unique_dims[(j*4)+k][2];
                uint32_t crop_start_y = unique_dims[(j*4)+k][3];
                uint32_t num_required_output = unique_dims[(j*4)+k][4];
                status = add_resize_block_output_group(&resize_block,
                                                        width,
                                                        height,
                                                        crop_start_x,
                                                        crop_start_y,
                                                        num_required_output);
                if(0 != status)
                {
                    TIOVX_APPS_ERROR("Cannot add output group to resize block\n");
                    return status;
                }
            }
            resize_blocks[j] = resize_block;
        }

        /* Iterate through subflow and create deep learning block if needed. */
        for (j = 0; j < flow_infos[i].num_subflows; j++)
        {
            char *output_name;
            uint32_t crop_start_x, crop_start_y;
            float crop_x_pct, crop_y_pct;
            PreProcInfo pre_proc_info;
            
            output_name =  flow_infos[i].subflow_infos[j].output_info.name;

            if(flow_infos[i].subflow_infos[j].has_model)
            {
                DeepLearningBlock dl_block;
                initialize_deep_learning_block(&dl_block);
                dl_block.subflow_info = &flow_infos[i].subflow_infos[j];
                dl_block.post_proc_width = flow_infos[i].subflow_infos[j].max_mosaic_width;
                dl_block.post_proc_height = flow_infos[i].subflow_infos[j].max_mosaic_height;
                dl_block.num_channels = flow_infos[i].input_info.num_channels;

                status = create_deep_learning_block(graph, &dl_block);
                if(0 != status)
                {
                    TIOVX_APPS_ERROR("Cannot create deep learning block\n");
                    return status;
                }

                pre_proc_info = flow_infos[i].subflow_infos[j].model_info.pre_proc_info;
                crop_x_pct = (float)(((pre_proc_info.resize_width - pre_proc_info.crop_width) / 2 ) / (float)pre_proc_info.resize_width);
                crop_y_pct = (float)(((pre_proc_info.resize_height - pre_proc_info.crop_height) / 2 ) / (float)pre_proc_info.resize_height);
                crop_start_x = crop_x_pct * flow_infos[i].input_info.width;
                crop_start_y = crop_y_pct * flow_infos[i].input_info.height;

                /* Link pre_proc_pad to appropriate resize block
                 * Iterate through all resize blocks, check which resize block
                 * and which output group in that reisze block matches the
                 * required dimension and then link the pad from that group to
                 * pre proc input pad. 
                 */
                for(k = 0; k < num_resize_blocks; k++)
                {
                    int32_t output_group_num;

                    /* Link pre proc pad */
                    output_group_num = get_resize_block_output_group_index(&resize_blocks[k],
                                                                           dl_block.pre_proc_width,
                                                                           dl_block.pre_proc_height,
                                                                           crop_start_x,
                                                                           crop_start_y);
                    if(-1 != output_group_num)
                    {
                        connect_pad_to_resize_block_output_group(&resize_blocks[k],
                                                                 output_group_num,
                                                                 dl_block.pre_proc_input_pad);
                    }

                    /* Link post proc pad */
                    output_group_num = get_resize_block_output_group_index(&resize_blocks[k],
                                                                           dl_block.post_proc_width,
                                                                           dl_block.post_proc_height,
                                                                           0,
                                                                           0);
                    if(-1 != output_group_num)
                    {
                        connect_pad_to_resize_block_output_group(&resize_blocks[k],
                                                                 output_group_num,
                                                                 dl_block.post_proc_input_pad);
                    }
                }
                
                /* Link output pad to Output block */
                for(k = 0; k < *num_output_blocks; k++)
                {
                    if (0 == strcmp(output_name, output_blocks[k].output_info->name))
                    {
                        connect_pad_to_output_block(&output_blocks[k],
                                                    dl_block.output_pad,
                                                    &flow_infos[i].subflow_infos[j]);
                        break;
                    }
                }
            }

            else
            {
                for(k = 0; k < num_resize_blocks; k++)
                {
                    int32_t output_group_num;

                    /* Link pre proc pad */
                    output_group_num = get_resize_block_output_group_index(&resize_blocks[k],
                                                                           flow_infos[i].subflow_infos[j].max_mosaic_width,
                                                                           flow_infos[i].subflow_infos[j].max_mosaic_height,
                                                                           0,
                                                                           0);
                    if(-1 != output_group_num)
                    {
                        create_resize_block_output_group_exposed_pad(&resize_blocks[k],
                                                                     output_group_num,
                                                                     &flow_infos[i].subflow_infos[j]);
                    }
                }
            }
        }

        /* Finally create the resize blocks and link to input.
         * Also link all exposed pad of resize block to output
         */
        for(j = 0; j < num_resize_blocks; j++)
        {
            status = create_resize_block(graph, &resize_blocks[j]);
            if(0 != status)
            {
                TIOVX_APPS_ERROR("Cannot create resize block\n");
                return status;
            }

            /* Link resize block to input block */
            status = connect_pad_to_input_block(&input_block,
                                                resize_blocks[j].input_pad);
            if(0 != status)
            {
                TIOVX_APPS_ERROR("Cannot link input and resize block\n");
                return status;
            }

            /* Link exposed pad of resize group to output block */
            for(k = 0; k < resize_blocks[j].total_output_group; k++)
            {
                uint32_t p, q;
                for (p = 0; p < resize_blocks[j].output_group[k].num_exposed_pads; p++)
                {
                    char *output_name = resize_blocks[j].output_group[k].exposed_pad_info[p]->output_info.name;
                    for(q = 0; q < *num_output_blocks; q++)
                    {
                        if (0 == strcmp(output_name, output_blocks[q].output_info->name))
                        {
                            connect_pad_to_output_block(&output_blocks[q],
                                                        resize_blocks[j].output_group[k].exposed_pads[p],
                                                        resize_blocks[j].output_group[k].exposed_pad_info[p]);
                            break;
                        }
                    }
                }
            }
        }

        /* Create Input Block */
        status = create_input_block(graph, &input_block);
        if(0 != status)
        {
            TIOVX_APPS_ERROR("Cannot create input block\n");
            return status;
        }
        input_blocks[i] = input_block;
        *num_input_blocks = *num_input_blocks + 1;
    }

    /* Create all output blocks */
    for(i = 0; i < *num_output_blocks; i++)
    {
        status = create_output_block(graph, &output_blocks[i]);
        if(0 != status)
        {
            TIOVX_APPS_ERROR("Cannot create output block\n");
            return status;
        }
    }

    return 0;
}

int32_t run_app(FlowInfo flow_infos[], uint32_t num_flows, CmdArgs *cmd_args)
{
    int32_t status;
    int32_t i, j;

    pthread_t read_img_thread_id[MAX_FLOWS];
    struct read_img_thread_data r_thread_data[MAX_FLOWS];
    uint32_t num_r_threads = 0;

    pthread_t write_img_thread_id[MAX_FLOWS];
    struct write_img_thread_data w_thread_data[MAX_FLOWS];
    uint32_t num_w_threads = 0;

    InputBlock input_blocks[num_flows];
    OutputBlock output_blocks[NUM_OUTPUT_SINKS];

    uint32_t num_input_blocks;
    uint32_t num_output_blocks;

    GraphObj graph;

    BufPool *in_buf_pool = NULL;
    Buf *inbuf = NULL;

#if defined(TARGET_OS_LINUX)
    BufPool *linux_h3a_buf_pool = NULL;
    Buf *linux_h3a_buf = NULL;

    BufPool *linux_aewb_buf_pool = NULL;
    Buf *linux_aewb_buf = NULL;

    Buf *v4l2_valid_bufs[num_flows];
    Buf *v4l2_dq_bufs[num_flows];
#endif

    BufPool *out_buf_pool = NULL;
    Buf *outbuf = NULL;

    BufPool *perf_overlay_buf_pool = NULL;
    Buf *perf_overlay_buf = NULL;

    EdgeAIPerfStats perf_stats_handle;

    /* Capture SIGINT to stop loop */
    signal(SIGINT, interrupt_handler);

    /* Initialize mutex for read image and write image thread */
    for (i = 0; i < MAX_FLOWS; i++)
    {
        pthread_mutex_init(&r_thread_lock[i], NULL);
        pthread_mutex_init(&w_thread_lock[i], NULL);
    }

#if defined(TARGET_OS_LINUX)
    /* Initialize v4l2 buffers to NULL */
    for (i = 0; i < num_flows; i++)
    {
        v4l2_valid_bufs[i] = NULL;
        v4l2_dq_bufs[i] = NULL;
    }
#endif

    /* Initialize the graph */
    status = tiovx_modules_initialize_graph(&graph);
    if(VX_SUCCESS != status)
    {
        TIOVX_APPS_ERROR("Unable to create TIOVX graph\n");
        goto exit;
    }

    /* Set scheduling mode to AUTO*/
    graph.schedule_mode = VX_GRAPH_SCHEDULE_MODE_QUEUE_AUTO;

    /* Connect the nodes in the graph */
    status = connect_blocks(&graph,
                           flow_infos,
                           num_flows,
                           input_blocks,
                           &num_input_blocks,
                           output_blocks,
                           &num_output_blocks);
    if(0 != status)
    {
        TIOVX_APPS_ERROR("Error connecting blocks\n");
        goto clean_graph;
    }

    /* Verify the graph */
    status = tiovx_modules_verify_graph(&graph);
    if (VX_SUCCESS != status)
    {
        TIOVX_APPS_ERROR("Error verifying graph\n");
        goto clean_graph;
    }

    /* Dump graph as a dot file */
    if(cmd_args->dump_dot)
    {
        status = tiovx_modules_export_graph(&graph,
                                            ".",
                                            "edgeai_tiovx_apps");
        if (VX_SUCCESS != status)
        {
            TIOVX_APPS_ERROR("Error exporting graph as dot\n");
            goto clean_graph;
        }
    }

    /* Initialize perf stats */
    initialize_edgeai_perf_stats(&perf_stats_handle);
    perf_stats_handle.overlayType = OVERLAY_TYPE_GRAPH;
    perf_stats_handle.numInstances = 0;
    for(i = 0; i < num_output_blocks; i++)
    {
        if(true == output_blocks[i].output_info->overlay_perf)
        {
            perf_stats_handle.numInstances++;
        }
    }

    /* Enqueue buffers at start based on input sources in all input blocks */
    for(i = 0; i < num_input_blocks; i++)
    {
        in_buf_pool = input_blocks[i].input_pad->buf_pool;

        if (RTOS_CAM == input_blocks[i].input_info->source)
        {
            for (j = 0; j < in_buf_pool->bufq_depth; j++)
            {
                /* Get buffers from buffer pool and enqueue */
                inbuf = tiovx_modules_acquire_buf(in_buf_pool);
                tiovx_modules_enqueue_buf(inbuf);
            }
        }

#if defined(TARGET_OS_LINUX)
        else if (LINUX_CAM == input_blocks[i].input_info->source)
        {
            /* Enqueue h3a and aewb buffers for linux capture */
            linux_h3a_buf_pool = input_blocks[i].v4l2_obj.h3a_pad->buf_pool;
            linux_aewb_buf_pool = input_blocks[i].v4l2_obj.aewb_pad->buf_pool;
            linux_h3a_buf = tiovx_modules_acquire_buf(linux_h3a_buf_pool);
            linux_aewb_buf = tiovx_modules_acquire_buf(linux_aewb_buf_pool);
            tiovx_modules_enqueue_buf(linux_h3a_buf);
            tiovx_modules_enqueue_buf(linux_aewb_buf);

            /* Enqueue buffers to v4l2 capture handle*/
            for (j = 0; j < in_buf_pool->bufq_depth; j++)
            {
                inbuf = tiovx_modules_acquire_buf(in_buf_pool);
                v4l2_capture_enqueue_buf(input_blocks[i].v4l2_obj.v4l2_capture_handle, inbuf);
            }

            /* Start v4l2 capture*/
            v4l2_capture_start(input_blocks[i].v4l2_obj.v4l2_capture_handle);
        }

        else if (VIDEO == input_blocks[i].input_info->source)
        {
            /* Enqueue buffers to v4l2 decode handle*/
            for (j = 0; j < in_buf_pool->bufq_depth; j++)
            {
                inbuf = tiovx_modules_acquire_buf(in_buf_pool);
                v4l2_decode_enqueue_buf(input_blocks[i].v4l2_obj.v4l2_decode_handle, inbuf);
            }

            /* Start v4l2 decode*/
            v4l2_decode_start(input_blocks[i].v4l2_obj.v4l2_decode_handle);

            /* Get buffers from v4l2 handle and enqueue */
            inbuf = v4l2_decode_dqueue_buf(input_blocks[i].v4l2_obj.v4l2_decode_handle);
            tiovx_modules_enqueue_buf(inbuf);
        }
#endif

        else if (RAW_IMG == input_blocks[i].input_info->source)
        {
            inbuf = tiovx_modules_acquire_buf(in_buf_pool);

            /* Create a seperate thread for reading image */
            r_thread_data[i].id = i;
            r_thread_data[i].read_image_buf = inbuf;
            r_thread_data[i].input_info = input_blocks[i].input_info;
            pthread_create(&read_img_thread_id[num_r_threads],
                           NULL,
                           read_img_thread,
                           (void *)&r_thread_data[i]);
            num_r_threads++;

            tiovx_modules_enqueue_buf(inbuf);
        }
    }

    for(i = 0; i < num_output_blocks; i++)
    {
        /* Enqueue buffers for output if output pad is exposed */
        if(NULL != output_blocks[i].output_pad)
        {
            out_buf_pool = output_blocks[i].output_pad->buf_pool;

            if(IMG_DIR == output_blocks[i].output_info->sink)
            {
                outbuf = tiovx_modules_acquire_buf(out_buf_pool);
                tiovx_modules_enqueue_buf(outbuf);
                /* Create a seperate thread for writing image */
                w_thread_data[i].id = i;
                w_thread_data[i].write_image_buf = outbuf;
                w_thread_data[i].output_info = output_blocks[i].output_info;
                pthread_create(&write_img_thread_id[num_w_threads],
                                NULL,
                                write_img_thread,
                                (void *)&w_thread_data[i]);
                num_w_threads++;
            }
#if defined(TARGET_OS_LINUX)
            else if (LINUX_DISPLAY == output_blocks[i].output_info->sink)
            {
                outbuf = tiovx_modules_acquire_buf(out_buf_pool);
                tiovx_modules_enqueue_buf(outbuf);
                for (j = 0; j < out_buf_pool->bufq_depth; j++)
                {
                    kms_display_register_buf(output_blocks[i].kms_obj.kms_display_handle,
                                             &out_buf_pool->bufs[j]);
                }
            }
            else if (H264_ENCODE == output_blocks[i].output_info->sink ||
                     H265_ENCODE == output_blocks[i].output_info->sink)
            {
                for (j = 0; j < out_buf_pool->bufq_depth; j++)
                {
                    outbuf = tiovx_modules_acquire_buf(out_buf_pool);
                    v4l2_encode_enqueue_buf(output_blocks[i].v4l2_obj.v4l2_encode_handle, outbuf);
                }

                /* Start v4l2 encode*/
                v4l2_encode_start(output_blocks[i].v4l2_obj.v4l2_encode_handle);

                /* Get buffers from v4l2 handle and enqueue */
                outbuf = v4l2_encode_dqueue_buf(output_blocks[i].v4l2_obj.v4l2_encode_handle);
                tiovx_modules_enqueue_buf(outbuf);
            }
#endif
        }

        /* Set perf overlay and enqueue */
        if(NULL != output_blocks[i].perf_overlay_pad)
        {
            perf_overlay_buf_pool = output_blocks[i].perf_overlay_pad->buf_pool;
            perf_overlay_buf = tiovx_modules_acquire_buf(perf_overlay_buf_pool);
            update_perf_overlay((vx_image)perf_overlay_buf->handle, &perf_stats_handle);
            tiovx_modules_enqueue_buf(perf_overlay_buf);
        }
    }

#if defined(TARGET_OS_LINUX)
    /* Enqueue half buffer from pool to tiovx graph for v4l2 capture */
    uint8_t buf_cnt = 0;
    while(buf_cnt < 2)
    {
        bool skip = false;

        for(i = 0; i < num_input_blocks; i++)
        {
            if (LINUX_CAM == input_blocks[i].input_info->source)
            {
                /*
                 * Dqueue v4l2 buffers and if not NULL store in valid buffers
                 * which will be used later to enqueue and dqueue in openvx graph
                 */
                v4l2_dq_bufs[i] = v4l2_capture_dqueue_buf(input_blocks[i].v4l2_obj.v4l2_capture_handle);
                if(NULL != v4l2_dq_bufs[i] && NULL ==  v4l2_valid_bufs[i])
                {
                    v4l2_valid_bufs[i] = v4l2_dq_bufs[i];
                    v4l2_dq_bufs[i] = NULL;
                }
            }
        }

        for(i = 0; i < num_input_blocks; i++)
        {
            if (LINUX_CAM == input_blocks[i].input_info->source &&
                NULL == v4l2_valid_bufs[i])
            {
                /*
                 * Skip if any v4l2 buffer is not there
                 */
                skip = true;
                break;
            }
        }

        for(i = 0; i < num_input_blocks; i++)
        {
            if (LINUX_CAM == input_blocks[i].input_info->source)
            {
                /*
                 * If all v4l2 buffers are present, enqueue to openvx graph.
                 */
                if(!skip)
                {
                    tiovx_modules_enqueue_buf(v4l2_valid_bufs[i]);
                    v4l2_valid_bufs[i] = NULL;

                    /* AEWB processing for linux */
                    linux_h3a_buf_pool = input_blocks[i].v4l2_obj.h3a_pad->buf_pool;
                    linux_aewb_buf_pool = input_blocks[i].v4l2_obj.aewb_pad->buf_pool;
                    linux_h3a_buf = tiovx_modules_dequeue_buf(linux_h3a_buf_pool);
                    linux_aewb_buf = tiovx_modules_dequeue_buf(linux_aewb_buf_pool);
                    aewb_process(input_blocks[i].v4l2_obj.aewb_handle, linux_h3a_buf, linux_aewb_buf);
                    tiovx_modules_enqueue_buf(linux_h3a_buf);
                    tiovx_modules_enqueue_buf(linux_aewb_buf);
                }

                /*
                 * Else, enqueue back the dequeue buffer from v4l2 capture
                 */
                if(NULL != v4l2_dq_bufs[i])
                {
                    v4l2_capture_enqueue_buf(input_blocks[i].v4l2_obj.v4l2_capture_handle, v4l2_dq_bufs[i]);
                }
            }
        }

        if(!skip)
        {
            buf_cnt++;
        }

    }

    for (i = 0; i < num_flows; i++)
    {
        v4l2_valid_bufs[i] = NULL;
        v4l2_dq_bufs[i] = NULL;
    }

#endif


    /* Main loop that runs till interrupt */
    while(run_loop)
    {
        bool skip = false;

#if defined(TARGET_OS_LINUX)
        /***********************************************************************
        *                        V4L2 Sources                                  *
        ***********************************************************************/

        for(i = 0; i < num_input_blocks; i++)
        {
            if (LINUX_CAM == input_blocks[i].input_info->source)
            {
                /*
                 * Dqueue v4l2 buffers and if not NULL store in valid buffers
                 * which will be used later to enqueue and dqueue in openvx graph
                 */
                v4l2_dq_bufs[i] = v4l2_capture_dqueue_buf(input_blocks[i].v4l2_obj.v4l2_capture_handle);
                if(NULL != v4l2_dq_bufs[i] && NULL ==  v4l2_valid_bufs[i])
                {
                    v4l2_valid_bufs[i] = v4l2_dq_bufs[i];
                    v4l2_dq_bufs[i] = NULL;
                }
            }
        }

        for(i = 0; i < num_input_blocks; i++)
        {
            if (LINUX_CAM == input_blocks[i].input_info->source &&
                NULL == v4l2_valid_bufs[i])
            {
                /*
                 * Skip if any v4l2 buffer is not there
                 */
                skip = true;
                break;
            }
        }

        for(i = 0; i < num_input_blocks; i++)
        {
            in_buf_pool = input_blocks[i].input_pad->buf_pool;
            if (LINUX_CAM == input_blocks[i].input_info->source)
            {
                /*
                 * If all v4l2 buffers are present, enqueue and dequeue to
                 * openvx graph.
                 */
                if(!skip)
                {
                    tiovx_modules_enqueue_buf(v4l2_valid_bufs[i]);
                    inbuf = tiovx_modules_dequeue_buf(in_buf_pool);
                    v4l2_capture_enqueue_buf(input_blocks[i].v4l2_obj.v4l2_capture_handle, inbuf);
                    v4l2_valid_bufs[i] = NULL;

                    /* AEWB processing for linux */
                    linux_h3a_buf_pool = input_blocks[i].v4l2_obj.h3a_pad->buf_pool;
                    linux_aewb_buf_pool = input_blocks[i].v4l2_obj.aewb_pad->buf_pool;
                    linux_h3a_buf = tiovx_modules_dequeue_buf(linux_h3a_buf_pool);
                    linux_aewb_buf = tiovx_modules_dequeue_buf(linux_aewb_buf_pool);
                    aewb_process(input_blocks[i].v4l2_obj.aewb_handle, linux_h3a_buf, linux_aewb_buf);
                    tiovx_modules_enqueue_buf(linux_h3a_buf);
                    tiovx_modules_enqueue_buf(linux_aewb_buf);
                }

                /*
                 * Else, enqueue back the dequeue buffer from v4l2 capture
                 */
                if(NULL != v4l2_dq_bufs[i])
                {
                    v4l2_capture_enqueue_buf(input_blocks[i].v4l2_obj.v4l2_capture_handle, v4l2_dq_bufs[i]);
                }

            }
        }

        if(skip)
        {
            continue;
        }
#endif

        /***********************************************************************
        *                        Other Sources                                 *
        ***********************************************************************/
        for(i = 0; i < num_input_blocks; i++)
        {
            in_buf_pool = input_blocks[i].input_pad->buf_pool;
            if (RTOS_CAM == input_blocks[i].input_info->source)
            {
                /* Dequeue and the enqueue buffers */
                inbuf = tiovx_modules_dequeue_buf(in_buf_pool);
                tiovx_modules_enqueue_buf(inbuf);
            }

#if defined(TARGET_OS_LINUX)
            else if (VIDEO == input_blocks[i].input_info->source)
            {
                /* Dequeue from graph, enqueue to v4l2 for decode,
                 * then dequeue from v4l2 and enqueue to graph
                 */
                inbuf = tiovx_modules_dequeue_buf(in_buf_pool);
                v4l2_decode_enqueue_buf(input_blocks[i].v4l2_obj.v4l2_decode_handle, inbuf);
                inbuf = v4l2_decode_dqueue_buf(input_blocks[i].v4l2_obj.v4l2_decode_handle);
                if(NULL == inbuf)
                {
                    run_loop = false;
                    skip = true;
                    break;
                }
                tiovx_modules_enqueue_buf(inbuf);
            }
#endif

            else if (RAW_IMG == input_blocks[i].input_info->source)
            {
                /* Dequeue the buffer, send to read image thread for filling
                 * the buffer and the enqueue it
                 */
                inbuf = tiovx_modules_dequeue_buf(in_buf_pool);

                pthread_mutex_lock(&r_thread_lock[i]);
                r_thread_data[i].read_image_buf = inbuf;
                pthread_mutex_unlock(&r_thread_lock[i]);

                tiovx_modules_enqueue_buf(inbuf);
            }
        }

        if(skip)
        {
            break;
        }

        for(i = 0; i < num_output_blocks; i++)
        {
            if(NULL != output_blocks[i].output_pad)
            {
                /* Dequeue and enqueue output buffer if pad is present */
                out_buf_pool = output_blocks[i].output_pad->buf_pool;
                outbuf = tiovx_modules_dequeue_buf(out_buf_pool);
                if (IMG_DIR == output_blocks[i].output_info->sink)
                {
                    /* Update write image thread buffer */
                    pthread_mutex_lock(&w_thread_lock[i]);
                    w_thread_data[i].write_image_buf = outbuf;
                    pthread_mutex_unlock(&w_thread_lock[i]);
                }
#if defined(TARGET_OS_LINUX)
                else if (LINUX_DISPLAY == output_blocks[i].output_info->sink)
                {
                    /* Render the dequeued buffer on kms display */
                    kms_display_render_buf(output_blocks[i].kms_obj.kms_display_handle,
                                           outbuf);
                }
                else if (H264_ENCODE == output_blocks[i].output_info->sink ||
                         H265_ENCODE == output_blocks[i].output_info->sink)
                {
                    v4l2_encode_enqueue_buf(output_blocks[i].v4l2_obj.v4l2_encode_handle, outbuf);
                    outbuf = v4l2_encode_dqueue_buf(output_blocks[i].v4l2_obj.v4l2_encode_handle);
                    if(NULL == outbuf)
                    {
                        run_loop = false;
                        break;
                    }
                }
#endif
                tiovx_modules_enqueue_buf(outbuf);
            }

            /* Dequeue and enqueue perf overlay buffer */
            if(NULL != output_blocks[i].perf_overlay_pad)
            {
                perf_overlay_buf_pool = output_blocks[i].perf_overlay_pad->buf_pool;
                perf_overlay_buf = tiovx_modules_dequeue_buf(perf_overlay_buf_pool);
                update_perf_overlay((vx_image)perf_overlay_buf->handle, &perf_stats_handle);
                if(cmd_args->verbose)
                {
                    print_perf(&graph, &perf_stats_handle);
                }
                tiovx_modules_enqueue_buf(perf_overlay_buf);
            }
        }
    }

    /* Stop all read image threads */
    for (i = 0; i < num_r_threads; i++)
    {
        pthread_join(read_img_thread_id[i], NULL); 
    }

    /* Stop all write image threads */
    for (i = 0; i < num_w_threads; i++)
    {
        pthread_join(write_img_thread_id[i], NULL);
    }

    /* Stop handles */
    for(i = 0; i < num_input_blocks; i++)
    {
        in_buf_pool = input_blocks[i].input_pad->buf_pool;
        /* Why RTOS Cam needs two */
        if (RTOS_CAM == input_blocks[i].input_info->source)
        {
            for(j = 0; j < in_buf_pool->bufq_depth - 2; j++)
            {
                inbuf = tiovx_modules_dequeue_buf(in_buf_pool);
                tiovx_modules_release_buf(inbuf);
            }
        }
#if defined(TARGET_OS_LINUX)
        else if (LINUX_CAM == input_blocks[i].input_info->source)
        {
            v4l2_capture_stop(input_blocks[i].v4l2_obj.v4l2_capture_handle);
            v4l2_capture_delete_handle(input_blocks[i].v4l2_obj.v4l2_capture_handle);
        }
        else if (VIDEO == input_blocks[i].input_info->source)
        {
            v4l2_decode_stop(input_blocks[i].v4l2_obj.v4l2_decode_handle);
            v4l2_decode_delete_handle(input_blocks[i].v4l2_obj.v4l2_decode_handle);
        }
#endif
    }

#if defined(TARGET_OS_LINUX)
    for(i = 0; i < num_output_blocks; i++)
    {
        if (H264_ENCODE == output_blocks[i].output_info->sink ||
            H265_ENCODE == output_blocks[i].output_info->sink)
        {
            v4l2_encode_stop(output_blocks[i].v4l2_obj.v4l2_encode_handle);
            v4l2_encode_delete_handle(output_blocks[i].v4l2_obj.v4l2_encode_handle);
        }
    }
#endif

clean_graph:

    tiovx_modules_clean_graph(&graph);

exit:
    return status;
}
