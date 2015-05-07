/* -*- mode: c; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 * bltGrPlay.c --
 *
 * This module implements regions for the BLT graph widget.
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

#ifdef HAVE_STRING_H
#  include <string.h>
#endif /* HAVE_STRING_H */

#ifdef HAVE_LIMITS_H
#  include <limits.h>
#endif

#include "bltHash.h"
#include "bltChain.h"
#include "bltOp.h"
#include "bltBind.h"
#include "bltPs.h"
#include "bltBg.h"
#include "bltPicture.h"
#include "bltGraph.h"
#include "bltGrElem.h"
#include "bltGrAxis.h"
#include "bltGrLegd.h"

#define DEF_REGION_FROM         "-1"
#define DEF_REGION_TO           "-1"
#define DEF_REGION_ENABLED      "0"

static Blt_OptionFreeProc FreeElements;
static Blt_OptionParseProc ObjToElements;
static Blt_OptionPrintProc ElementsToObj;
Blt_CustomOption elementsOption =
{
    ObjToElements, ElementsToObj, FreeElements, (ClientData)0
};

static Blt_ConfigSpec configSpecs[] =
{
    {BLT_CONFIG_BOOLEAN, "-enable", "enable", "enable", 
        DEF_REGION_ENABLED, Blt_Offset(Playback, enabled), 
        BLT_CONFIG_DONT_SET_DEFAULT},
    {BLT_CONFIG_INT, "-from", "from", "From", DEF_REGION_FROM, 
        Blt_Offset(Playback, from), BLT_CONFIG_DONT_SET_DEFAULT},
    {BLT_CONFIG_INT, "-to", "to", "To", DEF_REGION_TO, 
        Blt_Offset(Playback, to), BLT_CONFIG_DONT_SET_DEFAULT},
    {BLT_CONFIG_END, NULL, NULL, NULL, NULL, 0, 0}
};


static void
EnableElements(Blt_Chain chain)
{
    Blt_ChainLink link;
    
    for (link = Blt_Chain_FirstLink(chain); link != NULL;
         link = Blt_Chain_NextLink(link)) {
        Element *elemPtr;
        
        elemPtr = Blt_Chain_GetValue(link);
        elemPtr->flags |= REGION_ENABLED;
    }
}

static void
DisableElements(Blt_Chain chain)
{
    Blt_ChainLink link;
    
    for (link = Blt_Chain_FirstLink(chain); link != NULL;
         link = Blt_Chain_NextLink(link)) {
        Element *elemPtr;
        
        elemPtr = Blt_Chain_GetValue(link);
        elemPtr->flags &= ~REGION_ENABLED;
    }
}

/*ARGSUSED*/
static void
FreeElements(ClientData clientData, Display *display, char *widgRec,
             int offset)
{
    Blt_Chain *chainPtr = (Blt_Chain *)(widgRec + offset);

    if (*chainPtr != NULL) {
        DisableElements(*chainPtr);
        Blt_Chain_Destroy(*chainPtr);
        *chainPtr = NULL;
    }
}

