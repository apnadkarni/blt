/* -*- mode: c; c-basic-offset: 4; indent-tabs-mode: nil -*- */

typedef struct {
    int left, right;
} ScanLine;

static ScanLine *
MakeScanLines(int numLines)
{
    ScanLine *coords;
    int i;

    coords = Blt_AssertMalloc(sizeof(ScanLine) * numLines);
    for(i = 0; i < numLines; i++) {
        coords[i].left = INT_MAX;
        coords[i].right = -INT_MAX;
    }
    return coords;
}

static void 
AddEllipseCoord(ScanLine *coords, int x, int y)
{
    if (x < coords[y].left) {
        coords[y].left = x;
    } 
    if (x > coords[y].right) {
        coords[y].right = x;
    }
}

static ScanLine *
ComputeEllipseQuadrant(int a, int b)
{
    ScanLine *coords;
    double t;
    int dx, dy;
    double a2, b2;
    double p, px, py;
    a2 = a * a;
    b2 = b * b;
    
    dx = 0;
    dy = b;
    px = 0;                             /* b2 * (dx + 1) */
    py = (double)(a2 + a2) * dy;        /* a2 * (dy - 0.5) */

    coords = MakeScanLines(b + 1);
    if (coords == NULL) {
        return NULL;
    }
    AddEllipseCoord(coords, dx, dy);

    t = (b2 - (a2 * b) + (0.25 * a2));
    p = (int)ROUND(t);
    while (py > px) {
        dx++;
        px += b2 + b2;
        if (dy <= 0) {
            continue;
        }
        if (p < 0) {
            p += b2 + px;
        } else {
            dy--;
            py -= a2 + a2;
            p += b2 + px - py;
        }
        AddEllipseCoord(coords, dx, dy);
    }
    {
        double dx2, dy2;
 
        dx2 = (dx + 0.5) * (dx + 0.5);
        dy2 = (dy - 1) * (dy - 1);
        t = (b2 * dx2 + a2 * dy2 - (a2 * b2));
        p = (int)ROUND(t);
    }
    while (dy > 0) {
        dy--;
        py -= a2 + a2;
        if (p > 0) {
            p += a2 - py;
        } else {
            dx++;
            px += b2 + b2;
            p += a2 - py + px;
        }
        AddEllipseCoord(coords, dx, dy);
    }
    AddEllipseCoord(coords, dx, dy);
    return coords;
}

#ifdef notdef
static INLINE void
MixColors(Blt_Pixel *bp, Blt_Pixel *colorPtr)
{
    if ((bp->Alpha == 0x00) || (colorPtr->Alpha == 0xFF)) {
        bp->u32 = colorPtr->u32;
    } else if (colorPtr->Alpha != 0x00) {
        unsigned char beta;
        int a, r, g, b, t;

        /* beta = 1 - alpha */
        beta = colorPtr->Alpha ^ 0xFF; 
        r = colorPtr->Red   + imul8x8(beta, bp->Red, t);
        g = colorPtr->Green + imul8x8(beta, bp->Green, t);
        b = colorPtr->Blue  + imul8x8(beta, bp->Blue, t);
        a = colorPtr->Alpha + imul8x8(beta, bp->Alpha, t);
        bp->Red = (r > 255) ? 255 : ((r < 0) ? 0 : r);
        bp->Green = (g > 255) ? 255 : ((g < 0) ? 0 : g);
        bp->Blue = (b > 255) ? 255 : ((b < 0) ? 0 : b);
        bp->Alpha = 0xFF;
    } 
}

static INLINE void
MixPaint(Blt_Pixel *bp, Blt_Pixel *colorPtr)
{
    if ((bp->Alpha == 0x00) || (colorPtr->Alpha == 0xFF)) {
        bp->u32 = colorPtr->u32;
    } else if (colorPtr->Alpha != 0x00) {
        unsigned char beta, alpha;
        int a, r, g, b, t, u;

        /* beta = 1 - alpha */
        alpha = colorPtr->Alpha;
        beta = colorPtr->Alpha ^ 0xFF; 
        r = colorPtr->Red   + imul8x8(beta, bp->Red, t);
        g = colorPtr->Green + imul8x8(beta, bp->Green, t);
        b = colorPtr->Blue  + imul8x8(beta, bp->Blue, t);
        a = colorPtr->Alpha + imul8x8(beta, bp->Alpha, t);
        bp->Red = (r > 255) ? 255 : ((r < 0) ? 0 : r);
        bp->Green = (g > 255) ? 255 : ((g < 0) ? 0 : g);
        bp->Blue = (b > 255) ? 255 : ((b < 0) ? 0 : b);
        bp->Alpha = 0xFF;
    } 
}
#endif

static void INLINE
PaintPixel(Pict *destPtr, int x, int y, Blt_Pixel *colorPtr) 
{
    if ((x >= 0) && (x < destPtr->width) && (y >= 0) && (y < destPtr->height)) {
        BlendPixels(Blt_Picture_Pixel(destPtr, x, y), colorPtr);
    }
}

static void INLINE
PaintHorizontalLine(Pict *destPtr, int x1, int x2, int y, 
                    Blt_PaintBrush brush, int blend)  
{
    if ((y >= 0) && (y < destPtr->height)) {
        Blt_Pixel *dp, *dend;

        if (x1 > x2) {
            int tmp;
            
            tmp = x1, x1 = x2, x2 = tmp;
        }
        x1 = MAX(x1, 0);
        x2 = MIN(x2, destPtr->width);
        dp   = destPtr->bits + (y * destPtr->pixelsPerRow) + x1;
        dend = destPtr->bits + (y * destPtr->pixelsPerRow) + x2;
        if (blend) {
            int x;

            for (x = x1; dp < dend; dp++, x++) {
                Blt_Pixel color;

                color.u32 = Blt_GetAssociatedColorFromBrush(brush, x, y);
                BlendPixels(dp, &color);
            }
        } else {
            int x;

            for (x = x1; dp < dend; dp++, x++) {
                dp->u32 = Blt_GetAssociatedColorFromBrush(brush, x, y);
            }
        }
    }
}

