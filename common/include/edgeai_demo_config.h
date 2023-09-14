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
#ifndef _TI_EDGEAI_TIOVX_DEMO_CONFIG_H_
#define _TI_EDGEAI_TIOVX_DEMO_CONFIG_H_

/* Standard headers. */
#include <map>
#include <set>

/* Third-party headers. */
#include <yaml-cpp/yaml.h>

/**
 * \defgroup group_edgeai_demo_config Demo configuration processing.
 *
 * \brief Core demo configuration logic for parsing the configuration
 *        YAML file and setting up complex data flows.
 *
 * \ingroup group_edgeai_common
 */

namespace ti::edgeai::common
{
#define DEMO_CONFIG_APPSINK_BUFF_DEPTH  (2)

    using namespace std;
    using namespace ti::edgeai::common;

    /**
     * \brief Structure for holding parameters for specifying pipeline flows.
     *        Multiple SubFlows can be there in one data flow, where a flow
     *        is defined as data flowing through {pre-processing, inference,
     *        post-processing}.
     *
     * \ingroup group_edgeai_demo_config
     */
    struct SubFlowConfig
    {
        string                 model;
        string                 output;
        vector<int32_t>        mosaic_info;
    };

    /**
     * \brief Structure used to initailize FlowInfo.
     *
     * \ingroup group_edgeai_demo_config
     */
    struct FlowConfig
    {
        string                input;
        vector<SubFlowConfig> subflow_configs;
    };

    /* Forward declaration. */
    class OutputInfo;

    /**
     * \brief Class for holding mosaic parameters for constructing the output.
     *
     * \ingroup group_edgeai_demo_config
     */
    class MosaicInfo
    {
        public:
            /** Default constructor. Use the compiler generated default one. */
            MosaicInfo();

            /** Constructor.
             *
             * @param data Integer Vector containing posX,posY,width,height
             */
            MosaicInfo(vector<int32_t> &data);

            /** Destructor. */
            ~MosaicInfo();

            /**
             * Helper function to dump the configuration information.
             *
             * @param prefix Prefix to be added to the log outputs.
             */
            void dumpInfo(const char *prefix="") const;

        public:
            /** Post-process output width. */
            int32_t     m_width{};

            /** Post-process output height. */
            int32_t     m_height{};

            /** X-coordinate to place the window. */
            int32_t     m_posX{};

            /** Y-coordinate to place the window. */
            int32_t     m_posY{};

            /** True if mosaic is enabled, False if this need full output. */
            bool        m_mosaicEnabled{true};

            /** Output context this mosaic is tied to. */
            OutputInfo *m_outputInfo{nullptr};
    };

    /**
     * \brief Class for holding input parameters for setting up a openVX
     *        graph.
     *
     * \ingroup group_edgeai_demo_config
     */
    class InputInfo
    {
        public:
            /** Default constructor. Use the compiler generated default one. */
            InputInfo() = default;

            /** Constructor.
             *
             * @param node Parsed YAML configuration with input information.
             */
            InputInfo(const YAML::Node &node);

            /** Destructor. */
            ~InputInfo();

            /**
             * Helper function to dump the configuration information.
             *
             * @param prefix Prefix to be added to the log outputs.
             */
            void dumpInfo(const char *prefix="") const;

        public:
            /** Allowed sources of the input data. Allowed values
             * - /dev/videoX             [Camera]
             * - *%Nd*.jpg   [Image]
             * - *%Nd*.png   [Image]
             * - *.mp4       [Video]
             * - *.avi       [Video]
             *
             *ex:- ../c/d/input_image%02d.jpg
             *     /a/b/c/d/input_image%02d.png
             *     /a/b/c/d/input_video.mp4
             *     /a/b/c/d/input_video.avi
             *
             * The paths to images and videos can be absolute or relative to the directoty
             * the application is launched from.
             *
             * Default is /dev/video2
             */
            string                              m_source;

            /** Vector to store names of image files in case the 
             *  input is directory containing multiple images 
             */
            vector<string>                      m_multiImageVect{};

            /** Counter for indexing  m_multiImageVect. */
            uint32_t                            m_multiImageVectCnt{0};

            /* Unique name assigned to input in config file. */
            string                              m_name;

