#include "yaml_parser.h"
#include <yaml-cpp/yaml.h>
#include <filesystem>
#include <string.h>
#include <sstream>

static std::string allowed_input_sources[] = {"RTOS_CAM","LINUX_CAM","RAW_IMG"};

static inline bool ends_with(std::string const &value, std::string const &ending)
{
    if (ending.size() > value.size())
        return false;

    return std::equal(ending.rbegin(), ending.rend(), value.rbegin());
}

int32_t parse_input_node(InputInfo *input_info, const YAML::Node &input_node)
{
    const std::string source = input_node["source"].as<std::string>();

    if("RTOS_CAM" == source)
    {
        input_info->source = RTOS_CAM;
    }
    else if("LINUX_CAM" == source)
    {
        input_info->source = LINUX_CAM;
    }
    else if("VIDEO" == source)
    {
        input_info->source = VIDEO;
    }
    else if("RAW_IMG" == source)
    {
        input_info->source = RAW_IMG;
    }
    else
    {
        TIOVX_APPS_ERROR("Invalid source '%s' specified.\n", source.c_str());
        return -1;
    }

    sprintf(input_info->source_name, source.data());

    input_info->width = input_node["width"].as<uint32_t>();
    input_info->height = input_node["height"].as<uint32_t>();

    /* Set default values. */
    input_info->sensor_name[0] = '\0';
    input_info->channel_mask = 0;
    input_info->device[0] = '\0';
    input_info->subdev[0] = '\0';
    input_info->format[0] = '\0';
    input_info->loop = true;
    input_info->framerate = 1.0;
    input_info->num_raw_img = 0;
    input_info->ldc_enabled = false;
    input_info->num_channels = 1;

    /* Parse necessary information for RTOS_CAM. */
    if (input_info->source == RTOS_CAM)
    {
        /* Parse sensor-name. */
        if (input_node["sensor-name"])
        {
            sprintf(input_info->sensor_name,
                    input_node["sensor-name"].as<std::string>().data());
        }
        else
        {
            TIOVX_APPS_ERROR("Please specify sensor-name for %s.\n",
                   input_info->name);
            return -1;
        }

        /* Parse channel-mask. */
        if (input_node["channel-mask"])
        {
            const std::string channel_mask_str = input_node["channel-mask"].as<std::string>();
            uint32_t channel_mask = std::stoi(channel_mask_str, 0, 2);

            input_info->channel_mask = channel_mask;
            input_info->num_channels = 0;

            while(channel_mask > 0)
            {
                if(channel_mask & 0x1)
                {
                    input_info->num_channels++;
                }
                channel_mask = channel_mask >> 1;
            }
        }
        else
        {
            TIOVX_APPS_ERROR("Please specify channel-mask for %s.\n",
                   input_info->name);
            return -1;
        }
    }

    else if (input_info->source == LINUX_CAM)
    {
        /* Parse sensor-name. */
        if (input_node["sensor-name"])
        {
            sprintf(input_info->sensor_name,
                    input_node["sensor-name"].as<std::string>().data());
        }
        else
        {
            TIOVX_APPS_ERROR("Please specify sensor-name for %s.\n",
                input_info->name);
            return -1;
        }

        /* Parse device. */
        if (input_node["device"])
        {
            sprintf(input_info->device,
                    input_node["device"].as<std::string>().data());
        }
        else
        {
            TIOVX_APPS_ERROR("Please specify device for %s.\n",
                    input_info->name);
            return -1;
        }

        /* Parse subdev. */
        if (input_node["subdev"])
        {
            sprintf(input_info->subdev,
                    input_node["subdev"].as<std::string>().data());
        }
        else
        {
            TIOVX_APPS_ERROR("Please specify subdev for %s.\n",
                    input_info->name);
            return -1;
        }
    }

    else if (input_info->source == VIDEO)
    {
        /* Get video path */
        if (input_node["video_path"])
        {
            std::string video_path = input_node["video_path"].as<std::string>();
            if (!std::filesystem::exists(video_path))
            {
                TIOVX_APPS_ERROR("%s does not exist.\n", video_path.c_str());
                return -1;
            }

            sprintf(input_info->video_path, video_path.data());
        }
        else
        {
            TIOVX_APPS_ERROR("Please specify video_path for %s.\n",
                    input_info->name);
            return -1;
        }
    }

    else if (input_info->source == RAW_IMG)
    {

        if (input_node["framerate"])
        {
            input_info->framerate = input_node["framerate"].as<float>();
        }

        /* Get images */
        if (input_node["image_path"])
        {
            std::string image_path = input_node["image_path"].as<std::string>();
            if (!std::filesystem::exists(image_path))
            {
                TIOVX_APPS_ERROR("%s does not exist.\n", image_path.c_str());
                return -1;
            }

            if(std::filesystem::is_directory(image_path))
            {
                for(const auto &filename : std::filesystem::directory_iterator(image_path))
                {
                    std::string path(filename.path());
                    sprintf(input_info->raw_img_paths[input_info->num_raw_img],
                            path.data());
                    input_info->num_raw_img++;
                }

                char temp[MAX_CHAR_ARRAY_SIZE];
                for(uint32_t i = 1; i < input_info->num_raw_img; i++)
                {
                    for(uint32_t j = 0; j < input_info->num_raw_img-i; j++)
                    {
                        if(strcmp(input_info->raw_img_paths[j],input_info->raw_img_paths[j+1]) > 0)
                        {
                            strcpy(temp, input_info->raw_img_paths[j]);
                            strcpy(input_info->raw_img_paths[j], input_info->raw_img_paths[j+1]);
                            strcpy(input_info->raw_img_paths[j+1], temp);
                        }
                    }
                }
            }
            else
            {
                sprintf(input_info->raw_img_paths[0], image_path.data());
                input_info->num_raw_img = 1;
            }
        }
        else
        {
            TIOVX_APPS_ERROR("Please specify image_path for %s.\n",
                    input_info->name);
            return -1;
        }

        /* Parse format. */
        if (input_node["format"])
        {
            std::string format = input_node["format"].as<std::string>();
            transform(format.begin(), format.end(), format.begin(), ::toupper);
            sprintf(input_info->format,format.data());
        }
        else
        {
            TIOVX_APPS_ERROR("Please specify format for %s.\n",
                    input_info->name);
            return -1;
        }
    }

    /* Parse ldc */
    if (input_node["ldc"])
    {
        input_info->ldc_enabled = input_node["ldc"].as<bool>();
    }

    /* Parse loop. */
    if (input_node["loop"])
    {
        input_info->loop = input_node["loop"].as<bool>();
    }

    return 0;
}

