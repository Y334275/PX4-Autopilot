/****************************************************************************
 *
 *   Copyright (c) 2021 PX4 Development Team. All rights reserved.
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
 * @file board_config.h
 *
 * board internal definitions
 */

#pragma once

/****************************************************************************************************
 * Included Files
 ****************************************************************************************************/

#include <px4_platform_common/px4_config.h>
#include <nuttx/compiler.h>
#include <stdint.h>

#define PX4_NUMBER_I2C_BUSES 2

#define BOARD_SPI_BUS_MAX_BUS_ITEMS 2

#define ESP32S3_ATTENUATION_12

/*
 * ADC channels
 *
 * These are the channel numbers of the ADCs of the microcontroller that can be used by the Px4 Firmware in the adc driver
 */
#define ESP32S3_USE_ADC2
#define ADC_CHANNELS (1 << 6) | (1 << 7)	// Change this later based on the adc channels actually used
#define ADC_BATTERY_CURRENT_CHANNEL 6
#define ADC_BATTERY_VOLTAGE_CHANNEL 7

/* High-resolution timer */
/*
 * For wifi to work, it needs to use its own timer.
 * Make sure you are not using the timer for hrt
 * that is being using for the wifi
*/
#define HRT_TIMER 			2

/* This board provides a DMA pool and APIs */			// Needs to be figured out
#define BOARD_DMA_ALLOC_POOL_SIZE 	2048

#define BOARD_ENABLE_CONSOLE_BUFFER
#define BOARD_CONSOLE_BUFFER_SIZE	(1024*3)

/* PWM
 */
#define DIRECT_PWM_OUTPUT_CHANNELS      4

// Has pwm outputs
#define BOARD_HAS_PWM    		DIRECT_PWM_OUTPUT_CHANNELS

#define GPIO_nLED_GREEN 		21 | GPIO_OUTPUT

#define BOARD_ADC_USB_CONNECTED 1

__BEGIN_DECLS

#ifndef __ASSEMBLY__

extern void board_peripheral_reset(int ms);
extern void esp32s3_spiinitialize(void);

int board_spiflash_init(void);
void esp_newlib_init(void);

#include <px4_platform_common/board_common.h>

#endif /* __ASSEMBLY__ */

__END_DECLS
