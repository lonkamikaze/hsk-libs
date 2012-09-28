/** \file
 * HSK Shared Interrupt Service Routine headers
 *
 * This header is used by other libraries to use interrupts with multiple
 * sources. A callback function can be provided for each available interrupt
 * source.
 *
 * @author kami
 *
 * \section isr_pages SFR Pages
 *
 * An ISR callback function cannot make assumptions about current SFR pages
 * like the regular functions that can expect all pages to be set to 0.
 *
 * Instead a callback function needs to set all pages and restore whatever
 * page was in use previously.
 *
 * The following table lists the store and restore selectors by context and
 * must be obeyed to avoid memory corruption:
 * | Save	| Restore	| Context
 * |------------|---------------|------------------------
 * | SST0	| RST0		| ISRs
 * | SST1	| RST1		| ISR callback functions
 * | SST2	| RST2		| NMI ISR
 * | SST3	| RST3		| NMI callback functions
 *
 * Every callback function is called with RMAP = 0. If the callback function
 * changes RMAP it does not have to take care of restoring it. RMAP is always
 * restored to its original state by the shared ISRs.
 *
 * \section isr_banks Register Banks
 *
 * Interrupts are each a root node of their own call tree. This is why they
 * must preserve all the working registers.
 *
 * the pushing and popping of the 8 \c Rn registers for each interrupt call
 * costs 64 CCLK cycles.
 *
 * To avoid this overhead different register banks are used. Call trees, i.e.
 * interrupts, can use the same register bank if they cannot interrupt each
 * other. Each used register bank costs 8 bytes of regular \c data memory.
 * To minimize this cost all interrupts must have the same priority.
 *
 * The following table is used:
 * | Priority	| Context		| Bank
 * |-----------:|-----------------------|------
 * | -		| Regular code		| 0
 * | 0		| ISR, callback		| 1
 * | 1		| ISR, callback		| -
 * | 2		| ISR, callback		| -
 * | 3		| ISR, callback		| -
 * | NMI	| NMI ISR, callback	| 2
 *
 * Assigning higher priority to an ISR will affect (as in break) the operation
 * of all lower priority ISRs.
 */

#ifndef _HSK_ISR_H_
#define _HSK_ISR_H_

/*
 * ISR prototypes for SDCC.
 */
#ifdef SDCC
#include "hsk_isr.isr"
#endif /* SDCC */

/*
 * SDCC does not like the \c code keyword for function pointers, C51 needs it
 * or it will use generic pointers.
 */
#ifdef SDCC
	#undef code
	#define code
#endif /* SDCC */

/*
 * C51 does not include the used register bank in pointer types.
 */
#ifdef __C51__
	#define using(bank)
#endif

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
	void (code *TF2)(void) using(1);

	/**
	 * Function to be called back when the EXF2 interrupt event is
	 * triggered.
	 */
	void (code *EXF2)(void) using(1);

	/**
	 * Function to be called back when the CCTOVF interrupt event is
	 * triggered.
	 */
	void (code *CCTOVF)(void) using(1);

	/**
	 * Function to be called back when the NDOV interrupt event is
	 * triggered.
	 */
	void (code *NDOV)(void) using(1);

	/**
	 * Function to be called back when the EOFSYN interrupt event is
	 * triggered.
	 */
	void (code *EOFSYN)(void) using(1);

	/**
	 * Function to be called back when the ERRSYN interrupt event is
	 * triggered.
	 */
	void (code *ERRSYN)(void) using(1);

	/**
	 * Function to be called back when the CANSRC0 interrupt event is
	 * triggered.
	 */
	void (code *CANSRC0)(void) using(1);
};

/**
 * Introduce callback function pointers for ISR 5.
 */
extern volatile struct hsk_isr5_callback pdata hsk_isr5;

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
	void (code *CANSRC1)(void) using(1);

	/**
	 * Function to be called back when the CANSRC2 interrupt event is
	 * triggered.
	 */
	void (code *CANSRC2)(void) using(1);

	/**
	 * Function to be called back when the ADCSR0 interrupt event is
	 * triggered.
	 */
	void (code *ADCSR0)(void) using(1);

	/**
	 * Function to be called back when the ADCSR1 interrupt event is
	 * triggered.
	 */
	void (code *ADCSR1)(void) using(1);
};

