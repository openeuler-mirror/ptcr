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
 * Description: measure by client implement.
 ******************************************************************************/
#include "cli_wrap.h"
#include "utils.h"

int cliWrapperCls::pullImage(string &imageName)
{
    const char *argv[] = {m_cliName.c_str(), "pull", imageName.c_str(), NULL};

    int ret = utils_execute_process(m_cliName.c_str(), (char * const *)argv);
    LOG_DEBUG("%s pull %s, ret:%d\n", m_cliName.c_str(), imageName.c_str(), ret);

    return ret;
}

int cliWrapperCls::removeImage(string &imageName)
{
    const char *argv[] = {m_cliName.c_str(), "rmi", imageName.c_str(), NULL};

    int ret = utils_execute_process(m_cliName.c_str(), (char * const *)argv);
    LOG_DEBUG("%s remove image %s, ret:%d\n", m_cliName.c_str(), imageName.c_str(), ret);

    return 0;
}

int cliWrapperCls::createContainer(string &imageName, string *contStr)
{
    char *cont_id = utils_generate_random_str(4);
    const char *argv[] = {m_cliName.c_str(), "create", "--name", cont_id, imageName.c_str(), NULL};

    int ret = utils_execute_process(m_cliName.c_str(), (char * const *)argv);
    *contStr = cont_id;
    free(cont_id);

    LOG_DEBUG("%s create cont %s witch image %s, ret:%d\n", m_cliName.c_str(), (*contStr).c_str(), imageName.c_str(), ret);

    return 0;
}

int cliWrapperCls::startContainer(string &contStr)
{
    const char *argv[] = {m_cliName.c_str(), "start", contStr.c_str(), NULL};

    LOG_DEBUG("%s start cont %s\n", m_cliName.c_str(), contStr.c_str());
    return utils_execute_process(m_cliName.c_str(), (char * const *)argv);
}

int cliWrapperCls::runContainer(string &imageName, vector<string> &cmd, string *contStr)
{
    int i = 6;
    int argc = cmd.size() + 7;
    char *cont_id = utils_generate_random_str(4);
    NULL_PTR_CHECK(cont_id, RET_OUT_OF_MEMORY);

    char **argv = (char **)UTILS_CALLOC(sizeof(char *) * argc, ERR_FAILURE);
    NULL_PTR_CHECK(argv, RET_OUT_OF_MEMORY);

    argv[0] = (char *)m_cliName.c_str();
    argv[1] = (char *)"run";
    argv[2] = (char *)"--name";
    argv[3] = (char *)cont_id;
    argv[4] = (char *)"-d";
    argv[5] = (char *)imageName.c_str();
    for (auto item : cmd) {
        argv[i++] = strdup(item.c_str());
    }
    argv[i] = NULL;

    int ret = utils_execute_process(m_cliName.c_str(), (char * const *)argv);
    *contStr = cont_id;
    free(argv);
    free(cont_id);

    LOG_DEBUG("run cont %s ret:%d\n", (*contStr).c_str(), ret);

    return ret;
}

int cliWrapperCls::stopContainer(string contId, int timeOut)
{
    int ret;
    char cTimeOut[12] = {0};

    snprintf(cTimeOut, sizeof(cTimeOut), "%d", timeOut);

    if (timeOut > 0) {
        const char *argv[] = {m_cliName.c_str(), "stop", "--time", cTimeOut, contId.c_str(), NULL};
        ret = utils_execute_process(m_cliName.c_str(), (char * const *)argv);
    } else {
        const char *argv[] = {m_cliName.c_str(), "stop", "-f", contId.c_str(), NULL};
        ret = utils_execute_process(m_cliName.c_str(), (char * const *)argv);
    }

    LOG_DEBUG("stop cont %s ret:%d\n", contId.c_str(), ret);

    return ret;
}

int cliWrapperCls::stopAllContainer()
{
    return 0;
}

int cliWrapperCls::rmContainer(string ContId)
{
    const char *argv[] = {m_cliName.c_str(), "rm", "-f", ContId.c_str(), NULL};
    int ret = utils_execute_process(m_cliName.c_str(), (char * const *)argv);

    LOG_DEBUG("rmContainer cont %s ret:%d\n", ContId.c_str(), ret);

    return ret;
}
