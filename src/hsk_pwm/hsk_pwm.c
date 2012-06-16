/** \file
 * HSK Pulse Width Modulation implementation
 *
 * This would mostly be straightforward if it wasn't for the messy output
 * channel configuration.
 *
 * The init function buys a lot of simplicity by limiting the CCU6 use to
 * generating PWM. Also, the channels PWM_60, PWM_61 and PWM_62 operate at
 * the same base frequency and period. This is a hardware limitation.
 *
 * @author kami
 */

#include <Infineon/XC878.h>

#include "hsk_pwm.h"

/**
 * CR_MISC CCU6 Clock Configuration bit.
 */
#define BIT_CCUCCFG	5

/**
 * CCU6_TCTR0L/CCU6_TCTR0H Timer T12/T13 Input Clock Select and Prescaler bits.
 */
#define BIT_TnCLK	0

/**
 * TnCLK bit count.
 */
#define CNT_TnCLK	4

/**
 * PSLR Compare Outputs Passive State Level bits.
 */
#define BIT_PSL		0

/**
 * PSL bit count.
 */
#define CNT_PSL		6

/**
 * PSLR Passive State Level of Output COUT63 bit.
 */
#define BIT_PSL63	7

/**
 * CCU6_MODCTRL/CCU6_MODCTRH T12/T13 Modulation Enable bits.
 */
#define BIT_TnMODEN	0

/**
 * TnMODEN bit count.
 */
#define CNT_TnMODEN	6

/**
 * CCU6_MODCTRH Enable Compare Timer T13 Output bits.
 */
#define BIT_ECT13O	7

/**
 * T12MSELL/H Capture/Compare Mode Selection width.
 */
#define CNT_MSEL6n	4

/**
 * T12MSELL/H Capture/Compare Mode Selection mode.
 *
 * This mode means CC6n and COUT6n are in output mode.
 */
#define MOD_MSEL6n	0x3

/**
 * CCU6_TCTR4L/CCU6_TCTR4H Timer T12/T13 Shadow Transfer Request bit.
 */
#define BIT_TnSTR	6

