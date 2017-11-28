/* -*- mode: c; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 * bltWinFont.c --
 *
 * This module implements rotated/scaled fonts for the BLT toolkit.  
 *
 * This is different from the Unix fonts. Here, we're really just
 * augmenting the existing Tk font, not replacing it with a secondary
 * implementation (like Xft).
 *
 * For rotated fonts, the idea is to create a super Blt_Font structure that
 * subsumes the Tk font structure but also can display a single font at
 * various angles.  Rotated fonts are created by digging out the Windows
 * font handle from the Tk font and then calling CreateFontIndirect to
 * generate a new font for that angle.  The rotated fonts are stored/cached
 * in a hash table.
 *
 * For scaled fonts, this is a routine to duplicate the original font at
 * the new size from C code.
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

#define BUILD_BLT_TK_PROCS 1
#include "bltInt.h"
#include <X11/Xutil.h>
#include <ctype.h>
#include "bltAlloc.h"
#include "bltMath.h"
#include "bltString.h"
#include <bltHash.h>
#include "tkDisplay.h"
#include "tkFont.h"
#include "bltFont.h"
#include "bltAfm.h"
#include "tkWinFont.h"

#define DEBUG_FONT_SELECTION    0
#define DEBUG_FONT_SELECTION2   0

typedef struct _Blt_Font _Blt_Font;

enum FontsetTypes { 
    FONTSET_UNKNOWN=1,                  /* Unknown font type. */
    FONTSET_STD,                        /* Normal Tk font. */
    FONTSET_EXT                         /* Extended Tk font. */
};

#ifndef HAVE_LIBXFT
#define FC_WEIGHT_THIN              0
#define FC_WEIGHT_EXTRALIGHT        40
#define FC_WEIGHT_ULTRALIGHT        FC_WEIGHT_EXTRALIGHT
#define FC_WEIGHT_LIGHT             50
#define FC_WEIGHT_BOOK              75
#define FC_WEIGHT_REGULAR           80
#define FC_WEIGHT_NORMAL            FC_WEIGHT_REGULAR
#define FC_WEIGHT_MEDIUM            100
#define FC_WEIGHT_DEMIBOLD          180
#define FC_WEIGHT_SEMIBOLD          FC_WEIGHT_DEMIBOLD
#define FC_WEIGHT_BOLD              200
#define FC_WEIGHT_EXTRABOLD         205
#define FC_WEIGHT_ULTRABOLD         FC_WEIGHT_EXTRABOLD
#define FC_WEIGHT_BLACK             210
#define FC_WEIGHT_HEAVY             FC_WEIGHT_BLACK
#define FC_WEIGHT_EXTRABLACK        215
#define FC_WEIGHT_ULTRABLACK        FC_WEIGHT_EXTRABLACK

#define FC_SLANT_ROMAN              0
#define FC_SLANT_ITALIC             100
#define FC_SLANT_OBLIQUE            110

#define FC_WIDTH_ULTRACONDENSED     50
#define FC_WIDTH_EXTRACONDENSED     63
#define FC_WIDTH_CONDENSED          75
#define FC_WIDTH_SEMICONDENSED      87
#define FC_WIDTH_NORMAL             100
#define FC_WIDTH_SEMIEXPANDED       113
#define FC_WIDTH_EXPANDED           125
#define FC_WIDTH_EXTRAEXPANDED      150
#define FC_WIDTH_ULTRAEXPANDED      200

#define FC_PROPORTIONAL             0
#define FC_DUAL                     90
#define FC_MONO                     100
#define FC_CHARCELL                 110

#define FC_ANTIALIAS        "antialias"         /* Bool (depends) */
#define FC_AUTOHINT         "autohint"          /* Bool (false) */
#define FC_DECORATIVE       "decorative"        /* Bool  */
#define FC_EMBEDDED_BITMAP  "embeddedbitmap"    /* Bool  */
#define FC_EMBOLDEN         "embolden"          /* Bool */
#define FC_FAMILY           "family"            /* String */
#define FC_GLOBAL_ADVANCE   "globaladvance"     /* Bool (true) */
#define FC_HINTING          "hinting"           /* Bool (true) */
#define FC_MINSPACE         "minspace"          /* Bool */
#define FC_OUTLINE          "outline"           /* Bool */
#define FC_SCALABLE         "scalable"          /* Bool */
#define FC_SIZE             "size"              /* Double */
#define FC_SLANT            "slant"             /* Int */
#define FC_SPACING          "spacing"           /* Int */
#define FC_STYLE            "style"             /* String */
#define FC_VERTICAL_LAYOUT  "verticallayout"    /* Bool (false) */
#define FC_WEIGHT           "weight"            /* Int */
#define FC_WIDTH            "width"             /* Int */

#endif

#ifndef FC_WEIGHT_EXTRABLACK
#define FC_WEIGHT_EXTRABLACK        215
#define FC_WEIGHT_ULTRABLACK        FC_WEIGHT_EXTRABLACK
#endif

/* 
 * Extended Tk font container.
 */
typedef struct {
    const char *name;                   /* Name of the font. Points to the
                                         * hash table key. */
    int refCount;                       /* Reference count for this
                                         * structure.  When refCount
                                         * reaches zero, it means to free
                                         * the resources associated with
                                         * this structure. */
    Blt_HashEntry *hashPtr;             /* Pointer to this entry in the
                                         * global font hash table. Used to
                                         * remove the entry from the
                                         * table. */
    Blt_HashTable fontTable;            /* Hash table containing an Win32
                                         * font for each angle it's
                                         * used. */
    Tk_Font tkFont;                     /* The zero degree Tk font.  We use
                                         * it to get the Win32 font handle
                                         * to generate rotated fonts. */
} ExtFontset;

typedef struct {
    const char *family;
    const char *weight;
    const char *slant;
    const char *width;
    const char *spacing;
    int numPoints;                      
} FontPattern;

typedef struct {
    const char *name;
    int minChars;
    const char *key;
    int value;
    const char *oldvalue;
} FontSpec;
    
static FontSpec fontSpecs[] = {
    { "black",        2, FC_WEIGHT,  FC_WEIGHT_BLACK,     "*"},
    { "bold",         3, FC_WEIGHT,  FC_WEIGHT_BOLD,      "bold"},
    { "book",         3, FC_WEIGHT,  FC_WEIGHT_MEDIUM,    "medium"},
    { "charcell",     2, FC_SPACING, FC_CHARCELL,         "c"},
    { "condensed",    2, FC_WIDTH,   FC_WIDTH_CONDENSED,  "condensed"},
    { "demi",         4, FC_WEIGHT,  FC_WEIGHT_BOLD,      "semi"},
    { "demibold",     5, FC_WEIGHT,  FC_WEIGHT_DEMIBOLD,  "semibold"},
    { "dual",         2, FC_SPACING, FC_DUAL,             "*"},
    { "i",            1, FC_SLANT,   FC_SLANT_ITALIC,     "italic"},
    { "italic",       2, FC_SLANT,   FC_SLANT_ITALIC,     "italic"},
    { "light",        1, FC_WEIGHT,  FC_WEIGHT_LIGHT,     "light"},
    { "medium",       2, FC_WEIGHT,  FC_WEIGHT_MEDIUM,    "medium"},
    { "mono",         2, FC_SPACING, FC_MONO,             "m"},
    { "normal",       1, FC_WIDTH,   FC_WIDTH_NORMAL,     "normal"},
    { "o",            1, FC_SLANT,   FC_SLANT_OBLIQUE,    "oblique"},
    { "obilque",      2, FC_SLANT,   FC_SLANT_OBLIQUE,    "oblique"},
    { "overstrike",   2, NULL,       0,                   "*"},
    { "proportional", 1, FC_SPACING, FC_PROPORTIONAL,     "p"},
    { "r",            1, FC_SLANT,   FC_SLANT_ROMAN,      "roman"},
    { "roman",        2, FC_SLANT,   FC_SLANT_ROMAN,      "roman"},
    { "semibold",     5, FC_WEIGHT,  FC_WEIGHT_DEMIBOLD,  "semibold"},
    { "semicondensed",5, FC_WIDTH,   FC_WIDTH_SEMICONDENSED,  "semicondensed"},
    { "underline",    1, NULL,       0,                   "*"},
};
static int numFontSpecs = sizeof(fontSpecs) / sizeof(FontSpec);

