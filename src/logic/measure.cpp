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
 * Description: measure container runtime logic implement.
 ******************************************************************************/
#include "measure.h"
#include "memory.h"
#include <sys/time.h>
#include <thread>
#include <mutex>
#include "logger.h"
#include "utils.h"
#include "container_manager.h"

using namespace std;

typedef void *(*MEASURE_FUNC_DEF)(void *);

template<typename ARGS_TYPE>
void Free_Args(ARGS_TYPE *args[], int count)
{
    for (int i = 0; i < count; i++) {
        free(args[i]->spentUs);
        free(args[i]);
        args[i] = NULL;
    }
    free(args);
}

int Create_Args_Init
(Create_Cont_Args_T *args, string *contID, MeasureConfigCls *m_config, wrapperManager *m_wrapperManager)
{
    NULL_PTR_CHECK(args, RET_INVALID_INPUT_PARAM);
    NULL_PTR_CHECK(contID, RET_INVALID_INPUT_PARAM);

    args->bSave = true;
    args->ContID = contID;
    args->imageName = (*m_config).m_imageName;
    args->wm = m_wrapperManager;
    args->spentUs = (long int *)UTILS_CALLOC(sizeof(long int), RET_OUT_OF_MEMORY);

    return 0;
}

int Start_Args_Init
(Start_Cont_Args_T *args, string *contID, MeasureConfigCls *m_config, wrapperManager *m_wrapperManager)
{
    NULL_PTR_CHECK(args, RET_INVALID_INPUT_PARAM);
    NULL_PTR_CHECK(contID, RET_INVALID_INPUT_PARAM);

    args->contID = contID;
    args->wm = m_wrapperManager;
    args->spentUs = (long int *)UTILS_CALLOC(sizeof(long int), RET_OUT_OF_MEMORY);

    return 0;
}

int Stop_Args_Init
(Stop_Cont_Args_T *args, string *contID, MeasureConfigCls *m_config, wrapperManager *m_wrapperManager)
{
    NULL_PTR_CHECK(args, RET_INVALID_INPUT_PARAM);
    NULL_PTR_CHECK(contID, RET_INVALID_INPUT_PARAM);

    args->contID = contID;
    args->timeOut = (*m_config).m_timeOut;
    args->wm = m_wrapperManager;
    args->spentUs = (long int *)UTILS_CALLOC(sizeof(long int), RET_OUT_OF_MEMORY);

    return 0;
}

int Remove_Args_Init
(Remove_Cont_Args_T *args, string *contID, MeasureConfigCls *m_config, wrapperManager *m_wrapperManager)
{
    NULL_PTR_CHECK(args, RET_INVALID_INPUT_PARAM);
    NULL_PTR_CHECK(contID, RET_INVALID_INPUT_PARAM);

    args->contID = contID;
    args->wm = m_wrapperManager;
    args->spentUs = (long int *)UTILS_CALLOC(sizeof(long int), RET_OUT_OF_MEMORY);

    return 0;
}

int Run_Args_Init
(Run_Cont_Args_T *args, string *contID, MeasureConfigCls *m_config, wrapperManager *m_wrapperManager)
{
    NULL_PTR_CHECK(args, RET_INVALID_INPUT_PARAM);
    NULL_PTR_CHECK(contID, RET_INVALID_INPUT_PARAM);

    args->contID = contID;
    args->imageName = m_config->m_imageName;
    args->runCmd = m_config->m_runContCmd;
    args->spentUs = (long int *)UTILS_CALLOC(sizeof(long int), RET_OUT_OF_MEMORY);
    args->wm = m_wrapperManager;

    return 0;
}

class MixCmdCls {
public:
    MixCmdCls(const string &interfaceDesc, wrapperManager *wrapperManager,
              MeasureConfigCls *config, MeasureResultCls *measureResCls)
        : m_interfaceDesc(interfaceDesc), m_measureResult(NULL), m_wrapperManager(wrapperManager),
          m_config(config), m_measureResCls(measureResCls)
    {
    }
    ~MixCmdCls() {};

