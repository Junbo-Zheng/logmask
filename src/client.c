/* SPDX-License-Identifier: Apache-2.0 */
/* Copyright (c) 2026 Junbo Zheng */

/* Client side of the level-change transport: send a request to the
 * application that owns the mask. Compiles standalone -- no dependency on
 * the mask array or the name table, so a small request tool links only
 * this file.
 */

#include <fcntl.h>
#include <mqueue.h>
#include <string.h>

#include <logmask.h>

#ifdef LOGMASK_USE_MQ

int logmask_mq_send(const char* mod, const char* level)
{
    struct logmask_cmd cmd;
    int fd;
    int ret;

    memset(&cmd, 0, sizeof(cmd));
    strncpy(cmd.mod, mod, sizeof(cmd.mod) - 1);
    strncpy(cmd.level, level, sizeof(cmd.level) - 1);

    fd = mq_open(LOGMASK_MQ_NAME, O_WRONLY | O_NONBLOCK | O_CLOEXEC);
    if (fd < 0) {
        return -1;
    }

    ret = mq_send(fd, (const char*)&cmd, sizeof(cmd), 0);
    mq_close(fd);
    return ret < 0 ? -1 : 0;
}

#endif /* LOGMASK_USE_MQ */
