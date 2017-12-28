/* -*- mode: c; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 * bltChecksum.c --
 *
 * This module implements a checksum procedure for the BLT toolkit.
 *
 * Copyright 2015 George A. Howlett. All rights reserved.  
 *
 *   Redistribution and use in source and binary forms, with or without
 *   modification, are permitted provided that the following conditions are
 *   met:
 *
 *   1) Redistributions of source code must retain the above copyright
 *      notice, this list of conditions and the following disclaimer.
 *   2) Redistributions in binary form must reproduce the above copyright
 *      notice, this list of conditions and the following disclaimer in the
 *      documentation and/or other materials provided with the
 *      distribution.
 *   3) Neither the name of the authors nor the names of its contributors
 *      may be used to endorse or promote products derived from this
 *      software without specific prior written permission.
 *   4) Products derived from this software may not be called "BLT" nor may
 *      "BLT" appear in their names without specific prior written
 *      permission from the author.
 *
 *   THIS SOFTWARE IS PROVIDED ''AS IS'' AND ANY EXPRESS OR IMPLIED
 *   WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 *   MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 *   DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
 *   LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 *   CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 *   SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
 *   BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 *   WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
 *   OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
 *   IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

#define BUILD_BLT_TCL_PROCS 1
#include "bltInt.h"

#include <stdio.h>
#ifdef HAVE_STRING_H
#  include <string.h>
#endif /* HAVE_STRING_H */

#include "bltInitCmd.h"

/* open a file
 * calculate the CRC32 of the entire contents
 * return the CRC
 * if there is an error rdet the global variable Crcerror
 */
/*
 * ImportArgs --
 */
typedef struct {
    Tcl_Obj *fileObjPtr;                /* Name of file representing the
                                         * channel used as the input
                                         * source. */
    Tcl_Obj *dataObjPtr;                /* If non-NULL, data object to use as
                                         * input source. */
} ChecksumArgs;

static Blt_SwitchSpec checksumSwitches[] = 
{
    {BLT_SWITCH_OBJ,    "-data",      "string", (char *)NULL,
        Blt_Offset(ChecksumArgs, dataObjPtr), 0, 0, NULL},
    {BLT_SWITCH_OBJ,    "-file",      "fileName", (char *)NULL,
        Blt_Offset(ChecksumArgs, fileObjPtr), 0},
    {BLT_SWITCH_END}
};

/* -------------------------------------------------------------------------- */

/*-
 *  COPYRIGHT (C) 1986 Gary S. Brown.  You may use this program, or
 *  code or tables extracted from it, as desired without restriction.
 *
 *  First, the polynomial itself and its table of feedback terms.  The
 *  polynomial is
 *  X^32+X^26+X^23+X^22+X^16+X^12+X^11+X^10+X^8+X^7+X^5+X^4+X^2+X^1+X^0
 *
 *  Note that we take it "backwards" and put the highest-order term in
 *  the lowest-order bit.  The X^32 term is "implied"; the LSB is the
 *  X^31 term, etc.  The X^0 term (usually shown as "+1") results in
 *  the MSB being 1
 *
 *  Note that the usual hardware shift register implementation, which
 *  is what we're using (we're merely optimizing it by doing eight-bit
 *  chunks at a time) shifts bits into the lowest-order term.  In our
 *  implementation, that means shifting towards the right.  Why do we
 *  do it this way?  Because the calculated CRC must be transmitted in
 *  order from highest-order term to lowest-order term.  UARTs transmit
 *  characters in order from LSB to MSB.  By storing the CRC this way
 *  we hand it to the UART in the order low-byte to high-byte; the UART
 *  sends each low-bit to hight-bit; and the result is transmission bit
 *  by bit from highest- to lowest-order term without requiring any bit
 *  shuffling on our part.  Reception works similarly
 *
 *  The feedback terms table consists of 256, 32-bit entries.  Notes
 *
 *      The table can be generated at runtime if desired; code to do so
 *      is shown later.  It might not be obvious, but the feedback
 *      terms simply represent the results of eight shift/xor opera
 *      tions for all combinations of data and CRC register values
 *
 *      The values must be right-shifted by eight bits by the "updcrc
 *      logic; the shift must be unsigned (bring in zeroes).  On some
 *      hardware you could probably optimize the shift in assembler by
 *      using byte-swap instructions
 *      polynomial $edb88320
 *
 *
 * CRC32 code derived from work by Gary S. Brown.
 */
/* 
 * this is the CRC32 lookup table
 * thanks Gary S. Brown 
 * 64 lines of 4 values for a 256 dword table (1024 bytes)
 */
