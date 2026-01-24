/****************************************************************************
 *
 *   Copyright (C) 2019 PX4 Development Team. All rights reserved.
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

#include <board_config.h>
#include <stdint.h>
#include <drivers/drv_adc.h>
#include <drivers/drv_hrt.h>
#include <px4_arch/adc.h>

#include <px4_arch/rtc_io_reg.h>

extern "C" {
#include <xtensa.h>
#include <hardware/regi2c_ctrl.h>
}
#include <esp32s3_gpio.h>
#include <hardware/esp32s3_rtccntl.h>
#include <hardware/esp32s3_system.h>
#include <hardware/regi2c_saradc.h>

#include <esp_efuse_rtc_calib.h>

#ifdef ESP32S3_USE_ADC2
# define SENS_SAR_READER_CTRL_REG(a)	((a) + 0x24)
# define SENS_SAR_MEAS_CTRL2_REG(a)	((a) + 0x30)
# define SENS_SAR_MEAS_MUX_REG(a)	((a) + 0x34)
# define SENS_SAR_ATTEN_REG(a)		((a) + 0x38)
# define SENS_SAR_DATA_INV		SENS_SAR2_DATA_INV
# define RTCIO_OFFSET			11	// adc2 start from gpio11
# define ADC_UINT			1
#else // else ESP32S3_USE_ADC2
# define SENS_SAR_READER_CTRL_REG(a)	((a) + 0x0)
# define SENS_SAR_MEAS_CTRL2_REG(a)	((a) + 0xc)
# define SENS_SAR_MEAS_MUX_REG(a)	((a) + 0x10)
# define SENS_SAR_ATTEN_REG(a)		((a) + 0x14)
# define RTCIO_OFFSET			0
# define SENS_SAR_DATA_INV		SENS_SAR1_DATA_INV
# define ADC_UINT			0
#endif // ESP32S3_USE_ADC2

#define SOC_RTCIO_PIN_COUNT   		22

#define ESP32S3_ADC_MAX_CHANNELS	10

#define DEFAULT_ATTEN			0x3

#ifdef PX4_ADC_INTERNAL_TEMP_SENSOR_CHANNEL
# if PX4_ADC_INTERNAL_TEMP_SENSOR_CHANNEL < ESP32S3_ADC_MAX_CHANNELS
#  error "invalid temperature sensor channel, must greater than 10"
# endif
#else
# define PX4_ADC_INTERNAL_TEMP_SENSOR_CHANNEL	ESP32S3_ADC_MAX_CHANNELS
#endif

#define RTCIO_NUM(c)			(RTCIO_OFFSET + (c))

static uint16_t enabled_channels = 0;

typedef struct {
	uint32_t reg;       /*!< Register of RTC pad, or 0 if not an RTC GPIO */
	uint32_t mux;       /*!< Bit mask for selecting digital pad or RTC pad */
	uint32_t func;      /*!< Shift of pad function (FUN_SEL) field */
	uint32_t ie;        /*!< Mask of input enable */
	uint32_t pullup;    /*!< Mask of pullup enable */
	uint32_t pulldown;  /*!< Mask of pulldown enable */
} rtc_io_desc_t;

