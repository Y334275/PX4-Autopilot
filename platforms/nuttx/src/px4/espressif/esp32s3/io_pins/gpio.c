#include <px4_platform_common/px4_config.h>
#include <systemlib/px4_macros.h>

#include <arch/board/board.h>

#include <px4_arch/micro_hal.h>
#include <errno.h>
#include <stdio.h>
#include <syslog.h>
#include <nuttx/irq.h>
#include <arch/chip/irq.h>

int px4_arch_configgpio(uint32_t pinset)
{
	return esp32s3_configgpio(esp32s3_gpio(pinset), esp32s3_attr(pinset));
}

int px4_arch_unconfiggpio(uint32_t pinset)
{
	return px4_arch_configgpio(esp32s3_gpio(pinset) | GPIO_INPUT | GPIO_OPEN_DRAIN);
}

/****************************************************************************
 * Name: esp32s3_gpiosetevent
 *
 * Description:
 *   Sets/clears GPIO based event and interrupt triggers.
 *
 * Input Parameters:
 *  - pinset:      GPIO pin configuration
 *  - risingedge:  Enables interrupt on rising edges
 *  - fallingedge: Enables interrupt on falling edges
 *  - event:       Generate event when set
 *  - func:        When non-NULL, generate interrupt
 *  - arg:         Argument passed to the interrupt callback
 *
 * Returned Value:
 *   Zero (OK) on success; a negated errno value on failure indicating the
 *   nature of the failure.
 *
 ****************************************************************************/
int px4_arch_gpiosetevent(uint32_t pinset, bool risingedge, bool fallingedge,
			  bool event, xcpt_t func, void *arg)
{
	uint32_t pin = esp32s3_gpio(pinset);
	int irq = ESP32S3_PIN2IRQ(pin);

	if (event == true) {
		int ret = irq_attach(irq, func, arg);

		if (ret < 0) {
			syslog(LOG_ERR, "ERROR: irq_attach() failed: %d\n", ret);
			return ret;
		}

		if (risingedge == true && fallingedge == true) {
			esp32s3_gpioirqenable(irq, GPIO_INTR_ANYEDGE);

		} else if (risingedge == true && fallingedge == false) {
			esp32s3_gpioirqenable(irq, GPIO_INTR_POSEDGE);

		} else if (risingedge == false && fallingedge == true) {
			esp32s3_gpioirqenable(irq, GPIO_INTR_NEGEDGE);
		}

	} else {
		esp32s3_gpioirqdisable(irq);
	}

	//syslog(LOG_INFO, "esp32_gpiosetevent: %d\n", ret);
	return OK;
}