int32_t parse_model_node(ModelInfo *model_info, const YAML::Node &model_node)
{
    const std::string model_path = model_node["model_path"].as<std::string>();

    const std::filesystem::path& f_path = model_path;
    if(!std::filesystem::is_directory(f_path))
    {
        TIOVX_APPS_ERROR("%s is not a directory.\n",model_path.c_str());
        return -1;
    }

    sprintf(model_info->model_path, model_path.data());

    /* Parse params.yaml to get io_confing and network file paths. */
    const std::string params_path = model_path + "/param.yaml";
    if (!std::filesystem::exists(params_path))
    {
        TIOVX_APPS_ERROR("%s does not exist.\n", params_path.c_str());
        return -1;
    }

    const YAML::Node &params_yaml = YAML::LoadFile(params_path);
    const YAML::Node &session_node = params_yaml["session"];
    const YAML::Node &pre_proc_node = params_yaml["preprocess"];
    const YAML::Node &task_node = params_yaml["task_type"];
    const YAML::Node &metric_node = params_yaml["metric"];
    const YAML::Node &post_proc_node = params_yaml["postprocess"];

    if (session_node.IsNull())
    {
        TIOVX_APPS_ERROR("Session configuration parameters missing from param file.\n");
        return -1;
    }
    if (pre_proc_node.IsNull())
    {
        TIOVX_APPS_ERROR("Preprocess configuration parameters missing from param file.\n");
        return -1;
    }
    if (task_node.IsNull())
    {
        TIOVX_APPS_ERROR("Task type missing from param file.\n");
        return -1;
    }
    
    if (!session_node["artifacts_folder"])
    {
        TIOVX_APPS_ERROR("'artifacts_folder' not sepcified in param file.\n");
        return -1;
    }

    std::string artifacts_path = "";
    const std::string &temp_artifacts_path = session_node["artifacts_folder"].as<std::string>();
    std::filesystem::path a_path(temp_artifacts_path);
    if (a_path.is_relative())
    {
        artifacts_path = model_path + "/" + temp_artifacts_path;
    }
    else
    {
        artifacts_path = temp_artifacts_path;
    }

    for(const auto &filename : std::filesystem::directory_iterator(artifacts_path))
    {
        std::string x(filename.path());
        if(ends_with(x, "_tidl_io_1.bin"))
        {
            sprintf(model_info->io_config_path, x.data());
        }

        if(ends_with(x, "_tidl_net.bin"))
        {
            sprintf(model_info->network_path, x.data());
        }
    }

    if (!std::filesystem::exists(model_info->io_config_path))
    {
        TIOVX_APPS_ERROR("%s does not exist\n",model_info->io_config_path);
        return -1;
    }

    if (!std::filesystem::exists(model_info->network_path))
    {
        TIOVX_APPS_ERROR("%s does not exist\n",model_info->network_path);
        return -1;
    }

    /* Parse param.yaml file to get pre proc information. */
    if (!pre_proc_node["data_layout"])
    {
        TIOVX_APPS_ERROR("data layout specification missing from param file.\n");
        return -1;
    }
    else if (!pre_proc_node["resize"])
    {
        TIOVX_APPS_ERROR("resize specification missing from param file.\n");
        return -1;
    }
    else if (!pre_proc_node["crop"])
    {
        TIOVX_APPS_ERROR("crop specification missing from param file.\n");
        return -1;
    }

    if(pre_proc_node["reverse_channels"])
    {
        model_info->pre_proc_info.tensor_format = pre_proc_node["reverse_channels"].as<bool>();
    }
    else
    {
        model_info->pre_proc_info.tensor_format = 0;
    }

    
    model_info->pre_proc_info.mean[0] = 0.0;
    model_info->pre_proc_info.mean[1] = 0.0;
    model_info->pre_proc_info.mean[2] = 0.0;

    model_info->pre_proc_info.scale[0] = 1.0;
    model_info->pre_proc_info.scale[1] = 1.0;
    model_info->pre_proc_info.scale[2] = 1.0;
    
    const YAML::Node &mean_node = session_node["input_mean"];
    const YAML::Node &scale_node = session_node["input_scale"];

    if (!mean_node.IsNull())
    {
        for (uint32_t i = 0; i < mean_node.size(); i++)
        {
            model_info->pre_proc_info.mean[i] = mean_node[i].as<float>();
        }
    }
    if (!scale_node.IsNull())
    {
        for (uint32_t i = 0; i < scale_node.size(); i++)
        {
            model_info->pre_proc_info.scale[i] = scale_node[i].as<float>();
        }
    }

    const YAML::Node &crop_node = pre_proc_node["crop"];
    const YAML::Node &resize_node = pre_proc_node["resize"];

    if (crop_node.Type() == YAML::NodeType::Sequence)
    {
        model_info->pre_proc_info.crop_height = crop_node[0].as<uint32_t>();
        model_info->pre_proc_info.crop_width = crop_node[1].as<uint32_t>();
    }
    else if (crop_node.Type() == YAML::NodeType::Scalar)
    {
        model_info->pre_proc_info.crop_height = crop_node.as<uint32_t>();
        model_info->pre_proc_info.crop_width = crop_node.as<uint32_t>();
    }

    if (resize_node.Type() == YAML::NodeType::Sequence)
    {
        model_info->pre_proc_info.resize_height = resize_node[0].as<uint32_t>();
        model_info->pre_proc_info.resize_width  = resize_node[1].as<uint32_t>();
    }
    else if (resize_node.Type() == YAML::NodeType::Scalar)
    {
        model_info->pre_proc_info.resize_height = resize_node.as<uint32_t>();
        model_info->pre_proc_info.resize_width  = resize_node.as<uint32_t>();
    }

    /* Parse param.yaml to get post proc information */
    model_info->post_proc_info.formatter[0] = 0;
    model_info->post_proc_info.formatter[1] = 1;
    model_info->post_proc_info.formatter[2] = 2;
    model_info->post_proc_info.formatter[3] = 3;
    model_info->post_proc_info.formatter[4] = 4;
    model_info->post_proc_info.formatter[5] = 5;

    model_info->post_proc_info.norm_detect = false;

    if (post_proc_node)
    {
        if(post_proc_node["formatter"] && post_proc_node["formatter"]["src_indices"])
        {
            const YAML::Node &formatter_node = post_proc_node["formatter"]["src_indices"];

            if (formatter_node.size() == 2)
            {
                model_info->post_proc_info.formatter[4] = formatter_node[0].as<int32_t>();
                model_info->post_proc_info.formatter[5] = formatter_node[1].as<int32_t>();
            }
            else if ( formatter_node.size() == 4 || formatter_node.size() == 6)
            {
                for (uint8_t i = 0; i < formatter_node.size(); i++)
                {
                    model_info->post_proc_info.formatter[i] = formatter_node[i].as<int32_t>();
                }
            }
            else
            {
                TIOVX_APPS_ERROR("formatter specification incorrect  in param file.\n");
                return -1;
            }
        }

        if (post_proc_node["normalized_detections"])
        {
            model_info->post_proc_info.norm_detect = post_proc_node["normalized_detections"].as<bool>();
        }
    }

    sprintf(model_info->post_proc_info.task_type, task_node.as<std::string>().data());

    if (model_node["topN"])
    {
        model_info->post_proc_info.top_n = model_node["topN"].as<uint32_t>();
    }
    else
    {
        model_info->post_proc_info.top_n = 5;
    }

    if (model_node["viz_threshold"])
    {
        model_info->post_proc_info.viz_threshold = model_node["viz_threshold"].as<float>();
    }
    else
    {
       model_info->post_proc_info.viz_threshold = 0.6;
    }

    if (model_node["alpha"])
    {
        model_info->post_proc_info.alpha = model_node["alpha"].as<float>();
    }
    else
    {
        model_info->post_proc_info.alpha = 0.5;
    }


    return 0;
}

