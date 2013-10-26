
/*
 * bltPictText.c --
 *
 * This module implements text drawing for picture images in the BLT toolkit.
 *
 *	Copyright 1997-2004 George A Howlett.
 *
 *	Permission is hereby granted, free of charge, to any person obtaining
 *	a copy of this software and associated documentation files (the
 *	"Software"), to deal in the Software without restriction, including
 *	without limitation the rights to use, copy, modify, merge, publish,
 *	distribute, sublicense, and/or sell copies of the Software, and to
 *	permit persons to whom the Software is furnished to do so, subject to
 *	the following conditions:
 *
 *	The above copyright notice and this permission notice shall be
 *	included in all copies or substantial portions of the Software.
 *
 *	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 *	EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 *	MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 *	NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
 *	LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
 *	OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
 *	WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#include "bltInt.h"
#include <string.h>
#ifdef HAVE_LIBXFT
#include <ft2build.h>
#include FT_FREETYPE_H
#include <X11/Xft/Xft.h>
#endif
#include <X11/Xutil.h>
#include "bltMath.h"
#include "bltPicture.h"
#include "bltPictInt.h"
#include "bltBg.h"
#include "bltPainter.h"
#include "bltFont.h"
#include "bltText.h"
#include "bltOp.h"
#include "tkIntBorder.h"

#define imul8x8(a,b,t)	((t) = (a)*(b)+128,(((t)+((t)>>8))>>8))
#define CLAMP(c)	((((c) < 0.0) ? 0.0 : ((c) > 255.0) ? 255.0 : (c)))

#ifndef WIN32 
#  if defined (HAVE_LIBXFT) && defined(HAVE_X11_XFT_XFT_H)
#    define HAVE_XFT
#    include <X11/Xft/Xft.h>
#  endif	/* HAVE_XFT */
#endif /*WIN32*/

#if defined (HAVE_FT2BUILD_H)
#  define HAVE_FT2
#  include <ft2build.h>
#  include FT_FREETYPE_H
#endif

#ifndef HAVE_FT2
#  define DRAWTEXT 0
#  error("No Freetype")
#else 
#  ifdef WIN32
#    define DRAWTEXT 1
#  else 
#    ifdef HAVE_XFT
#      define DRAWTEXT 1
#    else
#      error("WIN32 &&  !XFT")
#      define DRAWTEXT 0
#    endif
#  endif
#endif

static FT_Library ftLibrary;

/* 
 * TextFont --
 *
 *	This structure contains the various pieces to draw text using the
 *	freetype library into a picture image.
 */
typedef struct {
    FT_Face face;			/* Type face of font */
    FT_Matrix matrix;			/* Rotation matrix for font. Used for
					* rotated fonts. */
#ifdef HAVE_XFT
    XftFont *xftFont;			/* Xft font handle used to get face
					 * from Xft font. */
#endif
    int fontSize;			/* Point size of font.  */
    float angle;			/* Rotation of font. */
    int height;				/* Line height of font.  */
    int ascent, descent;		/* Ascent and descent of font. */
} TextFont;    

typedef struct {
    size_t numValues;
    void *values;
} Array;

static Blt_SwitchParseProc AnchorSwitch;
static Blt_SwitchCustom anchorSwitch = {
    AnchorSwitch, NULL, NULL, (ClientData)0
};

static Blt_SwitchParseProc ShadowSwitch;
static Blt_SwitchCustom shadowSwitch = {
    ShadowSwitch, NULL, NULL, (ClientData)0
};

static Blt_SwitchParseProc JustifySwitch;
static Blt_SwitchCustom justifySwitch = {
    JustifySwitch, NULL, NULL, (ClientData)0
};

static Blt_SwitchParseProc ObjToPaintbrushProc;
static Blt_SwitchFreeProc PaintbrushFreeProc;
static Blt_SwitchCustom paintbrushSwitch =
{
    ObjToPaintbrushProc, NULL, PaintbrushFreeProc, (ClientData)0,
};

typedef struct {
    int kerning;			/* Indicates whether to kern text. */
    Blt_Paintbrush *brushPtr;		/* Color of text. */
    Blt_Shadow shadow;			/*  */
    int fontSize;			/* Size of requested font. */
    Tcl_Obj *fontObjPtr;		/* Requested font.  If the name of the
					 * font starts with a '@' sign, this
					 * is the path to a font file.
					 * Otherwise this is the name of
					 * font. We will use the font handling
					 * routines to get a corresponding
					 * face. */
    int justify;
    Tk_Anchor anchor;
    float angle;
} TextSwitches;

static Blt_SwitchSpec textSwitches[] = 
{
    {BLT_SWITCH_CUSTOM,  "-anchor",   "anchor", (char *)NULL,
	Blt_Offset(TextSwitches, anchor), 0, 0, &anchorSwitch},
    {BLT_SWITCH_CUSTOM,  "-color",    "colorName", (char *)NULL,
	Blt_Offset(TextSwitches, brushPtr),  0, 0, &paintbrushSwitch},
    {BLT_SWITCH_OBJ,     "-font",     "fontName", (char *)NULL,
	Blt_Offset(TextSwitches, fontObjPtr), 0},
    {BLT_SWITCH_CUSTOM,  "-justify", "left|right|center", (char *)NULL,
	Blt_Offset(TextSwitches, justify), 0, 0, &justifySwitch},
    {BLT_SWITCH_BOOLEAN, "-kerning",  "bool", (char *)NULL,
	Blt_Offset(TextSwitches, kerning),  0},
    {BLT_SWITCH_FLOAT,   "-rotate",   "angle", (char *)NULL,
	Blt_Offset(TextSwitches, angle), 0},
    {BLT_SWITCH_INT,     "-size",     "number", (char *)NULL,
	Blt_Offset(TextSwitches, fontSize),  0}, 
    {BLT_SWITCH_CUSTOM, "-shadow", "offset", (char *)NULL,
        Blt_Offset(TextSwitches, shadow), 0, 0, &shadowSwitch},
    {BLT_SWITCH_END}
};

DLLEXPORT extern Tcl_AppInitProc Blt_PictureTextOpInit;
DLLEXPORT extern Tcl_AppInitProc Blt_picture_text_Init;

