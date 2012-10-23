
/*
 * bltCanvEps.c --
 *
 * This file implements an Encapsulated PostScript item for canvas
 * widgets.
 *
 *	Copyright 1991-2004 George A Howlett.
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

/*
 * To do:
 *
 *	1. Add -rotate option.  Allow arbitrary rotation of image and EPS.
 *	2. Draw pictures instead of photos. This will eliminate the need
 *	   to create hidden photo images.
 *	3. Create a spiffy demo that lets you edit your page description.
 */
#define USE_OLD_CANVAS	1

#define BUILD_BLT_TK_PROCS 1
#include "bltInt.h"

#ifdef HAVE_CTYPE_H
#  include <ctype.h>
#endif /* HAVE_CTYPE_H */

#ifdef HAVE_STRING_H
#  include <string.h>
#endif /* HAVE_STRING_H */

#include <bltAlloc.h>
#include "bltMath.h"
#include "bltChain.h"
#include "bltPicture.h"
#include "bltPs.h"
#include "bltImage.h"
#include "bltPainter.h"

#undef HAVE_TIFF_H
#ifdef HAVE_TIFF_H
#include "tiff.h"
#endif
#include <fcntl.h>

#if defined(_MSC_VER) || defined(__BORLANDC__) 
#include <io.h>
#define open _open
#define close _close
#define write _write
#define unlink _unlink
#define lseek _lseek
#define fdopen _fdopen
#define fcntl _fcntl
#ifdef _MSC_VER
#define O_RDWR	_O_RDWR 
#define O_CREAT	_O_CREAT
#define O_TRUNC	_O_TRUNC
#define O_EXCL	_O_EXCL
#endif /* _MSC_VER */
#endif /* _MSC_VER || __BORLANDC__ */

#define DEBUG_READER 0
#ifndef WIN32
#define PurifyPrintf printf
#endif
#define PS_PREVIEW_EPSI	0
#define PS_PREVIEW_WMF	1
#define PS_PREVIEW_TIFF	2

#define MAX_EPS_LINE_LENGTH 255		/* Maximum line length for a EPS
					 * file */
/*
 * ParseInfo --
 *
 *	This structure is used to pass PostScript file information around to
 *	various routines while parsing the EPS file.
 */
typedef struct {
    int maxBytes;			/* Maximum length of PostScript
					 * code.  */
    int lineNumber;			/* Current line number of EPS file */
    char line[MAX_EPS_LINE_LENGTH + 1];	/* Buffer to contain a single line
					 * from the PostScript file. */
    unsigned char hexTable[256];	/* Table for converting ASCII hex
					 * digits to values */

    char *nextPtr;			/* Pointer to the next character to
					 * process on the current line.  If
					 * NULL (or if nextPtr points a NULL
					 * byte), this indicates the the next
					 * line needs to be read. */
    FILE *f;				/*  */
} ParseInfo;

#define DEF_ANCHOR		"nw"
#define DEF_OUTLINE_COLOR	RGB_BLACK
#define DEF_BORDERWIDTH		STD_BORDERWIDTH
#define DEF_FILE_NAME		(char *)NULL
#define DEF_FONT		STD_FONT
#define DEF_FILL_COLOR     	STD_NORMAL_FOREGROUND
#define DEF_HEIGHT		"0"
#define DEF_IMAGE_NAME		(char *)NULL
#define DEF_JUSTIFY		"center"
#define DEF_QUICK_RESIZE	"no"
#define DEF_RELIEF		"sunken"
#define DEF_SHOW_IMAGE		"yes"
#define DEF_STIPPLE		(char *)NULL
#define DEF_TAGS		(char *)NULL
#define DEF_TITLE		(char *)NULL
#define DEF_TITLE_ANCHOR	"center"
#define DEF_TITLE_COLOR		RGB_BLACK
#define DEF_WIDTH		"0"

/*
 * Information used for parsing configuration specs:
 */

static Tk_CustomOption tagsOption;

BLT_EXTERN Tk_CustomOption bltDistanceOption;

/*
 * The structure below defines the record for each EPS item.
 */
typedef struct {
    Tk_Item item;			/* Generic stuff that's the same for
					 * all types.  MUST BE FIRST IN
					 * STRUCTURE. */
    Tk_Canvas canvas;			/* Canvas containing the EPS item. */
    int lastWidth, lastHeight;		/* Last known dimensions of the EPS
					 * item.  This is used to know if the
					 * picture preview needs to be
					 * resized. */
    Tcl_Interp *interp;
    FILE *psFile;			/* File pointer to Encapsulated
					 * PostScript file. We'll hold this as
					 * long as the EPS item is using this
					 * file. */
    unsigned int psStart;		/* File offset of PostScript code. */
    unsigned int psLength;		/* Length of PostScript code. If zero,
					 * indicates to read to EOF. */
    unsigned int wmfStart;		/* File offset of Windows Metafile
					 * preview.  */
    unsigned int wmfLength;		/* Length of WMF portion in bytes. If
					 * zero, indicates there is no WMF
					 * preview. */
    unsigned int tiffStart;		/* File offset of TIFF preview. */
    unsigned int tiffLength;		/* Length of TIFF portion in bytes. If
					 * zero, indicates there is no TIFF *
					 * preview. */
    const char *previewImageName;
    int previewFormat;

    Tk_Image preview;			/* A Tk photo image provided as a
					 * preview of the EPS contents. This
					 * image supersedes any EPS preview
					 * embedded PostScript preview
					 * (EPSI). */
    Blt_Painter painter;
    Blt_Picture original;		/* The original photo or PostScript
					 * preview image converted to a
					 * picture. */
    int origFromPicture;
    Blt_Picture picture;		/* Holds resized preview image.
					 * Created and deleted internally. */
    int firstLine, lastLine;		/* First and last line numbers of the
					 * PostScript preview.  They are used
					 * to skip over the preview when
					 * encapsulating PostScript for the
					 * canvas item. */
    GC fillGC;				/* Graphics context to fill background
					 * of image outline if no preview
					 * image was present. */
    int llx, lly, urx, ury;		/* Lower left and upper right
					 * coordinates of PostScript bounding
					 * box, retrieved from file's
					 * "BoundingBox:" field. */
    const char *title;			/* Title, retrieved from the file's
					 * "Title:" field, to be displayed
					 * over the top of the EPS preview
					 * (malloc-ed).  */
    Tcl_DString ds;		/* Contains the encapsulated
					 * PostScript. */

    /* User configurable fields */

    double x, y;			/* Requested anchor in canvas
					 * coordinates of the item */
    Tk_Anchor anchor;

    Region2d bb;

    const char *fileName;		/* Name of the encapsulated PostScript
					 * file.  If NULL, indicates that no
					 * EPS file has be successfully
					 * loaded yet. */
    const char *reqTitle;		/* Title to be displayed in the EPS
					 * item.  Supersedes the title found
					 * in the EPS file. If NULL, indicates
					 * that the title found in the EPS
					 * file should be used. */
    int width, height;			/* Requested dimension of EPS item in
					 * canvas coordinates.  If non-zero,
					 * this overrides the dimension
					 * computed from the "%%BoundingBox:"
					 * specification in the EPS file
					 * used. */
    int showImage;			/* Indicates if the image or the
					 * outline rectangle should be
					 * displayed */

    int quick;
    unsigned int flags;

    XColor *fillColor;			/* Fill color of the image outline. */

    Tk_3DBorder border;			/* Outline color */

    int borderWidth;
    int relief;

    TextStyle titleStyle;		/* Font, color, etc. for title */
    Blt_Font font;		
    Pixmap stipple;			/* Stipple for image fill */

    ClientData tiffPtr;
#ifdef WIN32
    HENHMETAFILE *hMetaFile;		/* Windows metafile. */
#endif
} EpsItem;

static int StringToFont(ClientData clientData, Tcl_Interp *interp,
	Tk_Window tkwin, const char *string, char *widgRec, int offset);
static char *FontToString (ClientData clientData, Tk_Window tkwin, 
	char *widgRec, int offset, Tcl_FreeProc **proc);

static Tk_CustomOption bltFontOption =
{
    StringToFont, FontToString, (ClientData)0
};


