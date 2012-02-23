/** \file
 * HSK Controller Area Network implementation
 *
 * This file implements the functions defined in hsk_can.h.
 *
 * The following is a little excursion about CAN on the XC878.
 *
 * The MultiCAN module is accessible through 3 registers:
 * 	CAN_ADCON	CAN Address/Data Control Register	 8bits
 * 	CAN_AD		CAN Address Register			16bits
 * 	CAN_DATA	CAN Data Register			32bits
 *
 * These registers give access to a bus. CAN_ADCON is used to control
 * bus (e.g. write or read), everything else is done by writing
 * the desired MultiCAN address into the CAN_AD register. The desired
 * MultiCAN register is then accessible through the CAN_DATA register.
 *
 * Register	Representation	Bits	Starting
 * CAN_ADCON	CAN_ADCON	 8	 0
 * CAN_AD	CAN_ADL		 8	 0
 * 		CAN_ADH		 8	 8
 *		CAN_ADLH	16	 0
 * CAN_DATA	CAN_DATA0	 8	 0
 *		CAN_DATA1	 8	 8
 *		CAN_DATA2	 8	16
 *		CAN_DATA3	 8	24
 *		CAN_DATA01	16	 0
 *		CAN_DATA23	16	16
 *
 * Internally the MultiCAN module has register groups, i.e. a structured set
 * of registers that are repeated for each item having the registers.
 * An item may be a node or a list. Each register has a fixed base address
 * and each item a fixed offset.
 * Each register for an item is thus addressed by setting:
 * 	CAN_ADLH = REGISTER + ITEM_OFFSET
 * 
 * The following example points CAN_DATA to the Node 1 Status register:
 * 	CAN_ADLH = NSRx + (1 << OFF_NODEx)
 *
 * @author kami
 * @version 2011-09-27
 */

#include <Infineon/XC878.h>

#include "hsk_can.h"

#include <string.h> /* memset() */

/**
 * CAN_ADCON Read/Write Enable bit.
 *
 * Write is 1.
 */
#define BIT_RWEN		0

/**
 * CAN_ADCON Data Transmission Busy bit.
 */
#define BIT_BSY			1

/**
 * CAN_ADCON Auto Increment/Decrement the Address bits.
 */
#define BIT_AUAD		2

/**
 * AUAD auto increment off setting.
 */
#define AUAD_OFF		(0 << BIT_AUAD)

/**
 * AUAD auto increment setting.
 */
#define AUAD_INC1		(1 << BIT_AUAD)

/**
 * AUAD auto decrement setting.
 */
#define AUAD_DEC1		(2 << BIT_AUAD)

/**
 * AUAD auto increment setting.
 */
#define AUAD_INC8		(3 << BIT_AUAD)

/**
 * CAN_ADCON CAN Data Valid	bits.
 */
#define	BIT_DATA		4

/**
 * DATA bit count.
 */
#define CNT_DATA		4

/**
 * Sets up the CAN_AD bus for writing.
 *
 * @param msk
 * 	A bit mask representing the data bytes that should be written.
 * 	E.g. 0xC would only write CAN_DATA2 and CAN_DATA3.
 */
#define CAN_AD_WRITE(msk)	CAN_ADCON = (1 << BIT_RWEN) | ((msk) << BIT_DATA)

/**
 * Sets up the CAN_AD bus for reading.
 *
 * The controller always reads all 4 data bytes.
 */
#define CAN_AD_READ()		CAN_ADCON = 0

/**
 * Make sure the last read/write has completed.
 * 
 * This is obligatory for accessing the data bytes and CAN_ADCON.
 */
#define CAN_AD_READY()		while(CAN_ADCON & (1 << BIT_BSY))

/**
 * CMCON MultiCAN Clock Configuration bit.
 *
 * Used to select PCLK * 2 (1) or PCKL (0) to drive the MultiCAN module.
 */
#define	BIT_FCCFG		4

/**
 * The ld() of the List Register (LISTm) m offset factor.
 */
#define OFF_LISTm		0

/**
 * The ld() of the Message Index Register k offset factor.
 */
#define OFF_MSIDk		0

/**
 * The ld() of the Message Pending Register k offset factor.
 */
#define OFF_MSPNDk		0

/**
 * The ld() of the Node Register x offset factor.
 */
#define OFF_NODEx		6

/**
 * The ld() of the Message Object n offset factor.
 */
