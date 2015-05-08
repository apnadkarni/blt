/* -*- mode: c; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 * tkDisplay.h --
 *
 * Excerpts from tkInt.h.  Used to examine window internals.
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
 * This file contains excerpts from tkInt.h of the TCL library distribution.
 *
 * Copyright (c) 1987-1993 The Regents of the University of California.
 *
 * Copyright (c) 1994-1998 Sun Microsystems, Inc.
 *
 *   See the file "license.terms" for information on usage and
 *   redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef _TK_DISPLAY_H
#define _TK_DISPLAY_H

typedef struct _TkIdStack TkIdStack;
typedef struct _TkErrorHandler TkErrorHandler;
typedef struct _TkSelectionInfo TkSelectionInfo;
typedef struct _TkClipboardTarget TkClipboardTarget;

typedef struct _TkWindow TkWindow;
typedef struct _TkWindowEvent TkWindowEvent;
typedef struct _TkEventHandler TkEventHandler;
typedef struct _TkSelHandler TkSelHandler;
typedef struct _TkWinInfo TkWinInfo;
typedef struct _TkClassProcs TkClassProcs;
typedef struct _TkWindowPrivate TkWindowPrivate;
typedef struct _TkGrabEvent TkGrabEvent;
typedef struct _TkColormap TkColormap;
typedef struct _TkStressedCmap TkStressedCmap;
typedef struct _TkWmInfo TkWmInfo;

typedef struct _TkBindInfo *TkBindInfo;
#ifdef notdef
typedef struct _TkRegion *TkRegion;
#endif
typedef struct _TkpCursor *TkpCursor;

#ifdef XNQueryInputStyle
#define TK_USE_INPUT_METHODS
#endif  /* XNQueryInputStyle */

/*
 * This defines whether we should try to use XIM over-the-spot style
 * input.  Allow users to override it.  It is a much more elegant use
 * of XIM, but uses a bit more memory.
 */
#ifndef TK_XIM_SPOT
#   define TK_XIM_SPOT  1
#endif  /* TK_XIM_SPOT */

#ifndef TK_REPARENTED
#define TK_REPARENTED   0
#endif /* TK_REPARENTED */

/*
 * Tk keeps one of the following data structures for each main
 * window (created by a call to TkCreateMainWindow).  It stores
 * information that is shared by all of the windows associated
 * with a particular main window.
 */

typedef struct TkMainInfo {
    int refCount;               /* Number of windows whose "mainPtr" fields
                                 * point here.  When this becomes zero, can
                                 * free up the structure (the reference
                                 * count is zero because windows can get
                                 * deleted in almost any order;  the main
                                 * window isn't necessarily the last one
                                 * deleted). */
    struct TkWindow *winPtr;    /* Pointer to main window. */
    Tcl_Interp *interp;         /* Interpreter associated with application. */
    Tcl_HashTable nameTable;    /* Hash table mapping path names to TkWindow
                                 * structs for all windows related to this
                                 * main window.  Managed by tkWindow.c. */
    long deletionEpoch;         /* Incremented by window deletions */
    Tk_BindingTable bindingTable;
                                /* Used in conjunction with "bind" command
                                 * to bind events to TCL commands. */
    TkBindInfo bindInfo;        /* Information used by tkBind.c on a per
                                 * application basis. */
    struct TkFontInfo *fontInfoPtr;
                                /* Information used by tkFont.c on a per
                                 * application basis. */

    /*
     * Information used only by tkFocus.c and tk*Embed.c:
     */

    struct TkToplevelFocusInfo *tlFocusPtr;
                                /* First in list of records containing focus
                                 * information for each top-level in the
                                 * application.  Used only by tkFocus.c. */
    struct TkDisplayFocusInfo *displayFocusPtr;
                                /* First in list of records containing focus
                                 * information for each display that this
                                 * application has ever used.  Used only
                                 * by tkFocus.c. */

    struct ElArray *optionRootPtr;
                                /* Top level of option hierarchy for this
                                 * main window.  NULL means uninitialized.
                                 * Managed by tkOption.c. */
    Tcl_HashTable imageTable;   /* Maps from image names to Tk_ImageMaster
                                 * structures.  Managed by tkImage.c. */
    int strictMotif;            /* This is linked to the tk_strictMotif
                                 * global variable. */

#if (_TK_VERSION >= _VERSION(8,5,0))
    int alwaysShowSelection;    /* This is linked to the
                                 * ::tk::AlwaysShowSelection variable. */
#endif /* TK_VERSION >= 8.5.0 */
    struct TkMainInfo *nextPtr; /* Next in list of all main windows managed by
                                 * this process. */
#if (_TK_VERSION >= _VERSION(8,6,0))
    Tcl_HashTable busyTable;	/* Information used by [tk busy] command. */
#endif /* TK_VERSION >= 8.6.0 */
} TkMainInfo;

#if (_TK_VERSION >= _VERSION(8,6,0))

typedef struct TkCaret {
    struct TkWindow *winPtr;    /* the window on which we requested caret
                                 * placement */
    int x;                      /* relative x coord of the caret */
    int y;                      /* relative y coord of the caret */
    int height;                 /* specified height of the window */
} TkCaret;

/*
 * One of the following structures is maintained for each display containing a
 * window managed by Tk. In part, the structure is used to store thread-
 * specific data, since each thread will have its own TkDisplay structure.
 */

struct _TkDisplay {
    Display *display;		/* Xlib's info about display. */
    struct _TkDisplay *nextPtr;	/* Next in list of all displays. */
    char *name;			/* Name of display (with any screen identifier
				 * removed). Malloc-ed. */
    Time lastEventTime;		/* Time of last event received for this
				 * display. */

