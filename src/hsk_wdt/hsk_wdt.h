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
 * @warning
 *	The WDT should be set up at the end of the boot procedure. Setting
 *	the WDT up at the beginning of the boot process can trigger all kinds
 *	of erratic behaviour like reset races or a complete lockup.
 * @author
 *	kami
 *
 * \section hazards Hazards
 *
 * The WDT has proven a useful tool in hazardous EMI conditions. Severe EMI
 * may freeze the µC without causing a proper reboot. In most cases the WDT
 * can mitigate this issue by reactivating the system.
 *
 * However the WDT is trigger happy. A series of refresh time interval
 * measurements shows that the WDT resets the µC long before the end of
 * its interval shortly after boot. The best mitigation is to refresh
 * the WDT with the hsk_wdt_service() function unconditionally. Instead
 * of using fixed timings (e.g. for 20ms watchdog time a 5ms refresh interval
 * should have been quite safe).
 *
 * That however does not solve the problem with NMIs. Any non-maskable
 * interrupt may cause the WDT to reset the µC. This means it is incopatible
 * to the hsk_flash library, which requires precise timings with only a couple
 * of µs tolerance. To meet this requirement the library uses the Flash Timer
 * of the XC878, which triggers NMIs.
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
void hsk_wdt_init(const uword window);

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