    template<typename ARGS_TYPE>
    int run(string *contID,
            int (*Args_Init_Cbk)(ARGS_TYPE *args, string *contID,
                                 MeasureConfigCls *m_config, wrapperManager *m_wrapperManager),
            void *(*Measure_Func)(void *))
    {
        NULL_PTR_CHECK(contID, RET_INVALID_INPUT_PARAM);
        int ret = 0;
        ARGS_TYPE args;

        if (m_measureResult == NULL) {
            Init();
        }

        ret = Args_Init_Cbk(&args, contID, m_config, m_wrapperManager);
        RET_CHECK(ret, 0, return ret);

        int mret = (uintptr_t)Measure_Func(&args);
        /* Only record the timing sample if the underlying operation
         * succeeded; otherwise the spent time reflects a failure path and
         * would pollute the average/max/min statistics. */
        if (mret == 0 && args.retVal == 0) {
            m_measureResCls->InsertMeasureItem(m_measureResult, *(args.spentUs));
        } else {
            LOG_ERROR("%s measure failed (mret=%d, args.retVal=%d)\n",
                      m_interfaceDesc.c_str(), mret, args.retVal);
        }
        return (mret != 0) ? mret : args.retVal;
    }

    int Init()
    {
        m_measureResult = (Measure_Result_T *)UTILS_CALLOC(sizeof(Measure_Result_T), RET_OUT_OF_MEMORY);
        m_measureResult->interfaceDesc = m_interfaceDesc;

        m_measureResCls->InsertMeasureReulst(m_measureResult);
        return 0;
    }

private:
    string m_interfaceDesc;
    Measure_Result_T *m_measureResult;
    wrapperManager *m_wrapperManager;
    MeasureConfigCls *m_config;
    MeasureResultCls *m_measureResCls;
};

class MeasureSeriallyCls : public MeasureImpl {
public:
    MeasureSeriallyCls(unsigned int cnt, wrapperManager *wm, MeasureConfigCls *config)
    {
        m_cnt = cnt;
        m_wrapperManager = wm;
        m_config = config;
    }

    virtual ~MeasureSeriallyCls() {}

    virtual int startWithMixedCmd();
    virtual int startWithoutMixedCmd();

    template<typename ARGS_TYPE>
    int Serial_Func(const string &interfaceDesc,
                    int (*Args_Init_Cbk)(ARGS_TYPE *args, string *contID, /* saved used by start/stop/remove */
                                         MeasureConfigCls *m_config, wrapperManager *m_wrapperManager),
                    void *(*Measure_Func)(void *))
    {
        int ret = 0;
        ARGS_TYPE args;
        vector<Cont_Info_T *> contVect;

        Measure_Result_T *MeasureResult =
            (Measure_Result_T *)UTILS_CALLOC(sizeof(Measure_Result_T), RET_OUT_OF_MEMORY);
        MeasureResult->interfaceDesc = interfaceDesc;
        m_measureResCls->InsertMeasureReulst(MeasureResult);
        WrapContInfoCls::GetInstance()->GetContVectByWrapper(m_wrapperManager, &contVect);

        for (unsigned i = 0; i < m_cnt; i++) {
            if (contVect.empty()) { /* is create */
                string *contID = new string;
                ret = Args_Init_Cbk(&args, contID, m_config, m_wrapperManager);
            } else if (i < contVect.size()) {
                ret = Args_Init_Cbk(&args, contVect.at(i)->contId, m_config, m_wrapperManager);
            } else {
                ret = -1;
            }
            RET_CHECK(ret, 0, goto FAILURE);

            ret = (uintptr_t)Measure_Func(&args);
            if (ret != 0 || args.retVal != 0) {
                LOG_ERROR("Serial %s iteration %u failed (mret=%d, args.retVal=%d)\n",
                          interfaceDesc.c_str(), i, ret, args.retVal);
                goto FAILURE;
            }

            m_measureResCls->InsertMeasureItem(MeasureResult, *(args.spentUs));
            if (args.spentUs) {
                free(args.spentUs);
            }
        }

        return 0;
FAILURE:
        return ret;
    }
};

