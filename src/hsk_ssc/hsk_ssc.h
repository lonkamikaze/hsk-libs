/** \file
 * HSK Synchronous Serial Interface headers
 *
 * @note
 *	For half duplex operation TX and RX pins need to be connected.
 * @author kami
 */

#ifndef _HSK_SSC_H_
#define _HSK_SSC_H_

/*
 * ISR prototypes for SDCC.
 */
#ifdef SDCC
#include "hsk_ssc.isr"
#endif /* SDCC */

/**
 * \defgroup SSC_PORTS	SCC I/O Ports
 *
 * Used to create an I/O Port configuration, by unifying one of the
 * SSC_MRST_P* with a SSC_MTSR_P* and a SSC_SCLK_P* ports.
 * E.g.:
 * \verbatim
 * SSC_MRST_P05 | SSC_MTSR_P4 | SSC_SCLK_P03.
 * \endverbatim
 *
 * The ports have the following functions:
 * | Type | Master Mode | Slave Mode
 * |------|-------------|------------
 * | MRST | RX port     | TX port
 * | MTSR | TX port     | RX port
 * | SCLK | TX clock    | RX clock
 *
 * @{
 */

/**
 * Master mode RX, slave mode TX port P0.5.
 */
#define SSC_MRST_P05	1

/**
 * Master mode RX, slave mode TX port P1.4.
 */
#define SSC_MRST_P14	0

/**
 * Master mode RX, slave mode TX port P1.5.
 */
#define SSC_MRST_P15	2

/**
 * Master mode TX, slave mode RX port P0.4.
 */
#define SSC_MTSR_P04	(1 << 2)

/**
 * Master mode TX, slave mode RX port P1.3.
 */
#define SSC_MTSR_P13	(0 << 2)

/**
 * Master mode TX, slave mode RX port P1.4.
 */
#define SSC_MTSR_P14	(2 << 2)

/**
 * Synchronous clock port P0.3.
 */
#define SSC_SCLK_P03	(1 << 4)

/**
 * Synchronous clock port P1.2.
 */
#define SSC_SCLK_P12	(0 << 4)

/**
 * Synchronous clock port P1.3.
 */
#define SSC_SCLK_P13	(2 << 4)

/**
 * @}
 */

/**
 * Master mode, output shift clock on SCLK.
 */
#define SSC_MASTER	1

/**
 * Slave mode, receive shift clock on SCLK.
 */
#define SSC_SLAVE	0

/**
 * Converts a baud rate value in bits/s into a baud rate value for the
 * hsk_ssc_init() function.
 *
 * The distance between adjustable baud rates grows exponentially.
 * Available baud rates in kHz progress like this:
 *
 * \f[\{12000, 6000, 4000, 3000, 2400, 2000, ...\}\f]
 *
 * Use the following formula to determine the baud rate that results from
 * a desired value:
 * 	\f[{realBps}(bps) = \frac{12000000}{\lfloor\frac{12000000}{bps}\rfloor}\f]
 *
 * @note
 *	The maximum speed is 12 Mbit/s in master mode and 6 Mbit/s in slave
 *	mode.
 * @param bps
 *	The desired number in bits/s
 * @return
 *	A timer reload value
 */
#define SSC_BAUD(bps)		(uword)(12000000ul / (bps) - 1)

/**
 * Generates an SSC configuration byte.
 *
 * For details check the XC878 user manual section 12.3.5.1.
 *
 * @param width
 *	The data with in bits, the available range is $[2;8]$
 * @param heading
 *	Use 0 for transmitting/receiving LSB first, 1 for MSB first
 * @param phase
 *	Use 0 to shift on leading and latch on trailing edge, use 1 to
 *	shift on trailing and latch on leading edge
 * @param polarity
 *	Use 0 for low idle clock, and 1 for high idle clock
 * @param duplex
 *	Use 0 for full duplex mode and 1 for half duplex
 */
#define SSC_CONF(width, heading, phase, polarity, duplex) \
	(((width) - 1) | ((heading) << 4) | ((phase) << 5) | ((polarity) << 6) | ((duplex) << 7))

/**
 * The maximum baud rate in master mode is 12000000 bits/s, and 6000000
 * bits/s in slave mode.
 *
 * Calling this function turns the SCC off until hsk_ssc_enable() is called.
 *
 * @param baud
 *	The timer reload value for the baud rate generator, use \ref SSC_BAUD
 *	to generate this value
 * @param config
 *	The SSC configuration byte, use \ref SSC_CONF to generate it
 * @param mode
 *	Select master or slave operation
 */
void hsk_ssc_init(const uword baud, const ubyte config, const bool mode);

/**
 * Configure the I/O ports of the SSC unit.
 *
 * @param ports
 *	Selects an \ref SSC_PORTS I/O port configuration
 */
void hsk_ssc_ports(const ubyte ports);

/**
 * Send and receive data.
 *
 * The buffer with the given length should contain the data to transceive
 * and will be filled with the received data upon completion.
 *
 * @param buffer
 *	The rx/tx transmission buffer
 * @param len
 *	The length of the buffer
 */
void hsk_ssc_talk(char xdata * buffer, ubyte len);

/**
 * Returns whether the SSC is currently busy with data transmission.
 */
#define hsk_ssc_busy()	ESSC

/**
 * Turn the SSC module on.
 */
void hsk_ssc_enable();

/**
 * Turn the SSC module off.
 */
void hsk_ssc_disable();

#endif /* _HSK_SSC_H_ */
