
/*
 * bltBind.h --
 *
 *	Copyright 1998-2004 George A Howlett.
 *
 *	Permission is hereby granted, free of charge, to any person obtaining
 *	a copy of this software and associated documentation files (the
 *	"Software"), to deal in the Software without restriction, including
 *	without limitation the rights to use, copy, modify, merge, publish,
 *	distribute, sublicense, and/or sell copies of the Software, and to
 *	permit persons to whom the Software is furnished to do so, subject to
 *	the following conditions:
 *
 *	The above copyright notice and this permission notice shall be
 *	included in all copies or substantial portions of the Software.
 *
 *	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 *	EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 *	MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 *	NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
 *	LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
 *	OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
 *	WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#ifndef _BLT_BIND_H
#define _BLT_BIND_H

typedef struct _Blt_BindTable *Blt_BindTable;

typedef ClientData (Blt_BindPickProc)(ClientData clientData, int x, int y, 
	ClientData *hintPtr);

typedef void (Blt_BindAppendTagsProc)(Blt_BindTable bindTable, 
	ClientData object, ClientData hint, Blt_Chain tags);


/*
 *  Binding structure information:
 */

struct _Blt_BindTable {
    unsigned int flags;
    Tk_BindingTable bindingTable;	/* Table of all bindings currently
					 * defined.  NULL means that no
					 * bindings exist, so the table hasn't
					 * been created.  Each "object" used
					 * for this table is either a Tk_Uid
					 * for a tag or the address of an item
					 * named by id. */
    ClientData currentObj;		/* The item currently containing the
					 * mouse pointer, or NULL if none. */
    ClientData currentHint;		/* One word indicating what kind of
					 * object was picked. */
    ClientData newObj;			/* The item that is about to become
					 * the current one, or NULL.  This
					 * field is used to detect deletions
					 * of the new current item pointer
					 * that occur during Leave processing
					 * of the previous current tab. */
    ClientData newHint;			/* One-word indicating what kind of
					 * object was just picked. */
    ClientData focusObj;
    ClientData focusHint;
    XEvent pickEvent;			/* The event upon which the current
					 * choice of the current tab is based.
					 * Must be saved so that if the
					 * current item is deleted, we can
					 * pick another. */
    int activePick;			/* The pick event has been initialized
					 * so that we can repick it */
    int state;				/* Last known modifier state.  Used to
					 * defer picking a new current object
					 * while buttons are down. */
    ClientData clientData;
    Tk_Window tkwin;
    Blt_BindPickProc *pickProc;		/* Routine to report the item the
					 * mouse is currently over. */
    Blt_BindAppendTagsProc *tagProc;	/* Routine to report tags of picked
					 * items. */
};

BLT_EXTERN void Blt_DestroyBindingTable(Blt_BindTable table);

BLT_EXTERN Blt_BindTable Blt_CreateBindingTable(Tcl_Interp *interp, 
	Tk_Window tkwin, ClientData clientData, Blt_BindPickProc *pickProc,
	Blt_BindAppendTagsProc *tagProc);

BLT_EXTERN int Blt_ConfigureBindings(Tcl_Interp *interp, Blt_BindTable table, 
	ClientData item, int argc, const char **argv);

BLT_EXTERN int Blt_ConfigureBindingsFromObj(Tcl_Interp *interp, 
	Blt_BindTable table, ClientData item, int objc, Tcl_Obj *const *objv);

BLT_EXTERN void Blt_PickCurrentItem(Blt_BindTable table);

BLT_EXTERN void Blt_DeleteBindings(Blt_BindTable table, ClientData object);

BLT_EXTERN void Blt_MoveBindingTable(Blt_BindTable table, Tk_Window tkwin);

#define Blt_SetFocusItem(bindPtr, object, hint) \
	((bindPtr)->focusObj = (ClientData)(object),\
	 (bindPtr)->focusHint = (ClientData)(hint))

#define Blt_SetCurrentItem(bindPtr, object, hint) \
	((bindPtr)->currentObj = (ClientData)(object),\
	 (bindPtr)->currentHint = (ClientData)(hint))

#define Blt_GetCurrentItem(bindPtr)  ((bindPtr)->currentObj)
#define Blt_GetCurrentHint(bindPtr)  ((bindPtr)->currentHint)
#define Blt_GetLatestItem(bindPtr)  ((bindPtr)->newObj)

#define Blt_GetBindingData(bindPtr)  ((bindPtr)->clientData)

#endif /*_BLT_BIND_H*/
