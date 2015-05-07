/* -*- mode: c; c-basic-offset: 4; indent-tabs-mode: nil -*- */

#define BUILD_BLT_TK_PROCS 1
#include "bltInt.h"
#include "bltTkProcs.h"

extern char *Blt_InitTkProcs(Tcl_Interp *interp, const char *version);

Blt_TkProcs *bltTkProcsPtr = NULL;

const char *
Blt_InitTkProcs(Tcl_Interp *interp, const char *version)
{
    ClientData clientData;

    clientData = NULL;
    if (Tcl_PkgRequireEx(interp, "blt_tk", version, PKG_EXACT, &clientData) 
        == NULL) {
        return NULL;
    }
    bltTkProcsPtr = clientData;
    return version;
}
