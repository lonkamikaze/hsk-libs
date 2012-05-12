/** \file
 * HSK Shared Interrupt Service Routine headers
 *
 * This header is used by other libraries to use interrupts with multiple
 * sources. A callback function can be provided for each available interrupt
 * source.
 *
 * Callback functions should preserve SFR pages with SST1/RST1.
 *
 * The following table must be obeyed to avoid memory corruption:
 * \code
 *	Save	Restore	Function
 *	SST0	RST0	ISRs
 *	SST1	RST1	ISR callback functions
 *	SST2	RST2	NMI ISR
 *	SST3	RST3	NMI callback functions
 * \endcode
 *
 * Every callback function is called with RMAP = 0. If the callback function
 * changes RMAP it does not have to take care of restoring it. RMAP is always
 * restored to its original state by the shared ISRs.
 *
 * @author kami
 */

#ifndef _HSK_ISR_H_
#define _HSK_ISR_H_

/*
 * ISR prototypes for SDCC.
 */
#ifdef SDCC
#include "hsk_isr.isr"
#endif /* SDCC */

/**
 * Shared interrupt 5 routine. Activate the interrupt by setting ET2 = 1.
 *
 * This interrupt has the following sources:
 * 	- Timer 2 Overflow (TF2)
 * 	- Timer 2 External Event (EXF2)
 * 	- T2CCU CCT Overflow (CCTOVF)
 * 	- Normal Divider Overflow (NDOV)
 *	- End of Syn Byte (EOFSYN)
 *	- Syn Byte Error (ERRSYN)
 *	- CAN Interrupt 0 (CANSRC0)
 */
struct hsk_isr5_callback {
	/**
	 * Function to be called back when the TF2 interrupt event is
	 * triggered.
	 */
	void (code *TF2)(void);

	/**
	 * Function to be called back when the EXF2 interrupt event is
	 * triggered.
	 */
	void (code *EXF2)(void);

	/**
	 * Function to be called back when the CCTOVF interrupt event is
	 * triggered.
	 */
	void (code *CCTOVF)(void);

	/**
	 * Function to be called back when the NDOV interrupt event is
	 * triggered.
	 */
	void (code *NDOV)(void);

	/**
	 * Function to be called back when the EOFSYN interrupt event is
	 * triggered.
	 */
	void (code *EOFSYN)(void);

	/**
	 * Function to be called back when the ERRSYN interrupt event is
	 * triggered.
	 */
	void (code *ERRSYN)(void);

	/**
	 * Function to be called back when the CANSRC0 interrupt event is
	 * triggered.
	 */
	void (code *CANSRC0)(void);
};

/**
 * Introduce callback function pointers for ISR 5.
 */
extern volatile struct hsk_isr5_callback xdata hsk_isr5;

/**
 * Shared interrupt 6 routine. Activate the interrupt by setting EADC = 1.
 *
 * This interrupt has the following sources:
 * 	- CANSRC1
 * 	- CANSRC2
 * 	- ADCSR0
 * 	- ADCSR1
 */
struct hsk_isr6_callback {
	/**
	 * Function to be called back when the CANSRC1 interrupt event is
	 * triggered.
	 */
	void (code *CANSRC1)(void);

	/**
	 * Function to be called back when the CANSRC2 interrupt event is
	 * triggered.
	 */
	void (code *CANSRC2)(void);

	/**
	 * Function to be called back when the ADCSR0 interrupt event is
	 * triggered.
	 */
	void (code *ADCSR0)(void);

	/**
	 * Function to be called back when the ADCSR1 interrupt event is
	 * triggered.
	 */
	void (code *ADCSR1)(void);
};

/**
 * Introduce callback function pointers for ISR 6.
 */
extern volatile struct hsk_isr6_callback xdata hsk_isr6;

/**
 * Shared interrupt 9 routine. Activate the interrupt by setting EXM = 1.
 *
 * This interrupt has the following sources:
 * 	- EXINT3/T2CC0
 * 	- EXINT4/T2CC1
 * 	- EXINT5/T2CC2
 * 	- EXINT6/T2CC3
 * 	- CANSRC2
 */
struct hsk_isr9_callback {
	/**
	 * Function to be called back when the EXINT3/T2CC0 interrupt event is
	 * triggered.
	 */
	void (code *EXINT3)(void);

	/**
	 * Function to be called back when the EXINT4/T2CC1 interrupt event is
	 * triggered.
	 */
	void (code *EXINT4)(void);

	/**
	 * Function to be called back when the EXINT5/T2CC2 interrupt event is
	 * triggered.
	 */
	void (code *EXINT5)(void);

	/**
	 * Function to be called back when the EXINT6/T2CC3 interrupt event is
	 * triggered.
	 */
	void (code *EXINT6)(void);

	/**
	 * Function to be called back when the CANSRC3 interrupt event is
	 * triggered.
	 */
	void (code *CANSRC3)(void);
};

/**
 * Introduce callback function pointers for ISR 9.
 */
extern volatile struct hsk_isr9_callback xdata hsk_isr9;

/**
 * Shared non-maskable interrupt routine.
 *
 * This interrupt has the following sources:
 *	- Watchdog Timer NMI (NMIWDT)
 *	- PLL NMI (NMIPLL)
 *	- Flash Timer NMI (NMIFLASH)
 *	- VDDP Prewarning NMI (NMIVDDP)
 *	- Flash ECC NMI (NMIECC) 
 */
struct hsk_isr14_callback {
	/**
	 * Function to be called back when the NMIWDT interrupt event is
	 * triggered.
	 */
	void (code *NMIWDT)(void);

	/**
	 * Function to be called back when the NMIPLL interrupt event is
	 * triggered.
	 */
	void (code *NMIPLL)(void);

	/**
	 * Function to be called back when the NMIFLASH interrupt event is
	 * triggered.
	 */
	void (code *NMIFLASH)(void);

	/**
	 * Function to be called back when the NMIVDDP interrupt event is
	 * triggered.
	 */
	void (code *NMIVDDP)(void);

	/**
	 * Function to be called back when the NMIECC interrupt event is
	 * triggered.
	 */
	void (code *NMIECC)(void);
};

/**
 * Introduce callback function pointers for NMI ISR.
 *
 * Functions called back from the NMI ISR should use SST3/RST3 instead of
 * SST1/RST1, because they might interrupt other ISRs.
 */
extern volatile struct hsk_isr14_callback xdata hsk_isr14;

#endif /* _HSK_ISR_H_ */
