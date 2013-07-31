/** \file
 * HSK Synchronous Serial Interface implementation
 *
 * @author kami
 */

#include <Infineon/XC878.h>

#include "hsk_ssc.h"

#include "../hsk_isr/hsk_isr.h"

/**
 * PMCON1 Disable Request bit.
 */
#define BIT_SSC_DIS	1

/**
 * SSC_CONH_O Master Select bit.
 */
#define BIT_MS		6

void hsk_ssc_init(const uword baud, const ubyte config, const bool mode) {
	/* Power the SSC module. */
	SFR_PAGE(_su1, noSST);
	PMCON1 &= ~(1 << BIT_SSC_DIS);
	SFR_PAGE(_su0, noSST);

	/* Turn SSC off to configure it. */
	SSC_CONH_O = 0;

	/* Set baud rate. */
	SSC_BRH = baud >> 8;
	SSC_BRL = baud & 0xff;

	/* Configure SSC unit. */
	SSC_CONL_O = config;
	SSC_CONH_O = mode << BIT_MS;
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

void hsk_ssc_ports(const ubyte ports) {
	bool master = (SSC_CONH_O >> BIT_MS) & 1 == SSC_MASTER;

	/* Configure master RX, slave TX. */
	switch (ports & (((1 << CNT_SEL) - 1) << BIT_MIS)) {
	case SSC_MRST_P14:
		master ? (P1_DIR &= ~(1 << 4)) : (P1_DIR |= 1 << 4);
	case SSC_MRST_P05:
		master ? (P0_DIR &= ~(1 << 5)) : (P0_DIR |= 1 << 5);
	case SSC_MRST_P15:
		master ? (P1_DIR &= ~(1 << 5)) : (P1_DIR |= 1 << 5);
	}

	/* Configure master TX, slave RX. */
	switch (ports & (((1 << CNT_SEL) - 1) << BIT_SIS)) {
	case SSC_MTSR_P13:
		master ? (P1_DIR |= 1 << 3) : (P1_DIR &= ~(1 << 3));
	case SSC_MTSR_P04:
		master ? (P0_DIR |= 1 << 4) : (P0_DIR &= ~(1 << 4));
	case SSC_MTSR_P14:
		master ? (P1_DIR |= 1 << 4) : (P1_DIR &= ~(1 << 4));
	}

	/* Configure master clock output, slave clock input. */
	switch (ports & (((1 << CNT_SEL) - 1) << BIT_CIS)) {
	case SSC_SCLK_P12:
		master ? (P1_DIR |= 1 << 2) : (P1_DIR &= ~(1 << 2));
	case SSC_SCLK_P03:
		master ? (P0_DIR |= 1 << 3) : (P0_DIR &= ~(1 << 3));
	case SSC_SCLK_P13:
		master ? (P1_DIR |= 1 << 3) : (P1_DIR &= ~(1 << 3));
	}

	/* Select I/O ports. */
	SFR_PAGE(_su3, noSST);
	MODPISEL3 = ports;
	SFR_PAGE(_su0, noSST);
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
	SSC_CONH_O |= 1 << BIT_EN;
}

void hsk_ssc_disable() {
	/* Turn the module off. */
	SSC_CONH_O &= ~(1 << BIT_EN);
	/* Turn the power off. */
	SFR_PAGE(_su1, noSST);
	PMCON1 |= 1 << BIT_SSC_DIS;
	SFR_PAGE(_su0, noSST);
}