static void INLINE
FillHorizontalLine(Pict *destPtr, int x1, int x2, int y, Blt_Pixel *colorPtr, 
                   int blend)  
{
    if ((y >= 0) && (y < destPtr->height)) {
        Blt_Pixel *dp, *dend;

        if (x1 > x2) {
            int tmp;
            
            tmp = x1, x1 = x2, x2 = tmp;
        }
        x1 = MAX(x1, 0);
        x2 = MIN(x2, destPtr->width - 1);
        dp   = destPtr->bits + (y * destPtr->pixelsPerRow) + x1;
        dend = destPtr->bits + (y * destPtr->pixelsPerRow) + x2;
        if (blend) {
            for (/*empty*/; dp <= dend; dp++) {
                BlendPixels(dp, colorPtr);
            }
        } else {
            for (/*empty*/; dp < dend; dp++) {
                dp->u32 = colorPtr->u32;
            }
        }
    }
}

static void INLINE 
FillVerticalLine(Pict *destPtr, int x, int y1, int y2, Blt_Pixel *colorPtr, 
                 int blend)  
{
    if ((x >= 0) && (x < destPtr->width)) {
        Blt_Pixel *dp, *dend;

        if (y1 > y2) {
            int tmp;
            
            tmp = y1, y1 = y2, y2 = tmp;
        }
        y1 = MAX(y1, 0);
        y2 = MIN(y2, destPtr->height - 1);
        dp   = destPtr->bits + (y1 * destPtr->pixelsPerRow) + x;
        dend = destPtr->bits + (y2 * destPtr->pixelsPerRow) + x;
        if (blend) {
            for (/*empty*/; dp <= dend; dp += destPtr->pixelsPerRow) {
                BlendPixels(dp, colorPtr);
            }
        } else {
            for (/*empty*/; dp <= dend; dp += destPtr->pixelsPerRow) {
                dp->u32 = colorPtr->u32;
            }
        }           
    }
}

#ifdef notdef
static void
PaintCircle3(Blt_Picture src, int x, int y, int r, Blt_Pixel *colorPtr)
{
    Blt_Pixel edge, interior;
    double t;
    int r2;
    int y1, y2;

    r2 = r * r;
    t = 0.0;
    x = r;

    interior.u32 = edge.u32 = colorPtr->u32;
    /* Center line */
    PutPixel(src, r - x, r, &edge);
    PutPixel(src, r + x, r, &edge);
    y = 0;
    while (x > y) {
        double z;
        double d, q;
        unsigned char a;

        y++;
        z = sqrt(r2 - (y * y));
        d = floor(z) - z;
        if (d < t) {
            x--;
        }
        q = FABS(d * 255.0);
        a = (unsigned char)CLAMP(q);
        edge.Alpha = a;
        interior.Alpha = ~a;

        /* By symmetry we can fill upper and lower scan lines. */
        PaintPixel(src, r - x - 1, r + y, &interior);
        PaintPixel(src, r + x + 1, r + y, &edge);

        PaintPixel(src, r - x - 1, r - y, &edge);
        PaintPixel(src, r + x + 1, r - y, &interior);

        t = d;
    }
    y1 = r - y;
    y2 = r + y; 
    x = 0;
    y = r;
    t = 0;
    interior.u32 = edge.u32 = colorPtr->u32;
    VertLine(src, r, 0, y1, &edge);    /* Center line */
    VertLine(src, r, y2, r + r, &edge);    
    while (y > x) {
        unsigned char a;
        double z;
        double d, q;

        x++;
        z = sqrt(r2 - (x * x));
        d = floor(z) - z;
        if (d < t) {
            y--;
        }
        q = FABS(d * 255.0);
        a = (unsigned int)CLAMP(q);
        edge.Alpha = a;
        interior.Alpha = ~a;

        /* By symmetry we can fill upper and lower scan lines. */
        PaintPixel(src, r - x, r - y - 1, &edge);
        PaintPixel(src, r - x, r + y + 1, &interior);

        PaintPixel(src, r + x, r - y - 1, &interior);
        PaintPixel(src, r + x, r + y + 1, &edge);

        t = d;
    }
}
#endif

static INLINE float 
sqr(float x) 
{
    return x * x;
}
    