static unsigned long crc32[256] =
{                               /* CRC polynomial 0xedb88320 */
    0x00000000, 0x77073096, 0xEE0E612C, 0x990951BA, 0x076DC419, 0x706AF48F, 
    0xE963A535, 0x9E6495A3, 0x0EDB8832, 0x79DCB8A4, 0xE0D5E91E, 0x97D2D988,
    0x09B64C2B, 0x7EB17CBD, 0xE7B82D07, 0x90BF1D91, 0x1DB71064, 0x6AB020F2, 
    0xF3B97148, 0x84BE41DE, 0x1ADAD47D, 0x6DDDE4EB, 0xF4D4B551, 0x83D385C7,
    0x136C9856, 0x646BA8C0, 0xFD62F97A, 0x8A65C9EC, 0x14015C4F, 0x63066CD9, 
    0xFA0F3D63, 0x8D080DF5, 0x3B6E20C8, 0x4C69105E, 0xD56041E4, 0xA2677172,
    0x3C03E4D1, 0x4B04D447, 0xD20D85FD, 0xA50AB56B, 0x35B5A8FA, 0x42B2986C, 
    0xDBBBC9D6, 0xACBCF940, 0x32D86CE3, 0x45DF5C75, 0xDCD60DCF, 0xABD13D59,
    0x26D930AC, 0x51DE003A, 0xC8D75180, 0xBFD06116, 0x21B4F4B5, 0x56B3C423, 
    0xCFBA9599, 0xB8BDA50F, 0x2802B89E, 0x5F058808, 0xC60CD9B2, 0xB10BE924,
    0x2F6F7C87, 0x58684C11, 0xC1611DAB, 0xB6662D3D, 0x76DC4190, 0x01DB7106, 
    0x98D220BC, 0xEFD5102A, 0x71B18589, 0x06B6B51F, 0x9FBFE4A5, 0xE8B8D433,
    0x7807C9A2, 0x0F00F934, 0x9609A88E, 0xE10E9818, 0x7F6A0DBB, 0x086D3D2D, 
    0x91646C97, 0xE6635C01, 0x6B6B51F4, 0x1C6C6162, 0x856530D8, 0xF262004E,
    0x6C0695ED, 0x1B01A57B, 0x8208F4C1, 0xF50FC457, 0x65B0D9C6, 0x12B7E950, 
    0x8BBEB8EA, 0xFCB9887C, 0x62DD1DDF, 0x15DA2D49, 0x8CD37CF3, 0xFBD44C65,
    0x4DB26158, 0x3AB551CE, 0xA3BC0074, 0xD4BB30E2, 0x4ADFA541, 0x3DD895D7, 
    0xA4D1C46D, 0xD3D6F4FB, 0x4369E96A, 0x346ED9FC, 0xAD678846, 0xDA60B8D0, 
    0x44042D73, 0x33031DE5, 0xAA0A4C5F, 0xDD0D7CC9, 0x5005713C, 0x270241AA, 
    0xBE0B1010, 0xC90C2086, 0x5768B525, 0x206F85B3, 0xB966D409, 0xCE61E49F, 
    0x5EDEF90E, 0x29D9C998, 0xB0D09822, 0xC7D7A8B4, 0x59B33D17, 0x2EB40D81, 
    0xB7BD5C3B, 0xC0BA6CAD, 0xEDB88320, 0x9ABFB3B6, 0x03B6E20C, 0x74B1D29A,
    0xEAD54739, 0x9DD277AF, 0x04DB2615, 0x73DC1683, 0xE3630B12, 0x94643B84, 
    0x0D6D6A3E, 0x7A6A5AA8, 0xE40ECF0B, 0x9309FF9D, 0x0A00AE27, 0x7D079EB1,
    0xF00F9344, 0x8708A3D2, 0x1E01F268, 0x6906C2FE, 0xF762575D, 0x806567CB, 
    0x196C3671, 0x6E6B06E7, 0xFED41B76, 0x89D32BE0, 0x10DA7A5A, 0x67DD4ACC,
    0xF9B9DF6F, 0x8EBEEFF9, 0x17B7BE43, 0x60B08ED5, 0xD6D6A3E8, 0xA1D1937E, 
    0x38D8C2C4, 0x4FDFF252, 0xD1BB67F1, 0xA6BC5767, 0x3FB506DD, 0x48B2364B,
    0xD80D2BDA, 0xAF0A1B4C, 0x36034AF6, 0x41047A60, 0xDF60EFC3, 0xA867DF55, 
    0x316E8EEF, 0x4669BE79, 0xCB61B38C, 0xBC66831A, 0x256FD2A0, 0x5268E236,
    0xCC0C7795, 0xBB0B4703, 0x220216B9, 0x5505262F, 0xC5BA3BBE, 0xB2BD0B28, 
    0x2BB45A92, 0x5CB36A04, 0xC2D7FFA7, 0xB5D0CF31, 0x2CD99E8B, 0x5BDEAE1D,
    0x9B64C2B0, 0xEC63F226, 0x756AA39C, 0x026D930A, 0x9C0906A9, 0xEB0E363F, 
    0x72076785, 0x05005713, 0x95BF4A82, 0xE2B87A14, 0x7BB12BAE, 0x0CB61B38,
    0x92D28E9B, 0xE5D5BE0D, 0x7CDCEFB7, 0x0BDBDF21, 0x86D3D2D4, 0xF1D4E242, 
    0x68DDB3F8, 0x1FDA836E, 0x81BE16CD, 0xF6B9265B, 0x6FB077E1, 0x18B74777,
    0x88085AE6, 0xFF0F6A70, 0x66063BCA, 0x11010B5C, 0x8F659EFF, 0xF862AE69, 
    0x616BFFD3, 0x166CCF45, 0xA00AE278, 0xD70DD2EE, 0x4E048354, 0x3903B3C2,
    0xA7672661, 0xD06016F7, 0x4969474D, 0x3E6E77DB, 0xAED16A4A, 0xD9D65ADC, 
    0x40DF0B66, 0x37D83BF0, 0xA9BCAE53, 0xDEBB9EC5, 0x47B2CF7F, 0x30B5FFE9,
    0xBDBDF21C, 0xCABAC28A, 0x53B39330, 0x24B4A3A6, 0xBAD03605, 0xCDD70693, 
    0x54DE5729, 0x23D967BF, 0xB3667A2E, 0xC4614AB8, 0x5D681B02, 0x2A6F2B94,
    0xB40BBE37, 0xC30C8EA1, 0x5A05DF1B, 0x2D02EF8D
}; 