    /*
     * Information used primarily by tk3d.c:
     */

    int borderInit;		/* 0 means borderTable needs initializing. */
    Tcl_HashTable borderTable;	/* Maps from color name to TkBorder
				 * structure. */

    /*
     * Information used by tkAtom.c only:
     */

    int atomInit;		/* 0 means stuff below hasn't been initialized
				 * yet. */
    Tcl_HashTable nameTable;	/* Maps from names to Atom's. */
    Tcl_HashTable atomTable;	/* Maps from Atom's back to names. */

    /*
     * Information used primarily by tkBind.c:
     */

    int bindInfoStale;		/* Non-zero means the variables in this part
				 * of the structure are potentially incorrect
				 * and should be recomputed. */
    unsigned int modeModMask;	/* Has one bit set to indicate the modifier
				 * corresponding to "mode shift". If no such
				 * modifier, than this is zero. */
    unsigned int metaModMask;	/* Has one bit set to indicate the modifier
				 * corresponding to the "Meta" key. If no such
				 * modifier, then this is zero. */
    unsigned int altModMask;	/* Has one bit set to indicate the modifier
				 * corresponding to the "Meta" key. If no such
				 * modifier, then this is zero. */
    enum {LU_IGNORE, LU_CAPS, LU_SHIFT} lockUsage;
				/* Indicates how to interpret lock
				 * modifier. */
    int numModKeyCodes;		/* Number of entries in modKeyCodes array
				 * below. */
    KeyCode *modKeyCodes;	/* Pointer to an array giving keycodes for all
				 * of the keys that have modifiers associated
				 * with them. Malloc'ed, but may be NULL. */

    /*
     * Information used by tkBitmap.c only:
     */

    int bitmapInit;		/* 0 means tables above need initializing. */
    int bitmapAutoNumber;	/* Used to number bitmaps. */
    Tcl_HashTable bitmapNameTable;
				/* Maps from name of bitmap to the first
				 * TkBitmap record for that name. */
    Tcl_HashTable bitmapIdTable;/* Maps from bitmap id to the TkBitmap
				 * structure for the bitmap. */
    Tcl_HashTable bitmapDataTable;
				/* Used by Tk_GetBitmapFromData to map from a
				 * collection of in-core data about a bitmap
				 * to a reference giving an automatically-
				 * generated name for the bitmap. */

    /*
     * Information used by tkCanvas.c only:
     */

    int numIdSearches;
    int numSlowSearches;

    /*
     * Used by tkColor.c only:
     */

    int colorInit;		/* 0 means color module needs initializing. */
    TkStressedCmap *stressPtr;	/* First in list of colormaps that have filled
				 * up, so we have to pick an approximate
				 * color. */
    Tcl_HashTable colorNameTable;
				/* Maps from color name to TkColor structure
				 * for that color. */
    Tcl_HashTable colorValueTable;
				/* Maps from integer RGB values to TkColor
				 * structures. */

    /*
     * Used by tkCursor.c only:
     */

    int cursorInit;		/* 0 means cursor module need initializing. */
    Tcl_HashTable cursorNameTable;
				/* Maps from a string name to a cursor to the
				 * TkCursor record for the cursor. */
    Tcl_HashTable cursorDataTable;
				/* Maps from a collection of in-core data
				 * about a cursor to a TkCursor structure. */
    Tcl_HashTable cursorIdTable;
				/* Maps from a cursor id to the TkCursor
				 * structure for the cursor. */
    char cursorString[20];	/* Used to store a cursor id string. */
    Font cursorFont;		/* Font to use for standard cursors. None
				 * means font not loaded yet. */

    /*
     * Information used by tkError.c only:
     */

    struct TkErrorHandler *errorPtr;
				/* First in list of error handlers for this
				 * display. NULL means no handlers exist at
				 * present. */
    int deleteCount;		/* Counts # of handlers deleted since last
				 * time inactive handlers were garbage-
				 * collected. When this number gets big,
				 * handlers get cleaned up. */

    /*
     * Used by tkEvent.c only:
     */

    struct TkWindowEvent *delayedMotionPtr;
				/* Points to a malloc-ed motion event whose
				 * processing has been delayed in the hopes
				 * that another motion event will come along
				 * right away and we can merge the two of them
				 * together. NULL means that there is no
				 * delayed motion event. */

    /*
     * Information used by tkFocus.c only:
     */

    int focusDebug;		/* 1 means collect focus debugging
				 * statistics. */
    struct TkWindow *implicitWinPtr;
				/* If the focus arrived at a toplevel window
				 * implicitly via an Enter event (rather than
				 * via a FocusIn event), this points to the
				 * toplevel window. Otherwise it is NULL. */
    struct TkWindow *focusPtr;	/* Points to the window on this display that
				 * should be receiving keyboard events. When
				 * multiple applications on the display have
				 * the focus, this will refer to the innermost
				 * window in the innermost application. This
				 * information isn't used on Windows, but it's
				 * needed on the Mac, and also on X11 when XIM
				 * processing is being done. */

    /*
     * Information used by tkGC.c only:
     */

    Tcl_HashTable gcValueTable; /* Maps from a GC's values to a TkGC structure
				 * describing a GC with those values. */
    Tcl_HashTable gcIdTable;    /* Maps from a GC to a TkGC. */
    int gcInit;			/* 0 means the tables below need
				 * initializing. */

    /*
     * Information used by tkGeometry.c only:
     */

