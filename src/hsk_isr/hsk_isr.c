/** \file
 * HSK Shared Interrupt Service Routine implementation
 *
 * This contains interrupts, shared between several interrupt sources. These 
 * interrupt sources can hook into the ISRs by storing a callback function in
 * the hsk_isr* data structures.
 *
 * @author kami
 *
 * \section isr_speed ISR Callback Reaction Time
 *
 * The following table describes what happens up to the point that the NMI
 * ISR starts operation (based on SDCC code):
 * | CCLK Cycles| Task				| Instruction	| Duration
 * |-----------:|-------------------------------|---------------|-----------
 * | 0		| Core: Poll interrupt request	|		| 2
 * | 2		| Core: Call ISR		| lcall		| 1 x 4
 * | 6		| ISR setup: Push registers	| 6 x push	| 6 x 4
 * | 30		| ISR setup: Reset PSW		| 1 x mov dir,# | 1 x 4
 * | 34		| ISR: Backup RMAP		| …		| 3 x 2 + 4
 * | 44		| ISR: Reset RMAP		| …		| 2 x 2 + 4
 * | 52		| ISR: Select callback		| …		| … 
 */

#include <Infineon/XC878.h>

#include "hsk_isr.h"

/**
 * SYSCON0 Special Function Register Map Control bit.
 */
#define BIT_RMAP	0

/**
 * This is a dummy function to point unused function pointers to.
 *
 * @private
 */
void dummy(void) using 1 {
}


/**
 * This is a dummy function to point unused function pointers to.
 *
 * @private
 */
void nmidummy(void) using 2 {
}

/**
 * Define callback function pointers for ISR 5.
 */
volatile struct hsk_isr5_callback pdata hsk_isr5 = {&dummy, &dummy, &dummy, &dummy, &dummy, &dummy, &dummy};

/**
 * T2_T2CON Timer 2 Overflow bit.
 */
#define BIT_TF2			7

/**
 * T2_T2CON T2EX bit.
 */
#define BIT_EXF2		6

/**
 * T2CCU_CCTCON CCT Overflow bit.
 */
#define BIT_CCTOVF		3

/**
 * FDCON Normal Divider Overflow bit.
 */
#define BIT_NDOV		2

/**
 * FDCON End of Syn Byte bit.
 */
#define BIT_EOFSYN		4

/**
 * FDCON Syn Byte Error bit.
 */
#define BIT_ERRSYN		5

/**
 * IRCON2 MultiCAN Node 0 bit.
 */
#define BIT_CANSRC0		0

/**
 * Shared interrupt 5 routine.
 *
 * Activate the interrupt by setting ET2 = 1.
 *
 * This interrupt has the following sources:
 * 	- Timer 2 Overflow (TF2)
 * 	- Timer 2 External Event (EXF2)
 * 	- T2CCU CCT Overflow (CCTOVF)
 * 	- Normal Divider Overflow (NDOV)
 *	- End of Syn Byte (EOFSYN)
 *	- Syn Byte Error (ERRSYN)
 *	- CAN Interrupt 0 (CANSRC0)
 *
 * @private
 */
void ISR_hsk_isr5(void) interrupt 5 using 1 {
	bool rmap = (SYSCON0 >> BIT_RMAP) & 1;
	RESET_RMAP();

	SFR_PAGE(_t2_0, SST0);
	if (T2_T2CON & (1 << BIT_TF2)) {
		T2_T2CON &= ~(1 << BIT_TF2);
		hsk_isr5.TF2();
	}
	if (T2_T2CON & (1 << BIT_EXF2)) {
		T2_T2CON &= ~(1 << BIT_EXF2);
		hsk_isr5.EXF2();
	}
	SFR_PAGE(_t2_0, RST0);

	SFR_PAGE(_t2_1, SST0);
	if (T2CCU_CCTCON & (1 << BIT_CCTOVF)) {
		T2CCU_CCTCON &= ~(1 << BIT_CCTOVF);
		hsk_isr5.CCTOVF();
	}
	SFR_PAGE(_t2_2, RST0);

	SFR_PAGE(_su0, SST0);
	if (FDCON & (1 << BIT_NDOV)) {
		FDCON &= ~(1 << BIT_NDOV);
		hsk_isr5.NDOV();
	}
	if (FDCON & (1 << BIT_EOFSYN)) {
		FDCON &= ~(1 << BIT_EOFSYN);
		hsk_isr5.EOFSYN();
	}
	if (FDCON & (1 << BIT_ERRSYN)) {
		FDCON &= ~(1 << BIT_ERRSYN);
		hsk_isr5.ERRSYN();
	}
	if (IRCON2 & (1 << BIT_CANSRC0)) {
		FDCON &= ~(1 << BIT_CANSRC0);
		hsk_isr5.CANSRC0();
	}
	SFR_PAGE(_su0, RST0);

	rmap ? (SET_RMAP()) : (RESET_RMAP());
}