static void
PaintCircle4(Pict *destPtr, float cx, float cy, float r, float lineWidth, 
             Blt_PaintBrush brush, int blend)
{
    int x, y, i;
    int x1, x2, y1, y2;
    float outer, inner, outer2, inner2;
    float *squares;
    Blt_Pixel *destRowPtr;

    /* Determine some helpful values (singles) */
    outer = r;
    if (lineWidth > 0) {
        inner = r - lineWidth;
    } else {
        inner = 0;
    }
    outer2 = outer * outer;
    inner2 = inner * inner;

    /* Determine bounds: */
    x1 = (int)floor(cx - outer);
    if (x1 < 0) {
        x1 = 0;
    }
    x2 = (int)ceil(cx + outer) + 1;
    if (x2 >= destPtr->width) {
        x2 = destPtr->width;
    }
    y1 = (int)floor(cy - outer);
    if (y1 < 0) {
        y1 = 0;
    }
    y2 = (int)ceil(cy + outer) + 1;
    if (y2 >= destPtr->height) {
        y2 = destPtr->height;
    }
    /* Optimization run: find squares of X first */
    squares = Blt_AssertMalloc(sizeof(float) * (x2 - x1));
    for (i = 0, x = x1; x < x2; x++, i++) {
        squares[i] = (x - cx) * (x - cx);
    }
    /* Loop through Y values */
    destRowPtr = destPtr->bits + (y1 * destPtr->pixelsPerRow) + x1;
    for (y = y1; y < y2; y++) {
        Blt_Pixel *dp;
        float dy2;
        
        dy2 = (y - cy) * (y - cy);
        for (dp = destRowPtr, x = x1; x < x2; x++, dp++) {
            float dx2, d2, d;
            unsigned int a;
            float outerf, innerf;

            dx2 = squares[x - x1];
            /* Compute distance from circle center to this pixel. */
            d2 = dy2 + dx2;
            if (d2 > outer2) {
                continue;
            }
            if (d2 < inner2) {
                continue;
            }
            /* Mix the color.*/
            d = sqrt(d2);
            outerf = outer - d;
            innerf = d - inner;
#ifdef notdef
            dp->u32 = colorPtr->u32;
#endif
            if (outerf < 1.0) {
                a = (int)(outerf * 255.0 + 0.5);
            } else if ((inner > 0) && (innerf < 1.0)) {
                a = (int)(innerf * 255.0 + 0.5);
            } else {
                a = 255;
            }
            if (blend) {
                Blt_Pixel color;

                a = UCLAMP(a);
#ifdef notdef
                if (dp->Alpha != 0) {
                    a = imul8x8(a, dp->Alpha, t);
                }
#endif
                color.u32 = Blt_GetAssociatedColorFromBrush(brush, x, y);
                Blt_FadeColor(&color, a);
                BlendPixels(dp, &color);
            } else {
                int t;
                /* FIXME: This is overriding the alpha of a premultiplied
                 * color. */
                a = UCLAMP(a);
                dp->u32 = Blt_GetAssociatedColorFromBrush(brush, x, y);
                dp->Alpha = imul8x8(a, dp->Alpha, t);
            }
        }
        destRowPtr += destPtr->pixelsPerRow;
    }
    destPtr->flags &= ~BLT_PIC_PREMULT_COLORS;
    Blt_Free(squares);
}

#ifdef notdef
static Pict *
xFilledCircle(int diam, Blt_Pixel *colorPtr)
{
    Pict *destPtr;
    Blt_Pixel edge, interior;
    double t;
    int r, r2;
    int x, y;
    int y1, y2;

    diam |= 0x01;
    r = diam / 2;
    r2 = r * r;
    destPtr = Blt_CreatePicture(diam, diam);
    t = 0.0;
    x = r;
    interior = PremultiplyAlpha(colorPtr, 255);
    HorizLine(destPtr, x - r, x + r, r, &interior);    /* Center line */
    y = 0;
    while (x > y) {
        double z;
        double d, q;
        unsigned char a;

        y++;
        z = sqrt(r2 - (y * y));
        d = floor(z) - z;
        if (d < t) {
            x--;
        }
        q = FABS(d * 255.0);
        a = (unsigned int)CLAMP(q);
        edge = PremultiplyAlpha(colorPtr, (unsigned int)CLAMP(q));

        /* By symmetry we can fill upper and lower scan lines. */
        PutPixel(destPtr, r - x - 1, r + y, &edge);
        HorizLine(destPtr, r - x, r + x, r + y, &interior);
        PutPixel(destPtr, r + x + 1, r + y, &edge);


        PutPixel(destPtr, r - x - 1, r - y, &edge);
        HorizLine(destPtr, r - x, r + x, r - y, &interior);
        PutPixel(destPtr, r + x + 1, r - y, &edge);

        t = d;
    }
    y1 = r - y;
    y2 = r + y; 
    x = 0;
    y = r;
    t = 0;
    VertLine(destPtr, r, 0, y1, &interior);    /* Center line */
    VertLine(destPtr, r, y2, r + r, &interior);    /* Center line */
    while (y > x) {
        double z;
        double d, q;

        x++;
        z = sqrt(r2 - (x * x));
        d = floor(z) - z;
        if (d < t) {
            y--;
        }
        q = FABS(d * 255.0);
        edge = PremultiplyAlpha(colorPtr, (unsigned int)CLAMP(q));

        /* By symmetry we can fill upper and lower scan lines. */
        PutPixel(destPtr, r - x, r - y - 1, &edge);
        VertLine(destPtr, r - x, r - y, y1, &interior); 
        VertLine(destPtr, r - x, y2, r + y, &interior); 
        PutPixel(destPtr, r - x, r + y + 1, &edge);

        PutPixel(destPtr, r + x, r - y - 1, &edge);
        VertLine(destPtr, r + x, r - y, y1, &interior); 
        VertLine(destPtr, r + x, y2, r + y, &interior); 
        PutPixel(destPtr, r + x, r + y + 1, &edge);

        t = d;
    }
    return destPtr;
}
#endif