int MeasureSeriallyCls::startWithMixedCmd()
{
    m_measureResCls = new MeasureResultCls("searially", m_wrapperManager->m_endPoint);

    MixCmdCls *mixedCreate = new MixCmdCls("Create", m_wrapperManager, m_config, m_measureResCls);
    MixCmdCls *mixedStart = new MixCmdCls("Start", m_wrapperManager, m_config, m_measureResCls);
    MixCmdCls *mixedStop = new MixCmdCls("Stop", m_wrapperManager, m_config, m_measureResCls);
    MixCmdCls *mixedRemove = new MixCmdCls("Remove", m_wrapperManager, m_config, m_measureResCls);
    MixCmdCls *mixedRun = new MixCmdCls("Run", m_wrapperManager, m_config, m_measureResCls);

    while (m_cnt--) {
        string *contID = new string;
        mixedCreate->run(contID, Create_Args_Init, (MEASURE_FUNC_DEF)MeasureCreateInterface);
        mixedStart->run(contID, Start_Args_Init, (MEASURE_FUNC_DEF)MeasureStartInterface);
        mixedStop->run(contID, Stop_Args_Init, (MEASURE_FUNC_DEF)MeasureStopInterface);
        mixedRemove->run(contID, Remove_Args_Init, (MEASURE_FUNC_DEF)MeasureRMInterface);
        mixedRun->run(contID, Run_Args_Init, (MEASURE_FUNC_DEF)MeasureRunInterface);
    }

    FormatPrintCls::GetInstance()->InsertMeasureCls(m_measureResCls);
    WrapContInfoCls::GetInstance()->CleanAllCont(m_wrapperManager); // clean saved container info

    delete mixedCreate;
    delete mixedStart;
    delete mixedStop;
    delete mixedRemove;
    delete mixedRun;

    return 0;
}

int MeasureSeriallyCls::startWithoutMixedCmd()
{
    int ret = 0;
    /* allocate measure result class */
    m_measureResCls = new MeasureResultCls("searially", m_wrapperManager->m_endPoint);

    ret = Serial_Func<Create_Cont_Args_T>("Create", Create_Args_Init, (MEASURE_FUNC_DEF)MeasureCreateInterface);
    RET_CHECK(ret, 0, return ret);

    ret = Serial_Func<Start_Cont_Args_T>("Start", Start_Args_Init, (MEASURE_FUNC_DEF)MeasureStartInterface);
    RET_CHECK(ret, 0, return ret);

    ret = Serial_Func<Stop_Cont_Args_T>("Stop", Stop_Args_Init, (MEASURE_FUNC_DEF)MeasureStopInterface);
    RET_CHECK(ret, 0, return ret);

    ret = Serial_Func<Remove_Cont_Args_T>("Remove", Remove_Args_Init, (MEASURE_FUNC_DEF)MeasureRMInterface);
    RET_CHECK(ret, 0, return ret);

    ret = Serial_Func<Run_Cont_Args_T>("Run", Run_Args_Init, (MEASURE_FUNC_DEF)MeasureRunInterface);
    RET_CHECK(ret, 0, return ret);

    FormatPrintCls::GetInstance()->InsertMeasureCls(m_measureResCls);
    WrapContInfoCls::GetInstance()->CleanAllCont(m_wrapperManager); // clean saved container info

    return ret;
}

class MeasureParallyCls : public MeasureImpl {
public:
    MeasureParallyCls(unsigned int cnt, wrapperManager *wm, MeasureConfigCls *config)
    {
        m_cnt = cnt;
        m_wrapperManager = wm;
        m_config = config;
    }

    virtual ~MeasureParallyCls() {}

    virtual int startWithMixedCmd();
    virtual int startWithoutMixedCmd();

    friend void *ParallyMixedThrd(void *args);

