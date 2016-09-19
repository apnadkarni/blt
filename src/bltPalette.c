/* -*- mode: c; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 * bltPalette.c --
 *
 * This module implements palettes for the BLT graph widget.
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

#ifdef HAVE_STDLIB_H
  #include <stdlib.h>
#endif /* HAVE_STDLIB_H */

#ifdef HAVE_STRING_H
  #include <string.h>
#endif /* HAVE_STRING_H */

#ifdef HAVE_UNISTD_H
  #include <unistd.h>
#endif /* HAVE_UNISTD_H */

#ifdef HAVE_CTYPE_H
  #include <ctype.h>
#endif /* HAVE_CTYPE_H */

#ifdef HAVE_ERRNO_H
  #include <errno.h>
#endif /* HAVE_ERRNO_H */

#include "bltMath.h"
#include "bltPicture.h"
#include "bltPalette.h"

#define PALETTE_THREAD_KEY "BLT Palette Command Interface"
#define RCLAMP(c)       ((((c) < 0.0) ? 0.0 : ((c) > 1.0) ? 1.0 : (c)))

#define imul8x8(a,b,t)  ((t) = (a)*(b)+128,(((t)+((t)>>8))>>8))
#define CLAMP(c)        ((((c) < 0) ? 0 : ((c) > 255) ? 255 : (c)))

#define LOADED  (1<<0)

/*
 * PaletteCmdInterpData --
 *
 *      Structure containing global data, used on a interpreter by
 *      interpreter basis.
 *
 *      This structure holds the hash table of instances of datatable
 *      commands associated with a particular interpreter.
 */
typedef struct {
    Blt_HashTable paletteTable;         /* Tracks palettes in use. */
    Tcl_Interp *interp;
    int nextId;
} PaletteCmdInterpData;

/*
 *---------------------------------------------------------------------------
 *
 * PaletteInterval --
 *
 *      Represents an interval in the palette.  The interval is represented
 *      by its minimum and maximum values and the low and high colors to
 *      interpolate between.
 *
 *---------------------------------------------------------------------------
 */
typedef struct {
    Blt_Pixel low, high;                /* Range of colors. */
    double min, max;                    /* Range of z values. */
} PaletteInterval;


typedef struct _Blt_Palette {
    unsigned int flags;
    int refCount;
    PaletteInterval *colors;            /* Array of color ranges. */
    PaletteInterval *opacities;         /* Array of opacity ranges. */
    double maxColorValue;               /* Specifies the maximum RGB value.
                                         * 1, 255, ... */
    double min, max;                    /* Absolute min and max
                                         * values. This is needed if the
                                         * palette contains absolute values.
                                         * normalize the palette entries
                                         * to relative 0..1 values. */
    int numColors;                      /* # of entries in color array. */
    int numOpacities;                   /* # of entries in opacity
                                         * array. */
    int alpha;

    /* Additional fields for TCL API.  */
    PaletteCmdInterpData *dataPtr;      /*  */
    const char *name;                   /* Namespace-specific name for this
                                         * palette. */
    Blt_HashEntry *hashPtr;             /* Pointer to this entry in palette
                                         * hash table.  */
    int opacity;                        /* Overall opacity adjustment. */
    Tcl_Obj *colorFileObjPtr;           /* Name of file to use for color
                                         * data. */
    Tcl_Obj *colorDataObjPtr;           /* String to use for color data. */
    Tcl_Obj *opacityFileObjPtr;         /* Name of file to use for opacity
                                         * data. */
    Tcl_Obj *opacityDataObjPtr;         /* String to use for opacity
                                         * data. */
    unsigned int colorFlags;
    unsigned int opacityFlags;
    Blt_Chain notifiers;
} Palette;

typedef struct _PaletteNotifier {
    const char *name;                   /* Token id for notifier. */
    Blt_Palette_NotifyProc *proc;       /* Procedure to be called when the
                                         * paintbrush changes. */
    ClientData clientData;              /* Data to be passed on notifier
                                         * callbacks.  */
} PaletteNotifier;

static Blt_SwitchParseProc ObjToSpacing;
static Blt_SwitchCustom spacingSwitch =
{
    ObjToSpacing, NULL, NULL, (ClientData)0
};

static Blt_SwitchParseProc ObjToColorFormat;
static Blt_SwitchCustom colorFormatSwitch =
{
    ObjToColorFormat, NULL, NULL, (ClientData)0
};

static Blt_SwitchParseProc ObjToFade;
static Blt_SwitchCustom fadeSwitch =
{
    ObjToFade, NULL, (ClientData)0
};

#define SPACING_REGULAR         (1<<0)
#define SPACING_IRREGULAR       (1<<1)
#define SPACING_INTERVAL        (1<<2)
#define SPACING_MASK         \
        (SPACING_REGULAR|SPACING_IRREGULAR|SPACING_INTERVAL)
#define COLOR_NAME             (1<<3)
#define COLOR_RGB              (1<<4)
#define COLOR_HSV              (1<<5)
#define COLOR_MASK             (COLOR_RGB|COLOR_NAME|COLOR_HSV)

static Blt_SwitchSpec paletteSpecs[] =
{
    {BLT_SWITCH_OBJ, "-cdata", "dataString", (char *)NULL, 
        Blt_Offset(Palette, colorDataObjPtr), 0, 0},
    {BLT_SWITCH_OBJ, "-cfile", "fileName", (char *)NULL, 
        Blt_Offset(Palette, colorFileObjPtr), 0, 0},
    {BLT_SWITCH_OBJ, "-colordata", "dataString", (char *)NULL, 
        Blt_Offset(Palette, colorDataObjPtr), 0, 0},
    {BLT_SWITCH_OBJ, "-colorfile", "fileName", (char *)NULL, 
        Blt_Offset(Palette, colorFileObjPtr), 0, 0},
    {BLT_SWITCH_CUSTOM, "-colorformat", "formatName", (char *)NULL, 
        Blt_Offset(Palette, colorFlags), 0, 0, &colorFormatSwitch},
    {BLT_SWITCH_CUSTOM, "-colorspacing", "spacingName",
        (char *)NULL, Blt_Offset(Palette, colorFlags), 0, 0, &spacingSwitch},
    {BLT_SWITCH_CUSTOM, "-fade", "percentFade", (char *)NULL,
        Blt_Offset(Palette, alpha), 0, 0, &fadeSwitch},
    {BLT_SWITCH_OBJ, "-odata", "dataString", (char *)NULL, 
        Blt_Offset(Palette, opacityDataObjPtr), 0, 0},
    {BLT_SWITCH_OBJ, "-ofile", "fileName", (char *)NULL, 
        Blt_Offset(Palette, opacityFileObjPtr), 0, 0},
    {BLT_SWITCH_OBJ, "-opacityfile", "fileName", (char *)NULL, 
        Blt_Offset(Palette, opacityFileObjPtr), 0, 0},
    {BLT_SWITCH_OBJ, "-opacitydata", "dataString", (char *)NULL, 
        Blt_Offset(Palette, opacityDataObjPtr), 0, 0},
    {BLT_SWITCH_CUSTOM, "-opacityspacing", "spacingName",
        (char *)NULL, Blt_Offset(Palette, opacityFlags), 0, 0, &spacingSwitch},
    {BLT_SWITCH_END}
};

static PaletteCmdInterpData *GetPaletteCmdInterpData(Tcl_Interp *interp);

static int LoadData(Tcl_Interp *interp, Palette *palPtr);

typedef int (GetColorProc)(Tcl_Interp *interp, Palette *palPtr,
                           Tcl_Obj **objv, Blt_Pixel *colorPtr);

#define MAXRELERROR 0.0005
#define MAXABSERROR 0.0000005

#define SetColor(c,r,g,b) ((c)->Red = (int)((r) * 255.0), \
                           (c)->Green = (int)((g) * 255.0), \
                           (c)->Blue = (int)((b) * 255.0))

#ifdef notdef
static void
PixelToHSV(Blt_Pixel *colorPtr, float *huePtr, float *satPtr, float *valPtr)
{
    unsigned short max, min;
    double range;
    float h, s, v;
    
    /* Find the minimum and maximum RGB intensities */
    max = MAX3(colorPtr->Red, colorPtr->Green, colorPtr->Blue);
    min = MIN3(colorPtr->Red, colorPtr->Green, colorPtr->Blue);

    v = (double)max / 255.0;
    h = s = 0.0;

    range = (double)(max - min);
    if (max != min) {
        s = range / (double)max;
    }
    if (s > 0.0) {
        double r, g, b;

        /* Normalize the RGB values */
        r = (double)(max - colorPtr->Red)   / range;
        g = (double)(max - colorPtr->Green) / range;
        b = (double)(max - colorPtr->Blue)  / range;

        if (colorPtr->Red == max) {
            h = (b - g);
        } else if (colorPtr->Green == max) {
            h = 2 + (r - b);
        } else if (colorPtr->Blue == max) {
            h = 4 + (g - r);
        }
        h *= 60.0;
    } else {
        s = 0.5;
    }
    if (h < 0.0) {
        h += 360.0;
    }
    *huePtr = h, *satPtr = s, *valPtr = v;
}
#endif

static void
HSVToPixel(double hue, double sat, double val, Blt_Pixel *colorPtr)
{
    double xhue, p, q, t;
    double frac;
    int quadrant;

    if (val < 0.0) {
        val = 0.0;
    } else if (val > 1.0) {
        val = 1.0;
    }
    if (sat == 0.0) {
        SetColor(colorPtr, val, val, val);
        return;
    }
    xhue = FMOD(hue, 360.0) / 60.0;
    quadrant = (int)floor(xhue);
    frac = hue - quadrant;
    p = val * (1 - (sat));
    q = val * (1 - (sat * frac));
    t = val * (1 - (sat * (1 - frac)));

    switch (quadrant) {
    case 0:
        SetColor(colorPtr, val, t, p);
        break;
    case 1:
        SetColor(colorPtr, q, val, p);
        break;
    case 2:
        SetColor(colorPtr, p, val, t);
        break;
    case 3:
        SetColor(colorPtr, p, q, val);
        break;
    case 4:
        SetColor(colorPtr, t, p, val);
        break;
    case 5:
        SetColor(colorPtr, val, p, q);
        break;
    }
}

