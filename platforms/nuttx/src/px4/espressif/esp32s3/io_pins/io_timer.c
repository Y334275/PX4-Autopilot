/****************************************************************************
 *
 *   Copyright (C) 2021 PX4 Development Team. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 * 3. Neither the name PX4 nor the names of its contributors may be
 *    used to endorse or promote products derived from this software
 *    without specific prior written permission.
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

/**
 * @file io_timer.c
 *
 * Servo driver supporting PWM servos connected to RP2040 PWM blocks.
 */

#include <px4_platform_common/px4_config.h>
#include <systemlib/px4_macros.h>
#include <nuttx/arch.h>
#include <nuttx/irq.h>

#include <sys/types.h>
#include <stdbool.h>

#include <assert.h>
#include <debug.h>
#include <time.h>
#include <sys/queue.h>
#include <errno.h>
#include <string.h>
#include <stdio.h>

#include <arch/board/board.h>
#include <drivers/drv_pwm_output.h>

#include <px4_arch/io_timer.h>

static bool _rmt_initialized = false;
static io_timer_channel_mode_t _rmt_channels[MAX_TIMER_IO_CHANNELS] = {};

static inline uint8_t get_div_cnt(io_timer_channel_mode_t mode)
{
	switch (mode) {
	case IOTimerChanMode_PWMOut:
		return 100; // div to 400KHz

	default:
		return 1; // 40MHz
	}
}

int io_timer_channel_init(unsigned channel, io_timer_channel_mode_t mode,
			  channel_handler_t channel_handler, void *context)
{
	int ret = OK;

	if (io_timer_validate_channel_index(channel) != 0) {
		return -EINVAL;
	}

	if (io_timer_get_channel_mode(channel) > IOTimerChanMode_NotUsed) {
		return OK;
	}

	if (!_rmt_initialized) {
		ret = io_timer_init_timer(0, mode);

		if (ret != 0) {
			return ret;
		}

		_rmt_initialized = true;
	}

	uint32_t gpio = io_timer_channel_get_gpio_output(channel);
	uint8_t div_cnt = get_div_cnt(mode);

	// reset channel
	rmt_reset_ref_cnt(channel);
	rmt_reset_mem(channel);

	// configure gpio
	px4_arch_configgpio(gpio);
	esp32s3_gpio_matrix_out(esp32s3_gpio(gpio), RMT_SIG_OUT0_IDX + channel, false, false);

	// setup channel
	modifyreg32(RMT_CONF0_REG_CH(channel),
		    RMT_IDLE_OUT_LV
		    | RMT_MEM_TX_WRAP_EN
		    | RMT_CARRIER_EN
		    | RMT_DIV_CNT_M
		    | RMT_MEM_SIZE_M,
		    RMT_IDLE_OUT_EN
		    | (div_cnt << RMT_DIV_CNT_S)
		    | (1 << RMT_MEM_SIZE_S)
		    | RMT_TX_CONTI_MODE
		    | RMT_CONF_UPDATE);

	_rmt_channels[channel] = mode;
	return ret;
}

int io_timer_init_timer(unsigned timer, io_timer_channel_mode_t mode)
{
	// set RMT peripheral clock
	modifyreg32(SYSTEM_PERIP_CLK_EN0_REG, 0, SYSTEM_RMT_CLK_EN);
	modifyreg32(SYSTEM_PERIP_RST_EN0_REG, SYSTEM_RMT_RST, 0);

	// apb: 80MHz, rmt_sclk: 40MHz
	putreg32((1 << RMT_SCLK_SEL_S)
		 | (1 << RMT_SCLK_DIV_NUM_S)
		 | (1 << RMT_SCLK_DIV_B_S)
		 | RMT_APB_FIFO_MASK
		 | RMT_SCLK_ACTIVE,
		 RMT_SYS_CONF_REG);
	return OK;
}

int io_timer_set_enable(bool state, io_timer_channel_mode_t mode,
			io_timer_channel_allocation_t masks)
{
	if (!state) {
		return OK;
	}

	io_timer_channel_mode_t last_mode;

	for (unsigned channel = 0; channel < MAX_TIMER_IO_CHANNELS; ++channel) {
		if ((masks & (1 << channel)) == 0) {
			continue;
		}

		rmt_reset_mem(channel);

		last_mode = io_timer_get_channel_mode(channel);

		if (last_mode == mode) {
			continue;
		}

		uint8_t div_cnt = get_div_cnt(mode);

		rmt_reset_ref_cnt(channel);
		modifyreg32(RMT_CONF0_REG_CH(channel), RMT_DIV_CNT_M, (div_cnt << RMT_DIV_CNT_S) | RMT_CONF_UPDATE);

		_rmt_channels[channel] = mode;
	}

	return OK;
}

uint32_t io_timer_get_group(unsigned timer)
{
	uint32_t channels = 0;

	if (timer >= MAX_IO_TIMERS) {
		return channels;
	}

	return (1 << MAX_TIMER_IO_CHANNELS) - 1;

}

int io_timer_validate_channel_index(unsigned channel)
{
	if (channel < MAX_TIMER_IO_CHANNELS) {
		return 0;
	}

	return -EINVAL;
}

int io_timer_get_channel_mode(unsigned channel)
{
	return _rmt_channels[channel];
}

void io_timer_trigger(unsigned channels_mask)
{
	for (unsigned channel = 0; channel < MAX_TIMER_IO_CHANNELS; ++channel) {
		if (channels_mask & (1 << channel)) {
			rmt_start(channel);
		}
	}
}

/**
 * Returns the pin configuration for a specific channel, to be used as GPIO output.
 * 0 is returned if the channel is not valid.
 */
uint32_t io_timer_channel_get_gpio_output(unsigned channel)
{
	return timer_io_channels[channel].gpio_out | GPIO_OUTPUT;
}
