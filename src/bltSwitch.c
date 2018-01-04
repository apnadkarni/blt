/* -*- mode: c; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 * bltSwitch.c --
 *
 * This module implements command/argument switch parsing procedures for
 * the BLT toolkit.
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

#define BUILD_BLT_TCL_PROCS 1
#include "bltInt.h"

#ifdef HAVE_STRING_H
  #include <string.h>
#endif /* HAVE_STRING_H */

#ifdef HAVE_STDARG_H
  #include <stdarg.h>
#endif /* HAVE_STDARG_H */

#include "bltAlloc.h"
#include "bltSwitch.h"



int 
Blt_ExprDoubleFromObj(Tcl_Interp *interp, Tcl_Obj *objPtr, double *valuePtr)
{
    /* First try to extract the value as a double precision number. */
    if (Tcl_GetDoubleFromObj((Tcl_Interp *)NULL, objPtr, valuePtr) == TCL_OK) {
        return TCL_OK;
    }
    /* Then try to parse it as an expression. */
    if (Tcl_ExprDouble(interp, Tcl_GetString(objPtr), valuePtr) == TCL_OK) {
        return TCL_OK;
    }
    return TCL_ERROR;
}

int 
Blt_ExprIntFromObj(Tcl_Interp *interp, Tcl_Obj *objPtr, int *valuePtr)
{
    long lvalue;

    /* First try to extract the value as a simple integer. */
    if (Tcl_GetIntFromObj((Tcl_Interp *)NULL, objPtr, valuePtr) == TCL_OK) {
        return TCL_OK;
    }
    /* Otherwise try to parse it as an expression. */
    if (Tcl_ExprLong(interp, Tcl_GetString(objPtr), &lvalue) == TCL_OK) {
        *valuePtr = lvalue;
        return TCL_OK;
    }
    return TCL_ERROR;
}

int
Blt_GetStateFromObj(Tcl_Interp *interp, Tcl_Obj *objPtr, int *statePtr)
{
    char c;
    const char *string;
    int length;

    string = Tcl_GetStringFromObj(objPtr, &length);
    c = string[0];
    if ((c == 'n') && (strncmp(string, "normal", length) == 0)) {
        *statePtr = STATE_NORMAL;
    } else if ((c == 'd') && (strncmp(string, "disabled", length) == 0)) {
        *statePtr = STATE_DISABLED;
    } else if ((c == 'a') && (strncmp(string, "active", length) == 0)) {
        *statePtr = STATE_ACTIVE;
    } else {
        Tcl_AppendResult(interp, "bad state \"", string,
            "\": should be normal, active, or disabled", (char *)NULL);
        return TCL_ERROR;
    }
    return TCL_OK;
}

const char *
Blt_NameOfState(int state)
{
    switch (state) {
    case STATE_ACTIVE:
        return "active";
    case STATE_DISABLED:
        return "disabled";
    case STATE_NORMAL:
        return "normal";
    default:
        return "???";
    }
}

/*
 *---------------------------------------------------------------------------
 *
 * Blt_NameOfFill --
 *
 *      Converts the integer representing the fill style into a string.
 *
 *---------------------------------------------------------------------------
 */
const char *
Blt_NameOfFill(int fill)
{
    switch (fill) {
    case FILL_X:
        return "x";
    case FILL_Y:
        return "y";
    case FILL_NONE:
        return "none";
    case FILL_BOTH:
        return "both";
    default:
        return "unknown value";
    }
}

/*
 *---------------------------------------------------------------------------
 *
 * Blt_GetFillFromObj --
 *
 *      Converts the fill style string into its numeric representation.
 *
 *      Valid style strings are:
 *
 *        "none"   Use neither plane.
 *        "x"      X-coordinate plane.
 *        "y"      Y-coordinate plane.
 *        "both"   Use both coordinate planes.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
int
Blt_GetFillFromObj(Tcl_Interp *interp, Tcl_Obj *objPtr, int *fillPtr)
{
    char c;
    const char *string;
    int length;

    string = Tcl_GetStringFromObj(objPtr, &length);
    c = string[0];
    if ((c == 'n') && (strncmp(string, "none", length) == 0)) {
        *fillPtr = FILL_NONE;
    } else if ((c == 'x') && (strncmp(string, "x", length) == 0)) {
        *fillPtr = FILL_X;
    } else if ((c == 'y') && (strncmp(string, "y", length) == 0)) {
        *fillPtr = FILL_Y;
    } else if ((c == 'b') && (strncmp(string, "both", length) == 0)) {
        *fillPtr = FILL_BOTH;
    } else {
        Tcl_AppendResult(interp, "bad argument \"", string,
            "\": should be \"none\", \"x\", \"y\", or \"both\"", (char *)NULL);
        return TCL_ERROR;
    }
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * Blt_NameOfResize --
 *
 *      Converts the resize value into its string representation.
 *
 * Results:
 *      Returns a pointer to the static name string.
 *
 *---------------------------------------------------------------------------
 */
