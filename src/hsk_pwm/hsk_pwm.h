/** \file
 * HSK Pulse Width Modulation headers
 *
 * This file provides function prototypes to perform Timer T12 and T13
 * based PWM with CCU6.
 *
 * The CCU6 offers the following PWM channels:
 * - PWM_60
 * - PWM_61
 * - PWM_62
 * - PWM_63
 *
 * Each PWM channel is connected to two IO channels for output:
 * - PWM_CCx
 * - PWM_COUTx
 *
 * The distinction between PWM and IO channels is important to understand
 * the side effects of some operations.
 *
 * Refer to the PWM_OUT_x_* defines to know which channel can be connected to
 * which output pins.
 *
 * The functions are implemented under the assumption, that the use of
 * the timers T12 and T13 as well of the CCU6 is exclusive to this library.
 *
 * The safe boot order for pwm output is the following:
 * - hsk_pwm_init()
 * - hsk_pwm_enable()
 * - hsk_pwm_port_open()
 *
 * @author kami
 */

#ifndef _HSK_PWM_H_
#define _HSK_PWM_H_

/**
 * Type definition for PWM channels.
 */
typedef ubyte hsk_pwm_channel;

/**
 * PWM channel 60, Timer T12 driven.
 */
#define PWM_60            0

/**
 * PWM channel 61, Timer T12 driven.
 */
#define PWM_61            1

/**
 * PWM channel 62, Timer T12 driven.
 */
#define PWM_62            2

/**
 * PWM channel 63, Timer T13 driven.
 */
#define PWM_63            3

/**
 * Type definition for output channels.
 */
typedef ubyte hsk_pwm_outChannel;

/**
 * IO channel configuration for PWM_60.
 */
#define PWM_CC60          0

/**
 * Output channel configuration for PWM_60.
 */
#define PWM_COUT60        1

/**
 * IO channel configuration for PWM_61.
 */
#define PWM_CC61          2

/**
 * Output channel configuration for PWM_61.
 */
#define PWM_COUT61        3

/**
 * IO channel configuration for PWM_62.
 */
#define PWM_CC62          4

/**
 * Output channel configuration for PWM_62,
 */
#define PWM_COUT62        5

/**
 * Output channel configuration for PWM_63.
 */
#define PWM_COUT63        6

/**
 * Type definition for ports.
 */
typedef ubyte hsk_pwm_port;

/**
 * PWM_60 output configuration for P3.0 through PWM_CC60.
 */
#define PWM_OUT_60_P30    0

/**
 * PWM_60 output configuration for P3.1 through PWM_COUT60.
 */
#define PWM_OUT_60_P31    1

/**
 * PWM_60 output configuration for P4.0 through PWM_CC60.
 */
#define PWM_OUT_60_P40    2

/**
 * PWM_60 output configuration for P4.1 through PWM_COUT60.
 */
#define PWM_OUT_60_P41    3

/**
 * PWM_61 output configuration for P0.0 through PWM_CC61.
 */
#define PWM_OUT_61_P00    4

/**
 * PWM_61 output configuration for P0.1 through  PWM_COUT61.
 */
#define PWM_OUT_61_P01    5

/**
 * PWM_61 output configuration for P3.1 through  PWM_CC61.
 */
#define PWM_OUT_61_P31    6

/**
 * PWM_61 output configuration for P3.2 through PWM_CC61.
 */
#define PWM_OUT_61_P32    7

/**
 * PWM_61 output configuration for P3.3 through PWM_COUT61.
 */
#define PWM_OUT_61_P33    8

/**
 * PWM_61 output configuration for P4.4 through PWM_CC61.
 */
#define PWM_OUT_61_P44    9

/**
 * PWM_61 output configuration for P4.5 through PWM_COUT61.
 */
#define PWM_OUT_61_P45    10

/**
 * PWM_62 output configuration for P0.4 through PWM_CC62.
 */
#define PWM_OUT_62_P04    11

/**
 * PWM_62 output configuration for P0.5 through PWM_COUT62.
 */
#define PWM_OUT_62_P05    12

/**
 * PWM_62 output configuration for P3.4 through PWM_CC62.
 */
#define PWM_OUT_62_P34    13

