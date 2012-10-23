
/*
 * bltTile.h --
 *
 *	Copyright 1995-2004 George A Howlett.
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