const char *
Blt_NameOfResize(int resize)
{
    switch (resize & RESIZE_BOTH) {
    case RESIZE_NONE:
        return "none";
    case RESIZE_EXPAND:
        return "expand";
    case RESIZE_SHRINK:
        return "shrink";
    case RESIZE_BOTH:
        return "both";
    default:
        return "unknown resize value";
    }
}

/*
 *---------------------------------------------------------------------------
 *
 * Blt_GetResizeFromObj --
 *
 *      Converts the resize string into its numeric representation.
 *
 *      Valid style strings are:
 *
 *        "none"   
 *        "expand" 
 *        "shrink" 
 *        "both"   
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
int
Blt_GetResizeFromObj(Tcl_Interp *interp, Tcl_Obj *objPtr, int *resizePtr)
{
    char c;
    const char *string;
    int length;

    string = Tcl_GetStringFromObj(objPtr, &length);
    c = string[0];
    if ((c == 'n') && (strncmp(string, "none", length) == 0)) {
        *resizePtr = RESIZE_NONE;
    } else if ((c == 'b') && (strncmp(string, "both", length) == 0)) {
        *resizePtr = RESIZE_BOTH;
    } else if ((c == 'e') && (strncmp(string, "expand", length) == 0)) {
        *resizePtr = RESIZE_EXPAND;
    } else if ((c == 's') && (strncmp(string, "shrink", length) == 0)) {
        *resizePtr = RESIZE_SHRINK;
    } else {
        Tcl_AppendResult(interp, "bad resize argument \"", string,
            "\": should be \"none\", \"expand\", \"shrink\", or \"both\"",
            (char *)NULL);
        return TCL_ERROR;
    }
    return TCL_OK;
}


const char *
Blt_NameOfSide(int side)
{
    switch (side) {
    case SIDE_LEFT:
        return "left";
    case SIDE_RIGHT:
        return "right";
    case SIDE_BOTTOM:
        return "bottom";
    case SIDE_TOP:
        return "top";
    }
    return "unknown side value";
}

/*
 *---------------------------------------------------------------------------
 *
 * Blt_GetSideFromObj --
 *
 *      Converts the fill style string into its numeric representation.
 *
 *      Valid style strings are "left", "right", "top", or  "bottom".
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED */
int
Blt_GetSideFromObj(
    Tcl_Interp *interp,                 /* Interpreter to send results back
                                         * to */
    Tcl_Obj *objPtr,                    /* Value string */
    int *sidePtr)                       /* (out) Token representing side:
                                         * either SIDE_LEFT, SIDE_RIGHT,
                                         * SIDE_TOP, or SIDE_BOTTOM. */
{
    char c;
    const char *string;
    int length;

    string = Tcl_GetStringFromObj(objPtr, &length);
    c = string[0];
    if ((c == 'l') && (strncmp(string, "left", length) == 0)) {
        *sidePtr = SIDE_LEFT;
    } else if ((c == 'r') && (strncmp(string, "right", length) == 0)) {
        *sidePtr = SIDE_RIGHT;
    } else if ((c == 't') && (strncmp(string, "top", length) == 0)) {
        *sidePtr = SIDE_TOP;
    } else if ((c == 'b') && (strncmp(string, "bottom", length) == 0)) {
        *sidePtr = SIDE_BOTTOM;
    } else {
        Tcl_AppendResult(interp, "bad side \"", string,
            "\": should be left, right, top, or bottom", (char *)NULL);
        return TCL_ERROR;
    }
    return TCL_OK;
}

int
Blt_GetCount(Tcl_Interp *interp, const char *string, int check,
             long *valuePtr)
{
    long lvalue;

    if (Blt_GetLong(interp, string, &lvalue) != TCL_OK) {
        return TCL_ERROR;
    }
    if (lvalue < 0) {
        if (interp != NULL) {
            Tcl_AppendResult(interp, "bad value \"", string, 
                             "\": can't be negative", (char *)NULL);
        }
        return TCL_ERROR;
    }
    if (check == COUNT_POS) {
        if (lvalue <= 0) {
            if (interp != NULL) {
                Tcl_AppendResult(interp, "bad value \"", string, 
                                 "\": must be positive", (char *)NULL);
            }
            return TCL_ERROR;
        }
    }
    *valuePtr = lvalue;
    return TCL_OK;
}

