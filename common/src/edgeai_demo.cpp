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

/* Module headers. */
#include <common/include/edgeai_utils.h>
#include <common/include/edgeai_demo_config.h>
#include <common/include/edgeai_demo.h>
#include <common/include/edgeai_ovx_wrapper.h>
#include <common/include/edgeai_tidl.h>
#include <common/include/edgeai_post_proc.h>
#include <common/include/edgeai_pre_proc.h>
#include <common/include/edgeai_msc.h>

/* OpenVX headers */
#include <tiovx_utils.h>

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
    uint32_t cnt = 0;

    while(m_runLoop)
    {
        for (auto &[name,flow] : m_config.m_flowMap)
        {
            auto const &input = m_config.m_inputMap[flow->m_inputId];
            if(input->m_srcType == "image")
            {
                readImage(const_cast<char*>(input->m_source.c_str()), m_multiScalerObjs[0]->multiScalerObj1.input.image_handle[0]); 
                m_runLoop = input->m_loop;
            }
            else if(input->m_srcType == "images_directory")
            {
                readImage(const_cast<char*>(input->m_multiImageVect[input->m_multiImageVectCnt].c_str()), m_multiScalerObjs[0]->multiScalerObj1.input.image_handle[0]);
                input->m_multiImageVectCnt++;
                if (input->m_multiImageVectCnt == input->m_multiImageVect.size())
                {
                    input->m_multiImageVectCnt = 0;
                    m_runLoop = input->m_loop;
                }
            }
        }
        
        /*  vxGraphParameterEnqueueReadyRef */
        vxGraphParameterEnqueueReadyRef(m_ovxGraph->graph, 
                m_multiScalerObjs[0]->multiScalerObj1.input.graph_parameter_index, 
                (vx_reference*)&m_multiScalerObjs[0]->multiScalerObj1.input.image_handle[0], 1);
        vxGraphParameterEnqueueReadyRef(m_ovxGraph->graph,
                m_postProcObjs[0]->dlPostProcObj.output_image.graph_parameter_index,
                (vx_reference*)&m_postProcObjs[0]->dlPostProcObj.output_image.image_handle[0], 1);

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
        vxGraphParameterDequeueDoneRef(m_ovxGraph->graph,
                            m_multiScalerObjs[0]->multiScalerObj1.input.graph_parameter_index,
                            (vx_reference*)&input_o, 1, &num_refs);
        vxGraphParameterDequeueDoneRef(m_ovxGraph->graph, 
                            m_postProcObjs[0]->dlPostProcObj.output_image.graph_parameter_index, 
                            (vx_reference*)&output_o, 1, &num_refs);

        /*  Write Output to file    */
        for (auto &[name,flow] : m_config.m_flowMap)
        {
            auto const &outputIds = flow->m_outputIds;
            for(uint i=0; i < outputIds.size(); i++)
            {
                OutputInfo  *output = m_config.m_outputMap[outputIds[i]];
                if(output->m_sinkType == "image")
                {
                    string output_filename = output->m_sink + to_string(cnt);

                    writeImage(const_cast<char*>(output_filename.c_str()), m_postProcObjs[i]->dlPostProcObj.output_image.image_handle[0]);
                }
            }
        }
        cnt++;
    }

    return status;
}

