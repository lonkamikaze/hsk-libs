/** \file
 * HSK ICM7228 8-Digit LED Display Decoder Driver generator
 *
 * This file is a code generating facility, that offers preprocessor macros
 * that produce code for the Intersil ICM7228 display decoder.
 *
 * Generating code in this fashion avoids the hard coding of I/O registers
 * and bits and even allows the use of multiple ICM7228 ICs.
 *
 * @see Intersil ICM7228 Data Sheet: <a href="contrib/ICM7228.pdf">ICM7228.pdf</a>
 * @author kami
 */

#ifndef _HSK_ICM7228_H_
#define _HSK_ICM7228_H_

#include <string.h> /* memset() */

/**
 * Generate an ICM7228 driver instance.
 *
 * This creates functions to use a connect ICM7228 IC.
 * - void \<prefix\>_init(void)
 *	- Initialize the buffer and I/O register bits
 * - void \<prefix\>_refresh(void)
 *	- Commit buffered data to the 7 segment displays
 * - void \<prefix\>_writeString(char * str, ubyte pos, ubyte len)
 *	- Wrapper around hsk_icm7228_writeString()
 * - void \<prefix\>_writeDec(uword value, char power, ubyte pos, ubyte len)
 *	- Wrapper around hsk_icm7228_writeDec()
 * - void \<prefix\>_writeHex(uword value, char power, ubyte pos, ubyte len)
 *	- Wrapper around hsk_icm7228_writeHex()
 *
 * @param prefix
 *	A prefix for the names of generated functions
 * @param regData
 *	The register that is connected to the data input
 * @param regMode
 *	The register that is connected to the mode pin
 * @param bitMode
 *	The bit of the regMode register that is connected to the mode pin
 * @param regWrite
 *	The register that is connected to the write pin
 * @param bitWrite
 *	The bit of the regWrite register that is connected to the write pin
 */