#define OFF_MOn			3

/**
 * Node x Control Register base address.
 */
#define NCRx			0x0080

/**
 * Node x Status Register base address.
 */
#define NSRx			0x0081

/**
 * Node x Interrupt Pointer Register base address.
 */
#define NIPRx			0x0082

/**
 * Node x Port Control Register	base address.
 */
#define NPCRx			0x0083

/**
 * Node x Bit Timing Register base address.
 */
#define NBTRx			0x0084

/**
 * Node x Error Counter Register base address.
 */
#define NECNTx			0x0085

/**
 * Node x Frame Counter Register base address.
 */
#define NFCRx			0x0086

/**
 * CAN NCRx Node Initialization bit.
 */
#define BIT_INIT		0

/**
 * CAN NCRx Transfer Interrupt Enable bit.
 */
#define BIT_TRIE		1

/**
 * CAN NCRx LEC Indicated Error Interrupt Enable bit.
 */
#define BIT_LECIE		2

/**
 * CAN NCRx Alert Interrupt Enable bit.
 */
#define BIT_ALIE		3

/**
 * CAN NCRx CAN Disable
 *
 * Can be used for a complete shutdown of a CAN node.
 */
#define BIT_CANDIS		4

/**
 * CAN NCRx Configuration Change Enable bit.
 */
#define BIT_CCE			6

/**
 * CAN NCRx CAN Analyze Mode bit.
 */
#define BIT_CALM		7

/**
 * NBTRx Baud Rate Prescaler bits.
 */
#define BIT_BRP			0

/**
 * NBTRx (Re) Synchronization Jump Width bits.
 */
#define BIT_SJW			6

/**
 * NBTRx Time Segment Before Sample Point bits.
 */
#define BIT_TSEG1		8

/**
 * NBTRx Time Segment After Sample Point bits.
 */
#define BIT_TSEG2		12

/**
 * NBTRx Divide Prescaler Clock by 8 bit.
 */
#define BIT_DIV8		15

/**
 * NPCRx Receive Select bit.
 */
#define BIT_RXSEL		0

/**
 * RXSEL bit count.
 */
#define CNT_RXSEL		3

/**
 * Stores whether common initialization has been performed.
 */
bool hsk_can_initialized = 0;

/**
 * The Panel Control Register.
 *
 * All list manipulations are performed here.
 */
#define PANCTR			0x0071

/**
 * PANCTR Command Register.
 */
#define PANCMD			CAN_DATA0

/**
 * PANCTR Status Register.
 */
#define	PANSTATUS		CAN_DATA1

/**
 * PANCTR PANSTATUS Panel Busy Flag bit.
 */
#define	BIT_BUSY		0

/**
 * PANCTR PANSTATUS Result Busy Flag bit.
 */
#define BIT_RBUSY		1

/**
 * PANCTR Argument 1 Register.
 */
#define PANAR1			CAN_DATA2

/**
 * PANCTR Argument 2 Register.
 */
#define PANAR2			CAN_DATA3

/**
 * PANCTR PANAR2 Error bit.
 */
#define BIT_ERR			7

/**
 * Wait for list operations to complete.
 *
 * Only execute this if CAN_ADLH points to PANCTR.
 */
#define PANCTR_READY()		do { \
					CAN_AD_READ(); \
					CAN_AD_READY();	\
				} while (PANSTATUS & ((1 << BIT_BUSY) | (1 << BIT_RBUSY)))

/*
 * Check the MultiCAN Global Module Registers subsection for details
 * on the following panel commands.
 */

/**
 * List panel No Operation command.
 */
#define PAN_CMD_NOP		0x00

/**
 * List panel Initialize Lists command.
 */
#define PAN_CMD_INIT		0x01

/**
 * List panel Static Allocate command.
 */
#define PAN_CMD_MOVE		0x02

/**
 * List panel Dynamic Allocate command.
 */
#define PAN_CMD_ALLOC		0x03

/**
 * List panel Static Insert Before command.
 */
#define PAN_CMD_MOVEBEFORE	0x04

/**
 * List panel Dynamic Insert Before command.
 */
#define PAN_CMD_ALLOCBEFORE	0x05

/**
 * List panel Static Insert Behind command.
 */
#define PAN_CMD_MOVEBEHIND	0x06

/**
 * List panel Dynamic Insert Behind command.
 */
#define PAN_CMD_ALLOCBEHIND	0x07