static const rtc_io_desc_t rtc_io_desc[SOC_RTCIO_PIN_COUNT] = {
	/*REG                    MUX select                    function select               Input enable                 Pullup                    Pulldown */
	{RTC_IO_TOUCH_PAD0_REG,  RTC_IO_TOUCH_PAD0_MUX_SEL_M,  RTC_IO_TOUCH_PAD0_FUN_SEL_S,  RTC_IO_TOUCH_PAD0_FUN_IE_M,  RTC_IO_TOUCH_PAD0_RUE_M,  RTC_IO_TOUCH_PAD0_RDE_M}, //0
	{RTC_IO_TOUCH_PAD1_REG,  RTC_IO_TOUCH_PAD1_MUX_SEL_M,  RTC_IO_TOUCH_PAD1_FUN_SEL_S,  RTC_IO_TOUCH_PAD1_FUN_IE_M,  RTC_IO_TOUCH_PAD1_RUE_M,  RTC_IO_TOUCH_PAD1_RDE_M}, //1
	{RTC_IO_TOUCH_PAD2_REG,  RTC_IO_TOUCH_PAD2_MUX_SEL_M,  RTC_IO_TOUCH_PAD2_FUN_SEL_S,  RTC_IO_TOUCH_PAD2_FUN_IE_M,  RTC_IO_TOUCH_PAD2_RUE_M,  RTC_IO_TOUCH_PAD2_RDE_M}, //2
	{RTC_IO_TOUCH_PAD3_REG,  RTC_IO_TOUCH_PAD3_MUX_SEL_M,  RTC_IO_TOUCH_PAD3_FUN_SEL_S,  RTC_IO_TOUCH_PAD3_FUN_IE_M,  RTC_IO_TOUCH_PAD3_RUE_M,  RTC_IO_TOUCH_PAD3_RDE_M}, //3
	{RTC_IO_TOUCH_PAD4_REG,  RTC_IO_TOUCH_PAD4_MUX_SEL_M,  RTC_IO_TOUCH_PAD4_FUN_SEL_S,  RTC_IO_TOUCH_PAD4_FUN_IE_M,  RTC_IO_TOUCH_PAD4_RUE_M,  RTC_IO_TOUCH_PAD4_RDE_M}, //4
	{RTC_IO_TOUCH_PAD5_REG,  RTC_IO_TOUCH_PAD5_MUX_SEL_M,  RTC_IO_TOUCH_PAD5_FUN_SEL_S,  RTC_IO_TOUCH_PAD5_FUN_IE_M,  RTC_IO_TOUCH_PAD5_RUE_M,  RTC_IO_TOUCH_PAD5_RDE_M}, //5
	{RTC_IO_TOUCH_PAD6_REG,  RTC_IO_TOUCH_PAD6_MUX_SEL_M,  RTC_IO_TOUCH_PAD6_FUN_SEL_S,  RTC_IO_TOUCH_PAD6_FUN_IE_M,  RTC_IO_TOUCH_PAD6_RUE_M,  RTC_IO_TOUCH_PAD6_RDE_M}, //6
	{RTC_IO_TOUCH_PAD7_REG,  RTC_IO_TOUCH_PAD7_MUX_SEL_M,  RTC_IO_TOUCH_PAD7_FUN_SEL_S,  RTC_IO_TOUCH_PAD7_FUN_IE_M,  RTC_IO_TOUCH_PAD7_RUE_M,  RTC_IO_TOUCH_PAD7_RDE_M}, //7
	{RTC_IO_TOUCH_PAD8_REG,  RTC_IO_TOUCH_PAD8_MUX_SEL_M,  RTC_IO_TOUCH_PAD8_FUN_SEL_S,  RTC_IO_TOUCH_PAD8_FUN_IE_M,  RTC_IO_TOUCH_PAD8_RUE_M,  RTC_IO_TOUCH_PAD8_RDE_M}, //8
	{RTC_IO_TOUCH_PAD9_REG,  RTC_IO_TOUCH_PAD9_MUX_SEL_M,  RTC_IO_TOUCH_PAD9_FUN_SEL_S,  RTC_IO_TOUCH_PAD9_FUN_IE_M,  RTC_IO_TOUCH_PAD9_RUE_M,  RTC_IO_TOUCH_PAD9_RDE_M}, //9
	{RTC_IO_TOUCH_PAD10_REG, RTC_IO_TOUCH_PAD10_MUX_SEL_M, RTC_IO_TOUCH_PAD10_FUN_SEL_S, RTC_IO_TOUCH_PAD10_FUN_IE_M, RTC_IO_TOUCH_PAD10_RUE_M, RTC_IO_TOUCH_PAD10_RDE_M}, //10
	{RTC_IO_TOUCH_PAD11_REG, RTC_IO_TOUCH_PAD11_MUX_SEL_M, RTC_IO_TOUCH_PAD11_FUN_SEL_S, RTC_IO_TOUCH_PAD11_FUN_IE_M, RTC_IO_TOUCH_PAD11_RUE_M, RTC_IO_TOUCH_PAD11_RDE_M}, //11
	{RTC_IO_TOUCH_PAD12_REG, RTC_IO_TOUCH_PAD12_MUX_SEL_M, RTC_IO_TOUCH_PAD12_FUN_SEL_S, RTC_IO_TOUCH_PAD12_FUN_IE_M, RTC_IO_TOUCH_PAD12_RUE_M, RTC_IO_TOUCH_PAD12_RDE_M}, //12
	{RTC_IO_TOUCH_PAD13_REG, RTC_IO_TOUCH_PAD13_MUX_SEL_M, RTC_IO_TOUCH_PAD13_FUN_SEL_S, RTC_IO_TOUCH_PAD13_FUN_IE_M, RTC_IO_TOUCH_PAD13_RUE_M, RTC_IO_TOUCH_PAD13_RDE_M}, //13
	{RTC_IO_TOUCH_PAD14_REG, RTC_IO_TOUCH_PAD14_MUX_SEL_M, RTC_IO_TOUCH_PAD14_FUN_SEL_S, RTC_IO_TOUCH_PAD14_FUN_IE_M, RTC_IO_TOUCH_PAD14_RUE_M, RTC_IO_TOUCH_PAD14_RDE_M}, //14
	{RTC_IO_XTAL_32P_PAD_REG, RTC_IO_X32P_MUX_SEL_M,        RTC_IO_X32P_FUN_SEL_S,        RTC_IO_X32P_FUN_IE_M,        RTC_IO_X32P_RUE_M,        RTC_IO_X32P_RDE_M}, //15
	{RTC_IO_XTAL_32N_PAD_REG, RTC_IO_X32N_MUX_SEL_M,        RTC_IO_X32N_FUN_SEL_S,        RTC_IO_X32N_FUN_IE_M,        RTC_IO_X32N_RUE_M,        RTC_IO_X32N_RDE_M}, //16
	{RTC_IO_PAD_DAC1_REG,    RTC_IO_PDAC1_MUX_SEL_M,       RTC_IO_PDAC1_FUN_SEL_S,       RTC_IO_PDAC1_FUN_IE_M,       RTC_IO_PDAC1_RUE_M,       RTC_IO_PDAC1_RDE_M}, //17
	{RTC_IO_PAD_DAC2_REG,    RTC_IO_PDAC2_MUX_SEL_M,       RTC_IO_PDAC2_FUN_SEL_S,       RTC_IO_PDAC2_FUN_IE_M,       RTC_IO_PDAC2_RUE_M,       RTC_IO_PDAC2_RDE_M}, //18
	{RTC_IO_RTC_PAD19_REG,   RTC_IO_PAD19_MUX_SEL_M,       RTC_IO_PAD19_FUN_SEL_S,       RTC_IO_PAD19_FUN_IE_M,       RTC_IO_PAD19_RUE_M,       RTC_IO_PAD19_RDE_M}, //19
	{RTC_IO_RTC_PAD20_REG,   RTC_IO_PAD20_MUX_SEL_M,       RTC_IO_PAD20_FUN_SEL_S,       RTC_IO_PAD20_FUN_IE_M,       RTC_IO_PAD20_RUE_M,       RTC_IO_PAD20_RDE_M}, //20
	{RTC_IO_RTC_PAD21_REG,   RTC_IO_PAD21_MUX_SEL_M,       RTC_IO_PAD21_FUN_SEL_S,       RTC_IO_PAD21_FUN_IE_M,       RTC_IO_PAD21_RUE_M,       RTC_IO_PAD21_RDE_M}, //21
};