    template<typename ARGS_TYPE>
    int Paraller_Func
    (const string &interfaceDesc,
     int (*Args_Init_Cbk)(ARGS_TYPE *args, string *contID, /* saved used by start/stop/remove */
                          MeasureConfigCls *m_config, wrapperManager *m_wrapperManager),
     void *(*Measure_Func)(void *))
    {
        unsigned i = 0;

        vector<Cont_Info_T *> contVect;
        WrapContInfoCls::GetInstance()->GetContVectByWrapper(m_wrapperManager, &contVect);
        Measure_Result_T *measureResult = (Measure_Result_T *)UTILS_CALLOC(sizeof(Measure_Result_T), RET_OUT_OF_MEMORY);
        measureResult->interfaceDesc = interfaceDesc;

        m_measureResCls->InsertMeasureReulst(measureResult);
        ARGS_TYPE **ArgsList = (ARGS_TYPE **)
                               UTILS_CALLOC(sizeof(ARGS_TYPE *) * m_cnt, RET_OUT_OF_MEMORY);
        for (i = 0; i < m_cnt; i++) {
            ArgsList[i] = (ARGS_TYPE *)UTILS_CALLOC(sizeof(ARGS_TYPE), RET_OUT_OF_MEMORY);
            if (contVect.empty()) {
                string *contID = new string;
                Args_Init_Cbk(ArgsList[i], contID, m_config, m_wrapperManager);
            } else if (i < contVect.size()) {
                Args_Init_Cbk(ArgsList[i], contVect.at(i)->contId, m_config, m_wrapperManager);
            } else {
                LOG_FATAL("Created containers error!\n");
                return -1;
            }

            std::thread *thrd = new std::thread(Measure_Func, ArgsList[i]);
            m_stdThrds.push_back(thrd);
        }

        for (unsigned int i = 0; i < m_stdThrds.size(); i++) {
            m_stdThrds[i]->join();
            /* Only record the sample if the worker succeeded; failed
             * iterations would otherwise pollute min/max/avg. */
            if (ArgsList[i]->retVal == 0) {
                m_measureResCls->InsertMeasureItem(measureResult, *(ArgsList[i]->spentUs));
            } else {
                LOG_ERROR("Paraller %s iteration %u failed (args.retVal=%d)\n",
                          interfaceDesc.c_str(), i, ArgsList[i]->retVal);
            }
            delete m_stdThrds[i];
        }

        m_stdThrds.clear();
        Free_Args(ArgsList, m_cnt);

        return 0;
    }

    int ParallyMixedThrd(MixCmdCls *mixedCreate, MixCmdCls *mixedStart, MixCmdCls *mixedStop, MixCmdCls *mixedRemove,
                         MixCmdCls *mixedRun);
private:
    vector<std::thread *> m_stdThrds;
    std::mutex m_mixedCmdMutex;
};

int MeasureParallyCls::ParallyMixedThrd
(MixCmdCls *mixedCreate, MixCmdCls *mixedStart, MixCmdCls *mixedStop, MixCmdCls *mixedRemove, MixCmdCls *mixedRun)
{
    std::lock_guard<std::mutex> lock(m_mixedCmdMutex);

    int ret = 0;
    string *contID = new string;

    ret = mixedCreate->run(contID, Create_Args_Init, (MEASURE_FUNC_DEF)MeasureCreateInterface);
    RET_CHECK(ret, 0, return ret);

    ret = mixedStart->run(contID, Start_Args_Init, (MEASURE_FUNC_DEF)MeasureStartInterface);
    RET_CHECK(ret, 0, return ret);

    ret = mixedStop->run(contID, Stop_Args_Init, (MEASURE_FUNC_DEF)MeasureStopInterface);
    RET_CHECK(ret, 0, return ret);

    ret = mixedRemove->run(contID, Remove_Args_Init, (MEASURE_FUNC_DEF)MeasureRMInterface);
    RET_CHECK(ret, 0, return ret);

    ret = mixedRun->run(contID, Run_Args_Init, (MEASURE_FUNC_DEF)MeasureRunInterface);
    RET_CHECK(ret, 0, return ret);

    return ret;
}

int MeasureParallyCls::startWithMixedCmd()
{
    m_measureResCls = new MeasureResultCls("parallerlly", m_wrapperManager->m_endPoint);

    MixCmdCls *mixedCreate = new MixCmdCls("Create", m_wrapperManager, m_config, m_measureResCls);
    mixedCreate->Init();

    MixCmdCls *mixedStart = new MixCmdCls("Start", m_wrapperManager, m_config, m_measureResCls);
    mixedStart->Init();

    MixCmdCls *mixedStop = new MixCmdCls("Stop", m_wrapperManager, m_config, m_measureResCls);
    mixedStop->Init();

    MixCmdCls *mixedRemove = new MixCmdCls("Remove", m_wrapperManager, m_config, m_measureResCls);
    mixedRemove->Init();

    MixCmdCls *mixedRun = new MixCmdCls("Run", m_wrapperManager, m_config, m_measureResCls);
    mixedRun->Init();

    while (m_cnt--) {
        std::thread *thrd = new std::thread(&MeasureParallyCls::ParallyMixedThrd, this,
                                            mixedCreate, mixedStart, mixedStop, mixedRemove, mixedRun);
        m_stdThrds.push_back(thrd);
    }

    for (auto item : m_stdThrds) {
        item->join();
        delete item;
    }

    FormatPrintCls::GetInstance()->InsertMeasureCls(m_measureResCls);
    WrapContInfoCls::GetInstance()->CleanAllCont(m_wrapperManager); // clean saved container info

    delete mixedCreate;
    delete mixedStart;
    delete mixedStop;
    delete mixedRemove;
    delete mixedRun;

    return 0;
}

