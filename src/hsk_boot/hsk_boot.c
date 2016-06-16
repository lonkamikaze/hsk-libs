/** \file
 * HSK Boot implementation
 *
 * The High Speed Karlsruhe XC878 boot up code implementation.
 *
 * This obsoletes 3rd party provided assembler boot code.
 *
 * @author kami
 */

#include <Infineon/XC878.h>

#include "hsk_boot.h"

#include "../hsk_isr/hsk_isr.h"
#include "../hsk_io/hsk_io.h"

/**
 * Initialises all IO ports as input ports without pull.
 *
 * @private
 */
void hsk_boot_io(void) {
	/* Deactivate internal pullups. */
	IO_PORT_PULL_INIT(P0, -1, IO_PORT_PULL_DISABLE, IO_PORT_PULL_UP);
	IO_PORT_PULL_INIT(P1, -1, IO_PORT_PULL_DISABLE, IO_PORT_PULL_UP);
	IO_PORT_PULL_INIT(P3, -1, IO_PORT_PULL_DISABLE, IO_PORT_PULL_UP);
	IO_PORT_PULL_INIT(P4, -1, IO_PORT_PULL_DISABLE, IO_PORT_PULL_UP);
	IO_PORT_PULL_INIT(P5, -1, IO_PORT_PULL_DISABLE, IO_PORT_PULL_UP);
}

/**
 * MEX3 XRAM Bank Number bits.
 *
 * Used to select the memory bank where the XRAM is located.
 * This 4 bit field is divided, the highest bit goes into the BIT_MXB19 bit.
 */
#define BIT_MXB          0

/**
 * MEX3 XRAM Bank Number bit count.
 */
#define CNT_MXB          3

/**
 * MEX3 XRAM Bank Number highest bit.
 *
 * The final MXB bit.
 */
#define BIT_MXB19        4

/**
 * The selected XRAM bank number.
 */
#define XRAM_BANK        0xF

/**
 * MEX3 XRAM Bank Selector bit.
 */
#define BIT_MXM          3

/**
 * Set BIT_MXM to access the data memroy bank with MOVX instructions.
 *
 * Otherwise the current bank (whichever that is) would be addressed.
 * MOVX is used to access external memory. The data memory bank is selected
 * with the MXB bits.
 */
#define XRAM_SELECTOR    1

/**
 * The page to locate pdata at.
 *
 * Use the first XRAM page, because that is where the compilers expect it.
 */
#define PDATA_PAGE       0xF0

/**
 * Sets up xdata and pdata memory access.
 *
 * Refer to the Processor Architecture and Memory Organization chapters of the
 * XC878 User Manual.
 *
 * @private
 */
void hsk_boot_mem(void) {
	MEX3 = XRAM_SELECTOR << BIT_MXM \
		| ((XRAM_BANK & ((1 << CNT_MXB) - 1)) << BIT_MXB) \
		| ((XRAM_BANK >> CNT_MXB) << BIT_MXB19);

	/* Set pdata page. */
	SFR_PAGE(_su3, noSST);
	XADDRH = PDATA_PAGE;
	SFR_PAGE(_su0, noSST);
}

/**
 * Turns off pullup/-down for all ports prior to global/static initialisation.
 *
 * This function is automatically linked by SDCC and called from startup.a51
 * by Keil C51.
 *
 * @return
 *	Always returns 0, which indicates that SDCC should initialise globals
 *	and statics
 * @private
 */
ubyte _sdcc_external_startup(void) {
	/* Deactivate pullups. */
	hsk_boot_io();
	
	/* Provide XDATA and PDATA access. */
	hsk_boot_mem();
	return 0;
}

/**
 * OSC_CON bit.
 *
 * External Oscillator Run Status Bit, used to determine
 * whether the external oscilator is available.
 */
#define BIT_EXTOSCR      0

/**
 * OSC_CON bit.
 *
 * External Oscillator Watchdog Reset, used when switching
 * to an external clock.
 */
#define BIT_EORDRES      1

/**
 * OSC_CON bit.
 *
 * Oscillator Source Select, used to turn the external
 * oscillator on(1)/off(0).
 */
#define BIT_OSCSS        2

/**
 * OSC_CON bit.
 *
 * XTAL Power Down Control, used when switching to an
 * external clock.
 */
#define BIT_XPD          3

/**
 * OSC_CON bit.
 *
 * PLL Power Down Control, used when switching to an
 * external clock.
 */
#define BIT_PLLPD        5

/**
 * OSC_CON bit.
 *
 * PLL Output Bypass Control, used when switching to an
 * external clock.
 */
#define BIT_PLLBYP       6

/**
 * OSC_CON bit.
 *
 * PLL Watchdog Reset, used when switching to an
 * external clock.
 */
#define BIT_PLLRDRES     7

/**
 * PLL_CON bit.
 *
 * PLL Lock Status Flag, used when switching to an
 * external clock.
 */
#define BIT_PLL_LOCK     0

/**
 * PLL_CON bit.
 *
 * PLL Run Status Flag, used when switching to an
 * external clock.
 */
#define BIT_PLLR         1

/**
 * PLL_CON1 bit.
 *
 * Something to do with the CPU clock.
 */
#define BIT_PDIV         0

/**
 * PDIV bit count.
 */
#define CNT_PDIV         5

/**
 * PLL_CON low PLL NF-Divider bits.
 */
