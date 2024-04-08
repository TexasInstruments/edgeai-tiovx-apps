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

#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <yaml_parser.h>

#include <TI/tivx.h>
#include <app_init.h>

#include <apps/include/info.h>
#include <apps/include/app.h>

int main(int argc, char *argv[])
{
    int32_t     status = 0;
    FlowInfo    flow_infos[MAX_FLOWS];
    uint32_t    num_flows = 0;
    CmdArgs     cmd_args;

    /* Initialize cmd_args */
    cmd_args.verbose = false;
    cmd_args.dump_dot = false;

    int32_t long_index;
    int32_t opt;
    static struct option long_options[] =
    {
        {"help",      no_argument,       0, 'h' },
        {"verbose",   no_argument,       0, 'v' },
        {"dump",      no_argument,       0, 'd' },
        {0,           0,                 0,  0  }
    };

    while ((opt = getopt_long(argc, argv,"-hvdl:",
                   long_options, &long_index )) != -1)
    {
        switch (opt)
        {
            case 1 :
                sprintf(cmd_args.config_file, optarg);
                break;
            case 'v' :
                cmd_args.verbose = true;
                break;
            case 'd' :
                cmd_args.dump_dot = true;
                break;
            case 'h' :
            default:
                printf("# \n");
                printf("# %s PARAMETERS [OPTIONAL PARAMETERS]\n", argv[0]);
                printf("# POSITIONAL PARAMETERS:\n");
                printf("#  config_file - Path to the configuration file.\n");
                printf("# OPTIONAL PARAMETERS:\n");
                printf("#  [--verbose    |-v]\n");
                printf("#  [--dump       |-d]\n");
                printf("#  [--help       |-h]\n");
                printf("# \n");
                printf("# (C) Texas Instruments 2024\n");
                printf("# \n");
                printf("# EXAMPLE:\n");
                printf("#    %s configs/linux/object_detection.yaml\n", argv[0]);
                printf("# \n");
                exit(0);
        }
    }

    status = parse_yaml_file(cmd_args.config_file,
                             flow_infos,
                             MAX_FLOWS,
                             &num_flows);
    if (0 != status)
    {
        TIOVX_APPS_ERROR("Could not parse %s.\n", cmd_args.config_file);
        return status;
    }

    status = appInit();

    status = run_app(flow_infos, num_flows, &cmd_args);

    appDeInit();

    return status;
}
