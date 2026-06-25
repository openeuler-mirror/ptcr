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
 * Description: ptcr container manager implement.
 ******************************************************************************/
#include "container_manager.h"
#include <iostream>
#include "utils.h"
#include "logger.h"

int WrapContInfoCls::InsertWrapper(wrapperManager *wrapper)
{
    NULL_PTR_CHECK(wrapper, RET_INVALID_INPUT_PARAM);

    Wrap_Cont_Info_T *wrapContInfo = (Wrap_Cont_Info_T *)
                                     UTILS_CALLOC(sizeof(Wrap_Cont_Info_T), RET_OUT_OF_MEMORY);
    wrapContInfo->m_RtWrapper = wrapper;

    m_mutex.lock();
    m_wrapContInfo.push_back(wrapContInfo);
    m_mutex.unlock();
    return 0;
}

int WrapContInfoCls::ContBindWrapper(string *contId, CONT_STAT_E state, wrapperManager *wrapper)
{
    vector<Wrap_Cont_Info_T *>::iterator iter = m_wrapContInfo.begin();

    m_mutex.lock();
    for (; iter != m_wrapContInfo.end(); ++iter) {
        if ((*iter)->m_RtWrapper->m_endPoint.compare(wrapper->m_endPoint) == 0) {
            Cont_Info_T *contInfo = (Cont_Info_T *)
                                    UTILS_CALLOC(sizeof(Cont_Info_T), RET_OUT_OF_MEMORY);
            contInfo->contId = contId;
            contInfo->stat = state;

            (*iter)->m_ContInfo.push_back(contInfo);
            break;
        }
    }
    m_mutex.unlock();

    return 0;
}

int WrapContInfoCls::GetContVectByWrapper(wrapperManager *wrapper, vector<Cont_Info_T *> *ContVect)
{
    NULL_PTR_CHECK(wrapper, RET_INVALID_INPUT_PARAM);
    NULL_PTR_CHECK(ContVect, RET_INVALID_INPUT_PARAM);

    vector<Wrap_Cont_Info_T *>::iterator iter = m_wrapContInfo.begin();
    for (; iter != m_wrapContInfo.end(); ++iter) {
        if ((*iter)->m_RtWrapper->m_endPoint.compare(wrapper->m_endPoint) == 0) {
            *ContVect = (*iter)->m_ContInfo;
            break;
        }
    }

    return 0;
}

void WrapContInfoCls::PrintAllContInfo()
{
    for (auto item : m_wrapContInfo) {
        cout << "-----------client: " << item->m_RtWrapper->m_endPoint << endl;
        for (auto i : item->m_ContInfo) {
            cout << "contid: " << *i->contId << " state: " << i->stat << endl;
        }
    }
}

/* clean containers which specific wrapper bind */
void WrapContInfoCls::CleanAllCont(wrapperManager *wrapper)
{
    NULL_PTR_CHECK(wrapper, return);

    m_mutex.lock();
    for (auto item : m_wrapContInfo) {
        if (item->m_RtWrapper->m_endPoint.compare(wrapper->m_endPoint) == 0) {
            for (auto __item : item->m_ContInfo) {
                StopAndRemoveContainer(wrapper, __item);
            }

            item->m_ContInfo.clear();
        }
    }
    m_mutex.unlock();
}

/* clean all wrapper and it binded containers */
void WrapContInfoCls::ResourceRls()
{
    m_mutex.lock();
    for (auto item : m_wrapContInfo) {
        for (auto __item : item->m_ContInfo) { // If container is running, stop and delete it.
            StopAndRemoveContainer(item->m_RtWrapper, __item);
        }
        item->m_ContInfo.clear();

        delete item->m_RtWrapper;
    }
    m_wrapContInfo.clear();
    m_mutex.unlock();
}

void WrapContInfoCls::StopAndRemoveContainer(wrapperManager *wm, Cont_Info_T *contInfo)
{
    if (contInfo->stat == STAT_RUNNING) {
        wm->stopContainer(*contInfo->contId, -1); // timeout -1, force kill container.
        wm->rmContainer(*contInfo->contId);
    }
}
