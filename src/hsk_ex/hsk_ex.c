/** \file
 * HSK External Interrupt Routing implementation
 *
 * This file implements the methods necessary to route µC pins to external
 * interrupts.
 *
 * @author kami
 */

#include <Infineon/XC878.h>

#include "hsk_ex.h"

#include "../hsk_isr/hsk_isr.h"

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
 * EXICON0/1 External Interrupt Trigger Select bit count.
 */
#define CNT_EXINT	2

/**
 * EXICON0 External Interrupt 0 Trigger Select bits.
 */
#define BIT_EXINT0	0

/**
 * EXICON0 External Interrupt 1 Trigger Select bits.
 */
#define BIT_EXINT1	2

/**
 * EXICON0 External Interrupt 2 Trigger Select bits.
 */
#define BIT_EXINT2	4

/**
 * EXICON0 External Interrupt 3 Trigger Select bits.
 */
#define BIT_EXINT3	6

/**
 * EXICON1 External Interrupt 4 Trigger Select bits.
 */
#define BIT_EXINT4	0

/**
 * EXICON1 External Interrupt 5 Trigger Select bits.
 */
#define BIT_EXINT5	2

/**
 * EXICON1 External Interrupt 6 Trigger Select bits.
 */
#define BIT_EXINT6	4

/**
 * SYSCON0 Interrupt Structure 2 Mode Select bit.
 */
#define BIT_IMODE	4

void hsk_ex_channel_enable(const hsk_ex_channel channel,
		const ubyte edge,
		const void (code * const callback)(void) using(1)) {

	/**
	 * Setting up EXINT0/1 is somewhat confusing. Refer to UM 1.1 section
	 * 5.6.2 to make sense of this.
	 */
	switch (channel) {
	case EX_EXINT0:
		IT0 = 1;
		EXICON0 = EXICON0 & ~(((1 << CNT_EXINT) - 1) << BIT_EXINT0)
			| (edge << BIT_EXINT0);
		EX0 = 1;
		break;
	case EX_EXINT1:
		IT1 = 1;
		EXICON0 = EXICON0 & ~(((1 << CNT_EXINT) - 1) << BIT_EXINT1)
			| (edge << BIT_EXINT1);
		EX1 = 1;
		break;
	case EX_EXINT2:
		if (callback) {
			hsk_isr8.EXINT2 = callback;
		}
		EXICON0 = EXICON0 & ~(((1 << CNT_EXINT) - 1) << BIT_EXINT2)
			| (edge << BIT_EXINT2);
		EX2 = 1;
		break;
	case EX_EXINT3:
		if (callback) {
			hsk_isr9.EXINT3 = callback;
		}
		EXICON0 = EXICON0 & ~(((1 << CNT_EXINT) - 1) << BIT_EXINT3)
			| (edge << BIT_EXINT3);
		EXM = 1;
		break;
	case EX_EXINT4:
		if (callback) {
			hsk_isr9.EXINT4 = callback;
		}
		EXICON1 = EXICON1 & ~(((1 << CNT_EXINT) - 1) << BIT_EXINT4)
			| (edge << BIT_EXINT4);
		EXM = 1;
		break;
	case EX_EXINT5:
		if (callback) {
			hsk_isr9.EXINT5 = callback;
		}
		EXICON1 = EXICON1 & ~(((1 << CNT_EXINT) - 1) << BIT_EXINT5)
			| (edge << BIT_EXINT5);
		EXM = 1;
		break;
	case EX_EXINT6:
		if (callback) {
			hsk_isr9.EXINT6 = callback;
		}
		EXICON1 = EXICON1 & ~(((1 << CNT_EXINT) - 1) << BIT_EXINT6)
			| (edge << BIT_EXINT6);
		EXM = 1;
		break;
	}

	/* Set IMODE, 1 so that individual interrupts may be masked without
	 * loosing them. */
	SYSCON0 |= 1 << BIT_IMODE;
}

/**
 * \addtogroup EX_EDGE
 * @{
 */

/**
 * Deactivate external interrupt.
 */
#define EX_EDGE_DISABLE		3

/**
 * @}
 */

void hsk_ex_channel_disable(const hsk_ex_channel channel) {
	switch (channel) {
	case EX_EXINT0:
		EXICON0 = EXICON0 & ~(((1 << CNT_EXINT) - 1) << BIT_EXINT0)
			| (EX_EDGE_FALLING << BIT_EXINT0);
		IT0 = 0;
		break;
	case EX_EXINT1:
		EXICON0 = EXICON0 & ~(((1 << CNT_EXINT) - 1) << BIT_EXINT1)
			| (EX_EDGE_FALLING << BIT_EXINT1);
		IT0 = 0;
		break;
	case EX_EXINT2:
		EXICON0 = EXICON0 & ~(((1 << CNT_EXINT) - 1) << BIT_EXINT2)
			| (EX_EDGE_DISABLE << BIT_EXINT2);
		break;
	case EX_EXINT3:
		EXICON0 = EXICON0 & ~(((1 << CNT_EXINT) - 1) << BIT_EXINT3)
			| (EX_EDGE_DISABLE << BIT_EXINT3);
		break;
	case EX_EXINT4:
		EXICON1 = EXICON1 & ~(((1 << CNT_EXINT) - 1) << BIT_EXINT4)
			| (EX_EDGE_DISABLE << BIT_EXINT4);
		break;
	case EX_EXINT5:
		EXICON1 = EXICON1 & ~(((1 << CNT_EXINT) - 1) << BIT_EXINT5)
			| (EX_EDGE_DISABLE << BIT_EXINT5);
		break;
	case EX_EXINT6:
		EXICON1 = EXICON1 & ~(((1 << CNT_EXINT) - 1) << BIT_EXINT6)
			| (EX_EDGE_DISABLE << BIT_EXINT6);
		break;
	}
}