            /** Source type derived from m_source. This is used for various
             * purposes including debug logging so to avoid deriving it
             * multiple times, it is done once during initialization and
             * stored for later use.
             */
            string                              m_srcType{"image"};

            /* Channel Id for camera input from fusion board */
            int32_t                             m_cameraId{0};

            /** Path used for providing VISS DCC file path */
            string                              m_vissDccPath{""};

            /** Path used for providing LDC DCC file path */
            string                              m_ldcDccPath{""};

            /** Input data width. */
            int32_t                             m_width{};

            /** Input data height. */
            int32_t                             m_height{};

            /** Loop file input after EOF. */
            bool                                m_loop{true};

            /** Frame rate. */
            string                              m_framerate{};

            /** Instance Id for a specific instance of inputs. */
            int32_t                             m_instId;

            /** member for tracking the instances of this class. */
            static int32_t                      m_numInstances;

            /** if ldc required. */
            bool                                m_ldc{false};

            /** Sensor ID for raw sensor. */
            string                              m_sen_id{"imx390"};

            /** VPAC ID */
            uint8_t                             m_vpac_id{1};
    };

    /**
     * \brief Class for holding output parameters for setting up a openVX
     *        graph.
     *
     * \ingroup group_edgeai_demo_config
     */
    class OutputInfo
    {
        using MosaicInfoMap = map<int32_t, const MosaicInfo*>;

        public:
            /** Default constructor. Use the compiler generated default one. */
            OutputInfo() = default;

            /** Constructor.
             *
             * @param node Parsed YAML configuration with output information.
             * @param title String to be overlaid over the output buffer.
             */
            OutputInfo(const YAML::Node    &node,
                       const string        &title);

            /** Destructor. */
            ~OutputInfo();

            /** Function for registering the streams generating the display
             * mosaics.
             *
             * This function stores the mosaic information and returns an
             * identifier to be used later by the caller when pushing the
             * data to display, using the process() function.
             *
             * @param mosaicInfo Mosaic parameters
             * @param modelTitle The title to be displayed for this inference 
             * output
             */
            int32_t registerDispParams(const MosaicInfo    *mosaicInfo, 
                                       const string        &modelTitle);

            /**
             * Helper function to dump the configuration information.
             *
             * @param prefix Prefix to be added to the log outputs.
             */
            void dumpInfo(const char *prefix="") const;

        public:
            /** Title to be overlaid in the output buffer. */
            string              m_title;

            /** Allowed sink for the output. Allowed values
             * - Display     [Display]
             * - *%Nd*.jpg   [Image]
             * - *%Nd*.png   [Image]
             * - *.mp4       [Video]
             * - *.avi       [Video]
             *
             *ex:- ../c/d/output_image%02d.jpg
             *      /a/b/c/d/output_image%02d.png
             *      /a/b/c/d/output_video.mp4
             *      /a/b/c/d/output_video.avi
             *
             * The paths to images and videos can be absolute or relative to the directoty
             * the application is launched from.
             *
             * Default is Display
             */
            string                          m_sink;

            /** Sink type derived from m_sink. */
            string                          m_sinkType{"image"};

            /** Output display width. */
            int32_t                         m_width{};

            /** Output display height. */
            int32_t                         m_height{};

            /** Connector id to select output display.
             * This field is ignored for sinks other than Display
             */
            int32_t                         m_connector{};

            /** Instance Id for a specific instance of output. */
            int32_t                         m_instId;

            /** member for tracking the instances of this class. */
            static int32_t                  m_numInstances;

            /* Map of the inference pipe to the associated mosaic information
             * for output handling.
             */
            MosaicInfoMap                   m_instIdMosaicMap;

            /* Map of the inference pipe to the associated model title string
             */
            vector<string>                  m_titleMap;

            /** Id for referencing entries in the mosaic info map. */
            int32_t                         m_mosaicCnt{0};

            /** Set if mosaic is enabled */
            bool                            m_mosaicEnabled{true};
    };

    /**
     * \brief Class for holding model parameters.
     *
     * \ingroup group_edgeai_demo_config
     */
    class ModelInfo
    {
        public:
            /** Default constructor. Use the compiler generated default one. */
            ModelInfo() = default;

            /** Constructor.
             *
             * @param node Parsed YAML configuration with model information.
             */
            ModelInfo(const YAML::Node &node);

            /** Destructor. */
            ~ModelInfo();

