/** \file
 * HSK Pulse Width Counter implementation
 *
 * The Pulse Width Conter (PWC) module uses the T2CCU Capture/Compare Timer
 * (CCT) to measure pulse width.
 *
 * @author kami
 */

#include <Infineon/XC878.h>

#include "hsk_pwc.h"

#include <string.h> /* memset() */

#include "../hsk_isr/hsk_isr.h"

/**
 * The number of available PWC channels.
 */
#define PWC_CHANNELS		4

/**
 * The size of a PWC ring buffer.
 *
 * This must not be greater than 32 or the calculation of values returned
 * by hsk_pwc_channel_getValue() might overflow.
 *
 * The value 8 should be a sensible compromise between an interest to get
 * averages from a sufficient number of values and memory use.
 */
#define CHAN_BUF_SIZE		8

/**
 * The prescaling factor.
 */
ubyte xdata hsk_pwc_prescaler;

/**
 * A CCT overflow counter.
 */
volatile ubyte pdata hsk_pwc_overflow;

/** \var hsk_pwc_channels
 * Processing data for PWC channels.
 */
volatile struct {
	/**
	 * The sum of the values stored in the ring buffer.
	 */
	ulong sum;

	/**
	 * A ring buffer of PWC values.
	 */
	uword buffer[CHAN_BUF_SIZE];

	/**
	 * The last captured value.
	 */
	uword lastCapture;

	/**
	 * The number of pulses to average over.
	 */
	ubyte averageOver;

	/**
	 * The current ring position.
	 */
	ubyte pos;

	/**
	 * The overflow count during the last capture.
	 *
	 * This is used by hsk_pwc_channel_getSum() to detect whether the capturing
	 * time window was left.
	 */
	ubyte overflow;

	/**
	 * This is an invalidation counter.
	 *
	 * Each time 0 is written into the buffer this is increased. Each
	 * time a valid value makes it in it's decreased, so that results
	 * are only output by getSum when all values are valid.
	 */
	ubyte invalid;
} pdata hsk_pwc_channels[PWC_CHANNELS];

/**
 * This is the common implementation of the Capture ISRs.
 *
 * @param channel
 *	The channel that was captured.
 * @param capture
 *	The value that was captured.
 * @private
 */
#pragma save
#ifdef SDCC
#pragma nooverlay
#endif
void hsk_pwc_isr_ccn(const hsk_pwc_channel idata channel, uword idata capture) using 1 {
	#define channel	hsk_pwc_channels[channel]
	/* Get the new value and store the current capture value for next
	 * time. */
	capture -= channel.lastCapture;
	channel.lastCapture += capture;
	/* Update the sum and buffer. */
	channel.sum -= channel.buffer[channel.pos];
	channel.buffer[channel.pos++] = capture;
	channel.pos %= channel.averageOver;
	channel.sum += capture;
	/* Update the overflow count.. */
	channel.overflow = hsk_pwc_overflow;
	/* Update the invalidation count. */
	if (channel.invalid) {
		channel.invalid--;
	}
	#undef channel
}

/**
 * The ISR for Capture events on channel PWC_CC0.
 *
 * @private
 */
void hsk_pwc_isr_cc0(void) using 1 {
	SFR_PAGE(_t2_2, SST1);
	hsk_pwc_isr_ccn(0, T2CCU_CC0LH);
	SFR_PAGE(_t2_2, RST1);
}


/**
 * The ISR for Capture events on channel PWC_CC1.
 *
 * @private
 */
void hsk_pwc_isr_cc1(void) using 1 {
	SFR_PAGE(_t2_2, SST1);
	hsk_pwc_isr_ccn(1, T2CCU_CC1LH);
	SFR_PAGE(_t2_2, RST1);
}


/**
 * The ISR for Capture events on channel PWC_CC2.
 *
 * @private
 */
