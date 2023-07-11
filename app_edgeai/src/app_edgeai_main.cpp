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

/* Standard headers */
#include <signal.h>
#include <stdlib.h>
#include <string>

/* Module headers */
#include <common/include/edgeai_cmd_line_parse.h>
#include <common/include/edgeai_demo.h>

extern "C" 
{
#include <app_init.h>
#include <tiovx_utils.h>
}

using namespace ti::edgeai::common;

static EdgeAIDemo *gDemo = nullptr;

static void sigHandler(int32_t sig)
{
    (void)sig;

    if (gDemo)
    {
        gDemo->sendExitSignal();
    }
}

int main(int32_t argc, char * argv[])
{
    int32_t status = appInit();
    if(status == -1)
    {
        LOG_ERROR("appInit failed \n");
        exit(-1);
    }

    CmdlineArgs cmdArgs;

    /* Register SIGINT handler. */
    signal(SIGINT, sigHandler);

    /* Parse the command line options. */
    cmdArgs.parse(argc, argv);

    /* Parse the input configuration file. */
    const YAML::Node &yaml = YAML::LoadFile(cmdArgs.configFile);

    gDemo = new EdgeAIDemo(yaml);

    /* Dump openVX graph as dot file. */
    if(cmdArgs.dumpDot)
    {
        gDemo->dumpGraphAsDot();
    }

    /* Start the graph. */
    gDemo->startDemo();

    /* Wait for the threads to exit. */
    gDemo->waitForExit();

    delete gDemo;

    appDeInit();

    return 0;
}