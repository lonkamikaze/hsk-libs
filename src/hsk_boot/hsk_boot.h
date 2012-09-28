/** \file
 * HSK Boot headers
 *
 * This file contains the prototypes to put the µC into working condition.
 *
 * Currently implemented:
 * 	- hsk_boot_mem()
 *	  Activate access to xdata memory, useful for large data
 *	  structures like the ADC buffers
 *	- hsk_boot_extClock()
 *	  Activates external clock input and sets up the PLL, this
 *	  is important when communicating with other devices, the
 *	  internal clock is not sufficiently precise
 *
 * @author kami
 */

#ifndef _HSK_BOOT_H_
#define _HSK_BOOT_H_

/*
 * Required for SDCC to propagate ISR prototypes.
 */
#ifdef SDCC
#include "../hsk_isr/hsk_isr.isr"
#endif /* SDCC */

/**
 * Sets up xdata memory access.
 *
 * Refer to the Processor Architecture and Memory Organization chapters of the
 * XC878 User Manual.
 */
void hsk_boot_mem(void);

/**
 * Switches to an external oscilator.
 *
 * This function requires xdata access.
 *
 * The implemented process is named:
 * 	"Select the External Oscillator as PLL input source"
 *
 * The following is described in more detail in chapter 7.3 of the XC878
 * User Manual.
 *
 * The XC878 can either use an internal 4MHz oscilator (default) or an
 * external oscilator from 2 to 20MHz, normally referred to as FOSC.
 * A phase-locked loop (PLL) converts it to a faster internal speed FSYS,
 * 144MHz by default.
 *
 * This implementation is currently limited to oscilators from 2MHz to 20MHz
 * in 1MHz intervals.
 *
 * The oscilator frequency is vital for external communication (e.g. CAN)
 * and timer/counter speeds.
 *
 * This implementation switches to an external clock ensuring that the
 * PLL generates a 144MHz FSYS clock. The CLKREL divisor set to 6 generates
 * the fast clock (FCLK) that runs at 48MHz. The remaining clocks, i.e.
 * peripheral (PCLK), CPU (SCLK, CCLK), have a fixed divisor by 2, so
 * they run at 24MHz.
 *
 * After setting up the PLL, this function will register an ISR, that will
 * attempt to reactivate the external oscillator in a PLL loss-of-clock
 * event.
 *
 * @param clk
 *	The frequency of the external oscilator in Hz.
 */
void hsk_boot_extClock(const ulong idata clk);

#endif /* _HSK_BOOT_H_ */