void hsk_pwc_isr_cc2(void) using 1 {
	SFR_PAGE(_t2_2, SST1);
	hsk_pwc_isr_ccn(2, T2CCU_CC2LH);
	SFR_PAGE(_t2_2, RST1);
}


/**
 * The ISR for Capture events on channel PWC_CC3.
 *
 * @private
 */
void hsk_pwc_isr_cc3(void) using 1 {
	SFR_PAGE(_t2_3, SST1);
	hsk_pwc_isr_ccn(3, T2CCU_CC3LH);
	SFR_PAGE(_t2_3, RST1);
}


/**
 * The ISR for Capture/Compare overflow events.
 *
 * It simply increases hsk_pwc_overflow, which is used by hsk_pwc_channel_getSum() to
 * check whether the capture time window was left.
 *
 * @private
 */
void hsk_pwc_isr_cctOverflow(void) using 1 {
	hsk_pwc_overflow++;
}
#pragma restore

/**
 * CR_MISC Timer 2 Capture/Compare Unit Clock Configuration bit.
 */
#define BIT_T2CCFG		4

/**
 * T2CCU_CCTCON Capture/Compare Timer Start/Stop Control bit.
 */
#define BIT_CCTST		0

/**
 * T2CCU_CCTCON Enable synchronized Timer Starts.
 */
#define BIT_TIMSYN		1

/**
 * T2CCU_CCTCON Capture/Compare Timer Overflow Interrupt Enable bit.
 */
#define BIT_CCTOVEN		2

/**
 * T2CCU_CCTCON Capture/Compare Timer Overflow Flag bit.
 */
#define BIT_CCTOVF		3

/**
 * T2CCU_CCTCON T2CCU Capture/Compare Timer Control Register bits.
 */
#define BIT_CCTPRE		4

/**
 * T2CCU_CCTBSEL Channel x Time Base Select bit.
 */
#define BIT_CCTBx		0

/**
 * SYSCON0 Interrupt Structure 2 Mode Select bit.
 */
#define BIT_IMODE		4

void hsk_pwc_init(ulong idata window) {
	/* The prescaler in powers of 2. */
	hsk_pwc_prescaler = 0;

	/*
	 * Get the lowest prescaler that delivers the desired window time.
	 * 
	 * I.e. change the window time base to clock cycles and adjust it to
	 * a new prescaler until it fits into 2^16 - 1 timer cycles.
	 */
	window *= 48000;
	for (; hsk_pwc_prescaler < 12 && window >= (1ul << 16); \
		hsk_pwc_prescaler++, window >>= 1);

	/*
	 * Set the prescaler.
	 */
	SFR_PAGE(_su1, noSST);
	/* Set T2CCUCLK to FCLK or PCLK. */
	if (hsk_pwc_prescaler) {
		/* PCLK */
		CR_MISC &= ~(1 << BIT_T2CCFG);
	} else {
		/* FCLK */
		CR_MISC |= 1 << BIT_T2CCFG;
	}
	SFR_PAGE(_su0, noSST);

	SFR_PAGE(_t2_1, noSST);
	/* Set the internal prescaler. */
	T2CCU_CCTCON = (hsk_pwc_prescaler ? hsk_pwc_prescaler - 1 : 0) << BIT_CCTPRE;

	/* 
	 * Start the Capture/Compare Timer and the overflow interrupt.
	 */
	T2CCU_CCTCON |= (1 << BIT_CCTOVEN) | (1 << BIT_CCTST);
	SFR_PAGE(_t2_0, noSST);

	/*
	 * Enable the interrupts.
	 */
	/* Set IMODE, 1 so that EXM can be used to mask interrupts without
	 * loosing them. */
	SYSCON0 |= 1 << BIT_IMODE;
	/* CCT channel interrupt. */
	EXM = 1;
	/* CCT Overflow interrupt. */
	hsk_isr5.CCTOVF = &hsk_pwc_isr_cctOverflow;
	ET2 = 1;
}

/**
 * T2CCU_CCEN Capture/Compare Enable bits start.
 */
