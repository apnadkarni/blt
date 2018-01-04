/* -*- mode: c; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/* 
 * bltConfig.h --
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

#ifndef BLT_CONFIG_H
#define BLT_CONFIG_H

#ifndef offsetof
#define offsetof(TYPE, MEMBER) ((size_t) &((TYPE *)0)->MEMBER)
#endif

#ifndef Blt_Offset
#define Blt_Offset(type, field) ((int)offsetof(type, field))
#endif /* Blt_Offset */

typedef int (Blt_OptionParseProc)(ClientData clientData, Tcl_Interp *interp, 
        Tk_Window tkwin, Tcl_Obj *objPtr, char *widgRec, int offset, int flags);
typedef Tcl_Obj *(Blt_OptionPrintProc)(ClientData clientData, 
        Tcl_Interp *interp, Tk_Window tkwin, char *widgRec, int offset, 
        int flags);
typedef void (Blt_OptionFreeProc)(ClientData clientData, Display *display, 
        char *widgRec, int offset);

typedef struct _Blt_CustomOption {
    Blt_OptionParseProc *parseProc;     /* Procedure to call to parse an
                                         * option and store it in converted
                                         * form. */

    Blt_OptionPrintProc *printProc;     /* Procedure to return a Tcl_Obj
                                         * representing an existing option
                                         * value. */

    Blt_OptionFreeProc *freeProc;       /* Procedure used to free the
                                         * value. */

    ClientData clientData;              /* Arbitrary one-word value used by
                                         * option parser: passed to
                                         * parseProc and printProc. */
} Blt_CustomOption;

/*
 * Structure used to specify information for Tk_ConfigureWidget.  Each
 * structure gives complete information for one option, including how the
 * option is specified on the command line, where it appears in the option
 * database, etc.
 */

typedef struct {
    int type;                           /* Type of option, such as
                                         * BLT_CONFIG_COLOR; see
                                         * definitions below.  Last option
                                         * in table must have type
                                         * BLT_CONFIG_END. */

    const char *switchName;             /* Switch used to specify option in
                                         * argv.  NULL means this spec is
                                         * part of a group. */

    Tk_Uid dbName;                      /* Name for option in option
                                         * database. */

    Tk_Uid dbClass;                     /* Class for option in database. */

    Tk_Uid defValue;                    /* Default value for option if not
                                         * specified in command line or
                                         * database. */

    int offset;                         /* Where in widget record to store
                                         * value; use Blt_Offset macro to
                                         * generate values for this. */

    int specFlags;                      /* Any combination of the values
                                         * defined below; other bits are used
                                         * internally by tkConfig.c. */

    Blt_CustomOption *customPtr;       /* If type is BLT_CONFIG_CUSTOM then
                                        * this is a pointer to info about how
                                        * to parse and print the option.
                                        * Otherwise it is irrelevant. */
} Blt_ConfigSpec;

/*
 * Type values for Blt_ConfigSpec structures.  See the user documentation
 * for details.
 */
