/** \file
 * HSK Controller Area Network implementation
 *
 * This file implements the functions defined in hsk_can.h.
 *
 * @author kami
 *
 * \section multican The XC878 MulitCAN Module
 *
 * The following is a little excursion about CAN on the XC878.
 *
 * The MultiCAN module is accessible through 3 registers:
 *
 * | Register	| Function				| Width
 * |------------|---------------------------------------|--------:
 * | CAN_ADCON	| CAN Address/Data Control Register	|  8 bits
 * | CAN_AD	| CAN Address Register			| 16 bits
 * | CAN_DATA	| CAN Data Register			| 32 bits
 *
 * These registers give access to a bus. CAN_ADCON is used to control
 * bus (e.g. write or read), everything else is done by writing
 * the desired MultiCAN address into the CAN_AD register. The desired
 * MultiCAN register is then accessible through the CAN_DATA register.
 *
 * | Register	| Representation	| Bits	| Starting
 * |------------|-----------------------|------:|---------:
 * | CAN_ADCON	| CAN_ADCON		|  8	|  0
 * | CAN_AD	| CAN_ADL		|  8	|  0
 * | 		| CAN_ADH		|  8	|  8
 * | 		| CAN_ADLH		| 16	|  0
 * | CAN_DATA	| CAN_DATA0		|  8	|  0
 * | 		| CAN_DATA1		|  8	|  8
 * | 		| CAN_DATA2		|  8	| 16
 * | 		| CAN_DATA3		|  8	| 24
 * | 		| CAN_DATA01		| 16	|  0
 * | 		| CAN_DATA23		| 16	| 16
 *
 * Internally the MultiCAN module has register groups, i.e. a structured set
 * of registers that are repeated for each item having the registers.
 * An item may be a node or a list. Each register has a fixed base address
 * and each item a fixed offset.
 * Each register for an item is thus addressed by setting:
 * \code
 * 	CAN_ADLH = REGISTER + ITEM_OFFSET
 * \endcode
 *
 * The following example points CAN_DATA to the Node 1 Status register:
 * \code
 * 	CAN_ADLH = NSRx + (1 << OFF_NODEx)
 * \endcode
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
 * This is supposed to be mandatory for accessing the data bytes and
 * CAN_ADCON, but tests show that the busy flag is never set if the
 * module runs at 2 times PCLK, which is what this library does.
 */
#define CAN_AD_READY()		while (CAN_ADCON & (1 << BIT_BSY))

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

