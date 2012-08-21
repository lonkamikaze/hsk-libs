/** \file
 * HSK External Interrupt Routing headers
 *
 * This file offers functions to activate external interrupts and connect
 * them to the available input pins.
 *
 * @author kami
 */

#ifndef _HSK_EX_H_
#define _HSK_EX_H_

/*
 * C51 does not include the used register bank in pointer types.
 */
#ifdef __C51__
	#define using(bank)
#endif

/*
 * SDCC does not like the \c code keyword for function pointers, C51 needs it
 * or it will use generic pointers.
 */
#ifdef SDCC
	#undef code
	#define code
#endif /* SDCC */

/**
 * Typedef for externel interrupt channels.
 */
typedef ubyte hsk_ex_channel;

/**
 * \defgroup EX_EXINT External Interrupt Channels
 *
 * This group consists of defines representing external interrupt channels.
 *
 * @{
 */

/**
 * External interrupt channel EXINT0.
 *
 * Mask with EA, disable with EX0.
 */
#define EX_EXINT0	0

/**
 * External interrupt channel EXINT1.
 *
 * Mask with EA, disable with EX1.
 */
#define EX_EXINT1	1

/**
 * External interrupt channel EXINT2.
 *
 * Mask with EX2.
 */
#define EX_EXINT2	2

/**
 * External interrupt channel EXINT3.
 *
 * Mask with EXM.
 */
#define EX_EXINT3	3

/**
 * External interrupt channel EXINT4.
 *
 * Mask with EXM.
 */
#define EX_EXINT4	4

/**
 * External interrupt channel EXINT5.
 *
 * Mask with EXM.
 */
#define EX_EXINT5	5

/**
 * External interrupt channel EXINT6.
 *
 * Mask with EXM.
 */
#define EX_EXINT6	6

/**
 * @}
 */

/**
 * \defgroup EX_EDGE External Interrupt Triggers
 *
 * This group contains defines representing the different edge triggers
 *
 * @{
 */

/**
 * Trigger interrupt on rising edge.
 */
#define EX_EDGE_RISING	0

/**
 * Trigger interrupt on falling edge.
 */
#define EX_EDGE_FALLING	1

/**
 * Trigger interrupt on both edges.
 */
#define EX_EDGE_BOTH	2

/**
 * @}
 */

/**
 * Enable an external interrupt channel.
 *
 * It is good practice to enable a port for the channel first, because
 * port changes on an active interrupt may cause an undesired interrupt.
 *
 * The callback function can be set to 0 if a change of the function is
 * not desired. For channels EXINT0 and EXINT1 the callback is ignored,
 * implement interrupts 0 and 2 instead.
 *
 * @param channel
 *	The channel to activate, one of \ref EX_EXINT
 * @param edge
 *	The triggering edge, one of \ref EX_EDGE
 * @param callback
 *	The callback function for an interrupt event
 */
void hsk_ex_channel_enable(const hsk_ex_channel idata channel,
	const ubyte idata edge,
	const void (code * const idata callback)(void) using(1));

/**
 * Disables an external interrupt channel.
 *
 * @param channel
 *	The channel to disable, one of \ref EX_EXINT
 */
void hsk_ex_channel_disable(const hsk_ex_channel idata channel);

/**
 * Typedef for externel interrupt ports.
 */
typedef ubyte hsk_ex_port;

/**
 * \defgroup EX_EXINT_P External Interrupt Input Ports
 *
 * Each define of this group represents an external interrupt port
 * configuration
 *
 * @{
 */

/**
 * External interrupt EXINT0 input port P0.5.
 */
#define EX_EXINT0_P05	0

/**
 * External interrupt EXINT3 input port P1.1.
 */
#define EX_EXINT3_P11	1

/**
 * External interrupt EXINT0 input port P1.4.
 */
#define EX_EXINT0_P14	2

/**
 * External interrupt EXINT5 input port P1.5.
 */
#define EX_EXINT5_P15	3

/**
 * External interrupt EXINT6 input port P1.6.
 */
#define EX_EXINT6_P16	4

/**
 * External interrupt EXINT3 input port P3.0.
 */
#define EX_EXINT3_P30	5

/**
 * External interrupt EXINT4 input port P3.2.
 */
#define EX_EXINT4_P32	6

/**
 * External interrupt EXINT5 input port P3.3.
 */
#define EX_EXINT5_P33	7

/**
 * External interrupt EXINT6 input port P3.4.
 */
#define EX_EXINT6_P34	8

/**
 * External interrupt EXINT4 input port P3.7.
 */
#define EX_EXINT4_P37	9

/**
 * External interrupt EXINT3 input port P4.0.
 */
#define EX_EXINT3_P40	10

/**
 * External interrupt EXINT4 input port P4.1.
 */
#define EX_EXINT4_P41	11

/**
 * External interrupt EXINT6 input port P4.2.
 */
#define EX_EXINT6_P42	12

/**
 * External interrupt EXINT5 input port P4.4.
 */
#define EX_EXINT5_P44	13

/**
 * External interrupt EXINT6 input port P4.5.
 */
#define EX_EXINT6_P45	14

/**
 * External interrupt EXINT1 input port P5.0.
 */
#define EX_EXINT1_P50	15

/**
 * External interrupt EXINT2 input port P5.1.
 */
#define EX_EXINT2_P51	16

/**
 * External interrupt EXINT5 input port P5.2.
 */
#define EX_EXINT5_P52	17

/**
 * External interrupt EXINT1 input port P5.3.
 */
#define EX_EXINT1_P53	18

/**
 * External interrupt EXINT2 input port P5.4.
 */
#define EX_EXINT2_P54	19

/**
 * External interrupt EXINT3 input port P5.5.
 */
#define EX_EXINT3_P55	20

/**
 * External interrupt EXINT4 input port P5.6.
 */
#define EX_EXINT4_P56	21

/**
 * External interrupt EXINT6 input port P5.7.
 */
#define EX_EXINT6_P57	22

/**
 * @}
 */

/**
 * Opens an input port for an external interrupt.
 *
 * @param port
 *	The port to open, one of \ref EX_EXINT_P
 */
void hsk_ex_port_open(const hsk_ex_port idata port);

/**
 * Disconnects an input port from an external interrupt.
 *
 * @param port
 *	The port to close, one of \ref EX_EXINT_P
 */
void hsk_ex_port_close(const hsk_ex_port idata port);

/*
 * Restore the usual meaning of \c code.
 */
#ifdef SDCC
	#undef code
	#define code	__code
#endif

/*
 * Restore the usual meaning of \c using(bank).
 */
#ifdef __C51__
	#undef using
#endif /* __C51__ */

#endif /* _HSK_EX_H_ */

