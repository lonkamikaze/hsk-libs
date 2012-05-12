/** \file
 * HSK Persistence Facility implementation
 *
 * This file implements the persistence functions defined in hsk_persist.h.
 *
 * @author kami
 *
 * \section persist_registers Flash Registers
 *
 * All registers are in the mapped register are, i.e. RMAP=1 must be set to
 * access them.
 *
 * \subsection persist_timer Flash Timer
 *
 * Non-blocking flash reading/writing is controlled by a dedicated flash
 * timer.
 */

#include <Infineon/XC878.h>

#include "hsk_persist.h"

#include "../hsk_isr/hsk_isr.h"

/*
 * XC878-16FF code/flash layout.
 */
#ifdef XC878_16FF

/**
 * XC878-16FF code page that has the Boot ROM and XRAM mapped into it.
 */
#define PAGE_RAM		2

/**
 * XC878-16FF code page that has the flash.
 */
#define PAGE_FLASH		0

/**
 * XC878-16FF start address of the P-Flash.
 */
#define ADDR_PFLASH		0x0000

/**
 * XC878-16FF length of the P-Flash.
 */
#define LEN_PFLASH		(60u << 10)

/**
 * XC878-16FF ld() of the number of bytes in a P-Flash page.
 */
#define BYTES_PAGE_PFLASH	9

/**
 * XC878-16FF ld() of the number of bytes in a P-Flash wordline.
 */
#define BYTES_WORDLINE_PFLASH	6

/**
 * XC878-16FF start address of the D-Flash.
 */
#define ADDR_DFLASH		0xF000

/**
 * XC878-16FF length of the D-Flash.
 */
#define LEN_DFLASH		(4u << 10)

/**
 * XC878-16FF ld() of the number of bytes in a D-Flash page.
 */
#define BYTES_PAGE_DFLASH	6

/**
 * XC878-16FF ld() of the number of bytes in a D-Flash wordline.
 */
#define BYTES_WORDLINE_DFLASH	5

/**
 * XC878-16FF start address of the Boot ROM.
 */
#define ADDR_ROM		0xC000

/**
 * XC878-16FF length of the Boot ROM.
 */
#define LEN_ROM			(8u << 10)

/**
 * XC878-16FF start address of the XRAM.
 */
#define ADDR_XRAM		0xF000

/**
 * XC878-16FF length of the XRAM.
 */
#define LEN_XRAM		(3u << 10)

/*
 * XC878-13FF code/flash layout.
 */
#elif defined XC878_13FF

/**
 * XC878-13FF code page that has the Boot ROM and XRAM mapped into it.
 */
#define PAGE_RAM		0

/**
 * XC878-13FF code page that has the flash.
 */
#define PAGE_FLASH		0

/**
 * XC878-13FF start address of the P-Flash.
 */
#define ADDR_PFLASH		0x0000

/**
 * XC878-13FF length of the P-Flash.
 */
#define LEN_PFLASH		(48u << 10)

/**
 * XC878-13FF ld() of the number of bytes in a P-Flash page.
 */
#define BYTES_PAGE_PFLASH	9

/**
 * XC878-13FF ld() of the number of bytes in a P-Flash wordline.
 */
#define BYTES_WORDLINE_PFLASH	6

/**
 * XC878-13FF start address of the D-Flash.
 */
#define ADDR_DFLASH		((ubyte code *)0xE000)

/**
 * XC878-13FF length of the D-Flash.
 */
#define LEN_DFLASH		(4u << 10)

/**
 * XC878-13FF ld() of the number of bytes in a D-Flash page.
 */
#define BYTES_PAGE_DFLASH	6

/**
 * XC878-13FF ld() of the number of bytes in a D-Flash wordline.
 */
#define BYTES_WORDLINE_DFLASH	5

/**
 * XC878-13FF start address of the Boot ROM.
 */
#define ADDR_ROM		0xC000

/**
 * XC878-13FF length of the Boot ROM.
 */
#define LEN_ROM			(8u << 10)

/**
 * XC878-13FF start address of the XRAM.
 */
#define ADDR_XRAM		0xF000

