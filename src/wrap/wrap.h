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
 * Description: provide measure wrapper class definition.
 ******************************************************************************/
#ifndef SRC_WRAP_WRAP_H
#define SRC_WRAP_WRAP_H

#include <sys/time.h>
#include <string>
#include <vector>

using namespace std;

typedef enum {
    STAT_CREATED    = 0,
    STAT_RUNNING,
    STAT_STOPPED,
} CONT_STAT_E;

typedef enum {
    CONNECT_BY_CLI      = 0,
    CONNECT_BY_CRI,
    CONNECT_BY_REST,

    CONNECT_TYPE_NR,
} CONT_RT_CONNECT_TYPE;

class measureWrapperCls {
public:
    virtual ~measureWrapperCls() {};

    virtual int pullImage(string &imageName) = 0;
    virtual int removeImage(string &imageName) = 0;
    virtual int createContainer(string &imageName, string *contStr) = 0;
    virtual int startContainer(string &ContStr) = 0;
    virtual int runContainer(string &imageName, vector<string> &cmd, string *contStr) = 0;
    virtual int stopContainer(string contId, int timeOut) = 0;
    virtual int stopAllContainer() = 0;
    virtual int rmContainer(string ContId) = 0;

    CONT_RT_CONNECT_TYPE m_connectType;
};

class wrapperManager : public measureWrapperCls {
public:
    wrapperManager(CONT_RT_CONNECT_TYPE connectType, string &endPoint);
    ~wrapperManager();
    int init();
    void Deinit();

    int pullImage(string &imageName);
    int removeImage(string &imageName);
    int createContainer(string &imageName, string *contStr);
    int startContainer(string &ContStr);
    int runContainer(string &imageName, vector<string> &cmd, string *contStr);
    int stopContainer(string contId, int timeOut);
    int stopAllContainer();
    int rmContainer(string ContId);

    string              m_endPoint;
private:
    measureWrapperCls   *m_wrapOps;
};

typedef struct {
    char *cli_name;
    char *runtime_endpoint;
    CONT_RT_CONNECT_TYPE connect_type;
    const struct measure_operations *ops;
} ContainerRTWrapper_T;

typedef struct {
    string imageName;
    wrapperManager *wm;
    bool bSave;
    long int *spentUs;
    string *ContID;
    /* Result of the underlying measure operation. 0 means success. The
     * measure loop inspects this field to decide whether the spentUs sample
     * is valid for statistics. */
    int retVal;
} Create_Cont_Args_T;

typedef struct {
    string *contID;
    wrapperManager *wm;
    long int *spentUs;
    int retVal;
} Start_Cont_Args_T;

typedef struct {
    string *contID;
    wrapperManager *wm;
    unsigned int timeOut;
    long int *spentUs;
    int retVal;
} Stop_Cont_Args_T;

typedef struct {
    string *contID;
    wrapperManager *wm;
    long int *spentUs;
    int retVal;
} Remove_Cont_Args_T;

typedef struct {
    string imageName;
    string *contID;
    wrapperManager *wm;
    long int *spentUs;
    vector<string> runCmd;
    int retVal;
} Run_Cont_Args_T;

typedef struct {
    string *contSuffix;
    wrapperManager *wm;
    long int *memoryUsage;
} Ps_Outside_Cont_Args_T;

int MeasureCreateInterface(Create_Cont_Args_T *CreateArgs);
int MeasureStartInterface(Start_Cont_Args_T *startArgs);
int MeasureStopInterface(Stop_Cont_Args_T *stopArgs);
int MeasureRMInterface(Remove_Cont_Args_T *removeArgs);
int MeasureRunInterface(Run_Cont_Args_T *runArgs);

#endif