/*ARGSUSED*/
static void
PaintbrushFreeProc(ClientData clientData, char *record, int offset, int flags)
{
    Blt_Paintbrush **brushPtrPtr = (Blt_Paintbrush **)(record + offset);

    if (*brushPtrPtr != NULL) {
	Blt_Paintbrush_Free(*brushPtrPtr);
	*brushPtrPtr = NULL;
    }
}


/*
 *---------------------------------------------------------------------------
 *
 * ObjToPaintbrushProc --
 *
 *	Convert a Tcl_Obj representing a paint brush.
 *
 * Results:
 *	The return value is a standard TCL result.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
ObjToPaintbrushProc(
    ClientData clientData,		/* Not used. */
    Tcl_Interp *interp,			/* Interpreter to send results. */
    const char *switchName,		/* Not used. */
    Tcl_Obj *objPtr,			/* String representation */
    char *record,			/* Structure record */
    int offset,				/* Offset to field in structure */
    int flags)	
{
    Blt_Paintbrush **brushPtrPtr = (Blt_Paintbrush **)(record + offset);
    Blt_Paintbrush *brushPtr;

    if (Blt_Paintbrush_Get(interp, objPtr, &brushPtr) != TCL_OK) {
	return TCL_ERROR;
    }
    if (*brushPtrPtr != NULL) {
	Blt_Paintbrush_Free(*brushPtrPtr);
    }
    *brushPtrPtr = brushPtr;
    return TCL_OK;
}

static void GetTextExtents(TextFont *fontPtr, const char *string, size_t length,
			size_t *widthPtr, size_t *heightPtr);

static size_t GetTextWidth(TextFont *fontPtr, const char *string, size_t length,
			   int kerning);

