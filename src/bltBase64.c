/* -*- mode: c; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 * bltBase64.c --
 *
 * This module implements base64 processing procedures for the BLT toolkit.
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
#ifdef HAVE_CTYPE_H
  #include <ctype.h>
#endif /* HAVE_CTYPE_H */
#ifdef HAVE_STRING_H
  #include <string.h>
#endif /* HAVE_STRING_H */
#include <bltAlloc.h>
#include "bltDBuffer.h"
#include "bltInitCmd.h"

#define BRACKETS        (1<<0)          /* Add <,> brackets to output. */
#define LOWER_CASE      (1<<1)          /* Convert A-F to a-f. */
#define COMPRESS_SPACES (1<<2)          /* Use 'y' to represent all spaces. */
#define COMPRESS_ZEROS  (1<<3)          /* Use 'z' to represent all zeros.  */
#define IGNORE_BAD_CHARS (1<<4)         /* Ignore invalid encoding
                                         * characters. */

static Blt_SwitchSpec ascii85DecodingSwitches[] = 
{
    {BLT_SWITCH_OBJ,    "-data",      "string", (char *)NULL,
        Blt_Offset(DecodingSwitches, dataObjPtr), 0, 0, NULL},
    {BLT_SWITCH_OBJ,    "-file",      "fileName", (char *)NULL,
        Blt_Offset(DecodingSwitches, fileObjPtr), 0},
    {BLT_SWITCH_BITMASK, "-ignorebadchars", "", (char *)NULL,
        Blt_Offset(DecodingSwitches, flags), 0, IGNORE_BAD_CHARS},
    {BLT_SWITCH_END}
};

static Blt_SwitchSpec hexadecimalDecodingSwitches[] = 
{
    {BLT_SWITCH_OBJ,    "-data",      "string", (char *)NULL,
        Blt_Offset(DecodingSwitches, dataObjPtr), 0, 0, NULL},
    {BLT_SWITCH_OBJ,    "-file",      "fileName", (char *)NULL,
        Blt_Offset(DecodingSwitches, fileObjPtr), 0},
    {BLT_SWITCH_BITMASK, "-ignorebadchars", "", (char *)NULL,
        Blt_Offset(DecodingSwitches, flags), 0, IGNORE_BAD_CHARS},
    {BLT_SWITCH_END}
};

static Blt_SwitchSpec base64DecodingSwitches[] = 
{
    {BLT_SWITCH_OBJ,     "-data",      "string", (char *)NULL,
        Blt_Offset(DecodingSwitches, dataObjPtr), 0, 0, NULL},
    {BLT_SWITCH_OBJ,     "-file",      "fileName", (char *)NULL,
        Blt_Offset(DecodingSwitches, fileObjPtr), 0},
    {BLT_SWITCH_BITMASK, "-ignorebadchars", "", (char *)NULL,
        Blt_Offset(DecodingSwitches, flags), 0, IGNORE_BAD_CHARS},
    {BLT_SWITCH_END}
};

static Blt_SwitchSpec hexadecimalEncodingSwitches[] = 
{
    {BLT_SWITCH_OBJ,      "-data",      "string", (char *)NULL,
        Blt_Offset(EncodingSwitches, dataObjPtr), 0, 0, NULL},
    {BLT_SWITCH_OBJ,      "-file",      "fileName", (char *)NULL,
        Blt_Offset(EncodingSwitches, fileObjPtr), 0},
    {BLT_SWITCH_BITMASK,  "-lowercase", "", (char *)NULL,
        Blt_Offset(EncodingSwitches, flags), 0, LOWER_CASE},
    {BLT_SWITCH_STRING,   "-pad",      "fileName", (char *)NULL,
        Blt_Offset(EncodingSwitches, pad), 0},
    {BLT_SWITCH_STRING,    "-wrapchars",      "string", (char *)NULL,
        Blt_Offset(EncodingSwitches, wrap), 0, 0, NULL},
    {BLT_SWITCH_INT_NNEG, "-wraplength",      "number", (char *)NULL,
        Blt_Offset(EncodingSwitches, wrapLength), 0, 0, NULL},
    {BLT_SWITCH_END}
};

static Blt_SwitchSpec ascii85EncodingSwitches[] = 
{
    {BLT_SWITCH_BITMASK,   "-brackets", "", (char *)NULL,
       Blt_Offset(EncodingSwitches, flags), 0, BRACKETS},
    {BLT_SWITCH_OBJ,       "-data",      "varName", (char *)NULL,
        Blt_Offset(EncodingSwitches, dataObjPtr), 0, 0, NULL},
    {BLT_SWITCH_OBJ,       "-file",      "fileName", (char *)NULL,
        Blt_Offset(EncodingSwitches, fileObjPtr), 0},
    {BLT_SWITCH_BITMASK,   "-foldspaces", "", (char *)NULL,
        Blt_Offset(EncodingSwitches, flags), 0, COMPRESS_SPACES},
    {BLT_SWITCH_BITMASK,   "-foldzeros", "", (char *)NULL,
        Blt_Offset(EncodingSwitches, flags), 0, COMPRESS_ZEROS},
    {BLT_SWITCH_STRING,    "-pad",      "string", (char *)NULL,
        Blt_Offset(EncodingSwitches, pad), 0},
    {BLT_SWITCH_STRING,    "-wrapchars",      "string", (char *)NULL,
        Blt_Offset(EncodingSwitches, wrap), 0, 0, NULL},
    {BLT_SWITCH_INT_NNEG, "-wraplength",      "number", (char *)NULL,
        Blt_Offset(EncodingSwitches, wrapLength), 0, 0, NULL},
    {BLT_SWITCH_END}
};

static Blt_SwitchSpec base64EncodingSwitches[] = 
{
    {BLT_SWITCH_OBJ,       "-data",      "string", (char *)NULL,
        Blt_Offset(EncodingSwitches, dataObjPtr), 0, 0, NULL},
    {BLT_SWITCH_OBJ,       "-file",      "fileName", (char *)NULL,
        Blt_Offset(EncodingSwitches, fileObjPtr), 0},
    {BLT_SWITCH_STRING,    "-pad",      "fileName", (char *)NULL,
        Blt_Offset(EncodingSwitches, pad), 0},
    {BLT_SWITCH_STRING,    "-wrapchars",      "string", (char *)NULL,
        Blt_Offset(EncodingSwitches, wrap), 0, 0, NULL},
    {BLT_SWITCH_INT_NNEG, "-wraplength",      "number", (char *)NULL,
        Blt_Offset(EncodingSwitches, wrapLength), 0, 0, NULL},
    {BLT_SWITCH_END}
};

typedef size_t (FormatEncodeSizeProc)(size_t numBytes,
        EncodingSwitches *switchesPtr);
typedef size_t (FormatDecodeSizeProc)(size_t numChars,
        DecodingSwitches *switchesPtr);
typedef int (FormatDecodeProc)(Tcl_Interp *interp, const char *src,
        size_t numChars, unsigned char *dest, size_t *numBytesPtr,
        DecodingSwitches *switchesPtr);
typedef int (FormatEncodeProc)(const unsigned char *src, size_t numBytes,
        char *dest, size_t *numCharsPtr, EncodingSwitches *switchesPtr);

typedef struct {
    const char *name;
    Blt_SwitchSpec *encodeSpecs;
    Blt_SwitchSpec *decodeSpecs;
    FormatEncodeProc *encodeProc;
    FormatDecodeProc *decodeProc;
    FormatEncodeSizeProc *encodeSizeProc;
    FormatDecodeSizeProc *decodeSizeProc;
    int wrapLength;
}  FormatClass;

