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

#include <px4_arch/dshot.h>
#include <px4_arch/io_timer.h>

#include <drivers/drv_dshot.h>

#include <hardware/esp32s3_gpio_sigmap.h>
#include "xtensa.h"
#include <hardware/esp32s3_system.h>

#if DIRECT_PWM_OUTPUT_CHANNELS > 4
#error "DShot channels cannot be greater than 4."
#endif

// DShot protocol definitions
#define DSHOT_FRAME_SIZE 16u
#define DSHOT_THROTTLE_POSITION     5u
#define DSHOT_TELEMETRY_POSITION    4u
#define NIBBLES_SIZE                4u
#define DSHOT_NUMBER_OF_NIBBLES     3u

#define BOARD_DSHOT_RESOLUTION_HZ 40000000 // 40MHz

static rmt_block_mem_t *RMTMEM = (rmt_block_mem_t *)RMTMEM_BASE;

static uint32_t _dshot_frequency = 0;

static rmt_symbol_word_t bit0 = {
	.duration0 = 0,
	.level0 = 1,
	.duration1 = 0,
	.level1 = 0
};
static rmt_symbol_word_t bit1 = {
	.duration0 = 0,
	.level0 = 1,
	.duration1 = 0,
	.level1 = 0
};

void stop_rmt_transmission(unsigned channel)
{
	modifyreg32(RMT_CHxCONF0_REG(channel), 0, RMT_TX_STOP | RMT_CONF_UPDATE);
}

void start_rmt_transmission(unsigned channel)
{
	modifyreg32(RMT_CHxCONF0_REG(channel), 0, RMT_TX_START);
}

// static int rmt_isr(int irq, void *context, void *arg)
// {
// 	for (unsigned channel = 0; channel < DIRECT_PWM_OUTPUT_CHANNELS; ++channel) {
// 		if (getreg32(RMT_INT_ST_REG) & RMT_TX_LOOP_INT_CH(channel)) {
// 			// TODO
// 			putreg32(RMT_TX_LOOP_INT_CH(channel), RMT_INT_CLR_REG);
// 		}
// 	}

// 	return 0;
// }

int up_dshot_init(uint32_t channel_mask, unsigned dshot_pwm_freq, bool enable_bidirectional_dshot)
{
	_dshot_frequency = dshot_pwm_freq;
	// different dshot protocol have its own timing requirements,
	float period_ticks = (float)BOARD_DSHOT_RESOLUTION_HZ / dshot_pwm_freq;
	// 1 and 0 is represented by a 74.850% and 37.425% duty cycle respectively
	unsigned int t1h_ticks = (unsigned int)(period_ticks * 0.7485f);
	unsigned int t1l_ticks = (unsigned int)(period_ticks - t1h_ticks);
	unsigned int t0h_ticks = (unsigned int)(period_ticks * 0.37425f);
	unsigned int t0l_ticks = (unsigned int)(period_ticks - t0h_ticks);

	bit0.duration0 = t0h_ticks;
	bit0.duration1 = t0l_ticks;
	bit1.duration0 = t1h_ticks;
	bit1.duration1 = t1l_ticks;

	// set RMT peripheral clock
	putreg32((RMT_CLK_APB << RMT_SCLK_SEL_S)
		 | (SOC_RMT_PRESCALE << RMT_SCLK_DIV_NUM_S)
		 | (1 << RMT_SCLK_DIV_B_S)
		 | RMT_SCLK_ACTIVE,
		 RMT_SYS_CONF_REG);
	modifyreg32(SYSTEM_PERIP_CLK_EN0_REG, 0, SYSTEM_RMT_CLK_EN);
	modifyreg32(SYSTEM_PERIP_RST_EN0_REG, SYSTEM_RMT_RST, 0);

	// setup channels
	for (unsigned channel = 0; channel < DIRECT_PWM_OUTPUT_CHANNELS; ++channel) {
		if (channel_mask & (1 << channel)) {
			stop_rmt_transmission(channel);

			// reset channel
			modifyreg32(RMT_REF_CNT_RST_REG, 0, (1 << channel));
			modifyreg32(RMT_CHxCONF0_REG(channel), 0, RMT_MEM_RD_RST | RMT_APB_MEM_RST);

			// setup gpio
			px4_arch_configgpio(io_timer_channel_get_gpio_output(channel));
			esp32s3_gpio_matrix_out(esp32s3_gpio(timer_io_channels[channel].gpio_out), RMT_SIG_OUT0_IDX + channel, false, false);

			// setup channel
			modifyreg32(RMT_CHxCONF0_REG(channel), RMT_IDLE_OUT_LV | RMT_TX_CONTI_MODE | RMT_MEM_TX_WRAP_EN, RMT_IDLE_OUT_EN
				    | (SOC_RMT_CHANNEL_PRESCALE << RMT_DIV_CNT_S)
				    | (1 << RMT_MEM_SIZE_S)
				    | RMT_CONF_UPDATE);

			// putreg32((1 << RMT_TX_LOOP_NUM_S) | RMT_TX_LOOP_CNT_EN, RMT_TX_LIM_REG_CH(channel));

			// enable irq
			// putreg32(RMT_TX_LOOP_INT_CH(channel), RMT_INT_ENA_REG);
		}
	}

	// int ret = irq_attach(ESP32S3_IRQ_RMT, rmt_isr, NULL);

	// if (ret == OK) {
	// 	up_enable_irq(ESP32S3_IRQ_RMT);
	// }

	// return ret;

	return 0;
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

			RMTMEM->channels[channel].symbols[i] = bit1;

		} else {

			RMTMEM->channels[channel].symbols[i] = bit0;
		}
	}

	// add stop symbol
	rmt_symbol_word_t stop_symbol = {
		.duration0 = 0,
		.level0 = 0,
		.duration1 = 0,
		.level1 = 0
	};
	RMTMEM->channels[channel].symbols[i] = stop_symbol;
}

// Kicks off a DMA transmit for each configured timer and the associated channels
void up_dshot_trigger()
{
	for (unsigned channel = 0; channel < DIRECT_PWM_OUTPUT_CHANNELS; ++channel) {
		start_rmt_transmission(channel);
	}
}

int up_dshot_arm(bool armed)
{
	if (armed) { return 0; }

	for (unsigned channel = 0; channel < DIRECT_PWM_OUTPUT_CHANNELS; ++channel) {
		stop_rmt_transmission(channel);
	}

	return 0;
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