/*
 *---------------------------------------------------------------------------
 *
 * AnchorSwitch --
 *
 *	Convert a Tcl_Obj representing an anchor.
 *
 * Results:
 *	The return value is a standard TCL result.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
AnchorSwitch(
    ClientData clientData,		/* Not used. */
    Tcl_Interp *interp,			/* Interpreter to send results. */
    const char *switchName,		/* Not used. */
    Tcl_Obj *objPtr,			/* String representation */
    char *record,			/* Structure record */
    int offset,				/* Offset to field in structure */
    int flags)	
{
    Tk_Anchor *anchorPtr = (Tk_Anchor *)(record + offset);

    if (Tk_GetAnchorFromObj(interp, objPtr, anchorPtr) != TCL_OK) {
	return TCL_ERROR;
    }
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * AnchorSwitch --
 *
 *	Convert a Tcl_Obj representing an anchor.
 *
 * Results:
 *	The return value is a standard TCL result.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
JustifySwitch(
    ClientData clientData,		/* Not used. */
    Tcl_Interp *interp,			/* Interpreter to send results. */
    const char *switchName,		/* Not used. */
    Tcl_Obj *objPtr,			/* String representation */
    char *record,			/* Structure record */
    int offset,				/* Offset to field in structure */
    int flags)	
{
    Tk_Justify *justifyPtr = (Tk_Justify *)(record + offset);

    if (Tk_GetJustifyFromObj(interp, objPtr, justifyPtr) != TCL_OK) {
	return TCL_ERROR;
    }
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * ShadowSwitch --
 *
 *	Convert a Tcl_Obj representing a number for the alpha value.
 *
 * Results:
 *	The return value is a standard TCL result.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
ShadowSwitch(
    ClientData clientData,		/* Not used. */
    Tcl_Interp *interp,			/* Interpreter to send results. */
    const char *switchName,		/* Not used. */
    Tcl_Obj *objPtr,			/* String representation */
    char *record,			/* Structure record */
    int offset,				/* Offset to field in structure */
    int flags)				/* Not used. */
{
    Blt_Shadow *shadowPtr = (Blt_Shadow *)(record + offset);
    int objc;
    Tcl_Obj **objv;
    int i, width;

    shadowPtr->offset = 2;
    shadowPtr->width = 2;
    shadowPtr->color.u32 = 0xA0000000;
    if (Tcl_GetIntFromObj(NULL, objPtr, &width) == TCL_OK) {
	shadowPtr->width = shadowPtr->offset = width;
	return TCL_OK;
    }
    if (Tcl_ListObjGetElements(interp, objPtr, &objc, &objv) != TCL_OK) {
	return TCL_ERROR;
    }
    for (i = 0; i < objc; i += 2) {
	const char *string;
	int length;
	char c;

	string = Tcl_GetStringFromObj(objv[i], &length);
	if (string[0] != '-') {
	    Tcl_AppendResult(interp, "bad shadow option \"", string, 
		"\": should be -offset, -width, or -color", (char *)NULL);
	    return TCL_ERROR;
	}
	c = string[1];
	if ((c == 'w') && (strncmp(string, "-width", length) == 0)) {
	    if (Tcl_GetIntFromObj(interp, objv[i+1], &shadowPtr->width) 
		!= TCL_OK) {
		return TCL_ERROR;
	    }
	} else if ((c == 'o') && (strncmp(string, "-offset", length) == 0)) {
	    if (Tcl_GetIntFromObj(interp, objv[i+1], &shadowPtr->offset) 
		!= TCL_OK) {
		return TCL_ERROR;
	    }
	} else if ((c == 'c') && (strncmp(string, "-color", length) == 0)) {
	    if (Blt_GetPixelFromObj(interp, objv[i+1], &shadowPtr->color) 
		!= TCL_OK) {
		return TCL_ERROR;
	    }
	} else if ((c == 'a') && (strncmp(string, "-alpha", length) == 0)) {
	    int alpha;

	    if (Tcl_GetIntFromObj(interp, objv[i+1], &alpha) != TCL_OK) {
		return TCL_ERROR;
	    }
	    shadowPtr->color.Alpha = alpha;
	} else {
	    Tcl_AppendResult(interp, "unknown shadow option \"", string, 
		"\": should be -offset, -width, or -color", (char *)NULL);
	    return TCL_ERROR;
	}
    }
    return TCL_OK;
}

static void
DebugFace(FT_Face face)
{
    fprintf(stderr, "num_faces=%ld\n", face->num_faces);
    fprintf(stderr, "face_flags=%lx\n", face->face_flags);
    fprintf(stderr, "style_flags=%lx\n", face->style_flags);
    fprintf(stderr, "num_glyphs=%ld\n", face->num_glyphs);
    fprintf(stderr, "family_name=%s\n", face->family_name);
    fprintf(stderr, "style_name=%s\n", face->style_name);
    fprintf(stderr, "num_fixed_sizes=%d\n", face->num_fixed_sizes);
    fprintf(stderr, "num_charmaps=%d\n", face->num_charmaps);
    fprintf(stderr, "units_per_EM=%d\n", face->units_per_EM);
    fprintf(stderr, "face->size->metrics.height=%ld\n", face->size->metrics.height);
    fprintf(stderr, "face->size->metrics.ascender=%ld\n", face->size->metrics.ascender);
    fprintf(stderr, "face->size->metrics.descender=%ld\n", face->size->metrics.descender);
    fprintf(stderr, "ascender=%d\n", face->ascender);
    fprintf(stderr, "descender=%d\n", face->descender);
    fprintf(stderr, "height=%d\n", face->height);
    fprintf(stderr, "max_advance_width=%d\n", face->max_advance_width);
    fprintf(stderr, "max_advance_height=%d\n", face->max_advance_height);
    fprintf(stderr, "underline_position=%d\n", face->underline_position);
    fprintf(stderr, "underline_thickness=%d\n", (int)face->underline_thickness);
}

static void
DebugGlyph(FT_GlyphSlot slot)
{
    fprintf(stderr, "  slot.bitmap.width=%d\n", (int)slot->bitmap.width);
    fprintf(stderr, "  slot.bitmap.rows=%d\n", slot->bitmap.rows);
    fprintf(stderr, "  slot.bitmap_left=%d\n", (int)slot->bitmap_left);
    fprintf(stderr, "  slot.bitmap_top=%d\n", (int)slot->bitmap_top);
    fprintf(stderr, "  slot.bitmap.pixel_mode=%x\n", slot->bitmap.pixel_mode);
    fprintf(stderr, "  slot.advance.x=%d\n", (int)slot->advance.x);
    fprintf(stderr, "  slot.advance.y=%d\n", (int)slot->advance.y);
    
    fprintf(stderr, "  slot.format=%c%c%c%c\n", 
	    (slot->format >> 24) & 0xFF, 
	    (slot->format >> 16) & 0xFF, 
	    (slot->format >> 8) & 0xFF, 
	    (slot->format & 0xFF));
}


/*
 *---------------------------------------------------------------------------
 *
 * CreateSimpleTextLayout --
 *
 *	Get the extents of a possibly multiple-lined text string.
 *
 * Results:
 *	Returns via *widthPtr* and *heightPtr* the dimensions of the text
 *	string.
 *
 *---------------------------------------------------------------------------
 */
static TextLayout *
CreateSimpleTextLayout(TextFont *fontPtr, const char *text, int textLen, 
		       TextStyle *tsPtr)
{
    TextFragment *fp;
    TextLayout *layoutPtr;
    size_t count;			/* Count # of characters on each
					 * line. */
    int lineHeight;
    int maxHeight, maxWidth;
    int numFrags;
    const char *p, *endp, *start;
    int i;
    size_t size;

    numFrags = 0;
    endp = text + ((textLen < 0) ? strlen(text) : textLen);
    for (p = text; p < endp; p++) {
	if (*p == '\n') {
	    numFrags++;
	}
    }
    if ((p != text) && (*(p - 1) != '\n')) {
	numFrags++;
    }
    size = sizeof(TextLayout) + (sizeof(TextFragment) * (numFrags - 1));

    layoutPtr = Blt_AssertCalloc(1, size);
    layoutPtr->numFragments = numFrags;

    numFrags = count = 0;
    maxWidth = 0;
    maxHeight = tsPtr->padTop;
    lineHeight = fontPtr->height;

    fp = layoutPtr->fragments;
    for (p = start = text; p < endp; p++) {
	if (*p == '\n') {
	    size_t w;

	    if (count > 0) {
		w = GetTextWidth(fontPtr, start, count, 1);
		if (w > maxWidth) {
		    maxWidth = w;
		}
	    } else {
		w = 0;
	    }
	    fp->width = w;
	    fp->count = count;
	    fp->sy = fp->y = maxHeight + fontPtr->ascent;
	    fp->text = start;
	    maxHeight += lineHeight;
	    fp++;
	    numFrags++;
	    start = p + 1;		/* Start the text on the next line */
	    count = 0;			/* Reset to indicate the start of a
					 * new line */
	    continue;
	}
	count++;
    }
    if (numFrags < layoutPtr->numFragments) {
	size_t w;

	w = GetTextWidth(fontPtr, start, count, 1);
	if (w > maxWidth) {
	    maxWidth = w;
	}
	fp->width = w;
	fp->count = count;
	fp->sy = fp->y = maxHeight + fontPtr->ascent;
	fp->text = start;
	maxHeight += lineHeight;
	numFrags++;
    }
    maxHeight += tsPtr->padBottom;
    maxWidth += PADDING(tsPtr->xPad);
    for (i = 0; i < numFrags; i++) {
	TextFragment *fp;
	
	fp = layoutPtr->fragments + i;
	switch (tsPtr->justify) {
	default:
	case TK_JUSTIFY_LEFT:
	    /* No offset for left justified text strings */
	    fp->x = fp->sx = tsPtr->padLeft;
	    break;
	case TK_JUSTIFY_RIGHT:
	    fp->x = fp->sx = (maxWidth - fp->width) - tsPtr->padRight;
	    break;
	case TK_JUSTIFY_CENTER:
	    fp->x = fp->sx = (maxWidth - fp->width) / 2;
	    break;
	}
    }
    if (tsPtr->underline >= 0) {
	for (i = 0; i < numFrags; i++) {
	    int first, last;
	    TextFragment *fp;
	
	    fp = layoutPtr->fragments + i;
	    first = fp->text - text;
	    last = first + fp->count;
	    if ((tsPtr->underline >= first) && (tsPtr->underline < last)) {
		layoutPtr->underlinePtr = fp;
		layoutPtr->underline = tsPtr->underline - first;
		break;
	    }
	}
    }
    layoutPtr->width = maxWidth;
    layoutPtr->height = maxHeight - tsPtr->leader;
    return layoutPtr;
}

#undef __FTERRORS_H__
#define FT_ERRORDEF( e, v, s )  { e, s },
#define FT_ERROR_START_LIST     {
#define FT_ERROR_END_LIST       { -1, 0 } };

static const char *
FtError(FT_Error ftError)
{
    struct ft_errors {                                          
	int          code;             
	const char*  msg;
    };
    static struct ft_errors ft_err_mesgs[] = 
#include FT_ERRORS_H            

    struct ft_errors *fp;
    for (fp = ft_err_mesgs; fp->msg != NULL; fp++) {
	if (fp->code == ftError) {
	    return fp->msg;
	}
    }
    return "unknown Freetype error";
}

static void
GetTextExtents(TextFont *fontPtr, const char *string, size_t length,
	       size_t *widthPtr, size_t *heightPtr)
{
    FT_Vector pen;			/* Untransformed origin  */
    FT_GlyphSlot  slot;
    FT_Matrix matrix;			/* Transformation matrix. */
    int maxX, maxY;
    const char *p, *pend;
    double radians;
    int x;
    FT_Face face;

    radians = 0.0;
    matrix.yy = matrix.xx = (FT_Fixed)(cos(radians) * 65536.0);
    matrix.yx = (FT_Fixed)(sin(radians) * 65536.0);
    matrix.xy = -matrix.yx;

    face = fontPtr->face;
    slot = face->glyph;
    
    maxY = maxX = 0;
    pen.y = 0;
    x = 0;
#ifdef notdef
    fprintf(stderr, "face->height=%d, face->size->height=%d\n",
	    face->height, (int)face->size->metrics.height);
    fprintf(stderr, "face->ascender=%d, face->descender=%d\n",
	    face->ascender, face->descender);
#endif
    for (p = string, pend = p + length; p < pend; p++) {
	maxY += face->size->metrics.height;
	pen.x = x << 6;
	for (/*empty*/; (*p != '\n') && (p < pend); p++) {
	    FT_Error ftError;

	    FT_Set_Transform(face, &matrix, &pen);
	    /* Load glyph image into the slot (erase previous) */
	    ftError = FT_Load_Char(face, *p, FT_LOAD_RENDER);
	    if (ftError != 0) {
		Blt_Warn("can't load character \"%c\": %s\n", *p, 
			FtError(ftError));
		continue;                 /* Ignore errors. */
	    }
	    pen.x += slot->advance.x;
	    pen.y += slot->advance.y;
	}
	if (pen.x > maxX) {
	    maxX = pen.x;
	}
    }	
#ifdef notdef
    fprintf(stderr, "w=%d,h=%d\n", maxX >> 6, maxY >> 6);
#endif
    *widthPtr = (size_t)(maxX >> 6);
    *heightPtr = (size_t)(maxY >> 6);
#ifdef notdef
    fprintf(stderr, "w=%lu,h=%lu\n", (unsigned long)*widthPtr, 
	    (unsigned long)*heightPtr);
#endif
}

static size_t
GetTextWidth(TextFont *fontPtr, const char *string, size_t length, int kerning)
{
    FT_Vector pen;			/* Untransformed origin  */
    FT_GlyphSlot  slot;
    FT_Matrix matrix;			/* Transformation matrix. */
    int maxX;
    const char *p, *pend;
    double radians;
    FT_Face face;
    int previous;
    FT_Error ftError;

    radians = 0.0;
    matrix.yy = matrix.xx = (FT_Fixed)(cos(radians) * 65536.0);
    matrix.yx = (FT_Fixed)(sin(radians) * 65536.0);
    matrix.xy = -matrix.yx;

    face = fontPtr->face;
    slot = face->glyph;
    
    maxX = 0;
    pen.y = pen.x = 0;
#ifdef notdef
    fprintf(stderr, "face->height=%d, face->size->height=%d\n",
	    face->height, (int)face->size->metrics.height);
#endif
    previous = -1;
    for (p = string, pend = p + length; p < pend; p++) {
	int current;

	current = FT_Get_Char_Index(face, *p);
	if ((kerning) && (previous >= 0)) { 
	    FT_Vector delta; 
		
	    FT_Get_Kerning(face, previous, current, FT_KERNING_DEFAULT, &delta);
	    pen.x += delta.x; 
	} 
	FT_Set_Transform(face, &matrix, &pen);
	previous = current;
	/* load glyph image into the slot (erase previous one) */  
	ftError = FT_Load_Glyph(face, current, FT_LOAD_DEFAULT); 
	if (ftError) {
	    Blt_Warn("can't load character \"%c\" (%d): %s\n", *p, current, 
		FtError(ftError));
	    continue;			/* Ignore errors. */
	}
	pen.x += slot->advance.x;
	pen.y += slot->advance.y;
	if (pen.x > maxX) {
	    maxX = pen.x;
	}
    }	
    return maxX >> 6;
}

static INLINE void
BlendPixel(Blt_Pixel *bgPtr, Blt_Pixel *colorPtr, unsigned char weight)
{
    unsigned char alpha;
    int t1;

    alpha = imul8x8(colorPtr->Alpha, weight, t1);
    if ((bgPtr->Alpha == 0x0) || (alpha == 0xFF)) {
	bgPtr->u32 = colorPtr->u32;
	bgPtr->Alpha = alpha;
    } else if (alpha != 0x00) {
	unsigned char beta;
	int t1, t2;

	beta = alpha ^ 0xFF;
	bgPtr->Red   = imul8x8(alpha, colorPtr->Red, t1) + 
	    imul8x8(beta, bgPtr->Red, t2);
	bgPtr->Green = imul8x8(alpha, colorPtr->Green, t1) + 
	    imul8x8(beta, bgPtr->Green, t2);
	bgPtr->Blue  = imul8x8(alpha, colorPtr->Blue, t1)  + 
	    imul8x8(beta, bgPtr->Blue, t2);
	bgPtr->Alpha = alpha + imul8x8(beta, bgPtr->Alpha, t2);
    }
}

static void
BlitGlyph(Pict *destPtr, 
    FT_GlyphSlot slot, 
    int dx, int dy,
    int xx, int yy,
    Blt_Paintbrush *brushPtr)
{
    int x1, y1, x2, y2;
#ifdef notdef
    fprintf(stderr, "dx=%d, dy=%d\n", dx, dy);
    DebugGlyph(slot);
#endif
    if ((dx >= destPtr->width) || ((dx + slot->bitmap.width) <= 0) ||
	(dy >= destPtr->height) || ((dy + slot->bitmap.rows) <= 0)) {
	return;				/* No portion of the glyph is visible
					 * in the picture. */
    }
    /* By default, set the region to cover the entire glyph */
    x1 = y1 = 0;
    x2 = slot->bitmap.width;
    y2 = slot->bitmap.rows;

    /* Determine the portion of the glyph inside the picture. */

    if (dx < 0) {			/* Left side of glyph overhangs. */
	x1 -= dx;		
	x2 += dx;
	dx = 0;		
    }
    if (dy < 0) {			/* Top of glyph overhangs. */
	y1 -= dy;
	y2 += dy;
	dy = 0;
    }
    if ((dx + x2) > destPtr->width) {	/* Right side of glyph overhangs. */
	x2 = destPtr->width - dx;
    }
    if ((dy + y2) > destPtr->height) {	/* Bottom of glyph overhangs. */
	y2 = destPtr->height - dy;
    }
    if (slot->bitmap.pixel_mode == FT_PIXEL_MODE_MONO) {
	Blt_Pixel *destRowPtr;
	unsigned char *srcRowPtr;
	int y;

	srcRowPtr = slot->bitmap.buffer + (y1 * slot->bitmap.pitch);
	destRowPtr = Blt_PicturePixel(destPtr, xx, yy);
	for (y = y1; y < y2; y++) {
	    Blt_Pixel *dp;
	    int x;
	    
	    dp = destRowPtr;
	    for (x = x1; x < x2; x++, dp++) {
		int pixel;

		pixel = srcRowPtr[x >> 3] & (1 << (7 - (x & 0x7)));
		if (pixel != 0x0) {
		    Blt_Pixel color;

		    color.u32 = Blt_Paintbrush_GetColor(brushPtr, x, y);
		    BlendPixel(dp, &color, 0xFF);
		}
	    }
	    srcRowPtr += slot->bitmap.pitch;
	    destRowPtr += destPtr->pixelsPerRow;
	}
    } else {
	Blt_Pixel *destRowPtr;
	unsigned char *srcRowPtr;
	int y;

	srcRowPtr = slot->bitmap.buffer + ((y1 * slot->bitmap.pitch) + x1);
	destRowPtr = Blt_PicturePixel(destPtr, dx, dy);
	for (y = y1; y < y2; y++) {
	    Blt_Pixel *dp;
	    unsigned char *sp;
	    int x;

	    for (dp = destRowPtr, sp = srcRowPtr, x = x1; x < x2; 
		 x++, sp++, dp++) {
		if (*sp != 0x0) {
		    Blt_Pixel color;

		    color.u32 = Blt_Paintbrush_GetColor(brushPtr, x, y);
		    BlendPixel(dp, &color, *sp);
		}
	    }
	    srcRowPtr += slot->bitmap.pitch;
	    destRowPtr += destPtr->pixelsPerRow;
	}
    }
}

static void
CopyGrayGlyph(Pict *destPtr, FT_GlyphSlot slot, int xx, int yy, 
	      Blt_Paintbrush *brushPtr)
{
    int x1, y1, x2, y2;

#ifdef notdef
    fprintf(stderr, "dx=%d, dy=%d\n", dx, dy);
    DebugGlyph(slot);
#endif
    if ((xx >= destPtr->width) || ((xx + slot->bitmap.width) <= 0) ||
	(yy >= destPtr->height) || ((yy + slot->bitmap.rows) <= 0)) {
	return;			/* No portion of the glyph is visible in the
				 * picture. */
    }

    /* By default, set the region to cover the entire glyph */
    x1 = y1 = 0;
    x2 = slot->bitmap.width;
    y2 = slot->bitmap.rows;

    /* Determine the portion of the glyph inside the picture. */

    if (xx < 0) {			/* Left side of glyph overhangs. */
	x1 -= xx;		
	x2 += xx;
	xx = 0;		
    }
    if (yy < 0) {			/* Top of glyph overhangs. */
	y1 -= yy;
	y2 += yy;
	yy = 0;
    }
    if ((xx + x2) > destPtr->width) {	/* Right side of glyph overhangs. */
	x2 = destPtr->width - xx;
    }
    if ((yy + y2) > destPtr->height) {	/* Bottom of glyph overhangs. */
	y2 = destPtr->height - yy;
    }

    {
	Blt_Pixel *destRowPtr;
	unsigned char *srcRowPtr;
	int y;

	srcRowPtr = slot->bitmap.buffer + ((y1 * slot->bitmap.pitch) + x1);
	destRowPtr = Blt_PicturePixel(destPtr, xx, yy);
	for (y = y1; y < y2; y++) {
	    Blt_Pixel *dp;
	    unsigned char *sp;
	    int x;

	    dp = destRowPtr;
	    sp = srcRowPtr;
	    for (x = x1; x < x2; x++, sp++, dp++) {
		if (*sp != 0x0) {
		    int t;
		    Blt_Pixel color;

		    color.u32 = Blt_Paintbrush_GetColor(brushPtr, x, y);
		    color.Alpha = imul8x8(*sp, color.Alpha, t);
		    dp->u32 = color.u32;
		}
	    }
	    srcRowPtr += slot->bitmap.pitch;
	    destRowPtr += destPtr->pixelsPerRow;
	}
    }
}

static void
PaintGrayGlyph(Pict *destPtr, FT_GlyphSlot slot, int xx, int yy, 
	       Blt_Paintbrush *brushPtr)
{
    int x1, y1, x2, y2;

#ifdef notdef
    fprintf(stderr, "dx=%d, dy=%d\n", dx, dy);
    DebugGlyph(slot);
#endif
    if ((xx >= destPtr->width) || ((xx + slot->bitmap.width) <= 0) ||
	(yy >= destPtr->height) || ((yy + slot->bitmap.rows) <= 0)) {
	return;				/* No portion of the glyph is visible
					 * in the picture. */
    }

    /* By default, set the region to cover the entire glyph */
    x1 = y1 = 0;
    x2 = slot->bitmap.width;
    y2 = slot->bitmap.rows;

    /* Determine the portion of the glyph inside the picture. */

    if (xx < 0) {			/* Left side of glyph overhangs. */
	x1 -= xx;		
	x2 += xx;
	xx = 0;		
    }
    if (yy < 0) {			/* Top of glyph overhangs. */
	y1 -= yy;
	y2 += yy;
	yy = 0;
    }
    if ((xx + x2) > destPtr->width) {	/* Right side of glyph overhangs. */
	x2 = destPtr->width - xx;
    }
    if ((yy + y2) > destPtr->height) {	/* Bottom of glyph overhangs. */
	y2 = destPtr->height - yy;
    }

    {
	Blt_Pixel *destRowPtr;
	unsigned char *srcRowPtr;
	int y;

	srcRowPtr = slot->bitmap.buffer + ((y1 * slot->bitmap.pitch) + x1);
	destRowPtr = Blt_PicturePixel(destPtr, xx, yy);
	for (y = y1; y < y2; y++) {
	    Blt_Pixel *dp;
	    unsigned char *sp;
	    int x;

	    dp = destRowPtr;
	    sp = srcRowPtr;
	    for (x = x1; x < x2; x++, sp++, dp++) {
		if (*sp != 0x0) {
		    Blt_Pixel color;

		    color.u32 = Blt_Paintbrush_GetColor(brushPtr, x, y);
		    BlendPixel(dp, &color, *sp);
		}
	    }
	    srcRowPtr += slot->bitmap.pitch;
	    destRowPtr += destPtr->pixelsPerRow;
	}
    }
}

static void
CopyMonoGlyph(Pict *destPtr, FT_GlyphSlot slot, int xx, int yy,
	  Blt_Paintbrush *brushPtr)
{
    int x1, y1, x2, y2;

#ifdef notdef
    fprintf(stderr, "dx=%d, dy=%d\n", dx, dy);
    DebugGlyph(slot);
#endif
    
    if ((xx >= destPtr->width) || ((xx + slot->bitmap.width) <= 0) ||
	(yy >= destPtr->height) || ((yy + slot->bitmap.rows) <= 0)) {
	return;				/* No portion of the glyph is visible
					 * in the picture. */
    }
    /* By default, set the region to cover the entire glyph */
    x1 = y1 = 0;
    x2 = slot->bitmap.width;
    y2 = slot->bitmap.rows;

    /* Determine the portion of the glyph inside the picture. */

    if (xx < 0) {			/* Left side of glyph overhangs. */
	x1 -= xx;		
	x2 += xx;
	xx = 0;		
    }
    if (yy < 0) {			/* Top of glyph overhangs. */
	y1 -= yy;
	y2 += yy;
	yy = 0;
    }
    if ((xx + x2) > destPtr->width) {	/* Right side of glyph overhangs. */
	x2 = destPtr->width - xx;
    }
    if ((yy + y2) > destPtr->height) {	/* Bottom of glyph overhangs. */
	y2 = destPtr->height - yy;
    }
    {
	Blt_Pixel *destRowPtr;
	unsigned char *srcRowPtr;
	int y;

	srcRowPtr = slot->bitmap.buffer + (y1 * slot->bitmap.pitch);
	destRowPtr = Blt_PicturePixel(destPtr, xx, yy);
	for (y = y1; y < y2; y++) {
	    Blt_Pixel *dp;
	    int x;
	    
	    dp = destRowPtr;
	    for (x = x1; x < x2; x++, dp++) {
		int pixel;

		pixel = srcRowPtr[x >> 3] & (1 << (7 - (x & 0x7)));
		if (pixel != 0x0) {
		    dp->u32 = Blt_Paintbrush_GetColor(brushPtr, x, y);
		}
	    }
	    srcRowPtr += slot->bitmap.pitch;
	    destRowPtr += destPtr->pixelsPerRow;
	}
    } 
}

static int 
ScaleFont(Tcl_Interp *interp, TextFont *fontPtr, FT_F26Dot6 size)
{
    FT_Error ftError;
    int xdpi, ydpi;

    Blt_ScreenDPI(Tk_MainWindow(interp), &xdpi, &ydpi);
    ftError = FT_Set_Char_Size(fontPtr->face, size, size, xdpi, ydpi);
    if (ftError) {
	Tcl_AppendResult(interp, "can't set font size to \"", Blt_Itoa(size), 
		"\": ", FtError(ftError), (char *)NULL);
	return TCL_ERROR;
    }
    fontPtr->height  = fontPtr->face->size->metrics.height >> 6;
    fontPtr->ascent  = fontPtr->face->size->metrics.ascender >> 6;
    fontPtr->descent = fontPtr->face->size->metrics.descender >> 6;
    return TCL_OK;
}

static void
RotateFont(TextFont *fontPtr, float angle)
{
    /* Set up the transformation matrix. */
    double theta; 

    theta = angle * DEG2RAD;
    fontPtr->matrix.yy = fontPtr->matrix.xx = (FT_Fixed)(cos(theta) * 65536.0);
    fontPtr->matrix.yx = (FT_Fixed)(sin(theta) * 65536.0);
    fontPtr->matrix.xy = -fontPtr->matrix.yx;
}

static TextFont *
OpenFont(Tcl_Interp *interp, Tcl_Obj *objPtr, size_t fontSize) 
{
    TextFont *fontPtr;
    FT_Error ftError;
    FT_Face face;
    const char *fileName, *fontName;
    Tcl_Obj *fileObjPtr;

    fontName = Tcl_GetString(objPtr);
    fileObjPtr = NULL;
    face = NULL;
    if (fontName[0] == '@') {
	fileName = fontName + 1;
	fontSize <<= 6;
    } else {
	double size;

	fileObjPtr = Blt_Font_GetFile(interp, objPtr, &size);
	if (fileObjPtr == NULL) {
	    return NULL;
	}
	if (fontSize == 0) {
	    fontSize = (int)(size * 64.0 + 0.5);
	}
	Tcl_IncrRefCount(fileObjPtr);
	fileName = Tcl_GetString(fileObjPtr);
    }
    ftError = FT_New_Face(ftLibrary, fileName, 0, &face);
    if (ftError) {
	Tcl_AppendResult(interp, "can't create face from font file \"", 
		fileName, "\": ", FtError(ftError), (char *)NULL);
	goto error;
    }
    if (!FT_IS_SCALABLE(face)) {
	Tcl_AppendResult(interp, "can't use font \"", fontName, 
			 "\": font isn't scalable.", (char *)NULL);
	goto error;
    }
    if (fileObjPtr != NULL) {
	Tcl_DecrRefCount(fileObjPtr);
    }
    fontPtr = Blt_AssertCalloc(1, sizeof(TextFont));
    fontPtr->face = face;
    if (ScaleFont(interp, fontPtr, fontSize) != TCL_OK) {
	Blt_Free(fontPtr);
	goto error;
    }
    RotateFont(fontPtr, 0.0f);		/* Initializes the rotation matrix. */
    return fontPtr;
 error:
    if (fileObjPtr != NULL) {
	Tcl_DecrRefCount(fileObjPtr);
    }
    if (face != NULL) {
	FT_Done_Face(face);
    }	
    return NULL;
}

static void
CloseFont(TextFont *fontPtr) 
{
#ifdef HAVE_LIBXFT
    if (fontPtr->xftFont != NULL) {
	XftUnlockFace(fontPtr->xftFont);
    } else {
	FT_Done_Face(fontPtr->face);
    }
#else
    FT_Done_Face(fontPtr->face);
#endif /* HAVE_LIBXFT */ 
    Blt_Free(fontPtr);
}


static int
PaintText(Pict *destPtr, TextFont *fontPtr, const char *string,
	  size_t length, int x, int y, int kerning, Blt_Paintbrush *brushPtr)
{
    FT_Error ftError;
    int h;

    FT_Vector pen;			/* Untransformed origin  */
    const char *p, *pend;
    FT_GlyphSlot slot;
    FT_Face face;			/* Face object. */  

    h = destPtr->height;
    face = fontPtr->face;
    slot = face->glyph;
    int yy;
    int previous;

    if (destPtr->flags & BLT_PIC_ASSOCIATED_COLORS) {
	Blt_UnassociateColors(destPtr);
    }
#ifdef notdef
    DebugFace(face);
#endif
    yy = y;
    previous = -1;
    FT_Set_Transform(face, &fontPtr->matrix, NULL);
    pen.y = (h - y) << 6;
    pen.x = x << 6;
    for (p = string, pend = p + length; p < pend; p++) {
	int current;

	current = FT_Get_Char_Index(face, *p);
	if ((kerning) && (previous >= 0)) { 
	    FT_Vector delta; 
		
	    FT_Get_Kerning(face, previous, current, FT_KERNING_DEFAULT, &delta);
	    pen.x += delta.x; 
	} 
	FT_Set_Transform(face, &fontPtr->matrix, &pen);
	previous = current;
	/* load glyph image into the slot (erase previous one) */  
	ftError = FT_Load_Glyph(face, current, FT_LOAD_DEFAULT); 
	if (ftError) {
	    Blt_Warn("can't load character \"%c\": %s\n", *p, 
		     FtError(ftError));
	    continue;                 /* Ignore errors */
	}
	ftError = FT_Render_Glyph(slot, FT_RENDER_MODE_NORMAL);
	if (ftError) {
	    Blt_Warn("can't render glyph \"%c\": %s\n", *p, FtError(ftError));
	    continue;                 /* Ignore errors */
	}
	switch(slot->bitmap.pixel_mode) {
	case FT_PIXEL_MODE_MONO:
	    CopyMonoGlyph(destPtr, slot, pen.x >> 6, yy - slot->bitmap_top, 
		brushPtr);
	    break;
	case FT_PIXEL_MODE_LCD:
	case FT_PIXEL_MODE_LCD_V:
	case FT_PIXEL_MODE_GRAY:
#ifdef notdef
	    fprintf(stderr, "h=%d, slot->bitmap_top=%d\n", h, slot->bitmap_top);
#endif
	    PaintGrayGlyph(destPtr, slot, slot->bitmap_left, 
			   h - slot->bitmap_top, brushPtr);
	case FT_PIXEL_MODE_GRAY2:
	case FT_PIXEL_MODE_GRAY4:
	    break;
	}
	pen.x += slot->advance.x; 
	pen.y += slot->advance.y;
	previous = -1;
    }	
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * TextOp --
 *
 * Results:
 *	Returns a standard TCL return value.
 *
 * Side effects:
 *	None.
 *
 *	image draw text string x y switches 
 *---------------------------------------------------------------------------
 */
static int
TextOp(ClientData clientData, Tcl_Interp *interp, int objc, 
       Tcl_Obj *const *objv) 
{ 
    Pict *destPtr = clientData; /* Picture. */
    Blt_Shadow *shadowPtr;
    TextFont *fontPtr;
    TextSwitches switches;
    const char *string;
    int length;
    int x, y;

    string = Tcl_GetStringFromObj(objv[3], &length);
    if ((Tcl_GetIntFromObj(interp, objv[4], &x) != TCL_OK) ||
	(Tcl_GetIntFromObj(interp, objv[5], &y) != TCL_OK)) {
	return TCL_ERROR;
    }
    /* Process switches  */
    switches.anchor = TK_ANCHOR_NW;
    switches.angle = 0.0;
    Blt_Shadow_Set(&switches.shadow, 0, 0, 0x0, 0xA0);
    if (Blt_Paintbrush_GetFromString(interp, "black", &switches.brushPtr) 
	!= TCL_OK) {
	return TCL_ERROR;
    }
    switches.fontObjPtr = Tcl_NewStringObj("Arial 12", -1);
    switches.fontSize = 0;
    switches.kerning = FALSE;
    switches.justify = TK_JUSTIFY_LEFT;
    shadowPtr = &switches.shadow;
    if (Blt_ParseSwitches(interp, textSwitches, objc - 6, objv + 6, &switches, 
	BLT_SWITCH_DEFAULTS) < 0) {
	return TCL_ERROR;
    }
    fontPtr = OpenFont(interp, switches.fontObjPtr, switches.fontSize);
    if (fontPtr == NULL) {
	return TCL_ERROR;
    }
    if (destPtr->flags & BLT_PIC_ASSOCIATED_COLORS) {
	Blt_UnassociateColors(destPtr);
    }
    if (switches.angle != 0.0) {
	TextStyle ts;
	TextLayout *layoutPtr;

	Blt_Ts_InitStyle(ts);
	Blt_Ts_SetJustify(ts, switches.justify);
	layoutPtr = CreateSimpleTextLayout(fontPtr, string, length, &ts);
	if (fontPtr->face->face_flags & FT_FACE_FLAG_SCALABLE) {
	    double rw, rh;
	    int i;

	    Blt_RotateStartingTextPositions(layoutPtr, switches.angle);
	    Blt_GetBoundingBox(layoutPtr->width, layoutPtr->height, 
		switches.angle, &rw, &rh, (Point2d *)NULL);
	    Blt_TranslateAnchor(x, y, (int)(rw), (int)(rh), switches.anchor, 
		&x, &y);
	    RotateFont(fontPtr, switches.angle);
	    for (i = 0; i < layoutPtr->numFragments; i++) {
		TextFragment *fp;

		fp = layoutPtr->fragments + i;
		PaintText(destPtr, fontPtr, fp->text, fp->count, x + fp->sx, 
			y + fp->sy, switches.kerning, switches.brushPtr);
	    }
	} else {
	    Blt_Picture tmp;
	    Pict *rotPtr;
	    int i;
	    
	    tmp = Blt_CreatePicture(layoutPtr->width, layoutPtr->height);
	    Blt_BlankPicture(tmp, 0x00FF0000);
	    for (i = 0; i < layoutPtr->numFragments; i++) {
		TextFragment *fp;

		fp = layoutPtr->fragments + i;
		PaintText(tmp, fontPtr, fp->text, fp->count, fp->sx, 
			fp->sy, switches.kerning, switches.brushPtr);
	    }
	    rotPtr = Blt_RotatePicture(tmp, switches.angle);
	    Blt_FreePicture(tmp);
	    Blt_TranslateAnchor(x, y, rotPtr->width, rotPtr->height, 
		switches.anchor, &x, &y);
	    Blt_BlendRegion(destPtr, rotPtr, 0, 0, rotPtr->width, 
			    rotPtr->height, x, y);
	    Blt_FreePicture(rotPtr);
	}
	Blt_Free(layoutPtr);
    } else { 
	int w, h;
	TextStyle ts;
	TextLayout *layoutPtr;

	Blt_Ts_InitStyle(ts);
	Blt_Ts_SetJustify(ts, switches.justify);
	layoutPtr = CreateSimpleTextLayout(fontPtr, string, length, &ts);
	Blt_TranslateAnchor(x, y, layoutPtr->width, layoutPtr->height, 
		switches.anchor, &x, &y);
	w = layoutPtr->width;
	h = layoutPtr->height;
	if ((w > 1) && (h > 1) && (switches.shadow.width > 0)) {
	    Blt_Pixel color;
	    Pict *tmpPtr;
	    int extra;
	    Blt_Paintbrush brush;
	    int i;

	    extra = 2 * shadowPtr->width;
	    tmpPtr = Blt_CreatePicture(w + extra, h + extra);
	    color.u32 = 0x0;
	    Blt_BlankPicture(tmpPtr, color.u32);
	    Blt_Paintbrush_Init(&brush);
	    Blt_Paintbrush_SetColor(&brush, shadowPtr->color.u32);
	    for (i = 0; i < layoutPtr->numFragments; i++) {
		TextFragment *fp;

		fp = layoutPtr->fragments + i;
		PaintText(tmpPtr, fontPtr, fp->text, fp->count, 
			  fp->x + shadowPtr->width, 
			  fp->y + shadowPtr->width, 
			  switches.kerning, &brush);
	    }
	    Blt_BlurPicture(tmpPtr, tmpPtr, shadowPtr->width, 3);
#ifdef notdef
	    for (i = 0; i < layoutPtr->numFragments; i++) {
		TextFragment *fp;

		fp = layoutPtr->fragments + i;
		PaintText(tmpPtr, fontPtr, fp->text, fp->count, 
		    fp->x + shadowPtr->width - shadowPtr->offset,
		    fp->y + shadowPtr->width - shadowPtr->offset, 
                    switches.kerning, switches.brushPtr);
	    }
#endif
	    Blt_BlendRegion(destPtr, tmpPtr, 0, 0, tmpPtr->width, 
			    tmpPtr->height, x, y);
	    fprintf(stderr, "tmp width=%d height=%d, w=%d h=%d\n",
		    tmpPtr->width, tmpPtr->height, w, h);
	    Blt_FreePicture(tmpPtr);
	} else {
	    int i;

	    y += fontPtr->ascent;
	    for (i = 0; i < layoutPtr->numFragments; i++) {
		TextFragment *fp;

		fp = layoutPtr->fragments + i;
		PaintText(destPtr, fontPtr, fp->text, fp->count, 
			x + fp->x, y + fp->y, switches.kerning, 
			switches.brushPtr);
	    }
	}
	Blt_Free(layoutPtr);
    }
    CloseFont(fontPtr);
    Blt_FreeSwitches(textSwitches, (char *)&switches, 0);
    return TCL_OK;
}

int
Blt_PictureTextOpInit(Tcl_Interp *interp)
{
    FT_Error ftError;

    ftError = FT_Init_FreeType(&ftLibrary);
    if (ftError) {
	Tcl_AppendResult(interp, "can't initialize freetype library: ", 
			 FtError(ftError), (char *)NULL);
	return TCL_ERROR;
    }
    if (Blt_PictureRegisterProc(interp, "text", TextOp) != TCL_OK) {
	return TCL_ERROR;
    }
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * Blt_picture_text_Init --
 *
 *	This procedure is invoked to initialize the "text" operation
 *	of picture instance commands.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Creates the new command and adds a new entry into a global Tcl
 *	associative array.
 *
 *---------------------------------------------------------------------------
 */
int
Blt_picture_text_Init(Tcl_Interp *interp)
{
#ifdef USE_TCL_STUBS
    if (Tcl_InitStubs(interp, TCL_VERSION_COMPILED, PKG_ANY) == NULL) {
	return TCL_ERROR;
    };
#endif
#ifdef USE_TK_STUBS
    if (Tk_InitStubs(interp, TK_VERSION_COMPILED, PKG_ANY) == NULL) {
	return TCL_ERROR;
    };
#endif
#ifdef USE_BLT_STUBS
    if (Blt_InitTclStubs(interp, BLT_VERSION, PKG_EXACT) == NULL) {
	return TCL_ERROR;
    };
    if (Blt_InitTkStubs(interp, BLT_VERSION, PKG_EXACT) == NULL) {
	return TCL_ERROR;
    };
#else
    if (Tcl_PkgRequire(interp, "blt_tcl", BLT_VERSION, PKG_EXACT) == NULL) {
	return TCL_ERROR;
    }
    if (Tcl_PkgRequire(interp, "blt_tk", BLT_VERSION, PKG_EXACT) == NULL) {
	return TCL_ERROR;
    }
#endif    
    if (Blt_PictureTextOpInit(interp) != TCL_OK) {
	return TCL_ERROR;
    }
    if (Tcl_PkgProvide(interp, "blt_picture_text", BLT_VERSION) != TCL_OK) { 
	return TCL_ERROR;
    }
    return TCL_OK;
}

