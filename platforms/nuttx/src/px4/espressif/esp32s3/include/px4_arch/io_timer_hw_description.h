/****************************************************************************
 *
 *   Copyright (C) 2019 PX4 Development Team. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *	notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *	notice, this list of conditions and the following disclaimer in
 *	the documentation and/or other materials provided with the
 *	distribution.
 * 3. Neither the name PX4 nor the names of its contributors may be
 *	used to endorse or promote products derived from this software
 *	without specific prior written permission.
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

#pragma once


#include <px4_arch/io_timer.h>
#include <px4_arch/hw_description.h>
#include <px4_platform_common/constexpr_util.h>
#include <px4_platform_common/px4_config.h>
#include <px4_platform/io_timer_init.h>
#include <esp32s3_tim.h>
#include <esp32s3_gpio.h>

// static inline constexpr timer_io_channels_t initIOTimerGPIOInOut(Timer::TimerChannel timer, GPIO::GPIOPin pin);

#define initIOTimerChannelCapture initIOTimerChannel // alias, used for param metadata generation

static inline constexpr timer_io_channels_t initIOTimerChannel(const io_timers_t io_timers_conf[MAX_IO_TIMERS],
		Timer::TimerChannel timer, GPIO::GPIOPin pin)
{
	timer_io_channels_t ret = {};
	ret.gpio_out = pin.pin;
	ret.timer_channel = timer.channel;
	ret.timer_index = timer.timer;

	return ret;
}

static inline constexpr timer_io_channels_t initIOTimerChannelOutputClear(const io_timers_t
		io_timers_conf[MAX_IO_TIMERS], Timer::TimerChannel timer, GPIO::GPIOPin pin)
{
	timer_io_channels_t ret = initIOTimerChannel(io_timers_conf, timer, pin);
	ret.gpio_out |= 0;
	return ret;
}


static inline constexpr io_timers_t initIOTimer(Timer::Timer timer)
{
	bool nuttx_config_timer_enabled = true;
	io_timers_t ret{};

	switch (timer) {
	case Timer::Timer0: // refers to MCPWM peripheral 1
		ret.base = 0;
		break;

	case Timer::Timer1: // refers to MCPWM peripheral 2
		ret.base = 1;
		break;

	case Timer::Timer2: // refers to MCPWM peripheral 2
		ret.base = 2;
		break;

	case Timer::Timer3: // refers to MCPWM peripheral 2
		ret.base = 4;
		break;

	default: break;
	}

	constexpr_assert(nuttx_config_timer_enabled,
			 "IO Timer requires NuttX timer config to be Enabled (CONFIG_ESP32S3_LEDC_TIMx)");


	return ret;
}