static FontSpec weightSpecs[] ={
    { "black",          2, FC_WEIGHT, FC_WEIGHT_BLACK,      "bold"},
    { "bold",           3, FC_WEIGHT, FC_WEIGHT_BOLD,       "bold"},
    { "book",           3, FC_WEIGHT, FC_WEIGHT_MEDIUM,     "*"},
    { "demi",           4, FC_WEIGHT, FC_WEIGHT_BOLD,       "*"},
    { "demibold",       5, FC_WEIGHT, FC_WEIGHT_DEMIBOLD,   "*"},
    { "extrablack",     6, FC_WEIGHT, FC_WEIGHT_EXTRABLACK, "*"},
    { "extralight",     6, FC_WEIGHT, FC_WEIGHT_EXTRALIGHT, "*"},
    { "heavy",          1, FC_WEIGHT, FC_WEIGHT_HEAVY,      "*"},
    { "light",          1, FC_WEIGHT, FC_WEIGHT_LIGHT,      "light"},
    { "medium",         1, FC_WEIGHT, FC_WEIGHT_MEDIUM,     "medium"},
    { "normal",         1, FC_WEIGHT, FC_WEIGHT_MEDIUM,     "normal"},
    { "regular",        1, FC_WEIGHT, FC_WEIGHT_REGULAR,    "medium"},
    { "semibold",       1, FC_WEIGHT, FC_WEIGHT_SEMIBOLD,   "semibold"},
    { "thin",           1, FC_WEIGHT, FC_WEIGHT_THIN,       "thin"},
    { "ultrablack",     7, FC_WEIGHT, FC_WEIGHT_ULTRABLACK, "*"},
    { "ultrabold",      7, FC_WEIGHT, FC_WEIGHT_ULTRABOLD,  "*"},
    { "ultralight",     6, FC_WEIGHT, FC_WEIGHT_ULTRALIGHT, "*"},
};
static int numWeightSpecs = sizeof(weightSpecs) / sizeof(FontSpec);

static FontSpec slantSpecs[] ={
    { "i",              1, FC_SLANT, FC_SLANT_ITALIC,   "italic"},
    { "italic",         2, FC_SLANT, FC_SLANT_ITALIC,   "italic"},
    { "o",              1, FC_SLANT, FC_SLANT_OBLIQUE,  "oblique"},
    { "obilque",        3, FC_SLANT, FC_SLANT_OBLIQUE,  "oblique"},
    { "r",              1, FC_SLANT, FC_SLANT_ROMAN,    "roman"},
    { "roman",          2, FC_SLANT, FC_SLANT_ROMAN,    "roman"},
};
static int numSlantSpecs = sizeof(slantSpecs) / sizeof(FontSpec);

static FontSpec spacingSpecs[] = {
    { "charcell",     2, FC_SPACING, FC_CHARCELL,         "c"},
    { "dual",         2, FC_SPACING, FC_DUAL,             "*"},
    { "mono",         2, FC_SPACING, FC_MONO,             "m"},
    { "proportional", 1, FC_SPACING, FC_PROPORTIONAL,     "p"},
};
static int numSpacingSpecs = sizeof(spacingSpecs) / sizeof(FontSpec);

static FontSpec widthSpecs[] ={
    { "condensed",      1, FC_WIDTH, FC_WIDTH_CONDENSED,      "condensed"},
    { "expanded",       3, FC_WIDTH, FC_WIDTH_EXPANDED,       "*"},
    { "extracondensed", 6, FC_WIDTH, FC_WIDTH_EXTRACONDENSED, "*"},
    { "extraexpanded",  6, FC_WIDTH, FC_WIDTH_EXTRAEXPANDED,  "*"},
    { "narrow",         2, FC_WIDTH, FC_WIDTH_CONDENSED,      "narrow"},
    { "normal",         2, FC_WIDTH, FC_WIDTH_NORMAL,         "normal"},
    { "semicondensed",  5, FC_WIDTH, FC_WIDTH_SEMICONDENSED,  "semicondensed"},
    { "semiexpanded",   5, FC_WIDTH, FC_WIDTH_SEMIEXPANDED,   "*"},
    { "ultracondensed", 6, FC_WIDTH, FC_WIDTH_ULTRACONDENSED, "*"},
    { "ultraexpanded",  6, FC_WIDTH, FC_WIDTH_ULTRAEXPANDED,  "*"},
};
static int numWidthSpecs = sizeof(widthSpecs) / sizeof(FontSpec);

enum XLFDFields { 
    XLFD_FOUNDRY, 
    XLFD_FAMILY, 
    XLFD_WEIGHT, 
    XLFD_SLANT, 
    XLFD_SETWIDTH, 
    XLFD_ADD_STYLE, 
    XLFD_PIXEL_SIZE,
    XLFD_POINT_SIZE, 
    XLFD_RESOLUTION_X, 
    XLFD_RESOLUTION_Y,
    XLFD_SPACING, 
    XLFD_AVERAGE_WIDTH, 
    XLFD_CHARSET,
    XLFD_NUMFIELDS
};

typedef struct {
    const char *name, *aliases[10];
} FontAlias;

static FontAlias xlfdFontAliases[] = {
    { "math",           {"arial", "courier new"}},
    { "serif",          {"times"}},
    { "sans serif",     { "arial" }},
    { "monospace",      { "courier new" }},
    { NULL }
};

static int font_initialized = 0;
static Blt_HashTable aliasTable;
static Blt_HashTable fontSetTable;

static void GetFontFamilies(Tk_Window tkwin, Blt_HashTable *tablePtr);

static int
PointsToPixels(Display *display, int numPoints)
{
    double d;

    assert (numPoints > 0);
    d = numPoints * 25.4 / 72.0;
    d *= WidthOfScreen(DefaultScreenOfDisplay(display));
    d /= WidthMMOfScreen(DefaultScreenOfDisplay(display));
    return ROUND(d);
}

static int
PixelsToPoints(Display *display, int numPixels)               
{
    double d;

    assert (numPixels > 0);
    d = numPixels * 72.0 / 25.4;
    d *= WidthMMOfScreen(DefaultScreenOfDisplay(display));
    d /= WidthOfScreen(DefaultScreenOfDisplay(display));
    return ROUND(d);
}

/*
 *---------------------------------------------------------------------------
 *
 * SearchForFontSpec --
 *
 *      Performs a binary search on the array of font specification to find
 *      a partial, anchored match for the given option string.
 *
 * Results:
 *      If the string matches unambiguously the index of the specification
 *      in the array is returned.  If the string does not match, even as an
 *      abbreviation, any operation, -1 is returned.  If the string
 *      matches, but ambiguously -2 is returned.
 *
 *---------------------------------------------------------------------------
 */
static int
SearchForFontSpec(FontSpec *table, int numSpecs, const char *string, int length)
{
    char c;
    int high, low;

    low = 0;
    high = numSpecs - 1;
    c = tolower((unsigned char)string[0]);
    if (length < 0) {
        length = strlen(string);
    }
    while (low <= high) {
        FontSpec *sp;
        int compare;
        int median;
        
        median = (low + high) >> 1;
        sp = table + median;

        /* Test the first character */
        compare = c - sp->name[0];
        if (compare == 0) {
            /* Now test the entire string */
            compare = strncasecmp(string, sp->name, length);
            if (compare == 0) {
                if ((int)length < sp->minChars) {
                    return -2;          /* Ambiguous spec name */
                }
            }
        }
        if (compare < 0) {
            high = median - 1;
        } else if (compare > 0) {
            low = median + 1;
        } else {
            return median;              /* Spec found. */
        }
    }
    return -1;                          /* Can't find spec */
}

static FontSpec *
FindSpec(Tcl_Interp *interp, FontSpec *tablePtr, int numSpecs,
         const char *string, int length)
{
    int n;
    
    n = SearchForFontSpec(tablePtr, numSpecs, string, length);
    if (n < 0) {
        if (n == -1) {
            if (interp != NULL) {
                Tcl_AppendResult(interp, "unknown ", tablePtr[0].key,
                             " specification \"", string, "\"", (char *)NULL); 
            }
        }
        if (n == -2) {
            if (interp != NULL) {
                Tcl_AppendResult(interp, "ambiguous ", tablePtr[0].key,
                             " specification \"", string, "\"", (char *)NULL); 
            }
        }
        return NULL;
    }
    return tablePtr + n;
}

static void
MakeAliasTable(Tk_Window tkwin)
{
    Blt_HashTable familyTable;
    FontAlias *faPtr;
    FontAlias *table;

    Blt_InitHashTable(&familyTable, TCL_STRING_KEYS);
    GetFontFamilies(tkwin, &familyTable);
    Blt_InitHashTable(&aliasTable, TCL_STRING_KEYS);
    table = xlfdFontAliases;
    for(faPtr = table; faPtr->name != NULL; faPtr++) {
        Blt_HashEntry *hPtr;
        const char **alias;
           
        for (alias = faPtr->aliases; *alias != NULL; alias++) {
            hPtr = Blt_FindHashEntry(&familyTable, *alias);
            if (hPtr != NULL) {
                int isNew;
                
                hPtr = Blt_CreateHashEntry(&aliasTable, faPtr->name, &isNew);
                Blt_SetHashValue(hPtr, *alias);
                break;
            }
        }
    }
    Blt_DeleteHashTable(&familyTable);
}

static const char *
GetAlias(const char *family)
{
    Blt_HashEntry *hPtr;
    const char *lower;

    lower = Blt_AssertStrdup(family);
    Blt_LowerCase((char *)lower);
    hPtr = Blt_FindHashEntry(&aliasTable, lower);
    Blt_Free(lower);
    if (hPtr != NULL) {
        return Blt_GetHashValue(hPtr);
    }
    return family;
}

