
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
    px = 0;				/* b2 * (dx + 1) */
    py = (double)(a2 + a2) * dy;	/* a2 * (dy - 0.5) */

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
	r = imul8x8(alpha, colorPtr->Red, u)   + imul8x8(beta, bp->Red, t);
	g = imul8x8(alpha, colorPtr->Green, u) + imul8x8(beta, bp->Green, t);
	b = imul8x8(alpha, colorPtr->Blue, u)  + imul8x8(beta, bp->Blue, t);
	a = imul8x8(alpha, colorPtr->Alpha, u) + imul8x8(beta, bp->Alpha, t);
	bp->Red = (r > 255) ? 255 : ((r < 0) ? 0 : r);
	bp->Green = (g > 255) ? 255 : ((g < 0) ? 0 : g);
	bp->Blue = (b > 255) ? 255 : ((b < 0) ? 0 : b);
	bp->Alpha = 0xFF;
    } 
}

static void INLINE
PaintPixel(Pict *destPtr, int x, int y, Blt_Pixel *colorPtr) 
{
    if ((x >= 0) && (x < destPtr->width) && (y >= 0) && (y < destPtr->height)) {
	MixColors(Blt_PicturePixel(destPtr, x, y), colorPtr);
    }
}

static void INLINE
PaintHorizontalLine(Pict *destPtr, int x1, int x2, int y, Blt_Paintbrush *brushPtr, 
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
	    int x;

	    for (x = x1; dp <= dend; dp++, x++) {
		Blt_Pixel color;

		color.u32 = Blt_Paintbrush_GetColor(brushPtr, x, y);
		MixPaint(dp, &color);
	    }
	} else {
	    int x;

	    for (x = x1; dp < dend; dp++, x++) {
		dp->u32 = Blt_Paintbrush_GetColor(brushPtr, x, y);
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
		MixColors(dp, colorPtr);
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
		MixColors(dp, colorPtr);
	    }
	} else {
	    for (/*empty*/; dp <= dend; dp += destPtr->pixelsPerRow) {
		dp->u32 = colorPtr->u32;
	    }
	}	    
    }
}

static void
PaintThickRoundedRectangle(
    Blt_Picture picture,
    int xOrigin, int yOrigin,	/* Upper left corner of rectangle. */
    int w, int h, int r,
    int lineWidth,
    Blt_Paintbrush *brushPtr)
{
    int x1, x2, y1, y2;
    int blend = 1;

    {
	int d;

	d = MIN(w, h) / 2;
	if (r > d) {
	    r = d;
	}
	if ((r > 0) && (r < lineWidth)) {
	    r = lineWidth;
	}
    }
#ifdef notdef
    fprintf(stderr, "Paint Thick Rounded Rectangle\n");
#endif
    /* Compute the coordinates of the four corner radii. */
    x1 = xOrigin + r;
    y1 = yOrigin + r;
    x2 = xOrigin + w - r - 1;
    y2 = yOrigin + h - r - 1;

    if (lineWidth > r) {
	int left, right;
	int dy;

	/* Draw the extra width (not including the radius). */
	left = xOrigin;
	right = xOrigin + w - 1;
	for (dy = 0; dy < (lineWidth - r); dy++) {
	    PaintHorizontalLine(picture, left, right, y1 + dy, brushPtr, blend);
	    PaintHorizontalLine(picture, left, right, y2 - dy, brushPtr, blend);
	}
    }
    {
	int y;
	int left, right, midleft, midright, midtop, midbottom;
	int mid;

	left = xOrigin;
	midleft = xOrigin + (lineWidth - 1);
	right = xOrigin + w - 1;
	midright = right - (lineWidth - 1);

	mid = MAX(r, lineWidth - 1);
	/* Draw the left/right edges of the interior rectangle */
	if (lineWidth > r) {
	    midtop = yOrigin + mid + 1;
	    midbottom = yOrigin + h - mid - 1;
	} else {
	    midtop = yOrigin + mid;
	    midbottom = yOrigin + h - mid;
	}
	for (y = midtop; y < midbottom; y++) {
	    PaintHorizontalLine(picture, left, midleft, y, brushPtr, blend);
	    PaintHorizontalLine(picture, midright, right, y, brushPtr, blend); 
	}
    }

    if (r > 0) {
	ScanLine *outer, *inner;
	int dy;
	int dx1, dx2;

	lineWidth--;
	outer = ComputeEllipseQuadrant(r, r);
	inner = ComputeEllipseQuadrant(r - lineWidth, r - lineWidth);
 
	dy = 1;
	if (lineWidth <= r) {
	    /* Draw the bend from the left/right edge to each radius. */
	    for (dy = 1; dy < (r - lineWidth); dy++) {
		dx1 = outer[dy].right, dx2 = inner[dy].left;
		PaintHorizontalLine(picture, x1-dx1, x1-dx2, y1-dy, brushPtr, blend);
		PaintHorizontalLine(picture, x2+dx2, x2+dx1, y1-dy, brushPtr, blend);
		PaintHorizontalLine(picture, x1-dx1, x1-dx2, y2+dy, brushPtr, blend);
		PaintHorizontalLine(picture, x2+dx2, x2+dx1, y2+dy, brushPtr, blend);
	    }
	}
	/* Draw the top/bottom bend from the line width to the radius */
	for (/* empty */; dy <= r; dy++) {
	    int dx;
	    
	    dx = outer[dy].right;
#ifdef notdef
	    fprintf(stderr, "r=%d, dx=%d w=%d x1=%d x2=%d\n", r, dx, w, x1, x2);
	    fprintf(stderr, "x0=%d\n", xOrigin);
#endif
	    PaintHorizontalLine(picture, x1 - dx, x2 + dx, y1 - dy, brushPtr, blend);
	    PaintHorizontalLine(picture, x1 - dx, x2 + dx, y2 + dy, brushPtr, blend);
	}
	Blt_Free(outer);
	Blt_Free(inner);
    }
}

