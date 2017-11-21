/* powermeter.h - Functions for reading and parsing the blinking on the utility meter */

/*
 * Copyright (c) 2017 Oyvind Hovdsveen
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <zephyr/types.h>

void pm_init(void);
void pm_blink_count_update(void);
u8_t pm_moving_avg_get(void);
u32_t pm_last_time_avg_pow_get(u32_t secs);
u8_t pm_blink_count_get(void);