int32_t parse_output_node(OutputInfo *output_info, const YAML::Node &output_node)
{
    const std::string sink = output_node["sink"].as<std::string>();

    if("RTOS_DISPLAY" == sink)
    {
        output_info->sink = RTOS_DISPLAY;
    }
    else if("LINUX_DISPLAY" == sink)
    {
        output_info->sink = LINUX_DISPLAY;
    }
    else if("H264_ENCODE" == sink)
    {
        output_info->sink = H264_ENCODE;
    }
    else if("H265_ENCODE" == sink)
    {
        output_info->sink = H265_ENCODE;
    }
    else if("IMG_DIR" == sink)
    {
        output_info->sink = IMG_DIR;
    }
    else
    {
        TIOVX_APPS_ERROR("Invalid sink '%s' specified.\n", sink.c_str());
        return -1;
    }

    sprintf(output_info->sink_name, sink.data());

    output_info->width = output_node["width"].as<uint32_t>();

    output_info->height = output_node["height"].as<uint32_t>();

    output_info->crtc = 0;

    output_info->connector = 0;
    
    output_info->overlay_perf = true;

    if(output_node["crtc"])
    {
        output_info->crtc =  output_node["crtc"].as<uint32_t>();
    }

    if(output_node["connector"])
    {
        output_info->connector =  output_node["connector"].as<uint32_t>();
    }

    if(output_node["overlay-perf"])
    {
        output_info->overlay_perf =  output_node["overlay-perf"].as<bool>();
    }

    if(H264_ENCODE == output_info->sink || H265_ENCODE == output_info->sink )
    {
        if(output_node["output_path"])
        {
            std::string output_path = output_node["output_path"].as<std::string>();
            sprintf(output_info->output_path, output_path.data());
        }
        else
        {
            TIOVX_APPS_ERROR("Please specify 'output_path' for %s.\n",
                             output_info->name);
            return -1;
        }
    }

    else if(IMG_DIR == output_info->sink)
    {
        if(output_node["output_path"])
        {
            std::string output_path = output_node["output_path"].as<std::string>();
            if(!std::filesystem::exists(output_path))
            {
                std::filesystem::create_directory(output_path);
                if(!std::filesystem::exists(output_path))
                {
                    TIOVX_APPS_ERROR("Error creating directory %s.\n",
                                     output_path.c_str());
                    return -1;
                }
            }
            sprintf(output_info->output_path, output_path.data());
        }
        else
        {
            TIOVX_APPS_ERROR("Please specify 'output_path' for %s.\n",
                             output_info->name);
            return -1;
        }

        if(output_node["framerate"])
        {
            output_info->framerate = output_node["framerate"].as<float>();
        }
        else
        {
            output_info->framerate = 1.0;
        }
    }

    return 0;
}