void hsk_can_init(const ubyte pins, const ulong baud) {
	/* The node to configure. */
	hsk_can_node node;
	/* Select the receive signal pin. */
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
		CAN_ADLH = PANCTR;
		PANCTR_READY();
	}

	/*
	 * Figure out the CAN node.
	 */
	switch(pins) {
	case CAN0_IO_P10_P11:
	case CAN0_IO_P16_P17:
	case CAN0_IO_P34_P35:
	case CAN0_IO_P40_P41:
		node = CAN0;
		break;
	case CAN1_IO_P01_P02:
	case CAN1_IO_P14_P13:
	case CAN1_IO_P32_P33:
		node = CAN1;
		break;
	default:
		/* Shouldn't ever happen! */
		return;
	}

	/* Enable configuration changes. */
	CAN_ADLH = NCRx + (node << OFF_NODEx);
	CAN_DATA01 |= (1 << BIT_CCE) | (1 << BIT_INIT);
	CAN_AD_WRITE(0x1);

	/**
	 * <b>Configure the Bit Timing Unit</b>
	 *
	 * \note
	 * 	Careful study of section 16.1.3 "CAN Node Control" of the
	 *	<a href="../contrib/XC878_um_v1_1.pdf">XC878 Reference Manual</a>
	 *	is advised before changing the following code.
	 *
	 * \note
	 *	Minima and maxima are specified in ISO 11898.
	 *
	 * One bit is s separated into 3 blocks, each of which are multiples
	 * of a time quantum. The size of the time quantum (TQ) is controlled
	 * by the BRP and DIV8 bits.
	 * Because TSYNC is fixed to a single quantum, the other segments
	 * should be made up of a minimum of TQs, so TSYNC doesn't get too
	 * short (making a bit up of more TQs requires each one to be shorter
	 * at the same baud rate).
	 * However, the minimum number of TQs is 8 and some spare quantums
	 * are needed to adjust the timing between each bit transmission.
	 *
	 * | Time Slice	| Value	| Minimum	| Encoding
	 * |------------|-------|---------------|-----------
	 * | TSYNC	| 1	| Fixed		| Implicite
	 * | TSEG1	| 8	| 3		| 7
	 * | TSEG2	| 3	| 2		| 2
	 * | SWJ	| 4	| -		| 3
	 *
	 * The above values provide 4 time quantums to adjust between bits
	 * without dropping below 8 quantums. The adjustment value is provided
	 * with the SWJ time slice.
	 *
	 * The sample point is between TSEG1 and TSEG2, i.e. at 75%.
	 *
	 * This means one bit requires 12 cycles. The BRP bits can be
	 * used to achieve the desired baud rate:
	 * 	\f[ baud = 48000000 / 12 / BRP \f]
	 *	\f[ BRP = 48000000 / 12 / baud \f]
	 *
	 * The encoding of BRT is also VALUE+1.
	 */
	CAN_ADLH = NBTRx + (node << OFF_NODEx);
	CAN_DATA01 = (7 << BIT_TSEG1) | (2 << BIT_TSEG2) | (3 << BIT_SJW) \
		| ((48000000 / 12 / baud - 1) << BIT_BRP);
	CAN_AD_WRITE(0x3);

	/**
	 * <b>I/O Configuration</b>
	 *
	 * There are 7 different I/O pin configurations, four are availabe
	 * to node 0 and three to node 1.
	 *
	 * @see
	 * 	Section 16.1.11 "MultiCAN Port Control" of the
	 *	<a href="../contrib/XC878_um_v1_1.pdf">XC878 Reference Manual</a>
	 */

	CAN_ADLH = NPCRx + (node << OFF_NODEx);
	CAN_AD_READ();

	switch(pins) {
	case CAN0_IO_P10_P11:
		rxsel = 0x0;
		P1_DIR = P1_DIR & ~(1 << 0) | (1 << 1);
		SFR_PAGE(_pp2, noSST);
		P1_ALTSEL0 |= (1 << 1);
		P1_ALTSEL1 |= (1 << 1);
		break;
	case CAN0_IO_P16_P17:
		rxsel = 0x2;
		P1_DIR = P1_DIR & ~(1 << 6) | (1 << 7);
		SFR_PAGE(_pp2, noSST);
		P1_ALTSEL0 |= (1 << 7);
		P1_ALTSEL1 |= (1 << 7);
		break;
	case CAN0_IO_P34_P35:
		rxsel = 0x1;
		P3_DIR = P3_DIR & ~(1 << 4) | (1 << 5);
		SFR_PAGE(_pp2, noSST);
		P3_ALTSEL0 |= (1 << 5);
		P3_ALTSEL1 |= (1 << 5);
		break;
	case CAN0_IO_P40_P41:
		rxsel = 0x3;
		P4_DIR = P4_DIR & ~(1 << 0) | (1 << 1);
		SFR_PAGE(_pp2, noSST);
		P4_ALTSEL0 |= (1 << 1);
		P4_ALTSEL1 |= (1 << 1);
		break;
	case CAN1_IO_P01_P02:
		rxsel = 0x0;
		P0_DIR = P0_DIR & ~(1 << 1) | (1 << 2);
		SFR_PAGE(_pp2, noSST);
		P0_ALTSEL0 |= (1 << 2);
		P0_ALTSEL1 |= (1 << 2);
		break;
	case CAN1_IO_P14_P13:
		rxsel = 0x3;
		P1_DIR = P1_DIR & ~(1 << 4) | (1 << 3);
		SFR_PAGE(_pp2, noSST);
		P1_ALTSEL0 |= (1 << 3);
		P1_ALTSEL1 |= (1 << 3);
		break;
	case CAN1_IO_P32_P33:
		rxsel = 0x1;
		P3_DIR = P3_DIR & ~(1 << 2) | (1 << 3);
		SFR_PAGE(_pp2, noSST);
		P3_ALTSEL0 |= (1 << 3);
		P3_ALTSEL1 |= (1 << 3);
		break;
	default:
		/* Shouldn't ever happen! */
		return;
	}
	SFR_PAGE(_pp0, noSST);

	/* Write RX select bits. */
	CAN_DATA0 = CAN_DATA0 & ~(((1 << CNT_RXSEL) - 1) << BIT_RXSEL) \
		| (rxsel << BIT_RXSEL);
	CAN_AD_WRITE(0x1);

}

void hsk_can_enable(const hsk_can_node node) {
	/* Get the Node x Control Register. */
	CAN_ADLH = NCRx + (node << OFF_NODEx);
	CAN_AD_READ();
	/* Activate transmit interrupt and alert interrupt. */
	CAN_DATA0 &= ~((1 << BIT_INIT) | (1 << BIT_CCE) | (1 << BIT_CANDIS));
	CAN_AD_WRITE(0x1);
}

void hsk_can_disable(const hsk_can_node node) {
	/* Get the Node x Control Register. */
	CAN_ADLH = NCRx + (node << OFF_NODEx);
	CAN_AD_READ();
	/* Deactivate the node! */
	CAN_DATA0 |= 1 << BIT_CANDIS;
	CAN_AD_WRITE(0x1);
}