void hsk_pwm_init(const hsk_pwm_channel idata channel, const ulong idata freq) {
	/**
	 * <b>PWM Timings</b>
	 *
	 * The CCU6CLK can run at FCLK (48MHz) or PCLK (24MHz), configured in
	 * the CCUCCFG bit. This implementation always uses 48MHz.
	 *
	 * The T12CLK can run any power of two between CCU6CLK and
	 * CCU6CLK/128, configured in the T12CLK bit field.
	 *
	 * This value can additionally be multiplied with a prescaler of 1/256,
	 * activated with the T12PRE bit.
	 *
	 * The same is true for the T13CLK.
	 *
	 * Additionally the period is length for T12 and T13 can be configured
	 * to any 16 bit value. Assuming at least 1/1000 precision is
	 * desired that means the clock cycle can be shortened by any factor
	 * up to 2^6 (64).
	 *
	 * The conclusion is that PWM frequencies between 48kHz and ~0.02Hz can
	 * be configured. Very high values degrade the precision, e.g. 96kHz
	 * will only offer 1/500 precision.
	 * The freq value 0 will result in ~0.02Hz (\f$48000000 / 2^{31}\f$).
	 */

	/* The clock prescaller in powers of 2. */
	ubyte prescaler = 0;
	/* The period required to reach the frequency with the current
	 * prescaler. The highest value below or equal 2^16 offers the
	 * highest precision. */
	ulong period = 480000000ul / (freq > 0 ? freq : 1ul);

	/* Special case, get the slowest frequency possible. */
	if (freq == 0) {
		period = 1ul << 16;
		prescaler = 15;
	}

	/*
	 * All factors considered the clock can be divided by up to 2^15.
	 * This loop calculates the smallest division factor that can be
	 * used.
	 */
	for (; period >= (1ul << 16) && prescaler <= 15; period = (480000000ul >> ++prescaler) / freq);

	/*
	 * Set CCU6CLK to FCLK.
	 */
	SFR_PAGE(_su1, noSST);
	CR_MISC |= 1 << BIT_CCUCCFG;
	SFR_PAGE(_su0, noSST);

	/*
	 * Set up the selected channel.
	 */
	SFR_PAGE(_cc1, noSST);
	switch (channel) {
	case PWM_60:
	case PWM_61:
	case PWM_62:
		/* Set the timer T12 prescaler. */
		CCU6_TCTR0L = (prescaler << BIT_TnCLK);

		/* Set the timer period. */
		CCU6_T12PRLH = period - 1;

		SFR_PAGE(_cc2, noSST);
		/*
		 * The output is active above the compare value. So to have
		 * positive duty cycle logic, output 1 on passive mode (i.e. a
		 * higher duty cycle/compare value results in a longer period
		 * of high level output).
		 */
		CCU6_PSLR |= ((1 << CNT_PSL) - 1) << BIT_PSL;

		/* Enable timer T12 output. */
		CCU6_MODCTRL |= ((1 << CNT_TnMODEN) - 1) << BIT_TnMODEN;

		/* Switch all the CC6n and COUT6n channels to output mode. */
		CCU6_T12MSELL = MOD_MSEL6n << CNT_MSEL6n | MOD_MSEL6n;
		CCU6_T12MSELH = CCU6_T12MSELH & ~((1 << CNT_MSEL6n) - 1) | MOD_MSEL6n;

		/*
		 * Make sure the PWM comes up clean by setting all duty cycles to 0.
		 */
		SFR_PAGE(_cc0, noSST);
		CCU6_CC60SRLH = 0;
		CCU6_CC61SRLH = 0;
		CCU6_CC62SRLH = 0;

		/* Request shadow transfer for T12 duty cycles. */
		CCU6_TCTR4L |= 1 << BIT_TnSTR;
		break;
	case PWM_63:
		/* Set the timer T13 prescaler. */
		CCU6_TCTR0H = (prescaler << BIT_TnCLK);

		/* Set the timer period. */
		CCU6_T13PRLH = period - 1;

		SFR_PAGE(_cc2, noSST);
		/*
		 * The output is active above the compare value. So to have
		 * positive duty cycle logic, output 1 on passive mode (i.e. a
		 * higher duty cycle/compare value results in a longer period
		 * of high level output).
		 */
		CCU6_PSLR |= 1 << BIT_PSL63;

		/* Enable timer T13 output. */
		CCU6_MODCTRH |= 1 << BIT_ECT13O;

		/*
		 * Make sure the PWM comes up clean by setting all duty cycles to 0.
		 */
		SFR_PAGE(_cc0, noSST);
		CCU6_CC63SRLH = 0;

		/* Request shadow transfer for the T13 duty cycle. */
		CCU6_TCTR4H |= 1 << BIT_TnSTR;
		break;
	}
}

/**
 * Data structure to hold output port configurations.
 */
const struct {
	/**
	 * The Pn_ALTSEL[01] bit position to make the port configuration in.
	 */
	ubyte pos;

	/**
	 * Select a 2 bits Pn_ALTSEL[01] configuration.
	 */
	ubyte sel;
} code hsk_pwm_ports[] = {
	/* PWM_OUT_60_P30 */ {0, 1},
	/* PWM_OUT_60_P31 */ {1, 1},
	/* PWM_OUT_60_P40 */ {0, 1},
	/* PWM_OUT_60_P41 */ {1, 1},
	/* PWM_OUT_61_P00 */ {0, 2},
	/* PWM_OUT_61_P01 */ {1, 2},
	/* PWM_OUT_61_P31 */ {1, 2},
	/* PWM_OUT_61_P32 */ {2, 1},
	/* PWM_OUT_61_P33 */ {3, 1},
	/* PWM_OUT_61_P44 */ {4, 1},
	/* PWM_OUT_61_P45 */ {5, 2},
	/* PWM_OUT_62_P04 */ {4, 2},
	/* PWM_OUT_62_P05 */ {5, 2},
	/* PWM_OUT_62_P34 */ {4, 1},
	/* PWM_OUT_62_P35 */ {5, 1},
	/* PWM_OUT_62_P46 */ {6, 1},
	/* PWM_OUT_62_P47 */ {7, 1},
	/* PWM_OUT_63_P03 */ {3, 2},
	/* PWM_OUT_63_P37 */ {7, 1},
	/* PWM_OUT_63_P43 */ {3, 2}
};

