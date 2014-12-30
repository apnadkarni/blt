/* -*- mode: c; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 * bltGrPs.c --
 *
 * This module implements the "postscript" operation for BLT graph widget.
 *
 *	Copyright 1991-2004 George A Howlett.
 *
 *	Permission is hereby granted, free of charge, to any person
 *	obtaining a copy of this software and associated documentation
 *	files (the "Software"), to deal in the Software without
 *	restriction, including without limitation the rights to use, copy,
 *	modify, merge, publish, distribute, sublicense, and/or sell copies
 *	of the Software, and to permit persons to whom the Software is
 *	furnished to do so, subject to the following conditions:
 *
 *	The above copyright notice and this permission notice shall be
 *	included in all copies or substantial portions of the Software.
 *
 *	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 *	EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 *	MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 *	NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
 *	BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
 *	ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 *	CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 *	SOFTWARE.
 */

/*
 *---------------------------------------------------------------------------
 *
 * PostScript routines to print a graph
 *
 *---------------------------------------------------------------------------
 */
#define BUILD_BLT_TK_PROCS 1
#include "bltInt.h"

#ifdef HAVE_STRING_H
#  include <string.h>
#endif /* HAVE_STRING_H */

#ifdef HAVE_STDARG_H
#  include <stdarg.h>
#endif	/* HAVE_STDARG_H */

#ifdef HAVE_STDLIB_H
#  include <stdlib.h>
#endif /* HAVE_STDLIB_H */

#include <X11/Xutil.h>

#include "bltAlloc.h"
#include "bltHash.h"
#include "bltChain.h"
#include "bltBind.h"
#include "bltPs.h"
#include "bltBg.h"
#include "bltPicture.h"
#include "tkDisplay.h"
#include "bltPsInt.h"
#include "bltGraph.h"
#include "bltGrAxis.h"
#include "bltGrLegd.h"
#include "bltOp.h"

#define MM_INCH		25.4
#define PICA_INCH	72.0

typedef int (GraphPsProc)(Graph *graphPtr, Tcl_Interp *interp, int objc, 
	Tcl_Obj *const *objv);

static Blt_OptionParseProc ObjToPicaProc;
static Blt_OptionPrintProc PicaToObjProc;
static Blt_CustomOption picaOption =
{
    ObjToPicaProc, PicaToObjProc, NULL, (ClientData)0,
};

static Blt_OptionParseProc ObjToPad;
static Blt_OptionPrintProc PadToObj;
static Blt_CustomOption padOption =
{
    ObjToPad, PadToObj, NULL, (ClientData)0,
};

#define DEF_PS_CENTER		"yes"
#define DEF_PS_COLOR_MAP	(char *)NULL
#define DEF_PS_GREYSCALE	"no"
#define DEF_PS_DECORATIONS	"no"
#define DEF_PS_FONT_MAP		(char *)NULL
#define DEF_PS_FOOTER		"no"
#define DEF_PS_LEVEL		"1"
#define DEF_PS_HEIGHT		"0"
#define DEF_PS_LANDSCAPE	"no"
#define DEF_PS_PADX		"1.0i"
#define DEF_PS_PADY		"1.0i"
#define DEF_PS_PAPERHEIGHT	"11.0i"
#define DEF_PS_PAPERWIDTH	"8.5i"
#define DEF_PS_WIDTH		"0"
#define DEF_PS_COMMENTS		""

