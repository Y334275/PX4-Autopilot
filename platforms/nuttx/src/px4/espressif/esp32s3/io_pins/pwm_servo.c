/****************************************************************************
 *
 *   Copyright (C) 2012, 2017 PX4 Development Team. All rights reserved.
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

/*
 * @file drv_pwm_servo.c
 *
 * Servo driver supporting PWM servos connected to STM32 timer blocks.
 *
 * Works with any of the 'generic' or 'advanced' STM32 timers that
 * have output pins, does not require an interrupt.
 */

#include <px4_platform_common/px4_config.h>
#include <px4_platform_common/log.h>

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

#include <hardware/esp32s3_gpio_sigmap.h>

static float _period_ticks = 0;

static rmt_symbol_word_t _bit = {
	.duration0 = 1,
	.level0 = 1,
	.duration1 = 1,
	.level1 = 0
};

int up_pwm_servo_set(unsigned channel, uint16_t value)
{
	// 0.4 => PWM_RESOLUTION_FREQ_HZ / 1e6
	unsigned int h_ticks = (unsigned int)(value * 0.4);
	unsigned int l_ticks = (unsigned int)(_period_ticks - h_ticks);

	_bit.duration0 = h_ticks;
	_bit.duration1 = l_ticks;

	rmt_set_symbol(channel, 0, _bit);

	return OK;
}

uint16_t up_pwm_servo_get(unsigned channel)
{
	return 0;
}

int up_pwm_servo_init(uint32_t channel_mask)
{
	int ret;

	for (unsigned channel = 0; channel < MAX_TIMER_IO_CHANNELS; ++channel) {
		if (channel_mask & (1 << channel)) {
			ret = io_timer_channel_init(channel, IOTimerChanMode_PWMOut, NULL, NULL);

			if (ret != OK) {
				continue;
			}
		}

		rmt_set_symbol(channel, 0, _bit);
		rmt_set_symbol(channel, 1, STOP_BIT);
		rmt_start(channel);
	}

	return channel_mask;
}

void up_pwm_servo_deinit(uint32_t channel_mask)
{
	/* disable the timers */
	up_pwm_servo_arm(false, channel_mask);
}

int up_pwm_servo_set_rate_group_update(unsigned group, unsigned rate)
{
	_period_ticks = (float)PWM_RESOLUTION_FREQ_HZ / rate;

	return OK;
}

void up_pwm_update(unsigned channels_mask)
{
	// io_timer_trigger(channels_mask);
}

uint32_t up_pwm_servo_get_rate_group(unsigned group)
{
	return io_timer_get_group(group);
}

void
up_pwm_servo_arm(bool armed, uint32_t channel_mask)
{
	io_timer_set_enable(armed, IOTimerChanMode_PWMOut, channel_mask);
}