static FormatClass base64Class = {
    "base64",
    base64EncodingSwitches,
    base64DecodingSwitches,
    Blt_EncodeBase64,
    Blt_DecodeBase64,
    Blt_Base64EncodeBufferSize,
    Blt_Base64DecodeBufferSize,
    76
};

static FormatClass hexadecimalClass = {
    "hexadecimal",
    hexadecimalEncodingSwitches,
    hexadecimalDecodingSwitches,
    Blt_EncodeHexadecimal,
    Blt_DecodeHexadecimal,
    Blt_HexadecimalEncodeBufferSize,
    Blt_HexadecimalDecodeBufferSize,
    60,
};

static FormatClass ascii85Class = {
    "ascii85",
    ascii85EncodingSwitches,
    ascii85DecodingSwitches,
    Blt_EncodeAscii85,
    Blt_DecodeAscii85,
    Blt_Ascii85EncodeBufferSize,
    Blt_Ascii85DecodeBufferSize,
    60
};

#define NA      0xFF


/*
 * Table for decoding base64.  
 *
 * Note that NUL and '=' also return 0.  This is so we can blindly decode 4
 * octets without requiring special handing of left-over bytes (i.e. when the
 * encoded buffer did not end on a 3-byte boundary).
 */

const static char encode16[] = "0123456789ABCDEF";
const static char encode16lower[] = "0123456789abcdef";

const static char encode64[] = {
    "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/"
};

const static unsigned char decode64[256] = {
    0 /* '\0' */, 
    NA, NA, NA, NA, NA, NA, NA, NA, NA, NA, NA, NA, NA, NA, NA, NA, 
    NA, NA, NA, NA, NA, NA, NA, NA, NA, NA, NA, NA, NA, NA, NA, NA, 
    NA, NA, NA, NA, NA, NA, NA, NA, NA, NA, 
    62  /* + */, 
    NA, NA, NA, 
    63  /* / */,
    52  /* 0 */, 53  /* 1 */, 54  /* 2 */, 55  /* 3 */, 56  /* 4 */, 
    57  /* 5 */, 58  /* 6 */, 59  /* 7 */, 60  /* 8 */, 61  /* 9 */, 
    NA, NA, NA, 
    0 /* = */, 
    NA, NA, NA, 
    0   /* A */, 1   /* B */, 2   /* C */, 3   /* D */, 4   /* E */, 
    5   /* F */, 6   /* G */, 7   /* H */, 8   /* I */, 9   /* J */, 
    10  /* K */, 11  /* L */, 12  /* M */, 13  /* N */, 14  /* O */, 
    15  /* P */, 16  /* Q */, 17  /* R */, 18  /* S */, 19  /* T */, 
    20  /* U */, 21  /* V */, 22  /* W */, 23  /* X */, 24  /* Y */, 
    25  /* Z */, 
    NA, NA, NA, NA, NA, NA, 
    26  /* a */, 27  /* b */, 28  /* c */, 29  /* d */, 30  /* e */, 
    31  /* f */, 32  /* g */, 33  /* h */, 34  /* i */, 35  /* j */, 
    36  /* k */, 37  /* l */, 38  /* m */, 39  /* n */, 40  /* o */, 
    41  /* p */, 42  /* q */, 43  /* r */, 44  /* s */, 45  /* t */, 
    46  /* u */, 47  /* v */, 48  /* w */, 49  /* x */, 50  /* y */, 
    51  /* z */, 
    NA, NA, NA, NA, NA,

    NA, NA, NA, NA, NA, NA, NA, NA, NA, NA, NA, NA, NA, NA, NA, NA, 
    NA, NA, NA, NA, NA, NA, NA, NA, NA, NA, NA, NA, NA, NA, NA, NA, 
    NA, NA, NA, NA, NA, NA, NA, NA, NA, NA, NA, NA, NA, NA, NA, NA, 
    NA, NA, NA, NA, NA, NA, NA, NA, NA, NA, NA, NA, NA, NA, NA, NA, 
    NA, NA, NA, NA, NA, NA, NA, NA, NA, NA, NA, NA, NA, NA, NA, NA, 
    NA, NA, NA, NA, NA, NA, NA, NA, NA, NA, NA, NA, NA, NA, NA, NA, 
    NA, NA, NA, NA, NA, NA, NA, NA, NA, NA, NA, NA, NA, NA, NA, NA, 
    NA, NA, NA, NA, NA, NA, NA, NA, NA, NA, NA, NA, NA, NA, NA, NA
};

const static char encode85[]= {
    "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz!"
    "#$%&()*+-;<=>?@^_`{|}~"
};

const static unsigned char decode85[256] = {
    NA /* '\0' */, 
    NA, NA, NA, NA, NA, NA, NA, NA, NA, NA, NA, NA, NA, NA, NA, NA, 
    NA, NA, NA, NA, NA, NA, NA, NA, NA, NA, NA, NA, NA, NA, NA, NA, 
    62  /* ! */,
    NA,
    63  /* # */, 64  /* $ */, 65  /* % */, 66  /* & */,
    NA,
    67  /* ( */, 68  /* ) */, 69  /* * */, 70  /* + */,
    NA,
    71  /* - */,
    NA, NA, 
     0  /* 0 */,  1  /* 1 */,  2  /* 2 */,  3  /* 3 */,  4  /* 4 */, 
     5  /* 5 */,  6  /* 6 */,  7  /* 7 */,  8  /* 8 */,  9  /* 9 */, 
    NA, 
    72  /* ; */, 73  /* < */, 74  /* = */, 75  /* > */,  76 /* ? */,
    77  /* @ */,
    10  /* A */, 11  /* B */, 12  /* C */, 13  /* D */, 14  /* E */, 
    15  /* F */, 16  /* G */, 17  /* H */, 18  /* I */, 19  /* J */, 
    20  /* K */, 21  /* L */, 22  /* M */, 23  /* N */, 24  /* O */, 
    25  /* P */, 26  /* Q */, 27  /* R */, 28  /* S */, 29  /* T */, 
    30  /* U */, 31  /* V */, 32  /* W */, 33  /* X */, 34  /* Y */, 
    35  /* Z */, 
    NA, NA, NA,
    78  /* ^ */, 79  /* _ */, 80  /* ` */,
    36  /* a */, 37  /* b */, 38  /* c */, 39  /* d */, 40  /* e */, 
    41  /* f */, 42  /* g */, 43  /* h */, 44  /* i */, 45  /* j */, 
    46  /* k */, 47  /* l */, 48  /* m */, 49  /* n */, 50  /* o */, 
    51  /* p */, 52  /* q */, 53  /* r */, 54  /* s */, 55  /* t */, 
    56  /* u */, 57  /* v */, 58  /* w */, 59  /* x */, 60  /* y */, 
    61  /* z */, 
    81  /* { */, 82  /* | */, 83  /* } */, 84  /* ~ */,
    NA,

    NA, NA, NA, NA, NA, NA, NA, NA, NA, NA, NA, NA, NA, NA, NA, NA,
    NA, NA, NA, NA, NA, NA, NA, NA, NA, NA, NA, NA, NA, NA, NA, NA, 
    NA, NA, NA, NA, NA, NA, NA, NA, NA, NA, NA, NA, NA, NA, NA, NA, 
    NA, NA, NA, NA, NA, NA, NA, NA, NA, NA, NA, NA, NA, NA, NA, NA, 
    NA, NA, NA, NA, NA, NA, NA, NA, NA, NA, NA, NA, NA, NA, NA, NA, 
    NA, NA, NA, NA, NA, NA, NA, NA, NA, NA, NA, NA, NA, NA, NA, NA, 
    NA, NA, NA, NA, NA, NA, NA, NA, NA, NA, NA, NA, NA, NA, NA, NA, 
    NA, NA, NA, NA, NA, NA, NA, NA, NA, NA, NA, NA, NA, NA, NA, NA, 
};