    Tcl_HashTable maintainHashTable;
				/* Hash table that maps from a master's
				 * Tk_Window token to a list of slaves managed
				 * by that master. */
    int geomInit;

    /*
     * Information used by tkGet.c only:
     */

    Tcl_HashTable uidTable;	/* Stores all Tk_Uid used in a thread. */
    int uidInit;		/* 0 means uidTable needs initializing. */

    /*
     * Information used by tkGrab.c only:
     */

    struct TkWindow *grabWinPtr;/* Window in which the pointer is currently
				 * grabbed, or NULL if none. */
    struct TkWindow *eventualGrabWinPtr;
				/* Value that grabWinPtr will have once the
				 * grab event queue (below) has been
				 * completely emptied. */
    struct TkWindow *buttonWinPtr;
				/* Window in which first mouse button was
				 * pressed while grab was in effect, or NULL
				 * if no such press in effect. */
    struct TkWindow *serverWinPtr;
				/* If no application contains the pointer then
				 * this is NULL. Otherwise it contains the
				 * last window for which we've gotten an Enter
				 * or Leave event from the server (i.e. the
				 * last window known to have contained the
				 * pointer). Doesn't reflect events that were
				 * synthesized in tkGrab.c. */
    TkGrabEvent *firstGrabEventPtr;
				/* First in list of enter/leave events
				 * synthesized by grab code. These events must
				 * be processed in order before any other
				 * events are processed. NULL means no such
				 * events. */
    TkGrabEvent *lastGrabEventPtr;
				/* Last in list of synthesized events, or NULL
				 * if list is empty. */
    int grabFlags;		/* Miscellaneous flag values. See definitions
				 * in tkGrab.c. */

    /*
     * Information used by tkGrid.c only:
     */

    int gridInit;		/* 0 means table below needs initializing. */
    Tcl_HashTable gridHashTable;/* Maps from Tk_Window tokens to corresponding
				 * Grid structures. */

    /*
     * Information used by tkImage.c only:
     */

    int imageId;		/* Value used to number image ids. */

    /*
     * Information used by tkMacWinMenu.c only:
     */

    int postCommandGeneration;

    /*
     * Information used by tkPack.c only.
     */

    int packInit;		/* 0 means table below needs initializing. */
    Tcl_HashTable packerHashTable;
				/* Maps from Tk_Window tokens to corresponding
				 * Packer structures. */

    /*
     * Information used by tkPlace.c only.
     */

    int placeInit;		/* 0 means tables below need initializing. */
    Tcl_HashTable masterTable;	/* Maps from Tk_Window toke to the Master
				 * structure for the window, if it exists. */
    Tcl_HashTable slaveTable;	/* Maps from Tk_Window toke to the Slave
				 * structure for the window, if it exists. */

    /*
     * Information used by tkSelect.c and tkClipboard.c only:
     */

    struct TkSelectionInfo *selectionInfoPtr;
				/* First in list of selection information
				 * records. Each entry contains information
				 * about the current owner of a particular
				 * selection on this display. */
    Atom multipleAtom;		/* Atom for MULTIPLE. None means selection
				 * stuff isn't initialized. */
    Atom incrAtom;		/* Atom for INCR. */
    Atom targetsAtom;		/* Atom for TARGETS. */
    Atom timestampAtom;		/* Atom for TIMESTAMP. */
    Atom textAtom;		/* Atom for TEXT. */
    Atom compoundTextAtom;	/* Atom for COMPOUND_TEXT. */
    Atom applicationAtom;	/* Atom for TK_APPLICATION. */
    Atom windowAtom;		/* Atom for TK_WINDOW. */
    Atom clipboardAtom;		/* Atom for CLIPBOARD. */
    Atom utf8Atom;		/* Atom for UTF8_STRING. */

    Tk_Window clipWindow;	/* Window used for clipboard ownership and to
				 * retrieve selections between processes. NULL
				 * means clipboard info hasn't been
				 * initialized. */
    int clipboardActive;	/* 1 means we currently own the clipboard
				 * selection, 0 means we don't. */
    struct TkMainInfo *clipboardAppPtr;
				/* Last application that owned clipboard. */
    struct TkClipboardTarget *clipTargetPtr;
				/* First in list of clipboard type information
				 * records. Each entry contains information
				 * about the buffers for a given selection
				 * target. */

    /*
     * Information used by tkSend.c only:
     */

    Tk_Window commTkwin;	/* Window used for communication between
				 * interpreters during "send" commands. NULL
				 * means send info hasn't been initialized
				 * yet. */
    Atom commProperty;		/* X's name for comm property. */
    Atom registryProperty;	/* X's name for property containing registry
				 * of interpreter names. */
    Atom appNameProperty;	/* X's name for property used to hold the
				 * application name on each comm window. */

    /*
     * Information used by tkUnixWm.c and tkWinWm.c only:
     */

    struct TkWmInfo *firstWmPtr;/* Points to first top-level window. */
    struct TkWmInfo *foregroundWmPtr;
				/* Points to the foreground window. */

    /*
     * Information used by tkVisual.c only:
     */

    TkColormap *cmapPtr;	/* First in list of all non-default colormaps
				 * allocated for this display. */

    /*
     * Miscellaneous information:
     */

#ifdef TK_USE_INPUT_METHODS
    XIM inputMethod;		/* Input method for this display. */
    XIMStyle inputStyle;	/* Input style selected for this display. */
    XFontSet inputXfs;		/* XFontSet cached for over-the-spot XIM. */
#endif /* TK_USE_INPUT_METHODS */
    Tcl_HashTable winTable;	/* Maps from X window ids to TkWindow ptrs. */

