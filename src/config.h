/** \file
 * Configuration for the Infineon XC800 Starter Kit.
 *
 * @author kami
 * @version 2011-07-22
 */

#ifndef _HSK_CONFIG_H_
#define _HSK_CONFIG_H_

/**
 * The external oscilator clock frequency.
 */
#define	CLK		8000000UL

/**
 * The CAN0 baud rate in bits/s.
 */
#define CAN0_BAUD	1000000

/**
 * The CAN0 IO pin configuration RX P1.0 TX P1.1.
 */
#define CAN0_IO		CAN_IO_P10_P11

#endif /* _HSK_CONFIG_H_ */
