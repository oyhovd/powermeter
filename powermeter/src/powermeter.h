/*
 * Copyright (c) 2015-2016 Intel Corporation
 * Modification copyright (c) 2017 Oyvind Hovdsveen
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/types.h>

void pm_init(void);
void pm_blink_count_update(void);
u8_t pm_moving_avg_get(void);
u32_t pm_last_time_avg_pow_get(u32_t secs);
u8_t pm_blink_count_get(void);
