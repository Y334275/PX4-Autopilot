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

#include "hardware/esp32s3_soc.h"

// NOTE: these prescales only work on esp32s3
#define SOC_RMT_PRESCALE 1
#define SOC_RMT_CHANNEL_PRESCALE 1

// depend on soc, eg: esp32s3 has 48 words per channel
#ifndef SOC_RMT_MEM_WORDS_PER_CHANNEL
#define SOC_RMT_MEM_WORDS_PER_CHANNEL 48
#endif

// depend on soc, eg: esp32s3 has 8 channels
#ifndef SOC_RMT_CHANNELS_PER_GROUP
#define SOC_RMT_CHANNELS_PER_GROUP 8
#endif

#define RMT_DATA_REG_CH(i) (DR_REG_RMT_BASE + (i) * 0x4)

#define RMT_CHxCONF0_REG(i) (DR_REG_RMT_BASE + ((i) > 3 ? (((i) - 4) * 0x8 + 0x30) : ((i) * 0x4 + 0x20)))
#define RMT_CONF1_REG_CH(i) (DR_REG_RMT_BASE + ((i) - 4) * 0x8 + 0x34)
#define RMT_RX_CARRIER_RM_REG_CH(i) (DR_REG_RMT_BASE + ((i) - 4) * 0x4 + 0x90)
#define RMT_SYS_CONF_REG (DR_REG_RMT_BASE + 0xc0)
#define RMT_REF_CNT_RST_REG (DR_REG_RMT_BASE + 0xc8)

#define RMT_STATUS_REG_CH(i) (DR_REG_RMT_BASE + (i) * 0x4 + 0x50)

#define RMT_INT_RAW_REG (DR_REG_RMT_BASE + 0x70)
#define RMT_INT_ST_REG (DR_REG_RMT_BASE + 0x74)
#define RMT_INT_ENA_REG (DR_REG_RMT_BASE + 0x78)
#define RMT_INT_CLR_REG (DR_REG_RMT_BASE + 0x7c)

#define RMT_CARRIER_DUTY_REG_CH(i) (DR_REG_RMT_BASE + (i) * 0x4 + 0x80)

#define RMT_TX_LIM_REG_CH(i) (DR_REG_RMT_BASE + (i) * 0x4 + 0xa0)
#define RMT_TX_SIM_REG (DR_REG_RMT_BASE + 0xc4)

#define RMT_RX_LIM_REG_CH(i) (DR_REG_RMT_BASE + (i) * 0x4 + 0xb0)

#define RMT_DATE_REG (DR_REG_RMT_BASE + 0xcc)

#define RMTMEM_BASE (DR_REG_RMT_BASE + 0x800)

// RMT_SYS_CONF_REG (0x00C0)
#define RMT_MEM_CLK_FORCE_ON 	(BIT(1))
#define RMT_MEM_CLK_FORCE_ON_M (RMT_MEM_CLK_FORCE_ON_V << RMT_MEM_CLK_FORCE_ON_S)
#define RMT_MEM_CLK_FORCE_ON_V 0x00000001
#define RMT_MEM_CLK_FORCE_ON_S 1

#define RMT_SCLK_DIV_NUM 0x00000ff0
#define RMT_SCLK_DIV_NUM_M (RMT_SCLK_DIV_NUM_V << RMT_SCLK_DIV_NUM_S)
#define RMT_SCLK_DIV_NUM_V 0x000000ff
#define RMT_SCLK_DIV_NUM_S 4

#define RMT_SCLK_DIV_A 0x0003f000
#define RMT_SCLK_DIV_A_M (RMT_SCLK_DIV_A_V << RMT_SCLK_DIV_A_S)
#define RMT_SCLK_DIV_A_V 0x0000003f
#define RMT_SCLK_DIV_A_S 12

#define RMT_SCLK_DIV_B 0x00fc0000
#define RMT_SCLK_DIV_B_M (RMT_SCLK_DIV_B_V << RMT_SCLK_DIV_B_S)
#define RMT_SCLK_DIV_B_V 0x0000003f
#define RMT_SCLK_DIV_B_S 18

#define RMT_SCLK_SEL 0x3f000000
#define RMT_SCLK_SEL_M (RMT_SCLK_SEL_V << RMT_SCLK_SEL_S)
#define RMT_SCLK_SEL_V 0x00000003
#define RMT_SCLK_SEL_S 24

#define RMT_SCLK_ACTIVE (BIT(26))
#define RMT_SCLK_ACTIVE_M (RMT_SCLK_ACTIVE_V << RMT_SCLK_ACTIVE_S)
#define RMT_SCLK_ACTIVE_V 0x00000001
#define RMT_SCLK_ACTIVE_S 26

#define RMT_CLK_EN (BIT(31))
#define RMT_CLK_EN_M (RMT_CLK_EN_V << RMT_CLK_EN_S)
#define RMT_CLK_EN_V 0x00000001
#define RMT_CLK_EN_S 31

//  RMT_CHnCONF0_REG (n: 0-3) (0x0020+0x4*n)
#define RMT_TX_START (BIT(0))
#define RMT_TX_START_M (RMT_TX_START_V << RMT_TX_START_S)
#define RMT_TX_START_V 0x00000001
#define RMT_TX_START_S 0

#define RMT_MEM_RD_RST (BIT(1))
#define RMT_MEM_RD_RST_M (RMT_MEM_RD_RST_V << RMT_MEM_RD_RST_S)
#define RMT_MEM_RD_RST_V 0x00000001
#define RMT_MEM_RD_RST_S 1

