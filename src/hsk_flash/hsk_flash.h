/** \file
 * HSK Flash Facility headers
 *
 * This file contains function prototypes to menage information that survives
 * a reset and allow storage within the D-Flash.
 *
 * It provides the \ref FLASH_STRUCT_FACTORY to create a struct with data that
 * can be stored with hsk_flash_write() and recovered with hsk_flash_init().
 *
 * The D-Flash is used as a ring buffer, this distributes writes over the
 * entire flash to gain the maximum achievable lifetime. The lifetime
 * expectancy depends on your usage scenario and the size of the struct.
 *
 * Refer to section 3.3 table 20 and table 21 of the
 * <a href="../contrib/Microcontroller-XC87x-Data-Sheet-V15-infineon.pdf">
 * XC87x data sheet</a> for D-Flash life times.
 *
 * Complete coverage of the D-Flash counts as a single D-Flash cycle. Thus the
 * formula for the expected number of write calls is:
 * \f[
 * 	writes = \lfloor 4096 / sizeof(struct) \rfloor * expectedcycles
 * \f]
 *
 * - \c expectedcycles
 * 	- The expected number of possible write cycles
 * 	  depending on the usage scenario in table 20
 * - \c sizeof(struct)
 * 	- The number of bytes the struct covers
 * - \c floor()
 * 	- Round down to the next smaller integer
 *
 * E.g. to store 20 bytes of configuration data, the struct factory adds 2
 * bytes overhead to be able to check the consistency of written data, so
 * \f$ sizeof(struct) = 22 \f$.  Expecting that most of the µC use is
 * within the first year, table 20 suggests that
 * \f$ expectedcycles = 100000 \f$. In that case the expected number of
 * possible hsk_flash_write() calls is 18.6 million.
 *
 * @author kami
 *
 * \section flash_byte_order Byte Order
 *
 * C51 stores multiple byte variables in big endian order, whereas the
 * DPTR register, several SFRs and SDCC use little endian.
 *
 * If the data struct contains multibyte members such as int/uword or
 * long/ulong, this can lead to data corruption, when switching compilers.
 *
 * Both the checksum and identifier are single byte values and thus will
 * still match after a compiler switch, causing multibyte values to be
 * restored from the flash with the wrong byte order.
 *
 * A byte order change can be detected with a byte order word in the struct.
 * A BOW initialized with 0x1234 would read 0x3412 after a an order change.
 *
 * The suggested solution is to only create struct members with byte wise
 * access. E.g. a ulong member may be defined in the following way:
 * \code
 * ubyte ulongMember[sizeof(ulong)];
 * \endcode
 *
 * The value can be set like this:
 * \code
 * myStruct.ulongMember[0] = ulongValue;
 * myStruct.ulongMember[1] = ulongValue >> 8;
 * myStruct.ulongMember[2] = ulongValue >> 16;
 * myStruct.ulongMember[3] = ulongValue >> 24;
 * \endcode
 *
 * Reading works similarly:
 * \code
 * ulongValue  = (ubyte)myStruct.ulongMember[0];
 * ulongValue |= (uword)myStruct.ulongMember[1] << 8;
 * ulongValue |= (ulong)myStruct.ulongMember[2] << 16;
 * ulongValue |= (ulong)myStruct.ulongMember[3] << 24;
 * \endcode
 *
 * Another alternative is to use a single ubyte[] array and store/read
 * all data with the hsk_can_data_setSignal()/\ref hsk_can_data_getSignal()
 * functions. Due to the bit addressing of CAN message data the
 * maximum length of such an array would be 32 bytes (256bits).
 *
 * An advantage would be that less memory is required, because data
 * no longer needs to be byte aligned.
 */

#ifndef _HSK_PERSIST_H_
#define _HSK_PERSIST_H_

/*
 * Required for SDCC to propagate ISR prototypes.
 */
#ifdef SDCC
#include "../hsk_isr/hsk_isr.isr"
#endif /* SDCC */