#define CRC32(c, b) (crc32[((int)(c) ^ (b)) & 0xff] ^ ((c) >> 8))

/*
  Copyright (C) 1999, 2000, 2002 Aladdin Enterprises.  All rights reserved.

  This software is provided 'as-is', without any express or implied
  warranty.  In no event will the authors be held liable for any damages
  arising from the use of this software.

  Permission is granted to anyone to use this software for any purpose,
  including commercial applications, and to alter it and redistribute it
  freely, subject to the following restrictions:

  1. The origin of this software must not be misrepresented; you must not
     claim that you wrote the original software. If you use this software
     in a product, an acknowledgment in the product documentation would be
     appreciated but is not required.
  2. Altered source versions must be plainly marked as such, and must not be
     misrepresented as being the original software.
  3. This notice may not be removed or altered from any source distribution.

  L. Peter Deutsch
  ghost@aladdin.com

 */
/* $Id: md5.c,v 1.6 2002/04/13 19:20:28 lpd Exp $ */
/*
  Independent implementation of MD5 (RFC 1321).

  This code implements the MD5 Algorithm defined in RFC 1321, whose
  text is available at
	http://www.ietf.org/rfc/rfc1321.txt
  The code is derived from the text of the RFC, including the test suite
  (section A.5) but excluding the rest of Appendix A.  It does not include
  any code or documentation that is identified in the RFC as being
  copyrighted.

  The original and principal author of md5.c is L. Peter Deutsch
  <ghost@aladdin.com>.  Other authors are noted in the change history
  that follows (in reverse chronological order):

  2002-04-13 lpd Clarified derivation from RFC 1321; now handles byte order
	either statically or dynamically; added missing #include <string.h>
	in library.
  2002-03-11 lpd Corrected argument list for main(), and added int return
	type, in test program and T value program.
  2002-02-21 lpd Added missing #include <stdio.h> in test program.
  2000-07-03 lpd Patched to eliminate warnings about "constant is
	unsigned in ANSI C, signed in traditional"; made test program
	self-checking.
  1999-11-04 lpd Edited comments slightly for automatic TOC extraction.
  1999-10-18 lpd Fixed typo in header comment (ansi2knr rather than md5).
  1999-05-03 lpd Original version.
 */

/*
 * This package supports both compile-time and run-time determination of CPU
 * byte order.  If ARCH_IS_BIG_ENDIAN is defined as 0, the code will be
 * compiled to run only on little-endian CPUs; if ARCH_IS_BIG_ENDIAN is
 * defined as non-zero, the code will be compiled to run only on big-endian
 * CPUs; if ARCH_IS_BIG_ENDIAN is not defined, the code will be compiled to
 * run on either big- or little-endian CPUs, but will run slightly less
 * efficiently on either one than if ARCH_IS_BIG_ENDIAN is defined.
 */

typedef unsigned char md5_byte_t; /* 8-bit byte */
typedef unsigned int md5_word_t; /* 32-bit word */

/* Define the state of the MD5 Algorithm. */
typedef struct md5_state_s {
    md5_word_t count[2];	/* message length in bits, lsw first */
    md5_word_t abcd[4];		/* digest buffer */
    md5_byte_t buf[64];		/* accumulate block */
} md5_state_t;

#include <string.h>

#undef BYTE_ORDER	/* 1 = big-endian, -1 = little-endian, 0 = unknown */
#ifdef ARCH_IS_BIG_ENDIAN
#  define BYTE_ORDER (ARCH_IS_BIG_ENDIAN ? 1 : -1)
#else
#  define BYTE_ORDER 0
#endif