            /**
             * Helper function to dump the configuration information.
             *
             * @param prefix Prefix to be added to the log outputs.
             */
            void dumpInfo(const char *prefix="") const;

        public:
            /** Path to the model. */
            string                  m_modelPath;

            /** Alpha value used for blending the sementic segmentation output. */
            float                   m_alpha{0.5f};

            /** Threshold for visualizing the output from the detection models. */
            float                   m_vizThreshold{0.5f};

            /** Number of classification results to pick from the top of the model output. */
            int32_t                 m_topN{5};
    };

    /**
     * \brief Class for holding parameters for specifying pipeline flows. This
     *        class can handle more than one data flow, where a flow is defined
     *        as data flowing through {pre-processing, inference,
     *        post-processing}. Flow will be characterized by a unique input.
     *        There can be multiple sub flows in a single flow, differentiated by
     *        each unique model.
     *        Eg:
     *        flow0: [input2,model0,output0]
     *        flow1: [input2,model1,output2]
     *        flow2: [input0,model0,output2]
     *        There are two flows in above example, one corresponds to input2
     *        and other to input0. And there are two subflows in flow
     *        corresponding to input2, one for model0 and other for model1,
     *        whereas the flow with input0, just have single subflow.
     *
     * \ingroup group_edgeai_demo_config
     */
    class FlowInfo
    {
        public:
            /** Default constructor. Use the compiler generated default one. */
            FlowInfo() = default;

            /** Constructor.
             *
             * @param flowConfig FlowConfig Structure.
             */
            FlowInfo(FlowConfig &flowConfig);

            /** Destructor. */
            ~FlowInfo();

            int32_t initialize(map<string, ModelInfo*>   &modelMap,
                               map<string, InputInfo*>   &inputMap,
                               map<string, OutputInfo*>  &outputMap,
                               int32_t                   m_numCam);

            /**
             * Helper function to dump the configuration information.
             *
             * @param prefix Prefix to be added to the log outputs.
             */
            void dumpInfo(const char *prefix="") const;

        public:
            /** Source. */
            string                              m_inputId;
            
            /** List of all SubFlow Configs. */
            vector<SubFlowConfig>               m_subFlowConfigs;

            /** List of models using this input source. */
            vector<string>                      m_modelIds;

            /** List of outputs. */
            vector<string>                      m_outputIds;

            /** List of display information. */
            vector<MosaicInfo*>                 m_mosaicVec;

            /** List of output information. */
            map<string,vector<OutputInfo*>>     m_outputMap;

            /** List of Dimension for sensor paths of flow. */
            vector<vector<int32_t>>             m_mosaicInfoVec;

            /** Input frame rate. */
            string                              m_framerate{0};

            /** Instance Id for a specific instance. */
            int32_t                             m_instId;

            /** member for tracking the instances of this class. */
            static int32_t                      m_numInstances;
    };

    /**
     * \brief Demo configuration information. This information does not come
     *        from the YAML specification used for providing critical
     *        parameters for the pre/inference/post processing stages. The
     *        fields provided through this is very application specific.
     *
     * \ingroup group_edgeai_demo_config
     */
    class DemoConfig
    {
        public:
            /** Constructor. */
            DemoConfig();

            /** Function for parsing the demo configuration
             *
             * @param yaml Parsed YAML configuration with configuration information.
             */
            int32_t parse(const YAML::Node &yaml);

            /** Destructor. */
            ~DemoConfig();

            /** Dump configuration. */
            void dumpInfo() const;

        public:
            /** Map of all inputs defined. */
            map<string, InputInfo*>     m_inputMap;

            /** Map of all models defined. */
            map<string, ModelInfo*>     m_modelMap;

            /** Map of all outputs defined. */
            map<string, OutputInfo*>    m_outputMap;

            /** Map of all flows defined. */
            map<string, FlowInfo*>      m_flowMap;

            /** Title to be overlaid with display. */
            string                      m_title;

        private:
            /** Function for parsing the flow information.
             */
            int32_t parseFlowInfo(const YAML::Node &config);

        private:
            /** Vector of input order. */
            vector<string>              m_inputOrder;

    };

} /* namespace ti::edgeai::common */

#endif /* _TI_EDGEAI_TIOVX_DEMO_CONFIG_H_ */

