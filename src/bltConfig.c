/* -*- mode: c; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/* 
 * bltConfig.c --
 *
 * This file contains a Tcl_Obj based replacement for the widget
 * configuration functions in Tk.
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
 * This is based on the old tkConfig.c routines.
 *
 * Copyright (c) 1990-1994 The Regents of the University of California.
 * Copyright (c) 1994-1997 Sun Microsystems, Inc.
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

#define BUILD_BLT_TK_PROCS 1
#include "bltInt.h"

#ifdef HAVE_STDLIB_H
  #include <stdlib.h>
#endif /* HAVE_STDLIB_H */

#ifdef HAVE_STDARG_H
  #include <stdarg.h>
#endif  /* HAVE_STDARG_H */

#ifdef HAVE_LIMITS_H
  #include <limits.h>
#endif  /* HAVE_LIMITS_H */

#ifdef HAVE_STRING_H
  #include <string.h>
#endif /* HAVE_STRING_H */

#ifdef HAVE_CTYPE_H
  #include <ctype.h>
#endif /* HAVE_CTYPE_H */

#include <bltAlloc.h>
#include "bltFont.h"
#include "bltPicture.h"
#include "bltBg.h"

#if (_TK_VERSION < _VERSION(8,1,0))
/*
 *---------------------------------------------------------------------------
 *
 * Tk_GetAnchorFromObj --
 *
 *      Return a Tk_Anchor value based on the value of the objPtr.
 *
 * Results:
 *      The return value is a standard TCL result. If an error occurs
 *      during conversion, an error message is left in the interpreter's
 *      result unless "interp" is NULL.
 *
 * Side effects:
 *      The object gets converted by Tcl_GetIndexFromObj.
 *
 *---------------------------------------------------------------------------
 */
int
Tk_GetAnchorFromObj(
    Tcl_Interp *interp,                 /* Used for error reporting. */
    Tcl_Obj *objPtr,                    /* The object we are trying to get
                                         * the value from. */
    Tk_Anchor *anchorPtr)               /* Where to place the Tk_Anchor
                                         * that corresponds to the string
                                         * value of objPtr. */
{
    return Tk_GetAnchor(interp, Tcl_GetString(objPtr), anchorPtr);
}

/*
 *---------------------------------------------------------------------------
 *
 * Tk_GetJustifyFromObj --
 *
 *      Return a Tk_Justify value based on the value of the objPtr.
 *
 * Results:
 *      The return value is a standard TCL result. If an error occurs
 *      during conversion, an error message is left in the interpreter's
 *      result unless "interp" is NULL.
 *
 * Side effects:
 *      The object gets converted by Tcl_GetIndexFromObj.
 *
 *---------------------------------------------------------------------------
 */
int
Tk_GetJustifyFromObj(
    Tcl_Interp *interp,                 /* Used for error reporting. */
    Tcl_Obj *objPtr,                    /* The object we are trying to get
                                         * the value from. */
    Tk_Justify *justifyPtr)             /* Where to place the Tk_Justify
                                         * that corresponds to the string
                                         * value of objPtr. */
{
    return Tk_GetJustify(interp, Tcl_GetString(objPtr), justifyPtr);
}
/*
 *---------------------------------------------------------------------------
 *
 * Tk_GetReliefFromObj --
 *
 *      Return an integer value based on the value of the objPtr.
 *
 * Results:
 *      The return value is a standard TCL result. If an error occurs
 *      during conversion, an error message is left in the interpreter's
 *      result unless "interp" is NULL.
 *
 * Side effects:
 *      The object gets converted by Tcl_GetIndexFromObj.
 *
 *---------------------------------------------------------------------------
 */
int
Tk_GetReliefFromObj(
    Tcl_Interp *interp,                 /* Used for error reporting. */
    Tcl_Obj *objPtr,                    /* The object we are trying to get
                                         * the value from. */
    int *reliefPtr)                     /* Where to place the answer. */
{
    return Tk_GetRelief(interp, Tcl_GetString(objPtr), reliefPtr);
}


/*
 *---------------------------------------------------------------------------
 *
 * Tk_Alloc3DBorderFromObj --
 *
 *      Given a Tcl_Obj *, map the value to a corresponding Tk_3DBorder
 *      structure based on the tkwin given.
 *
 * Results:
 *      The return value is a token for a data structure describing a 3-D
 *      border.  This token may be passed to procedures such as
 *      Blt_Draw3DRectangle and Tk_Free3DBorder.  If an error prevented the
 *      border from being created then NULL is returned and an error
 *      message will be left in the interp's result.
 *
 * Side effects:
 *      The border is added to an internal database with a reference
 *      count. For each call to this procedure, there should eventually be
 *      a call to FreeBorderObjProc so that the database is cleaned up when
 *      borders aren't in use anymore.
 *
 *---------------------------------------------------------------------------
 */
Tk_3DBorder
Tk_Alloc3DBorderFromObj(
    Tcl_Interp *interp,                 /* Interp for error results. */
    Tk_Window tkwin,                    /* Need the screen the border is
                                         * used on.*/
    Tcl_Obj *objPtr)                    /* Object giving name of color for
                                         * window background. */
{
    return Tk_Get3DBorder(interp, tkwin, Tcl_GetString(objPtr));
}
/*
 *---------------------------------------------------------------------------
 *
 * Tk_AllocBitmapFromObj --
 *
 *      Given a Tcl_Obj *, map the value to a corresponding Pixmap
 *      structure based on the tkwin given.
 *
 * Results:
 *      The return value is the X identifer for the desired bitmap (i.e. a
 *      Pixmap with a single plane), unless string couldn't be parsed
 *      correctly.  In this case, None is returned and an error message is
 *      left in the interp's result.  The caller should never modify the
 *      bitmap that is returned, and should eventually call
 *      Tk_FreeBitmapFromObj when the bitmap is no longer needed.
 *
 * Side effects:
 *      The bitmap is added to an internal database with a reference count.
 *      For each call to this procedure, there should eventually be a call
 *      to Tk_FreeBitmapFromObj, so that the database can be cleaned up
 *      when bitmaps aren't needed anymore.
 *
 *---------------------------------------------------------------------------
 */
Pixmap
Tk_AllocBitmapFromObj(
    Tcl_Interp *interp,                 /* Interp for error results. This
                                         * may be NULL. */
    Tk_Window tkwin,                    /* Need the screen the bitmap is
                                         * used on.*/
    Tcl_Obj *objPtr)                    /* Object describing bitmap; see
                                         * manual entry for legal syntax of
                                         * string value. */
{
    return Tk_GetBitmap(interp, tkwin, Tcl_GetString(objPtr));
}

/*
 *---------------------------------------------------------------------------
 *
 * Tk_AllocFontFromObj -- 
 *
 *      Given a string description of a font, map the description to a
 *      corresponding Blt_Font that represents the font.
 *
 * Results:
 *      The return value is token for the font, or NULL if an error
 *      prevented the font from being created.  If NULL is returned, an
 *      error message will be left in interp's result object.
 *
 * Side effects:
 *      The font is added to an internal database with a reference
 *      count.  For each call to this procedure, there should eventually
 *      be a call to Blt_Font_Free() or Blt_Font_FreeFromObj() so that the
 *      database is cleaned up when fonts aren't in use anymore.
 *
 *---------------------------------------------------------------------------
 */
Tk_Font
Tk_AllocFontFromObj(
    Tcl_Interp *interp,                 /* Interp for database and error
                                         * return. */
    Tk_Window tkwin,                    /* For screen on which font will be
                                         * used. */
    Tcl_Obj *objPtr)                    /* Object describing font, as:
                                         * named font, native format, or
                                         * parseable string. */
{
    return Tk_GetFont(interp, tkwin, Tcl_GetString(objPtr));
}

/*
 *---------------------------------------------------------------------------
 *
 * Tk_AllocCursorFromObj --
 *
 *      Given a Tcl_Obj *, map the value to a corresponding Tk_Cursor
 *      structure based on the tkwin given.
 *
 * Results:
 *      The return value is the X identifer for the desired cursor, unless
 *      objPtr couldn't be parsed correctly.  In this case, None is
 *      returned and an error message is left in the interp's result.  The
 *      caller should never modify the cursor that is returned, and should
 *      eventually call Tk_FreeCursorFromObj when the cursor is no longer
 *      needed.
 *
 * Side effects:
 *      The cursor is added to an internal database with a reference count.
 *      For each call to this procedure, there should eventually be a call
 *      to Tk_FreeCursorFromObj, so that the database can be cleaned up 
 *      when cursors aren't needed anymore.
 *
 *---------------------------------------------------------------------------
 */
Tk_Cursor
Tk_AllocCursorFromObj(
    Tcl_Interp *interp,                 /* Interp for error results. */
    Tk_Window tkwin,                    /* Window in which the cursor will
                                         * be used.*/
    Tcl_Obj *objPtr)                    /* Object describing cursor; see
                                         * manual entry for description of
                                         * legal syntax of this obj's
                                         * string rep. */
{
    return Tk_GetCursor(interp, tkwin, Tcl_GetString(objPtr));
}

