/* SPDX-License-Identifier: Apache-2.0 */
/* Copyright (c) 2026 Junbo Zheng */

#include <string.h>
#include <strings.h>

#include <logmask.h>

/* Generated from LOGMASK_MODULE_LIST -- same source as the enum, so the
 * two can never drift apart.
 */

static const char* const g_logmask_mod_names[LOGMASK_MOD_MAX] = {
#define X(id, name) [LOGMASK_MOD_##id] = #name,
    LOGMASK_MODULE_LIST
#undef X
};

/* Must be statically initialised: LOG_ENABLED() reads this array
 * directly from the log macros, so there is no hook where a lazy init
 * could run. The GCC range designator keeps it correct no matter how
 * many modules the enum grows to.
 */

uint8_t g_logmask[LOGMASK_MOD_MAX] = {
    [0 ... LOGMASK_MOD_MAX - 1] = LOGMASK_MASK_DEFAULT,
};

const char* logmask_level_name(int level)
{
    switch (level) {
    case LOG_EMERG:
        return "EMERG";
    case LOG_ALERT:
        return "ALERT";
    case LOG_CRIT:
        return "CRIT";
    case LOG_ERR:
        return "ERROR";
    case LOG_WARNING:
        return "WARN";
    case LOG_NOTICE:
        return "NOTICE";
    case LOG_INFO:
        return "INFO";
    case LOG_DEBUG:
        return "DEBUG";
    default:
        return "LOG";
    }
}

static const char* level_text(int mask)
{
    switch (mask) {
    case 0:
        return "off";
    case LOG_UPTO(LOG_EMERG):
        return "emerg";
    case LOG_UPTO(LOG_ALERT):
        return "alert";
    case LOG_UPTO(LOG_CRIT):
        return "crit";
    case LOG_UPTO(LOG_ERR):
        return "err";
    case LOG_UPTO(LOG_WARNING):
        return "warning";
    case LOG_UPTO(LOG_NOTICE):
        return "notice";
    case LOG_UPTO(LOG_INFO):
        return "info";
    case LOG_UPTO(LOG_DEBUG):
        return "debug";
    default:
        return "custom";
    }
}

static int parse_level(const char* level)
{
    /* Single letter or full word. */

    if (strcasecmp(level, "debug") == 0) {
        return LOG_UPTO(LOG_DEBUG);
    }
    if (strcasecmp(level, "info") == 0) {
        return LOG_UPTO(LOG_INFO);
    }
    if (strcasecmp(level, "notice") == 0) {
        return LOG_UPTO(LOG_NOTICE);
    }
    if (strcasecmp(level, "warning") == 0 || strcasecmp(level, "warn") == 0) {
        return LOG_UPTO(LOG_WARNING);
    }
    if (strcasecmp(level, "error") == 0 || strcasecmp(level, "err") == 0) {
        return LOG_UPTO(LOG_ERR);
    }
    if (strcasecmp(level, "crit") == 0) {
        return LOG_UPTO(LOG_CRIT);
    }
    if (strcasecmp(level, "alert") == 0) {
        return LOG_UPTO(LOG_ALERT);
    }
    if (strcasecmp(level, "emerg") == 0) {
        return LOG_UPTO(LOG_EMERG);
    }
    if (strcasecmp(level, "off") == 0) {
        return 0;
    }

    switch (level[0]) {
    case 'd':
        return LOG_UPTO(LOG_DEBUG);
    case 'i':
        return LOG_UPTO(LOG_INFO);
    case 'n':
        return LOG_UPTO(LOG_NOTICE);
    case 'w':
        return LOG_UPTO(LOG_WARNING);
    case 'e':
        return LOG_UPTO(LOG_ERR);
    case 'c':
        return LOG_UPTO(LOG_CRIT);
    case 'a':
        return LOG_UPTO(LOG_ALERT);
    case 'r':
        return LOG_UPTO(LOG_EMERG);
    case 'o':
        return 0;
    default:
        return -1;
    }
}

void logmask_show(void)
{
    int name_width = 8;
    int mod;

    for (mod = 0; mod < LOGMASK_MOD_MAX; mod++) {
        int len = strlen(g_logmask_mod_names[mod]);
        if (len > name_width) {
            name_width = len;
        }
    }

    LOG_BACKEND(LOG_INFO, "%-*s  mask  level\n", name_width, "module");
    LOG_BACKEND(LOG_INFO, "%-*s  ----  -------\n", name_width, "--------");

    for (mod = 0; mod < LOGMASK_MOD_MAX; mod++) {
        LOG_BACKEND(LOG_INFO, "%-*s  0x%02x  %s\n", name_width,
                    g_logmask_mod_names[mod], g_logmask[mod],
                    level_text(g_logmask[mod]));
    }
}

int logmask_set_mask(int mod, int mask)
{
    int old;

    if (mod < -1 || mod >= LOGMASK_MOD_MAX) {
        return -1;
    }

    if (mod == -1) {
        for (int i = 0; i < LOGMASK_MOD_MAX; i++) {
            g_logmask[i] = (uint8_t)mask;
        }

        return (int)LOGMASK_MASK_DEFAULT;
    }

    old = g_logmask[mod];
    g_logmask[mod] = (uint8_t)mask;
    return old;
}

int logmask_set_level(const char* mod, const char* level)
{
    int mask;
    int i;

    if (mod == NULL || level == NULL || level[0] == '\0') {
        return -1;
    }

    mask = parse_level(level);
    if (mask < 0) {
        return -1;
    }

    if (strcasecmp(mod, "all") == 0) {
        return logmask_set_mask(-1, mask) >= 0 ? 0 : -1;
    }

    for (i = 0; i < LOGMASK_MOD_MAX; i++) {
        if (strcasecmp(g_logmask_mod_names[i], mod) == 0) {
            logmask_set_mask(i, mask);
            return 0;
        }
    }

    return -1;
}