static FontPattern *
NewFontPattern(void)
{
    FontPattern *patternPtr;

    patternPtr = Blt_Calloc(1, sizeof(FontPattern));
    return patternPtr;
}

static void
FreeFontPattern(FontPattern *patternPtr)
{
    if (patternPtr->family != NULL) {
        Blt_Free(patternPtr->family);
    }
    Blt_Free(patternPtr);
}

static int CALLBACK
FontFamilyEnumProc(
    ENUMLOGFONT *lfPtr,                 /* Logical-font data. */
    NEWTEXTMETRIC *tmPtr,               /* Physical-font data (not used). */
    int fontType,                       /* Type of font (not used). */
    LPARAM lParam)                      /* Result object to hold result. */
{
    Blt_HashEntry *hPtr;
    Blt_HashTable *tablePtr;
    Tcl_DString ds;
    const char *faceName;
    Tcl_Encoding encoding;
    int isNew;

    tablePtr = (Blt_HashTable *)lParam;
    faceName = lfPtr->elfLogFont.lfFaceName;
    encoding = Tcl_GetEncoding(NULL, "Unicode");
    Tcl_ExternalToUtfDString(encoding, faceName, -1, &ds);
    faceName = Tcl_DStringValue(&ds);
    Blt_LowerCase((char *)faceName);
    hPtr = Blt_CreateHashEntry(tablePtr, faceName, &isNew);
    Blt_SetHashValue(hPtr, NULL);
    Tcl_DStringFree(&ds);
    return 1;
}

static void
GetFontFamilies(Tk_Window tkwin, Blt_HashTable *tablePtr)
{    
    HDC hDC;
    HWND hWnd;
    Window window;

    window = Tk_WindowId(tkwin);
    hWnd = (window == None) ? (HWND)NULL : Tk_GetHWND(window);
    hDC = GetDC(hWnd);

    /*
     * On any version NT, there may fonts with international names.  Use
     * the NT-only Unicode version of EnumFontFamilies to get the font
     * names.  If we used the ANSI version on a non-internationalized
     * version of NT, we would get font names with '?' replacing all the
     * international characters.
     *
     * On a non-internationalized verson of 95, fonts with international
     * names are not allowed, so the ANSI version of EnumFontFamilies will
     * work.  On an internationalized version of 95, there may be fonts
     * with international names; the ANSI version will work, fetching the
     * name in the system code page.  Can't use the Unicode version of
     * EnumFontFamilies because it only exists under NT.
     */

    if (Blt_GetPlatformId() == VER_PLATFORM_WIN32_NT) {
        EnumFontFamiliesW(hDC, NULL, (FONTENUMPROCW)FontFamilyEnumProc,
                (LPARAM)tablePtr);
    } else {
        EnumFontFamiliesA(hDC, NULL, (FONTENUMPROCA)FontFamilyEnumProc,
                (LPARAM)tablePtr);
    }       
    ReleaseDC(hWnd, hDC);
}

static void
SplitXLFD(Tcl_Obj *objPtr, int *argcPtr, char ***argvPtr)
{
    char *p, *pend, *desc, *buf;
    size_t arrayLen;
    int count;
    char **field;
    const char *fontName;
    int stringLen;

    fontName = Tcl_GetStringFromObj(objPtr, &stringLen);
    if (fontName[0] == '-') {
        fontName++;
        stringLen--;
    }
    arrayLen = (sizeof(char *) * (XLFD_NUMFIELDS + 1));
    buf = Blt_AssertCalloc(1, arrayLen + stringLen + 1);
    desc = buf + arrayLen;
    strcpy(desc, fontName);
    field = (char **)buf;

    count = 0;
    for (p = desc, pend = p + stringLen; p < pend; p++, count++) {
        char *word;

        field[count] = NULL;
        /* Get the next word, separated by dashes (-). */
        word = p;
        while ((*p != '\0') && (*p != '-')) {
            if (((*p & 0x80) == 0) && Tcl_UniCharIsUpper(UCHAR(*p))) {
                *p = (char)Tcl_UniCharToLower(UCHAR(*p));
            }
            p++;
        }
        if (*p != '\0') {
            *p = '\0';
        }
        if ((word[0] == '\0') || 
            (((word[0] == '*') || (word[0] == '?')) && (word[1] == '\0'))) {
            continue;                   /* Field not specified. -- -*- -?- */
        }
        field[count] = word;
    }

    /*
     * An XLFD of the form -adobe-times-medium-r-*-12-*-* is pretty common,
     * but it is (strictly) malformed, because the first is eliding both
     * the Setwidth and the Addstyle fields. If the Addstyle field is a
     * number, then assume the above incorrect form was used and shift all
     * the rest of the fields right by one, so the number gets interpreted
     * as a pixelsize.  This fix is so that we don't get a million reports
     * that "it works under X (as a native font name), but gives a syntax
     * error under Windows (as a parsed set of attributes)".
     */

    if ((count > XLFD_ADD_STYLE) && (field[XLFD_ADD_STYLE] != NULL)) {
        int dummy;

        if (Tcl_GetInt(NULL, field[XLFD_ADD_STYLE], &dummy) == TCL_OK) {
            int j;
            
            for (j = XLFD_NUMFIELDS - 1; j >= XLFD_ADD_STYLE; j--) {
                field[j + 1] = field[j];
            }
            field[XLFD_ADD_STYLE] = NULL;
            count++;
        }
    }
    *argcPtr = count;
    *argvPtr = field;

    field[XLFD_NUMFIELDS] = NULL;
}

static FontPattern *
ParseXLFDDesc(Tcl_Interp *interp, Tk_Window tkwin, Tcl_Obj *objPtr)
{
    FontPattern *patternPtr;
    FontSpec *specPtr;
    int argc;
    char **argv;
    int numPoints;
    
    SplitXLFD(objPtr, &argc, &argv);
    patternPtr = NewFontPattern();
    if (argv[XLFD_FAMILY] != NULL) {
        patternPtr->family = Blt_AssertStrdup(argv[XLFD_FAMILY]);
    }
    if (argv[XLFD_WEIGHT] != NULL) {
        specPtr = FindSpec(interp, weightSpecs, numWeightSpecs, 
                           argv[XLFD_WEIGHT], -1);
        if (specPtr == NULL) {
            goto error;
        }
        patternPtr->weight = specPtr->oldvalue;
    }
    if (argv[XLFD_SLANT] != NULL) {
        specPtr = FindSpec(interp, slantSpecs, numSlantSpecs, argv[XLFD_SLANT], 
                -1);
        if (specPtr == NULL) {
            goto error;
        }
        patternPtr->slant = specPtr->oldvalue;
    }
    if (argv[XLFD_SETWIDTH] != NULL) {
        specPtr = FindSpec(interp, widthSpecs, numWidthSpecs, 
                           argv[XLFD_SETWIDTH], -1);
        if (specPtr == NULL) {
            goto error;
        }
        patternPtr->width = specPtr->oldvalue;
    }
    numPoints = 12;
    if (argv[XLFD_PIXEL_SIZE] != NULL) {
        int value;
        if (argv[XLFD_PIXEL_SIZE][0] == '[') {
            /*
             * Some X fonts have the point size specified as follows:
             *
             *      [ N1 N2 N3 N4 ]
             *
             * where N1 is the point size (in points, not decipoints!), and
             * N2, N3, and N4 are some additional numbers that I don't know
             * the purpose of, so I ignore them.
             */
            value = atoi(argv[XLFD_PIXEL_SIZE]+1);
        } else if (Tcl_GetInt(NULL, argv[XLFD_PIXEL_SIZE], &value) == TCL_OK) {
            /* empty */
        } else {
            goto error;
        }
        numPoints = PixelsToPoints(Tk_Display(tkwin), value);
    }
    patternPtr->numPoints = numPoints;

    if (argv[XLFD_SPACING] != NULL) {
        specPtr = FindSpec(interp, spacingSpecs, numSpacingSpecs, 
                           argv[XLFD_SPACING], -1);
        if (specPtr == NULL) {
            goto error;
        }
        patternPtr->spacing = specPtr->oldvalue;
    }
    if (argv[XLFD_SETWIDTH] != NULL) {
        specPtr = FindSpec(interp, widthSpecs, numWidthSpecs, 
                           argv[XLFD_SETWIDTH], -1);
        if (specPtr == NULL) {
            goto error;
        }
        patternPtr->width = specPtr->oldvalue;
    }
    Blt_Free((char *)argv);
#if DEBUG_FONT_SELECTION
    fprintf(stderr, "parsed XLFD font \"%s\"\n", fontName);
#endif
    return patternPtr;
 error:
#if DEBUG_FONT_SELECTION
    fprintf(stderr, "can't open font \"%s\" as XLFD\n", fontName);
#endif
    Blt_Free((char *)argv);
    FreeFontPattern(patternPtr);
    return NULL;
}

