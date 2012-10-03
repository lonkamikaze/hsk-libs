/** \file
 * HSK I/O headers
 *
 * This file contains macro definitions to use and initialize I/O ports
 * and variables bitwise.
 *
 * All the macros take a port and a mask to select the affected pins. All
 * operations are masked with the selected pins so it is safe to define
 * -1 (every bit 1) to activate a certain property.
 *
 * Set and get macros take a bit field to define the value that represents
 * the \c on or \c true state, so the logic code can always use a \c 1 for
 * <tt>true</tt>/<tt>on</tt>.
 *
 * The macros are grouped as:
 *	- \ref IO_PORT_IN
 *	- \ref IO_PORT_OUT
 *	- \ref IO_VAR
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

#ifndef _HSK_IO_H_
#define _HSK_IO_H_

/**
 * \defgroup IO_PORT_IN Input Port Access
 *
 * This group contains defines and macros to initialize port pins as inputs
 * and read them.
 *
 * @{
 */

/**
 * Bit mask to disable pull up/down for all selected pins.
 */
#define IO_PORT_PULL_DISABLE	0

/**
 * Bit mask to enable pull up/down for all selected pins.
 */
#define IO_PORT_PULL_ENABLE	-1

/**
 * Bit mask to select pull down for all selected pins.
 */
#define IO_PORT_PULL_DOWN		0

/**
 * Bit mask to select pull up for all selected pins.
 */
#define IO_PORT_PULL_UP		-1

/**
 * Initializes a set of port pins as inputs.
 *
 * @warning
 *	Expects port page 0 and RMAP 0, take care in ISRs
 * @param port
 *	The parallel port to configure
 * @param pins
 *	A bit mask of the pins to select
 * @param pull
 *	A bit mask of pins to activate the internal pull up/down device for
 * @param dir
 *	A bit mask of pins to set the pull direction
 */
#define IO_PORT_IN_INIT(port, pins, pull, dir)	{ \
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
 *	Can also be used for \ref IO_PORT_OUT
 */
#define IO_PORT_ON_GND		0

/**
 * Bit mask to set the logical 1 to high level for all selected pins.
 *
 * @note
 *	Can also be used for \ref IO_PORT_OUT
 */
#define IO_PORT_ON_HIGH		-1


/**
 * Evaluates to a bit mask of logical pin states of a port.
 *
 * @note
 *	Can also be used for \ref IO_PORT_OUT
 * @warning
 *	Expects port page 0 and RMAP 0, take care in ISRs
 * @param port
 *	The parallel port to access
 * @param pins
 *	A bit mask of the pins to select
 * @param on
 *	A bit mask of pins that defines the states which represent on
 */
#define IO_PORT_GET(port, pins, on) ( \
	(port##_DATA ^ ~(on)) & (pins) \
)

/**
 * @}
 */

/**
 * \defgroup IO_PORT_OUT Output Port Access
 *
 * This group contains macros and defines to initialize port pins for output
 * and safely set output states.
 *
 * @{
 */

/**
 * Bit mask to set weak drive strength for all selected pins.
 */
#define IO_PORT_STRENGTH_WEAK	0

/**
 * Bit mask to set strong drive strength for all selected pins.
 */
#define IO_PORT_STRENGTH_STRONG	-1

/**
 * Bit mask to disable drain mode for all selected pins.
 */
#define IO_PORT_DRAIN_DISABLE	0

/**
 * Bit mask to enable drain mode for all selected pins.
 */
#define IO_PORT_DRAIN_ENABLE	-1

/**
 * Initializes a set of port pins as outputs.
 *
 * @warning
 *	Expects port page 0 and RMAP 0, take care in ISRs
 * @param port
 *	The parallel port to configure
 * @param pins
 *	A bit mask of the pins to select
 * @param strength
 *	A bit mask of pins with strong drive strength
 * @param drain
 *	A bit mask of pins that only drive GND
 * @param on
 *	A bit mask of pins that defines the states which represent on
 * @see IO_PORT_ON_GND
 * @see IO_PORT_ON_HIGH
 * @param set
 *	Initial logical values for the defined outputs
 */
#define IO_PORT_OUT_INIT(port, pins, strength, drain, on, set)	{\
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
 *	Expects port page 0 and RMAP 0, take care in ISRs
 * @param port
 *	The parallel port to set
 * @param pins
 *	A bit mask of the pins to select
 * @param on
 *	A bit mask of pins that defines the states which represent on
 * @see IO_PORT_ON_GND
 * @see IO_PORT_ON_HIGH
 * @param set
 *	Set logical values for the defined outputs
 */
#define IO_PORT_OUT_SET(port, pins, on, set)	{\
	port##_DATA &= ((set) ^ ~(on)) | ~(pins); \
	port##_DATA |= ((set) ^ ~(on)) & (pins); \
}

/**
 * @}
 */

/**
 * \defgroup IO_VAR Variable Access
 *
 * This group specifies macros to access bits of a variable. Their value lies
 * in the seperation of encoded \c on state and logical \c on (1), as well as
 * the safe bit masking.
 *
 * @{
 */

/**
 * Set a set of variable bits.
 *
 * @param var
 *	The variable to set
 * @param bits
 *	A bit mask of the bits to select
 * @param on
 *	A bit mask that defines the states which represent true
 * @param set
 *	Set logical values for the defined bits
 */
#define IO_VAR_SET(var, bits, on, set)	{\
	(var) &= ((set) ^ ~(on)) | ~(bits); \
	(var) |= ((set) ^ ~(on)) & (bits); \
}

/**
 * Evaluates to a bit mask of logical states of a variable.
 *
 * @param var
 *	The variable to access
 * @param bits
 *	A bit mask of the bits to select
 * @param on
 *	A bit mask that defines the states which represent true
 */
#define IO_VAR_GET(var, bits, on) ( \
	((var) ^ ~(on)) & (bits) \
)

/**
 * @}
 */

#endif /* _HSK_IO_H_ */

