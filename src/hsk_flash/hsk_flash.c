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
 * \section flash_timer Flash Timer
 *
 * Non-blocking flash reading/writing is controlled by a dedicated flash
 * timer. Timings, especially flash writing, are so critical that all the
 * flash delete/write procedures are implemented in a single state machine
 * within hsk_flash_isr_nmiflash(), which is called by a non-maskable
 * interrupt upon timer overflow.
 *
 * \section flash_dptr DPTR Byte Order
 *
 * Due to the \ref flash_byte_order differences between SDCC and C51, the
 * \ref DPL and \ref DPH macros are used to adjust DPTR assignments in
 * inline assembler.
 */

#include <Infineon/XC878.h>

#include "hsk_flash.h"

#include <string.h> /* memset() */

#include "../hsk_isr/hsk_isr.h"

/*
 * Compiler tweaks.
 */
#ifdef SDCC
/**
 * MOVC @(DPTR++),A instruction.
 */
#define MOVCI                       .db 0xA5

/**
 * DPTR low byte.
 *
 * @see \ref flash_dptr
 */
#define DPL                         dpl

/**
 * DPTR high byte.
 *
 * @see \ref flash_dptr
 */
#define DPH                         dph

/**
 * Create variable at a certain address, SDCC version.
 */
#define VAR_AT(type, name, addr)    type __at(addr) name

/**
 * Insert global variable address into inline assembler SDCC style.
 */
#define VAR_ASM(name)               _##name

#elif defined __C51__
/**
 * MOVC @(DPTR++),A instruction.
 */
#define MOVCI                       db 0xA5

/**
 * DPTR low byte.
 *
 * @see \ref flash_dptr
 */
#define DPL                         dph

/**
 * DPTR high byte.
 *
 * @see \ref flash_dptr
 */
#define DPH                         dpl

/**
 * Create variable at a certain address, C51 version.
 */
#define VAR_AT(type, name, addr)    type name _at_ addr

/**
 * Insert global variable address into inline assembler C51 style.
 */
#define VAR_ASM(name)               name

#else
#error Assembly code for the current compiler missing
#endif

/*
 * XC878-16FF code/flash layout.
 */
#ifdef XC878_16FF

/**
 * XC878-16FF code page that has the Boot ROM and XRAM mapped into it.
 */
#define PAGE_RAM                    2

/**
 * XC878-16FF code page that has the flash.
 */
#define PAGE_FLASH                  0

/**
 * XC878-16FF start address of the P-Flash.
 */
#define ADDR_PFLASH                 0x0000

/**
 * XC878-16FF length of the P-Flash.
 */
#define LEN_PFLASH                  (60u << 10)

/**
 * XC878-16FF the number of bytes in a P-Flash page.
 */
#define BYTES_PAGE_PFLASH           (1 << 9)

/**
 * XC878-16FF the number of bytes in a P-Flash wordline.
 */
#define BYTES_WORDLINE_PFLASH       (1 << 6)

/**
 * XC878-16FF start address of the D-Flash.
 */
#define ADDR_DFLASH                 0xF000

/**
 * XC878-16FF length of the D-Flash.
 */
#define LEN_DFLASH                  (4u << 10)

/**
 * XC878-16FF the number of bytes in a D-Flash page.
 */
#define BYTES_PAGE_DFLASH           (1 << 6)

/**
 * XC878-16FF ld() of the number of bytes in a D-Flash wordline.
 */
#define BYTES_WORDLINE_DFLASH       (1 << 5)

/**
 * XC878-16FF start address of the Boot ROM.
 */
#define ADDR_ROM                    0xC000

/**
 * XC878-16FF length of the Boot ROM.
 */
#define LEN_ROM                     (8u << 10)

/**
 * XC878-16FF start address of the XRAM.
 */
#define ADDR_XRAM                   0xF000

/**
 * XC878-16FF length of the XRAM.
 */
#define LEN_XRAM                    (3u << 10)