ubyte hsk_can_status(const hsk_can_node node, const ubyte field) {
	ubyte status = -1;

	/* Get the Node x Status Register. */
	CAN_ADLH = NSRx + (node << OFF_NODEx);
	CAN_AD_READ();

	switch (field) {
	case CAN_STATUS_LEC:
		status = CAN_DATA0 & 0x7;
		break;
	case CAN_STATUS_TXOK:
		status = (CAN_DATA0 >> 3) & 1;
		CAN_DATA0 &= ~(1 << 3);
		CAN_AD_WRITE(0x1);
		break;
	case CAN_STATUS_RXOK:
		status = (CAN_DATA0 >> 4) & 1;
		CAN_DATA0 &= ~(1 << 4);
		CAN_AD_WRITE(0x1);
		break;
	case CAN_STATUS_ALERT:
		status = (CAN_DATA0 >> 5) & 1;
		CAN_DATA0 &= ~(1 << 5);
		CAN_AD_WRITE(0x1);
		break;
	case CAN_STATUS_EWRN:
		status = (CAN_DATA0 >> 6) & 1;
		CAN_DATA0 &= ~(1 << 6);
		CAN_AD_WRITE(0x1);
		break;
	case CAN_STATUS_BOFF:
		status = (CAN_DATA0 >> 7) & 1;
		break;
	}

	return status;
}

/** \file
 * \section lists CAN List Management
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
 * Message Object n FIFO/Gateway Pointer Register base address.
 */
#define MOFGPRn		0x0401

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
 * Message Object n Status Register base address.
 *
 * The status register is at the same address as the control register.
 * It is accessed by reading from the address instead of writing.
 */
#define MOSTATn		MOCTRn

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
 * MOCTRn/MOSTATn Receive Pending bit.
 */
#define BIT_RXPND		0

/**
 * MOCTRn/MOSTATn Transmit Pending bit.
 */
#define BIT_TXPND		1

/**
 * MOCTRn/MOSTATn Message Valid bit.
 */
#define BIT_MSGVAL		5

/**
 * MOCTRn/MOSTATn Receive Signal Enable bit.
 */
#define	BIT_RXEN		7

/**
 * MOCTRn/MOSTATn Transmit Signal Request bit.
 */
#define BIT_TXRQ		8

/**
 * MOCTRn/MOSTATn Transmit Signal Enable bit.
 */
#define BIT_TXEN0		9

/**
 * MOCTRn/MOSTATn Transmit Signal Enable Select bit.
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
 * MOFCRn Data Length Code bits in byte 3.
 *
 * Valid DLC values range from 0 to 8.
 */
#define BIT_DLC			0

/**
 * DLC bit count.
 */
#define CNT_DLC			4

/**
 * MOFCRn Message Mode Control bits in byte 0.
 */
#define BIT_MMC			0

/**
 * MMC bit count.
 */
#define CNT_MMC			4

/**
 * Regular message mode.
 */
#define MMC_DEFAULT		0

/**
 * Message is the base of an RX FIFO.
 */
#define MMC_RXBASEFIFO		1

/**
 * Message is the base of a TX FIFO.
 */
#define MMC_TXBASEFIFO		2

/**
 * Message is a TX FIFO slave.
 */
#define MMC_TXSLAVEFIFO		3

/**
 * Message is a source object for a gateway.
 */
#define MMC_GATEWAYSRC		4

/**
 * MOARn Extended CAN Identifier of Message Object n bits.
 */
#define BIT_IDEXT		0

/**
 * ID bit count.
 */
#define CNT_IDEXT		29

/**
 * MOARn Standard CAN Identifier of Message Object n bits.
 */
#define BIT_IDSTD		18

/**
 * ID bit count.
 */
#define CNT_IDSTD		11

/**
 * MOARn Identifier Extension Bit of Message Object n.
 */
#define BIT_IDE			29

/**
 * MOARn Priority Class bits.
 */
#define BIT_PRI			30

/**
 * PRI bit count.
 */
#define CNT_PRI			2

/**
 * List order based transmit priority.
 */
#define PRI_LIST		1

/**
 * CAN ID based transmit priority.
 */
#define PRI_ID			2

