/* -*- mode: c; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 * bltGrIsoline.c --
 *
 * This module implements isolines for the BLT graph widget.
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

#ifdef HAVE_CTYPE_H
  #include <ctype.h>
#endif  /* HAVE_CTYPE_H */

#ifdef HAVE_STRING_H
  #include <string.h>
#endif /* HAVE_STRING_H */

#ifdef HAVE_STDLIB_H
  #include <stdlib.h>
#endif /* HAVE_STDLIB_H */

#include <X11/Xutil.h>
#include "bltAlloc.h"
#include "bltChain.h"
#include "bltHash.h"
#include "bltBind.h"
#include "bltBg.h"
#include "bltOp.h"
#include "bltGraph.h"
#include "bltGrElem.h"
#include "bltGrIsoline.h"

#define SYMBOLS         (1<<17)         /* Draw the symbols on top of the
                                         * isolines. */

#define DEF_ACTIVE_PEN  "activeIsoline"
#define DEF_HIDE        "no"
#define DEF_ELEMENT     (char *)NULL
#define DEF_LABEL       (char *)NULL
#define DEF_MAX         ""
#define DEF_MIN         ""
#define DEF_PEN         (char *)NULL
#define DEF_SHOW        "yes"
#define DEF_SYMBOLS     "0"
#define DEF_TAGS        "all"
#define DEF_VALUE       "0.0"

/*
 * IsolineIterator --
 *
 *      Tabs may be tagged with strings.  A tab may have many tags.  The
 *      same tag may be used for many tabs.
 *      
 */
typedef enum { 
    ITER_SINGLE, ITER_ALL, ITER_TAG, 
} IteratorType;

typedef struct _IsolineIterator {
    Graph *graphPtr;                    /* Graph that we're iterating over. */

    IteratorType type;                  /* Type of iteration:
                                         * ITER_TAG      By item tag.
                                         * ITER_ALL      By every item.
                                         * ITER_SINGLE   Single item: either 
                                         *               tag or index.
                                         */

    Isoline *startPtr;                  /* Starting item.  Starting point
                                         * of search, saved if iterator is
                                         * reused.  Used for ITER_ALL and
                                         * ITER_SINGLE searches. */
    Isoline *endPtr;                    /* Ending item (inclusive). */
    Isoline *nextPtr;                   /* Next item. */
                                        /* For tag-based searches. */
    const char *tagName;                /* If non-NULL, is the tag that we
                                         * are currently iterating over. */
    Blt_HashTable *tablePtr;            /* Pointer to tag hash table. */
    Blt_HashSearch cursor;              /* Search iterator for tag hash
                                         * table. */
    Blt_ChainLink link;
} IsolineIterator;

static Blt_OptionFreeProc FreeTagsProc;
static Blt_OptionParseProc ObjToTags;
static Blt_OptionPrintProc TagsToObj;
static Blt_CustomOption tagsOption = {
    ObjToTags, TagsToObj, FreeTagsProc, (ClientData)0
};


BLT_EXTERN Blt_CustomOption bltContourElementOption;
BLT_EXTERN Blt_CustomOption bltContourPenOption;
BLT_EXTERN Blt_CustomOption bltLimitOption;

static Blt_ConfigSpec isolineSpecs[] =
{
    {BLT_CONFIG_CUSTOM, "-activepen", "activePen", "ActivePen",
         DEF_ACTIVE_PEN, Blt_Offset(Isoline, activePenPtr), 
         BLT_CONFIG_NULL_OK, &bltContourPenOption},
    {BLT_CONFIG_SYNONYM, "-bindtags", "tags"},
    {BLT_CONFIG_CUSTOM, "-element", "element", "Element", DEF_ELEMENT, 
         Blt_Offset(Isoline, elemPtr), BLT_CONFIG_NULL_OK,
         &bltContourElementOption},
    {BLT_CONFIG_BITMASK, "-hide", "hide", "Hide", DEF_HIDE, 
         Blt_Offset(Isoline, flags), BLT_CONFIG_DONT_SET_DEFAULT,
        (Blt_CustomOption *)HIDDEN},
    {BLT_CONFIG_STRING, "-label", "label", "Label", DEF_LABEL, 
        Blt_Offset(Isoline, label), BLT_CONFIG_NULL_OK},
    {BLT_CONFIG_CUSTOM, "-min", "min", "Min", DEF_MIN,
        Blt_Offset(Isoline, reqMin), BLT_CONFIG_DONT_SET_DEFAULT,
        &bltLimitOption},
    {BLT_CONFIG_CUSTOM, "-max", "max", "Max", DEF_MAX,
        Blt_Offset(Isoline, reqMax), BLT_CONFIG_DONT_SET_DEFAULT,
        &bltLimitOption},
    {BLT_CONFIG_CUSTOM, "-pen", "pen", "Pen", DEF_PEN, 
        Blt_Offset(Isoline, penPtr), BLT_CONFIG_NULL_OK, 
        &bltContourPenOption},
    {BLT_CONFIG_BITMASK_INVERT, "-show", "show", "Show", DEF_SHOW, 
         Blt_Offset(Isoline, flags), BLT_CONFIG_DONT_SET_DEFAULT,
        (Blt_CustomOption *)HIDDEN},
    {BLT_CONFIG_BITMASK, "-symbols", "symbols", "Symbols", DEF_SYMBOLS,
        Blt_Offset(Isoline, flags), BLT_CONFIG_DONT_SET_DEFAULT,
        (Blt_CustomOption *)SYMBOLS}, 
    {BLT_CONFIG_CUSTOM, "-tags", "tags", "Tags", DEF_TAGS, 0,
        BLT_CONFIG_NULL_OK, &tagsOption},
    {BLT_CONFIG_DOUBLE, "-value", "value", "Value", DEF_VALUE,
        Blt_Offset(Isoline, reqValue), BLT_CONFIG_DONT_SET_DEFAULT},
    {BLT_CONFIG_END, NULL, NULL, NULL, NULL, 0, 0}
};

ClientData
Blt_MakeIsolineTag(Graph *graphPtr, const char *tagName)
{
    Blt_HashEntry *hPtr;
    int isNew;

    assert(tagName != NULL);
    hPtr = Blt_CreateHashEntry(&graphPtr->isolines.bindTagTable, tagName,
                &isNew);
    return Blt_GetHashKey(&graphPtr->isolines.bindTagTable, hPtr);
}

/* Isoline procedures. */
/*
 *---------------------------------------------------------------------------
 *
 * SetTag --
 *
 *      Associates a tag with a given isoline.  Individual isoline tags are
 *      stored in hash tables keyed by the tag name.  Each table is in turn
 *      stored in a hash table keyed by the isoline pointer.
 *
 * Results:
 *      None.
 *
 * Side Effects:
 *      A tag is stored for a particular isoline.
 *
 *---------------------------------------------------------------------------
 */
static int
SetTag(Tcl_Interp *interp, Isoline *isoPtr, Tcl_Obj *objPtr)
{
    Graph *graphPtr;
    const char *string;
    char c;

    string = Tcl_GetString(objPtr);
    c = string[0];
    if ((c == 'a') && (strcmp(string, "all") == 0)) {
        return TCL_OK;                  /* Don't need to create reserved
                                         * tag. */
    }
    if (c == '\0') {
        if (interp != NULL) {
            Tcl_AppendResult(interp, "tag \"", string, "\" can't be empty.", 
                (char *)NULL);
        }
        return TCL_ERROR;
    }
    if (c == '-') {
        if (interp != NULL) {
            Tcl_AppendResult(interp, "tag \"", string, 
                "\" can't start with a '-'.", (char *)NULL);
        }
        return TCL_ERROR;
    }
    if ((isdigit(c)) && (Blt_ObjIsInteger(objPtr))) {
        if (interp != NULL) {
            Tcl_AppendResult(interp, "tag \"", string, "\" can't be a number.",
                             (char *)NULL);
        }
        return TCL_ERROR;
    }
    graphPtr = isoPtr->obj.graphPtr;
    Blt_Tags_AddItemToTag(&graphPtr->isolines.tags, string, isoPtr);
    return TCL_OK;
}

