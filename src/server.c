/* SPDX-License-Identifier: Apache-2.0 */
/* Copyright (c) 2026 Junbo Zheng */

/* Server side of the level-change transport: the application that owns
 * the mask opens the queue and applies requests in its own task.
 */

#include <mqueue.h>
#include <stddef.h>

#include <logmask.h>

#ifdef LOGMASK_USE_MQ

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


#endif /* LOGMASK_USE_MQ */
