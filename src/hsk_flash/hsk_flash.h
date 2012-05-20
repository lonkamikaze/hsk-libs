/** \file
 * HSK Flash Facility headers
 *
 * This file contains function prototypes to menage information that survives
 * a reset and allow storage within the D-Flash.
 *
 * @author kami
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
struct {\
	/** For data integrity/compatibilty detection. @private */\
	ubyte hsk_flash_prefix;\
	members\
	/** For data integrity detection. @private */\
	ubyte hsk_flash_chksum;\
	/** For data integrity/compatibilty detection. @private */\
	ubyte hsk_flash_postfix;\
} xdata

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
 * @retval 0
 *	No valid data was recovered
 * @retval 1
 *	Continue operation after a reset
 * @retval 2
 *	Data restore from the D-Flash succeeded
 */
ubyte hsk_flash_init(void xdata * idata ptr, uword idata size,
		ubyte idata version);

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