    int refCount;		/* Reference count of how many Tk applications
				 * are using this display. Used to clean up
				 * the display when we no longer have any Tk
				 * applications using it. */

    /*
     * The following field were all added for Tk8.3
     */

    int mouseButtonState;	/* Current mouse button state for this
				 * display. */
    Window mouseButtonWindow;	/* Window the button state was set in, added
				 * in Tk 8.4. */
    Tk_Window warpWindow;
    Tk_Window warpMainwin;	/* For finding the root window for warping
				 * purposes. */
    int warpX;
    int warpY;

    /*
     * The following field(s) were all added for Tk8.4
     */

    unsigned int flags;		/* Various flag values: these are all defined
				 * in below. */
    TkCaret caret;		/* Information about the caret for this
				 * display. This is not a pointer. */

    int iconDataSize;		/* Size of default iconphoto image data. */
    unsigned char *iconDataPtr;	/* Default iconphoto image data, if set. */
};

#endif /* _TK_VERSION >= _VERSION(8,6,0) */

#if (_TK_VERSION >= _VERSION(8,1,0)) && (_TK_VERSION < _VERSION(8,6,0))

typedef struct TkCaret {
    struct TkWindow *winPtr;    /* the window on which we requested caret
                                 * placement */
    int x;                      /* relative x coord of the caret */
    int y;                      /* relative y coord of the caret */
    int height;                 /* specified height of the window */
} TkCaret;

/*
 * One of the following structures is maintained for each display
 * containing a window managed by Tk.  In part, the structure is
 * used to store thread-specific data, since each thread will have
 * its own TkDisplay structure.
 */

struct _TkDisplay {
    Display *display;           /* Xlib's info about display. */
    struct _TkDisplay *nextPtr; /* Next in list of all displays. */
    char *name;                 /* Name of display (with any screen
                                 * identifier removed).  Malloc-ed. */
    Time lastEventTime;         /* Time of last event received for this
                                 * display. */

    /*
     * Information used primarily by tk3d.c:
     */

    int borderInit;             /* 0 means borderTable needs initializing. */
    Tcl_HashTable borderTable;  /* Maps from color name to TkBorder
                                 * structure. */

    /*
     * Information used by tkAtom.c only:
     */

    int atomInit;               /* 0 means stuff below hasn't been
                                 * initialized yet. */
    Tcl_HashTable nameTable;    /* Maps from names to Atom's. */
    Tcl_HashTable atomTable;    /* Maps from Atom's back to names. */

    /*
     * Information used primarily by tkBind.c:
     */

    int bindInfoStale;          /* Non-zero means the variables in this
                                 * part of the structure are potentially
                                 * incorrect and should be recomputed. */
    unsigned int modeModMask;   /* Has one bit set to indicate the modifier
                                 * corresponding to "mode shift".  If no
                                 * such modifier, than this is zero. */
    unsigned int metaModMask;   /* Has one bit set to indicate the modifier
                                 * corresponding to the "Meta" key.  If no
                                 * such modifier, then this is zero. */
    unsigned int altModMask;    /* Has one bit set to indicate the modifier
                                 * corresponding to the "Meta" key.  If no
                                 * such modifier, then this is zero. */
    enum {
        LU_IGNORE, LU_CAPS, LU_SHIFT
    } lockUsage;                /* Indicates how to interpret lock
                                 * modifier. */
    int numModKeyCodes;         /* Number of entries in modKeyCodes array
                                 * below. */
    KeyCode *modKeyCodes;       /* Pointer to an array giving keycodes for
                                 * all of the keys that have modifiers
                                 * associated with them.  Malloc'ed, but
                                 * may be NULL. */

    /*
     * Information used by tkBitmap.c only:
     */

    int bitmapInit;             /* 0 means tables above need initializing. */
    int bitmapAutoNumber;       /* Used to number bitmaps. */
    Tcl_HashTable bitmapNameTable;
                                /* Maps from name of bitmap to the first
                                 * TkBitmap record for that name. */
    Tcl_HashTable bitmapIdTable;/* Maps from bitmap id to the TkBitmap
                                 * structure for the bitmap. */
    Tcl_HashTable bitmapDataTable;
                                /* Used by Tk_GetBitmapFromData to map from
                                 * a collection of in-core data about a
                                 * bitmap to a reference giving an auto-
                                 * matically-generated name for the bitmap. */

    /*
     * Information used by tkCanvas.c only:
     */

    int numIdSearches;
    int numSlowSearches;

    /*
     * Used by tkColor.c only:
     */

    int colorInit;              /* 0 means color module needs initializing. */
    TkStressedCmap *stressPtr;  /* First in list of colormaps that have
                                 * filled up, so we have to pick an
                                 * approximate color. */
    Tcl_HashTable colorNameTable;
                                /* Maps from color name to TkColor structure
                                 * for that color. */
    Tcl_HashTable colorValueTable;
                                /* Maps from integer RGB values to TkColor
                                 * structures. */

    /*
     * Used by tkCursor.c only:
     */

    int cursorInit;             /* 0 means cursor module need initializing. */
    Tcl_HashTable cursorNameTable;
                                /* Maps from a string name to a cursor to the
                                 * TkCursor record for the cursor. */
    Tcl_HashTable cursorDataTable;
                                /* Maps from a collection of in-core data
                                 * about a cursor to a TkCursor structure. */
    Tcl_HashTable cursorIdTable;
                                /* Maps from a cursor id to the TkCursor
                                 * structure for the cursor. */
    char cursorString[20];      /* Used to store a cursor id string. */
    Font cursorFont;            /* Font to use for standard cursors.
                                 * None means font not loaded yet. */

