/** \file
 * HSK Analog Digital Conversion implementation
 *
 * This file implements the functions defined in hsk_adc.h.
 *
 * To be able to use all 8 channels the ADC is kept in sequential mode.
 *
 * @author kami
 */

#include <Infineon/XC878.h>

#include "hsk_adc.h"

#include <string.h> /* memset() */

#include "../hsk_isr/hsk_isr.h"

/**
 * Conversion clock prescaler setting for 12MHz.
 */
#define ADC_CLK_12MHz		0

/**
 * Conversion clock prescaler setting for 8MHz.
 */
#define ADC_CLK_8MHz		1

/**
 * Conversion clock prescaler setting for 6MHz.
 */
#define ADC_CLK_6MHz		2

/**
 * Conversion clock prescaler setting for 750kHz.
 */
#define ADC_CLK_750kHz		3


/**
 * Number of availbale ADC channels.
 */
#define ADC_CHANNELS		8

/**
 * Number of queue slots.
 */
#define ADC_QUEUE		4

/**
 * Holds the channel of the next conversion that will be requested.
 */
hsk_adc_channel pdata hsk_adc_nextChannel = ADC_CHANNELS;

/**
 * Holds the number of queue entries.
 */
volatile ubyte pdata hsk_adc_queue = 0;

/**
 * An array of target addresses to write conversion results into.
 */
volatile uword * pdata hsk_adc_targets[ADC_CHANNELS] = {0};

/**
 * ADC_RESRxL Channel Number bits.
 */
#define BIT_CHNR		0

/**
 * CHNR bit count.
 */
#define CNT_CHNR		3

/**
 * ADC_RESRxLH Conversion Result bits.
 */
#define BIT_RESULT		6

/**
 * RESULT bit count.
 */
#define CNT_RESULT		10

/**
 * ADC_QINR0 Request Channel Number bits.
 */
#define BIT_REQCHNR		0

/**
 * REQCHNR bit count.
 */
#define CNT_REQCHNR		3

/**
 * ADC_QMR0 Trigger Event bit.
 */
#define BIT_TREV		6

/**
 * ADC_GLOBCTR Data Width bit.
 */
#define BIT_DW			6

/**
 * Write the conversion result to the targeted memory address.
 *
 * @private
 */
#pragma save
#ifdef SDCC
#pragma nooverlay
#endif
void hsk_adc_isr(void) using 1 {
	hsk_adc_channel channel;
	uword result;

	/* Read the result. */
	SFR_PAGE(_ad2, SST1);
	result = ADC_RESR0LH;

	/* Update the queue counter. */
	hsk_adc_queue--;

	/*
	 * Deliver the conversion result.
	 */
	/* Extract the channel. */
	SFR_PAGE(_ad0, noSST);
	channel = (result >> BIT_CHNR) & ((1 << CNT_CHNR) - 1);
	if (((ADC_GLOBCTR >> BIT_DW) & 1) == ADC_RESOLUTION_10) {
		result = (result >> BIT_RESULT) & ((1 << CNT_RESULT) - 1);
	} else {
		result = (result >> (BIT_RESULT + 2)) & ((1 << (CNT_RESULT - 2)) - 1);
	}
	SFR_PAGE(_ad2, RST1);

	/* Deliver result to the target address. */
	if (hsk_adc_targets[channel]) {
		/* Get the result bits and deliver them. */
		*hsk_adc_targets[channel] = result;
	}
}
#pragma restore

/**
 * ADC_GLOBCTR Conversion Time Control bits.
 */
#define BIT_CTC			4

/**
 * CTC bit count.
 */
#define CNT_CTC			2

/**
 * ADC_PRAR Arbitration Slot Sequential Enable bit.
 */
#define BIT_ASEN_SEQUENTIAL	6

/**
 * ADC_PRAR Arbitration Slot Parallel Enable bit.
 */
#define BIT_ASEN_PARALLEL	7

/**
 * RCRx Interrupt Enable bit.
 */
#define BIT_IEN			4

/**
 * RCRx Wait-for-Read Mode.
 */
#define BIT_WFR			5

/**
 * RCRx Valid Flag Control bit.
 */
#define BIT_VFCTR		7

/**
 * QMR0 Enable Gate bit.
 */
#define BIT_ENGT		0

/**
 * ADC_GLOBCTR Analog Part Switched On bit.
 */
#define BIT_ANON		7

/**
 * SYSCON0 Interrupt Structure 2 Mode Select bit.
 */
