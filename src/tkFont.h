/* -*- mode: c; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 * tkFont.h --
 *
 *
 * This file contains definitions of internal Tk font structures.
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
 */

#ifndef _TK_FONT_H
#define _TK_FONT_H

/*
 * Possible values for the "weight" field in a TkFontAttributes structure.
 * Weight is a subjective term and depends on what the company that created
 * the font considers bold.
 */

#define TK_FW_NORMAL    0
#define TK_FW_BOLD      1

#define TK_FW_UNKNOWN   -1      /* Unknown weight.  This value is used for
                                 * error checking and is never actually stored
                                 * in the weight field. */

/*
 * Possible values for the "slant" field in a TkFontAttributes structure.
 */

#define TK_FS_ROMAN     0       
#define TK_FS_ITALIC    1
#define TK_FS_OBLIQUE   2       /* This value is only used when parsing X
                                 * font names to determine the closest
                                 * match.  It is only stored in the
                                 * XLFDAttributes structure, never in the
                                 * slant field of the TkFontAttributes. */

#define TK_FS_UNKNOWN   -1      /* Unknown slant.  This value is used for
                                 * error checking and is never actually stored
                                 * in the slant field. */
typedef struct {
    Tk_Uid family;              /* Font family. The most important field. */
#if (_TCL_VERSION >=  _VERSION(8,6,0)) 
    double size;
#else
    int size;                   /* Pointsize of font, 0 for default size, or
                                 * negative number meaning pixel size. */
#endif
    int weight;                 /* Weight flag; see below for def'n. */
    int slant;                  /* Slant flag; see below for def'n. */
    int underline;              /* Non-zero for underline font. */
    int overstrike;             /* Non-zero for overstrike font. */
} TkFontAttributes;

typedef struct {
    int ascent;                 /* From baseline to top of font. */
    int descent;                /* From baseline to bottom of font. */
    int maxWidth;               /* Width of widest character in font. */
    int fixed;                  /* Non-zero if this is a fixed-width font,
                                 * 0 otherwise. */
} TkFontMetrics;


typedef struct _TkFont {
    /*
     * Fields used and maintained exclusively by generic code.
     */
#if (_TK_VERSION >= _VERSION(8,1,0))
    int resourceRefCount;       /* Number of active uses of this font (each
                                 * active use corresponds to a call to
                                 * Tk_AllocFontFromTable or Tk_GetFont).
                                 * If this count is 0, then this TkFont
                                 * structure is no longer valid and it isn't
                                 * present in a hash table: it is being
                                 * kept around only because there are objects
                                 * referring to it.  The structure is freed
                                 * when resourceRefCount and objRefCount
                                 * are both 0. */
    int objRefCount;            /* The number of TCL objects that reference
                                 * this structure. */
#else
    int refCount;               /* Number of users of the TkFont. */
#endif
    Tcl_HashEntry *cacheHashPtr;/* Entry in font cache for this structure,
                                 * used when deleting it. */
    Tcl_HashEntry *namedHashPtr;/* Pointer to hash table entry that
                                 * corresponds to the named font that the
                                 * tkfont was based on, or NULL if the tkfont
                                 * was not based on a named font. */
#if (_TK_VERSION >= _VERSION(8,1,0))
    Screen *screen;             /* The screen where this font is valid. */
#endif /* _TK_VERSION >= 8.1.0 */
    int tabWidth;               /* Width of tabs in this font (pixels). */
    int underlinePos;           /* Offset from baseline to origin of
                                 * underline bar (used for drawing underlines
                                 * on a non-underlined font). */
    int underlineHeight;        /* Height of underline bar (used for drawing
                                 * underlines on a non-underlined font). */

    /*
     * Fields in the generic font structure that are filled in by
     * platform-specific code.
     */

    Font fid;                   /* For backwards compatibility with XGCValues
                                 * structures.  Remove when TkGCValues is
                                 * implemented.  */
    TkFontAttributes fa;        /* Actual font attributes obtained when the
                                 * the font was created, as opposed to the
                                 * desired attributes passed in to
                                 * TkpGetFontFromAttributes().  The desired
                                 * metrics can be determined from the string
                                 * that was used to create this font. */
    TkFontMetrics fm;           /* Font metrics determined when font was
                                 * created. */
#if (_TK_VERSION >= _VERSION(8,1,0))
    struct _TkFont *nextPtr;    /* Points to the next TkFont structure with
                                 * the same name.  All fonts with the
                                 * same name (but different displays) are
                                 * chained together off a single entry in
                                 * a hash table. */
#endif /* _TK_VERSION >= 8.1.0 */
} TkFont;

typedef struct TkXLFDAttributes {
    Tk_Uid foundry;             /* The foundry of the font. */
    int slant;                  /* The tristate value for the slant, which
                                 * is significant under X. */
    int setwidth;               /* The proportionate width, see below for
                                 * definition. */
    Tk_Uid charset;             /* The actual charset string. */
} TkXLFDAttributes;


#ifdef notdef
static const char *encodingList[] = {
    "ucs-2be", "iso8859-1", "jis0208", "jis0212", NULL
};
#endif
/*
 * The following structure and definition is used to keep track of the
 * alternative names for various encodings.  Asking for an encoding that
 * matches one of the alias patterns will result in actually getting the
 * encoding by its real name.
 */
 
typedef struct EncodingAlias {
    char *realName;             /* The real name of the encoding to load if
                                 * the provided name matched the pattern. */
    char *aliasPattern;         /* Pattern for encoding name, of the form
                                 * that is acceptable to Tcl_StringMatch. */
} EncodingAlias;

/*
 * Just some utility structures used for passing around values in helper
 * procedures.
 */
 
typedef struct FontAttributes {
    TkFontAttributes fa;
    TkXLFDAttributes xa;
} FontAttributes;

typedef struct TkFontInfo {
    Tcl_HashTable fontCache;    /* Map a string to an existing Tk_Font.
                                 * Keys are string font names, values are
                                 * TkFont pointers. */
    Tcl_HashTable namedTable;   /* Map a name to a set of attributes for a
                                 * font, used when constructing a Tk_Font from
                                 * a named font description.  Keys are
                                 * strings, values are NamedFont pointers. */
    TkMainInfo *mainPtr;        /* Application that owns this structure. */
    int updatePending;          /* Non-zero when a World Changed event has
                                 * already been queued to handle a change to
                                 * a named font. */
} TkFontInfo;

#endif /* _TK_FONT_H */