#define T_MASK ((md5_word_t)~0)
#define T1 /* 0xd76aa478 */ (T_MASK ^ 0x28955b87)
#define T2 /* 0xe8c7b756 */ (T_MASK ^ 0x173848a9)
#define T3    0x242070db
#define T4 /* 0xc1bdceee */ (T_MASK ^ 0x3e423111)
#define T5 /* 0xf57c0faf */ (T_MASK ^ 0x0a83f050)
#define T6    0x4787c62a
#define T7 /* 0xa8304613 */ (T_MASK ^ 0x57cfb9ec)
#define T8 /* 0xfd469501 */ (T_MASK ^ 0x02b96afe)
#define T9    0x698098d8
#define T10 /* 0x8b44f7af */ (T_MASK ^ 0x74bb0850)
#define T11 /* 0xffff5bb1 */ (T_MASK ^ 0x0000a44e)
#define T12 /* 0x895cd7be */ (T_MASK ^ 0x76a32841)
#define T13    0x6b901122
#define T14 /* 0xfd987193 */ (T_MASK ^ 0x02678e6c)
#define T15 /* 0xa679438e */ (T_MASK ^ 0x5986bc71)
#define T16    0x49b40821
#define T17 /* 0xf61e2562 */ (T_MASK ^ 0x09e1da9d)
#define T18 /* 0xc040b340 */ (T_MASK ^ 0x3fbf4cbf)
#define T19    0x265e5a51
#define T20 /* 0xe9b6c7aa */ (T_MASK ^ 0x16493855)
#define T21 /* 0xd62f105d */ (T_MASK ^ 0x29d0efa2)
#define T22    0x02441453
#define T23 /* 0xd8a1e681 */ (T_MASK ^ 0x275e197e)
#define T24 /* 0xe7d3fbc8 */ (T_MASK ^ 0x182c0437)
#define T25    0x21e1cde6
#define T26 /* 0xc33707d6 */ (T_MASK ^ 0x3cc8f829)
#define T27 /* 0xf4d50d87 */ (T_MASK ^ 0x0b2af278)
#define T28    0x455a14ed
#define T29 /* 0xa9e3e905 */ (T_MASK ^ 0x561c16fa)
#define T30 /* 0xfcefa3f8 */ (T_MASK ^ 0x03105c07)
#define T31    0x676f02d9
#define T32 /* 0x8d2a4c8a */ (T_MASK ^ 0x72d5b375)
#define T33 /* 0xfffa3942 */ (T_MASK ^ 0x0005c6bd)
#define T34 /* 0x8771f681 */ (T_MASK ^ 0x788e097e)
#define T35    0x6d9d6122
#define T36 /* 0xfde5380c */ (T_MASK ^ 0x021ac7f3)
#define T37 /* 0xa4beea44 */ (T_MASK ^ 0x5b4115bb)
#define T38    0x4bdecfa9
#define T39 /* 0xf6bb4b60 */ (T_MASK ^ 0x0944b49f)
#define T40 /* 0xbebfbc70 */ (T_MASK ^ 0x4140438f)
#define T41    0x289b7ec6
#define T42 /* 0xeaa127fa */ (T_MASK ^ 0x155ed805)
#define T43 /* 0xd4ef3085 */ (T_MASK ^ 0x2b10cf7a)
#define T44    0x04881d05
#define T45 /* 0xd9d4d039 */ (T_MASK ^ 0x262b2fc6)
#define T46 /* 0xe6db99e5 */ (T_MASK ^ 0x1924661a)
#define T47    0x1fa27cf8
#define T48 /* 0xc4ac5665 */ (T_MASK ^ 0x3b53a99a)
#define T49 /* 0xf4292244 */ (T_MASK ^ 0x0bd6ddbb)
#define T50    0x432aff97
#define T51 /* 0xab9423a7 */ (T_MASK ^ 0x546bdc58)
#define T52 /* 0xfc93a039 */ (T_MASK ^ 0x036c5fc6)
#define T53    0x655b59c3
#define T54 /* 0x8f0ccc92 */ (T_MASK ^ 0x70f3336d)
#define T55 /* 0xffeff47d */ (T_MASK ^ 0x00100b82)
#define T56 /* 0x85845dd1 */ (T_MASK ^ 0x7a7ba22e)
#define T57    0x6fa87e4f
#define T58 /* 0xfe2ce6e0 */ (T_MASK ^ 0x01d3191f)
#define T59 /* 0xa3014314 */ (T_MASK ^ 0x5cfebceb)
#define T60    0x4e0811a1
#define T61 /* 0xf7537e82 */ (T_MASK ^ 0x08ac817d)
#define T62 /* 0xbd3af235 */ (T_MASK ^ 0x42c50dca)
#define T63    0x2ad7d2bb
#define T64 /* 0xeb86d391 */ (T_MASK ^ 0x14792c6e)


