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
#include <common/include/edgeai_camera.h>

/* OpenVX headers */
#include <tiovx_utils.h>
#include <tiovx_display_module.h>

extern "C"
{
#include <utils/grpx/include/app_grpx.h>
}

/**
 * \defgroup group_edgeai_common Master demo code
 *
 * \brief Common code used across all EdgeAI applications.
 *
 * \ingroup group_edgeai_cpp_apps
 */

namespace ti::edgeai::common
{

#define RTOS_DISPLAY_WIDTH  1920
#define RTOS_DISPLAY_HEIGHT 1080

#define GRAPH_PARAMETERS_QUEUE_PARAMS_LIST_SIZE 16

#if !defined (SOC_AM62A)
/* Callback function for drawing performance stats on display. */
static void app_draw_perf_graphics(Draw2D_Handle *draw2d_obj, Draw2D_BufInfo *draw2d_buf_info, uint32_t update_type)
{
    uint16_t cpuWidth = 0;
    uint16_t cpuHeight = 0;

    uint16_t hwaWidth = 0;
    uint16_t hwaHeight = 0;

    uint16_t ddrWidth = 0;
    uint16_t ddrHeight = 0;

    uint16_t startX, startY;

    if (draw2d_obj == NULL)
    {
        return;
    }

    if(update_type==0)
    {
        appGrpxShowLogo(0, 0);
    }
    else
    {
        /* Get height and width for cpu,hwa & ddr graphs*/
        appGrpxGetDimCpuLoad(&cpuWidth, &cpuHeight);
        appGrpxGetDimHwaLoad(&hwaWidth, &hwaHeight);
        appGrpxGetDimDdrLoad(&ddrWidth, &ddrHeight);

        /* Draw the graphics. */
        startX = (draw2d_buf_info->bufWidth - (cpuWidth + hwaWidth + ddrWidth)) / 2;
        startY = draw2d_buf_info->bufHeight - cpuHeight;
        appGrpxShowCpuLoad(startX, startY);

        startX = startX + cpuWidth;
        startY = draw2d_buf_info->bufHeight - hwaHeight;
        appGrpxShowHwaLoad(startX, startY);

        startX = startX + hwaWidth;
        startY = draw2d_buf_info->bufHeight - ddrHeight;
        appGrpxShowDdrLoad(startX, startY);
    }

    return;
}
#endif

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
         * Copy Constructor.
         *
         * Copy Constructor is not required and allowed and hence prevent
         * the compiler from generating a default Copy Constructor.
         */
        EdgeAIDemoImpl(const EdgeAIDemoImpl& ) = delete;

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

        /** Vector of camera objects */
        camera                              *m_cameraObj{NULL};

        /** Vector of multiscaler objects */
        vector<multiScaler*>                m_multiScalerObjs{};

        /** Vector of pre process objects */
        vector<preProc*>                    m_preProcObjs{};

        /** Vector of TIDL objects */
        vector<tidlInf*>                    m_tidlInfObjs{};
        
        /** Vector of post process objects */
        vector<postProc*>                   m_postProcObjs{};

        /** Vector of mosaic objects */
        vector<imgMosaic*>                  m_imgMosaicObjs{};

        /** Module display object */
        TIOVXDisplayModuleObj               *m_displayObj{NULL};

        /** Mapping from camera input to its corresponding MSC object */
        vector<int32_t>                     m_camMscIdxMap{};

        /** For multiple flows having camera input, init and create just one set of nodes */
        bool                                m_camNodesInit{false};

        /** Number of camera input present across the flows */
        int32_t                             m_numCam{0};