int MeasureParallyCls::startWithoutMixedCmd()
{
    int ret = 0;

    if (m_wrapperManager->m_connectType == CONNECT_BY_CRI) {
        /* TODO */
        LOG_WARN("Current not support API parallerlly, waiting a predestined person to add\n");
        return 0;
    }

    m_measureResCls = new MeasureResultCls("parallerlly", m_wrapperManager->m_endPoint);
    NULL_PTR_CHECK(m_measureResCls, RET_OUT_OF_MEMORY);

    ret = Paraller_Func<Create_Cont_Args_T>("Create", Create_Args_Init, (MEASURE_FUNC_DEF)MeasureCreateInterface);
    RET_CHECK(ret, 0, return ret);

    ret = Paraller_Func<Start_Cont_Args_T>("Start", Start_Args_Init, (MEASURE_FUNC_DEF)MeasureStartInterface);
    RET_CHECK(ret, 0, return ret);

    ret = Paraller_Func<Stop_Cont_Args_T>("Stop", Stop_Args_Init, (MEASURE_FUNC_DEF)MeasureStopInterface);
    RET_CHECK(ret, 0, return ret);

    ret = Paraller_Func<Remove_Cont_Args_T>("Remove", Remove_Args_Init, (MEASURE_FUNC_DEF)MeasureRMInterface);
    RET_CHECK(ret, 0, return ret);

    ret = Paraller_Func<Run_Cont_Args_T>("Run", Run_Args_Init, (MEASURE_FUNC_DEF)MeasureRunInterface);
    RET_CHECK(ret, 0, return ret);

    FormatPrintCls::GetInstance()->InsertMeasureCls(m_measureResCls);
    WrapContInfoCls::GetInstance()->CleanAllCont(m_wrapperManager);

    return ret;
}

int measureCls::runWrapperRepeat(MeasureConfigCls *config, wrapperManager *wm)
{
    for (auto item : config->m_measureWayVect) {
        if (item->measureType == MEASURE_SERIALLY && item->measureCnt != 0) {
            MeasureSeriallyCls mSerially(item->measureCnt, wm, config);

            mSerially.start();
        } else if (item->measureType == MEASURE_PARALLERL && item->measureCnt != 0) {
            MeasureParallyCls mParally(item->measureCnt, wm, config);

            mParally.start();
        }
    }

    return 0;
}

int measureCls::startMeasure(MeasureConfigCls *config)
{
    NULL_PTR_CHECK(config, RET_INVALID_INPUT_PARAM);

    for (unsigned i = 0; i < config->m_runtimeName.size(); i++) {
        wrapperManager *cliWrapper = new wrapperManager(CONNECT_BY_CLI, config->m_runtimeName[i]);
        NULL_PTR_CHECK(cliWrapper, return -1);

        if (cliWrapper->init()) {
            break;
        }
        WrapContInfoCls::GetInstance()->InsertWrapper(cliWrapper);
        runWrapperRepeat(config, cliWrapper);

        cliWrapper->Deinit();
    }

    for (unsigned i = 0; i < config->m_runtimeEndpoint.size(); i++) {
        wrapperManager *apiWrapper = new wrapperManager(CONNECT_BY_CRI, config->m_runtimeEndpoint[i]);
        NULL_PTR_CHECK(apiWrapper, return -1);

        if (apiWrapper->init()) {
            break;
        }
        WrapContInfoCls::GetInstance()->InsertWrapper(apiWrapper);
        runWrapperRepeat(config, apiWrapper);

        apiWrapper->Deinit();
    }

    MeasureMemImpl memImpl(config->m_runtimeName, config->m_daemonName, config);
    memImpl.PsDaemonOutsideFunc();
    memImpl.PsShimOutsideFunc();

    FormatPrintCls::GetInstance()->formatPrint();
    FormatPrintCls::GetInstance()->GenerateReport(config->m_outputFile);
    WrapContInfoCls::GetInstance()->ResourceRls();

    return 0;
}
