/** \file
 * Configuration for the Infineon XC800 Starter Kit.
 *
 * @author kami
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
 * The CAN1 baud rate in bits/s.
 */
#define CAN1_BAUD	1000000

/**
 * The CAN0 IO pin configuration RX P1.0 TX P1.1.
 */
#define CAN0_IO		CAN0_IO_P10_P11

/**
 * The CAN1 IO pin configuration RX P1.4 TX P1.3.
 */
#define CAN1_IO		CAN1_IO_P14_P13

#endif /* _HSK_CONFIG_H_ */