#define ICM7228_FACTORY(prefix, regData, regMode, bitMode, regWrite, bitWrite) \
	\
	/**
	 * Buffer for display driver at  I/O port regData.
	 *
	 * @see ICM7228_FACTORY
	 */\
	ubyte xdata prefix##_buffer[8]; \
	\
	/**
	 * Set up buffer and ports for display driver at I/O port regData.
	 *
	 * @see ICM7228_FACTORY
	 */\
	void prefix##_init(void) { \
		memset(prefix##_buffer, 0, sizeof(prefix##_buffer)); \
	\
		regMode##_DIR |= 1 << bitMode; \
		regWrite##_DIR |= 1 << bitWrite; \
		regData##_DIR = -1; \
	} \
	\
	/**
	 * Reflesh displays at I/O port regData with the buffered data.
	 *
	 * @see ICM7228_FACTORY
	 */\
	void prefix##_refresh(void) { \
		ubyte i; \
	\
		/* Select write to control register. */ \
		regMode##_DATA |= 1 << bitMode; \
	\
		/* Write to control register. */ \
		regData##_DATA = 0xB0; /* Control Word */ \
		regWrite##_DATA &= ~(1 << bitWrite); \
		regWrite##_DATA |= 1 << bitWrite; \
	\
		/* Select write to display ram. */ \
		regMode##_DATA &= ~(1 << bitMode); \
	\
		for (i = 0; i < 8; i++) { \
			regData##_DATA = prefix##_buffer[i]; \
			regWrite##_DATA &= ~(1 << bitWrite); \
			regWrite##_DATA |= 1 << bitWrite; \
		} \
	} \
	\
	/**
	 * Write an ASCII encoded string into \ref prefix##_buffer.
	 *
	 * @param str
	 *	The buffer to read the ASCII string from
	 * @param pos
	 *	The position in the buffer to write the encoded string to
	 * @param len
	 *	The target length of the encoded string
	 * @see ICM7228_FACTORY
	 * @see hsk_icm7228_writeString
	 */\
	void prefix##_writeString(const char * const str, \
	                          const ubyte pos, const ubyte len) { \
		hsk_icm7228_writeString(prefix##_buffer, str, pos, len); \
	} \
	\
	/**
	 * Write a decimal number into \ref prefix##_buffer.
	 *
	 * @param value
	 *	The number to encode
	 * @param power
	 *	The 10 base power of the number to encode
	 * @param pos
	 *	The target position in the buffer
	 * @param len
	 *	The number of digits available to encode the number
	 * @see ICM7228_FACTORY
	 * @see hsk_icm7228_writeDec
	 */\
	void prefix##_writeDec(const uword value, const char power, \
	                       const ubyte pos, const ubyte len) {\
		hsk_icm7228_writeDec(prefix##_buffer, value, power, pos, len); \
	} \
	\
	/**
	 * Write a hexadecimal number into \ref prefix##_buffer.
	 *
	 * @param value
	 *	The number to encode
	 * @param power
	 *	The 16 base power of the number to encode
	 * @param pos
	 *	The target position in the buffer
	 * @param len
	 *	The number of digits available to encode the number
	 * @see ICM7228_FACTORY
	 * @see hsk_icm7228_writeHex
	 */\
	void prefix##_writeHex(const uword value, const char power, \
	                       const ubyte pos, const ubyte len) {\
		hsk_icm7228_writeHex(prefix##_buffer, value, power, pos, len); \
	} \
	\
	/**
	 * Illuminate a number of segments in \ref prefix##_buffer.
	 * @param segments
	 *	The number of segments to illuminate
	 * @param pos
	 *	The target position in the buffer
	 * @param len
	 *	The number of digits available to encode the number
	 * @see ICM7228_FACTORY
	 * @see hsk_icm7228_illuminate
	 */\
	void prefix##_illuminate(const ubyte segments, const ubyte pos, \
	                         const ubyte len) {\
		hsk_icm7228_illuminate(prefix##_buffer, segments, pos, len); \
	} \

/**
 * Convert an ASCII string to 7 segment encoding and store it in an xdata
 * buffer.
 *
 * This function is usually invoked through the \<prefix\>_writeString()
 * function created by ICM7228_FACTORY.
 *
 * The function will write into the buffer until it has been filled with len
 * characters or it encounters a 0 character reading from str.
 * If the character '.' is encountered it is merged with the previous
 * character, unless that character is a '.' itself. Thus a single dot does
 * not use additional buffer space. The 7 character string "foo ..." would
 * result in 6 encoded bytes. Thus the proper len value for that string would
 * be 6.
 *
 * @param buffer
 *	The target buffer for the encoded string
 * @param str
 *	The buffer to read the ASCII string from
 * @param pos
 *	The position in the buffer to write the encoded string to
 * @param len
 *	The target length of the encoded string
 */
void hsk_icm7228_writeString(ubyte xdata * const buffer, \
                             const char * str, ubyte pos, ubyte len);

/**
 * Write a 7 segment encoded, right aligned decimal number into an xdata
 * buffer.
 *
 * The power parameter controlls the placing of the '.' by 10 to the power.
 * E.g. value = 12, power = -1 and len = 3 would result in the encoding of
 * " 1.2". If power = 0, no dot is drawn. If the power is positive (typically
 * 1), the resulting string would be filled with '0' characters.
 * I.e. the previous example with power = 1 would result in an encoding of
 * "012".
 *
 * @param buffer
 *	The target buffer for the encoded string
 * @param value
 *	The number to encode
 * @param power
 *	The 10 base power of the number to encode
 * @param pos
 *	The target position in the buffer
 * @param len
 *	The number of digits available to encode the number
 */
void hsk_icm7228_writeDec(ubyte xdata * const buffer, uword value,
                          char power, const ubyte pos, ubyte len);

/**
 * Write a 7 segment encoded, right aligned hexadecimal number into an xdata
 * buffer.
 *
 * The power parameter controlls the placing of the '.' by 16 to the power.
 * E.g. value = 0x1A, power = -1 and len = 3 would result in the encoding of
 * " 1.A". If power = 0, no dot is drawn. If the power is positive (typically
 * 1), the resulting string would be filled with '0' characters.
 * I.e. the previous example with power = 1 would result in an encoding of
 * "01A".
 *
 * @param buffer
 *	The target buffer for the encoded string
 * @param value
 *	The number to encode
 * @param power
 *	The 16 base power of the number to encode
 * @param pos
 *	The target position in the buffer
 * @param len
 *	The number of digits available to encode the number
 */
void hsk_icm7228_writeHex(ubyte xdata * const buffer, uword value,
                          char power, const ubyte pos, ubyte len);

/**
 * Illumante the given number of segments.
 *
 * @param buffer
 *	The target buffer for the encoded string
 * @param segments
 *	The number of segments to illuminate
 * @param pos
 *	The target position in the buffer
 * @param len
 *	The number of digits available to encode the number
 */
void hsk_icm7228_illuminate(ubyte xdata * const buffer,
                            ubyte segments, ubyte pos, ubyte len);

#endif /* _HSK_ICM7228_H_ */

