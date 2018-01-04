/* -*- mode: c; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 * bltPalette.h --
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
 */

#ifndef _BLT_PALETTE_H
#define _BLT_PALETTE_H

/*
 *---------------------------------------------------------------------------
 *
 * Blt_Palette --
 *
 *      Represents a gradient color palette.  The color palette is made up
 *      of an array of palette entries.  There can also be an array opacity
 *      entries.
 *
 *---------------------------------------------------------------------------
 */
#ifndef _BLT_PAINTBRUSH_H
typedef struct _Blt_Palette *Blt_Palette;
#endif /*_BLT_PAINTBRUSH_H*/

typedef void (Blt_Palette_NotifyProc) (Blt_Palette palette, 
        ClientData clientData, unsigned int flags);

#define PALETTE_CHANGE_NOTIFY   (1<<0)

BLT_EXTERN int Blt_Palette_GetFromObj(Tcl_Interp *interp, Tcl_Obj *objPtr, 
        Blt_Palette *palPtr);
BLT_EXTERN int Blt_Palette_GetFromString(Tcl_Interp *interp, const char *string,
        Blt_Palette *palPtr);
BLT_EXTERN int Blt_Palette_GetAssociatedColor(Blt_Palette palette, double val);
BLT_EXTERN void Blt_Palette_CreateNotifier(Blt_Palette palette, 
        Blt_Palette_NotifyProc *proc, ClientData clientData);
BLT_EXTERN void Blt_Palette_DeleteNotifier(Blt_Palette palette, 
        Blt_Palette_NotifyProc *proc, ClientData clientData);
BLT_EXTERN const char *Blt_Palette_Name(Blt_Palette palette);
BLT_EXTERN Blt_Palette Blt_Palette_TwoColorPalette(int low, int high);
BLT_EXTERN void Blt_Palette_Delete(Blt_Palette palette);

BLT_EXTERN int Blt_Palette_GetRGBColor(Blt_Palette palette, double value);

#endif /*_BLT_PALETTE_H*/
