/* SPDX-License-Identifier: Apache-2.0 */
/* Copyright (c) 2026 Junbo Zheng */

/* This file is the application's module taxonomy. Include it (directly or
 * via a force-include) in EVERY translation unit that uses logmask,
 * logmask.c included -- the enum and the mask array must be generated
 * from the same list everywhere.
 */

#ifndef _EXAMPLE_LOGMASK_CONF_H_
#define _EXAMPLE_LOGMASK_CONF_H_

#define LOGMASK_MODULE_LIST \
    X(DEFAULT, default)     \
    X(AP, ap)               \
    X(CP, cp)               \
    X(TEE, tee)             \
    X(SENSOR, sensor)       \
    X(AUDIO, audio)         \
    X(OTA, ota)             \
    X(BL, bl)

#endif