#define BIT_CCM0		0

/**
 * CCMx bit count.
 */
#define CNT_CCMx		2

/**
 * Default to using both edges for pulse detection.
 */
#define EDGE_DEFAULT_MODE	PWC_EDGE_BOTH

void hsk_pwc_channel_open(const hsk_pwc_channel idata channel,
		ubyte idata averageOver) {
	/*
	 * Set up channel information.
	 */
	if (averageOver < 1 || averageOver > CHAN_BUF_SIZE) {
		averageOver = 1;
	}
	memset(&hsk_pwc_channels[channel], 0, sizeof(hsk_pwc_channels[channel]));
	hsk_pwc_channels[channel].averageOver = averageOver;
	hsk_pwc_channels[channel].invalid = averageOver + 1;

	/**
	 * Set the PWC capture mode.
	 */
	hsk_pwc_channel_captureMode(channel, PWC_MODE_SOFT);

	/*
	 * Register the interrupt handler and select the Capture Compare
	 * Timer as capture source.
	 */
	SFR_PAGE(_t2_1, noSST);
	switch (channel) {
	case PWC_CC0:
		hsk_isr9.EXINT3 = &hsk_pwc_isr_cc0;
		T2CCU_CCTBSEL |= 1 << (BIT_CCTBx + 0);
		break;
	case PWC_CC1:
		hsk_isr9.EXINT4 = &hsk_pwc_isr_cc1;
		T2CCU_CCTBSEL |= 1 << (BIT_CCTBx + 1);
		break;
	case PWC_CC2:
		hsk_isr9.EXINT5 = &hsk_pwc_isr_cc2;
		T2CCU_CCTBSEL |= 1 << (BIT_CCTBx + 2);
		break;
	case PWC_CC3:
		hsk_isr9.EXINT6 = &hsk_pwc_isr_cc3;
		T2CCU_CCTBSEL |= 1 << (BIT_CCTBx + 3);
		break;
	}
	SFR_PAGE(_t2_0, noSST);

	/**
	 * Set the interrupt triggering edge.
	 */
	hsk_pwc_channel_edgeMode(channel, EDGE_DEFAULT_MODE);
}

/** \var hsk_pwc_ports
 * External input configuration structure.
 */
const struct {
	/**
	 * The input port configuration bit position.
	 */
	ubyte portBit;

	/**
	 * The input port configuration bits to select.
	 */
	ubyte portSel;

	/**
	 * The external interrupt configuration bit position.
	 */
	ubyte inBit;

	/**
	 * The external interrupt configuration to select.
	 */
	ubyte inSel;

	/**
	 * The external interrupt configuration bit count.
	 */
	ubyte inCount;
} code hsk_pwc_ports[] = {
	/* PWC_CC0_P30 */ {0, 3, 0, 2, 2},
	/* PWC_CC0_P40 */ {0, 4, 0, 1, 2},
	/* PWC_CC0_P55 */ {5, 2, 0, 3, 2},
	/* PWC_CC1_P32 */ {2, 5, 2, 2, 2},
	/* PWC_CC1_P41 */ {1, 1, 2, 1, 2},
	/* PWC_CC1_P56 */ {6, 2, 2, 3, 2},
	/* PWC_CC2_P33 */ {3, 1, 4, 2, 2},
	/* PWC_CC2_P44 */ {4, 3, 4, 1, 2},
	/* PWC_CC2_P52 */ {2, 2, 4, 3, 2},
	/* PWC_CC3_P34 */ {4, 4, 5, 3, 3},
	/* PWC_CC3_P45 */ {5, 3, 5, 2, 3},
	/* PWC_CC3_P57 */ {7, 3, 5, 4, 3}
};

