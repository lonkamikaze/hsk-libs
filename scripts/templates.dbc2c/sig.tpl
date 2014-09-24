/**
 * @defgroup SIG_<:id:> Signal <:name:> of Message <:msgname:> (<:msg:%#x:>)
 *
 * <:comment:>
 *<?comment?>
 * <?enum?>Contains the value table \ref ENUM_<:id:>.
 *<?enum?>
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
 *	Use \ref SET_<:id:> and \ref GET_<:id:> instead.
 */
#define SIG_<:id:%-32s:>        <:motorola:>, <:signed:>, <:sbit:>, <:len:>

/**
 * Signal <:name:> setup tuple.
 *
 * @deprecated
 *	Use \ref INITSIG_<:id:> or \ref INIT_<:msgname:> instead.
 */
#define SETUP_<:id:%-32s:>      <:motorola:>, <:signed:>, <:sbit:>, <:len:>, <:start:>

/**
 * Get signal <:name:> from buffer.
 *
 * @param buf
 *	The can message buffer containing the signal
 * @return
 *	The raw signal
 */
#define GET_<:id:>(buf) (0 \
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
#define SET_<:id:>(buf, val) { \
	<:setbuf:> \
}

/**
 * Set signal <:name:> in buffer to its initial value.
 *
 * @param buf
 *	The can message buffer to initialise
 */
#define INITSIG_<:id:>(buf) \
	SET_<:id:>(buf, <:start:>)

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
#define CALC_<:id:>(x, fmt) \
	(<:calc16:>)

/**
 * Signal <:name:> raw initial value.
 */
#define START_<:id:%-32s:>      <:start:>

/**
 * Signal <:name:> raw minimum value.
 */
#define MIN_<:id:%-32s:>        <:min:>

/**
 * Signal <:name:> raw maximum value.
 */
#define MAX_<:id:%-32s:>        <:max:>

/**
 * Signal <:name:> raw offset value.
 */
#define OFF_<:id:%-32s:>        <:off:>

/**
 * @}
 */

