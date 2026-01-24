/****************************************************************************
 *
 *   Copyright (c) 2012, 2013 PX4 Development Team. All rights reserved.
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
 * @file drv_hrt.c
 *
 * High-resolution timer callouts and timekeeping.
 *
 * This can use any general or advanced STM32 timer.
 *
 * Note that really, this could use systick too, but that's
 * monopolised by NuttX and stealing it would just be awkward.
 *
 * We don't use the NuttX STM32 driver per se; rather, we
 * claim the timer and then drive it directly.
 */

#include <px4_platform_common/px4_config.h>

#include <nuttx/arch.h>
#include <nuttx/irq.h>
#include <nuttx/spinlock.h>

#include <sys/types.h>
#include <stdbool.h>

#include <assert.h>
#include <debug.h>
#include <time.h>
#include <sys/queue.h>
#include <errno.h>
#include <string.h>

#include <xtensa.h>
#include <xtensa_attr.h>

#include "esp32s3_irq.h"
#include <esp32s3_clockconfig.h>
#include <esp32s3_gpio.h>
#include <hardware/esp32s3_system.h>
#include <hardware/esp32s3_tim.h>

#include <board_config.h>
#include <drivers/drv_hrt.h>

#ifdef CONFIG_DEBUG_HRT
#  define hrtinfo _info
#  define hrtwarn _warn
#  define hrterr _err
#else
#  define hrtinfo(x...)
#  define hrtwarn(x...)
#  define hrterr(x...)
// #  define hrtinfo _info
#endif

#ifdef	HRT_TIMER

/* HRT configuration */
#if HRT_TIMER == 0
# define HRT_TIM_GROUP			0
# define HRT_TIM_CHANNEL		0
# if CONFIG_ESP32S3_WIFI
#  error must not set CONFIG_ESP32S3_WIFI=y and HRT_TIMER=0. WIFI makes use of TIMER=0
# endif
#elif HRT_TIMER == 1
# define HRT_TIM_GROUP			0
# define HRT_TIM_CHANNEL		1
# if CONFIG_ESP32S3_TIMER1
#  error must not set CONFIG_ESP32S3_TIMER1=y and HRT_TIMER=1
# endif
#elif HRT_TIMER == 2
# define HRT_TIM_GROUP			1
# define HRT_TIM_CHANNEL		0
# if CONFIG_ESP32S3_TIMER2
#  error must not set CONFIG_ESP32S3_TIMER2=y and HRT_TIMER=2
# endif
#elif HRT_TIMER == 3
# define HRT_TIM_GROUP			1
# define HRT_TIM_CHANNEL		1
# if CONFIG_ESP32S3_TIMER3
#  error must not set CONFIG_ESP32S3_TIMER3=y and HRT_TIMER=3
# endif
#else
# error HRT_TIMER must be a value between 0 and 3
#endif

#if HRT_TIM_CHANNEL == 0
#define HRT_TIM_CONFIG_REG		TIMG_T0CONFIG_REG(HRT_TIM_GROUP)
#define HRT_TIM_LO_REG			TIMG_T0LO_REG(HRT_TIM_GROUP)
#define HRT_TIM_HI_REG			TIMG_T0HI_REG(HRT_TIM_GROUP)
#define HRT_TIM_UPDATE_REG		TIMG_T0UPDATE_REG(HRT_TIM_GROUP)

#define HRT_TIM_ALARMLO_REG		TIMG_T0ALARMLO_REG(HRT_TIM_GROUP)
#define HRT_TIM_ALARMHI_REG		TIMG_T0ALARMHI_REG(HRT_TIM_GROUP)

#define HRT_TIM_LOADLO_REG		TIMG_T0LOADLO_REG(HRT_TIM_GROUP)
#define HRT_TIM_LOADHI_REG		TIMG_T0LOADHI_REG(HRT_TIM_GROUP)
#define HRT_TIM_LOAD_REG		TIMG_T0LOAD_REG(HRT_TIM_GROUP)
#define HRT_TIM_INT_ENA			TIMG_T0_INT_ENA
#define HRT_TIM_INT_CLR			TIMG_T0_INT_CLR

