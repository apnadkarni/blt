/* -*- mode: c; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 * tkUnixFont.h --
 *
 * The Font structure used internally by the Tk_3D* routines.
 * The following is a copy of it from tk3d.c.
 *
 * Contains copies of internal Tk structures.
 *
 * Copyright (c) 1997 by Sun Microsystems, Inc.
 *
 *   This software is copyrighted by the Regents of the University of
 *   California, Sun Microsystems, Inc., and other parties.  The following
 *   terms apply to all files associated with the software unless
 *   explicitly disclaimed in individual files.
 * 
 *   The authors hereby grant permission to use, copy, modify, distribute,
 *   and license this software and its documentation for any purpose,
 *   provided that existing copyright notices are retained in all copies
 *   and that this notice is included verbatim in any distributions. No
 *   written agreement, license, or royalty fee is required for any of the
 *   authorized uses.  Modifications to this software may be copyrighted by
 *   their authors and need not follow the licensing terms described here,
 *   provided that the new terms are clearly indicated on the first page of
 *   each file where they apply.
 * 
 *   IN NO EVENT SHALL THE AUTHORS OR DISTRIBUTORS BE LIABLE TO ANY PARTY
 *   FOR DIRECT, INDIRECT, SPECIAL, INCIDENTAL, OR CONSEQUENTIAL DAMAGES
 *   ARISING OUT OF THE USE OF THIS SOFTWARE, ITS DOCUMENTATION, OR ANY
 *   DERIVATIVES THEREOF, EVEN IF THE AUTHORS HAVE BEEN ADVISED OF THE
 *   POSSIBILITY OF SUCH DAMAGE.
 * 
 *   THE AUTHORS AND DISTRIBUTORS SPECIFICALLY DISCLAIM ANY WARRANTIES,
 *   INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 *   MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE, AND
 *   NON-INFRINGEMENT.  THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, AND
 *   THE AUTHORS AND DISTRIBUTORS HAVE NO OBLIGATION TO PROVIDE
 *   MAINTENANCE, SUPPORT, UPDATES, ENHANCEMENTS, OR MODIFICATIONS.
 *
 *   GOVERNMENT USE: If you are acquiring this software on behalf of the
 *   U.S. government, the Government shall have only "Restricted Rights" in
 *   the software and related documentation as defined in the Federal
 *   Acquisition Regulations (FARs) in Clause 52.227.19 (c) (2).  If you
 *   are acquiring the software on behalf of the Department of Defense, the
 *   software shall be classified as "Commercial Computer Software" and the
 *   Government shall have only "Restricted Rights" as defined in Clause
 *   252.227-7013 (b) (3) of DFARs.  Notwithstanding the foregoing, the
 *   authors grant the U.S. Government and others acting in its behalf
 *   permission to use and distribute the software in accordance with the
 *   terms specified in this license.
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

#ifndef _TK_UNIXFONT_H
#define _TK_UNIXFONT_H


/*
 * The following structure represents a font family.  It is assumed that all
 * screen fonts constructed from the same "font family" share certain
 * properties; all screen fonts with the same "font family" point to a shared
 * instance of this structure.  The most important shared property is the
 * character existence metrics, used to determine if a screen font can display
 * a given Unicode character.
 *
 * Under Unix, there are three attributes that uniquely identify a "font
 * family": the foundry, face name, and charset.
 */

#define FONTMAP_SHIFT           10

#define FONTMAP_PAGES           (1 << (sizeof(Tcl_UniChar)*8 - FONTMAP_SHIFT))
#define FONTMAP_BITSPERPAGE     (1 << FONTMAP_SHIFT)

typedef struct _FontFamily {
    struct _FontFamily *nextPtr; /* Next in list of all known font families. */
    int refCount;               /* How many SubFonts are referring to this
                                 * FontFamily.  When the refCount drops to
                                 * zero, this FontFamily may be freed. */
    /*
     * Key.
     */

    Tk_Uid foundry;             /* Foundry key for this FontFamily. */
    Tk_Uid faceName;            /* Face name key for this FontFamily. */
    Tcl_Encoding encoding;      /* Encoding key for this FontFamily. */

    /*
     * Derived properties.
     */

    int isTwoByteFont;          /* 1 if this is a double-byte font, 0 
                                 * otherwise. */
    char *fontMap[FONTMAP_PAGES];
                                /* Two-level sparse table used to determine
                                 * quickly if the specified character exists.
                                 * As characters are encountered, more pages
                                 * in this table are dynamically alloced.  The
                                 * contents of each page is a bitmask
                                 * consisting of FONTMAP_BITSPERPAGE bits,
                                 * representing whether this font can be used
                                 * to display the given character at the
                                 * corresponding bit position.  The high bits
                                 * of the character are used to pick which
                                 * page of the table is used. */
} FontFamily;

/*
 * The following structure encapsulates an individual screen font.  A font
 * object is made up of however many SubFonts are necessary to display a
 * stream of multilingual characters.
 */
typedef struct _SubFont {
    char **fontMap;             /* Pointer to font map from the FontFamily, 
                                 * cached here to save a dereference. */
    XFontStruct *fontStructPtr; /* The specific screen font that will be
                                 * used when displaying/measuring chars
                                 * belonging to the FontFamily. */
    FontFamily *familyPtr;      /* The FontFamily for this SubFont. */
} SubFont;

/*
 * The following structure represents Unix's implementation of a font
 * object.
 */
 
#define SUBFONT_SPACE           3
#define BASE_CHARS              256

typedef struct _UnixFont {
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
    SubFont controlSubFont;     /* Font to use to display control-character
                                 * expansions. */

    Display *display;           /* Display that owns font. */
    int pixelSize;              /* Original pixel size used when font was
                                 * constructed. */
    TkXLFDAttributes xa;        /* Additional attributes that specify the
                                 * preferred foundry and encoding to use when
                                 * constructing additional SubFonts. */
    int widths[BASE_CHARS];     /* Widths of first 256 chars in the base
                                 * font, for handling common case. */
    int underlinePos;           /* Offset from baseline to origin of
                                 * underline bar (used when drawing underlined
                                 * font) (pixels). */
    int barHeight;              /* Height of underline or overstrike bar
                                 * (used when drawing underlined or strikeout
                                 * font) (pixels). */
} UnixFont;

#endif /* _TK_UNIXFONT_H */