/** \var hsk_ex_ports
 * External input configuration structure.
 */
const struct {
	/**
	 * The MODPISEL[n] bit(s) to select.
	 */
	ubyte modpiselBit;

	/**
	 * The MODPISEL value.
	 */
	ubyte modpiselSel;

	/**
	 * The port bit.
	 */
	ubyte portBit;

	/**
	 * The port ALTSEL (alternative select) setting.
	 */
	ubyte portAltsel;
} code hsk_ex_ports[] = {
	/* EXINT0_P05 */ {1, 0, 5, 2},
	/* EXINT3_P11 */ {0, 0, 1, 2},
	/* EXINT0_P14 */ {1, 1, 4, 2},
	/* EXINT5_P15 */ {4, 0, 5, 2},
	/* EXINT6_P16 */ {5, 0, 6, 3},
	/* EXINT3_P30 */ {0, 2, 0, 3},
	/* EXINT4_P32 */ {2, 2, 2, 5},
	/* EXINT5_P33 */ {4, 2, 3, 1},
	/* EXINT6_P34 */ {5, 3, 4, 4},
	/* EXINT4_P37 */ {2, 0, 7, 2},
	/* EXINT3_P40 */ {0, 1, 0, 4},
	/* EXINT4_P41 */ {2, 2, 1, 1},
	/* EXINT6_P42 */ {5, 1, 2, 2},
	/* EXINT5_P44 */ {4, 1, 4, 3},
	/* EXINT6_P45 */ {5, 2, 5, 3},
	/* EXINT1_P50 */ {2, 1, 0, 2},
	/* EXINT2_P51 */ {3, 1, 1, 2},
	/* EXINT5_P52 */ {4, 3, 2, 2},
	/* EXINT1_P53 */ {2, 0, 3, 2},
	/* EXINT2_P54 */ {3, 0, 4, 2},
	/* EXINT3_P55 */ {0, 3, 5, 2},
	/* EXINT4_P56 */ {2, 3, 6, 2},
	/* EXINT6_P57 */ {5, 4, 7, 3}
};