static Blt_ConfigSpec configSpecs[] =
{
    {BLT_CONFIG_BITMASK, "-center", "center", "Center", DEF_PS_CENTER, 
	Blt_Offset(PageSetup, flags), BLT_CONFIG_DONT_SET_DEFAULT, 
        (Blt_CustomOption *)PS_CENTER},
    {BLT_CONFIG_STRING, "-colormap", "colorMap", "ColorMap",
	DEF_PS_COLOR_MAP, Blt_Offset(PageSetup, colorVarName),
	BLT_CONFIG_NULL_OK},
    {BLT_CONFIG_LIST,    "-comments", "comments", "Comments",
	DEF_PS_COMMENTS, Blt_Offset(PageSetup, comments), BLT_CONFIG_NULL_OK},
    {BLT_CONFIG_BITMASK, "-decorations", "decorations", "Decorations",
	DEF_PS_DECORATIONS, Blt_Offset(PageSetup, flags),
	BLT_CONFIG_DONT_SET_DEFAULT, (Blt_CustomOption *)PS_DECORATIONS},
    {BLT_CONFIG_STRING, "-fontmap", "fontMap", "FontMap",
	DEF_PS_FONT_MAP, Blt_Offset(PageSetup, fontVarName),
	BLT_CONFIG_NULL_OK},
    {BLT_CONFIG_BITMASK, "-footer", "footer", "Footer", DEF_PS_FOOTER, 
        Blt_Offset(PageSetup, flags), BLT_CONFIG_DONT_SET_DEFAULT,
        (Blt_CustomOption *)PS_FOOTER},
    {BLT_CONFIG_BITMASK, "-greyscale", "greyscale", "Greyscale",
	DEF_PS_GREYSCALE, Blt_Offset(PageSetup, flags),
	BLT_CONFIG_DONT_SET_DEFAULT, (Blt_CustomOption *)PS_GREYSCALE},
    {BLT_CONFIG_CUSTOM, "-height", "height", "Height", DEF_PS_HEIGHT, 
	Blt_Offset(PageSetup, reqHeight), BLT_CONFIG_DONT_SET_DEFAULT,
	&picaOption},
    {BLT_CONFIG_BITMASK, "-landscape", "landscape", "Landscape",
	DEF_PS_LANDSCAPE, Blt_Offset(PageSetup, flags),
	BLT_CONFIG_DONT_SET_DEFAULT, (Blt_CustomOption *)PS_LANDSCAPE},
    {BLT_CONFIG_INT_POS, "-level", "level", "Level", DEF_PS_LEVEL, 
	Blt_Offset(PageSetup, level), BLT_CONFIG_DONT_SET_DEFAULT},
    {BLT_CONFIG_CUSTOM, "-padx", "padX", "PadX", DEF_PS_PADX, 
	Blt_Offset(PageSetup, xPad), 0, &padOption},
    {BLT_CONFIG_CUSTOM, "-pady", "padY", "PadY", DEF_PS_PADY, 
	Blt_Offset(PageSetup, yPad), 0, &padOption},
    {BLT_CONFIG_CUSTOM, "-paperheight", "paperHeight", "PaperHeight",
	DEF_PS_PAPERHEIGHT, Blt_Offset(PageSetup, reqPaperHeight), 0,
	&picaOption},
    {BLT_CONFIG_CUSTOM, "-paperwidth", "paperWidth", "PaperWidth",
	DEF_PS_PAPERWIDTH, Blt_Offset(PageSetup, reqPaperWidth), 0,
	&picaOption},
    {BLT_CONFIG_CUSTOM, "-width", "width", "Width", DEF_PS_WIDTH, 
        Blt_Offset(PageSetup, reqWidth), BLT_CONFIG_DONT_SET_DEFAULT, 
	&picaOption},
    {BLT_CONFIG_END, NULL, NULL, NULL, NULL, 0, 0}
};

/*
 *---------------------------------------------------------------------------
 *
 * ObjToPicaProc --
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
ObjToPicaProc(
    ClientData clientData,		/* Not used. */
    Tcl_Interp *interp,			/* Interpreter to send results back
					 * to */
    Tk_Window tkwin,			/* Not used. */
    Tcl_Obj *objPtr,			/* New value. */
    char *widgRec,			/* Widget record */
    int offset,				/* Offset to field in structure */
    int flags)				/* Not used. */
{
    int *picaPtr = (int *)(widgRec + offset);

    return Blt_Ps_GetPicaFromObj(interp, objPtr, picaPtr);
}

/*
 *---------------------------------------------------------------------------
 *
 * PicaToObj --
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static Tcl_Obj *
PicaToObjProc(
    ClientData clientData,		/* Not used. */
    Tcl_Interp *interp,			/* Not used. */
    Tk_Window tkwin,			/* Not used. */
    char *widgRec,			/* PostScript structure record */
    int offset,				/* Offset to field in structure */
    int flags)				/* Not used. */
{
    int pica = *(int *)(widgRec + offset);

    return Tcl_NewIntObj(pica);
}

