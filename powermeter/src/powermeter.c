/* powermeter.c - Functions for reading and parsing the blinking on the utility meter */

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

#include "powermeter.h"

#include "nrf_adc.h"

#include <zephyr/types.h>
#include <device.h>
#include <flash.h>
#include <stddef.h>
#include <string.h>
#include <errno.h>
#include <misc/printk.h>
#include <misc/byteorder.h>
#include <zephyr.h>


//Uncomment to get some faked measurements
//#define SIMULATE

#define M_SAMPLE_INTERVAL_MS (10) //100Hz
#define M_TAU (128) //In samples
#define M_TAU_M (M_TAU-1)
#define M_FP_SHIFT (32)
#define M_DEBOUNCE_MS (50)
#define M_BLINK_TIMEOUT_MS (5*M_DEBOUNCE_MS)
#define M_INIT_TIME_MS (M_SAMPLE_INTERVAL_MS*M_TAU*4) //4 time constants should get us close
#define M_SECS_SLOT (60)  //Seconds per slot in the array below. Let it be a factor in 60
#define M_SECS_HOUR (3600)

#define M_FLASH_FIRST_PAGE (0x20000)
#define M_FLASH_PAGE_SIZE (1024)
#define M_FLASH_WORDS_RESERVED (64*(M_FLASH_PAGE_SIZE/sizeof(u32_t)))
#define M_EMPTY_FLASH_WORD (0xFFFFFFFF)

typedef enum
{
	M_SAMPLER_UNINIT,
	M_SAMPLER_DARK,
	M_SAMPLER_LIGHT_DEBOUNCE,
	M_SAMPLER_LIGHT,
	M_SAMPLER_DARK_DEBOUNCE,
} m_sampler_state_t;

static u8_t m_sample_get(void);
static void m_slot_update(void);
static void m_update_flash_store(u32_t data);

//Kept static for debugging purposes
static u64_t m_moving_avg;
static u32_t m_meas = 0;
static const u32_t m_flash_page_pointer = M_FLASH_FIRST_PAGE;
u32_t (*m_flash_array)[M_FLASH_WORDS_RESERVED];// = (u32_t (const *)[M_FLASH_WORDS_RESERVED])m_flash_page_pointer;
static u32_t m_next_word_to_write = 0;


//Returns 1 if a blink is detected. Can be called at any time, but must be
//called often enough (at least every M_SAMPLE_INTERVAL_MS)
static u8_t m_sample_get(void)
{
	static m_sampler_state_t state = M_SAMPLER_UNINIT;
	static u64_t next_action_uptime = 0; //Assume this never wraps
	static u64_t state_entered = 0;

	u64_t now = k_uptime_get();

	if(now < next_action_uptime)
	{
		return 0;
	}

	if(next_action_uptime == 0)
	{
		//First call, update next_action_uptime.
		next_action_uptime = now;
		//Initial state
		state = M_SAMPLER_UNINIT;
		state_entered = now;
	}
	next_action_uptime += M_SAMPLE_INTERVAL_MS;

#ifdef SIMULATE
	static u8_t rand_byte;
	rand_byte = (rand_byte + 19);
	u8_t sample_8;
	if(rand_byte > 220)
	{
		sample_8 = 0;
	}
	else
	{
		sample_8 = 255;
	}
	u64_t sample = (u64_t)rand_byte << M_FP_SHIFT;
#else
	u64_t sample = (u64_t)nrf_adc_convert_single(NRF_ADC_CONFIG_INPUT_2) << M_FP_SHIFT;
#endif
	m_moving_avg = (m_moving_avg*M_TAU_M)/M_TAU;
	m_moving_avg += sample/M_TAU;

	switch(state)
	{
		case M_SAMPLER_UNINIT:
			if(now - state_entered >= M_INIT_TIME_MS)
			{
				//Stay here for a while to tune the moving avg
				state = M_SAMPLER_DARK;
				state_entered = now;
			}
			break;

		case M_SAMPLER_DARK:
			if(sample < (2*m_moving_avg)/3)
			{
				//We detected light
				state = M_SAMPLER_LIGHT_DEBOUNCE;
				state_entered = now;
			}
			break;

		case M_SAMPLER_LIGHT_DEBOUNCE:
			if(now - state_entered >= M_DEBOUNCE_MS)
			{
				//Debounce done
				state = M_SAMPLER_LIGHT;
				state_entered = now;
			}
			break;

		case M_SAMPLER_LIGHT:
			if(now - state_entered >= M_BLINK_TIMEOUT_MS)
			{
				//Timeout waiting for light to turn off.
				state = M_SAMPLER_DARK;
				state_entered = now;
			}
			else if(sample >= (2*m_moving_avg)/3)
			{
				//We detected dark
				state = M_SAMPLER_DARK_DEBOUNCE;
				state_entered = now;
			}
			break;

		case M_SAMPLER_DARK_DEBOUNCE:
			if(now - state_entered >= M_DEBOUNCE_MS)
			{
				//Debounce done
				state = M_SAMPLER_DARK;
				state_entered = now;

				return 1; //Count the blink
			}
			break;

		default:
			printk("Invalid state");
			while(1);
	}
	return 0;
}

