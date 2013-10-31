/** \file
 * HSK Controller Area Network headers
 *
 * This file contains the function prototypes to initialize and engage in
 * CAN communication over the builtin CAN nodes 0 and 1.
 *
 * @author kami
 *
 * \section tuples CAN Message/Signal Tuples
 *
 * The recommended way to use messages and signals is not to specify them
 * inline, but to provide defines with a set of parameters.
 *
 * These tupples should follow the following pattern:
 * \code
 * #define MSG_<MSGNAME>    <id>, <extended>, <dlc>
 * #define SIG_<SIGNAME>    <endian>, <bitPos>, <bitCount>
 * \endcode
 *
 * The symbols have the following meaning:
 * - MSGNAME: The name of the message in capitals, e.g. AFB_CHANNELS
 * - id: The CAN id of the message, e.g. 0x403
 * - extended: Whether the CAN ID is extended or not, e.g. 0 for a
 *   regular ID
 * - dlc: The data length count of the message, e.g. 3
 * - SIGNAME: The name of the signal in capitals, e.g.
 *   AFB_CHANNEL0_CURRENT
 * - endian: Whether the signal is in little or big endian format,
 *   e.g. CAN_ENDIAN_INTEL
 * - bitPos: The starting bit of the signal, e.g. 0
 * - bitCount: The length of the signal in bits, e.g. 10
 *
 * Tuples using the specified format can directly be used as parameters
 * for several functions in the library.
 */

#ifndef _HSK_CAN_H_
#define _HSK_CAN_H_

/**
 * Value returned by functions in case of an error.
 */
#define CAN_ERROR              0xff

/**
 * CAN node 0.
 */
#define CAN0                   0

/**
 * CAN node 1.
 */
#define CAN1                   1

/**
 * CAN node 0 IO RX on P1.0, TX on P1.1.
 */
#define CAN0_IO_P10_P11        0

/**
 * CAN node 0 IO RX on P1.6, TX on P1.7.
 */
#define CAN0_IO_P16_P17        1

/**
 * CAN node 0 IO RX on P3.4, TX on P3.5.
 */
#define CAN0_IO_P34_P35        2

/**
 * CAN node 0 IO RX on P4.0, TX on P4.1.
 */
#define CAN0_IO_P40_P41        3

/**
 * CAN node 1 IO RX on P0.1, TX on P0.2.
 */
#define CAN1_IO_P01_P02        4

/**
 * CAN node 1 IO RX on P1.4, TX on P1.3.
 */
#define CAN1_IO_P14_P13        5

/**
 * CAN node 1 IO RX on P3.2, TX on P3.3.
 */
#define CAN1_IO_P32_P33        6

/**
 * Little endian signal encoding.
 */
#define CAN_ENDIAN_INTEL       0

/**
 * Big endian signal encoding.
 */
#define CAN_ENDIAN_MOTOROLA    1

/**
 * CAN node identifiers.
 */
typedef ubyte hsk_can_node;

/**
 * CAN message object identifiers.
 */
typedef ubyte hsk_can_msg;

/**
 * CAN message FIFO identifiers.
 */
typedef ubyte hsk_can_fifo;

/** \file
 * \section nodes CAN Node Management
 *
 * There are 7 port pairs available for CAN communication, check the CANn_IO_*
 * defines. Four for the node CAN0 and three for CAN1.
 */

/**
 * Setup CAN communication with the desired baud rate.
 *
 * The CAN node is chosen with the pin configuration.
 *
 * The bus still needs to be enabled after being setup.
 *
 * @param pins
 *	Choose one of 7 CANn_IO_* configurations
 * @param baud
 *	The target baud rate to use
 */
void hsk_can_init(const ubyte pins, const ulong __xdata baud);

/**
 * Go live on the CAN bus.
 *
 * To be called when everything is set up.
 *
 * @param node
 *	The CAN node to enable
 */
void hsk_can_enable(const hsk_can_node node);

/**
 * Disable a CAN node.
 *
 * This completely shuts down a CAN node, cutting it off from the
 * internal clock, to reduce energy consumption.
 *
 * @param node
 *	The CAN node to disable
 */
void hsk_can_disable(const hsk_can_node node);

/**
 * \defgroup CAN_STATUS CAN Node Status Fields
 *
 * This group of defines specifies status fields that can be queried from
 * hsk_can_status().
 *
 * @{
 */

