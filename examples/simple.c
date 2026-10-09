/* SPDX-License-Identifier: Apache-2.0 */
/* Copyright (c) 2026 Junbo Zheng */

/* Minimal usage example. Every line in this file belongs to the module
 * chosen by the #define below; the tag passed to the macros is a free-text
 * label only -- filtering always follows the module, not the tag.
 *
 * The module taxonomy lives in config.h next to this file: the
 * X-macro list written out there generates LOGMASK_MOD_* constants and
 * the runtime name table. Keep X(DEFAULT, default) first -- files that
 * declare no module fall back to it.
 */
#include "config.h"

#include <stdio.h>

/* the default backend is syslog(3); this example overrides it with
 * printf so the output is visible on stdout where CI can assert on it
 */
#define LOG_BACKEND(level, fmt, ...) \
    printf("%s: " fmt, logmask_level_name(level), ##__VA_ARGS__)

#define LOGMASK_LOG_MODULE LOGMASK_MOD_AP
#include <logmask.h>

int main(void)
{
    logmask_show();

    printf("\n-- everything at default (all debug) --\n");
    LOG_INF(ap, "link up\n");
    LOG_DBG(ap, "packet len=%d\n", 42);
    LOG_INF(audio, "negotiated 1M\n");

    printf("\n-- bt module to error: info/debug silenced, error passes --\n");
    logmask_set_level("ap", "e");
    LOG_INF(ap, "link up (silenced)\n");
    LOG_DBG(ap, "packet len=%d (silenced)\n", 42);
    LOG_INF(audio, "negotiated 1M (silenced)\n");
    LOG_ERROR(ap, "link lost\n");

    printf("\n-- bt back to debug --\n");
    logmask_set_level("ap", "debug");
    LOG_DBG(ap, "packet len=%d\n", 43);

    printf("\n-- final state --\n");
    logmask_show();
    return 0;
}