int px4_arch_adc_init(uint32_t base_address)
{
	/* Perform ADC init once per ADC */

	static uint32_t once[SYSTEM_ADC_COUNT] {};

	uint32_t *free = nullptr;

	uint32_t init_code = 0;
	int version = esp_efuse_rtc_calib_get_ver();

	if ((version >= ESP_EFUSE_ADC_CALIB_VER_MIN) &&
	    (version <= ESP_EFUSE_ADC_CALIB_VER_MAX)) {
		// Guarantee the calibration version before calling efuse function
		init_code = esp_efuse_rtc_calib_get_init_code(version, ADC_UINT, DEFAULT_ATTEN);

	} else {
		return ERROR;
	}

	for (uint32_t i = 0; i < SYSTEM_ADC_COUNT; i++) {
		if (once[i] == base_address) {

			/* This one was done already */

			return OK;
		}

		/* Use first free slot */

		if (free == nullptr && once[i] == 0) {
			free = &once[i];
		}
	}

	if (free == nullptr) {

		/* ADC misconfigured SYSTEM_ADC_COUNT too small */;

		PANIC();
	}

	*free = base_address;

	modifyreg32(SYSTEM_PERIP_CLK_EN0_REG, 0, SYSTEM_APB_SARADC_CLK_EN);
	modifyreg32(SYSTEM_PERIP_RST_EN0_REG, SYSTEM_APB_SARADC_RST, 0);

	modifyreg32(SENS_SAR_READER_CTRL_REG(base_address), SENS_SAR_DATA_INV, 1 << SENS_SAR1_CLK_DIV_S);

	modifyreg32(SENS_SAR_PERI_CLK_GATE_CONF_REG, 0, SENS_SARADC_CLK_EN | SENS_IOMUX_CLK_EN | SENS_TSENS_CLK_EN);

	modifyreg32(SENS_SAR_MEAS_CTRL2_REG(base_address), 0, SENS_MEAS1_START_FORCE | SENS_SAR1_EN_PAD_FORCE);

#ifdef ESP32S3_USE_ADC2
	modifyreg32(APB_SARADC_APB_ADC_ARB_CTRL_REG, APB_SARADC_ADC_ARB_GRANT_FORCE,
		    APB_SARADC_ADC_ARB_FIX_PRIORITY | (1 << APB_SARADC_ADC_ARB_RTC_PRIORITY_S) | (2 << APB_SARADC_ADC_ARB_WIFI_PRIORITY_S));

	REGI2C_WRITE_MASK(I2C_ADC, I2C_ADC2_DEF, 4);
	REGI2C_WRITE_MASK(I2C_ADC, I2C_ADC2_INITVAL_H, init_code >> 8);
	REGI2C_WRITE_MASK(I2C_ADC, I2C_ADC2_INITVAL_L, init_code & 0xff);
#else
	REGI2C_WRITE_MASK(I2C_ADC, I2C_ADC1_DEF, 4);
	REGI2C_WRITE_MASK(I2C_ADC, I2C_ADC1_INITVAL_H, init_code >> 8);
	REGI2C_WRITE_MASK(I2C_ADC, I2C_ADC1_INITVAL_L, init_code & 0xff);
#endif

	// enable temperature sensor
	modifyreg32(SENS_SAR_TSENS_CTRL_REG, SENS_TSENS_DUMP_OUT,
		    SENS_TSENS_POWER_UP | SENS_TSENS_POWER_UP_FORCE);

	// power up
	putreg32(0x3, SENS_SAR_POWER_XPD_SAR_REG);

	return OK;
}