/**
 * The maximum number of message objects.
 */
#define HSK_CAN_MSG_MAX		32

/**
 * A CAN message container to hold everything to manage a message
 * object state.
 */
struct hsk_can_msg_container {
	/**
	 * The list the message is currently stored in.
	 */
	ubyte list;

	/**
	 * The data length counter of the message.
	 */
	ubyte dlc;
};

/**
 * Statically assign memory for 32 message containers.
 */
struct hsk_can_msg_container xdata hsk_can_messages[HSK_CAN_MSG_MAX];

/**
 * This list holds unallocated message objects.
 */
#define LIST_UNALLOC		0

/**
 * These lists hold message objects connected to a CAN node.
 */
#define LIST_NODEx		1

/**
 * This list holds message objects pending assignment to a can node.
 */
#define LIST_PENDING		3

/**
 * PMCON1 CAN Disable Request bit.
 */
#define BIT_CAN_DIS		5

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
void hsk_can_init(hsk_can_node idata node, ulong idata baud, ubyte idata pins) {
	/*
	 * Select the receive signal pin.
	 */
	ubyte rxsel;

	/*
	 * Perform initialization tasks common to all nodes.
	 */
	if (!hsk_can_initialized) {
		hsk_can_initialized = 1;

		/* Enable clock. */
		SFR_PAGE(_su1, noSST);
		PMCON1 &= ~(1 << BIT_CAN_DIS);

		/* 
		 * Set the MultiCAN module to run at 2*PCLK speed (48MHz),
		 * to reduce the waiting time for bus transmissions.
		 */
		CMCON |= 1 << BIT_FCCFG;
		SFR_PAGE(_su0, noSST);

		/*
		 * Make sure the CAN message lists are initialized.
		 */
		memset(hsk_can_messages, 0, sizeof(hsk_can_messages));
		CAN_AD_READY();
		CAN_ADLH = PANCTR;
		PANCTR_READY();
	}

	/* Enable configuration changes. */
	CAN_AD_READY();
	CAN_ADLH = NCRx + (node << OFF_NODEx);
	CAN_DATA01 |= (1 << BIT_CCE) | (1 << BIT_INIT);
	CAN_AD_WRITE(0x1);

	/**
	 * Configure the Bit Timing Unit.
	 *
	 * Carefully read the chapter in the manual to understand this.
	 * Minima and maxima are specified in ISO 11898.
	 *
	 * One bit is s separated into 3 blocks, each of which are multiples
	 * of a time quantum. The size of the time quantum (TQ) is controlled
	 * by the BRP and DIV8 bits.
	 * Because TSYNC is fixed to a single quantum, the other segments
	 * should be made up of a minimum of TQs, so TSYNC doesn't get too
	 * short (making a bit up of more TQs requires each one to be shorter
	 * at the same baud rate).
	 * However, the minimum number of TQs is 8 and we need some spare
	 * quantums to adjust the timing between each bit transmission.
	 * TSYNC = 1 (fixed, not changeable)
	 * TSEG1 = 8 (min 3)	encoded 7
	 * TSEG2 = 3 (min 2)	encoded 2
	 * The above values give us 4 time quantums to adjust between bits
	 * without dropping below 8 quantums.
	 * SJW = 4				encoded 3
	 *
	 * The sample point is between TSEG1 and TSEG2, i.e. at 75%.
	 *
	 * This means we need 12 cycles per bit. Now the BRP bits can be
	 * used to achieve the desired baud rate:
	 *		baud = 48000000 / 12 / BRP
	 *
	 * The encoding of BRT is also VALUE+1.
	 */
	CAN_AD_READY();
	CAN_ADLH = NBTRx + (node << OFF_NODEx);
	CAN_DATA01 = (7 << BIT_TSEG1) | (2 << BIT_TSEG2) | (3 << BIT_SJW) \
		| ((48000000 / 12 / baud - 1) << BIT_BRP);
	CAN_AD_WRITE(0x3);

	/**
	 * Each CAN node can be routed to one of 7 IO pin configurations.
	 *
	 * See the MultiCAN Port Control subsection of the user manual for
	 * details.
	 */

	CAN_AD_READY();
	CAN_ADLH = NPCRx + (node << OFF_NODEx);
	CAN_AD_READ();

	switch(pins) {
	case CAN_IO_P01_P02:
		rxsel = 0x0;
		P0_DIR = P0_DIR & ~(1 << 1) | (1 << 2);
		SFR_PAGE(_pp2, noSST);
		P0_ALTSEL0 |= (1 << 2);
		P0_ALTSEL1 |= (1 << 2);
		break;
	case CAN_IO_P10_P11:
		rxsel = 0x0;
		P1_DIR = P1_DIR & ~(1 << 0) | (1 << 1);
		SFR_PAGE(_pp2, noSST);
		P1_ALTSEL0 |= (1 << 1);
		P1_ALTSEL1 |= (1 << 1);
		break;
	case CAN_IO_P14_P13:
		rxsel = 0x3;
		P1_DIR = P1_DIR & ~(1 << 4) | (1 << 3);
		SFR_PAGE(_pp2, noSST);
		P1_ALTSEL0 |= (1 << 3);
		P1_ALTSEL1 |= (1 << 3);
		break;
	case CAN_IO_P16_P17:
		rxsel = 0x2;
		P1_DIR = P1_DIR & ~(1 << 6) | (1 << 7);
		SFR_PAGE(_pp2, noSST);
		P1_ALTSEL0 |= (1 << 7);
		P1_ALTSEL1 |= (1 << 7);
		break;
	case CAN_IO_P32_P33:
		rxsel = 0x1;
		P3_DIR = P3_DIR & ~(1 << 2) | (1 << 3);
		SFR_PAGE(_pp2, noSST);
		P3_ALTSEL0 |= (1 << 3);
		P3_ALTSEL1 |= (1 << 3);
		break;
	case CAN_IO_P34_P35:
		rxsel = 0x1;
		P3_DIR = P3_DIR & ~(1 << 4) | (1 << 5);
		SFR_PAGE(_pp2, noSST);
		P3_ALTSEL0 |= (1 << 5);
		P3_ALTSEL1 |= (1 << 5);
		break;
	case CAN_IO_P40_P41:
		rxsel = 0x3;
		P4_DIR = P4_DIR & ~(1 << 0) | (1 << 1);
		SFR_PAGE(_pp2, noSST);
		P4_ALTSEL0 |= (1 << 1);
		P4_ALTSEL1 |= (1 << 1);
		break;
	default:
		/* Shouldn't ever happen! */
		return;
	}
	SFR_PAGE(_pp0, noSST);

	/* Write RX select bits. */
	CAN_AD_READY();
	CAN_DATA0 = CAN_DATA0 & ~(((1 << CNT_RXSEL) - 1) << BIT_RXSEL) \
		| (rxsel << BIT_RXSEL);
	CAN_AD_WRITE(0x1);

}

