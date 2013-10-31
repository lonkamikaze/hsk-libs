/** \file
 * HSK Pulse Width Counter headers
 *
 * This library uses the T2CCU to measure pulse width on the external
 * interrupt pins.
 *
 * Every caputre channel blocks an external interrupt. Opening a channel
 * will block this interrupt and change its configuration.
 *
 * Pulse with measurement has a window time that is configured with
 * hsk_pwc_init() and defines the time frame within which pulses can be
 * detected.
 *
 * If no pulse occurs during the window, the channel buffer is invalidated
 * and the hsk_pwc_channel_getValue() function will returns invalid (0) until
 * the buffer is repopulated with valid measurements.
 *
 * In order to guarantee the detection of invalid channels, the
 * hsk_pwc_channel_getValue() function has to be called at least once every
 * 256 window times.
 *
 * @author kami
 */

#ifndef _HSK_PWC_H_
#define _HSK_PWC_H_

/*
 * Required for SDCC to propagate ISR prototypes.
 */
#ifdef SDCC
#include "../hsk_isr/hsk_isr.isr"
#endif /* SDCC */

/**
 * Typedef for PWC channel IDs.
 */
typedef ubyte hsk_pwc_channel;

/**
 * Capture/Compare channel 0 on EXINT3.
 */
#define PWC_CC0               0

/**
 * Capture/Compare channel 1 on EXINT4.
 */
#define PWC_CC1               1

/**
 * Capture/Compare channel 2 on EXINT5.
 */
#define PWC_CC2               2

/**
 * Capture/Compare channel 3 on EXINT6.
 */
#define PWC_CC3               3

/**
 * Typedef for PWC input port.
 */
typedef ubyte hsk_pwc_port;

/**
 * Capture/Compare channel 0 input port P3.0 configuration.
 */
#define PWC_CC0_P30           0

/**
 * Capture/Compare channel 0 input port P4.0 configuration.
 */
#define PWC_CC0_P40           1

/**
 * Capture/Compare channel 0 input port P5.5 configuration.
 */
#define PWC_CC0_P55           2

/**
 * Capture/Compare channel 1 input port P3.2 configuration.
 */
#define PWC_CC1_P32           3

/**
 * Capture/Compare channel 1 input port P4.1 configuration.
 */
#define PWC_CC1_P41           4

/**
 * Capture/Compare channel 1 input port P5.6 configuration.
 */
#define PWC_CC1_P56           5

/**
 * Capture/Compare channel 2 input port P3.3 configuration.
 */
#define PWC_CC2_P33           6

/**
 * Capture/Compare channel 4 input port P4.4 configuration.
 */
#define PWC_CC2_P44           7

/**
 * Capture/Compare channel 2 input port P5.2 configuration.
 */
#define PWC_CC2_P52           8

/**
 * Capture/Compare channel 3 input port P3.4 configuration.
 */
#define PWC_CC3_P34           9

/**
 * Capture/Compare channel 3 input port P4.5 configuration.
 */
#define PWC_CC3_P45           10

/**
 * Capture/Compare channel 3 input port P5.7 configuration.
 */
#define PWC_CC3_P57           11

/**
 * Configuration selection to trigger pulse detection on falling edge.
 */
#define PWC_EDGE_FALLING      0

/**
 * Configuration selection to trigger pulse detection on rising edge.
 */
#define PWC_EDGE_RISING       1

/**
 * Configuration selection to trigger pulse detection on both edges.
 */
#define PWC_EDGE_BOTH         2

/**
 * Available capture modes, capture on external interrupt.
 */
#define PWC_MODE_EXT          1

/**
 * Available capture modes, capture on sofware event.
 */
#define PWC_MODE_SOFT         3

