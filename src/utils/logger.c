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
 * Description: debug logger module implement.
 ******************************************************************************/
#include "logger.h"

#include <stdio.h>
#include <unistd.h>
#include <time.h>
#include <stdarg.h>

static LOG_LERVER_E g_log_lever = L_DEBUG;

int vlog_in(LOG_LERVER_E lever, const char *format, ...)
{
    if (lever < g_log_lever) {
        return 0;
    }

    char buf[1024] = {0};
    int  time_len = 0;
    int  msg_len = 0;
    int  total_len = 0;
    va_list ap;

    time_t  tm = 0;
    struct  tm st_time = {0};

    tm = time(&tm);
    localtime_r(&tm, &st_time);
    time_len = snprintf(buf, sizeof(buf), "%02d:%02d:%02d ",
                        (st_time.tm_hour) % 24, st_time.tm_min, st_time.tm_sec);

    va_start(ap, format);
    msg_len = vsnprintf(buf + time_len, sizeof(buf) - time_len, format, ap);
    va_end(ap);
    if (msg_len < 0) {
        return -1;
    }

    total_len = time_len + msg_len;
    if (total_len >= (int)sizeof(buf)) {
        total_len = (int)sizeof(buf) - 1;
    }

    return write(1, buf, total_len);
}

void set_log_lever(LOG_LERVER_E lever)
{
    if (lever >= L_DEBUG && lever < L_NR) {
        g_log_lever = lever;
    }
}