/*
 * XC878-13FF code/flash layout.
 */
#elif defined XC878_13FF

/**
 * XC878-13FF code page that has the Boot ROM and XRAM mapped into it.
 */
#define PAGE_RAM                    0

/**
 * XC878-13FF code page that has the flash.
 */
#define PAGE_FLASH                  0

/**
 * XC878-13FF start address of the P-Flash.
 */
#define ADDR_PFLASH                 0x0000

/**
 * XC878-13FF length of the P-Flash.
 */
#define LEN_PFLASH                  (48u << 10)

/**
 * XC878-13FF the number of bytes in a P-Flash page.
 */
#define BYTES_PAGE_PFLASH           (1 << 9)

/**
 * XC878-13FF the number of bytes in a P-Flash wordline.
 */
#define BYTES_WORDLINE_PFLASH       (1 << 6)

/**
 * XC878-13FF start address of the D-Flash.
 */
#define ADDR_DFLASH                 ((ubyte code *)0xE000)

/**
 * XC878-13FF length of the D-Flash.
 */
#define LEN_DFLASH                  (4u << 10)

/**
 * XC878-13FF the number of bytes in a D-Flash page.
 */
#define BYTES_PAGE_DFLASH           (1 << 6)

/**
 * XC878-13FF the number of bytes in a D-Flash wordline.
 */
#define BYTES_WORDLINE_DFLASH       (1 << 5)

/**
 * XC878-13FF start address of the Boot ROM.
 */
#define ADDR_ROM                    0xC000

/**
 * XC878-13FF length of the Boot ROM.
 */
#define LEN_ROM                     (8u << 10)

/**
 * XC878-13FF start address of the XRAM.
 */
#define ADDR_XRAM                   0xF000

/**
 * XC878-13FF length of the XRAM.
 */
#define LEN_XRAM                    (3u << 10)

/*
 * Missing code/flash layout.
 */
#else
#error Either XC878_16FF or XC878_13FF must be defined
#endif

/**
 * Bytewise access to the D-Flash area.
 */
VAR_AT(static const ubyte code, dflash[LEN_DFLASH], ADDR_DFLASH);

/**
 * P-Flash Control Register.
 */
SFR(FCON,    0xD1);

/**
 * D-Flash Control Register
 */
SFR(EECON,   0xD2);

/**
 * Flash Control and Status Register.
 */
SFR(FCS,     0xD3);

/**
 * Flash Error Address Register Low.
 */
SFR(FEAL,    0xD4);

/**
 * Flash Error Address Register High.
 */
SFR(FEAH,    0xD5);

/**
 * Flash Error Address Register Low and High (16 bits).
 */
SFR16(FEALH, 0xD4);

/**
 * Flash Timer Value Register.
 */
SFR(FTVAL,   0xD6);

/**
 * Flash Control and Status Register 1.
 */
SFR(FCS1,    0xDD);

/**
 * FCON/EECON Program Bit.
 */
#define BIT_PROG                    0

/**
 * FCON/EECON Erase Bit.
 */
#define BIT_ERASE                   1

/**
 * FCON/EECON Mass Erase Bit.
 */
#define BIT_MAS1                    2

/**
 * FCON/EECON Non-Volatile Store Bit.
 */
#define BIT_NVSTR                   3

/**
 * FCON/EECON Y-Address Enable Bit.
 */
#define BIT_YE                      5

/**
 * EECON D-Flash Busy Bit.
 */
#define BIT_EEBSY                   6

/**
 * FCS Flash Timer Enable Bit.
 */
#define BIT_FTEN                    5

/**
 * FCS1 D-Flash Program/Erase Abort bit.
 */
#define BIT_EEABORT                 0

/**
 * FTVAL Overflow Value bits.
 */
#define BIT_OFVAL                   0

/**
 * OFVAL bit count.
 */
#define CNT_OFVAL                   7