int
Blt_IsBase64(const char *string, size_t numBytes)
{
    unsigned const char *sp, *send;

    for (sp = (unsigned const char *)string, send = sp + numBytes; sp < send; 
         sp++) {
        unsigned int byte;

        byte = (unsigned int)*sp;
        if (isspace(byte)) {
            continue;
        }
        if ((byte < '+') || (byte > 'z') || (decode64[byte] == NA)) {
            return FALSE;
        }
    }
    return TRUE;
}

static INLINE unsigned char
NextBase64EncodedChar(const char **bp, const char *lastPtr,
                      DecodingSwitches *switchesPtr) 
{
    char c;
    const char *p;
    
    p = *bp;
    if (switchesPtr->flags & IGNORE_BAD_CHARS) {
        /* Skip whitespace and invalid characters. Let's see if being
         * fault-tolerant is better than erroring out here.*/
        while (((isspace(*p)) || (decode64[(int)*p] == NA)) && (p < lastPtr)) {
            p++;
        }
    } else {
        while ((isspace(*p)) && (p < lastPtr)) {
            p++;
        }
    }
    c = (p < lastPtr) ? *p : 0;
    if ((c != '\0') && (c != '=')) {
        p++;
    }
    *bp = p;
    return c;                           /* Valid symbol */
}

static INLINE unsigned char
GetNextEncodedChar(const char **chPtrPtr, const char *endPtr,
                   DecodingSwitches *switchesPtr, const unsigned char *table) 
{
    char c;
    const char *p;

    for (p = *chPtrPtr; p < endPtr; p++) {
        if (isspace(*p)) {
            continue;                   /* Skip whitespace.*/
        }
        if ((table[(int)*p] == NA) && (switchesPtr->flags & IGNORE_BAD_CHARS)) {
            continue;                   /* Skip invalid characters. */
        }
        break;
    }
    if (p < endPtr) {
        c = *p;
        p++;
    } else {
        c = 0;
    }
    *chPtrPtr = p;
    return c;
}

static void
AddEncodedChar(char **dpp, int c, EncodingSwitches *switchesPtr)
{
    char *dp;

    dp = *dpp;
    /* Check if we're at the being of a new line and need to add
     * padding.  */
    if ((switchesPtr->fill == 0) && (switchesPtr->pad != NULL)) {
        const char *p;
                
        for (p = switchesPtr->pad; *p != '\0'; p++) {
            *dp++ = *p;
        }
    }
    /* Add the character to the buffer */
    *dp++ = c;
    switchesPtr->fill++;

    /* Check if we need to wrap the line.  */
    if ((switchesPtr->wrapLength > 0) &&
        (switchesPtr->fill >= switchesPtr->wrapLength)) {
        if (switchesPtr->wrap != NULL) {
            const char *p;
            
            for (p = switchesPtr->wrap; *p != '\0'; p++) {
                *dp++ = *p;
            }
        } else {
            *dp++ = '\n';
        }
        switchesPtr->fill = 0;
    }
    *dpp = dp;
}

static void
InitAscii85DecodeTable(unsigned char *table)
{
    int i;
    
    memset(table, NA, sizeof(unsigned char) * 256);
    for (i = 0; i < 85; i++) {
        table[i + '!'] = i;
    }
}

size_t 
Blt_HexadecimalEncodeBufferSize(size_t numBytes, EncodingSwitches *switchesPtr)
{
    size_t numChars, numLines;

    /*
     * Compute worst-case length of buffer needed for encoding.  That is 8
     * characters per 4 bytes. 
     */
    numChars = numBytes * 2;           /* Two characters per byte */
    numLines = 0;
    if (switchesPtr->wrapLength > 0) {  /* # of newlines required. */
        numLines = (numChars + (switchesPtr->wrapLength - 1)) /
            switchesPtr->wrapLength;    
    } 
    if (switchesPtr->wrap != NULL) {
        numChars += numLines * strlen(switchesPtr->wrap);
    } else {
        numChars += numLines;
    }
    if (switchesPtr->pad != NULL) {
        numChars += numLines * strlen(switchesPtr->pad);
    }
    numChars++;                         /* NUL byte */
    return numChars;
}

size_t 
Blt_HexadecimalDecodeBufferSize(size_t numChars,
                                    DecodingSwitches *switchesPtr)
{
    size_t numBytes;

    numBytes =  (numChars + 1) / 2;     /* Two characters per byte */
    return numBytes;
}

int
Blt_DecodeHexadecimal(Tcl_Interp *interp, const char *src,  size_t numChars,
                      unsigned char *dest, size_t *numBytesPtr,
                      DecodingSwitches *switchesPtr)
{
    unsigned char *dp;
    const char *p, *pend;
    static unsigned char decode16[256];
    static int initialized = FALSE;
    
    if (!initialized) {
        Blt_InitHexTable(decode16);
        initialized = TRUE;
    }
    /* 
     * Assuming that the string contains no padding or whitespace, allocate a
     * buffer with a worst-case length.
     */
    dp = dest;
    for (p = src, pend = p + numChars; p < pend; /*empty*/) {
        unsigned int byte[2];
        unsigned char c;
        
        c = GetNextEncodedChar(&p, pend, switchesPtr, decode16);
        if (c == '\0') {
            break;                      /* EOF */
        }
        byte[0] = decode16[c];
        if (byte[0] == NA) {
            Tcl_AppendResult(interp, "invalid character found at ",
                             Blt_Itoa(p - src), (char *)NULL);
            return TCL_ERROR;
        }
        c = GetNextEncodedChar(&p, pend, switchesPtr, decode16);
        if (c == '\0') {               /* Unexpected EOF */
            Tcl_AppendResult(interp, "odd number of hexadecimal digits.",
                         (char *)NULL);
            return TCL_ERROR;
        }
        byte[1] = decode16[c];
        if (byte[1] == NA) {                  
            Tcl_AppendResult(interp, "invalid character found at ",
                             Blt_Itoa(p - src), (char *)NULL);
            return TCL_ERROR;
        }
        dp[0] = (byte[0] << 4) | byte[1];
        dp++;
    }
    *numBytesPtr = dp - dest;
    return TCL_OK;
}

int
Blt_EncodeHexadecimal(const unsigned char *src, size_t numBytes, char *dest,
                      size_t *numCharsPtr, EncodingSwitches *switchesPtr) 
{
    char *dp;
    const unsigned char *sp, *send;
    const char *table;
    
    if (switchesPtr->flags & LOWER_CASE) {
        table = encode16lower;
    } else {
        table = encode16;
    }
    dp = dest;
    for (sp = src, send = sp + numBytes; sp < send; sp++) {
        AddEncodedChar(&dp, table[*sp >> 4], switchesPtr);
        AddEncodedChar(&dp, table[*sp & 0x0F], switchesPtr);
    }
    *numCharsPtr = (dp - dest);
    *dp++ = '\0';
    return TCL_OK;
}