#ifdef notdef
static void
FilledOval(Pict *destPtr, int destX, int destY, int ovalWidth, int ovalHeight, 
           Blt_Pixel *colorPtr)
{
    Blt_Pixel edge, interior;
    double t;
    int r, r2;
    int x, y;
    int diam;
    int x1, x2, y1, y2;
    int blend = 1;
    t = 0.0;

    interior = PremultiplyAlpha(colorPtr, 255);
    if (ovalWidth < ovalHeight) {
        diam = ovalWidth;
        diam |= 0x1;
        r = diam / 2;
        r2 = r * r;
        ovalWidth = diam;

        /* Fill the center rectangle */
        x1 = destX;
        x2 = destX + ovalWidth - 1;
        y1 = destY + r;
        y2 = destY + ovalHeight - r - 1;
        for (y = y1; y <= y2; y++) {
            FillHorizontalLine(destPtr, x1, x2, y, &interior, blend);
        }
        x1 = x2 = r;
    } else if (ovalWidth > ovalHeight) {
        diam = ovalHeight;
        diam |= 0x1;
        r = diam / 2;
        r2 = r * r;
        ovalHeight = diam;
        y1 = y2 = destY + r;
        x1 = destX + r, x2 = destY + ovalWidth - r - 1;
        FillHorizontalLine(destPtr, destX, destX + ovalWidth - 1, y1, 
                           &interior, blend);
        for (y = destY; y < y1; y++) {
            FillHorizontalLine(destPtr, x1, x2, y, &interior, blend);
        }
        for (y = y2; y < destY + ovalHeight; y++) {
            FillHorizontalLine(destPtr, x1, x2, y, &interior, blend);
        }
    } else {
        diam = ovalWidth;
        diam |= 0x1;
        ovalHeight = ovalWidth = diam;
        r = diam / 2;
        r2 = r * r;
        /* Fill the center rectangle */
        x1 = destX;
        x2 = destX + ovalWidth - 1;
        y1 = destY + r;
        y2 = destY + ovalHeight - r - 1;
    }
    y = 0;
    x = r;
    while (x > y) {
        double z;
        double d, q;
        unsigned char a;

        y++;
        z = sqrt(r2 - (y * y));
        d = floor(z) - z;
        if (d < t) {
            x--;
        }
        q = FABS(d * 255.0);
        a = (unsigned int)CLAMP(q);
        edge = PremultiplyAlpha(colorPtr, (unsigned int)CLAMP(q));

        /* By symmetry we can fill upper and lower scan lines. */
        PaintPixel(destPtr, x1 - x - 1, y2 + y, &edge); 
        FillHorizontalLine(destPtr, x1 - x, x2 + x, y2 + y, &interior, blend);
        PaintPixel(destPtr, x2 + x + 1, y2 + y, &edge);

        PaintPixel(destPtr, x1 - x - 1, y1 - y, &edge);
        FillHorizontalLine(destPtr, x1 - x, x2 + x, y1 - y, &interior, blend);
        PaintPixel(destPtr, x2 + x + 1, y1 - y, &edge);
        t = d;
    }
    x = 0;
    y = r;
    t = 0;

    FillVerticalLine(destPtr, destX + r, destY, y1, &interior, blend);
    FillVerticalLine(destPtr, destX + r, y2, destY + ovalHeight - 1, &interior, 
                     blend);
    while (y > x) {
        double z;
        double d, q;

        x++;
        z = sqrt(r2 - (x * x));
        d = floor(z) - z;
        if (d < t) {
            y--;
        }
        q = FABS(d * 255.0);
        edge = PremultiplyAlpha(colorPtr, (unsigned int)CLAMP(q));

        /* By symmetry we can fill upper and lower scan lines. */
        PaintPixel(destPtr, x1 - x, y1 - y - 1, &edge);
        FillVerticalLine(destPtr, x1 - x, y1 - y, y1, &interior, blend); 
        FillVerticalLine(destPtr, x1 - x, y2, y2 + y, &interior, blend); 
        PaintPixel(destPtr, x1 - x, y2 + y + 1, &edge);

        PaintPixel(destPtr, x2 + x, y1 - y - 1, &edge);
        FillVerticalLine(destPtr, x2 + x, y1 - y, y1, &interior, blend); 
        FillVerticalLine(destPtr, x2 + x, y2, y2 + y, &interior, blend); 
        PaintPixel(destPtr, x2 + x, y2 + y + 1, &edge);

        t = d;
    }
}
#endif

#ifdef notdef
static void 
PaintArc(Pict *destPtr, int x1, int y1, int x2, int y2, int lineWidth, 
         Blt_Pixel *colorPtr)
{
    Blt_Pixel *dp;
    double t;
    int r2;
    int radius;
    int dx, dy;
    int x, y;
    int xoff, yoff;
    int fill = 1;

    t = 0.0;
    dx = x2 - x1;
    dy = y2 - y1;
    radius = MIN(dx, dy) / 2;
    xoff = x1;
    yoff = y1;
    x = radius;
    y = 0;
    dp = Blt_Picture_Pixel(destPtr, x + xoff - 1, y + yoff);
    dp->u32 = colorPtr->u32;
    r2 = radius * radius;
    if (fill) {
        PaintLineSegment(destPtr, x1, y + yoff, x + xoff - 2, y + yoff, 1, 
                         colorPtr);
    }
    while (x > y) {
        double z;
        double d, q;
        unsigned char a;

        y++;
        z = sqrt(r2 - (y * y));
        d = floor(z) - z;
        if (d < t) {
            x--;
        }
        dp = Blt_Picture_Pixel(destPtr, x + xoff, y + yoff);
        q = FABS(d * 255.0);
        a = (unsigned int)CLAMP(q);
        Blt_FadeColor(colorPtr, a);
        BlendPixels(dp, colorPtr);
        dp--;                   /* x - 1 */
        a = (unsigned int)CLAMP(255.0 - q);
        Blt_FadeColor(colorPtr, a);
        BlendPixels(dp, colorPtr);
        t = d;
        x1++;
        if (fill) {
            PaintLineSegment(destPtr, x1, y + yoff, x + xoff - 1, y + yoff, 1, 
                             colorPtr);
        }
    }
}
#endif