/*
 *---------------------------------------------------------------------------
 *
 * ObjToElements --
 *
 *      Converts a TCL list of element names into a chain of Element 
 *      pointers.  This is used to define a subset of elements that
 *      have a region displayed.
 *
 * Results:
 *      The return value is a standard TCL result.  
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
ObjToElements(ClientData clientData, Tcl_Interp *interp, Tk_Window tkwin,
              Tcl_Obj *objPtr, char *widgRec, int offset, int flags)
{
    Blt_Chain *chainPtr = (Blt_Chain *)(widgRec + offset);
    Graph *graphPtr = clientData;
    Tcl_Obj **objv;
    int objc;
    Blt_Chain chain;

    if (Tcl_ListObjGetElements(interp, objPtr, &objc, &objv) != TCL_OK) {
        return TCL_ERROR;
    }
    chain = NULL;
    if (objc > 0) {
        int i;

        chain = Blt_Chain_Create();
        for (i = 0; i < objc; i++) {
            Element *elemPtr;
            
            if (Blt_GetElement(interp, graphPtr, objPtr, &elemPtr) != TCL_OK) {
                Blt_Chain_Destroy(chain);
                return TCL_ERROR;
            }
            Blt_Chain_Append(chain, elemPtr);
        }
    }
    FreeElements(clientData, graphPtr->display, widgRec, offset);
    *chainPtr = chain;
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * ElementsToObj --
 *
 *      Converts the chain of Element pointers to a TCL list of element
 *      names.
 *
 * Results:
 *      The return value is a string (TCL list).
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static Tcl_Obj *
ElementsToObj(ClientData clientData, Tcl_Interp *interp, Tk_Window tkwin,
              char *widgRec, int offset, int flags)
{
    Blt_Chain *chainPtr = (Blt_Chain *)(widgRec + offset);
    Tcl_Obj *listObjPtr;
    Blt_ChainLink link;

    listObjPtr = Tcl_NewListObj(0, (Tcl_Obj **)NULL);
    for (link = Blt_Chain_FirstLink(*chainPtr); link != NULL;
         link = Blt_Chain_NextLink(link)) {
        Element *elemPtr;
        Tcl_Obj *objPtr;

        elemPtr = Blt_Chain_GetValue(link);
        objPtr = Tcl_NewStringObj(elemPtr->obj.name, -1);
        Tcl_ListObjAppendElement(interp, listObjPtr, objPtr);
    }
    return listObjPtr;
}

static int
ConfigurePlayback(Graph *graphPtr, Tcl_Interp *interp, int objc, 
                  Tcl_Obj *const *objv, int flags)
{
    Playback *playPtr = &graphPtr->play;
    
    if (Blt_ConfigureWidgetFromObj(interp, graphPtr->tkwin, configSpecs, 
        objc, objv, (char *)playPtr, flags) != TCL_OK) {
        return TCL_ERROR;
    }
    if (playPtr->enabled) {
        if (playPtr->elements != NULL) {
            DisableElements(graphPtr->elements.displayList);
            EnableElements(playPtr->elements);
        } else {
            EnableElements(graphPtr->elements.displayList);
        }
    } else {
        DisableElements(graphPtr->elements.displayList);
    }
    if (playPtr->from > playPtr->to) {
        playPtr->t1 = playPtr->to;
        playPtr->t2 = playPtr->from;
    } else {
        playPtr->t1 = playPtr->from;
        playPtr->t2 = playPtr->to;
    }
    if (playPtr->t2 < 0) {
        playPtr->t2 = UINT_MAX;
    }
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * Blt_DestroyPlayback --
 *
 * Results:
 *      None
 *
 *---------------------------------------------------------------------------
 */
void
Blt_DestroyPlayback(Graph *graphPtr)
{
    Playback *playPtr = &graphPtr->play;

    Blt_FreeOptions(configSpecs, (char *)playPtr, graphPtr->display, 0);
}

/*
 *---------------------------------------------------------------------------
 *
 * Blt_CreatePlayback --
 *
 *      Initializes the playback structure used to replay graphs.
 *
 * Results:
 *      Returns TCL_ERROR if the play structure can't be created,
 *      otherwise TCL_OK.
 *
 *---------------------------------------------------------------------------
 */