int
Blt_GetCountFromObj(
    Tcl_Interp *interp,
    Tcl_Obj *objPtr,
    int check,                          /* Can be COUNT_POS or COUNT_NNEG */
    long *valuePtr)
{
    long lvalue;

    if (Blt_GetLongFromObj(interp, objPtr, &lvalue) != TCL_OK) {
        return TCL_ERROR;
    }
    if (lvalue < 0) {
        if (interp != NULL) {
            Tcl_AppendResult(interp, "bad value \"", Tcl_GetString(objPtr), 
                             "\": can't be negative", (char *)NULL);
        }
        return TCL_ERROR;
    }
    if (check == COUNT_POS) {
        if (lvalue <= 0) {
            if (interp != NULL) {
                Tcl_AppendResult(interp, "bad value \"", Tcl_GetString(objPtr), 
                                 "\": must be positive", (char *)NULL);
            }
            return TCL_ERROR;
        }
    }
    *valuePtr = lvalue;
    return TCL_OK;
}

static void
DoHelp(Tcl_Interp *interp, Blt_SwitchSpec *specs)
{
    Tcl_DString ds;
    Blt_SwitchSpec *sp;

    Tcl_DStringInit(&ds);
    Tcl_DStringAppend(&ds, "The following switches are available:", -1);
    for (sp = specs; sp->type != BLT_SWITCH_END; sp++) {
            Tcl_DStringAppend(&ds, "\n    ", 4);
            Tcl_DStringAppend(&ds, sp->switchName, -1);
            if (sp->help != NULL) {
                Tcl_DStringAppend(&ds, " ", 1);
                Tcl_DStringAppend(&ds, sp->help, -1);
            }
    }
    Tcl_AppendResult(interp, Tcl_DStringValue(&ds), (char *)NULL);
    Tcl_DStringFree(&ds);
}

/*
 *---------------------------------------------------------------------------
 *
 * FormatSwitchValue --
 *
 *      This procedure formats the current value of a configuration option.
 *
 * Results:
 *      The return value is the formatted value of the option given by
 *      specPtr and record.  If the value is static, so that it need not be
 *      freed, *freeProcPtr will be set to NULL; otherwise *freeProcPtr
 *      will be set to the address of a procedure to free the result, and
 *      the caller must invoke this procedure when it is finished with the
 *      result.
 *
 * Side effects:
 *      None.
 *
 *---------------------------------------------------------------------------
 */
