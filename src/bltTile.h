/* -*- mode: c; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 * bltTile.h --
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

#ifndef BLT_TILE_H
#define BLT_TILE_H

#define TILE_THREAD_KEY	"BLT Tile Data"
#define TILE_MAGIC ((unsigned int) 0x46170277)

typedef struct _Blt_TileClient *Blt_Tile; /* Opaque type for tiles */

typedef void (Blt_TileChangedProc)(ClientData clientData, Blt_Tile tile);

BLT_EXTERN int Blt_GetTile(Tcl_Interp *interp, Tk_Window tkwin, char *imageName,
	Blt_Tile *tilePtr);

BLT_EXTERN void Blt_FreeTile(Blt_Tile tile);

BLT_EXTERN const char *Blt_NameOfTile(Blt_Tile tile);

BLT_EXTERN void Blt_SetTileChangedProc(Blt_Tile tile, 
	Blt_TileChangedProc *changeProc, ClientData clientData);

BLT_EXTERN void Blt_TileRectangle(Tk_Window tkwin, Drawable drawable, 
	Blt_Tile tile, int x, int y, unsigned int width, unsigned int height);
BLT_EXTERN void Blt_TileRectangles(Tk_Window tkwin, Drawable drawable,
	Blt_Tile tile, XRectangle *rectArr, int numRects);
BLT_EXTERN void Blt_TilePolygon(Tk_Window tkwin, Drawable drawable, 
	Blt_Tile tile, XPoint *pointArr, int numPoints);
BLT_EXTERN Pixmap Blt_PixmapOfTile(Blt_Tile tile);

BLT_EXTERN void Blt_SizeOfTile(Blt_Tile tile, int *widthPtr, int *heightPtr);

BLT_EXTERN void Blt_SetTileOrigin(Tk_Window tkwin, Blt_Tile tile, int x, int y);

BLT_EXTERN void Blt_SetTSOrigin(Tk_Window tkwin, Blt_Tile tile, int x, int y);

#endif /* BLT_TILE_H */