static Tk_ConfigSpec configSpecs[] =
{
    {TK_CONFIG_ANCHOR, (char *)"-anchor", (char *)NULL, (char *)NULL,
	DEF_ANCHOR, Blt_Offset(EpsItem, anchor),
	TK_CONFIG_DONT_SET_DEFAULT},
    {TK_CONFIG_SYNONYM, (char *)"-bd", "borderWidth", (char *)NULL, (char *)NULL, 0, 0},
    {TK_CONFIG_CUSTOM, (char *)"-borderwidth", "borderWidth", (char *)NULL,
	DEF_BORDERWIDTH, Blt_Offset(EpsItem, borderWidth),
	TK_CONFIG_DONT_SET_DEFAULT, &bltDistanceOption},
    {TK_CONFIG_STRING, (char *)"-file", (char *)NULL, (char *)NULL,
	DEF_FILE_NAME, Blt_Offset(EpsItem, fileName), TK_CONFIG_NULL_OK},
    {TK_CONFIG_CUSTOM, (char *)"-font", "font", "Font",
        DEF_FONT, Blt_Offset(EpsItem, font), 0, &bltFontOption},
    {TK_CONFIG_COLOR, (char *)"-fill", "fill", (char *)NULL,
	DEF_FILL_COLOR, Blt_Offset(EpsItem, fillColor), 0},
    {TK_CONFIG_CUSTOM, (char *)"-height", (char *)NULL, (char *)NULL,
	DEF_HEIGHT, Blt_Offset(EpsItem, height),
	TK_CONFIG_DONT_SET_DEFAULT, &bltDistanceOption},
    {TK_CONFIG_STRING, (char *)"-image", (char *)NULL, (char *)NULL,
	DEF_IMAGE_NAME, Blt_Offset(EpsItem, previewImageName),
	TK_CONFIG_NULL_OK},
    {TK_CONFIG_JUSTIFY, (char *)"-justify", "justify", "Justify",
	DEF_JUSTIFY, Blt_Offset(EpsItem, titleStyle.justify),
	TK_CONFIG_DONT_SET_DEFAULT},
    {TK_CONFIG_BORDER, (char *)"-outline", "outline", (char *)NULL,
	DEF_OUTLINE_COLOR, Blt_Offset(EpsItem, border),
	TK_CONFIG_NULL_OK},
    {TK_CONFIG_BOOLEAN, (char *)"-quick", "quick", "Quick",
	DEF_QUICK_RESIZE, Blt_Offset(EpsItem, quick),
	TK_CONFIG_DONT_SET_DEFAULT},
    {TK_CONFIG_RELIEF, (char *)"-relief", (char *)NULL, (char *)NULL,
	DEF_RELIEF, Blt_Offset(EpsItem, relief),
	TK_CONFIG_DONT_SET_DEFAULT},
    {TK_CONFIG_BOOLEAN, (char *)"-showimage", "showImage", "ShowImage",
	DEF_SHOW_IMAGE, Blt_Offset(EpsItem, showImage),
	TK_CONFIG_DONT_SET_DEFAULT},
    {TK_CONFIG_BITMAP, (char *)"-stipple", (char *)NULL, (char *)NULL,
	DEF_STIPPLE, Blt_Offset(EpsItem, stipple), TK_CONFIG_NULL_OK},
    {TK_CONFIG_CUSTOM, (char *)"-tags", (char *)NULL, (char *)NULL,
	DEF_TAGS, 0, TK_CONFIG_NULL_OK, &tagsOption},
    {TK_CONFIG_STRING, (char *)"-title", (char *)NULL, (char *)NULL,
	DEF_TITLE, Blt_Offset(EpsItem, reqTitle), TK_CONFIG_NULL_OK},
    {TK_CONFIG_ANCHOR, (char *)"-titleanchor", (char *)NULL, (char *)NULL,
	DEF_TITLE_ANCHOR, Blt_Offset(EpsItem, titleStyle.anchor), 0},
    {TK_CONFIG_COLOR, (char *)"-titlecolor", (char *)NULL, (char *)NULL,
	DEF_TITLE_COLOR, Blt_Offset(EpsItem, titleStyle.color), 0},
    {TK_CONFIG_CUSTOM, (char *)"-width", (char *)NULL, (char *)NULL,
	DEF_WIDTH, Blt_Offset(EpsItem, width),
	TK_CONFIG_DONT_SET_DEFAULT, &bltDistanceOption},
    {TK_CONFIG_END, (char *)NULL, (char *)NULL, (char *)NULL,
	(char *)NULL, 0, 0}
};

/* Prototypes for procedures defined in this file: */
static Tk_ImageChangedProc ImageChangedProc;
static Tk_ItemCoordProc CoordProc;
static Tk_ItemAreaProc AreaProc;
static Tk_ItemPointProc PointProc;
static Tk_ItemConfigureProc ConfigureProc;
static Tk_ItemCreateProc CreateProc;
static Tk_ItemDeleteProc DeleteProc;
static Tk_ItemDisplayProc DisplayProc;
static Tk_ItemScaleProc ScaleProc;
static Tk_ItemTranslateProc TranslateProc;
static Tk_ItemPostscriptProc PostScriptProc;

static void ComputeBbox(Tk_Canvas canvas, EpsItem *imgPtr);
static int ReadPostScript(Tcl_Interp *interp, EpsItem *itemPtr);


