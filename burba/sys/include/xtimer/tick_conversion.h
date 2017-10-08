/*
 * Copyright (C) 2016 Eistec AB
 *
 * This file is subject to the terms and conditions of the GNU Lesser
 * General Public License v2.1. See the file LICENSE in the top level
 * directory for more details.
 */

/**
 * @ingroup sys_xtimer
 *
 * @{
 * @file
 * @brief   xtimer tick <-> seconds conversions for different values of
 * XTIMER_HZ
 * @author  Joakim Nohlgård <joakim.nohlgard@eistec.se>
 */

#ifndef XTIMER_TICK_CONVERSION_H
#define XTIMER_TICK_CONVERSION_H

#ifndef XTIMER_H
#error "Do not include this file directly! Use xtimer.h instead"
#endif

#include "div.h"

#ifdef __cplusplus
extern "C" {
#endif

/* This is the most straightforward as the xtimer API is based around
 * microseconds for representing time values. */
inline static uint32_t _xtimer_usec_from_ticks(uint32_t ticks) {
    return ticks / XTIMER_USEC_TO_TICKS_FACTOR; /* no-op */
}

inline static uint64_t _xtimer_usec_from_ticks64(uint64_t ticks) {
    return ticks / XTIMER_USEC_TO_TICKS_FACTOR; /* no-op */
}

inline static uint32_t _xtimer_ticks_from_usec(uint32_t usec) {
    return usec * XTIMER_USEC_TO_TICKS_FACTOR; /* no-op */
}

inline static uint64_t _xtimer_ticks_from_usec64(uint64_t usec) {
    return usec * XTIMER_USEC_TO_TICKS_FACTOR; /* no-op */
}

#ifdef __cplusplus
}
#endif

#endif