typedef enum {
    BLT_CONFIG_ACTIVE_CURSOR, 
    BLT_CONFIG_ANCHOR, 
    BLT_CONFIG_BITMAP,
    BLT_CONFIG_BOOLEAN, 
    BLT_CONFIG_BORDER, 
    BLT_CONFIG_CAP_STYLE, 
    BLT_CONFIG_COLOR, 
    BLT_CONFIG_CURSOR, 
    BLT_CONFIG_CUSTOM, 
    BLT_CONFIG_DOUBLE, 
    BLT_CONFIG_FONT, 
    BLT_CONFIG_INT, 
    BLT_CONFIG_JOIN_STYLE,
    BLT_CONFIG_JUSTIFY, 
    BLT_CONFIG_MM, 
    BLT_CONFIG_RELIEF, 
    BLT_CONFIG_STRING,
    BLT_CONFIG_SYNONYM, 
    BLT_CONFIG_UID, 
    BLT_CONFIG_WINDOW, 

    BLT_CONFIG_BITMASK,
    BLT_CONFIG_BITMASK_INVERT,
    BLT_CONFIG_DASHES,
    BLT_CONFIG_FILL,
    BLT_CONFIG_FLOAT, 
    BLT_CONFIG_INT64, 
    BLT_CONFIG_INT_NNEG,        /* 0..N */
    BLT_CONFIG_INT_POS,         /* 1..N */
    BLT_CONFIG_LIST,
    BLT_CONFIG_LISTOBJ,
    BLT_CONFIG_LONG, 
    BLT_CONFIG_LONG_NNEG,       /* 0..N */
    BLT_CONFIG_LONG_POS,        /* 1..N */
    BLT_CONFIG_OBJ,
    BLT_CONFIG_PAD,
    BLT_CONFIG_PIXELS_NNEG,     /* 1.1c 2m 3.2i excluding negative
                                   values. */
    BLT_CONFIG_PIXELS_POS,      /* 1.1c 2m 3.2i excluding negative
                                 * values and zero. */
    BLT_CONFIG_PIXELS,          /* 1.1c 2m 3.2i. */
    BLT_CONFIG_POSITION,
    BLT_CONFIG_RESIZE,
    BLT_CONFIG_SIDE,
    BLT_CONFIG_STATE, 
    BLT_CONFIG_BACKGROUND,
    BLT_CONFIG_PAINTBRUSH,
    BLT_CONFIG_PIX32, 

    BLT_CONFIG_TK_FONT, 
    BLT_CONFIG_END
} Blt_ConfigTypes;

/*
 * Possible values for flags argument to Tk_ConfigureWidget:
 */
#define BLT_CONFIG_OBJV_ONLY    1

/*
 * Possible flag values for Blt_ConfigSpec structures.  Any bits at or
 * above BLT_CONFIG_USER_BIT may be used by clients for selecting certain
 * entries.  Before changing any values here, coordinate with tkOldConfig.c
 * (internal-use-only flags are defined there).
 */
/*
 * Values for "flags" field of Blt_ConfigSpec structures.  Be sure to
 * coordinate these values with those defined in tk.h
 * (BLT_CONFIG_COLOR_ONLY, etc.).  There must not be overlap!
 *
 * INIT -               Non-zero means (char *) things have been
 *                      converted to Tk_Uid's.
 */
#define INIT                            (1<<0)
#define BLT_CONFIG_NULL_OK              (1<<1)
#define BLT_CONFIG_COLOR_ONLY           (1<<2)
#define BLT_CONFIG_MONO_ONLY            (1<<3)
#define BLT_CONFIG_DONT_SET_DEFAULT     (1<<4)
#define BLT_CONFIG_OPTION_SPECIFIED     (1<<5)
#define BLT_CONFIG_USER_BIT             (1<<8)


#define STATE_NORMAL            (0)
#define STATE_ACTIVE            (1<<0)
#define STATE_DISABLED          (1<<1)
#define STATE_EMPHASIS          (1<<2)

/*
 *---------------------------------------------------------------------------
 *
 * Blt_Pad --
 *
 *      Specifies vertical and horizontal padding.
 *
 *      Padding can be specified on a per side basis.  The fields side1 and
 *      side2 refer to the opposite sides, either horizontally or
 *      vertically.
 *
 *              side1   side2
 *              -----   -----
 *          x | left    right
 *          y | top     bottom
 *
 *---------------------------------------------------------------------------
 */
typedef struct {
    unsigned short int side1, side2;
} Blt_Pad;

#define padLeft         padX.side1
#define padRight        padX.side2
#define padTop          padY.side1
#define padBottom       padY.side2
#define PADDING(x)      ((x).side1 + (x).side2)

