/* SPDX-License-Identifier: Apache-2.0 */
/* Copyright (c) 2026 Junbo Zheng */

/* 'logmask' CLI -- request a level change in a running application.
 *
 * The mask array lives inside each process, so a shell command cannot
 * print another process's table; it can only ask the application to
 * change its own mask. Applications that want runtime control call
 * logmask_mq_open() + poll logmask_mq_process() in their event loop
 * (the unix port's transport), and this CLI sends requests to them:
 *
 *   logmask bt debug
 *   logmask all off
 */

#include <stdio.h>

#include <logmask_unix.h>

static void usage(const char* progname)
{
    printf("Usage: %s <module> <level>\n", progname);
    printf("       %s all <level>\n", progname);
    printf("\nRequests a log level change in the application listening on\n"
           "the %s queue (unix port transport).\n",
           LOGMASK_MQ_NAME);
    printf("\nLevel: debug info notice warning error crit alert emerg off\n"
           "       (or the single letters d i n w e c a r o)\n");
}

int main(int argc, char* argv[])
{
    if (argc != 3) {
        usage(argv[0]);
        return 1;
    }

    if (logmask_mq_send(argv[1], argv[2]) != 0) {
        printf("send failed: no application is listening on %s\n"
               "(it must call logmask_mq_open() from the unix port)\n",
               LOGMASK_MQ_NAME);
        return 1;
    }

    printf("requested: %s -> %s (applied by the receiving application)\n",
           argv[1], argv[2]);
    return 0;
}
