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
 * Description: common utils implement.
 ******************************************************************************/
#include "utils.h"

#include <stdio.h>
#include <stdarg.h>
#include <errno.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <fcntl.h>

typedef enum {
    READ_PIPE   = 0,
    WRITE_PIPE,

    PIPE_BUTT,
} PIPE_E;

int utils_execute_process(const char *file, char * const params[])
{
    int wstatus = 0;
    int pid = 0;
    int ret = 0;

    pid = fork();
    if (pid < 0) {
        LOG_OUT(L_ERROR, "fork failed\n");
        goto OUT;
    } else if (pid == 0) {
        close(1);
        close(2);
        execvp(file, params);
        _exit(127);
    }

WAIT:
    ret = waitpid(pid, &wstatus, 0);
    if (ret == -1) {
        if (errno == EINTR) {
            goto WAIT;
        }
        ret = -1;
    } else if (ret != pid) {
        goto WAIT;
    } else {
        ret = 0;
    }

OUT:

    return ret;
}

char **utils_create_str_array(unsigned str_cnt, ...)
{
    va_list ap;
    int i = 0;
    char **arr;
    va_start(ap, str_cnt);

    arr = malloc(sizeof(char *) * str_cnt);
    NULL_PTR_CHECK(arr, goto exit);

    for (i = 0; i < str_cnt; i++) {
        arr[i] = va_arg(ap, char *);
    }

exit:
    va_end(ap);
    return arr;
}

void utils_free_arr(char ***arr)
{
    int i = 0;

    while ((*arr)[i] != NULL) {
        free((*arr)[i]);
        i++;
    }

    free(*arr);
    *arr = NULL;
}

char *utils_generate_random_str(int len)
{
    int fd;
    int num = 0;
    int i = 0;
    int m = 256;
    char *str;

    str = malloc(len * sizeof(char));
    NULL_PTR_CHECK(str, ERR_NULL);

    len /= 2;
    fd = open("/dev/urandom", O_RDONLY);
    if (fd < 0) {
        ERR_NULL;
    }

    for (i = 0; i < len; i++) {
        int nret;
        if (read(fd, &num, sizeof(int)) < 0) {
            LOG_ERROR("Failed to read urandom value");
            close(fd);
            ERR_NULL;
        }
        unsigned char rs = (unsigned char)(num % m);
        nret = snprintf((str + i * 2), ((len - i) * 2 + 1), "%02x", (unsigned int)rs);
        if (nret < 0 || (size_t)nret >= ((len - i) * 2 + 1)) {
            LOG_ERROR("Failed to snprintf random string");
            close(fd);
            ERR_NULL;
        }
    }

    close(fd);
    str[i * 2] = '\0';
    return str;
}

int utils_exe_cmd_read_out(const char *command, char *out, int size)
{
    NULL_PTR_CHECK(command, RET_INVALID_INPUT_PARAM);

    LOG_DEBUG("execute %s\n", command);
    FILE *pF = popen(command, "r");
    NULL_PTR_CHECK(pF, ERR_FAILURE);

    if (out != NULL) {
        fgets(out, size, pF);
        strtok(out, "\n");
    }

    pclose(pF);

    return 0;
}

int get_cont_id_by_name(const char *endpoint, const char *name, char id[CONTAINER_ID_SIZE])
{
    NULL_PTR_CHECK(name, RET_INVALID_INPUT_PARAM);

    char *command = (char *)UTILS_CALLOC(sizeof(char) * MAX_COMMAND_SIZE, RET_OUT_OF_MEMORY);
    /* snprintf is used instead of sprintf to avoid writing past the end of
     * `command` when endpoint or name is long. */
    snprintf(command, MAX_COMMAND_SIZE, "%s ps | grep %s | awk '{print $1}'", endpoint, name);

    utils_exe_cmd_read_out(command, id, CONTAINER_ID_SIZE);

    free(command);
    return 0;
}

int read_memory(const char *id, int *rss)
{
    NULL_PTR_CHECK(id, RET_INVALID_INPUT_PARAM);

    char *command = (char *)UTILS_CALLOC(sizeof(char) * MAX_COMMAND_SIZE, RET_OUT_OF_MEMORY);
    sprintf(command, "ps -aux | grep %s", id);

    FILE *pF = popen(command, "r");
    NULL_PTR_CHECK(pF, ERR_FAILURE);

    char *out = (char *)UTILS_CALLOC(sizeof(char) * MAX_PS_READ_SIZE, RET_OUT_OF_MEMORY);
    fgets(out, MAX_PS_READ_SIZE, pF);

    char *delim = " ";
    char *rss_str = strtok(out, delim);
    for (int i = 0; i < PS_RSS_INDEX - 1; i++) {
        rss_str = strtok(NULL, delim);
    }
    *rss = atoi(rss_str);

    free(out);
    pclose(pF);
    free(command);

    return 0;
}