static void
PaintRoundedRectangle(
    Blt_Picture picture,
    int xOrigin, int yOrigin, 
    int width, int height,
    int r,
    Blt_Paintbrush *brushPtr)
{
    int x1, x2, y1, y2; /* Centers of each corner */
    int blend = 1;

    {
	int w2, h2;

	/* Radius of each rounded corner can't be bigger than half the width
	 * or height of the rectangle. */
	w2 = width / 2;
	h2 = height / 2;
	if (r > w2) {
	    r = w2;
	}
	if (r > h2) {
	    r = h2;
	}
    }

    x1 = xOrigin + r;
    y1 = yOrigin + r;
    x2 = xOrigin + width - r - 1;
    y2 = yOrigin + height - r - 1;

    {    
	/* Rectangular interior. */
	int left, right;
	int y;

	left = xOrigin;
	right = xOrigin + width - 1;
	for (y = y1; y <= y2; y++) {
	    PaintHorizontalLine(picture, left, right, y, brushPtr, blend); 
	}
    }
    if (r > 0) {
	ScanLine *outer;
	int dy;

	outer = ComputeEllipseQuadrant(r, r);
	for (dy = 1; dy <= r; dy++) {
	    int dx;
	    
 	    dx = outer[dy].right;
	    PaintHorizontalLine(picture, x1 - dx, x2 + dx, y1 - dy, brushPtr, blend);
	    PaintHorizontalLine(picture, x1 - dx, x2 + dx, y2 + dy, brushPtr, blend);
	}
	Blt_Free(outer);
    }
}

#ifdef notdef
static Pict *
FilledCircle(int diam, Blt_Pixel *colorPtr)
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
    interior = PremultiplyAlpha(color, 255);
    HorizLine(destPtr, x - r, x + r, r, interior);    /* Center line */
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
	edge = PremultiplyAlpha(color, (unsigned int)CLAMP(q));

	/* By symmetry we can fill upper and lower scan lines. */
	PutPixel(destPtr, r - x - 1, r + y, edge);
	HorizLine(destPtr, r - x, r + x, r + y, interior);
	PutPixel(destPtr, r + x + 1, r + y, edge);


	PutPixel(destPtr, r - x - 1, r - y, edge);
	HorizLine(destPtr, r - x, r + x, r - y, interior);
	PutPixel(destPtr, r + x + 1, r - y, edge);

	t = d;
    }
    y1 = r - y;
    y2 = r + y; 
    x = 0;
    y = r;
    t = 0;
    VertLine(destPtr, r, 0, y1, interior);    /* Center line */
    VertLine(destPtr, r, y2, r + r, interior);    /* Center line */
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
	edge = PremultiplyAlpha(color, (unsigned int)CLAMP(q));

	/* By symmetry we can fill upper and lower scan lines. */
	PutPixel(destPtr, r - x, r - y - 1, edge);
	VertLine(destPtr, r - x, r - y, y1, interior); 
	VertLine(destPtr, r - x, y2, r + y, interior); 
	PutPixel(destPtr, r - x, r + y + 1, edge);

	PutPixel(destPtr, r + x, r - y - 1, edge);
	VertLine(destPtr, r + x, r - y, y1, interior); 
	VertLine(destPtr, r + x, y2, r + y, interior); 
	PutPixel(destPtr, r + x, r + y + 1, edge);

	t = d;
    }
    return destPtr;
}