/**
 * FTVAL MODE bit.
 *
 * Controls the flash timer speed.
 *
 * | Mode    | Value | Effect
 * |---------|-------|---------------------------------------------
 * | Program | 0     | 1 count per \f$ CCLK \f$ (24MHz) clock cycle
 * | Erase   | 1     | 1 count per \f$ CCLK/2^{12} \f$ clock cycles
 */
#define BIT_MODE                    7

/**
 * NMICON Flash Timer NMI Enable bit.
 */
#define BIT_NMIFLASH                2

/**
 * The state to use when nothing is to be done.
 */
#define STATE_IDLE                  0

/**
 * The state to use to kick off a write.
 */
#define STATE_REQUEST               1

/**
 * The state that decides whether a delete or idle is appropriate.
 */
#define STATE_DETECT                10

/**
 * The state to use when starting to write to the D-Flash.
 */
#define STATE_WRITE                 20

/**
 * The state to use when erasing D-Flash pages.
 */
#define STATE_DELETE                40

/**
 * The state to use when mass erasing the D-Flash.
 */
#define STATE_RESET                 60

/**
 * The block indicated \ref hsk_flash.latest is available for writing.
 */
#define FREE_LATEST                 0

/**
 * The block behind the block indicated by \ref hsk_flash.latest is available
 * for writing.
 */
#define FREE_BEHIND                 1

/**
 * There is no block available for writing.
 */
#define FREE_NONE                   2

/** \var flash
 * Holds the persistence configuration.
 */
static volatile struct {
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
	 * This byte indicates where free space can be found in the D-Flash.
	 *
	 * Available values are:
	 * - \ref FREE_LATEST
	 * - \ref FREE_BEHIND
	 * - \ref FREE_NONE
	 */
	ubyte free;

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
} pdata flash;

/**
 * A pointer to the flash target address.
 *
 * Not in a struct for easier inline assembler access.
 */
static volatile ubyte code * xdata flashDptr;

/**
 * A pointer to the xdata src address.
 *
 * Not in a struct for easier inline assembler access.
 */
static volatile ubyte xdata * xdata xdataDptr;

/**
 * Flash delete/write state machine.
 *
 * Every named state is the root of a state machine that performs a specific
 * task.
 *
 * @see
 *	Section 4.4 <i>Flash Memory</i> - <i>Operating Modes</i> from the
 *	XC8787 reference manual:
 *	<a href="../contrib/XC878_um_v1_1.pdf">XC878_um_v1_1.pdf</a>
 * @private
 */
