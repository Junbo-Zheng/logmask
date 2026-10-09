/* SPDX-License-Identifier: Apache-2.0 */
/* Copyright (c) 2026 Junbo Zheng */

/**
 * @file logmask.h
 * @brief Runtime per-module log level filtering, portable C99 core.
 *
 * logmask adds a per-module level gate in front of whatever logging
 * backend you use: each source directory declares which module it belongs
 * to, and every LOGMASK_* log line first checks the level mask of its own
 * module before the line is emitted. One module can be turned up to debug
 * at runtime while the rest stay quiet.
 *
 * The default output backend is syslog(3) and levels use the POSIX syslog
 * names, so unix integrations need zero mapping; the backend is pluggable
 * (LOG_BACKEND) for platforms without syslog.h.
 *
 * A .c file states its module before its first #include (the header may be
 * pulled in indirectly, and a late #define lands after the fallback below
 * has already applied):
 *
 *   #define LOGMASK_LOG_MODULE LOGMASK_MOD_BT
 *   #include <logmask.h>
 *
 * Files that declare nothing fall back to LOGMASK_MOD_DEFAULT.
 */

#ifndef _LOGMASK_H_
#define _LOGMASK_H_

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include <stdint.h>

/* Levels are the POSIX syslog names with the standard numbering, so
 * syslog-based code reads naturally and unix/NuttX integrations pass them
 * straight through. When syslog.h is also included, the identical
 * definitions merge silently (same values, object-like macros). Values
 * must stay contiguous and ordered: the mask encoding is 1 << level and
 * LOG_UPTO assumes EMERG lowest, DEBUG highest.
 */

#ifndef LOG_EMERG
#define LOG_EMERG 0
#endif
#ifndef LOG_ALERT
#define LOG_ALERT 1
#endif
#ifndef LOG_CRIT
#define LOG_CRIT 2
#endif
#ifndef LOG_ERR
#define LOG_ERR 3
#endif
#ifndef LOG_WARNING
#define LOG_WARNING 4
#endif
#ifndef LOG_NOTICE
#define LOG_NOTICE 5
#endif
#ifndef LOG_INFO
#define LOG_INFO 6
#endif
#ifndef LOG_DEBUG
#define LOG_DEBUG 7
#endif


/* The module list is the project's own taxonomy, so it is defined OUTSIDE
 * the library: consumers pass their list as a compile definition (or
 * #define it before the first #include) and never edit this header --
 * upstream updates then merge conflict-free:
 *
 *   target_compile_definitions(app PRIVATE
 *       LOGMASK_MODULE_LIST="X(DEFAULT,default) X(BT,bt) X(UI,ui)")
 *
 * Names are bare tokens (no quotes); the name table stringifies them.
 * Keep X(DEFAULT, default) as the first entry: files that declare no
 * module fall back to it. The enum here and the name table in logmask.c
 * are both generated from the list, so they cannot drift apart.
 */

#ifndef LOGMASK_MODULE_LIST
#define LOGMASK_MODULE_LIST X(DEFAULT, default)
#endif

enum logmask_mod_e {
#define X(id, name) LOGMASK_MOD_##id,
    LOGMASK_MODULE_LIST
#undef X
    LOGMASK_MOD_MAX
};

#ifndef LOGMASK_LOG_MODULE
#define LOGMASK_LOG_MODULE LOGMASK_MOD_DEFAULT
#endif

/* Boot-time mask for every module. 0xff lets everything through. */

#ifndef LOGMASK_MASK_DEFAULT
#define LOGMASK_MASK_DEFAULT LOG_ALL
#endif

extern uint8_t g_logmask[LOGMASK_MOD_MAX];

#define LOG_ENABLED(level) \
    ((g_logmask[LOGMASK_LOG_MODULE] & LOG_MASK(level)) != 0)

/* Pluggable emit backend: receive the already-prefixed format string and
 * forward it to the real logger. The default is syslog(3) -- the level
 * values are the syslog priorities, so it passes straight through and the
 * lines land under the process's normal syslog filtering. Platforms
 * without syslog.h override LOG_BACKEND before including this header
 * (printf, a UART write, whatever the target logs through).
 */