void px4_arch_adc_uninit(uint32_t base_address)
{
	// disable temperature sensor
	modifyreg32(SENS_SAR_TSENS_CTRL_REG, SENS_TSENS_POWER_UP, 0);
	putreg32(0x0, SENS_SAR_POWER_XPD_SAR_REG);

	modifyreg32(SYSTEM_PERIP_CLK_EN0_REG, SYSTEM_APB_SARADC_CLK_EN, 0);
	modifyreg32(SYSTEM_PERIP_RST_EN0_REG, 0, SYSTEM_APB_SARADC_RST);
}

uint32_t px4_arch_adc_sample(uint32_t base_address, unsigned channel)
{
	irqstate_t flags = px4_enter_critical_section();

	// temperature sensor
	if (channel == PX4_ADC_INTERNAL_TEMP_SENSOR_CHANNEL) {
		const hrt_abstime now = hrt_absolute_time();

		modifyreg32(SENS_SAR_TSENS_CTRL_REG, 0, SENS_TSENS_DUMP_OUT);

		while (!(getreg32(SENS_SAR_TSENS_CTRL_REG) & SENS_TSENS_READY)) {

			/* don't wait for more than 50us, since that means something broke - should reset here if we see this */
			if ((hrt_absolute_time() - now) > 50) {
				px4_leave_critical_section(flags);
				return UINT32_MAX;
			}
		}

		modifyreg32(SENS_SAR_TSENS_CTRL_REG, SENS_TSENS_DUMP_OUT, 0);

		uint32_t result = (getreg32(SENS_SAR_TSENS_CTRL_REG) & SENS_TSENS_OUT_M) >> SENS_TSENS_OUT_S;

		px4_leave_critical_section(flags);

		return result;
	}

	if (channel >= ESP32S3_ADC_MAX_CHANNELS) {
		px4_leave_critical_section(flags);
		return UINT32_MAX;
	}


	if ((enabled_channels & (1 << channel)) == 0) {
		enabled_channels |= (1 << channel);
		putreg32(rtc_io_desc[RTCIO_NUM(channel)].mux, rtc_io_desc[RTCIO_NUM(channel)].reg);
		modifyreg32(rtc_io_desc[RTCIO_NUM(channel)].reg, RTC_IO_TOUCH_PAD1_FUN_SEL_M,
			    0 << rtc_io_desc[RTCIO_NUM(channel)].func);
		modifyreg32(RTC_GPIO_ENABLE_W1TC_REG, 0, (1 << RTCIO_NUM(channel)) << RTC_GPIO_ENABLE_W1TC_S);
		modifyreg32(rtc_io_desc[RTCIO_NUM(channel)].reg,
			    rtc_io_desc[RTCIO_NUM(channel)].ie | rtc_io_desc[RTCIO_NUM(channel)].pulldown | rtc_io_desc[RTCIO_NUM(channel)].pullup, 0);
	}

	// set channel
	modifyreg32(SENS_SAR_MEAS_CTRL2_REG(base_address), SENS_SAR1_EN_PAD_M, ((1 << channel) << SENS_SAR1_EN_PAD_S));

	// sample once
	modifyreg32(SENS_SAR_MEAS_CTRL2_REG(base_address), SENS_MEAS1_START_SAR, 0);
	modifyreg32(SENS_SAR_MEAS_CTRL2_REG(base_address), 0, SENS_MEAS1_START_SAR);

	/* wait for the conversion to complete */
	const hrt_abstime now = hrt_absolute_time();

	while (!(getreg32(SENS_SAR_MEAS_CTRL2_REG(base_address)) & SENS_MEAS1_DONE_SAR)) {

		/* don't wait for more than 50us, since that means something broke - should reset here if we see this */
		if ((hrt_absolute_time() - now) > 50) {
			px4_leave_critical_section(flags);
			return UINT32_MAX;
		}
	}

	/* read the result and clear EOC */
	uint32_t result = (getreg32(SENS_SAR_MEAS_CTRL2_REG(base_address)) & SENS_MEAS1_DATA_SAR_M) >> SENS_MEAS1_DATA_SAR_S;

	px4_leave_critical_section(flags);

	return result;
}

float px4_arch_adc_reference_v()
{
	return BOARD_ADC_POS_REF_V;	// TODO: provide true vref
}

uint32_t px4_arch_adc_temp_sensor_mask()
{

	return 1 << PX4_ADC_INTERNAL_TEMP_SENSOR_CHANNEL;

}

uint32_t px4_arch_adc_dn_fullcount()
{
	return 1 << 12; // 12 bit ADC
}