    /*
     * Information used by tkError.c only:
     */

    struct TkErrorHandler *errorPtr;
                                /* First in list of error handlers
                                 * for this display.  NULL means
                                 * no handlers exist at present. */
    int deleteCount;            /* Counts # of handlers deleted since
                                 * last time inactive handlers were
                                 * garbage-collected.  When this number
                                 * gets big, handlers get cleaned up. */

    /*
     * Used by tkEvent.c only:
     */

    struct TkWindowEvent *delayedMotionPtr;
                                /* Points to a malloc-ed motion event
                                 * whose processing has been delayed in
                                 * the hopes that another motion event
                                 * will come along right away and we can
                                 * merge the two of them together.  NULL
                                 * means that there is no delayed motion
                                 * event. */

    /*
     * Information used by tkFocus.c only:
     */

    int focusDebug;             /* 1 means collect focus debugging
                                 * statistics. */
    struct TkWindow *implicitWinPtr;
                                /* If the focus arrived at a toplevel window
                                 * implicitly via an Enter event (rather
                                 * than via a FocusIn event), this points
                                 * to the toplevel window.  Otherwise it is
                                 * NULL. */
    struct TkWindow *focusPtr;  /* Points to the window on this display that
                                 * should be receiving keyboard events.  When
                                 * multiple applications on the display have
                                 * the focus, this will refer to the
                                 * innermost window in the innermost
                                 * application.  This information isn't used
                                 * under Unix or Windows, but it's needed on
                                 * the Macintosh. */

    /*
     * Information used by tkGC.c only:
     */

    Tcl_HashTable gcValueTable; /* Maps from a GC's values to a TkGC structure
                                 * describing a GC with those values. */
    Tcl_HashTable gcIdTable;    /* Maps from a GC to a TkGC. */
    int gcInit;                 /* 0 means the tables below need
                                 * initializing. */

    /*
     * Information used by tkGeometry.c only:
     */

    Tcl_HashTable maintainHashTable;
                                /* Hash table that maps from a master's
                                 * Tk_Window token to a list of slaves
                                 * managed by that master. */
    int geomInit;

    /*
     * Information used by tkGet.c only:
     */

    Tcl_HashTable uidTable;     /* Stores all Tk_Uids used in a thread. */
    int uidInit;                /* 0 means uidTable needs initializing. */

    /*
     * Information used by tkGrab.c only:
     */

    struct TkWindow *grabWinPtr;
                                /* Window in which the pointer is currently
                                 * grabbed, or NULL if none. */
    struct TkWindow *eventualGrabWinPtr;
                                /* Value that grabWinPtr will have once the
                                 * grab event queue (below) has been
                                 * completely emptied. */
    struct TkWindow *buttonWinPtr;
                                /* Window in which first mouse button was
                                 * pressed while grab was in effect, or NULL
                                 * if no such press in effect. */
    struct TkWindow *serverWinPtr;
                                /* If no application contains the pointer then
                                 * this is NULL.  Otherwise it contains the
                                 * last window for which we've gotten an
                                 * Enter or Leave event from the server (i.e.
                                 * the last window known to have contained
                                 * the pointer).  Doesn't reflect events
                                 * that were synthesized in tkGrab.c. */
    TkGrabEvent *firstGrabEventPtr;
                                /* First in list of enter/leave events
                                 * synthesized by grab code.  These events
                                 * must be processed in order before any other
                                 * events are processed.  NULL means no such
                                 * events. */
    TkGrabEvent *lastGrabEventPtr;
                                /* Last in list of synthesized events, or NULL
                                 * if list is empty. */
    int grabFlags;              /* Miscellaneous flag values.  See definitions
                                 * in tkGrab.c. */

    /*
     * Information used by tkGrid.c only:
     */

    int gridInit;               /* 0 means table below needs initializing. */
    Tcl_HashTable gridHashTable;/* Maps from Tk_Window tokens to
                                 * corresponding Grid structures. */

    /*
     * Information used by tkImage.c only:
     */

    int imageId;                /* Value used to number image ids. */

    /*
     * Information used by tkMacWinMenu.c only:
     */

    int postCommandGeneration;

    /*
     * Information used by tkOption.c only.
     */



    /*
     * Information used by tkPack.c only.
     */

    int packInit;               /* 0 means table below needs initializing. */
    Tcl_HashTable packerHashTable;
                                /* Maps from Tk_Window tokens to
                                 * corresponding Packer structures. */


    /*
     * Information used by tkPlace.c only.
     */

    int placeInit;              /* 0 means tables below need initializing. */
    Tcl_HashTable masterTable;  /* Maps from Tk_Window toke to the Master
                                 * structure for the window, if it exists. */
    Tcl_HashTable slaveTable;   /* Maps from Tk_Window toke to the Slave
                                 * structure for the window, if it exists. */

    /*
     * Information used by tkSelect.c and tkClipboard.c only:
     */