/*
 *---------------------------------------------------------------------------
 *
 * ObjToPad --
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
ObjToPad(
    ClientData clientData,		/* Not used. */
    Tcl_Interp *interp,			/* Interpreter to send results back
					 * to */
    Tk_Window tkwin,			/* Not used. */
    Tcl_Obj *objPtr,			/* New value. */
    char *widgRec,			/* Widget record */
    int offset,				/* Offset to field in structure */
    int flags)				/* Not used. */
{
    Blt_Pad *padPtr = (Blt_Pad *) (widgRec + offset);

    return Blt_Ps_GetPadFromObj(interp, objPtr, padPtr);
}

/*
 *---------------------------------------------------------------------------
 *
 * PadToObj --
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static Tcl_Obj *
PadToObj(
    ClientData clientData,	/* Not used. */
    Tcl_Interp *interp,
    Tk_Window tkwin,		/* Not used. */
    char *widgRec,		/* PostScript structure record */
    int offset,			/* Offset to field in structure */
    int flags)			/* Not used. */
{
    Blt_Pad *padPtr = (Blt_Pad *)(widgRec + offset);
    Tcl_Obj *objPtr, *listObjPtr;
	    
    listObjPtr = Tcl_NewListObj(0, (Tcl_Obj **)NULL);
    objPtr = Tcl_NewIntObj(padPtr->side1);
    Tcl_ListObjAppendElement(interp, listObjPtr, objPtr);
    objPtr = Tcl_NewIntObj(padPtr->side2);
    Tcl_ListObjAppendElement(interp, listObjPtr, objPtr);
    return listObjPtr;
}

static void
AddComments(Blt_Ps ps, const char **comments)
{
    const char **p;

    for (p = comments; *p != NULL; p += 2) {
	if (*(p+1) == NULL) {
	    break;
	}
	Blt_Ps_Format(ps, "%% %s: %s\n", *p, *(p+1));
    }
}

/*
 *---------------------------------------------------------------------------
 *
 * PostScriptPreamble
 *
 *    	The PostScript preamble calculates the needed translation and
 *    	scaling to make X11 coordinates compatible with PostScript.
 *
 *---------------------------------------------------------------------------
 */

#ifdef TIME_WITH_SYS_TIME
#include <sys/time.h>
#include <time.h>
#else
#ifdef HAVE_SYS_TIME_H
#include <sys/time.h>
#else
#include <time.h>
#endif /* HAVE_SYS_TIME_H */
#endif /* TIME_WITH_SYS_TIME */


