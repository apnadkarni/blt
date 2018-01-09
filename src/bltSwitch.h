/* -*- mode: c; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 * bltSwitch.h --
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

#ifndef BLT_SWITCH_H
#define BLT_SWITCH_H

#ifndef offsetof
#define offsetof(TYPE, MEMBER) ((size_t) &((TYPE *)0)->MEMBER)
#endif

#ifndef Blt_Offset
#define Blt_Offset(type, field) ((int)offsetof(type, field))
#endif /* Blt_Offset */

typedef int (Blt_SwitchParseProc)(ClientData clientData, Tcl_Interp *interp, 
        const char *switchName, Tcl_Obj *valueObjPtr, char *record, int offset,
        int flags);

typedef Tcl_Obj *(Blt_SwitchPrintProc)(ClientData clientData, 
        Tcl_Interp *interp, char *record, int offset, int flags);

typedef void (Blt_SwitchFreeProc)(ClientData clientData, char *record, 
        int offset, int flags);

typedef struct {
    Blt_SwitchParseProc *parseProc;     /* Procedure to parse a switch value
                                         * and store it in its converted form
                                         * in the data record. */
    Blt_SwitchPrintProc *printProc;     /* Procedure to free a switch. */
    Blt_SwitchFreeProc *freeProc;       /* Procedure to free a switch. */
    ClientData clientData;              /* Arbitrary one-word value used by
                                         * switch parser, passed to
                                         * parseProc. */
} Blt_SwitchCustom;


/*
 * Type values for Blt_SwitchSpec structures.  See the user documentation for
 * details.
 */
typedef enum {
    BLT_SWITCH_BITS, 
    BLT_SWITCH_BITS_NOARG, 
    BLT_SWITCH_BOOLEAN, 
    BLT_SWITCH_BOOLEAN_NOARG, 
    BLT_SWITCH_CUSTOM, 
    BLT_SWITCH_DOUBLE, 
    BLT_SWITCH_FLOAT, 
    BLT_SWITCH_INT, 
    BLT_SWITCH_INT64, 
    BLT_SWITCH_INT_NNEG, 
    BLT_SWITCH_INT_POS,
    BLT_SWITCH_INVERT_BITS, 
    BLT_SWITCH_INVERT_BITS_NOARG, 
    BLT_SWITCH_LIST, 
    BLT_SWITCH_LISTOBJ, 
    BLT_SWITCH_LONG, 
    BLT_SWITCH_LONG_NNEG, 
    BLT_SWITCH_LONG_POS,
    BLT_SWITCH_OBJ,
    BLT_SWITCH_SIDE, 
    BLT_SWITCH_STRING, 
    BLT_SWITCH_VALUE, 
    BLT_SWITCH_END
} Blt_SwitchTypes;


typedef struct {
    Blt_SwitchTypes type;               /* Type of option, such as
                                         * BLT_SWITCH_COLOR; see definitions
                                         * below.  Last option in table must
                                         * have type BLT_SWITCH_END. */
    const char *switchName;             /* Switch used to specify option in
                                         * argv.  NULL means this spec is part
                                         * of a group. */
    const char *help;                   /* Help string. */
    const char *defValue;               /* Default value for option if not
                                         * specified in command line or
                                         * database. */
    int offset;                         /* Where in widget record to store
                                         * value; use Blt_Offset macro to
                                         * generate values for this. */
    int flags;                          /* Any combination of the values
                                         * defined below. */
    unsigned int mask;
    Blt_SwitchCustom *customPtr;        /* If type is BLT_SWITCH_CUSTOM then
                                         * this is a pointer to info about how
                                         * to parse and print the option.
                                         * Otherwise it is irrelevant. */
} Blt_SwitchSpec;

#define BLT_SWITCH_DEFAULTS             (0)
#define BLT_SWITCH_ARGV_PARTIAL         (1<<1)
#define BLT_SWITCH_OBJV_PARTIAL         (1<<1)
#define BLT_SWITCH_INITIALIZE           (1<<2)

/*
 * Possible flag values for Blt_SwitchSpec structures.  Any bits at or above
 * BLT_SWITCH_USER_BIT may be used by clients for selecting certain entries.
 */
#define BLT_SWITCH_NULL_OK              (1<<0)
#define BLT_SWITCH_DONT_SET_DEFAULT     (1<<3)
#define BLT_SWITCH_SPECIFIED            (1<<4)
#define BLT_SWITCH_USER_BIT             (1<<8)

BLT_EXTERN int Blt_ExprDoubleFromObj (Tcl_Interp *interp, Tcl_Obj *objPtr, 
        double *valuePtr);

BLT_EXTERN int Blt_ExprIntFromObj (Tcl_Interp *interp, Tcl_Obj *objPtr, 
        int *valuePtr);

BLT_EXTERN int Blt_GetStateFromObj(Tcl_Interp *interp, Tcl_Obj *objPtr, 
        int *statePtr);

BLT_EXTERN const char *Blt_NameOfState(int state);

BLT_EXTERN int Blt_GetFillFromObj(Tcl_Interp *interp, Tcl_Obj *objPtr, 
        int *fillPtr);

BLT_EXTERN const char *Blt_NameOfFill(int fill);

BLT_EXTERN int Blt_GetResizeFromObj(Tcl_Interp *interp, Tcl_Obj *objPtr, 
        int *fillPtr);

BLT_EXTERN const char *Blt_NameOfResize(int resize);

BLT_EXTERN int Blt_GetSideFromObj(Tcl_Interp *interp, Tcl_Obj *objPtr, 
        int *sidePtr);

BLT_EXTERN const char *Blt_NameOfSide(int side);

BLT_EXTERN int Blt_GetCount(Tcl_Interp *interp, const char *string, 
        int check, long *countPtr);

BLT_EXTERN int Blt_GetCountFromObj(Tcl_Interp *interp, Tcl_Obj *objPtr, 
        int check, long *countPtr);

BLT_EXTERN int Blt_ParseSwitches(Tcl_Interp *interp, Blt_SwitchSpec *specPtr, 
        int objc, Tcl_Obj *const *objv, void *rec, int flags);

BLT_EXTERN void Blt_FreeSwitches(Blt_SwitchSpec *specs, void *rec, int flags);

BLT_EXTERN int Blt_SwitchChanged(Blt_SwitchSpec *specs, ...);
 
BLT_EXTERN int Blt_SwitchInfo(Tcl_Interp *interp, Blt_SwitchSpec *specs,
        void *record, Tcl_Obj *objPtr, int flags);

BLT_EXTERN int Blt_SwitchValue(Tcl_Interp *interp, Blt_SwitchSpec *specs, 
        void *record, Tcl_Obj *objPtr, int flags);

#endif /* BLT_SWITCH_H */
