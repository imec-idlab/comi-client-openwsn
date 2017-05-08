/*
 * coap_time.h -- Clock Handling
 *
 * Copyright (C) 2010-2013 Olaf Bergmann <bergmann@tzi.org>
 *
 * This file is part of the CoAP library libcoap. Please see README for terms
 * of use.
 */

/**
 * @file coap_time.h
 * @brief Clock Handling
 */

#ifndef _COAP_TIME_H_
#define _COAP_TIME_H_

/**
 * @defgroup clock Clock Handling
 * Default implementation of internal clock.
 * @{
 */

extern uint32_t board_timer_get(void);

#include "board.h"
typedef uint32_t clock_time_t;

typedef clock_time_t coap_tick_t;
typedef clock_time_t coap_time_t;

/**
 * This data type is used to represent the difference between two clock_tick_t
 * values. This data type must have the same size in memory as coap_tick_t to
 * allow wrapping.
 */
typedef int coap_tick_diff_t;

#define COAP_TICKS_PER_SECOND PORT_TICS_PER_MS

static inline void coap_clock_init(void) {
	board_init();
}

static inline void coap_ticks(coap_tick_t *t) {
  *t = board_timer_get();
}

static inline coap_time_t coap_ticks_to_rt(coap_tick_t t) {
  return t / COAP_TICKS_PER_SECOND;
}

/**
 * Returns @c 1 if and only if @p a is less than @p b where less is defined on a
 * signed data type.
 */
static inline int coap_time_lt(coap_tick_t a, coap_tick_t b) {
  return ((coap_tick_diff_t)(a - b)) < 0;
}

/**
 * Returns @c 1 if and only if @p a is less than or equal @p b where less is
 * defined on a signed data type.
 */
static inline int coap_time_le(coap_tick_t a, coap_tick_t b) {
  return a == b || coap_time_lt(a,b);
}

/** @} */

#endif /* _COAP_TIME_H_ */
