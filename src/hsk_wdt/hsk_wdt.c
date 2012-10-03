/** \file
 * HSK Watchdog Timer implementation
 *
 * The WDT is a 16bit counter that counts up, upon overflow a reset is
 * initiated. When the timer is serviced the higher byte is loaded from the
 * WDTREL SFR.
 *
 * @author
 *	kami
 *
 * \section wdt_registers Watchdog Timer Registers
 *
 * All registers are in the mapped register are, i.e. RMAP=1 must be set to
 * access them.
 */

#include <Infineon/XC878.h>

#include "hsk_wdt.h"

#include "../hsk_isr/hsk_isr.h"

/**
 * WDTCON Watchdog Timer Input Frequency Selection bit.
 *
 * Used to select PCLK/128 instead of PCLK/2.
 */
#define BIT_WDTIN	0

void hsk_wdt_init(const uword idata window) {
	/**
 	 * The WDT runs at PCLK/2 or PCLK/128, i.e. the WDT low byte WDTL
	 * overlow occurs either every 21.333µs or every 1365.333ms.
	 *
	 * One time unit (10µs) equals 120 PCLK/2 clock ticks. One PCLK/128
	 * time unit (640µs) equals 120 PCLK/128 clock ticks.
	 */

	SET_RMAP();

	/*
	 * Check if the time fits into the PCLK/2 mode.
	 *
	 * 256 is the overflow value of WDTH, \f$ 32 / 15 \f$ is the
	 * overflow time of the low byte \f$ 256 / 120 \f$.
	 */
	if (window <= 256 * 32 / 15) {
		WDTCON &= ~(1 << BIT_WDTIN);
		/* Set the reload value. */
		WDTREL = - (window * 15 + 16) / 32;
	} else {
		WDTCON |= 1 << BIT_WDTIN;
		/* Set the reload value. */
		WDTREL = - (window / 64 * 15 + 16) / 32;
	}

	RESET_RMAP();
}

/**
 * WDTCON WDT Enable bit.
 *
 * This bit is protected.
 */
#define	BIT_WDTEN	2

void hsk_wdt_enable(void) {
	bool ea = EA;

	SFR_PAGE(_su1, noSST);
	EA = 0;
	MAIN_vUnlockProtecReg();
	SET_RMAP();
	WDTCON |= 1 << BIT_WDTEN;
	EA = ea;
	SFR_PAGE(_su0, noSST);
	RESET_RMAP();
}

void hsk_wdt_disable(void) {
	bool ea = EA;

	SFR_PAGE(_su1, noSST);
	EA = 0;
	MAIN_vUnlockProtecReg();
	SET_RMAP();
	WDTCON &= ~(1 << BIT_WDTEN);
	EA = ea;
	SFR_PAGE(_su0, noSST);
	RESET_RMAP();
}

/**
 * WDTCON WDT Refresh Start bit.
 */
#define	BIT_WDTRS	1

void hsk_wdt_service(void) {
	SET_RMAP();
	/* Set the watchdog timer refresh (WDTRS) bit. */
	WDTCON |= 1 << BIT_WDTRS;
	RESET_RMAP();
}