#if HRT_TIM_GROUP == 0
# define HRT_TIM_PERIPH			ESP32S3_PERIPH_TG_T0_LEVEL
# define HRT_TIM_IRQ	        	ESP32S3_IRQ_TG_T0_LEVEL
# define HRT_TIMG_CLK_EN		SYSTEM_TIMERGROUP_CLK_EN
# define HRT_TIMG_RST			SYSTEM_TIMERGROUP_RST
#else // HRT_TIM_GROUP == 1
# define HRT_TIM_PERIPH			ESP32S3_PERIPH_TG1_T0_LEVEL
# define HRT_TIM_IRQ	        	ESP32S3_IRQ_TG1_T0_LEVEL
# define HRT_TIMG_CLK_EN		SYSTEM_TIMERGROUP1_CLK_EN
# define HRT_TIMG_RST			SYSTEM_TIMERGROUP1_RST
#endif // !HRT_TIM_GROUP

#else // HRT_TIM_CHANNEL == 1

#define HRT_TIM_CONFIG_REG		TIMG_T1CONFIG_REG(HRT_TIM_GROUP)
#define HRT_TIM_LO_REG			TIMG_T1LO_REG(HRT_TIM_GROUP)
#define HRT_TIM_HI_REG			TIMG_T1HI_REG(HRT_TIM_GROUP)
#define HRT_TIM_UPDATE_REG		TIMG_T1UPDATE_REG(HRT_TIM_GROUP)

#define HRT_TIM_ALARMLO_REG		TIMG_T1ALARMLO_REG(HRT_TIM_GROUP)
#define HRT_TIM_ALARMHI_REG		TIMG_T1ALARMHI_REG(HRT_TIM_GROUP)

#define HRT_TIM_LOADLO_REG		TIMG_T1LOADLO_REG(HRT_TIM_GROUP)
#define HRT_TIM_LOADHI_REG		TIMG_T1LOADHI_REG(HRT_TIM_GROUP)
#define HRT_TIM_LOAD_REG		TIMG_T1LOAD_REG(HRT_TIM_GROUP)

#if HRT_TIM_GROUP == 0
# define HRT_TIM_PERIPH			ESP32S3_PERIPH_TG_T1_LEVEL
# define HRT_TIM_IRQ	        	ESP32S3_IRQ_TG_T1_LEVEL
# define HRT_TIMG_CLK_EN		SYSTEM_TIMERGROUP_CLK_EN
# define HRT_TIMG_RST			SYSTEM_TIMERGROUP_RST
#else // HRT_TIM_GROUP == 1
# define HRT_TIM_PERIPH			ESP32S3_PERIPH_TG1_T1_LEVEL
# define HRT_TIM_IRQ	        	ESP32S3_IRQ_TG1_T1_LEVEL
# define HRT_TIMG_CLK_EN		SYSTEM_TIMERGROUP1_CLK_EN
# define HRT_TIMG_RST			SYSTEM_TIMERGROUP1_RST
#endif // !HRT_TIM_GROUP

#define HRT_TIM_INT_ENA			TIMG_T1_INT_ENA
#define HRT_TIM_INT_CLR			TIMG_T1_INT_CLR

#endif // !HRT_TIM_CHANNEL

#define HRT_TIM_INT_CLR_TIMERS_REG	TIMG_INT_CLR_TIMERS_REG(HRT_TIM_GROUP)

/**
 * Minimum/maximum deadlines.
 *
 * These are suitable for use with a 16-bit timer/counter clocked
 * at 1MHz.  The high-resolution timer need only guarantee that it
 * not wrap more than once in the 50ms period for absolute time to
 * be consistently maintained.
 *
 * The minimum deadline must be such that the time taken between
 * reading a time and writing a deadline to the timer cannot
 * result in missing the deadline.
 */
#define HRT_INTERVAL_MIN	50
#define HRT_INTERVAL_MAX	50000

static spinlock_t 	_hrt_lock;
/*
 * Queue of callout entries.
 */
static struct sq_queue_s	callout_queue;

/* latency baseline (last compare value applied) */
static uint64_t			latency_baseline;

/* timer count at interrupt (for latency purposes) */
static uint64_t			latency_actual;

/* latency histogram */
const uint16_t latency_bucket_count = LATENCY_BUCKET_COUNT;
const uint16_t latency_buckets[LATENCY_BUCKET_COUNT] = { 1, 2, 5, 10, 20, 50, 100, 1000 };
__EXPORT uint32_t latency_counters[LATENCY_BUCKET_COUNT + 1];

/* timer-specific functions */
static void		hrt_tim_init(void);
static int		hrt_tim_isr(int irq, void *context, void *arg);
static void		hrt_latency_update(void);

/* callout list manipulation */
static void		hrt_call_internal(struct hrt_call *entry,
		hrt_abstime deadline,
		hrt_abstime interval,
		hrt_callout callout,
		void *arg);
static void		hrt_call_enter(struct hrt_call *entry);
static void		hrt_call_reschedule(void);
static void		hrt_call_invoke(void);