/*
 *---------------------------------------------------------------------------
 *
 * ParseTkDesc --
 *
 *      Parses an array of Tcl_Objs as a Tk style font description .  
 *      
 *            "family [size] [optionList]"
 *
 * Results:
 *      Returns a pattern structure, filling in with the necessary fields.
 *      Returns NULL if objv doesn't contain a  Tk font description.
 *
 * Side effects:
 *      Memory is allocated for the font pattern and the its strings.
 *
 *---------------------------------------------------------------------------
 */
static FontPattern *
ParseTkDesc(Tcl_Interp *interp, Tk_Window tkwin, Tcl_Obj *objPtr)
{
    FontPattern *patternPtr;
    Tcl_Obj **objv, **aobjv;
    int objc, aobjc;
    int i;

    if (Tcl_ListObjGetElements(interp, objPtr, &objc, &objv) != TCL_OK) {
        return NULL;
    }
    patternPtr = NewFontPattern();
    /* Font family. */
    {
        char *family, *dash;

        family = Tcl_GetString(objv[0]);
        dash = strchr(family, '-');
        if (dash != NULL) {
            int size;
            
            if (Tcl_GetInt(NULL, dash + 1, &size) != TCL_OK) {
                goto error;
            }
            if (size < 0) {
                patternPtr->numPoints = PixelsToPoints(Tk_Display(tkwin),-size);
            } else {
                patternPtr->numPoints = size;
            }
        }
        if (dash != NULL) {
            *dash = '\0';
        }
        patternPtr->family = Blt_AssertStrdup(GetAlias(family));
        if (dash != NULL) {
            *dash = '-';
        }
        objv++, objc--;
    }
    if (objc > 0) {
        int size;

        if (Tcl_GetIntFromObj(NULL, objv[0], &size) == TCL_OK) {
            if (size < 0) {
                patternPtr->numPoints = PixelsToPoints(Tk_Display(tkwin),-size);
            } else {
                patternPtr->numPoints = size;
            }
            objv++, objc--;
        }
    }
    aobjc = objc, aobjv = objv;
    if (objc > 0) {
        if (Tcl_ListObjGetElements(NULL, objv[0], &aobjc, &aobjv) != TCL_OK) {
            goto error;
        }
    }
    for (i = 0; i < aobjc; i++) {
        FontSpec *specPtr;
        const char *key;
        int length;

        key = Tcl_GetStringFromObj(aobjv[i], &length);
        specPtr = FindSpec(interp, fontSpecs, numFontSpecs, key, length);
        if (specPtr == NULL) {
            goto error;
        }
        if (specPtr->key == NULL) {
            continue;
        }
        if (strcmp(specPtr->key, FC_WEIGHT) == 0) {
            patternPtr->weight = specPtr->oldvalue;
        } else if (strcmp(specPtr->key, FC_SLANT) == 0) {
            patternPtr->slant = specPtr->oldvalue;
        } else if (strcmp(specPtr->key, FC_SPACING) == 0) {
            patternPtr->spacing = specPtr->oldvalue;
        } else if (strcmp(specPtr->key, FC_WIDTH) == 0) {
            patternPtr->width = specPtr->oldvalue;
        }
    }
    return patternPtr;
 error:
    FreeFontPattern(patternPtr);
    return NULL;
}       

/*
 *---------------------------------------------------------------------------
 *
 * ParseNameValuePairs --
 *
 *      Given Tcl_Obj list of name value pairs, parse the list saving in the
 *      values in a font pattern structure.
 *      
 *            "-family family -size size -weight weight"
 *
 * Results:
 *      Returns a pattern structure, filling in with the necessary fields.
 *      Returns NULL if objv doesn't contain a valid name-value list 
 *      describing a font.
 *
 * Side effects:
 *      Memory is allocated for the font pattern and the its strings.
 *
 *---------------------------------------------------------------------------
 */
static FontPattern *
ParseNameValuePairs(Tcl_Interp *interp, Tk_Window tkwin, Tcl_Obj *objPtr) 
{
    FontPattern *patternPtr;
    Tcl_Obj **objv;
    int objc;
    int i;

    if ((Tcl_ListObjGetElements(NULL, objPtr, &objc, &objv) != TCL_OK) ||
        (objc < 1)) {
        return NULL;                    /* Can't split list or list is
                                         * empty. */
    }
    if (objc & 0x1) {
        if (interp != NULL) {
            Tcl_AppendResult(interp, "odd number of elements, missing value",
                         (char *)NULL);
        }
        return NULL;                    /* Odd number of elements in list. */
    }
    patternPtr = NewFontPattern();
    for (i = 0; i < objc; i += 2) {
        const char *key, *value;
        int length;

        key = Tcl_GetString(objv[i]);
        value = Tcl_GetStringFromObj(objv[i+1], &length);
        if (strcmp(key, "-family") == 0) {
            if (patternPtr->family != NULL) {
                Blt_Free(patternPtr->family);
            }
            patternPtr->family = Blt_AssertStrdup(GetAlias(value));
        } else if (strcmp(key, "-size") == 0) {
            int size;

            if (Tcl_GetIntFromObj(interp, objv[i+1], &size) != TCL_OK) {
                goto error;
            }
            if (size < 0) {
                patternPtr->numPoints = PixelsToPoints(Tk_Display(tkwin),-size);
            } else {
                patternPtr->numPoints = size;
            }
        } else if (strcmp(key, "-weight") == 0) {
            FontSpec *specPtr;

            specPtr = FindSpec(interp, weightSpecs, numWeightSpecs, value, 
                length);
            if (specPtr == NULL) {
                goto error;
            }
            patternPtr->weight = specPtr->oldvalue;
        } else if (strcmp(key, "-slant") == 0) {
            FontSpec *specPtr;

            specPtr = FindSpec(interp, slantSpecs, numSlantSpecs, value,length);
            if (specPtr == NULL) {
                goto error;
            }
            patternPtr->slant = specPtr->oldvalue;
        } else if (strcmp(key, "-spacing") == 0) {
            FontSpec *specPtr;

            specPtr = FindSpec(interp, spacingSpecs, numSpacingSpecs, value, 
                length);
            if (specPtr == NULL) {
                goto error;
            }
            patternPtr->spacing = specPtr->oldvalue;
        } else if (strcmp(key, "-hint") == 0) {
            /* Ignore */
        } else if (strcmp(key, "-rgba") == 0) {
            /* Ignore */
        } else if (strcmp(key, "-underline") == 0) {
            /* Ignore */
        } else if (strcmp(key, "-overstrike") == 0) {
            /* Ignore */
        } else {
            /* Ignore */
        }
    }
#if DEBUG_FONT_SELECTION
    fprintf(stderr, "found TkAttrList => Tk font \"%s\"\n", 
            Tcl_GetString(objPtr));
#endif
    return patternPtr;
 error:
    FreeFontPattern(patternPtr);
    return NULL;
}

/*
 *---------------------------------------------------------------------------
 *
 * ParseFontObj --
 *
 *      Given the name of a Tk font object, get its configuration values 
 *      save the data in a font pattern structure.
 *      
 *            "-family family -size size -weight weight"
 *
 * Results:
 *      Returns a pattern structure, filling in with the necessary fields.
 *      Returns NULL if objv doesn't contain a valid name-value list 
 *      describing a font.
 *
 * Side effects:
 *      Memory is allocated for the font pattern and the its strings.
 *
 *---------------------------------------------------------------------------
 */
static FontPattern *
ParseFontObj(Tcl_Interp *interp, Tk_Window tkwin, Tcl_Obj *objPtr) 
{
    FontPattern *patternPtr;
    Tcl_Obj *cmd[3];
    int result;

    patternPtr = NULL;
    cmd[0] = Tcl_NewStringObj("font", -1);
    cmd[1] = Tcl_NewStringObj("configure", -1);
    cmd[2] = objPtr;
    Tcl_IncrRefCount(cmd[0]);
    Tcl_IncrRefCount(cmd[1]);
    Tcl_IncrRefCount(cmd[2]);
    result = Tcl_EvalObjv(interp, 3, cmd, 0);
    Tcl_DecrRefCount(cmd[2]);
    Tcl_DecrRefCount(cmd[1]);
    Tcl_DecrRefCount(cmd[0]);
    if (result == TCL_OK) {
        patternPtr = ParseNameValuePairs(interp, tkwin, 
                                         Tcl_GetObjResult(interp));
    }
    Tcl_ResetResult(interp);
#if DEBUG_FONT_SELECTION
    if (patternPtr != NULL) {
        fprintf(stderr, "found FontObject => Tk font \"%s\"\n", 
            Tcl_GetString(objPtr));
    }
#endif
    return patternPtr;
}