static Pict *
FilledOval(int width, int height, Color color)
{
    Pict *destPtr;
    Color edge, interior;
    double t;
    int r, r2;
    int x, y;
    int diam, offset;
    int x1, x2, y1, y2;

    t = 0.0;
    interior = PremultiplyAlpha(color, 255);
    if (width < height) {
	diam = width;
	diam |= 0x1;
	r = diam / 2;
	r2 = r * r;
	width = diam;
	offset = height - width;
	destPtr = Blt_CreatePicture(width, height);
	/* Fill the center rectangle */
	y1 = r, y2 = height - r - 1;
	for (y = y1; y <= y2; y++) {
	    HorizLine(destPtr, 0, width - 1, y, interior);
	}
	x1 = x2 = r;
    } else {
	diam = height;
	diam |= 0x1;
	r = diam / 2;
	r2 = r * r;
	height = diam;
	offset = width - height;
	destPtr = Blt_CreatePicture(width, height);
	y1 = y2 = r;
	x1 = r, x2 = width - r - 1;
	HorizLine(destPtr, 0, width - 1, r, interior);    /* Center line */
	for (y = 0; y < y1; y++) {
	    HorizLine(destPtr, x1, x2, y, interior);
	}
	for (y = y2; y < height; y++) {
	    HorizLine(destPtr, x1, x2, y, interior);
	}
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
	edge = PremultiplyAlpha(color, (unsigned int)CLAMP(q));

	/* By symmetry we can fill upper and lower scan lines. */
	PutPixel(destPtr, x1 - x - 1, y2 + y, edge); 
	HorizLine(destPtr, x1 - x, x2 + x, y2 + y, interior);
	PutPixel(destPtr, x2 + x + 1, y2 + y, edge);

	PutPixel(destPtr, x1 - x - 1, y1 - y, edge);
	HorizLine(destPtr, x1 - x, x2 + x, y1 - y, interior);
	PutPixel(destPtr, x2 + x + 1, y1 - y, edge);
	t = d;
    }
    x = 0;
    y = r;
    t = 0;
    VertLine(destPtr, r, 0, y1, interior);    /* Center line */
    VertLine(destPtr, r, y2, height - 1, interior);    /* Center line */
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
	edge = PremultiplyAlpha(color, (unsigned int)CLAMP(q));

	/* By symmetry we can fill upper and lower scan lines. */
	PutPixel(destPtr, x1 - x, y1 - y - 1, edge);
	VertLine(destPtr, x1 - x, y1 - y, y1, interior); 
	VertLine(destPtr, x1 - x, y2, y2 + y, interior); 
	PutPixel(destPtr, x1 - x, y2 + y + 1, edge);

	PutPixel(destPtr, x2 + x, y1 - y - 1, edge);
	VertLine(destPtr, x2 + x, y1 - y, y1, interior); 
	VertLine(destPtr, x2 + x, y2, y2 + y, interior); 
	PutPixel(destPtr, x2 + x, y2 + y + 1, edge);

	t = d;
    }
    return destPtr;
}
#endif

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

static INLINE float 
sqr(float x) 
{
    return x * x;
}
    