#define BIT_NDIVL        2

/**
 * NDIVL bit count.
 */
#define CNT_NDIVL        6

/**
 * PLL_CON1 high PLL NF-Divider bits.
 */
#define BIT_NDIVH        5

/**
 * NDIVH bit count.
 */
#define CNT_NDIVH        3

/**
 * NMICON PLL Loss of Clock NMI Enable bit.
 */
#define BIT_NMIPLL       1

/** \var boot
 * Boot parameter storage for the loss of clock ISR callback.
 */
static struct {
	/**
	 * The PDIV value for the configured clock speed.
	 *
	 * See table 7-5 in the data sheet for desired PDIV values.
	 * See the PDIV description for value encoding.
	 */
	ubyte pdiv;

	/**
	 * The NDIV value for the configured clock speed.
	 *
	 * See table 7-5 in the data sheet for desired NDIV values.
	 * See the NDIV description for value encoding.
	 */
	uword ndiv;
} xdata boot;

/**
 * Loss of clock recovery ISR.
 *
 * This takes very long.
 *
 * @private
 */
#pragma save
#ifdef SDCC
#pragma nooverlay
#endif
void hsk_boot_isr_nmipll(void) using 2 {
	/*
	 * Loop counter for active waiting.
	 */
	ubyte idata activeWait;

	/* Go to page 1 where all the oscillator registers and the PASSWD
	 * registers are. */
	SFR_PAGE(_su1, SST3);

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
		/* That would be about 4 - 8 loop cycles. A little more does
		 * not hurt, though. */
		for (activeWait = 16; activeWait > 0; activeWait--);
		/* If bit OSC_CON.EXTOSCR is set after 65 internal oscillator
		 * clock cycles, then: */
	} while (!((OSC_CON >> BIT_EXTOSCR) & 0x01));

	/* Select the external oscillator as the source of oscillator
	 * (OSCSS = 1). */
	MAIN_vUnlockProtecReg();
	OSC_CON	|= 1 << BIT_OSCSS;
	/* Reprogram NDIV, PDIV and KDIV values is necessary. */
	activeWait = (boot.ndiv >> CNT_NDIVL) << BIT_NDIVH;
	activeWait |= boot.pdiv << BIT_PDIV;
	MAIN_vUnlockProtecReg();
	PLL_CON1 = activeWait;
	activeWait = (boot.ndiv & ((1 << CNT_NDIVL) - 1)) << BIT_NDIVL;
	MAIN_vUnlockProtecReg();
	PLL_CON = activeWait;

	/* Change to PLL normal operation mode (PLLPD=0). */
	MAIN_vUnlockProtecReg();
	OSC_CON &= ~(1 << BIT_PLLPD);
	/* Wait until the PLL_LOCK and PLLR flag has been set. */
	while (!(PLL_CON & (1 << BIT_PLL_LOCK) && PLL_CON & (1 << BIT_PLLR)));

	/* Disable the bypass of PLL output (PLLBYP = 0) and normal operation
	 * resumed. */
	MAIN_vUnlockProtecReg();
	OSC_CON &= ~(1 << BIT_PLLBYP);

	/* Restore original page. */
	SFR_PAGE(_su1, RST3);

}
#pragma restore

void hsk_boot_extClock(const ulong clk) {
	/**
	 * <b>WARNING - Here be dragons ...</b>
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
	 * Loop counter for active waiting.
	 */
	ubyte activeWait;

	/*
	 * Used to calculate NDIV and PDIV.
	 * See table 7-5 in the data sheet for desired NDIV and PDIV values.
	 * See the NDIV and PDIV descriptions for value encoding.
	 */
	boot.ndiv = 144 - 2;
	boot.pdiv = clk / 1000000UL - 2;

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
	/*
	 * At this point the core is still running with 4MHz, so 1ms (1 really
	 * ought to be enough) would be 4000 clock cycles. One cycle through
	 * the active waiting loop can be expected to take around 16clk cycles.
	 */
	for (activeWait = 250; activeWait > 0; activeWait--);
	/* Restart the external oscillator watchdog by setting bit EORDRES. */
	MAIN_vUnlockProtecReg();
	OSC_CON |= 1 << BIT_EORDRES;
	/* Wait for 65 cycles based on internal oscillator frequency. */
	for (activeWait = 16; activeWait > 0; activeWait--);
	/* Wait until EXTOSCR is set. */
	while (!((OSC_CON >> BIT_EXTOSCR) & 0x01));
	/* The source of external oscillator is selected by setting bit OSCSS.
	 * External oscillator as system clock */
	MAIN_vUnlockProtecReg();
	OSC_CON	|= 1 << BIT_OSCSS;
	/* Program desired NDIV, PDIV and KDIV values. */
	/* The KDIV value is not touched. */
	activeWait = (boot.ndiv >> CNT_NDIVL) << BIT_NDIVH;
	activeWait |= boot.pdiv << BIT_PDIV;
	MAIN_vUnlockProtecReg();
	PLL_CON1 = activeWait;
	activeWait = (boot.ndiv & ((1 << CNT_NDIVL) - 1)) << BIT_NDIVL;
	MAIN_vUnlockProtecReg();
	PLL_CON = activeWait;

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
	hsk_isr14.NMIPLL = &hsk_boot_isr_nmipll;
	NMICON |= 1 << BIT_NMIPLL;
}

