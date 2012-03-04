/** \file
 * HSK Pulse Width Counter implementation
 *
 * The Pulse Width Conter (PWC) module uses the T2CCU Capture/Compare Timer
 * (CCT) to measure pulse width.
 *
 * @author kami
 * @version 2012-02-12
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
 * The size of a channel ring buffer in \f$2^n\f$.
 *
 * This must not be greater than 5 or the calculation of pulse times might
 * overflow.
 */
#define CHAN_BUF_DEPTH		4

/**
 * The size of a PWC ring buffer.
 */
#define CHAN_BUF_SIZE		(1 << CHAN_BUF_DEPTH)

/**
 * The window time in timer cycles that should be observed.
 */
volatile uword idata hsk_pwc_window;

/**
 * The prescaling factor.
 */
volatile uword idata hsk_pwc_prescaler;

/**
 * A CCT overflow counter.
 */
volatile ubyte idata hsk_pwc_overflow;

/**
 * Processing data for PWC channels.
 */
struct hsk_pwc_channel_data {
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
};

/**
 * PWC channel data.
 */
volatile struct hsk_pwc_channel_data xdata hsk_pwc_channels[PWC_CHANNELS];

/**
 * This is the common implementation of the Capture ISRs.
 *
 * @param channel
 *	The channel that was captured.
 * @param capture
 *	The value that was captured.
 */