static void
PaintCircle4(Pict *destPtr, float cx, float cy, float r, float lineWidth, 
	     Blt_Paintbrush *brushPtr, int blend)
{
    int x, y, i;
    int x1, x2, y1, y2;
    float outer, inner, outer2, inner2;
    float *squares;
    Blt_Pixel *destRowPtr;

    // Determine some helpful values (singles)
    outer = r;
    if (lineWidth > 0) {
	inner = r - lineWidth;
    } else {
	inner = 0;
    }
    outer2 = outer * outer;
    inner2 = inner * inner;

    // Determine bounds:
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
    // Optimization run: find squares of X first
    squares = Blt_AssertMalloc(sizeof(float) * (x2 - x1));
    for (i = 0, x = x1; x < x2; x++, i++) {
	squares[i] = (x - cx) * (x - cx);
    }
    // Loop through Y values
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
		if (dp->Alphas != 0) {
		    a = imul8x8(a, dp->Alpha, t);
		}
#endif
		color.u32 = Blt_Paintbrush_GetColor(brushPtr, x, y);
		BlendPixel(dp, &color, a);
	    } else {
		int t;
		a = UCLAMP(a);
		dp->u32 = Blt_Paintbrush_GetColor(brushPtr, x, y);
		dp->Alpha = imul8x8(a, dp->Alpha, t);
	    }
	}
	destRowPtr += destPtr->pixelsPerRow;
    }
    destPtr->flags &= ~BLT_PIC_ASSOCIATED_COLORS;
    Blt_Free(squares);
}

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
    dp = Blt_PicturePixel(destPtr, x + xoff - 1, y + yoff);
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
	dp = Blt_PicturePixel(destPtr, x + xoff, y + yoff);
	q = FABS(d * 255.0);
	a = (unsigned int)CLAMP(q);
	BlendPixel(dp, colorPtr, a);
	dp--;			/* x - 1 */
	a = (unsigned int)CLAMP(255.0 - q);
	BlendPixel(dp, colorPtr, a);
	t = d;
	x1++;
	if (fill) {
	    PaintLineSegment(destPtr, x1, y + yoff, x + xoff - 1, y + yoff, 1, 
			     colorPtr);
	}
    }
}

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
		MixColors(dp, &fill); /* Interior */
		continue;
	    }
	    z = (double)r - sqrt((double)d2);
	    count++;
	    if (z > 1.0) {
		MixColors(dp, &fill); /* Interior */
	    } else if (z > 0.0) {
		double q;
		unsigned char a;
		Blt_Pixel edge;

		q = z * 255.0;
		a = (unsigned int)CLAMP(q);	
		edge = PremultiplyAlpha(colorPtr, a);
		MixColors(dp, &edge); /* Edge */
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

static void
PaintThickEllipse(
    Blt_Picture picture, 
    int x, int y,		/* Center of the ellipse. */
    int a,			/* Half the width of the ellipse. */
    int b,			/* Half the height of the ellipse. */
    int lineWidth,		/* Line width of the ellipse. Must be 1 or
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
    int x, int y,		/* Center of the ellipse. */
    int a,			/* Half the width of the ellipse. */
    int b,			/* Half the height of the ellipse. */
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
    int x, int y,		/* Center of the ellipse. */
    int a,			/* Half the width of the ellipse. */
    int b,			/* Half the height of the ellipse. */
    int lineWidth,		/* Line width of the ellipse.  If zero,
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
    int x, int y,		/* Center of the ellipse. */
    int a,			/* Half the width of the ellipse. */
    int b,			/* Half the height of the ellipse. */
    int lineWidth,		/* Line thickness of the ellipse.  If zero,
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
	return;			/* Ellipse is totally clipped. */
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
	PaintEllipse(big, 
	    cx * numSamples,	/* Center of ellipse. */
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
	Blt_BlendPictures(picture, tmp, 0, 0, region.w, region.h, 
		region.x, region.y);
	Blt_FreePicture(tmp);
    }
}

static void
PaintCircle(
    Blt_Picture picture, 
    int x, int y,		/* Center of the circle in picture. */
    int r,			/* Radius of the circle. */
    int lineWidth,		/* Line width of the circle.  */
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


void
Blt_PaintRectangle(
    Blt_Picture picture, 
    int x, int y,		/* Upper left corner of rectangle. */
    int width, int height,	/* Dimension of rectangle. */
    int r,			/* Radius of rounded corner. If zero, draw
				 * square corners. */
    int lineWidth,		/* Line width of the rectangle.  If zero, then
				 * draw a solid fill rectangle. */
    Blt_Paintbrush *brushPtr)
{
    if (((lineWidth*2) >= width) || ((lineWidth*2) >= height)) {
	lineWidth = 0;		/* Paint solid rectangle instead.*/
    }
    if (lineWidth == 0) {
	PaintRoundedRectangle(picture, x, y, width, height, r, brushPtr);
    } else {
	PaintThickRoundedRectangle(picture, x, y, width, height, r, 
		lineWidth, brushPtr);
    }
}

static void
PaintRectangleAA(
    Blt_Picture picture, 
    int x, int y,			/* Upper left corner of rectangle. */
    int w, int h,			/* Dimension of rectangle. */
    int r,				/* Radius of rounded corner. If zero,
					 * draw square corners. */
    int lineWidth,			/* Line width of the rectangle.  If
					 * zero, then draw a solid fill
					 * rectangle. */
    Blt_Paintbrush *brushPtr)
{
    Blt_Picture big;			/* Super-sampled background. */
    int numSamples = 4; 
    Blt_Picture tmp;
    int offset = 4;

    if (((lineWidth*2) >= w) || ((lineWidth*2) >= h)) {
	lineWidth = 0;			/* Paint solid rectangle instead.*/
    }
    if (r < 0) {
	Blt_PaintRectangle(picture, x, y, w, h, r, lineWidth, brushPtr);
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
    Blt_Paintbrush_Region(brushPtr, 0, 0, w * numSamples, h * numSamples);
    Blt_PaintRectangle(big,		/* Contains super-sampled original
					 * background. */
		       0, 0,
		       w * numSamples,	/* Scaled width */
		       h * numSamples,	/* Scaled height */
		       r * numSamples,	/* Scaled radius. */
		       lineWidth * numSamples, /* Scaled line width. */
		       brushPtr); 
	    
   /* Reduce the picture back to its original size using a simple box
    * filter for smoothing. */
   tmp = Blt_CreatePicture(w+2*offset, h+2*offset);
   Blt_ResamplePicture(tmp, big, bltBoxFilter, bltBoxFilter);
	
   /* Replace the bounding box in the original with the new. */
   Blt_BlendPictures(picture, tmp, 0, 0, w+offset, h+offset, x, y);
   Blt_FreePicture(big);
   Blt_FreePicture(tmp);
}

static void
PaintRectangleShadow(
    Blt_Picture picture, 
    int x, int y,		/* Upper left corner of rectangle. */
    int w, int h,		/* Dimension of rectangle. */
    int r,			/* Radius of rounded corner. If zero, draw
				 * square corners. */
    int lineWidth,
    Blt_Shadow *shadowPtr)
{
    int dw, dh;
#ifdef notdef
    Blt_Picture tmp;
#endif
    Blt_Picture blur;
    Blt_Paintbrush brush;

    dw = (w + shadowPtr->offset*2);
    dh = (h + shadowPtr->offset*2);
    blur = Blt_CreatePicture(dw, dh);
    Blt_BlankPicture(blur, 0x0);
    Blt_Paintbrush_Init(&brush);
    Blt_Paintbrush_SetColor(&brush, shadowPtr->color.u32);
#ifdef notdef
    /* 
     * draw the rectangle.
     * blur into a slightly bigger picture (for blur width).
     * mask off original rectangle, leaving fringe blur.
     * blend into destination.
     */
    tmp = Blt_CreatePicture(dw, dh);
    Blt_BlankPicture(tmp, 0x0);
    color.Alpha = 0xFF;
    Blt_PaintRectangle(tmp, 0, 0, w, h, r, lineWidth, &color);
    Blt_CopyPictureBits(blur, tmp, 0, 0, w, h, shadowPtr->offset*2, 
			shadowPtr->offset*2);
    Blt_BlurPicture(blur, blur, shadowPtr->offset, 3);
    color.u32 = 0x00;
    Blt_MaskPicture(blur, tmp, 0, 0, w, h, 0, 0, &color);
    Blt_BlendPictures(picture, blur, 0, 0, dw, dh, x, y);
    Blt_FreePicture(blur);
#else
    Blt_PaintRectangle(blur, shadowPtr->offset, shadowPtr->offset, w, h, r, 
		   lineWidth, &brush);
    Blt_BlurPicture(blur, blur, shadowPtr->offset, 3);
    Blt_BlendPictures(picture, blur, 0, 0, dw, dh, x, y);
    Blt_FreePicture(blur);
#endif
}

