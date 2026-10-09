/* SPDX-License-Identifier: Apache-2.0 */
/* Copyright (c) 2026 Junbo Zheng */

/* POSIX message-queue transport, both ends. The client side (send) is
 * self-contained so a small request tool links only this file; the server
 * side applies received commands through the core's logmask_set_level().
 */

#include <fcntl.h>
#include <mqueue.h>
#include <stddef.h>
#include <string.h>

#include <logmask.h>
#include <logmask_unix.h>

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

int logmask_mq_open(void)
{
    struct mq_attr attr = { 0 };

    attr.mq_msgsize = sizeof(struct logmask_cmd);
    attr.mq_maxmsg = 8;

    return mq_open(LOGMASK_MQ_NAME,
                   O_RDWR | O_CREAT | O_NONBLOCK | O_CLOEXEC, 0660, &attr);
}

void logmask_mq_process(int fd)
{
    struct logmask_cmd cmd;

    while (1) {
        ssize_t n = mq_receive(fd, (char*)&cmd, sizeof(cmd), NULL);
        if (n < 0) {
            /* EAGAIN: queue drained, back to waiting on the loop */
            return;
        }

        if (n != sizeof(cmd)) {
            continue;
        }

        if (logmask_set_level(cmd.mod, cmd.level) == 0) {
            LOG_BACKEND(LOG_INFO, "logmask: %s level applied\n", cmd.mod);
        } else {
            LOG_BACKEND(LOG_WARNING,
                        "logmask: unknown module '%s' or level '%s'\n",
                        cmd.mod, cmd.level);
        }
    }
}
