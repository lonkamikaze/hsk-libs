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
 * @param members
 *	Struct member definitions
 */
#define HSK_FLASH_STRUCT_FACTORY(members)	\
struct {\
	ubyte hsk_flash_prefix;\
	members\
	ubyte hsk_flash_chksum;\
	ubyte hsk_flash_postfix;\
} xdata

ubyte hsk_flash_init(void xdata * idata ptr, uword idata size,
		ubyte idata version);
void hsk_flash_write(void);

#endif /* _HSK_PERSIST_H_ */