/**
 * Ensure that a flash memory layout is defined.
 *
 * Either XC878_16FF (64k flash) or XC878_13FF(52k flash) are supported.
 * XC878_16FF is the default.
 */
#if !defined XC878_16FF && !defined XC878_13FF
#define XC878_16FF
#endif

/**
 * Used to create a struct that can be used with the hsk_flash_init()
 * function.
 *
 * The hsk_flash_init() function expects certain fields to exist, in the
 * struct, which are used to ensure the consistency of data in the flash.
 *
 * The following example shows how to create a struct named storableData:
 * \code
 * FLASH_STRUCT_FACTORY(
 * 	ubyte storableByte;
 * 	uword storableWord;
 * ) storableData;
 * \endcode
 *
 * @param members
 *	Struct member definitions
 */
#define FLASH_STRUCT_FACTORY(members)	\
	/** \struct hsk_flash_struct "" ""
	 * This struct is a template for data that can be written to the D-Flash.
	 * It is created by invoking the \ref FLASH_STRUCT_FACTORY macro.
	 */\
	volatile struct hsk_flash_struct {\
		/**
		 * For data integrity/compatibilty detection.
		 *
		 * @private
		 */\
		ubyte hsk_flash_prefix;\
		\
		members\
		\
		/**
		 * For data integrity detection.
		 *
		 * @private
		 */\
		ubyte hsk_flash_chksum;\
	} xdata

/**
 * Returned by hsk_flash_init() when the µC boots for the first time.
 *
 * This statements holds true <i>as far as can be told</i>. I.e. a first
 * boot is diagnosed when all attempts to recover data have failed.
 *
 * Two scenarios may cause this:
 * - No valid data has yet been written to the D-Flash
 * - The latest flash data is corrupted, may happen in case of power
 *   down during write
 */
#define FLASH_PWR_FIRST		0

/**
 * Returned by hsk_flash_init() after booting from a reset without power
 * loss.
 *
 * The typical mark of a reset is that \c xdata memory still holds data from
 * the previous session. If such data is found it will just be picked up.
 *
 * For performance reasons access to the struct is not guarded, which means
 * that there can be no protection against data corruption, such as might
 * be caused by a software bug like an overflow.
 */
#define FLASH_PWR_RESET		1

/**
 * Returned by hsk_flash_init() during power on, if valid data was recovered
 * from the D-Flash.
 *
 * A power on is detected when two criteria are met:
 * - Data could not be recovered from \c xdata memory
 * - Valid data was recovered from the D-Flash
 */
#define FLASH_PWR_ON		2

/**
 * Recovers a struct from a previous session and sets everything up for
 * storage of changes.
 *
 * There are two modes of recovery. After a fresh boot the data can be
 * recovered from flash, if previously stored there. After a simple reset the
 * data can still be found in XRAM and recovery can be sped up.
 *
 * If recovery fails entirely all members of the struct will be set to 0.
 *
 * @param version
 *	Version number of the persisted data structure, used to prevent
 *	initilization with incompatible data
 * @param ptr
 *	A pointer to the xdata struct/array to persist
 * @param size
 *	The size of the data structure to persist
 * @retval FLASH_PWR_FIRST
 *	No valid data was recovered
 * @retval FLASH_PWR_RESET
 *	Continue operation after a reset
 * @retval FLASH_PWR_ON
 *	Data restore from the D-Flash succeeded
 */
ubyte hsk_flash_init(void xdata * const ptr, const uword __xdata size,
		const ubyte __xdata version);

/**
 * Writes the current data to the D-Flash.
 *
 * Ongoing writes are interrupted. Ongoing deletes are interrupted unless
 * there is insufficient space left to write the data.
 *
 * @retval 1
 *	The D-Flash write is on the way
 * @retval 0
 *	Not enough free D-Flash space to write, try again later
 */
bool hsk_flash_write(void);

#endif /* _HSK_PERSIST_H_ */