static int
PostScriptPreamble(Graph *graphPtr, const char *fileName, Blt_Ps ps)
{
    PageSetup *setupPtr = graphPtr->pageSetup;
    time_t ticks;
    char date[200];			/* Holds the date string from ctime() */
    const char *version;
    char *newline;

    if (fileName == NULL) {
	fileName = Tk_PathName(graphPtr->tkwin);
    }
    Blt_Ps_Append(ps, "%!PS-Adobe-3.0 EPSF-3.0\n");

    /*
     * The "BoundingBox" comment is required for EPS files. The box
     * coordinates are integers, so we need round away from the center of
     * the box.
     */
    Blt_Ps_Format(ps, "%%%%BoundingBox: %d %d %d %d\n",
	setupPtr->left, setupPtr->paperHeight - setupPtr->top,
	setupPtr->right, setupPtr->paperHeight - setupPtr->bottom);
	
    Blt_Ps_Append(ps, "%%Pages: 1\n");

    version = Tcl_GetVar(graphPtr->interp, "blt_version", TCL_GLOBAL_ONLY);
    if (version == NULL) {
	version = "???";
    }
    Blt_Ps_Format(ps, "%%%%Creator: (BLT %s %s)\n", version,
	Tk_Class(graphPtr->tkwin));

    ticks = time((time_t *) NULL);
    strcpy(date, ctime(&ticks));
    newline = date + strlen(date) - 1;
    if (*newline == '\n') {
	*newline = '\0';
    }
    Blt_Ps_Format(ps, "%%%%CreationDate: (%s)\n", date);
    Blt_Ps_Format(ps, "%%%%Title: (%s)\n", fileName);
    Blt_Ps_Append(ps, "%%DocumentData: Clean7Bit\n");
    if (setupPtr->flags & PS_LANDSCAPE) {
	Blt_Ps_Append(ps, "%%Orientation: Landscape\n");
    } else {
	Blt_Ps_Append(ps, "%%Orientation: Portrait\n");
    }
    Blt_Ps_Append(ps, "%%DocumentNeededResources: font Helvetica Courier\n");
    AddComments(ps, setupPtr->comments);
    Blt_Ps_Append(ps, "%%EndComments\n\n");
    if (Blt_Ps_IncludeFile(graphPtr->interp, ps, "bltGraph.pro") != TCL_OK) {
	return TCL_ERROR;
    }
    if (setupPtr->flags & PS_FOOTER) {
	const char *who;

	who = getenv("LOGNAME");
	if (who == NULL) {
	    who = "???";
	}
	Blt_Ps_VarAppend(ps,
	    "8 /Helvetica SetFont\n",
	    "10 30 moveto\n",
	    "(Date: ", date, ") show\n",
	    "10 20 moveto\n",
	    "(File: ", fileName, ") show\n",
	    "10 10 moveto\n",
	    "(Created by: ", who, "@", Tcl_GetHostName(), ") show\n",
	    "0 0 moveto\n",
	    (char *)NULL);
    }
    /*
     * Set the conversion from PostScript to X11 coordinates.  Scale pica to
     * pixels and flip the y-axis (the origin is the upperleft corner).
     */
    Blt_Ps_VarAppend(ps,
	"% Transform coordinate system to use X11 coordinates\n\n",
	"% 1. Flip y-axis over by reversing the scale,\n",
	"% 2. Translate the origin to the other side of the page,\n",
	"%    making the origin the upper left corner\n", (char *)NULL);
    Blt_Ps_Format(ps, "1 -1 scale\n");
    /* Papersize is in pixels.  Translate the new origin *after* changing
     * the scale. */
    Blt_Ps_Format(ps, "0 %d translate\n\n", -setupPtr->paperHeight);
    Blt_Ps_VarAppend(ps, "% User defined page layout\n\n",
		     "% Set color level\n", (char *)NULL);
    Blt_Ps_Format(ps, "%% Set origin\n%d %d translate\n\n",
	setupPtr->left, setupPtr->bottom);
    if (setupPtr->flags & PS_LANDSCAPE) {
	Blt_Ps_Format(ps,
	    "%% Landscape orientation\n0 %g translate\n-90 rotate\n",
	    ((double)graphPtr->width * setupPtr->scale));
    }
    Blt_Ps_Append(ps, "\n%%EndSetup\n\n");
    return TCL_OK;
}

static void
MarginsToPostScript(Graph *graphPtr, Blt_Ps ps)
{
    PageSetup *setupPtr = graphPtr->pageSetup;
    XRectangle margin[4];

    Blt_Ps_Append(ps, "% Margins\n");
    margin[0].x = margin[0].y = margin[3].x = margin[1].x = 0;
    margin[0].width = margin[3].width = graphPtr->width;
    margin[0].height = graphPtr->top;
    margin[3].y = graphPtr->bottom;
    margin[3].height = graphPtr->height - graphPtr->bottom;
    margin[2].y = margin[1].y = graphPtr->top;
    margin[1].width = graphPtr->left;
    margin[2].height = margin[1].height = graphPtr->bottom - graphPtr->top;
    margin[2].x = graphPtr->right;
    margin[2].width = graphPtr->width - graphPtr->right;

    /* Clear the surrounding margins and clip the plotting surface */
    if (setupPtr->flags & PS_DECORATIONS) {
	Blt_Ps_XSetBackground(ps,Blt_Bg_BorderColor(graphPtr->normalBg));
    } else {
	Blt_Ps_SetClearBackground(ps);
    }
    Blt_Ps_XFillRectangles(ps, 4, margin);
    
    Blt_Ps_Append(ps, "% Interior 3D border\n");
    /* Interior 3D border */
    if (graphPtr->plotBW > 0) {
	Tk_3DBorder border;
	int x, y, w, h;

	x = graphPtr->left - graphPtr->plotBW;
	y = graphPtr->top - graphPtr->plotBW;
	w = (graphPtr->right - graphPtr->left) + (2*graphPtr->plotBW);
	h = (graphPtr->bottom - graphPtr->top) + (2*graphPtr->plotBW);
	border = Blt_Bg_Border(graphPtr->normalBg);
	Blt_Ps_Draw3DRectangle(ps, border, (double)x, (double)y, w, h,
		graphPtr->plotBW, graphPtr->plotRelief);
    }
    if (Blt_Legend_Site(graphPtr) & LEGEND_MARGIN_MASK) {
	/*
	 * Print the legend if we're using a site which lies in one of the
	 * margins (left, right, top, or bottom) of the graph.
	 */
	Blt_LegendToPostScript(graphPtr, ps);
    }
    if (graphPtr->title != NULL) {
	Blt_Ps_Append(ps, "% Graph title\n");
	Blt_Ps_DrawText(ps, graphPtr->title, &graphPtr->titleTextStyle, 
		(double)graphPtr->titleX, (double)graphPtr->titleY);
    }
    Blt_AxesToPostScript(graphPtr, ps);
}


