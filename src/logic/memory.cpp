#include "memory.h"
#include "logger.h"
#include "utils.h"
#include "container_manager.h"
#include "format_print.h"
#include <new>

int MeasureMemImpl::PsDaemonOutsideFunc()
{
    int ret = 0;

    for (auto it = m_daemon.begin(); it != m_daemon.end(); it++) {
        int rss;
        // string endpoint = it->first;
        ret = read_memory(it->first.c_str(), &rss);
        if (ret != 0) {
            return ret;
        }
        it->second += rss;

        Mem_Daemon_T *daemon = new (std::nothrow) Mem_Daemon_T();
        NULL_PTR_CHECK(daemon, RET_OUT_OF_MEMORY);
        daemon->daemonName = it->first;
        daemon->daemonMemory = it->second;
        FormatPrintCls::GetInstance()->InsertDaemonRes(daemon);
    }

    return ret;
}

int MeasureMemImpl::PsShimOutsideFunc()
{
    int ret = 0;

    for (auto it = m_shim.begin(); it != m_shim.end(); it++) {
        string endpoint = it->first;
        wrapperManager *wm = new wrapperManager(CONNECT_BY_CLI, endpoint);
        if (wm->init()) {
            delete wm;
            break;
        }

        for (unsigned i = 0; i < m_cnt; i++) {
            string *contID = new string;
            wm->runContainer(m_config->m_imageName, m_config->m_runContCmd, contID);

            char *finalID = (char *)UTILS_CALLOC(sizeof(char) * CONTAINER_ID_SIZE, RET_OUT_OF_MEMORY);
            ret = get_cont_id_by_name(endpoint.c_str(), contID->c_str(), finalID);
            if (ret != 0) {
                wm->stopContainer(*contID, m_config->m_timeOut);
                wm->rmContainer(*contID);
                free(finalID);
                delete contID;
                wm->Deinit();
                delete wm;
                return ret;
            }

            int rss;
            ret = read_memory(finalID, &rss);
            if (ret != 0) {
                wm->stopContainer(*contID, m_config->m_timeOut);
                wm->rmContainer(*contID);
                free(finalID);
                delete contID;
                wm->Deinit();
                delete wm;
                return ret;
            }
            it->second += rss;

            wm->stopContainer(*contID, m_config->m_timeOut);
            wm->rmContainer(*contID);
            free(finalID);
            delete contID;
        }

        Mem_Shim_T *shim = new (std::nothrow) Mem_Shim_T();
        NULL_PTR_CHECK(shim, RET_OUT_OF_MEMORY);
        shim->endpoint = it->first;
        shim->shimTotalMemory = it->second;
        shim->cnt = m_cnt;
        FormatPrintCls::GetInstance()->InsertShimRes(shim);

        wm->Deinit();
        delete wm;
    }

    return ret;
}