/**
 * XC878-13FF length of the XRAM.
 */
#define LEN_XRAM		(3u << 10)

/*
 * Missing code/flash layout.
 */
#else
#error Either XC878_16FF or XC878_13FF must be defined
#endif

/**
 * Bytewise access to the D-Flash area.
 */
const ubyte code at ADDR_DFLASH hsk_persist_dflash[LEN_DFLASH];

/**
 * Bytewise access to the P-Flash area.
 */
const ubyte code at ADDR_PFLASH hsk_persist_pflash[LEN_PFLASH];

/**
 * P-Flash Control Register.
 */
SFR(FCON,	0xD1);

/**
 * D-Flash Control Register
 */
SFR(EECON,	0xD2);

/**
 * Flash Control and Status Register.
 */
SFR(FCS,	0xD3);

/**
 * Flash Error Address Register Low.
 */
SFR(FEAL,	0xD4);

/**
 * Flash Error Address Register High.
 */
SFR(FEAH,	0xD5);

/**
 * Flash Error Address Register Low and High (16 bits).
 */
SFR16(FEALH,	0xD4);

/**
 * Flash Timer Value Register.
 */
SFR(FTVAL,	0xD6);

/**
 * Flash Control and Status Register 1.
 */
SFR(FCS1,	0xDD);

/**
 * FCON/EECON Program Bit.
 */
#define BIT_PROG	0

/**
 * FCON/EECON Non-Volatile Store Bit.
 */
#define BIT_NVSTR	3

/**
 * FCON/EECON Y-Address Enable Bit.
 */
#define BIT_YE		5

/**
 * FCS Flash Timer Enable Bit.
 */
#define BIT_FTEN	5

/**
 * FTVAL Overflow Value bits.
 */
#define BIT_OFVAL 	0

/**
 * OFVAL bit count.
 */
#define CNT_OFVAL	7

/**
 * FTVAL MODE bit.
 *
 * \code
 *	Mode	Value	Effect
 *	Program	0	1 count per CCLK (24MHz) clock cycle
 *	Erase	1	1 count per CCLK/2^12 clock cycles
 * \endcode
 */
#define BIT_MODE	7

/**
 * NMICON Flash Timer NMI Enable bit.
 */
#define BIT_NMIFLASH	2

#ifdef SDCC
/**
 * MOVC @(DPTR++),A instruction.
 */
#define MOVCI	.db	0xA5

/**
 * Code data pointers need the code keyword just like in C51.
 */
#undef code
#define code	__code

#elif defined __C51__
/**
 * MOVC @(DPTR++),A instruction.
 */
#define MOVCI	db	0xA5

#else
#error Assembly code for the current compiler missing
#endif

/**
 * The state to use when starting to write to the flash.
 */
#define STATE_WRITE	0

/**
 * Holds the persistence configuration.
 */
struct {
	/**
	 * The pointer to the data structure to persist.
	 */
	ubyte xdata * ptr;

	/**
	 * The size of the data structure to persist.
	 */
	uword size;

	/**
	 * The useable amount of D-Flash.
	 */
	uword wrap;

	/**
	 * The offset of the oldest data in the D-Flash.
	 */
	uword oldest;

	/**
	 * The offset of the latest data in the D-Flash.
	 */
	uword latest;

	/**
	 * The prefix/postfix to identify the data structure in the flash.
	 *
	 * It consist of the last 6 bits of the version and two alternating
	 * bits to make sure the value can neither become 0x00 nor 0xff.
	 *
	 * Pre-/postfixing the ident ensures that the data was completely
	 * written.
	 */
	ubyte ident;

	/**
	 * The current state of the flash ISR state machine.
	 */
	ubyte state;
} xdata hsk_persist;

/**
 * A pointer to the flash target address.
 *
 * Not in a struct for easier inline assembler access.
 */
ubyte code * xdata hsk_persist_flashDptr;

/**
 * A pointer to the xdata src address.
 *
 * Not in a struct for easier inline assembler access.
 */
ubyte xdata * xdata hsk_persist_xdataDptr;

