/** \file
 * HSK Controller Area Network headers
 *
 * This file contains the function prototypes to initialize and engage in
 * CAN communication over the builtin CAN nodes 0 and 1.
 *
 * @author kami
 * @version 2012-03-14
 */

#ifndef _HSK_CAN_H_
#define _HSK_CAN_H_

/**
 * Value returned by functions in case of an error.
 */
#define	CAN_ERROR		0xff

/**
 * CAN node 0.
 */
#define CAN0			0

/**
 * CAN node 1.
 */
#define CAN1			1

/**
 * CAN node 0 IO RX on P1.0, TX on P1.1.
 */
#define CAN0_IO_P10_P11		0

/**
 * CAN node 0 IO RX on P1.6, TX on P1.7.
 */
#define CAN0_IO_P16_P17		1

/**
 * CAN node 0 IO RX on P3.4, TX on P3.5.
 */
#define CAN0_IO_P34_P35		2

/**
 * CAN node 0 IO RX on P4.0, TX on P4.1.
 */
#define CAN0_IO_P40_P41		3

/**
 * CAN node 1 IO RX on P0.1, TX on P0.2.
 */
#define CAN1_IO_P01_P02		4

/**
 * CAN node 1 IO RX on P1.4, TX on P1.3.
 */
#define CAN1_IO_P14_P13		5

/**
 * CAN node 1 IO RX on P3.2, TX on P3.3.
 */
#define CAN1_IO_P32_P33		6

/**
 * Little endian signal encoding.
 */
#define CAN_ENDIAN_INTEL	0

/**
 * Big endian signal encoding.
 */
#define CAN_ENDIAN_MOTOROLA	1

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
 * 	Choose one of 7 CANn_IO_* configurations
 * @param baud
 * 	The target baud rate to use
 */
void hsk_can_init(ubyte idata pins, ulong idata baud);

/**
 * Go live on the CAN bus.
 *
 * To be called when everything is set up.
 *
 * @param node
 * 	The CAN node to enable
 */
void hsk_can_enable(hsk_can_node idata node);

/**
 * Disable a CAN node.
 *
 * This completely shuts down a CAN node, cutting it off from the
 * internal clock, to reduce energy consumption.
 *
 * @param node
 * 	The CAN node to disable
 */