int hrt_ioctl(unsigned int cmd, unsigned long arg);

static inline irqstate_t hrt_lock(void)
{
	return spin_lock_irqsave(&_hrt_lock);
}

static inline void hrt_unlock(irqstate_t flags)
{
	spin_unlock_irqrestore(&_hrt_lock, flags);
}

/**
 * Initialise the timer we are going to use.
 *
 * We expect that we'll own one of the reduced-function STM32 general
 * timers, and that we can use channel 1 in compare mode.
 */

static void
hrt_tim_init(void)
{
	int ret, cpuint;
	int cpu = this_cpu();
	modifyreg32(SYSTEM_PERIP_CLK_EN0_REG, 0, HRT_TIMG_CLK_EN);
	modifyreg32(SYSTEM_PERIP_RST_EN0_REG, HRT_TIMG_RST, 0);

	// config timer to 1MHz
	modifyreg32(HRT_TIM_CONFIG_REG, TIMG_T0_USE_XTAL, 0);
	uint32_t divider = (esp_clk_apb_freq() / MHZ) - 1;
	modifyreg32(HRT_TIM_CONFIG_REG, TIMG_T0_DIVIDER_M, ((divider << TIMG_T0_DIVIDER_S) & TIMG_T0_DIVIDER_M));
	modifyreg32(HRT_TIM_CONFIG_REG, 0, TIMG_T0_INCREASE);

	// set counter value
	putreg32(0, HRT_TIM_LOADLO_REG);
	putreg32(0, HRT_TIM_LOADHI_REG);
	putreg32(1, HRT_TIM_LOAD_REG); //reload

	uint64_t val = 1000;
	uint64_t low_64 = val & 0xffffffff;
	uint64_t high_64 = (val >> 32) & 0xffffffff;
	putreg32((uint32_t)low_64, HRT_TIM_ALARMLO_REG);
	putreg32((uint32_t)high_64, HRT_TIM_ALARMHI_REG);

	// set interrupt
	cpuint = esp32s3_setup_irq(cpu, HRT_TIM_PERIPH, 1, ESP32S3_CPUINT_LEVEL);

	if (cpuint < 0) {
		hrterr("ERROR: No CPU Interrupt available");
		return;
	}

	ret = irq_attach(HRT_TIM_IRQ, hrt_tim_isr, NULL);

	if (ret != OK) {
		esp32s3_teardown_irq(cpu, HRT_TIM_PERIPH, cpuint);
		hrterr("ERROR: Failed to associate an IRQ Number");
		return;
	}

	up_enable_irq(HRT_TIM_IRQ);

	modifyreg32(TIMG_INT_ENA_TIMERS_REG(HRT_TIM_GROUP), 0, HRT_TIM_INT_ENA);

	// disable auto reload
	modifyreg32(HRT_TIM_CONFIG_REG, TIMG_T0_AUTORELOAD, 0);

	// enable alarm
	modifyreg32(HRT_TIM_CONFIG_REG, 0, TIMG_T0_ALARM_EN);

	// start timer
	modifyreg32(HRT_TIM_CONFIG_REG, 0, TIMG_T0_EN);
	hrtinfo("conf reg: %" PRIu32 ", alarm lo: %" PRIu32 ", alarm hi: %" PRIu32 ", divider: %" PRIu32 "\n", getreg32(HRT_TIM_CONFIG_REG),
		getreg32(HRT_TIM_ALARMLO_REG), getreg32(HRT_TIM_ALARMHI_REG), divider);
}

/**
 * Handle the compare interrupt by calling the callout dispatcher
 * and then re-scheduling the next deadline.
 */
static int IRAM_ATTR
hrt_tim_isr(int irq, void *context, void *arg)
{
	/* grab the timer for latency tracking purposes */
	uint32_t value_32;
	irqstate_t flags;

	latency_actual = 0;
	/* Dummy value to latch the counter value to read it */
	putreg32(TIMG_T0_UPDATE, HRT_TIM_UPDATE_REG);
	hrtinfo("hrt interrupt");

	// wait until UPDATE_REG become 0
	while (getreg32(HRT_TIM_UPDATE_REG) != 0);

	/* Read value */
	value_32 = getreg32(HRT_TIM_HI_REG); /* High 32 bits */
	latency_actual |= (uint64_t)value_32;
	latency_actual <<= 32;
	value_32 = getreg32(HRT_TIM_LO_REG); /* Low 32 bits */
	latency_actual |= (uint64_t)value_32;

	/* do latency calculations */
	hrt_latency_update();

	/* run any callouts that have met their deadline */
	hrt_call_invoke();

	flags = hrt_lock();
	/* and schedule the next interrupt */
	hrt_call_reschedule();
	hrt_unlock(flags);

	// acknowledge the interrupt
	putreg32(HRT_TIM_INT_CLR, HRT_TIM_INT_CLR_TIMERS_REG);
	modifyreg32(HRT_TIM_CONFIG_REG, 0, TIMG_T0_ALARM_EN);

	return OK;
}

