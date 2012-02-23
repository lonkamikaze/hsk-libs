/** \file
 * HSK Boot implementation
 * 
 * The High Speed Karlsruhe XC878 boot up code implementation.
 *
 * This obsoletes 3rd party provided assembler boot code.
 *
 * @author kami
 * @version 2012-02-08
 */

#include <Infineon/XC878.h>

#include "hsk_boot.h"

#include "../hsk_isr/hsk_isr.h"

/**
 * MEX3 XRAM Bank Number bits.
 *
 * Used to select the memory bank where the XRAM is located.
 * This 4 bit field is divided, the highest bit goes into the BIT_MXB19 bit.
 */
#define BIT_MXB			0

/**
 * MEX3 XRAM Bank Number bit count.
 */
#define CNT_MXB			3

/**
 * MEX3 XRAM Bank Number highest bit.
 *
 * The final MXB bit.
 */
#define BIT_MXB19		4

/**
 * The selected XRAM bank number.
 */
#define XRAM_BANK		0xF

/**
 * MEX3 XRAM Bank Selector bit.
 */
#define BIT_MXM			3

/**
 * Set BIT_MXM to access XRAM, the alternative would be banking mode, which
 * would allow us to separate modules into their idividual memory spaces, but
 * deprive us of the use of XRAM.
 */
#define XRAM_SELECTOR		1

/**
 * Sets up xdata memory access.
 *
 * Refer to the Processor Architecture and Memory Organization chapters of the
 * XC878 User Manual.
 */
void hsk_boot_mem(void) {
	MEX3 = XRAM_SELECTOR << BIT_MXM \
		| ((XRAM_BANK & ((1 << CNT_MXB) - 1)) << BIT_MXB) \
		| ((XRAM_BANK >> CNT_MXB) << BIT_MXB19);
}

/**
 * OSC_CON bit.
 *
 * External Oscillator Run Status Bit, used to determine
 * whether the external oscilator is available.
 */
#define BIT_EXTOSCR		0

/**
 * OSC_CON bit.
 *
 * External Oscillator Watchdog Reset, used when switching
 * to an external clock.
 */
#define BIT_EORDRES		1

/**
 * OSC_CON bit.
 *
 * Oscillator Source Select, used to turn the external
 * oscillator on(1)/off(0).
 */
#define BIT_OSCSS		2

/**
 * OSC_CON bit.
 *
 * XTAL Power Down Control, used when switching to an
 * external clock.
 */
#define BIT_XPD			3

/**
 * OSC_CON bit.
 *
 * PLL Power Down Control, used when switching to an
 * external clock.
 */
#define BIT_PLLPD		5

/**
 * OSC_CON bit.
 *
 * PLL Output Bypass Control, used when switching to an
 * external clock.
 */
#define BIT_PLLBYP		6

/**
 * OSC_CON bit.
 *
 * PLL Watchdog Reset, used when switching to an
 * external clock.
 */
#define BIT_PLLRDRES	7

/**
 * PLL_CON bit.
 *
 * PLL Lock Status Flag, used when switching to an
 * external clock.
 */
#define BIT_PLL_LOCK	0

/**
 * PLL_CON bit.
 *
 * PLL Run Status Flag, used when switching to an
 * external clock.
 */
#define BIT_PLLR		1

/**
 * PLL_CON1 bit.
 *
 * Something to do with the CPU clock.
 */
#define BIT_PDIV		0

/**
 * PDIV bit count.
 */
#define CNT_PDIV		5

/**
 * NMICON PLL Loss of Clock NMI Enable bit.
 */
#define BIT_NMIPLL 		1

/**
 * The PDIV value for the configured clock speed.
 * See table 7-5 in the data sheet for desired PDIV values.
 * See the PDIV description for value encoding.
 */
ubyte xdata pdiv;

/**
 * Loss of clock recovery ISR.
 */
#pragma save
#pragma nooverlay
void hsk_nmipll_isr(void) {
	/*
	 * Non-optimizable loop counter for active waiting.
	 */
	ubyte xdata active_wait;

	/* Go to page1 where all the oscillator registers are. */
	SFR_PAGE(_su1, SST1);

	do {
		/* Restart the external oscillator watchdog by setting bit
		 * EORDRES. */
		MAIN_vUnlockProtecReg();
		OSC_CON |= 1 << BIT_EORDRES;
		/* Restart the PLL watchdog by setting bit PLLRDRES */
		OSC_CON |= 1 << BIT_PLLRDRES;
		/* Bypass the PLL output (PLLBYP = 1). */
		MAIN_vUnlockProtecReg();
		OSC_CON |= 1 << BIT_PLLBYP;
		/* Select the PLL power down mode (PLLPD = 1). */
		MAIN_vUnlockProtecReg();
		OSC_CON |= 1 << BIT_PLLPD;
		/* Select the internal oscillator as the source of oscillator
		 * (OSCSS = 0). */
		MAIN_vUnlockProtecReg();
		OSC_CON	&= ~(1 << BIT_OSCSS);
		/* Wait for 65 cycles based on internal oscillator frequency. */
		/* If bit OSC_CON.EXTOSCR is set after 65 internal oscillator
		 * clock cycles, then: */
		for (active_wait = 200; active_wait > 0; active_wait--);
	} while (!((OSC_CON >> BIT_EXTOSCR) & 0x01));

	/* Select the external oscillator as the source of oscillator
	 * (OSCSS = 1). */
	MAIN_vUnlockProtecReg();
	OSC_CON	|= 1 << BIT_OSCSS;
	/* Reprogram NDIV, PDIV and KDIV values is necessary. */
	MAIN_vUnlockProtecReg();
	PLL_CON1 = (PLL_CON1 & ~(((1 << CNT_PDIV) - 1) << BIT_PDIV)) | (pdiv << BIT_PDIV);

	/* Change to PLL normal operation mode (PLLPD=0). */
	MAIN_vUnlockProtecReg();
	OSC_CON &= ~(1 << BIT_PLLPD);
	/* Wait until the PLL_LOCK and PLLR flag has been set. */
	while (!(PLL_CON & (1 << BIT_PLL_LOCK) && PLL_CON & (1 << BIT_PLLR)));

	/* Disable the bypass of PLL output (PLLBYP = 0) and normal operation
	 * resumed. */
	MAIN_vUnlockProtecReg();
	OSC_CON &= ~(1 << BIT_PLLBYP);

	/*  Restore original page. */
	SFR_PAGE(_su1, RST1);
}
#pragma restore

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
 * This implementation is currently limited to oscilators from 4MHz to 20MHz
 * in 2MHz intervals, because only PDIV is used to control the PLL phase.
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
 *		The frequency of the external oscilator in Hz.
 */