/**
 * PWM_62 output configuration for P3.5 through PWM_COUT62.
 */
#define PWM_OUT_62_P35    14

/**
 * PWM_62 output configuration for P4.6 through PWM_CC62.
 */
#define PWM_OUT_62_P46    15

/**
 * PWM_62 output configuration for P4.7 through PWM_COUT62.
 */
#define PWM_OUT_62_P47    16

/**
 * PWM_63 output configuration for P0.3 through PWM_COUT63.
 */
#define PWM_OUT_63_P03    17

/**
 * PWM_63 output configuration for P3.7 through PWM_COUT63.
 */
#define PWM_OUT_63_P37    18

/**
 * PWM_63 output configuration for P4.3 through PWM_COUT63.
 */
#define PWM_OUT_63_P43    19

/**
 * Sets up the the CCU6 timer frequencies that control the PWM
 * cycle.
 *
 * The channels PWM_60, PWM_61 and PWM_62 share the timer T12, thus
 * initializing one of them, initializes them all.
 * The channel PWM_63 has exclusive use of the timer T13 and can thus be used
 * with its own operating frequency.
 *
 * Frequencies up to ~732.4Hz are always between 15 and 16 bits precision.
 *
 * Frequencies above 48kHz offer less than 1/1000 precision.
 * From there it is a linear function, i.e. 480kHz still offer 1/100
 * precision.
 *
 * The freq value 0 will result in ~0.02Hz (\f$48000000 / 2^{31}\f$).
 *
 * The following formula results in the freq value that yields exactly the
 * desired precision, this is useful to avoid precision loss by rounding:
 *	\f[freq(precision) = 480000000 * precision\f]
 *
 * E.g. 10 bit precision: \f$freq(1/2^{10}) = 468750\f$
 *
 * @param channel
 *	The channel to change the frequency for
 * @param freq
 *	The desired PWM cycle frequency in units of 0.1Hz
 */
void hsk_pwm_init(const hsk_pwm_channel channel, const ulong freq);

/**
 * Set up a PWM output port.
 *
 * This configures the necessary port direction bits and activates the
 * corresponding output channels.
 *
 * The port can be any one of the PWM_OUT_x_* defines.
 *
 * @pre
 *	This function should only be called after hsk_pwm_enable(), otherwise
 *	the output port will be driven (1) until PWM is enabled
 * @param port
 *	The output port to activate
 */
void hsk_pwm_port_open(const hsk_pwm_port port);

/**
 * Close a PWM output port.
 *
 * This configures the necessary port direction bits.
 *
 * The port can be any one of the PWM_OUT_x_* defines.
 *
 * @param port
 *	The output port to deactivate
 */
void hsk_pwm_port_close(const hsk_pwm_port port);

/**
 * Set the duty cycle for the given channel.
 *
 * I.e. the active time frame slice of period can be set with max and value.
 *
 * To set the duty cycle in percent specify a max of 100 and values from 0 to
 * 100.
 *
 * @param channel
 *	The PWM channel to set the duty cycle for, check the PWM_6x defines
 * @param max
 *	Defines the scope value can move in
 * @param value
 *	The current duty cycle value
 */
void hsk_pwm_channel_set(const hsk_pwm_channel channel,
                         const uword max, const uword value);

/**
 * Set the direction of an output channel.
 *
 * The channel value can be taken from any of the PWM_CCx/PWM_COUTx defines.
 *
 * @param channel
 *	The IO channel to set the direction bit for
 * @param up
 *	Set 1 to output a 1 during the cycle set with hsk_pwm_channel_set(),
 *	set 0 to output a 0 during the cycle set with hsk_pwm_channel_set()
 */
void hsk_pwm_outChannel_dir(hsk_pwm_outChannel channel,
                            const bool up);

/**
 * Turns on the CCU6.
 *
 * Deactivates the power disable mode and sets the T12 and T13 Timer Run bits.
 *
 * @pre
 *	All hsk_pwm_init() calls have to be completed to call this
 */
void hsk_pwm_enable(void);

/**
 * Deactivates the CCU6 to reduce power consumption.
 */
void hsk_pwm_disable(void);

#endif /* _HSK_PWM_H_ */