static int
GraphToPostScript(Graph *graphPtr, const char *ident, Blt_Ps ps)
{
    int x, y, w, h;
    int result;
    PageSetup *setupPtr = graphPtr->pageSetup;

    /*   
     * We need to know how big a graph to print.  If the graph hasn't been
     * drawn yet, the width and height will be 1.  Instead use the
     * requested size of the widget.  The user can still override this with
     * the -width and -height postscript options.
     */
    if (setupPtr->reqWidth > 0) {
	graphPtr->width = setupPtr->reqWidth;
    } else if (graphPtr->width < 2) {
	graphPtr->width = Tk_ReqWidth(graphPtr->tkwin);
    }
    if (setupPtr->reqHeight > 0) {
	graphPtr->height = setupPtr->reqHeight;
    } else if (graphPtr->height < 2) {
	graphPtr->height = Tk_ReqHeight(graphPtr->tkwin);
    }
    Blt_Ps_ComputeBoundingBox(setupPtr, graphPtr->width, graphPtr->height);
    graphPtr->flags |= LAYOUT_NEEDED | RESET_WORLD;

    /* Turn on PostScript measurements when computing the graph's layout. */
    Blt_Ps_SetPrinting(ps, TRUE);
    Blt_ReconfigureGraph(graphPtr);
    Blt_MapGraph(graphPtr);

    result = PostScriptPreamble(graphPtr, ident, ps);
    if (result != TCL_OK) {
	goto error;
    }
    /* Determine rectangle of the plotting area for the graph window */
    x = graphPtr->left - graphPtr->plotBW;
    y = graphPtr->top - graphPtr->plotBW;

    w = (graphPtr->right  - graphPtr->left + 1) + (2*graphPtr->plotBW);
    h = (graphPtr->bottom - graphPtr->top  + 1) + (2*graphPtr->plotBW);

    Blt_Ps_Append(ps, "%%Page: 1 1\n\n");
    Blt_Ps_XSetFont(ps, Blt_Ts_GetFont(graphPtr->titleTextStyle));
    if (graphPtr->pageSetup->flags & PS_DECORATIONS) {
	Blt_Ps_XSetBackground(ps, Blt_Bg_BorderColor(graphPtr->plotBg));
    } else {
	Blt_Ps_SetClearBackground(ps);
    }
    Blt_Ps_XFillRectangle(ps, x, y, w, h);
    Blt_Ps_Rectangle(ps, x, y, w, h);
    Blt_Ps_Append(ps, "gsave clip\n\n");
    /* Draw the grid, elements, and markers in the plotting area. */
    Blt_GridsToPostScript(graphPtr, ps);
    Blt_MarkersToPostScript(graphPtr, ps, TRUE);
    if ((Blt_Legend_Site(graphPtr) & LEGEND_PLOTAREA_MASK) && 
	(!Blt_Legend_IsRaised(graphPtr))) {
	/* Print legend underneath elements and markers */
	Blt_LegendToPostScript(graphPtr, ps);
    }
    Blt_AxisLimitsToPostScript(graphPtr, ps);
    Blt_ElementsToPostScript(graphPtr, ps);
    if ((Blt_Legend_Site(graphPtr) & LEGEND_PLOTAREA_MASK) && 
	(Blt_Legend_IsRaised(graphPtr))) {
	/* Print legend above elements (but not markers) */
	Blt_LegendToPostScript(graphPtr, ps);
    }
    Blt_MarkersToPostScript(graphPtr, ps, FALSE);
    Blt_ActiveElementsToPostScript(graphPtr, ps);
    Blt_Ps_VarAppend(ps, "\n",
	"% Unset clipping\n",
	"grestore\n\n", (char *)NULL);
    MarginsToPostScript(graphPtr, ps);
    Blt_Ps_VarAppend(ps,
	"showpage\n",
	"%Trailer\n",
	"grestore\n",
	"end\n",
	"%%Trailer\n",
	"%%EOF\n", (char *)NULL);
  error:
    /* Reset height and width of graph window */
    graphPtr->width = Tk_Width(graphPtr->tkwin);
    graphPtr->height = Tk_Height(graphPtr->tkwin);
    graphPtr->flags |= MAP_WORLD;
    Blt_Ps_SetPrinting(ps, FALSE);
    Blt_ReconfigureGraph(graphPtr);
    Blt_MapGraph(graphPtr);
    /*
     * Redraw the graph in order to re-calculate the layout as soon as
     * possible. This is in the case the crosshairs are active.
     */
    Blt_EventuallyRedrawGraph(graphPtr);
    return result;
}