/*
 *---------------------------------------------------------------------------
 *
 * Tk_AllocColorFromObj --
 *
 *      Given a Tcl_Obj *, map the value to a corresponding
 *      XColor structure based on the tkwin given.
 *
 * Results:
 *      The return value is a pointer to an XColor structure that
 *      indicates the red, blue, and green intensities for the color
 *      given by the string in objPtr, and also specifies a pixel value 
 *      to use to draw in that color.  If an error occurs, NULL is 
 *      returned and an error message will be left in interp's result
 *      (unless interp is NULL).
 *
 * Side effects:
 *      The color is added to an internal database with a reference count.
 *      For each call to this procedure, there should eventually be a call
 *      to Tk_FreeColorFromObj so that the database is cleaned up when colors
 *      aren't in use anymore.
 *
 *---------------------------------------------------------------------------
 */
XColor *
Tk_AllocColorFromObj(
    Tcl_Interp *interp,         /* Used only for error reporting.  If NULL,
                                 * then no messages are provided. */
    Tk_Window tkwin,            /* Window in which the color will be used.*/
    Tcl_Obj *objPtr)            /* Object that describes the color; string
                                 * value is a color name such as "red" or
                                 * "#ff0000".*/
{
    const char *string;

    string = Tcl_GetString(objPtr);
    return Tk_GetColor(interp, tkwin, Tk_GetUid(string));
}
#endif  /* _TK_VERSION < 8.1.0 */

/* Converters that require Tk. */

/*
 *---------------------------------------------------------------------------
 *
 * Blt_GetPixelsFromObj --
 *
 *      Like Tk_GetPixelsFromObj, but checks for negative, zero.
 *
 * Results:
 *      A standard TCL result.
 *
 *---------------------------------------------------------------------------
 */
