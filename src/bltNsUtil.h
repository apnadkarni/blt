/* -*- mode: c; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 * bltNsUtil.h --
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

#ifndef BLT_NS_UTIL_H
#define BLT_NS_UTIL_H 1

#ifndef TCL_NAMESPACE_ONLY
#define TCL_NAMESPACE_ONLY TCL_GLOBAL_ONLY
#endif

#define NS_SEARCH_NONE		(0)
#define NS_SEARCH_CURRENT	(1<<0)
#define NS_SEARCH_GLOBAL	(1<<1)
#define NS_SEARCH_BOTH		(NS_SEARCH_GLOBAL | NS_SEARCH_CURRENT)

typedef struct _Blt_ObjectName {
    const char *name;
    Tcl_Namespace *nsPtr;
} Blt_ObjectName;

#define BLT_NO_DEFAULT_NS	(1<<0)
#define BLT_NO_ERROR_MSG	(1<<1)

#ifndef USE_TCL_STUBS

#if (_TCL_VERSION < _VERSION(8,5,0)) 
BLT_EXTERN Tcl_Command Tcl_FindCommand(Tcl_Interp *interp, const char *name, 
	Tcl_Namespace *nsPtr, int flags);
/*
 * Namespace procedures not prototyped defined in Tcl.h 
 */
BLT_EXTERN Tcl_Namespace *Tcl_GetCurrentNamespace(Tcl_Interp *interp);

BLT_EXTERN Tcl_Namespace *Tcl_GetGlobalNamespace(Tcl_Interp *interp);

BLT_EXTERN Tcl_Namespace *Tcl_CreateNamespace(Tcl_Interp *interp, 
	const char *name, ClientData clientData, 
	Tcl_NamespaceDeleteProc *nsDelProc);

BLT_EXTERN void Tcl_DeleteNamespace(Tcl_Namespace *nsPtr);

BLT_EXTERN Tcl_Namespace *Tcl_FindNamespace(Tcl_Interp *interp, 
	const char *name, Tcl_Namespace *context, int flags);

BLT_EXTERN int Tcl_Export(Tcl_Interp *interp, Tcl_Namespace *nsPtr,
	const char *name, int resetFlag);

#endif	/* < 8.5.0 */

BLT_EXTERN Tcl_Var Tcl_FindNamespaceVar(Tcl_Interp *interp, const char *name, 
	Tcl_Namespace *contextNsPtr, int flags);

BLT_EXTERN void Tcl_PopCallFrame(Tcl_Interp *interp);

BLT_EXTERN int Tcl_PushCallFrame(Tcl_Interp *interp, Tcl_CallFrame *framePtr, 
	Tcl_Namespace *nsPtr, int isProcCallFrame);

#endif /* USE_TCL_STUBS */

/* 
 * Auxillary procedures 
 */
BLT_EXTERN Tcl_Namespace *Blt_GetVariableNamespace(Tcl_Interp *interp, 
	const char *varName);

BLT_EXTERN Tcl_Namespace *Blt_GetCommandNamespace(Tcl_Command cmdToken);

BLT_EXTERN Tcl_CallFrame *Blt_EnterNamespace(Tcl_Interp *interp, 
	Tcl_Namespace *nsPtr);

BLT_EXTERN void Blt_LeaveNamespace(Tcl_Interp *interp, Tcl_CallFrame *framePtr);

BLT_EXTERN int Blt_ParseObjectName(Tcl_Interp *interp, const char *name, 
	Blt_ObjectName *objNamePtr, unsigned int flags);

BLT_EXTERN const char *Blt_MakeQualifiedName(Blt_ObjectName *objNamePtr, 
	Tcl_DString *resultPtr);

BLT_EXTERN int Blt_CommandExists(Tcl_Interp *interp, const char *string);

#endif /* BLT_NS_UTIL_H */