size_t 
Blt_Base64EncodeBufferSize(size_t numBytes, EncodingSwitches *switchesPtr)
{
    size_t numChars, numLines;

    /* Compute worst-case length. */
    numChars = (((numBytes + 1) * 4) + 2) / 3; 
    numLines = 0;
    if (switchesPtr->wrapLength > 0) {  /* # of newlines required. */
        numLines = (numChars + (switchesPtr->wrapLength - 1)) /
            switchesPtr->wrapLength;    
    } 
    if (switchesPtr->wrap != NULL) {
        numChars += numLines * strlen(switchesPtr->wrap);
    } else {
        numChars += numLines;
    }
    if (switchesPtr->pad != NULL) {
        numChars += numLines * strlen(switchesPtr->pad);
    }
    numChars++;                         /* NUL byte */
    return numChars;
}

size_t 
Blt_Base64DecodeBufferSize(size_t numChars, DecodingSwitches *switchesPtr)
{
    size_t numBytes;
    /* 
     * Assuming that the string contains no padding or whitespace, allocate a
     * buffer with a worst-case length.
     */
    numBytes = ((numChars + 1) * 3)  / 4; 
    return numBytes;
}

int
Blt_DecodeBase64(Tcl_Interp *interp, const char *src, size_t numChars,
                 unsigned char *dest, size_t *numBytesPtr,
                 DecodingSwitches *switchesPtr)
{
    unsigned char *dp;
    const char *p, *pend;

    dp = dest;
    for (p = src, pend = p + numChars; p < pend; /*empty*/) {
        unsigned char byte[4];
        unsigned int u1, u2, u3;
        const char *start;
        int i;
        
        start = p;
        byte[0] = NextBase64EncodedChar(&p, pend, switchesPtr);
        byte[1] = NextBase64EncodedChar(&p, pend, switchesPtr);
        byte[2] = NextBase64EncodedChar(&p, pend, switchesPtr);
        byte[3] = NextBase64EncodedChar(&p, pend, switchesPtr);

        if (byte[3] == '\0') {
            if (byte[0] != '\0') {
                Tcl_AppendResult(interp, "premature end of base64 data",
                        (char *)NULL);
                return TCL_ERROR;
            }
            break;
        }

        for (i = 0; i < 4; i++) {
            if (decode64[byte[i]] == NA) {
                Tcl_AppendResult(interp, "invalid character found at ",
                        Blt_Itoa(start - src + i + 1), (char *)NULL);
                return TCL_ERROR;
            }
        }
        /*
         * in:     a      b      c     d
         *       ------.......-------......
         *      |54321054|32105432|10543210|
         * out:    u1       u2       u3
         */

        /* a = [543210xx] | [xxxxxx54] >> 4 */
        u1 = (decode64[byte[0]] << 2) | ((decode64[byte[1]] & 0x30) >> 4);
        /* b = [3210xxxx] | [xxxx5432]  */
        u2 = (((decode64[byte[1]] & 0x0F) << 4) |
              ((decode64[byte[2]] & 0x3C) >> 2));
        /* c = [10xxxxxx] | [xx543210]  */
        u3 = ((decode64[byte[2]] & 0x03) << 6) | decode64[byte[3]];

        if (byte[3] == '=') {
            if ((byte[0] == '=') || (byte[1] == '=')) {
                break;                  /* This should not be possible. */
            }
            if (byte[2] == '=') {
                *dp++ = (unsigned char)u1;
            } else {
                *dp++ = (unsigned char)u1;
                *dp++ = (unsigned char)u2;
            }
            break;
        }
        dp[0] = (unsigned char)u1;
        dp[1] = (unsigned char)u2;
        dp[2] = (unsigned char)u3;
        dp += 3;
    }
    *numBytesPtr = dp - dest;
    return TCL_OK;
}

int
Blt_EncodeBase64(const unsigned char *src, size_t numBytes, char *dest,
                 size_t *numCharsPtr, EncodingSwitches *switchesPtr) 
{
    char *dp;
    int remainder;
    const unsigned char *sp, *send;

    remainder = numBytes % 3;
    send = src + (numBytes - remainder);
    dp = dest;
    for (sp = src; sp < send; sp += 3) {
        unsigned char byte[4];
        int i;
        
        /*
         * in:        0        1        2
         *       |76543210|76543210|76543210|
         *        ------.......-------......
         * out:     a      b      c     d
         */
        /* a = [xx765432] */
        byte[0] = sp[0] >> 2;
        /* b = [xx10xxxx] | [xxxx7654]  */
        byte[1] = ((sp[0] & 0x03) << 4) | ((sp[1] & 0xF0) >> 4);
        /* c = [xx3210xx] | [xxxxxx76]  */
        byte[2] = ((sp[1] & 0x0F) << 2) | ((sp[2] & 0xC0) >> 6);
        /* d = [xx543210]  */
        byte[3] = (sp[2] & 0x3F);

        for (i = 0; i < 4; i++) {
            AddEncodedChar(&dp, encode64[byte[i]], switchesPtr);
        }
    }

    if (remainder > 0) {
        unsigned int byte[3];
        int i;
        
        /* 
         * Handle the two cases where the input buffer doesn't end on a 3-byte
         * boundary.
         */
        byte[0] = sp[0] >> 2;
        if (remainder == 2) {
            byte[1] = ((sp[0] & 0x03) << 4) | ((sp[1] & 0xF0) >> 4);
            byte[2] = ((sp[1] & 0x0F) << 2);
        } else if (remainder == 1) {
            byte[1] = ((sp[0] & 0x03) << 4);
        }
        for (i = 0; i <= remainder; i++) {
            AddEncodedChar(&dp, encode64[byte[i]], switchesPtr);
        }            
        for (i = remainder + 1; i < 4; i++) {
            AddEncodedChar(&dp, '=', switchesPtr);
        }
    }
    *numCharsPtr = (dp - dest);
    return TCL_OK;
}

size_t 
Blt_Base85EncodeBufferSize(size_t numBytes, EncodingSwitches *switchesPtr)
{
    size_t numChars, numLines;
    /*
     * Compute worst-case length of buffer needed for encoding.  That is 5
     * characters per 4 bytes.  The actual size can be smaller, depending
     * upon the number of 0 tuples ('z' bytes).
     */
    numChars = ((numBytes + 3) / 4) * 5;  /* 5 characters per 4 bytes. */
    numLines = 0;
    if (switchesPtr->wrapLength > 0) {  /* # of newlines required. */
        numLines = (numChars + (switchesPtr->wrapLength - 1)) /
            switchesPtr->wrapLength;    
    } 
    if (switchesPtr->wrap != NULL) {
        numChars += numLines * strlen(switchesPtr->wrap);
    } else {
        numChars += numLines;
    }
    if (switchesPtr->pad != NULL) {
        numChars += numLines * strlen(switchesPtr->pad);
    }
    numChars++;                         /* NUL byte */
    return numChars;
}

size_t 
Blt_Base85DecodeBufferSize(size_t numChars, DecodingSwitches *switchesPtr)
{
    size_t numBytes;
    /*
     * Compute worst-case length of buffer needed for encoding.  Normally,
     * that's 4 bytes per 5 characters.  But because of the 'y' and 'z'
     * characters there can be 4 bytes per character.
     */
    numBytes = numChars * 4;
    numBytes++;                           /* NUL byte. */
    return numBytes;
}

