/* -*- mode: c; c-basic-offset: 4; indent-tabs-mode: nil -*- */

/*
 * bltFont.h --
 *
 *	Copyright 1993-2004 George A Howlett.
 *
 *	Permission is hereby granted, free of charge, to any person
 *	obtaining a copy of this software and associated documentation
 *	files (the "Software"), to deal in the Software without
 *	restriction, including without limitation the rights to use,
 *	copy, modify, merge, publish, distribute, sublicense, and/or
 *	sell copies of the Software, and to permit persons to whom the
 *	Software is furnished to do so, subject to the following
 *	conditions:
 *
 *	The above copyright notice and this permission notice shall be
 *	included in all copies or substantial portions of the
 *	Software.
 *
 *	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY
 *	KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
 *	WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR
 *	PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS
 *	OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR
 *	OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
 *	OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 *	SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#ifndef _BLT_FONT_H
#define _BLT_FONT_H

#define FONT_ITALIC	(1<<0)
#define FONT_BOLD	(1<<1)

typedef struct _Blt_Font *Blt_Font;
typedef struct _Blt_FontClass Blt_FontClass;

typedef struct {
    int ascent;				/* The amount in pixels that the
					 * tallest letter sticks up above the
					 * baseline, plus any extra blank
					 * space added by the designer of the
					 * font. */
    int descent;			/* The largest amount in pixels that
					 * any letter sticks below the
					 * baseline, plus any extra blank
					 * space added by the designer of the
					 * font. */
    int linespace;			/* The sum of the ascent and descent.
					 * How far apart two lines of text in
					 * the same font should be placed so
					 * that none of the characters in one
					 * line overlap any of the characters
					 * in the other line. */
    int tabWidth;			/* Width of tabs in this font
					 * (pixels). */
    int	underlinePos;			/* Offset from baseline to origin of
					 * underline bar (used for drawing
					 * underlines on a non-underlined
					 * font). */
    int underlineHeight;		/* Height of underline bar (used for
					 * drawing underlines on a
					 * non-underlined font). */
} Blt_FontMetrics;

typedef const char *(Blt_Font_NameProc)(Blt_Font font);
typedef void (Blt_Font_GetMetricsProc)(Blt_Font font, 
	Blt_FontMetrics *metricsPtr);
typedef Font (Blt_Font_IdProc)(Blt_Font font);
typedef int (Blt_Font_TextWidthProc)(Blt_Font font, const char *string, 
	int numBytes);
typedef void (Blt_Font_FreeProc)(Blt_Font font);
typedef int (Blt_Font_MeasureProc)(Blt_Font font, const char *text, 
	int numBytes, int maxLength, int flags, int *lengthPtr);
typedef void (Blt_Font_DrawProc)(Display *display, Drawable drawable, GC gc, 
	Blt_Font font, int depth, float angle, const char *text, int length, 
	int x, int y);
typedef int (Blt_Font_PostscriptNameProc)(Blt_Font font, 
	Tcl_DString *resultPtr);
typedef const char *(Blt_Font_FamilyProc)(Blt_Font font);
typedef int (Blt_Font_CanRotateProc)(Blt_Font font, float angle);
typedef void (Blt_Font_UnderlineProc)(Display *display, Drawable drawable, 
	GC gc, Blt_Font font, const char *text, int textLen, int x, int y, 
	int first, int last, int xMax);

struct _Blt_FontClass {
    int type;				/* Indicates the type of font used. */
    Blt_Font_CanRotateProc *canRotateProc;
    Blt_Font_DrawProc *drawProc;
    Blt_Font_FamilyProc *familyProc;
    Blt_Font_FreeProc *freeProc;
    Blt_Font_GetMetricsProc *getMetricsProc;
    Blt_Font_IdProc *idProc;
    Blt_Font_MeasureProc *measureProc;
    Blt_Font_NameProc *nameProc;
    Blt_Font_PostscriptNameProc *psNameProc;
    Blt_Font_TextWidthProc *textWidthProc;
    Blt_Font_UnderlineProc *underlineProc;
};

struct _Blt_Font {
    Blt_FontClass *classPtr;
    Tcl_Interp *interp;
    Display *display;
    TkRegion rgn;
    void *clientData;
};

#define Blt_Font_Name(f)	(*(f)->classPtr->nameProc)(f)
#define Blt_Font_Id(f)		(*(f)->classPtr->idProc)(f)
#define Blt_Font_Measure(f,s,l,ml,fl,lp) \
	(*(f)->classPtr->measureProc)(f,s,l,ml,fl,lp)
#define Blt_Font_Draw(d,w,gc,f,dp,a,t,l,x,y)		\
	(*(f)->classPtr->drawProc)(d,w,gc,f,dp,a,t,l,x,y)
#define Blt_Font_PostscriptName(f,rp) \
	(*(f)->classPtr->psNameProc)(f,rp)
#define Blt_Font_Family(f)	(*(f)->classPtr->familyProc)(f)
#define Blt_Font_CanRotate(f,a) (*(f)->classPtr->canRotateProc)(f,a)
#define Blt_Font_Free(f)	(*(f)->classPtr->freeProc)(f)
#define Blt_Font_Underline(d,w,g,f,s,l,x,y,a,b,m)		\
	(*(f)->classPtr->underlineProc)(d,w,g,f,s,l,x,y,a,b,m)
#define Blt_Font_SetClipRegion(f,r) \
	((f)->rgn = (r))

BLT_EXTERN Blt_Font Blt_GetFont(Tcl_Interp *interp, Tk_Window tkwin, 
	const char *string);
BLT_EXTERN Blt_Font Blt_AllocFontFromObj(Tcl_Interp *interp, Tk_Window tkwin, 
	Tcl_Obj *objPtr);
BLT_EXTERN void Blt_DrawWithEllipsis(Tk_Window tkwin, Drawable drawable,
	GC gc, Blt_Font font, int depth, float angle, const char *string, 
	int numBytes, int x, int y, int maxLength);
BLT_EXTERN Blt_Font Blt_GetFontFromObj(Tcl_Interp *interp, Tk_Window tkwin,
	Tcl_Obj *objPtr);
BLT_EXTERN void Blt_Font_GetMetrics(Blt_Font font, Blt_FontMetrics *fmPtr);
BLT_EXTERN int Blt_TextWidth(Blt_Font font, const char *string, int length);
BLT_EXTERN Tcl_Interp *Blt_Font_GetInterp(Blt_Font font);

BLT_EXTERN Tcl_Obj *Blt_Font_GetFile(Tcl_Interp *interp, Tcl_Obj *objPtr, 
	double *sizePtr);

#endif /* _BLT_FONT_H */