/*
 *---------------------------------------------------------------------------
 *
 * The following enumerated values are used as bit flags.
 *      FILL_NONE               Neither coordinate plane is specified 
 *      FILL_X                  Horizontal plane.
 *      FILL_Y                  Vertical plane.
 *      FILL_BOTH               Both vertical and horizontal planes.
 *
 *---------------------------------------------------------------------------
 */
#define FILL_NONE       0
#define FILL_X          1
#define FILL_Y          2
#define FILL_BOTH       3

/*
 * Resize --
 *
 *      These flags indicate in what ways each partition in a table can be
 *      resized from its default dimensions.  The normal size of a
 *      row/column is the minimum amount of space needed to hold the
 *      widgets that span it.  The table may then be stretched or shrunk
 *      depending if the container is larger or smaller than the
 *      table. This can occur if 1) the user resizes the toplevel widget,
 *      or 2) the container is in turn packed into a larger widget and the
 *      "fill" option is set.
 *
 *        RESIZE_NONE     - No resizing from normal size.
 *        RESIZE_EXPAND   - Do not allow the size to decrease.
 *                          The size may increase however.
 *        RESIZE_SHRINK   - Do not allow the size to increase.
 *                          The size may decrease however.
 *        RESIZE_BOTH     - Allow the size to increase or
 *                          decrease from the normal size.
 *        RESIZE_VIRGIN   - Special case of the resize flag.  Used to
 *                          indicate the initial state of the flag.
 *                          Empty rows/columns are treated differently
 *                          if this row/column is set.
 */

#define RESIZE_NONE     0
#define RESIZE_EXPAND   (1<<0)
#define RESIZE_SHRINK   (1<<1)
#define RESIZE_BOTH     (RESIZE_EXPAND | RESIZE_SHRINK)

/*
 *---------------------------------------------------------------------------
 *
 * Blt_Dashes --
 *
 *      List of dash values (maximum 11 based upon PostScript limit).
 *
 *---------------------------------------------------------------------------
 */
typedef struct {
    unsigned char values[12];
    int offset;
} Blt_Dashes;

#define LineIsDashed(d) ((d).values[0] != 0)

/*
 * Blt_Limits --
 *
 *      Defines the bounding of a size (width or height) in the paneset.
 *      It may be related to the widget, pane or paneset size.
 */
typedef struct {
    int flags;                          /* Flags indicate whether using
                                         * default values for limits or
                                         * not. See flags below. */
    int max, min;                       /* Values for respective limits. */
    int nom;                            /* Nominal starting value. */
} Blt_Limits;

#define LIMITS_MIN_SET  (1<<0)
#define LIMITS_MAX_SET  (1<<1)
#define LIMITS_NOM_SET  (1<<2)

#define LIMITS_MIN      0               /* Default minimum limit  */
#define LIMITS_MAX      SHRT_MAX        /* Default maximum limit */
#define LIMITS_NOM      -1000           /* Default nomimal value.
                                         * Indicates if a pane has received
                                         * any space yet */

BLT_EXTERN void Blt_SetDashes (Display *display, GC gc, Blt_Dashes *dashesPtr);

BLT_EXTERN void Blt_ResetLimits(Blt_Limits *limitsPtr);
BLT_EXTERN int Blt_GetLimitsFromObj(Tcl_Interp *interp, Tk_Window tkwin, 
        Tcl_Obj *objPtr, Blt_Limits *limitsPtr);

BLT_EXTERN int Blt_ConfigureInfoFromObj(Tcl_Interp *interp, Tk_Window tkwin, 
        Blt_ConfigSpec *specs, char *widgRec, Tcl_Obj *objPtr, int flags);

BLT_EXTERN int Blt_ConfigureValueFromObj(Tcl_Interp *interp, Tk_Window tkwin, 
        Blt_ConfigSpec *specs, char *widgRec, Tcl_Obj *objPtr, int flags);