void hsk_ex_port_open(const hsk_ex_port port) {

	/*
	 * Select input port.
	 */
	#define modpiselBit	hsk_ex_ports[port].modpiselBit
	#define modpiselSel	hsk_ex_ports[port].modpiselSel
	switch (port) {
	case EX_EXINT0_P05:
	case EX_EXINT0_P14:
	case EX_EXINT1_P50:
	case EX_EXINT1_P53:
	case EX_EXINT2_P51:
	case EX_EXINT2_P54:
		MODPISEL &= ~(1 << modpiselBit);
		MODPISEL |= modpiselSel << modpiselBit;
		break;
	case EX_EXINT3_P11:
	case EX_EXINT3_P30:
	case EX_EXINT3_P40:
	case EX_EXINT3_P55:
	case EX_EXINT4_P32:
	case EX_EXINT4_P37:
	case EX_EXINT4_P41:
	case EX_EXINT4_P56:
	case EX_EXINT5_P15:
	case EX_EXINT5_P33:
	case EX_EXINT5_P44:
	case EX_EXINT5_P52:
		SFR_PAGE(_su3, noSST);
		MODPISEL4 &= ~(3 << modpiselBit);
		MODPISEL4 |= modpiselSel << modpiselBit;
		SFR_PAGE(_su0, noSST);
		break;
	case EX_EXINT6_P16:
	case EX_EXINT6_P34:
	case EX_EXINT6_P42:
	case EX_EXINT6_P45:
	case EX_EXINT6_P57:
		SFR_PAGE(_su3, noSST);
		MODPISEL1 &= ~(7 << modpiselBit);
		MODPISEL1 |= modpiselSel << modpiselBit;
		SFR_PAGE(_su0, noSST);
		break;
	}
	#undef modpiselBit
	#undef modpiselSel

	/*
	 * Activate input port.
	 */
	#define portBit		hsk_ex_ports[port].portBit
	#define portAltsel	hsk_ex_ports[port].portAltsel
	switch (port) {
	case EX_EXINT0_P05:
		P0_DIR &= ~(1 << portBit);
		SFR_PAGE(_pp2, noSST);
		P0_ALTSEL0 &= ~(1 << portBit);
		P0_ALTSEL0 |= (portAltsel & 1) << portBit;
		P0_ALTSEL1 &= ~(1 << portBit);
		P0_ALTSEL1 |= ((portAltsel >> 1) & 1) << portBit;
		break;
	case EX_EXINT3_P11:
	case EX_EXINT0_P14:
	case EX_EXINT5_P15:
	case EX_EXINT6_P16:
		P1_DIR &= ~(1 << portBit);
		SFR_PAGE(_pp2, noSST);
		P1_ALTSEL0 &= ~(1 << portBit);
		P1_ALTSEL0 |= (portAltsel & 1) << portBit;
		P1_ALTSEL1 &= ~(1 << portBit);
		P1_ALTSEL1 |= ((portAltsel >> 1) & 1) << portBit;
		break;
	case EX_EXINT3_P30:
	case EX_EXINT4_P32:
	case EX_EXINT5_P33:
	case EX_EXINT6_P34:
	case EX_EXINT4_P37:
		P3_DIR &= ~(1 << portBit);
		SFR_PAGE(_pp2, noSST);
		P3_ALTSEL0 &= ~(1 << portBit);
		P3_ALTSEL0 |= (portAltsel & 1) << portBit;
		P3_ALTSEL1 &= ~(1 << portBit);
		P3_ALTSEL1 |= ((portAltsel >> 1) & 1) << portBit;
		break;
	case EX_EXINT3_P40:
	case EX_EXINT4_P41:
	case EX_EXINT6_P42:
	case EX_EXINT5_P44:
	case EX_EXINT6_P45:
		P4_DIR &= ~(1 << portBit);
		SFR_PAGE(_pp2, noSST);
		P4_ALTSEL0 &= ~(1 << portBit);
		P4_ALTSEL0 |= (portAltsel & 1) << portBit;
		P4_ALTSEL1 &= ~(1 << portBit);
		P4_ALTSEL1 |= ((portAltsel >> 1) & 1) << portBit;
		break;
	case EX_EXINT1_P50:
	case EX_EXINT2_P51:
	case EX_EXINT5_P52:
	case EX_EXINT1_P53:
	case EX_EXINT2_P54:
	case EX_EXINT3_P55:
	case EX_EXINT4_P56:
	case EX_EXINT6_P57:
		P5_DIR &= ~(1 << portBit);
		SFR_PAGE(_pp2, noSST);
		P5_ALTSEL0 &= ~(1 << portBit);
		P5_ALTSEL0 |= (portAltsel & 1) << portBit;
		P5_ALTSEL1 &= ~(1 << portBit);
		P5_ALTSEL1 |= ((portAltsel >> 1) & 1) << portBit;
		break;
	}
	SFR_PAGE(_pp0, noSST);
	#undef portBit
	#undef portAltsel
}

void hsk_ex_port_close(const hsk_ex_port port) {
	/*
	 * Deactivate input port.
	 */
	#define portBit		hsk_ex_ports[port].portBit
	switch (port) {
	case EX_EXINT0_P05:
		P0_DIR &= ~(1 << portBit);
		SFR_PAGE(_pp2, noSST);
		P0_ALTSEL0 &= ~(1 << portBit);
		P0_ALTSEL1 &= ~(1 << portBit);
		break;
	case EX_EXINT3_P11:
	case EX_EXINT0_P14:
	case EX_EXINT5_P15:
	case EX_EXINT6_P16:
		P1_DIR &= ~(1 << portBit);
		SFR_PAGE(_pp2, noSST);
		P1_ALTSEL0 &= ~(1 << portBit);
		P1_ALTSEL1 &= ~(1 << portBit);
		break;
	case EX_EXINT3_P30:
	case EX_EXINT4_P32:
	case EX_EXINT5_P33:
	case EX_EXINT6_P34:
	case EX_EXINT4_P37:
		P3_DIR &= ~(1 << portBit);
		SFR_PAGE(_pp2, noSST);
		P3_ALTSEL0 &= ~(1 << portBit);
		P3_ALTSEL1 &= ~(1 << portBit);
		break;
	case EX_EXINT3_P40:
	case EX_EXINT4_P41:
	case EX_EXINT6_P42:
	case EX_EXINT5_P44:
	case EX_EXINT6_P45:
		P4_DIR &= ~(1 << portBit);
		SFR_PAGE(_pp2, noSST);
		P4_ALTSEL0 &= ~(1 << portBit);
		P4_ALTSEL1 &= ~(1 << portBit);
		break;
	case EX_EXINT1_P50:
	case EX_EXINT2_P51:
	case EX_EXINT5_P52:
	case EX_EXINT1_P53:
	case EX_EXINT2_P54:
	case EX_EXINT3_P55:
	case EX_EXINT4_P56:
	case EX_EXINT6_P57:
		P5_DIR &= ~(1 << portBit);
		SFR_PAGE(_pp2, noSST);
		P5_ALTSEL0 &= ~(1 << portBit);
		P5_ALTSEL1 &= ~(1 << portBit);
		break;
	}
	SFR_PAGE(_pp0, noSST);
	#undef portBit
}