/**
 * The Last Error Code field provides the error triggered by the last message
 * on the bus.
 *
 * For details check table 16-8 from the User Manual 1.1.
 *
 * @retval 0
 *	No Error
 * @retval 1
 *	Stuff Error, 5 consecutive bits of the same value are stuffed, this
 *	error is triggered when the stuff bit is missing
 * @retval 2
 *	Form Error, the frame format was violated
 * @retval 3
 *	Ack Error, the message was not acknowledged, maybe nobody else
 *	is on the bus
 * @retval 4
 *	Bit1 Error, a recessive (1) bit was sent out of sync
 * @retval 5
 *	Bit0 Error, a recessive (1) bit won against a dominant (0) bit
 * @retval 6
 *	CRC Error, wrong checksum for a received message
 */
#define CAN_STATUS_LEC         0

/**
 * Message Transmitted Successfully.
 *
 * @retval 0
 *	No successful transmission since TXOK was queried last time
 * @retval 1
 *	A message was transmitted and acknowledged successfully
 */
#define CAN_STATUS_TXOK        1

/**
 * Message Received Successfully.
 *
 * @retval 0
 *	No successful receptions since the last time this field was queried
 * @retval 1
 *	A message was received successfully
 */
#define CAN_STATUS_RXOK        2

/**
 * Alert Warning.
 *
 * @retval 0
 *	No warnings
 * @retval 1
 *	One of the following error conditions applies: \ref CAN_STATUS_EWRN;
 *	\ref CAN_STATUS_BOFF
 */
#define CAN_STATUS_ALERT       3

/**
 * Error Warning Status.
 *
 * @retval 0
 *	No error warnings exceeded
 * @retval 1
 *	An error counter has exceeded the warning level of 96
 */
#define CAN_STATUS_EWRN        4

/**
 * Bus-off Status
 *
 * @retval 0
 *	The bus is not off
 * @retval 1
 *	The bus is turned off due to an error counter exceeding 256
 */
#define CAN_STATUS_BOFF        5

/**
 * @}
 */

/**
 * Returns a status field of a CAN node.
 *
 * @param node
 *	The CAN node to return the status of
 * @param field
 *	The status field to select
 * @return
 *	The status field state
 * @see \ref CAN_STATUS
 */
ubyte hsk_can_status(const hsk_can_node node, const ubyte field);

/** \file
 * \section CAN Message Object Management
 *
 * The MultiCAN module offers up to 32 message objects. New messages are set
 * up for receiving messages. Message object can be switched from RX to TX
 * mode and back with the hsk_can_msg_send() and hsk_can_msg_receive()
 * functions.
 */

/**
 * Creates a new CAN message.
 *
 * Note that only up to 32 messages can exist at any given time.
 *
 * Extended messages have 29 bit IDs and non-extended 11 bit IDs.
 *
 * @param id
 *	The message ID.
 * @param extended
 *	Set this to 1 for an extended CAN message.
 * @param dlc
 *	The data length code, # of bytes in the message, valid values
 *	range from 0 to 8.
 * @retval CAN_ERROR
 *	Creating the message failed
 * @retval [0;32[
 *	A message identifier
 */
hsk_can_msg hsk_can_msg_create(const ulong id, const bool extended,
                               const ubyte dlc);

/**
 * Connect a message object to a CAN node.
 *
 * @param msg
 *	The identifier of the message object
 * @param node
 *	The CAN node to connect to
 * @retval CAN_ERROR
 *	The given message is not valid
 * @retval 0
 *	Success
 */
ubyte hsk_can_msg_connect(const hsk_can_msg msg,
                          const hsk_can_node __xdata node);

/**
 * Disconnect a CAN message object from its CAN node.
 *
 * This takes a CAN message out of active communication, without deleting
 * it.
 *
 * @param msg
 *	The identifier of the message object
 * @retval CAN_ERROR
 *	The given message is not valid
 * @retval 0
 *	Success
 */
ubyte hsk_can_msg_disconnect(const hsk_can_msg msg);

/**
 * Delete a CAN message object.
 *
 * @param msg
 *	The identifier of the message object
 * @retval CAN_ERROR
 *	The given message is not valid
 * @retval 0
 *	Success
 */
ubyte hsk_can_msg_delete(const hsk_can_msg msg);

/**
 * Gets the current data in the CAN message.
 *
 * This writes DLC bytes from the CAN message object into msgdata.
 *
 * @param msg
 *	The identifier of the message object
 * @param msgdata
 *	The character array to store the message data in
 */
void hsk_can_msg_getData(const hsk_can_msg msg,
                         ubyte * const msgdata);

/**
 * Sets the current data in the CAN message.
 *
 * This writes DLC bytes from msgdata to the CAN message object.
 *
 * @param msg
 *	The identifier of the message object
 * @param msgdata
 *	The character array to get the message data from
 */