void hsk_pwm_port_open(const hsk_pwm_port idata port) {
	/*
	 * Warning, hard coded magic numbers.
	 * Check the "CCU6 I/O Control Selection" table.
	 */

	#define portBit hsk_pwm_ports[port].pos
	#define portSel hsk_pwm_ports[port].sel

	/* Activate CC6n/COUT6n output ports. */
	SFR_PAGE(_pp2, noSST);
	switch (port) {
	case PWM_OUT_61_P00:
	case PWM_OUT_61_P01:
	case PWM_OUT_62_P04:
	case PWM_OUT_62_P05:
	case PWM_OUT_63_P03:
		P0_ALTSEL0 = (P0_ALTSEL0 & ~(1 << portBit)) | ((portSel & 1) << portBit);
		P0_ALTSEL1 = (P0_ALTSEL1 & ~(1 << portBit)) | ((portSel >> 1) << portBit);
		SFR_PAGE(_pp0, noSST);
		P0_DIR |= 1 << portBit;
		break;
	case PWM_OUT_60_P30:
	case PWM_OUT_60_P31:
	case PWM_OUT_61_P31:
	case PWM_OUT_61_P32:
	case PWM_OUT_61_P33:
	case PWM_OUT_62_P34:
	case PWM_OUT_62_P35:
	case PWM_OUT_63_P37:
		P3_ALTSEL0 = (P3_ALTSEL0 & ~(1 << portBit)) | ((portSel & 1) << portBit);
		P3_ALTSEL1 = (P3_ALTSEL1 & ~(1 << portBit)) | ((portSel >> 1) << portBit);
		SFR_PAGE(_pp0, noSST);
		P3_DIR |= 1 << portBit;
		break;
	case PWM_OUT_60_P40:
	case PWM_OUT_60_P41:
	case PWM_OUT_61_P44:
	case PWM_OUT_61_P45:
	case PWM_OUT_62_P46:
	case PWM_OUT_62_P47:
	case PWM_OUT_63_P43:
		P4_ALTSEL0 = (P4_ALTSEL0 & ~(1 << portBit)) | ((portSel & 1) << portBit);
		P4_ALTSEL1 = (P4_ALTSEL1 & ~(1 << portBit)) | ((portSel >> 1) << portBit);
		SFR_PAGE(_pp0, noSST);
		P4_DIR |= 1 << portBit;
		break;
	#ifdef SDCC
	/*
	 * This is here for what appears to be a bug in SDCC.
	 * I suspect a problem with the jump table optimisation for switch
	 * statements, the disparate case number will result in less optimised
	 * code.
	 * Without this workaround multiple calls to the function lock up the
	 * controller. The first call always works. As far as I've been able to
	 * determine the end of the function is reached, but the line after
	 * the second function call never is.
	 */
	case 0xFF:
		SFR_PAGE(_pp0, noSST);
		break;
	#endif
	}

	#undef portBit
	#undef portSel
}

void hsk_pwm_port_close(const hsk_pwm_port idata port) {
	/*
	 * Warning, hard coded magic numbers.
	 * Check the "CCU6 I/O Control Selection" table and the T12MSEL
	 * register to understand what's going on.
	 */

	/*
	 * Note that the T12/T13 modes are not touched here, because turning an
	 * output channel off might affect still active port pins attached to
	 * the same output channel.
	 */

	#define portBit hsk_pwm_ports[port].pos
	#define portSel hsk_pwm_ports[port].sel

	/* Deactivate CC6n/COUT6n output ports. */
	SFR_PAGE(_pp2, noSST);
	switch (port) {
	case PWM_OUT_61_P00:
	case PWM_OUT_61_P01:
	case PWM_OUT_62_P04:
	case PWM_OUT_62_P05:
	case PWM_OUT_63_P03:
		P0_ALTSEL0 ^= (portSel & 1) << portBit;
		P0_ALTSEL1 ^= (portSel >> 1) << portBit;
		SFR_PAGE(_pp0, noSST);
		P0_DIR &= ~(1 << portBit);
		break;
	case PWM_OUT_60_P30:
	case PWM_OUT_60_P31:
	case PWM_OUT_61_P31:
	case PWM_OUT_61_P32:
	case PWM_OUT_61_P33:
	case PWM_OUT_62_P34:
	case PWM_OUT_62_P35:
	case PWM_OUT_63_P37:
		P3_ALTSEL0 ^= (portSel & 1) << portBit;
		P3_ALTSEL1 ^= (portSel >> 1) << portBit;
		SFR_PAGE(_pp0, noSST);
		P3_DIR &= ~(1 << portBit);
		break;
	case PWM_OUT_60_P40:
	case PWM_OUT_60_P41:
	case PWM_OUT_61_P44:
	case PWM_OUT_61_P45:
	case PWM_OUT_62_P46:
	case PWM_OUT_62_P47:
	case PWM_OUT_63_P43:
		P4_ALTSEL0 ^= (portSel & 1) << portBit;
		P4_ALTSEL1 ^= (portSel >> 1) << portBit;
		SFR_PAGE(_pp0, noSST);
		P4_DIR &= ~(1 << portBit);
		break;
	}

	#undef portBit
	#undef portSel
}

