/** \file
 * HSK Synchronous Serial Interface implementation
 *
 * @note
 *	The SFRs SSC_CONx_O and SSC_CONx_P refer to the same register
 *	address. The different suffixes signify the operation and
 *	programming modes in which the register exposes different bits.
 * @author kami
 */

#include <Infineon/XC878.h>

#include "hsk_ssc.h"

/**
 * Master mode storage bit, so it does not need to be extracted
 * from SSC_CONH all the time.
 */
bool hsk_ssc_master = 1;

/** \var hsk_ssc_buffer
 * Keeps the SSC communication state.
 */
struct {
	/**
	 * Pointer used for storing data read from the serial connection.
	 */
	char xdata * rptr;

	/**
	 * Pointer used to fetch data for writing on the serial connection. 
	 */
	char xdata * wptr;

	/**
	 * Bytes left to read from the connection.
	 */
	ubyte rcount;

	/**
	 * Bytes left to write on the connection.
	 */
	ubyte wcount;
} pdata hsk_ssc_buffer;

/**
 * SYSCON0 Special Function Register Map Control bit.
 */
#define BIT_RMAP	0

/**
 * IRCON1 Error Interrupt Flag for SSC bit.
 */
#define BIT_EIR		0

/**
 * IRCON1 Transmit Interrupt Flag for SSC bit.
 */
#define BIT_TIR		1

/**
 * IRCON1 Receive Interrupt Flag for SSC bit.
 */
#define BIT_RIR		2

/**
 * Transmit and receive interrupt.
 */
void ISR_hsk_ssc(void) interrupt 7 using 1 {
	bool rmap = (SYSCON0 >> BIT_RMAP) & 1;
	RESET_RMAP();
 	SFR_PAGE(_su0, SST0);

	if ((IRCON1 >> BIT_RIR) & 1) {
		IRCON1 &= ~(1 << BIT_RIR);
		if (hsk_ssc_buffer.rcount) {
			*(hsk_ssc_buffer.rptr++) = SSC_RBL;
			hsk_ssc_buffer.rcount--;
		}
	}
	if ((IRCON1 >> BIT_TIR) & 1) {
		IRCON1 &= ~(1 << BIT_TIR);
		if (hsk_ssc_buffer.wcount) {
			SSC_TBL = *(hsk_ssc_buffer.wptr++);
			hsk_ssc_buffer.wcount--;
		}
	}
	if (!hsk_ssc_buffer.wcount && !hsk_ssc_buffer.rcount) {
		ESSC = 0;
	}

	SFR_PAGE(_su0, RST0);
	rmap ? (SET_RMAP()) : (RESET_RMAP());
}

/**
 * PMCON1 Disable Request bit.
 */
#define BIT_SSC_DIS	1

/**
 * SSC_CONH_P Master Select bit.
 */
#define BIT_MS		6

/**
 * MODIEN Error Interrupt Enable Bit for SSC.
 */
#define BIT_EIREN	0

/**
 * MODIEN Transmit Interrupt Enable Bit for SSC.
 */
#define BIT_TIREN	1

/**
 * MODIEN Receive Interrupt Enable Bit for SSC.
 */
#define BIT_RIREN	2

void hsk_ssc_init(const uword baud, const ubyte config, const bool mode) {
	/* Power the SSC module. */
	SFR_PAGE(_su1, noSST);
	PMCON1 &= ~(1 << BIT_SSC_DIS);

	/* Turn SSC off to configure it. */
	SSC_CONH_P = 0;

	/* Set baud rate. */
	SSC_BRH = baud >> 8;
	SSC_BRL = baud & 0xff;

	/* Configure SSC unit. */
	SSC_CONL_P = config;
	SSC_CONH_P = (ubyte)mode << BIT_MS;

	/* Set up the interrupt. */
	SFR_PAGE(_su3, noSST);
	MODIEN |= (1 << BIT_TIREN) | (1 << BIT_RIREN);
	ESSC = 0;
	SFR_PAGE(_su0, noSST);
}

/**
 * MODPISEL3 Master Mode Input Select bits.
 */
#define BIT_MIS		0

/**
 * MODPISEL3 Slave Mode Input Select bits.
 */
#define BIT_SIS		2

/**
 * MODPISEL3 Slave Mode Clock Input Select bits.
 */
#define BIT_CIS		4

/**
 * Input Select bit count.
 */
#define CNT_SEL		2

/**
 * SSC_CONL Loop Back Control bit.
 *
 * Half-duplex mode when set.
 */
#define BIT_LB		7