static void
md5_process(md5_state_t *pms, const md5_byte_t *data /*[64]*/)
{
    md5_word_t
	a = pms->abcd[0], b = pms->abcd[1],
	c = pms->abcd[2], d = pms->abcd[3];
    md5_word_t t;
#if BYTE_ORDER > 0
    /* Define storage only for big-endian CPUs. */
    md5_word_t X[16];
#else
    /* Define storage for little-endian or both types of CPUs. */
    md5_word_t xbuf[16];
    const md5_word_t *X;
#endif

    {
#if BYTE_ORDER == 0
	/*
	 * Determine dynamically whether this is a big-endian or
	 * little-endian machine, since we can use a more efficient
	 * algorithm on the latter.
	 */
	static const int w = 1;

	if (*((const md5_byte_t *)&w)) /* dynamic little-endian */
#endif
#if BYTE_ORDER <= 0		/* little-endian */
	{
	    /*
	     * On little-endian machines, we can process properly aligned
	     * data without copying it.
	     */
	    if (!((data - (const md5_byte_t *)0) & 3)) {
		/* data are properly aligned */
		X = (const md5_word_t *)data;
	    } else {
		/* not aligned */
		memcpy(xbuf, data, 64);
		X = xbuf;
	    }
	}
#endif
#if BYTE_ORDER == 0
	else			/* dynamic big-endian */
#endif
#if BYTE_ORDER >= 0		/* big-endian */
	{
	    /*
	     * On big-endian machines, we must arrange the bytes in the
	     * right order.
	     */
	    const md5_byte_t *xp = data;
	    int i;

#  if BYTE_ORDER == 0
	    X = xbuf;		/* (dynamic only) */
#  else
#    define xbuf X		/* (static only) */
#  endif
	    for (i = 0; i < 16; ++i, xp += 4)
		xbuf[i] = xp[0] + (xp[1] << 8) + (xp[2] << 16) + (xp[3] << 24);
	}
#endif
    }

#define ROTATE_LEFT(x, n) (((x) << (n)) | ((x) >> (32 - (n))))

    /* Round 1. */
    /* Let [abcd k s i] denote the operation
       a = b + ((a + F(b,c,d) + X[k] + T[i]) <<< s). */
#define F(x, y, z) (((x) & (y)) | (~(x) & (z)))
#define SET(a, b, c, d, k, s, Ti)\
  t = a + F(b,c,d) + X[k] + Ti;\
  a = ROTATE_LEFT(t, s) + b
    /* Do the following 16 operations. */
    SET(a, b, c, d,  0,  7,  T1);
    SET(d, a, b, c,  1, 12,  T2);
    SET(c, d, a, b,  2, 17,  T3);
    SET(b, c, d, a,  3, 22,  T4);
    SET(a, b, c, d,  4,  7,  T5);
    SET(d, a, b, c,  5, 12,  T6);
    SET(c, d, a, b,  6, 17,  T7);
    SET(b, c, d, a,  7, 22,  T8);
    SET(a, b, c, d,  8,  7,  T9);
    SET(d, a, b, c,  9, 12, T10);
    SET(c, d, a, b, 10, 17, T11);
    SET(b, c, d, a, 11, 22, T12);
    SET(a, b, c, d, 12,  7, T13);
    SET(d, a, b, c, 13, 12, T14);
    SET(c, d, a, b, 14, 17, T15);
    SET(b, c, d, a, 15, 22, T16);
#undef SET

     /* Round 2. */
     /* Let [abcd k s i] denote the operation
          a = b + ((a + G(b,c,d) + X[k] + T[i]) <<< s). */
#define G(x, y, z) (((x) & (z)) | ((y) & ~(z)))
#define SET(a, b, c, d, k, s, Ti)\
  t = a + G(b,c,d) + X[k] + Ti;\
  a = ROTATE_LEFT(t, s) + b
     /* Do the following 16 operations. */
    SET(a, b, c, d,  1,  5, T17);
    SET(d, a, b, c,  6,  9, T18);
    SET(c, d, a, b, 11, 14, T19);
    SET(b, c, d, a,  0, 20, T20);
    SET(a, b, c, d,  5,  5, T21);
    SET(d, a, b, c, 10,  9, T22);
    SET(c, d, a, b, 15, 14, T23);
    SET(b, c, d, a,  4, 20, T24);
    SET(a, b, c, d,  9,  5, T25);
    SET(d, a, b, c, 14,  9, T26);
    SET(c, d, a, b,  3, 14, T27);
    SET(b, c, d, a,  8, 20, T28);
    SET(a, b, c, d, 13,  5, T29);
    SET(d, a, b, c,  2,  9, T30);
    SET(c, d, a, b,  7, 14, T31);
    SET(b, c, d, a, 12, 20, T32);
#undef SET

     /* Round 3. */
     /* Let [abcd k s t] denote the operation
          a = b + ((a + H(b,c,d) + X[k] + T[i]) <<< s). */
#define H(x, y, z) ((x) ^ (y) ^ (z))
#define SET(a, b, c, d, k, s, Ti)\
  t = a + H(b,c,d) + X[k] + Ti;\
  a = ROTATE_LEFT(t, s) + b
     /* Do the following 16 operations. */
    SET(a, b, c, d,  5,  4, T33);
    SET(d, a, b, c,  8, 11, T34);
    SET(c, d, a, b, 11, 16, T35);
    SET(b, c, d, a, 14, 23, T36);
    SET(a, b, c, d,  1,  4, T37);
    SET(d, a, b, c,  4, 11, T38);
    SET(c, d, a, b,  7, 16, T39);
    SET(b, c, d, a, 10, 23, T40);
    SET(a, b, c, d, 13,  4, T41);
    SET(d, a, b, c,  0, 11, T42);
    SET(c, d, a, b,  3, 16, T43);
    SET(b, c, d, a,  6, 23, T44);
    SET(a, b, c, d,  9,  4, T45);
    SET(d, a, b, c, 12, 11, T46);
    SET(c, d, a, b, 15, 16, T47);
    SET(b, c, d, a,  2, 23, T48);
#undef SET

     /* Round 4. */
     /* Let [abcd k s t] denote the operation
          a = b + ((a + I(b,c,d) + X[k] + T[i]) <<< s). */
#define I(x, y, z) ((y) ^ ((x) | ~(z)))
#define SET(a, b, c, d, k, s, Ti)\
  t = a + I(b,c,d) + X[k] + Ti;\
  a = ROTATE_LEFT(t, s) + b
     /* Do the following 16 operations. */
    SET(a, b, c, d,  0,  6, T49);
    SET(d, a, b, c,  7, 10, T50);
    SET(c, d, a, b, 14, 15, T51);
    SET(b, c, d, a,  5, 21, T52);
    SET(a, b, c, d, 12,  6, T53);
    SET(d, a, b, c,  3, 10, T54);
    SET(c, d, a, b, 10, 15, T55);
    SET(b, c, d, a,  1, 21, T56);
    SET(a, b, c, d,  8,  6, T57);
    SET(d, a, b, c, 15, 10, T58);
    SET(c, d, a, b,  6, 15, T59);
    SET(b, c, d, a, 13, 21, T60);
    SET(a, b, c, d,  4,  6, T61);
    SET(d, a, b, c, 11, 10, T62);
    SET(c, d, a, b,  2, 15, T63);
    SET(b, c, d, a,  9, 21, T64);
#undef SET

     /* Then perform the following additions. (That is increment each
        of the four registers by the value it had before this block
        was started.) */
    pms->abcd[0] += a;
    pms->abcd[1] += b;
    pms->abcd[2] += c;
    pms->abcd[3] += d;
}