/**
 * Fetch a never-wrapping absolute time value in microseconds from
 * some arbitrary epoch shortly after system start.
 */
hrt_abstime IRAM_ATTR
hrt_absolute_time(void)
{
	hrt_abstime	abstime;

	/*
	 * Counter state.  Marked volatile as they may change
	 * inside this routine but outside the irqsave/restore
	 * pair.  Discourage the compiler from moving loads/stores
	 * to these outside of the protected range.
	 */
	// static volatile uint64_t last_count;

	/* prevent re-entry */
	putreg32(TIMG_T0_UPDATE, HRT_TIM_UPDATE_REG);

	// wait until UPDATE_REG become 0
	while (getreg32(HRT_TIM_UPDATE_REG) != 0);

	uint64_t high_32 = ((uint64_t)getreg32(HRT_TIM_HI_REG)) << 32;
	uint64_t low_32 = (uint64_t)getreg32(HRT_TIM_LO_REG);

	abstime = (hrt_abstime)(high_32 | low_32);

	return abstime;
}

/**
 * Store the absolute time in an interrupt-safe fashion
 */
void
hrt_store_absolute_time(volatile hrt_abstime *t)
{
	irqstate_t flags = hrt_lock();
	*t = hrt_absolute_time();
	hrt_unlock(flags);
}

/**
 * Initialise the high-resolution timing module.
 */
void
hrt_init(void)
{
	sq_init(&callout_queue);

	spin_lock_init(&_hrt_lock);

	hrt_tim_init();

}

/**
 * Call callout(arg) after interval has elapsed.
 */
void IRAM_ATTR
hrt_call_after(struct hrt_call *entry, hrt_abstime delay, hrt_callout callout, void *arg)
{
	hrt_call_internal(entry,
			  hrt_absolute_time() + delay,
			  0,
			  callout,
			  arg);
}

/**
 * Call callout(arg) at calltime.
 */
void IRAM_ATTR
hrt_call_at(struct hrt_call *entry, hrt_abstime calltime, hrt_callout callout, void *arg)
{
	hrt_call_internal(entry, calltime, 0, callout, arg);
}

/**
 * Call callout(arg) every period.
 */
void IRAM_ATTR
hrt_call_every(struct hrt_call *entry, hrt_abstime delay, hrt_abstime interval, hrt_callout callout, void *arg)
{
	hrt_call_internal(entry,
			  hrt_absolute_time() + delay,
			  interval,
			  callout,
			  arg);
}

static void IRAM_ATTR
hrt_call_internal(struct hrt_call *entry, hrt_abstime deadline, hrt_abstime interval, hrt_callout callout, void *arg)
{
	irqstate_t flags = hrt_lock();

	/* if the entry is currently queued, remove it */
	/* note that we are using a potentially uninitialised
	   entry->link here, but it is safe as sq_rem() doesn't
	   dereference the passed node unless it is found in the
	   list. So we potentially waste a bit of time searching the
	   queue for the uninitialised entry->link but we don't do
	   anything actually unsafe.
	*/
	if (entry->deadline != 0) {
		sq_rem(&entry->link, &callout_queue);
	}

	entry->deadline = deadline;
	entry->period = interval;
	entry->callout = callout;
	entry->arg = arg;

	hrt_call_enter(entry);

	hrt_unlock(flags);
}

/**
 * If this returns true, the call has been invoked and removed from the callout list.
 *
 * Always returns false for repeating callouts.
 */
bool IRAM_ATTR
hrt_called(struct hrt_call *entry)
{
	return (entry->deadline == 0);
}

/**
 * Remove the entry from the callout list.
 */
void IRAM_ATTR
hrt_cancel(struct hrt_call *entry)
{
	irqstate_t flags = hrt_lock();

	sq_rem(&entry->link, &callout_queue);
	entry->deadline = 0;

	/* if this is a periodic call being removed by the callout, prevent it from
	 * being re-entered when the callout returns.
	 */
	entry->period = 0;

	hrt_unlock(flags);
}