hsk_can_msg hsk_can_msg_create(const ulong id, const bool extended,
		const ubyte dlc) {
	hsk_can_msg msg;

	/*
	 * Fetch message into list of pending message objects.
	 */
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

	/*
	 * Set the DLC and message mode.
	 */
	CAN_ADLH = MOFCRn + (msg << OFF_MOn);
	CAN_DATA3 = (dlc <= 8 ? dlc : 8) << BIT_DLC;
	CAN_DATA0 = MMC_DEFAULT << BIT_MMC;
	CAN_AD_WRITE(0x9);

	/*
	 * Set ID.
	 */
	CAN_ADLH = MOARn + (msg << OFF_MOn);
	CAN_DATA01 = id << (extended ? BIT_IDEXT : BIT_IDSTD);
	CAN_DATA23 = (extended ? id >> (16 - BIT_IDEXT) : id << (BIT_IDSTD - 16)) \
		| ((ubyte)extended << (BIT_IDE - 16)) | (PRI_ID << (BIT_PRI - 16));
	CAN_AD_WRITE(0xF);

	/*
	 * Set up the message for reception.
	 */

	/* Adjust filtering mask to only accept complete ID matches. */
	CAN_ADLH = MOAMRn + (msg << OFF_MOn);
	CAN_DATA01 = -1;
	CAN_DATA23 = (1 << (BIT_MIDE - 16)) | ((1 << (CNT_AM - (16 - BIT_AM))) - 1);
	CAN_AD_WRITE(0xF);

	/* Set message up for receiving. */
	CAN_ADLH = MOCTRn + (msg << OFF_MOn);
	RESET_DATA = (1 << BIT_TXEN0) | (1 << BIT_TXEN1) | (1 << BIT_RXPND);
	SET_DATA = (1 << BIT_MSGVAL) | (1 << BIT_RXEN);
	CAN_AD_WRITE(0xF);

	return msg;
}

/**
 * MOSTATn List Allocation bits in byte 1.
 */
#define BIT_LIST		4

/**
 * LIST bit count.
 */
#define CNT_LIST		4

/**
 * MOSTATn Pointer to Next Message Object byte.
 */
#define MOSTATn_PNEXT		CAN_DATA3

/**
 * MOFGPRn bottom pointer byte.
 */
#define MOFGPRn_BOT		CAN_DATA0

/**
 * MOFGPRn top pointer byte.
 */
#define MOFGPRn_TOP		CAN_DATA1

/**
 * MOFGPRn current pointer byte.
 */
#define MOFGPRn_CUR		CAN_DATA2

/**
 * MOFGPRn select pointer byte.
 */
#define MOFGPRn_SEL		CAN_DATA3

/**
 * Move the selected message and its slaves to a different list.
 *
 * @param msg
 * 	The identifier of the message object
 * @param list
 * 	The list to move the message object to
 * @retval CAN_ERROR
 *	The given message object id is not valid
 * @retval 0
 *	Move successful
 * @private
 */
ubyte hsk_can_msg_move(const hsk_can_msg msg, const ubyte list) {
	/* Check whether this is a valid message ID. */
	if (msg >= HSK_CAN_MSG_MAX) {
		return CAN_ERROR;
	}

	/* Move message to the requested CAN node. */
	CAN_ADLH = PANCTR;
	PANCTR_READY();
	PANCMD = PAN_CMD_MOVE;
	PANAR1 = msg;
	PANAR2 = list;
	CAN_AD_WRITE(0xD);
	PANCTR_READY();

	return 0;
}

ubyte hsk_can_msg_connect(const hsk_can_msg msg, const hsk_can_node node) {
	/* Move message to the requested CAN node. */
	return hsk_can_msg_move(msg, LIST_NODEx + node);
}

ubyte hsk_can_msg_disconnect(const hsk_can_msg msg) {
	/* Move the message object to the list of pending message objects. */
	return hsk_can_msg_move(msg, LIST_PENDING);
}

ubyte hsk_can_msg_delete(const hsk_can_msg msg) {
	/* Move the message object into the list of unallocated objects. */
	return hsk_can_msg_move(msg, LIST_UNALLOC);
}