#pragma save
#ifdef SDCC
#pragma nooverlay
#endif
void hsk_flash_isr_nmiflash(void) using 2 {
	SET_RMAP();

	switch(flash.state) {
	/**
	 * - \ref STATE_IDLE is a sleeping state that turns off the state
	 *   machine. This state is a dead end, the state machine has to be
	 *   reactivated externally to resume operation.
	 */
	case STATE_IDLE:
		/* Turn off the timer. */
		FCS &= ~(1 << BIT_FTEN);
		break;
	/**
	 * - \ref STATE_REQUEST implements the procedure called
	 *   "Abort Operation" from the XC878 UM 1.1.
	 *
	 *   After completing the abort \ref STATE_WRITE is entered.
	 */
	case STATE_REQUEST:
		/* SW checks EEBSY */
		if (((EECON >> BIT_EEBSY) & 1) == 0) {
			/* SW clears NVSTR */
			EECON &= ~(1 << BIT_NVSTR);
			goto state_write;
		}

		/* SW clears PROG/ERASE */
		EECON &= ~(1 << BIT_PROG) & ~(1 << BIT_MAS1) & ~(1 << BIT_ERASE);

		/* SW waits 5 us (min) */
		FCS |= 1 << BIT_FTEN;
		flash.state++;
		break;
	case STATE_REQUEST + 1:
		/* SW sets EEABORT, HW clears all control signals */
		FCS1 |= 1 << BIT_EEABORT;

		/* SW waits 1 us (min) */
		/* Just wait for the completion of the 5µs. */
		flash.state++;
		break;
	case STATE_REQUEST + 2:
		/* SW clears NVSTR, HW clears EEABORT */
		EECON &= ~(1 << BIT_NVSTR);

		/* Continue with User interrupt routine */
		flash.state++;
		break;
	case STATE_REQUEST + 3:
		/* Turn the timer off after 5 µs. */
		FCS &= ~(1 << BIT_FTEN);

		/* SW sets EEABORT */
		FCS1 |= 1 << BIT_EEABORT;

		/* SW clears NVSTR if any, HW clears EEABORT */
		EECON &= ~(1 << BIT_NVSTR);

		goto state_write;
		break;
	/**
	 * - \ref STATE_DETECT checks whether there is a page that should be
	 *   deleted.
	 *
	 *   It either goes into \ref STATE_DELETE or \ref STATE_IDLE.
	 */
	case STATE_DETECT:
		/* Turn off the timer. */
		FCS &= ~(1 << BIT_FTEN);

		switch (flash.free) {
		case FREE_NONE:
			flashDptr = dflash + flash.oldest;
			goto state_delete;
			break;
		case FREE_LATEST:
			if (flash.oldest >= flash.size) {
				flashDptr = dflash + flash.oldest;
				goto state_delete;
			}
			break;
		case FREE_BEHIND:
			if (flash.oldest + BYTES_PAGE_DFLASH <= flash.latest \
					|| flash.latest + flash.size <= flash.oldest) {
				flashDptr = dflash + flash.oldest;
				goto state_delete;
			}
			break;
		}
		flash.state = STATE_IDLE;
		break;
	/**
	 * - \ref STATE_WRITE implements the procedure called
	 *   "Program Operation" from the XC878 UM 1.1.
	 *
	 *   The next address to write is expected in \ref flashDptr.
	 *   The next address to read from XRAM is expected in
	 *   \ref xdataDptr.
	 */
	case STATE_WRITE:
		/* Set program flash timer mode, 5µs for an overflow. */
		FTVAL &= ~(1 << BIT_MODE);
	state_write:
		flash.state = STATE_WRITE;

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
#elif defined __C51__
#pragma asm
#endif
		; Backup used registers
		push    psw
		push    acc
		push    ar0
		; Load flashDptr into r0, a
		mov     dptr,#VAR_ASM(flashDptr)
		movx    a,@dptr
		mov     r0,a
		inc     dptr
		movx    a,@dptr
		; Follow flashDptr
		mov     DPL,r0
		mov     DPH,a
		MOVCI
		; Restore used registers
		pop     ar0
		pop     acc
		pop     psw
#ifdef SDCC
__endasm;
#elif defined __C51__
#pragma endasm
#endif

		/* 3.
		 * Delay for a minimum of 5 us (Tvns). */
		FCS |= 1 << BIT_FTEN;
		flash.state++;
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
		flash.state++;
		break;
	case STATE_WRITE + 3:
		/* Turn the timer off after 10 µs. */
		FCS &= ~(1 << BIT_FTEN);

	state_write_loop:
		flash.state = STATE_WRITE + 3;

		/* 6.
		 * Execute a “MOVC” instruction to the flash address to be
		 * accessed. FCON/EECON.YE and FCS.FTEN is set by hardware
		 * at next clock cycle (YE hold time of 40 ns is needed). */
#ifdef SDCC
__asm
#elif defined __C51__
#pragma asm
#endif
		; Backup used registers
		push    psw
		push    acc
		push    ar2
		push    ar1
		push    ar0
		; Load flashDptr into r0, r1
		mov     dptr,#VAR_ASM(flashDptr)
		movx    a,@dptr
		mov     r0,a
		inc     dptr
		movx    a,@dptr
		mov     r1,a
		; Load xdataDptr into r2, a
		mov     dptr,#VAR_ASM(xdataDptr)
		movx    a,@dptr
		mov     r2,a
		inc     dptr
		movx    a,@dptr
		; Follow xdataDptr
		mov     DPL,r2
		mov     DPH,a
		movx    a,@dptr
		; Follow flashDptr
		mov     DPL,r0
		mov     DPH,r1
		MOVCI
		; Restore used registers
		pop     ar0
		pop     ar1
		pop     ar2
		pop     acc
		pop     psw
#ifdef SDCC
__endasm;
#elif defined __C51__
#pragma endasm
#endif

		/* 7.
		 * Delay for a minimum of 20 us but not longer than 40 us
		 * (Tprog). */
		/* Note FCS.FTEN was set by hardware. */

		/* Point forward. */
		flashDptr++;
		xdataDptr++;

		/* fallthrough */
	case STATE_WRITE + 4:
	case STATE_WRITE + 5:
	case STATE_WRITE + 6:
		flash.state++;
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
		if (xdataDptr < &flash.ptr[flash.size] && (flashDptr - dflash) % BYTES_WORDLINE_DFLASH != 0) {
			goto state_write_loop;
		}

		/* 10.
		 * Clear the bit FCON/EECON.PROG. */
		EECON &= ~(1 << BIT_PROG);

		/* 11.
		 * Delay for a minimum of 5 us (Tnvh) */
		FCS |= 1 << BIT_FTEN;
		flash.state++;
		break;
	case STATE_WRITE + 8:
		/* 12.
		 * Clear the bit FCON/EECON.NVSTR. */
		EECON &= ~(1 << BIT_NVSTR);
		/* 13.
		 * Delay for a minimum of 1 us (Trcv). */
		/* Actually just wait for the completion of another 5µs. */
		if (xdataDptr < &flash.ptr[flash.size]) {
			/* Still something left to write, start with a
			 * new wordline. */
			flash.state = STATE_WRITE;
		} else {
			/* Write completed. */
			flash.state = STATE_DETECT;
			flash.free = FREE_BEHIND;
		}
		break;
	/**
	 * - \ref STATE_DELETE implements the procedure called
	 *   "Erase Operation" from the XC878 UM 1.1.
	 */
	case STATE_DELETE:
	state_delete:
		/* Set program flash timer mode, 5µs for an overflow. */
		FTVAL &= ~(1 << BIT_MODE);
		flash.state = STATE_DELETE;
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
#elif defined __C51__
#pragma asm
#endif
		; Backup used registers
		push    psw
		push    acc
		push    ar0
		; Load flashDptr into r0, a
		mov     dptr,#VAR_ASM(flashDptr)
		movx    a,@dptr
		mov     r0,a
		inc     dptr
		movx    a,@dptr
		; Follow flashDptr
		mov     DPL,r0
		mov     DPH,a
		MOVCI
		; Restore used registers
		pop     ar0
		pop     acc
		pop     psw
#ifdef SDCC
__endasm;
#elif defined __C51__
#pragma endasm
#endif

		/* 3.
		 * Delay for a minimum of 5 μs (Tvns). */
		FCS |= 1 << BIT_FTEN;
		flash.state++;
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
		flash.state++;
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
		flash.state++;
		break;
	case STATE_DELETE + 3:
		/* 8.
		 * Clear the bit FCON/EECON.NVSTR. */
		EECON &= ~(1 << BIT_NVSTR);

		/* 9.
		 * Delay for a minimum of 1 us (Trcv). */
		/* Just wait for the next 5µs tick. */

		/* Move the oldest pointer to the start of the next page. */
		flash.oldest += BYTES_PAGE_DFLASH - (flash.oldest % BYTES_PAGE_DFLASH);
		if (flash.oldest >= flash.wrap) {
			flash.oldest = 0;
		}
		/* FREE_NONE implicates that flash.latest is 0 and that
		 * the page just deleted intersects with the first write block. */
		if (flash.free == FREE_NONE && flash.oldest >= flash.size) {
			flash.free = FREE_LATEST;
		}
		flash.state = STATE_DETECT;
		break;
	/**
	 * - \ref STATE_RESET implements the procedure called
	 *   "Mass Erase Operation" from the XC878 UM 1.1.
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
#elif defined __C51__
#pragma asm
#endif
		; Backup used registers
		push    psw
		; Make a dummy write to the dflash
		mov     dptr,#VAR_ASM(dflash)
		MOVCI
		; Restore used registers
		pop     psw
#ifdef SDCC
__endasm;
#elif defined __C51__
#pragma endasm
#endif

		/* 3.
		 * Delay for a minimum of 5 μs (Tvns). */
		FCS |= 1 << BIT_FTEN;
		flash.state++;
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
		flash.state++;
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
		flash.state++;
		break;
	case STATE_RESET + 12:
		/* Turn off the timer after 170µs. */
		FCS &= ~(1 << BIT_FTEN);

		/* 8.
		 * Clear the bit FCON/EECON.NVSTR and FCON/EECON.MAS1. */
		EECON &= ~(1 << BIT_NVSTR) & ~(1 << BIT_MAS1);

		/* 9.
		 * Delay for a minimum of 1 us (Trcv). */
		/* Actually restore the usual 5 µs timer cycle and go
		 * to sleep. */
		FTVAL = 120 << BIT_OFVAL;
		flash.state = STATE_IDLE;
		break;
	}
}
#pragma restore