int32_t parse_mosaic_node(SubflowInfo *subflow_info, const YAML::Node &mosaic_node)
{
    uint32_t j;
    uint32_t max_mosaic_width = 0, max_mosaic_height = 0;
    uint32_t pos_x, pos_y, width, height;
    std::vector<std::vector<uint32_t>> mosaic_datas;

    uint32_t num_mosaic_info = 0;

    mosaic_datas = mosaic_node.as<std::vector<std::vector<uint32_t>>>();
    for (j = 0; j < mosaic_datas.size(); j++)
    {
        pos_x = mosaic_datas[j][0];
        pos_y = mosaic_datas[j][1];
        width = mosaic_datas[j][2];
        height = mosaic_datas[j][3];

        /* Verify mosaic info. */
        if((pos_x + width) > subflow_info->output_info.width)
        {
            TIOVX_APPS_ERROR("Mosaic (pos_x + width) > output_width\n");
            return -1;
        }

        if((pos_y + height) > subflow_info->output_info.height)
        {
            TIOVX_APPS_ERROR("Mosaic (pos_y + height) > output_height\n");
            return -1;
        }

        subflow_info->mosaic_info[j].pos_x = pos_x;
        subflow_info->mosaic_info[j].pos_y = pos_y;
        subflow_info->mosaic_info[j].width = width;
        subflow_info->mosaic_info[j].height = height;

        num_mosaic_info++;

        if(mosaic_datas[j][2] > max_mosaic_width)
        {
            max_mosaic_width = mosaic_datas[j][2];
        }
        if(mosaic_datas[j][3] > max_mosaic_height)
        {
            max_mosaic_height = mosaic_datas[j][3];
        }
    }

    subflow_info->max_mosaic_width = max_mosaic_width;
    subflow_info->max_mosaic_height = max_mosaic_height;

    /* Check if number of mosaic matches the number of channels */
    if (subflow_info->num_channels > num_mosaic_info)
    {
        TIOVX_APPS_ERROR("Mosaic Info provided is less than"
                         "number of channels of the input\n");
        return -1;
    }

    return 0;
}