int
Blt_DecodeBase85(Tcl_Interp *interp, const char *src, size_t numChars,
                 unsigned char *dest, size_t *numBytesPtr,
                 DecodingSwitches *switchesPtr)
{
    unsigned char *dp;
    const char *p, *pend;
    int numBytesInBlock;
    unsigned int byte[5];
    
    dp = dest;
    for (p = src, pend = p + numChars; p < pend; /*empty*/) {
        unsigned int c;
        unsigned int value;
        unsigned int i;
        
        numBytesInBlock = 0;
        c = GetNextEncodedChar(&p, pend, switchesPtr, decode85);
        if (c == '\0') {
            break;
        }
        value = 0;
        byte[0] = c;
        c = GetNextEncodedChar(&p, pend, switchesPtr, decode85);
        if (c == '\0') {
            numBytesInBlock = 1;
            break;
        }
        byte[1] = c;
        c = GetNextEncodedChar(&p, pend, switchesPtr, decode85);
        if (c == '\0') {
            numBytesInBlock = 2;
            break;
        }
        byte[2] = c;
        c = GetNextEncodedChar(&p, pend, switchesPtr, decode85);
        if (c == '\0') {
            numBytesInBlock = 3;
            break;
        }
        byte[3] = c;
        c = GetNextEncodedChar(&p, pend, switchesPtr, decode85);
        if (c == '\0') {
            numBytesInBlock = 4;
            break;
        }
        byte[4] = c;
        
        for (i = 0; i < 5; i++) {
            if (decode85[byte[i]] == NA) {
                return TCL_ERROR;
            }
            value = (value * 85) + decode85[byte[i]];
        }
        
#ifdef WORDS_BIGENDIAN
        dp[0] = (value)       & 0xFF;
        dp[1] = (value >> 8)  & 0xFF;
        dp[2] = (value >> 16) & 0xFF;
        dp[3] = (value >> 24) & 0xFF;
#else
        dp[0] = (value >> 24) & 0xFF;
        dp[1] = (value >> 16) & 0xFF;
        dp[2] = (value >> 8)  & 0xFF;
        dp[3] = (value)       & 0xFF;
#endif
        dp += 4;
    }
    if (numBytesInBlock > 0) {
        unsigned int value;
        int i;
        
        for (i = numBytesInBlock; i < 5; i++) {
            byte[i] = '_';
        }
        value = 0;
        value = (value * 85) + decode85[byte[0]];
        value = (value * 85) + decode85[byte[1]];
        value = (value * 85) + decode85[byte[2]];
        value = (value * 85) + decode85[byte[3]];
        value = (value * 84) + decode85[byte[4]];
#ifdef WORDS_BIGENDIAN
        dp[0] = (value)       & 0xFF;
        dp[1] = (value >> 8)  & 0xFF;
        dp[2] = (value >> 16) & 0xFF;
        dp[3] = (value >> 24) & 0xFF;
#else
        dp[0] = (value >> 24) & 0xFF;
        dp[1] = (value >> 16) & 0xFF;
        dp[2] = (value >> 8)  & 0xFF;
        dp[3] = (value)       & 0xFF;
#endif
        dp += 4;
    }
    *dp++ = '\0';
    *numBytesPtr = dp - dest - (5 - numBytesInBlock);
    return TCL_OK;
}

int
Blt_EncodeBase85(const unsigned char *src, size_t numBytes, char *dest,
                 size_t *numCharsPtr, EncodingSwitches *switchesPtr) 
{
    char *dp; 
    int fill, remainder;
    const unsigned char *sp, *send;

    remainder = numBytes % 4;
    dp = dest;
    fill = 0;
    for (sp = src, send = sp + numBytes - remainder; sp < send; sp += 4) {
        unsigned int value;             /* Value of next 4 bytes. */
        unsigned int tuple[5];
        int i;

        value = 0;
#ifdef WORDS_BIGENDIAN
        value |= (sp[3] << 24);
        value |= (sp[2] << 16); 
        value |= (sp[1] <<  8);
        value |= sp[0];
#else 
        value |= (sp[0] << 24);
        value |= (sp[1] << 16); 
        value |= (sp[2] <<  8);
        value |= sp[3];
#endif
            
        tuple[4] = (value % 85);
        value /= 85;
        tuple[3] = (value % 85);
        value /= 85;
        tuple[2] = (value % 85);
        value /= 85;
        tuple[1] = (value % 85);
        value /= 85;
        tuple[0] = (value % 85);
        
        for (i = 0; i < 5; i++) {
            *dp++ = encode85[tuple[i]];
            fill++;
            if (fill >= 60) {
                *dp++ = '\n';
                fill = 0;
            }
        }
    }
    if (remainder > 0) {
        unsigned int value;
        unsigned int tuple[5];
        int i;

        /* Handle remaining bytes (0-3). */
        sp = src + numBytes - remainder;
        value = 0;
        switch (remainder) {
#ifdef WORDS_BIGENDIAN
        case 3:
            value |= (sp[2] << 24);
        case 2:
            value |= (sp[1] << 16); 
        case 1:
            value |= (sp[0] <<  8);
#else
        case 3:
            value |= (sp[2] <<  8);
        case 2:
            value |= (sp[1] << 16); 
        case 1:
            value |= (sp[0] << 24);
#endif
        default:
            break;
        }
        value /= 85;        
        tuple[3] = (value % 85);
        value /= 85;        
        tuple[2] = (value % 85);
        value /= 85;        
        tuple[1] = (value % 85);
        value /= 85;        
        tuple[0] = (value % 85);
        for (i = 0; i <= remainder; i++) {
            *dp++ = encode85[tuple[i]];
            fill++;
            if (fill >= 60) {
                *dp++ = '\n';
                fill = 0;
            }
        }
    }
    *numCharsPtr = (dp - dest);
    return TCL_OK;
}

size_t 
Blt_Ascii85EncodeBufferSize(size_t numBytes, EncodingSwitches *switchesPtr)
{
    size_t numChars, numLines;
    /*
     * Compute worst-case length of buffer needed for encoding.  That is 5
     * characters per 4 bytes.  The actual size can be smaller, depending
     * upon the number of 0 tuples ('z' bytes).
     */
    numChars = ((numBytes + 3) / 4) * 5; /* 5 characters per 4 bytes. */
    if (switchesPtr->flags & BRACKETS) {
        numChars += 4;
    }
    if (switchesPtr->wrapLength > 0) {  /* Newlines required. */
        numLines = (numChars + (switchesPtr->wrapLength - 1)) /
            switchesPtr->wrapLength;    
    } else {
        numLines = 1;
    }
    if (switchesPtr->wrap != NULL) {
        numChars += numLines * strlen(switchesPtr->wrap);
    } else {
        numChars += numLines;
    }
    if (switchesPtr->pad != NULL) {
        numChars += numLines * strlen(switchesPtr->pad);
    }
    numChars++;                         /* NUL byte */
    return numChars;
}

size_t 
Blt_Ascii85DecodeBufferSize(size_t numChars, DecodingSwitches *switchesPtr)
{
    size_t numBytes;
    /*
     * Compute worst-case length of buffer needed for encoding.  Normally,
     * that's 4 bytes per 5 characters.  But because of the 'y' and 'z'
     * characters there can be 4 bytes per character.
     */
    numBytes = numChars * 4;
    return numBytes;
}