#define BIT_IMODE		4

void hsk_adc_init(ubyte idata resolution, uword idata convTime) {
	/* The Conversion Time Control bits, any of ADC_CLK_*. */
	ubyte ctc;
	/* The Sample Time Control bits, values from 0 to 255. */
	uword stc;

	/* Make sure the conversion target list is clean. */
	memset(hsk_adc_targets, 0, sizeof(hsk_adc_targets));

	/* Set ADC resolution */
	ADC_GLOBCTR = ADC_GLOBCTR & ~(1 << BIT_DW) | (resolution << BIT_DW);
	resolution = resolution == ADC_RESOLUTION_10 ? 10 : 8;

	/*
	 * Calculate conversion time parameters.
	 */
	/* Convert convTime into clock ticks. */
	convTime *= 24;
	/*
	 * Find the fastest possible CTC prescaler, based on the maximum
	 * STC value.
	 * Then find the appropriate STC value, check the Conversion Timing
	 * section of the Analog-to-Digital Converter chapter.
	 */
	if (convTime <= 1 + 2 * (258 + resolution)) {
		ctc = ADC_CLK_12MHz;
		stc = (convTime - 1) / 2 - 3 - resolution;
	} else if (convTime <= 1 + 3 * (258 + resolution)) {
		ctc = ADC_CLK_8MHz;
		stc = (convTime - 1) / 3 - 3 - resolution;
	} else if (convTime <= 1 + 4 * (258 + resolution)) {
		ctc = ADC_CLK_6MHz;
		stc = (convTime - 1) / 4 - 3 - resolution;
	} else {
		ctc = ADC_CLK_750kHz;
		stc = (convTime - 1) / 32 - 3 - resolution;
	}
	/* Make sure STC fits into an 8 bit register. */
	stc = stc >= 1 << 8 ? (1 << 8) - 1 : stc;

	/* Set ADC module clk */
	ADC_GLOBCTR = ADC_GLOBCTR & ~(((1 << CNT_CTC) - 1) << BIT_CTC) | (ctc << BIT_CTC);
	/* Set sample time in multiples of ctc scaled clock cycles. */
	ADC_INPCR0 = stc;

	/* No boundary checks. */
	ADC_LCBR = 0x00;

	/* Allow sequential arbitration mode only. */
	SFR_PAGE(_ad0, noSST);
	ADC_PRAR |= 1 << BIT_ASEN_SEQUENTIAL;
	ADC_PRAR &= ~(1 << BIT_ASEN_PARALLEL);

	/* Reset valid flag on result register 0 access. */
	SFR_PAGE(_ad4, noSST);
	ADC_RCR0 |= (1 << BIT_IEN) | (1 << BIT_WFR) | (1 << BIT_VFCTR);

	/* Use ADCSR0 interrupt. */
	SFR_PAGE(_ad5, noSST);
	ADC_CHINPR = 0x00;
	ADC_EVINPR = 0x00;

	/* Enable the queue mode gate. */
	SFR_PAGE(_ad6, noSST);
	ADC_QMR0 |= 1 << BIT_ENGT;

	/* Turn on analogue part. */
	SFR_PAGE(_ad0, noSST);
	ADC_GLOBCTR |= 1 << BIT_ANON;
	/* Wait 100ns, that's less than 3 cycles, don't bother. */

	/* Register interrupt handler. */
	EADC = 0;
	hsk_isr6.ADCSR0 = &hsk_adc_isr;
	/* Set IMODE, 1 so that EADC can be used to mask interrupts without
	 * loosing them. */
	SYSCON0 |= 1 << BIT_IMODE;
	/* Enable interrupt. */
	EADC = 1;
}

/**
 * PMCON1 ADC Disable Request bit.
 */
#define BIT_ADC_DIS		0

void hsk_adc_enable(void) {
	/* Enable clock. */
	SFR_PAGE(_su1, noSST);
	PMCON1 &= ~(1 << BIT_ADC_DIS);
	SFR_PAGE(_su0, noSST);
}

void hsk_adc_disable(void) {
	/* Stop clock in module. */
	SFR_PAGE(_su1, noSST);
	PMCON1 |= 1 << BIT_ADC_DIS;
	SFR_PAGE(_su0, noSST);
}

/**
 * QSR0 bits Filling Level.
 */
#define BIT_FILL	0

/**
 * Filling Level bit count.
 */
#define CNT_FILL	2

/**
 * QSR0 bit Queue Empty.
 */