/**
 * Go live on the CAN bus.
 *
 * To be called when everything is set up.
 *
 * @param node
 * 	The CAN node to enable.
 */
void hsk_can_enable(hsk_can_node idata node) {
	/* Get the Node x Control Register. */
	CAN_AD_READY();
	CAN_ADLH = NCRx + (node << OFF_NODEx);
	CAN_AD_READ();
	/* Activate transmit interrupt and alert interrupt. */
	CAN_AD_READY();
	CAN_DATA0 &= ~((1 << BIT_INIT) | (1 << BIT_CCE) | (1 << BIT_CANDIS));
	CAN_AD_WRITE(0x1);
}

/**
 * Disable a CAN node.
 *
 * This completely shuts down a CAN node, cutting it off from the
 * internal clock, to reduce energy consumption.
 *
 * @param node
 * 	The CAN node to disable.
 */
void hsk_can_disable(hsk_can_node idata node) {
	/* Get the Node x Control Register. */
	CAN_AD_READY();
	CAN_ADLH = NCRx + (node << OFF_NODEx);
	CAN_AD_READ();
	/* Deactivate the node! */
	CAN_AD_READY();
	CAN_DATA0 |= 1 << BIT_CANDIS;
	CAN_AD_WRITE(0x1);		
}

/**
 * CAN list management.
 * 
 * The MultiCAN module offers 32 message objects that can be linked
 * to one of 8 lists.
 *
 * List 0 holds the unallocated (i.e. unused) objects.
 * List 1 is connected to CAN node 0.
 * List 2 is connected to CAN node 1.
 *
 * The following implementation will use 1 of the 5 general purpose lists
 * to park messages.
 *
 * All the list management will be hidden from the "user".
 */

/**
 * Message Object n Function Control Register base address.
 */