void hsk_can_disable(hsk_can_node idata node);

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
 * 	The message ID.
 * @param extended
 * 	Set this to 1 for an extended CAN message.
 * @param dlc
 * 	The data length code, # of bytes in the message, valid values
 * 	range from 0 to 8.
 * @retval CAN_ERROR
 *	Creating the message failed
 * @retval [0;32[
 *	A message identifier
 */
hsk_can_msg hsk_can_msg_create(ulong idata id, bool extended,
	ubyte idata dlc);

/**
 * Connect a message object to a CAN node.
 *
 * @param msg
 * 	The identifier of the message object
 * @param node
 * 	The CAN node to connect to
 * @retval CAN_ERROR
 *	The given message is not valid
 * @retval 0
 *	Success
 */
ubyte hsk_can_msg_connect(hsk_can_msg idata msg, hsk_can_node idata node);

/**
 * Disconnect a CAN message object from its CAN node.
 *
 * This takes a CAN message out of active communication, without deleting
 * it.
 *
 * @param msg
 * 	The identifier of the message object
 * @retval CAN_ERROR
 *	The given message is not valid
 * @retval 0
 *	Success
 */
ubyte hsk_can_msg_disconnect(hsk_can_msg idata msg);

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
ubyte hsk_can_msg_delete(hsk_can_msg idata msg);

/**
 * Gets the current data in the CAN message.
 *
 * This writes DLC bytes from the CAN message object into msgdata.
 *
 * @param msg
 * 	The identifier of the message object
 * @param msgdata
 * 	The character array to store the message data in
 */
void hsk_can_msg_getData(hsk_can_msg idata msg, ubyte * idata msgdata);

/**
 * Sets the current data in the CAN message.
 *
 * This writes DLC bytes from msgdata to the CAN message object.
 *
 * @param msg
 * 	The identifier of the message object
 * @param msgdata
 * 	The character array to get the message data from
 */
void hsk_can_msg_setData(hsk_can_msg idata msg, ubyte * idata msgdata);

/**
 * Request transmission of a message.
 *
 * @param msg
 * 	The identifier of the message to send
 */
void hsk_can_msg_send(hsk_can_msg idata msg);

/**
 * Return the message into RX mode after sending a message.
 *
 * After sending a message the messages with the same ID from other
 * bus participants are ignored. This restores the original setting to receive
 * messages.
 *
 * @param msg
 * 	The identifier of the message to receive
 */
void hsk_can_msg_receive(hsk_can_msg idata msg);

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
 * 	The identifier of the message to check
 * @retval 1
 *	The message was updated since the last call of this function
 * @retval 0
 *	The message has not been updated since the last call of this function
 */
bool hsk_can_msg_updated(hsk_can_msg idata msg);

/** \file
 * \section fifos FIFOs
 *
 * Only RX FIFOs are currently implemented. In most use cases only the latest
 * version of a message is relevant and FIFOs are not required. But messages
 * containing multiplexed signals may contain critical signals that would be
 * overwritten by a message with the same ID, but a different multiplexor.
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
hsk_can_fifo hsk_can_fifo_create(ubyte idata size);

/**
 * Set the FIFO up for receiving messages.
 *
 * @param fifo
 * 	The FIFO to setup
 * @param id
 * 	The message ID.
 * @param extended
 * 	Set this to 1 for an extended CAN message
 * @param dlc
 * 	The data length code, # of bytes in the message, valid values
 * 	range from 0 to 8
 */
void hsk_can_fifo_setupRX(hsk_can_fifo idata fifo, ulong idata id,
	bool extended, ubyte idata dlc);

/**
 * Connect a FIFO to a CAN node.
 *
 * @param fifo
 * 	The identifier of the FIFO
 * @param node
 * 	The CAN node to connect to
 * @retval CAN_ERROR
 *	The given FIFO is not valid
 * @retval 0
 *	Success
 */
ubyte hsk_can_fifo_connect(hsk_can_fifo idata fifo, hsk_can_node idata node);

/**
 * Disconnect a FIFO from its CAN node.
 *
 * This takes the FIFO out of active communication, without deleting
 * it.
 *
 * @param fifo
 * 	The identifier of the FIFO
 * @retval CAN_ERROR
 *	The given FIFO is not valid
 * @retval 0
 *	Success
 */
ubyte hsk_can_fifo_disconnect(hsk_can_fifo idata fifo);

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
ubyte hsk_can_fifo_delete(hsk_can_fifo idata fifo);

/**
 * Select the next FIFO entry.
 *
 * The hsk_can_fifo_updated() and hsk_can_fifo_getData() functions always
 * refer to a certain message within the FIFO. This function selects the
 * next entry.
 *
 * @param fifo
 *	The ID of the FIFO to select the next entry from.
 */
void hsk_can_fifo_next(hsk_can_fifo idata fifo);

/**
 * Return whether the currently selected FIFO entry was updated via CAN bus
 * between this call and the previous call of this method.
 *
 * It can be used to decide when to call hsk_can_fifo_getData() and
 * hsk_can_fifo_next().
 *
 * @param fifo
 * 	The identifier of the FIFO to check
 * @retval 1
 *	The FIFO entry was updated since the last call of this function
 * @retval 0
 *	The FIFO entry has not been updated since the last call of this function
 */
bool hsk_can_fifo_updated(hsk_can_fifo idata fifo);

/**
 * Gets the data from the currently selected FIFO entry.
 *
 * This writes DLC bytes from the FIFO entry into msgdata.
 *
 * @param fifo
 * 	The identifier of the FIFO
 * @param msgdata
 * 	The character array to store the message data in
 */
void hsk_can_fifo_getData(hsk_can_fifo idata fifo, ubyte * idata msgdata);

/** \file
 * \section data Message Data
 *
 * The hsk_can_data_setSignal() and hsk_can_data_getSignal() functions allow
 * reading and writing signals across byte boundaries to a buffer.
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
 * 	The message data field to write into
 * @param endian
 *	Little or big endian encoding
 * @param bitPos
 * 	The bit position of the signal
 * @param bitCount
 * 	The length of the signal
 * @param value
 * 	The signal value to write into the data field
 */
void hsk_can_data_setSignal(ubyte * idata msg, bool endian, char idata bitPos,
		char idata bitCount, ulong idata value);

/**
 * Get a signal value from a data field.
 *
 * @param msg
 * 	The message data field to read from
 * @param endian
 *	Little or big endian encoding
 * @param bitPos
 * 	The bit position of the signal
 * @param bitCount
 * 	The length of the signal
 * @return
 *	The signal from the data field msg
 */
ulong hsk_can_data_getSignal(ubyte * idata msg, bool endian,
		char idata bitPos, char idata bitCount);

#endif /* _HSK_CAN_H_ */