/* 
 *---------------------------------------------------------------------------
 *
 * GetFontPattern --
 * 
 *      Parses the font description so that the font can rewritten with an
 *      aliased font name.  This allows us to use
 *
 *        "Sans Serif", "Serif", "Math", "Monospace"
 *
 *      font names that correspond to the proper font regardless if the
 *      standard X fonts or XFT fonts are being used.
 *
 *      Leave XLFD font descriptions alone.  Let users describe exactly the
 *      font they wish.
 *
 *---------------------------------------------------------------------------
 */
static FontPattern *
GetFontPattern(Tcl_Interp *interp, Tk_Window tkwin, Tcl_Obj *objPtr)
{
    FontPattern *patternPtr;
    const char *desc;

    desc = Tcl_GetString(objPtr);
    while (isspace(UCHAR(*desc))) {
        desc++;                         /* Skip leading blanks. */
    }
    if (*desc == '-') {
        /* 
         * Case 1: XLFD font description or Tk attribute list.   
         *
         *   If the font description starts with a '-', it could be either an
         *   old fashion XLFD font description or a Tk font attribute
         *   option-value list.
         */
        patternPtr = ParseNameValuePairs(interp, tkwin, objPtr);
        if (patternPtr == NULL) {
            patternPtr = ParseXLFDDesc(interp, tkwin, objPtr);
        }
    } else if (*desc == '*') {
        patternPtr = ParseXLFDDesc(interp, tkwin, objPtr);
    } else if (strpbrk(desc, "::") != NULL) {
        patternPtr = ParseFontObj(interp, tkwin, objPtr);
    } else {
        int numElems;
        /* 
         * Case 3: Tk-style description.   
         */
        if ((Tcl_ListObjLength(NULL, objPtr, &numElems) != TCL_OK) || 
            (numElems < 1)) {
            return NULL;                /* Can't split into a list or
                                         * list is empty. */
        }
        patternPtr = NULL;
        if (numElems == 1) {
            /* 
             * Case 3a: Tk font object name.
             *
             *   Assuming that Tk font object names won't contain whitespace,
             *   see if its a font object.
             */
            patternPtr = ParseFontObj(interp, tkwin, objPtr);
        } 
        if (patternPtr == NULL) {
            /* 
             * Case 3b: List of font attributes in the form "family size
             *          ?attrs?"
             */
            patternPtr = ParseTkDesc(interp, tkwin, objPtr);
        }
    }   
    return patternPtr;
}

static FontPattern *
GetPatternFromFont(Display *display, Tk_Font tkFont)
{
    FontPattern *patternPtr;
    TkFont *tkFontPtr = (TkFont *)tkFont;

    patternPtr = NewFontPattern();
    patternPtr->family = Blt_AssertStrdup(tkFontPtr->fa.family);
    patternPtr->slant = (tkFontPtr->fa.slant) ? "italic" : "roman";
    patternPtr->weight = (tkFontPtr->fa.weight == FW_BOLD) ? "bold" : "normal";
    if (tkFontPtr->fa.size < 0) {
        patternPtr->numPoints = PixelsToPoints(display, -tkFontPtr->fa.size);
    } else {
        patternPtr->numPoints = tkFontPtr->fa.size;
    }
    patternPtr->spacing = "*";
    return patternPtr;
}

static void
FontPatternToDString(Tk_Window tkwin, FontPattern *patternPtr, 
                     Tcl_DString *resultPtr)
{
    Tcl_DStringInit(resultPtr);
    /* Family */
    if (patternPtr->family != NULL) {
        Tcl_DStringAppendElement(resultPtr, "-family");
        Tcl_DStringAppendElement(resultPtr, patternPtr->family);
    }
    /* Weight */
    if (patternPtr->weight != NULL) {
        Tcl_DStringAppendElement(resultPtr, "-weight");
        Tcl_DStringAppendElement(resultPtr, patternPtr->weight);
    }
    /* Slant */
    if (patternPtr->slant != NULL) {
        Tcl_DStringAppendElement(resultPtr, "-slant");
        Tcl_DStringAppendElement(resultPtr, patternPtr->slant);
    }
    /* Size */
    Tcl_DStringAppendElement(resultPtr, "-size");
    Tcl_DStringAppendElement(resultPtr, Blt_Itoa(patternPtr->numPoints));
}

/* 
 * GetFontFromPattern --
 *
 *      Tries to open a font based upon the given font pattern.  The
 *      pattern is converted into a string in canonical format, and then we
 *      use the normal Tk routines to open it.  
 *
 *  Results:
 *      If successful, returns a Tk_Font, otherwise NULL;
 *
 */
static Tk_Font 
GetFontFromPattern(Tcl_Interp *interp, Tk_Window tkwin, FontPattern *patternPtr)
{
    Tk_Font tkFont;
    Tcl_DString ds;

    /* Rewrite the font description using the aliased family. */
    FontPatternToDString(tkwin, patternPtr, &ds);
    tkFont = Tk_GetFont(interp, tkwin, Tcl_DStringValue(&ds));
    Tcl_DStringFree(&ds);
    return tkFont;
}

/* 
 * NewExtFontset --
 *
 *      Allocates and fills a new fontset structure. The name of the
 *      fontset is detemined from the hash entry pointer passed in.
 *
 *  Results:
 *      Returns a pointer to the newly allocated ExtFontset structure.
 *
 */
static ExtFontset *
NewExtFontset(Tk_Font tkFont, Blt_HashEntry *hPtr)
{
    ExtFontset *setPtr;

    setPtr = Blt_AssertCalloc(1, sizeof(ExtFontset));
    setPtr->refCount = 1;
    setPtr->tkFont = tkFont;
    setPtr->name = Blt_GetHashKey(&fontSetTable, hPtr);
    setPtr->hashPtr = hPtr;
    Blt_InitHashTable(&setPtr->fontTable, BLT_ONE_WORD_KEYS);
    Blt_SetHashValue(hPtr, setPtr);
    return setPtr;
}

/* 
 * GetFontsetFromObj --
 *
 *      Tries to allocate a new fontset. The name of the fontset is
 *      standardized and then used to open the font. If the font already
 *      exists, a pointer to the existing fontset is returned. Otherwise a
 *      new fontset is allocated.
 *
 *  Results:
 *      Returns a pointer to the ExtFontset structure or NULL if an 
 *      error occurred.
 *
 */
static ExtFontset *
GetFontsetFromObj(Tcl_Interp *interp, Tk_Window tkwin, Tcl_Obj *objPtr)
{
    FontPattern *patternPtr;
    ExtFontset *setPtr;
    const char *fontName;
    int isNew;
    Tcl_DString ds;
    Blt_HashEntry *hPtr;

    if (!font_initialized) {
        Blt_InitHashTable(&fontSetTable, BLT_STRING_KEYS);
        MakeAliasTable(tkwin);
        font_initialized++;
    }
    patternPtr = GetFontPattern(interp, tkwin, objPtr);
    if (patternPtr == NULL) {
        Tcl_AppendResult(interp, "can't parse font description \"", 
                         Tcl_GetString(objPtr), "\"", (char *)NULL);
        return NULL;
    }
    /* This re-generates the font description in a canonical format. */
    FontPatternToDString(tkwin, patternPtr, &ds);
    fontName = Tcl_DStringValue(&ds);

    /* See if we already have this fontset. */
    hPtr = Blt_CreateHashEntry(&fontSetTable, (char *)fontName, &isNew);
    Tcl_DStringFree(&ds);
    if (isNew) {
        Tk_Font tkFont;

        tkFont = GetFontFromPattern(interp, tkwin, patternPtr);
        if (patternPtr != NULL) {
            FreeFontPattern(patternPtr);
        }
        if (tkFont == NULL) {
            Blt_DeleteHashEntry(&fontSetTable, hPtr);
            return NULL;
        }
        /* Attach it to the fontset as the base font. */
        setPtr = NewExtFontset(tkFont, hPtr);
    } else {
        setPtr = Tcl_GetHashValue(hPtr);
	setPtr->refCount++;
    }
    return setPtr;
}

/*
 *---------------------------------------------------------------------------
 *
 * MakeRotatedFont --
 *
 *      Creates a rotated copy of the given font.  This only works for
 *      TrueType fonts.
 *
 * Results:
 *      Returns the newly create font or NULL if the font could not be
 *      created.
 *
 *---------------------------------------------------------------------------
 */
