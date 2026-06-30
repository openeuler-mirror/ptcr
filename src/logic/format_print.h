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
 * Description: privide format printter class definition.
 ******************************************************************************/
#ifndef SRC_LOGIC_FORMAT_PRINT_H
#define SRC_LOGIC_FORMAT_PRINT_H

#include <mutex>
#include "wrap.h"
#include "config.h"

typedef struct {
    string interfaceDesc;
    long int totalSpent;
    long int maxSpent;
    long int minSpent;
    bool minSpentSet;
    bool maxSpentSet;
    int Cnt;
} Measure_Result_T;

typedef struct {
    string daemonName;
    long int daemonMemory;
} Mem_Daemon_T;

typedef struct {
    string endpoint;
    long int shimTotalMemory;
    unsigned cnt;
} Mem_Shim_T;

class MeasureResultCls {
public:
    int InsertMeasureReulst(Measure_Result_T *measureRes);

    int InsertMeasureItem(Measure_Result_T *measureRes, long int currentSpent);

    void PrintAllResult();

    void GetDescripesForReport(string *descArray);

    MeasureResultCls(string measureDesc, string &endPoint)
        : m_measureDesc(measureDesc), m_endPoint(endPoint)
    {
    };
    ~MeasureResultCls();

    std::string     m_measureDesc;
    std::string     m_endPoint;
    std::vector<Measure_Result_T *> m_measureResultVect;
private:
    mutex           m_mutex;
};

class FormatPrintCls {
public:

    int InsertMeasureCls(MeasureResultCls *);

    int InsertDaemonRes(Mem_Daemon_T *);
    int InsertShimRes(Mem_Shim_T *);

    void formatPrint();

    void GenerateReport(std::string &filePath);

    static FormatPrintCls *GetInstance()
    {
        static FormatPrintCls *self = NULL;
        if (self == NULL) {
            self = new FormatPrintCls();
        }
        return self;
    }

private:
    FormatPrintCls() {};
    ~FormatPrintCls();

    std::vector<MeasureResultCls *> m_measureResClsVect; // time
    std::vector<Mem_Daemon_T *> m_memDaemonVect; // daemon memory
    std::vector<Mem_Shim_T *>   m_memShimVect;   // shim memory
};

const char *GetMeasureTypeDesc(MEASURE_TYPE_E type);

#endif