void hsk_pwc_port_open(const hsk_pwc_port idata port, 
		ubyte idata averageOver) {
	hsk_pwc_channel channel;

	/*
	 * Select the channel connected to the port.
	 */
	switch(port) {
	case PWC_CC0_P30:
	case PWC_CC0_P40:
	case PWC_CC0_P55:
		channel = PWC_CC0;
		break;
	case PWC_CC1_P32:
	case PWC_CC1_P41:
	case PWC_CC1_P56:
		channel = PWC_CC1;
		break;
	case PWC_CC2_P33:
	case PWC_CC2_P44:
	case PWC_CC2_P52:
		channel = PWC_CC2;
		break;
	case PWC_CC3_P34:
	case PWC_CC3_P45:
	case PWC_CC3_P57:
		channel = PWC_CC3;
		break;
	default:
		channel = PWC_CC0;
		break;
	}

	/* Open the channel. */
	hsk_pwc_channel_open(channel, averageOver);
	/* Set up the channel for external interrupt input. */
	hsk_pwc_channel_captureMode(channel, PWC_MODE_EXT);

	#define portBit	hsk_pwc_ports[port].portBit
	#define portSel	hsk_pwc_ports[port].portSel

	/*
	 * Configure input pins.
	 */
	SFR_PAGE(_pp2, noSST);
	switch (port) {
	case PWC_CC0_P30:
	case PWC_CC1_P32:
	case PWC_CC2_P33:
	case PWC_CC3_P34:
		P3_ALTSEL0 = P3_ALTSEL0 & ~(1 << portBit) | ((portSel & 1) << portBit);
		P3_ALTSEL1 = P3_ALTSEL1 & ~(1 << portBit) | (((portSel >> 1) & 1) << portBit);
		SFR_PAGE(_pp0, noSST);
		P3_DIR &= ~(1 << portBit);
		break;
	case PWC_CC0_P40:
	case PWC_CC1_P41:
	case PWC_CC2_P44:
	case PWC_CC3_P45:
		P4_ALTSEL0 = P4_ALTSEL0 & ~(1 << portBit) | ((portSel & 1) << portBit);
		P4_ALTSEL1 = P4_ALTSEL1 & ~(1 << portBit) | (((portSel >> 1) & 1) << portBit);
		SFR_PAGE(_pp0, noSST);
		P4_DIR &= ~(1 << portBit);
		break;
	case PWC_CC0_P55:
	case PWC_CC1_P56:
	case PWC_CC2_P52:
	case PWC_CC3_P57:
		P5_ALTSEL0 = P5_ALTSEL0 & ~(1 << portBit) | ((portSel & 1) << portBit);
		P5_ALTSEL1 = P5_ALTSEL1 & ~(1 << portBit) | (((portSel >> 1) & 1) << portBit);
		SFR_PAGE(_pp0, noSST);
		P5_DIR &= ~(1 << portBit);
		break;
	}

	#undef portBit
	#undef portSel

	#define inBit	hsk_pwc_ports[port].inBit
	#define inSel	hsk_pwc_ports[port].inSel
	#define inCount	hsk_pwc_ports[port].inCount

	/*
	 * Configure Peripheral Input Selection for external interrupts.
	 */
	SFR_PAGE(_su3, noSST);
	switch (port) {
	case PWC_CC3_P34:
	case PWC_CC3_P45:
	case PWC_CC3_P57:
		MODPISEL1 = MODPISEL1 & ~(((1 << inCount) - 1) << inBit) | (inSel << inBit);
		break;
	case PWC_CC0_P30:
	case PWC_CC0_P40:
	case PWC_CC0_P55:
	case PWC_CC1_P32:
	case PWC_CC1_P41:
	case PWC_CC1_P56:
	case PWC_CC2_P33:
	case PWC_CC2_P44:
	case PWC_CC2_P52:
		MODPISEL4 = MODPISEL4 & ~(((1 << inCount) - 1) << inBit) | (inSel << inBit);
		break;
	}
	SFR_PAGE(_su0, noSST);

	#undef inBit
	#undef inSel
	#undef inCount
}