/**
 * This function initializes the T2CCU Capture/Compare Unit for capture mode.
 *
 * The capturing is based on the CCT timer. Timer T2 is not used and thus can
 * be useed without interference.
 *
 * The window time is the time frame within which pulses should be detected.
 * A smaller time frame results in higher precission, but detection of longer
 * pulses will fail.
 *
 * Window times vary between ~1ms (\f$(2^{16} - 1) / (48 * 10^6)\f$) and ~5592ms
 * (\f$(2^{16} - 1) * 2^{12} / (48 * 10^6)\f$). The shortest window time delivers
 * ~20ns and the longest time ~85µs precision.
 *
 * The real window time is on a logarithmic scale (base 2), the init function
 * will select the lowest scale that guarantees the required window time.
 * I.e. the highest precision possible with the desired window time, which is
 * at least \f$2^{15}\f$ for all windows below or equal 5592ms.
 *
 * @param window
 *	The time in ms to detect a pulse.
 */
void hsk_pwc_init(ulong window);

/**
 * Configures a PWC channel without an input port.
 *
 * The channel is set up for software triggering (PWC_MODE_SOFT), and
 * triggering on both edges (PWC_EDGE_BOTH).
 *
 * @param channel
 *	The PWC channel to open
 * @param averageOver
 *	The number of pulse values to average over when returning a
 *	value or speed. The value must be between 1 and 8.
 */
void hsk_pwc_channel_open(const hsk_pwc_channel channel,
                          ubyte __xdata averageOver);

/**
 * Opens an input port and the connected channel.
 *
 * The available configurations are available from the PWC_CCn_* defines.
 *
 * @param port
 *	The input port to open
 * @param averageOver
 *	The number of pulse values to average over when returning a
 *	value or speed. The value must be between 1 and CHAN_BUF_SIZE
 */
void hsk_pwc_port_open(const hsk_pwc_port port,
                       ubyte __xdata averageOver);

/**
 * Close a PWC channel.
 *
 * @param channel
 *	The channel to close.
 */
void hsk_pwc_channel_close(const hsk_pwc_channel channel);

/**
 * Select the edge that is used to detect a pulse.
 *
 * Available edges are specified in the PWC_EDGE_* defines.
 *
 * @param channel
 *	The channel to configure the edge for.
 * @param edgeMode
 *	The selected edge detection mode.
 */
void hsk_pwc_channel_edgeMode(const hsk_pwc_channel channel,
                              const ubyte edgeMode);

/**
 * Allows switching between external and soft trigger.
 *
 * This does not reconfigure the input ports. Available modes are specified
 * in the PWC_MODE_* defines. PWC_MODE_EXT is the default.
 *
 * @param channel
 *	The channel to configure.
 * @param captureMode
 *	The mode to set the channel to.
 */
void hsk_pwc_channel_captureMode(const hsk_pwc_channel channel,
                                 const ubyte captureMode);

/**
 * Triggers a channel in soft trigger mode.
 *
 * @param channel
 *	The channel to trigger.
 */
void hsk_pwc_channel_trigger(const hsk_pwc_channel channel);

/**
 * Enables T2CCU module if disabled.
 */
void hsk_pwc_enable(void);

/**
 * Turns off the T2CCU clock to preserve power.
 */
void hsk_pwc_disable(void);


/**
 * \defgroup PWC_UNIT Pulse Width Detection Units
 *
 * This group of defines is used to select return format of
 * hsk_pwc_channel_getValue().
 *
 * @{
 */

/**
 * Sum of buffered pulse widths in multiples of \f$ 1/48 * 10^{-6} s \f$.
 *
 * This is the sum of the buffered values, not the average.
 *
 * Use this if precision is of the utmost importance.
 */
#define PWC_UNIT_SUM_RAW      0

/**
 * \defgroup PWC_UNIT_WIDTH Pulse Width Times
 *
 * The defines are for returning average pulse width.
 *
 * @{
 */

/**
 * Average of buffered pulse widths in multiples of \f$ 1/48 * 10^{-6} s \f$.
 */
#define PWC_UNIT_WIDTH_RAW    1