#define MOFCRn		0x0400

/**
 * Message Object n Acceptance Mask Register base address.
 */
#define MOAMRn		0x0403
/**
 * Message Object n Data Register Low base address.
 */
#define MODATALn	0x0404

/**
 * Message Object n Data Register High base address.
 */
#define	MODATAHn	0x0405

/**
 * Message Object n Arbitration Register base address.
 */
#define MOARn		0x0406

/**
 * Message Object n Control Register base address.
 */
#define MOCTRn		0x0407

/**
 * The register to write Control Register resets into.
 */
#define RESET_DATA	CAN_DATA01

/**
 * The register to write Control Register settings into.
 */
#define SET_DATA	CAN_DATA23

/**
 * Bit mask for writing resets.
 */
#define RESET			0x3

/**
 * Bit mask for writing settings.
 */
#define SET			0xC

/**
 * MOCTRn Message Valid bit.
 */
#define BIT_MSGVAL		5

/**
 * MOCTRn Receive Signal Enable bit.
 */
#define	BIT_RXEN		7

/**
 * MOCTRn Transmit Signal Request bit.
 */
#define BIT_TXRQ		8

/**
 * MOCTRn Transmit Signal 0 (Low???) Enable bit.
 */
#define BIT_TXEN0		9

/**
 * MOCTRn Transmit Signal 1 (High???) Enable bit.
 */
#define BIT_TXEN1		10

/**
 * MOCTRn Direction bit.
 *
 * Set this to 1 for TX, this was figured out by trial and error.
 */
#define BIT_DIR			11

/**
 * MOAMRn Acceptance Mask for Message Identifier bits.
 */
#define BIT_AM			0

/**
 * AM bit count.
 */
#define CNT_AM			29

/**
 * MOAMRn Acceptance Mask Bit for Message IDE Bit.
 */
