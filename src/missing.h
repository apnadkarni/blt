#ifndef _MISSING_H
#define _MISSING_H

#include <winspool.h>

#ifndef DeleteBitmap
#define DeleteBitmap(hbm)       DeleteObject((HGDIOBJ)(HBITMAP)(hbm))
#endif
#ifndef DeleteBrush
#define DeleteBrush(hbr)	DeleteObject((HGDIOBJ)(HBRUSH)(hbr))
#endif
#ifndef DeleteFont
#define DeleteFont(hfont)	DeleteObject((HGDIOBJ)(HFONT)(hfont))
#endif
#ifndef DeletePalette
#define DeletePalette(hpal)     DeleteObject((HGDIOBJ)(HPALETTE)(hpal))
#endif
#ifndef DeletePen
#define DeletePen(hpen)		DeleteObject((HGDIOBJ)(HPEN)(hpen))
#endif
#ifndef SelectBitmap
#define SelectBitmap(hdc, hbm)  ((HBITMAP)SelectObject((hdc), (HGDIOBJ)(HBITMAP)(hbm)))
#endif
#ifndef SelectBrush
#define SelectBrush(hdc, hbr)   ((HBRUSH)SelectObject((hdc), (HGDIOBJ)(HBRUSH)(hbr)))
#endif
#ifndef SelectFont
#define SelectFont(hdc, hfont)  ((HFONT)SelectObject((hdc), (HGDIOBJ)(HFONT)(hfont)))
#endif
#ifndef SelectPen
#define SelectPen(hdc, hpen)    ((HPEN)SelectObject((hdc), (HGDIOBJ)(HPEN)(hpen)))
#endif
#ifndef GetNextWindow
#define GetNextWindow(hWnd,wCmd) GetWindow((hWnd),(wCmd))
#endif
#ifndef GetStockBrush
#define GetStockBrush(i)     ((HBRUSH)GetStockObject(i))
#endif
#ifndef GetStockPen
#define GetStockPen(i)       ((HPEN)GetStockObject(i))
#endif


#endif /* _MISSING_H */
