/** \file
 * HSK Flash Facility implementation
 *
 * This file implements the flash management functions defined in hsk_flash.h.
 *
 * @author kami
 *
 * \section flash_registers Flash Registers
 *
 * All registers are in the mapped register are, i.e. RMAP=1 must be set to
 * access them.
 *
 * \subsection flash_timer Flash Timer
 *
 * Non-blocking flash reading/writing is controlled by a dedicated flash
 * timer.
 */

#include <Infineon/XC878.h>

#include "hsk_flash.h"

#include <string.h> /* memset() */

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
const ubyte code at ADDR_DFLASH hsk_flash_dflash[LEN_DFLASH];

/**
 * Bytewise access to the P-Flash area.
 */
const ubyte code at ADDR_PFLASH hsk_flash_pflash[LEN_PFLASH];

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
 * FCON/EECON Erase Bit.
 */
#define BIT_ERASE	1

/**
 * FCON/EECON Mass Erase Bit.
 */
#define BIT_MAS1	2

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
 * The state to use when nothing is to be done.
 */
#define STATE_IDLE	0

/**
 * The state to use to kick of a write/delete or idle.
 */
#define STATE_RUN	1

/**
 * The state to use when starting to write to the D-Flash.
 */
#define STATE_WRITE	10

/**
 * The state to use when mass erasing the D-Flash.
 */
#define STATE_RESET	30

/**
 * The state to use when erasing D-Flash pages.
 */
#define STATE_DELETE	50

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

	/**
	 * Set to request a state.
	 */
	ubyte stateRequest;
} xdata hsk_flash;

/**
 * A pointer to the flash target address.
 *
 * Not in a struct for easier inline assembler access.
 */
ubyte code * xdata hsk_flash_flashDptr;

/**
 * A pointer to the xdata src address.
 *
 * Not in a struct for easier inline assembler access.
 */
ubyte xdata * xdata hsk_flash_xdataDptr;

/**
 * Flash delete/write state machine.
 *
 * When hsk_flash.state == STATE_IDLE, it is safe to set
 * hsk_flash.state to STATE_WRITE or STATE_RESET.
 *
 * @private
 */