static void
md5_init(md5_state_t *pms)
{
    pms->count[0] = pms->count[1] = 0;
    pms->abcd[0] = 0x67452301;
    pms->abcd[1] = /*0xefcdab89*/ T_MASK ^ 0x10325476;
    pms->abcd[2] = /*0x98badcfe*/ T_MASK ^ 0x67452301;
    pms->abcd[3] = 0x10325476;
}

static void
md5_append(md5_state_t *pms, const md5_byte_t *data, int nbytes)
{
    const md5_byte_t *p = data;
    int left = nbytes;
    int offset = (pms->count[0] >> 3) & 63;
    md5_word_t nbits = (md5_word_t)(nbytes << 3);

    if (nbytes <= 0)
	return;

    /* Update the message length. */
    pms->count[1] += nbytes >> 29;
    pms->count[0] += nbits;
    if (pms->count[0] < nbits)
	pms->count[1]++;

    /* Process an initial partial block. */
    if (offset) {
	int copy = (offset + nbytes > 64 ? 64 - offset : nbytes);

	memcpy(pms->buf + offset, p, copy);
	if (offset + copy < 64)
	    return;
	p += copy;
	left -= copy;
	md5_process(pms, pms->buf);
    }

    /* Process full blocks. */
    for (; left >= 64; p += 64, left -= 64)
	md5_process(pms, p);

    /* Process a final partial block. */
    if (left)
	memcpy(pms->buf, p, left);
}

static void
md5_finish(md5_state_t *pms, md5_byte_t digest[16])
{
    static const md5_byte_t pad[64] = {
	0x80, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
    };
    md5_byte_t data[8];
    int i;

    /* Save the length before padding. */
    for (i = 0; i < 8; ++i)
	data[i] = (md5_byte_t)(pms->count[i >> 2] >> ((i & 3) << 3));
    /* Pad to 56 bytes mod 64. */
    md5_append(pms, pad, ((55 - (pms->count[0] >> 3)) & 63) + 1);
    /* Append the length. */
    md5_append(pms, data, 8);
    for (i = 0; i < 16; ++i)
	digest[i] = (md5_byte_t)(pms->abcd[i >> 2] >> ((i & 3) << 3));
}