#define BIT_EMPTY	5

void hsk_adc_open(const hsk_adc_channel idata channel,
		uword * const idata target) {
	bool eadc = EADC;

	EADC = 0;
	/* Register callback function. */
	hsk_adc_targets[channel] = target;
	EADC = eadc;

	/* Check if there are no open channels. */
	if (hsk_adc_nextChannel >= ADC_CHANNELS) {
		/* Claim the spot as the first open channel. */
		hsk_adc_nextChannel = channel;
	}
}

void hsk_adc_close(const hsk_adc_channel idata channel) {
	bool eadc = EADC;
	EADC = 0;
	/* Unregister conversion target address. */
	hsk_adc_targets[channel] = 0;
	EADC = eadc;
	/* If this channel is scheduled for the next conversion, find an
	 * alternative. */
	if (hsk_adc_nextChannel == channel) {
		/* Get next channel. */
		for (; hsk_adc_nextChannel < channel + ADC_CHANNELS && !hsk_adc_targets[hsk_adc_nextChannel]; hsk_adc_nextChannel++);
		hsk_adc_nextChannel %= ADC_CHANNELS;
		/* Check whether no active channel was found. */
		if (!hsk_adc_targets[hsk_adc_nextChannel]) {
			hsk_adc_nextChannel = ADC_CHANNELS;
		}
	}
}

void hsk_adc_service(void) {
	/* Check for a full queue and available channels. */
	if (hsk_adc_queue >= ADC_QUEUE || hsk_adc_nextChannel >= ADC_CHANNELS) {
		return;
	}
	SFR_PAGE(_ad6, noSST);
	/* Set next channel. */
	ADC_QINR0 = ADC_QINR0 & ~(((1 << CNT_REQCHNR) - 1) << BIT_REQCHNR) | (hsk_adc_nextChannel << BIT_REQCHNR);
	/* Request next conversion. */
	ADC_QMR0 |= 1 << BIT_TREV;
	SFR_PAGE(_ad0, noSST);

	/* Find next conversion channel. */
	while (!hsk_adc_targets[++hsk_adc_nextChannel % ADC_CHANNELS]);
	hsk_adc_nextChannel %= ADC_CHANNELS;
}

void hsk_adc_request(const hsk_adc_channel idata channel) {
	/* Check for a full queue. */
	if (hsk_adc_queue >= ADC_QUEUE) {
		return;
	}
	/* Check for empty slots in the queue. */
	SFR_PAGE(_ad6, noSST);
	/* Set next channel. */
	ADC_QINR0 = ADC_QINR0 & ~(((1 << CNT_REQCHNR) - 1) << BIT_REQCHNR) | (channel << BIT_REQCHNR);
	/* Request next conversion. */
	ADC_QMR0 |= 1 << BIT_TREV;
	SFR_PAGE(_ad0, noSST);
}

/**
 * Special ISR for warming up the conversion.
 *
 * This is used as the ISR by hsk_adc_warmup() after the warmup countdowns
 * have been initialized. After all warmup countdowns have returned to zero
 * The original ISR will be put back in control.
 *
 * @private
 */
#pragma save
#ifdef SDCC
#pragma nooverlay
#endif
void hsk_adc_isr_warmup(void) using 1 {
	ubyte i;

	/* Let the original ISR do its thing. */
	hsk_adc_isr();

	/* Check whether all channels have completed warmup. */
	for (i = 0; i < ADC_CHANNELS; i++) {
		if (hsk_adc_targets[i] && *hsk_adc_targets[i] == -1) {
			/* Bail out if a channel is not warmed up. */
			return;
		}
	}

	/* Hand over to the original ISR. */
	hsk_isr6.ADCSR0 = &hsk_adc_isr;
}
#pragma restore

void hsk_adc_warmup(void) {
	ubyte i;

	/* Set all conversion targets to an invalid value so the value that
	 * was written can be detected. */
	for (i = 0; i < ADC_CHANNELS; i++) {
		if (hsk_adc_targets[i]) {
			*hsk_adc_targets[i] = -1;
		}
	}

	/* Hijack the ISR. */
	EADC = 0;
	hsk_isr6.ADCSR0 = &hsk_adc_isr_warmup;
	EADC = 1;

	/*
	 * Now just keep on performing conversion until the warumup isr
	 * unregisters itself.
	 */
	while (hsk_isr6.ADCSR0 == &hsk_adc_isr_warmup) {
		hsk_adc_service();
	}
}