static void m_slot_update(void)
{
	//Write flash if a new slot has started
	u32_t current_second = (u32_t)(k_uptime_get()/1000);
	static u32_t prev_slot = 0;
	u32_t current_slot = (current_second/M_SECS_SLOT) % (2);
	if(prev_slot != current_slot)
	{
		m_update_flash_store(m_meas);
		prev_slot = current_slot;
		m_meas=0;
	}
}

static void m_update_flash_store(u32_t data)
{
	if(data == M_EMPTY_FLASH_WORD)
	{
		data--;
	}

	u32_t offset = (u32_t)m_flash_page_pointer + m_next_word_to_write*sizeof(u32_t);
	//Enable writing and write the word
	struct device *flash_dev;
	flash_dev = device_get_binding(CONFIG_SOC_FLASH_NRF5_DEV_NAME);
	flash_write_protection_set(flash_dev, false);
	if (flash_write(flash_dev, offset, &data,
				sizeof(data)) != 0) {
		while(1);
	}
	flash_write_protection_set(flash_dev, true);

	m_next_word_to_write++;
	if(m_next_word_to_write == M_FLASH_WORDS_RESERVED)
	{
		//wraparound
		m_next_word_to_write = 0;
	}
	//The word after the next must be empty. If not, the page must be
	//erased.
	u32_t next_empty_word = m_next_word_to_write + 1;
	if(next_empty_word == M_FLASH_WORDS_RESERVED)
	{
		//wraparound
		next_empty_word = 0;
	}

	if((*m_flash_array)[next_empty_word] != M_EMPTY_FLASH_WORD)
	{
		//TODO
		//Find start of page
		//erase it
	}
}

void pm_init(void)
{
	nrf_adc_config_t config = {
		.resolution = NRF_ADC_CONFIG_RES_8BIT,
		.scaling = NRF_ADC_CONFIG_SCALING_INPUT_FULL_SCALE,
		.reference = NRF_ADC_CONFIG_REF_VBG,
	};
	m_flash_array = (u32_t (*)[M_FLASH_WORDS_RESERVED])m_flash_page_pointer;
	nrf_adc_configure(&config);

	//find next word to write
	u32_t i;
	for(i = 0; i < M_FLASH_WORDS_RESERVED; i++)
	{
		if((*m_flash_array)[i] == M_EMPTY_FLASH_WORD)
		{
			break;
		}
	}
	m_next_word_to_write = i;
}

void pm_blink_count_update(void)
{
	m_slot_update();
	u8_t sample = m_sample_get();
	m_meas += sample;
}

u8_t pm_moving_avg_get(void)
{
	return m_moving_avg >> M_FP_SHIFT;
}

u32_t pm_last_time_avg_pow_get(u32_t secs)
{
	u32_t blinks = 0;
	for(u32_t i = 0; i < secs/M_SECS_SLOT; i++)
	{
		//Get the data from previous slot and before
		u32_t word = (m_next_word_to_write-1-i) % M_FLASH_WORDS_RESERVED;
		u32_t value = (*m_flash_array)[word];
		if(value == M_EMPTY_FLASH_WORD)
		{
			value = 0;
		}
		blinks+=value;
	}

	return (M_SECS_HOUR*blinks)/secs;
}

u8_t pm_blink_count_get(void)
{
	return m_meas;
}