void hsk_can_msg_getData(const hsk_can_msg msg,
		ubyte * const msgdata) {
	ubyte dlc, i;

	/* Get the DLC. */
	CAN_ADLH = MOFCRn + (msg << OFF_MOn);
	CAN_AD_READ();
	dlc = (CAN_DATA3 >> BIT_DLC) & ((1 << CNT_DLC) - 1);

	CAN_ADLH = MODATALn + (msg << OFF_MOn);
	for (i = 0; i < dlc; i++) {
		switch (i % 4) {
		case 0:
			CAN_AD_READ() | AUAD_INC1;
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

void hsk_can_msg_setData(const hsk_can_msg msg,
		const ubyte * const msgdata) {
	ubyte dlc, i;

	/* Get the DLC. */
	CAN_ADLH = MOFCRn + (msg << OFF_MOn);
	CAN_AD_READ();
	dlc = (CAN_DATA3 >> BIT_DLC) & ((1 << CNT_DLC) - 1);

	CAN_ADLH = MODATALn + (msg << OFF_MOn);
	for (i = 0; i < dlc; i++) {
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
			break;
		}
	}
	if (i % 4) {
		CAN_AD_WRITE((1 << (i % 4)) - 1);
	}
}

void hsk_can_msg_send(const hsk_can_msg msg) {
	/* Request transmission. */
	CAN_ADLH = MOCTRn + (msg << OFF_MOn);
	SET_DATA = (1 << BIT_TXEN0) | (1 << BIT_TXEN1) | (1 << BIT_TXRQ) | (1 << BIT_DIR);
	RESET_DATA = (1 << BIT_RXEN);
	CAN_AD_WRITE(0xF);
}

bool hsk_can_msg_sent(const hsk_can_msg msg) {

	/* Get the message status. */
	CAN_ADLH = MOSTATn + (msg << OFF_MOn);
	CAN_AD_READ();
	if (!((CAN_DATA0 >> BIT_TXPND) & 1)) {
		return 0;
	}

	/* Reset the TXPND bit. */
	RESET_DATA = 1 << BIT_TXPND;
	CAN_AD_WRITE(RESET);

	return 1;
}

void hsk_can_msg_receive(const hsk_can_msg msg) {
	/* Return to rx mode. */
	CAN_ADLH = MOCTRn + (msg << OFF_MOn);
	SET_DATA = (1 << BIT_RXEN);
	RESET_DATA = (1 << BIT_TXEN0) | (1 << BIT_TXEN1) | (1 << BIT_TXRQ) | (1 << BIT_DIR);
	CAN_AD_WRITE(0xF);
}

bool hsk_can_msg_updated(const hsk_can_msg msg) {

	/* Get the message status. */
	CAN_ADLH = MOSTATn + (msg << OFF_MOn);
	CAN_AD_READ();
	if (!((CAN_DATA0 >> BIT_RXPND) & 1)) {
		return 0;
	}

	/* Reset the RXPND bit. */
	RESET_DATA = 1 << BIT_RXPND;
	CAN_AD_WRITE(RESET);

	return 1;
}

hsk_can_fifo hsk_can_fifo_create(ubyte size) {
	hsk_can_fifo base;
	hsk_can_msg top;

	/*
	 * Fetch message into list of pending message objects.
	 */
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
	base = PANAR1;

	/**
	 * <b>Slave Objects</b>
	 *
	 * Slave objects are put into the same list as the base message
	 * object, so it can be used as a slave as well.
	 *
	 * Always configure slave messages as TXSLAVEs, because in RXMODE the
	 * setting is ignored anyway.
	 */

	top = base;
	/* Grow slave objects. */
	while (size-- > 1) {
		/* Acquire a new slave object. */
		CAN_ADLH = PANCTR;
		PANCTR_READY();
		PANCMD = PAN_CMD_ALLOCBEHIND;
		PANAR2 = top;
		CAN_AD_WRITE(0xD);
		PANCTR_READY();

		/* Check for success. */
		if (PANAR2 & (1 << BIT_ERR)) {
			break;
		}
		top = PANAR1;

		/* Set message mode to TXSLAVE. */
		CAN_ADLH = MOFCRn + (top << OFF_MOn);
		CAN_AD_READ();
		CAN_DATA0 = CAN_DATA0 & ~(((1 << CNT_MMC) - 1) << BIT_MMC) \
			| (MMC_TXSLAVEFIFO << BIT_MMC);
		CAN_AD_WRITE(0x1);

		/* Reset TXEN1, RXPND and MSGVAL. */
		CAN_ADLH = MOCTRn + (top << OFF_MOn);
		RESET_DATA = (1 << BIT_TXEN1) | (1 << BIT_RXPND) | (1 << BIT_MSGVAL);
		CAN_AD_WRITE(RESET);

		/* Point to the base object. */
		CAN_ADLH = MOFGPRn + (top << OFF_MOn);
		MOFGPRn_CUR = base;
		CAN_AD_WRITE(0x4);
	}

	/*
	 * Create the FIFO base.
	 */
	/**
	 * <b>Message Pointers</b>
	 *
	 * MOFGPRn of the base object holds the message pointers that define
	 * the list boundaries. SEL will be used to keep track of where to
	 * read/write the next message when interacting with the FIFO.
	 */
	CAN_ADLH = MOFGPRn + (base << OFF_MOn);
	MOFGPRn_BOT = base;
	MOFGPRn_CUR = base;
	MOFGPRn_TOP = top;
	MOFGPRn_SEL = base;
	CAN_AD_WRITE(0xF);

	/* Set the message mode to RXBASE. */
	CAN_ADLH = MOFCRn + (base << OFF_MOn);
	CAN_AD_READ();
	CAN_DATA0 = MMC_RXBASEFIFO << BIT_MMC;
	CAN_AD_WRITE(0x9);

	/* Reset TXEN1, RXPND and MSGVAL. */
	CAN_ADLH = MOCTRn + (base << OFF_MOn);
	RESET_DATA = (1 << BIT_TXEN1) | (1 << BIT_RXPND) | (1 << BIT_MSGVAL);
	CAN_AD_WRITE(RESET);

	return base;
}

void hsk_can_fifo_setupRx(hsk_can_fifo fifo, const ulong id,
		const bool extended, const ubyte dlc) {
	hsk_can_msg top;

	/*
	 * Set the DLC and message mode.
	 */
	CAN_ADLH = MOFCRn + (fifo << OFF_MOn);
	CAN_DATA3 = (dlc <= 8 ? dlc : 8) << BIT_DLC;
	CAN_DATA0 = MMC_RXBASEFIFO << BIT_MMC;
	CAN_AD_WRITE(0x9);

	/*
	 * Set ID.
	 */
	CAN_ADLH = MOARn + (fifo << OFF_MOn);
	CAN_DATA01 = id << (extended ? BIT_IDEXT : BIT_IDSTD);
	CAN_DATA23 = (extended ? id >> (16 - BIT_IDEXT) : id << (BIT_IDSTD - 16)) \
		| ((ubyte)extended << (BIT_IDE - 16)) | (PRI_ID << (BIT_PRI - 16));
	CAN_AD_WRITE(0xF);

	/* Adjust filtering mask to only accept complete ID matches. */
	CAN_ADLH = MOAMRn + (fifo << OFF_MOn);
	CAN_DATA01 = -1 << BIT_AM;
	CAN_DATA23 = (1 << (BIT_MIDE - 16)) | ((1 << (CNT_AM - (16 - BIT_AM))) - 1);
	CAN_AD_WRITE(0xF);

	/* Enable RX and set message valid. */
	CAN_ADLH = MOCTRn + (fifo << OFF_MOn);
	RESET_DATA = (1 << BIT_TXEN0) | (1 << BIT_TXEN1);
	SET_DATA = (1 << BIT_MSGVAL) | (1 << BIT_RXEN);
	CAN_AD_WRITE(0xF);

	/* Get the FIFO top. */
	CAN_ADLH = MOFGPRn + (fifo << OFF_MOn);
	CAN_AD_READ();
	top = MOFGPRn_TOP;

	/* Set all messages in the FIFO valid. */
	while (fifo != top) {
		/* Get the next message. */
		CAN_ADLH = MOSTATn + (fifo << OFF_MOn);
		CAN_AD_READ();
		fifo = MOSTATn_PNEXT;

		/* Set message valid. */
		CAN_ADLH = MOCTRn + (fifo << OFF_MOn);
		RESET_DATA = (1 << BIT_TXEN1);
		SET_DATA = (1 << BIT_MSGVAL);
		CAN_AD_WRITE(0xF);
	}
}

void hsk_can_fifo_setRxMask(const hsk_can_fifo fifo, ulong msk) {

	/* Shift msk into position. */
	CAN_ADLH = MOARn + (fifo << OFF_MOn);
	CAN_AD_READ();
	if ((CAN_DATA3 >> (BIT_IDE - 24)) & 1) {
		msk &= (1ul << CNT_IDEXT) - 1;
		msk <<= BIT_IDEXT;
	} else {
		msk &= (1ul << CNT_IDSTD) - 1;
		msk <<= BIT_IDSTD;
	}

	/* Adjust filtering mask. */
	CAN_ADLH = MOAMRn + (fifo << OFF_MOn);
	CAN_DATA01 = msk;
	CAN_DATA23 = (1 << (BIT_MIDE - 16)) | (msk >> (16 - BIT_AM));
	CAN_AD_WRITE(0xF);

}

/**
 * Move the selected FIFO to a different list.
 *
 * @param fifo
 * 	The identifier of the FIFO
 * @param list
 * 	The list to move the FIFO to
 * @retval CAN_ERROR
 *	The given FIFO id is not valid
 * @retval 0
 *	Move successful
 * @private
 */
ubyte hsk_can_fifo_move(hsk_can_fifo fifo, const ubyte list) {
	ubyte top, pre, next;

	/* Check whether this is a valid ID. */
	if (fifo >= HSK_CAN_MSG_MAX) {
		return CAN_ERROR;
	}

	/* Check whether this actually is a FIFO. */
	CAN_ADLH = MOFCRn + (fifo << OFF_MOn);
	CAN_AD_READ();
	switch ((CAN_DATA0 >> BIT_MMC) & ((1 << CNT_MMC) -1)) {
	case MMC_TXBASEFIFO:
	case MMC_RXBASEFIFO:
		break;
	default:
		return CAN_ERROR;
	}

	/* Get the top. */
	CAN_ADLH = MOFGPRn + (fifo << OFF_MOn);
	CAN_AD_READ();
	top = MOFGPRn_TOP;

	/* Get the next message in the list. */
	CAN_ADLH = MOSTATn + (fifo << OFF_MOn);
	CAN_AD_READ();
	next = MOSTATn_PNEXT;

	/* Move message to the requested CAN node. */
	CAN_ADLH = PANCTR;
	PANCTR_READY();
	PANCMD = PAN_CMD_MOVE;
	PANAR1 = fifo;
	PANAR2 = list;
	CAN_AD_WRITE(0xD);
	PANCTR_READY();

	/* Take care of slaves. */
	while (top != fifo) {
		/* Get the new set of messages to work on. */
		pre = fifo;
		fifo = next;

		/* Get the next message in the list. */
		CAN_ADLH = MOSTATn + (fifo << OFF_MOn);
		CAN_AD_READ();
		next = MOSTATn_PNEXT;

		/* Move the slave. */
		CAN_ADLH = PANCTR;
		PANCTR_READY();
		PANCMD = PAN_CMD_MOVEBEHIND;
		PANAR1 = fifo;
		PANAR2 = pre;
		CAN_AD_WRITE(0xD);
		PANCTR_READY();
	}

	return 0;
}

ubyte hsk_can_fifo_connect(const hsk_can_fifo fifo,
		const hsk_can_node node) {
	/* Move message to the requested CAN node. */
	return hsk_can_fifo_move(fifo, LIST_NODEx + node);
}

ubyte hsk_can_fifo_disconnect(const hsk_can_fifo fifo) {
	/* Move the FIFO to the list of pending message objects. */
	return hsk_can_fifo_move(fifo, LIST_PENDING);
}

ubyte hsk_can_fifo_delete(const hsk_can_fifo fifo) {
	/* Move the FIFO into the list of unallocated objects. */
	return hsk_can_fifo_move(fifo, LIST_UNALLOC);
}

void hsk_can_fifo_next(const hsk_can_fifo fifo) {

	/* Get the current selection. */
	CAN_ADLH = MOFGPRn + (fifo << OFF_MOn);
	CAN_AD_READ();
	/* The top entry is selected start from the bottom. */
	if (MOFGPRn_SEL == MOFGPRn_TOP) {
		MOFGPRn_SEL = MOFGPRn_BOT;
		CAN_AD_WRITE(0x8);
		return;
	}

	/* Get the next message. */
	CAN_ADLH = MOSTATn + (MOFGPRn_SEL << OFF_MOn);
	CAN_AD_READ();

	/* Write PNEXT into SEL. */
	CAN_ADLH = MOFGPRn + (fifo << OFF_MOn);
	MOFGPRn_SEL = MOSTATn_PNEXT;
	CAN_AD_WRITE(0x8);
}

bool hsk_can_fifo_updated(const hsk_can_fifo fifo) {
	/* Get the current selection. */
	CAN_ADLH = MOFGPRn + (fifo << OFF_MOn);
	CAN_AD_READ();
	return hsk_can_msg_updated(MOFGPRn_SEL);
}

void hsk_can_fifo_getData(const hsk_can_fifo fifo,
		ubyte * const msgdata) {
	/* Get the current selection. */
	CAN_ADLH = MOFGPRn + (fifo << OFF_MOn);
	CAN_AD_READ();
	hsk_can_msg_getData(MOFGPRn_SEL, msgdata);
}

ulong hsk_can_fifo_getId(const hsk_can_fifo fifo) {
	ulong result;

	/* Get the current selection. */
	CAN_ADLH = MOFGPRn + (fifo << OFF_MOn);
	CAN_AD_READ();
	CAN_ADLH = MOARn + (MOFGPRn_SEL << OFF_MOn);
	CAN_AD_READ();

	#define extended ((CAN_DATA3 >> (BIT_IDE - 24)) & 1)
	result = CAN_DATA23;
	result <<= 16;
	result |= CAN_DATA01;
	result >>= extended ? BIT_IDEXT : BIT_IDSTD;
	result &= (1ul << (extended ? CNT_IDEXT : CNT_IDSTD)) - 1;
	return result;
	#undef extended
}

/**
 * Sets a signal value in a data field.
 *
 * @param msg
 * 	The message data field to write into
 * @param bitPos
 * 	The bit position of the signal
 * @param bitCount
 * 	The length of the signal
 * @param value
 * 	The signal value to write into the data field
 * @private
 */
void hsk_can_data_setIntelSignal(ubyte * const msg,
		ubyte bitPos, char bitCount, ulong idata value) {
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
 * The start position of a signal is supposed to point to the most
 * significant bit of a signal. Consider a 10 bit message, the bits are
 * indexed:
 * \code
 * 	 9  8  7  6  5  4  3  2  1  0
 * \endcode
 *
 * In that example bit 9 is the most significant bit, bit 0 the least
 * significant. The most significant bit of a signal will be stored in
 * the most significant bits of the message. Under the assumption that
 * the start bit is 2, the message would be stored in the following
 * bits:
 * \code
 *	Signal	 9  8  7  6  5  4  3  2  1  0
 * 	Message	 2  1  0 15 14 13 12 11 10  9
 * \endcode
 *
 * Note that the signal spreads to the most significant bits of the next
 * byte. Special care needs to be taken, when mixing little and big endian
 * signals. A 10 bit little endian signal with start bit 2 would cover the
 * following message bits:
 * \code
 *	Signal	 9  8  7  6  5  4  3  2  1  0
 *	Message 11 10  9  8  7  6  5  4  3  2
 * \endcode
 *
 * @param msg
 * 	The message data field to write into
 * @param bitPos
 * 	The bit position of the signal
 * @param bitCount
 * 	The length of the signal
 * @param value
 * 	The signal value to write into the data field
 * @private
 */
void hsk_can_data_setMotorolaSignal(ubyte * const msg,
		ubyte bitPos, char bitCount, ulong idata value) {
	char bits;

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

void hsk_can_data_setSignal(ubyte * const msg, const bool endian,
		const bool sign, const ubyte bitPos,
		const char bitCount, const ulong idata value) {
	/**
	 * The sign parameter is not required for setting signals, it is just
	 * there so that one signal configuration tuple suffices for
	 * hsk_can_data_setSignal() and hsk_can_data_getSignal().
	 */
	/* Shut up the compiler about unused parameters. */
	if (sign);

	switch((ubyte)endian) {
	case CAN_ENDIAN_INTEL:
		hsk_can_data_setIntelSignal(msg, bitPos, bitCount, value);
		break;
	case CAN_ENDIAN_MOTOROLA:
		hsk_can_data_setMotorolaSignal(msg, bitPos, bitCount, value);
		break;
	}
}

/**
 * Get a little endian signal value from a data field.
 *
 * @param msg
 * 	The message data field to read from
 * @param sign
 *	Indicates whether the value has a signed type
 * @param bitPos
 * 	The bit position of the signal
 * @param bitCount
 * 	The length of the signal
 * @return
 *	The signal from the data field msg
 * @private
 */
ulong hsk_can_data_getIntelSignal(const ubyte * const msg,
		const bool sign, ubyte bitPos, char bitCount) {
	ulong value = 0;
	ubyte shift = 0;
	while (bitCount > 0) {
		/* Get the bottommost part of the value. */
		value |= ((msg[bitPos / 8] >> (bitPos % 8)) & ((1ul << bitCount) - 1)) << shift;
		/* Update counters. */
		bitCount -= 8 - (bitPos % 8);
		shift += 8 - (bitPos % 8);
		bitPos += 8 - (bitPos % 8);
	}

	if (sign && (value >> (shift + bitCount - 1))) {
		return ((-1ul) << (shift + bitCount)) | value;
	}
	return value;
}

/**
 * Get a big endian signal value from a data field.
 *
 * @see hsk_can_data_setMotorolaSignal()
 *	For details on the difference between big and little endian
 * @param msg
 * 	The message data field to read from
 * @param sign
 *	Indicates whether the value has a signed type
 * @param bitPos
 * 	The bit position of the signal
 * @param bitCount
 * 	The length of the signal
 * @return
 *	The signal from the data field msg
 * @private
 */
ulong hsk_can_data_getMotorolaSignal(const ubyte * const msg,
		const bool sign, ubyte bitPos, char bitCount) {
	ulong value = 0;
	ubyte shift = bitCount;
	char bits;

	while (bitCount > 0) {
		/* Get the number of bits to work on. */
		bits = bitPos % 8 + 1;
		bits = bits < bitCount ? bits : bitCount;
		/* Get the most significant bits. */
		bitCount -= bits;
		value |= ((msg[bitPos / 8] >> (bitPos % 8 + 1 - bits)) & ((1ul << bits) - 1)) << bitCount;
		/* Get the next bit position. */
		bitPos = (bitPos & ~(0x07)) + 15;
	}

	if (sign && (value >> (shift - 1))) {
		return ((-1ul) << shift) | value;
	}
	return value;
}

ulong hsk_can_data_getSignal(const ubyte * const msg, const bool endian,
		const bool sign, const ubyte bitPos,
		const char bitCount) {
	switch((ubyte)endian) {
	case CAN_ENDIAN_INTEL:
		return hsk_can_data_getIntelSignal(msg, sign, bitPos, bitCount);
	case CAN_ENDIAN_MOTOROLA:
		return hsk_can_data_getMotorolaSignal(msg, sign, bitPos, bitCount);
	default:
		/* This cannot actually happen. */
		return CAN_ERROR;
	}
}