void hsk_ssc_ports(const ubyte ports) {
	bool master = (((SSC_CONH_P >> BIT_MS) & 1) == SSC_MASTER);
	bool halfDuplex = (SSC_CONL_P >> BIT_LB) & 1;

	/* Configure master RX, slave TX. */
	switch (ports & (((1 << CNT_SEL) - 1) << BIT_MIS)) {
	case SSC_MRST_P14:
		master ? (P1_DIR &= ~(1 << 4)) : (P1_DIR |= 1 << 4);
		SFR_PAGE(_pp2, noSST);
		P1_ALTSEL0 |= 1 << 4;
		P1_ALTSEL1 &= ~(1 << 4);
		SFR_PAGE(_pp3, noSST);
		!master && halfDuplex ? (P1_OD |= 1 << 4) : (P1_OD &= ~(1 << 4));
		break;
	case SSC_MRST_P05:
		master ? (P0_DIR &= ~(1 << 5)) : (P0_DIR |= 1 << 5);
		SFR_PAGE(_pp2, noSST);
		P0_ALTSEL0 |= 1 << 5;
		P0_ALTSEL1 &= ~(1 << 5);
		SFR_PAGE(_pp3, noSST);
		!master && halfDuplex ? (P0_OD |= 1 << 5) : (P0_OD &= ~(1 << 5));
		break;
	case SSC_MRST_P15:
		master ? (P1_DIR &= ~(1 << 5)) : (P1_DIR |= 1 << 5);
		SFR_PAGE(_pp2, noSST);
		master ? (P1_ALTSEL0 &= ~(1 << 5)) : (P1_ALTSEL0 |= 1 << 5);
		master ? (P1_ALTSEL1 &= ~(1 << 5)) : (P1_ALTSEL1 |= 1 << 5);
		SFR_PAGE(_pp3, noSST);
		!master && halfDuplex ? (P1_OD |= 1 << 5) : (P1_OD &= ~(1 << 5));
		break;
	}
	SFR_PAGE(_pp0, noSST);

	/* Configure master TX, slave RX. */
	switch (ports & (((1 << CNT_SEL) - 1) << BIT_SIS)) {
	case SSC_MTSR_P13:
		master ? (P1_DIR |= 1 << 3) : (P1_DIR &= ~(1 << 3));
		SFR_PAGE(_pp2, noSST);
		P1_ALTSEL0 |= 1 << 3;
		P1_ALTSEL1 &= ~(1 << 3);
		SFR_PAGE(_pp3, noSST);
		master && halfDuplex ? (P1_OD |= 1 << 3) : (P1_OD &= ~(1 << 3));
		break;
	case SSC_MTSR_P04:
		master ? (P0_DIR |= 1 << 4) : (P0_DIR &= ~(1 << 4));
		SFR_PAGE(_pp2, noSST);
		P0_ALTSEL0 |= 1 << 4;
		P0_ALTSEL1 &= ~(1 << 4);
		SFR_PAGE(_pp3, noSST);
		master && halfDuplex ? (P0_OD |= 1 << 4) : (P0_OD &= ~(1 << 4));
		break;
	case SSC_MTSR_P14:
		master ? (P1_DIR |= 1 << 4) : (P1_DIR &= ~(1 << 4));
		SFR_PAGE(_pp2, noSST);
		master ? (P1_ALTSEL0 &= ~(1 << 4)) : (P1_ALTSEL0 &= ~(1 << 4));
		master ? (P1_ALTSEL1 |= 1 << 4) : (P1_ALTSEL1 &= ~(1 << 4));
		SFR_PAGE(_pp3, noSST);
		master && halfDuplex ? (P1_OD |= 1 << 4) : (P1_OD &= ~(1 << 4));
		break;
	}
	SFR_PAGE(_pp0, noSST);

	/* Configure master clock output, slave clock input. */
	switch (ports & (((1 << CNT_SEL) - 1) << BIT_CIS)) {
	case SSC_SCLK_P12:
		master ? (P1_DIR |= 1 << 2) : (P1_DIR &= ~(1 << 2));
		SFR_PAGE(_pp2, noSST);
		P1_ALTSEL0 |= 1 << 2;
		P1_ALTSEL1 &= ~(1 << 2);
		SFR_PAGE(_pp3, noSST);
		P1_OD &= ~(1 << 2);
		break;
	case SSC_SCLK_P03:
		master ? (P0_DIR |= 1 << 3) : (P0_DIR &= ~(1 << 3));
		SFR_PAGE(_pp2, noSST);
		P0_ALTSEL0 |= 1 << 3;
		P0_ALTSEL1 &= ~(1 << 3);
		SFR_PAGE(_pp3, noSST);
		P0_OD &= ~(1 << 3);
		break;
	case SSC_SCLK_P13:
		master ? (P1_DIR |= 1 << 3) : (P1_DIR &= ~(1 << 3));
		SFR_PAGE(_pp2, noSST);
		P1_ALTSEL0 &= ~(1 << 3);
		P1_ALTSEL1 |= 1 << 3;
		SFR_PAGE(_pp3, noSST);
		P1_OD &= ~(1 << 3);
		break;
	}
	SFR_PAGE(_pp0, noSST);

	/* Select I/O ports. */
	SFR_PAGE(_su3, noSST);
	MODPISEL3 = ports;
	SFR_PAGE(_su0, noSST);
}

void hsk_ssc_talk(char xdata * buffer, ubyte len) {
	IRCON1 &= ~(1 << BIT_TIR) & ~(1 << BIT_RIR);
	hsk_ssc_buffer.wptr = buffer + 1;
	hsk_ssc_buffer.rptr = buffer;
	hsk_ssc_buffer.wcount = len - 1;
	hsk_ssc_buffer.rcount = len;
	ESSC = 1;
	SSC_TBL = *buffer;
}

/**
 * SSC_CONH_O Enable Bit.
 */
#define BIT_EN		7

void hsk_ssc_enable() {
	/* Turn the power on. */
	SFR_PAGE(_su1, noSST);
	PMCON1 &= ~(1 << BIT_SSC_DIS);
	SFR_PAGE(_su0, noSST);
	/* Turn the module on. */
	SSC_CONH_P |= 1 << BIT_EN;
}

void hsk_ssc_disable() {
	/* Turn the module off. */
	SSC_CONH_P &= ~(1 << BIT_EN);
	/* Turn the power off. */
	SFR_PAGE(_su1, noSST);
	PMCON1 |= 1 << BIT_SSC_DIS;
	SFR_PAGE(_su0, noSST);
}

