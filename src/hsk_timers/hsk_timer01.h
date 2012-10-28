/** \file
 * HSK Timer 0/1 headers
 *
 * Provides access to the timers 0 and 1. Each timer can be provided with
 * a callback function that will be called by the timers ISR.
 *
 * @author kami
 */

#ifndef _HSK_TIMER01_H_
#define _HSK_TIMER01_H_

/*
 * ISR prototypes for SDCC.
 */
#ifdef SDCC
#include "hsk_timer01.isr"
#endif /* SDCC */

/*
 * C51 does not include the used register bank in pointer types.
 */
#ifdef __C51__
	#define using(bank)
#endif

/*
 * SDCC does not like the \c code keyword for function pointers, C51 needs it
 * or it will use generic pointers.
 */
#ifdef SDCC
	#undef code
	#define code
#endif /* SDCC */

/**
 * Setup timer 0 to tick at a given interval.
 *
 * The callback function will be called by the interrupt once the
 * interrupt has been enabled. Note that the callback function is
 * entered with the current page unknown.
 *
 * This works on the assumption, that PCLK is set to 24MHz.
 *
 * @param interval
 *	The ticking interval in µs, don't go beyond 5461.
 * @param callback
 *	A function pointer to a callback function.
 */
void hsk_timer0_setup(const uword interval,
	const void (code * const __xdata callback)(void) using(1));

/**
 * Enables the timer 0 and its interrupt.
 */
void hsk_timer0_enable(void);

/**
 * Disables timer 0 and its interrupt.
 */
void hsk_timer0_disable(void);

/**
 * Setup timer 1 to tick at a given interval.
 *
 * The callback function will be called by the interrupt once the
 * interrupt has been enabled. Note that the callback function is
 * entered with the current page unknown.
 *
 * This works on the assumption, that PCLK is set to 24MHz.
 *
 * @param interval
 *	The ticking interval in µs, don't go beyond 5461.
 * @param callback
 *	A function pointer to a callback function.
 */
void hsk_timer1_setup(const uword interval,
	const void (code * const __xdata callback)(void) using(1));

/**
 * Enables the timer 1 and its interrupt.
 */
void hsk_timer1_enable(void);

/**
 * Disables timer 1 and its interrupt.
 */
void hsk_timer1_disable(void);

/*
 * Restore the usual meaning of \c code.
 */
#ifdef SDCC
	#undef code
	#define code	__code
#endif

/*
 * Restore the usual meaning of \c using(bank).
 */
#ifdef __C51__
	#undef using
#endif /* __C51__ */

#endif /* _HSK_TIMER01_H_ */