/**
 * Define callback function pointers for ISR 6.
 */
volatile struct hsk_isr6_callback pdata hsk_isr6 = {&dummy, &dummy, &dummy, &dummy};

/**
 * IRCON1 Interrupt Flag 1 for MultiCAN bit.
 */
#define BIT_CANSRC1		5

/**
 * IRCON1 Interrupt Flag 2 for MultiCAN bit.
 */
#define BIT_CANSRC2		6

/**
 * IRCON1 Interrupt Flag 0 for ADC bit.
 */
#define BIT_ADCSR0		3

/**
 * IRCON1 Interrupt Flag 1 for ADC bit.
 */
#define BIT_ADCSR1		4

/**
 * Shared interrupt 6 routine.
 *
 * Activate the interrupt by setting EADC = 1.
 *
 * This interrupt has the following sources:
 * 	- CANSRC1
 * 	- CANSRC2
 * 	- ADCSR0
 * 	- ADCSR1
 *
 * @private
 */
void ISR_hsk_isr6(void) interrupt 6 using 1 {
	bool rmap = (SYSCON0 >> BIT_RMAP) & 1;
	RESET_RMAP();

	SFR_PAGE(_su0, SST0);
	if (IRCON1 & (1 << BIT_CANSRC1)) {
		IRCON1 &= ~(1 << BIT_CANSRC1);
		hsk_isr6.CANSRC1();
	}
	if (IRCON1 & (1 << BIT_CANSRC2)) {
		IRCON1 &= ~(1 << BIT_CANSRC2);
		hsk_isr6.CANSRC2();
	}
	if (IRCON1 & (1 << BIT_ADCSR0)) {
		IRCON1 &= ~(1 << BIT_ADCSR0);
		hsk_isr6.ADCSR0();
	}
	if (IRCON1 & (1 << BIT_ADCSR1)) {
		IRCON1 &= ~(1 << BIT_ADCSR1);
		hsk_isr6.ADCSR1();
	}
	SFR_PAGE(_su0, RST0);

	rmap ? (SET_RMAP()) : (RESET_RMAP());
}

/**
 * Define callback function pointers for ISR 9.
 */
volatile struct hsk_isr9_callback pdata hsk_isr9 = {&dummy, &dummy, &dummy, &dummy, &dummy};

/**
 * IRCON0 Interrupt Flag for External Interrupt 3 or T2CC0 Capture/Compare
 * Channel bit.
 */
#define BIT_EXINT3		3

/**
 * IRCON0 Interrupt Flag for External Interrupt 4 or T2CC1 Capture/Compare
 * Channel bit.
 */
#define BIT_EXINT4		4

/**
 * IRCON0 Interrupt Flag for External Interrupt 5 or T2CC2 Capture/Compare
 * Channel bit.
 */
#define BIT_EXINT5		5

/**
 * IRCON0 Interrupt Flag for External Interrupt 6 or T2CC3 Capture/Compare
 * Channel bit.
 */
#define BIT_EXINT6		6

/**
 * IRCON2 Interrupt Flag 3 for MultiCAN bit.
 */
#define BIT_CANSRC3		4