/*
 *---------------------------------------------------------------------------
 *
 * StringToFont --
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
StringToFont(
    ClientData clientData,		/* Indicated how to check distance */
    Tcl_Interp *interp,			/* Interpreter to send results back
					 * to */
    Tk_Window tkwin,			/* Window */
    const char *string,			/* Pixel value string */
    char *widgRec,			/* Widget record */
    int offset)				/* Offset of pixel size in record */
{
    Blt_Font *fontPtr = (Blt_Font *)(widgRec + offset);
    Blt_Font font;

    font = Blt_GetFont(interp, tkwin, string);
    if (font == NULL) {
	return TCL_ERROR;
    }
    *fontPtr = font;
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * FontToString --
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static char *
FontToString(
    ClientData clientData,		/* Not used. */
    Tk_Window tkwin,			/* Not used. */
    char *widgRec,			/* Widget structure record */
    int offset,				/* Offset in widget record */
    Tcl_FreeProc **freeProcPtr)		/* Not used. */
{
    Blt_Font font = *(Blt_Font *)(widgRec + offset);
    const char *string;

    string = Blt_Font_Name(font);
    *freeProcPtr = (Tcl_FreeProc *)TCL_STATIC;
    return (char *)string;
}

static char *
SkipBlanks(ParseInfo *piPtr)
{
    char *s;

    for (s = piPtr->line; isspace(UCHAR(*s)); s++) {
	/*empty*/
    }
    return s;
}

static int
ReadPsLine(ParseInfo *piPtr)
{
    int numBytes;

    numBytes = 0;
    if (ftell(piPtr->f) < piPtr->maxBytes) {
	char *cp;

	cp = piPtr->line;
	while ((*cp = fgetc(piPtr->f)) != EOF) {
	    if (*cp == '\r') {
		continue;
	    }
	    numBytes++;
	    if ((*cp == '\n') || (numBytes >= MAX_EPS_LINE_LENGTH)) {
		break;
	    }
	    cp++;
	}
	if (*cp == '\n') {
	    piPtr->lineNumber++;
	}
	*cp = '\0';
    }
    return numBytes;
}

/*
 *---------------------------------------------------------------------------
 *
 * ReverseBits --
 *
 *	Convert a byte from a X image into PostScript image order.  This
 *	requires not only the nybbles to be reversed but also their bit
 *	values.
 *
 * Results:
 *	The converted byte is returned.
 *
 *---------------------------------------------------------------------------
 */
INLINE static unsigned char
ReverseBits(unsigned char byte)
{
    byte = ((byte >> 1) & 0x55) | ((byte << 1) & 0xaa);
    byte = ((byte >> 2) & 0x33) | ((byte << 2) & 0xcc);
    byte = ((byte >> 4) & 0x0f) | ((byte << 4) & 0xf0);
    return byte;
}

/*
 *---------------------------------------------------------------------------
 *
 * GetHexValue --
 *
 *	Reads the next ASCII hex value from EPS preview image and converts it.
 *
 * Results:
 *	One of three TCL return values is possible.
 *
 *	TCL_OK		the next byte was successfully parsed.
 *	TCL_ERROR	an error occurred processing the next hex value.
 *	TCL_RETURN	"%%EndPreview" line was detected.
 *
 *	The converted hex value is returned via "bytePtr".
 *
 *---------------------------------------------------------------------------
 */
static int
GetHexValue(ParseInfo *piPtr, unsigned char *bytePtr)
{
    char *p;
    unsigned int byte;
    unsigned char a, b;

    p = piPtr->nextPtr;
    if (p == NULL) {

      nextLine:
	if (!ReadPsLine(piPtr)) {
#if DEBUG_READER
	    PurifyPrintf("short file\n");
#endif
	    return TCL_ERROR;		/* Short file */
	}
	if (piPtr->line[0] != '%') {
#if DEBUG_READER
	    PurifyPrintf("line doesn't start with %% (%s)\n", piPtr->line);
#endif
	    return TCL_ERROR;
	}
	if ((piPtr->line[1] == '%') &&
	    (strncmp(piPtr->line + 2, "EndPreview", 10) == 0)) {
#if DEBUG_READER
	    PurifyPrintf("end of preview (%s)\n", piPtr->line);
#endif
	    return TCL_RETURN;
	}
	p = piPtr->line + 1;
    }
    while (isspace((int)*p)) {
	p++;				/* Skip spaces */
    }
    if (*p == '\0') {
	goto nextLine;
    }

    a = piPtr->hexTable[(int)p[0]];
    b = piPtr->hexTable[(int)p[1]];

    if ((a == 0xFF) || (b == 0xFF)) {
#if DEBUG_READER
	PurifyPrintf("not a hex digit (%s)\n", piPtr->line);
#endif
	return TCL_ERROR;
    }
    byte = (a << 4) | b;
    p += 2;
    piPtr->nextPtr = p;
    *bytePtr = byte;
    return TCL_OK;
}


/*
 *---------------------------------------------------------------------------
 *
 * ReadEPSI --
 *
 *	Reads the EPS preview image from the PostScript file, converting it
 *	into a picture.  If an error occurs when parsing the preview, the
 *	preview is silently ignored.
 *
 * Results:
 *	None.
 *
 *---------------------------------------------------------------------------
 */
static void
ReadEPSI(EpsItem *itemPtr, ParseInfo *piPtr)
{
    Blt_Picture picture;
    int width, height, bitsPerPixel, numLines;
    char *dscBeginPreview;

    dscBeginPreview = piPtr->line + 16;
    if (sscanf(dscBeginPreview, "%d %d %d %d", &width, &height, &bitsPerPixel, 
	&numLines) != 4) {
#if DEBUG_READER
	PurifyPrintf("bad %%BeginPreview (%s) format\n", dscBeginPreview);
#endif
	return;
    }
    if (((bitsPerPixel != 1) && (bitsPerPixel != 8)) || (width < 1) ||
	(width > SHRT_MAX) || (height < 1) || (height > SHRT_MAX)) {
#if DEBUG_READER
	PurifyPrintf("Bad %%BeginPreview (%s) values\n", dscBeginPreview);
#endif
	return;			/* Bad "%%BeginPreview:" information */
    }
    itemPtr->firstLine = piPtr->lineNumber;
    Blt_InitHexTable(piPtr->hexTable);
    piPtr->nextPtr = NULL;
    picture = Blt_CreatePicture(width, height);

    if (bitsPerPixel == 8) {
	Blt_Pixel *destRowPtr;
	int y;

	destRowPtr = Blt_PictureBits(picture) + 
	    (height - 1) * Blt_PictureStride(picture);
	for (y = height - 1; y >= 0; y--) {
	    Blt_Pixel *dp;
	    int x;

	    dp = destRowPtr;
	    for (x = 0; x < width; x++, dp++) {
		int result;
		unsigned char byte;

		result = GetHexValue(piPtr, &byte);
		if (result == TCL_ERROR) {
		    goto error;
		}
		if (result == TCL_RETURN) {
		    goto done;
		}
		dp->Red = dp->Green = dp->Blue = ~byte;
		dp->Alpha = ALPHA_OPAQUE;
	    }
	    destRowPtr -= Blt_PictureStride(picture);
	}
    } else if (bitsPerPixel == 1) {
	Blt_Pixel *destRowPtr;
	int y;

	destRowPtr = Blt_PictureBits(picture);
	for (y = 0; y < height; y++) {
	    Blt_Pixel *dp, *dend;
	    int bit;
	    unsigned char byte;

	    bit = 8;
	    byte = 0;			/* Suppress compiler warning. */
	    for (dp = destRowPtr, dend = dp + width; dp < dend; dp++) {
		if (bit == 8) {
		    int result;

		    result = GetHexValue(piPtr, &byte);
		    if (result == TCL_ERROR) {
			goto error;
		    }
		    if (result == TCL_RETURN) {
			goto done;
		    }
		    byte = ReverseBits(byte);
		    bit = 0;
		}
		if (((byte >> bit) & 0x01) == 0) {
		    dp->u32 = 0xFFFFFFFF;
		}
		bit++;
	    }
	    destRowPtr += Blt_PictureStride(picture);
	}
    } else {
	Blt_Warn("unknown EPSI bitsPerPixel (%d)\n", bitsPerPixel);
    }
  done:
    itemPtr->original = picture;
    itemPtr->origFromPicture = FALSE;
    itemPtr->lastWidth = Blt_PictureWidth(picture);
    itemPtr->lastHeight = Blt_PictureHeight(picture);
    itemPtr->lastLine = piPtr->lineNumber + 1;
    return;

  error:
    itemPtr->firstLine = itemPtr->lastLine = -1;
    if (picture != NULL) {
	Blt_FreePicture(picture);
    }
}

/*
 *---------------------------------------------------------------------------
 *
 * ReadPostScript --
 *
 *	This routine reads and parses the few fields we need out of an EPS
 *	file.
 *
 *	The EPS standards are outlined from Appendix H of the "PostScript
 *	Language Reference Manual" pp. 709-736.
 *
 *	Mandatory fields:
 *
 *	- Starts with "%!PS*"
 *	- Contains "%%BoundingBox: llx lly urx ury"
 *
 *	Optional fields for EPS item:
 *	- "%%BeginPreview: w h bpp #lines"
 *		Preview is in hexadecimal. Each line must start with "%"
 *      - "%%EndPreview"
 *	- "%%Title: (string)"
 *
 *---------------------------------------------------------------------------
 */
static int
ReadPostScript(Tcl_Interp *interp, EpsItem *itemPtr)
{
    char *field;
    char *dscTitle, *dscBoundingBox;
    char *dscEndComments;
    ParseInfo pi;

    pi.line[0] = '\0';
    pi.maxBytes = itemPtr->psLength;
    pi.lineNumber = 0;
    pi.f = itemPtr->psFile;

    Tcl_DStringInit(&itemPtr->ds);
    if (pi.maxBytes == 0) {
	pi.maxBytes = INT_MAX;
    }
    if (itemPtr->psStart > 0) {
	if (fseek(itemPtr->psFile, itemPtr->psStart, 0) != 0) {
	    Tcl_AppendResult(interp, 
			     "can't seek to start of PostScript code in \"", 
			     itemPtr->fileName, "\"", (char *)NULL);
	    return TCL_ERROR;
	}
    }
    if (!ReadPsLine(&pi)) {
	Tcl_AppendResult(interp, "file \"", itemPtr->fileName, "\" is empty?",
	    (char *)NULL);
	return TCL_ERROR;
    }
    if (strncmp(pi.line, "%!PS", 4) != 0) {
	Tcl_AppendResult(interp, "file \"", itemPtr->fileName,
	    "\" doesn't start with \"%!PS\"", (char *)NULL);
	return TCL_ERROR;
    }

    /*
     * Initialize field flags to NULL. We want to look only at the first
     * appearance of these comment fields.  The file itself may have another
     * EPS file embedded into it.
     */
    dscBoundingBox = dscTitle = dscEndComments = NULL;
    pi.lineNumber = 1;
    while (ReadPsLine(&pi)) {
	pi.lineNumber++;
	if ((pi.line[0] == '%') && (pi.line[1] == '%')) { /* Header comment */
	    field = pi.line + 2;
	    if (field[0] == 'B') {
		if (strncmp(field, "BeginSetup", 8) == 0) {
		    break;		/* Done */
		}
		if (strncmp(field, "BeginProlog", 8) == 0) {
		    break;		/* Done */
		}
		if ((strncmp(field, "BoundingBox:", 12) == 0) &&
		    (dscBoundingBox == NULL)) {
		    int numFields;
		    
		    dscBoundingBox = field + 12;
		    numFields = sscanf(dscBoundingBox, "%d %d %d %d",
				     &itemPtr->llx, &itemPtr->lly,
				     &itemPtr->urx, &itemPtr->ury);
		    if (numFields != 4) {
			Tcl_AppendResult(interp,
					 "bad \"%%BoundingBox\" values: \"",
					 dscBoundingBox, "\"", (char *)NULL);
			goto error;
		    }
		}
	    } else if ((field[0] == 'T') &&
		(strncmp(field, "Title:", 6) == 0)) {
		if (dscTitle == NULL) {
		    char *lp, *rp;

		    lp = strchr(field + 6, '(');
		    if (lp != NULL) {
			rp = strrchr(field + 6, ')');
			if (rp != NULL) {
			    *rp = '\0';
			}
			dscTitle = Blt_AssertStrdup(lp + 1);
		    } else {
			dscTitle = Blt_AssertStrdup(field + 6);
		    }
		}
	    } else if (field[0] == 'E') {
		if (strncmp(field, "EndComments", 11) == 0) {
		    dscEndComments = field;
		    break;		/* Done */
		}
	    }
	} /* %% */
    }
    if (dscBoundingBox == NULL) {
	Tcl_AppendResult(interp, "no \"%%BoundingBox:\" found in \"",
			 itemPtr->fileName, "\"", (char *)NULL);
	goto error;
    }
    if (dscEndComments != NULL) {
	/* Check if a "%%BeginPreview" immediately follows */
	while (ReadPsLine(&pi)) {
	    field = SkipBlanks(&pi);
	    if (field[0] != '\0') {
		break;
	    }
	}
	if (strncmp(pi.line, "%%BeginumPreview:", 15) == 0) {
	    ReadEPSI(itemPtr, &pi);
	}
    }
    if (dscTitle != NULL) {
	itemPtr->title = dscTitle;
    }
    /* Finally save the PostScript into a dynamic string. */
    while (ReadPsLine(&pi)) {
	Tcl_DStringAppend(&itemPtr->ds, pi.line, -1);
	Tcl_DStringAppend(&itemPtr->ds, "\n", 1);
    }
    return TCL_OK;
 error:
    if (dscTitle != NULL) {
	Blt_Free(dscTitle);
    }
    return TCL_ERROR;	/* BoundingBox: is required. */
}

static int
OpenEpsFile(Tcl_Interp *interp, EpsItem *itemPtr)
{
    FILE *f;
#ifdef WIN32
    DOSEPSHEADER dosHeader;
    int numBytes;
#endif

    f = Blt_OpenFile(interp, itemPtr->fileName, "rb");
    if (f == NULL) {
	Tcl_AppendResult(itemPtr->interp, "can't open \"", itemPtr->fileName,
	    "\": ", Tcl_PosixError(itemPtr->interp), (char *)NULL);
	return TCL_ERROR;
    }
    itemPtr->psFile = f;
    itemPtr->psStart = itemPtr->psLength = 0L;
    itemPtr->wmfStart = itemPtr->wmfLength = 0L;
    itemPtr->tiffStart = itemPtr->tiffLength = 0L;
    
#ifdef WIN32
    numBytes = fread(&dosHeader, sizeof(DOSEPSHEADER), 1, f);
    if ((numBytes == sizeof(DOSEPSHEADER)) &&
	(dosHeader.magic[0] == 0xC5) && (dosHeader.magic[1] == 0xD0) &&
	(dosHeader.magic[2] == 0xD3) && (dosHeader.magic[3] == 0xC6)) {

	/* DOS EPS file */
	itemPtr->psStart = dosHeader.psStart;
	itemPtr->wmfStart = dosHeader.wmfStart;
	itemPtr->wmfLength = dosHeader.wmfLength;
	itemPtr->tiffStart = dosHeader.tiffStart;
	itemPtr->tiffLength = dosHeader.tiffLength;
	itemPtr->previewFormat = PS_PREVIEW_EPSI;
#ifdef HAVE_TIFF_H
	if (itemPtr->tiffLength > 0) {
	    itemPtr->previewFormat = PS_PREVIEW_TIFF;
	}	    
#endif /* HAVE_TIFF_H */
	if (itemPtr->wmfLength > 0) {
	    itemPtr->previewFormat = PS_PREVIEW_WMF;
	}
    }
    fseek(f, 0, 0);
#endif /* WIN32 */
    return ReadPostScript(interp, itemPtr);
}

static void
CloseEpsFile(EpsItem *itemPtr)
{
    if (itemPtr->psFile != NULL) {
	fclose(itemPtr->psFile);
	itemPtr->psFile = NULL;
    }
}

#ifdef WIN32
#ifdef HAVE_TIFF_H
static void
ReadTiffPreview(EpsItem *itemPtr)
{
    unsigned int width, height;
    Blt_Picture picture;
    Blt_Pixel *dataPtr;
    FILE *f;
    int n;

    TIFFGetField(itemPtr->tiffPtr, TIFFTAG_IMAGEWIDTH, &width);
    TIFFGetField(itemPtr->tiffPtr, TIFFTAG_IMAGELENGTH, &height);
    picture = Blt_CreatePicture(width, height);
    dataPtr = Blt_PictureBits(picture);
    if (!TIFFReadRGBAImage(itemPtr->tiffPtr, width, height, dataPtr, 0)) {
	Blt_FreePicture(picture);
	return;
    }
    /* Reverse the order of the components for each pixel. */
    /* ... */
    itemPtr->origFromPicture = FALSE;
    itemPtr->picture = picture;
}
#endif

#ifdef notdef
ReadWMF(f, itemPtr, headerPtr)
    FILE *f;
{
    HANDLE hMem;
    Tk_Window tkwin;

    if (fseek(f, headerPtr->wmfStart, 0) != 0) {
	Tcl_AppendResult(interp, "can't seek in \"", itemPtr->fileName, 
			 "\"", (char *)NULL);
	return TCL_ERROR;
    }
    hMem = GlobalAlloc(GHND, size);
    if (hMem == NULL) {
	Tcl_AppendResult(graphPtr->interp, "can't allocate global memory:", 
			 Blt_LastError(), (char *)NULL);
	return TCL_ERROR;
    }
    buffer = (LPVOID)GlobalLock(hMem);
    /* Read the header and see what kind of meta file it is. */
    fread(buffer, sizeof(unsigned char), headerPtr->wmfLength, f);
    mfp.mm = 0;
    mfp.xExt = itemPtr->width;
    mfp.yExt = itemPtr->height;
    mfp.hMF = hMetaFile;
    tkwin = Tk_CanvasTkwin(itemPtr->canvas);
    hRefDC = TkWinGetDrawableDC(Tk_Display(tkwin), Tk_WindowId(tkwin), &state);
    hDC = CreateEnhMetaFile(hRefDC, NULL, NULL, NULL);
    mfp.hMF = CloseEnhMetaFile(hDC);
    hMetaFile = SetWinMetaFileBits(size, buffer, MM_ANISOTROPIC, &picture);
	Tcl_AppendResult(graphPtr->interp, "can't get metafile data:", 
		Blt_LastError(), (char *)NULL);
	goto error;
}
#endif
#endif /* WIN32 */
/*
 *---------------------------------------------------------------------------
 *
 * DeleteProc --
 *
 *	This procedure is called to clean up the data structure associated
 *	with a EPS item.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Resources associated with itemPtr are released.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static void
DeleteProc( 
    Tk_Canvas canvas,			/* Info about overall canvas
					 * widget. */
    Tk_Item *canvItemPtr,		/* Item that is being deleted. */
    Display *display)			/* Display containing window for
					 * canvas. */
{
    EpsItem *itemPtr = (EpsItem *)canvItemPtr;

    Tk_FreeOptions(configSpecs, (char *)itemPtr, display, 0);
    CloseEpsFile(itemPtr);
    if ((!itemPtr->origFromPicture) && (itemPtr->original != NULL)) {
	Blt_FreePicture(itemPtr->original);
    }
    if (itemPtr->picture != NULL) {
	Blt_FreePicture(itemPtr->picture);
    }
    if (itemPtr->painter != NULL) {
	Blt_FreePainter(itemPtr->painter);
    }
    if (itemPtr->preview != NULL) {
	Tk_FreeImage(itemPtr->preview);
    }
    if (itemPtr->previewImageName != NULL) {
	Blt_Free(itemPtr->previewImageName);
    }
    if (itemPtr->stipple != None) {
	Tk_FreePixmap(display, itemPtr->stipple);
    }
    if (itemPtr->fillGC != NULL) {
	Tk_FreeGC(display, itemPtr->fillGC);
    }
    Blt_Ts_FreeStyle(display, &itemPtr->titleStyle);

    if (itemPtr->title != NULL) {
	Blt_Free(itemPtr->title);
    }
}