/**
 * Average of buffered pulse widths in multiples of \f$ 10^{-9} s \f$.
 */
#define PWC_UNIT_WIDTH_NS     2

/**
 * Average of buffered pulse widths in multiples of \f$ 10^{-6} s \f$.
 */
#define PWC_UNIT_WIDTH_US     3

/**
 * Average of buffered pulse widths in multiples of \f$ 10^{-3} s \f$.
 */
#define PWC_UNIT_WIDTH_MS     4

/**
 * @}
 */

/**
 * \defgroup PWC_UNIT_FREQ Pulse Frequencies
 *
 * These defines are for returning average frequencies.
 *
 * @{
 */

/**
 * Average frequency of buffered pulses in multiples of \f$ 1/s \f$.
 */
#define PWC_UNIT_FREQ_S       5

/**
 * Average frequency of buffered pulses in multiples of \f$ 1/m \f$.
 *
 * To prevent overflow issues this value is always a multiple of the number
 * of averaged values.
 */
#define PWC_UNIT_FREQ_M       6

/**
 * Average frequency of buffered pulses in multiples of \f$ 1/h \f$.
 *
 * To prevent overflow issues this value is always a multiple of the number
 * of averaged values * 60.
 *
 * This is just a convenience feature for quick testing, it is possible to
 * achieve much better precision if the use case is known.
 */
#define PWC_UNIT_FREQ_H       7

/**
 * @}
 */

/**
 * \defgroup PWC_UNIT_DUTY Pulse Duty Times
 *
 * These defines are used for returning the duty time of the latest pulse.
 *
 * In order to use this return type, the channel buffer must hold at least
 * 2 values. I.e. the averageOver argument of hsk_pwc_port_open() must
 * be 2 or greater (there is no benefit to a value above 2).
 *
 * To produce correct results the channel must also be in edge mode
 * \ref PWC_EDGE_BOTH.
 *
 * @{
 */

/**
 * Latest high pulse in multiples of \f$ 1/48 * 10^{-6} s \f$.
 */
#define PWC_UNIT_DUTYH_RAW    8

/**
 * Latest high pulse in multiples of \f$ 1 * 10^{-9} s \f$.
 */
#define PWC_UNIT_DUTYH_NS     9

/**
 * Latest high pulse in multiples of \f$ 1 * 10^{-6} s \f$.
 */
#define PWC_UNIT_DUTYH_US     10

/**
 * Latest high pulse in multiples of \f$ 1 * 10^{-3} s \f$.
 */
#define PWC_UNIT_DUTYH_MS     11

/**
 * Latest low pulse in multiples of \f$ 1/48 * 10^{-6} s \f$.
 */
#define PWC_UNIT_DUTYL_RAW    12

/**
 * Latest low pulse in multiples of \f$ 1 * 10^{-9} s \f$.
 */
#define PWC_UNIT_DUTYL_NS     13

/**
 * Latest low pulse in multiples of \f$ 1 * 10^{-6} s \f$.
 */
#define PWC_UNIT_DUTYL_US     14

/**
 * Latest low pulse in multiples of \f$ 1 * 10^{-3} s \f$.
 */
#define PWC_UNIT_DUTYL_MS     15

/**
 * @}
 */

/**
 * @}
 */

/**
 * Returns a measure of the values in a channel buffer.
 *
 * It also takes care of invalidating channels that haven't been captured
 * for too long.
 *
 * The value is returned in a requested unit, the units defined as
 * PWC_UNIT_* are available.
 *
 * @param channel
 *	The channel to return the buffer sum of
 * @param unit
 '	The unit to return the channel value in
 * @retval >0
 *	The channel value in the requested unit
 * @retval 0
 *	Invalid channel, measurement timed out
 */
ulong hsk_pwc_channel_getValue(const hsk_pwc_channel channel,
                               const ubyte unit);

#endif /* _HSK_PWC_H_ */

