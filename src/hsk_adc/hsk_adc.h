/** \file
 * HSK Analog Digital Conversion headers
 *
 * This library provides access to all 8 ADC channels. Each channel can be
 * provided with a pointer. Every completed conversion is written to the
 * address provided by the pointer.
 * The target memory can be protected for read access by msking the interrupts
 * with EADC.
 *
 * The conversion time can be freely configured in a wide range. Even
 * short conversion times like 5µs yield good precission.
 *
 * In order to keep the conversion going a service function hsk_adc_service()
 * has to be called on a regular basis. This prevents locking up of the
 * CPU due to an overload of interrupts, the ADC module can provide a new
 * conversion result every 30 clock cycles.
 *
 * Making the hsk_adc_service() call only as often as needed reduces the
 * drain on the analogue input and reduces flickering.
 *
 * Alternatively hsk_adc_request() can be used to request single just in time
 * conversions.
 *
 * @author kami
 */

#ifndef _HSK_ADC_H_
#define _HSK_ADC_H_

/*
 * Required for SDCC to propagate ISR prototypes.
 */
#ifdef SDCC
#include "../hsk_isr/hsk_isr.isr"
#endif /* SDCC */


/**
 * 10 bit ADC resolution.
 */
#define ADC_RESOLUTION_10	0

/**
 * 8 bit ADC resultion.
 */
#define ADC_RESOLUTION_8	1

/**
 * Typedef for ADC channel ids.
 */
typedef ubyte hsk_adc_channel;

/**
 * Initialize the AD conversion.
 *
 * The shortest possible conversion time is 1.25µs, the longest is 714.75µs.
 * The given value will be rounded down.
 *
 * Note if hsk_adc_service() is not called in intervals shorter than convTime,
 * there will be a waiting period between conversions. This prevents locking
 * up of the controler with erratic interrupts.
 *
 * There is a 4 entry queue, for starting conversions, so it suffices to
 * average the interval below convTime.
 *
 * All already open channels will be closed upon calling this function.
 *
 * @param resolution
 *	The conversion resolution, any of ADC_RESOLUTION_*
 * @param convTime
 *	The desired conversion time in µs
 */
void hsk_adc_init(ubyte idata resolution, uword idata convTime);

/**
 * Turns on ADC conversion, if previously deactivated.
 */
void hsk_adc_enable(void);

/**
 * Turns off ADC conversion unit to converse power.
 */
void hsk_adc_disable(void);

/**
 * Open the given ADC channel.
 *
 * @param channel
 *	The channel id
 * @param target
 *	A pointer where to store conversion results
 */
void hsk_adc_open(const hsk_adc_channel idata channel,
	uword * const idata target);

/**
 * Close the given ADC channel.
 *
 * Stopp ADC if no more channels were left.
 *
 * @param channel
 *	The channel id
 */
void hsk_adc_close(const hsk_adc_channel idata channel);

/**
 * A maintenance function that takes care of keeping AD conversions going.
 * This has to be called repeatedly.
 *
 * There is a queue of up to 4 conversion jobs. One call of this function
 * only adds one job to the queue.
 */
void hsk_adc_service(void);

/**
 * Warm up the AD conversion.
 *
 * I.e. make sure all conversion targets have been initialized with a
 * conversion result. This is a blocking function only intended for single
 * use during the boot procedure.
 *
 * This function will not terminate unless interrupts are enabled.
 */
void hsk_adc_warmup(void);

/**
 * Requests an ADC for a specific channel.
 *
 * This function is an alternative to hsk_adc_service(). Make requests
 * in time before the updated value is required.
 *
 * This function uses the same queue as hsk_adc_service(), if the queue is
 * full it fails silently.
 *
 * @param channel
 *	The channel id
 */
void hsk_adc_request(const hsk_adc_channel idata channel);

#endif /* _HSK_ADC_H_ */
