/**
 * @defgroup SIG_<:name:> Signal <:name:>
 *
 * <:comment:>
 *
 * Sent in message \ref MSG_<:msgname:>.
 *
 * Member of the following signal groups:
 * - \ref SG_<:sgid:>
 *
 * Received by the ECUs:
 * - \ref ECU_<:ecu:>
 *
 * @ingroup MSG_<:msgname:>
 * @ingroup SG_<:sgid:>
 * @{
 */

/**
 * Signal <:name:> configuration tuple.
 *
 * @deprecated
 *	Use \ref SET_<:name:> and \ref GET_<:name:> instead.
 */
#define SIG_<:name:>              <:motorola:>, <:signed:>, <:sbit:>, <:len:>

/**
 * Signal <:name:> setup tuple.
 *
 * @deprecated
 *	Use \ref INITSIG_<:name:> or \ref INIT_<:msgname:> instead.
 */
#define SETUP_<:name:>            <:motorola:>, <:signed:>, <:sbit:>, <:len:>, <:start:>

/**
 * Get signal <:name:> from buffer.
 *
 * @param buf
 *	The can message buffer containing the signal
 * @return
 *	The raw signal
 */
#define GET_<:name:>(buf) (0 \
	<:getbuf:> \
)

/**
 * Set signal <:name:> in buffer.
 *
 * @param buf
 *	The can message buffer to add the signal to
 * @param val
 *	The raw value to set the signal to
 */
#define SET_<:name:>(buf, val) { \
	<:setbuf:> \
}

/**
 * Set signal <:name:> in buffer to its initial value.
 *
 * @param buf
 *	The can message buffer to initialise
 */
#define INITSIG_<:name:>(buf)     SET_<:name:>(buf, <:start:>)

/**
 * Signal <:name:> value conversion with 16 bit factor and offset.
 *
 * @param x
 *	The raw signal value
 * @param fmt
 *	A factor to adjust values, e.g. 10 to get one additional
 *	digit or 1 / 1000 to dispay a fraction
 * @return
 *	The signal value as a human readable number
 */
#define CALC_<:name:>(x, fmt)     (<:calc16:>)

/**
 * Signal <:name:> raw initial value.
 */
#define START_<:name:>            <:start:>

/**
 * Signal <:name:> raw minimum value.
 */
#define MIN_<:name:>              <:min:>

/**
 * Signal <:name:> raw maximum value.
 */
#define MAX_<:name:>              <:max:>

/**
 * Signal <:name:> raw offset value.
 */
#define OFF_<:name:>              <:off:>

/**
 * @}
 */