/**
 * Introduce callback function pointers for ISR 6.
 */
extern volatile struct hsk_isr6_callback pdata hsk_isr6;

/**
 * Shared interrupt 8 routine. Activate the interrupt by setting EX2 = 1.
 *
 * This interrupt has the following sources:
 *	- External Interrupt 2 (EXINT2)
 *	- UART1 (RI)
 *	- UART1 (TI)
 *	- Timer 21 Overflow (TF2)
 *	- T21EX (EXF2)
 *	- UART1 Fractional Divider (Normal Divider Overflow) (NDOV)
 *	- CORDIC (EOC)
 *	- MDU Result Ready (IRDY)
 *	- MDU Error (IERR)
 */
struct hsk_isr8_callback {
	/**
	 * Function to be called back when the EXINT2 interrupt event is
	 * triggered.
	 */
	void (code *EXINT2)(void) using(1);

	/**
	 * Function to be called back when the RI interrupt event is
	 * triggered.
	 */
	void (code *RI)(void) using(1);

	/**
	 * Function to be called back when the TI interrupt event is
	 * triggered.
	 */
	void (code *TI)(void) using(1);

	/**
	 * Function to be called back when the TF2 interrupt event is
	 * triggered.
	 */
	void (code *TF2)(void) using(1);

	/**
	 * Function to be called back when the EXF2 interrupt event is
	 * triggered.
	 */
	void (code *EXF2)(void) using(1);

	/**
	 * Function to be called back when the NDOV interrupt event is
	 * triggered.
	 */
	void (code *NDOV)(void) using(1);

	/**
	 * Function to be called back when the EOC interrupt event is
	 * triggered.
	 */
	void (code *EOC)(void) using(1);

	/**
	 * Function to be called back when the IRDY interrupt event is
	 * triggered.
	 */
	void (code *IRDY)(void) using(1);

	/**
	 * Function to be called back when the IERR interrupt event is
	 * triggered.
	 */
	void (code *IERR)(void) using(1);
};

/**
 * Introduce callback function pointers for ISR 8.
 */
extern volatile struct hsk_isr8_callback pdata hsk_isr8;

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
	void (code *EXINT3)(void) using(1);

	/**
	 * Function to be called back when the EXINT4/T2CC1 interrupt event is
	 * triggered.
	 */
	void (code *EXINT4)(void) using(1);

	/**
	 * Function to be called back when the EXINT5/T2CC2 interrupt event is
	 * triggered.
	 */
	void (code *EXINT5)(void) using(1);

	/**
	 * Function to be called back when the EXINT6/T2CC3 interrupt event is
	 * triggered.
	 */
	void (code *EXINT6)(void) using(1);

	/**
	 * Function to be called back when the CANSRC3 interrupt event is
	 * triggered.
	 */
	void (code *CANSRC3)(void) using(1);
};

/**
 * Introduce callback function pointers for ISR 9.
 */
extern volatile struct hsk_isr9_callback pdata hsk_isr9;

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
	void (code *NMIWDT)(void) using(2);

	/**
	 * Function to be called back when the NMIPLL interrupt event is
	 * triggered.
	 */
	void (code *NMIPLL)(void) using(2);

	/**
	 * Function to be called back when the NMIFLASH interrupt event is
	 * triggered.
	 */
	void (code *NMIFLASH)(void) using(2);

	/**
	 * Function to be called back when the NMIVDDP interrupt event is
	 * triggered.
	 */
	void (code *NMIVDDP)(void) using(2);

	/**
	 * Function to be called back when the NMIECC interrupt event is
	 * triggered.
	 */
	void (code *NMIECC)(void) using(2);
};

/**
 * Introduce callback function pointers for NMI ISR.
 *
 * Functions called back from the NMI ISR should use SST3/RST3 instead of
 * SST1/RST1, because they might interrupt other ISRs.
 */
extern volatile struct hsk_isr14_callback pdata hsk_isr14;

/*
 * Restore the usual meaning of \c code.
 */
#ifdef SDCC
	#undef code
	#define code	__code
#endif /* SDCC */

/*
 * Restore the usual meaning of \c using(bank).
 */
#ifdef __C51__
	#undef using
#endif /* __C51__ */

#endif /* _HSK_ISR_H_ */
