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
 * Description: wrapper class function implement.
 ******************************************************************************/
#include "wrap.h"
#include <iostream>
#include "sys/time.h"
#include "cli_wrap.h"
#include "api_wrap.h"
#include "utils.h"
#include "container_manager.h"

using namespace std;

wrapperManager::wrapperManager(CONT_RT_CONNECT_TYPE connectType, string &endPoint)
    : m_endPoint(endPoint)
{
    m_connectType = connectType;
    m_wrapOps = NULL;
}

wrapperManager::~wrapperManager()
{
}

int wrapperManager::init()
{
    int ret = 0;
    switch (m_connectType) {
        case CONNECT_BY_CLI:
            m_wrapOps = new cliWrapperCls(m_endPoint);
            break;
        case CONNECT_BY_CRI:
#ifdef CRI_CLIENT
            m_wrapOps = new ApiWrapperCls(m_endPoint);
#else
            LOG_ERROR("CRI client not open, use cmake -DENABLE_CRI_CLIENT=ON open first!\n");
            ret = -1;
#endif
            break;
        case CONNECT_BY_REST:
            m_wrapOps = NULL; // TODO;
            break;
        default:
            m_wrapOps = NULL; // TODO;
            break;
    }
    return ret;
}

void wrapperManager::Deinit()
{
    if (m_wrapOps != NULL) {
        delete m_wrapOps;
        m_wrapOps = NULL;
    }
}

int wrapperManager::pullImage(string &imageName)
{
    int ret = -1;

    if (m_wrapOps != NULL) {
        ret = m_wrapOps->pullImage(imageName);
    }

    return ret;
}

int wrapperManager::removeImage(string &imageName)
{
    int ret = -1;

    if (m_wrapOps != NULL) {
        ret = m_wrapOps->removeImage(imageName);
    }
    return ret;
}

int wrapperManager::createContainer(string &imageName, string *contStr)
{
    int ret = 0;

    if (m_wrapOps != NULL) {
        ret = m_wrapOps->createContainer(imageName, contStr);
    }
    return ret;
}

int wrapperManager::startContainer(string &contStr)
{
    int ret = 0;

    if (m_wrapOps != NULL) {
        ret = m_wrapOps->startContainer(contStr);
    }

    return ret;
}

int wrapperManager::runContainer(string &imageName, vector<string> &cmd, string *contStr)
{
    int ret = 0;

    if (m_wrapOps != NULL) {
        ret = m_wrapOps->runContainer(imageName, cmd, contStr);
    }
    return ret;
}

int wrapperManager::stopContainer(string contId, int timeOut)
{
    int ret = -1;

    if (m_wrapOps != NULL) {
        ret = m_wrapOps->stopContainer(contId, timeOut);
    }
    return ret;
}

int wrapperManager::stopAllContainer()
{
    int ret = -1;

    if (m_wrapOps != NULL) {
        ret = m_wrapOps->stopAllContainer();
    }
    return ret;
}

int wrapperManager::rmContainer(string ContId)
{
    int ret = -1;

    if (m_wrapOps != NULL) {
        ret = m_wrapOps->rmContainer(ContId);
    }
    return ret;
}

int MeasureCreateInterface(Create_Cont_Args_T *CreateArgs)
{
    struct timeval before, now;

    NULL_PTR_CHECK(CreateArgs, RET_INVALID_INPUT_PARAM);
    NULL_PTR_CHECK(CreateArgs->wm, RET_INVALID_INPUT_PARAM);
    NULL_PTR_CHECK(CreateArgs->spentUs, RET_INVALID_INPUT_PARAM);

    gettimeofday(&before, NULL);
    CreateArgs->retVal = CreateArgs->wm->createContainer(CreateArgs->imageName, CreateArgs->ContID);
    gettimeofday(&now, NULL);

    if (CreateArgs->retVal != 0) {
        LOG_ERROR("Create Container failed!\n");
    } else if (CreateArgs->bSave && !(*CreateArgs->ContID).empty()) {
        WrapContInfoCls::GetInstance()->ContBindWrapper(CreateArgs->ContID, STAT_CREATED, CreateArgs->wm);
    }
    *(CreateArgs->spentUs) = (now.tv_usec - before.tv_usec) + (now.tv_sec - before.tv_sec) * 1000000;

    return 0;
}

int MeasureStartInterface(Start_Cont_Args_T *startArgs)
{
    struct timeval before, now;

    NULL_PTR_CHECK(startArgs, RET_INVALID_INPUT_PARAM);
    NULL_PTR_CHECK(startArgs->wm, RET_INVALID_INPUT_PARAM);
    NULL_PTR_CHECK(startArgs->spentUs, RET_INVALID_INPUT_PARAM);

    gettimeofday(&before, NULL);
    startArgs->retVal = startArgs->wm->startContainer(*startArgs->contID);
    gettimeofday(&now, NULL);

    *(startArgs->spentUs) = (now.tv_usec - before.tv_usec) + (now.tv_sec - before.tv_sec) * 1000000;

    return 0;
}

int MeasureStopInterface(Stop_Cont_Args_T *stopArgs)
{
    struct timeval before, now;

    NULL_PTR_CHECK(stopArgs, RET_INVALID_INPUT_PARAM);
    NULL_PTR_CHECK(stopArgs->wm, RET_INVALID_INPUT_PARAM);
    NULL_PTR_CHECK(stopArgs->spentUs, RET_INVALID_INPUT_PARAM);

    gettimeofday(&before, NULL);
    stopArgs->retVal = stopArgs->wm->stopContainer(*stopArgs->contID, stopArgs->timeOut);
    gettimeofday(&now, NULL);

    *(stopArgs->spentUs) = (now.tv_usec - before.tv_usec) + (now.tv_sec - before.tv_sec) * 1000000;

    return 0;
}

int MeasureRMInterface(Remove_Cont_Args_T *removeArgs)
{
    struct timeval before, now;

    NULL_PTR_CHECK(removeArgs, RET_INVALID_INPUT_PARAM);
    NULL_PTR_CHECK(removeArgs->wm, RET_INVALID_INPUT_PARAM);
    NULL_PTR_CHECK(removeArgs->spentUs, RET_INVALID_INPUT_PARAM);

    gettimeofday(&before, NULL);
    removeArgs->retVal = removeArgs->wm->rmContainer(*removeArgs->contID);
    gettimeofday(&now, NULL);

    *(removeArgs->spentUs) = (now.tv_usec - before.tv_usec) + (now.tv_sec - before.tv_sec) * 1000000;

    return 0;
}

int MeasureRunInterface(Run_Cont_Args_T *runArgs)
{
    struct timeval before, now;

    NULL_PTR_CHECK(runArgs, RET_INVALID_INPUT_PARAM);
    NULL_PTR_CHECK(runArgs->wm, RET_INVALID_INPUT_PARAM);
    NULL_PTR_CHECK(runArgs->spentUs, RET_INVALID_INPUT_PARAM);

    gettimeofday(&before, NULL);
    runArgs->retVal = runArgs->wm->runContainer(runArgs->imageName, runArgs->runCmd, runArgs->contID);
    gettimeofday(&now, NULL);

    if (runArgs->retVal == 0) {
        // save running container id for clean resource.
        WrapContInfoCls::GetInstance()->ContBindWrapper(runArgs->contID, STAT_RUNNING, runArgs->wm);
    }

    *(runArgs->spentUs) = (now.tv_usec - before.tv_usec) + (now.tv_sec - before.tv_sec) * 1000000;

    return 0;
}