void hsk_pwc_channel_close(const hsk_pwc_channel idata channel) {
	/*
	 * Deactivate the channel.
	 */
	SFR_PAGE(_t2_1, noSST);
	T2CCU_CCEN = T2CCU_CCEN & ~(((1 << CNT_CCMx) - 1) << ((channel * CNT_CCMx) + BIT_CCM0));
	SFR_PAGE(_t2_0, noSST);
}

/**
 * EXICONn EXINTx mode bit count.
 */
#define CNT_EXINTx		2

/**
 * External Interrupt Control Register for setting the PWC_CC0 edge detection
 * mode.
 */
#define PWC_CC0_EXINT_REG	EXICON0

/**
 * The edge detection mode bit position for PWC_CC0.
 */
#define PWC_CC0_EXINT_BIT	6

/**
 * External Interrupt Control Register for setting the PWC_CC1 edge detection
 * mode.
 */
#define PWC_CC1_EXINT_REG	EXICON1

/**
 * The edge detection mode bit position for PWC_CC1.
 */
#define PWC_CC1_EXINT_BIT	0

/**
 * External Interrupt Control Register for setting the PWC_CC2 edge detection
 * mode.
 */
#define PWC_CC2_EXINT_REG	EXICON1

/**
 * The edge detection mode bit position for PWC_CC2.
 */
#define PWC_CC2_EXINT_BIT	2

/**
 * External Interrupt Control Register for setting the PWC_CC3 edge detection
 * mode.
 */
#define PWC_CC3_EXINT_REG	EXICON1

/**
 * The edge detection mode bit position for PWC_CC3.
 */
#define PWC_CC3_EXINT_BIT	4

void hsk_pwc_channel_edgeMode(const hsk_pwc_channel idata channel,
		const ubyte idata edgeMode) {
	/*
	 * Configure the corresponding external interrupt to trigger with
	 * the desired edge.
	 */
	SFR_PAGE(_t2_1, noSST);
	switch (channel) {
	case PWC_CC0:
		PWC_CC0_EXINT_REG = PWC_CC0_EXINT_REG & ~(((1 << CNT_EXINTx) - 1) << PWC_CC0_EXINT_BIT) | (edgeMode << PWC_CC0_EXINT_BIT);
		break;
	case PWC_CC1:
		PWC_CC1_EXINT_REG = PWC_CC1_EXINT_REG & ~(((1 << CNT_EXINTx) - 1) << PWC_CC1_EXINT_BIT) | (edgeMode << PWC_CC1_EXINT_BIT);
		break;
	case PWC_CC2:
		PWC_CC2_EXINT_REG = PWC_CC2_EXINT_REG & ~(((1 << CNT_EXINTx) - 1) << PWC_CC2_EXINT_BIT) | (edgeMode << PWC_CC2_EXINT_BIT);
		break;
	case PWC_CC3:
		PWC_CC3_EXINT_REG = PWC_CC3_EXINT_REG & ~(((1 << CNT_EXINTx) - 1) << PWC_CC3_EXINT_BIT) | (edgeMode << PWC_CC3_EXINT_BIT);
		break;
	}
	SFR_PAGE(_t2_0, noSST);	
}

void hsk_pwc_channel_captureMode(const hsk_pwc_channel idata channel,
		const ubyte idata captureMode) {
	/*
	 * Configure capture mode for the channel.
	 */
	SFR_PAGE(_t2_1, noSST);
	T2CCU_CCEN = T2CCU_CCEN & ~(((1 << CNT_CCMx) - 1) << ((channel * CNT_CCMx) + BIT_CCM0)) | (captureMode << ((channel * CNT_CCMx) + BIT_CCM0));
	SFR_PAGE(_t2_0, noSST);
}

