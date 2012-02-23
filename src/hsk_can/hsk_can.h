/** \file
 * HSK Controller Area Network headers
 *
 * This file contains the function prototypes to initialize and engage in
 * CAN communication over the builtin CAN nodes 0 and 1.
 *
 * It contains 2 groups of functions:
 * 	- CAN node management
 *		- hsk_can_init()
 *		- hsk_can_enable()
 *		- hsk_can_disable()
 *	- CAN message management
 *		- hsk_can_msg_*()
 *
 * There are 7 port pairs available for CAN communication, check the CAN_IO_*
 * defines.
 *
 * All message objects automatically receive from the can bus. The sending
 * of messages has to be triggered manually.
 *
 * @author kami
 * @version 2011-09-27
 */

#ifndef _HSK_CAN_H_
#define _HSK_CAN_H_

/**
 * Value returned by functions in case of an error.
 */
#define	CAN_ERROR		0xff

/**
 * CAN IO RX on P0.1, TX on P0.2.
 */
#define CAN_IO_P01_P02		0

/**
 * CAN IO RX on P1.0, TX on P1.1.
 */
#define CAN_IO_P10_P11		1

/**
 * CAN IO RX on P1.4, TX on P1.3.
 */
#define CAN_IO_P14_P13		2

/**
 * CAN IO RX on P1.6, TX on P1.7.
 */
#define CAN_IO_P16_P17		3

/**
 * CAN IO RX on P3.2, TX on P3.3.
 */
#define CAN_IO_P32_P33		4

/**
 * CAN IO RX on P3.4, TX on P3.5.
 */
#define CAN_IO_P34_P35		5

/**
 * CAN IO RX on P4.0, TX on P4.1.
 */
#define CAN_IO_P40_P41		6

/**
 * CAN node identifiers.
 */
typedef ubyte hsk_can_node;

/**
 * CAN message object identifiers.
 */
typedef ubyte hsk_can_msg;

/*
 * CAN node management.
 */

/**
 * Setup CAN communication with the desired baud rate.
 *
 * The bus still needs to be enabled after being setup.
 *
 * @param node
 * 	The CAN node to set up.
 * @param baud
 * 	The target baud rate to use.
 * @param pins
 * 	Choose one of 7 CAN_IO_* configurations.
 */
void hsk_can_init(hsk_can_node idata node, ulong idata baud, ubyte idata pins);

/**
 * Go live on the CAN bus.
 *
 * To be called when everything is set up.
 *
 * @param node
 * 	The CAN node to enable.
 */
void hsk_can_enable(hsk_can_node idata node);

/**
 * Disable a CAN node.
 *
 * This completely shuts down a CAN node, cutting it off from the
 * internal clock, to reduce energy consumption.
 *
 * @param node
 * 	The CAN node to disable.
 */
void hsk_can_disable(hsk_can_node idata node);

/*
 * CAN message object management.
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
 * @return
 * 	CAN_ERROR in case of failure, a message identifier otherwise.
 */
hsk_can_msg hsk_can_msg_create(ulong idata id, bool extended,
		ubyte idata dlc);

/**
 * Connect a message object to a CAN node.
 *
 * @param msg
 * 	The identifier of the message object.
 * @param node
 * 	The CAN node to connect to.
 * @return
 * 	CAN_ERROR	If the given message is not valid
 * 	1		If the given message is not allocated
 * 	0		Otherwise
 */
ubyte hsk_can_msg_connect(hsk_can_msg idata msg, hsk_can_node idata node);

/**
 * Disconnect a CAN message object from its CAN node.
 *
 * This takes a CAN message out of active communication, without deleting
 * it.
 *
 * @param msg
 * 	The identifier of the message object.
 * @return
 * 	CAN_ERROR	If the given message is not valid
 * 	1		If the given message is not connected to a CAN node
 * 	0		otherwise
 */
ubyte hsk_can_msg_disconnect(hsk_can_msg idata msg);

/**
 * Delete a CAN message object.
 *
 * @param msg
 *	The identifier of the message object.
 * @return
 *	CAN_ERROR	If the given message is not valid
 *	1		The message is already deleted
 *	0		Otherwise
 */
ubyte hsk_can_msg_delete(hsk_can_msg idata msg);

/**
 * Gets the current data in the CAN message.
 *
 * This writes DLC bytes from msgdata to the CAN message object.
 *
 * @param msg
 * 	The identifier of the message object.
 * @param msgdata
 * 	The character array to store the message data in.
 */
void hsk_can_msg_getData(hsk_can_msg idata msg, ubyte * idata msgdata);

/**
 * Sets the current data in the CAN message.
 *
 * This writes DLC bytes from the CAN message object into msgdata.
 *
 * @param msg
 * 	The identifier of the message object.
 * @param msgdata
 * 	The character array to get the message data from.
 */
void hsk_can_msg_setData(hsk_can_msg idata msg, ubyte * idata msgdata);

/**
 * Request transmission of a message.
 *
 * @param msg
 * 	The identifier of the message to send.
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
 * 	The identifier of the message to send.
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
 * 	The identifier of the message to check.
 * @return
 * 	Returns 1 (true) if the message was received since the last call,
 *	0 (fals) otherwise.
 */
ubyte hsk_can_msg_updated(hsk_can_msg idata msg);

/**
 * Sets a signal value in a data field.
 *
 * @param msg
 * 	The message data field to write into.
 * @param bitPos
 * 	The bit position of the signal.
 * @param bitCount
 * 	The length of the signal.
 * @param value
 * 	The signal value to write into the data field.
 */
void hsk_can_data_setSignal(ubyte * idata msg, char idata bitPos, char idata bitCount, ulong idata value);

/**
 * Sets a big endian signal value in a data field.
 *
 * Big endian signals are bit strange, play with them in the Vector CANdb
 * editor to figure them out.
 *
 * @param msg
 * 	The message data field to write into.
 * @param bitPos
 * 	The bit position of the signal.
 * @param bitCount
 * 	The length of the signal.
 * @param value
 * 	The signal value to write into the data field.
 */
void hsk_can_data_setMotorolaSignal(ubyte * idata msg, char idata bitPos, char idata bitCount, ulong idata value);

/**
 * Get a signal value from a data field.
 *
 * @param msg
 * 	The message data field to read from.
 * @param bitPos
 * 	The bit position of the signal.
 * @param bitCount
 * 	The length of the signal.
 * @return
 *	The signal from the data field msg.
 */
ulong hsk_can_data_getSignal(ubyte * idata msg, char idata bitPos, char idata bitCount);

/**
 * Get a big endian signal value from a data field.
 *
 * Big endian signals are bit strange, play with them in the Vector CANdb
 * editor to figure them out.
 *
 * @param msg
 * 	The message data field to read from.
 * @param bitPos
 * 	The bit position of the signal.
 * @param bitCount
 * 	The length of the signal.
 * @return
 *	The signal from the data field msg.
 */
ulong hsk_can_data_getMotorolaSignal(ubyte * idata msg, char idata bitPos, char idata bitCount);

#endif /* _HSK_CAN_H_ */