#ifndef LOG_BACKEND
/* Default backend: syslog when the platform has it, stdio otherwise.
 * Define LOGMASK_BACKEND_PRINTF to force stdio (demos, simple hosts);
 * define LOG_BACKEND yourself to route anywhere else (UART, ...).
 * The stdio fallback drops the level -- it is a last resort, not a
 * logger.
 */
#if defined(LOGMASK_BACKEND_PRINTF) || \
    !(defined(__has_include) && __has_include(<syslog.h>))
#include <stdio.h>

#define LOG_BACKEND(level, fmt, ...) printf(fmt, ##__VA_ARGS__)
#else
#include <syslog.h>

#define LOG_BACKEND(level, fmt, ...) syslog((level), fmt, ##__VA_ARGS__)
#endif
#endif

/* Level name ("DEBUG", "INFO", ...) for backends that want a textual
 * prefix; the syslog default does not need it.
 */
const char* logmask_level_name(int level);

/* Mask macros use the POSIX syslog names. With the default backend,
 * syslog.h has already provided them; these fallbacks only fire on
 * custom-backend platforms that have no syslog.h. Bodies follow the
 * syslog.h semantics (NuttX's LOG_GETPRI wrapper yields the same result
 * for plain levels, so either definition works with g_logmask).
 * If you override LOG_BACKEND but still use syslog.h elsewhere, include
 * syslog.h first to avoid a macro-redefinition warning there.
 */
#ifndef LOG_MASK
#define LOG_MASK(level) (1u << (level))
#endif
#ifndef LOG_UPTO
#define LOG_UPTO(level) ((1u << ((level) + 1)) - 1u)
#endif
#ifndef LOG_ALL
#define LOG_ALL 0xffu
#endif

/* Single place where a logmask log line is actually emitted: the module
 * gate is checked first, then the line goes to the backend.
 */

#ifdef LOGMASK_FUNC_NAMES
#define LOG_PRINT(level, tag, format, ...)                         \
    do {                                                           \
        if (LOG_ENABLED(level)) {                                  \
            LOG_BACKEND(level, "[%s] %s: " format, #tag, __func__, \
                        ##__VA_ARGS__);                            \
        }                                                          \
    } while (0)
#else
#define LOG_PRINT(level, tag, format, ...)                            \
    do {                                                              \
        if (LOG_ENABLED(level)) {                                     \
            LOG_BACKEND(level, "[%s]: " format, #tag, ##__VA_ARGS__); \
        }                                                             \
    } while (0)
#endif

#define LOG_DBG(tag, format, ...) \
    LOG_PRINT(LOG_DEBUG, tag, format, ##__VA_ARGS__)

#define LOG_INF(tag, format, ...) \
    LOG_PRINT(LOG_INFO, tag, format, ##__VA_ARGS__)

#define LOG_WRN(tag, format, ...) \
    LOG_PRINT(LOG_WARNING, tag, format, ##__VA_ARGS__)

#define LOG_ERROR(tag, format, ...) \
    LOG_PRINT(LOG_ERR, tag, format, ##__VA_ARGS__)

/* Print every module with its current level (via LOG_BACKEND). */
void logmask_show(void);

/* Transports (requesting level changes from outside the process) are
 * port-owned: see ports/<platform>/ for the wire format and the send /
 * receive helpers -- a port's receive side applies commands by calling
 * this function.
 *
 * mod: module name from the list, or "all".
 * level: one letter (d i n w e c a r o), or a full level word (debug, info,
 *      warning, ...).
 * Returns 0 on success, -1 on unknown module or level.
 *
 * Stores are single-byte writes, so calling this from any thread is safe
 * on architectures with atomic aligned byte stores; readers only ever see
 * an old or a new mask, never a torn one.
 */
int logmask_set_level(const char* mod, const char* level);

/* Same as above but takes a raw mask (LOG_UPTO(...) / LOG_ALL / 0).
 * Returns the previous mask, or -1 on unknown module.
 */
int logmask_set_mask(int mod, int mask);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _LOGMASK_H_ */