#ifdef notdef
static void
PaintFilledCircle(Pict *destPtr, int x, int y, int r, Blt_Pixel *colorPtr)
{
    Blt_Pixel *destRowPtr;
    Blt_Pixel fill;
    int inner, outer;
    int count;

    count = 0;

    inner = (r - 1) * (r - 1);
    outer = (r + 1) * (r + 1);

    fill = PremultiplyAlpha(colorPtr, 255);
    destRowPtr = destPtr->bits + ((y - r) * destPtr->pixelsPerRow) + (x - r);
    for (y = -r; y <= r; y++) {
        Blt_Pixel *dp, *dend;
        int dy2;

        dy2 = y * y;
        for (dp = destRowPtr, dend = dp + destPtr->width, x = -r; 
             (x <= r) && (dp < dend); x++, dp++) {
            double z;
            int d2;

            d2 = dy2 + x * x;
            if (d2 > outer) {
                continue;
            } 
            if (d2 < inner) {
                BlendPixels(dp, &fill); /* Interior */
                continue;
            }
            z = (double)r - sqrt((double)d2);
            count++;
            if (z > 1.0) {
                BlendPixels(dp, &fill); /* Interior */
            } else if (z > 0.0) {
                double q;
                unsigned char a;
                Blt_Pixel edge;

                q = z * 255.0;
                a = (unsigned int)CLAMP(q);     
                edge = PremultiplyAlpha(colorPtr, a);
                BlendPixels(dp, &edge); /* Edge */
            }
        }
        destRowPtr += destPtr->pixelsPerRow;
    }
#ifdef notdef
    fprintf(stderr, "%d pixels %d sqrts\n", (r + r + 1) * (r + r + 1), count);
#endif
}

static void
PaintThinCircle(Pict *destPtr, int x, int y, int r, Blt_Pixel *colorPtr)
{
    Blt_Pixel edge, fill;
    double r2;
    double t;
    int dx, dy;

    fill = PremultiplyAlpha(colorPtr, 255);

    PaintPixel(destPtr, x, y + r - 1, &fill);
    PaintPixel(destPtr, x, y - r + 1, &fill);
    PaintPixel(destPtr, x + r - 1, y, &fill);
    PaintPixel(destPtr, x - r + 1, y, &fill);

    r2 = r * r;

    dx = r, dy = 0;
    t = 0.0;
    while (dx > dy) {
        double z;
        double d, q;
        unsigned char a;

        dy++;
        z = sqrt(r2 - (dy * dy));
        d = floor(z) - z;
        if (d < t) {
            dx--;
        }
        q = FABS(d * 255.0);
        a = (unsigned int)CLAMP(q);     
        edge = PremultiplyAlpha(colorPtr, a);
        fill = PremultiplyAlpha(colorPtr, ~a);

        /* By symmetry we can add pixels in 4 octants. */
        PaintPixel(destPtr, x + dx,     y + dy, &edge); 
        PaintPixel(destPtr, x + dx - 1, y + dy, &fill); 
        PaintPixel(destPtr, x + dx,     y - dy, &edge);
        PaintPixel(destPtr, x + dx - 1, y - dy, &fill); 
        PaintPixel(destPtr, x - dx,     y + dy, &edge);
        PaintPixel(destPtr, x - dx + 1, y + dy, &fill); 
        PaintPixel(destPtr, x - dx,     y - dy, &edge);
        PaintPixel(destPtr, x - dx + 1, y - dy, &fill); 
        t = d;
    }
    
    dx = 0, dy = r;
    t = 0.0;
    while (dy > dx) {
        double z;
        double d, q;
        unsigned char a;

        dx++;
        z = sqrt(r2 - (dx * dx));
        d = floor(z) - z;
        if (d < t) {
            dy--;
        }
        q = FABS(d * 255.0);
        a = (unsigned char)CLAMP(q);
        edge = PremultiplyAlpha(colorPtr, a);
        fill = PremultiplyAlpha(colorPtr, ~a);

        PaintPixel(destPtr, x + dx, y + dy, &edge); 
        PaintPixel(destPtr, x + dx, y + dy - 1, &fill); 
        PaintPixel(destPtr, x - dx, y + dy, &edge); 
        PaintPixel(destPtr, x - dx, y + dy - 1, &fill); 
        PaintPixel(destPtr, x + dx, y - dy, &edge); 
        PaintPixel(destPtr, x + dx, y - dy + 1, &fill); 
        PaintPixel(destPtr, x - dx, y - dy, &edge); 
        PaintPixel(destPtr, x - dx, y - dy + 1, &fill); 
        t = d;
    }
}


static void
PaintThinEllipse(
    Pict *destPtr, 
    int x, int y, int a, int b, 
    Blt_Pixel *colorPtr, 
    int blend)
{
    ScanLine *coords;
    Blt_Pixel fill;
    int dy;
    int dx1, dx2;

    coords = ComputeEllipseQuadrant(a, b);
    if (blend) {
        fill = PremultiplyAlpha(colorPtr, 255);
    } else {
        fill.u32 = colorPtr->u32;
    }
    if (coords == NULL) {
        return;
    }
    dx1 = coords[0].right;
    dx2 = coords[0].left;
    FillHorizontalLine(destPtr, x + dx2, x + dx1, y, &fill, blend);
    FillHorizontalLine(destPtr, x - dx1, x - dx2, y, &fill, blend);
    dx1 = coords[b].right;
    FillHorizontalLine(destPtr, x - dx1, x + dx1, y - b, &fill, blend);
    FillHorizontalLine(destPtr, x - dx1, x + dx1, y + b, &fill, blend);
    for (dy = 1; dy < b; dy++) {
        dx1 = coords[dy].right;
        dx2 = coords[dy].left;
        FillHorizontalLine(destPtr, x + dx2, x + dx1, y - dy, &fill, blend);
        FillHorizontalLine(destPtr, x + dx2, x + dx1, y + dy, &fill, blend);
        FillHorizontalLine(destPtr, x - dx1, x - dx2, y - dy, &fill, blend);
        FillHorizontalLine(destPtr, x - dx1, x - dx2, y + dy, &fill, blend);
    }
    Blt_Free(coords);
}
#endif
static void
PaintThickEllipse(
    Blt_Picture picture, 
    int x, int y,               /* Center of the ellipse. */
    int a,                      /* Half the width of the ellipse. */
    int b,                      /* Half the height of the ellipse. */
    int lineWidth,              /* Line width of the ellipse. Must be 1 or
                                 * greater. */
    Blt_Pixel *colorPtr,
    int blend)
{
    ScanLine *outer, *inner;
    Blt_Pixel fill;
    int dy;
    int dx1, dx2;

    lineWidth--;
    outer = ComputeEllipseQuadrant(a, b);
    if (outer == NULL) {
        return;
    }
    inner = ComputeEllipseQuadrant(a - lineWidth, b - lineWidth);
    if (blend) {
        fill = PremultiplyAlpha(colorPtr, 255);
    } else {
        fill.u32 = colorPtr->u32;
    }
    dx1 = outer[0].right;
    dx2 = inner[0].left;
    FillHorizontalLine(picture, x + dx2, x + dx1, y, &fill, blend);
    FillHorizontalLine(picture, x - dx1, x - dx2, y, &fill, blend);
    for (dy = 1; dy < (b - lineWidth); dy++) {
        dx1 = outer[dy].right;
        dx2 = inner[dy].left;
        FillHorizontalLine(picture, x + dx2, x + dx1, y - dy, &fill, blend);
        FillHorizontalLine(picture, x + dx2, x + dx1, y + dy, &fill, blend);
        FillHorizontalLine(picture, x - dx1, x - dx2, y - dy, &fill, blend);
        FillHorizontalLine(picture, x - dx1, x - dx2, y + dy, &fill, blend);
    }
    for (/* empty */; dy <= b; dy++) {
        int dx;

        dx = outer[dy].right;
        FillHorizontalLine(picture, x - dx, x + dx, y + dy, &fill, blend);
        FillHorizontalLine(picture, x - dx, x + dx, y - dy, &fill, blend);
    }
    Blt_Free(outer);
    Blt_Free(inner);
}