/**
 * Flash delete/write state machine.
 *
 * @private
 */
void hsk_persist_isr_nmiflash(void) {

	SET_RMAP();

	switch(hsk_persist.state) {
	/**
	 * Implements the procedure called "Program Operation" from the XC878
	 * UM 1.1.
	 */
	case STATE_WRITE:
		/* 1.
		 * Set the bit FCON.PROG (P-Flash) or EECON.PROG (D-Flash) to
		 * signal the start of a programming cyle. */
		EECON |= 1 << BIT_PROG;

		/* 2.
		 * Execute a “MOVC” instruction with a dummy data to any
		 * address in the same wordline as the address to be accessed.
		 */
#ifdef SDCC
__asm
		; Backup used registers
		push	ar0
		; Load hsk_persist_flashDptr into r0, a
		mov	dptr,#_hsk_persist_flashDptr
		movx	a,@dptr
		mov	r0,a
		inc	dptr
		movx	a,@dptr
		; Follow hsk_persist_flashDptr
		mov	dpl,r0
		mov	dph,a
		MOVCI
		; Restore used registers
		pop	ar0
__endasm;
#elif defined __C51__
#endif

		/* 3.
		 * Delay for a minimum of 5 us (Tvns). */
		FCS |= 1 << BIT_FTEN;
		
		hsk_persist.state++;
		break;
	case 1:
		/* Turn off the timer after 5µs. */
		FCS &= ~(1 << BIT_FTEN);

		/* 4.
		 * Set the bit FCON/EECON.NVSTR for charge pump to drive high
		 * voltage. */
		EECON |= 1 << BIT_NVSTR;


		/* 5.
		 * Delay for a minimum of 10 us (Tpgs). */
		FCS |= 1 << BIT_FTEN;

		/* fallthrough */
	case 2:
		hsk_persist.state++;
		break;
	case 3:
		/* Turn the timer off after 10 µs. */
		FCS &= ~(1 << BIT_FTEN);

	hsk_persist_write_loop:
		/* 6.
		 * Execute a “MOVC” instruction to the flash address to be
		 * accessed. FCON/EECON.YE and FCS.FTEN is set by hardware
		 * at next clock cycle (YE hold time of 40 ns is needed). */
#ifdef SDCC
__asm
		; Backup used registers
		push	ar2
		push	ar1
		push	ar0
		; Load hsk_persist_flashDptr into r0, r1
		mov	dptr,#_hsk_persist_flashDptr
		movx	a,@dptr
		mov	r0,a
		inc	dptr
		movx	a,@dptr
		mov	r1,a
		; Load hsk_persist_xdataDptr into r2, a
		mov	dptr,#_hsk_persist_xdataDptr
		movx	a,@dptr
		mov	r2,a
		inc	dptr
		movx	a,@dptr
		; Follow hsk_persist_xdataDptr
		mov	dpl,r2
		mov	dph,a
		movx	a,@dptr
		; Follow hsk_persist_flashDptr
		mov	dpl,r0
		mov	dph,r1
		MOVCI
		; Restore used registers
		pop	ar0
		pop	ar1
		pop	ar2
__endasm;
#elif defined __C51__
#endif
		/* 7.
		 * Delay for a minimum of 20 us but not longer than 40 us
		 * (Tprog). */
		FCS |= 1 << BIT_FTEN;

		/* fallthrough */
	case 4:
	case 5:
	case 6:
		hsk_persist.state++;
		break;
	case 7:
		/* Turn the timer off after 20µs. */
		FCS &= ~(1 << BIT_FTEN);

		/* 8.
		 * Clear the bits FCON/EECON.YE and FCS.FTEN respectively. */
		EECON &= ~(1 << BIT_YE);

		/* 9.
		 * Repeat steps 6 to 8 for any further programming of data to
		 * the same row. */
		//TODO

		hsk_persist.state++;
		/* fallthrough */
	case 8:
		/* 10.
		 * Clear the bit FCON/EECON.PROG. */
		EECON &= ~(1 << BIT_PROG);
		/* 11.
		 * Delay for a minimum of 5 us (Tnvh) */
		FCS |= 1 << BIT_FTEN;
		hsk_persist.state++;
		break;
	case 9:
		/* 12.
		 * Clear the bit FCON/EECON.NVSTR. */
		EECON &= ~(1 << BIT_NVSTR);
		/* 13.
		 * Delay for a minimum of 1 us (Trcv). */
		/* Actually just wait for another 5µs. */
		hsk_persist.state++;
		break;
	case 10:
		/* Turn the timer off after 5µs. */
		FCS &= ~(1 << BIT_FTEN);
		break;
	}
}