static int 
GetMD5FromObj(Tcl_Obj *objPtr, char *out)
{
    char *buffer;
    int numBytes;
    md5_state_t state;
    md5_byte_t digest[16];
    int i;
    
    buffer = Tcl_GetStringFromObj(objPtr, &numBytes);
    md5_init(&state);
    md5_append(&state, (const md5_byte_t *)buffer, numBytes);
    md5_finish(&state, digest);
    for (i = 0; i < 16; i++) {
        sprintf(out + i * 2, "%02x", digest[i]);
    }
    return TCL_OK;
}

static int 
GetMD5FromFile(Tcl_Interp *interp, Tcl_Obj *objPtr, char *out)
{
    Tcl_Channel channel;
    int closeChannel;
    int done;
    int i;
    const char *fileName;
    md5_state_t state;
    md5_byte_t digest[16];
#define BUFFSIZE        8192

    fileName = Tcl_GetString(objPtr);
    closeChannel = TRUE;
    if ((fileName[0] == '@') && (fileName[1] != '\0')) {
        int mode;
        
        channel = Tcl_GetChannel(interp, fileName+1, &mode);
        if (channel == NULL) {
            return TCL_ERROR;
        }
        if ((mode & TCL_WRITABLE) == 0) {
            Tcl_AppendResult(interp, "channel \"", fileName, 
                "\" not opened for writing", (char *)NULL);
            return TCL_ERROR;
        }
        closeChannel = FALSE;
    } else {
        channel = Tcl_OpenFileChannel(interp, fileName, "r", 0);
        if (channel == NULL) {
            return TCL_ERROR;
        }
    }
    if (Tcl_SetChannelOption(interp, channel, "-translation", "binary") 
        != TCL_OK) {
        return TCL_ERROR;
    }
    md5_init(&state);
    done = FALSE;
    while (!done) {
        int numBytes;
#define BUFFSIZE        8192
        char buffer[BUFFSIZE];

        numBytes = Tcl_Read(channel, buffer, sizeof(char) * BUFFSIZE);
        if (numBytes < 0) {
            Tcl_AppendResult(interp, "\nread error: ", Tcl_PosixError(interp),
                             (char *)NULL);
            if (closeChannel) {
                Tcl_Close(interp, channel);
            }
            return TCL_ERROR;
        }
        md5_append(&state, (const md5_byte_t *)buffer, numBytes);
        done = Tcl_Eof(channel);
    }
    md5_finish(&state, digest);
    if (closeChannel) {
        Tcl_Close(interp, channel);
    }
    for (i = 0; i < 16; i++) {
        sprintf(out + i * 2, "%02x", digest[i]);
    }
    return TCL_OK;
}


static int 
GetCrc32FromObj(Tcl_Obj *objPtr, unsigned long *sumPtr)
{
    char *bp, *bend, *buffer;
    int numBytes;
    unsigned long sum;
    
    buffer = Tcl_GetStringFromObj(objPtr, &numBytes);
    sum = *sumPtr;
    for (bp = buffer, bend = bp + numBytes; bp < bend; bp++) {
        sum = CRC32(sum, *bp);
    }
    *sumPtr = sum;
    return TCL_OK;
}

static int 
GetCrc32FromFile(Tcl_Interp *interp, Tcl_Obj *objPtr, unsigned long *sumPtr)
{
    Tcl_Channel channel;
    int closeChannel;
    int done;
    unsigned long sum;
    const char *fileName;
#define BUFFSIZE        8192

    fileName = Tcl_GetString(objPtr);
    closeChannel = TRUE;
    if ((fileName[0] == '@') && (fileName[1] != '\0')) {
        int mode;
        
        channel = Tcl_GetChannel(interp, fileName+1, &mode);
        if (channel == NULL) {
            return TCL_ERROR;
        }
        if ((mode & TCL_WRITABLE) == 0) {
            Tcl_AppendResult(interp, "channel \"", fileName, 
                "\" not opened for writing", (char *)NULL);
            return TCL_ERROR;
        }
        closeChannel = FALSE;
    } else {
        channel = Tcl_OpenFileChannel(interp, fileName, "r", 0);
        if (channel == NULL) {
            return TCL_ERROR;
        }
    }
    if (Tcl_SetChannelOption(interp, channel, "-translation", "binary") 
        != TCL_OK) {
        return TCL_ERROR;
    }
    done = FALSE;
    sum = *sumPtr;
    while (!done) {
        char *bp, *bend;
        int numBytes;
#define BUFFSIZE        8192
        char buffer[BUFFSIZE];

        numBytes = Tcl_Read(channel, buffer, sizeof(char) * BUFFSIZE);
        if (numBytes < 0) {
            Tcl_AppendResult(interp, "\nread error: ", Tcl_PosixError(interp),
                             (char *)NULL);
            if (closeChannel) {
                Tcl_Close(interp, channel);
            }
            return TCL_ERROR;
        }
        done = Tcl_Eof(channel);
        for (bp = buffer, bend = bp + numBytes; bp < bend; bp++) {
            sum = CRC32(sum, *bp);
        }
    }
    if (closeChannel) {
        Tcl_Close(interp, channel);
    }
    *sumPtr = sum;
    return TCL_OK;
}