static void
PaintFilledEllipse(
    Blt_Picture picture, 
    int x, int y,               /* Center of the ellipse. */
    int a,                      /* Half the width of the ellipse. */
    int b,                      /* Half the height of the ellipse. */
    Blt_Pixel *colorPtr,
    int blend)
{
    ScanLine *coords;
    Blt_Pixel fill;
    int dx, dy;

    coords = ComputeEllipseQuadrant(a, b);
    if (blend) {
        fill = PremultiplyAlpha(colorPtr, 255);
    } else {
        fill.u32 = colorPtr->u32;
    }
    if (coords == NULL) {
        return;
    }
    FillHorizontalLine(picture, x - a, x + a, y, &fill, blend);
    for (dy = 1; dy <= b; dy++) {
        dx = coords[dy].right;
        FillHorizontalLine(picture, x - dx, x + dx, y + dy, &fill, blend);
        FillHorizontalLine(picture, x - dx, x + dx, y - dy, &fill, blend);
    }
    Blt_Free(coords);
}

static void
PaintEllipse(
    Blt_Picture picture, 
    int x, int y,               /* Center of the ellipse. */
    int a,                      /* Half the width of the ellipse. */
    int b,                      /* Half the height of the ellipse. */
    int lineWidth,              /* Line width of the ellipse.  If zero,
                                 * then draw a solid filled ellipse. */
    Blt_Pixel *colorPtr,
    int blend)
{
    if ((lineWidth >= a) || (lineWidth >= b)) {
        lineWidth = 0;
    }
    if (lineWidth < 1) {
        PaintFilledEllipse(picture, x, y, a, b, colorPtr, blend);
    } else {
        PaintThickEllipse(picture, x, y, a, b, lineWidth, colorPtr, blend);
    }
}

static void
PaintEllipseAA(
    Blt_Picture picture, 
    int x, int y,               /* Center of the ellipse. */
    int a,                      /* Half the width of the ellipse. */
    int b,                      /* Half the height of the ellipse. */
    int lineWidth,              /* Line thickness of the ellipse.  If zero,
                                 * then draw a solid filled ellipse. */
    Blt_Pixel *colorPtr)
{
    PictRegion region;
    Blt_Picture big;
    int numSamples = 3; 
    int ellipseWidth, ellipseHeight;
    int blend = 1;

    if ((lineWidth >= a) || (lineWidth >= b)) {
        lineWidth = 0;
    }
    ellipseWidth = a + a + 3;
    ellipseHeight = b + b + 3;
    region.x = x - (a + 1);
    region.y = y - (b + 1);
    region.w = ellipseWidth;
    region.h = ellipseHeight;
    
    if (!Blt_AdjustRegionToPicture(picture, &region)) {
        return;                 /* Ellipse is totally clipped. */
    }
    /* Scale the region forming the bounding box of the ellipse into a new
     * picture. The bounding box is scaled by *nSamples* times. */
    big = Blt_CreatePicture(ellipseWidth * numSamples, ellipseHeight * numSamples);
    if (big != NULL) {
        Blt_Picture tmp;
        int cx, cy;
        Blt_Pixel color;

        cx = a + 1;
        cy = b + 1;
        /* Now draw an ellipse scaled by the same amount. The center of the
         * ellipse is the center of the picture. */
        Blt_BlankPicture(big, 0x0);
        color.u32 = 0xFF000000;
        Blt_PremultiplyColor(&color);
        PaintEllipse(big, 
            cx * numSamples,    /* Center of ellipse. */
            cy * numSamples, 
            a * numSamples,     
            b * numSamples, 
            lineWidth * numSamples, /* Scaled line width. */
                &color,
                blend); 
            
        /* Reduce the picture back to the original size using a simple box
         * filter for smoothing. */
        tmp = Blt_CreatePicture(ellipseWidth, ellipseHeight);
        Blt_ResamplePicture(tmp, big, bltBoxFilter, bltBoxFilter);
        Blt_FreePicture(big);
        Blt_ApplyColorToPicture(tmp, colorPtr);
        /* Replace the bounding box in the original with the new. */
        Blt_CompositeRegion(picture, tmp, 0, 0, region.w, region.h, 
                region.x, region.y);
        Blt_FreePicture(tmp);
    }
}