/**
 * Shared interrupt 9 routine.
 *
 * Activate the interrupt by setting EXM = 1.
 *
 * This interrupt has the following sources:
 * 	- EXINT3/T2CC0
 * 	- EXINT4/T2CC1
 * 	- EXINT5/T2CC2
 * 	- EXINT6/T2CC3
 * 	- CANSRC3
 *
 * @private
 */
void ISR_hsk_isr9(void) interrupt 9 using 1 {
	bool rmap = (SYSCON0 >> BIT_RMAP) & 1;
	RESET_RMAP();

	SFR_PAGE(_su0, SST0);
	if (IRCON0 & (1 << BIT_EXINT3)) {
		IRCON0 &= ~(1 << BIT_EXINT3);
		hsk_isr9.EXINT3();
	}
	if (IRCON0 & (1 << BIT_EXINT4)) {
		IRCON0 &= ~(1 << BIT_EXINT4);
		hsk_isr9.EXINT4();
	}
	if (IRCON0 & (1 << BIT_EXINT5)) {
		IRCON0 &= ~(1 << BIT_EXINT5);
		hsk_isr9.EXINT5();
	}
	if (IRCON0 & (1 << BIT_EXINT6)) {
		IRCON0 &= ~(1 << BIT_EXINT6);
		hsk_isr9.EXINT6();
	}
	if (IRCON2 & (1 << BIT_CANSRC3)) {
		IRCON1 &= ~(1 << BIT_CANSRC3);
		hsk_isr9.CANSRC3();
	}
	SFR_PAGE(_su0, RST0);

	rmap ? (SET_RMAP()) : (RESET_RMAP());
}

/**
 * Define callback function pointers for NMI ISR.
 */
volatile struct hsk_isr14_callback pdata hsk_isr14 = {&nmidummy, &nmidummy, &nmidummy, &nmidummy, &nmidummy};

/**
 * NMISR Watchdog Timer NMI Flag bit.
 */
#define BIT_NMIWDT		0

/**
 * NMISR PLL NMI Flag bit.
 */
#define BIT_NMIPLL		1

/**
 * NMISR FLASH Timer NMI Flag bit.
 */
#define BIT_NMIFLASH		2

/**
 * NMISR VDDP Prewarning NMI Flag bit.
 */
#define BIT_NMIVDDP		5

/**
 * NMISR ECC NMI Flag bit.
 */
#define BIT_NMIECC		6

/**
 * Shared non-maskable interrupt routine.
 *
 * This interrupt has the following sources:
 *	- Watchdog Timer NMI (NMIWDT)
 *	- PLL NMI (NMIPLL)
 *	- Flash Timer NMI (NMIFLASH)
 *	- VDDP Prewarning NMI (NMIVDDP)
 *	- Flash ECC NMI (NMIECC) 
 *
 * @private
 */
void ISR_hsk_isr14(void) interrupt 14 using 2 {
	bool rmap = (SYSCON0 >> BIT_RMAP) & 1;
	RESET_RMAP();

	SFR_PAGE(_su0, SST2);
	if (NMISR & (1 << BIT_NMIWDT)) {
		NMISR &= ~(1 << BIT_NMIWDT);
		hsk_isr14.NMIWDT();
	}
	if (NMISR & (1 << BIT_NMIPLL)) {
		NMISR &= ~(1 << BIT_NMIPLL);
		hsk_isr14.NMIPLL();
	}
	if (NMISR & (1 << BIT_NMIFLASH)) {
		NMISR &= ~(1 << BIT_NMIFLASH);
		hsk_isr14.NMIFLASH();
	}
	if (NMISR & (1 << BIT_NMIVDDP)) {
		NMISR &= ~(1 << BIT_NMIVDDP);
		hsk_isr14.NMIVDDP();
	}
	if (NMISR & (1 << BIT_NMIECC)) {
		NMISR &= ~(1 << BIT_NMIECC);
		hsk_isr14.NMIECC();
	}
	SFR_PAGE(_su0, RST2);

	rmap ? (SET_RMAP()) : (RESET_RMAP());
}