int32_t parse_yaml_file(char       *input_filename,
                        FlowInfo   flow_infos[],
                        uint32_t   max_flows,
                        uint32_t   *num_flows)
{
    int32_t status = 0;
    uint32_t i = 0;

    std::string unique_inputs[100];
    uint32_t    num_unique_inputs = 0;

    if (!std::filesystem::exists(input_filename))
    {
        TIOVX_APPS_ERROR("%s does not exist.\n", input_filename);
        return -1;
    }

    const YAML::Node &yaml = YAML::LoadFile(input_filename);

    /* Get all unique inputs to be used. */
    for (auto &n : yaml["flows"])
    {
        const YAML::Node pipeline_node = n.second["pipeline"];
        const std::string &input_name = pipeline_node[0].as<std::string>();
        if (std::find(std::begin(unique_inputs),std::end(unique_inputs),input_name)
            == std::end(unique_inputs))
        {
            unique_inputs[num_unique_inputs] = input_name;
            num_unique_inputs++;
        }
    }

    for(i = 0; i < num_unique_inputs; i++)
    {
        const std::string &input_name = unique_inputs[i];
        if (!yaml["inputs"][input_name])
        {
            TIOVX_APPS_ERROR("Invalid input '%s' specified.\n", input_name.c_str());
            status = -1;
            return status;
        }

        /* Populate InputInfo*/
        sprintf(flow_infos[i].input_info.name, input_name.data());

        status = parse_input_node(&flow_infos[i].input_info,
                                  yaml["inputs"][input_name]);
        if (0 != status)
        {
            return status;
        }

        for (auto &n : yaml["flows"])
        {
            const YAML::Node pipeline_node = n.second["pipeline"];

            if (input_name != pipeline_node[0].as<std::string>())
            {
                continue;
            }

            uint32_t s_idx = flow_infos[i].num_subflows;

            flow_infos[i].subflow_infos[s_idx].num_channels = \
                                          flow_infos[i].input_info.num_channels;

            /* Populate ModelInfo if present. */
            if (pipeline_node[1].IsNull())
            {
                flow_infos[i].subflow_infos[s_idx].has_model = false;
            }
            else
            {
                flow_infos[i].subflow_infos[s_idx].has_model = true;
                const std::string &model_name = pipeline_node[1].as<std::string>();
                if (!yaml["models"][model_name])
                {
                    TIOVX_APPS_ERROR("Invalid model '%s' specified.\n", model_name.c_str());
                    status = -1;
                    return status;
                }

                sprintf(flow_infos[i].subflow_infos[s_idx].model_info.name,
                        model_name.data());

                status = parse_model_node(&flow_infos[i].subflow_infos[s_idx].model_info,
                                        yaml["models"][model_name]);
                if (0 != status)
                {
                    return status;
                }
            }

            /* Populate OutputInfo. */
            const std::string &output_name = pipeline_node[2].as<std::string>();
            if (!yaml["outputs"][output_name])
            {
                TIOVX_APPS_ERROR("Invalid output '%s' specified.\n", output_name.c_str());
                status = -1;
                return status;
            }

            sprintf(flow_infos[i].subflow_infos[s_idx].output_info.name,
                    output_name.data());

            if(yaml["title"])
            {
                sprintf(flow_infos[i].subflow_infos[s_idx].output_info.title,
                        yaml["title"].as<std::string>().data());
            }
            else
            {
                sprintf(flow_infos[i].subflow_infos[s_idx].output_info.title,
                        "No title");
            }

            status = parse_output_node(&flow_infos[i].subflow_infos[s_idx].output_info,
                                       yaml["outputs"][output_name]);
            if (0 != status)
            {
                return status;
            }

            /* Verify valid output for given input */
            if(flow_infos[i].input_info.source == RTOS_CAM &&
               flow_infos[i].subflow_infos[s_idx].output_info.sink == LINUX_DISPLAY)
            {
                TIOVX_APPS_ERROR("RTOS_CAM cannot have LINUX_DISPLAY as output\n");
                status = -1;
                return status;
            }

            if(flow_infos[i].input_info.source == LINUX_CAM &&
               flow_infos[i].subflow_infos[s_idx].output_info.sink == RTOS_DISPLAY)
            {
                TIOVX_APPS_ERROR("LINUX_CAM cannot have RTOS_DISPLAY as output\n");
                status = -1;
                return status;
            }

            /* Populate MosaicInfos */
            if(n.second["mosaic"])
            {
                status = parse_mosaic_node(&flow_infos[i].subflow_infos[s_idx],
                                        n.second["mosaic"]);
                if (0 != status)
                {
                    return status;
                }
            }
            else
            {
                flow_infos[i].subflow_infos[s_idx].mosaic_info[0].pos_x = 0;
                flow_infos[i].subflow_infos[s_idx].mosaic_info[0].pos_y = 0;
                flow_infos[i].subflow_infos[s_idx].mosaic_info[0].width = flow_infos[i].subflow_infos[s_idx].output_info.width;
                flow_infos[i].subflow_infos[s_idx].mosaic_info[0].height = flow_infos[i].subflow_infos[s_idx].output_info.height;
                flow_infos[i].subflow_infos[s_idx].max_mosaic_width =  flow_infos[i].subflow_infos[s_idx].output_info.width;
                flow_infos[i].subflow_infos[s_idx].max_mosaic_height =  flow_infos[i].subflow_infos[s_idx].output_info.height;
                if (flow_infos[i].subflow_infos[s_idx].num_channels > 1)
                {
                    TIOVX_APPS_ERROR("Please provide mosaic information if "
                                     "number of channels of the input > 1\n");
                    return -1;
                }
            }
            
            flow_infos[i].num_subflows++;
        }
    }

    *num_flows = num_unique_inputs;

    return status;
}