/**
 * Fetches a stored data structure from the DFLASH into XRAM and sets
 * everything up to write updates back to the DFLASH.
 *
 * @param version
 *	Version number of the persisted data structure, used to prevent
 *	initilization with incompatible data
 * @param ptr
 *	A pointer to the xdata struct/array to persist
 * @param size
 *	The size of the data structure to persist
 * @return
 * \code
 *	1	Loading persisted data succeeded
 *	0	No valid data was found in the D-Flash
 * \endcode
 */
bool hsk_persist_init(ubyte xdata * idata ptr, uword idata size,
		ubyte idata version) {
	/* Setup the xdata area to persist. */
	hsk_persist.ptr = ptr;
	hsk_persist.size = size;
	hsk_persist.wrap = sizeof(hsk_persist_dflash) / (size + 2) * (size + 2);
	hsk_persist.ident = (version & 0x3f) | 0x40;

	/* Activate NMIFLASH interrupt to allow the use of the state
	 * machine. */
	hsk_isr14.NMIFLASH = &hsk_persist_isr_nmiflash;
	NMICON |= 1 << BIT_NMIFLASH;

	/* Find an unused block. */
	for (hsk_persist.oldest = 0;
		hsk_persist_dflash[hsk_persist.oldest] != 0xff && hsk_persist.oldest < sizeof(hsk_persist_dflash);
		hsk_persist.oldest += size + 2);

	/* No free blocks at all, mass delete obligatory! */
	if (hsk_persist.oldest >= sizeof(hsk_persist_dflash)) {
		//TODO execute mass delete with active waiting
		hsk_persist.oldest = 0;
	}

	/* Walk left, seek the newest data. */
	for (hsk_persist.latest = (hsk_persist.wrap - size - 2 + hsk_persist.oldest) % hsk_persist.wrap;
		hsk_persist_dflash[hsk_persist.latest] == 0xff && hsk_persist.latest != hsk_persist.oldest;
		hsk_persist.latest = (hsk_persist.wrap + hsk_persist.latest - size - 2) % hsk_persist.wrap);

	/* Walk right, seek the oldest data. */
	for (hsk_persist.oldest = (hsk_persist.oldest + size + 2) % hsk_persist.wrap;
		hsk_persist_dflash[hsk_persist.oldest] == 0xff && hsk_persist.oldest != hsk_persist.latest;
		hsk_persist.oldest = (hsk_persist.oldest + size + 2) % hsk_persist.wrap);

	/* Validate the latest data. */
	if (hsk_persist_dflash[hsk_persist.latest] != hsk_persist.ident \
			|| hsk_persist_dflash[hsk_persist.latest] != hsk_persist.ident) {
		return 0;
	} else {
		/* Copy persisted data. */
		for (hsk_persist_flashDptr = (ubyte code *)hsk_persist_dflash + hsk_persist.latest + 1;
			size > 0; size--, ptr++, hsk_persist_flashDptr++) {
			ptr[0] = hsk_persist_flashDptr[0];
		}
		return 1;
	}
}

void hsk_persist_write(void) {
	SET_RMAP();
	/* Set flash timer mode for writing. */
	FTVAL &= ~(1 << BIT_MODE);
	/* Kick off the ISR. */
	hsk_persist.state = STATE_WRITE;
	hsk_persist_flashDptr = (ubyte code *)ADDR_DFLASH;
	hsk_persist_xdataDptr = &(hsk_persist.ident);
	hsk_persist_isr_nmiflash();
	RESET_RMAP();
}