static int 
CompareEntries(const void *a, const void *b)
{
    const PaletteInterval *e1Ptr, *e2Ptr;

    e1Ptr = a;
    e2Ptr = b;
    if (e1Ptr->min < e2Ptr->min) {
        return -1;
    } else if (e1Ptr->min > e2Ptr->min) {
        return 1;
    }
    return 0;
}

static void
SortEntries(size_t numEntries, PaletteInterval *entries)
{
    qsort((char *)entries, numEntries, sizeof(PaletteInterval),
          CompareEntries);
}

static int 
RelativeError(double x, double y) 
{
    double e;

    if (fabs(x - y) < MAXABSERROR) {
        return TRUE;
    }
   if (fabs(x) > fabs(y)) {
        e = fabs((x - y) / y);
    } else {
        e = fabs((x - y) / x);
    }
    if (e <= MAXRELERROR) {
        return TRUE;
    }
    return FALSE;
}

static int
InRange(double x, double min, double max)
{
    double range;

    range = max - min;
    if (fabs(range) < DBL_EPSILON) {
        return Blt_AlmostEquals(x, max);
    } else {
        double t;

        t = (x - min) / range;
        if ((t >= 0.0) &&  (t <= 1.0)) {
            return TRUE;
        }
        if (RelativeError(0.0, t) || RelativeError(1.0, t)) {
            return TRUE;
        }
    }
    return FALSE;
}

static void
NotifyClients(Palette *palPtr, unsigned int flags)
{
     Blt_ChainLink link;

     for (link = Blt_Chain_FirstLink(palPtr->notifiers); link != NULL;
        link = Blt_Chain_NextLink(link)) {
         PaletteNotifier *notifyPtr;

         /* Notify each client that the paint brush has changed. The client
          * should schedule itself for redrawing.  */
         notifyPtr = Blt_Chain_GetValue(link);
        if (notifyPtr->proc != NULL) {
            (*notifyPtr->proc)(palPtr, notifyPtr->clientData, flags);
        }
    }
}

static int
GetPalette(Tcl_Interp *interp, PaletteCmdInterpData *dataPtr, 
              const char *string, Palette **palPtrPtr)
{
    Blt_HashEntry *hPtr;

    hPtr = Blt_FindHashEntry(&dataPtr->paletteTable, string);
    if (hPtr == NULL) {
        if (interp != NULL) {
            Tcl_AppendResult(interp, "can't find a palette \"", string, "\"",
                         (char *)NULL);
        }
        return TCL_ERROR;
    }
    *palPtrPtr = Blt_GetHashValue(hPtr);
    return TCL_OK;
}

static int
GetPaletteFromObj(Tcl_Interp *interp, PaletteCmdInterpData *dataPtr, 
                     Tcl_Obj *objPtr, Palette **palPtrPtr)
{
    return GetPalette(interp, dataPtr, Tcl_GetString(objPtr), palPtrPtr);
}

static int
GetStepFromObj(Tcl_Interp *interp, Tcl_Obj *objPtr, double *stepPtr)
{
    const char *string;
    char *end;
    double value;
    extern int errno;
    
    string = Tcl_GetString(objPtr);
    errno = 0;
    value = strtod(string, &end);
    if (end == string) {
        Tcl_AppendResult(interp, "expected floating-point number "
                "but got \"", string, "\"", (char *) NULL);
        return TCL_ERROR;
    }
    if ((errno != 0) && ((value == HUGE_VAL) || (value == -HUGE_VAL) ||
        (value == 0))) {
        if (interp != NULL) {
            Tcl_AppendResult(interp,  "value \"", string,
                        "\" can't be represented: ", strerror(errno),
                        (char *)NULL);
            Tcl_SetErrorCode(interp, "ARITH", Tcl_ErrnoId(),
                             Tcl_ErrnoMsg(errno), (char *) NULL);
        }
        return TCL_ERROR;
    }
    if (*end == '%') {
        if ((value < 0.0) || (value > 100.0)) {
            Tcl_AppendResult(interp, "relative value is out of range "
                "\"", string, "\"", (char *) NULL);
            return TCL_ERROR;
        }
        value *= 0.01;
        end++;
    }        
    while ((*end != 0) && isspace(UCHAR(*end))) { /* INTL: ISO space. */
        end++;
    }
    if (*end != 0) {
        Tcl_AppendResult(interp,
                "unexpected characters trailing floating-point number "
                "\"", string, "\"", (char *) NULL);
        return TCL_ERROR;
    }
    *stepPtr = value;
    return TCL_OK;
}

static int
GetOpacity(Tcl_Interp *interp, Tcl_Obj *objPtr, Blt_Pixel *pixelPtr)
{
    double value;

    if (Tcl_GetDoubleFromObj(interp, objPtr, &value) != TCL_OK) {
        return TCL_ERROR;
    }
    if ((value < 0.0) || (value > 1.0)) {
        if (interp != NULL) {
            Tcl_AppendResult(interp, "bad opacity value \"",
                Tcl_GetString(objPtr), "\": should be 0.0 - 1.0",
                (char *)NULL);
        }
        return TCL_ERROR;
    }
    pixelPtr->u32 = 0;
    pixelPtr->Alpha = (int)(value * 255.0);
    return TCL_OK;
}

static int
AutoTestRGBs(Tcl_Interp *interp, Palette *palPtr, int numComponents,
             int objc, Tcl_Obj **objv)
{
    int i;
    double max;
    
    max = 0.0;
    switch (numComponents) {
    case 3:                             /* Regular RGB triplets */
        for (i = 0; i < objc; i++) {
            double x;
            
            if (Tcl_GetDoubleFromObj(interp, objv[i], &x) != TCL_OK) {
                return TCL_ERROR;
            }
            if (x > max) {
                max = x;
            }
        }
        break;
    case 4:                             /* Irregular RGB triplets */
        for (i = 0; i < objc; i += 4) {
            int j;
            
            for (j = 1; j < 4; j++) {
                double x;
                if (Tcl_GetDoubleFromObj(interp, objv[i+j], &x) != TCL_OK) {
                    return TCL_ERROR;
                }
                if (x > max) {
                    max = x;
                }
            }
        }
        break;
    case 8:                             /* Interval RGB triplets  */
        for (i = 0; i < objc; i += 8) {
            static int indices[] = { 1, 2, 3, 5, 6, 7 };
            int j;
            
            for (j = 0; j < 6; j++) {
                double x;
                int index;

                index = i + indices[j];
                if (Tcl_GetDoubleFromObj(interp, objv[index], &x) != TCL_OK) {
                    return TCL_ERROR;
                }
                if (x > max) {
                    max = x;
                }
            }
        }
        break;
    }
     
    if (max > 255.0) {
        palPtr->maxColorValue = 65535;
    } else if (max > 1.0) {
        palPtr->maxColorValue = 255;
    } else {
        palPtr->maxColorValue = 1.0;
    }
    return TCL_OK;
}

static int
GetRGBFromObjv(Tcl_Interp *interp, Palette *palPtr, Tcl_Obj **objv,
               Blt_Pixel *colorPtr) 
{
    double r, g, b;
    Blt_Pixel color;

    if (Tcl_GetDoubleFromObj(interp, objv[0], &r) != TCL_OK) {
        return TCL_ERROR;
    }
    if (r < 0.0) { 
        r = 0.0;
    } else if (r > palPtr->maxColorValue) {
        r = palPtr->maxColorValue;
    }
    if (Tcl_GetDoubleFromObj(interp, objv[1], &g) != TCL_OK) {
        return TCL_ERROR;
    }
    if (g < 0.0) { 
        g = 0.0;
    } else if (g > palPtr->maxColorValue) {
        g = palPtr->maxColorValue;
    }
    if (Tcl_GetDoubleFromObj(interp, objv[2], &b) != TCL_OK) {
        return TCL_ERROR;
    }
    if (b < 0.0) { 
        b = 0.0;
    } else if (b > palPtr->maxColorValue) {
        b = palPtr->maxColorValue;
    }
    color.Red   = (int)((r / palPtr->maxColorValue) * 255.0);
    color.Green = (int)((g / palPtr->maxColorValue) * 255.0);
    color.Blue  = (int)((b / palPtr->maxColorValue) * 255.0);
    color.Alpha = 0xFF;
    colorPtr->u32 = color.u32;
    return TCL_OK;
}

static int
GetHSVFromObjv(Tcl_Interp *interp, Palette *palPtr, Tcl_Obj **objv,
               Blt_Pixel *colorPtr) 
{
    double h, s, v;

    if (Tcl_GetDoubleFromObj(interp, objv[0], &h) != TCL_OK) {
        return TCL_ERROR;
    }
    if (Tcl_GetDoubleFromObj(interp, objv[1], &s) != TCL_OK) {
        return TCL_ERROR;
    }
    if (Tcl_GetDoubleFromObj(interp, objv[2], &v) != TCL_OK) {
        return TCL_ERROR;
    }
    HSVToPixel(h, s, v, colorPtr);
    colorPtr->Alpha = 0xFF;
    return TCL_OK;
}

static const char *
NameOfSpacing(int flags)
{
    switch (flags & SPACING_MASK) {
    case SPACING_REGULAR:
        return "regular";
    case SPACING_IRREGULAR:
        return "irregular";
    case SPACING_INTERVAL:
        return "interval";
    default:
        return "???";
    }
}

static const char *
NameOfColorType(int flags)
{
    switch (flags & COLOR_MASK) {
    case COLOR_RGB:
        return "rgb";
    case COLOR_HSV:
        return "hsv";
    case COLOR_NAME:
        return "name";
    default:
        return "???";
    }
}