/*
 *---------------------------------------------------------------------------
 *
 * CreateProc --
 *
 *	This procedure is invoked to create a new EPS item in a canvas.
 *
 * Results:
 *	A standard TCL return value.  If an error occurred in creating the
 *	item, then an error message is left in interp->result; in this case
 *	itemPtr is left uninitialized, so it can be safely freed by the
 *	caller.
 *
 * Side effects:
 *	A new EPS item is created.
 *
 *---------------------------------------------------------------------------
 */
static int
CreateProc(
    Tcl_Interp *interp,			/* Interpreter for error reporting. */
    Tk_Canvas canvas,			/* Canvas to hold new item. */
    Tk_Item *canvItemPtr,		/* Record to hold new item; header has
					 * been initialized by caller. */
    int argc,				/* Number of arguments in argv. */
    char **argv)			/* Arguments describing rectangle. */
{
    EpsItem *itemPtr = (EpsItem *)canvItemPtr;
    Tk_Window tkwin;
    double x, y;

    tkwin = Tk_CanvasTkwin(canvas);
    if (argc < 2) {
	Tcl_AppendResult(interp, "wrong # args: should be \"",
	    Tk_PathName(tkwin), " create ", canvItemPtr->typePtr->name,
	    " x1 y1 ?options?\"", (char *)NULL);
	return TCL_ERROR;
    }

    /* Initialize the item's record by hand (bleah). */
    itemPtr->anchor = TK_ANCHOR_NW;
    itemPtr->border = NULL;
    itemPtr->borderWidth = 0;
    itemPtr->canvas = canvas;
    itemPtr->fileName = NULL;
    itemPtr->psFile = NULL;
    itemPtr->fillGC = NULL;
    itemPtr->fillColor = NULL;
    itemPtr->painter = NULL;
    itemPtr->original = NULL;
    itemPtr->origFromPicture = FALSE;
    itemPtr->previewImageName = NULL;
    itemPtr->preview = NULL;
    itemPtr->interp = interp;
    itemPtr->picture = NULL;
    itemPtr->firstLine = itemPtr->lastLine = -1;
    itemPtr->relief = TK_RELIEF_SUNKEN;
    itemPtr->reqTitle = NULL;
    itemPtr->stipple = None;
    itemPtr->showImage = TRUE;
    itemPtr->quick = FALSE;
    itemPtr->title = NULL;
    itemPtr->lastWidth = itemPtr->lastHeight = 0;
    itemPtr->width = itemPtr->height = 0;
    itemPtr->x = itemPtr->y = 0.0; 
    itemPtr->llx = itemPtr->lly = itemPtr->urx = itemPtr->ury = 0;
    itemPtr->bb.left = itemPtr->bb.right = itemPtr->bb.top = itemPtr->bb.bottom = 0;
    Tcl_DStringInit(&itemPtr->ds);
    Blt_Ts_InitStyle(itemPtr->titleStyle);
#define PAD	8
    Blt_Ts_SetPadding(itemPtr->titleStyle, PAD, PAD, PAD, PAD);

    /* Process the arguments to fill in the item record. */
    if ((Tk_CanvasGetCoord(interp, canvas, argv[0], &x) != TCL_OK) ||
	(Tk_CanvasGetCoord(interp, canvas, argv[1], &y) != TCL_OK)) {
	DeleteProc(canvas, canvItemPtr, Tk_Display(tkwin));
	return TCL_ERROR;
    }
    itemPtr->x = x;
    itemPtr->y = y;
    if (ConfigureProc(interp, canvas, canvItemPtr, argc - 2, argv + 2, 0) 
	!= TCL_OK) {
	DeleteProc(canvas, canvItemPtr, Tk_Display(tkwin));
	return TCL_ERROR;
    }
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * ImageChangedProc
 *
 *	The image is over-written each time the EPS item is resized.  So we
 *	only worry if the image is deleted.
 *
 *	We always resample from the picture we saved when the photo image was
 *	specified (-image option).
 *
 * Results:
 *	None.
 *
 *---------------------------------------------------------------------------
 */
/* ARGSUSED */
static void
ImageChangedProc(
    ClientData clientData,
    int x, int y, int width, int height, /* Not used. */
    int imageWidth, int imageHeight)	 /* Not used. */
{
    EpsItem *itemPtr = clientData;

    if ((itemPtr->preview == NULL) || (Blt_Image_IsDeleted(itemPtr->preview))) {
	itemPtr->preview = NULL;
	if (itemPtr->previewImageName != NULL) {
	    Blt_Free(itemPtr->previewImageName);
	    itemPtr->previewImageName = NULL;
	}
	Tk_CanvasEventuallyRedraw(itemPtr->canvas, itemPtr->item.x1, 
		itemPtr->item.y1, itemPtr->item.x2, itemPtr->item.y2);
    }
    if (itemPtr->preview != NULL) {
	int result;

	if ((!itemPtr->origFromPicture) && (itemPtr->original != NULL)) {
	    Blt_FreePicture(itemPtr->original);
	}
	result = Blt_GetPicture(itemPtr->interp, itemPtr->previewImageName, 
				&itemPtr->original);
	if (result == TCL_OK) {
	    itemPtr->origFromPicture = TRUE;
	} else {
	    Tk_PhotoHandle photo;	/* Photo handle to Tk image. */
	    
	    photo = Tk_FindPhoto(itemPtr->interp, itemPtr->previewImageName);
	    if (photo == NULL) {
		Blt_Warn("image \"%s\" isn't a picture or photo image\n",
			itemPtr->previewImageName);
		return;
	    }
	    itemPtr->original = Blt_PhotoToPicture(photo);
	    itemPtr->origFromPicture = FALSE;
	}
    }
}

/*
 *---------------------------------------------------------------------------
 *
 * ConfigureProc --
 *
 *	This procedure is invoked to configure various aspects of an EPS item,
 *	such as its background color.
 *
 * Results:
 *	A standard TCL result code.  If an error occurs, then an error message
 *	is left in interp->result.
 *
 * Side effects:
 *	Configuration information may be set for itemPtr.
 *
 *---------------------------------------------------------------------------
 */
static int
ConfigureProc(
    Tcl_Interp *interp,			/* Used for error reporting. */
    Tk_Canvas canvas,			/* Canvas containing itemPtr. */
    Tk_Item *canvItemPtr,		/* EPS item to reconfigure. */
    int argc,				/* Number of elements in argv.  */
    char **argv,			/* Arguments describing things to
					 * configure. */
    int flags)				/* Flags to pass to
					 * Tk_ConfigureWidget. */
{
    EpsItem *itemPtr = (EpsItem *)canvItemPtr;
    Tk_Window tkwin;
    XGCValues gcValues;
    unsigned long gcMask;
    GC newGC;
    int width, height;
    Blt_Painter painter;

    tkwin = Tk_CanvasTkwin(canvas);
    if (Tk_ConfigureWidget(interp, tkwin, configSpecs, argc, (const char**)argv,
		(char *)itemPtr, flags) != TCL_OK) {
	return TCL_ERROR;
    }
    painter = Blt_GetPainter(tkwin, 1.0);
    if (itemPtr->painter != NULL) {
	Blt_FreePainter(itemPtr->painter);
    }
    itemPtr->painter = painter;
    /* Determine the size of the EPS item */
    /*
     * Check for a "-image" option specifying an image to be displayed
     * representing the EPS canvas item.
     */
    if (Blt_OldConfigModified(configSpecs, "-image", (char *)NULL)) {
	if (itemPtr->preview != NULL) {
	    Tk_FreeImage(itemPtr->preview);	/* Release old Tk image */
	    if ((!itemPtr->origFromPicture) && (itemPtr->original != NULL)) {
		Blt_FreePicture(itemPtr->original);
	    }
	    itemPtr->original = NULL;
	    if (itemPtr->picture != NULL) {
		Blt_FreePicture(itemPtr->picture);
	    }
	    itemPtr->picture = NULL;
	    itemPtr->preview = NULL;
	    itemPtr->origFromPicture = FALSE;
	}
	if (itemPtr->previewImageName != NULL) {
	    int result;

	    /* Allocate a new image, if one was named. */
	    itemPtr->preview = Tk_GetImage(interp, tkwin, 
			itemPtr->previewImageName, ImageChangedProc, itemPtr);
	    if (itemPtr->preview == NULL) {
		Tcl_AppendResult(interp, "can't find an image \"",
		    itemPtr->previewImageName, "\"", (char *)NULL);
		Blt_Free(itemPtr->previewImageName);
		itemPtr->previewImageName = NULL;
		return TCL_ERROR;
	    }
	    result = Blt_GetPicture(interp, itemPtr->previewImageName, 
				    &itemPtr->original);
	    if (result == TCL_OK) {
		itemPtr->origFromPicture = TRUE;
	    } else {
		Tk_PhotoHandle photo;	/* Photo handle to Tk image. */

		photo = Tk_FindPhoto(interp, itemPtr->previewImageName);
		if (photo == NULL) {
		    Tcl_AppendResult(interp, "image \"", 
			itemPtr->previewImageName,
			"\" is not a picture or photo image", (char *)NULL);
		    return TCL_ERROR;
		}
		itemPtr->original = Blt_PhotoToPicture(photo);
		itemPtr->origFromPicture = FALSE;
	    }
	}
    }
    if (Blt_OldConfigModified(configSpecs, "-file", (char *)NULL)) {
	CloseEpsFile(itemPtr);
	if ((!itemPtr->origFromPicture) && (itemPtr->original != NULL)) {
	    Blt_FreePicture(itemPtr->original);
	    itemPtr->original = NULL;
	}
	if (itemPtr->picture != NULL) {
	    Blt_FreePicture(itemPtr->picture);
	    itemPtr->picture = NULL;
	}
	itemPtr->firstLine = itemPtr->lastLine = -1;
	if (itemPtr->fileName != NULL) {
	    if (OpenEpsFile(interp, itemPtr) != TCL_OK) {
		return TCL_ERROR;
	    }
	}
    }
    /* Compute the normal width and height of the item, but let the
     * user-requested dimensions override them. */
    width = height = 0;
    if (itemPtr->preview != NULL) {
	/* Default dimension is the size of the image. */
	Tk_SizeOfImage(itemPtr->preview, &width, &height);
    }
    if (itemPtr->fileName != NULL) {
	/* Use dimensions provided by the BoundingBox. */
	width = (itemPtr->urx - itemPtr->llx); 
	height = (itemPtr->ury - itemPtr->lly); 
    }
    if (itemPtr->width == 0) {
	itemPtr->width = width;
    }
    if (itemPtr->height == 0) {
	itemPtr->height = height;
    }

    if (Blt_OldConfigModified(configSpecs, "-quick", (char *)NULL)) {
	itemPtr->lastWidth = itemPtr->lastHeight = 0;
    }
    /* Fill color GC */

    newGC = NULL;
    if (itemPtr->fillColor != NULL) {
	gcMask = GCForeground;
	gcValues.foreground = itemPtr->fillColor->pixel;
	if (itemPtr->stipple != None) {
	    gcMask |= (GCStipple | GCFillStyle);
	    gcValues.stipple = itemPtr->stipple;
	    if (itemPtr->border != NULL) {
		gcValues.foreground = Tk_3DBorderColor(itemPtr->border)->pixel;
		gcValues.background = itemPtr->fillColor->pixel;
		gcMask |= GCBackground;
		gcValues.fill_style = FillOpaqueStippled;
	    } else {
		gcValues.fill_style = FillStippled;
	    }
	}
	newGC = Tk_GetGC(tkwin, gcMask, &gcValues);
    }
    if (itemPtr->fillGC != NULL) {
	Tk_FreeGC(Tk_Display(tkwin), itemPtr->fillGC);
    }
    itemPtr->fillGC = newGC;
    CloseEpsFile(itemPtr);
    ComputeBbox(canvas, itemPtr);
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * CoordProc --
 *
 *	This procedure is invoked to process the "coords" widget command on
 *	EPS items.  See the user documentation for details on what it does.
 *
 * Results:
 *	Returns TCL_OK or TCL_ERROR, and sets interp->result.
 *
 * Side effects:
 *	The coordinates for the given item may be changed.
 *
 *---------------------------------------------------------------------------
 */
static int
CoordProc(
    Tcl_Interp *interp,			/* Used for error reporting. */
    Tk_Canvas canvas,			/* Canvas containing item. */
    Tk_Item *canvItemPtr,		/* Item whose coordinates are to be
					 * read or modified. */
    int argc,				/* Number of coordinates supplied in
					 * argv. */
    char **argv)			/* Array of coordinates: x1, y1, x2,
					 * y2, ... */
{
    EpsItem *itemPtr = (EpsItem *)canvItemPtr;

    if ((argc != 0) && (argc != 2)) {
	Tcl_AppendResult(interp, "wrong # coordinates: expected 0 or 2, got ",
	    Blt_Itoa(argc), (char *)NULL);
	return TCL_ERROR;
    }
    if (argc == 2) {
	double x, y;			/* Don't overwrite old coordinates on
					 * errors */

	if ((Tk_CanvasGetCoord(interp, canvas, argv[0], &x) != TCL_OK) ||
	    (Tk_CanvasGetCoord(interp, canvas, argv[1], &y) != TCL_OK)) {
	    return TCL_ERROR;
	}
	itemPtr->x = x;
	itemPtr->y = y;
	ComputeBbox(canvas, itemPtr);
	return TCL_OK;
    }
    Tcl_AppendElement(interp, Blt_Dtoa(interp, itemPtr->x));
    Tcl_AppendElement(interp, Blt_Dtoa(interp, itemPtr->y));
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * ComputeBbox --
 *
 *	This procedure is invoked to compute the bounding box of all the
 *	pixels that may be drawn as part of a EPS item.  This procedure is
 *	where the preview image's placement is computed.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The fields x1, y1, x2, and y2 are updated in the item for itemPtr.
 *
 *---------------------------------------------------------------------------
 */
 /* ARGSUSED */
static void
ComputeBbox(
    Tk_Canvas canvas,			/* Canvas that contains item. */
    EpsItem *itemPtr)			/* Item whose bbox is to be
					 * recomputed. */
{
    Point2d anchorPos;

    /* Translate the coordinates wrt the anchor. */
    anchorPos = Blt_AnchorPoint(itemPtr->x, itemPtr->y, (double)itemPtr->width, 
	(double)itemPtr->height, itemPtr->anchor);
    /*
     * Note: The right and bottom are exterior to the item.  
     */

    itemPtr->bb.left = anchorPos.x;
    itemPtr->bb.top = anchorPos.y;
    itemPtr->bb.right = itemPtr->bb.left + itemPtr->width;
    itemPtr->bb.bottom = itemPtr->bb.top + itemPtr->height;

    itemPtr->item.x1 = ROUND(itemPtr->bb.left);
    itemPtr->item.y1 = ROUND(itemPtr->bb.top);
    itemPtr->item.x2 = ROUND(itemPtr->bb.right);
    itemPtr->item.y2 = ROUND(itemPtr->bb.bottom);
}

/*
 *---------------------------------------------------------------------------
 *
 * DisplayProc --
 *
 *	This procedure is invoked to draw the EPS item in a given drawable.
 *	The EPS item may be drawn as either a solid rectangle or a pixmap of
 *	the preview image.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	ItemPtr is drawn in drawable using the transformation information in
 *	canvas.
 *
 *---------------------------------------------------------------------------
 */
static void
DisplayProc(
    Tk_Canvas canvas,			/* Canvas that contains item. */
    Tk_Item *canvItemPtr,		/* Item to be displayed. */
    Display *display,			/* Display on which to draw item. */
    Drawable drawable,			/* Pixmap or window in which to draw
					 * item. */
    int rx, int ry, 
    int rw, int rh)			/* Describes region of canvas that
					 * must be redisplayed (not used). */
{
    Blt_Picture picture;
    EpsItem *itemPtr = (EpsItem *)canvItemPtr;
    Tk_Window tkwin;
    const char *title;
    int w, h;
    short int dx, dy;

    w = (int)(itemPtr->bb.right - itemPtr->bb.left);
    h = (int)(itemPtr->bb.bottom - itemPtr->bb.top);
    if ((w < 1) || (h < 1)) {
	return;
    }
    tkwin = Tk_CanvasTkwin(canvas);
    if (itemPtr->original != NULL) {
	if ((itemPtr->lastWidth != w) || (itemPtr->lastHeight != h)) {
	    if (itemPtr->quick) {
		picture = Blt_ScalePicture(itemPtr->original, 0, 0,
			Blt_PictureWidth(itemPtr->original),
			Blt_PictureHeight(itemPtr->original), w, h);
	    } else {
#ifdef notdef
		fprintf(stderr, "orig=%dx%d new=width=%dx%d last=%dx%d\n", 
			Blt_PictureWidth(itemPtr->original),
			Blt_PictureHeight(itemPtr->original),
			w, h,
			itemPtr->lastWidth, itemPtr->lastHeight);
#endif
		picture = Blt_CreatePicture(w, h);
		Blt_ResamplePicture(picture, itemPtr->original, bltBoxFilter, 
			bltBoxFilter);
	    }
	    if (itemPtr->picture != NULL) {
		Blt_FreePicture(itemPtr->picture);
	    }
	    itemPtr->picture = picture;
	    itemPtr->lastHeight = h;
	    itemPtr->lastWidth = w;
	} 
    }
    picture = itemPtr->picture;
    if (picture == NULL) {
	picture = itemPtr->original;
    }

    /*
     * Translate the coordinates to those of the EPS item, then redisplay it.
     */
    Tk_CanvasDrawableCoords(canvas, itemPtr->bb.left, itemPtr->bb.top, 
			    &dx, &dy);

    title = itemPtr->title;

    if (itemPtr->reqTitle != NULL) {
	title = itemPtr->reqTitle;
    }
    if ((itemPtr->showImage) && (picture != NULL)) {
	struct region {
	    short int left, right, top, bottom;
	} p, r;
	short int destX, destY;

	/* The eps item may only partially exposed. Be careful to clip the
	 * unexposed portions. */

	/* Convert everything to screen coordinates since the origin of the
	 * item is only available in */

	p.left = dx, p.top = dy;
	Tk_CanvasDrawableCoords(canvas, itemPtr->bb.right, itemPtr->bb.bottom,
		&p.right, &p.bottom);
	Tk_CanvasDrawableCoords(canvas, (double)rx, (double)ry, 
		&r.left, &r.top);
	Tk_CanvasDrawableCoords(canvas,(double)(rx+rw), (double)(ry+rh), 
		&r.right, &r.bottom);
	destX = (int)dx, destY = (int)dy;
	if (p.left < r.left) {
	    p.left = r.left;
	}
	if (p.top < r.top) {
	    p.top = r.top;
	}
	if (p.right > r.right) {
	    p.right = r.right;
	}
	if (p.bottom > r.bottom) {
	    p.bottom = r.bottom;
	}
	if (destX < r.left) {
	    destX = r.left;
	}
	if (destY < r.top) {
	    destY = r.top;
	}
	p.left -= dx, p.right -= dx;
	p.top -= dy, p.bottom -= dy;;
	if (0 /* itemPtr->quick */) {
	    Blt_Picture fade;

	    fade = Blt_ClonePicture(picture);
	    Blt_FadePicture(fade, 0, 0, Blt_PictureWidth(fade), 
		Blt_PictureHeight(fade), 150);
	    Blt_PaintPicture(itemPtr->painter, drawable, fade, 
		(int)p.left, (int)p.top, (int)(p.right - p.left), 
		(int)(p.bottom - p.top), destX, destY, FALSE);
	    Blt_FreePicture(fade);
	} else {
	    Blt_PaintPicture(itemPtr->painter, drawable, picture, (int)p.left, 
		(int)p.top, (int)(p.right - p.left), (int)(p.bottom - p.top), 
		destX, destY, FALSE);
	}
    } else {
	if (itemPtr->fillGC != NULL) {
	    XSetTSOrigin(display, itemPtr->fillGC, dx, dy);
	    XFillRectangle(display, drawable, itemPtr->fillGC, dx, dy,
		itemPtr->width, itemPtr->height);
	    XSetTSOrigin(display, itemPtr->fillGC, 0, 0);
	}
    }

    if (title != NULL) {
	TextLayout *textPtr;
	double rw, rh;
	int dw, dh;

	/* Translate the title to an anchor position within the EPS item */
	itemPtr->titleStyle.font = itemPtr->font;
	textPtr = Blt_Ts_CreateLayout(title, -1, &itemPtr->titleStyle);
	Blt_GetBoundingBox(textPtr->width, textPtr->height, 
	     itemPtr->titleStyle.angle, &rw, &rh, (Point2d *)NULL);
	dw = (int)ceil(rw);
	dh = (int)ceil(rh);
	if ((dw <= w) && (dh <= h)) {
	    int tx, ty;

	    Blt_TranslateAnchor(dx, dy, w, h, itemPtr->titleStyle.anchor, 
		&tx, &ty);
	    if (picture == NULL) {
		tx += itemPtr->borderWidth;
		ty += itemPtr->borderWidth;
	    }
	    Blt_Ts_DrawLayout(tkwin, drawable, textPtr, &itemPtr->titleStyle, 
		tx, ty);
	}
	Blt_Free(textPtr);
    }
    if ((picture == NULL) && (itemPtr->border != NULL) && 
	(itemPtr->borderWidth > 0)) {
	Blt_Draw3DRectangle(tkwin, drawable, itemPtr->border, dx, dy,
	    itemPtr->width, itemPtr->height, itemPtr->borderWidth, itemPtr->relief);
    }
}

/*
 *---------------------------------------------------------------------------
 *
 * PointProc --
 *
 *	Computes the distance from a given point to a given rectangle, in
 *	canvas units.
 *
 * Results:
 *	The return value is 0 if the point whose x and y coordinates are
 *	coordPtr[0] and coordPtr[1] is inside the EPS item.  If the point
 *	isn't inside the item then the return value is the distance from the
 *	point to the EPS item.
 *
 * Side effects:
 *	None.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static double
PointProc(
    Tk_Canvas canvas,			/* Canvas containing item. */
    Tk_Item *canvItemPtr,		/* Item to check against point. */
    double *pts)			/* Array of x and y coordinates. */
{
    EpsItem *itemPtr = (EpsItem *)canvItemPtr;
    double x, y, dx, dy;

    x = pts[0], y = pts[1];

    /*
     * Check if point is outside the bounding rectangle and compute the
     * distance to the closest side.
     */
    dx = dy = 0;
    if (x < itemPtr->item.x1) {
	dx = itemPtr->item.x1 - x;
    } else if (x > itemPtr->item.x2) {
	dx = x - itemPtr->item.x2;
    }
    if (y < itemPtr->item.y1) {
	dy = itemPtr->item.y1 - y;
    } else if (y > itemPtr->item.y2) {
	dy = y - itemPtr->item.y2;
    }
    return hypot(dx, dy);
}

/*
 *---------------------------------------------------------------------------
 *
 * AreaProc --
 *
 *	This procedure is called to determine whether an item lies entirely
 *	inside, entirely outside, or overlapping a given rectangle.
 *
 * Results:
 *	-1 is returned if the item is entirely outside the area given by
 *	rectPtr, 0 if it overlaps, and 1 if it is entirely inside the given
 *	area.
 *
 * Side effects:
 *	None.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
AreaProc(
    Tk_Canvas canvas,			/* Canvas containing the item. */
    Tk_Item *canvItemPtr,		/* Item to check against bounding
					 * rectangle. */
    double pts[])			/* Array of four coordinates (x1, y1,
					 * x2, y2) describing area.  */
{
    EpsItem *itemPtr = (EpsItem *)canvItemPtr;

    if ((pts[2] <= itemPtr->bb.left) || (pts[0] >= itemPtr->bb.right) ||
	(pts[3] <= itemPtr->bb.top)  || (pts[1] >= itemPtr->bb.bottom)) {
	return -1;			/* Outside. */
    }
    if ((pts[0] <= itemPtr->bb.left)  && (pts[1] <= itemPtr->bb.top) &&
	(pts[2] >= itemPtr->bb.right) && (pts[3] >= itemPtr->bb.bottom)) {
	return 1;			/* Inside. */
    }
    return 0;				/* Overlap. */
}

/*
 *---------------------------------------------------------------------------
 *
 * ScaleProc --
 *
 *	This procedure is invoked to rescale an item.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The item referred to by itemPtr is rescaled so that the
 *	following transformation is applied to all point coordinates:
 *		x' = xOrigin + xScale*(x-xOrigin)
 *		y' = yOrigin + yScale*(y-yOrigin)
 *
 *---------------------------------------------------------------------------
 */
static void
ScaleProc(
    Tk_Canvas canvas,			/* Canvas containing rectangle. */
    Tk_Item *canvItemPtr,			/* Rectangle to be scaled. */
    double xOrigin, double yOrigin,	/* Origin wrt scale rect. */
    double xScale, double yScale)
{
    EpsItem *itemPtr = (EpsItem *)canvItemPtr;

    itemPtr->bb.left = xOrigin + xScale * (itemPtr->bb.left - xOrigin);
    itemPtr->bb.right = xOrigin + xScale * (itemPtr->bb.right - xOrigin);
    itemPtr->bb.top = yOrigin + yScale * (itemPtr->bb.top - yOrigin);
    itemPtr->bb.bottom = yOrigin + yScale *(itemPtr->bb.bottom - yOrigin);

    /* Reset the user-requested values to the newly scaled values. */
    itemPtr->width = ROUND(itemPtr->bb.right - itemPtr->bb.left);
    itemPtr->height = ROUND(itemPtr->bb.bottom - itemPtr->bb.top);
    itemPtr->x = ROUND(itemPtr->bb.left);
    itemPtr->y = ROUND(itemPtr->bb.top);

    itemPtr->item.x1 = ROUND(itemPtr->bb.left);
    itemPtr->item.y1 = ROUND(itemPtr->bb.top);
    itemPtr->item.x2 = ROUND(itemPtr->bb.right);
    itemPtr->item.y2 = ROUND(itemPtr->bb.bottom);
}

/*
 *---------------------------------------------------------------------------
 *
 * TranslateProc --
 *
 *	This procedure is called to move an item by a given amount.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The position of the item is offset by (dx, dy), and the bounding box
 *	is updated in the generic part of the item structure.
 *
 *---------------------------------------------------------------------------
 */
static void
TranslateProc(
    Tk_Canvas canvas,			/* Canvas containing item. */
    Tk_Item *canvItemPtr,			/* Item that is being moved. */
    double dx, double dy)		/* Amount by which item is to be
					 * moved. */
{
    EpsItem *itemPtr = (EpsItem *)canvItemPtr;

    itemPtr->bb.left += dx;
    itemPtr->bb.right += dx;
    itemPtr->bb.top += dy;
    itemPtr->bb.bottom += dy;

    itemPtr->x = itemPtr->bb.left;
    itemPtr->y = itemPtr->bb.top;

    itemPtr->item.x1 = ROUND(itemPtr->bb.left);
    itemPtr->item.x2 = ROUND(itemPtr->bb.right);
    itemPtr->item.y1 = ROUND(itemPtr->bb.top);
    itemPtr->item.y2 = ROUND(itemPtr->bb.bottom);
}

/*
 *---------------------------------------------------------------------------
 *
 * PostScriptProc --
 *
 *	This procedure is called to generate PostScript for EPS canvas items.
 *
 * Results:
 *	The return value is a standard TCL result.  If an error occurs in
 *	generating PostScript then an error message is left in interp->result,
 *	replacing whatever used to be there.  If no errors occur, then
 *	PostScript output for the item is appended to the interpreter result.
 *
 * Side effects:
 *	None.
 *
 *---------------------------------------------------------------------------
 */

static int
PostScriptProc(
    Tcl_Interp *interp,			/* Interpreter to hold generated
					 * PostScript or reports errors back
					 * to. */
    Tk_Canvas canvas,			/* Information about overall
					 * canvas. */
    Tk_Item *canvItemPtr,		/* eps item. */
    int prepass)			/* If 1, this is a prepass to collect
					 * font information; 0 means final *
					 * PostScript is being created. */
{
    EpsItem *itemPtr = (EpsItem *)canvItemPtr;
    Blt_Ps ps;
    double xScale, yScale;
    double x, y, w, h;
    PageSetup setup;

    if (prepass) {
	return TCL_OK;			/* Don't worry about fonts. */
    }
    memset(&setup, 0, sizeof(setup));
    ps = Blt_Ps_Create(interp, &setup);

    /* Lower left corner of item on page. */
    x = itemPtr->bb.left;
    y = Tk_CanvasPsY(canvas, itemPtr->bb.bottom);
    w = itemPtr->bb.right - itemPtr->bb.left;
    h = itemPtr->bb.bottom - itemPtr->bb.top;

    if (itemPtr->fileName == NULL) {
	/* No PostScript file, generate PostScript of resized image instead. */
	if (itemPtr->picture != NULL) {
	    Blt_Ps_Format(ps, "gsave\n");
	    /*
	     * First flip the PostScript y-coordinate axis so that the origin
	     * is the upper-left corner like our picture.
	     */
	    Blt_Ps_Format(ps, "  %g %g translate\n", x, y + h);
	    Blt_Ps_Format(ps, "  1 -1 scale\n");

	    Blt_Ps_DrawPicture(ps, itemPtr->picture, 0.0, 0.0);
	    Blt_Ps_Format(ps, "grestore\n");

	    Blt_Ps_SetInterp(ps, interp);
	    Blt_Ps_Free(ps);
	}
	return TCL_OK;
    }

    /* Copy in the PostScript prolog for EPS encapsulation. */
    if (Blt_Ps_IncludeFile(interp, ps, "bltCanvEps.pro") != TCL_OK) {
	goto error;
    }
    Blt_Ps_Append(ps, "BeginEPSF\n");

    xScale = w / (double)(itemPtr->urx - itemPtr->llx);
    yScale = h / (double)(itemPtr->ury - itemPtr->lly);

    /* Set up scaling and translation transformations for the EPS item */

    Blt_Ps_Format(ps, "%g %g translate\n", x, y);
    Blt_Ps_Format(ps, "%g %g scale\n", xScale, yScale);
    Blt_Ps_Format(ps, "%d %d translate\n", -(itemPtr->llx), -(itemPtr->lly));

    /* FIXME: Why clip against the old bounding box? */
    Blt_Ps_Format(ps, "%d %d %d %d SetClipRegion\n", itemPtr->llx, 
	itemPtr->lly, itemPtr->urx, itemPtr->ury);

    Blt_Ps_VarAppend(ps, "%% including \"", itemPtr->fileName, "\"\n\n",
	 (char *)NULL);

    Blt_Ps_AppendBytes(ps, Tcl_DStringValue(&itemPtr->ds), 
	Tcl_DStringLength(&itemPtr->ds));
    Blt_Ps_Append(ps, "EndEPSF\n");
    Blt_Ps_SetInterp(ps, interp);
    Blt_Ps_Free(ps);
    return TCL_OK;

  error:
    Blt_Ps_Free(ps);
    return TCL_ERROR;
}


/*
 * The structures below defines the EPS item type in terms of procedures that
 * can be invoked by generic item code.
 */
static Tk_ItemType itemType = {
    (char *)"eps",			/* name */
    sizeof(EpsItem),			/* itemSize */
    CreateProc,		
    configSpecs,			/* configSpecs */
    ConfigureProc,	
    CoordProc,
    DeleteProc,
    DisplayProc,
    0,					/* alwaysRedraw */
    PointProc,
    AreaProc,
    PostScriptProc,
    ScaleProc,
    TranslateProc,
    (Tk_ItemIndexProc *)NULL,		/* indexProc */
    (Tk_ItemCursorProc *)NULL,		/* icursorProc */
    (Tk_ItemSelectionProc *)NULL,	/* selectionProc */
    (Tk_ItemInsertProc *)NULL,		/* insertProc */
    (Tk_ItemDCharsProc *)NULL,		/* dTextProc */
    (Tk_ItemType *)NULL			/* nextPtr */
};

/*ARGSUSED*/
void
Blt_RegisterEpsCanvasItem(void)
{
    Tk_CreateItemType(&itemType);
    /* Initialize custom canvas option routines. */
    tagsOption.parseProc = Tk_CanvasTagsParseProc;
    tagsOption.printProc = Tk_CanvasTagsPrintProc;
}