ubyte hsk_flash_init(void xdata * const ptr, const uword __xdata size,
		const ubyte __xdata version) {
	uword i;
	ubyte chksum;

	/* Setup the xdata area to persist. */
	flash.ptr = ptr;
	flash.size = size;
	flash.wrap = (sizeof(dflash) / size) * size;
	flash.ident = (version & 0x3f) | 0x40;
	flashDptr = 0;
	xdataDptr = 0;

	/* Set up the NMIFLASH ISR. */
	hsk_isr14.NMIFLASH = &hsk_flash_isr_nmiflash;

	#define ptr       flash.ptr
	#define size      flash.size
	#define oldest    flash.oldest
	#define wrap      flash.wrap
	#define latest    flash.latest
	#define free      flash.free
	#define state     flash.state
	#define ident     flash.ident
	/* Find an unused block. */
	free = FREE_NONE;
	for (oldest = 0; oldest < wrap; oldest++) {
		/* There is data written in this block. */
		if (dflash[oldest] != 0xff) {
			/* Jump forward to the next block. */
			oldest -= oldest % size;
			oldest += size;
		}
		/* End of block reached. */
		else if (oldest % size == size - 1) {
			/* Go back to the beginning of the block. */
			oldest -= oldest % size;
			free = FREE_BEHIND;
			break;
		}
	}

	/* No free blocks at all, mass delete obligatory! */
	if (oldest >= wrap) {
		oldest = 0;
		latest = 0;
		/* Kick off the ISR, start to delete. */
		state = STATE_DETECT;
		NMICON |= 1 << BIT_NMIFLASH;
		SET_RMAP();
		FCS |= 1 << BIT_FTEN;
		RESET_RMAP();

		/* Check whether XRAM data is consistent. */
		if (ptr[0] == ident) {
			return 1;
		}

		/* Setup data envelope. */
		memset(ptr, 0, size);
		ptr[0] = ident;
		return 0;
	}

	/* Walk left, seek the newest data. */
	for (latest = (wrap + oldest - 1) % wrap;
		dflash[latest] == 0xff && latest != oldest;
		latest = (wrap + latest - 1) % wrap);
	/* Align to the beginning of the block. */
	latest -= latest % size;
	/* No data at all in the flash, i.e. the flash is entirely free.
	 * Note that up to this point oldest holds the position of a free
	 * block. */
	if (latest == oldest) {
		/* The latest pointer points to a free block. */
		free = FREE_LATEST;
	}

	/* Walk right, seek the oldest data. */
	for (oldest = (oldest + size) % wrap;
		dflash[oldest] == 0xff && oldest != latest;
		oldest = (oldest + 1) % wrap);
	/* Align to the beginning of the page. */
	oldest = oldest - (oldest % BYTES_PAGE_DFLASH);

	/* Kick off the ISR, in case there is something to delete. */
	state = STATE_DETECT;
	NMICON |= 1 << BIT_NMIFLASH;
	SET_RMAP();
	FCS |= 1 << BIT_FTEN;
	RESET_RMAP();

	/* Check whether XRAM data is consistent. */
	if (ptr[0] == ident) {
		return 1;
	}

	/*
	 * Restore data from the D-Flash.
	 */
	/* Validate the prefix. */
	if (dflash[latest] != ident) {
		/* Setup data envelope. */
		memset(ptr, 0, size);
		ptr[0] = ident;
		return 0;
	}
	/* Validate the data checksum. It is the simple checksum used in the
	 * Intel HEX (.ihx) file format. */
	chksum = 0;
	for (i = 0; i < size - 1; i++) {
		chksum += dflash[latest + i];
	}
	chksum = -chksum;
	if (dflash[latest + i] != chksum) {
		/* Setup data envelope. */
		memset(ptr, 0, size);
		ptr[0] = ident;
		return 0;
	}

	/* Copy data from the D-Flash to the xram. */
	for (i = 0; i < size; i++) {
		ptr[i] = dflash[latest + i];
	}
	return 2;
	#undef ptr
	#undef size
	#undef oldest
	#undef wrap
	#undef latest
	#undef free
	#undef state
	#undef ident
}