void hsk_can_msg_setData(const hsk_can_msg msg,
                         const ubyte * const msgdata);

/**
 * Request transmission of a message.
 *
 * @param msg
 *	The identifier of the message to send
 */
void hsk_can_msg_send(const hsk_can_msg msg);


/**
 * Return whether the message was successfully sent between this and the
 * previous call of this method.
 *
 * @param msg
 *	The identifier of the message to check
 * @retval 1
 *	The message was sent since the last call of this function
 * @retval 0
 *	The message has not been sent since the last call of this function
 */
bool hsk_can_msg_sent(const hsk_can_msg msg);

/**
 * Return the message into RX mode after sending a message.
 *
 * After sending a message the messages with the same ID from other
 * bus participants are ignored. This restores the original setting to receive
 * messages.
 *
 * @param msg
 *	The identifier of the message to receive
 */
void hsk_can_msg_receive(const hsk_can_msg msg);

/**
 * Return whether the message was updated via CAN bus between this call and
 * the previous call of this method.
 *
 * An update does not entail a change of message data. It just means the
 * message was received on the CAN bus.
 *
 * This is useful for cyclic message occurance checks.
 *
 * @param msg
 *	The identifier of the message to check
 * @retval 1
 *	The message was updated since the last call of this function
 * @retval 0
 *	The message has not been updated since the last call of this function
 */
bool hsk_can_msg_updated(const hsk_can_msg msg);

/** \file
 * \section fifos FIFOs
 *
 * FIFOs are the weapon of choice when dealing with large numbers of
 * individual messages or when receving multiplexed data. In most use cases
 * only the latest version of a message is relevant and FIFOs are not
 * required. But messages containing multiplexed signals may contain critical
 * signals that would be overwritten by a message with the same ID, but a
 * different multiplexor.
 *
 * If more message IDs than available message objects are used to send and/or
 * receive data, there is no choice but to use a FIFO.
 *
 * Currently only RX FIFOs are supported.
 *
 * A FIFO can act as a buffer the CAN module can store message data in until
 * it can be dealt with. The following example illustrates how to read from
 * a FIFO:
 * \code
 * if (hsk_can_fifo_updated(fifo0)) {
 * 	hsk_can_fifo_getData(fifo0, data0);
 * 	hsk_can_fifo_next(fifo0);
 * 	select = hsk_can_data_getSignal(data0, SIG_MULTIPLEXOR);
 * 	[...]
 * }
 * \endcode
 *
 * When using a mask to accept several messages checking the ID becomes
 * necessary:
 * \code
 * if (hsk_can_fifo_updated(fifo0)) {
 * 	switch (hsk_can_fifo_getId()) {
 * 	case MSG_0_ID:
 * 		hsk_can_fifo_getData(fifo0, data0);
 * 		[...]
 * 		break;
 * 	case MSG_1_ID:
 * 		hsk_can_fifo_getData(fifo0, data1);
 * 		[...]
 * 		break;
 * 	[...]
 * 	}
 * 	hsk_can_fifo_next(fifo0);
 * }
 * \endcode
 *
 *
 * FIFOs draw from the same message object pool regular message objects do.
 */

/**
 * Creates a message FIFO.
 *
 * FIFOs can be used to ensure that multiplexed signals are not lost.
 *
 * For receiving multiplexed signals it is recommended to use a FIFO as large
 * as the number of multiplexed messages that might occur in a single burst.
 *
 * If the multiplexor is large, e.g. 8 bits, it's obviously not possible to
 * carve a 256 messages FIFO out of 32 message objects. Make an educated
 * guess and hope that the signal provider is not hostile.
 *
 * If the number of available message objects is at least one, but less than
 * the requested length this function succeeds, but the FIFO is only created
 * as long as possible.
 *
 * @param size
 *	The desired FIFO size
 * @retval CAN_ERROR
 *	Creating the FIFO failed
 * @retval [0;32[
 *	The created FIFO id
 */
hsk_can_fifo hsk_can_fifo_create(ubyte size);

/**
 * Set the FIFO up for receiving messages.
 *
 * @param fifo
 *	The FIFO to setup
 * @param id
 *	The message ID.
 * @param extended
 *	Set this to 1 for an extended CAN message
 * @param dlc
 *	The data length code, # of bytes in the message, valid values
 *	range from 0 to 8
 */
void hsk_can_fifo_setupRx(hsk_can_fifo fifo, const ulong id,
                          const bool extended, const ubyte dlc);