/*ARGSUSED*/
static void
FreeTagsProc(ClientData clientData, Display *display, char *widgRec, int offset)
{
    Isoline *isoPtr = (Isoline *)widgRec;
    Graph *graphPtr;
    
    graphPtr = isoPtr->obj.graphPtr;
    Blt_Tags_ClearTagsFromItem(&graphPtr->isolines.tags, isoPtr);
}

/*
 *---------------------------------------------------------------------------
 *
 * ObjToTags --
 *
 *      Convert the string representation of a list of tags.
 *
 * Results:
 *      The return value is a standard TCL result.  The tags are
 *      save in the widget.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
ObjToTags(ClientData clientData, Tcl_Interp *interp, Tk_Window tkwin, 
          Tcl_Obj *objPtr, char *widgRec, int offset, int flags)  
{
    Isoline *isoPtr = (Isoline *)widgRec;
    int i;
    const char *string;
    int objc;
    Tcl_Obj **objv;
    Graph *graphPtr;
    
    graphPtr = isoPtr->obj.graphPtr;
    Blt_Tags_ClearTagsFromItem(&graphPtr->isolines.tags, isoPtr);
    string = Tcl_GetString(objPtr);
    if ((string[0] == '\0') && (flags & BLT_CONFIG_NULL_OK)) {
        return TCL_OK;
    }
    if (Tcl_ListObjGetElements(interp, objPtr, &objc, &objv) != TCL_OK) {
        return TCL_ERROR;
    }
    for (i = 0; i < objc; i++) {
        SetTag(interp, isoPtr, objv[i]);
    }
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * TagsToObj --
 *
 *      Returns the tags associated with the element.
 *
 * Results:
 *      The names representing the tags are returned.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static Tcl_Obj *
TagsToObj(ClientData clientData, Tcl_Interp *interp, Tk_Window tkwin,
          char *widgRec, int offset, int flags)  
{
    Isoline *isoPtr = (Isoline *)widgRec;
    Tcl_Obj *listObjPtr;
    Graph *graphPtr;
    
    graphPtr = isoPtr->obj.graphPtr;
    listObjPtr = Tcl_NewListObj(0, (Tcl_Obj **)NULL);
    Blt_Tags_AppendTagsToObj(&graphPtr->isolines.tags, isoPtr, listObjPtr);
    return listObjPtr;
}

/*
 *---------------------------------------------------------------------------
 *
 * NextTaggedIsoline --
 *
 *      Returns the next isoline derived from the given tag.
 *
 * Results:
 *      Returns the pointer to the next tab in the iterator.  If no more
 *      tabs are available, then NULL is returned.
 *
 *---------------------------------------------------------------------------
 */
static Isoline *
NextTaggedIsoline(IsolineIterator *iterPtr)
{
    switch (iterPtr->type) {
    case ITER_TAG:
        if (iterPtr->link != NULL) {
            Isoline *isoPtr;
            
            isoPtr = Blt_Chain_GetValue(iterPtr->link);
            iterPtr->link = Blt_Chain_NextLink(iterPtr->link);
            return isoPtr;
        }
        break;

    case ITER_ALL:
        {
            Blt_HashEntry *hPtr;
            
            hPtr = Blt_NextHashEntry(&iterPtr->cursor); 
            if (hPtr != NULL) {
                return Blt_GetHashValue(hPtr);
            }
            break;
        }

    default:
        break;
    }   
    return NULL;
}

/*
 *---------------------------------------------------------------------------
 *
 * FirstTaggedIsoline --
 *
 *      Returns the first isoline derived from the given tag.
 *
 * Results:
 *      Returns the first isoline in the sequence.  If no more isolines are in
 *      the list, then NULL is returned.
 *
 *---------------------------------------------------------------------------
 */
static Isoline *
FirstTaggedIsoline(IsolineIterator *iterPtr)
{
    switch (iterPtr->type) {
    case ITER_TAG:
        if (iterPtr->link != NULL) {
            Isoline *isoPtr;
            
            isoPtr = Blt_Chain_GetValue(iterPtr->link);
            iterPtr->link = Blt_Chain_NextLink(iterPtr->link);
            return isoPtr;
        }
        break;
    case ITER_ALL:
        {
            Blt_HashEntry *hPtr;
            
            hPtr = Blt_FirstHashEntry(iterPtr->tablePtr, &iterPtr->cursor);
            if (hPtr != NULL) {
                return Blt_GetHashValue(hPtr);
            }
        }
        break;

    case ITER_SINGLE:
        return iterPtr->startPtr;
    } 
    return NULL;
}