    struct TkSelectionInfo *selectionInfoPtr;
    /* First in list of selection information
                                 * records.  Each entry contains information
                                 * about the current owner of a particular
                                 * selection on this display. */
    Atom multipleAtom;          /* Atom for MULTIPLE.  None means
                                 * selection stuff isn't initialized. */
    Atom incrAtom;              /* Atom for INCR. */
    Atom targetsAtom;           /* Atom for TARGETS. */
    Atom timestampAtom;         /* Atom for TIMESTAMP. */
    Atom textAtom;              /* Atom for TEXT. */
    Atom compoundTextAtom;      /* Atom for COMPOUND_TEXT. */
    Atom applicationAtom;       /* Atom for TK_APPLICATION. */
    Atom windowAtom;            /* Atom for TK_WINDOW. */
    Atom clipboardAtom;         /* Atom for CLIPBOARD. */
#if (_TK_VERSION >= _VERSION(8,4,0))
    Atom utf8Atom;
#endif  /* TK_VERSION >= 8.4.0 */
    Tk_Window clipWindow;       /* Window used for clipboard ownership and to
                                 * retrieve selections between processes. NULL
                                 * means clipboard info hasn't been
                                 * initialized. */
    int clipboardActive;        /* 1 means we currently own the clipboard
                                 * selection, 0 means we don't. */
    struct TkMainInfo *clipboardAppPtr;
                                /* Last application that owned clipboard. */
    struct TkClipboardTarget *clipTargetPtr;
                                /* First in list of clipboard type information
                                 * records.  Each entry contains information
                                 * about the buffers for a given selection
                                 * target. */

    /*
     * Information used by tkSend.c only:
     */

    Tk_Window commTkwin;        /* Window used for communication
                                 * between interpreters during "send"
                                 * commands.  NULL means send info hasn't
                                 * been initialized yet. */
    Atom commProperty;          /* X's name for comm property. */
    Atom registryProperty;      /* X's name for property containing
                                 * registry of interpreter names. */
    Atom appNameProperty;       /* X's name for property used to hold the
                                 * application name on each comm window. */

    /*
     * Information used by tkXId.c only:
     */

    struct TkIdStack *idStackPtr;
                                /* First in list of chunks of free resource
                                 * identifiers, or NULL if there are no free
                                 * resources. */
    XID(*defaultAllocProc)(Display *display);
                                /* Default resource allocator for display. */
    struct TkIdStack *windowStackPtr;
                                /* First in list of chunks of window
                                 * identifers that can't be reused right
                                 * now. */
#if (_TK_VERSION < _VERSION(8,4,0))
    int idCleanupScheduled;     /* 1 means a call to WindowIdCleanup has
                                 * already been scheduled, 0 means it
                                 * hasn't. */
#else
    Tcl_TimerToken idCleanupScheduled;
                                /* If set, it means a call to WindowIdCleanup
                                 * has already been scheduled, 0 means it
                                 * hasn't. */
#endif  /* TK_VERSION < 8.4.0 */
    /*
     * Information used by tkUnixWm.c and tkWinWm.c only:
     */

#if (_TK_VERSION < _VERSION(8,4,0))
    int wmTracing;              /* Used to enable or disable tracing in
                                 * this module.  If tracing is enabled,
                                 * then information is printed on
                                 * standard output about interesting
                                 * interactions with the window manager. */
#endif /* TK_VERSION < 8.4.0 */
    struct TkWmInfo *firstWmPtr; /* Points to first top-level window. */
    struct TkWmInfo *foregroundWmPtr;
                                /* Points to the foreground window. */

    /*
     * Information maintained by tkWindow.c for use later on by tkXId.c:
     */


    int destroyCount;           /* Number of Tk_DestroyWindow operations
                                 * in progress. */
    unsigned long lastDestroyRequest;
                                /* Id of most recent XDestroyWindow request;
                                 * can re-use ids in windowStackPtr when
                                 * server has seen this request and event
                                 * queue is empty. */

    /*
     * Information used by tkVisual.c only:
     */

    TkColormap *cmapPtr;        /* First in list of all non-default colormaps
                                 * allocated for this display. */

    /*
     * Miscellaneous information:
     */

#ifdef TK_USE_INPUT_METHODS
    XIM inputMethod;            /* Input method for this display */
#  if (_TK_VERSION >= _VERSION(8,5,0))
    XIMStyle inputStyle;        /* Input style selected for this display. */
#  endif        /* TK_VERSION >= 8.5.0 */
#  if (_TK_VERSION >= _VERSION(8,4,0))
#    if TK_XIM_SPOT
    XFontSet inputXfs;          /* XFontSet cached for over-the-spot XIM. */
#    endif /* TK_XIM_SPOT */
#  endif /* _TK_VERSION >= 8.4 */
#endif /* TK_USE_INPUT_METHODS */
    Tcl_HashTable winTable;     /* Maps from X window ids to TkWindow ptrs. */
    int refCount;               /* Reference count of how many Tk applications
                                 * are using this display. Used to clean up
                                 * the display when we no longer have any
                                 * Tk applications using it.
                                 */
    /*
     * The following field were all added for Tk8.3
     */
    int mouseButtonState;       /* current mouse button state for this
                                 * display */

#if (_TK_VERSION < _VERSION(8,4,0))
    int warpInProgress;
#else 
    Window mouseButtonWindow;   /* Window the button state was set in, added
                                 * in Tk 8.4. */
#endif  /* TK_VERSION < 8.4.0 */
    Window warpWindow;
    int warpX;
    int warpY;

#if (_TK_VERSION < _VERSION(8,4,0))
    int useInputMethods;        /* Whether to use input methods */
    long deletionEpoch;         /* Incremented by window deletions */
#endif  /* TK_VERSION < 8.4.0 */
    /*
     * The following field(s) were all added for Tk8.4
     */
    unsigned int flags;         /* Various flag values:  these are all
                                 * defined in below. */
    TkCaret caret;              /* information about the caret for this
                                 * display.  This is not a pointer. */
#if (_TK_VERSION >= _VERSION(8,4,0))
    int iconDataSize;           /* Size of default iconphoto image data. */
    unsigned char *iconDataPtr; /* Default iconphoto image data, if set. */
#endif  /* TK_VERSION > 8.4.0 */

};