void dump_data(FlowInfo flow_infos[], uint32_t num_flows)
{
    uint32_t i, j, k;

    for(i = 0; i < num_flows; i++)
    {
        TIOVX_APPS_PRINTF("-FLOW%d\n",i);
        TIOVX_APPS_PRINTF("\tinput: %s %s\n",
               flow_infos[i].input_info.name,
               flow_infos[i].input_info.source_name);
        TIOVX_APPS_PRINTF("\twidth: %u\n",flow_infos[i].input_info.width);
        TIOVX_APPS_PRINTF("\theight: %u\n",flow_infos[i].input_info.height);
        TIOVX_APPS_PRINTF("\tloop: %d\n",flow_infos[i].input_info.loop);
        TIOVX_APPS_PRINTF("\tformat: %s\n",flow_infos[i].input_info.format);
        TIOVX_APPS_PRINTF("\tsensor_name: %s\n",flow_infos[i].input_info.sensor_name);
        TIOVX_APPS_PRINTF("\tchannel_mask: %u\n",flow_infos[i].input_info.channel_mask);
        TIOVX_APPS_PRINTF("\tldc_enabled: %d\n",flow_infos[i].input_info.ldc_enabled);
        TIOVX_APPS_PRINTF("\tdevice: %s\n",flow_infos[i].input_info.device);
        TIOVX_APPS_PRINTF("\tnum_subflows: %u\n",flow_infos[i].num_subflows);
        for(j = 0; j < flow_infos[i].num_subflows; j++)
        {
            TIOVX_APPS_PRINTF("\tSUBFLOW-%d\n",j);
            TIOVX_APPS_PRINTF("\t\tmodel (io_config): %s\n",
                   flow_infos[i].subflow_infos[j].model_info.io_config_path);
            TIOVX_APPS_PRINTF("\t\tmodel (network): %s\n",
                   flow_infos[i].subflow_infos[j].model_info.network_path);
            TIOVX_APPS_PRINTF("\t\toutput: %s %s\n",
                   flow_infos[i].subflow_infos[j].output_info.name,
                   flow_infos[i].subflow_infos[j].output_info.sink_name);
            TIOVX_APPS_PRINTF("\t\tmosaic:");
            for (k = 0; k < flow_infos[i].subflow_infos[j].num_channels; k++)
            {
                TIOVX_APPS_PRINTF(" (%u, %u, %u, %u)",
                       flow_infos[i].subflow_infos[j].mosaic_info[k].pos_x,
                       flow_infos[i].subflow_infos[j].mosaic_info[k].pos_y,
                       flow_infos[i].subflow_infos[j].mosaic_info[k].width,
                       flow_infos[i].subflow_infos[j].mosaic_info[k].height);
            }
            TIOVX_APPS_PRINTF("\n");

        }
        TIOVX_APPS_PRINTF("\n");
    }
}