bool hsk_flash_write(void) {
	uword i;
	ubyte chksum;

	/* Turn off the state machine, because that's the only way to
	 * safely access the hsk_flash struct. */
	NMICON &= ~(1 << BIT_NMIFLASH);

	/*
	 * Check for sufficient free space in the D-Flash.
	 */
	switch (flash.free) {
	case FREE_NONE:
		/* There is no space to write. */
		NMICON |= 1 << BIT_NMIFLASH;
		return 0;
		break;
	case FREE_BEHIND:
		/* Check the space left to write, this prevents a write flood
		 * in so far that the flash can only be written as long as
		 * the state machine manages to keep up with deleting old
		 * data. Still writing at such a rate is not advisable. */
		if ((flash.wrap + flash.oldest - flash.latest - flash.size) % flash.wrap < flash.size) {
			/* Insufficient. */
			NMICON |= 1 << BIT_NMIFLASH;
			return 0;
		}
		break;
	}

	/*
	 * Prepare to abort current operation.
	 */
	/* Return the flash timer to the default state (5µs cycle, off). */
	SET_RMAP();
	FCS &= ~(1 << BIT_FTEN);
	FTVAL = 120 << BIT_OFVAL;
	RESET_RMAP();
	/* Now that the interrupt generating clock is turned off, it is safe
	 * to reactivate the interrupt. */
	NMICON |= 1 << BIT_NMIFLASH;

	/*
	 * Update pointers for writing.
	 */
	if (flash.free == FREE_BEHIND) {
		flash.latest = (flash.latest + flash.size) % flash.wrap;
	}
	xdataDptr = flash.ptr;
	flashDptr = dflash + flash.latest;

	/*
	 * Create chksum.
	 */
	chksum = 0;
	for (i = 0; i < flash.size - 1; i++) {
		chksum += flash.ptr[i];
	}
	chksum = -chksum;
	flash.ptr[flash.size - 1] = chksum;

	/*
	 * Resume operation from the appropriate state.
	 */
	if (flash.state == STATE_IDLE) {
		flash.state = STATE_WRITE;
	} else {
		flash.state = STATE_REQUEST;
	}
	SET_RMAP();
	FTVAL = 120 << BIT_OFVAL;
	FCS |= 1 << BIT_FTEN;
	RESET_RMAP();
	return 1;
}