#endif /*_TK_VERSION >= _VERSION(8,1,0) && _TK_VERSION < _VERSION(8,6,0) */

#if (_TK_VERSION < _VERSION(8,1,0))

/*
 * One of the following structures is maintained for each display
 * containing a window managed by Tk:
 */
typedef struct _TkDisplay {
    Display *display;           /* Xlib's info about display. */
    struct _TkDisplay *nextPtr; /* Next in list of all displays. */
    char *name;                 /* Name of display (with any screen
                                 * identifier removed).  Malloc-ed. */
    Time lastEventTime;         /* Time of last event received for this
                                 * display. */

    /*
     * Information used primarily by tkBind.c:
     */

    int bindInfoStale;          /* Non-zero means the variables in this
                                 * part of the structure are potentially
                                 * incorrect and should be recomputed. */
    unsigned int modeModMask;   /* Has one bit set to indicate the modifier
                                 * corresponding to "mode shift".  If no
                                 * such modifier, than this is zero. */
    unsigned int metaModMask;   /* Has one bit set to indicate the modifier
                                 * corresponding to the "Meta" key.  If no
                                 * such modifier, then this is zero. */
    unsigned int altModMask;    /* Has one bit set to indicate the modifier
                                 * corresponding to the "Meta" key.  If no
                                 * such modifier, then this is zero. */
    enum {
        LU_IGNORE, LU_CAPS, LU_SHIFT
    } lockUsage;
    /* Indicates how to interpret lock modifier. */
    int numModKeyCodes;         /* Number of entries in modKeyCodes array
                                 * below. */
    KeyCode *modKeyCodes;       /* Pointer to an array giving keycodes for
                                 * all of the keys that have modifiers
                                 * associated with them.  Malloc'ed, but
                                 * may be NULL. */

    /*
     * Information used by tkError.c only:
     */

    TkErrorHandler *errorPtr;
    /* First in list of error handlers
                                 * for this display.  NULL means
                                 * no handlers exist at present. */
     int deleteCount;           /* Counts # of handlers deleted since
                                 * last time inactive handlers were
                                 * garbage-collected.  When this number
                                 * gets big, handlers get cleaned up. */

    /*
     * Information used by tkSend.c only:
     */

    Tk_Window commTkwin;        /* Window used for communication
                                 * between interpreters during "send"
                                 * commands.  NULL means send info hasn't
                                 * been initialized yet. */
    Atom commProperty;          /* X's name for comm property. */
    Atom registryProperty;      /* X's name for property containing
                                 * registry of interpreter names. */
    Atom appNameProperty;       /* X's name for property used to hold the
                                 * application name on each comm window. */

    /*
     * Information used by tkSelect.c and tkClipboard.c only:
     */

     TkSelectionInfo *selectionInfoPtr;
    /* First in list of selection information
                                 * records.  Each entry contains information
                                 * about the current owner of a particular
                                 * selection on this display. */
    Atom multipleAtom;          /* Atom for MULTIPLE.  None means
                                 * selection stuff isn't initialized. */
    Atom incrAtom;              /* Atom for INCR. */
    Atom targetsAtom;           /* Atom for TARGETS. */
    Atom timestampAtom;         /* Atom for TIMESTAMP. */
    Atom textAtom;              /* Atom for TEXT. */
    Atom compoundTextAtom;      /* Atom for COMPOUND_TEXT. */
    Atom applicationAtom;       /* Atom for TK_APPLICATION. */
    Atom windowAtom;            /* Atom for TK_WINDOW. */
    Atom clipboardAtom;         /* Atom for CLIPBOARD. */

    Tk_Window clipWindow;       /* Window used for clipboard ownership and to
                                 * retrieve selections between processes. NULL
                                 * means clipboard info hasn't been
                                 * initialized. */
    int clipboardActive;        /* 1 means we currently own the clipboard
                                 * selection, 0 means we don't. */
     TkMainInfo *clipboardAppPtr;
     /* Last application that owned clipboard. */
     TkClipboardTarget *clipTargetPtr;
     /* First in list of clipboard type information
                                 * records.  Each entry contains information
                                 * about the buffers for a given selection
                                 * target. */

    /*
     * Information used by tkAtom.c only:
     */

    int atomInit;               /* 0 means stuff below hasn't been
                                 * initialized yet. */
    Tcl_HashTable nameTable;    /* Maps from names to Atom's. */
    Tcl_HashTable atomTable;    /* Maps from Atom's back to names. */

    /*
     * Information used by tkCursor.c only:
     */

    Font cursorFont;            /* Font to use for standard cursors.
                                 * None means font not loaded yet. */

    /*
     * Information used by tkGrab.c only:
     */

     TkWindow *grabWinPtr;
    /* Window in which the pointer is currently
                                 * grabbed, or NULL if none. */
     TkWindow *eventualGrabWinPtr;
    /* Value that grabWinPtr will have once the
                                 * grab event queue (below) has been
                                 * completely emptied. */
     TkWindow *buttonWinPtr;
    /* Window in which first mouse button was
                                 * pressed while grab was in effect, or NULL
                                 * if no such press in effect. */
     TkWindow *serverWinPtr;
    /* If no application contains the pointer then
                                 * this is NULL.  Otherwise it contains the
                                 * last window for which we've gotten an
                                 * Enter or Leave event from the server (i.e.
                                 * the last window known to have contained
                                 * the pointer).  Doesn't reflect events
                                 * that were synthesized in tkGrab.c. */
    TkGrabEvent *firstGrabEventPtr;
    /* First in list of enter/leave events
                                 * synthesized by grab code.  These events
                                 * must be processed in order before any other
                                 * events are processed.  NULL means no such
                                 * events. */
    TkGrabEvent *lastGrabEventPtr;
    /* Last in list of synthesized events, or NULL
                                 * if list is empty. */
    int grabFlags;              /* Miscellaneous flag values.  See definitions
                                 * in tkGrab.c. */

    /*
     * Information used by tkXId.c only:
     */

     TkIdStack *idStackPtr;
    /* First in list of chunks of free resource
                                 * identifiers, or NULL if there are no free
                                 * resources. */
              XID(*defaultAllocProc)(Display *display);
    /* Default resource allocator for display. */
     TkIdStack *windowStackPtr;
    /* First in list of chunks of window
                                 * identifers that can't be reused right
                                 * now. */
    int idCleanupScheduled;     /* 1 means a call to WindowIdCleanup has
                                 * already been scheduled, 0 means it
                                 * hasn't. */

    /*
     * Information maintained by tkWindow.c for use later on by tkXId.c:
     */


    int destroyCount;           /* Number of Tk_DestroyWindow operations
                                 * in progress. */
    unsigned long lastDestroyRequest;
    /* Id of most recent XDestroyWindow request;
                                 * can re-use ids in windowStackPtr when
                                 * server has seen this request and event
                                 * queue is empty. */

    /*
     * Information used by tkVisual.c only:
     */

    TkColormap *cmapPtr;        /* First in list of all non-default colormaps
                                 * allocated for this display. */

    /*
     * Information used by tkFocus.c only:
     */
     TkWindow *implicitWinPtr;
                                /* If the focus arrived at a toplevel window
                                 * implicitly via an Enter event (rather
                                 * than via a FocusIn event), this points
                                 * to the toplevel window.  Otherwise it is
                                 * NULL. */
     TkWindow *focusPtr;        /* Points to the window on this display that
                                 * should be receiving keyboard events.  When
                                 * multiple applications on the display have
                                 * the focus, this will refer to the
                                 * innermost window in the innermost
                                 * application.  This information isn't used
                                 * under Unix or Windows, but it's needed on
                                 * the Macintosh. */

    /*
     * Used by tkColor.c only:
     */

    TkStressedCmap *stressPtr;  /* First in list of colormaps that have
                                 * filled up, so we have to pick an
                                 * approximate color. */

    /*
     * Used by tkEvent.c only:
     */

     TkWindowEvent *delayedMotionPtr;
                                /* Points to a malloc-ed motion event
                                 * whose processing has been delayed in
                                 * the hopes that another motion event
                                 * will come along right away and we can
                                 * merge the two of them together.  NULL
                                 * means that there is no delayed motion
                                 * event. */
    /*
     * Miscellaneous information:
     */

#ifdef TK_USE_INPUT_METHODS
    XIM inputMethod;            /* Input method for this display */
#endif /* TK_USE_INPUT_METHODS */
    Tcl_HashTable winTable;     /* Maps from X window ids to TkWindow ptrs. */
    int refCount;               /* Reference count of how many Tk applications
                                 * are using this display. Used to clean up
                                 * the display when we no longer have any
                                 * Tk applications using it.
                                 */
} TkDisplay;