void hsk_boot_extClock(ulong idata clk) {
	/**
	 * WARNING - Here be dragons ...
	 *
	 * Before messing with this stuff you should be aware that this is
	 * tricky business. Mistakes can result in hardware damage. Or at
	 * least all your timers and external interfaces will act weird.
	 *
	 * Basically this bypasses/turns off the PLL, sets up the external
	 * oscilator and than reconfigures the PLL and brings it back into
	 * play.
	 *
	 * Many of the OSC_CON, which is on page 1, bits are write protected.
	 * The MAIN_vUnlockProtecReg() turns the protection off for 32 cycles.
	 * So it has to be turned off each time protected bits are accessed.
	 */

	/*
	 * Non-optimizable loop counter for active waiting.
	 */
	ubyte active_wait;

	/*
	 * Used to calculate PDIV.
	 * See table 7-5 in the data sheet for desired PDIV values.
	 * See the PDIV description for value encoding.
	 */
	pdiv = clk / 2000000UL - 2;

	/* Go to page1 where all the oscillator registers are. */
	SFR_PAGE(_su1, noSST);
	/* Bypass the PLL output (PLLBYP = 1); On-chip oscillator as system clock. */
	MAIN_vUnlockProtecReg();
	OSC_CON |= 1 << BIT_PLLBYP;
	/* Select the PLL power down mode (PLLPD = 1). */
	MAIN_vUnlockProtecReg();
	OSC_CON |= 1 << BIT_PLLPD;
	/* Power up external oscillator (XPD = 0) */
	MAIN_vUnlockProtecReg();
	OSC_CON &= ~(1 << BIT_XPD);
	/* Wait for 1.5 ms until the external oscillator is stable (the delay
	 * time should be adjusted according to different external oscillators).
	 */
	/* Well, actually just do some active waiting for a safe time. */
	for (active_wait = 200; active_wait > 0; active_wait--);
	/* Restart the external oscillator watchdog by setting bit EORDRES. */
	MAIN_vUnlockProtecReg();
	OSC_CON |= 1 << BIT_EORDRES;
	/* Wait for 65 cycles based on internal oscillator frequency. */
	/* Wait until EXTOSCR is set. */
	while (!((OSC_CON >> BIT_EXTOSCR) & 0x01));
	/* The source of external oscillator is selected by setting bit OSCSS.
	 * External oscillator as system clock */
	MAIN_vUnlockProtecReg();
	OSC_CON	|= 1 << BIT_OSCSS;
	/* Program desired NDIV, PDIV and KDIV values. */
	/* Except for PDIV the defaults fit. */
	MAIN_vUnlockProtecReg();
	PLL_CON1 = (PLL_CON1 & ~(((1 << CNT_PDIV) - 1) << BIT_PDIV)) | (pdiv << BIT_PDIV);
	/* Change to PLL normal operation mode (PLLPD=0). */
	MAIN_vUnlockProtecReg();
	OSC_CON &= ~(1 << BIT_PLLPD);
	/* Restart the PLL watchdog by setting PLLRDRES. */
	OSC_CON |= 1 << BIT_PLLRDRES;
	/* Wait untill the PLL_LOCK and PLLR flag has been set. */
	while (!(PLL_CON & (1 << BIT_PLL_LOCK) && PLL_CON & (1 << BIT_PLLR)));
	/* Disable the bypass of PLL output (PLLBYP = 0). */
	MAIN_vUnlockProtecReg();
	OSC_CON &= ~(1 << BIT_PLLBYP);
	/* Deactivate changing protected bits, just for completeness. */
	MAIN_vlockProtecReg();
	SFR_PAGE(_su0, noSST);

	/* Activate PLL Loss of Clock non-maskable interrupt. */
	hsk_isr14.NMIPLL = &hsk_nmipll_isr;
	NMICON |= 1 << BIT_NMIPLL;
}