#ifdef notdef
static void
PaintCircle(
    Blt_Picture picture, 
    int x, int y,               /* Center of the circle in picture. */
    int r,                      /* Radius of the circle. */
    int lineWidth,              /* Line width of the circle.  */
    Blt_Pixel *colorPtr)
{
    int blend = 1;
    if (lineWidth >= r) {
        lineWidth = 0;
    }
    if (lineWidth < 1) {
        PaintFilledEllipse(picture, x, y, r, r, colorPtr, blend);
    } else if (lineWidth == 1) {
        PaintThinEllipse(picture, x, y, r, r, colorPtr, blend);
    } else {
        PaintThickEllipse(picture, x, y, r, r, lineWidth, colorPtr, blend);
    }
}
#endif

#ifdef notdef
static void
PaintRectangleAA(
    Blt_Picture picture, 
    int x, int y,                       /* Upper left corner of rectangle. */
    int w, int h,                       /* Dimension of rectangle. */
    int r,                              /* Radius of rounded corner. If zero,
                                         * draw square corners. */
    int lineWidth,                      /* Line width of the rectangle.  If
                                         * zero, then draw a solid fill
                                         * rectangle. */
    Blt_PaintBrush brush)
{
    Blt_Picture big;                    /* Super-sampled background. */
    int numSamples = 4; 
    Blt_Picture tmp;
    int offset = 4;

    if (((lineWidth*2) >= w) || ((lineWidth*2) >= h)) {
        lineWidth = 0;                  /* Paint solid rectangle instead.*/
    }
    if (r < 0) {
        Blt_PaintRectangle(picture, x, y, w, h, r, lineWidth, brush, TRUE);
        return;
    }
    /* 
     * Scale the region forming the bounding box of the ellipse into a new
     * picture. The bounding box is scaled by *nSamples* times.
     */
    big = Blt_CreatePicture((w+2*offset) * numSamples, 
                            (h+2*offset) * numSamples); 
    Blt_BlankPicture(big, 0x0);
    if (big == NULL) {
        return;
    }
    Blt_SetBrushRegion(brush, 0, 0, w * numSamples, h * numSamples);
    Blt_PaintRectangle(big,             /* Contains super-sampled original
                                         * background. */
                       0, 0,
                       w * numSamples,  /* Scaled width */
                       h * numSamples,  /* Scaled height */
                       r * numSamples,  /* Scaled radius. */
                       lineWidth * numSamples, /* Scaled line width. */
                       brush, TRUE); 
            
   /* Reduce the picture back to its original size using a simple box
    * filter for smoothing. */
   tmp = Blt_CreatePicture(w+2*offset, h+2*offset);
   Blt_ResamplePicture(tmp, big, bltBoxFilter, bltBoxFilter);
        
   /* Replace the bounding box in the original with the new. */
   Blt_CompositeRegion(picture, tmp, 0, 0, w+offset, h+offset, x, y);
   Blt_FreePicture(big);
   Blt_FreePicture(tmp);
}
#endif

static void
PaintRectangleShadow(Blt_Picture picture, int x, int y, int w, int h, int r, 
                     int lineWidth, Blt_Shadow *shadowPtr)
{
    int dw, dh;
    Blt_Picture blur;
    Blt_PaintBrush brush;

    dw = (w + shadowPtr->offset*3);
    dh = (h + shadowPtr->offset*3);
    blur = Blt_CreatePicture(dw, dh);
    Blt_BlankPicture(blur, 0x0);
    brush = Blt_NewColorBrush(shadowPtr->color.u32);
    Blt_PaintRectangle(blur, shadowPtr->offset, shadowPtr->offset, w, h, r, 
        lineWidth, brush, TRUE);
    Blt_FreeBrush(brush);
    Blt_BlurPicture(blur, blur, shadowPtr->offset, 2);
    Blt_CompositeRegion(picture, blur, 0, 0, dw, dh, x, y);
    Blt_FreePicture(blur);
}

#define UPPER_LEFT      0
#define UPPER_RIGHT     1
#define LOWER_LEFT      2
#define LOWER_RIGHT     3

static void
PaintCorner(Pict *destPtr, int x, int y, int r, int lineWidth, int corner, 
            Blt_PaintBrush brush)
{
    int blend = 1;
    int outer, inner, outer2, inner2;
    int x1, x2, y1, y2, dx, dy;

    outer = r;
    if ((lineWidth > 0) && (lineWidth < r)) {
        inner = r - lineWidth;
    } else {
        inner = 0;
    }
    outer2 = r * r;
    inner2 = floor(inner * inner);
    
    x1 = x2 = y1 = y2 = 0;              /* Suppress compiler warning. */
    switch (corner) {
    case UPPER_LEFT:
        x1 = 0;
        x2 = r;
        y1 = 0;
        y2 = r;
        break;
    case UPPER_RIGHT:
        x1 = r + 1;
        x2 = r + r;
        y1 = 0;
        y2 = r;
        break;
    case LOWER_LEFT:
        x1 = 0;
        x2 = r;
        y1 = r + 1;
        y2 = r + r;
        break;
    case LOWER_RIGHT:
        x1 = r + 1;
        x2 = r + r;
        y1 = r + 1;
        y2 = r + r;
        break;
    }   
    for (dy = y1; dy < y2; dy++) {
        float dy2;

        if (((y + dy) < 0) || ((y + dy) >= destPtr->height)) {
            continue;
        }
        dy2 = (dy - r) * (dy - r);
        for (dx = x1; dx < x2; dx++) {
            float dx2, d2, d;
            unsigned int a;
            float outerf, innerf;
            Blt_Pixel *dp;

            if (((x + dx) < 0) || ((x + dx) >= destPtr->width)) {
                continue;
            }
            dx2 = (dx - r) * (dx - r);
            /* Compute distance from circle center to this pixel. */
            d2 = dy2 + dx2;
            if (d2 > outer2) {
                continue;
            }
            if (d2 < inner2) {
                continue;
            }
            /* Mix the color.*/
            d = sqrt(d2);
            outerf = outer - d;
            innerf = d - inner;
            if (outerf < 1.0) {
                a = (int)(outerf * 255.0 + 0.5);
            } else if ((inner > 0) && (innerf < 1.0)) {
                a = (int)(innerf * 255.0 + 0.5);
            } else {
                a = 255;
            }
            dp = Blt_Picture_Pixel(destPtr, x+dx, y+dy);
            if (blend) {
                Blt_Pixel color;
                
                a = UCLAMP(a);
                color.u32 = Blt_GetAssociatedColorFromBrush(brush,x+dx,y+dy);
                Blt_FadeColor(&color, a);
                BlendPixels(dp, &color);
            } else {
                a = UCLAMP(a);
                dp->u32 = Blt_GetAssociatedColorFromBrush(brush, x+dx, y+dy);
                Blt_FadeColor(dp, a);
            }
        }
    }
}

