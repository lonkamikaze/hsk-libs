/** \file
 * HSK Persistence Facility headers
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

void hsk_persist_write(void);

#endif /* _HSK_PERSIST_H_ */