int32_t get_classname(char *model_path,
                      char (*classnames)[TIVX_DL_POST_PROC_MAX_SIZE_CLASSNAME],
                      size_t max_classname)
{
    const std::string &dataset_path = std::string(model_path) + "/dataset.yaml";

    if (!std::filesystem::exists(dataset_path))
    {
        TIOVX_APPS_ERROR("%s does not exist\n",dataset_path.c_str());
        return -1;
    }

    for (uint32_t i = 0; i < max_classname; i++)
    {
        sprintf(classnames[i], "Unknown");
    }

    const YAML::Node dataset_yaml = YAML::LoadFile(dataset_path.c_str());
    const YAML::Node &categories_node = dataset_yaml["categories"];
    if (!categories_node)
    {
        TIOVX_APPS_ERROR("Parameter categories missing from dataset file.\n");
        return -1;
    }

    for (YAML::Node data : categories_node)
    {
        int32_t id = data["id"].as<int32_t>();
        std::string name = data["name"].as<std::string>();

        if (data["supercategory"])
        {
            name = data["supercategory"].as<std::string>() + "/" + name;
        }

        if(id < (int32_t) max_classname)
        {
            snprintf(classnames[id],
                     TIVX_DL_POST_PROC_MAX_SIZE_CLASSNAME,
                     name.data());
        }
    }

    return 0;
}

int32_t get_label_offset(char *model_path,
                         int32_t label_offset[],
                         int32_t *label_index_offset)
{
    const std::string params_path = std::string(model_path) + "/param.yaml";

    if (!std::filesystem::exists(params_path))
    {
        TIOVX_APPS_ERROR("%s does not exist.\n", params_path.c_str());
        return -1;
    }

    const YAML::Node &params_yaml = YAML::LoadFile(params_path);
    const YAML::Node &metric_node = params_yaml["metric"];

    if (metric_node && metric_node["label_offset_pred"])
    {
        const YAML::Node &offset_node = metric_node["label_offset_pred"];

        if (offset_node.Type() == YAML::NodeType::Scalar)
        {
            label_offset[0] = offset_node.as<int32_t>();
        }
        else if (offset_node.Type() == YAML::NodeType::Map)
        {
            bool first_iter = true;
            int32_t cnt = 0;
            for (const auto& it : offset_node)
            {
                if (it.second.Type() == YAML::NodeType::Scalar)
                {
                    if(first_iter)
                    {
                        *label_index_offset = it.first.as<int32_t>();
                        first_iter = false;
                    }
                    label_offset[cnt++] = it.second.as<int32_t>();
                }
            }
        }
        else
        {
            TIOVX_APPS_ERROR("label_offset_pred specification incorrect in param file.\n");
            return -1;
        }
    }

    return 0;
}