BLT_EXTERN int Blt_ConfigureWidgetFromObj(Tcl_Interp *interp, Tk_Window tkwin, 
        Blt_ConfigSpec *specs, int objc, Tcl_Obj *const *objv, char *widgRec, 
        int flags);

BLT_EXTERN int Blt_ConfigureComponentFromObj(Tcl_Interp *interp, 
        Tk_Window tkwin, const char *name, const char *className, 
        Blt_ConfigSpec *specs, int objc, Tcl_Obj *const *objv, char *widgRec, 
        int flags);

BLT_EXTERN int Blt_ConfigModified(Blt_ConfigSpec *specs, ...);

BLT_EXTERN const char *Blt_NameOfState(int state);

BLT_EXTERN void Blt_FreeOptions(Blt_ConfigSpec *specs, char *widgRec, 
        Display *display, int needFlags);

BLT_EXTERN int Blt_ObjIsOption(Blt_ConfigSpec *specs, Tcl_Obj *objPtr, 
        int flags);

BLT_EXTERN int Blt_GetPixelsFromObj(Tcl_Interp *interp, Tk_Window tkwin, 
        Tcl_Obj *objPtr, int flags, int *valuePtr);

BLT_EXTERN int Blt_GetPadFromObj(Tcl_Interp *interp, Tk_Window tkwin, 
        Tcl_Obj *objPtr, Blt_Pad *padPtr);

BLT_EXTERN int Blt_GetStateFromObj(Tcl_Interp *interp, Tcl_Obj *objPtr, 
        int *statePtr);

BLT_EXTERN int Blt_GetFillFromObj(Tcl_Interp *interp, Tcl_Obj *objPtr, 
        int *fillPtr);

BLT_EXTERN int Blt_GetResizeFromObj(Tcl_Interp *interp, Tcl_Obj *objPtr, 
        int *fillPtr);

BLT_EXTERN int Blt_GetDashesFromObj(Tcl_Interp *interp, Tcl_Obj *objPtr, 
        Blt_Dashes *dashesPtr);

#if (_TK_VERSION < _VERSION(8,1,0))
BLT_EXTERN int Tk_GetAnchorFromObj(Tcl_Interp *interp, Tcl_Obj *objPtr, 
        Tk_Anchor *anchorPtr);

BLT_EXTERN int Tk_GetJustifyFromObj(Tcl_Interp *interp, Tcl_Obj *objPtr, 
        Tk_Justify *justifyPtr);

BLT_EXTERN int Tk_GetReliefFromObj(Tcl_Interp *interp, Tcl_Obj *objPtr, 
        int *reliefPtr);

BLT_EXTERN int Tk_GetMMFromObj(Tcl_Interp *interp, Tk_Window tkwin, 
        Tcl_Obj *objPtr, double *doublePtr);

BLT_EXTERN int Tk_GetPixelsFromObj(Tcl_Interp *interp, Tk_Window tkwin, 
        Tcl_Obj *objPtr, int *intPtr);

BLT_EXTERN Tk_3DBorder Tk_Alloc3DBorderFromObj(Tcl_Interp *interp, 
        Tk_Window tkwin, Tcl_Obj *objPtr);

BLT_EXTERN Pixmap Tk_AllocBitmapFromObj(Tcl_Interp *interp, Tk_Window tkwin, 
        Tcl_Obj *objPtr);

BLT_EXTERN Tk_Font Tk_AllocFontFromObj(Tcl_Interp *interp, Tk_Window tkwin, 
        Tcl_Obj *objPtr);

BLT_EXTERN Tk_Cursor Tk_AllocCursorFromObj(Tcl_Interp *interp, Tk_Window tkwin,
        Tcl_Obj *objPtr);

BLT_EXTERN XColor *Tk_AllocColorFromObj(Tcl_Interp *interp, Tk_Window tkwin, 
        Tcl_Obj *objPtr);
#endif /* < 8.1 */

#endif /* BLT_CONFIG_H */
