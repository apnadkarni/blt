/* -*- mode: c; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 * tkWinFont.h --
 *
 * This file contains structure definitions from tkWinFont.h of the Tk
 * library distribution.
 *
 * Copyright (c) 1998-1999 by Scriptics Corporation.
 *
 *   See the file "license.terms" for information on usage and
 *   redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.
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

#ifndef _TK_WINFONT_H
#define _TK_WINFONT_H

/*
 * The following structure represents a font family.  It is assumed that
 * all screen fonts constructed from the same "font family" share certain
 * properties; all screen fonts with the same "font family" point to a
 * shared instance of this structure.  The most important shared property
 * is the character existence metrics, used to determine if a screen font
 * can display a given Unicode character.
 *
 * Under Windows, a "font family" is uniquely identified by its face name.
 */

#define FONTMAP_SHIFT       10

#define FONTMAP_PAGES           (1 << (sizeof(Tcl_UniChar)*8 - FONTMAP_SHIFT))
#define FONTMAP_BITSPERPAGE     (1 << FONTMAP_SHIFT)


typedef struct FontFamily {
    struct FontFamily *nextPtr; /* Next in list of all known font families. */
    int refCount;               /* How many SubFonts are referring to this
                                 * FontFamily.  When the refCount drops to
                                 * zero, this FontFamily may be freed. */
    /*
     * Key.
     */
     
    Tk_Uid faceName;            /* Face name key for this FontFamily. */

    /*
     * Derived properties.
     */
     
    Tcl_Encoding encoding;      /* Encoding for this font family. */
    int isSymbolFont;           /* Non-zero if this is a symbol font. */
    int isWideFont;             /* 1 if this is a double-byte font, 0 
                                 * otherwise. */
    BOOL (WINAPI *textOutProc)(HDC, int, int, TCHAR *, int);
                                /* The procedure to use to draw text after
                                 * it has been converted from UTF-8 to the 
                                 * encoding of this font. */
    BOOL (WINAPI *getTextExtentPoint32Proc)(HDC, TCHAR *, int, LPSIZE);
                                /* The procedure to use to measure text after
                                 * it has been converted from UTF-8 to the 
                                 * encoding of this font. */

    char *fontMap[FONTMAP_PAGES];
                                /* Two-level sparse table used to determine
                                 * quickly if the specified character exists.
                                 * As characters are encountered, more pages
                                 * in this table are dynamically added.  The
                                 * contents of each page is a bitmask
                                 * consisting of FONTMAP_BITSPERPAGE bits,
                                 * representing whether this font can be used
                                 * to display the given character at the
                                 * corresponding bit position.  The high bits
                                 * of the character are used to pick which
                                 * page of the table is used. */

    /*
     * Cached Truetype font info.
     */
     
    int segCount;               /* The length of the following arrays. */
    USHORT *startCount;         /* Truetype information about the font, */
    USHORT *endCount;           /* indicating which characters this font
                                 * can display (malloced).  The format of
                                 * this information is (relatively) compact,
                                 * but would take longer to search than 
                                 * indexing into the fontMap[][] table. */
} FontFamily;

/*
 * The following structure encapsulates an individual screen font.  A font
 * object is made up of however many SubFonts are necessary to display a
 * stream of multilingual characters.
 */
typedef struct SubFont {
    char **fontMap;             /* Pointer to font map from the FontFamily, 
                                 * cached here to save a dereference. */
    HFONT hFont;                /* The specific screen font that will be
                                 * used when displaying/measuring chars
                                 * belonging to the FontFamily. */
    FontFamily *familyPtr;      /* The FontFamily for this SubFont. */
} SubFont;

/*
 * The following structure represents Windows' implementation of a font
 * object.
 */

#define SUBFONT_SPACE           3
#define BASE_CHARS              128

typedef struct WinFont {
    TkFont font;                /* Stuff used by generic font package.  Must
                                 * be first in structure. */
    SubFont staticSubFonts[SUBFONT_SPACE];
                                /* Builtin space for a limited number of
                                 * SubFonts. */
    int numSubFonts;            /* Length of following array. */
    SubFont *subFontArray;      /* Array of SubFonts that have been loaded
                                 * in order to draw/measure all the characters
                                 * encountered by this font so far.  All fonts
                                 * start off with one SubFont initialized by
                                 * AllocFont() from the original set of font
                                 * attributes.  Usually points to
                                 * staticSubFonts, but may point to malloced
                                 * space if there are lots of SubFonts. */

    HWND hwnd;                  /* Toplevel window of application that owns
                                 * this font, used for getting HDC for
                                 * offscreen measurements. */
    int pixelSize;              /* Original pixel size used when font was
                                 * constructed. */
    int widths[BASE_CHARS];     /* Widths of first 128 chars in the base
                                 * font, for handling common case.  The base
                                 * font is always used to draw characters
                                 * between 0x0000 and 0x007f. */
} WinFont;

#endif /* _TK_WINFONT_H */