int
Blt_CreatePlayback(Graph *graphPtr)
{
    Playback *playPtr;

    playPtr = &graphPtr->play;
    playPtr->from = -1;
    playPtr->to = -1;
    playPtr->elements = NULL;
    if (ConfigurePlayback(graphPtr, graphPtr->interp, 0, NULL, 0) != TCL_OK) {
        return TCL_ERROR;
    }
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * CgetOp --
 *
 *      Queries configuration attributes of the playback.
 *
 * Results:
 *      A standard TCL result.
 *
 *---------------------------------------------------------------------------
 */
/* ARGSUSED */
static int
CgetOp(ClientData clientData, Tcl_Interp *interp, int objc,
       Tcl_Obj *const *objv)
{
    Graph *graphPtr = clientData;
    Playback *playPtr = &graphPtr->play;
    
    return Blt_ConfigureValueFromObj(interp, graphPtr->tkwin, configSpecs,
            (char *)playPtr, objv[3], 0);
}

/*
 *---------------------------------------------------------------------------
 *
 * ConfigureOp --
 *
 *      Queries or resets configuration attributes of the playback.
 *
 * Results:
 *      A standard TCL result.
 *
 * Side Effects:
 *      Playback options are reset.
 *
 *---------------------------------------------------------------------------
 */
static int
ConfigureOp(ClientData clientData, Tcl_Interp *interp, int objc,
            Tcl_Obj *const *objv)
{
    Graph *graphPtr = clientData;
    Playback *playPtr = &graphPtr->play;

    if (objc == 3) {
        return Blt_ConfigureInfoFromObj(interp, graphPtr->tkwin, configSpecs,
                (char *)playPtr, (Tcl_Obj *)NULL, 0);
    } else if (objc == 4) {
        return Blt_ConfigureInfoFromObj(interp, graphPtr->tkwin, configSpecs,
                (char *)playPtr, objv[3], 0);
    }
    if (ConfigurePlayback(graphPtr, interp, objc - 3, objv + 3, 
        BLT_CONFIG_OBJV_ONLY) != TCL_OK) {
        return TCL_ERROR;
    }
    graphPtr->flags |= REDRAW_WORLD | CACHE_DIRTY;
    Blt_EventuallyRedrawGraph(graphPtr);
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * MaxPointsOp --
 *
 *      Returns the maximum number of points of the selected elements
 *      (designated by the -elements option).  This is a convenience
 *      function to determine the limit of the data point indices.  
 *
 * Results:
 *      A standard TCL result.
 *
 *---------------------------------------------------------------------------
 */
/* ARGSUSED */
static int
MaxPointsOp(ClientData clientData, Tcl_Interp *interp, int objc,
          Tcl_Obj *const *objv)
{
    Graph *graphPtr = clientData;
    Blt_ChainLink link;
    Blt_Chain chain;
    Playback *playPtr = &graphPtr->play;
    int maxNumPts;

    maxNumPts = 0;
    if (playPtr->elements != NULL) {
        chain = playPtr->elements;
    } else {
        chain = graphPtr->elements.displayList;
    }
    for (link = Blt_Chain_LastLink(chain); link != NULL;
         link = Blt_Chain_PrevLink(link)) {
        Element *elemPtr;

        elemPtr = Blt_Chain_GetValue(link);
        if ((elemPtr->flags & HIDDEN) == 0) {
            int length;

            length = NUMBEROFPOINTS(elemPtr);
            if (length > maxNumPts) {
                maxNumPts = length;
            }
        }
    }
    Tcl_SetIntObj(Tcl_GetObjResult(interp), maxNumPts);
    return TCL_OK;
}

static Blt_OpSpec playOps[] =
{
    {"cget",      2, CgetOp,      4, 4, "option",},
    {"configure", 2, ConfigureOp, 3, 0, "?options...?",},
    {"maxpoints", 1, MaxPointsOp, 3, 3, "",},
};
static int numPlayOps = sizeof(playOps) / sizeof(Blt_OpSpec);

/*
 *---------------------------------------------------------------------------
 *
 * Blt_GraphRegionOp --
 *
 *      Used to configure the playback.  Playback
 *
 * Results:
 *      The return value is a standard TCL result.
 *
 *      .g playback configure -from 0 -to 10
 *---------------------------------------------------------------------------
 */
int
Blt_GraphRegionOp(ClientData clientData, Tcl_Interp *interp, int objc,
               Tcl_Obj *const *objv)
{
    Tcl_ObjCmdProc *proc;

    proc = Blt_GetOpFromObj(interp, numPlayOps, playOps, BLT_OP_ARG2, 
        objc, objv, 0);
    if (proc == NULL) {
        return TCL_ERROR;
    }
    return (*proc) (clientData, interp, objc, objv);
}