/**
 * Changes the ID matching mask of an RX FIFO.
 *
 * Every RX FIFO is setup to receive only on complete ID matches. This
 * function allows updating the mask.
 *
 * To generate a mask from a list of IDs use the following formula:
 *	\f[ msk = \sim(id_0 | id_1 | ... | id_n) | (id_0 \& id_1 \& ... \& id_n)  \f]
 *
 * @pre hsk_can_fifo_setupRx()
 * @param fifo
 *	The FIFO to change the RX mask for
 * @param msk
 *	The bit mask to set for the FIFO
 */
void hsk_can_fifo_setRxMask(const hsk_can_fifo fifo, ulong msk);

/**
 * Connect a FIFO to a CAN node.
 *
 * @param fifo
 *	The identifier of the FIFO
 * @param node
 *	The CAN node to connect to
 * @retval CAN_ERROR
 *	The given FIFO is not valid
 * @retval 0
 *	Success
 */
ubyte hsk_can_fifo_connect(const hsk_can_fifo fifo,
                           const hsk_can_node __xdata node);

/**
 * Disconnect a FIFO from its CAN node.
 *
 * This takes the FIFO out of active communication, without deleting
 * it.
 *
 * @param fifo
 *	The identifier of the FIFO
 * @retval CAN_ERROR
 *	The given FIFO is not valid
 * @retval 0
 *	Success
 */
ubyte hsk_can_fifo_disconnect(const hsk_can_fifo fifo);

/**
 * Delete a FIFO.
 *
 * @param fifo
 *	The identifier of the FIFO
 * @retval CAN_ERROR
 *	The given FIFO is not valid
 * @retval 0
 *	Success
 */
ubyte hsk_can_fifo_delete(const hsk_can_fifo fifo);

/**
 * Select the next FIFO entry.
 *
 * The hsk_can_fifo_updated() and hsk_can_fifo_getData() functions always
 * refer to a certain message within the FIFO. This function selects the
 * next entry.
 *
 * @param fifo
 *	The ID of the FIFO to select the next entry from
 */
void hsk_can_fifo_next(const hsk_can_fifo fifo);

/**
 * Returns the CAN ID of the selected FIFO entry.
 *
 * @param fifo
 *	The ID of the FIFO
 * @return
 *	The ID of the currently selected message object
 */
ulong hsk_can_fifo_getId(const hsk_can_fifo fifo);

/**
 * Return whether the currently selected FIFO entry was updated via CAN bus
 * between this call and the previous call of this method.
 *
 * It can be used to decide when to call hsk_can_fifo_getData() and
 * hsk_can_fifo_next().
 *
 * @param fifo
 *	The identifier of the FIFO to check
 * @retval 1
 *	The FIFO entry was updated since the last call of this function
 * @retval 0
 *	The FIFO entry has not been updated since the last call of this function
 */
bool hsk_can_fifo_updated(const hsk_can_fifo fifo);

/**
 * Gets the data from the currently selected FIFO entry.
 *
 * This writes DLC bytes from the FIFO entry into msgdata.
 *
 * @param fifo
 *	The identifier of the FIFO
 * @param msgdata
 *	The character array to store the message data in
 */
void hsk_can_fifo_getData(const hsk_can_fifo fifo,
                          ubyte * const msgdata);

/** \file
 * \section data Message Data
 *
 * The hsk_can_data_setSignal() and hsk_can_data_getSignal() functions allow
 * writing and reading signals across byte boundaries to and from a buffer.
 *
 * For big endian signals the bit position of the most significant bit must
 * be supplied (highest bit in the first byte). For little endian signals
 * the least significant bit must be supplied (lowest bit in the first byte).
 *
 * This conforms to the  way signal positions are stored in Vector CANdb++
 * DBC files.
 */

/**
 * Sets a signal value in a data field.
 *
 * @param msg
 *	The message data field to write into
 * @param endian
 *	Little or big endian encoding
 * @param sign
 *	Indicates whether the value has a signed type
 * @param bitPos
 *	The bit position of the signal
 * @param bitCount
 *	The length of the signal
 * @param value
 *	The signal value to write into the data field
 */
void hsk_can_data_setSignal(ubyte * const msg, const bool endian,
                            const bool sign, const ubyte bitPos,
                            const char bitCount, const ulong idata value);

/**
 * Get a signal value from a data field.
 *
 * @param msg
 *	The message data field to read from
 * @param endian
 *	Little or big endian encoding
 * @param sign
 *	Indicates whether the value has a signed type
 * @param bitPos
 *	The bit position of the signal
 * @param bitCount
 *	The length of the signal
 * @return
 *	The signal from the data field msg
 */
ulong hsk_can_data_getSignal(const ubyte * const msg, const bool endian,
                             const bool sign, const ubyte bitPos,
                             const char bitCount);

#endif /* _HSK_CAN_H_ */