/*ARGSUSED*/
static int
Crc32Op(ClientData clientData, Tcl_Interp *interp, int objc,
         Tcl_Obj *const *objv)
{
    unsigned long crc;
    int result;
    ChecksumArgs args;
    
    memset(&args, 0, sizeof(args));
    if (Blt_ParseSwitches(interp, checksumSwitches, objc - 2 , objv + 2, 
        &args, BLT_SWITCH_DEFAULTS) < 0) {
        return TCL_ERROR;
    }
    crc = 0L;
    crc = crc ^ 0xffffffffUL;
    if ((args.dataObjPtr != NULL) && (args.fileObjPtr != NULL)) {
        Tcl_AppendResult(interp, "can't set both -file and -data switches.",
                         (char *)NULL);
        Blt_FreeSwitches(checksumSwitches, (char *)&args, 0);
        return TCL_ERROR;
    }
    if (args.dataObjPtr != NULL) {
        result = GetCrc32FromObj(args.dataObjPtr, &crc);
    } else if (args.fileObjPtr != NULL) {
        result = GetCrc32FromFile(interp, args.fileObjPtr, &crc);
    } else {
        Tcl_AppendResult(interp, "must specify either -file or -data switch.",
                (char *)NULL);
        result = TCL_ERROR;
    }
    if (result == TCL_OK) {
        char buf[200];
        crc = crc ^ 0xffffffffUL;

        Blt_FmtString(buf, 200, "%lx", crc);
        Tcl_SetStringObj(Tcl_GetObjResult(interp), buf, -1);
    }
    Blt_FreeSwitches(checksumSwitches, (char *)&args, 0);
    return result;
}

/*ARGSUSED*/
static int
MD5Op(ClientData clientData, Tcl_Interp *interp, int objc,
      Tcl_Obj *const *objv)
{
    int result;
    ChecksumArgs args;
    char out[33];
    
    memset(&args, 0, sizeof(args));
    if (Blt_ParseSwitches(interp, checksumSwitches, objc - 2 , objv + 2, 
        &args, BLT_SWITCH_DEFAULTS) < 0) {
        return TCL_ERROR;
    }
    if ((args.dataObjPtr != NULL) && (args.fileObjPtr != NULL)) {
        Tcl_AppendResult(interp, "can't set both -file and -data switches.",
                         (char *)NULL);
        Blt_FreeSwitches(checksumSwitches, (char *)&args, 0);
        return TCL_ERROR;
    }
    if (args.dataObjPtr != NULL) {
        result = GetMD5FromObj(args.dataObjPtr, out);
    } else if (args.fileObjPtr != NULL) {
        result = GetMD5FromFile(interp, args.fileObjPtr, out);
    } else {
        Tcl_AppendResult(interp, "must specify either -file or -data switch.",
                (char *)NULL);
        result = TCL_ERROR;
    }
    if (result == TCL_OK) {
        Tcl_SetStringObj(Tcl_GetObjResult(interp), out, -1);
    }
    Blt_FreeSwitches(checksumSwitches, (char *)&args, 0);
    return result;
}

/*
 *---------------------------------------------------------------------------
 *
 * ChecksumObjCmd --
 *
 *---------------------------------------------------------------------------
 */
static Blt_OpSpec checksumOps[] =
{
    {"crc32", 1, Crc32Op,  2, 0, "?switches?",},
    {"md5",   1, MD5Op,    2, 0, "?switches?",},
};

static int numChecksumOps = sizeof(checksumOps) / sizeof(Blt_OpSpec);

/*ARGSUSED*/
static int
ChecksumObjCmd(ClientData clientData, Tcl_Interp *interp, int objc,
           Tcl_Obj *const *objv)
{
    Tcl_ObjCmdProc *proc;

    proc = Blt_GetOpFromObj(interp, numChecksumOps, checksumOps,
                BLT_OP_ARG1, objc, objv, 0);
    if (proc == NULL) {
        return TCL_ERROR;
    }
    return (*proc) (clientData, interp, objc, objv);
}

int
Blt_ChecksumCmdInitProc(Tcl_Interp *interp)
{
    static Blt_CmdSpec cmdSpec = {"checksum", ChecksumObjCmd,};

    return Blt_InitCmd(interp, "::blt", &cmdSpec);
}
