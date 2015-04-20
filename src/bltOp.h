/* -*- mode: c; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 * bltOp.h --
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

#ifndef _BLT_OP_H
#define _BLT_OP_H

/*
 *---------------------------------------------------------------------------
 *
 * Blt_OpSpec --
 *
 * 	Structure to specify a set of operations for a TCL command.
 *      This is passed to the Blt_GetOp procedure to look
 *      for a function pointer associated with the operation name.
 *
 *---------------------------------------------------------------------------
 */
typedef struct _Blt_OpSpec {
    const char *name;		/* Name of operation */
    int minChars;		/* Minimum # characters to disambiguate */
    void *proc;
    int minArgs;		/* Minimum # args required */
    int maxArgs;		/* Maximum # args required */
    const char *usage;		/* Usage message */
} Blt_OpSpec;

typedef enum {
    BLT_OP_ARG0,		/* Op is the first argument. */
    BLT_OP_ARG1,		/* Op is the second argument. */
    BLT_OP_ARG2,		/* Op is the third argument. */
    BLT_OP_ARG3,		/* Op is the fourth argument. */
    BLT_OP_ARG4			/* Op is the fifth argument. */

} Blt_OpIndex;

#define BLT_OP_BINARY_SEARCH	0
#define BLT_OP_LINEAR_SEARCH	1

BLT_EXTERN void *Blt_GetOpFromObj(Tcl_Interp *interp, int numSpecs, 
	Blt_OpSpec *specs, int operPos, int objc, Tcl_Obj *const *objv, 
	int flags);

#endif /*_BLT_OP_H*/