void hsk_pwm_channel_set(const hsk_pwm_channel idata channel,
		const uword idata max, const uword idata value) {
	ulong duty;

	/* Set the new cycle and request shadow transfer. */
	SFR_PAGE(_cc1, noSST);
	switch (channel) {
	case PWM_60:
		/* Calculate the duty cycle. */
		duty = (ulong)(CCU6_T12PRLH + 1) * value / max;
		/* Write the duty cycle. */
		SFR_PAGE(_cc0, noSST);
		CCU6_CC60SRLH = duty;
		CCU6_TCTR4L |= 1 << BIT_TnSTR;
		break;
	case PWM_61:
		/* Calculate the duty cycle. */
		duty = (ulong)(CCU6_T12PRLH + 1) * value / max;
		/* Write the duty cycle. */
		SFR_PAGE(_cc0, noSST);
		CCU6_CC61SRLH = duty;
		CCU6_TCTR4L |= 1 << BIT_TnSTR;
		break;
	case PWM_62:
		/* Calculate the duty cycle. */
		duty = (ulong)(CCU6_T12PRLH + 1) * value / max;
		/* Write the duty cycle. */
		SFR_PAGE(_cc0, noSST);
		CCU6_CC62SRLH = duty;
		CCU6_TCTR4L |= 1 << BIT_TnSTR;
		break;
	case PWM_63:
		/* Calculate the duty cycle. */
		duty = (ulong)(CCU6_T13PRLH + 1) * value / max;
		/* Write the duty cycle. */
		SFR_PAGE(_cc0, noSST);
		CCU6_CC63SRLH = duty;
		CCU6_TCTR4H |= 1 << BIT_TnSTR;
		break;
	}
}

void hsk_pwm_outChannel_dir(hsk_pwm_outChannel idata channel,
		const bool up) {
	/* The configuration bit for COUT63 is misplaced. */
	if (channel == PWM_COUT63) {
		channel = BIT_PSL63;
	}

	/* Change the direction bit in the PSLR. */ 
	SFR_PAGE(_cc2, noSST);
	CCU6_PSLR = CCU6_PSLR & ~(1 << channel) | ((ubyte)up << channel);
	SFR_PAGE(_cc0, noSST);
}

/**
 * PMCON1 Capture Compare Unit Disable bit.
 */
#define BIT_CCU_DIS	2

/**
 * CCU6_TCTR4L/CCU6_TCTR4H Timer T12/T13 Run Reset bit.
 */
#define BIT_TnRR	0

/**
 * CCU6_TCTR4L/CCU6_TCTR4H Timer T12/T13 Run Set bit.
 */
#define BIT_TnRS	1

void hsk_pwm_enable(void) {
	/* Enable clock. */
	SFR_PAGE(_su1, noSST);
	PMCON1 &= ~(1 << BIT_CCU_DIS);
	SFR_PAGE(_su0, noSST);

	/* Set T12 and T13 Timer Run bits. */
	CCU6_TCTR4L |= 1 << BIT_TnRS;
	CCU6_TCTR4H |= 1 << BIT_TnRS;

}

void hsk_pwm_disable(void) {
	/* Reset T12 and T13 Timer Run bits. */
	CCU6_TCTR4L |= 1 << BIT_TnRR;
	CCU6_TCTR4H |= 1 << BIT_TnRR;

	/* Stop clock in module. */
	SFR_PAGE(_su1, noSST);
	PMCON1 |= 1 << BIT_CCU_DIS;
	SFR_PAGE(_su0, noSST);
}

