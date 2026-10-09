/* SPDX-License-Identifier: Apache-2.0 */
/* Copyright (c) 2026 Junbo Zheng */

/* Receiver demo: a tiny application that owns the logmask queue. Run it,
 * then use the CLI from another terminal to change its module levels:
 *
 *   ./logmask_server                          (terminal 1)
 *   logmask_cli ap debug                        (terminal 2)
 *
 * A real application wires the same two calls into its own event loop
 * instead of this standalone poll loop.
 */

#include <poll.h>
#include <stdio.h>

/* the module taxonomy comes in via force-include and the backend is
 * switched to printf via LOGMASK_BACKEND_PRINTF -- both set in
 * CMakeLists.txt, so every TU of this demo agrees
 */
#include "config.h"

#include <logmask_unix.h>

#define LOGMASK_LOG_MODULE LOGMASK_MOD_AP
#include <logmask.h>

int main(void)
{
    int fd = logmask_mq_open();
    if (fd < 0) {
        perror("logmask_mq_open");
        return 1;
    }

    LOG_BACKEND(LOG_INFO, "listening on %s -- try: logmask_cli ap debug\n",
                LOGMASK_MQ_NAME);

    struct pollfd pfd = { .fd = fd, .events = POLLIN };
    for (;;) {
        if (poll(&pfd, 1, -1) > 0) {
            logmask_mq_process(fd);
            logmask_show();
        }
    }
}