static int
GetIsolineFromObj(Tcl_Interp *interp, Graph *graphPtr, Tcl_Obj *objPtr,
                  Isoline **isoPtrPtr)
{
    Blt_HashEntry *hPtr;
    const char *string;

    string = Tcl_GetString(objPtr);
    hPtr = Blt_FindHashEntry(&graphPtr->isolines.nameTable, string);
    if (hPtr == NULL) {
        if (interp != NULL) {
            Tcl_AppendResult(interp, "can't find an isoline \"", string, 
		"\" in graph \"", Tk_PathName(graphPtr->tkwin), "\"",
		(char *)NULL);
        }
        return TCL_ERROR;
    }
    *isoPtrPtr = Blt_GetHashValue(hPtr);
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * GetIsolineIterator --
 *
 *      Converts a string representing a tab index into an tab pointer.  The
 *      index may be in one of the following forms:
 *
 *       "all"      
    All isolines.
 *       name           Name of the isoline.
 *       tag            Tag associated with isolines.
 *
 *---------------------------------------------------------------------------
 */
static int
GetIsolineIterator(Tcl_Interp *interp, Graph *graphPtr, Tcl_Obj *objPtr,
                   IsolineIterator *iterPtr)
{
    Isoline *isoPtr;
    Blt_Chain chain;
    const char *string;
    char c;
    int numBytes, length;
    
    iterPtr->graphPtr = graphPtr;
    iterPtr->type = ITER_SINGLE;
    iterPtr->tagName = Tcl_GetStringFromObj(objPtr, &numBytes);
    iterPtr->link = NULL;
    iterPtr->nextPtr = NULL;
    iterPtr->startPtr = iterPtr->endPtr = NULL;

    string = Tcl_GetStringFromObj(objPtr, &length);
    c = string[0];
    if ((c == 'a') && (strcmp(iterPtr->tagName, "all") == 0)) {
        iterPtr->type  = ITER_ALL;
        iterPtr->tablePtr = &graphPtr->isolines.nameTable;
    } else if ((c == 'c') && (strcmp(string, "current") == 0)) {
        GraphObj *objPtr;

        objPtr = Blt_GetCurrentItem(graphPtr->bindTable);
        /* Report only on isolines. */
        if ((objPtr != NULL) && (!objPtr->deleted) &&
            (objPtr->classId == CID_ISOLINE)) {
            iterPtr->type = ITER_SINGLE;
            iterPtr->startPtr = iterPtr->endPtr = (Isoline *)objPtr;
        }
    } else if (GetIsolineFromObj(NULL, graphPtr, objPtr, &isoPtr) == TCL_OK) {
        iterPtr->startPtr = iterPtr->endPtr = isoPtr;
        iterPtr->type = ITER_SINGLE;
    } else if ((chain = Blt_Tags_GetItemList(&graphPtr->isolines.tags, string)) 
               != NULL) {
        iterPtr->tagName = string;
        iterPtr->link = Blt_Chain_FirstLink(chain);
        iterPtr->type = ITER_TAG;
    } else {
        if (interp != NULL) {
            Tcl_AppendResult(interp, "can't find isoline name or tag \"", 
                string, "\" in \"", Tk_PathName(graphPtr->tkwin), 
                "\"", (char *)NULL);
        }
        return TCL_ERROR;
    }   
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * NewIsoline --
 *
 *      Creates a new isoline structure and inserts it into the element's
 *      isoline table.h
 *
 *---------------------------------------------------------------------------
 */
static Isoline *
NewIsoline(Tcl_Interp *interp, Graph *graphPtr, const char *name)
{
    Isoline *isoPtr;
    Blt_HashEntry *hPtr;
    int isNew;
    char string[200];

    isoPtr = Blt_AssertCalloc(1, sizeof(Isoline));
    if (name == NULL) {
        sprintf(string, "isoline%d", graphPtr->nextIsolineId);
        graphPtr->nextIsolineId++;
        name = string;
    }
    hPtr = Blt_CreateHashEntry(&graphPtr->isolines.nameTable, name, &isNew);
    assert(isNew);
    Blt_GraphSetObjectClass(&isoPtr->obj, CID_ISOLINE);
    isoPtr->obj.graphPtr = graphPtr;
    isoPtr->obj.name = Blt_GetHashKey(&graphPtr->isolines.nameTable, hPtr);
    isoPtr->relValue = Blt_NaN();
    isoPtr->reqMin = isoPtr->reqMax = Blt_NaN();
    Blt_SetHashValue(hPtr, isoPtr);
    isoPtr->hashPtr = hPtr;
    isoPtr->link = Blt_Chain_Prepend(graphPtr->isolines.displayList, isoPtr); 
    return isoPtr;
}

/*
 *---------------------------------------------------------------------------
 *
 * DestroyIsoline --
 *
 *      Creates a new isoline structure and inserts it into the element's
 *      isoline table.
 *
 *---------------------------------------------------------------------------
 */
static void
DestroyIsoline(Isoline *isoPtr)
{
    Graph *graphPtr;

    graphPtr = isoPtr->obj.graphPtr;
    if (isoPtr->hashPtr != NULL) {
        Blt_DeleteHashEntry(&graphPtr->isolines.nameTable, isoPtr->hashPtr);
    }
    if (isoPtr->link != NULL) {
        Blt_Chain_DeleteLink(graphPtr->isolines.displayList, isoPtr->link);
    }
    if (graphPtr->bindTable != NULL) {
        Blt_DeleteBindings(graphPtr->bindTable, isoPtr);
    }
    if (isoPtr->elemPtr != NULL) {
	Blt_RemoveIsoline(isoPtr->elemPtr, isoPtr);
    }
    Blt_Tags_ClearTagsFromItem(&graphPtr->isolines.tags, isoPtr);
    Blt_FreeOptions(isolineSpecs, (char *)isoPtr, graphPtr->display, 0);
    Blt_Free(isoPtr);
}

static int
ConfigureIsoline(Tcl_Interp *interp, Isoline *isoPtr, int objc, 
                 Tcl_Obj *const *objv, int flags)
{
    Graph *graphPtr;
    Element *oldElemPtr;
    
    graphPtr = isoPtr->obj.graphPtr;
    oldElemPtr = isoPtr->elemPtr;
    if (Blt_ConfigureComponentFromObj(interp, graphPtr->tkwin, isoPtr->obj.name,
        "Isoline", isolineSpecs, objc, objv, (char *)isoPtr, flags) != TCL_OK) {
        return TCL_ERROR;
    }
    if (oldElemPtr != isoPtr->elemPtr) {
	if (oldElemPtr != NULL) {
	    Blt_RemoveIsoline(oldElemPtr, isoPtr);
	}
	if (isoPtr->elemPtr != NULL) {
	    Blt_AddIsoline(isoPtr->elemPtr, isoPtr);
	}
    }
    return TCL_OK;
}

/* Nearest isoline procedures. */

static double
DistanceToLine(
    int x, int y,                       /* Sample X-Y coordinate. */
    Point2d *p, Point2d *q,             /* End points of the line segment. */
    Point2d *t)                         /* (out) Point on line segment. */
{
    double right, left, top, bottom;

    *t = Blt_GetProjection(x, y, p, q);
    if (p->x > q->x) {
        right = p->x, left = q->x;
    } else {
        left = p->x, right = q->x;
    }
    if (p->y > q->y) {
        bottom = p->y, top = q->y;
    } else {
        top = p->y, bottom = q->y;
    }
    if (t->x > right) {
        t->x = right;
    } else if (t->x < left) {
        t->x = left;
    }
    if (t->y > bottom) {
        t->y = bottom;
    } else if (t->y < top) {
        t->y = top;
    }
    return hypot((t->x - x), (t->y - y));
}

static void
NearestPoint(Graph *graphPtr, NearestElement *nearestPtr)
{
    Blt_ChainLink link;

    for (link = Blt_Chain_FirstLink(graphPtr->isolines.displayList); 
         link != NULL; link = Blt_Chain_NextLink(link)) {
        Element *elemPtr;
        Isoline *isoPtr;
        IsolineSegment *s;
        int i;
        
        isoPtr = Blt_Chain_GetValue(link);
	if (isoPtr->flags & HIDDEN) {
	    continue;
	}
	elemPtr = isoPtr->elemPtr;
	if (elemPtr == NULL) {
	    continue;
	}
	if ((nearestPtr->elemPtr != NULL) && (nearestPtr->elemPtr != elemPtr)) {
	    continue;
	}
	if (elemPtr->flags & (HIDDEN|MAP_ITEM)) {
	    continue;
	}
        for (i = 0, s = isoPtr->segments; s != NULL; s = s->next, i++) {
            double d1, d2;
            
            d1 = hypot(s->x1 - nearestPtr->x, s->y1 - nearestPtr->y);
            d2 = hypot(s->x2 - nearestPtr->x, s->y2 - nearestPtr->y);
#ifdef notdef
            fprintf(stderr, "segment %d: x=%d, y=%d d1=%g d2=%g x1=%g y1=%g x2=%g y2=%g\n",
                    i, nearestPtr->x, nearestPtr->y, d1, d2,
                    s->x1, s->y1, s->x2, s->y2);
#endif
            if ((d1 < d2) && (d1 < nearestPtr->distance)) {
                nearestPtr->index = i;
                nearestPtr->distance = d1;
                nearestPtr->item = isoPtr;
                nearestPtr->value = isoPtr->relValue;
                nearestPtr->point = Blt_InvMap2D(graphPtr, s->x1, s->y1,
                                                 &elemPtr->axes);
            } else if (d2 < nearestPtr->distance) {
                nearestPtr->index = i;
                nearestPtr->distance = d2;
                nearestPtr->item = isoPtr;
                nearestPtr->value = isoPtr->relValue;
                nearestPtr->point = Blt_InvMap2D(graphPtr, s->x2, s->y2,
                                                 &elemPtr->axes);
            }
        }
    }
}

static void
NearestSegment(Graph *graphPtr, NearestElement *nearestPtr)
{
    Blt_ChainLink link;

    for (link = Blt_Chain_FirstLink(graphPtr->isolines.displayList); 
         link != NULL; link = Blt_Chain_NextLink(link)) {
        Element *elemPtr;
        Isoline *isoPtr;
        IsolineSegment *s;
        int i;

        isoPtr = Blt_Chain_GetValue(link);
	if (isoPtr->flags & HIDDEN) {
	    continue;
	}
	elemPtr = isoPtr->elemPtr;
	if (elemPtr == NULL) {
	    continue;
	}
	if ((nearestPtr->elemPtr != NULL) && (nearestPtr->elemPtr != elemPtr)) {
	    continue;
	}
	if (elemPtr->flags & (HIDDEN|MAP_ITEM)) {
	    continue;
	}
        for (i = 0, s = isoPtr->segments; s != NULL; s = s->next, i++) {
            Point2d p1, p2, b;
            double d;
            
            p1.x = s->x1, p1.y = s->y1;
            p2.x = s->x2, p2.y = s->y2;
            d = DistanceToLine(nearestPtr->x, nearestPtr->y, &p1, &p2, &b);
            if (d < nearestPtr->distance) {
                nearestPtr->index = i;
                nearestPtr->distance = d;
                nearestPtr->item = isoPtr;
                nearestPtr->value = isoPtr->relValue;
                nearestPtr->point = Blt_InvMap2D(graphPtr, b.x, b.y,
                        &elemPtr->axes);
            }
        }
    }   
}

/* Isoline operations. */

/*
 *---------------------------------------------------------------------------
 *
 * ActivateOp --
 *
 *      Activates the given isolines in the element.
 *
 * Results:
 *      The return value is a standard TCL result. 
 *
 *      pathName isoline activate isoName
 *
 *---------------------------------------------------------------------------
 */
static int
ActivateOp(ClientData clientData, Tcl_Interp *interp, int objc, 
           Tcl_Obj *const *objv)
{
    Graph *graphPtr = clientData;
    IsolineIterator iter;
    Isoline *isoPtr;

    if (GetIsolineIterator(interp, graphPtr, objv[3], &iter) != TCL_OK) {
        return TCL_ERROR;
    }
    for (isoPtr = FirstTaggedIsoline(&iter); isoPtr != NULL; 
         isoPtr = NextTaggedIsoline(&iter)) {
        isoPtr->elemPtr->flags |= ACTIVE;
        isoPtr->flags |= ACTIVE;
    }
    graphPtr->flags |= (CACHE_DIRTY |  REDRAW_WORLD);
    Blt_EventuallyRedrawGraph(graphPtr);
    return TCL_OK;
}    

/*
 *---------------------------------------------------------------------------
 *
 * BindOp --
 *
 *      pathName isoline bind isoName sequence command
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
BindOp(ClientData clientData, Tcl_Interp *interp, int objc,
       Tcl_Obj *const *objv)
{
    Graph *graphPtr = clientData;

    if (objc == 4) {
        Blt_HashEntry *hPtr;
        Blt_HashSearch iter;
        Tcl_Obj *listObjPtr;

        listObjPtr = Tcl_NewListObj(0, (Tcl_Obj **)NULL);
        for (hPtr = Blt_FirstHashEntry(&graphPtr->isolines.bindTagTable, &iter);
             hPtr != NULL; hPtr = Blt_NextHashEntry(&iter)) {
            const char *tagName;
            Tcl_Obj *objPtr;

            tagName = Blt_GetHashKey(&graphPtr->isolines.bindTagTable, hPtr);
            objPtr = Tcl_NewStringObj(tagName, -1);
            Tcl_ListObjAppendElement(interp, listObjPtr, objPtr);
        }
        Tcl_SetObjResult(interp, listObjPtr);
        return TCL_OK;
    }
    return Blt_ConfigureBindingsFromObj(interp, graphPtr->bindTable,
        Blt_MakeIsolineTag(graphPtr, Tcl_GetString(objv[3])),
        objc - 4, objv + 4);
}

/*
 *---------------------------------------------------------------------------
 *
 * CgetOp --
 *
 *      pathName isoline cget isoName -option
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
CgetOp(ClientData clientData, Tcl_Interp *interp, int objc,
       Tcl_Obj *const *objv)
{
    Graph *graphPtr = clientData;
    Isoline *isoPtr;

    if (GetIsolineFromObj(interp, graphPtr, objv[3], &isoPtr) != TCL_OK) {
        return TCL_ERROR;               /* Can't find named isoline. */
    }
    return Blt_ConfigureValueFromObj(interp, graphPtr->tkwin, isolineSpecs,
        (char *)isoPtr, objv[4], 0);
}