int
Blt_GetPixelsFromObj(Tcl_Interp *interp, Tk_Window tkwin, Tcl_Obj *objPtr,
                     int check, int *valuePtr)
{
    int length;

    if (Tk_GetPixelsFromObj(interp, tkwin, objPtr, &length) != TCL_OK) {
        return TCL_ERROR;
    }
    if (length >= SHRT_MAX) {
        Tcl_AppendResult(interp, "bad distance \"", Tcl_GetString(objPtr), 
                 "\": too big to represent", (char *)NULL);
        return TCL_ERROR;
    }
    switch (check) {
    case PIXELS_NNEG:
        if (length < 0) {
            Tcl_AppendResult(interp, "bad distance \"", Tcl_GetString(objPtr), 
                     "\": can't be negative", (char *)NULL);
            return TCL_ERROR;
        }
        break;

    case PIXELS_POS:
        if (length <= 0) {
            Tcl_AppendResult(interp, "bad distance \"", Tcl_GetString(objPtr), 
                     "\": must be positive", (char *)NULL);
            return TCL_ERROR;
        }
        break;

    case PIXELS_ANY:
        break;
    }
    *valuePtr = length;
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * Blt_GetPadFromObj --
 *
 * Results:
 *      A standard TCL result.
 *
 *---------------------------------------------------------------------------
 */
int
Blt_GetPadFromObj(Tcl_Interp *interp, Tk_Window tkwin, Tcl_Obj *objPtr,
                  Blt_Pad *padPtr)
{
    int side1, side2;
    int objc;
    Tcl_Obj **objv;

    if (Tcl_ListObjGetElements(interp, objPtr, &objc, &objv) != TCL_OK) {
        return TCL_ERROR;
    }
    if ((objc < 1) || (objc > 2)) {
        Tcl_AppendResult(interp, "wrong # elements in padding list",
            (char *)NULL);
        return TCL_ERROR;
    }
    if (Blt_GetPixelsFromObj(interp, tkwin, objv[0], PIXELS_NNEG, 
             &side1) != TCL_OK) {
        return TCL_ERROR;
    }
    side2 = side1;
    if ((objc > 1) && 
        (Blt_GetPixelsFromObj(interp, tkwin, objv[1], PIXELS_NNEG, 
              &side2) != TCL_OK)) {
        return TCL_ERROR;
    }
    /* Don't update the pad structure until we know both values are okay. */
    padPtr->side1 = side1;
    padPtr->side2 = side2;
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * Blt_GetDashesFromObj --
 *
 *      Converts a TCL list of dash values into a dash list ready for
 *      use with XSetDashes.
 *
 *      A valid list dash values can have zero through 11 elements
 *      (PostScript limit).  Values must be between 1 and 255. Although
 *      a list of 0 (like the empty string) means no dashes.
 *
 * Results:
 *      A standard TCL result. If the list represented a valid dash
 *      list TCL_OK is returned and *dashesPtr* will contain the
 *      valid dash list. Otherwise, TCL_ERROR is returned and
 *      interp->result will contain an error message.
 *
 *
 *---------------------------------------------------------------------------
 */
int
Blt_GetDashesFromObj(
    Tcl_Interp *interp,
    Tcl_Obj *objPtr,
    Blt_Dashes *dashesPtr)
{
    const char *string;
    char c;

    string = Tcl_GetString(objPtr);
    if (string == NULL) {
        dashesPtr->values[0] = 0;
        return TCL_OK;
    }
    c = string[0];
    if (c == '\0') {
        dashesPtr->values[0] = 0;
    } else if ((c == 'd') && (strcmp(string, "dot") == 0)) {    
        /* 1 */
        dashesPtr->values[0] = 1;
        dashesPtr->values[1] = 0;
    } else if ((c == 'd') && (strcmp(string, "dash") == 0)) {   
        /* 5 2 */
        dashesPtr->values[0] = 5;
        dashesPtr->values[1] = 2;
        dashesPtr->values[2] = 0;
    } else if ((c == 'd') && (strcmp(string, "dashdot") == 0)) { 
        /* 2 4 2 */
        dashesPtr->values[0] = 2;
        dashesPtr->values[1] = 4;
        dashesPtr->values[2] = 2;
        dashesPtr->values[3] = 0;
    } else if ((c == 'd') && (strcmp(string, "dashdotdot") == 0)) { 
        /* 2 4 2 2 */
        dashesPtr->values[0] = 2;
        dashesPtr->values[1] = 4;
        dashesPtr->values[2] = 2;
        dashesPtr->values[3] = 2;
        dashesPtr->values[4] = 0;
    } else {
        int objc;
        Tcl_Obj **objv;
        int i;

        if (Tcl_ListObjGetElements(interp, objPtr, &objc, &objv) != TCL_OK) {
            return TCL_ERROR;
        }
        if (objc > 11) {        /* This is the postscript limit */
            Tcl_AppendResult(interp, "too many values in dash list \"", 
                             string, "\"", (char *)NULL);
            return TCL_ERROR;
        }
        for (i = 0; i < objc; i++) {
            int value;

            if (Tcl_GetIntFromObj(interp, objv[i], &value) != TCL_OK) {
                return TCL_ERROR;
            }
            /*
             * Backward compatibility:
             * Allow list of 0 to turn off dashes
             */
            if ((value == 0) && (objc == 1)) {
                break;
            }
            if ((value < 1) || (value > 255)) {
                Tcl_AppendResult(interp, "dash value \"", 
                         Tcl_GetString(objv[i]), "\" is out of range", 
                         (char *)NULL);
                return TCL_ERROR;
            }
            dashesPtr->values[i] = (unsigned char)value;
        }
        /* Make sure the array ends with a NUL byte  */
        dashesPtr->values[i] = 0;
    }
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * Blt_ResetLimits --
 *
 *      Resets the limits to their default values.
 *
 * Results:
 *      None.
 *
 *---------------------------------------------------------------------------
 */
void
Blt_ResetLimits(Blt_Limits *limitsPtr) /* Limits to be imposed on the value */
{
    limitsPtr->flags = 0;
    limitsPtr->min = LIMITS_MIN;
    limitsPtr->max = LIMITS_MAX;
    limitsPtr->nom = LIMITS_NOM;
}

int 
Blt_GetLimitsFromObj(Tcl_Interp *interp, Tk_Window tkwin, Tcl_Obj *objPtr, 
                     Blt_Limits *limitsPtr)
{
    int values[3];
    int numValues;
    int limitsFlags;

    /* Initialize limits to default values */
    values[2] = LIMITS_NOM;
    values[1] = LIMITS_MAX;
    values[0] = LIMITS_MIN;
    limitsFlags = 0;
    numValues = 0;
    if (objPtr != NULL) {
        Tcl_Obj **objv;
        int objc;
        int i;

        if (Tcl_ListObjGetElements(interp, objPtr, &objc, &objv) != TCL_OK) {
            return TCL_ERROR;
        }
        if (objc > 3) {
            Tcl_AppendResult(interp, "wrong # limits \"", Tcl_GetString(objPtr),
                             "\"", (char *)NULL);
            return TCL_ERROR;
        }
        for (i = 0; i < objc; i++) {
            const char *string;
            int size;

            string = Tcl_GetString(objv[i]);
            if (string[0] == '\0') {
                continue;               /* Empty string: use default value */
            }
            limitsFlags |= (1 << i);
            if (Tk_GetPixelsFromObj(interp, tkwin, objv[i], &size) != TCL_OK) {
                return TCL_ERROR;
            }
            if ((size < LIMITS_MIN) || (size > LIMITS_MAX)) {
                Tcl_AppendResult(interp, "bad limit \"", string, "\"",
                                 (char *)NULL);
                return TCL_ERROR;
            }
            values[i] = size;
        }
        numValues = objc;
    }
    /*
     * Check the limits specified.  We can't check the requested size of
     * widgets.
     */
    switch (numValues) {
    case 1:
        limitsFlags |= (LIMITS_MIN_SET | LIMITS_MAX_SET);
        values[1] = values[0];          /* Set minimum and maximum to value */
        break;

    case 2:
        if (values[1] < values[0]) {
            Tcl_AppendResult(interp, "bad range \"", Tcl_GetString(objPtr),
                "\": min > max", (char *)NULL);
            return TCL_ERROR;           /* Minimum is greater than maximum */
        }
        break;

    case 3:
        if (values[1] < values[0]) {
            Tcl_AppendResult(interp, "bad range \"", Tcl_GetString(objPtr),
                             "\": min > max", (char *)NULL);
            return TCL_ERROR;           /* Minimum is greater than maximum */
        }
        if ((values[2] < values[0]) || (values[2] > values[1])) {
            Tcl_AppendResult(interp, "nominal value \"", Tcl_GetString(objPtr),
                "\" out of range", (char *)NULL);
            return TCL_ERROR;           /* Nominal is outside of range defined
                                         * by minimum and maximum */
        }
        break;
    }
    limitsPtr->min = values[0];
    limitsPtr->max = values[1];
    limitsPtr->nom = values[2];
    limitsPtr->flags = limitsFlags;
    return TCL_OK;
}

/* Configuration option helper routines */

/*
 *---------------------------------------------------------------------------
 *
 * DoConfig --
 *
 *      This procedure applies a single configuration option
 *      to a widget record.
 *
 * Results:
 *      A standard TCL return value.
 *
 * Side effects:
 *      WidgRec is modified as indicated by specPtr and value.
 *      The old value is recycled, if that is appropriate for
 *      the value type.
 *
 *---------------------------------------------------------------------------
 */
static int
DoConfig(
    Tcl_Interp *interp,         /* Interpreter for error reporting. */
    Tk_Window tkwin,            /* Window containing widget (needed to
                                 * set up X resources). */
    Blt_ConfigSpec *sp,         /* Specifier to apply. */
    Tcl_Obj *objPtr,            /* Value to use to fill in widgRec. */
    char *widgRec,              /* Record whose fields are to be
                                 * modified.  Values must be properly
                                 * initialized. */
    int specFlags)
{
    int objIsEmpty;

    objIsEmpty = FALSE;
    if (objPtr == NULL) {
        objIsEmpty = TRUE;
    } else if (sp->specFlags & BLT_CONFIG_NULL_OK) {
        int length;

        if (objPtr->bytes != NULL) {
            length = objPtr->length;
        } else {
            Tcl_GetStringFromObj(objPtr, &length);
        }
        objIsEmpty = (length == 0);
    }
    do {
        char *ptr;

        ptr = widgRec + sp->offset;
        switch (sp->type) {
        case BLT_CONFIG_ANCHOR: 
            {
                Tk_Anchor anchor;
                
                if (Tk_GetAnchorFromObj(interp, objPtr, &anchor) != TCL_OK) {
                    return TCL_ERROR;
                }
                *(Tk_Anchor *)ptr = anchor;
            }
            break;

        case BLT_CONFIG_BITMAP: 
            {
                Pixmap bitmap;
                
                if (objIsEmpty) {
                    bitmap = None;
                } else {
                    bitmap = Tk_AllocBitmapFromObj(interp, tkwin, objPtr);
                    if (bitmap == None) {
                        return TCL_ERROR;
                    }
                }
                if (*(Pixmap *)ptr != None) {
                    Tk_FreeBitmap(Tk_Display(tkwin), *(Pixmap *)ptr);
                }
                *(Pixmap *)ptr = bitmap;
            }
            break;

        case BLT_CONFIG_BOOLEAN: 
            {
                int bool;
                
                if (Tcl_GetBooleanFromObj(interp, objPtr, &bool) != TCL_OK) {
                    return TCL_ERROR;
                }
                if (sp->customPtr != NULL) {
                    if (bool) {
                        *((int *)ptr) |= (intptr_t)sp->customPtr;
                    } else {
                        *((int *)ptr) &= ~(intptr_t)sp->customPtr;
                    }
                } else {
                    *((int *)ptr) = bool;
                }
            }
            break;

        case BLT_CONFIG_BORDER: 
            {
                Tk_3DBorder border;

                if (objIsEmpty) {
                    border = NULL;
                } else {
                    border = Tk_Alloc3DBorderFromObj(interp, tkwin, objPtr);
                    if (border == NULL) {
                        return TCL_ERROR;
                    }
                }
                if (*((Tk_3DBorder *)ptr) != NULL) {
                    Tk_Free3DBorder(*((Tk_3DBorder *)ptr));
                }
                *((Tk_3DBorder *)ptr) = border;
            }
            break;

        case BLT_CONFIG_CAP_STYLE: 
            {
                int cap;
                Tk_Uid uid;
                
                uid = Tk_GetUid(Tcl_GetString(objPtr));
                if (Tk_GetCapStyle(interp, uid, &cap) != TCL_OK) {
                    return TCL_ERROR;
                }
                *(int *)ptr = cap;
            }
            break;

        case BLT_CONFIG_COLOR: 
            {
                XColor *color;
                
                if (objIsEmpty) {
                    color = NULL;
                } else {
                    color = Tk_GetColor(interp, tkwin, 
                        Tk_GetUid(Tcl_GetString(objPtr)));
                    if (color == NULL) {
                        return TCL_ERROR;
                    }
                }
                if (*(XColor **)ptr != NULL) {
                    Tk_FreeColor(*(XColor **)ptr);
                }
                *(XColor **)ptr = color;
            }
            break;

        case BLT_CONFIG_CURSOR:
        case BLT_CONFIG_ACTIVE_CURSOR: 
            {
                Tk_Cursor cursor;
                
                if (objIsEmpty) {
                    cursor = None;
                } else {
                    cursor = Tk_AllocCursorFromObj(interp, tkwin, objPtr);
                    if (cursor == None) {
                        return TCL_ERROR;
                    }
                }
                if (*(Tk_Cursor *)ptr != None) {
                    Tk_FreeCursor(Tk_Display(tkwin), *(Tk_Cursor *)ptr);
                }
                *(Tk_Cursor *)ptr = cursor;
                if (sp->type == BLT_CONFIG_ACTIVE_CURSOR) {
                    Tk_DefineCursor(tkwin, cursor);
                }
            }
            break;

        case BLT_CONFIG_CUSTOM: 
            if ((*sp->customPtr->parseProc)(sp->customPtr->clientData, interp, 
                tkwin, objPtr, widgRec, sp->offset, sp->specFlags) != TCL_OK) {
                return TCL_ERROR;
            }
            break;

        case BLT_CONFIG_DOUBLE: 
            {
                double value;
                
                if (Tcl_GetDoubleFromObj(interp, objPtr, &value) != TCL_OK) {
                    return TCL_ERROR;
                }
                *(double *)ptr = value;
            }
            break;

        case BLT_CONFIG_FONT: 
            {
                Blt_Font font;
                
                if (objIsEmpty) {
                    font = NULL;
                } else {
                    font = Blt_AllocFontFromObj(interp, tkwin, objPtr);
                    if (font == NULL) {
                        return TCL_ERROR;
                    }
                }
                if (*(Blt_Font *)ptr != NULL) {
                    Blt_Font_Free(*(Blt_Font *)ptr);
                }
                *(Blt_Font *)ptr = font;
            }
            break;

        case BLT_CONFIG_TK_FONT: 
            {
                Tk_Font font;
                
                if (objIsEmpty) {
                    font = NULL;
                } else {
                    font = Tk_AllocFontFromObj(interp, tkwin, objPtr);
                    if (font == NULL) {
                        return TCL_ERROR;
                    }
                }
                if (*(Tk_Font *)ptr != NULL) {
                    Tk_FreeFont(*(Tk_Font *)ptr);
                }
                *(Tk_Font *)ptr = font;
            }
            break;

        case BLT_CONFIG_INT: 
            {
                int value;
                
                if (Tcl_GetIntFromObj(interp, objPtr, &value) != TCL_OK) {
                    return TCL_ERROR;
                }
                *(int *)ptr = value;
            }
            break;

        case BLT_CONFIG_JOIN_STYLE: 
            {
                int join;
                Tk_Uid uid;

                uid = Tk_GetUid(Tcl_GetString(objPtr));
                if (Tk_GetJoinStyle(interp, uid, &join) != TCL_OK) {
                    return TCL_ERROR;
                }
                *(int *)ptr = join;
            }
            break;

        case BLT_CONFIG_JUSTIFY: 
            {
                Tk_Justify justify;
                
                if (Tk_GetJustifyFromObj(interp, objPtr, &justify) != TCL_OK) {
                    return TCL_ERROR;
                }
                *(Tk_Justify *)ptr = justify;
            }
            break;

        case BLT_CONFIG_MM: 
            {
                double value;

                if (Tk_GetMMFromObj(interp, tkwin, objPtr, &value) != TCL_OK) {
                    return TCL_ERROR;
                }
                *(double *)ptr = value;
            }
            break;


        case BLT_CONFIG_RELIEF: 
            {
                int relief;
                
                if (Tk_GetReliefFromObj(interp, objPtr, &relief) != TCL_OK) {
                    return TCL_ERROR;
                }
                *(int *)ptr = relief;
            }
            break;

        case BLT_CONFIG_STRING: 
            {
                const char *value;
                
                value = (objIsEmpty) ? NULL : 
                    Blt_AssertStrdup(Tcl_GetString(objPtr));
                if (*(char **)ptr != NULL) {
                    Blt_Free(*(char **)ptr);
                }
                *(const char **)ptr = value;
            }
            break;

        case BLT_CONFIG_UID: 
            if (*(Blt_Uid *)ptr != NULL) {
                Blt_FreeUid(*(Blt_Uid *)ptr);
            }
            if (objIsEmpty) {
                *(Blt_Uid *)ptr = NULL;
            } else {
                *(Blt_Uid *)ptr = Blt_GetUid(Tcl_GetString(objPtr));
            }
            break;

        case BLT_CONFIG_WINDOW: 
            {
                Tk_Window tkwin2;

                if (objIsEmpty) {
                    tkwin2 = None;
                } else {
                    const char *path;

                    path = Tcl_GetString(objPtr);
                    tkwin2 = Tk_NameToWindow(interp, path, tkwin);
                    if (tkwin2 == NULL) {
                        return TCL_ERROR;
                    }
                }
                *(Tk_Window *)ptr = tkwin2;
            }
            break;

        case BLT_CONFIG_BITMASK: 
            {
                int bool;
                uintptr_t mask;
                uintptr_t flags;

                if (Tcl_GetBooleanFromObj(interp, objPtr, &bool) != TCL_OK) {
                    return TCL_ERROR;
                }
                mask = (uintptr_t)sp->customPtr;
                flags = *(uintptr_t *)ptr;
                flags &= ~mask;
                if (bool) {
                    flags |= mask;
                }
                *(uintptr_t *)ptr = flags;
            }
            break;

        case BLT_CONFIG_BITMASK_INVERT: 
            {
                int bool;
                uintptr_t mask;
                uintptr_t flags;

                if (Tcl_GetBooleanFromObj(interp, objPtr, &bool) != TCL_OK) {
                    return TCL_ERROR;
                }
                mask = (uintptr_t)sp->customPtr;
                flags = *(uintptr_t *)ptr;
                flags &= ~mask;
                if (!bool) {
                    flags |= mask;
                }
                *(uintptr_t *)ptr = flags;
            }
            break;

        case BLT_CONFIG_DASHES:
            if (Blt_GetDashesFromObj(interp, objPtr, (Blt_Dashes *)ptr) 
                != TCL_OK) {
                return TCL_ERROR;
            }
            break;


        case BLT_CONFIG_FILL:
            if (Blt_GetFillFromObj(interp, objPtr, (int *)ptr) != TCL_OK) {
                return TCL_ERROR;
            }
            break;

        case BLT_CONFIG_RESIZE:
            if (Blt_GetResizeFromObj(interp, objPtr, (int *)ptr) != TCL_OK) {
                return TCL_ERROR;
            }
            break;

        case BLT_CONFIG_FLOAT: 
            {
                double value;
                
                if (Tcl_GetDoubleFromObj(interp, objPtr, &value) != TCL_OK) {
                    return TCL_ERROR;
                }
                *(float *)ptr = (float)value;
            }
            break;

        case BLT_CONFIG_INT_NNEG: 
            {
                int value;
                
                if (Tcl_GetIntFromObj(interp, objPtr, &value) != TCL_OK) {
                    return TCL_ERROR;
                }
                if (value < 0) {
                    Tcl_AppendResult(interp, "value \"", Tcl_GetString(objPtr),
                                     "\" can't be negative.", (char *)NULL);
                    return TCL_ERROR;
                }
                *(int *)ptr = value;
            }
            break;


        case BLT_CONFIG_INT_POS: 
            {
                int value;
                
                if (Tcl_GetIntFromObj(interp, objPtr, &value) != TCL_OK) {
                    return TCL_ERROR;
                }
                if (value <= 0) {
                    Tcl_AppendResult(interp, "value \"", Tcl_GetString(objPtr),
                                     "\" must be positive.", (char *)NULL);
                    return TCL_ERROR;
                }
                *(int *)ptr = value;
            }
            break;

        case BLT_CONFIG_LIST: 
            {
                const char **argv;
                int argc;
                
                if (Tcl_SplitList(interp, Tcl_GetString(objPtr), &argc, &argv) 
                    != TCL_OK) {
                    return TCL_ERROR;
                }
                if (*(char ***)ptr != NULL) {
                    Tcl_Free((char *)(*(char ***)ptr));
                }
                *(const char ***)ptr = argv;
            }
            break;

        case BLT_CONFIG_LISTOBJ: 
            {
                Tcl_Obj *newObjPtr;

                if (objIsEmpty) {
                    newObjPtr = NULL;
                } else {
                    Tcl_Obj **objv;
                    int objc;
                
                    if (Tcl_ListObjGetElements(interp, objPtr, &objc, &objv) 
                        != TCL_OK) {
                        return TCL_ERROR;
                    }
                    /* Increment the new obj. In case old is new. */
                    Tcl_IncrRefCount(objPtr);
                    newObjPtr = objPtr;
                }
                if (*(Tcl_Obj **)ptr != NULL) {
                    Tcl_DecrRefCount(*(Tcl_Obj **)ptr);
                }
                *(Tcl_Obj **)ptr  = newObjPtr;
            }
            break;

        case BLT_CONFIG_INT64: 
            {
                int64_t value;
                
                if (Blt_GetInt64FromObj(interp, objPtr, &value) != TCL_OK) {
                    return TCL_ERROR;
                }
                *(int64_t *)ptr = value;
            }
            break;


        case BLT_CONFIG_LONG: 
            {
                long value;
                
                if (Blt_GetLongFromObj(interp, objPtr, &value) != TCL_OK) {
                    return TCL_ERROR;
                }
                *(long *)ptr = value;
            }
            break;

        case BLT_CONFIG_LONG_NNEG: 
            {
                long value;
                
                if (Blt_GetLongFromObj(interp, objPtr, &value)
                    != TCL_OK) {
                    return TCL_ERROR;
                }
                if (value < 0) {
                    Tcl_AppendResult(interp, "value \"", Tcl_GetString(objPtr),
                                     "\" can't be negative.", (char *)NULL);
                    return TCL_ERROR;
                }
                *(long *)ptr = value;
            }
            break;


        case BLT_CONFIG_LONG_POS: 
            {
                long value;
                
                if (Blt_GetLongFromObj(interp, objPtr, &value)
                    != TCL_OK) {
                    return TCL_ERROR;
                }
                if (value <= 0) {
                    Tcl_AppendResult(interp, "value \"", Tcl_GetString(objPtr),
                                     "\" must be positive.", (char *)NULL);
                    return TCL_ERROR;
                }
                *(long *)ptr = value;
            }
            break;

        case BLT_CONFIG_OBJ: 
            {
                Tcl_Obj *newObjPtr;

                if (objIsEmpty) {
                    newObjPtr = NULL;
                } else {
                    /* First increment the new obj. In case old is new. */
                    Tcl_IncrRefCount(objPtr);
                    newObjPtr = objPtr;
                }
                if (*(Tcl_Obj **)ptr != NULL) {
                    Tcl_DecrRefCount(*(Tcl_Obj **)ptr);
                }
                *(Tcl_Obj **)ptr  = newObjPtr;
            }
            break;

        case BLT_CONFIG_PAD:
            if (Blt_GetPadFromObj(interp, tkwin, objPtr, (Blt_Pad *)ptr) 
                != TCL_OK) {
                return TCL_ERROR;
            }
            break;

        case BLT_CONFIG_PIXELS_NNEG: 
            {
                int value;
                
                if (Blt_GetPixelsFromObj(interp, tkwin, objPtr, 
                        PIXELS_NNEG, &value) != TCL_OK) {
                    return TCL_ERROR;
                }
                *(int *)ptr = value;
            }
            break;

        case BLT_CONFIG_PIXELS: 
            {
                int value;
                
                if (Blt_GetPixelsFromObj(interp, tkwin, objPtr, PIXELS_ANY, 
                        &value) != TCL_OK) {
                    return TCL_ERROR;
                }
                *(int *)ptr = value;
            }
            break;

        case BLT_CONFIG_PIXELS_POS: 
            {
                int value;
                
                if (Blt_GetPixelsFromObj(interp, tkwin, objPtr, PIXELS_POS,
                        &value) != TCL_OK) {
                    return TCL_ERROR;
                }
                *(int *)ptr = value;
            }
            break;

        case BLT_CONFIG_POSITION: 
            {
                long value;
                
                if (Blt_GetPositionFromObj(interp, objPtr, &value) != TCL_OK) {
                    return TCL_ERROR;
                }
                *(long *)ptr = value;
            }
            break;

        case BLT_CONFIG_STATE: 
            if (Blt_GetStateFromObj(interp, objPtr, (int *)ptr) != TCL_OK) {
                return TCL_ERROR;
            }
            break;

        case BLT_CONFIG_SIDE:
            if (Blt_GetSideFromObj(interp, objPtr, (int *)ptr) != TCL_OK) {
                return TCL_ERROR;
            }
            break;

        case BLT_CONFIG_BACKGROUND: 
            {
                Blt_Bg bg;
                
                if (objIsEmpty) {
                    bg = NULL;
                } else {
                    if (Blt_GetBgFromObj(interp, tkwin, objPtr, &bg)
                        != TCL_OK) {
                        return TCL_ERROR;
                    }
                }
                if (*(Blt_Bg *)ptr != NULL) {
                    Blt_Bg_Free(*(Blt_Bg *)ptr);
                }
                *(Blt_Bg *)ptr = bg;
            }
            break;

        case BLT_CONFIG_PAINTBRUSH: 
            {
                Blt_PaintBrush brush;
                
                if (objIsEmpty) {
                    brush = NULL;
                } else {
                    if (Blt_GetPaintBrushFromObj(interp, objPtr, &brush)
                        != TCL_OK) {
                        return TCL_ERROR;
                    }
                }
                if (*(Blt_PaintBrush *)ptr != NULL) {
                    Blt_FreeBrush(*(Blt_PaintBrush *)ptr);
                }
                *(Blt_PaintBrush *)ptr = brush;
            }
            break;


        case BLT_CONFIG_PIX32: 
            if (Blt_GetPixelFromObj(interp, objPtr, (Blt_Pixel *)ptr)!=TCL_OK) {
                return TCL_ERROR;
            }
            break;

        default: 
            Tcl_AppendResult(interp, "bad config table: unknown type ", 
                             Blt_Itoa(sp->type), (char *)NULL);
            return TCL_ERROR;
        }
        sp++;
    } while ((sp->switchName == NULL) && (sp->type != BLT_CONFIG_END));
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * FormatConfigValue --
 *
 *      This procedure formats the current value of a configuration
 *      option.
 *
 * Results:
 *      The return value is the formatted value of the option given
 *      by specPtr and widgRec.  If the value is static, so that it
 *      need not be freed, *freeProcPtr will be set to NULL;  otherwise
 *      *freeProcPtr will be set to the address of a procedure to
 *      free the result, and the caller must invoke this procedure
 *      when it is finished with the result.
 *
 * Side effects:
 *      None.
 *
 *---------------------------------------------------------------------------
 */
static Tcl_Obj *
FormatConfigValue(
    Tcl_Interp *interp,         /* Interpreter for use in real conversions. */
    Tk_Window tkwin,            /* Window corresponding to widget. */
    Blt_ConfigSpec *sp,         /* Pointer to information describing option.
                                 * Must not point to a synonym option. */
    char *widgRec)              /* Pointer to record holding current
                                 * values of info for widget. */
{
    char *ptr;
    const char *string;

    ptr = widgRec + sp->offset;
    string = "";
    switch (sp->type) {
    case BLT_CONFIG_ANCHOR:
        string = Tk_NameOfAnchor(*(Tk_Anchor *)ptr);
        break;

    case BLT_CONFIG_BITMAP: 
        if (*(Pixmap *)ptr != None) {
            string = Tk_NameOfBitmap(Tk_Display(tkwin), *(Pixmap *)ptr);
        }
        break;

    case BLT_CONFIG_BOOLEAN: 
        {
            int bool;

            if (sp->customPtr != NULL) {
                bool = *((int *)ptr) & (uintptr_t)sp->customPtr;
            } else {
                bool = *((int *)ptr) &= ~(uintptr_t)sp->customPtr;
            }
            return Tcl_NewBooleanObj(bool);
        }
    case BLT_CONFIG_BORDER: 
        if (*((Tk_3DBorder *)ptr) != NULL) {
            string = Tk_NameOf3DBorder(*((Tk_3DBorder *)ptr));
        }
        break;

    case BLT_CONFIG_CAP_STYLE:
        string = Tk_NameOfCapStyle(*(int *)ptr);
        break;

    case BLT_CONFIG_COLOR: 
        if (*(XColor **)ptr != NULL) {
            string = Tk_NameOfColor(*(XColor **)ptr);
        }
        break;

    case BLT_CONFIG_CURSOR:
    case BLT_CONFIG_ACTIVE_CURSOR:
        if (*(Tk_Cursor *)ptr != None) {
            string = Tk_NameOfCursor(Tk_Display(tkwin), *(Tk_Cursor *)ptr);
        }
        break;

    case BLT_CONFIG_CUSTOM:
        return (*sp->customPtr->printProc)
                (sp->customPtr->clientData, interp, tkwin, widgRec, 
                sp->offset, sp->specFlags);

    case BLT_CONFIG_DOUBLE: 
        return Tcl_NewDoubleObj(*(double *)ptr);

    case BLT_CONFIG_FONT: 
        if (*(Blt_Font *)ptr != NULL) {
            string = Blt_Font_Name(*(Blt_Font *)ptr);
        }
        break;

    case BLT_CONFIG_TK_FONT: 
        if (*(Tk_Font *)ptr != NULL) {
            string = Tk_NameOfFont(*(Tk_Font *)ptr);
        }
        break;

    case BLT_CONFIG_INT: 
        return Tcl_NewIntObj(*(int *)ptr);

    case BLT_CONFIG_JOIN_STYLE:
        string = Tk_NameOfJoinStyle(*(int *)ptr);
        break;

    case BLT_CONFIG_JUSTIFY:
        string = Tk_NameOfJustify(*(Tk_Justify *)ptr);
        break;

    case BLT_CONFIG_MM:
        return Tcl_NewDoubleObj(*(double *)ptr);

    case BLT_CONFIG_PIXELS: 
    case BLT_CONFIG_PIXELS_POS: 
    case BLT_CONFIG_PIXELS_NNEG: 
        return Tcl_NewIntObj(*(int *)ptr);

    case BLT_CONFIG_POSITION: 
        return Tcl_NewLongObj(*(long *)ptr);

    case BLT_CONFIG_RELIEF: 
        string = Tk_NameOfRelief(*(int *)ptr);
        break;

    case BLT_CONFIG_STRING: 
    case BLT_CONFIG_UID:
        if (*(char **)ptr != NULL) {
            string = *(char **)ptr;
        }
        break;

    case BLT_CONFIG_BITMASK:
        {
            uintptr_t flags;
            uintptr_t mask;

            flags = (uintptr_t)sp->customPtr;
            mask = (*(uintptr_t *)ptr);
            return Tcl_NewBooleanObj((mask & flags));
        }

    case BLT_CONFIG_BITMASK_INVERT:
        {
            uintptr_t flags;
            uintptr_t mask;

            flags = (uintptr_t)sp->customPtr;
            mask = (*(uintptr_t *)ptr);
            return Tcl_NewBooleanObj((mask & flags) == 0);
        }

    case BLT_CONFIG_DASHES: 
        {
            unsigned char *p;
            Tcl_Obj *listObjPtr;
            Blt_Dashes *dashesPtr = (Blt_Dashes *)ptr;
            
            listObjPtr = Tcl_NewListObj(0, (Tcl_Obj **)NULL);
            for(p = dashesPtr->values; *p != 0; p++) {
                Tcl_ListObjAppendElement(interp, listObjPtr, Tcl_NewIntObj(*p));
            }
            return listObjPtr;
        }

    case BLT_CONFIG_INT_NNEG:
    case BLT_CONFIG_INT_POS:
        return Tcl_NewIntObj(*(int *)ptr);

    case BLT_CONFIG_FILL: 
        string = Blt_NameOfFill(*(int *)ptr);
        break;

    case BLT_CONFIG_RESIZE: 
        string = Blt_NameOfResize(*(int *)ptr);
        break;

    case BLT_CONFIG_FLOAT: 
        {
            double x = *(float *)ptr;
            return Tcl_NewDoubleObj(x);
        }

    case BLT_CONFIG_LIST: 
        {
            Tcl_Obj *listObjPtr;
            char *const *p;
            
            listObjPtr = Tcl_NewListObj(0, (Tcl_Obj **)NULL);
            for (p = *(char ***)ptr; *p != NULL; p++) {
                Tcl_Obj *objPtr;

                objPtr = Tcl_NewStringObj(*p, -1);
                Tcl_ListObjAppendElement(interp, listObjPtr, objPtr);
            }
            return listObjPtr;
        }

    case BLT_CONFIG_LONG: 
        return Tcl_NewLongObj(*(long *)ptr);

    case BLT_CONFIG_LONG_NNEG:
    case BLT_CONFIG_LONG_POS:
        return Tcl_NewLongObj(*(long *)ptr);

    case BLT_CONFIG_OBJ:
    case BLT_CONFIG_LISTOBJ:
        if (*(Tcl_Obj **)ptr != NULL) {
            return *(Tcl_Obj **)ptr;
        }
        break;

    case BLT_CONFIG_PAD: 
        {
            Blt_Pad *padPtr = (Blt_Pad *)ptr;
            Tcl_Obj *objPtr, *listObjPtr;
            
            listObjPtr = Tcl_NewListObj(0, (Tcl_Obj **)NULL);
            objPtr = Tcl_NewIntObj(padPtr->side1);
            Tcl_ListObjAppendElement(interp, listObjPtr, objPtr);
            objPtr = Tcl_NewIntObj(padPtr->side2);
            Tcl_ListObjAppendElement(interp, listObjPtr, objPtr);
            return listObjPtr;
        }

    case BLT_CONFIG_STATE: 
        string = Blt_NameOfState(*(int *)ptr);
        break;

    case BLT_CONFIG_SIDE: 
        string = Blt_NameOfSide(*(int *)ptr);
        break;

    case BLT_CONFIG_BACKGROUND: 
        if (*(Blt_Bg *)ptr != NULL) {
            string = Blt_Bg_Name(*(Blt_Bg *)ptr);
        }
        break;

    case BLT_CONFIG_PAINTBRUSH: 
        if (*(Blt_PaintBrush *)ptr != NULL) {
            string = Blt_GetBrushName(*(Blt_PaintBrush *)ptr);
        }
        break;

    case BLT_CONFIG_PIX32: 
        string = Blt_NameOfPixel((Blt_Pixel *)ptr);
        break;

    default: 
        string = "?? unknown type ??";
    }
    return Tcl_NewStringObj(string, -1);
}

/*
 *---------------------------------------------------------------------------
 *
 * FormatConfigInfo --
 *
 *      Create a valid TCL list holding the configuration information
 *      for a single configuration option.
 *
 * Results:
 *      A TCL list, dynamically allocated.  The caller is expected to
 *      arrange for this list to be freed eventually.
 *
 * Side effects:
 *      Memory is allocated.
 *
 *---------------------------------------------------------------------------
 */
static Tcl_Obj *
FormatConfigInfo(
    Tcl_Interp *interp,         /* Interpreter to use for things
                                 * like floating-point precision. */
    Tk_Window tkwin,            /* Window corresponding to widget. */
    Blt_ConfigSpec *sp,         /* Pointer to information describing
                                 * option. */
    char *widgRec)              /* Pointer to record holding current
                                 * values of info for widget. */
{
    Tcl_Obj *listObjPtr;

    listObjPtr = Tcl_NewListObj(0, (Tcl_Obj **)NULL);
    if (sp->switchName != NULL) {
        Tcl_ListObjAppendElement(interp, listObjPtr, 
                Tcl_NewStringObj(sp->switchName, -1));
    }  else {
        Tcl_ListObjAppendElement(interp, listObjPtr, Tcl_NewStringObj("", -1));
    }
    if (sp->dbName != NULL) {
        Tcl_ListObjAppendElement(interp, listObjPtr,  
                Tcl_NewStringObj(sp->dbName, -1));
    } else {
        Tcl_ListObjAppendElement(interp, listObjPtr, Tcl_NewStringObj("", -1));
    }
    if (sp->type == BLT_CONFIG_SYNONYM) {
        return listObjPtr;
    } 
    if (sp->dbClass != NULL) {
        Tcl_ListObjAppendElement(interp, listObjPtr, 
                Tcl_NewStringObj(sp->dbClass, -1));
    } else {
        Tcl_ListObjAppendElement(interp, listObjPtr, Tcl_NewStringObj("", -1));
    }
    if (sp->defValue != NULL) {
        Tcl_ListObjAppendElement(interp, listObjPtr, 
                Tcl_NewStringObj(sp->defValue, -1));
    } else {
        Tcl_ListObjAppendElement(interp, listObjPtr, Tcl_NewStringObj("", -1));
    }
    Tcl_ListObjAppendElement(interp, listObjPtr, 
        FormatConfigValue(interp, tkwin, sp, widgRec));
    return listObjPtr;
}

/*
 *---------------------------------------------------------------------------
 *
 * FindConfigSpec --
 *
 *      Search through a table of configuration specs, looking for
 *      one that matches a given switchName.
 *
 * Results:
 *      The return value is a pointer to the matching entry, or NULL
 *      if nothing matched.  In that case an error message is left
 *      in the interp's result.
 *
 * Side effects:
 *      None.
 *
 *---------------------------------------------------------------------------
 */
static Blt_ConfigSpec *
FindConfigSpec(
    Tcl_Interp *interp,         /* Used for reporting errors. */
    Blt_ConfigSpec *specs,      /* Pointer to table of configuration
                                 * specifications for a widget. */
    Tcl_Obj *objPtr,            /* Name (suitable for use in a "config"
                                 * command) identifying particular option. */
    int needFlags,              /* Flags that must be present in matching
                                 * entry. */
    int hateFlags)              /* Flags that must NOT be present in
                                 * matching entry. */
{
    Blt_ConfigSpec *matchPtr;   /* Matching spec, or NULL. */
    Blt_ConfigSpec *sp;
    const char *string;
    char c;                     /* First character of current argument. */
    int length;

    string = Tcl_GetStringFromObj(objPtr, &length);
    c = string[1];
    matchPtr = NULL;
    for (sp = specs; sp->type != BLT_CONFIG_END; sp++) {
        if (sp->switchName == NULL) {
            continue;
        }
        if ((sp->switchName[1] != c) || 
            (strncmp(sp->switchName, string, length) != 0)) {
            continue;
        }
        if (((sp->specFlags & needFlags) != needFlags) || 
            (sp->specFlags & hateFlags)) {
            continue;
        }
        if (sp->switchName[length] == 0) {
            matchPtr = sp;
            goto gotMatch;
        }
        if (matchPtr != NULL) {
            if (interp != NULL) {
                Tcl_AppendResult(interp, "ambiguous option \"", string, "\"", 
                        (char *)NULL);
            }
            return (Blt_ConfigSpec *)NULL;
        }
        matchPtr = sp;
    }

    if (matchPtr == NULL) {
        if (interp != NULL) {
            Tcl_AppendResult(interp, "unknown option \"", string, "\"", 
                (char *)NULL);
        }
        return (Blt_ConfigSpec *)NULL;
    }

    /*
     * Found a matching entry.  If it's a synonym, then find the
     * entry that it's a synonym for.
     */

 gotMatch:
    sp = matchPtr;
    if (sp->type == BLT_CONFIG_SYNONYM) {
        for (sp = specs; /*empty*/; sp++) {
            if (sp->type == BLT_CONFIG_END) {
                if (interp != NULL) {
                    Tcl_AppendResult(interp, 
                        "couldn't find synonym for option \"", string, "\"", 
                        (char *)NULL);
                }
                return (Blt_ConfigSpec *) NULL;
            }
            if ((sp->dbName == matchPtr->dbName) && 
                (sp->type != BLT_CONFIG_SYNONYM) && 
                ((sp->specFlags & needFlags) == needFlags) && 
                !(sp->specFlags & hateFlags)) {
                break;
            }
        }
    }
    return sp;
}

/* Public routines */

/*
 *---------------------------------------------------------------------------
 *
 * Blt_ConfigureWidgetFromObj --
 *
 *      Process command-line options and database options to
 *      fill in fields of a widget record with resources and
 *      other parameters.
 *
 * Results:
 *      A standard TCL return value.  In case of an error,
 *      the interp's result will hold an error message.
 *
 * Side effects:
 *      The fields of widgRec get filled in with information
 *      from argc/argv and the option database.  Old information
 *      in widgRec's fields gets recycled.
 *
 *---------------------------------------------------------------------------
 */
int
Blt_ConfigureWidgetFromObj(
    Tcl_Interp *interp,         /* Interpreter for error reporting. */
    Tk_Window tkwin,            /* Window containing widget (needed to
                                 * set up X resources). */
    Blt_ConfigSpec *specs,      /* Describes legal options. */
    int objc,                   /* Number of elements in argv. */
    Tcl_Obj *const *objv,       /* Command-line options. */
    char *widgRec,              /* Record whose fields are to be
                                 * modified.  Values must be properly
                                 * initialized. */
    int flags)                  /* Used to specify additional flags
                                 * that must be present in config specs
                                 * for them to be considered.  Also,
                                 * may have BLT_CONFIG_OBJV_ONLY set. */
{
    Blt_ConfigSpec *sp;
    int needFlags;              /* Specs must contain this set of flags
                                 * or else they are not considered. */
    int hateFlags;              /* If a spec contains any bits here, it's
                                 * not considered. */

    if (tkwin == NULL) {
        /*
         * Either we're not really in Tk, or the main window was destroyed and
         * we're on our way out of the application
         */
        Tcl_AppendResult(interp, "NULL main window", (char *)NULL);
        return TCL_ERROR;
    }

    needFlags = flags & ~(BLT_CONFIG_USER_BIT - 1);
    if (Tk_Depth(tkwin) <= 1) {
        hateFlags = BLT_CONFIG_COLOR_ONLY;
    } else {
        hateFlags = BLT_CONFIG_MONO_ONLY;
    }

    /*
     * Pass one:  scan through all the option specs, replacing strings
     * with Tk_Uid structs (if this hasn't been done already) and
     * clearing the BLT_CONFIG_OPTION_SPECIFIED flags.
     */

    for (sp = specs; sp->type != BLT_CONFIG_END; sp++) {
        if (!(sp->specFlags & INIT) && (sp->switchName != NULL)) {
            if (sp->dbName != NULL) {
                sp->dbName = Tk_GetUid(sp->dbName);
            }
            if (sp->dbClass != NULL) {
                sp->dbClass = Tk_GetUid(sp->dbClass);
            }
            if (sp->defValue != NULL) {
                sp->defValue = Tk_GetUid(sp->defValue);
            }
        }
        sp->specFlags = (sp->specFlags & ~BLT_CONFIG_OPTION_SPECIFIED) | INIT;
    }

    /*
     * Pass two:  scan through all of the arguments, processing those
     * that match entries in the specs.
     */
    while (objc > 0) {
        sp = FindConfigSpec(interp, specs, objv[0], needFlags, hateFlags);
        if (sp == NULL) {
            return TCL_ERROR;
        }

        /* Process the entry.  */
        if (objc < 2) {
            Tcl_AppendResult(interp, "value for \"", Tcl_GetString(objv[0]),
                    "\" missing", (char *)NULL);
            return TCL_ERROR;
        }
        if (DoConfig(interp, tkwin, sp, objv[1], widgRec, flags) != TCL_OK) {
            char msg[100];

            Blt_FmtString(msg, 100, "\n    (processing \"%.40s\" option)",
                    sp->switchName);
            Tcl_AddErrorInfo(interp, msg);
            return TCL_ERROR;
        }
        sp->specFlags |= BLT_CONFIG_OPTION_SPECIFIED;
        objc -= 2, objv += 2;
    }

    /*
     * Pass three:  scan through all of the specs again;  if no
     * command-line argument matched a spec, then check for info
     * in the option database.  If there was nothing in the
     * database, then use the default.
     */

    if ((flags & BLT_CONFIG_OBJV_ONLY) == 0) {
        Tcl_Obj *objPtr;

        for (sp = specs; sp->type != BLT_CONFIG_END; sp++) {
            if ((sp->specFlags & BLT_CONFIG_OPTION_SPECIFIED) || 
                (sp->switchName == NULL) || (sp->type == BLT_CONFIG_SYNONYM)) {
                continue;
            }
            if (((sp->specFlags & needFlags) != needFlags) || 
                (sp->specFlags & hateFlags)) {
                continue;
            }
            objPtr = NULL;
            if (sp->dbName != NULL) {
                Tk_Uid value;

                /* If a resource name was specified, check if there's also
                 * a value was associated with it.  This overrides the
                 * default value. */
                value = Tk_GetOption(tkwin, sp->dbName, sp->dbClass);
                if (value != NULL) {
                    objPtr = Tcl_NewStringObj(value, -1);
                }
            }

            if (objPtr != NULL) {
                int result;

                Tcl_IncrRefCount(objPtr);
                result = DoConfig(interp, tkwin, sp, objPtr, widgRec, flags);
                Tcl_DecrRefCount(objPtr);
                if (result != TCL_OK) {
                    char msg[200];
    
                    Blt_FmtString(msg, 200, 
                        "\n    (%s \"%.50s\" in widget \"%.50s\")",
                        "database entry for", sp->dbName, Tk_PathName(tkwin));
                    Tcl_AddErrorInfo(interp, msg);
                    return TCL_ERROR;
                }
            } else if ((sp->defValue != NULL) && 
                ((sp->specFlags & BLT_CONFIG_DONT_SET_DEFAULT) == 0)) {
                int result;

                /* No resource value is found, use the default value. */
                objPtr = Tcl_NewStringObj(sp->defValue, -1);
                Tcl_IncrRefCount(objPtr);
                result = DoConfig(interp, tkwin, sp, objPtr, widgRec, flags);
                Tcl_DecrRefCount(objPtr);
                if (result != TCL_OK) {
                    char msg[200];
                    
                    Blt_FmtString(msg, 200, 
                        "\n    (%s \"%.50s\" in widget \"%.50s\")",
                        "default value for", sp->dbName, Tk_PathName(tkwin));
                    Tcl_AddErrorInfo(interp, msg);
                    return TCL_ERROR;
                }
            }
        }
    }
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * Blt_ConfigureInfoFromObj --
 *
 *      Return information about the configuration options
 *      for a window, and their current values.
 *
 * Results:
 *      Always returns TCL_OK.  The interp's result will be modified
 *      hold a description of either a single configuration option
 *      available for "widgRec" via "specs", or all the configuration
 *      options available.  In the "all" case, the result will
 *      available for "widgRec" via "specs".  The result will
 *      be a list, each of whose entries describes one option.
 *      Each entry will itself be a list containing the option's
 *      name for use on command lines, database name, database
 *      class, default value, and current value (empty string
 *      if none).  For options that are synonyms, the list will
 *      contain only two values:  name and synonym name.  If the
 *      "name" argument is non-NULL, then the only information
 *      returned is that for the named argument (i.e. the corresponding
 *      entry in the overall list is returned).
 *
 * Side effects:
 *      None.
 *
 *---------------------------------------------------------------------------
 */

int
Blt_ConfigureInfoFromObj(
    Tcl_Interp *interp,         /* Interpreter for error reporting. */
    Tk_Window tkwin,            /* Window corresponding to widgRec. */
    Blt_ConfigSpec *specs,      /* Describes legal options. */
    char *widgRec,              /* Record whose fields contain current
                                 * values for options. */
    Tcl_Obj *objPtr,            /* If non-NULL, indicates a single option
                                 * whose info is to be returned.  Otherwise
                                 * info is returned for all options. */
    int flags)                  /* Used to specify additional flags
                                 * that must be present in config specs
                                 * for them to be considered. */
{
    Blt_ConfigSpec *sp;
    Tcl_Obj *listObjPtr, *valueObjPtr;
    const char *string;
    int needFlags, hateFlags;

    needFlags = flags & ~(BLT_CONFIG_USER_BIT - 1);
    if (Tk_Depth(tkwin) <= 1) {
        hateFlags = BLT_CONFIG_COLOR_ONLY;
    } else {
        hateFlags = BLT_CONFIG_MONO_ONLY;
    }

    /*
     * If information is only wanted for a single configuration
     * spec, then handle that one spec specially.
     */

    Tcl_SetResult(interp, (char *)NULL, TCL_STATIC);
    if (objPtr != NULL) {
        sp = FindConfigSpec(interp, specs, objPtr, needFlags, hateFlags);
        if (sp == NULL) {
            return TCL_ERROR;
        }
        valueObjPtr =  FormatConfigInfo(interp, tkwin, sp, widgRec);
        Tcl_SetObjResult(interp, valueObjPtr);
        return TCL_OK;
    }

    /*
     * Loop through all the specs, creating a big list with all
     * their information.
     */
    string = NULL;              /* Suppress compiler warning. */
    if (objPtr != NULL) {
        string = Tcl_GetString(objPtr);
    }
    listObjPtr = Tcl_NewListObj(0, (Tcl_Obj **)NULL);
    for (sp = specs; sp->type != BLT_CONFIG_END; sp++) {
        if ((objPtr != NULL) && (sp->switchName != string)) {
            continue;
        }
        if (((sp->specFlags & needFlags) != needFlags) || 
            (sp->specFlags & hateFlags)) {
            continue;
        }
        if (sp->switchName == NULL) {
            continue;
        }
        valueObjPtr = FormatConfigInfo(interp, tkwin, sp, widgRec);
        Tcl_ListObjAppendElement(interp, listObjPtr, valueObjPtr);
    }
    Tcl_SetObjResult(interp, listObjPtr);
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * Blt_ConfigureValueFromObj --
 *
 *      This procedure returns the current value of a configuration
 *      option for a widget.
 *
 * Results:
 *      The return value is a standard TCL completion code (TCL_OK or
 *      TCL_ERROR).  The interp's result will be set to hold either the value
 *      of the option given by objPtr (if TCL_OK is returned) or
 *      an error message (if TCL_ERROR is returned).
 *
 * Side effects:
 *      None.
 *
 *---------------------------------------------------------------------------
 */
int
Blt_ConfigureValueFromObj(
    Tcl_Interp *interp,         /* Interpreter for error reporting. */
    Tk_Window tkwin,            /* Window corresponding to widgRec. */
    Blt_ConfigSpec *specs,      /* Describes legal options. */
    char *widgRec,              /* Record whose fields contain current
                                 * values for options. */
    Tcl_Obj *objPtr,            /* Gives the command-line name for the
                                 * option whose value is to be returned. */
    int flags)                  /* Used to specify additional flags
                                 * that must be present in config specs
                                 * for them to be considered. */
{
    Blt_ConfigSpec *sp;
    int needFlags, hateFlags;

    needFlags = flags & ~(BLT_CONFIG_USER_BIT - 1);
    if (Tk_Depth(tkwin) <= 1) {
        hateFlags = BLT_CONFIG_COLOR_ONLY;
    } else {
        hateFlags = BLT_CONFIG_MONO_ONLY;
    }
    sp = FindConfigSpec(interp, specs, objPtr, needFlags, hateFlags);
    if (sp == NULL) {
        return TCL_ERROR;
    }
    objPtr = FormatConfigValue(interp, tkwin, sp, widgRec);
    Tcl_SetObjResult(interp, objPtr);
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * Blt_FreeOptions --
 *
 *      Free up all resources associated with configuration options.
 *
 * Results:
 *      None.
 *
 * Side effects:
 *      Any resource in widgRec that is controlled by a configuration
 *      option (e.g. a Tk_3DBorder or XColor) is freed in the appropriate
 *      fashion.
 *
 *---------------------------------------------------------------------------
 */
void
Blt_FreeOptions(
    Blt_ConfigSpec *specs,      /* Describes legal options. */
    char *widgRec,              /* Record whose fields contain current
                                 * values for options. */
    Display *display,           /* X display; needed for freeing some
                                 * resources. */
    int needFlags)              /* Used to specify additional flags
                                 * that must be present in config specs
                                 * for them to be considered. */
{
    Blt_ConfigSpec *sp;

    for (sp = specs; sp->type != BLT_CONFIG_END; sp++) {
        char *ptr;

        if ((sp->specFlags & needFlags) != needFlags) {
            continue;
        }
        ptr = widgRec + sp->offset;
        switch (sp->type) {
        case BLT_CONFIG_STRING:
            if (*((char **) ptr) != NULL) {
                Blt_Free(*((char **) ptr));
                *((char **) ptr) = NULL;
            }
            break;

        case BLT_CONFIG_COLOR:
            if (*((XColor **) ptr) != NULL) {
                Tk_FreeColor(*((XColor **) ptr));
                *((XColor **) ptr) = NULL;
            }
            break;

        case BLT_CONFIG_FONT:
            if (*((Blt_Font *) ptr) != NULL) {
                Blt_Font_Free(*((Blt_Font *) ptr));
                *((Blt_Font *) ptr) = NULL;
            }
            break;

        case BLT_CONFIG_TK_FONT:
            if (*((Tk_Font *) ptr) != None) {
                Tk_FreeFont(*((Tk_Font *) ptr));
                *((Tk_Font *) ptr) = NULL;
            }
            break;

        case BLT_CONFIG_BITMAP:
            if (*((Pixmap *) ptr) != None) {
                Tk_FreeBitmap(display, *((Pixmap *) ptr));
                *((Pixmap *) ptr) = None;
            }
            break;

        case BLT_CONFIG_BORDER:
            if (*((Tk_3DBorder *)ptr) != NULL) {
                Tk_Free3DBorder(*((Tk_3DBorder *)ptr));
                *((Tk_3DBorder *)ptr) = NULL;
            }
            break;

        case BLT_CONFIG_CURSOR:
        case BLT_CONFIG_ACTIVE_CURSOR:
            if (*((Tk_Cursor *) ptr) != None) {
                Tk_FreeCursor(display, *((Tk_Cursor *) ptr));
                *((Tk_Cursor *) ptr) = None;
            }
            break;

        case BLT_CONFIG_OBJ:
        case BLT_CONFIG_LISTOBJ:
            if (*(Tcl_Obj **)ptr != NULL) {
                Tcl_DecrRefCount(*(Tcl_Obj **)ptr);
                *(Tcl_Obj **)ptr = NULL;
            }
            break;

        case BLT_CONFIG_LIST:
            if (*((char ***) ptr) != NULL) {
                Tcl_Free((char *)(*((char ***) ptr)));
                *((char ***) ptr) = NULL;
            }
            break;

        case BLT_CONFIG_UID:
            if (*(Blt_Uid *)ptr != NULL) {
                Blt_FreeUid(*(Blt_Uid *)ptr);
                *(Blt_Uid *)ptr = NULL;
            }
            break;

        case BLT_CONFIG_BACKGROUND:
            if (*((Blt_Bg *)ptr) != NULL) {
                Blt_Bg_Free(*((Blt_Bg *)ptr));
                *((Blt_Bg *)ptr) = NULL;
            }
            break;

        case BLT_CONFIG_PAINTBRUSH:
            if (*((Blt_PaintBrush *)ptr) != NULL) {
                Blt_FreeBrush(*((Blt_PaintBrush *)ptr));
                *((Blt_PaintBrush *)ptr) = NULL;
            }
            break;

        case BLT_CONFIG_CUSTOM:
            if ((sp->customPtr->freeProc != NULL) && (*(char **)ptr != NULL)) {
                (*sp->customPtr->freeProc)(sp->customPtr->clientData,
                        display, widgRec, sp->offset);
            }
            break;

        }
    }
}

/*
 *---------------------------------------------------------------------------
 *
 * Blt_ConfigModified --
 *
 *      Given the configuration specifications and one or more option
 *      patterns (terminated by a NULL), indicate if any of the matching
 *      configuration options has been reset.
 *
 * Results:
 *      Returns 1 if one of the options has changed, 0 otherwise.
 *
 *---------------------------------------------------------------------------
 */
int 
Blt_ConfigModified(Blt_ConfigSpec *specs, ...)
{
    Blt_ConfigSpec *sp;
    const char *option;
    va_list args;

    va_start(args, specs);
    while ((option = va_arg(args, const char *)) != NULL) {
        for (sp = specs; sp->type != BLT_CONFIG_END; sp++) {
            if ((Tcl_StringMatch(sp->switchName, option)) &&
                (sp->specFlags & BLT_CONFIG_OPTION_SPECIFIED)) {
                va_end(args);
                return 1;
            }
        }
    }
    va_end(args);
    return 0;
}

/*
 *---------------------------------------------------------------------------
 *
 * Blt_ConfigureComponentFromObj --
 *
 *      Configures a component of a widget.  This is useful for
 *      widgets that have multiple components which aren't uniquely
 *      identified by a Tk_Window. It allows us, for example, set
 *      resources for axes of the graph widget. The graph really has
 *      only one window, but its convenient to specify components in a
 *      hierarchy of options.
 *
 *              *graph.x.logScale yes
 *              *graph.Axis.logScale yes
 *              *graph.temperature.scaleSymbols yes
 *              *graph.Element.scaleSymbols yes
 *
 *      This is really a hack to work around the limitations of the Tk
 *      resource database.  It creates a temporary window, needed to
 *      call Tk_ConfigureWidget, using the name of the component.
 *
 * Results:
 *      A standard TCL result.
 *
 * Side Effects:
 *      A temporary window is created merely to pass to Tk_ConfigureWidget.
 *
 *---------------------------------------------------------------------------
 */
int
Blt_ConfigureComponentFromObj(
    Tcl_Interp *interp,
    Tk_Window parent,                   /* Window to associate with
                                         * component */
    const char *name,                   /* Name of component */
    const char *className,
    Blt_ConfigSpec *sp,
    int objc,
    Tcl_Obj *const *objv,
    char *widgRec,
    int flags)
{
    Tk_Window tkwin;
    int result;
    char *tmpName;
    Tcl_Obj *nameObjPtr;

    nameObjPtr = Tcl_NewStringObj(name, -1);
    tmpName = Tcl_GetString(nameObjPtr);
    /* Window name can't start with an upper case letter */
    tmpName[0] = tolower(name[0]);
    
    /*
     * Create a child window by the component's name. If one already
     * exists, create a temporary name.
     */
    tkwin = Blt_FindChild(parent, tmpName);
    if (tkwin != NULL) {
        Tcl_AppendToObj(nameObjPtr, "-temp", 5);
    }
    Tcl_IncrRefCount(nameObjPtr);
    tmpName = Tcl_GetString(nameObjPtr);
    tkwin = Tk_CreateWindow(interp, parent, tmpName, (char *)NULL);
    if (tkwin == NULL) {
        Tcl_AppendResult(interp, "can't create temporary window \"",
                tmpName, "\" in \"", Tk_PathName(parent), "\"", 
                (char *)NULL);
        Tcl_DecrRefCount(nameObjPtr);
        return TCL_ERROR;
    }
    assert(Tk_Depth(tkwin) == Tk_Depth(parent));
    Tcl_DecrRefCount(nameObjPtr);

    Tk_SetClass(tkwin, className);
    result = Blt_ConfigureWidgetFromObj(interp, tkwin, sp, objc, objv, 
        widgRec, flags);
    Tk_DestroyWindow(tkwin);
    return result;
}

/*
 *---------------------------------------------------------------------------
 *
 * Blt_ObjIsOption --
 *
 *      Indicates whether objPtr is a valid configuration option 
 *      such as -background.
 *
 * Results:
 *      Returns 1 is a matching option is found and 0 otherwise.
 *
 *---------------------------------------------------------------------------
 */
int
Blt_ObjIsOption(
    Blt_ConfigSpec *specs,      /* Describes legal options. */
    Tcl_Obj *objPtr,            /* Command-line option name. */
    int flags)                  /* Used to specify additional flags
                                 * that must be present in config specs
                                 * for them to be considered.  Also,
                                 * may have BLT_CONFIG_OBJV_ONLY set. */
{
    Blt_ConfigSpec *sp;
    int needFlags;              /* Specs must contain this set of flags
                                 * or else they are not considered. */

    needFlags = flags & ~(BLT_CONFIG_USER_BIT - 1);
    sp = FindConfigSpec((Tcl_Interp *)NULL, specs, objPtr, needFlags, 0);
    return (sp != NULL);
}
