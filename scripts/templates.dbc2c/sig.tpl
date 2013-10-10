/**
 * @defgroup SIG_<:name:> Signal <:name:>
 *
 * <:comment:>
 *
 * Sent in message \ref MSG_<:msgid:>.
 *
 * Member of the following signal groups:
 * - \ref SG_<:sgid:>
 *
 * Received by the ECUs:
 * - \ref ECU_<:ecu:>
 *
 * @ingroup MSG_<:msgid:>
 * @ingroup SG_<:sgid:>
 * @{
 */

/**
 * Siganal <:name:> configuration tuple.
 */
#define SIG_<:name:>             CAN_ENDIAN_<:endian:>, <:signed:>, <:sbit:>, <:len:>

/**
 * Signal <:name:> setup tuple.
 */
#define SETUP_<:name:>           CAN_ENDIAN_<:endian:>, <:signed:>, <:sbit:>, <:len:>, <:start:>

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
#define CALC_<:name:>(x, fmt)    (<:calc16:>)

/**
 * Signal <:name:> raw initial value.
 */
#define START_<:name:>           <:start:>

/**
 * Signal <:name:> raw minimum value.
 */
#define MIN_<:name:>             <:min:>

/**
 * Signal <:name:> raw maximum value.
 */
#define MAX_<:name:>             <:max:>

/**
 * Signal <:name:> raw offset value.
 */
#define OFF_<:name:>             <:off:>

/**
 * @}
 */