#pragma save
#pragma nooverlay
void hsk_pwc_isr_ccn(hsk_pwc_channel idata channel, uword idata capture) {
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
 */
void hsk_pwc_isr_cc0(void) {
	SFR_PAGE(_t2_2, SST1);
	hsk_pwc_isr_ccn(0, T2CCU_CC0LH);
	SFR_PAGE(_t2_2, RST1);
}


/**
 * The ISR for Capture events on channel PWC_CC1.
 */
void hsk_pwc_isr_cc1(void) {
	SFR_PAGE(_t2_2, SST1);
	hsk_pwc_isr_ccn(1, T2CCU_CC1LH);
	SFR_PAGE(_t2_2, RST1);
}


/**
 * The ISR for Capture events on channel PWC_CC2.
 */
void hsk_pwc_isr_cc2(void) {
	SFR_PAGE(_t2_2, SST1);
	hsk_pwc_isr_ccn(2, T2CCU_CC2LH);
	SFR_PAGE(_t2_2, RST1);
}


/**
 * The ISR for Capture events on channel PWC_CC3.
 */
void hsk_pwc_isr_cc3(void) {
	SFR_PAGE(_t2_3, SST1);
	hsk_pwc_isr_ccn(3, T2CCU_CC3LH);
	SFR_PAGE(_t2_3, RST1);
}


/**
 * The ISR for Capture/Compare overflow events.
 *
 * It simply increases hsk_pwc_overflow, which is used by hsk_pwc_channel_getSum() to
 * check whether the capture time window was left.
 */
void hsk_pwc_isr_cctOverflow(void) {
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

/**
 * This function initializes the T2CCU Capture/Compare Unit for capture mode.
 *
 * The capturing is based on the CCT timer. Timer T2 is not used and thus can
 * be used by other modules without interference.
 *
 * The window time is the time frame within which pulses should be detected.
 * A smaller time frame results in higher precission, but detection of longer
 * pulses will fail.
 * 
 * Window times vary between ~1ms (\f$(2^{16} - 1) / (48 * 10^6)\f$) and ~5592ms
 * (\f$(2^{16} - 1) * 2^{12} / (48 * 10^6)\f$). The shortest window time delivers
 * ~20ns and the longest time ~85µs precision.
 *
 * The real window time is on a logarithmic scale (base 2), the init function
 * will select the lowest scale that guarantees the required window time.
 * I.e. the highest precision possible with the desired window time, which is
 * at least \f$2^{15}\f$ for all windows below or equal 5592ms.
 *
 * @param window
 *	The time in ms to detect a pulse.
 */
void hsk_pwc_init(ulong idata window) {
	/* The prescaler in powers of 2. */
	ubyte prescaler = 0;

	/*
	 * Get the lowest prescaler that delivers the desired window time.
	 * 
	 * I.e. change the window time base to clock cycles and adjust it to
	 * a new prescaler until it fits into 2^16 - 1 timer cycles.
	 */
	window *= 48000;
	for (; prescaler < 12 && window >= (1ul << 16); prescaler++, window >>= 1);

	/*
	 * Store the timer specs.
	 */
	hsk_pwc_window = window;
	hsk_pwc_prescaler = prescaler;

	/*
	 * Set the prescaler.
	 */
	SFR_PAGE(_su1, noSST);
	/* Set T2CCUCLK to FCLK or PCLK. */
	if (prescaler) {
		/* PCLK */
		prescaler--;
		CR_MISC &= ~(1 << BIT_T2CCFG);
	} else {
		/* FCLK */
		CR_MISC |= 1 << BIT_T2CCFG;
	}
	SFR_PAGE(_su0, noSST);

	SFR_PAGE(_t2_1, noSST);
	/* Set the internal prescaler. */
	T2CCU_CCTCON = prescaler << BIT_CCTPRE;

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

/**
 * Configures a PWC channel without an input port.
 *
 * The channel is set up for software triggering (PWC_MODE_SOFT), and
 * triggering on both edges (PWC_EDGE_BOTH).
 *
 * @param channel
 *	The PWC channel to open
 * @param averageOver
 * 	The number of pulse values to average over when returning a
 * 	value or speed. The value must be between 1 and CHAN_BUF_SIZE.
 */
void hsk_pwc_channel_open(hsk_pwc_channel idata channel, ubyte idata averageOver) {
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

/**
 * External input configuration structure.
 */
struct hsk_pwc_port_conf {
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
};

/**
 * A table of input port configurations.
 */
const struct hsk_pwc_port_conf code hsk_pwc_ports[] = {
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

/**
 * Opens an input port and the connected channel.
 *
 * The available configurations are available from the PWC_CCn_* defines.
 *
 * @param port
 * 	The input port to open.
 * @param averageOver
 * 	The number of pulse values to average over when returning a
 * 	value or speed. The value must be between 1 and CHAN_BUF_SIZE.
 */			    
void hsk_pwc_port_open(hsk_pwc_port idata port, ubyte idata averageOver) {
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

/**
 * Close a PWC channel.
 *
 * @param channel
 * 	The channel to close.
 */
void hsk_pwc_channel_close(hsk_pwc_channel idata channel) {
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

/**
 * Select the edge that is used to detect a pulse.
 *
 * Available edges are specified in the PWC_EDGE_* defines.
 *
 * @param channel
 *	The channel to configure the edge for.
 * @param edgeMode
 * 	The selected edge detection mode.
 */
void hsk_pwc_channel_edgeMode(hsk_pwc_channel idata channel, ubyte idata edgeMode) {
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

/**
 * Allows switching between external and soft trigger.
 *
 * This does not reconfigure the input ports. Available modes are specified
 * in the PWC_MODE_* defines. PWC_MODE_EXT is the default.
 *
 * @param channel
 * 	The channel to configure.
 * @param captureMode
 * 	The mode to set the channel to.
 */
void hsk_pwc_channel_captureMode(hsk_pwc_channel idata channel, ubyte idata captureMode) {
	/*
	 * Configure capture mode for the channel.
	 */
	SFR_PAGE(_t2_1, noSST);
	T2CCU_CCEN = T2CCU_CCEN & ~(((1 << CNT_CCMx) - 1) << ((channel * CNT_CCMx) + BIT_CCM0)) | (captureMode << ((channel * CNT_CCMx) + BIT_CCM0));
	SFR_PAGE(_t2_0, noSST);
}

/**
 * Triggers a channel in soft trigger mode.
 * 
 * @param channel
 * 	The channel to trigger.
 */
void hsk_pwc_channel_trigger(hsk_pwc_channel idata channel) {
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

/**
 * Enables T2CCU module if disabled.
 */
void hsk_pwc_enable(void) {
	/* Enable clock. */
	SFR_PAGE(_su1, noSST);
	PMCON1 &= ~(1 << BIT_T2CCU_DIS);
	SFR_PAGE(_su0, noSST);
}

/**
 * Turns off the T2CCU clock to preserve power.
 */
void hsk_pwc_disable(void) {
	/* Enable clock. */
	SFR_PAGE(_su1, noSST);
	PMCON1 |= 1 << BIT_T2CCU_DIS;
	SFR_PAGE(_su0, noSST);
}

/**
 * Returns the current sum of values in a channel buffer.
 *
 * @param channel
 * 	The channel to return the buffer sum of.
 * @return
 * 	The sum of buffered capture results for this channel.
 */
ulong hsk_pwc_channel_getSum(hsk_pwc_channel idata channel) {
	#define channel	hsk_pwc_channels[channel]
	ubyte exm = EXM;
	ubyte overflow;
	long capture;


	/* Get the current timer data, make this quick. */
	SFR_PAGE(_t2_1, noSST);
	EXM = 0;
	/* Get the last capture interval. */
	capture = T2CCU_CCTLH;
	capture -= channel.lastCapture;
	/* Get the overflow. */
	overflow = hsk_pwc_overflow - channel.overflow;
	EXM = exm;
	SFR_PAGE(_t2_0, noSST);
	/* Check whether the window time frame has been left. */
	if ((ulong)hsk_pwc_window < ((ulong)overflow << 16) + capture) {
		channel.invalid = channel.averageOver + 1;
	}
	if (channel.invalid) {
		return 0;
	} else {
		return channel.sum;
	}
	#undef channel
}

/**
 * Returns the measured pulse width in FCLK cycles.
 * I.e. 1/48µs.
 *
 * @param channel
 * 	The channel to return the pulse width for.
 * @return
 *	The measured pulse width.
 */
ulong hsk_pwc_channel_getWidthFclk(hsk_pwc_channel idata channel) {
	ulong sum = hsk_pwc_channel_getSum(channel);
	return (sum << hsk_pwc_prescaler) / hsk_pwc_channels[channel].averageOver;
}

/**
 * Returns the measured pulse width in ns.
 *
 * @param channel
 * 	The channel to return the pulse width for.
 * @return
 *	The measured pulse width.
 */
ulong hsk_pwc_channel_getWidthNs(hsk_pwc_channel idata channel) {
	ulong sum = hsk_pwc_channel_getSum(channel);
	return (sum << hsk_pwc_prescaler) * 1000 / 48 / hsk_pwc_channels[channel].averageOver;
}

/**
 * Returns the measured pulse width in µs.
 *
 * @param channel
 * 	The channel to return the pulse width for.
 * @return
 *	The measured pulse width.
 */
ulong hsk_pwc_channel_getWidthUs(hsk_pwc_channel idata channel) {
	ulong sum = hsk_pwc_channel_getSum(channel);
	return (sum << hsk_pwc_prescaler) / 48 / hsk_pwc_channels[channel].averageOver;
}

/**
 * Returns the measured pulse width in ms.
 *
 * @param channel
 * 	The channel to return the pulse width for.
 * @return
 *	The measured pulse width.
 */
uword hsk_pwc_channel_getWidthMs(hsk_pwc_channel idata channel) {
	ulong sum = hsk_pwc_channel_getSum(channel);
	return ((sum << hsk_pwc_prescaler) / 48000 / hsk_pwc_channels[channel].averageOver);
}


/**
 * Returns the measured frequency in Hz.
 * 
 * @param channel
 * 	The channel to return the frequency from.
 * @return
 *	The measured frequency or 0 if none was measured.
 */
ulong hsk_pwc_channel_getFreqHz(hsk_pwc_channel idata channel) {
	ulong sum = hsk_pwc_channel_getSum(channel);
	if (sum) {
		return ((ulong)hsk_pwc_channels[channel].averageOver * 48000000ul / sum) >> hsk_pwc_prescaler;
	}
	return 0;
}

/**
 * Returns the measured frequency in pulses per minute.
 * 
 * @param channel
 * 	The channel to return the frequency from.
 * @return
 *	The measured frequency or 0 if none was measured.
 */
ulong hsk_pwc_channel_getFreqPpm(hsk_pwc_channel idata channel) {
	ulong sum = hsk_pwc_channel_getSum(channel);
	if (sum) {
		return (hsk_pwc_channels[channel].averageOver * 48000000 * 60 / sum) >> hsk_pwc_prescaler;
	}
	return 0;
}

/**
 * Returns the measured frequency in pulses per hour.
 * 
 * @param channel
 * 	The channel to return the frequency from.
 * @return
 *	The measured frequency or 0 if none was measured.
 */
ulong hsk_pwc_channel_getFreqPph(hsk_pwc_channel idata channel) {
	ulong sum = hsk_pwc_channel_getSum(channel);
	if (sum) {
		return hsk_pwc_channels[channel].averageOver * (48000000ul >> hsk_pwc_prescaler) * 3600 / sum;
	}
	return 0;
}
