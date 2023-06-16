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

/* Standard headers. */
#include <filesystem>
#include <iostream>
#include <unistd.h>

/* Module headers. */
#include <common/include/edgeai_utils.h>
#include <common/include/edgeai_demo_config.h>
#include <common/include/edgeai_demo.h>
#include <common/include/edgeai_ovx_wrapper.h>
#include <common/include/edgeai_tidl.h>
#include <common/include/edgeai_post_proc.h>
#include <common/include/edgeai_pre_proc.h>
#include <common/include/edgeai_msc.h>
#include <common/include/edgeai_mosaic.h>

/* OpenVX headers */
#include <tiovx_utils.h>
#include <tiovx_display_module.h>

/**
 * \defgroup group_edgeai_common Master demo code
 *
 * \brief Common code used across all EdgeAI applications.
 *
 * \ingroup group_edgeai_cpp_apps
 */

namespace ti::edgeai::common
{

class EdgeAIDemoImpl
{
    public:
        /** Constructor.
         * @param yaml Parsed yaml data to extract the parameters from.
         */
        EdgeAIDemoImpl(const YAML::Node    &yaml);

        /**
        * Dumps openVX graph as dot file
        */
        void dumpGraphAsDot();

        /**
         * Starts the demo
         */
        int32_t startDemo();

        /**
         * Sets a flag for the threads to exit at the next opportunity.
         * This controls the stopping all threads, stop the openVX graph
         * and other cleanup.
         */
        void sendExitSignal();

        /**
         * This is a blocking call that waits for all the internal threads to exit.
         */
        void waitForExit();

        /** Destructor. */
        ~EdgeAIDemoImpl();

    private:
        /** Setup the data flows. */
        int32_t setupFlows();

        /**
         * Assignment operator.
         *
         * Assignment is not required and allowed and hence prevent
         * the compiler from generating a default assignment operator.
         */
        EdgeAIDemoImpl & operator=(const EdgeAIDemoImpl& rhs) = delete;

    private:
        /** OpenVX graph object. */
        ovxGraph                            *m_ovxGraph{nullptr};

        /** Vector of pre process objects */
        vector<preProc*>                    m_preProcObjs{};

        /** Vector of TIDL objects */
        vector<tidlInf*>                    m_tidlInfObjs{};
        
        /** Vector of post process objects */
        vector<postProc*>                   m_postProcObjs{};

        /** Vector of multiscaler objects */
        vector<multiScaler*>                m_multiScalerObjs{};

        /** Vector of mosaic objects */
        vector<imgMosaic*>                  m_imgMosaicObjs{};

        /** Module display object */
        TIOVXDisplayModuleObj               *m_displayObj{NULL};

        /** Flag to store which mosaic that goes to display */
        uint                                m_dispMosaicIdx{0};

        /** Demo configuration. */
        DemoConfig                          m_config;