/* 
 * -wraplength number
 * -pad string
 * -wrapchars "\r\n"
 * -adobebrackets yes 
 * -yoption yes
 */
int
Blt_EncodeAscii85(const unsigned char *src, size_t numBytes, char *dest,
                  size_t *numCharsPtr, EncodingSwitches *switchesPtr) 
{
    char *dp; 
    int remainder, initial;
    const unsigned char *sp, *send;

    remainder = numBytes % 4;
    dp = dest;
    initial = TRUE;
    for (sp = src, send = sp + numBytes - remainder; sp < send; sp += 4) {
        unsigned int value;             /* Value of next 4 bytes. */

        value = 0;
#ifdef WORDS_BIGENDIAN
        value |= (sp[3] << 24);
        value |= (sp[2] << 16); 
        value |= (sp[1] <<  8);
        value |= sp[0];
#else 
        value |= (sp[0] << 24);
        value |= (sp[1] << 16); 
        value |= (sp[2] <<  8);
        value |= sp[3];
#endif
        if ((initial) && (switchesPtr->flags & BRACKETS)) {
            AddEncodedChar(&dp, '<', switchesPtr);
            AddEncodedChar(&dp, '-', switchesPtr);
            initial = FALSE;
        }
        if ((switchesPtr->flags & COMPRESS_ZEROS) && (value == 0)) {
            AddEncodedChar(&dp, 'z', switchesPtr);
        } else if ((switchesPtr->flags & COMPRESS_SPACES) &&
                   (value == 0x20202020)) {
            AddEncodedChar(&dp, 'y', switchesPtr);
        } else {
            unsigned int tuple[5];
            int i;
            
            tuple[4] = (value % 85);
            value /= 85;
            tuple[3] = (value % 85);
            value /= 85;
            tuple[2] = (value % 85);
            value /= 85;
            tuple[1] = (value % 85);
            value /= 85;
            tuple[0] = (value % 85);

            for (i = 0; i < 5; i++) {
                AddEncodedChar(&dp, tuple[i] + '!', switchesPtr);
            }
        }
    }
    
    if (remainder > 0) {
        unsigned int value;
        unsigned int tuple[5];
        int i;

        /* Handle remaining bytes (0-3). */
        sp = src + numBytes - remainder;
        value = 0;
        /* Block is implicitly padded with 0 bytes.  */
        switch (remainder) {
#ifdef WORDS_BIGENDIAN
        case 3:
            value |= (sp[2] << 24);
        case 2:
            value |= (sp[1] << 16); 
        case 1:
            value |= (sp[0] <<  8);
#else
        case 3:
            value |= (sp[2] <<  8);
        case 2:
            value |= (sp[1] << 16); 
        case 1:
            value |= (sp[0] << 24);
#endif
        default:
            break;
        }
        value /= 85;        
        tuple[3] = (value % 85);
        value /= 85;        
        tuple[2] = (value % 85);
        value /= 85;        
        tuple[1] = (value % 85);
        value /= 85;        
        tuple[0] = (value % 85);
        for (i = 0; i <= remainder; i++) {
            AddEncodedChar(&dp, tuple[i] + '!', switchesPtr);
        }
    }
    if (switchesPtr->flags & BRACKETS) {
        AddEncodedChar(&dp, '-', switchesPtr);
        AddEncodedChar(&dp, '>', switchesPtr);
    }
    *numCharsPtr = (dp - dest);
    *dp++ = '\0';
    return TCL_OK;
}

int
Blt_DecodeAscii85(Tcl_Interp *interp, const char *src, size_t numChars,
                  unsigned char *dest, size_t *numBytesPtr,
                  DecodingSwitches *switchesPtr)
{
    unsigned char *dp;
    const char *p, *pend;
    int numBytesInBlock;
    unsigned int byte[5];
    static unsigned char decode85[256];
    static int initialized = FALSE;
    
    if (!initialized) {
        InitAscii85DecodeTable(decode85);
        initialized = TRUE;
    }
    dp = dest;
    for (p = src, pend = p + numChars; p < pend; /*empty*/) {
        unsigned int c;
        
        numBytesInBlock = 0;
        c = GetNextEncodedChar(&p, pend, switchesPtr, decode85);
        if (c == '\0') {
            break;
        }
        if (c == 'z') {
            /* Start of block is 'z' */
            dp[0] = dp[1] = dp[2] = dp[3] = '\0';
        } else if (c == 'y') {
            /* Start of block is 'y' */
            dp[0] = dp[1] = dp[2] = dp[3] = ' ';
        } else {
            unsigned int value;
            unsigned int i;

            value = 0;
            byte[0] = c;
            c = GetNextEncodedChar(&p, pend, switchesPtr, decode85);
            if (c == '\0') {
                numBytesInBlock = 1;
                break;
            }
            byte[1] = c;
            c = GetNextEncodedChar(&p, pend, switchesPtr, decode85);
            if (c == '\0') {
                numBytesInBlock = 2;
                break;
            }
            byte[2] = c;
            c = GetNextEncodedChar(&p, pend, switchesPtr, decode85);
            if (c == '\0') {
                numBytesInBlock = 3;
                break;
            }
            byte[3] = c;
            c = GetNextEncodedChar(&p, pend, switchesPtr, decode85);
            if (c == '\0') {
                numBytesInBlock = 4;
                break;
            }
            byte[4] = c;
            
            for (i = 0; i < 5; i++) {
                if ((byte[i] < '!') || (byte[i] > 'u')) {
                    Tcl_AppendResult(interp, "invalid character found at ",
                        Blt_Itoa(p - src - 5 + i + 1), (char *)NULL);
                    return TCL_ERROR;
                }
                value = (value * 85) + (byte[i] - '!');
            }
            
#ifdef WORDS_BIGENDIAN
            dp[0] = (value)       & 0xFF;
            dp[1] = (value >> 8)  & 0xFF;
            dp[2] = (value >> 16) & 0xFF;
            dp[3] = (value >> 24) & 0xFF;
#else
            dp[0] = (value >> 24) & 0xFF;
            dp[1] = (value >> 16) & 0xFF;
            dp[2] = (value >> 8)  & 0xFF;
            dp[3] = (value)       & 0xFF;
#endif
        }
        dp += 4;
    }
    if (numBytesInBlock > 0) {
        unsigned int value;
        int i;
        
        if (numBytesInBlock == 1) {
            if (byte[0] == 'z') {
                /* Start of block is 'z' */
                dp[0] = dp[1] = dp[2] = dp[3] = '\0';
                goto done;
            } else if (byte[0] == 'y') {
                /* Start of block is 'y' */
                dp[0] = dp[1] = dp[2] = dp[3] = ' ';
                goto done;
            }
        }
        for (i = numBytesInBlock; i < 5; i++) {
            byte[i] = 'u';              /* Pad block with 'u' */
        }
        value = 0;
        for (i = 0; i < 5; i++) {
            value = (value * 85) + (byte[i] - '!');
        }
#ifdef WORDS_BIGENDIAN
        dp[0] = (value)       & 0xFF;
        dp[1] = (value >> 8)  & 0xFF;
        dp[2] = (value >> 16) & 0xFF;
        dp[3] = (value >> 24) & 0xFF;
#else
        dp[0] = (value >> 24) & 0xFF;
        dp[1] = (value >> 16) & 0xFF;
        dp[2] = (value >> 8)  & 0xFF;
        dp[3] = (value)       & 0xFF;
#endif
 done:
        dp += 4;
    }
    *numBytesPtr = dp - dest;
    if (numBytesInBlock > 0) {
        *numBytesPtr -= (5 - numBytesInBlock);
    }
    *dp++ = '\0';
    return TCL_OK;
}

