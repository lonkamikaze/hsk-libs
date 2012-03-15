/** \file
 * HSK Timer 0/1 implementation
 *
 * This simple library implements access to the timers T0 and T1, as 16 bit
 * timers to use as a ticking source.
 *
 * @author kami
 */

#include <Infineon/XC878.h>

#include "hsk_timer01.h"

/**
 * IEN0 Timer 0 Overflow Interrupt Enable bit.
 */
#define BIT_ET0		1

/**
 * IEN0 Timer 1 Overflow Interrupt Enable bit.
 */
#define BIT_ET1		3

/**
 * TMOD Timer 0 Mode select bits.
 */
#define BIT_T0M		0

/**
 * T0M bit count.
 */
#define CNT_T0M		2

/**
 * TMOD Timer 0 Mode select bits.
 */
#define BIT_T1M		4

/**
 * T1M bit count.
 */
#define CNT_T1M		2

/**
 * Struct representing runtime information for a timer.
 */
struct {
	/**
	 * The value to load into the timer upon overflow.
	 */
	uword overflow;

	/**
	 * A callback function pointer used by the ISR.
	 */
	void (code *callback)(void);
} xdata hsk_timers[2];

/**
 * The ISR for timer 0.
 *
 * Sets up the timer 0 count registers and calls the callback function.
 *
 * @private
 */
void ISR_hsk_timer0(void) interrupt 1 {
	uword ticks = hsk_timers[0].overflow + ((TH0 << 8) | TL0);
	TL0 = ticks;
	TH0 = ticks >> 8;
	hsk_timers[0].callback();
}

/**
 * The ISR for timer 1.
 *
 * Sets up the timer 1 count registers and calls the callback function.
 *
 * @private
 */
void ISR_hsk_timer1(void) interrupt 3 {
	uword ticks = hsk_timers[1].overflow + ((TH1 << 8) | TL1);
	TL1 = ticks;
	TH1 = ticks >> 8;
	hsk_timers[1].callback();
}

/**
 * Setup timer 0 or 1 to tick at a given interval.
 *
 * The callback function will be called by the interrupt once the
 * interrupt has been enabled. Note that the callback function is
 * entered with the current page unknown.
 *
 * This works on the assumption, that PCLK is set to 24MHz.
 *
 * @param id
 *	Timer 0 or 1.
 * @param interval
 *	The ticking interval in µs, don't go beyond 5461.
 * @param callback
 *	A function pointer to a callback function.
 * @private
 */
void hsk_timer01_setup(ubyte idata id, uword idata interval, void (code * idata callback)(void)) {
	/*
	 * The timer ticks with PCLK / 2, we want 1µs precission, which is
	 * up to 1000000 ticks per second.
	 *
	 * PCLK / 2 / precission = 12
	 */
	ulong ticks = 12 * interval;

	/* Set 16 bit mode. */
	switch (id) {
	case 0:
		TMOD = TMOD & ~(((1 << CNT_T0M) - 1) << BIT_T0M) | 1 << BIT_T0M;
		break;
	case 1:
		TMOD = TMOD & ~(((1 << CNT_T1M) - 1) << BIT_T1M) | 1 << BIT_T1M;
		break;
	}
	/* Set up timer data for the ISRs. */
	hsk_timers[id].overflow = (-1) - ticks;
	hsk_timers[id].callback = callback;
}

void hsk_timer0_setup(uword idata interval, void (code * idata callback)(void)) {
	hsk_timer01_setup(0, interval, callback);
}

void hsk_timer0_enable(void) {
	ET0 = 1;
	TR0 = 1;
}

void hsk_timer0_disable(void) {
	TR0 = 0;
	ET0 = 0;
}

void hsk_timer1_setup(uword idata interval, void (code * idata callback)(void)) {
	hsk_timer01_setup(1, interval, callback);
}

void hsk_timer1_enable(void) {
	ET1 = 1;
	TR1 = 1;
}

void hsk_timer1_disable(void) {
	TR1 = 0;
	ET1 = 0;
}

