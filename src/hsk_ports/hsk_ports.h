/** \file
 * HSK I/O Port headers
 *
 * This file contains macro definitions to use and initialize I/O ports.
 *
 * All the macros take a port and a mask to select the affected pins. All
 * operations are masked with the selected pins so it is safe to define
 * -1 (every bit 1) to activate a certain property.
 *
 * The macros are grouped as:
 *	- \ref PORTS_IN
 *	- \ref PORTS_OUT
 *
 * @author kami
 *
 * \section ports I/O Port Pull-Up/-Down Table
 *
 * The device boots with all parallel ports configured as inputs.
 * The following table lists the pins that come up with activated internal
 * pull up:
 *
 * | Port\\Bit	| 7 | 6 | 5 | 4 | 3 | 2 | 1 | 0
 * |------------|:-:|:-:|:-:|:-:|:-:|:-:|:-:|:-:
 * | P0 	| 1 | 1 | x | x | x | 1 | x | x
 * | P1		| 1 | 1 | 1 | 1 | 1 | 1 | 1 | 1
 * | P3		| x | 1 | x | x | x | x | x | x
 * | P4		| x | x | x | x | x | 1 | x | x
 * | P5		| 1 | 1 | 1 | 1 | 1 | 1 | 1 | 1
 */

#ifndef _HSK_PORTS_H_
#define _HSK_PORTS_H_

/**
 * \defgroup PORTS_IN Input Port Access
 * @{
 */

/**
 * Bit mask to disable pull up/down for all selected pins.
 */
#define PORTS_PULL_DISABLE	0

/**
 * Bit mask to enable pull up/down for all selected pins.
 */
#define PORTS_PULL_ENABLE	-1

/**
 * Bit mask to select pull down for all selected pins.
 */
#define PORTS_PULL_DOWN		0

/**
 * Bit mask to select pull up for all selected pins.
 */
#define PORTS_PULL_UP		-1

/**
 * Initializes a set of port pins as inputs.
 *
 * @warning
 *	This function must not be used in an interrupt
 * @param port
 *	The parallel port to configure
 * @param pins
 *	A bit mask of the pins to select
 * @param pull
 *	A bit mask of pins to activate the internal pull up/down device for
 * @param dir
 *	A bit mask of pins to set the pull direction
 */
#define PORTS_IN_INIT(port, pins, pull, dir)	{ \
	port##_DIR &= ~(pins); \
	SFR_PAGE(_pp1, noSST); \
	port##_PUDSEL &= (dir) | ~(pins); \
	port##_PUDSEL |= (dir) & (pins); \
	port##_PUDEN &= (pull) | ~(pins); \
	port##_PUDEN |= (pull) & (pins); \
	SFR_PAGE(_pp0, noSST); \
}

/**
 * Bit mask to set the logical 1 to GND level for all selected pins.
 *
 * @note
 *	Can also be used for \ref PORTS_OUT
 */
#define PORTS_ON_GND		0

/**
 * Bit mask to set the logical 1 to high level for all selected pins.
 *
 * @note
 *	Can also be used for \ref PORTS_OUT
 */
#define PORTS_ON_HIGH		-1


/**
 * Evaluates to a bit mask of logical pin states of a port.
 *
 * @note
 *	Can also be used for \ref PORTS_OUT
 * @param port
 *	The parallel port to configure
 * @param pins
 *	A bit mask of the pins to select
 * @param on
 *	A bit mask of pins that defines the state which is considered on
 */
#define PORTS_GET(port, pins, on) ( \
	(port##_DATA ^ ~(on)) & (pins) \
)

/**
 * @}
 */

/**
 * \defgroup PORTS_OUT Output Port Access
 * @{
 */

/**
 * Bit mask to set weak drive strength for all selected pins.
 */
#define PORTS_STRENGTH_WEAK	0

/**
 * Bit mask to set strong drive strength for all selected pins.
 */
#define PORTS_STRENGTH_STRONG	-1

/**
 * Bit mask to disable drain mode for all selected pins.
 */
#define PORTS_DRAIN_DISABLE	0

/**
 * Bit mask to enable drain mode for all selected pins.
 */
#define PORTS_DRAIN_ENABLE	-1

/**
 * Initializes a set of port pins as outputs.
 *
 * @warning
 *	This function must not be used in an interrupt
 * @param port
 *	The parallel port to configure
 * @param pins
 *	A bit mask of the pins to select
 * @param strength
 *	A bit mask of pins with strong drive strength
 * @param drain
 *	A bit mask of pins that only drive GND
 * @param on
 *	A bit mask of pins that defines the state which is considered on
 * @see PORTS_ON_GND
 * @see PORTS_ON_HIGH
 * @param set
 *	Initial logical values for the defined outputs
 */
#define PORTS_OUT_INIT(port, pins, strength, drain, on, set)	{\
	port##_DIR |= pins; \
	SFR_PAGE(_pp3, noSST); \
	port##_OD &= (drain) | ~(pins); \
	port##_OD |= (drain) & (pins); \
	port##_DS &= (strength) | ~(pins); \
	port##_DS |= (strength) & (pins); \
	SFR_PAGE(_pp0, noSST); \
	port##_DATA &= ((set) ^ ~(on)) | ~(pins); \
	port##_DATA |= ((set) ^ ~(on)) & (pins); \
}

/**
 * Set a set of output port pins.
 *
 * @warning
 *	This function must not be used in an interrupt
 * @param port
 *	The parallel port to configure
 * @param pins
 *	A bit mask of the pins to select
 * @param on
 *	A bit mask of pins that defines the state which is considered on
 * @see PORTS_ON_GND
 * @see PORTS_ON_HIGH
 * @param set
 *	Set logical values for the defined outputs
 */
#define PORTS_OUT_SET(port, pins, on, set)	{\
	port##_DATA &= ((set) ^ ~(on)) | ~(pins); \
	port##_DATA |= ((set) ^ ~(on)) & (pins); \
}

/**
 * @}
 */


#endif /* _HSK_PORTS_H_ */