#endif /* _TK_VERSION >= _VERSION(8,1,0) */


struct _TkWindow {
    Display *display;
    TkDisplay *dispPtr;
    int screenNum;
    Visual *visual;
    int depth;
    Window window;
    TkWindow *childList;
    TkWindow *lastChildPtr;
    TkWindow *parentPtr;
    TkWindow *nextPtr;
    TkMainInfo *mainPtr;
    char *pathName;
    Tk_Uid nameUid;
    Tk_Uid classUid;
    XWindowChanges changes;
    unsigned int dirtyChanges;
    XSetWindowAttributes atts;
    unsigned long dirtyAtts;
    unsigned int flags;
    TkEventHandler *handlerList;
#ifdef TK_USE_INPUT_METHODS
    XIC inputContext;
#endif /* TK_USE_INPUT_METHODS */
    ClientData *tagPtr;
    int numTags;
    int optionLevel;
    TkSelHandler *selHandlerList;
    Tk_GeomMgr *geomMgrPtr;
    ClientData geomData;
    int reqWidth, reqHeight;
    int internalBorderLeft;
    TkWinInfo *wmInfoPtr;
    TkClassProcs *classProcsPtr;
    ClientData instanceData;
    TkWindowPrivate *privatePtr;

#if (_TK_VERSION >= _VERSION(8,4,0))
    /* The remaining fields of internal border. */
    int internalBorderRight; 
    int internalBorderTop;
    int internalBorderBottom;
    
    int minReqWidth;            /* Minimum requested width. */
    int minReqHeight;           /* Minimum requested height. */
#endif /* _TK_VERSION >= _VERSION(8,4,0) */
#if (_TK_VERSION >= _VERSION(8,6,0))
    char *geometryMaster;
#endif  /* _TK_VERSION >= _VERSION(8,6,0) */
};

/*
 * This structure is used by the Mac and Window porting layers as
 * the internal representation of a clip_mask in a GC.
 */

typedef struct {
    int type;                   /* One of TKP_CLIP_PIXMAP or TKP_CLIP_REGION */
    union {
        Pixmap pixmap;
        TkRegion region;
    } value;
} TkpClipMask;

#define TKP_CLIP_PIXMAP 0
#define TKP_CLIP_REGION 1

#ifdef WIN32
#include "tkWinDisplay.h"
#endif

#endif /* _TK_DISPLAY_H */
