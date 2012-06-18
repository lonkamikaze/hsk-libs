/** \file
 * HSK Watchdog Timer headers
 *
 * Provides access to the Watchdog Timer (WDT) of the XC878.
 *
 * Depending on the configured window time the µC reset is delayed for
 * 1.024ms (window < 5460µs) or 65.536ms (window >= 5460µs).
 *
 * This time can be used by assigning a callback function to
 * \ref hsk_isr14 member \ref hsk_isr14_callback::NMIWDT and setting
 * the NMICON.NMIWDT bit.
 *
 * @note
 *	The WDT should be set up at the end of the boot procedure. Setting
 *	the WDT up at the beginning of the boot process can trigger all kinds
 *	of erratic behaviour like reset races or a complete lockup.
 * @author
 *	kami
 */

#ifndef _HSK_WDT_H_
#define _HSK_WDT_H_

/**
 * Sets up the watchdog timer.
 *
 * The window time specifies the time available to call hsk_wdt_service()
 * before a reset is triggered. Possible times range from 21.3µs to 350ms.
 *
 * The window time is rounded up to the next higher possible value.
 * Exceeding the value range causes an overflow that results in shorter
 * window times.
 *
 * @param window
 *	The time window in multiples of 10µs
 */
void hsk_wdt_init(const uword idata window);

/**
 * Activates the Watchdog Timer.
 */
void hsk_wdt_enable(void);

/**
 * Disables the Watchdog Timer.
 */
void hsk_wdt_disable(void);

/**
 * Resets the watchdog timer.
 *
 * This function needs to be called to prevent the WDT from resetting the
 * device.
 */
void hsk_wdt_service(void);

#endif /* _HSK_WDT_H_ */