        /** Flag to store which mosaic goes to display */
        uint32_t                            m_dispMosaicIdx{0};

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

int32_t EdgeAIDemoImpl::setupFlows()
{
    int32_t status = 0;

    vx_int32 graph_parameter_index = 0;
    vx_graph_parameter_queue_params_t graph_parameters_queue_params_list[GRAPH_PARAMETERS_QUEUE_PARAMS_LIST_SIZE];
#if !defined (SOC_AM62A)
    app_grpx_init_prms_t grpx_prms;
#endif

    vector<vector<vector<int32_t>>> commonMosaicInfo;
    vector<vector<string>>          commonMosaicTitle;
    vector<vector<int32_t>>         postProcMosaicConn;
    string                          modelName{""};
    int32_t                         cameraChMask = 0;
    bool                            isMultiCam = false;
    InputInfo*                      camInputInfo;
    uint32_t                        subflowObjSizes = 0;

    for(int32_t i = 0; i < OutputInfo::m_numInstances; i++)
    {
        commonMosaicInfo.push_back({});
        commonMosaicTitle.push_back({});
        postProcMosaicConn.push_back({});
    }

    /* Create graph structure */
    m_ovxGraph  = new ovxGraph;

    /* Create graph */
    m_ovxGraph->graph = vxCreateGraph(m_ovxGraph->context);
    status = vxGetStatus((vx_reference)m_ovxGraph->graph);

    if(status != VX_SUCCESS)
    {
        throw runtime_error("Graph Creation failed \n");
    }

    /* Get channel mask for sensor node. */
    for (auto const &[name,flow] : m_config.m_flowMap)
    {
        auto const &input = m_config.m_inputMap[flow->m_inputId];

        /**
         * cameraChMask = 00001101 means cameras connected to port 
         *  (or m_cameraId) 0, 2 and 3 of fusion board
         */
        if(input->m_source == "camera")
        {
            cameraChMask = cameraChMask | (1 << input->m_cameraId);
            m_numCam++;
            camInputInfo = input;
        }
    }

    if (m_numCam > 1)
    {
        isMultiCam = true;
    }

    /**
     * Make camera object and initialize camera related nodes only if atleast
     * one camera is input across the flows.
     */
    if(cameraChMask != 0)
    {
        m_cameraObj = new camera;
        m_cameraObj->getConfig(camInputInfo, cameraChMask);
        if(status == VX_SUCCESS)
        {
            status = tiovx_sensor_module_init(&m_cameraObj->sensorObj,
                                              const_cast<char*>(string("sensor_obj").c_str()));
        }
        if(status == VX_SUCCESS)
        {
            status = tiovx_capture_module_init(m_ovxGraph->context,
                                               &m_cameraObj->captureObj,
                                               &m_cameraObj->sensorObj);
        }
        if(status == VX_SUCCESS)
        {
            status = tiovx_viss_module_init(m_ovxGraph->context,
                                            &m_cameraObj->vissObj,
                                            &m_cameraObj->sensorObj);
        }
#if !defined (SOC_AM62A)
        if(status == VX_SUCCESS)
        {
            status = tiovx_aewb_module_init(m_ovxGraph->context,
                                            &m_cameraObj->aewbObj,
                                            &m_cameraObj->sensorObj,
                                            const_cast<char*>(string("aewb_obj").c_str()),
                                            0,
                                            m_cameraObj->sensorObj.num_cameras_enabled);
        }
#endif
        if(status == VX_SUCCESS)
        {
            status = tiovx_ldc_module_init(m_ovxGraph->context,
                                           &m_cameraObj->ldcObj,
                                           &m_cameraObj->sensorObj);
        }
    }

    for (auto const &[name,flow] : m_config.m_flowMap)
    {
        flow->initialize(m_config.m_modelMap,
                         m_config.m_inputMap,
                         m_config.m_outputMap,
                         isMultiCam);

        auto const &modelIds    = flow->m_modelIds;
        auto const &outputIds   = flow->m_outputIds;
        auto const &input       = m_config.m_inputMap[flow->m_inputId];

        if(modelIds.size() != outputIds.size())
        {
            throw runtime_error("Size of modelIds is not equal to size of outputIds \n");
        }

        /* subflowObjSizes denotes the total number of subflows traversed 
           within all flows till now */
        if((input->m_source != "camera") || (m_camNodesInit != true))
        {
            m_preProcObjs.resize(subflowObjSizes+modelIds.size());
            m_tidlInfObjs.resize(subflowObjSizes+modelIds.size());
            m_postProcObjs.resize(subflowObjSizes+modelIds.size());
            m_multiScalerObjs.resize(subflowObjSizes+modelIds.size());
        }

        for(uint32_t i=0; i < modelIds.size(); i++)
        {
            if( (input->m_source != "camera") || (m_camNodesInit != true) )
            {
                m_preProcObjs[i+subflowObjSizes]     = new preProc;
                m_tidlInfObjs[i+subflowObjSizes]     = new tidlInf;
                m_postProcObjs[i+subflowObjSizes]    = new postProc;
                m_multiScalerObjs[i+subflowObjSizes] = new multiScaler;
            }
            
            /**
            * modelIds is a vector of all models used for a unique input
            * outputIds is a vector of all outputs used for a unique input
            */
            ModelInfo   *model = m_config.m_modelMap[modelIds[i]];
            OutputInfo  *output = m_config.m_outputMap[outputIds[i]];
            
            /* Init modules (for camera input just init once). */
            if( (input->m_source != "camera") || (m_camNodesInit != true) )
            {
                /* Get config for TIDL module. */
                m_tidlInfObjs[i+subflowObjSizes]->getConfig(model->m_modelPath, m_ovxGraph->context);

                if(input->m_srcType == "camera")
                {
                    m_tidlInfObjs[i+subflowObjSizes]->tidlObj.num_cameras = m_cameraObj->sensorObj.num_cameras_enabled;
                }
                status = tiovx_tidl_module_init(m_ovxGraph->context,
                                                &m_tidlInfObjs[i+subflowObjSizes]->tidlObj,
                                                const_cast<vx_char*>(string("tidl_obj").c_str()));
                
                /* Get config for Pre Process module. */
                m_preProcObjs[i+subflowObjSizes]->getConfig(model->m_modelPath, m_tidlInfObjs[i+subflowObjSizes]->ioBufDesc);

                if(input->m_srcType == "camera")
                {
                    m_preProcObjs[i+subflowObjSizes]->dlPreProcObj.num_channels = m_cameraObj->sensorObj.num_cameras_enabled;
                }
                status = tiovx_dl_pre_proc_module_init(m_ovxGraph->context,
                                                       &m_preProcObjs[i+subflowObjSizes]->dlPreProcObj);

                /* Get config for Post Process Module. */
                m_postProcObjs[i+subflowObjSizes]->getConfig(model->m_modelPath,
                                         m_tidlInfObjs[i+subflowObjSizes]->ioBufDesc,
                                         flow->m_mosaicInfoVec[i][2],
                                         flow->m_mosaicInfoVec[i][3]);

                m_postProcObjs[i+subflowObjSizes]->dlPostProcObj.params.oc_prms.num_top_results = model->m_topN;
                m_postProcObjs[i+subflowObjSizes]->dlPostProcObj.params.od_prms.viz_th = model->m_vizThreshold;
                m_postProcObjs[i+subflowObjSizes]->dlPostProcObj.params.ss_prms.alpha = model->m_alpha;

                if(input->m_srcType == "camera")
                {
                    m_postProcObjs[i+subflowObjSizes]->dlPostProcObj.num_channels = m_cameraObj->sensorObj.num_cameras_enabled;
                }
                status = tiovx_dl_post_proc_module_init(m_ovxGraph->context,
                                                        &m_postProcObjs[i+subflowObjSizes]->dlPostProcObj);

                /* Get config for multiscaler module. */
                m_multiScalerObjs[i+subflowObjSizes]->getConfig(input->m_width,
                                            input->m_height,
                                            flow->m_mosaicInfoVec[i][2],
                                            flow->m_mosaicInfoVec[i][3],
                                            m_preProcObjs[i+subflowObjSizes]);

                if(input->m_srcType == "camera")
                {
                    m_multiScalerObjs[i+subflowObjSizes]->multiScalerObj1.num_channels = m_cameraObj->sensorObj.num_cameras_enabled;
                    m_multiScalerObjs[i+subflowObjSizes]->multiScalerObj2.num_channels = m_cameraObj->sensorObj.num_cameras_enabled;
                }

                status = tiovx_multi_scaler_module_init(m_ovxGraph->context,
                                                        &m_multiScalerObjs[i+subflowObjSizes]->multiScalerObj1);
                if (m_multiScalerObjs[i+subflowObjSizes]->useSecondaryMsc)
                {
                    status = tiovx_multi_scaler_module_init(m_ovxGraph->context,
                                                            &m_multiScalerObjs[i+subflowObjSizes]->multiScalerObj2);
                }

                /**
                 * This vector will contain index of all multiScalerObj which
                 * will take input from the camera nodes.
                 */
                if(input->m_srcType == "camera")
                {
                    m_camMscIdxMap.push_back(i+subflowObjSizes);
                }
            }

            /**
             * This map contains mosaic info (posX,posY,width,height...) about
             * all inputs that will end up being a part of single mosaic that
             * will eventually go to this particulat output.
             */
            if(!commonMosaicInfo.empty())
            {
                commonMosaicInfo[output->m_instId].push_back(flow->m_mosaicInfoVec[i]);
            }

            /**
             * Push all model names for particular output.
             * Used to render text above mosaic windows.
             */
            modelName = model->m_modelPath;
            if (modelName.back() == '/')
            {
                modelName.pop_back();
            }
            modelName = std::filesystem::path(modelName).filename();
            if(!commonMosaicTitle.empty())
            {
                commonMosaicTitle[output->m_instId].push_back(modelName);
            }

            /**
             * This map contains index of all post-process nodes that will
             * be the inputs to a single mosaic. This mosaic will eventually
             * go to this particular output.
             */
            if(!postProcMosaicConn.empty())
            {
                postProcMosaicConn[output->m_instId].push_back(m_postProcObjs.size() - 1);
            }

#if !defined(SOC_AM62A)
            if( (output->m_sinkType == "display") && (m_displayObj == NULL) )
            {
                /* Init display module. */
                m_displayObj = new TIOVXDisplayModuleObj;
                tiovx_display_module_params_init(m_displayObj);

                m_displayObj->input_width  = output->m_width;
                m_displayObj->input_height = output->m_height;
                m_displayObj->input_color_format = VX_DF_IMAGE_NV12;

                m_displayObj->params.outWidth  = output->m_width;
                m_displayObj->params.outHeight = output->m_height;

                /* Rtos display has fixed resolution of 1920x1080.*/
                m_displayObj->params.posX = (RTOS_DISPLAY_WIDTH - output->m_width)/2;
                m_displayObj->params.posY = (RTOS_DISPLAY_HEIGHT - output->m_height)/2;

                tiovx_display_module_init(m_ovxGraph->context, m_displayObj);

                /* Save the instance id of mosaic output which goes to display. */
                m_dispMosaicIdx = output->m_instId;

                /* Overlay preformance */
                appGrpxInitParamsInit(&grpx_prms, m_ovxGraph->context);
                /* Rtos display has fixed resolution of 1920x1080.*/
                grpx_prms.width = RTOS_DISPLAY_WIDTH;
                grpx_prms.height = RTOS_DISPLAY_HEIGHT;
                /* Custom callback for perf overlay*/
                grpx_prms.draw_callback = app_draw_perf_graphics;
                appGrpxInit(&grpx_prms);
            }
#endif
        }

        if((input->m_source != "camera") || (m_camNodesInit != true))
        {
            subflowObjSizes += modelIds.size();
        }
        
        if(input->m_srcType == "camera")
        {
            m_camNodesInit = true;
        }
    }

    /**
     * Initialize mosaic module with single/multiple inputs.
     * Information (posX,posY,width,height...) regarding all inputs are
     * contained in commonMosaicInfo and commonMosaicTitle vectors.
     */
    for(uint32_t i = 0; i < commonMosaicInfo.size(); i++)
    {
        imgMosaic *img_mosaic_obj = new imgMosaic;
        if(!commonMosaicInfo[i].empty())
        {
            img_mosaic_obj->getConfig(commonMosaicInfo[i]);
        }

        status = tiovx_img_mosaic_module_init(m_ovxGraph->context,
                                              &img_mosaic_obj->imgMosaicObj);

        /* Set black mosaic background and overlay titles and texts.*/
        if(!commonMosaicTitle[i].empty())
        {
            img_mosaic_obj->setBackground(m_config.m_title, commonMosaicTitle[i]);
        }

        m_imgMosaicObjs.push_back(img_mosaic_obj);
    }

    /* Create all camera related module if needed.*/
    if(m_cameraObj != NULL)
    {
        string viss_target = TIVX_TARGET_VPAC_VISS1;
        string ldc_target = TIVX_TARGET_VPAC_LDC1;

#if defined(SOC_J784S4)
        if(camInputInfo->m_vpac_id == 2)
        {
            viss_target = TIVX_TARGET_VPAC2_VISS1;
            ldc_target = TIVX_TARGET_VPAC2_LDC1;
        }
#endif

        if(status == VX_SUCCESS)
        {
            status = tiovx_capture_module_create(m_ovxGraph->graph,
                                                 &m_cameraObj->captureObj,
                                                 TIVX_TARGET_CAPTURE1);
        }
        if(status == VX_SUCCESS)
        {
            status = vxReleaseObjectArray(&m_cameraObj->vissObj.ae_awb_result_arr[0]);
            m_cameraObj->vissObj.ae_awb_result_arr[0] = NULL;
            status = tiovx_viss_module_create(m_ovxGraph->graph,
                                              &m_cameraObj->vissObj,
                                              m_cameraObj->captureObj.image_arr[0],
                                              NULL,
                                              viss_target.c_str());
        }
#if !defined (SOC_AM62A)
        if(status == VX_SUCCESS)
        {
            status = tiovx_aewb_module_create(m_ovxGraph->graph,
                                              &m_cameraObj->aewbObj,
                                              m_cameraObj->vissObj.h3a_stats_arr[0]);
        }
#endif
        if(status == VX_SUCCESS)
        {
            status = tiovx_ldc_module_create(m_ovxGraph->graph,
                                             &m_cameraObj->ldcObj,
                                             m_cameraObj->vissObj.output2.arr[0],
                                             ldc_target.c_str());
        }
    }

    if(m_cameraObj != NULL)
    {
        /* Create multiscaler module with appropriate msc connected to ldc node.*/
        for(auto &iter : m_camMscIdxMap)
        {
            if(status == VX_SUCCESS)
            {
                status = tiovx_multi_scaler_module_create(m_ovxGraph->graph,
                                                          &m_multiScalerObjs[iter]->multiScalerObj1,
                                                          m_cameraObj->ldcObj.output0.arr[0],
                                                          TIVX_TARGET_VPAC_MSC1);

                if( (status == VX_SUCCESS) && (m_multiScalerObjs[iter]->useSecondaryMsc) )
                {
                    status = tiovx_multi_scaler_module_create(m_ovxGraph->graph,
                                                              &m_multiScalerObjs[iter]->multiScalerObj2,
                                                              m_multiScalerObjs[iter]->multiScalerObj1.output[0].arr[0],
                                                              TIVX_TARGET_VPAC_MSC2);
                }

                /* If the input is camera, the multiscaler used with this
                 * subflow is not the first node.
                 */
                m_multiScalerObjs[iter]->isHeadNode = false;
            }
        }
    }

    for(uint32_t i=0; i < m_multiScalerObjs.size(); i++)
    {
        /* Create multiscaler module with NULL input in case it is the first node.
         * readImage will be used to feed the nodes with frames.
         */
        if( (status == VX_SUCCESS) && (m_multiScalerObjs[i]->isHeadNode) )
        {
            status = tiovx_multi_scaler_module_create(m_ovxGraph->graph,
                                                      &m_multiScalerObjs[i]->multiScalerObj1,
                                                      NULL,
                                                      TIVX_TARGET_VPAC_MSC1);

            if( (status == VX_SUCCESS) && (m_multiScalerObjs[i]->useSecondaryMsc) )
            {
                status = tiovx_multi_scaler_module_create(m_ovxGraph->graph,
                                                          &m_multiScalerObjs[i]->multiScalerObj2,
                                                          m_multiScalerObjs[i]->multiScalerObj1.output[0].arr[0],
                                                          TIVX_TARGET_VPAC_MSC2);
            }
        }
    }

    /* Create pre process module. */
    for(uint32_t i=0; i < m_multiScalerObjs.size(); i++)
    {
        if(status == VX_SUCCESS)
        {
            /**
             * If two back to back msc is needed , in case pre-process resolution
             * is less than 1/4th original image, the original image will be
             * resized to 1/4th by the first scaler node and the remaining
             * scaling needed by pre-process will be scaled by second scaler node.
             */
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

        /* Create tidl module. */
        if(status == VX_SUCCESS)
        {
            status = tiovx_tidl_module_create(m_ovxGraph->context,
                                        m_ovxGraph->graph,
                                        &m_tidlInfObjs[i]->tidlObj,
                                        m_preProcObjs[i]->dlPreProcObj.output.arr);
        }

        /* Create post process module. */
        if(status == VX_SUCCESS)
        {
            status = tiovx_dl_post_proc_module_create(m_ovxGraph->graph,
                                            &m_postProcObjs[i]->dlPostProcObj,
                                            m_multiScalerObjs[i]->multiScalerObj1.output[1].arr[0],
                                            m_tidlInfObjs[i]->tidlObj.output, TIVX_TARGET_MPU_0);
        }
    }

    /* Create mosaic module. */
    for(uint32_t i = 0; i < m_imgMosaicObjs.size(); i++)
    {
        /* Connect appropriate post process node to its appropriate mosaic node. */
        for(uint32_t j = 0; j < postProcMosaicConn[i].size(); j++)
        {
            int32_t index = postProcMosaicConn[i][j];
            m_imgMosaicObjs[i]->mosaic_input_arr[j] = m_postProcObjs[index]->dlPostProcObj.output_image.arr[0];
        }
        status = tiovx_img_mosaic_module_create(m_ovxGraph->graph,
                                                &m_imgMosaicObjs[i]->imgMosaicObj,
                                                m_imgMosaicObjs[i]->imgMosaicObj.background_image[0],
                                                m_imgMosaicObjs[i]->mosaic_input_arr,
                                                TIVX_TARGET_VPAC_MSC1);
    }

#if !defined (SOC_AM62A)
    /* Create display module if needed. */
    if(m_displayObj != NULL)
    {
        status = tiovx_display_module_create(m_ovxGraph->graph,
                                             m_displayObj,
                                             m_imgMosaicObjs[m_dispMosaicIdx]->imgMosaicObj.output_image[0],
                                             TIVX_TARGET_DISPLAY1);
    }
#endif

    /* Add graph parameter by node index */
    graph_parameter_index = 0;
    if(m_cameraObj != NULL)
    {
        status = add_graph_parameter_by_node_index(m_ovxGraph->graph, m_cameraObj->captureObj.node, 1);
        m_cameraObj->captureObj.graph_parameter_index = graph_parameter_index;
        graph_parameters_queue_params_list[graph_parameter_index].graph_parameter_index = graph_parameter_index;
        graph_parameters_queue_params_list[graph_parameter_index].refs_list_size = m_cameraObj->captureObj.out_bufq_depth;
        graph_parameters_queue_params_list[graph_parameter_index].refs_list = (vx_reference*)&m_cameraObj->captureObj.image_arr[0];
        graph_parameter_index++;
    }
    for(uint32_t i=0; i < m_multiScalerObjs.size(); i++)
    {
        if( (status == VX_SUCCESS) && (m_multiScalerObjs[i]->isHeadNode) )
        {
            status = add_graph_parameter_by_node_index(m_ovxGraph->graph, m_multiScalerObjs[i]->multiScalerObj1.node, 0);
            m_multiScalerObjs[i]->multiScalerObj1.input.graph_parameter_index = graph_parameter_index;
            graph_parameters_queue_params_list[graph_parameter_index].graph_parameter_index = graph_parameter_index;
            graph_parameters_queue_params_list[graph_parameter_index].refs_list_size = 1;
            graph_parameters_queue_params_list[graph_parameter_index].refs_list = (vx_reference*)&m_multiScalerObjs[i]->multiScalerObj1.input.image_handle[0];
            graph_parameter_index++;
        }
    }

    for(uint32_t i = 0; i < m_imgMosaicObjs.size(); i++)
    {
        if( (i == m_dispMosaicIdx) && (m_displayObj != NULL) )
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
            graph_parameters_queue_params_list[graph_parameter_index].refs_list = (vx_reference*)&m_imgMosaicObjs[i]->imgMosaicObj.output_image[0];
            graph_parameter_index++;
        }

        if((vx_status)VX_SUCCESS == status)
        {
            /* Background image index of Mosaic Node is 2 */
            status = add_graph_parameter_by_node_index(m_ovxGraph->graph, m_imgMosaicObjs[i]->imgMosaicObj.node, 2);
            m_imgMosaicObjs[i]->imgMosaicObj.background_graph_parameter_index = graph_parameter_index;
            graph_parameters_queue_params_list[graph_parameter_index].graph_parameter_index = graph_parameter_index;
            graph_parameters_queue_params_list[graph_parameter_index].refs_list_size = 1;
            graph_parameters_queue_params_list[graph_parameter_index].refs_list = (vx_reference*)&m_imgMosaicObjs[i]->imgMosaicObj.background_image[0];
            graph_parameter_index++;
        }

    }

    /* Schedule graph */
    if((vx_status)VX_SUCCESS == status)
    {
        /* Scheduling graph in auto mode. */
        status = vxSetGraphScheduleConfig(m_ovxGraph->graph,
                    VX_GRAPH_SCHEDULE_MODE_QUEUE_AUTO,
                    graph_parameter_index,
                    graph_parameters_queue_params_list);
    }

    /* Verify graph*/
    if((vx_status)VX_SUCCESS == status)
    {
        status = vxVerifyGraph(m_ovxGraph->graph);
    }

    if(m_cameraObj != NULL)
    {
        if ( (m_cameraObj->captureObj.enable_error_detection) && (status == VX_SUCCESS) )
        {
            status = tiovx_capture_module_send_error_frame(&m_cameraObj->captureObj);
        }
    }

    for(uint32_t i=0; i < m_multiScalerObjs.size(); i++)
    {
        if((vx_status)VX_SUCCESS == status)
        {
            tiovx_multi_scaler_module_update_filter_coeffs(&m_multiScalerObjs[i]->multiScalerObj1);
            tiovx_multi_scaler_module_update_crop_params(&m_multiScalerObjs[i]->multiScalerObj1);
            if (m_multiScalerObjs[i]->useSecondaryMsc)
            {
                tiovx_multi_scaler_module_update_filter_coeffs(&m_multiScalerObjs[i]->multiScalerObj2);
                tiovx_multi_scaler_module_update_crop_params(&m_multiScalerObjs[i]->multiScalerObj2);
            }
        }
    }

    return status;
}

int32_t EdgeAIDemoImpl::startDemo()
{
    int32_t             status = 0;
    uint32_t            num_refs;
    vx_image            input_o, output_o, background_o;
    vx_object_array     input_obj_arr;
    bool                camera_first_deq_done = false;
    uint32_t            cnt = 0;

    if(m_cameraObj != NULL)
    {
        /* Start sensor module. */
        status = tiovx_sensor_module_start(&m_cameraObj->sensorObj);
        for(int32_t buf_id = 0; buf_id < m_cameraObj->captureObj.out_bufq_depth; buf_id++)
        {
            if((vx_status)VX_SUCCESS == status)
            {
                status = vxGraphParameterEnqueueReadyRef(m_ovxGraph->graph,
                                                         m_cameraObj->captureObj.graph_parameter_index,
                                                         (vx_reference*) &m_cameraObj->captureObj.image_arr[buf_id],
                                                         1);
            }
        }
    }

    /* main loop that runs every frame. */
    while(m_runLoop)
    {
        /* msc_cnt is the index of multiScaler obj in vector of objects. */
        int32_t msc_cnt = 0;
        for (auto &[name,flow] : m_config.m_flowMap)
        {
            auto const &input = m_config.m_inputMap[flow->m_inputId];

            for(uint32_t i = 0; i < flow->m_subFlowConfigs.size(); i++)
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

        /*  Enqueue graph parameters */
        if(m_cameraObj != NULL)
        {
            if(camera_first_deq_done)
            {
                vxGraphParameterEnqueueReadyRef(m_ovxGraph->graph,
                                                m_cameraObj->captureObj.graph_parameter_index,
                                                (vx_reference*) &input_obj_arr,
                                                1);
            }
        }

        for(uint32_t i = 0; i < m_multiScalerObjs.size(); i++)
        {
            if(m_multiScalerObjs[i]->isHeadNode)
            {
                vxGraphParameterEnqueueReadyRef(m_ovxGraph->graph,
                                                m_multiScalerObjs[i]->multiScalerObj1.input.graph_parameter_index,
                                                (vx_reference*) &m_multiScalerObjs[i]->multiScalerObj1.input.image_handle[0],
                                                1);
            }
        }

        for(uint32_t i = 0; i < m_imgMosaicObjs.size(); i++)
        {
            if( (i == m_dispMosaicIdx) && (m_displayObj != NULL) )
            {
                continue;
            }

            vxGraphParameterEnqueueReadyRef(m_ovxGraph->graph,
                                            m_imgMosaicObjs[i]->imgMosaicObj.output_graph_parameter_index,
                                            (vx_reference*) &m_imgMosaicObjs[i]->imgMosaicObj.output_image[0],
                                            1);

            vxGraphParameterEnqueueReadyRef(m_ovxGraph->graph,
                                            m_imgMosaicObjs[i]->imgMosaicObj.background_graph_parameter_index,
                                            (vx_reference*) &m_imgMosaicObjs[i]->imgMosaicObj.background_image[0],
                                            1);
        }

        /*  Dequeue graph parameters. */
        if(m_cameraObj != NULL)
        {
            vxGraphParameterDequeueDoneRef(m_ovxGraph->graph,
                                           m_cameraObj->captureObj.graph_parameter_index,
                                           (vx_reference*) &input_obj_arr,
                                           1,
                                           &num_refs);
            camera_first_deq_done = true;
        }

        for(uint32_t i = 0; i < m_multiScalerObjs.size(); i++)
        {
            if(m_multiScalerObjs[i]->isHeadNode)
            {
                vxGraphParameterDequeueDoneRef(m_ovxGraph->graph,
                                               m_multiScalerObjs[i]->multiScalerObj1.input.graph_parameter_index,
                                               (vx_reference*) &input_o,
                                               1,
                                               &num_refs);
            }
        }

        for(uint32_t i = 0; i < m_imgMosaicObjs.size(); i++)
        {
            if( (i == m_dispMosaicIdx) && (m_displayObj != NULL) )
            {
                continue;
            }

            vxGraphParameterDequeueDoneRef(m_ovxGraph->graph,
                                           m_imgMosaicObjs[i]->imgMosaicObj.output_graph_parameter_index,
                                           (vx_reference*) &output_o,
                                           1,
                                           &num_refs);
            vxGraphParameterDequeueDoneRef(m_ovxGraph->graph,
                                           m_imgMosaicObjs[i]->imgMosaicObj.background_graph_parameter_index,
                                           (vx_reference*) &background_o,
                                           1,
                                           &num_refs);
        }

        /*  Write Output to directory or file if needed. */
        for(uint32_t i = 0; i < m_imgMosaicObjs.size(); i++)
        {
            string sink = "";
            string sink_type = "";
            for (auto &[name,flow] : m_config.m_flowMap)
            {
                auto const &outputIds = flow->m_outputIds;
                for(uint32_t j=0; j < outputIds.size(); j++)
                {
                    OutputInfo  *output = m_config.m_outputMap[outputIds[j]];
                    if ((uint32_t)output->m_instId == i)
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
                writeImage(const_cast<char*>(sink.c_str()),
                           m_imgMosaicObjs[i]->imgMosaicObj.output_image[0]);
            }
            else if(sink_type == "images_directory")
            {
                string mosaicId = "mosaic" + to_string(i);
                string output_filename = sink +
                                         "/output_image_" +
                                         mosaicId +
                                         "_" +
                                         to_string(cnt) +
                                         ".nv12";
                writeImage(const_cast<char*>(output_filename.c_str()),
                                             m_imgMosaicObjs[i]->imgMosaicObj.output_image[0]);
            }
        }
        cnt++;

        /* If no camera input then sleep of 1 second to maintain FPS for image input */
        if(m_numCam == 0)
        {
            sleep(1);
        }
    }

    /* Stop camera sensor module. */
    if(m_cameraObj != NULL)
    {
        status = tiovx_sensor_module_stop(&m_cameraObj->sensorObj);
    }

    return status;
}

/**
 * Dumps openVX graph as dot file
 */
void EdgeAIDemoImpl::dumpGraphAsDot()
{
    tivxExportGraphToDot(m_ovxGraph->graph,
                         const_cast<vx_char*>(string(".").c_str()),
                         const_cast<vx_char*>(string("edgeai_tiovx_apps_graph").c_str()));
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

void EdgeAIDemoImpl::waitForExit()
{
}

/** Destructor. */
EdgeAIDemoImpl::~EdgeAIDemoImpl()
{
    LOG_DEBUG("EdgeAIDemoImpl Destructor \n");

    /* Delete all openVX modules */
    if(m_cameraObj != NULL)
    {
        tiovx_capture_module_delete(&m_cameraObj->captureObj);
        tiovx_viss_module_delete(&m_cameraObj->vissObj);
#if !defined (SOC_AM62A)
        tiovx_aewb_module_delete(&m_cameraObj->aewbObj);
#endif
        tiovx_ldc_module_delete(&m_cameraObj->ldcObj);
    }

#if !defined(SOC_AM62A)
    if(m_displayObj != NULL)
    {
        appGrpxDeInit();
        tiovx_display_module_delete(m_displayObj);
    }
#endif

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

    /* Deinit all openVX modules */
    if(m_cameraObj != NULL)
    {
        tiovx_sensor_module_deinit(&m_cameraObj->sensorObj);
        tiovx_capture_module_deinit(&m_cameraObj->captureObj);
        tiovx_viss_module_deinit(&m_cameraObj->vissObj);
#if !defined (SOC_AM62A)
        tiovx_aewb_module_deinit(&m_cameraObj->aewbObj);
#endif
        tiovx_ldc_module_deinit(&m_cameraObj->ldcObj);
        delete m_cameraObj;
    }

#if !defined(SOC_AM62A)
    if(m_displayObj != NULL)
    {
        tiovx_display_module_deinit(m_displayObj);
        delete m_displayObj;
    }
#endif

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

    /**
     * Delete application openVX object
     * Also release context in its destructor
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

void EdgeAIDemo::waitForExit()
{
    m_impl->waitForExit();
}

} /* namespace ti::edgeai::common */