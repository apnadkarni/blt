
/*
 * tkFont.h --
 *
 *
 * This file contains definitions of internal Tk font structures.
 *
 * Copyright (c) 1997 by Sun Microsystems, Inc.
 *
 * See the file "license.terms" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef _TK_FONT_H
#define _TK_FONT_H

/*
 * Possible values for the "weight" field in a TkFontAttributes structure.
 * Weight is a subjective term and depends on what the company that created
 * the font considers bold.
 */

#define TK_FW_NORMAL	0
#define TK_FW_BOLD	1

#define TK_FW_UNKNOWN	-1	/* Unknown weight.  This value is used for
				 * error checking and is never actually stored
				 * in the weight field. */

/*
 * Possible values for the "slant" field in a TkFontAttributes structure.
 */

#define TK_FS_ROMAN	0	
#define TK_FS_ITALIC	1
#define TK_FS_OBLIQUE	2	/* This value is only used when parsing X
				 * font names to determine the closest
				 * match.  It is only stored in the
				 * XLFDAttributes structure, never in the
				 * slant field of the TkFontAttributes. */

#define TK_FS_UNKNOWN	-1	/* Unknown slant.  This value is used for
				 * error checking and is never actually stored
				 * in the slant field. */
typedef struct {
    Tk_Uid family;		/* Font family. The most important field. */
    int size;			/* Pointsize of font, 0 for default size, or
				 * negative number meaning pixel size. */
    int weight;			/* Weight flag; see below for def'n. */
    int slant;			/* Slant flag; see below for def'n. */
    int underline;		/* Non-zero for underline font. */
    int overstrike;		/* Non-zero for overstrike font. */
} TkFontAttributes;

typedef struct {
    int ascent;			/* From baseline to top of font. */
    int descent;		/* From baseline to bottom of font. */
    int maxWidth;		/* Width of widest character in font. */
    int fixed;			/* Non-zero if this is a fixed-width font,
				 * 0 otherwise. */
} TkFontMetrics;


typedef struct _TkFont {
    /*
     * Fields used and maintained exclusively by generic code.
     */
#if (_TK_VERSION >= _VERSION(8,1,0))
    int resourceRefCount;	/* Number of active uses of this font (each
				 * active use corresponds to a call to
				 * Tk_AllocFontFromTable or Tk_GetFont).
				 * If this count is 0, then this TkFont
				 * structure is no longer valid and it isn't
				 * present in a hash table: it is being
				 * kept around only because there are objects
				 * referring to it.  The structure is freed
				 * when resourceRefCount and objRefCount
				 * are both 0. */
    int objRefCount;		/* The number of TCL objects that reference
				 * this structure. */
#else
    int refCount;		/* Number of users of the TkFont. */
#endif
    Tcl_HashEntry *cacheHashPtr;/* Entry in font cache for this structure,
				 * used when deleting it. */
    Tcl_HashEntry *namedHashPtr;/* Pointer to hash table entry that
				 * corresponds to the named font that the
				 * tkfont was based on, or NULL if the tkfont
				 * was not based on a named font. */
#if (_TK_VERSION >= _VERSION(8,1,0))
    Screen *screen;		/* The screen where this font is valid. */
#endif /* _TK_VERSION >= 8.1.0 */
    int tabWidth;		/* Width of tabs in this font (pixels). */
    int underlinePos;		/* Offset from baseline to origin of
				 * underline bar (used for drawing underlines
				 * on a non-underlined font). */
    int underlineHeight;	/* Height of underline bar (used for drawing
				 * underlines on a non-underlined font). */

    /*
     * Fields in the generic font structure that are filled in by
     * platform-specific code.
     */

    Font fid;			/* For backwards compatibility with XGCValues
				 * structures.  Remove when TkGCValues is
				 * implemented.  */
    TkFontAttributes fa;	/* Actual font attributes obtained when the
				 * the font was created, as opposed to the
				 * desired attributes passed in to
				 * TkpGetFontFromAttributes().  The desired
				 * metrics can be determined from the string
				 * that was used to create this font. */
    TkFontMetrics fm;		/* Font metrics determined when font was
				 * created. */
#if (_TK_VERSION >= _VERSION(8,1,0))
    struct _TkFont *nextPtr;	/* Points to the next TkFont structure with
				 * the same name.  All fonts with the
				 * same name (but different displays) are
				 * chained together off a single entry in
				 * a hash table. */
#endif /* _TK_VERSION >= 8.1.0 */
} TkFont;

typedef struct TkXLFDAttributes {
    Tk_Uid foundry;		/* The foundry of the font. */
    int slant;			/* The tristate value for the slant, which
				 * is significant under X. */
    int setwidth;		/* The proportionate width, see below for
				 * definition. */
    Tk_Uid charset;		/* The actual charset string. */
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
    char *realName;		/* The real name of the encoding to load if
				 * the provided name matched the pattern. */
    char *aliasPattern;		/* Pattern for encoding name, of the form
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
    Tcl_HashTable fontCache;	/* Map a string to an existing Tk_Font.
				 * Keys are string font names, values are
				 * TkFont pointers. */
    Tcl_HashTable namedTable;	/* Map a name to a set of attributes for a
				 * font, used when constructing a Tk_Font from
				 * a named font description.  Keys are
				 * strings, values are NamedFont pointers. */
    TkMainInfo *mainPtr;	/* Application that owns this structure. */
    int updatePending;		/* Non-zero when a World Changed event has
				 * already been queued to handle a change to
				 * a named font. */
} TkFontInfo;

#endif /* _TK_FONT_H */