#define BIT_MIDE		29

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
		ubyte idata dlc) {
	hsk_can_msg msg;

	/* 
	 * Fetch message into list of pending message objects.
	 */
	CAN_AD_READY();
	CAN_ADLH = PANCTR;

	PANCTR_READY();
	PANCMD = PAN_CMD_ALLOC;
	PANAR2 = LIST_PENDING;
	CAN_AD_WRITE(0xD);
	PANCTR_READY();

	/* Check for success. */
	if (PANAR2 & (1 << BIT_ERR)) {
		return CAN_ERROR;
	}
	msg = PANAR1;

	/* Store meta information. */
	hsk_can_messages[msg].list = LIST_PENDING;
	hsk_can_messages[msg].dlc = dlc;

	/* 
	 * Set the DLC. 
	 */
	CAN_AD_READY();
	CAN_ADLH = MOFCRn + (msg << OFF_MOn);
	CAN_DATA3 = (dlc <= 8 ? dlc : 8);
	CAN_AD_WRITE(0x8);

	/* 
	 * Set ID.
	 */
	CAN_AD_READY();
	CAN_ADLH = MOARn + (msg << OFF_MOn);

	if (!extended) {
		/* 11 bits standard message ID. */
		CAN_DATA23 = (id & ((1 << 11) - 1)) << (18 - 16);
	} else {
		/* 29 bits extended message ID. */
		CAN_DATA01 = id;
		CAN_DATA23 = (id & ((1ul << 29) - 1)) >> 16;
	}
	/* Set extended ID bit and ID based priority. */
	CAN_DATA3 |= ((ubyte)extended << 5) | (0x2 << 6);
	CAN_AD_WRITE(0xF);

	/*
	 * Set up the message for reception.
	 */

	/* Adjust filtering mask to only accept complete ID matches. */
	CAN_AD_READY();
	CAN_ADLH = MOAMRn + (msg << OFF_MOn);
	CAN_DATA01 = -1;
	CAN_DATA23 = (1 << (BIT_MIDE - 16)) | (((1 << (CNT_AM - 16)) - 1) << BIT_AM);
	CAN_AD_WRITE(0xF);

	/* Set message valid. */
	CAN_AD_READY();
	CAN_ADLH = MOCTRn + (msg << OFF_MOn);
	SET_DATA = (1 << BIT_MSGVAL) | (1 << BIT_RXEN);
	CAN_AD_WRITE(SET);

	return msg;
}

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
ubyte hsk_can_msg_connect(hsk_can_msg idata msg, hsk_can_node idata node) {
	if (msg >= HSK_CAN_MSG_MAX) {
		return CAN_ERROR;
	}
	if (hsk_can_messages[msg].list == LIST_UNALLOC) {
		return 1;
	}

	/* Move message to the requested CAN node. */
	CAN_AD_READY();
	CAN_ADLH = PANCTR;

	PANCTR_READY();
	PANCMD = PAN_CMD_MOVE;
	PANAR1 = msg;
	PANAR2 = LIST_NODEx + node;
	CAN_AD_WRITE(0xD);
	PANCTR_READY();

	hsk_can_messages[msg].list = LIST_NODEx + node;

	return 0;
}

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
ubyte hsk_can_msg_disconnect(hsk_can_msg idata msg) {
	/*
	 * Check whether the message id is valid and the message is connected
	 * to a CAN node.
	 */
	if (msg >= HSK_CAN_MSG_MAX) {
		return CAN_ERROR;
	}
	if (hsk_can_messages[msg].list < LIST_NODEx || hsk_can_messages[msg].list == LIST_PENDING) {
		return 1;
	}

	/*
	 * Move the message object to the list of pending message objects.
	 */
	CAN_AD_READY();
	CAN_ADLH = PANCTR;

	PANCTR_READY();	  
	PANCMD = PAN_CMD_MOVE;
	PANAR1 = msg;
	PANAR2 = LIST_PENDING;
	CAN_AD_WRITE(0xD);
	PANCTR_READY();

	hsk_can_messages[msg].list = LIST_PENDING;

	return 0;
}

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
ubyte hsk_can_msg_delete(hsk_can_msg idata msg) {
	if (msg >= HSK_CAN_MSG_MAX) {
		return CAN_ERROR;
	}
	if (hsk_can_messages[msg].list == LIST_UNALLOC) {
		return 1;
	}

	/*
	 * Move the message object into the list of unallocated objects.
	 */
	CAN_AD_READY();
	CAN_ADLH = PANCTR;

	PANCTR_READY();
	PANCMD = PAN_CMD_MOVE;
	PANAR1 = msg;
	PANAR2 = LIST_UNALLOC;
	CAN_AD_WRITE(0xD);
	PANCTR_READY();

	hsk_can_messages[msg].list = LIST_UNALLOC;

	return 0;
}

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
void hsk_can_msg_getData(hsk_can_msg idata msg, ubyte * idata msgdata) {
	ubyte i;

	CAN_AD_READY();
	CAN_ADLH = MODATALn + (msg << OFF_MOn);

	for (i = 0; i < hsk_can_messages[msg].dlc; i++) {
		switch (i % 4) {
		case 0:
			CAN_AD_READ() | AUAD_INC1;
			CAN_AD_READY();
			msgdata[i] = CAN_DATA0;
			break;
		case 1:
			msgdata[i] = CAN_DATA1;
			break;
		case 2:
			msgdata[i] = CAN_DATA2;
			break;
		case 3:
			msgdata[i] = CAN_DATA3;
			break;
		}
	}
}

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
void hsk_can_msg_setData(hsk_can_msg idata msg, ubyte * idata msgdata) {
	ubyte i;

	CAN_AD_READY();
	CAN_ADLH = MODATALn + (msg << OFF_MOn);

	for (i = 0; i < hsk_can_messages[msg].dlc; i++) {
		switch (i % 4) {
		case 0:
			CAN_DATA0 = msgdata[i];
			break;
		case 1:
			CAN_DATA1 = msgdata[i];
			break;
		case 2:
			CAN_DATA2 = msgdata[i];
			break;
		case 3:
			CAN_DATA3 = msgdata[i];
			CAN_AD_WRITE(0xf) | AUAD_INC1;
			CAN_AD_READY();
			break;
		}
	}
	if (i % 4) {
		CAN_AD_WRITE((1 << (i % 4)) - 1);
	}
}

/**
 * Request transmission of a message.
 *
 * @param msg
 * 	The identifier of the message to send.
 */
void hsk_can_msg_send(hsk_can_msg idata msg) {

	/* Request transmission. */
	CAN_AD_READY();
	CAN_ADLH = MOCTRn + (msg << OFF_MOn);
	SET_DATA = (1 << BIT_TXEN0) | (1 << BIT_TXEN1) | (1 << BIT_TXRQ) \
			| (1 << BIT_DIR);
	CAN_AD_WRITE(SET);
}

