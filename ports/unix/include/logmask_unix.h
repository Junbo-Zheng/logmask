/* SPDX-License-Identifier: Apache-2.0 */
/* Copyright (c) 2026 Junbo Zheng */

/**
 * @file logmask_unix.h
 * @brief Unix port: level-change transport over a POSIX message queue.
 *
 * The core library is transport-free; this port owns the wire format and
 * the queue mechanics. The receive side applies commands by calling the
 * core's logmask_set_level().
 */

#ifndef _LOGMASK_UNIX_H_
#define _LOGMASK_UNIX_H_

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#ifndef LOGMASK_MQ_NAME
#define LOGMASK_MQ_NAME "/logmask"
#endif

struct logmask_cmd {
    char mod[24]; /* module name, or "all" */
    char level[8]; /* level letter or word */
};

/* Client side: send a level-change request to the application that owns
 * the queue. Returns 0 on success, -1 if no listener / send failed.
 * Compiles standalone -- no dependency on the mask array, so a small
 * request tool links only the transport.
 */
int logmask_mq_send(const char* mod, const char* level);

/* Server side: open the queue from the application that owns the mask.
 * Returns the queue fd, or -1 on failure.
 */
int logmask_mq_open(void);

/* Server side: drain pending requests and apply them via
 * logmask_set_level(). Call from the application's event loop when the
 * fd is readable.
 */
void logmask_mq_process(int fd);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _LOGMASK_UNIX_H_ */