/*ARGSUSED*/
static int
DecodeCmd(ClientData clientData, Tcl_Interp *interp, int objc, 
          Tcl_Obj *const *objv)
{
    DecodingSwitches args;
    FormatClass *classPtr;
    Tcl_Obj *objPtr;
    const char *format, *src;
    int numChars;
    size_t numBytes, maxBytes;
    unsigned char *dest;
    int length;
    char c;
    
    if (objc < 3) {
        Tcl_AppendResult(interp, "wrong # args: should be \"", 
                Tcl_GetString(objv[0]), " formatName string ?switches ...?\"",
                (char *)NULL);
        return TCL_ERROR;
    }
    format = Tcl_GetStringFromObj(objv[1], &length);
    c = format[0];
    src = Tcl_GetStringFromObj(objv[2], &numChars);
    if ((c == 'b') && (strncmp(format, "base64", length) == 0)) {
        classPtr = &base64Class;
    } else if ((c == 'h') && (strncmp(format, "hexadecimal", length) == 0)) {
        classPtr = &hexadecimalClass;
    } else if ((c == 'a') && (strncmp(format, "ascii85", length) == 0)) {
        classPtr = &ascii85Class;
    } else {
        Tcl_AppendResult(interp, "bad format \"", format, 
                "\": should be hexadecimal, base64, or ascii85", (char *)NULL);
        return TCL_ERROR;
    }
    memset(&args, 0, sizeof(args));
    if (Blt_ParseSwitches(interp, classPtr->decodeSpecs, objc - 3 , objv + 3, 
        &args, BLT_SWITCH_DEFAULTS) < 0) {
        return TCL_ERROR;
    }
    
    maxBytes = (*classPtr->decodeSizeProc)(numChars, &args);
    dest = Blt_Malloc(sizeof(unsigned char) * maxBytes);
    if (dest == NULL) {
        Tcl_AppendResult(interp, "can't allocate ", Blt_Itoa(maxBytes),
                " bytes for decode buffer.", (char *)NULL);
        return TCL_ERROR;
    }
    if ((*classPtr->decodeProc)(interp, src, numChars, dest, &numBytes, &args)
        != TCL_OK) {
        Blt_Free(dest);
        return TCL_ERROR;
    }
    objPtr = Tcl_NewByteArrayObj(dest, numBytes);
    Blt_Free(dest);
    if (args.fileObjPtr != NULL) {
        Tcl_Channel channel;
        const char *fileName;
        int closeChannel;
        
        closeChannel = TRUE;
        fileName = Tcl_GetString(args.fileObjPtr);
        if ((fileName[0] == '@') && (fileName[1] != '\0')) {
            int mode;
            
            channel = Tcl_GetChannel(interp, fileName+1, &mode);
            if (channel == NULL) {
                goto error;
            }
            if ((mode & TCL_WRITABLE) == 0) {
                Tcl_AppendResult(interp, "channel \"", fileName, 
                                 "\" not opened for writing", (char *)NULL);
                goto error;
            }
            closeChannel = FALSE;
        } else {
            channel = Tcl_OpenFileChannel(interp, fileName, "w", 0666);
            if (channel == NULL) {
                goto error;             /* Can't open file. */
            }
        }
        Tcl_WriteObj(channel, objPtr);
        if (closeChannel) {
            Tcl_Close(interp, channel);
        }
    } else if (args.dataObjPtr != NULL) {
        /* Write the image into the designated TCL variable. */
        objPtr = Tcl_ObjSetVar2(interp, args.dataObjPtr, NULL, objPtr, 0);
        if (objPtr == NULL) {
            goto error;
        }
    } else {
        Tcl_SetObjResult(interp, objPtr);
    }
    Blt_FreeSwitches(classPtr->decodeSpecs, (char *)&args, 0);
    return TCL_OK;
 error:
    Blt_FreeSwitches(classPtr->decodeSpecs, (char *)&args, 0);
    return TCL_ERROR;
}

/*ARGSUSED*/
static int
EncodeCmd(ClientData clientData, Tcl_Interp *interp, int objc, 
          Tcl_Obj *const *objv)
{
    EncodingSwitches args;
    FormatClass *classPtr;
    Tcl_Obj *objPtr;
    char *dest;
    const char *format;
    const unsigned char *src;
    int numBytes;
    size_t numChars, maxChars;
    int length;
    char c;
    
    if (objc < 3) {
        Tcl_AppendResult(interp, "wrong # args: should be \"", 
                Tcl_GetString(objv[0]), " formatName string ?switches ...?\"",
                (char *)NULL);
        return TCL_ERROR;
    }
    format = Tcl_GetStringFromObj(objv[1], &length);
    c = format[0];
    src = Tcl_GetByteArrayFromObj(objv[2], &numBytes);
    if ((c == 'b') && (strncmp(format, "base64", length) == 0)) {
        classPtr = &base64Class;
    } else if ((c == 'h') && (strncmp(format, "hexadecimal", length) == 0)) {
        classPtr = &hexadecimalClass;
    } else if ((c == 'a') && (strncmp(format, "ascii85", length) == 0)) {
        classPtr = &ascii85Class;
    } else {
        Tcl_AppendResult(interp, "bad format \"", format, 
                "\": should be hexadecimal, base64, or ascii85", (char *)NULL);
        return TCL_ERROR;
    }
    memset(&args, 0, sizeof(args));
    args.wrapLength = classPtr->wrapLength;
    if (Blt_ParseSwitches(interp, classPtr->encodeSpecs, objc - 3 , objv + 3, 
        &args, BLT_SWITCH_DEFAULTS) < 0) {
        return TCL_ERROR;
    }
    maxChars = (*classPtr->encodeSizeProc)(numBytes, &args);
    dest = Blt_Malloc(sizeof(char) * maxChars);
    if (dest == NULL) {
        Tcl_AppendResult(interp, "can't allocate ", Blt_Itoa(maxChars),
                " bytes for encode buffer.", (char *)NULL);
        return TCL_ERROR;
    }
    (*classPtr->encodeProc)(src, numBytes, dest, &numChars, &args);
#ifdef notdef
    fprintf(stderr, "numChars=%ld maxChars=%ld\n", numChars, maxChars);
#endif
    assert(numChars <= maxChars);
    objPtr = Tcl_NewStringObj(dest, numChars);
    Blt_Free(dest);
    if (args.fileObjPtr != NULL) {
        Tcl_Channel channel;
        const char *fileName;
        int closeChannel;
        
        closeChannel = TRUE;
        fileName = Tcl_GetString(args.fileObjPtr);
        if ((fileName[0] == '@') && (fileName[1] != '\0')) {
            int mode;
            
            channel = Tcl_GetChannel(interp, fileName+1, &mode);
            if (channel == NULL) {
                goto error;
            }
            if ((mode & TCL_WRITABLE) == 0) {
                Tcl_AppendResult(interp, "channel \"", fileName, 
                                 "\" not opened for writing", (char *)NULL);
                goto error;
            }
            closeChannel = FALSE;
        } else {
            channel = Tcl_OpenFileChannel(interp, fileName, "w", 0666);
            if (channel == NULL) {
                goto error;             /* Can't open output file. */
            }
        }
        Tcl_WriteObj(channel, objPtr);
        if (closeChannel) {
            Tcl_Close(interp, channel);
        }
    } else if (args.dataObjPtr != NULL) {
        /* Write the image into the designated TCL variable. */
        objPtr = Tcl_ObjSetVar2(interp, args.dataObjPtr, NULL, objPtr, 0);
        if (objPtr == NULL) {
            goto error;
        }
    } else {
        Tcl_SetObjResult(interp, objPtr);
    }
    Blt_FreeSwitches(classPtr->encodeSpecs, (char *)&args, 0);
    return TCL_OK;
 error:
    Blt_FreeSwitches(classPtr->encodeSpecs, (char *)&args, 0);
    return TCL_ERROR;
}