/**
 * Message Object n Status Register base address.
 *
 * The status register is at the same address as the control register.
 * It is accessed by reading from the address instead of writing.
 */
#define MOSTATn		MOCTRn

/**
 * MOSTATn Receive Pending bit.
 */
#define BIT_RXPND	0

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
void hsk_can_msg_receive(hsk_can_msg idata msg) {
	/* Return to rx mode. */
	CAN_AD_READY();
	CAN_ADLH = MOCTRn + (msg << OFF_MOn);
	RESET_DATA = (1 << BIT_TXEN0) | (1 << BIT_TXEN1) | (1 << BIT_TXRQ) \
			| (1 << BIT_DIR);
	CAN_AD_WRITE(RESET);
}

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
ubyte hsk_can_msg_updated(hsk_can_msg idata msg) {
	ubyte status;

	/* Get the message status. */
	CAN_AD_READY();
	CAN_ADLH = MOSTATn + (msg << OFF_MOn);
	CAN_AD_READ();
	CAN_AD_READY();
	status = (CAN_DATA0 >> BIT_RXPND) & 1;
	/* Reset the RXPND bit. */
	CAN_DATA0 = 1 << BIT_RXPND;
	CAN_AD_WRITE(0x1);

	return status;
}

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
void hsk_can_data_setSignal(ubyte * idata msg, char idata bitPos,
		char idata bitCount, ulong idata value) {
	ubyte shift;
	while (bitCount > 0) {
		shift = bitPos % 8;
		/* Clear the bits to write. */
		msg[bitPos / 8] &= ~(((1 << bitCount) - 1) << shift);
		/* Set the bits to write. */
		msg[bitPos / 8] |= (((1 << bitCount) - 1) & value) << shift;
		/* Get the next bit position. */
		bitCount -= 8 - shift;
		bitPos += 8 - shift;
		/* Remove the currently written part of value. */
		value >>= 8 - shift;
	}
}

/**
 * Sets a big endian signal value in a data field.
 *
 * Big endian signals are bit strange, play with them in the Vector CANdb
 * editor to figure them out.
 *
 * * @param msg
 * 	The message data field to write into.
 * @param bitPos
 * 	The bit position of the signal.
 * @param bitCount
 * 	The length of the signal.
 * @param value
 * 	The signal value to write into the data field.
 */
void hsk_can_data_setMotorolaSignal(ubyte * idata msg, char idata bitPos,
		char idata bitCount, ulong idata value) {
	ubyte bits;

	while (bitCount > 0) {
		/* Get the number of bits to work on. */
		bits = bitPos % 8 + 1;
		bits = bits < bitCount ? bits : bitCount;
		/*  Clear the bits to write. */
		msg[bitPos / 8] &= ~(((1 << bits) - 1) << (bitPos % 8 + 1 - bits));
		/* Set the bits to write. */
		msg[bitPos / 8] |= (((1 << bitCount) - 1) & value) >> (bitCount - bits) << (bitPos % 8 + 1 - bits);
		/* Get the next bit position. */
		bitCount -= bits;
		bitPos = (bitPos & ~(0x07)) + 15;
	}
}

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
ulong hsk_can_data_getSignal(ubyte * idata msg, char idata bitPos, char idata bitCount) {
	ulong value = 0;
	ubyte shift = 0;
	while (bitCount > 0) {
		/* Get the bottommost part of the value. */
		value |= ((msg[bitPos / 8] >> (bitPos % 8)) & ((1 << bitCount) - 1)) << shift;
		/* Update counters. */
		bitCount -= 8 - (bitPos % 8);
		shift += 8 - (bitPos % 8);
		bitPos += 8 - (bitPos % 8);
	}
	return value;
}

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
ulong hsk_can_data_getMotorolaSignal(ubyte * idata msg, char idata bitPos, char idata bitCount) {
	ulong value = 0;
	ubyte bits;

	while (bitCount > 0) {
		/* Get the number of bits to work on. */
		bits = bitPos % 8 + 1;
		bits = bits < bitCount ? bits : bitCount;
		/* Get the most significant bits. */
		value |= ((msg[bitPos / 8] >> (bitPos % 8 + 1 - bits)) & ((1 << bits) - 1)) << (bitCount - bits);
		/* Get the next bit position. */
		bitCount -= bits;
		bitPos = (bitPos & ~(0x07)) + 15;
	}

	return value;
}