static HFONT
MakeRotatedFont(
    Tk_Font tkFont,                     /* Font identifier (actually a
                                         * Tk_Font) */
    long angle10)                       /* # of degrees to rotate font */
{                                       
    TkFontAttributes *faPtr;            /* Set of attributes to match. */
    HFONT hFont;
    LOGFONTW lf;
    TkFont *tkFontPtr = (TkFont *)tkFont;

    faPtr = &tkFontPtr->fa;
    ZeroMemory(&lf, sizeof(LOGFONT));
    lf.lfHeight = -faPtr->size;
    if (lf.lfHeight < 0) {
        HDC hDC;
        double numPixels;

        hDC = GetDC(NULL);
        /* Convert from points to integral (rounded) # of pixels. */
        numPixels = faPtr->size * GetDeviceCaps(hDC, LOGPIXELSY) / 72.0;
        lf.lfHeight = -((LONG)(numPixels + 0.5)); 
        ReleaseDC(NULL, hDC);
    }
    lf.lfWidth = 0;
    lf.lfEscapement = lf.lfOrientation = angle10;
#define TK_FW_NORMAL    0
    lf.lfWeight = (faPtr->weight == TK_FW_NORMAL) ? FW_NORMAL : FW_BOLD;
    lf.lfItalic = faPtr->slant;
    lf.lfUnderline = faPtr->underline;
    lf.lfStrikeOut = faPtr->overstrike;
    lf.lfCharSet = DEFAULT_CHARSET;
    lf.lfOutPrecision = OUT_TT_ONLY_PRECIS;
    lf.lfClipPrecision = CLIP_DEFAULT_PRECIS;
    lf.lfQuality = DEFAULT_QUALITY;
    lf.lfQuality = ANTIALIASED_QUALITY;
    lf.lfPitchAndFamily = DEFAULT_PITCH | FF_DONTCARE;

    hFont = NULL;
    if (faPtr->family == NULL) {
        lf.lfFaceName[0] = '\0';
    } else {
#if (_TCL_VERSION >= _VERSION(8,1,0)) 
        Tcl_DString ds;
        Tcl_Encoding encoding;

        encoding = Tcl_GetEncoding(NULL, "unicode");
        Tcl_UtfToExternalDString(encoding, faPtr->family, -1, &ds);
        if (Blt_GetPlatformId() == VER_PLATFORM_WIN32_NT) {
            Tcl_UniChar *src, *dst;
            
            /*
             * We can only store up to LF_FACESIZE wide characters
             */
            if (Tcl_DStringLength(&ds) >= (LF_FACESIZE * sizeof(WCHAR))) {
                Tcl_DStringSetLength(&ds, LF_FACESIZE);
            }
            src = (Tcl_UniChar *)Tcl_DStringValue(&ds);
            dst = (Tcl_UniChar *)lf.lfFaceName;
            while (*src != '\0') {
                *dst++ = *src++;
            }
            *dst = '\0';
            hFont = CreateFontIndirectW((LOGFONTW *)&lf);
        } else {
            /*
             * We can only store up to LF_FACESIZE characters
             */
            if (Tcl_DStringLength(&ds) >= LF_FACESIZE) {
                Tcl_DStringSetLength(&ds, LF_FACESIZE);
            }
            strcpy((char *)lf.lfFaceName, Tcl_DStringValue(&ds));
            hFont = CreateFontIndirectA((LOGFONTA *)&lf);
        }
        Tcl_DStringFree(&ds);
#else
        strncpy((char *)lf.lfFaceName, faPtr->family, LF_FACESIZE - 1);
        lf.lfFaceName[LF_FACESIZE] = '\0';
#endif /* _TCL_VERSION >= 8.1.0 */
    }

    if (hFont != NULL) {
        HFONT oldFont;
        TEXTMETRIC tm;
        HDC hDC;
        int result;

        /* Check if the rotated font is really a TrueType font. */

        hDC = GetDC(NULL);              /* Get the desktop device context */
        oldFont = SelectFont(hDC, hFont);
        result = ((GetTextMetrics(hDC, &tm)) && 
                  (tm.tmPitchAndFamily & TMPF_TRUETYPE));
        (void)SelectFont(hDC, oldFont);
        ReleaseDC(NULL, hDC);
        if (!result) {
            DeleteFont(hFont);
            return NULL;
        }
    }
    return hFont;
}

static void
DestroyExtFontset(ExtFontset *setPtr)
{
    Blt_HashEntry *hPtr;
    Blt_HashSearch cursor;
    
    for (hPtr = Blt_FirstHashEntry(&setPtr->fontTable, &cursor); 
         hPtr != NULL; hPtr = Blt_NextHashEntry(&cursor)) {
        HFONT hFont;
        
        hFont = Blt_GetHashValue(hPtr);
        DeleteFont(hFont);
    }
    Tk_FreeFont(setPtr->tkFont);
    Blt_DeleteHashTable(&setPtr->fontTable);
    Blt_DeleteHashEntry(&fontSetTable, setPtr->hashPtr);
    Blt_Free(setPtr);
}

static int
GetFile(Tcl_Interp *interp, const char *fontName, Tcl_DString *namePtr, 
        Tcl_DString *valuePtr)
{
    HKEY hkey;
    char *name;
    char *value;
    const char *fileName;
    const char *fontSubKey;
    unsigned long maxBytesKey, maxBytesName, maxBytesValue;
    unsigned long numSubKeys, numValues;
    int result, length;
    int i;

    if (Blt_GetPlatformId() == VER_PLATFORM_WIN32_NT) {
        fontSubKey = "Software\\Microsoft\\Windows NT\\CurrentVersion\\Fonts";
    } else {
        fontSubKey = "Software\\Microsoft\\Windows\\CurrentVersion\\Fonts";
    }
    length = strlen(fontName);
    fileName = NULL;
    /* Open the registry key. */
    result = RegOpenKeyEx(
        HKEY_LOCAL_MACHINE,             /* Predefined key. */
        fontSubKey,                     /* Registry subkey. */
        0,                              /* Reserved. Must be zero. */
        KEY_READ,                       /* Access rights. */
        &hkey);                         /* (out) Handle to resulting key. */
    if (result != ERROR_SUCCESS) {
        Tcl_AppendResult(interp, "can't open font registry: ", 
                Blt_LastError(), (char *)NULL);
        return TCL_ERROR;
    }
    /* Get the number of values. */
    result = RegQueryInfoKey(
        hkey,                           /* key handle */
        NULL,                           /* Buffer for class name */
        NULL,                           /* Size of class string. */
        NULL,                           /* Reserved. Must be NULL. */ 
        &numSubKeys,                    /* # of subkeys. */
        &maxBytesKey,                   /* Longest subkey size. */
        NULL,                           /* Longest class string. */
        &numValues,                     /* # of values for this key. */
        &maxBytesName,                  /* Longest value name  */
        &maxBytesValue,                 /* Longest value data */
        NULL,                           /* Security descriptor. */
        NULL);                          /* Last write time. */
    if (result != ERROR_SUCCESS) {
        Tcl_AppendResult(interp, "can't query registry info: ", 
                Blt_LastError(), (char *)NULL);
        goto error;
    }
    Tcl_DStringSetLength(namePtr, maxBytesName);
    name = Tcl_DStringValue(namePtr);
    Tcl_DStringSetLength(valuePtr, maxBytesValue);
    value = Tcl_DStringValue(valuePtr);
    /* Look for the font value. */
    for (i = 0; i < numValues; i++) {
        unsigned long numBytesName, numBytesValue;
        unsigned long regType;

        numBytesName = maxBytesName;    
        numBytesValue = maxBytesValue;  
        result = RegEnumValue(
                hkey, 
                i, 
                name, 
                &numBytesName, 
                NULL,                   /* Reserved. Must be NULL. */
                &regType,               /* (out) Registry value type  */
                (unsigned char *)value, 
                &numBytesValue);
        if (result != ERROR_SUCCESS) {
            Tcl_AppendResult(interp, "can't retrieve registry value: ", 
                Blt_LastError(), (char *)NULL);
            goto error;
        }
        /* Perform anchored case-insensitive string comparison. */
        if (strncasecmp(name, fontName, length) == 0) {
            fileName = value;
            break;
        }
    }
    if (fileName == NULL) {
        Tcl_AppendResult(interp, "can't find font \"", fontName, "\": ",
                Blt_LastError(), (char *)NULL);
    }
 error:
    RegCloseKey(hkey);
    if (fileName == NULL) {
        return TCL_ERROR;
    }
    return TCL_OK;
}


/* 
 * ExtFontset --
 *
 *      Set of routines specific to the ExtFontset.  Many of them are
 *      simply wrappers for Tk font routines. The differences are the
 *      CanRotateProc, DuplicateProc, DrawProc (for rotated fonts).
 *
 */

static Blt_Font_CanRotateProc           ExtFontCanRotateProc;
static Blt_Font_DrawProc                ExtFontDrawProc;
static Blt_Font_DuplicateProc           ExtFontDupProc;
static Blt_Font_FamilyProc              ExtFontFamilyProc;
static Blt_Font_FreeProc                ExtFontFreeProc;
static Blt_Font_GetMetricsProc          ExtFontGetMetricsProc;
static Blt_Font_IdProc                  ExtFontIdProc;
static Blt_Font_MeasureProc             ExtFontMeasureProc;
static Blt_Font_NameProc                ExtFontNameProc;
static Blt_Font_PixelSizeProc           ExtFontPixelSizeProc;
static Blt_Font_PointSizeProc           ExtFontPointSizeProc;
static Blt_Font_PostscriptNameProc      ExtFontPostscriptNameProc;
static Blt_Font_TextWidthProc           ExtFontTextWidthProc;
static Blt_Font_UnderlineCharsProc      ExtFontUnderlineCharsProc;

