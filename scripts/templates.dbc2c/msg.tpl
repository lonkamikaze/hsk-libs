/**
 * @defgroup MSG_<:name:> Message <:name:> (<:msg:%#x:>)
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
#define MSG_<:name:%-32s:>        <:msg:%#x:>, <:ext:>, <:dlc:>

/**
 * Message <:name:> id.
 */
#define ID_<:name:%-32s:>         <:msg:%#x:>

/**
 * Message <:name:> extended id bit.
 */
#define EXT_<:name:%-32s:>        <:ext:>

/**
 * Message <:name:> Data Length Count.
 */
#define DLC_<:name:%-32s:>        <:dlc:>

/**
 * Message <:name:> cycle time.
 */
#define CYCLE_<:name:%-32s:>      <:cycle:>

/**
 * Message <:name:> fast cycle time.
 */
#define FAST_<:name:%-32s:>       <:fast:>

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

