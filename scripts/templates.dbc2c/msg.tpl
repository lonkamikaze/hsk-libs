/**
 * @defgroup MSG_<:name:> Message <:name:> (0x<:id:>)
 *
 * <:comment:>
 *<?comment?>
 * Sent by \ref ECU_<:ecu:>.
 *<?ecu?>
 * Contains signal groups:
 * - \ref SG_<:sgid:>
 *
 * Contains signals:
 * - \ref SIG_<:sigid:>
 *
 * @ingroup ECU_<:ecu:>
 * @{
 */

/**
 * Message <:name:> configuration tuple.
 */
#define MSG_<:name:>        0x<:id:>, <:ext:>, <:dlc:>

/**
 * Message <:name:> id.
 */
#define ID_<:name:>         0x<:id:>

/**
 * Message <:name:> Data Length Count.
 */
#define DLC_<:name:>        <:dlc:>

/**
 * Message <:name:> cycle time.
 */
#define CYCLE_<:name:>      <:cycle:>

/**
 * Message <:name:> fast cycle time.
 */
#define FAST_<:name:>       <:fast:>

/**
 * Initialise message <:name:> buffer.
 *
 * @param buf
 *	The can message buffer to initialise
 */
#define INIT_<:name:>(buf) { \
	INITSIG_<:sigid:>(buf); \
}

/**
 * @}
 */