static Blt_FontClass extFontClass = {
    FONTSET_EXT,
    "extfont",
    ExtFontCanRotateProc,               /* Blt_Font_CanRotateProc */
    ExtFontDrawProc,                    /* Blt_Font_DrawProc */
    ExtFontDupProc,                     /* Blt_Font_DuplicateProc */
    ExtFontFamilyProc,                  /* Blt_Font_FamilyProc */
    ExtFontFreeProc,                    /* Blt_Font_FreeProc */
    ExtFontGetMetricsProc,              /* Blt_Font_GetMetricsProc */
    ExtFontIdProc,                      /* Blt_Font_IdProc */
    ExtFontMeasureProc,                 /* Blt_Font_MeasureProc */
    ExtFontNameProc,                    /* Blt_Font_NameProc */
    ExtFontPixelSizeProc,               /* Blt_Font_PixelSizeProc */
    ExtFontPointSizeProc,               /* Blt_Font_PointSizeProc */
    ExtFontPostscriptNameProc,          /* Blt_Font_PostscriptNameProc */
    ExtFontTextWidthProc,               /* Blt_Font_TextWidthProc */
    ExtFontUnderlineCharsProc,          /* Blt_Font_UnderlineCharsProc */
};

static const char *
ExtFontNameProc(_Blt_Font *fontPtr) 
{
    ExtFontset *setPtr = fontPtr->clientData;

    return Tk_NameOfFont(setPtr->tkFont);
}

static const char *
ExtFontFamilyProc(_Blt_Font *fontPtr) 
{
    ExtFontset *setPtr = fontPtr->clientData;

    return ((TkFont *)setPtr->tkFont)->fa.family;
}

static double
ExtFontPointSizeProc(_Blt_Font *fontPtr) 
{
    ExtFontset *setPtr = fontPtr->clientData;
    TkFont *tkFontPtr;
    int numPoints;
    
    tkFontPtr = (TkFont *)setPtr->tkFont;
    if (tkFontPtr->fa.size < 0) { 
        numPoints = PixelsToPoints(fontPtr->display, -tkFontPtr->fa.size);
    } else {
        numPoints = tkFontPtr->fa.size;
    }
    return numPoints;
}

static double
ExtFontPixelSizeProc(_Blt_Font *fontPtr) 
{
    ExtFontset *setPtr = fontPtr->clientData;
    TkFont *tkFontPtr;
    int numPixels;

    tkFontPtr = (TkFont *)setPtr->tkFont;
    if (tkFontPtr->fa.size < 0) { 
        numPixels = -tkFontPtr->fa.size; 
    } else {
        numPixels = PointsToPixels(fontPtr->display, tkFontPtr->fa.size); 
    }
    return numPixels;
}

static Blt_Font
ExtFontDupProc(Tk_Window tkwin, _Blt_Font *fontPtr, double numPoints) 
{
    Blt_HashEntry *hPtr;
    ExtFontset *newPtr;
    ExtFontset *setPtr = fontPtr->clientData;
    FontPattern *patternPtr;
    Tcl_DString ds;
    _Blt_Font *dupPtr;
    const char *fontName;
    int isNew;

    /* Create a font description with the new requested size. Use it to see
    * if we've already created a font this size. */
    patternPtr = GetPatternFromFont(Tk_Display(tkwin), setPtr->tkFont);
    patternPtr->numPoints = numPoints;   /* Override the size. */
    FontPatternToDString(tkwin, patternPtr, &ds);
    fontName = Tcl_DStringValue(&ds);

    /* See if we already have this fontset. */
    hPtr = Blt_CreateHashEntry(&fontSetTable, (char *)fontName, &isNew);
    Tcl_DStringFree(&ds);
    if (!isNew) {
        newPtr = Blt_GetHashValue(hPtr);
        newPtr->refCount++;
    } else {
	Tk_Font tkFont;

        tkFont = GetFontFromPattern(fontPtr->interp, tkwin, patternPtr);
        if (tkFont == NULL) {
            Blt_DeleteHashEntry(&fontSetTable, hPtr);
            return NULL;
        }
        /* Attach it to the fontset as the base font. */
        newPtr = NewExtFontset(tkFont, hPtr);
        if (newPtr == NULL) {
            Blt_DeleteHashEntry(&fontSetTable, hPtr);
            return NULL;
        }
    }
    /* Create a new Blt_Font shell for this fontset. */
    dupPtr = Blt_Calloc(1, sizeof(_Blt_Font));
    if (dupPtr == NULL) {
        return NULL;                    /* Out of memory. */
    }
    dupPtr->classPtr = &extFontClass;
    dupPtr->interp = fontPtr->interp;
    dupPtr->display = fontPtr->display;
    dupPtr->clientData = newPtr;
    return dupPtr;             
}

static Font
ExtFontIdProc(_Blt_Font *fontPtr) 
{
    ExtFontset *setPtr = fontPtr->clientData;

    return Tk_FontId(setPtr->tkFont);
}

static void
ExtFontGetMetricsProc(_Blt_Font *fontPtr, Blt_FontMetrics *fmPtr)
{
    ExtFontset *setPtr = fontPtr->clientData;
    TkFont *tkFontPtr;

    tkFontPtr = (TkFont *)setPtr->tkFont;
    fmPtr->ascent = tkFontPtr->fm.ascent;
    fmPtr->descent = tkFontPtr->fm.descent;
    fmPtr->linespace = tkFontPtr->fm.ascent + tkFontPtr->fm.descent;
    fmPtr->tabWidth = tkFontPtr->tabWidth;
    fmPtr->underlinePos = tkFontPtr->underlinePos;
    fmPtr->underlineHeight = tkFontPtr->underlineHeight;
}

static int
ExtFontMeasureProc(_Blt_Font *fontPtr, const char *text, int numBytes,
                   int max, int flags, int *lengthPtr)
{
    ExtFontset *setPtr = fontPtr->clientData;
    return Tk_MeasureChars(setPtr->tkFont, text, numBytes, max, flags, 
                lengthPtr);
}

static int
ExtFontTextWidthProc(_Blt_Font *fontPtr, const char *text, int numBytes)
{
    ExtFontset *setPtr = fontPtr->clientData;

    return Tk_TextWidth(setPtr->tkFont, text, numBytes);
}    


static void
ExtFontDrawProc(
    Display *display,                   /* Display on which to draw. */
    Drawable drawable,                  /* Window or pixmap in which to
                                         * draw. */
    GC gc,                              /* Graphics context for drawing
                                         * characters. */
    _Blt_Font *fontPtr,                 /* Font in which characters will be
                                         * drawn; must be the same as font
                                         * used in GC. */
    int depth,                          /* Not used. */
    float angle,                        /* Angle to rotate text in
                                         * degrees. */
    const char *text,                   /* UTF-8 string to be displayed.
                                         * Need not be '\0' terminated.
                                         * All Tk meta-characters (tabs,
                                         * control characters, and
                                         * newlines) should be stripped out
                                         * of the string that is passed to
                                         * this function.  If they are not
                                         * stripped out, they will be
                                         * displayed as regular printing
                                         * characters. */
    int numBytes,                       /* Number of bytes in string. */
    int x, int y)                       /* Coordinates at which to place
                                         * origin of string when
                                         * drawing. */
{
    ExtFontset *setPtr = fontPtr->clientData;

    if (angle != 0.0) {
        intptr_t angle10;
        Blt_HashEntry *hPtr;
    
        angle *= 10.0f;
        angle10 = ROUND(angle);
        hPtr = Blt_FindHashEntry(&setPtr->fontTable, (char *)angle10);
        if (hPtr == NULL) { 
            HFONT hFont;

            hFont = MakeRotatedFont(setPtr->tkFont, angle10);
            if (hFont == NULL) {
                Blt_Warn("can't find font \"%s\" at %I64d rotated\n",
                         setPtr->name, 
                         angle10);
                return;                 /* Can't find instance at requested
                                         * angle. */
            } 
            /* Add it to the set of scaled or rotated fonts.  */
            Blt_SetHashValue(hPtr, hFont);
        }
        display->request++;
        if (drawable != None) {
            HDC hDC;
            HFONT hFont;
            TkWinDCState state;
            
            hFont = Blt_GetHashValue(hPtr);
            hDC = TkWinGetDrawableDC(display, drawable, &state);
            Blt_TextOut(hDC, gc, hFont, text, numBytes, x, y);
            TkWinReleaseDrawableDC(drawable, hDC, &state);
        }
    } else {
        Tk_DrawChars(display, drawable, gc, setPtr->tkFont, text, numBytes, 
                     x, y);
    }
}


static int
ExtFontPostscriptNameProc(_Blt_Font *fontPtr, Tcl_DString *resultPtr) 
{
    ExtFontset *setPtr = fontPtr->clientData;

    return Tk_PostscriptFontName(setPtr->tkFont, resultPtr);
}

