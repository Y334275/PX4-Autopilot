/****************************************************************************
 *
 *   Copyright (c) 2019 PX4 Development Team. All rights reserved.
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
#pragma once

#include <board_config.h>
#include <hardware/esp32s3_soc.h>
#include <hardware/esp32s3_sens.h>

#undef BOARD_ADC_POS_REF_V
#define BOARD_ADC_POS_REF_V		3.1f

#define SYSTEM_ADC_BASE 	DR_REG_SENS_BASE
#define HW_REV_VER_ADC_BASE 	DR_REG_SENS_BASE

#define  APB_SARADC_APB_ADC_ARB_CTRL_REG (DR_REG_APB_SARADC_BASE + 0x38)

/* APB_SARADC_ADC_ARB_RTC_FORCE : R/W ;bitpos:[3] ;default: 1'b0 ; */

/* description: adc2 arbiter force rtc */

#define APB_SARADC_ADC_ARB_RTC_FORCE    (BIT(3))
#define APB_SARADC_ADC_ARB_RTC_FORCE_M  (BIT(3))
#define APB_SARADC_ADC_ARB_RTC_FORCE_V  0x1
#define APB_SARADC_ADC_ARB_RTC_FORCE_S  3

/* APB_SARADC_ADC_ARB_WIFI_FORCE : R/W ;bitpos:[4] ;default: 1'b0 ; */

/* description: adc2 arbiter force wifi */

#define APB_SARADC_ADC_ARB_WIFI_FORCE    (BIT(4))
#define APB_SARADC_ADC_ARB_WIFI_FORCE_M  (BIT(4))
#define APB_SARADC_ADC_ARB_WIFI_FORCE_V  0x1
#define APB_SARADC_ADC_ARB_WIFI_FORCE_S  4

/* APB_SARADC_ADC_ARB_GRANT_FORCE : R/W ;bitpos:[5] ;default: 1'b0 ; */

/* description: adc2 arbiter force grant */

#define APB_SARADC_ADC_ARB_GRANT_FORCE    (BIT(5))
#define APB_SARADC_ADC_ARB_GRANT_FORCE_M  (BIT(5))
#define APB_SARADC_ADC_ARB_GRANT_FORCE_V  0x1
#define APB_SARADC_ADC_ARB_GRANT_FORCE_S  5

/* APB_SARADC_ADC_ARB_RTC_PRIORITY : R/W ; */

/* description: adc2 arbiter rtc priority */

#define APB_SARADC_ADC_ARB_RTC_PRIORITY    0x00000003
#define APB_SARADC_ADC_ARB_RTC_PRIORITY_M  ((APB_SARADC_ADC_ARB_RTC_PRIORITY_V) << APB_SARADC_ADC_ARB_RTC_PRIORITY_S)
#define APB_SARADC_ADC_ARB_RTC_PRIORITY_V  0x3
#define APB_SARADC_ADC_ARB_RTC_PRIORITY_S  8

/* APB_SARADC_ADC_ARB_WIFI_PRIORITY : R/W ; */

/* description: adc2 arbiter rtc priority */

#define APB_SARADC_ADC_ARB_WIFI_PRIORITY    0x00000003
#define APB_SARADC_ADC_ARB_WIFI_PRIORITY_M  ((APB_SARADC_ADC_ARB_WIFI_PRIORITY_V) << APB_SARADC_ADC_ARB_WIFI_PRIORITY_S)
#define APB_SARADC_ADC_ARB_WIFI_PRIORITY_V  0x3
#define APB_SARADC_ADC_ARB_WIFI_PRIORITY_S  10

/* APB_SARADC_ADC_ARB_FIX_PRIORITY : R/W ;bitpos:[5] ;default: 1'b0 ; */

/* description: adc2 arbiter fix priority */

#define APB_SARADC_ADC_ARB_FIX_PRIORITY    (BIT(12))
#define APB_SARADC_ADC_ARB_FIX_PRIORITY_M  (BIT(12))
#define APB_SARADC_ADC_ARB_FIX_PRIORITY_V  0x1
#define APB_SARADC_ADC_ARB_FIX_PRIORITY_S  12

#define I2C_ADC2_DEF            (0x5)
#define I2C_ADC2_DEF_MSB        (0x6)
#define I2C_ADC2_DEF_LSB        (0x4)

#define I2C_ADC2_INITVAL_L      (0x3)
#define I2C_ADC2_INITVAL_L_MSB  (0x7)
#define I2C_ADC2_INITVAL_L_LSB  (0x0)

#define I2C_ADC2_INITVAL_H      (0x4)
#define I2C_ADC2_INITVAL_H_MSB  (0x3)
#define I2C_ADC2_INITVAL_H_LSB  (0x0)

#include <px4_platform/adc.h>