static void IRAM_ATTR
hrt_call_enter(struct hrt_call *entry)
{
	struct hrt_call	*call, *next;

	call = (struct hrt_call *)sq_peek(&callout_queue);

	if ((call == NULL) || (entry->deadline < call->deadline)) {
		sq_addfirst(&entry->link, &callout_queue);
		hrtinfo("call enter at head, reschedule\n");
		/* we changed the next deadline, reschedule the timer event */
		hrt_call_reschedule();

	} else {
		do {
			next = (struct hrt_call *)sq_next(&call->link);

			if ((next == NULL) || (entry->deadline < next->deadline)) {
				hrtinfo("call enter after head\n");
				sq_addafter(&call->link, &entry->link, &callout_queue);
				break;
			}
		} while ((call = next) != NULL);
	}

	hrtinfo("scheduled\n");
}

static void IRAM_ATTR
hrt_call_invoke(void)
{
	struct hrt_call	*call;
	hrt_abstime deadline;

	irqstate_t flags = hrt_lock();

	while (true) {
		/* get the current time */
		hrt_abstime now = hrt_absolute_time();

		call = (struct hrt_call *)sq_peek(&callout_queue);

		if (call == NULL) {
			break;
		}

		if (call->deadline > now) {
			break;
		}

		sq_rem(&call->link, &callout_queue);
		hrtinfo("call pop\n");

		/* save the intended deadline for periodic calls */
		deadline = call->deadline;

		/* zero the deadline, as the call has occurred */
		call->deadline = 0;

		/* invoke the callout (if there is one) */
		if (call->callout) {
			// Unlock so we don't deadlock in callback
			hrt_unlock(flags);

			hrtinfo("call %p: %p(%p)\n", call, call->callout, call->arg);
			call->callout(call->arg);

			flags = hrt_lock();
		}

		/* if the callout has a non-zero period, it has to be re-entered */
		if (call->period != 0) {
			// re-check call->deadline to allow for
			// callouts to re-schedule themselves
			// using hrt_call_delay()
			if (call->deadline <= now) {
				call->deadline = deadline + call->period;
			}

			hrt_call_enter(call);
		}
	}

	hrt_unlock(flags);
}

/**
 * Reschedule the next timer interrupt.
 *
 * This routine must be called with interrupts disabled.
 */
static void IRAM_ATTR
hrt_call_reschedule()
{
	hrt_abstime	now = hrt_absolute_time();
	struct hrt_call	*next = (struct hrt_call *)sq_peek(&callout_queue);
	hrt_abstime	deadline = now + HRT_INTERVAL_MAX;

	/*
	 * Determine what the next deadline will be.
	 *
	 * Note that we ensure that this will be within the counter
	 * period, so that when we truncate all but the low 16 bits
	 * the next time the compare matches it will be the deadline
	 * we want.
	 *
	 * It is important for accurate timekeeping that the compare
	 * interrupt fires sufficiently often that the base_time update in
	 * hrt_absolute_time runs at least once per timer period.
	 */
	if (next != NULL) {
		hrtinfo("entry in queue\n");

		if (next->deadline <= (now + HRT_INTERVAL_MIN)) {
			hrtinfo("pre-expired\n");
			/* set a minimal deadline so that we call ASAP */
			deadline = now + HRT_INTERVAL_MIN;

		} else if (next->deadline < deadline) {
			hrtinfo("due soon\n");
			deadline = next->deadline;
		}
	}

	hrtinfo("schedule for %u at %u\n", (unsigned)(deadline & 0xffffffff), (unsigned)(now & 0xffffffff));

	/* set the new compare value and remember it for latency tracking */
	latency_baseline = deadline & 0xffff;

	putreg32((uint32_t)(deadline & 0xffffffff), HRT_TIM_ALARMLO_REG);
	putreg32((uint32_t)((deadline >> 32) & 0xffffffff), HRT_TIM_ALARMHI_REG);
}

static void
hrt_latency_update(void)
{
	uint16_t latency = latency_actual - latency_baseline;
	unsigned	index;

	/* bounded buckets */
	for (index = 0; index < LATENCY_BUCKET_COUNT; index++) {
		if (latency <= latency_buckets[index]) {
			latency_counters[index]++;
			return;
		}
	}

	/* catch-all at the end */
	latency_counters[index]++;
}

void IRAM_ATTR
hrt_call_init(struct hrt_call *entry)
{
	memset(entry, 0, sizeof(*entry));
}

void IRAM_ATTR
hrt_call_delay(struct hrt_call *entry, hrt_abstime delay)
{
	irqstate_t flags = hrt_lock();
	entry->deadline = hrt_absolute_time() + delay;
	hrt_unlock(flags);
}

#endif /* HRT_TIMER */