#pragma save
#pragma nooverlay
void hsk_flash_isr_nmiflash(void) {
	static uword written;

	SET_RMAP();

	switch(hsk_flash.state) {
	/**
	 * STATE_IDLE implements an idle state that never actually needs to
	 * be called.
	 */
	case STATE_IDLE:
		/* Turn the timer off, there's no need to ever actually call
		 * this state. */
		FCS &= ~(1 << BIT_FTEN);
		break;
	/**
	 * STATE_RUN implements the decision making process to kick off a
	 * delete or write. A write always has precedence, a delete is started
	 * if no writes are pending and hsk_flash.oldest points to a different
	 * D-Flash page than hsk_flash.latest.
	 */
	case STATE_RUN:
		if (hsk_flash.stateRequest == STATE_WRITE) {
			/* A write is requested. */
			hsk_flash.state = STATE_WRITE;
			hsk_flash.latest = (hsk_flash.latest + hsk_flash.size + 2) % hsk_flash.wrap;
			written = 0;
			goto state_write;
		} else if (hsk_flash.latest / BYTES_PAGE_DFLASH != hsk_flash.oldest / BYTES_PAGE_DFLASH) {
			/* The oldest data is on a different page, kick off
			 * a delete. */
			hsk_flash.state = STATE_DELETE;
			hsk_flash_flashDptr = hsk_flash_dflash + hsk_flash.oldest;
			/* Point the oldest pointer to the next page. 
			 * It doesn't neet to be accurate, just point
			 * somewhere into the next page. */
			hsk_flash.oldest = (hsk_flash.oldest + BYTES_PAGE_DFLASH) % sizeof(hsk_flash_dflash);
			goto state_delete;

		} else {
			hsk_flash.state = STATE_IDLE;
			FCS &= ~(1 << BIT_FTEN);
		}
		break;
	/**
	 * STATE_WRITE implements the procedure called "Program Operation"
	 * from the XC878 UM 1.1.
	 */
	case STATE_WRITE:
	state_write:
		/* Set program flash timer mode, 5µs for an overflow. */
		FTVAL &= ~(1 << BIT_MODE);

		/* 1.
		 * Set the bit FCON.PROG (P-Flash) or EECON.PROG (D-Flash) to
		 * signal the start of a programming cyle. */
		EECON |= 1 << BIT_PROG;

		/* 2.
		 * Execute a “MOVC” instruction with a dummy data to any
		 * address in the same wordline as the address to be accessed.
		 */
		hsk_flash_flashDptr = hsk_flash_dflash + hsk_flash.latest + written;
#ifdef SDCC
__asm
		; Backup used registers
		push	ar0
		; Load hsk_flash_flashDptr into r0, a
		mov	dptr,#_hsk_flash_flashDptr
		movx	a,@dptr
		mov	r0,a
		inc	dptr
		movx	a,@dptr
		; Follow hsk_flash_flashDptr
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
		hsk_flash.state++;
		break;
	case STATE_WRITE + 1:
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
	case STATE_WRITE + 2:
		hsk_flash.state++;
		break;
	case STATE_WRITE + 3:
		/* Turn the timer off after 10 µs. */
		FCS &= ~(1 << BIT_FTEN);

	state_write_loop:
		if (written == 0 || written == hsk_flash.size + 1) {
			hsk_flash_xdataDptr = &hsk_flash.ident;
		} else {
			hsk_flash_xdataDptr = hsk_flash.ptr + written - 1;
		}

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
		; Load hsk_flash_flashDptr into r0, r1
		mov	dptr,#_hsk_flash_flashDptr
		movx	a,@dptr
		mov	r0,a
		inc	dptr
		movx	a,@dptr
		mov	r1,a
		; Load hsk_flash_xdataDptr into r2, a
		mov	dptr,#_hsk_flash_xdataDptr
		movx	a,@dptr
		mov	r2,a
		inc	dptr
		movx	a,@dptr
		; Follow hsk_flash_xdataDptr
		mov	dpl,r2
		mov	dph,a
		movx	a,@dptr
		; Follow hsk_flash_flashDptr
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

		/* Point forward. */
		hsk_flash_flashDptr++;
		written++;

		/* 7.
		 * Delay for a minimum of 20 us but not longer than 40 us
		 * (Tprog). */
		FCS |= 1 << BIT_FTEN;

		/* fallthrough */
	case STATE_WRITE + 4:
	case STATE_WRITE + 5:
	case STATE_WRITE + 6:
		hsk_flash.state++;
		break;
	case STATE_WRITE + 7:
		/* Turn the timer off after 20µs. */
		FCS &= ~(1 << BIT_FTEN);

		/* 8.
		 * Clear the bits FCON/EECON.YE and FCS.FTEN respectively. */
		EECON &= ~(1 << BIT_YE);

		/* 9.
		 * Repeat steps 6 to 8 for any further programming of data to
		 * the same row. */
		if (written < hsk_flash.size + 2 && (hsk_flash.latest + written) % BYTES_WORDLINE_DFLASH != 0) {
			goto state_write_loop;
		}

		hsk_flash.state++;
		/* fallthrough */
	case STATE_WRITE + 8:
		/* 10.
		 * Clear the bit FCON/EECON.PROG. */
		EECON &= ~(1 << BIT_PROG);
		/* 11.
		 * Delay for a minimum of 5 us (Tnvh) */
		FCS |= 1 << BIT_FTEN;
		hsk_flash.state++;
		break;
	case STATE_WRITE + 9:
		/* 12.
		 * Clear the bit FCON/EECON.NVSTR. */
		EECON &= ~(1 << BIT_NVSTR);
		/* 13.
		 * Delay for a minimum of 1 us (Trcv). */
		/* Actually just wait for the completion of another 5µs. */
		if (written < hsk_flash.size + 2) {
			/* Still something left to write, start with a
			 * new wordline. */
			hsk_flash.state = STATE_WRITE;
		} else {
			hsk_flash.state = STATE_RUN;
		}
		break;
	/**
	 * STATE_DELETE implements the procedure called "Erase Operation"
	 * from the XC878 UM 1.1.
	 */
	case STATE_DELETE:
	state_delete:
		/* 1.
		 * Set the bit FCON/EECON.ERASE and clear the bit
		 * FCON/EECON.MAS1 to trigger the start of the page erase
		 * cycle. */
		EECON = (EECON & ~(1 << BIT_MAS1)) | (1 << BIT_ERASE);

		/* 2.
		 * Execute a “MOVC” instruction with a dummy data to any
		 * address in the page to be erased. */
#ifdef SDCC
__asm
		; Backup used registers
		push	ar0
		; Load hsk_flash_flashDptr into r0, a
		mov	dptr,#_hsk_flash_flashDptr
		movx	a,@dptr
		mov	r0,a
		inc	dptr
		movx	a,@dptr
		; Follow hsk_flash_flashDptr
		mov	dpl,r0
		mov	dph,a
		MOVCI
		; Restore used registers
		pop	ar0
__endasm;
#elif defined __C51__
#endif

		/* 3.
		 * Delay for a minimum of 5 μs (Tvns). */
		FCS |= 1 << BIT_FTEN;
		hsk_flash.state++;
		break;
	case STATE_DELETE + 1:
		/* Turn off the timer after 5µs. */
		FCS &= ~(1 << BIT_FTEN);

		/* 4.
		 * Set the bit FCON/EECON.NVSTR for charge pump to drive high
		 * voltage. */
		EECON |= 1 << BIT_NVSTR;

		/* 5.
		 * Delay for a minimum of 20 ms (Terase). */
		FTVAL |= 1 << BIT_MODE;
		FCS |= 1 << BIT_FTEN;
		hsk_flash.state++;
		break;
	case STATE_DELETE + 2:
		/* Turn off the timer after 20ms. */
		FCS &= ~(1 << BIT_FTEN);
		/* Return to 5µs timer overflow cycle. */
		FTVAL &= ~(1 << BIT_MODE);

		/* 6.
		 * Clear bit FCON/EECON.ERASE. */
		EECON &= ~(1 << BIT_ERASE);

		/* 7.
		 * Delay for a minimum of 5 us (Tnvh) */
		FCS |= 1 << BIT_FTEN;
		hsk_flash.state++;
		break;
	case STATE_DELETE + 3:
		/* 8.
		 * Clear the bit FCON/EECON.NVSTR. */
		EECON &= ~(1 << BIT_NVSTR);

		/* 9.
		 * Delay for a minimum of 1 us (Trcv). */
		/* Just wait for the next 5µs tick. */
		hsk_flash.state = STATE_RUN;
		break;
	/**
	 * STATE_RESET implements the procedure called "Mass Erase Operation"
	 * from the XC878 UM 1.1.
	 */
	case STATE_RESET:
		/* 1.
		 * Set the bits FCON/EECON.ERASE and FCON/EECON.MAS1 to
		 * trigger the start of the mass erase cycle. */
		EECON |= (1 << BIT_ERASE) | (1 << BIT_MAS1);

		/* 2.
		 * Execute a “MOVC” instruction with a dummy data to any
		 * address in the page to be erased. */
#ifdef SDCC
__asm
		; Make a dummy write to the dflash
		mov	dptr,#_hsk_flash_dflash
		MOVCI
__endasm;
#elif defined __C51__
#endif

		/* 3.
		 * Delay for a minimum of 5 μs (Tvns). */
		FCS |= 1 << BIT_FTEN;
		hsk_flash.state++;
		break;
	case STATE_RESET + 1:
		/* Turn off the timer after 5µs. */
		FCS &= ~(1 << BIT_FTEN);

		/* 4.
		 * Set the bit FCON/EECON.NVSTR for charge pump to drive high
		 * voltage. */
		EECON |= 1 << BIT_NVSTR;

		/* 5.
		 * Delay for a minimum of 200 ms (Tme). */
		/* Set erase flash timer mode, ~20ms for an overflow. */
		FTVAL |= 1 << BIT_MODE;
		FCS |= 1 << BIT_FTEN;
		/* fallthrough */
	case STATE_RESET + 2:
	case STATE_RESET + 3:
	case STATE_RESET + 4:
	case STATE_RESET + 5:
	case STATE_RESET + 6:
	case STATE_RESET + 7:
	case STATE_RESET + 8:
	case STATE_RESET + 9:
	case STATE_RESET + 10:
		hsk_flash.state++;
		break;
	case STATE_RESET + 11:
		/* Turn off the timer after 200ms. */
		FCS &= ~(1 << BIT_FTEN);

		/* 6.
		 * Clear bit FCON/EECON.ERASE. */
		EECON &= ~(1 << BIT_ERASE);

		/* 7.
		 * Delay for a minimum of 100 us (Tnvhl) */
		/* Set overflow value to 1 = ~170µs. */
		FTVAL = (FTVAL & ~(((1 << CNT_OFVAL) - 1) << BIT_OFVAL)) | (1 << BIT_OFVAL);
		FCS |= 1 << BIT_FTEN;
		hsk_flash.state++;
		break;
	case STATE_RESET + 12:
		/* Turn off the timer after 170µs. */
		FCS &= ~(1 << BIT_FTEN);

		/* 8.
		 * Clear the bit FCON/EECON.NVSTR and FCON/EECON.MAS1. */
		EECON &= ~(1 << BIT_NVSTR) & ~(1 << BIT_MAS1);

		/* 9.
		 * Delay for a minimum of 1 us (Trcv). */
		/* Actually restore the usual 5 µs timer cycle. */
		FTVAL = 120 << BIT_OFVAL;
		FCS |= 1 << BIT_FTEN;
		hsk_flash.state = STATE_RUN;
		break;
	}
}
#pragma restore

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
		ubyte idata version) {
	uword i;
	ubyte chksum;

	/* Setup the xdata area to persist. */
	hsk_flash.ptr = ptr;
	hsk_flash.size = size;
	hsk_flash.wrap = (sizeof(hsk_flash_dflash) / size) * size;
	hsk_flash.ident = (version & 0x3f) | 0x40;
	hsk_flash_flashDptr = 0;
	hsk_flash_xdataDptr = 0;

	/* Activate NMIFLASH interrupt to allow the use of the state
	 * machine. */
	hsk_isr14.NMIFLASH = &hsk_flash_isr_nmiflash;
	NMICON |= 1 << BIT_NMIFLASH;

	#define oldest		hsk_flash.oldest
	#define wrap		hsk_flash.wrap
	#define latest		hsk_flash.latest
	#define state		hsk_flash.state
	#define ident		hsk_flash.ident
	/* Find an unused block. */
	for (oldest = 0; oldest < wrap; oldest++) {
		/* There is data written in this block. */
		if (hsk_flash_dflash[oldest] != 0xff) {
			/* Jump forward to the next block. */
			oldest -= oldest % size;
			oldest += size;
		}
		/* End of block reached. */
		else if (oldest % size == size - 1) {
			/* Go back to the beginning of the block. */
			oldest -= oldest % size;
			break;
		}
	}

	/* No free blocks at all, mass delete obligatory! */
	if (oldest >= wrap) {
		/* The state machine will be busy for over 200ms once it
		 * starts. */
		state = STATE_RESET;
		hsk_flash_isr_nmiflash();
		oldest = 0;
		latest = 0;
		/* Check whether XRAM data is consistent. */
		if (((ubyte xdata *)ptr)[0] == ident \
				&& ((ubyte xdata *)ptr)[size - 1] == ident) {
			return 1;
		}
		memset(&((ubyte xdata *)ptr)[1], 0, size - 3);
		return 0;
	}

	/* Walk left, seek the newest data. */
	for (latest = (wrap + oldest - 1) % wrap;
		hsk_flash_dflash[latest] == 0xff && latest != oldest;
		latest = (wrap + latest - 1) % wrap);
	/* Align to the beginning of the block. */
	latest -= latest % size;

	/* Walk right, seek the oldest data. */
	for (oldest = (oldest + size) % wrap;
		hsk_flash_dflash[oldest] == 0xff && oldest != latest;
		oldest = (oldest + 1) % wrap);

	/* Kick off the ISR, in case there is something to delete. */
	state = STATE_RUN;
	hsk_flash_isr_nmiflash();

	/* Check whether XRAM data is consistent. */
	if (((ubyte xdata *)ptr)[0] == ident \
			&& ((ubyte xdata *)ptr)[size - 1] == ident) {
		return 1;
	}

	/*
	 * Restore data from the D-Flash.
	 */
	/* Validate the envelope. */
	if (hsk_flash_dflash[latest] != ident \
			|| hsk_flash_dflash[latest + size - 1] != ident) {
		return 0;
	}
	/* Validate the data checksum. It is the simple checksum used in the
	 * Intel HEX (.ihx) file format. */
	chksum = 0;
	for (i = latest; i < latest + size - 2; i++) {
		chksum += hsk_flash_dflash[latest];
	}
	if (hsk_flash_dflash[i] != -chksum) {
		return 0;
	}

	/* Copy data from the D-Flash to the xram. */
	for (i = 0; i < size; i++) {
		((ubyte xdata *)ptr)[i] = hsk_flash_flashDptr[latest + i];
	}
	return 2;
	#undef oldest
	#undef wrap
	#undef latest
	#undef state
	#undef stateRequest
	#undef ident
}

void hsk_flash_write(void) {
// Locking safe???
	hsk_flash.stateRequest = STATE_WRITE;
	if (hsk_flash.state == STATE_IDLE) {
		hsk_flash.state = STATE_RUN;
		hsk_flash_isr_nmiflash();
		hsk_flash.stateRequest = STATE_IDLE;
	}
}