Blt_DBuffer 
Blt_DecodeBase64ToBuffer(Tcl_Interp *interp, const char *src, size_t numChars)
{
    Blt_DBuffer dbuffer;
    size_t numBytes, maxBytes;
    unsigned char *dest;
    DecodingSwitches switches;

    memset(&switches, 0, sizeof(DecodingSwitches));
    maxBytes = Blt_Base64DecodeBufferSize(numChars, &switches);
    dest = Blt_Malloc(sizeof(unsigned char) * maxBytes);
    if (dest == NULL) {
        Tcl_AppendResult(interp, "can't allocate ", Blt_Itoa(maxBytes),
                " bytes for decode buffer.", (char *)NULL);
        return NULL;
    }
    if (Blt_DecodeHexadecimal(interp, src, numChars, dest, &numBytes, &switches)
        != TCL_OK) {
        Blt_Free(dest);
        return NULL;
    }
    dbuffer = Blt_DBuffer_Create();
    Blt_DBuffer_AppendData(dbuffer, dest, numBytes);
    Blt_Free(dest);
    return dbuffer;
}

Tcl_Obj *
Blt_DecodeHexadecimalToObj(Tcl_Interp *interp, const char *src, size_t numChars)
{
    unsigned char *dest;
    size_t numBytes, maxBytes;
    Tcl_Obj *objPtr;
    DecodingSwitches switches;

    memset(&switches, 0, sizeof(DecodingSwitches));
    maxBytes = Blt_HexadecimalDecodeBufferSize(numChars, &switches);
    dest = Blt_Malloc(sizeof(unsigned char) * maxBytes);
    if (dest == NULL) {
        Tcl_AppendResult(interp, "can't allocate ", Blt_Itoa(maxBytes),
                " bytes for decode buffer.", (char *)NULL);
        return NULL;
    }
    if (Blt_DecodeHexadecimal(interp, src, numChars, dest, &numBytes, &switches)
        != TCL_OK) {
        Blt_Free(dest);
        return NULL;
    }
    objPtr = Tcl_NewByteArrayObj(dest, numBytes);
    Blt_Free(dest);
    return objPtr;
}

Tcl_Obj *
Blt_DecodeBase64ToObj(Tcl_Interp *interp, const char *src, size_t numChars)
{
    Tcl_Obj *objPtr;
    size_t numBytes, maxBytes;
    unsigned char *dest;
    DecodingSwitches switches;

    memset(&switches, 0, sizeof(DecodingSwitches));
    maxBytes = Blt_Base64DecodeBufferSize(numChars, &switches);
    dest = Blt_Malloc(sizeof(unsigned char) * maxBytes);
    if (dest == NULL) {
        Tcl_AppendResult(interp, "can't allocate ", Blt_Itoa(maxBytes),
                " bytes for decode buffer.", (char *)NULL);
        return NULL;
    }
    if (Blt_DecodeBase64(interp, src, numChars, dest, &numBytes, &switches)
        != TCL_OK) {
        Blt_Free(dest);
        return NULL;
    }
    objPtr = Tcl_NewByteArrayObj(dest, numBytes);
    Blt_Free(dest);
    return objPtr;
}

Tcl_Obj *
Blt_DecodeBase85ToObj(Tcl_Interp *interp, const char *src, size_t numChars)
{
    unsigned char *dest;
    Tcl_Obj *objPtr;
    size_t numBytes, maxBytes;
    DecodingSwitches switches;

    memset(&switches, 0, sizeof(DecodingSwitches));
    maxBytes = Blt_Base85DecodeBufferSize(numChars, &switches);
    dest = Blt_Malloc(sizeof(unsigned char) * maxBytes);
    if (dest == NULL) {
        Tcl_AppendResult(interp, "can't allocate ", Blt_Itoa(maxBytes),
                " bytes for decode buffer.", (char *)NULL);
        return NULL;
    }
    if (Blt_DecodeBase85(interp, src, numChars, dest, &numBytes, &switches)
        != TCL_OK) {
        Blt_Free(dest);
        return NULL;
    }
    objPtr = Tcl_NewByteArrayObj(dest, numBytes);
    Blt_Free(dest);
    return objPtr;
}

Tcl_Obj *
Blt_EncodeBase64ToObj(const unsigned char *src, size_t numBytes) 
{
    Tcl_Obj *objPtr;
    char *dest;
    size_t numChars, maxChars;
    EncodingSwitches switches;

    memset(&switches, 0, sizeof(EncodingSwitches));
    maxChars = Blt_Base64EncodeBufferSize(numBytes, &switches);
    dest = Blt_Malloc(sizeof(char) * maxChars);
    if (dest == NULL) {
        return NULL;
    }
    Blt_EncodeBase64(src, numBytes, dest, &numChars, &switches);
    assert(numChars <= maxChars);
    objPtr = Tcl_NewStringObj(dest, numChars);
    Blt_Free(dest);
    return objPtr;
}

Tcl_Obj *
Blt_EncodeBase85ToObj(const unsigned char *src, size_t numBytes) 
{
    Tcl_Obj *objPtr;
    char *dest;
    size_t numChars, maxChars;
    EncodingSwitches switches;

    memset(&switches, 0, sizeof(EncodingSwitches));
    maxChars = Blt_Base85EncodeBufferSize(numBytes, &switches);
    dest = Blt_Malloc(sizeof(char) * maxChars);
    if (dest == NULL) {
        return NULL;
    }
    Blt_EncodeBase85(src, numBytes, dest, &numChars, &switches);
    assert(numChars <= maxChars);
    objPtr = Tcl_NewStringObj(dest, numChars);
    Blt_Free(dest);
    return objPtr;
}

Tcl_Obj *
Blt_EncodeHexadecimalToObj(const unsigned char *src, size_t numBytes) 
{
    Tcl_Obj *objPtr;
    char *dest;
    size_t numChars, maxChars;
    EncodingSwitches switches;

    memset(&switches, 0, sizeof(EncodingSwitches));
    maxChars = Blt_HexadecimalEncodeBufferSize(numBytes, &switches);
    dest = Blt_Malloc(sizeof(char) * maxChars);
    if (dest == NULL) {
        return NULL;
    }
    Blt_EncodeHexadecimal(src, numBytes, dest, &numChars, &switches);
    assert(numChars <= maxChars);
    objPtr = Tcl_NewStringObj(dest, numChars);
    Blt_Free(dest);
    return objPtr;
}

int
Blt_Base64CmdInitProc(Tcl_Interp *interp)
{
    static Blt_CmdSpec cmdSpecs[] = {
        { "encode", EncodeCmd, 0, 0 },
        { "decode", DecodeCmd, 0, 0 }
    };
    return Blt_InitCmds(interp, "::blt", cmdSpecs, 2);
}