int32_t EdgeAIDemoImpl::setupFlows()
{
    int32_t status = 0;

    /* Create graph struct */
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
            /* flow->m_sensorDimVec[i][0] is width */
            /* flow->m_sensorDimVec[i][0] is height */
            post_proc_obj->getConfig(model->m_modelPath, tidl_inf_obj->ioBufDesc, flow->m_sensorDimVec[i][0], flow->m_sensorDimVec[i][1]);
            post_proc_obj->dlPostProcObj.params.oc_prms.num_top_results = model->m_topN;
            post_proc_obj->dlPostProcObj.params.od_prms.viz_th = model->m_vizThreshold;
            post_proc_obj->dlPostProcObj.params.ss_prms.alpha = model->m_alpha;
            status = tiovx_dl_post_proc_module_init(m_ovxGraph->context, &post_proc_obj->dlPostProcObj);

            m_postProcObjs.push_back(post_proc_obj);

            /* Get Configuration of MSC module by parsing params.yaml  */
            multi_scaler_obj->getConfig(input->m_width, input->m_height, flow->m_sensorDimVec[i][0], flow->m_sensorDimVec[i][1], pre_proc_obj);
            status = tiovx_multi_scaler_module_init(m_ovxGraph->context, &multi_scaler_obj->multiScalerObj1);
            if (multi_scaler_obj->useSecondaryMsc)
            {
                status = tiovx_multi_scaler_module_init(m_ovxGraph->context, &multi_scaler_obj->multiScalerObj2);
            }

            m_multiScalerObjs.push_back(multi_scaler_obj);
        }
    }

    if(status == VX_SUCCESS)
    {
        status = tiovx_multi_scaler_module_create(m_ovxGraph->graph, &m_multiScalerObjs[0]->multiScalerObj1, NULL, TIVX_TARGET_VPAC_MSC1);
        if(status == VX_SUCCESS && m_multiScalerObjs[0]->useSecondaryMsc)
        {
            status = tiovx_multi_scaler_module_create(m_ovxGraph->graph, &m_multiScalerObjs[0]->multiScalerObj2, 
                                                        m_multiScalerObjs[0]->multiScalerObj1.output[0].arr[0], TIVX_TARGET_VPAC_MSC2);
        }
    }

    if(status == VX_SUCCESS)
    {
        if(m_multiScalerObjs[0]->useSecondaryMsc)
        {
            status = tiovx_dl_pre_proc_module_create(m_ovxGraph->graph, &m_preProcObjs[0]->dlPreProcObj, 
                                                     m_multiScalerObjs[0]->multiScalerObj2.output[0].arr[0], TIVX_TARGET_MPU_0);
        }
        else
        {
            status = tiovx_dl_pre_proc_module_create(m_ovxGraph->graph, &m_preProcObjs[0]->dlPreProcObj, 
                                                     m_multiScalerObjs[0]->multiScalerObj1.output[0].arr[0], TIVX_TARGET_MPU_0);
        }
    }

    if(status == VX_SUCCESS)
    {
        tiovx_tidl_module_create(m_ovxGraph->context, m_ovxGraph->graph, &m_tidlInfObjs[0]->tidlObj, m_preProcObjs[0]->dlPreProcObj.output.arr);
    }

    if(status == VX_SUCCESS)
    {
        tiovx_dl_post_proc_module_create(m_ovxGraph->graph, &m_postProcObjs[0]->dlPostProcObj, m_multiScalerObjs[0]->multiScalerObj1.output[1].arr[0], m_tidlInfObjs[0]->tidlObj.output, TIVX_TARGET_MPU_0);
    }

    /* add_graph_parameter by node index */

    vx_int32 graph_parameter_index = 0;
    vx_graph_parameter_queue_params_t graph_parameters_queue_params_list[8];

    if((vx_status)VX_SUCCESS == status)
    {
        status = add_graph_parameter_by_node_index(m_ovxGraph->graph, m_multiScalerObjs[0]->multiScalerObj1.node, 0);
        m_multiScalerObjs[0]->multiScalerObj1.input.graph_parameter_index = graph_parameter_index;
        graph_parameters_queue_params_list[graph_parameter_index].graph_parameter_index = graph_parameter_index;
        graph_parameters_queue_params_list[graph_parameter_index].refs_list_size = 1;
        graph_parameters_queue_params_list[graph_parameter_index].refs_list = (vx_reference*)&m_multiScalerObjs[0]->multiScalerObj1.input.image_handle[0];
        graph_parameter_index++;
    }

    if((vx_status)VX_SUCCESS == status)
    {
        status = add_graph_parameter_by_node_index(m_ovxGraph->graph, m_postProcObjs[0]->dlPostProcObj.node, 2);
        m_postProcObjs[0]->dlPostProcObj.output_image.graph_parameter_index = graph_parameter_index;
        graph_parameters_queue_params_list[graph_parameter_index].graph_parameter_index = graph_parameter_index;
        graph_parameters_queue_params_list[graph_parameter_index].refs_list_size = 1;
        graph_parameters_queue_params_list[graph_parameter_index].refs_list = (vx_reference*)&m_postProcObjs[0]->dlPostProcObj.output_image.image_handle[0];
        graph_parameter_index++;
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

    tiovx_multi_scaler_module_update_filter_coeffs(&m_multiScalerObjs[0]->multiScalerObj1);
    tiovx_multi_scaler_module_update_crop_params(&m_multiScalerObjs[0]->multiScalerObj1);
    if (m_multiScalerObjs[0]->useSecondaryMsc)
    {
        tiovx_multi_scaler_module_update_filter_coeffs(&m_multiScalerObjs[0]->multiScalerObj2);
        tiovx_multi_scaler_module_update_crop_params(&m_multiScalerObjs[0]->multiScalerObj2);
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

    for(auto &iter : m_multiScalerObjs)
    {
        tiovx_multi_scaler_module_delete(&iter->multiScalerObj1);
        tiovx_multi_scaler_module_deinit(&iter->multiScalerObj1);
        if(iter->useSecondaryMsc)
        {
            tiovx_multi_scaler_module_delete(&iter->multiScalerObj2);
            tiovx_multi_scaler_module_deinit(&iter->multiScalerObj2);
        }
        delete iter;
    }

    for(auto &iter : m_preProcObjs)
    {
        tiovx_dl_pre_proc_module_delete(&iter->dlPreProcObj);
        tiovx_dl_pre_proc_module_deinit(&iter->dlPreProcObj);
        delete iter;
    }

    for(auto &iter : m_postProcObjs)
    {
        tiovx_dl_post_proc_module_delete(&iter->dlPostProcObj);
        tiovx_dl_post_proc_module_deinit(&iter->dlPostProcObj);
        delete iter;
    }
    
    for(auto &iter : m_tidlInfObjs)
    {
        tiovx_tidl_module_delete(&iter->tidlObj);
        tiovx_tidl_module_deinit(&iter->tidlObj);
        delete iter;
    }

    if(m_ovxGraph->graph != NULL)
    {
        LOG_ERROR("Before vxReleaseGraph \n");
        vxReleaseGraph(&m_ovxGraph->graph);
        LOG_ERROR("After vxReleaseGraph \n");
    }

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