static int
ExtFontCanRotateProc(_Blt_Font *fontPtr, float angle) 
{
    Blt_HashEntry *hPtr;
    HFONT hFont;
    ExtFontset *setPtr = fontPtr->clientData;
    int isNew;
    intptr_t angle10;

    angle *= 10.0f;
    angle10 = ROUND(angle);
    if (angle == 0L) {
        return TRUE;
    }
    hPtr = Blt_CreateHashEntry(&setPtr->fontTable, (char *)angle10, &isNew);
    if (!isNew) {
        return TRUE;                    /* Rotated font already exists. */
    }
    /* Create the rotated font and add it to the fontset. */
    hFont = MakeRotatedFont(setPtr->tkFont, angle10);
    if (hFont == NULL) {
        Blt_DeleteHashEntry(&setPtr->fontTable, hPtr);
        return FALSE;
    }
    Blt_SetHashValue(hPtr, hFont);
    return TRUE;
}

/* 
 * ExtFontFreeProc --
 *
 *      Free the fontset. The fontset if destroyed if its reference count
 *      is zero.
 */
static void
ExtFontFreeProc(_Blt_Font *fontPtr) 
{
    ExtFontset *setPtr = fontPtr->clientData;

    setPtr->refCount--;
    if (setPtr->refCount <= 0) {
        DestroyExtFontset(setPtr);
        fontPtr->clientData = NULL;
    }
}

/*
 *---------------------------------------------------------------------------
 *
 * ExtFontUnderlineCharsProc --
 *
 *      This procedure draws an underline for a given range of characters
 *      in a given string.  It doesn't draw the characters (which are
 *      assumed to have been displayed previously); it just draws the
 *      underline.  This procedure would mainly be used to quickly
 *      underline a few characters without having to construct an
 *      underlined font.  To produce properly underlined text, the
 *      appropriate underlined font should be constructed and used.
 *
 * Results:
 *      None.
 *
 *---------------------------------------------------------------------------
 */
static void
ExtFontUnderlineCharsProc(
    Display *display,                   /* Display on which to draw. */
    Drawable drawable,                  /* Window or pixmap in which to
                                         * draw. */
    GC gc,                              /* Graphics context for actually
                                         * drawing line. */
    _Blt_Font *fontPtr,                 /* Font used in GC; must have been
                                         * allocated by Tk_GetFont().  Used
                                         * for character dimensions, etc. */
    const char *string,                 /* String containing characters to be
                                         * underlined or overstruck. */
    int textLen,                        /* Unused. */
    int x, int y,                       /* Coordinates at which first
                                         * character of string is drawn. */
    int first,                          /* Byte offset of the first
                                         * character. */
    int last,                           /* Byte offset after the last
                                         * character. */
    int xMax)
{
    ExtFontset *setPtr = fontPtr->clientData;

    if (last == -1) {
	last = textLen;
    }
    Tk_UnderlineChars(display, drawable, gc, setPtr->tkFont, string, x, y, 
        first, last);
}

/* Public routines. */

/*
 *---------------------------------------------------------------------------
 *
 * Blt_GetFontFromObj -- 
 *
 *      Given a string description of a font, map the description to a
 *      corresponding Tk_Font that represents the font.
 *
 * Results:
 *      The return value is token for the font, or NULL if an error prevented
 *      the font from being created.  If NULL is returned, an error message
 *      will be left in the interp's result.
 *
 * Side effects:
 *      The font is added to an internal database with a reference count.
 *      For each call to this procedure, there should eventually be a call
 *      to Tk_FreeFont() or Tk_FreeFontFromObj() so that the database is
 *      cleaned up when fonts aren't in use anymore.
 *
 *---------------------------------------------------------------------------
 */
Blt_Font
Blt_GetFontFromObj(
    Tcl_Interp *interp,                 /* Interp for database and error
                                         * return. */
    Tk_Window tkwin,                    /* For display on which font will be
                                         * used. */
    Tcl_Obj *objPtr)                    /* String describing font, as: named
                                         * font, native format, or parseable 
                                         * string. */
{
    _Blt_Font *fontPtr; 
    ExtFontset *setPtr;

    if (!font_initialized) {
        Blt_InitHashTable(&fontSetTable, BLT_STRING_KEYS);
        MakeAliasTable(tkwin);
        font_initialized++;
    }
    setPtr = GetFontsetFromObj(interp, tkwin, objPtr);
    if (setPtr == NULL) {
#if DEBUG_FONT_SELECTION
        Blt_Warn("FAILED to find Tk font \"%s\"\n", Tcl_GetString(objPtr));
#endif
        return NULL;
    }
    fontPtr = Blt_AssertCalloc(1, sizeof(_Blt_Font));
    fontPtr->interp = interp;
    fontPtr->display = Tk_Display(tkwin);
    fontPtr->clientData = setPtr;
    fontPtr->classPtr = &extFontClass;
    return fontPtr;
}


Blt_Font
Blt_AllocFontFromObj(
    Tcl_Interp *interp,                 /* Interp for database and error
                                         * return. */
    Tk_Window tkwin,                    /* For screen on which font will be
                                         * used. */
    Tcl_Obj *objPtr)                    /* Object describing font, as: named
                                         * font, native format, or parseable 
                                         * string. */
{
    return Blt_GetFontFromObj(interp, tkwin, objPtr);
}

/*
 *---------------------------------------------------------------------------
 *
 * Blt_GetFont -- 
 *
 *      Given a string description of a font, map the description to a
 *      corresponding Tk_Font that represents the font.
 *
 * Results:
 *      The return value is token for the font, or NULL if an error prevented
 *      the font from being created.  If NULL is returned, an error message
 *      will be left in interp's result object.
 *
 * Side effects:
 *      The font is added to an internal database with a reference count.  For
 *      each call to this procedure, there should eventually be a call to
 *      Blt_Font_Free so that the database is cleaned up when fonts aren't in
 *      use anymore.
 *
 *---------------------------------------------------------------------------
 */

Blt_Font
Blt_GetFont(
    Tcl_Interp *interp,                 /* Interp for database and error
                                         * return. */
    Tk_Window tkwin,                    /* For screen on which font will be
                                         * used. */
    const char *string)                 /* Object describing font, as: named
                                         * font, native format, or parseable 
                                         * string. */
{
    Blt_Font font;
    Tcl_Obj *objPtr;

    objPtr = Tcl_NewStringObj(string, strlen(string));
    Tcl_IncrRefCount(objPtr);
    font = Blt_GetFontFromObj(interp, tkwin, objPtr);
    Tcl_DecrRefCount(objPtr);
    return font;
}

Tcl_Interp *
Blt_Font_GetInterp(_Blt_Font *fontPtr) 
{
    return fontPtr->interp;
}

int
Blt_TextWidth(_Blt_Font *fontPtr, const char *string, int length)
{
    if (Blt_Afm_IsPrinting()) {
        int width;

        width = Blt_Afm_TextWidth(fontPtr, string, length);
        if (width >= 0) {
            return width;
        }
    }
    return (*fontPtr->classPtr->textWidthProc)(fontPtr, string, length);
}

void
Blt_Font_GetMetrics(_Blt_Font *fontPtr, Blt_FontMetrics *fmPtr)
{
    if (Blt_Afm_IsPrinting()) {
        if (Blt_Afm_GetMetrics(fontPtr, fmPtr) == TCL_OK) {
            return;
        }
    }
    (*fontPtr->classPtr->getMetricsProc)(fontPtr, fmPtr);
}

Tcl_Obj *
Blt_Font_GetFile(Tcl_Interp *interp, Tcl_Obj *objPtr, double *numPointsPtr)
{
    Tcl_DString nameStr, valueStr;
    Tcl_Obj *fileObjPtr;
    FontPattern *patternPtr;
    int result;

    if (!font_initialized) {
        Blt_InitHashTable(&fontSetTable, BLT_STRING_KEYS);
        MakeAliasTable(Tk_MainWindow(interp));
        font_initialized++;
    }
    patternPtr = GetFontPattern(interp, Tk_MainWindow(interp), objPtr);
    if (patternPtr == NULL) {
        return NULL;
    }
    fileObjPtr = NULL;
    Tcl_DStringInit(&nameStr);
    Tcl_DStringInit(&valueStr);
    *numPointsPtr = patternPtr->numPoints;

    result = GetFile(interp, patternPtr->family, &nameStr, &valueStr);
    FreeFontPattern(patternPtr);
    Tcl_DStringFree(&nameStr);

    if (result == TCL_OK) {
        const char *root;

        root = getenv("SYSTEMROOT");
        if (root == NULL) {
            root = "c:/WINDOWS";
        }
        fileObjPtr = Tcl_NewStringObj(root, -1);
        Tcl_IncrRefCount(fileObjPtr);
        Tcl_AppendToObj(fileObjPtr, "/fonts/", -1);
        Tcl_AppendToObj(fileObjPtr, Tcl_DStringValue(&valueStr),
                        Tcl_DStringLength(&valueStr));
    }
    Tcl_DStringFree(&valueStr);
    return fileObjPtr;
}