/*
 *---------------------------------------------------------------------------
 *
 * ConfigureOp --
 *
 *      This procedure is called to process an objv/objc list, plus the Tk
 *      option database, in order to configure (or reconfigure) the
 *      isoline.
 *
 * Results:
 *      A standard TCL result.  If TCL_ERROR is returned, then the
 *      interpreter result will contain an error message.
 *
 * Side Effects:
 *      Configuration information, such as text string, colors, font,
 *      etc. get set for setPtr; old resources get freed, if there were
 *      any.  The widget is redisplayed.
 *
 *      pathName isoline configure isoName ?option value?...
 *
 *---------------------------------------------------------------------------
 */
static int
ConfigureOp(ClientData clientData, Tcl_Interp *interp, int objc, 
            Tcl_Obj *const *objv)
{
    Graph *graphPtr = clientData;
    Isoline *isoPtr;
    IsolineIterator iter;

    if (objc <= 5) {
        if (GetIsolineFromObj(interp, graphPtr, objv[3], &isoPtr) != TCL_OK) {
            return TCL_ERROR;
        }
        if (objc == 4) {
            return Blt_ConfigureInfoFromObj(interp, graphPtr->tkwin, 
                isolineSpecs, (char *)isoPtr, (Tcl_Obj *)NULL, 0);
        } else if (objc == 5) {
            return Blt_ConfigureInfoFromObj(interp, graphPtr->tkwin, 
                isolineSpecs, (char *)isoPtr, objv[4], 0);
        }
    }
    if (GetIsolineIterator(interp, graphPtr, objv[3], &iter) != TCL_OK) {
        return TCL_ERROR;
    }
    for (isoPtr = FirstTaggedIsoline(&iter); isoPtr != NULL; 
         isoPtr = NextTaggedIsoline(&iter)) {
        if (ConfigureIsoline(interp, isoPtr, objc - 4, objv + 4, 
                BLT_CONFIG_OBJV_ONLY) != TCL_OK) {
            return TCL_ERROR;
        }
    }
    graphPtr->flags |= (CACHE_DIRTY | REDRAW_WORLD);
    Blt_EventuallyRedrawGraph(graphPtr);
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * CreateOp --
 *
 *      Creates a isoline for the named element.
 *
 * Results:
 *      The return value is a standard TCL result. 
 *
 *      pathName isoline create ?isoName? ?option value?...
 *
 *---------------------------------------------------------------------------
 */
static int
CreateOp(ClientData clientData, Tcl_Interp *interp, int objc, 
         Tcl_Obj *const *objv)
{
    Graph *graphPtr = clientData;
    Isoline *isoPtr;
    const char *name;
    char ident[200];

    name = NULL;
    if (objc > 3) {
        const char *string;

        string = Tcl_GetString(objv[3]);
        if (string[0] != '-') {
            if (GetIsolineFromObj(NULL, graphPtr, objv[3], &isoPtr) == TCL_OK) {
                Tcl_AppendResult(interp, "isoline \"", string, 
                        "\" already exists", (char *)NULL);
                return TCL_ERROR;
            }
            name = string;
            objc--, objv++;
        }
    }
    if (name == NULL) {
        Blt_FmtString(ident, 200, "isoline%d", graphPtr->nextIsolineId++);
        name = ident;
    }
    isoPtr = NewIsoline(interp, graphPtr, name);
    if (isoPtr == NULL) {
        return TCL_ERROR;
    }

    if (ConfigureIsoline(interp, isoPtr, objc - 3, objv + 3, 0) != TCL_OK) {
        DestroyIsoline(isoPtr);
        return TCL_ERROR;
    }
    graphPtr->flags |= REDRAW_WORLD;
    Blt_EventuallyRedrawGraph(graphPtr);
    Tcl_SetStringObj(Tcl_GetObjResult(interp), isoPtr->obj.name, -1);
    return TCL_OK;
}    

/*
 *---------------------------------------------------------------------------
 *
 * DeactivateOp --
 *
 *      Deactivates the given isoline in the element.
 *
 * Results:
 *      The return value is a standard TCL result. 
 *
 *      pathName isoline deactivate isoName
 *
 *---------------------------------------------------------------------------
 */
static int
DeactivateOp(ClientData clientData, Tcl_Interp *interp, int objc, 
                    Tcl_Obj *const *objv)
{
    Graph *graphPtr = clientData;
    IsolineIterator iter;
    Isoline *isoPtr;

    if (GetIsolineIterator(interp, graphPtr, objv[3], &iter) != TCL_OK) {
        return TCL_ERROR;
    }
    for (isoPtr = FirstTaggedIsoline(&iter); isoPtr != NULL; 
         isoPtr = NextTaggedIsoline(&iter)) {
        isoPtr->flags &= ~ACTIVE;
        isoPtr->elemPtr->flags &= ~ACTIVE;
    }
    graphPtr->flags |= (CACHE_DIRTY | REDRAW_WORLD);
    Blt_EventuallyRedrawGraph(graphPtr);
    return TCL_OK;
}    


/*
 *---------------------------------------------------------------------------
 *
 * DeleteOp --
 *
 *      Deletes one or more isolines from the named element.
 *
 * Results:
 *      The return value is a standard TCL result. 
 *
 *      pathName isoline delete ?isoName...?
 *
 *---------------------------------------------------------------------------
 */
static int
DeleteOp(ClientData clientData, Tcl_Interp *interp, int objc, 
         Tcl_Obj *const *objv)
{
    Graph *graphPtr = clientData;
    Blt_HashTable deleteTable;
    Blt_HashEntry *hPtr;
    Blt_HashSearch iter;
    int i;

    Blt_InitHashTable(&deleteTable, BLT_ONE_WORD_KEYS);
    for (i = 3; i < objc; i++) {
        IsolineIterator iter;
        Isoline *isoPtr;
        
        if (GetIsolineIterator(interp, graphPtr, objv[i], &iter) != TCL_OK) {
            return TCL_ERROR;
        }
        for (isoPtr = FirstTaggedIsoline(&iter); isoPtr != NULL; 
             isoPtr = NextTaggedIsoline(&iter)) {
            Blt_HashEntry *hPtr;
            int isNew;

            hPtr = Blt_CreateHashEntry(&deleteTable, (char *)isoPtr, &isNew);
            if (isNew) {
                Blt_SetHashValue(hPtr, isoPtr);
            }
        }
    }
    for (hPtr = Blt_FirstHashEntry(&deleteTable, &iter); hPtr != NULL;
         hPtr = Blt_NextHashEntry(&iter)) {
        Isoline *isoPtr;

        isoPtr = Blt_GetHashValue(hPtr);
        DestroyIsoline(isoPtr);
    }
    Blt_DeleteHashTable(&deleteTable);
    graphPtr->flags |= REDRAW_WORLD;
    Blt_EventuallyRedrawGraph(graphPtr);
    return TCL_OK;
}    

/*
 *---------------------------------------------------------------------------
 *
 * ExistsOp --
 *
 *      Indicates if a isoline by the given name exists in the element.
 *
 * Results:
 *      The return value is a standard TCL result. 
 *
 *      pathName isoline exists isoName
 *
 *---------------------------------------------------------------------------
 */
static int
ExistsOp(ClientData clientData, Tcl_Interp *interp, int objc, 
         Tcl_Obj *const *objv)
{
    Graph *graphPtr = clientData;
    Isoline *isoPtr;
    int bool;

    bool = (GetIsolineFromObj(NULL, graphPtr, objv[3], &isoPtr) == TCL_OK);
    Tcl_SetBooleanObj(Tcl_GetObjResult(interp), bool);
    return TCL_OK;
}    

/*
 *---------------------------------------------------------------------------
 *
 * NamesOp --
 *
 *      Returns the names of the isolines in the element matching one of
 *      the patterns provided.  If no pattern arguments are given, then
 *      all isoline names will be returned.
 *
 * Results:
 *      The return value is a standard TCL result. The interpreter result
 *      will contain a TCL list of the isoline names.
 *
 *      pathName isoline names ?pattern...?
 *
 *---------------------------------------------------------------------------
 */
static int
NamesOp(ClientData clientData, Tcl_Interp *interp, int objc, 
        Tcl_Obj *const *objv)
{
    Tcl_Obj *listObjPtr;
    Graph *graphPtr = clientData;

    listObjPtr = Tcl_NewListObj(0, (Tcl_Obj **)NULL);
    if (objc == 3) {
        Blt_ChainLink link;

        for (link = Blt_Chain_FirstLink(graphPtr->isolines.displayList); 
             link != NULL; link = Blt_Chain_NextLink(link)) {
            Isoline *isoPtr;
            Tcl_Obj *objPtr;

            isoPtr = Blt_Chain_GetValue(link);
            objPtr = Tcl_NewStringObj(isoPtr->obj.name, -1);
            Tcl_ListObjAppendElement(interp, listObjPtr, objPtr);
        }
    } else {
        Blt_ChainLink link;

        for (link = Blt_Chain_FirstLink(graphPtr->isolines.displayList); 
             link != NULL; link = Blt_Chain_NextLink(link)) {
            Isoline *isoPtr;
            int i;

            isoPtr = Blt_Chain_GetValue(link);
            for (i = 3; i < objc; i++) {
                const char *pattern;

                pattern = Tcl_GetString(objv[i]);
                if (Tcl_StringMatch(isoPtr->obj.name, pattern)) {
                    Tcl_Obj *objPtr;

                    objPtr = Tcl_NewStringObj(isoPtr->obj.name, -1);
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
 * NearestOp --
 *
 *      Finds the isoline in the given element closest to the given screen
 *      coordinates.
 *
 * Results:
 *      The return value is a standard TCL result. 
 *
 *      pathName isoline nearest x y ?option value?...
 *
 *---------------------------------------------------------------------------
 */
static Blt_ConfigSpec nearestSpecs[] = {
    {BLT_CONFIG_PIXELS_NNEG, "-halo", (char *)NULL, (char *)NULL,
        (char *)NULL, Blt_Offset(NearestElement, halo), 0},
    {BLT_CONFIG_BOOLEAN, "-interpolate", (char *)NULL, (char *)NULL,
        (char *)NULL, Blt_Offset(NearestElement, mode), 0 }, 
    {BLT_CONFIG_END, (char *)NULL, (char *)NULL, (char *)NULL,
        (char *)NULL, 0, 0}
};

static int
NearestOp(ClientData clientData, Tcl_Interp *interp, int objc, 
          Tcl_Obj *const *objv)
{
    Graph *graphPtr = clientData;
    NearestElement nearest;
    int x, y;

    if (graphPtr->flags & RESET_AXES) {
        Blt_ResetAxes(graphPtr);
    }
    if (Tcl_GetIntFromObj(interp, objv[3], &x) != TCL_OK) {
        Tcl_AppendResult(interp, ": bad window x-coordinate", (char *)NULL);
        return TCL_ERROR;
    }
    if (Tcl_GetIntFromObj(interp, objv[4], &y) != TCL_OK) {
        Tcl_AppendResult(interp, ": bad window y-coordinate", (char *)NULL);
        return TCL_ERROR;
    }
    memset(&nearest, 0, sizeof(NearestElement));
    if (Blt_ConfigureWidgetFromObj(interp, graphPtr->tkwin, nearestSpecs, 
        objc - 5, objv + 5, (char *)&nearest, BLT_CONFIG_OBJV_ONLY) != TCL_OK) {
        return TCL_ERROR;               /* Error occurred processing an
                                         * option. */
    }
    if (nearest.halo == 0) {
        nearest.halo = graphPtr->halo;
    }
    nearest.maxDistance = nearest.halo + 1;
    nearest.distance = nearest.maxDistance + 1;
    nearest.along = NEAREST_SEARCH_XY;
    nearest.x = x;
    nearest.y = y;

    if (nearest.mode == NEAREST_SEARCH_POINTS) {
        NearestPoint(graphPtr, &nearest);
    } else {
        int found;
        
        NearestSegment(graphPtr, &nearest);
        found = (nearest.distance <= nearest.maxDistance);
        if ((!found) && (nearest.along != NEAREST_SEARCH_XY)) {
            NearestPoint(graphPtr, &nearest);
        }
    }
    if (nearest.distance <= nearest.maxDistance) {
        Tcl_Obj *objPtr, *listObjPtr;   /* Return a list of name value
                                         * pairs. */
        Isoline *isoPtr;

        listObjPtr = Tcl_NewListObj(0, (Tcl_Obj **)NULL);

        isoPtr = nearest.item;
        /* Name of isoline. */
        objPtr = Tcl_NewStringObj("name", 4);
        Tcl_ListObjAppendElement(interp, listObjPtr, objPtr);
        objPtr = Tcl_NewStringObj(isoPtr->obj.name, -1); 
        Tcl_ListObjAppendElement(interp, listObjPtr, objPtr);

        /* Value of isoline. */
        objPtr = Tcl_NewStringObj("value", 5);
        Tcl_ListObjAppendElement(interp, listObjPtr, objPtr);
        objPtr = Tcl_NewDoubleObj(nearest.value);
        Tcl_ListObjAppendElement(interp, listObjPtr, objPtr);

        /* X-coordinate of nearest point on isoline. */
        objPtr = Tcl_NewStringObj("x", 1);
        Tcl_ListObjAppendElement(interp, listObjPtr, objPtr);
        objPtr = Tcl_NewDoubleObj(nearest.point.x);
        Tcl_ListObjAppendElement(interp, listObjPtr, objPtr);

        /* Y-coordinate of nearest point on isoline. */
        objPtr = Tcl_NewStringObj("y", 1);
        Tcl_ListObjAppendElement(interp, listObjPtr, objPtr);
        objPtr = Tcl_NewDoubleObj(nearest.point.y);
        Tcl_ListObjAppendElement(interp, listObjPtr, objPtr);

        /* Distance to from search point. */
        objPtr = Tcl_NewStringObj("dist", 4);
        Tcl_ListObjAppendElement(interp, listObjPtr, objPtr);
        objPtr = Tcl_NewDoubleObj(nearest.distance);
        Tcl_ListObjAppendElement(interp, listObjPtr, objPtr);

        /* Distance to from search point. */
        objPtr = Tcl_NewStringObj("index", 5);
        Tcl_ListObjAppendElement(interp, listObjPtr, objPtr);
        objPtr = Tcl_NewIntObj(nearest.index);
        Tcl_ListObjAppendElement(interp, listObjPtr, objPtr);

        Tcl_SetObjResult(interp, listObjPtr);
    }
    return TCL_OK;
}    


/*
 *---------------------------------------------------------------------------
 *
 * StepsOp --
 *
 *      Generates the given number of evenly placed isolines in the
 *      element.
 *
 * Results:
 *      The return value is a standard TCL result. 
 *
 *      pathName isoline steps numSteps ?option value?...
 *
 *---------------------------------------------------------------------------
 */
static int
StepsOp(ClientData clientData, Tcl_Interp *interp, int objc, 
        Tcl_Obj *const *objv)
{
    Graph *graphPtr = clientData;
    long numSteps, i;

    if (Blt_GetCountFromObj(interp, objv[3], COUNT_POS, &numSteps) != TCL_OK) {
        return TCL_ERROR;
    }
    if (numSteps < 2) {
        Tcl_AppendResult(interp, "two few steps: must >= 2", (char *)NULL);
        return TCL_ERROR;
    }
    for (i = 0; i < numSteps; i++) {
        Isoline *isoPtr;

        isoPtr = NewIsoline(interp, graphPtr, NULL);
        if (isoPtr == NULL) {
            return TCL_ERROR;
        }
        isoPtr->reqValue = (double)i / (double)(numSteps - 1);
        if (ConfigureIsoline(interp, isoPtr, objc - 4, objv + 4, 0) != TCL_OK) {
            DestroyIsoline(isoPtr);
            return TCL_ERROR;
        }
    }
    graphPtr->flags |= REDRAW_WORLD;
    Blt_EventuallyRedrawGraph(graphPtr);
    return TCL_OK;
}    

/* Isoline tag api */

/*
 *---------------------------------------------------------------------------
 *
 * TagAddOp --
 *
 *      pathName isoline tag add tagName ?isoName...?
 *
 *---------------------------------------------------------------------------
 */
static int
TagAddOp(ClientData clientData, Tcl_Interp *interp, int objc, 
         Tcl_Obj *const *objv)
{
    Graph *graphPtr = clientData;
    const char *tag;

    tag = Tcl_GetString(objv[4]);
    if (strcmp(tag, "all") == 0) {
        Tcl_AppendResult(interp, "can't add reserved tag \"", tag, "\"", 
                         (char *)NULL);
        return TCL_ERROR;
    }
    if (objc == 5) {
        /* No nodes specified.  Just add the tag. */
        Blt_Tags_AddTag(&graphPtr->isolines.tags, tag);
    } else {
        int i;

        for (i = 5; i < objc; i++) {
            Isoline *isoPtr;
            IsolineIterator iter;
            
            if (GetIsolineIterator(interp, graphPtr, objv[i], &iter) !=TCL_OK) {
                return TCL_ERROR;
            }
            for (isoPtr = FirstTaggedIsoline(&iter); isoPtr != NULL; 
                 isoPtr = NextTaggedIsoline(&iter)) {
                Blt_Tags_AddItemToTag(&graphPtr->isolines.tags, tag, isoPtr);
            }
        }
    }
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * TagDeleteOp --
 *
 *      pathName isoline tag delete tagName ?isoName...?
 *
 *---------------------------------------------------------------------------
 */
static int
TagDeleteOp(ClientData clientData, Tcl_Interp *interp, int objc, 
            Tcl_Obj *const *objv)
{
    Graph *graphPtr = clientData;
    const char *tag;
    int i;

    tag = Tcl_GetString(objv[4]);
    if (strcmp(tag, "all") == 0) {
        Tcl_AppendResult(interp, "can't delete reserved tag \"", tag, "\"", 
                         (char *)NULL);
        return TCL_ERROR;
    }
    for (i = 5; i < objc; i++) {
        Isoline *isoPtr;
        IsolineIterator iter;
        
        if (GetIsolineIterator(interp, graphPtr, objv[i], &iter) != TCL_OK) {
            return TCL_ERROR;
        }
        for (isoPtr = FirstTaggedIsoline(&iter); isoPtr != NULL; 
             isoPtr = NextTaggedIsoline(&iter)) {
            Blt_Tags_RemoveItemFromTag(&graphPtr->isolines.tags, tag, isoPtr);
        }
    }
    return TCL_OK;
}


/*
 *---------------------------------------------------------------------------
 *
 * TagExistsOp --
 *
 *      Returns the existence of the one or more tags in the given isoline.
 *      If the isoline has any the tags, true is returned in the
 *      interpreter.
 *
 *      pathName isoline tag exists isoName ?tagName...?
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
TagExistsOp(ClientData clientData, Tcl_Interp *interp, int objc, 
            Tcl_Obj *const *objv)
{
    Graph *graphPtr = clientData;
    IsolineIterator iter;
    int i;

    if (GetIsolineIterator(interp, graphPtr, objv[4], &iter) != TCL_OK) {
        return TCL_ERROR;
    }
    for (i = 5; i < objc; i++) {
        const char *tag;
        Isoline *isoPtr;

        tag = Tcl_GetString(objv[i]);
        if (strcmp(tag, "all") == 0) {
            Tcl_SetBooleanObj(Tcl_GetObjResult(interp), TRUE);
            return TCL_OK;
        }
        for (isoPtr = FirstTaggedIsoline(&iter); isoPtr != NULL; 
             isoPtr = NextTaggedIsoline(&iter)) {
            if (Blt_Tags_ItemHasTag(&graphPtr->isolines.tags, isoPtr, tag)) {
                Tcl_SetBooleanObj(Tcl_GetObjResult(interp), TRUE);
                return TCL_OK;
            }
        }
    }
    Tcl_SetBooleanObj(Tcl_GetObjResult(interp), FALSE);
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * TagForgetOp --
 *
 *      Removes the given tags from all isolines in the graph.
 *
 *      pathName isoline tag forget ?tagName...?
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
TagForgetOp(ClientData clientData, Tcl_Interp *interp, int objc, 
            Tcl_Obj *const *objv)
{
    Graph *graphPtr = clientData;
    int i;

    for (i = 4; i < objc; i++) {
        const char *tag;

        tag = Tcl_GetString(objv[i]);
        if (strcmp(tag, "all") == 0) {
            continue;                   /* Can't remove tag "all". */
        }
        Blt_Tags_ForgetTag(&graphPtr->isolines.tags, tag);
    }
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * TagGetOp --
 *
 *      Returns tag names for a given isoline.  If one of more pattern
 *      arguments are provided, then only those matching tags are returned.
 *
 *      pathName isoline tag get isoName pat1 pat2...
 *
 *---------------------------------------------------------------------------
 */
static int
TagGetOp(ClientData clientData, Tcl_Interp *interp, int objc, 
         Tcl_Obj *const *objv)
{
    Graph *graphPtr = clientData;
    Isoline *isoPtr;
    IsolineIterator iter;
    Tcl_Obj *listObjPtr;

    if (GetIsolineIterator(interp, graphPtr, objv[4], &iter) != TCL_OK) {
        return TCL_ERROR;
    }
    listObjPtr = Tcl_NewListObj(0, (Tcl_Obj **) NULL);
    for (isoPtr = FirstTaggedIsoline(&iter); isoPtr != NULL; 
         isoPtr = NextTaggedIsoline(&iter)) {
        if (objc == 5) {
            Blt_Tags_AppendTagsToObj(&graphPtr->isolines.tags, isoPtr,
				     listObjPtr);
            Tcl_ListObjAppendElement(interp, listObjPtr, 
                Tcl_NewStringObj("all", 3));
        } else {
            int i;
            
            /* Check if we need to add the special tags "all" */
            for (i = 5; i < objc; i++) {
                const char *pattern;

                pattern = Tcl_GetString(objv[i]);
                if (Tcl_StringMatch("all", pattern)) {
                    Tcl_Obj *objPtr;

                    objPtr = Tcl_NewStringObj("all", 3);
                    Tcl_ListObjAppendElement(interp, listObjPtr, objPtr);
                    break;
                }
            }
            /* Now process any standard tags. */
            for (i = 5; i < objc; i++) {
                Blt_ChainLink link;
                const char *pattern;
                Blt_Chain chain;

                chain = Blt_Chain_Create();
                Blt_Tags_AppendTagsToChain(&graphPtr->isolines.tags, isoPtr,
			chain);
                pattern = Tcl_GetString(objv[i]);
                for (link = Blt_Chain_FirstLink(chain); link != NULL; 
                     link = Blt_Chain_NextLink(link)) {
                    const char *tag;
                    Tcl_Obj *objPtr;

                    tag = (const char *)Blt_Chain_GetValue(link);
                    if (!Tcl_StringMatch(tag, pattern)) {
                        continue;
                    }
                    objPtr = Tcl_NewStringObj(tag, -1);
                    Tcl_ListObjAppendElement(interp, listObjPtr, objPtr);
                }
                Blt_Chain_Destroy(chain);
            }
        }    
    }
    Tcl_SetObjResult(interp, listObjPtr);
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * TagNamesOp --
 *
 *      Returns the names of all the tags for the isoline.  If one of more
 *      isoline arguments are provided, then only the tags found in those
 *      isolines are returned.
 *
 *      pathName isoline tag names ?isoName...?
 *
 *---------------------------------------------------------------------------
 */
static int
TagNamesOp(ClientData clientData, Tcl_Interp *interp, int objc, 
              Tcl_Obj *const *objv)
{
    Graph *graphPtr = clientData;
    Tcl_Obj *listObjPtr, *objPtr;

    listObjPtr = Tcl_NewListObj(0, (Tcl_Obj **) NULL);
    objPtr = Tcl_NewStringObj("all", -1);
    Tcl_ListObjAppendElement(interp, listObjPtr, objPtr);
    if (objc == 4) {
        Blt_Tags_AppendAllTagsToObj(&graphPtr->isolines.tags, listObjPtr);
    } else {
        Blt_HashTable uniqTable;
        int i;

        Blt_InitHashTable(&uniqTable, BLT_STRING_KEYS);
        for (i = 4; i < objc; i++) {
            IsolineIterator iter;
            Isoline *isoPtr;

            if (GetIsolineIterator(interp, graphPtr, objPtr, &iter) != TCL_OK) {
                goto error;
            }
            for (isoPtr = FirstTaggedIsoline(&iter); isoPtr != NULL; 
                 isoPtr = NextTaggedIsoline(&iter)) {
                Blt_ChainLink link;
                Blt_Chain chain;

                chain = Blt_Chain_Create();
                Blt_Tags_AppendTagsToChain(&graphPtr->isolines.tags, isoPtr,
			chain);
                for (link = Blt_Chain_FirstLink(chain); link != NULL; 
                     link = Blt_Chain_NextLink(link)) {
                    const char *tag;
                    int isNew;

                    tag = Blt_Chain_GetValue(link);
                    Blt_CreateHashEntry(&uniqTable, tag, &isNew);
                }
                Blt_Chain_Destroy(chain);
            }
        }
        {
            Blt_HashEntry *hPtr;
            Blt_HashSearch hiter;

            for (hPtr = Blt_FirstHashEntry(&uniqTable, &hiter); hPtr != NULL;
                 hPtr = Blt_NextHashEntry(&hiter)) {
                objPtr = Tcl_NewStringObj(Blt_GetHashKey(&uniqTable, hPtr), -1);
                Tcl_ListObjAppendElement(interp, listObjPtr, objPtr);
            }
        }
        Blt_DeleteHashTable(&uniqTable);
    }
    Tcl_SetObjResult(interp, listObjPtr);
    return TCL_OK;
 error:
    Tcl_DecrRefCount(listObjPtr);
    return TCL_ERROR;
}

/*
 *---------------------------------------------------------------------------
 *
 * TagSetOp --
 *
 *      Sets one or more tags for a given isoline.  Tag names can't start
 *      with a digit and can't be a reserved tag ("all").
 *
 *      pathName isoline tag set isoName tag1 tag2...
 *
 *---------------------------------------------------------------------------
 */
static int
TagSetOp(ClientData clientData, Tcl_Interp *interp, int objc, 
            Tcl_Obj *const *objv)
{
    Graph *graphPtr = clientData;
    IsolineIterator iter;
    Isoline *isoPtr;

    if (GetIsolineIterator(interp, graphPtr, objv[4], &iter) != TCL_OK) {
        return TCL_ERROR;
    }
    for (isoPtr = FirstTaggedIsoline(&iter); isoPtr != NULL; 
         isoPtr = NextTaggedIsoline(&iter)) {
        int i;

        for (i = 5; i < objc; i++) {
            const char *tag;
            
            tag = Tcl_GetString(objv[i]);
            if (strcmp(tag, "all") == 0) {
                continue;
            }
            Blt_Tags_AddItemToTag(&graphPtr->isolines.tags, tag, isoPtr);
        }
    }    
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * TagUnsetOp --
 *
 *      Removes one or more tags from a given isoline. If a isoline doesn't
 *      exist or is a reserved tag ("all"), nothing will be done and no
 *      error message will be returned.
 *
 *      pathName isoline tag unset isoName tag1 tag2...
 *
 *---------------------------------------------------------------------------
 */
static int
TagUnsetOp(ClientData clientData, Tcl_Interp *interp, int objc, 
	   Tcl_Obj *const *objv)
{
    Graph *graphPtr = clientData;
    Isoline *isoPtr;
    IsolineIterator iter;

    if (GetIsolineIterator(interp, graphPtr, objv[4], &iter) != TCL_OK) {
        return TCL_ERROR;
    }
    for (isoPtr = FirstTaggedIsoline(&iter); isoPtr != NULL; 
         isoPtr = NextTaggedIsoline(&iter)) {
        int i;

        for (i = 5; i < objc; i++) {
            const char *tag;

            tag = Tcl_GetString(objv[i]);
            Blt_Tags_RemoveItemFromTag(&graphPtr->isolines.tags, tag, isoPtr);
        }    
    }
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * TagOp --
 *
 *      This procedure is invoked to process tag operations.
 *
 * Results:
 *      A standard TCL result.
 *
 * Side Effects:
 *      See the user documentation.
 *
 *      pathName isoline tag op args...
 *
 *---------------------------------------------------------------------------
 */
static Blt_OpSpec tagOps[] =
{
    {"add",     1, TagAddOp,      5, 0, "isoName ?tagName...?",},
    {"delete",  1, TagDeleteOp,   5, 0, "isoName ?tagName...?",},
    {"exists",  1, TagExistsOp,   5, 0, "isoName ?tagName...?",},
    {"forget",  1, TagForgetOp,   4, 0, "?tagName...?",},
    {"get",     1, TagGetOp,      5, 0, "isoName ?pattern...?",},
    {"names",   1, TagNamesOp,    4, 0, "?tagName...?",},
    {"set",     1, TagSetOp,      5, 0, "isoName ?tagName...",},
    {"unset",   1, TagUnsetOp,    5, 0, "isoName ?tagName...",},
};

static int numTagOps = sizeof(tagOps) / sizeof(Blt_OpSpec);

static int
TagOp(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *const *objv)
{
    Tcl_ObjCmdProc *proc;
    int result;

    proc = Blt_GetOpFromObj(interp, numTagOps, tagOps, BLT_OP_ARG3,
                            objc, objv, 0);
    if (proc == NULL) {
        return TCL_ERROR;
    }
    result = (*proc)(clientData, interp, objc, objv);
    return result;
}

/*
 *---------------------------------------------------------------------------
 *
 * IsolineOp --
 *
 *	pathName isoline activate isoName
 *	pathName isoline bind sequence command
 *	pathName isoline cget isoName option
 *	pathName isoline configure isoName ?option value...?
 *	pathName isoline create ?isoName? ?option value...?
 *	pathName isoline create ?isoName? ?option value...?
 *	pathName isoline deactivate isoName
 *	pathName isoline delete ?isoName...?
 *	pathName isoline exists isoName
 *	pathName isoline names ?pattern ...?
 *	pathName isoline nearest x y ?switches?
 *	pathName isoline steps count ?option values...?
 *	pathName isoline tag args...
 *
 *---------------------------------------------------------------------------
 */
static Blt_OpSpec isolineOps[] = {
    {"activate",   1, ActivateOp,   4, 4, "isoName",},
    {"bind",       1, BindOp,       3, 6, "sequence command"},
    {"cget",       2, CgetOp,       5, 5, "isoName option",},
    {"configure",  2, ConfigureOp,  4, 0, "isoName ?option value ...?",},
    {"create",     2, CreateOp,     3, 0, "?isoName? ?option value ...?",},
    {"deactivate", 3, DeactivateOp, 4, 4, "isoName",},
    {"delete",     3, DeleteOp,     3, 0, "?isoName ...?",},
    {"exists",     1, ExistsOp,     4, 4, "isoName",},
    {"names",      2, NamesOp,      3, 0, "?pattern ...?",},
    {"nearest",    2, NearestOp,    5, 0, "x y ?switches?",},
    {"steps",      1, StepsOp,      4, 0, "numSteps ?option value ...?",},
    {"tag",        1, TagOp,        3, 0, "args",},
};

static int numIsolineOps = sizeof(isolineOps) / sizeof(Blt_OpSpec);

/*ARGSUSED*/
int
Blt_IsolineOp(ClientData clientData, Tcl_Interp *interp, int objc, 
              Tcl_Obj *const *objv)
{
    Tcl_ObjCmdProc *proc;

    proc = Blt_GetOpFromObj(interp, numIsolineOps, isolineOps, BLT_OP_ARG2, 
        objc, objv, 0);
    if (proc == NULL) {
        return TCL_ERROR;
    }
    return (*proc) (clientData, interp, objc, objv);
}


void
Blt_ClearIsolines(Graph *graphPtr, Element *elemPtr)
{
    Blt_HashEntry *hPtr;
    Blt_HashSearch iter;

    for (hPtr = Blt_FirstHashEntry(&graphPtr->isolines.nameTable, &iter);
         hPtr != NULL; hPtr = Blt_NextHashEntry(&iter)) {
        Isoline *isoPtr;
        
        isoPtr = Blt_GetHashValue(hPtr);
        if ((isoPtr->elemPtr != NULL) && (isoPtr->elemPtr == elemPtr)) {
            isoPtr->elemPtr = NULL; 
        }
    }
}

/*
 *---------------------------------------------------------------------------
 *
 * Blt_DestroyIsolines --
 *
 *---------------------------------------------------------------------------
 */
void
Blt_DestroyIsolines(Graph *graphPtr)
{
    Blt_HashEntry *hPtr;
    Blt_HashSearch cursor;
    
    for (hPtr = Blt_FirstHashEntry(&graphPtr->isolines.nameTable, &cursor);
         hPtr != NULL; hPtr = Blt_NextHashEntry(&cursor)) {
        Isoline *isoPtr;
        
        isoPtr = Blt_GetHashValue(hPtr);
        isoPtr->hashPtr = NULL;
        DestroyIsoline(isoPtr);
    }
    Blt_DeleteHashTable(&graphPtr->isolines.nameTable);
    Blt_DeleteHashTable(&graphPtr->isolines.bindTagTable);
    Blt_Chain_Destroy(graphPtr->isolines.displayList);
}


/*
 *  DestroyIsoline - notify elements to remove from list.
 *  ConfigureIsoline - notify old and new elements.
 *  NewIsoline - notify new element.
 */


Isoline *
Blt_NearestIsoline(Graph *graphPtr, int x, int y)
{
    NearestElement nearest;
    Isoline *isoPtr;
    
    memset(&nearest, 0, sizeof(NearestElement));
    nearest.halo = graphPtr->halo;
    nearest.maxDistance = nearest.halo + 1;
    nearest.distance = nearest.maxDistance + 1;
    nearest.along = NEAREST_SEARCH_XY;
    nearest.x = x;
    nearest.y = y;
    NearestSegment(graphPtr, &nearest);
    isoPtr = nearest.item;
    return isoPtr;
}

#ifdef notdef
/* 
 * For labeling:  
 *      Create polylines from list of isoline segments.  
 *      Simplify each polyline to produce the longest segment.  
 *      Place rotated labels above/below left/right from the center of
 *      the longest segment.
 */
static void
StitchSegments(Isoline *isoPtr)
{
    long count;
    long i;
    Blt_HashTable pointTable;
    Blt_HashEntry *hPtr;
    Blt_HashSearch iter;
    IsolineSegment *s;
    
    Blt_InitHashTable(&pointTable, sizeof(PointKey) / sizeof(int));
    for (i = 0, s = isoPtr->segments; s != NULL; s = s->next, i++) {
        PointKey key;
        int isNew;
        
        if (s->x1 == s->x2 && s->y1 == s->y2) {
            continue;
        }
        MakePointKey(&key, s->x1, s->y1);
        hPtr = Blt_CreateHashEntry(&pointTable, &key, &isNew);
#ifdef notdef
        fprintf(stderr, "segment %ld px=%.15g py=%.15g qx=%.15g qy=%.15g\n",
                i, s->x1, s->y1, s->x2, s->y2);
#endif
        if (isNew) {
            count = 1;
        } else {
            count = Blt_GetHashValue(hPtr);
            count++;
        }
        Blt_SetHashValue(hPtr, count);
        key.x = s->x2;
        key.y = s->y2;
        hPtr = Blt_CreateHashEntry(&pointTable, &key, &isNew);
        if (isNew) {
            count = 1;
        } else {
            count = Blt_GetHashValue(hPtr);
            count++;
        }
        Blt_SetHashValue(hPtr, count);
    }
    i = 0;
    for (hPtr = Blt_FirstHashEntry(&pointTable, &iter); hPtr != NULL;
         hPtr = Blt_NextHashEntry(&iter)) {
        PointKey *keyPtr;
        keyPtr = Blt_GetHashKey(&pointTable, hPtr);
        count = Blt_GetHashValue(hPtr);
#ifdef notdef
        if (count == 1) {
            fprintf(stderr, "point %ld x=%.15g y=%.15g count=%ld\n",
                i, keyPtr->x, keyPtr->y, count);
        }
#endif
        i++;
    }
    Blt_DeleteHashTable(&pointTable);
}

typedef struct {
    Blt_Pool pool;                      /* Pool of points. */
    Blt_HashTable edgeTable;            /* Hashtable of edges */
    int numPoints;
} Stitches;

static Stitch *
NewStitch(Stitches *stitchesPtr, Point2d *p)
{
    stitchPtr = Blt_Pool_AllocItem(stitchesPtr->pool, sizeof(Stitch));
    stitchPtr->x = p.x;
    stitchPtr->y = p.y;
    stitchPtr->next = stitchPtr->last = NULL;
    stitchesPtr->numPoints++;
    return stitchPtr;
}

static void
FreeStitch(Stitches *stitchesPtr, pool, Stitch *s)
{
    Blt_Pool_FreeItem(stitchesPtr->pool, s);
    stitchesPtr->numPoints--;
}

#endif
