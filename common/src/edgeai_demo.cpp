/*
 *  Copyright (C) 2021 Texas Instruments Incorporated - http://www.ti.com/
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
// #include <common/include/edgeai_utils.h>
#include <common/include/edgeai_demo_config.h>
#include <common/include/edgeai_demo.h>

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
        // ovxGraph                            *m_ovxGraph{nullptr};

        /** Demo configuration. */
        DemoConfig                          m_config;

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

    /* Create a set of models to instantiate from the flows. */
    set<string> modelSet;

    for (auto const &[name,flow] : m_config.m_flowMap)
    {
        auto const &modelIds = flow->m_modelIds;

        modelSet.insert(modelIds.begin(), modelIds.end());
    }

    /* Create the instances of the set of models. */
    for (auto const &mId: modelSet)
    {
        ModelInfo  *model = m_config.m_modelMap[mId];
    }

    /* Setup the flows. By this time all the relavant model contexts have been
     * setup.
     */
    set<string>             outputSet;

    for (auto &[name,flow] : m_config.m_flowMap)
    {
        flow->initialize(m_config.m_modelMap,
                         m_config.m_inputMap,
                         m_config.m_outputMap);

        /* Collect the GST strings from each flow, concatenate them, and create
         * the toplevel GST string.
         */
        auto const &input = m_config.m_inputMap[flow->m_inputId];
        // input->getSrcPipelines(srcPipelines, srcElemNames);

        /* Update the output set. */
        auto const &outputIds = flow->m_outputIds;
        outputSet.insert(outputIds.begin(), outputIds.end());
    }

    // for (auto const &oId: outputSet)
    // {
    //     auto const &output = m_config.m_outputMap[oId];
    //     // output->appendGstPipeline();
    // }

    // for (auto &[name,flow] : m_config.m_flowMap)
    // {
    //     // flow->getSinkPipeline(sinkPipeline, sinkElemNames);
    // }
         
    /* Instantiate the GST pipe. */
    // m_gstPipe = new GstPipe(srcPipelines, sinkPipeline, srcElemNames, sinkElemNames);

    /* Start GST Pipelines. */
    // status = m_gstPipe->startPipeline();

    // if (status < 0)
    // {
    //      LOG_ERROR("Failed to start GST pipelines.\n");
    // }
    // else
    // {
    //     /* Loop through each flow, and start the inference pipes. */
    //     for (auto &[name, flow] : m_config.m_flowMap)
    //     {
    //         status = flow->start(m_gstPipe);

    //         if (status < 0)
    //         {
    //             LOG_ERROR("Flow start failed.\n");
    //             break;
    //         }
    //     }
    // }
    return status;
}

/**
 * Dumps openVX graph as dot file
 */
void EdgeAIDemoImpl::dumpGraphAsDot()
{

}

/**
 * Sets a flag for the threads to exit at the next opportunity.
 * This controls the stopping all threads, stop the openVX graph
 * and other cleanup.
 */
void EdgeAIDemoImpl::sendExitSignal()
{
    // for (auto &[name, flow] : m_config.m_flowMap)
    // {
    //     flow->sendExitSignal();
    // }
}

/**
 * This is a blocking call that waits for all the internal threads to exit.
 */
void EdgeAIDemoImpl::waitForExit()
{
    // for (auto &[name, flow] : m_config.m_flowMap)
    // {
    //     flow->waitForExit();
    // }
}

/** Destructor. */
EdgeAIDemoImpl::~EdgeAIDemoImpl()
{
}

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