/* 
 *      
 *      ul  xxxxxxxxxxxxxxxxxx ur       Upper section
 *          xxxxxxxxxxxxxxxxxx  
 *      xxxxxxxxxxxxxxxxxxxxxxxxxx      Middle section
 *      xxxxxxxxxxxxxxxxxxxxxxxxxx 
 *      xxxxxxxxxxxxxxxxxxxxxxxxxx
 *          xxxxxxxxxxxxxxxxxx          Lower section
 *      ll  xxxxxxxxxxxxxxxxxx lr
 *      
 */
void
Blt_PaintRectangle(Blt_Picture picture, int x, int y, int w, int h, int r, 
               int lineWidth, Blt_PaintBrush brush)
{
    int blend = 1;

    /* If the linewidth exceeds half the height or width of the rectangle,
     * then paint as a solid rectangle.*/
    if (((lineWidth*2) >= w) || ((lineWidth*2) >= h)) {
        lineWidth = 0;
    }
    /* Radius of each rounded corner can't be bigger than half the width or
     * height of the rectangle. */
    if (r > (w / 2)) {
        r = w / 2;
    }
    if (r > (h / 2)) {
        r = h / 2;
    }

    if (r > 0) {
        if (lineWidth > 0) {
            int x1, x2, x3, x4, y1, y2, dy;

            /* Thick, rounded rectangle. */
            x1 = x + r;
            x2 = x + w - r;
            y1 = y;
            y2 = y + h - 1;
            for (dy = 0; dy < lineWidth; dy++) {
                PaintHorizontalLine(picture, x1, x2, y1+dy, brush, blend);
                PaintHorizontalLine(picture, x1, x2, y2-dy, brush, blend);
            }
            x1 = x;
            x2 = x + lineWidth;
            x3 = x + w - lineWidth;
            x4 = x + w;
            for (dy = r; dy < (h - r); dy++) {
                PaintHorizontalLine(picture, x1, x2, y+dy, brush, blend);
                PaintHorizontalLine(picture, x3, x4, y+dy, brush, blend);
            }
        } else {
            int x1, x2, y1, y2, dy;

            /* Filled, rounded, rectangle. */
            x1 = x + r;
            x2 = x + w - r;
            y1 = y;
            y2 = y + h - 1;
            for (dy = 0; dy < r; dy++) {
                PaintHorizontalLine(picture, x1, x2, y1+dy, brush, blend);
                PaintHorizontalLine(picture, x1, x2, y2-dy, brush, blend);
            }
            x1 = x;
            x2 = x + w;
            for (dy = r; dy < (h - r); dy++) {
                PaintHorizontalLine(picture, x1, x2, y+dy, brush, blend);
            }
        }
        { 
            int x1, y1;
            int d;

            d = r + r;
            /* Draw the rounded corners. */
            x1 = x - 1;
            y1 = y - 1;
            PaintCorner(picture, x1, y1, r + 1, lineWidth, 0, brush);
            x1 = x + w - d - 2;
            y1 = y - 1;
            PaintCorner(picture, x1, y1, r + 1, lineWidth, 1, brush);
            x1 = x - 1;
            y1 = y + h - d - 2;
            PaintCorner(picture, x1, y1, r + 1, lineWidth, 2, brush);
            x1 = x + w - d - 2;
            y1 = y + h - d - 2;
            PaintCorner(picture, x1, y1, r + 1, lineWidth, 3, brush);
        }
    } else {
        if (lineWidth > 0) {
            int x1, x2, x3, x4, y1, y2, dy;
            
            /* Thick, non-rounded, rectangle.  */
            x1 = x;
            x2 = x + w;
            y1 = y;
            y2 = y + h - lineWidth;
            for (dy = 0; dy < lineWidth; dy++) {
                PaintHorizontalLine(picture, x1, x2, y1+dy, brush, blend);
                PaintHorizontalLine(picture, x1, x2, y2-dy, brush, blend);
            }
            x1 = x;
            x2 = x + lineWidth;
            x3 = x + w - lineWidth;
            x4 = x + w;
            for (dy = r; dy < (h - lineWidth); dy++) {
                PaintHorizontalLine(picture, x1, x2, y+dy, brush, blend);
                PaintHorizontalLine(picture, x3, x4, y+dy, brush, blend);
            }
        } else {
            int x1, x2, dy;

            /* Filled, non-rounded, rectangle. */
            x1 = x;
            x2 = x + w;
            for (dy = 0; dy < h; dy++) {
                PaintHorizontalLine(picture, x1, x2, y+dy, brush, blend);
            }
        }
    } 
}