#define RMT_APB_MEM_RST (BIT(2))
#define RMT_APB_MEM_RST_M (RMT_APB_MEM_RST_V << RMT_APB_MEM_RST_S)
#define RMT_APB_MEM_RST_V 0x00000001
#define RMT_APB_MEM_RST_S 2

#define RMT_TX_CONTI_MODE (BIT(4))
#define RMT_TX_CONTI_MODE_M (RMT_TX_CONTI_MODE_V << RMT_TX_CONTI_MODE_S)
#define RMT_TX_CONTI_MODE_V 0x00000001
#define RMT_TX_CONTI_MODE_S 4

#define RMT_MEM_TX_WRAP_EN (BIT(3))
#define RMT_MEM_TX_WRAP_EN_M (RMT_MEM_TX_WRAP_EN_V << RMT_MEM_TX_WRAP_EN_S)
#define RMT_MEM_TX_WRAP_EN_V 0x00000001
#define RMT_MEM_TX_WRAP_EN_S 3

#define RMT_IDLE_OUT_LV (BIT(6))
#define RMT_IDLE_OUT_LV_M (RMT_IDLE_OUT_LV_V << RMT_IDLE_OUT_LV_S)
#define RMT_IDLE_OUT_LV_V 0x00000001
#define RMT_IDLE_OUT_LV_S 5

#define RMT_IDLE_OUT_EN (BIT(6))
#define RMT_IDLE_OUT_EN_M (RMT_IDLE_OUT_EN_V << RMT_IDLE_OUT_EN_S)
#define RMT_IDLE_OUT_EN_V 0x00000001
#define RMT_IDLE_OUT_EN_S 6

#define RMT_TX_STOP (BIT(7))
#define RMT_TX_STOP_M (RMT_TX_STOP_V << RMT_TX_STOP_S)
#define RMT_TX_STOP_V 0x00000001
#define RMT_TX_STOP_S 7

#define RMT_DIV_CNT 0x0000ff00
#define RMT_DIV_CNT_M (RMT_DIV_CNT_V << RMT_DIV_CNT_S)
#define RMT_DIV_CNT_V 0x000000ff
#define RMT_DIV_CNT_S 8

#define RMT_MEM_SIZE 0x000f0000
#define RMT_MEM_SIZE_M (RMT_MEM_SIZE_V << RMT_MEM_SIZE_S)
#define RMT_MEM_SIZE_V 0x0000000f
#define RMT_MEM_SIZE_S 16

#define RMT_CONF_UPDATE (BIT(24))
#define RMT_CONF_UPDATE_M (RMT_CONF_UPDATE_V << RMT_CONF_UPDATE_S)
#define RMT_CONF_UPDATE_V 0x00000001
#define RMT_CONF_UPDATE_S 24

// RMT_CHn_TX_LIM_REG (n: 0-3) (0x00A0+0x4*n)
#define RMT_TX_LOOP_NUM 0x0003ff00
#define RMT_TX_LOOP_NUM_M (RMT_TX_LOOP_NUM_V << RMT_TX_LOOP_NUM_S)
#define RMT_TX_LOOP_NUM_V 0x000003ff
#define RMT_TX_LOOP_NUM_S 9

#define RMT_TX_LOOP_CNT_EN (BIT(19))
#define RMT_TX_LOOP_CNT_EN_M (RMT_TX_LOOP_CNT_EN_V << RMT_TX_LOOP_CNT_EN_S)
#define RMT_TX_LOOP_CNT_EN_V 0x00000001
#define RMT_TX_LOOP_CNT_EN_S 19

#define RMT_LOOP_COUNT_RESET (BIT(20))
#define RMT_LOOP_COUNT_RESET_M (RMT_LOOP_COUNT_RESET_V << RMT_LOOP_COUNT_RESET_S)
#define RMT_LOOP_COUNT_RESET_V 0x00000001
#define RMT_LOOP_COUNT_RESET_S 20

#define RMT_LOOP_STOP_EN (BIT(21))
#define RMT_LOOP_STOP_EN_M (RMT_LOOP_STOP_EN_V << RMT_LOOP_STOP_EN_S)
#define RMT_LOOP_STOP_EN_V 0x00000001
#define RMT_LOOP_STOP_EN_S 21

//  RMT_INT_ENA_REG (0x0078)
#define RMT_TX_LOOP_INT_CH(i) (BIT(i + 12))

enum {
	RMT_CLK_APB = 1,
	RMT_CLK_RC_FAST,
	RMT_CLK_XTAL
};
/**
 * @brief The layout of RMT symbol stored in memory, which is decided by the hardware design
 */
typedef union {
	struct {
		uint16_t duration0 : 15; /*!< Duration of level0 */
		uint16_t level0 : 1;     /*!< Level of the first part */
		uint16_t duration1 : 15; /*!< Duration of level1 */
		uint16_t level1 : 1;     /*!< Level of the second part */
	};
	uint32_t val; /*!< Equivalent unsigned value for the RMT symbol */
} rmt_symbol_word_t;

typedef struct {
	struct {
		rmt_symbol_word_t symbols[SOC_RMT_MEM_WORDS_PER_CHANNEL];
	} channels[SOC_RMT_CHANNELS_PER_GROUP];
} rmt_block_mem_t;