/*
 *---------------------------------------------------------------------------
 *
 * ObjToSpacing --
 *
 *      Converts the Tcl_Obj into its spacing flag.
 *
 *      Valid spacing values are:
 *
 *      regular                Uniform spacing of entries.  Each
 *                             entry consists of 1 or 3 color values.
 *      irregular              Nonuniform spacing of entries. Each
 *                             entry will also contain a value 
 *                             (absolute or relative) indicating
 *                             the location of entry.
 *      interval               Each entry describes an interval
 *                             consisting of the min value and
 *                             associated color and the max value
 *                             and its associate color.
 *
 * Results:
 *      A standard TCL result.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
ObjToSpacing(ClientData clientData, Tcl_Interp *interp, const char *switchName,
             Tcl_Obj *objPtr, char *record, int offset, int flags)      
{
    unsigned int *flagsPtr = (unsigned int *)(record + offset);
    char c;
    const char *string;
    unsigned int flag;
    
    string = Tcl_GetString(objPtr);
    c = string[0];
    if ((c == 'r') && (strcmp(string, "regular") == 0)) {
        flag = SPACING_REGULAR;
    } else if ((c == 'i') && (strcmp(string, "irregular") == 0)) {
        flag = SPACING_IRREGULAR;
    } else if ((c == 'i') && (strcmp(string, "interval") == 0)) {
        flag = SPACING_INTERVAL;
    } else {
        Tcl_AppendResult(interp, "bad spacing value \"", string,
                         "\": should be regular, irregular, or interval.",
                         (char *)NULL);
        return TCL_ERROR;
    }
    *flagsPtr &= ~SPACING_MASK;
    *flagsPtr |= flag;
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * ObjToColorFormat --
 *
 *      Converts the Tcl_Obj into its color format flag.
 *
 *      Valid color format values are:
 *
 *      -colorformat name               Each color is specified by a single
 *                                      color name or #hex string.
 *      -colorformat rgb                Each color is a triplet of red, green,
 *                                      and blue values (0..1).
 *      -colorformat hsv                Each color is a triplet of hue, sat,
 *                                      and val (0..1).
 *
 * Results:
 *      A standard TCL result.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
ObjToColorFormat(ClientData clientData, Tcl_Interp *interp,
                const char *switchName, Tcl_Obj *objPtr, char *record,
                int offset, int flags)  
{
    unsigned int *flagsPtr = (unsigned int *)(record + offset);
    char c;
    const char *string;
    unsigned int flag;
    
    string = Tcl_GetString(objPtr);
    c = string[0];
    if ((c == 'r') && (strcmp(string, "rgb") == 0)) {
        flag = COLOR_RGB;
    } else if ((c == 'n') && (strcmp(string, "name") == 0)) {
        flag = COLOR_NAME;
    } else if ((c == 'h') && (strcmp(string, "hsv") == 0)) {
        flag = COLOR_HSV;
    } else {
        Tcl_AppendResult(interp, "bad color format value \"", string,
                         "\": should be rgb or name.",
                         (char *)NULL);
        return TCL_ERROR;
    }
    *flagsPtr &= ~COLOR_MASK;
    *flagsPtr |= flag;
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * ObjToFade --
 *
 *      Convert the string representation of opacity (a percentage) to
 *      an alpha value 0..255.
 *
 * Results:
 *      The return value is a standard TCL result.  
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
ObjToFade(
    ClientData clientData,              /* Not used. */
    Tcl_Interp *interp,                 /* Interpreter to send results back
                                         * to */
    const char *switchName,
    Tcl_Obj *objPtr,                    /* Opacity string */
    char *record,                       /* Palette structure record */
    int offset,                         /* Offset to field in structure */
    int flags)  
{
    int *alphaPtr = (int *)(record + offset);
    double fade, opacity;

    if (Tcl_GetDoubleFromObj(interp, objPtr, &fade) != TCL_OK) {
        return TCL_ERROR;
    }
    if ((fade < 0.0) || (fade > 100.0)) {
        Tcl_AppendResult(interp, "invalid percent opacity \"", 
                Tcl_GetString(objPtr), "\" should be 0 to 100", (char *)NULL);
        return TCL_ERROR;
    }
    opacity = (1.0 - (fade / 100.0)) * 255.0;
    *alphaPtr = ROUND(opacity);
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * NewPalette --
 *
 *      Creates a new palette command structure and inserts it into the
 *      interpreter's hash table.
 *
 *---------------------------------------------------------------------------
 */
static Palette *
NewPalette(Tcl_Interp *interp, PaletteCmdInterpData *dataPtr, 
              const char *name)
{
    Palette *palPtr;
    Blt_HashEntry *hPtr;
    int isNew;

    palPtr = Blt_AssertCalloc(1, sizeof(Palette));
    hPtr = Blt_CreateHashEntry(&dataPtr->paletteTable, name, &isNew);
    if (!isNew) {
        Tcl_AppendResult(interp, "palette \"", name, "\" already exists",
                         (char *)NULL);
        return NULL;
    }
    palPtr->colorFlags = COLOR_RGB | SPACING_REGULAR;
    palPtr->refCount = 1;
    palPtr->opacityFlags = SPACING_REGULAR;
    palPtr->alpha = 0xFF;
    palPtr->name = Blt_GetHashKey(&dataPtr->paletteTable, hPtr);
    Blt_SetHashValue(hPtr, palPtr);
    palPtr->maxColorValue = 1.0;
    palPtr->hashPtr = hPtr;
    palPtr->dataPtr = dataPtr;
    return palPtr;
}

/*
 *---------------------------------------------------------------------------
 *
 * DestroyPalette --
 *
 *---------------------------------------------------------------------------
 */
static void
DestroyPalette(Palette *palPtr)
{
    NotifyClients(palPtr, PALETTE_DELETE_NOTIFY);
    if (palPtr->hashPtr != NULL) {
        Blt_DeleteHashEntry(&palPtr->dataPtr->paletteTable, palPtr->hashPtr);
    }
    Blt_FreeSwitches(paletteSpecs, (char *)palPtr, 0);
    if (palPtr->notifiers != NULL) {
        Blt_Chain_Destroy(palPtr->notifiers);
    }
    if (palPtr->colors != NULL) {
        Blt_Free(palPtr->colors);
    }
    if (palPtr->opacities != NULL) {
        Blt_Free(palPtr->opacities);
    }
    Blt_Free(palPtr);
}

static void
DestroyPalettes(PaletteCmdInterpData *dataPtr)
{
    Blt_HashEntry *hPtr;
    Blt_HashSearch iter;

    for (hPtr = Blt_FirstHashEntry(&dataPtr->paletteTable, &iter); hPtr != NULL;
         hPtr = Blt_NextHashEntry(&iter)) {
        Palette *palPtr;

        palPtr = Blt_GetHashValue(hPtr);
        palPtr->hashPtr = NULL;
        DestroyPalette(palPtr);
    }
}

/*
 *      "c1 c2 c3 c4.."
 */
static int
ParseRegularColorNames(Tcl_Interp *interp, Palette *palPtr, int objc, 
                      Tcl_Obj **objv)
{
    PaletteInterval *entries;
    double step;
    int i, numEntries;

    numEntries = objc - 1;
    entries = Blt_AssertMalloc(sizeof(PaletteInterval) * numEntries);
    step = 1.0 / numEntries;
    for (i = 0; i < numEntries; i++) {
        PaletteInterval *entryPtr;
        Blt_Pixel low, high;
        
        entryPtr = entries + i;
        if ((Blt_GetPixelFromObj(interp, objv[i], &low) != TCL_OK) ||
            (Blt_GetPixelFromObj(interp, objv[i+1], &high) != TCL_OK)) {
            goto error;
        }
        entryPtr->high.u32 = high.u32;
        entryPtr->low.u32 = low.u32;
        entryPtr->min = i * step;
        entryPtr->max = (i+1) * step;
    }
    if (palPtr->colors != NULL) {
        Blt_Free(palPtr->colors);
    }
    palPtr->colors = entries;
    palPtr->numColors = numEntries;
    palPtr->min = 0.0;
    palPtr->max = 1.0;
    return TCL_OK;
 error:
    Blt_Free(entries);
    return TCL_ERROR;
}

/*
 *      "r g b  r b g  r g b"...
 */
static int
ParseRegularColorTriplets(Tcl_Interp *interp, Palette *palPtr, int objc, 
                          Tcl_Obj **objv)
{
    PaletteInterval *entries;
    double step;
    int i, j, numEntries;
    GetColorProc *proc;

    proc = (palPtr->colorFlags & COLOR_RGB) ? GetRGBFromObjv : GetHSVFromObjv;
    numEntries = (objc / 3) - 1;
    entries = Blt_AssertMalloc(sizeof(PaletteInterval) * numEntries);
    step = 1.0 / numEntries;
    for (i = j = 0; j < numEntries; i += 3, j++) {
        PaletteInterval *entryPtr;

        entryPtr = entries + j;
        if (((*proc)(interp, palPtr, objv+i, &entryPtr->low) != TCL_OK) ||
            ((*proc)(interp, palPtr, objv+(i+3), &entryPtr->high) != TCL_OK)) {
            goto error;
        }
        entryPtr->min = j * step;
        entryPtr->max = (j+1) * step;
    }
    if (palPtr->colors != NULL) {
        Blt_Free(palPtr->colors);
    }
    palPtr->colors = entries;
    palPtr->numColors = numEntries;
    palPtr->min = 0.0;
    palPtr->max = 1.0;
    return TCL_OK;
 error:
    Blt_Free(entries);
    return TCL_ERROR;
}

/*
 *      "0.0 red 0.4 green 8.2 blue..."
 */
static int
ParseIrregularColorNames(Tcl_Interp *interp, Palette *palPtr, int objc, 
                         Tcl_Obj **objv)
{
    Blt_Pixel low;
    PaletteInterval *entries, *entryPtr;
    double min;
    int i, numEntries;
    double valueMin, valueMax;
    
    valueMin = DBL_MAX;
    valueMax = -DBL_MAX;
    numEntries = (objc / 2) - 1;
    entries = Blt_AssertMalloc(sizeof(PaletteInterval) * numEntries);
    if (GetStepFromObj(interp, objv[0], &min) != TCL_OK) {
        return TCL_ERROR;
    }
    if (Blt_GetPixelFromObj(interp, objv[1], &low) != TCL_OK) {
        return TCL_ERROR;
    }
    
    for (entryPtr = entries, i = 2; i < objc; i += 2, entryPtr++) {
        Blt_Pixel high;
        double max;
        
        if (GetStepFromObj(interp, objv[i], &max) != TCL_OK) {
            goto error;
        }
        if (Blt_GetPixelFromObj(interp, objv[i+1], &high) != TCL_OK) {
            goto error;
        }
        entryPtr->high.u32 = high.u32;
        entryPtr->low.u32 = low.u32;
        entryPtr->min = min;
        entryPtr->max = max;
        if (max > valueMax) {
            valueMax = max;
        }
        if (min < valueMin) {
            valueMin = min;
        }
        low.u32 = high.u32;
        min = max;
    }
    if (palPtr->colors != NULL) {
        Blt_Free(palPtr->colors);
    }
    palPtr->colors = entries;
    palPtr->numColors = numEntries;
    palPtr->min = valueMin;
    palPtr->max = valueMax;
    return TCL_OK;
 error:
    Blt_Free(entries);
    return TCL_ERROR;
}

/*
 *      "0.0 0.1 0.4 0.2 0.1 0.1 0.2 0.3"...
 */
static int
ParseIrregularColorTriplets(Tcl_Interp *interp, Palette *palPtr, int objc, 
                            Tcl_Obj **objv)
{
    PaletteInterval *entries, *entryPtr;
    int i, numEntries;
    Blt_Pixel low;
    double min;
    GetColorProc *proc;
    double valueMin, valueMax;

    valueMin = DBL_MAX;
    valueMax = -DBL_MAX;
    proc = (palPtr->colorFlags & COLOR_RGB) ? GetRGBFromObjv : GetHSVFromObjv;
    numEntries = (objc / 4) - 1;
    entries = Blt_AssertMalloc(sizeof(PaletteInterval) * numEntries);
    if (GetStepFromObj(interp, objv[0], &min) != TCL_OK) {
        return TCL_ERROR;
    }
    if ((*proc)(interp, palPtr, objv + 1, &low) != TCL_OK) {
        return TCL_ERROR;
    }
    for (entryPtr = entries, i = 4; i < objc; i += 4, entryPtr++) {
        Blt_Pixel high;
        double max;
        
        if (GetStepFromObj(interp, objv[i], &max) != TCL_OK) {
            goto error;
        }
        if ((*proc)(interp, palPtr, objv + i + 1, &high) != TCL_OK) {
            goto error;
        }
        entryPtr->high.u32 = high.u32;
        entryPtr->low.u32 = low.u32;
        entryPtr->min = min;
        entryPtr->max = max;
        if (max > valueMax) {
            valueMax = max;
        }
        if (min < valueMin) {
            valueMin = min;
        }
        low.u32 = high.u32;
        min = max;
    }
    if (palPtr->colors != NULL) {
        Blt_Free(palPtr->colors);
    }
    palPtr->colors = entries;
    palPtr->numColors = numEntries;
    palPtr->min = valueMin;
    palPtr->max = valueMax;
    return TCL_OK;
 error:
    Blt_Free(entries);
    return TCL_ERROR;
}

/*
 *      "0.0 red 0.4 green 0.4 blue 0.5 violet..."
 */
static int
ParseIntervalColorNames(Tcl_Interp *interp, Palette *palPtr, int objc, 
                        Tcl_Obj **objv)
{
    PaletteInterval *entries, *entryPtr;
    int i, numEntries;
    double valueMin, valueMax;
        
    valueMin = DBL_MAX;
    valueMax = -DBL_MAX;
    numEntries = (objc / 4) - 1;
    entries = Blt_AssertMalloc(sizeof(PaletteInterval) * numEntries);
    for (entryPtr = entries, i = 0; i < objc; i += 4, entryPtr++) {
        Blt_Pixel low, high;
        double min, max;
        
        if (GetStepFromObj(interp, objv[i], &min) != TCL_OK) {
            goto error;
        }
        if (Blt_GetPixelFromObj(interp, objv[i+1], &low) != TCL_OK) {
            goto error;
        }
        if (GetStepFromObj(interp, objv[i+2], &max) != TCL_OK) {
            goto error;
        }
        if (Blt_GetPixelFromObj(interp, objv[i+3], &high) != TCL_OK) {
            goto error;
        }
        entryPtr->high.u32 = high.u32;
        entryPtr->low.u32 = low.u32;
        entryPtr->min = min;
        entryPtr->max = max;
        if (max > valueMax) {
            valueMax = max;
        }
        if (min < valueMin) {
            valueMin = min;
        }
    }
    if (palPtr->colors != NULL) {
        Blt_Free(palPtr->colors);
    }
    palPtr->colors = entries;
    palPtr->numColors = numEntries;
    palPtr->min = valueMin;
    palPtr->max = valueMax;
    return TCL_OK;
 error:
    Blt_Free(entries);
    return TCL_ERROR;
}

/*
 *      "0.0 0.1 0.1 0.1 0.4 green 0.4 blue 0.5 violet..."
 */
static int
ParseIntervalColorTriplets(Tcl_Interp *interp, Palette *palPtr, int objc, 
                           Tcl_Obj **objv)
{
    PaletteInterval *entries, *entryPtr;
    int i, numEntries;
    GetColorProc *proc;
    double valueMin, valueMax;
        
    valueMin = DBL_MAX;
    valueMax = -DBL_MAX;
    proc = (palPtr->colorFlags & COLOR_RGB) ? GetRGBFromObjv : GetHSVFromObjv;
    numEntries = (objc / 8) - 1;
    entries = Blt_AssertMalloc(sizeof(PaletteInterval) * numEntries);
    for (entryPtr = entries, i = 0; i < objc; i += 8, entryPtr++) {
        Blt_Pixel low, high;
        double min, max;
        
        if (GetStepFromObj(interp, objv[i], &min) != TCL_OK) {
            goto error;
        }
        if ((*proc)(interp, palPtr, objv + i + 1,  &low) != TCL_OK) {
            goto error;
        }
        if (GetStepFromObj(interp, objv[i + 4], &max) != TCL_OK) {
            goto error;
        }
        if ((*proc)(interp, palPtr, objv + i + 5, &high) != TCL_OK) {
            goto error;
        }
        entryPtr->high.u32 = high.u32;
        entryPtr->low.u32 = low.u32;
        entryPtr->min = min;
        entryPtr->max = max;
        if (max > valueMax) {
            valueMax = max;
        }
        if (min < valueMin) {
            valueMin = min;
        }
    }
    if (palPtr->colors != NULL) {
        Blt_Free(palPtr->colors);
    }
    palPtr->colors = entries;
    palPtr->numColors = numEntries;
    palPtr->min = valueMin;
    palPtr->max = valueMax;
    return TCL_OK;
 error:
    Blt_Free(entries);
    return TCL_ERROR;
}

/*
 *      -colors "c1 c2 c3 c4.."
 */
static int
ParseColorData(Tcl_Interp *interp, Palette *palPtr, Tcl_Obj *objPtr)
{
    Tcl_Obj **objv;
    int objc, result, numComponents;

    if (Tcl_ListObjGetElements(interp, objPtr, &objc, &objv) != TCL_OK) {
        return TCL_ERROR;
    }
    if (objc == 0) {
        if (interp != NULL) {
            Tcl_AppendResult(interp, "no color component data", (char *)NULL);
        }
        return TCL_ERROR;
    }
    numComponents = 0;
    if ((palPtr->colorFlags & SPACING_REGULAR) == 0)  {
        numComponents++;                /* Add 1 component for value.  */
    }
    if (palPtr->colorFlags & (COLOR_RGB|COLOR_HSV)) {
        numComponents += 3;             /* Add 3 components for R,G, and B. */
    } else {
        numComponents++;                 /* Add 1 component for color name.  */
    }
    if (palPtr->colorFlags & SPACING_INTERVAL) {
        numComponents += numComponents; /* Double the number components.  */
    }
    if ((objc % numComponents) != 0) {
        if (interp != NULL) {
            char mesg[200];
            sprintf(mesg, "wrong # of color components (%d) (%s): should be %d "
                    "components per %s spaced %s entry.", objc,
                    Tcl_GetString(objPtr), numComponents,
                    NameOfSpacing(palPtr->colorFlags),
                    NameOfColorType(palPtr->colorFlags));
            Tcl_AppendResult(interp, mesg, (char *)NULL);
        }
        return TCL_ERROR;
    }
    if (palPtr->colorFlags & COLOR_RGB) {
        if (AutoTestRGBs(interp, palPtr, numComponents, objc, objv) != TCL_OK) {
            return TCL_ERROR;
        }
    }
    switch (numComponents) {
    case 1:                             /* Uniform color names. */
        result = ParseRegularColorNames(interp, palPtr, objc, objv);
        break;
    case 2:                             /* Nonuniform color names. */
        result = ParseIrregularColorNames(interp, palPtr, objc, objv);
        break;
    case 3:                             /* Uniform RGB|HSV values. */
        result = ParseRegularColorTriplets(interp, palPtr, objc, objv);
        break;
    case 4:                             /* Nonuniform RGB|HSV values. */
        if (palPtr->colorFlags & (COLOR_RGB|COLOR_HSV)) {
            result = ParseIrregularColorTriplets(interp, palPtr, objc, objv);
        } else {
            result = ParseIntervalColorNames(interp, palPtr, objc, objv);
        }
        break;
    case 8:                             /* Interval RGB|HSV colors. */
        result = ParseIntervalColorTriplets(interp, palPtr, objc, objv);
        break;
    default:
        if (interp != NULL) {
            Tcl_AppendResult(interp, "unknown number of color components \"",
                             Blt_Itoa(numComponents), "\"", (char *)NULL);
        }
        return TCL_ERROR;
    }
    if (result == TCL_OK) {
        SortEntries(palPtr->numColors, palPtr->colors);
    }
    return result;
}

/*
 *      "o1 o2 o3 o4.."
 */
static int
ParseRegularOpacity(Tcl_Interp *interp, Palette *palPtr, int objc, 
                    Tcl_Obj **objv)
{
    PaletteInterval *entries;
    double step;
    int i, numEntries;
    
    numEntries = objc - 1;
    entries = Blt_AssertMalloc(sizeof(PaletteInterval) * numEntries);
    step = 1.0 / numEntries;
    for (i = 0; i < numEntries; i++) {
        Blt_Pixel low, high;
        PaletteInterval *entryPtr;
        
        entryPtr = entries + i;
        if ((GetOpacity(interp, objv[i], &low) != TCL_OK) ||
            (GetOpacity(interp, objv[i+1], &high) != TCL_OK)) {
            goto error;
        }
        entryPtr->low.u32 = low.u32;
        entryPtr->high.u32 = high.u32;
        entryPtr->min = i * step;
        entryPtr->max = (i+1) * step;
        low.u32 = high.u32;
    }
    if (palPtr->opacities != NULL) {
        Blt_Free(palPtr->opacities);
    }
    palPtr->opacities = entries;
    palPtr->numOpacities = numEntries;
    palPtr->min = 0.0;
    palPtr->max = 1.0;
    return TCL_OK;
 error:
    Blt_Free(entries);
    return TCL_ERROR;
}

/*
 *      "x1 o1 x2 o2 x3 o3 x4 o4.."
 */
static int
ParseIrregularOpacity(Tcl_Interp *interp, Palette *palPtr, int objc, 
                      Tcl_Obj **objv)
{
    Blt_Pixel low;
    PaletteInterval *entries, *entryPtr;
    double min;
    int i, numEntries;
    double valueMin, valueMax;
        
    valueMin = DBL_MAX;
    valueMax = -DBL_MAX;
    numEntries = (objc / 2) - 1;
    entries = Blt_AssertMalloc(sizeof(PaletteInterval) * numEntries);

    if (GetStepFromObj(interp, objv[0], &min) != TCL_OK) {
        goto error;
    }
    if (GetOpacity(interp, objv[1], &low) != TCL_OK) {
        goto error;
    }
    for (entryPtr = entries, i = 2; i < objc; i += 2, entryPtr++) {
        Blt_Pixel high;
        double max;
        
        if (GetStepFromObj(interp, objv[i], &max) != TCL_OK) {
            goto error;
        }
        if (GetOpacity(interp, objv[i+1], &high) != TCL_OK) {
            goto error;
        }
        entryPtr->low.u32 = low.u32;
        entryPtr->high.u32 = high.u32;
        entryPtr->min = min;
        entryPtr->max = max;
        if (max > valueMax) {
            valueMax = max;
        }
        if (min < valueMin) {
            valueMin = min;
        }
        low.u32 = high.u32;
        min = max;
    }
    if (palPtr->opacities != NULL) {
        Blt_Free(palPtr->opacities);
    }
    palPtr->opacities = entries;
    palPtr->numOpacities = numEntries;
    palPtr->min = valueMin;
    palPtr->max = valueMax;
    return TCL_OK;
 error:
    Blt_Free(entries);
    return TCL_ERROR;
}


/*
 *      "0.0 0.1 0.4 0.4 0.4 0.4 0.5 0.6..."
 */
static int
ParseIntervalOpacity(Tcl_Interp *interp, Palette *palPtr, int objc, 
                     Tcl_Obj **objv)
{
    PaletteInterval *entries, *entryPtr;
    int i, numEntries;
    double valueMin, valueMax;
        
    valueMin = DBL_MAX;
    valueMax = -DBL_MAX;
    numEntries = (objc / 4) - 1;
    entries = Blt_AssertMalloc(sizeof(PaletteInterval) * numEntries);
    for (entryPtr = entries, i = 0; i < objc; i += 4, entryPtr++) {
        Blt_Pixel low, high;
        double min, max;
        
        if (GetStepFromObj(interp, objv[i], &min) != TCL_OK) {
            goto error;
        }
        if (GetOpacity(interp, objv[i+1], &low) != TCL_OK) {
            goto error;
        }
        if (GetStepFromObj(interp, objv[i+2], &max) != TCL_OK) {
            goto error;
        }
        if (GetOpacity(interp, objv[i+3], &high) != TCL_OK) {
            goto error;
        }
        entryPtr->high.u32 = high.u32;
        entryPtr->low.u32 = low.u32;
        entryPtr->min = min;
        entryPtr->max = max;
        if (max > valueMax) {
            valueMax = max;
        }
        if (min < valueMin) {
            valueMin = min;
        }
    }
    if (palPtr->opacities != NULL) {
        Blt_Free(palPtr->opacities);
    }
    palPtr->opacities = entries;
    palPtr->numOpacities = numEntries;
    palPtr->min = valueMin;
    palPtr->max = valueMax;
    return TCL_OK;
 error:
    Blt_Free(entries);
    return TCL_ERROR;
}

/*
 *      -colors "c1 c2 c3 c4.."
 */
static int
ParseOpacityData(Tcl_Interp *interp, Palette *palPtr, Tcl_Obj *objPtr)
{
    Tcl_Obj **objv;
    int objc, result, numComponents;
    
    if (Tcl_ListObjGetElements(interp, objPtr, &objc, &objv) != TCL_OK) {
        return TCL_ERROR;
    }
    if (objc == 0) {
        if (interp != NULL) {
            Tcl_AppendResult(interp, "no opacity component data", (char *)NULL);
        }
        return TCL_ERROR;
    }
    numComponents = 0;
    if ((palPtr->opacityFlags & SPACING_REGULAR) == 0)  {
        numComponents++;                /* Add 1 component for value.  */
    }
    numComponents++;                    /* Add 1 component for opacity. */
    if (palPtr->colorFlags & SPACING_INTERVAL) {
        numComponents += numComponents; /* Double the number components.  */
    }
    if ((objc % numComponents) != 0) {
        if (interp != NULL) {
            Tcl_AppendResult(interp,
                             "wrong # of opacity components: should be ",
                             Blt_Itoa(numComponents), " components per entry.",
                             (char *)NULL);
        }
        return TCL_ERROR;
    }
    switch (numComponents) {
    case 1:                             /* Uniform opacities. */
        result = ParseRegularOpacity(interp, palPtr, objc, objv);
        break;
    case 2:                             /* Nonuniform opacities. */
        result = ParseIrregularOpacity(interp, palPtr, objc, objv);
        break;
    case 4:                             /* Nonuniform intervals. */
        result = ParseIntervalOpacity(interp, palPtr, objc, objv);
        break;
    default:
        if (interp != NULL) {
            Tcl_AppendResult(interp, "unknown number of opacity components \"",
                         Blt_Itoa(numComponents), "\"", (char *)NULL);
        }
        return TCL_ERROR;
    }
    if (result == TCL_OK) {
        SortEntries(palPtr->numColors, palPtr->colors);
    }
    return result;
}

static int
LoadData(Tcl_Interp *interp, Palette *palPtr)
{
    int result;
    
    palPtr->flags |= LOADED;
    result = TCL_ERROR;
    if (palPtr->colorFileObjPtr != NULL) {
        const char *fileName;
        Blt_DBuffer dbuffer;
        Tcl_Obj *objPtr;
        
        dbuffer = Blt_DBuffer_Create();
        fileName = Tcl_GetString(palPtr->colorFileObjPtr);
        if (Blt_DBuffer_LoadFile(interp, fileName, dbuffer) != TCL_OK) {
            return TCL_ERROR;
        }
        objPtr = Blt_DBuffer_StringObj(dbuffer);
        Tcl_IncrRefCount(objPtr);
        result = ParseColorData(interp, palPtr, objPtr);
        Tcl_DecrRefCount(objPtr);
        Blt_DBuffer_Destroy(dbuffer);
    } else if (palPtr->colorDataObjPtr != NULL) {
        result = ParseColorData(interp, palPtr, palPtr->colorDataObjPtr);
    }
    if (result != TCL_OK) {
        return TCL_ERROR;
    }
    if (palPtr->opacityFileObjPtr != NULL) {
        const char *fileName;
        Blt_DBuffer dbuffer;
        Tcl_Obj *objPtr;
        
        dbuffer = Blt_DBuffer_Create();
        fileName = Tcl_GetString(palPtr->opacityFileObjPtr);
        if (Blt_DBuffer_LoadFile(interp, fileName, dbuffer) != TCL_OK) {
            return TCL_ERROR;
        }
        objPtr = Blt_DBuffer_StringObj(dbuffer);
        Tcl_IncrRefCount(objPtr);
        result = ParseOpacityData(interp, palPtr, objPtr);
        Tcl_DecrRefCount(objPtr);
        Blt_DBuffer_Destroy(dbuffer);
    } else if (palPtr->opacityDataObjPtr != NULL) {
        result = ParseOpacityData(interp, palPtr, palPtr->opacityDataObjPtr);
    } else {
        return TCL_OK;
    }
    NotifyClients(palPtr, PALETTE_CHANGE_NOTIFY);
    return result;
}

static unsigned int
ColorLerp(PaletteInterval *entryPtr, double t)
{
    Blt_Pixel color;
    int alpha, beta, t1, t2;
    int r, g, b, a;

    a = (int)(t * 255.0);
    alpha = CLAMP(a);
    if (alpha == 0xFF) {
        return entryPtr->high.u32;
    } else if (alpha == 0x00) {
        return entryPtr->low.u32;
    }
    beta = alpha ^ 0xFF;                /* beta = 1 - alpha */
    r = imul8x8(beta, entryPtr->low.Red, t1) + 
        imul8x8(alpha, entryPtr->high.Red, t2);
    g = imul8x8(beta, entryPtr->low.Green, t1) + 
        imul8x8(alpha, entryPtr->high.Green, t2);
    b = imul8x8(beta, entryPtr->low.Blue, t1) + 
        imul8x8(alpha, entryPtr->high.Blue, t2);
    a = imul8x8(beta, entryPtr->low.Alpha, t1) + 
        imul8x8(alpha, entryPtr->high.Alpha, t2);

    color.Red   = CLAMP(r);
    color.Green = CLAMP(g);
    color.Blue  = CLAMP(b);
    color.Alpha = CLAMP(a);
    color.Alpha = 0xFF;
    return color.u32;
}

static unsigned int
OpacityLerp(PaletteInterval *entryPtr, double t)
{
    int alpha, beta, t1, t2;
    int a;

    a = (int)(t * 255.0);
    alpha = CLAMP(a);
    if (alpha == 0xFF) {
        return entryPtr->high.Alpha;
    } else if (alpha == 0x00) {
        return entryPtr->low.Alpha;
    }
    beta = alpha ^ 0xFF;                /* beta = 1 - alpha */
    a = imul8x8(beta, entryPtr->low.Alpha, t1) + 
        imul8x8(alpha, entryPtr->high.Alpha, t2);
    return CLAMP(a);
}

static PaletteInterval * 
SearchForEntry(size_t length, PaletteInterval *entries, double value)
{
    int low, high;

    low = 0;
    high = length - 1;
    while (low <= high) {
        PaletteInterval *entryPtr;
        int median;
        
        median = (low + high) >> 1;
        entryPtr = entries + median;
        if (InRange(value, entryPtr->min, entryPtr->max)) {
            return entryPtr;
        }
        if (value < entryPtr->min) {
            high = median - 1;
        } else if (value > entryPtr->max) {
            low = median + 1;
        } else {
            return NULL;
        }
    }
    return NULL;                        /* Can't find value. */
}


static int 
InterpolateColor(Palette *palPtr, double value, Blt_Pixel *colorPtr)
{
    PaletteInterval *entryPtr;
    double t;

    colorPtr->u32 = 0x00;               /* Default to empty. */
    if (palPtr->numColors == 0) {
        return FALSE;
    }
    if (palPtr->colorFlags & SPACING_REGULAR) {
        int i;

        i = (int)(value * (palPtr->numColors));
        if (i >= palPtr->numColors) {
            i = palPtr->numColors - 1;
        } else if (i < 0) {
            i = 0;
        }
        entryPtr = palPtr->colors + i;
    } else {
        entryPtr = SearchForEntry(palPtr->numColors, palPtr->colors, value);
    }
    if (entryPtr == NULL) {
        return FALSE;
    }
    t = (value - entryPtr->min) / (entryPtr->max - entryPtr->min);
    colorPtr->u32 = ColorLerp(entryPtr, t);
    return TRUE;
}


static int 
InterpolateOpacity(Palette *palPtr, double value, unsigned int *alphaPtr)
{
    PaletteInterval *entryPtr;
    double t;
    
    if (palPtr->opacityFlags & SPACING_REGULAR) {
        int i;

        i = (int)(value * (palPtr->numOpacities));
        if (i >= palPtr->numOpacities) {
            i = palPtr->numOpacities - 1;
        } else if (i < 0) {
            i = 0;
        }
        entryPtr = palPtr->opacities + i;
    } else {
        entryPtr = SearchForEntry(palPtr->numOpacities, palPtr->opacities,
                                  value);
    }
    if (entryPtr == NULL) {
        return FALSE;
    }
    t = (value - entryPtr->min) / (entryPtr->max - entryPtr->min);
    *alphaPtr = OpacityLerp(entryPtr, t);
    return TRUE;
}

static int 
InterpolateColorAndOpacity(Palette *palPtr, double value, Blt_Pixel *colorPtr)
{
    Blt_Pixel color;

    if (InterpolateColor(palPtr, value, &color))  {
        if (palPtr->numOpacities > 0) {
            unsigned int alpha;

            if (InterpolateOpacity(palPtr, value, &alpha)) {
                color.Alpha = alpha;
            }
        }
        colorPtr->u32 = color.u32;
        return TRUE;
    } 
    colorPtr->u32 = 0x0;
    return FALSE;
}

/*
 *---------------------------------------------------------------------------
 *
 * DefaultPalettes --
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
DefaultPalettes(Tcl_Interp *interp, PaletteCmdInterpData *dataPtr)
{
#ifndef notdef
    static char cmd[] = "source [file join $blt_library bltPalette.tcl]";
#else
    static char cmd[] =
        "if { [catch {source [file join $blt_library bltPalette.tcl]}] != 0} { global errorInfo; puts stderr errorInfo=$errorInfo }";
#endif
    if (Tcl_GlobalEval(interp, cmd) != TCL_OK) {
        char info[2000];

        Blt_FormatString(info, 2000, "\n    (while loading palettes)");
        fprintf(stderr, "error load palettes: %s: %s\n", info,
                Tcl_GetString(Tcl_GetObjResult(interp)));
        Tcl_AddErrorInfo(interp, info);
        Tcl_BackgroundError(interp);
        return TCL_ERROR;
    }
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * ColorsOp --
 *
 *      Dumps the entries for the color look-up table.
 *
 * Results:
 *      Returns a list of the opacity entries.
 *
 *      blt::palette opacities paletteName
 *
 *---------------------------------------------------------------------------
 */
static int
ColorsOp(ClientData clientData, Tcl_Interp *interp, int objc, 
         Tcl_Obj *const *objv)
{
    PaletteCmdInterpData *dataPtr = clientData;
    Palette *palPtr;
    Tcl_Obj *listObjPtr;
    int i;
    
    if (GetPaletteFromObj(interp, dataPtr, objv[2], &palPtr) != TCL_OK) {
        return TCL_ERROR;
    }
    if ((palPtr->flags & LOADED) == 0) {
        if (LoadData(interp, palPtr) != TCL_OK) {
            return TCL_ERROR;
        }
    }
    listObjPtr = Tcl_NewListObj(0, (Tcl_Obj **)NULL);
    for (i = 0; i < palPtr->numColors; i++) {
        PaletteInterval *entryPtr;
        Tcl_Obj *objPtr;
        char string[200];
        
        entryPtr = palPtr->colors + i;
        objPtr = Tcl_NewDoubleObj(entryPtr->min);
        Tcl_ListObjAppendElement(interp, listObjPtr, objPtr);
        objPtr = Tcl_NewDoubleObj(entryPtr->max);
        Tcl_ListObjAppendElement(interp, listObjPtr, objPtr);

        sprintf(string, "#%02x%02x%02x", entryPtr->low.Red,
                entryPtr->low.Green, entryPtr->low.Blue);
        objPtr = Tcl_NewStringObj(string, -1);
        Tcl_ListObjAppendElement(interp, listObjPtr, objPtr);

        sprintf(string, "#%02x%02x%02x", entryPtr->high.Red,
                entryPtr->high.Green, entryPtr->high.Blue);
        objPtr = Tcl_NewStringObj(string, -1);
        Tcl_ListObjAppendElement(interp, listObjPtr, objPtr);
    }
    Tcl_SetObjResult(interp, listObjPtr);
    return TCL_OK;
}    


/*
 *---------------------------------------------------------------------------
 *
 * CreateOp --
 *
 *      Creates a palette.  
 *
 *      We don't actually load the palette data here. We just check that we
 *      can read the files (if they are specified) but don't actually read
 *      the data until we need it. This is so that we can create lots of
 *      palettes that can be listed with the "palette names" command. 
 *
 * Results:
 *      The return value is a standard TCL result. The interpreter result
 *      will contain a TCL list of the element names.
 *
 *      blt::palette create paletteName ?option value ...?
 *
 *---------------------------------------------------------------------------
 */
static int
CreateOp(ClientData clientData, Tcl_Interp *interp, int objc, 
         Tcl_Obj *const *objv)
{
    PaletteCmdInterpData *dataPtr = clientData;
    Palette *palPtr;
    const char *name;
    char ident[200];

    name = NULL;
    if (objc > 2) {
        const char *string;

        string = Tcl_GetString(objv[2]);
        if (string[0] != '-') {
            if (Blt_FindHashEntry(&dataPtr->paletteTable, string) != NULL) {
                Tcl_AppendResult(interp, "palette \"", string,
                        "\" already exists", (char *)NULL);
                return TCL_ERROR;
            }
            name = string;
            objc--, objv++;
        }
    }
    /* If no name was given for the palette, generate one. */
    if (name == NULL) {
        do {
            Blt_FormatString(ident, 200, "palette%d", dataPtr->nextId++);
        } while (Blt_FindHashEntry(&dataPtr->paletteTable, ident));
        name = ident;
    }
    palPtr = NewPalette(interp, dataPtr, name);
    if (palPtr == NULL) {
        return TCL_ERROR;
    }
    if (Blt_ParseSwitches(interp, paletteSpecs, objc - 2, objv + 2,
                (char *)palPtr, BLT_SWITCH_DEFAULTS) < 0) {
        goto error;
    }
    if ((palPtr->colorFileObjPtr != NULL) && (palPtr->colorDataObjPtr != NULL)){
        Tcl_AppendResult(interp,
                "can't set both -colorfile and -colordata flags", (char *)NULL);
        goto error;
    }
    if ((palPtr->colorFileObjPtr == NULL) && (palPtr->colorDataObjPtr == NULL)){
        Tcl_AppendResult(interp,
                "one of -colorfile and -colordata switches are required",
                (char *)NULL);
        goto error;
    }
    if (palPtr->colorFileObjPtr != NULL) {
        const char *fileName;

        fileName = Tcl_GetString(palPtr->colorFileObjPtr);
        if (Tcl_Access(fileName, R_OK) != 0) {
            Tcl_AppendResult(interp, "can't access \"", fileName, "\":",
                Tcl_PosixError(interp), (char *)NULL);
            goto error;
        }
    }
    if ((palPtr->opacityFileObjPtr != NULL) &&
        (palPtr->opacityDataObjPtr != NULL)) {
        Tcl_AppendResult(interp,
                "can't set both -opacityfile and -opacitydata flags",
                (char *)NULL);
        goto error;
    }
    if (palPtr->opacityFileObjPtr != NULL) {
        const char *fileName;

        fileName = Tcl_GetString(palPtr->opacityFileObjPtr);
        if (Tcl_Access(fileName, R_OK) != 0) {
            Tcl_AppendResult(interp, "can't access \"", fileName, "\":",
                Tcl_PosixError(interp), (char *)NULL);
            goto error;
        }
    }
    NotifyClients(palPtr, PALETTE_CHANGE_NOTIFY);
    Tcl_SetStringObj(Tcl_GetObjResult(interp), palPtr->name, -1);
    return TCL_OK;
 error:
    DestroyPalette(palPtr);
    return TCL_ERROR;
}

/*
 *---------------------------------------------------------------------------
 *
 * DeleteOp --
 *
 *      Deletes one or more palettees from the graph.
 *
 * Results:
 *      The return value is a standard TCL result. The interpreter result will
 *      contain a TCL list of the element names.
 *
 *      blt::palette delete $name...
 *
 *---------------------------------------------------------------------------
 */
static int
DeleteOp(ClientData clientData, Tcl_Interp *interp, int objc, 
         Tcl_Obj *const *objv)
{
    PaletteCmdInterpData *dataPtr = clientData;
    int i;

    for (i = 2; i < objc; i++) {
        Palette *palPtr;

        if (GetPaletteFromObj(interp, dataPtr, objv[i], &palPtr) != TCL_OK) {
            return TCL_ERROR;
        }
        DestroyPalette(palPtr);
    }
    return TCL_OK;
}    


/*
 *---------------------------------------------------------------------------
 *
 * DrawOp --
 *
 *      Draws the palette onto a picture.
 *
 * Results:
 *      The return value is a standard TCL result. The interpreter
 *      result will contain a TCL list of the element names.
 *
 *      blt::palette draw $name $picture
 *
 *---------------------------------------------------------------------------
 */
static int
DrawOp(ClientData clientData, Tcl_Interp *interp, int objc, 
       Tcl_Obj *const *objv)
{
    PaletteCmdInterpData *dataPtr = clientData;
    Palette *palPtr;
    Blt_Picture picture;
    int w, h;

    if (GetPaletteFromObj(interp, dataPtr, objv[2], &palPtr) != TCL_OK) {
        return TCL_ERROR;               /* Can't find named palette. */
    }
    if (Blt_GetPictureFromObj(interp, objv[3], &picture) != TCL_OK) {
        return TCL_ERROR;
    }
    if ((palPtr->flags & LOADED) == 0) {
        if (LoadData(interp, palPtr) != TCL_OK) {
            return TCL_ERROR;
        }
    }
    w = Blt_Picture_Width(picture);
    h = Blt_Picture_Height(picture);
    if (w > h) {
        int x;
        double range;
        
        range = palPtr->max - palPtr->min;
        for (x = 0; x < w; x++) {
            double value;
            int y;
            Blt_Pixel color;

            value = ((double)x / (double)(w-1)) * range + palPtr->min;
            InterpolateColorAndOpacity(palPtr, value, &color);
            /* Draw band. */
            for (y = 0; y < h; y++) {
                Blt_Pixel *dp;

                dp = Blt_Picture_Pixel(picture, x, y);
                dp->u32 = color.u32;
            }
        }
    } else {
        int y;
        double range;
        
        range = palPtr->max - palPtr->min;
        for (y = 0; y < h; y++) {
            int x;
            double value;
            Blt_Pixel color;

            value = ((double)y / (double)(h-1)) * range + palPtr->min;
            InterpolateColorAndOpacity(palPtr, value, &color);
            /* Draw band. */
            for (x = 0; x < w; x++) {
                Blt_Pixel *dp;

                dp = Blt_Picture_Pixel(picture, x, y);
                dp->u32 = color.u32;
            }
        }
    }
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * ExistsOp --
 *
 *      Indicates if a palette by the given name exists in the element.
 *
 * Results:
 *      The return value is a standard TCL result. The interpreter result will
 *      contain a TCL list of the element names.
 *
 *      blt::palette exists $name
 *
 *---------------------------------------------------------------------------
 */
static int
ExistsOp(ClientData clientData, Tcl_Interp *interp, int objc, 
         Tcl_Obj *const *objv)
{
    PaletteCmdInterpData *dataPtr = clientData;
    Palette *palPtr;
    int bool;

    bool = (GetPaletteFromObj(NULL, dataPtr, objv[2], &palPtr) == TCL_OK);
    Tcl_SetBooleanObj(Tcl_GetObjResult(interp), bool);
    return TCL_OK;
}    

/*
 *---------------------------------------------------------------------------
 *
 * InterpolateOp --
 *
 *      Computes the interpolated color value from the value given.
 *
 * Results:
 *      The return value is a standard TCL result. The interpreter result
 *      will contain a TCL list of the element names.
 *
 *      blt::palette interpolate paletteName value
 *
 *---------------------------------------------------------------------------
 */
static int
InterpolateOp(ClientData clientData, Tcl_Interp *interp, int objc, 
              Tcl_Obj *const *objv)
{
    Blt_Pixel color;
    Palette *palPtr;
    PaletteCmdInterpData *dataPtr = clientData;
    Tcl_Obj *listObjPtr;
    double value;

    if (GetPaletteFromObj(interp, dataPtr, objv[2], &palPtr) != TCL_OK) {
        return TCL_ERROR;
    }
    if (GetStepFromObj(interp, objv[3], &value) != TCL_OK) {
        return TCL_ERROR;
    }
    if ((palPtr->flags & LOADED) == 0) {
        if (LoadData(interp, palPtr) != TCL_OK) {
            return TCL_ERROR;
        }
    }
    if (!InterpolateColorAndOpacity(palPtr, value, &color)) {
        Tcl_AppendResult(interp, "value \"", Tcl_GetString(objv[3]), 
                "\" not in any range", (char *)NULL);
        return TCL_ERROR;
    }
    if (palPtr->colorFlags & COLOR_NAME) {
        Tcl_Obj *objPtr;
        char string[200];

        if (palPtr->numOpacities == 0) {
            sprintf(string, "#%02x%02x%02x", color.Red, color.Green,
                    color.Blue);
        } else {
            sprintf(string, "0x%02x%02x%02x%02x", color.Alpha, color.Red,
                    color.Green, color.Blue);
        }
        objPtr = Tcl_NewStringObj(string, -1);
        Tcl_SetObjResult(interp, objPtr);
        return TCL_OK;
    }
    listObjPtr = Tcl_NewListObj(0, (Tcl_Obj **)NULL);
    if (palPtr->numOpacities > 0) {
        Tcl_Obj *objPtr;

        objPtr = Tcl_NewIntObj(color.Alpha);
        Tcl_ListObjAppendElement(interp, listObjPtr, objPtr);
    }
    {
        Tcl_Obj *objPtr;

        objPtr = Tcl_NewIntObj(color.Red);
        Tcl_ListObjAppendElement(interp, listObjPtr, objPtr);
        objPtr = Tcl_NewIntObj(color.Green);
        Tcl_ListObjAppendElement(interp, listObjPtr, objPtr);
        objPtr = Tcl_NewIntObj(color.Blue);
        Tcl_ListObjAppendElement(interp, listObjPtr, objPtr);
    }
    Tcl_SetObjResult(interp, listObjPtr);
    return TCL_OK;
}    

/*
 *---------------------------------------------------------------------------
 *
 * NamesOp --
 *
 *      Returns the names of the palette in the graph matching one of more
 *      patterns provided.  If no pattern arguments are given, then all
 *      palette names will be returned.
 *
 * Results:
 *      The return value is a standard TCL result. The interpreter result will
 *      contain a TCL list of the element names.
 *
 *      blt::palette names $pattern
 *
 *---------------------------------------------------------------------------
 */
static int
NamesOp(ClientData clientData, Tcl_Interp *interp, int objc, 
        Tcl_Obj *const *objv)
{
    PaletteCmdInterpData *dataPtr = clientData;
    Tcl_Obj *listObjPtr;

    listObjPtr = Tcl_NewListObj(0, (Tcl_Obj **)NULL);
    if (objc == 2) {
        Blt_HashEntry *hPtr;
        Blt_HashSearch iter;

        for (hPtr = Blt_FirstHashEntry(&dataPtr->paletteTable, &iter);
             hPtr != NULL; hPtr = Blt_NextHashEntry(&iter)) {
            Palette *palPtr;
            Tcl_Obj *objPtr;

            palPtr = Blt_GetHashValue(hPtr);
            objPtr = Tcl_NewStringObj(palPtr->name, -1);
            Tcl_ListObjAppendElement(interp, listObjPtr, objPtr);
        }
    } else {
        Blt_HashEntry *hPtr;
        Blt_HashSearch iter;

        for (hPtr = Blt_FirstHashEntry(&dataPtr->paletteTable, &iter);
             hPtr != NULL; hPtr = Blt_NextHashEntry(&iter)) {
            Palette *palPtr;
            int i;

            palPtr = Blt_GetHashValue(hPtr);
            for (i = 2; i < objc; i++) {
                const char *pattern;

                pattern = Tcl_GetString(objv[i]);
                if (Tcl_StringMatch(palPtr->name, pattern)) {
                    Tcl_Obj *objPtr;

                    objPtr = Tcl_NewStringObj(palPtr->name, -1);
                    Tcl_ListObjAppendElement(interp, listObjPtr, objPtr);
                    break;
                }
            }
        }
    }
    Tcl_SetObjResult(interp, listObjPtr);
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * OpacitiesOp --
 *
 *      Dumps the entries for the opacity look-up table.
 *
 * Results:
 *      Returns a list of the opacity entries.
 *
 *      blt::palette opacities paletteName
 *
 *---------------------------------------------------------------------------
 */
static int
OpacitiesOp(ClientData clientData, Tcl_Interp *interp, int objc, 
            Tcl_Obj *const *objv)
{
    PaletteCmdInterpData *dataPtr = clientData;
    Palette *palPtr;
    Tcl_Obj *listObjPtr;
    int i;
    
    if (GetPaletteFromObj(interp, dataPtr, objv[2], &palPtr) != TCL_OK) {
        return TCL_ERROR;
    }
    if ((palPtr->flags & LOADED) == 0) {
        if (LoadData(interp, palPtr) != TCL_OK) {
            return TCL_ERROR;
        }
    }
    listObjPtr = Tcl_NewListObj(0, (Tcl_Obj **)NULL);
    for (i = 0; i < palPtr->numOpacities; i++) {
        PaletteInterval *entryPtr;
        Tcl_Obj *objPtr;
        
        entryPtr = palPtr->opacities + i;
        objPtr = Tcl_NewDoubleObj(entryPtr->min);
        Tcl_ListObjAppendElement(interp, listObjPtr, objPtr);
        objPtr = Tcl_NewDoubleObj(entryPtr->max);
        Tcl_ListObjAppendElement(interp, listObjPtr, objPtr);
        objPtr = Tcl_NewDoubleObj(entryPtr->low.Alpha/ 255.0);
        Tcl_ListObjAppendElement(interp, listObjPtr, objPtr);
        objPtr = Tcl_NewDoubleObj(entryPtr->high.Alpha / 255.0);
        Tcl_ListObjAppendElement(interp, listObjPtr, objPtr);
    }
    Tcl_SetObjResult(interp, listObjPtr);
    return TCL_OK;
}    

/*
 *---------------------------------------------------------------------------
 *
 * PaletteCmd --
 *
 *      blt::palette create ?paletteName? -colors {} -opacity {}
 *      blt::palette delete paletteName
 *      blt::palette exists paletteName
 *      blt::palette names ?pattern?
 *      blt::palette interpolate paletteName value
 *      blt::palette draw paletteName pictureName
 *      blt::palette ranges paletteName
 *
 *---------------------------------------------------------------------------
 */
static Blt_OpSpec paletteOps[] = {
    {"colors",      1, ColorsOp,      3, 3, "paletteName",},
    {"create",      1, CreateOp,     2, 0, "?paletteName? ?option value ...?",},
    {"delete",      2, DeleteOp,      2, 0, "?paletteName?...",},
    {"draw",        2, DrawOp,        4, 4, "paletteName picture",},
    {"exists",      1, ExistsOp,      3, 3, "paletteName",},
    {"interpolate", 1, InterpolateOp, 4, 4, "paletteName value",},
    {"names",       1, NamesOp,       2, 0, "?pattern ...?",},
    {"opacities",   1, OpacitiesOp,   3, 3, "paletteName",},
};
static int numPaletteOps = sizeof(paletteOps) / sizeof(Blt_OpSpec);

static int
PaletteObjCmd(ClientData clientData, Tcl_Interp *interp, int objc, 
              Tcl_Obj *const *objv)
{
    Tcl_ObjCmdProc *proc;

    proc = Blt_GetOpFromObj(interp, numPaletteOps, paletteOps, BLT_OP_ARG1, 
        objc, objv, 0);
    if (proc == NULL) {
        return TCL_ERROR;
    }
    return (*proc) (clientData, interp, objc, objv);
}

/*
 *---------------------------------------------------------------------------
 *
 * PaletteInterpDeleteProc --
 *
 *      This is called when the interpreter registering the "palette"
 *      command is deleted.
 *
 * Results:
 *      None.
 *
 * Side Effects:
 *      Removes the hash table managing all table names.
 *
 *---------------------------------------------------------------------------
 */
/* ARGSUSED */
static void
PaletteInterpDeleteProc(ClientData clientData, Tcl_Interp *interp)
{
    PaletteCmdInterpData *dataPtr = clientData;

    /* All table instances should already have been destroyed when their
     * respective TCL commands were deleted. */
    DestroyPalettes(dataPtr);
    Blt_DeleteHashTable(&dataPtr->paletteTable);
    Tcl_DeleteAssocData(interp, PALETTE_THREAD_KEY);
    Blt_Free(dataPtr);
}

/*
 *
 * GetPaletteCmdInterpData --
 *
 */
static PaletteCmdInterpData *
GetPaletteCmdInterpData(Tcl_Interp *interp)
{
    PaletteCmdInterpData *dataPtr;
    Tcl_InterpDeleteProc *proc;

    dataPtr = (PaletteCmdInterpData *)
        Tcl_GetAssocData(interp, PALETTE_THREAD_KEY, &proc);
    if (dataPtr == NULL) {
        dataPtr = Blt_AssertMalloc(sizeof(PaletteCmdInterpData));
        dataPtr->interp = interp;
        Tcl_SetAssocData(interp, PALETTE_THREAD_KEY, PaletteInterpDeleteProc, 
                dataPtr);
        Blt_InitHashTable(&dataPtr->paletteTable, BLT_STRING_KEYS);
        dataPtr->nextId = 0;
    }
    return dataPtr;
}

/*
 *---------------------------------------------------------------------------
 *
 * Blt_PaletteCmdInitProc --
 *
 *      This procedure is invoked to initialize the "palette" command.
 *
 * Results:
 *      None.
 *
 * Side Effects:
 *      Creates the new command and adds a new entry into a global Tcl
 *      associative array.
 *
 *---------------------------------------------------------------------------
 */
int
Blt_PaletteCmdInitProc(Tcl_Interp *interp)
{
    static Blt_CmdSpec cmdSpec = { "palette", PaletteObjCmd, };

    cmdSpec.clientData = GetPaletteCmdInterpData(interp);
    if (Blt_InitCmd(interp, "::blt", &cmdSpec) != TCL_OK) {
        return TCL_ERROR;
    }
    return DefaultPalettes(interp, cmdSpec.clientData);
}


int
Blt_Palette_GetFromObj(Tcl_Interp *interp, Tcl_Obj *objPtr, 
                       Palette **palPtrPtr)
{
    PaletteCmdInterpData *dataPtr;
    Palette *palPtr;

    dataPtr = GetPaletteCmdInterpData(interp);
    if (GetPaletteFromObj(interp, dataPtr, objPtr, &palPtr) != TCL_OK) {
        return TCL_ERROR;
    }
    *palPtrPtr = palPtr;
    palPtr->refCount++;
    if ((palPtr->flags & LOADED) == 0) {
        if (LoadData(interp, palPtr) != TCL_OK) {
            return TCL_ERROR;
        }
    }
    return TCL_OK;
}

int
Blt_Palette_GetFromString(Tcl_Interp *interp, const char *string, 
                          Palette **palPtrPtr)
{
    PaletteCmdInterpData *dataPtr;
    Palette *palPtr;

    dataPtr = GetPaletteCmdInterpData(interp);
    if (GetPalette(interp, dataPtr, string, &palPtr) != TCL_OK) {
        return TCL_ERROR;
    }
    *palPtrPtr = (Blt_Palette)palPtr;
    palPtr->refCount++;
    return TCL_OK;
}

int
Blt_Palette_GetRGBColor(Blt_Palette palette, double value)
{
    Palette *palPtr = (Palette *)palette;
    Blt_Pixel color;

    if ((palPtr->flags & LOADED) == 0) {
        if (LoadData(NULL, palPtr) != TCL_OK) {
            return 0x0;
        }
    }
    if (!InterpolateColor(palPtr, value, &color)) {
        color.u32 = 0x00;
    } 
    return color.u32;
}

int
Blt_Palette_GetAssociatedColor(Blt_Palette palette, double value)
{
    Palette *palPtr = (Palette *)palette;
    Blt_Pixel color;

    if ((palPtr->flags & LOADED) == 0) {
        if (LoadData(NULL, palPtr) != TCL_OK) {
            return 0x0;
        }
    }
    if (!InterpolateColorAndOpacity(palPtr, value, &color)) {
        return 0x00;
    } 
    Blt_PremultiplyColor(&color);
    return color.u32;
}

void
Blt_Palette_CreateNotifier(Blt_Palette palette,
                           Blt_Palette_NotifyProc *notifyProc,
                           ClientData clientData)
{
    Palette *palPtr = (Palette *)palette;
    PaletteNotifier *notifyPtr;
    Blt_ChainLink link;
    
    if (palPtr->notifiers == NULL) {
        palPtr->notifiers = Blt_Chain_Create();
    }
     for (link = Blt_Chain_FirstLink(palPtr->notifiers); link != NULL;
        link = Blt_Chain_NextLink(link)) {
         PaletteNotifier *notifyPtr;

         notifyPtr = Blt_Chain_GetValue(link);
         if ((notifyPtr->proc == notifyProc) &&
             (notifyPtr->clientData == clientData)) {
             notifyPtr->clientData = clientData;
             return;                    /* Notifier already exists. */
         }
     }
    link = Blt_Chain_AllocLink(sizeof(PaletteNotifier));
    notifyPtr = Blt_Chain_GetValue(link);
    notifyPtr->proc = notifyProc;
    notifyPtr->clientData = clientData;
    Blt_Chain_LinkAfter(palPtr->notifiers, link, NULL);
}

void
Blt_Palette_DeleteNotifier(Blt_Palette palette,
                           Blt_Palette_NotifyProc *notifyProc,
                           ClientData clientData)
{
    Palette *palPtr = (Palette *)palette;
    Blt_ChainLink link;
    
     for (link = Blt_Chain_FirstLink(palPtr->notifiers); link != NULL;
        link = Blt_Chain_NextLink(link)) {
         PaletteNotifier *notifyPtr;

         notifyPtr = Blt_Chain_GetValue(link);
         if ((notifyPtr->proc == notifyProc) &&
             (notifyPtr->clientData == clientData)) {
             Blt_Chain_DeleteLink(palPtr->notifiers, link);
             return;
         }
     }
}

const char *
Blt_Palette_Name(Blt_Palette palette)
{
    Palette *palPtr = (Palette *)palette;

    return palPtr->name;
}

Blt_Palette
Blt_Palette_TwoColorPalette(int low, int high)
{
    Palette *palPtr;

    palPtr = Blt_AssertCalloc(1, sizeof(Palette));
    palPtr->colors = Blt_AssertMalloc(sizeof(PaletteInterval));
    palPtr->colors[0].low.u32 = low;
    palPtr->colors[0].high.u32 = high;
    palPtr->colors[0].min = 0.0;
    palPtr->colors[0].max = 1.0;
    palPtr->numColors = 1;
    return palPtr;
}

void
Blt_Palette_Free(Blt_Palette palette)
{
    Palette *palPtr = (Palette *)palette;

    if (palPtr->hashPtr != NULL) {
        Blt_DeleteHashEntry(&palPtr->dataPtr->paletteTable, palPtr->hashPtr);
        palPtr->hashPtr = NULL;
    }
    palPtr->refCount--;
    if (palPtr->refCount <= 0) {
        DestroyPalette(palPtr);
    }
}