static Tcl_Obj *
FormatSwitchValue(
    Tcl_Interp *interp,                 /* Interpreter for use in real
                                         * conversions. */
    Blt_SwitchSpec *sp,                 /* Pointer to information describing
                                         * option.  Must not point to a
                                         * synonym option. */
    void *record)                       /* Pointer to record holding current
                                         * values of info for widget. */
{
    char *ptr;
    const char *string;

    ptr = (char *)record + sp->offset;
    string = "";
    switch (sp->type) {
    case BLT_SWITCH_BITS:
    case BLT_SWITCH_BITS_NOARG:
        {
            uintptr_t flag;

            flag = (*(uintptr_t *)ptr) & (uintptr_t)sp->customPtr;
            return Tcl_NewBooleanObj((flag != 0));
        }

    case BLT_SWITCH_INVERT_BITS:
    case BLT_SWITCH_INVERT_BITS_NOARG:
        {
            uintptr_t flag;

            flag = (*(uintptr_t *)ptr) & (uintptr_t)sp->customPtr;
            return Tcl_NewBooleanObj((flag == 0));
        }

    case BLT_SWITCH_BOOLEAN: 
    case BLT_SWITCH_BOOLEAN_NOARG: 
        return Tcl_NewBooleanObj(*(int *)ptr);

    case BLT_SWITCH_DOUBLE: 
        return Tcl_NewDoubleObj(*(double *)ptr);

    case BLT_SWITCH_OBJ:
    case BLT_SWITCH_LISTOBJ:
        if (*(Tcl_Obj **)ptr != NULL) {
            return *(Tcl_Obj **)ptr;
        }
        break;

    case BLT_SWITCH_FLOAT: 
        {
            double x = *(float *)ptr;
            return Tcl_NewDoubleObj(x);
        }

    case BLT_SWITCH_INT: 
        return Tcl_NewIntObj(*(int *)ptr);

    case BLT_SWITCH_INT_NNEG:
    case BLT_SWITCH_INT_POS:
        return Tcl_NewIntObj(*(int *)ptr);

    case BLT_SWITCH_LIST: 
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

    case BLT_SWITCH_LONG: 
        return Tcl_NewLongObj(*(long *)ptr);

    case BLT_SWITCH_LONG_NNEG:
    case BLT_SWITCH_LONG_POS:
        return Tcl_NewLongObj(*(long *)ptr);

    case BLT_SWITCH_STRING: 
        if (*(char **)ptr != NULL) {
            string = *(char **)ptr;
        }
        break;

    case BLT_SWITCH_SIDE: 
        if (*(char **)ptr != NULL) {
            string = Blt_NameOfSide(*(int *)ptr);
        }
        break;

    case BLT_SWITCH_CUSTOM:
        return (*sp->customPtr->printProc)
                (sp->customPtr->clientData, interp, record, sp->offset, 
                        sp->flags);

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
 *      Create a valid TCL list holding the configuration information for a
 *      single configuration option.
 *
 * Results:
 *      A TCL list, dynamically allocated.  The caller is expected to arrange
 *      for this list to be freed eventually.
 *
 * Side effects:
 *      Memory is allocated.
 *
 *---------------------------------------------------------------------------
 */
static Tcl_Obj *
FormatSwitchInfo(
    Tcl_Interp *interp,                 /* Interpreter to use for things
                                         * like floating-point precision. */
    Blt_SwitchSpec *sp,                 /* Pointer to information describing
                                         * option. */
    char *record)                       /* Pointer to record holding current
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
    if (sp->defValue != NULL) {
        Tcl_ListObjAppendElement(interp, listObjPtr, 
                Tcl_NewStringObj(sp->defValue, -1));
    } else {
        Tcl_ListObjAppendElement(interp, listObjPtr, Tcl_NewStringObj("", -1));
    }
    Tcl_ListObjAppendElement(interp, listObjPtr, 
        FormatSwitchValue(interp, sp, record));
    return listObjPtr;
}

/*
 *---------------------------------------------------------------------------
 *
 * FindSwitchSpec --
 *
 *      Search through a table of configuration specs, looking for one that
 *      matches a given argvName.
 *
 * Results:
 *      The return value is a pointer to the matching entry, or NULL if
 *      nothing matched.  In that case an error message is left in the
 *      interp's result.
 *
 * Side effects:
 *      None.
 *
 *---------------------------------------------------------------------------
 */
static Blt_SwitchSpec *
FindSwitchSpec(
    Tcl_Interp *interp,                 /* Used for reporting errors. */
    Blt_SwitchSpec *specs,              /* Pointer to table of
                                         * configuration specifications for
                                         * a widget. */
    Tcl_Obj *objPtr,                    /* Name identifying a particular
                                         * switch. */
    int needFlags,                      /* Flags that must be present in
                                         * matching entry. */
    int hateFlags)                      /* Flags that must NOT be present
                                         * in matching entry. */
{
    Blt_SwitchSpec *sp;
    char c;                             /* First character of current
                                         * argument. */
    Blt_SwitchSpec *matchPtr;           /* Matching spec, or NULL. */
    const char *name;
    int length;                         /* Length of name. */

    name = Tcl_GetStringFromObj(objPtr, &length);
    c = name[1];
    matchPtr = NULL;
    for (sp = specs; sp->type != BLT_SWITCH_END; sp++) {
        if (sp->switchName == NULL) {
            continue;
        }
        if (((sp->flags & needFlags) != needFlags) || (sp->flags & hateFlags)) {
            continue;
        }
        if ((sp->switchName[1] != c) || 
            (strncmp(sp->switchName, name, length) != 0)) {
            continue;
        }
        if (sp->switchName[length] == '\0') {
            return sp;          /* Stop on a perfect match. */
        }
        if (matchPtr != NULL) {
            Tcl_AppendResult(interp, "ambiguous switch \"", name, "\"\n", 
                (char *) NULL);
            DoHelp(interp, specs);
            return NULL;
        }
        matchPtr = sp;
    }
    if (strcmp(name, "-help") == 0) {
        DoHelp(interp, specs);
        return NULL;
    }
    if (matchPtr == NULL) {
        Tcl_AppendResult(interp, "unknown switch \"", name, "\"\n", 
                         (char *)NULL);
        DoHelp(interp, specs);
        return NULL;
    }
    return matchPtr;
}

/*
 *---------------------------------------------------------------------------
 *
 * DoSwitch --
 *
 *      This procedure applies a single configuration switch to a widget
 *      record.
 *
 * Results:
 *      A standard TCL return value.
 *
 * Side effects:
 *      WidgRec is modified as indicated by specPtr and value.  The old
 *      value is recycled, if that is appropriate for the value type.
 *
 *---------------------------------------------------------------------------
 */
static int
DoSwitch(
    Tcl_Interp *interp,                 /* Interpreter for error
                                         * reporting. */
    Blt_SwitchSpec *sp,                 /* Specifier to apply. */
    Tcl_Obj *objPtr,                    /* Value to use to fill in
                                         * record. */
    void *record)                       /* Record whose fields are to be
                                         * modified.  Values must be
                                         * properly initialized. */
{
    int objIsEmpty;

    objIsEmpty = FALSE;
    if (objPtr == NULL) {
        objIsEmpty = TRUE;
    } else if (sp->flags & BLT_CONFIG_NULL_OK) {
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

        ptr = (char *)record + sp->offset;
        switch (sp->type) {

        case BLT_SWITCH_BITS: 
            {
                int bool;
                uintptr_t mask, flags;

                if (Tcl_GetBooleanFromObj(interp, objPtr, &bool) != TCL_OK) {
                    return TCL_ERROR;
                }
                mask = (uintptr_t)sp->customPtr;
                flags = *(uintptr_t *)ptr;
                flags &= ~mask;
                if (bool) {
                    flags |= mask;
                }
                *(int *)ptr = flags;
            }
            break;

        case BLT_SWITCH_INVERT_BITS: 
            {
                int bool;
                uintptr_t mask, flags;

                if (Tcl_GetBooleanFromObj(interp, objPtr, &bool) != TCL_OK) {
                    return TCL_ERROR;
                }
                mask = (uintptr_t)sp->customPtr;
                flags = *(uintptr_t *)ptr;
                flags &= ~mask;
                if (!bool) {
                    flags |= mask;
                }
                *(int *)ptr = flags;
            }
            break;

        case BLT_SWITCH_BOOLEAN:
            {
                int bool;

                if (Tcl_GetBooleanFromObj(interp, objPtr, &bool) != TCL_OK) {
                    return TCL_ERROR;
                }
                if (sp->mask > 0) {
                    if (bool) {
                        *((int *)ptr) |= sp->mask;
                    } else {
                        *((int *)ptr) &= ~sp->mask;
                    }
                } else {
                    *((int *)ptr) = bool;
                }
            }
            break;

        case BLT_SWITCH_DOUBLE:
            if (Tcl_GetDoubleFromObj(interp, objPtr, (double *)ptr) != TCL_OK) {
                return TCL_ERROR;
            }
            break;

        case BLT_SWITCH_OBJ:
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

        case BLT_SWITCH_FLOAT:
            {
                double value;

                if (Tcl_GetDoubleFromObj(interp, objPtr, &value) != TCL_OK) {
                    return TCL_ERROR;
                }
                *(float *)ptr = (float)value;
            }
            break;

        case BLT_SWITCH_INT:
            if (Tcl_GetIntFromObj(interp, objPtr, (int *)ptr) != TCL_OK) {
                return TCL_ERROR;
            }
            break;

        case BLT_SWITCH_INT_NNEG:
            {
                long value;
                
                if (Blt_GetCountFromObj(interp, objPtr, COUNT_NNEG, 
                        &value) != TCL_OK) {
                    return TCL_ERROR;
                }
                *(int *)ptr = (int)value;
            }
            break;

        case BLT_SWITCH_INT_POS:
            {
                long value;
                
                if (Blt_GetCountFromObj(interp, objPtr, COUNT_POS, 
                        &value) != TCL_OK) {
                    return TCL_ERROR;
                }
                *(int *)ptr = (int)value;
            }
            break;

        case BLT_SWITCH_LIST:
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

        case BLT_SWITCH_LISTOBJ: 
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

        case BLT_SWITCH_INT64:
            {
                int64_t value;
                
                if (Blt_GetInt64FromObj(interp, objPtr, &value) != TCL_OK) {
                    return TCL_ERROR;
                }
                *(int64_t *)ptr = value;
            }
            break;

        case BLT_SWITCH_LONG:
            {
                int64_t value;
                
                if (Blt_GetInt64FromObj(interp, objPtr, &value) != TCL_OK) {
                    return TCL_ERROR;
                }
                *(long *)ptr = value;
            }
            break;
        case BLT_SWITCH_LONG_NNEG:
            {
                long value;
                
                if (Blt_GetCountFromObj(interp, objPtr, COUNT_NNEG, 
                        &value) != TCL_OK) {
                    return TCL_ERROR;
                }
                *(long *)ptr = value;
            }
            break;

        case BLT_SWITCH_LONG_POS:
            {
                long value;
                
                if (Blt_GetCountFromObj(interp, objPtr, COUNT_POS, &value)
                        != TCL_OK) {
                    return TCL_ERROR;
                }
                *(long *)ptr = value;
            }
            break;

        case BLT_SWITCH_STRING: 
            {
                const char *value;
                
                value = Tcl_GetString(objPtr);
                value =  (*value == '\0') ?  NULL : Blt_AssertStrdup(value);
                if (*(char **)ptr != NULL) {
                    Blt_Free(*(char **)ptr);
                }
                *(const char **)ptr = value;
            }
#ifdef notdef
            {
                char *old, *new, **strPtr;
                char *string;

                string = Tcl_GetString(objPtr);
                strPtr = (char **)ptr;
                new = ((*string == '\0') && (sp->flags & BLT_SWITCH_NULL_OK))
                    ? NULL : Blt_AssertStrdup(string);
                old = *strPtr;
                if (old != NULL) {
                    Blt_Free(old);
                }
                *strPtr = new;
            }
#endif
            break;

        case BLT_SWITCH_SIDE:
            if (Blt_GetSideFromObj(interp, objPtr, (int *)ptr) != TCL_OK) {
                return TCL_ERROR;
            }
            break;

        case BLT_SWITCH_CUSTOM:
            assert(sp->customPtr != NULL);
            if ((*sp->customPtr->parseProc)(sp->customPtr->clientData, interp,
                sp->switchName, objPtr, (char *)record, sp->offset, sp->flags) 
                != TCL_OK) {
                return TCL_ERROR;
            }
            break;

        default: 
            Tcl_AppendResult(interp, "bad switch table: unknown type \"",
                 Blt_Itoa(sp->type), "\"", (char *)NULL);
            return TCL_ERROR;
        }
        sp++;
    } while ((sp->switchName == NULL) && (sp->type != BLT_SWITCH_END));
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * Blt_ParseSwitches --
 *
 *      Process command-line switches to fill in fields of a record with
 *      resources and other parameters.
 *
 * Results:
 *      Returns the number of arguments comsumed by parsing the command
 *      line.  If an error occurred, -1 will be returned and an error
 *      messages can be found as the interpreter result.
 *
 * Side effects:
 *      The fields of record get filled in with information from argc/argv.
 *      Old information in record's fields gets recycled.
 *
 *---------------------------------------------------------------------------
 */
int
Blt_ParseSwitches(
    Tcl_Interp *interp,                 /* Interpreter for error reporting. */
    Blt_SwitchSpec *specs,              /* Describes legal switches. */
    int objc,                           /* Number of elements in argv. */
    Tcl_Obj *const *objv,               /* Command-line switches. */
    void *record,                       /* Record whose fields are to be
                                         * modified.  Values may or may not be
                                         * initialized. */
    int flags)                          /* Used to specify additional flags
                                         * that must be present in switch
                                         * specs for them to be considered. */
{
    Blt_SwitchSpec *sp;
    int count;
    int needFlags;                      /* Specs must contain this set of
                                         * flags or else they are not
                                         * considered. */
    int hateFlags;                      /* If a spec contains any bits here,
                                         * it's not considered. */

    needFlags = flags & ~(BLT_SWITCH_USER_BIT - 1);
    hateFlags = 0;

    /*
     * Pass 1:  Clear the change flags on all the specs so that we 
     *          can check it later.
     */
    for (sp = specs; sp->type != BLT_SWITCH_END; sp++) {
        sp->flags &= ~BLT_SWITCH_SPECIFIED;
    }
    /*
     * Pass 2:  Process the arguments that match entries in the specs.
     *          It's an error if the argument doesn't match anything.
     */
    for (count = 0; count < objc; count++) {
        char *arg;
        int length;

        arg = Tcl_GetStringFromObj(objv[count], &length);
        if (flags & BLT_SWITCH_OBJV_PARTIAL) {
            /* 
             * If the argument doesn't start with a '-' (not a switch) or
             * is '--', stop processing and return the number of arguments
             * comsumed.
             */
            if (arg[0] != '-') {
                return count;
            }
            if ((arg[1] == '-') && (arg[2] == '\0')) {
                return count + 1; /* include the "--" in the count. */
            }
        }
        sp = FindSwitchSpec(interp, specs, objv[count], needFlags, hateFlags);
        if (sp == NULL) {
            return -1;
        }
        if (sp->type == BLT_SWITCH_BITS_NOARG) {
            char *ptr;

            ptr = (char *)record + sp->offset;
            *((int *)ptr) |= sp->mask;
        } else if (sp->type == BLT_SWITCH_BOOLEAN_NOARG) {
            char *ptr;

            ptr = (char *)record + sp->offset;
            *((int *)ptr) = TRUE;
        } else if (sp->type == BLT_SWITCH_INVERT_BITS_NOARG) {
            char *ptr;
            
            ptr = (char *)record + sp->offset;
            *((int *)ptr) &= ~sp->mask;
        } else if (sp->type == BLT_SWITCH_VALUE) {
            char *ptr;
            
            ptr = (char *)record + sp->offset;
            *((int *)ptr) = sp->mask;
        } else {
            count++;
            if (count == objc) {
                Tcl_AppendResult(interp, "value for \"", arg, "\" missing", 
                                 (char *) NULL);
                return -1;
            }
            if (DoSwitch(interp, sp, objv[count], record) != TCL_OK) {
                char msg[200];

                Blt_FmtString(msg, 200, "\n    (processing \"%.40s\" switch)", 
                        sp->switchName);
                Tcl_AddErrorInfo(interp, msg);
                return -1;
            }
        }
        sp->flags |= BLT_SWITCH_SPECIFIED;
    }

    /*
     * Pass 3: Scan through all of the specs again; if no command-line
     *         argument matched a spec, then check for info in the option
     *         database.  If there was nothing in the database, then use
     *         the default.
     */
    if (flags & BLT_SWITCH_INITIALIZE) {
        Tcl_Obj *objPtr;

        for (sp = specs; sp->type != BLT_SWITCH_END; sp++) {
            if ((sp->flags & BLT_SWITCH_SPECIFIED) || 
                (sp->switchName == NULL)) {
                continue;
            }
            if (((sp->flags & needFlags) != needFlags) || 
                (sp->flags & hateFlags)) {
                continue;
            }
            if ((sp->defValue != NULL) && 
                ((sp->flags & BLT_SWITCH_DONT_SET_DEFAULT) == 0)) {
                int result;

                /* No resource value is found, use the default value. */
                objPtr = Tcl_NewStringObj(sp->defValue, -1);
                Tcl_IncrRefCount(objPtr);
                result = DoSwitch(interp, sp, objPtr, record);
                Tcl_DecrRefCount(objPtr);
                if (result != TCL_OK) {
                    char msg[200];
                    
                    Blt_FmtString(msg, 200, "\n    (processing \"%.40s\" switch)", 
                              sp->switchName);
                    Tcl_AddErrorInfo(interp, msg);
                    return -1;
                }
            }
        }
    }
    return count;
}

/*
 *---------------------------------------------------------------------------
 *
 * Blt_SwitchInfo --
 *
 *      Return information about the configuration options for a window,
 *      and their current values.
 *
 * Results:
 *      Always returns TCL_OK.  The interp's result will be modified hold a
 *      description of either a single configuration option available for
 *      "record" via "specs", or all the configuration options available.
 *      In the "all" case, the result will available for "record" via
 *      "specs".  The result will be a list, each of whose entries
 *      describes one option.  Each entry will itself be a list containing
 *      the option's name for use on command lines, database name, database
 *      class, default value, and current value (empty string if none).
 *      For options that are synonyms, the list will contain only two
 *      values: name and synonym name.  If the "name" argument is non-NULL,
 *      then the only information returned is that for the named argument
 *      (i.e. the corresponding entry in the overall list is returned).
 *
 * Side effects:
 *      None.
 *
 *---------------------------------------------------------------------------
 */
int
Blt_SwitchInfo(
    Tcl_Interp *interp,                 /* Interpreter for error
                                         * reporting. */
    Blt_SwitchSpec *specs,              /* Describes legal options. */
    void *record,                       /* Record whose fields contain
                                         * current values for options. */
    Tcl_Obj *objPtr,                    /* If non-NULL, indicates a single
                                         * option whose info is to be
                                         * returned.  Otherwise info is
                                         * returned for all options. */
    int flags)                          /* Used to specify additional flags
                                         * that must be present in config
                                         * specs for them to be
                                         * considered. */
{
    Blt_SwitchSpec *sp;
    Tcl_Obj *listObjPtr, *valueObjPtr;
    const char *string;
    int needFlags, hateFlags;

    needFlags = flags & ~(BLT_SWITCH_USER_BIT - 1);
    hateFlags = 0;

    /*
     * If information is only wanted for a single configuration spec, then
     * handle that one spec specially.
     */
    Tcl_SetResult(interp, (char *)NULL, TCL_STATIC);
    if (objPtr != NULL) {
        sp = FindSwitchSpec(interp, specs, objPtr, needFlags, hateFlags);
        if (sp == NULL) {
            return TCL_ERROR;
        }
        valueObjPtr = FormatSwitchInfo(interp, sp, record);
        Tcl_SetObjResult(interp, valueObjPtr);
        return TCL_OK;
    }

    /*
     * Loop through all the specs, creating a big list with all their
     * information.
     */
    string = NULL;                      /* Suppress compiler warning. */
    if (objPtr != NULL) {
        string = Tcl_GetString(objPtr);
    }
    listObjPtr = Tcl_NewListObj(0, (Tcl_Obj **)NULL);
    for (sp = specs; sp->type != BLT_SWITCH_END; sp++) {
        if ((objPtr != NULL) && (sp->switchName != string)) {
            continue;
        }
        if (((sp->flags & needFlags) != needFlags) || 
            (sp->flags & hateFlags)) {
            continue;
        }
        if (sp->switchName == NULL) {
            continue;
        }
        valueObjPtr = FormatSwitchInfo(interp, sp, record);
        Tcl_ListObjAppendElement(interp, listObjPtr, valueObjPtr);
    }
    Tcl_SetObjResult(interp, listObjPtr);
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * Blt_SwitchValue --
 *
 *      This procedure returns the current value of a configuration option for
 *      a widget.
 *
 * Results:
 *      The return value is a standard TCL completion code (TCL_OK or
 *      TCL_ERROR).  The interp's result will be set to hold either the value
 *      of the option given by objPtr (if TCL_OK is returned) or an error
 *      message (if TCL_ERROR is returned).
 *
 * Side effects:
 *      None.
 *
 *---------------------------------------------------------------------------
 */
int
Blt_SwitchValue(
    Tcl_Interp *interp,                 /* Interpreter for error reporting. */
    Blt_SwitchSpec *specs,              /* Describes legal options. */
    void *record,                       /* Record whose fields contain current
                                         * values for options. */
    Tcl_Obj *objPtr,                    /* Gives the command-line name for the
                                         * option whose value is to be
                                         * returned. */
    int flags)                          /* Used to specify additional flags
                                         * that must be present in config
                                         * specs * for them to be
                                         * considered. */
{
    Blt_SwitchSpec *sp;
    int needFlags, hateFlags;

    needFlags = flags & ~(BLT_SWITCH_USER_BIT - 1);
    hateFlags = 0;

    sp = FindSwitchSpec(interp, specs, objPtr, needFlags, hateFlags);
    if (sp == NULL) {
        return TCL_ERROR;
    }
    objPtr = FormatSwitchValue(interp, sp, record);
    Tcl_SetObjResult(interp, objPtr);
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * Blt_FreeSwitches --
 *
 *      Free up all resources associated with switches.
 *
 * Results:
 *      None.
 *
 *---------------------------------------------------------------------------
 */
/* ARGSUSED */
void
Blt_FreeSwitches(
    Blt_SwitchSpec *specs,              /* Describes legal switches. */
    void *record,                       /* Record whose fields contain current
                                         * values for switches. */
    int needFlags)                      /* Used to specify additional flags
                                         * that must be present in config
                                         * specs for them to be considered. */
{
    Blt_SwitchSpec *sp;

    for (sp = specs; sp->type != BLT_SWITCH_END; sp++) {
        if ((sp->flags & needFlags) == needFlags) {
            char *ptr;

            ptr = (char *)record + sp->offset;
            switch (sp->type) {
            case BLT_SWITCH_STRING:
                /* Strings are allocated via Blt_Strdup */
                if (*((char **) ptr) != NULL) {
                    Blt_Free(*((char **) ptr));
                    *((char **) ptr) = NULL;
                }
                break;

            case BLT_SWITCH_LIST:
                /* Lists are allocated via Tcl_SplitList */
                if (*((char **) ptr) != NULL) {
                    Tcl_Free(*((char **) ptr));
                    *((char **) ptr) = NULL;
                }
                break;

            case BLT_SWITCH_LISTOBJ:
            case BLT_SWITCH_OBJ:
                if (*((Tcl_Obj **) ptr) != NULL) {
                    Tcl_DecrRefCount(*((Tcl_Obj **)ptr));
                    *((Tcl_Obj **) ptr) = NULL;
                }
                break;

            case BLT_SWITCH_CUSTOM:
                assert(sp->customPtr != NULL);
                if ((sp->customPtr->freeProc != NULL) && 
                    (*(char **)ptr != NULL)) {
                    (*sp->customPtr->freeProc)(sp->customPtr->clientData, 
                        (char *)record, sp->offset, sp->flags);
                }
                break;

            default:
                break;
            }
        }
    }
}

/*
 *---------------------------------------------------------------------------
 *
 * Blt_SwitchChanged --
 *
 *      Given the configuration specifications and one or more switch patterns
 *      (terminated by a NULL), indicate if any of the matching switches has
 *      been reset.
 *
 * Results:
 *      Returns 1 if one of the switches have changed, 0 otherwise.
 *
 *---------------------------------------------------------------------------
 */
int 
Blt_SwitchChanged(Blt_SwitchSpec *specs, ...)
{
    const char *switchName;
    va_list args;

    va_start(args, specs);
    while ((switchName = va_arg(args, char *)) != NULL) {
        Blt_SwitchSpec *sp;

        for (sp = specs; sp->type != BLT_SWITCH_END; sp++) {
            if ((Tcl_StringMatch(sp->switchName, switchName)) &&
                (sp->flags & BLT_SWITCH_SPECIFIED)) {
                va_end(args);
                return 1;
            }
        }
    }
    va_end(args);
    return 0;
}