void hsk_pwc_channel_trigger(const hsk_pwc_channel idata channel) {
	switch (channel) {
	case PWC_CC0:
		SFR_PAGE(_t2_2, noSST);
		T2CCU_CC0L = 0;
		hsk_pwc_isr_ccn(0, T2CCU_CC0LH);
		break;
	case PWC_CC1:
		SFR_PAGE(_t2_2, noSST);
		T2CCU_CC1L = 0;
		hsk_pwc_isr_ccn(1, T2CCU_CC1LH);
		break;
	case PWC_CC2:
		SFR_PAGE(_t2_2, noSST);
		T2CCU_CC2L = 0;
		hsk_pwc_isr_ccn(2, T2CCU_CC2LH);
		break;
	case PWC_CC3:
		SFR_PAGE(_t2_3, noSST);
		T2CCU_CC3L = 0;
		hsk_pwc_isr_ccn(3, T2CCU_CC3LH);
		break;
	}
	SFR_PAGE(_t2_0, noSST);
}

/**
 * PMCON1 T2CCU Disable Request bit. 
 */
#define BIT_T2CCU_DIS		3

void hsk_pwc_enable(void) {
	/* Enable clock. */
	SFR_PAGE(_su1, noSST);
	PMCON1 &= ~(1 << BIT_T2CCU_DIS);
	SFR_PAGE(_su0, noSST);
}

void hsk_pwc_disable(void) {
	/* Enable clock. */
	SFR_PAGE(_su1, noSST);
	PMCON1 |= 1 << BIT_T2CCU_DIS;
	SFR_PAGE(_su0, noSST);
}

ulong hsk_pwc_channel_getValue(const hsk_pwc_channel idata channel, \
		const ubyte idata unit) {
	#define channel	hsk_pwc_channels[channel]
	bool exm = EXM;
	bool et2 = ET2;
	bool ea = EA;
	ulong age;

	/* Get the current timer data, make this quick. */
	SFR_PAGE(_t2_1, noSST);
	EA = 0;
	EXM = 0;
	ET2 = 0;
	EA = ea;
	/* Get the age of the last capture event. */
	age = (ulong)((ubyte)(hsk_pwc_overflow - channel.overflow)) << 16;
	age |= T2CCU_CCTLH;
	age -= channel.lastCapture;
	EA = 0;
	EXM = exm;
	ET2 = et2;
	EA = ea;
	SFR_PAGE(_t2_0, noSST);
	/* Check whether the window time frame has been left. */
	if (age >= (1ul << 16)) {
		channel.invalid = channel.averageOver + 1;
	}
	/* Return 0 for invalid channels. */
	if (channel.invalid) {
		return 0;
	}

	/*
	 * Return the buffered values in the requested format.
	 */
	switch(unit) {
	case PWC_UNIT_SUM_RAW:
		return channel.sum << hsk_pwc_prescaler;
		break;
	case PWC_UNIT_WIDTH_RAW:
		return (channel.sum << hsk_pwc_prescaler) \
			/ channel.averageOver;
		break;
	case PWC_UNIT_WIDTH_NS:
		return (channel.sum << hsk_pwc_prescaler) \
			* 250 / 12 / channel.averageOver;
		break;
	case PWC_UNIT_WIDTH_US:
		return (channel.sum << hsk_pwc_prescaler) \
			/ 48 / channel.averageOver;
		break;
	case PWC_UNIT_WIDTH_MS:
		return (channel.sum << hsk_pwc_prescaler) \
			/ 48000 / channel.averageOver;
		break;
	case PWC_UNIT_FREQ_S:
		return (48000000ul * channel.averageOver \
			/ channel.sum) >> hsk_pwc_prescaler;
		break;
	case PWC_UNIT_FREQ_M:
		return ((48000000ul * 60) >> hsk_pwc_prescaler) \
			/ channel.sum * channel.averageOver;
		break;
	case PWC_UNIT_FREQ_H:
		return ((48000000ul * 60) >> hsk_pwc_prescaler) \
			/ channel.sum * 60 * channel.averageOver;
		break;
	}
	return 0;
	#undef channel
}

