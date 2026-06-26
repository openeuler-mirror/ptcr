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
 * Description: provide ptcr configure read.
 ******************************************************************************/
#include "config.h"
#include <string.h>
#include "logger.h"
#include "utils.h"

using namespace std;

std::string MeasureConfigCls::GetNodeKeyValue(const std::string key, std::string deflt)
{
    string str;
    try {
        str = m_node[key].as<string>();
    } catch (...) {
        str = deflt;
    }

    return str;
}

int MeasureConfigCls::GetNodeKeyValue(std::string key, int deflt)
{
    int value;
    try {
        value = m_node[key].as<int>();
    } catch (...) {
        value = deflt;
    }

    return value;
}

int MeasureConfigCls::GetStringVector(std::string key, std::vector<std::string> &strVect)
{
    strVect.clear();

    try {
        for (auto item : m_node[key]) {
            strVect.push_back(item.as<std::string>());
        }
    } catch (...) {
        return -1;
    }

    return 0;
}

std::string MeasureConfigCls::GetNodeKeyValue(const std::string section, std::string key, std::string deflt)
{
    string str;
    try {
        str = m_node[section][key].as<string>();
    } catch (...) {
        str = deflt;
    }

    return str;
}

int MeasureConfigCls::GetNodeKeyValue(std::string section, std::string key, int deflt)
{
    int value;
    try {
        value = m_node[section][key].as<int>();
    } catch (...) {
        value = deflt;
    }

    return value;
}

MeasureConfigCls::MeasureConfigCls(std::string &configPath) : m_cfgPath(configPath)
{
    m_isCmdMixed            = 0;
    m_timeOut               = 1;
}

int MeasureConfigCls::GetMeasureWay()
{
    Measure_Way_T      *msSerially;
    Measure_Way_T      *msParallerlly;

    msSerially = (Measure_Way_T *)UTILS_CALLOC(sizeof(Measure_Way_T), RET_OUT_OF_MEMORY);
    msParallerlly = (Measure_Way_T *)UTILS_CALLOC(sizeof(Measure_Way_T), goto FAILURE);

    msSerially->measureType = MEASURE_SERIALLY;
    msSerially->measureCnt = GetNodeKeyValue("measure_count", "serially", 10);
    m_measureWayVect.push_back(msSerially);

    msParallerlly->measureType = MEASURE_PARALLERL;
    msParallerlly->measureCnt = GetNodeKeyValue("measure_count", "parallerlly", 10);
    m_measureWayVect.push_back(msParallerlly);

    return 0;
FAILURE:
    free(msSerially);
    RET_OUT_OF_MEMORY;
}

int MeasureConfigCls::Init()
{
    try {
        m_node = YAML::LoadFile(m_cfgPath);
    } catch (YAML::BadFile &e) {
        LOG_ERROR("config file %s not exit\n", m_cfgPath.c_str());
        return -1;
    }

    if (GetMeasureWay()) {
        LOG_ERROR("Get measure count error!\n");
        return -1;
    }
    m_logLever = (LOG_LERVER_E)GetNodeKeyValue("log_lever", 3);
    m_outputFile = GetNodeKeyValue("out_put_file", "./ptcr_result.txt");
    m_timeOut = GetNodeKeyValue("time_out", 1);
    m_imageName = GetNodeKeyValue("image_name", "busybox");
    m_isCmdMixed = (bool)GetNodeKeyValue("mixed_cmd", true);
    m_cntMemory = GetNodeKeyValue("memory_shim_count", 10);

    GetStringVector("daemon_name", m_daemonName);
    GetStringVector("runtime_names", m_runtimeName);
    GetStringVector("runtime_endpoint", m_runtimeEndpoint);
    GetStringVector("start_cmd", m_runContCmd);

    return 0;
}

void MeasureConfigCls::Deinit()
{
    for (auto item : m_measureWayVect) {
        free(item);
    }
    m_measureWayVect.clear();
}

static LOG_LERVER_E OptArgToLoglever(string &optarg)
{
    if (optarg.compare("DEBUG") == 0) {
        return L_DEBUG;
    } else if (optarg.compare("INFO") == 0) {
        return L_INFO;
    } else if (optarg.compare("ERROR") == 0) {
        return L_ERROR;
    } else if (optarg.compare("FATAL") == 0) {
        return L_FATAL;
    }

    return L_ERROR;
}

int MeasureConfigCls::ConfigOptArgs(OptArgs_T *optArgs)
{
    NULL_PTR_CHECK(optArgs, RET_INVALID_INPUT_PARAM);

    if (!optArgs->outputFile.empty()) {
        m_outputFile = optArgs->outputFile;
    }

    if (!optArgs->logLever.empty()) {
        m_logLever = OptArgToLoglever(optArgs->logLever);
    }

    for (auto item : m_measureWayVect) {
        if (item->measureType == MEASURE_SERIALLY && !optArgs->seriallyCount.empty()) {
            item->measureCnt = atoi(optArgs->seriallyCount.c_str());
        } else if (item->measureType == MEASURE_PARALLERL && !optArgs->parallyCount.empty()) {
            item->measureCnt = atoi(optArgs->parallyCount.c_str());
        }
    }
    return 0;
}
