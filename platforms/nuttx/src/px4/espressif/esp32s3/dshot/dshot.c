/****************************************************************************
 *
 * Copyright (C) 2024 PX4 Development Team. All rights reserved.
 * Author: Igor Misic <igy1000mb@gmail.com>
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *  notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *  notice, this list of conditions and the following disclaimer in
 *  the documentation and/or other materials provided with the
 *  distribution.
 * 3. Neither the name PX4 nor the names of its contributors may be
 *  used to endorse or promote products derived from this software
 *  without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
 * OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 ****************************************************************************/
#include <px4_platform_common/px4_config.h>
#include <px4_platform_common/micro_hal.h>
#include <px4_platform_common/log.h>

#include <px4_arch/io_timer.h>

#include <drivers/drv_dshot.h>

#include <hardware/esp32s3_system.h>

#if DIRECT_PWM_OUTPUT_CHANNELS > 4
#error "DShot for esp32s3 only has 4 channels."
#endif

#if defined(CONFIG_ESP_RMT)
# error "Can't enable esp rmt and dshot at same time."
#endif

// DShot protocol definitions
#define DSHOT_FRAME_SIZE 16u
#define DSHOT_THROTTLE_POSITION     5u
#define DSHOT_TELEMETRY_POSITION    4u
#define NIBBLES_SIZE                4u
#define DSHOT_NUMBER_OF_NIBBLES     3u

static uint32_t _dshot_frequency = 0;
static int _dshot_channels_mask = 0;

static rmt_symbol_word_t _bit0 = {
	.duration0 = 0,
	.level0 = 1,
	.duration1 = 0,
	.level1 = 0
};
static rmt_symbol_word_t _bit1 = {
	.duration0 = 0,
	.level0 = 1,
	.duration1 = 0,
	.level1 = 0
};
static rmt_symbol_word_t _delay_and_stop = {
	.duration0 = 0,
	.level0 = 0,
	.duration1 = 0,
	.level1 = 0
};

int up_dshot_init(uint32_t channel_mask, unsigned dshot_pwm_freq, bool enable_bidirectional_dshot)
{
	_dshot_frequency = dshot_pwm_freq;
	_dshot_channels_mask = channel_mask;

	// different dshot protocol have its own timing requirements,
	float period_ticks = (float)DSHOT_RESOLUTION_FREQ_HZ / (float)dshot_pwm_freq;
	// 1 and 0 is represented by a 74.850% and 37.425% duty cycle respectively
	unsigned int t1h_ticks = (unsigned int)(period_ticks * 0.7485f + 0.5f); // round up
	unsigned int t1l_ticks = (unsigned int)(period_ticks - t1h_ticks);
	unsigned int t0h_ticks = (unsigned int)(period_ticks * 0.37425f + 0.5f); // round up
	unsigned int t0l_ticks = (unsigned int)(period_ticks - t0h_ticks);
	unsigned int delay_ticks = (unsigned int)(period_ticks * 3);

	int ret = 0;

	_bit0.duration0 = t0h_ticks;
	_bit0.duration1 = t0l_ticks;
	_bit1.duration0 = t1h_ticks;
	_bit1.duration1 = t1l_ticks;
	_delay_and_stop.duration0 = delay_ticks;

	// setup channels
	for (unsigned channel = 0; channel < MAX_TIMER_IO_CHANNELS; ++channel) {
		if (channel_mask & (1 << channel)) {
			ret = io_timer_channel_init(channel, IOTimerChanMode_Dshot, NULL, NULL);

			if (ret != OK) {
				continue;
			}

			dshot_motor_data_set(channel, 0, false);
			// delay and stop symbol
			rmt_set_symbol(channel, DSHOT_FRAME_SIZE, _delay_and_stop);

			rmt_start(channel);
		}
	}

	return _dshot_channels_mask;
}


/**
* bits  1-11    - throttle value (0-47 are reserved for commands, 48-2047 give 2000 steps of throttle resolution)
* bit   12      - dshot telemetry enable/disable
* bits  13-16   - XOR checksum
**/
void dshot_motor_data_set(unsigned channel, uint16_t data, bool telemetry)
{
	uint16_t packet = 0;
	uint16_t checksum = 0;

	packet |= data << DSHOT_THROTTLE_POSITION;
	packet |= ((uint16_t)telemetry & 0x01) << DSHOT_TELEMETRY_POSITION;

	uint16_t csum_data = packet;

	/* XOR checksum calculation */
	csum_data >>= NIBBLES_SIZE;

	for (uint8_t i = 0; i < DSHOT_NUMBER_OF_NIBBLES; i++) {
		checksum ^= (csum_data & 0x0F); // XOR data by nibbles
		csum_data >>= NIBBLES_SIZE;
	}

	packet |= ((checksum) & 0x0F);
	unsigned i = 0;

	for (; i < DSHOT_FRAME_SIZE; ++i) {
		if (packet & (1 << (DSHOT_FRAME_SIZE - i - 1))) {

			rmt_set_symbol(channel, i, _bit1);

		} else {

			rmt_set_symbol(channel, i, _bit0);
		}
	}
}

// Kicks off a DMA transmit for each configured timer and the associated channels
void up_dshot_trigger()
{
	// io_timer_trigger(_dshot_channels_mask);
}

int up_dshot_arm(bool armed)
{
	return io_timer_set_enable(armed, IOTimerChanMode_Dshot, _dshot_channels_mask);
}

int up_bdshot_num_erpm_ready(void)
{
	int num_ready = 0;
	return num_ready;
}

int up_bdshot_get_erpm(uint8_t output_channel, int *erpm)
{

	return PX4_ERROR;
}

int up_bdshot_channel_status(uint8_t channel)
{
	return 0;
}

void up_bdshot_status(void)
{
	PX4_INFO("dshot driver stats:");
}