        /* Runs the graph in loop */
        bool                                m_runLoop{true};

};

EdgeAIDemoImpl::EdgeAIDemoImpl(const YAML::Node    &yaml)
{
    int32_t status;

    /* Parse the configuration information and build the objects. */
    status = m_config.parse(yaml);

    if (status == 0)
    {
        /* Setup the flows and run the graph. */
        status = setupFlows();
    }

    if (status < 0)
    {
        throw runtime_error("EdgeAIDemoImpl object creation failed.");
    }
    
}

int32_t EdgeAIDemoImpl::startDemo()
{
    /*
        RUN GRAPH
    */
    int status = 0;
    uint32_t num_refs;
    vx_image input_o, output_o;

    /*  cnt is the counter which gets appended to the output filename.
        Especially used for processing multiple images from a directory. */
    uint32_t cnt = 0;

    while(m_runLoop)
    {
        /*  msc_cnt will be a counter to use the appropriate MSC objects
            across all the flows and its sub flows */
        int msc_cnt = 0;
        for (auto &[name,flow] : m_config.m_flowMap)
        {
            auto const &input = m_config.m_inputMap[flow->m_inputId];

            for(uint i = 0; i < flow->m_subFlowConfigs.size(); i++)
            {
                if(input->m_srcType == "image")
                {
                    readImage(const_cast<char*>(input->m_source.c_str()),
                        m_multiScalerObjs[msc_cnt]->multiScalerObj1.input.image_handle[0]);
                    m_runLoop = input->m_loop;
                }
                else if(input->m_srcType == "images_directory")
                {
                    readImage(const_cast<char*>(input->m_multiImageVect[input->m_multiImageVectCnt].c_str()),
                        m_multiScalerObjs[msc_cnt]->multiScalerObj1.input.image_handle[0]);
                }
                msc_cnt++;
            }
            if(input->m_srcType == "images_directory")
            {
                input->m_multiImageVectCnt++;
                if (input->m_multiImageVectCnt == input->m_multiImageVect.size())
                {
                    input->m_multiImageVectCnt = 0;
                    m_runLoop = input->m_loop;
                }
            }
        }
        
        /*  vxGraphParameterEnqueueReadyRef */
        for(uint i = 0; i < m_multiScalerObjs.size(); i++)
        {
            vxGraphParameterEnqueueReadyRef(m_ovxGraph->graph,
                    m_multiScalerObjs[i]->multiScalerObj1.input.graph_parameter_index,
                    (vx_reference*)&m_multiScalerObjs[i]->multiScalerObj1.input.image_handle[0], 1);
        }
        for(uint i = 0; i < m_imgMosaicObjs.size(); i++)
        {
            if(i == m_dispMosaicIdx && m_displayObj != NULL)
            {
                continue;
            }

            vxGraphParameterEnqueueReadyRef(m_ovxGraph->graph,
                    m_imgMosaicObjs[i]->imgMosaicObj.output_graph_parameter_index,
                    (vx_reference*)&m_imgMosaicObjs[i]->imgMosaicObj.output_image[0], 1);
        }

        /*
            vxScheduleGraph
            vxWaitGraph
        */
        status = vxScheduleGraph(m_ovxGraph->graph);
        if((vx_status)VX_SUCCESS != status) {
            LOG_ERROR("Schedule Graph failed: %d!\n", status);
            exit(-1);
        }
        status = vxWaitGraph(m_ovxGraph->graph);
        if((vx_status)VX_SUCCESS != status) {
            LOG_ERROR("Wait Graph failed: %d!\n", status);
            exit(-1);
        }

        /*  vxGraphParameterDequeueDoneRef  */
        for(uint i = 0; i < m_multiScalerObjs.size(); i++)
        {
            vxGraphParameterDequeueDoneRef(m_ovxGraph->graph,
                                m_multiScalerObjs[i]->multiScalerObj1.input.graph_parameter_index,
                                (vx_reference*)&input_o, 1, &num_refs);
        }
        for(uint i = 0; i < m_imgMosaicObjs.size(); i++)
        {
            if(i == m_dispMosaicIdx && m_displayObj != NULL)
            {
                continue;
            }

            vxGraphParameterDequeueDoneRef(m_ovxGraph->graph,
                                m_imgMosaicObjs[i]->imgMosaicObj.output_graph_parameter_index,
                                (vx_reference*)&output_o, 1, &num_refs);
        }

        /*  Write Output to file    */

        /*  out_cnt will be a counter to use the appropriate post_proc objects
            across all the flows and its sub flows */

        for(uint i = 0; i < m_imgMosaicObjs.size(); i++)
        {
            string sink = "";
            string sink_type = "";
            for (auto &[name,flow] : m_config.m_flowMap)
            {
                auto const &outputIds = flow->m_outputIds;
                for(uint j=0; j < outputIds.size(); j++)
                {
                    OutputInfo  *output = m_config.m_outputMap[outputIds[j]];
                    if ((uint)output->m_instId == i)
                    {
                        sink = output->m_sink;
                        sink_type = output->m_sinkType;
                        break;
                    }
                }
                if (sink != "")
                {
                    break;
                }
            }

            if(sink_type == "image")
            {
                writeImage(const_cast<char*>(sink.c_str()), m_imgMosaicObjs[i]->imgMosaicObj.output_image[0]);
            }
            else if(sink_type == "images_directory")
            {
                string mosaicId = "mosaic" + to_string(i);
                string output_filename = sink + "/output_image_" + mosaicId + "_" + to_string(cnt) + ".nv12";
                writeImage(const_cast<char*>(output_filename.c_str()), m_imgMosaicObjs[i]->imgMosaicObj.output_image[0]);
            }
        }

        /* Increment cnt */
        cnt++;

        sleep(1);
    }

    return status;
}

int32_t EdgeAIDemoImpl::setupFlows()
{
    int32_t status = 0;

    vx_int32 graph_parameter_index = 0;
    vx_graph_parameter_queue_params_t graph_parameters_queue_params_list[16];

    vector<vector<vector<int32_t>>> commonMosaicInfo{};
    vector<vector<int32_t>>         postProcMosaicConn{};

    for(int i = 0; i < OutputInfo::m_numInstances; i++)
    {
        commonMosaicInfo.push_back({});
        postProcMosaicConn.push_back({});
    }

    /* Create graph structure */
    m_ovxGraph  = new ovxGraph;

    /* CREATE GRAPH */
    m_ovxGraph->graph = vxCreateGraph(m_ovxGraph->context);
    status = vxGetStatus((vx_reference)m_ovxGraph->graph);

    if(status != VX_SUCCESS)
    {
        throw runtime_error("Graph Creation failed \n");
    }

    for (auto const &[name,flow] : m_config.m_flowMap)
    {
        flow->initialize(m_config.m_modelMap,
                         m_config.m_inputMap,
                         m_config.m_outputMap);

        auto const &modelIds    = flow->m_modelIds;
        auto const &outputIds   = flow->m_outputIds;
        auto const &input       = m_config.m_inputMap[flow->m_inputId];

        if(modelIds.size() != outputIds.size())
        {
            throw runtime_error("Size of modelIds is not equal to size of outputIds \n");
        }
        
        /* modelIds is a vector of all models used for a unique input */
        /* outputIds is a vector of all outputs used for a unique input */
        for(uint i=0; i < modelIds.size(); i++)
        {
            preProc         *pre_proc_obj     = new preProc;
            tidlInf         *tidl_inf_obj     = new tidlInf;
            postProc        *post_proc_obj    = new postProc;
            multiScaler     *multi_scaler_obj = new multiScaler;
            
            ModelInfo   *model = m_config.m_modelMap[modelIds[i]];
            OutputInfo  *output = m_config.m_outputMap[outputIds[i]];

            /* Get Configuration of TIDL module by parsing params.yaml  */
            tidl_inf_obj->getConfig(model->m_modelPath, m_ovxGraph->context);
            status = tiovx_tidl_module_init(m_ovxGraph->context, &tidl_inf_obj->tidlObj, const_cast<vx_char*>(string("tidl_obj").c_str()));

            m_tidlInfObjs.push_back(tidl_inf_obj);
            
            /* Get Configuration of Pre Process module by parsing params.yaml  */
            pre_proc_obj->getConfig(model->m_modelPath, tidl_inf_obj->ioBufDesc);
            status = tiovx_dl_pre_proc_module_init(m_ovxGraph->context, 
                                                &pre_proc_obj->dlPreProcObj);

            m_preProcObjs.push_back(pre_proc_obj);

            /* Get Configuration of Post Process Module by parsing params.yaml  */
            /* flow->m_mosaicInfoVec[i][0] is width of the output */
            /* flow->m_mosaicInfoVec[i][0] is height of the output */
            post_proc_obj->getConfig(model->m_modelPath, tidl_inf_obj->ioBufDesc, flow->m_mosaicInfoVec[i][2], flow->m_mosaicInfoVec[i][3]);
            post_proc_obj->dlPostProcObj.params.oc_prms.num_top_results = model->m_topN;
            post_proc_obj->dlPostProcObj.params.od_prms.viz_th = model->m_vizThreshold;
            post_proc_obj->dlPostProcObj.params.ss_prms.alpha = model->m_alpha;
            status = tiovx_dl_post_proc_module_init(m_ovxGraph->context, &post_proc_obj->dlPostProcObj);

            m_postProcObjs.push_back(post_proc_obj);

            /* Get Configuration of MSC module by parsing params.yaml  */
            multi_scaler_obj->getConfig(input->m_width, input->m_height, flow->m_mosaicInfoVec[i][2], flow->m_mosaicInfoVec[i][3], pre_proc_obj);
            status = tiovx_multi_scaler_module_init(m_ovxGraph->context, &multi_scaler_obj->multiScalerObj1);
            if (multi_scaler_obj->useSecondaryMsc)
            {
                status = tiovx_multi_scaler_module_init(m_ovxGraph->context, &multi_scaler_obj->multiScalerObj2);
            }

            m_multiScalerObjs.push_back(multi_scaler_obj);


            /* Mosaic */
            commonMosaicInfo[output->m_instId].push_back(flow->m_mosaicInfoVec[i]);
            postProcMosaicConn[output->m_instId].push_back(m_postProcObjs.size() - 1);

            if(output->m_sinkType == "display" && m_displayObj == NULL)
            {
                m_displayObj = new TIOVXDisplayModuleObj;
                tiovx_display_module_params_init(m_displayObj);
                m_displayObj->input_width = output->m_width;
                m_displayObj->input_height = output->m_height;
                m_displayObj->input_color_format = VX_DF_IMAGE_NV12;

                m_displayObj->params.outWidth = output->m_width;
                m_displayObj->params.outHeight = output->m_height;
                m_displayObj->params.posX = (1920-output->m_width)/2;
                m_displayObj->params.posY = (1080-output->m_height)/2;

                tiovx_display_module_init(m_ovxGraph->context, m_displayObj);

                /* Save the inst Id of output which goes to display */
                m_dispMosaicIdx = output->m_instId;
            }
        }
    }

    for(long unsigned int i = 0; i < commonMosaicInfo.size(); i++)
    {
        imgMosaic *img_mosaic_obj = new imgMosaic;
        img_mosaic_obj->getConfig(commonMosaicInfo[i]);
        status = tiovx_img_mosaic_module_init(m_ovxGraph->context, &img_mosaic_obj->imgMosaicObj);
        m_imgMosaicObjs.push_back(img_mosaic_obj);
    }

    /* Create openVX nodes for all required modules */

    for(uint i=0; i < m_multiScalerObjs.size(); i++)
    {
        if(status == VX_SUCCESS)
        {
            status = tiovx_multi_scaler_module_create(m_ovxGraph->graph,
                                                        &m_multiScalerObjs[i]->multiScalerObj1,
                                                        NULL, TIVX_TARGET_VPAC_MSC1);
            if(status == VX_SUCCESS && m_multiScalerObjs[i]->useSecondaryMsc)
            {
                status = tiovx_multi_scaler_module_create(m_ovxGraph->graph,
                                        &m_multiScalerObjs[i]->multiScalerObj2,
                                        m_multiScalerObjs[i]->multiScalerObj1.output[0].arr[0],
                                        TIVX_TARGET_VPAC_MSC2);
            }
        }

        if(status == VX_SUCCESS)
        {
            if(m_multiScalerObjs[i]->useSecondaryMsc)
            {
                status = tiovx_dl_pre_proc_module_create(m_ovxGraph->graph,
                                                        &m_preProcObjs[i]->dlPreProcObj,
                                                        m_multiScalerObjs[i]->multiScalerObj2.output[0].arr[0],
                                                        TIVX_TARGET_MPU_0);
            }
            else
            {
                status = tiovx_dl_pre_proc_module_create(m_ovxGraph->graph,
                                                        &m_preProcObjs[i]->dlPreProcObj,
                                                        m_multiScalerObjs[i]->multiScalerObj1.output[0].arr[0],
                                                        TIVX_TARGET_MPU_0);
            }
        }

        if(status == VX_SUCCESS)
        {
            status = tiovx_tidl_module_create(m_ovxGraph->context,
                                        m_ovxGraph->graph,
                                        &m_tidlInfObjs[i]->tidlObj,
                                        m_preProcObjs[i]->dlPreProcObj.output.arr);
        }

        if(status == VX_SUCCESS)
        {
            status = tiovx_dl_post_proc_module_create(m_ovxGraph->graph,
                                            &m_postProcObjs[i]->dlPostProcObj,
                                            m_multiScalerObjs[i]->multiScalerObj1.output[1].arr[0],
                                            m_tidlInfObjs[i]->tidlObj.output, TIVX_TARGET_MPU_0);
        }
    }

    for(uint i = 0; i < m_imgMosaicObjs.size(); i++)
    {
        for(uint j = 0; j < postProcMosaicConn[i].size(); j++)
        {
            int index = postProcMosaicConn[i][j];
            m_imgMosaicObjs[i]->mosaic_input_arr[j] = m_postProcObjs[index]->dlPostProcObj.output_image.arr[0];
        }
        status = tiovx_img_mosaic_module_create(m_ovxGraph->graph,
                                &m_imgMosaicObjs[i]->imgMosaicObj,
                                NULL,
                                m_imgMosaicObjs[i]->mosaic_input_arr, TIVX_TARGET_VPAC_MSC1);
    }

    if(m_displayObj != NULL)
    {
        status = tiovx_display_module_create(m_ovxGraph->graph, m_displayObj,
                                        m_imgMosaicObjs[m_dispMosaicIdx]->imgMosaicObj.output_image[0],
                                        TIVX_TARGET_DISPLAY1);
    }

    for(uint i=0; i < m_multiScalerObjs.size(); i++)
    {
        /* Add graph parameter by node index */

        if((vx_status)VX_SUCCESS == status)
        {
            status = add_graph_parameter_by_node_index(m_ovxGraph->graph, m_multiScalerObjs[i]->multiScalerObj1.node, 0);
            m_multiScalerObjs[i]->multiScalerObj1.input.graph_parameter_index = graph_parameter_index;
            graph_parameters_queue_params_list[graph_parameter_index].graph_parameter_index = graph_parameter_index;
            graph_parameters_queue_params_list[graph_parameter_index].refs_list_size = 1;
            graph_parameters_queue_params_list[graph_parameter_index].refs_list =
                (vx_reference*)&m_multiScalerObjs[i]->multiScalerObj1.input.image_handle[0];
            graph_parameter_index++;
        }
    }

    for(uint i = 0; i < m_imgMosaicObjs.size(); i++)
    {
        if(i == m_dispMosaicIdx && m_displayObj != NULL)
        {
            continue;
        }

        if((vx_status)VX_SUCCESS == status)
        {
            /* Output image index of Mosaic Node is 1 */
            status = add_graph_parameter_by_node_index(m_ovxGraph->graph, m_imgMosaicObjs[i]->imgMosaicObj.node, 1);
            m_imgMosaicObjs[i]->imgMosaicObj.output_graph_parameter_index = graph_parameter_index;
            graph_parameters_queue_params_list[graph_parameter_index].graph_parameter_index = graph_parameter_index;
            graph_parameters_queue_params_list[graph_parameter_index].refs_list_size = 1;
            graph_parameters_queue_params_list[graph_parameter_index].refs_list =
                (vx_reference*)&m_imgMosaicObjs[i]->imgMosaicObj.output_image[0];
            graph_parameter_index++;
        }
    }

    /* Schedule Graph */

    if((vx_status)VX_SUCCESS == status)
    {
        status = vxSetGraphScheduleConfig(m_ovxGraph->graph,
                    VX_GRAPH_SCHEDULE_MODE_QUEUE_MANUAL,
                    graph_parameter_index,
                    graph_parameters_queue_params_list);
    }

    /* VERIFY GRAPH */

    if((vx_status)VX_SUCCESS == status)
    {
        status = vxVerifyGraph(m_ovxGraph->graph);
    }

    for(uint i=0; i < m_multiScalerObjs.size(); i++)
    {
        tiovx_multi_scaler_module_update_filter_coeffs(&m_multiScalerObjs[i]->multiScalerObj1);
        tiovx_multi_scaler_module_update_crop_params(&m_multiScalerObjs[i]->multiScalerObj1);
        if (m_multiScalerObjs[i]->useSecondaryMsc)
        {
            tiovx_multi_scaler_module_update_filter_coeffs(&m_multiScalerObjs[i]->multiScalerObj2);
            tiovx_multi_scaler_module_update_crop_params(&m_multiScalerObjs[i]->multiScalerObj2);
        }
    }

    return status;
}

/**
 * Dumps openVX graph as dot file
 */
void EdgeAIDemoImpl::dumpGraphAsDot()
{
    tivxExportGraphToDot(m_ovxGraph->graph, const_cast<vx_char*>(string(".").c_str()), const_cast<vx_char*>(string("edgeai_tiovx_apps_graph").c_str()));
}

/**
 * Sets a flag for the threads to exit at the next opportunity.
 * This controls the stopping all threads, stop the openVX graph
 * and other cleanup.
 */
void EdgeAIDemoImpl::sendExitSignal()
{
    m_runLoop = false;
}

/**
 * This is a blocking call that waits for all the internal threads to exit.
 */
void EdgeAIDemoImpl::waitForExit()
{
}

/** Destructor. */
EdgeAIDemoImpl::~EdgeAIDemoImpl()
{
    LOG_DEBUG("EdgeAIDemoImpl Destructor \n");

    /* Delete openVX modules */

    if(m_displayObj != NULL)
    {
        tiovx_display_module_delete(m_displayObj);
    }

    for(auto &iter : m_multiScalerObjs)
    {
        tiovx_multi_scaler_module_delete(&iter->multiScalerObj1);
        if(iter->useSecondaryMsc)
        {
            tiovx_multi_scaler_module_delete(&iter->multiScalerObj2);
        }
    }

    for(auto &iter : m_preProcObjs)
    {
        tiovx_dl_pre_proc_module_delete(&iter->dlPreProcObj);
    }

    for(auto &iter : m_postProcObjs)
    {
        tiovx_dl_post_proc_module_delete(&iter->dlPostProcObj);
    }

    for(auto &iter : m_tidlInfObjs)
    {
        tiovx_tidl_module_delete(&iter->tidlObj);
    }

    for(auto &iter : m_imgMosaicObjs)
    {
        tiovx_img_mosaic_module_delete(&iter->imgMosaicObj);
    }

    /* Release openVX Graph */

    if(m_ovxGraph->graph != NULL)
    {
        vxReleaseGraph(&m_ovxGraph->graph);
    }

    /* Deinit openVX modules */

    if(m_displayObj != NULL)
    {
        tiovx_display_module_deinit(m_displayObj);
        delete m_displayObj;
    }

    for(auto &iter : m_multiScalerObjs)
    {
        tiovx_multi_scaler_module_deinit(&iter->multiScalerObj1);
        if(iter->useSecondaryMsc)
        {
            tiovx_multi_scaler_module_deinit(&iter->multiScalerObj2);
        }
        delete iter;
    }

    for(auto &iter : m_preProcObjs)
    {
        tiovx_dl_pre_proc_module_deinit(&iter->dlPreProcObj);
        delete iter;
    }

    for(auto &iter : m_postProcObjs)
    {
        tiovx_dl_post_proc_module_deinit(&iter->dlPostProcObj);
        delete iter;
    }
    
    for(auto &iter : m_tidlInfObjs)
    {
        tiovx_tidl_module_deinit(&iter->tidlObj);
        delete iter;
    }

    for(auto &iter : m_imgMosaicObjs)
    {
        tiovx_img_mosaic_module_deinit(&iter->imgMosaicObj);
        delete iter;
    }

    /*
        Delete application openVX object
        Also release context in its destructor
    */

    delete m_ovxGraph;
}

/** Constructor. */
EdgeAIDemo::EdgeAIDemo(const YAML::Node    &yaml)
{
    m_impl = new EdgeAIDemoImpl(yaml);
}

EdgeAIDemo::~EdgeAIDemo()
{
    delete m_impl;
}

/**
 * Dumps as dot file
 */
void EdgeAIDemo::dumpGraphAsDot()
{
    m_impl->dumpGraphAsDot();
}

int32_t EdgeAIDemo::startDemo()
{
    int32_t status = 0;

    status = m_impl->startDemo();

    return status;
}

/**
 * Sets a flag for the threads to exit at the next opportunity.
 * This controls the stopping all threads, stop the openVX graph
 * and other cleanup.
 */
void EdgeAIDemo::sendExitSignal()
{
    m_impl->sendExitSignal();
}

/**
 * This is a blocking call that waits for all the internal threads to exit.
 */
void EdgeAIDemo::waitForExit()
{
    m_impl->waitForExit();
}

} // namespace ti::edgeai::common