/*
 *---------------------------------------------------------------------------
 *
 * CgetOp --
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
CgetOp(Graph *graphPtr, Tcl_Interp *interp, int objc, Tcl_Obj *const *objv)
{
    PageSetup *setupPtr = graphPtr->pageSetup;

    if (Blt_ConfigureValueFromObj(interp, graphPtr->tkwin, configSpecs, 
	(char *)setupPtr, objv[3], 0) != TCL_OK) {
	return TCL_ERROR;
    }
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * ConfigureOp --
 *
 *      This procedure is invoked to print the graph in a file.
 *
 * Results:
 *      A standard TCL result.
 *
 * Side effects:
 *      A new PostScript file is created.
 *
 *---------------------------------------------------------------------------
 */
static int
ConfigureOp(Graph *graphPtr, Tcl_Interp *interp, int objc, Tcl_Obj *const *objv)
{
    int flags = BLT_CONFIG_OBJV_ONLY;
    PageSetup *setupPtr = graphPtr->pageSetup;

    if (objc == 3) {
	return Blt_ConfigureInfoFromObj(interp, graphPtr->tkwin, configSpecs,
		(char *)setupPtr, (Tcl_Obj *)NULL, flags);
    } else if (objc == 4) {
	return Blt_ConfigureInfoFromObj(interp, graphPtr->tkwin, configSpecs,
		(char *)setupPtr, objv[3], flags);
    }
    if (Blt_ConfigureWidgetFromObj(interp, graphPtr->tkwin, configSpecs, 
	    objc - 3, objv + 3, (char *)setupPtr, flags) != TCL_OK) {
	return TCL_ERROR;
    }
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * OutputOp --
 *
 *      This procedure is invoked to print the graph in a file.
 *
 * Results:
 *      Standard TCL result.  TCL_OK if plot was successfully printed, 
 *      TCL_ERROR otherwise.
 *
 * Side effects:
 *      A new PostScript file is created.
 *
 *---------------------------------------------------------------------------
 */
static int
OutputOp(Graph *graphPtr, Tcl_Interp *interp, int objc, Tcl_Obj *const *objv)
{
    const char *buffer;
    PostScript *psPtr;
    Tcl_Channel channel;
    const char *fileName;		/* Name of file to write PostScript
					 * output If NULL, output is returned
					 * via interp->result. */
    int length;

    fileName = NULL;			/* Used to identify the output sink. */
    channel = NULL;
    if (objc > 3) {
	fileName = Tcl_GetString(objv[3]);
	if (fileName[0] != '-') {
	    objv++, objc--;		/* First argument is the file name. */
	    channel = Tcl_OpenFileChannel(interp, fileName, "w", 0666);
	    if (channel == NULL) {
		return TCL_ERROR;
	    }
	    if (Tcl_SetChannelOption(interp, channel, "-translation", "binary") 
		!= TCL_OK) {
		return TCL_ERROR;
	    }
	}
    }

    psPtr = Blt_Ps_Create(graphPtr->interp, graphPtr->pageSetup);
    if (Blt_ConfigureWidgetFromObj(interp, graphPtr->tkwin, configSpecs, 
	objc - 3, objv + 3, (char *)graphPtr->pageSetup, BLT_CONFIG_OBJV_ONLY) 
	!= TCL_OK) {
	return TCL_ERROR;
    }
    if (GraphToPostScript(graphPtr, fileName, psPtr) != TCL_OK) {
	goto error;
    }
    buffer = Blt_Ps_GetValue(psPtr, &length);
    if (channel != NULL) {
	int numBytes;
	/*
	 * If a file name was given, write the results to that file
	 */
	numBytes = Tcl_Write(channel, buffer, length);
	if (numBytes < 0) {
	    Tcl_AppendResult(interp, "error writing file \"", fileName, "\": ",
		Tcl_PosixError(interp), (char *)NULL);
	    goto error;
	}
        Tcl_Close(interp, channel);
    } else {
	Tcl_SetStringObj(Tcl_GetObjResult(interp), buffer, length);
    }
    Blt_Ps_Free(psPtr);
    return TCL_OK;

  error:
    if (channel != NULL) {
        Tcl_Close(interp, channel);
    }
    Blt_Ps_Free(psPtr);
    return TCL_ERROR;
}

/*
 *---------------------------------------------------------------------------
 *
 * Blt_CreatePostScript --
 *
 *      Creates a postscript structure.
 *
 * Results:
 *      Always TCL_OK.
 *
 * Side effects:
 *      A new PostScript structure is created.
 *
 *---------------------------------------------------------------------------
 */
int
Blt_CreatePageSetup(Graph *graphPtr)
{
    PageSetup *setupPtr;

    setupPtr = Blt_AssertCalloc(1, sizeof(PostScript));
    setupPtr->flags = PS_CENTER;
    setupPtr->level = 1;
    graphPtr->pageSetup = setupPtr;

    if (Blt_ConfigureComponentFromObj(graphPtr->interp, graphPtr->tkwin,
	    "postscript", "Postscript", configSpecs, 0, (Tcl_Obj **)NULL,
	    (char *)setupPtr, 0) != TCL_OK) {
	return TCL_ERROR;
    }
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * Blt_PostScriptOp --
 *
 *	This procedure is invoked to process the TCL command that corresponds
 *	to a widget managed by this module.  See the user documentation for
 *	details on what it does.
 *
 * Results:
 *	A standard TCL result.
 *
 * Side effects:
 *	See the user documentation.
 *
 *---------------------------------------------------------------------------
 */
static Blt_OpSpec psOps[] =
{
    {"cget",      2, CgetOp,      4, 4, "option",},
    {"configure", 2, ConfigureOp, 3, 0, "?option value?...",},
    {"output",    1, OutputOp,    3, 0, "?fileName? ?option value?...",},
};

static int numPsOps = sizeof(psOps) / sizeof(Blt_OpSpec);

int
Blt_PostScriptOp(Graph *graphPtr, Tcl_Interp *interp, int objc, 
	Tcl_Obj *const *objv)
{
    GraphPsProc *proc;
    int result;

    proc = Blt_GetOpFromObj(interp, numPsOps, psOps, BLT_OP_ARG2, objc, objv, 0);
    if (proc == NULL) {
	return TCL_ERROR;
    }
    result = (*proc) (graphPtr, interp, objc, objv);
    return result;
}

void
Blt_DestroyPageSetup(Graph *graphPtr)
{
    if (graphPtr->pageSetup != NULL) {
	Blt_FreeOptions(configSpecs, (char *)graphPtr->pageSetup, 
		graphPtr->display, 0);
	Blt_Free(graphPtr->pageSetup);
    }
}
