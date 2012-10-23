
/*
 * tkUnixFont.h --
 *
 *
 * The Font structure used internally by the Tk_3D* routines.
 * The following is a copy of it from tk3d.c.
 *
 *	Contains copies of internal Tk structures.
 *
 * Copyright (c) 1997 by Sun Microsystems, Inc.
 *
 * See the file "license.terms" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
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

#define FONTMAP_SHIFT		10

#define FONTMAP_PAGES	    	(1 << (sizeof(Tcl_UniChar)*8 - FONTMAP_SHIFT))
#define FONTMAP_BITSPERPAGE	(1 << FONTMAP_SHIFT)

typedef struct _FontFamily {
    struct _FontFamily *nextPtr; /* Next in list of all known font families. */
    int refCount;		/* How many SubFonts are referring to this
				 * FontFamily.  When the refCount drops to
				 * zero, this FontFamily may be freed. */
    /*
     * Key.
     */

    Tk_Uid foundry;		/* Foundry key for this FontFamily. */
    Tk_Uid faceName;		/* Face name key for this FontFamily. */
    Tcl_Encoding encoding;	/* Encoding key for this FontFamily. */

    /*
     * Derived properties.
     */

    int isTwoByteFont;		/* 1 if this is a double-byte font, 0 
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
    char **fontMap;		/* Pointer to font map from the FontFamily, 
				 * cached here to save a dereference. */
    XFontStruct *fontStructPtr;	/* The specific screen font that will be
				 * used when displaying/measuring chars
				 * belonging to the FontFamily. */
    FontFamily *familyPtr;	/* The FontFamily for this SubFont. */
} SubFont;

/*
 * The following structure represents Unix's implementation of a font
 * object.
 */
 
#define SUBFONT_SPACE		3
#define BASE_CHARS		256

typedef struct _UnixFont {
    TkFont font;		/* Stuff used by generic font package.  Must
				 * be first in structure. */
    SubFont staticSubFonts[SUBFONT_SPACE];
				/* Builtin space for a limited number of
				 * SubFonts. */
    int numSubFonts;		/* Length of following array. */
    SubFont *subFontArray;	/* Array of SubFonts that have been loaded
				 * in order to draw/measure all the characters
				 * encountered by this font so far.  All fonts
				 * start off with one SubFont initialized by
				 * AllocFont() from the original set of font
				 * attributes.  Usually points to
				 * staticSubFonts, but may point to malloced
				 * space if there are lots of SubFonts. */
    SubFont controlSubFont;	/* Font to use to display control-character
				 * expansions. */

    Display *display;		/* Display that owns font. */
    int pixelSize;		/* Original pixel size used when font was
				 * constructed. */
    TkXLFDAttributes xa;	/* Additional attributes that specify the
				 * preferred foundry and encoding to use when
				 * constructing additional SubFonts. */
    int widths[BASE_CHARS];	/* Widths of first 256 chars in the base
				 * font, for handling common case. */
    int underlinePos;		/* Offset from baseline to origin of
				 * underline bar (used when drawing underlined
				 * font) (pixels). */
    int barHeight;		/* Height of underline or overstrike bar
				 * (used when drawing underlined or strikeout
				 * font) (pixels). */
} UnixFont;

#endif /* _TK_UNIXFONT_H */
