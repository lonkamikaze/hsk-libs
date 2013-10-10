/**
 * @defgroup MSG_<:id:> Message <:name:> (0x<:id:>)
 *
 * <:comment:>
 *
 * Sent by \ref ECU_<:ecu:>.
 *
 * Contains signals:
 * - \ref SIG_<:sig:>
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
 * @}
 */

