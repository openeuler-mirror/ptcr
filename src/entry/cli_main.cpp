/******************************************************************************
 * Copyright (c) KylinSoft  Co., Ltd. 2021. All rights reserved.
 * ptcr licensed under the Mulan PSL v2.

 * You can use this software according to the terms and conditions of the Mulan PSL v2.
 * You may obtain a copy of Mulan PSL v2 at:
 *     http://license.coscl.org.cn/MulanPSL2
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY OR FIT FOR A PARTICULAR
 * PURPOSE.
 * See the Mulan PSL v2 for more details.
 * Author: xiapin
 * Create: 2021-11-29
 * Description: provide ptcr entry functions.
 ******************************************************************************/
#include "utils.h"
#include "config.h"
#include "measure.h"
#include "logger.h"
#include <getopt.h>
#include <iostream>

static const char *short_opts = "hc:o:l:s:p:";

static void ShowHelp(const char *self)
{
    printf("usage: %s [<flags>]\n\n"
           "-c,--config\t\tSet configuration file path.\n"
           "-o,--output\t\tSet report file path.\n"
           "-l,--loglever\t\tOnly log messages with the given severity or above. One of: [DEBUG, INFO, WARN, ERROR]\n"
           "-s,--sCount\t\tSpecifies the number of serial tests\n"
           "-p,--pCount\t\tSpecifies the number of parallel tests\n", self);
}

int ParserArgs(int argc, char **argv, OptArgs_T *optArgs)
{
    NULL_PTR_CHECK(optArgs, RET_INVALID_INPUT_PARAM);

    int opt;
    int option_index = 0;

    struct option long_options[] = {
        {"help", no_argument, NULL, 'h'},
        {"config", optional_argument, NULL, 'c'},
        {"output", optional_argument, NULL, 'o'},
        {"loglever", optional_argument, NULL, 'l'},
        {"sCount", optional_argument, NULL, 's'},
        {"pCount", optional_argument, NULL, 'p'},
        {0, 0, 0, 0}
    };

    while ((opt = getopt_long(argc, argv, short_opts, long_options, &option_index)) != -1) {
        switch (opt) {
            case 'c':
                optArgs->configPath = optarg;
                break;
            case 'o':
                optArgs->outputFile = optarg;
                break;
            case 'l':
                optArgs->logLever = optarg;
                break;
            case 's':
                optArgs->seriallyCount = optarg;
                break;
            case 'p':
                optArgs->parallyCount = optarg;
                break;
            case 'h':
            default:
                ShowHelp(argv[0]);
                return -1;
        }
    }

    return 0;
}

int main(int argc, char **argv)
{
    /* Use new/delete so that OptArgs_T's std::string members run their
     * destructors and release their internal buffers. The previous UTILS_CALLOC
     * + free(optArgs) combination skipped destructors and leaked. */
    OptArgs_T *optArgs = new OptArgs_T();

    optArgs->configPath = "/etc/ptcr/ptcr.yml";
    if (argc > 1 && ParserArgs(argc, argv, optArgs)) {
        delete optArgs;
        return -1;
    }

    MeasureConfigCls config(optArgs->configPath);
    if (config.Init()) {
        LOG_ERROR("Config Init failed, check if config file exit.\n");
        delete optArgs;
        return -1;
    }
    config.ConfigOptArgs(optArgs);

    set_log_lever(LOG_LERVER_E(config.m_logLever));

    measureCls msCls;
    msCls.startMeasure(&config);
    delete optArgs;

    return 0;
}
