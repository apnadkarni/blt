/* -*- mode: c; c-basic-offset: 4; indent-tabs-mode: nil -*- */

#define BUILD_BLT_TK_PROCS 1
#include "bltInt.h"

#ifdef HAVE_STDLIB_H
#  include <stdlib.h>
#endif /* HAVE_STDLIB_H */

#ifdef HAVE_STRING_H
#  include <string.h>
#endif /* HAVE_STRING_H */

#include "bltAlloc.h"
#include "bltMath.h"
#include "bltChain.h"
#include "bltHash.h"
typedef struct _Graph Graph;
#include "bltGrMesh.h"

#define DELETED		((Edge *)-2)
#define LE 0
#define RE 1

#define Dist(p,q)    \
	hypot((p)->point.x - (q)->point.x, (p)->point.y - (q)->point.y)

typedef struct _HalfEdge HalfEdge;
typedef struct _FreeNode FreeNode;

struct _FreeNode {
    FreeNode *nextPtr;
};

typedef struct {
    FreeNode *headPtr;
    int nodesize;
} FreeList;

typedef struct {
    Point2d point;
    int neighbor;
    int refCount;
} Site;

typedef struct {
    double a, b, c;
    Site *ep[2];
    Site *leftReg, *rightReg;
    int neighbor;
} Edge;

struct _HalfEdge {
    HalfEdge *leftPtr, *rightPtr;
    Edge *edgePtr;
    int refCount;
    int pm;
    Site *vertex;
    double ystar;
    HalfEdge *pqNext;
};

/* Static variables */

typedef struct {
    double xMin, xMax, yMin, yMax, xDelta, yDelta;
    Site *sites;
    int numSites;
    int siteIndex;
    int sqrtNumSites;
    int numVertices;
    Site *bottomsite;
    int numEdges;
    FreeList freeSites;
    FreeList freeEdges;
    FreeList freeHalfEdges;
    HalfEdge *elLeftEnd, *elRightEnd;
    int elHashsize;
    HalfEdge **elHash;
    int pqHashsize;
    HalfEdge *pqHash;
    int pqCount;
    int pqMin;
    Blt_Chain allocChain;
} Voronoi;

static void
FreeInit(FreeList *listPtr, int size)
{
    listPtr->headPtr = NULL;
    listPtr->nodesize = size;
}

static void
InitMemorySystem(Voronoi *vPtr)
{
    if (vPtr->allocChain == NULL) {
	vPtr->allocChain = Blt_Chain_Create();
    }
    FreeInit(&vPtr->freeSites, sizeof(Site));
}

static void *
AllocMemory(Voronoi *vPtr, size_t size)
{
    void *ptr;

    ptr = Blt_Malloc(size);
    if (ptr == NULL) {
	return NULL;
    }
    Blt_Chain_Append(vPtr->allocChain, ptr);
    return ptr;
}
    
static void
ReleaseMemorySystem(Voronoi *vPtr)
{
    Blt_ChainLink link;

    for (link = Blt_Chain_LastLink(vPtr->allocChain); link != NULL;
	link = Blt_Chain_PrevLink(link)) {
	void *ptr;

	ptr = Blt_Chain_GetValue(link);
	if (ptr != NULL) {
	    Blt_Free(ptr);
	}
    }
    Blt_Chain_Destroy(vPtr->allocChain);
    vPtr->allocChain = NULL;
}

INLINE static void
MakeFree(void *item, FreeList *listPtr)
{
    FreeNode *nodePtr = item;

    nodePtr->nextPtr = listPtr->headPtr;
    listPtr->headPtr = nodePtr;
}

static void *
GetFree(Voronoi *vPtr, FreeList *listPtr)
{
    FreeNode *nodePtr;

    if (listPtr->headPtr == NULL) {
	int i;

	nodePtr = AllocMemory(vPtr, vPtr->sqrtNumSites * listPtr->nodesize);
	/* Thread the free nodes as a list */
	for (i = 0; i < vPtr->sqrtNumSites; i++) {
	    MakeFree(((char *)nodePtr + i * listPtr->nodesize), listPtr);
	}
    }
    nodePtr = listPtr->headPtr;
    listPtr->headPtr = listPtr->headPtr->nextPtr;
    return nodePtr;
}

INLINE static void
DecrRefCount(Voronoi *vPtr, Site *vertexPtr)
{
    vertexPtr->refCount--;
    if (vertexPtr->refCount == 0) {
	MakeFree(vertexPtr, &vPtr->freeSites);
    }
}

INLINE static void
IncrRefCount(Site *vertexPtr)
{
    vertexPtr->refCount++;
}

INLINE static HalfEdge *
CreateHalfEdge(Voronoi *vPtr, Edge *edgePtr, int pm)
{
    HalfEdge *halfPtr;

    halfPtr = GetFree(vPtr, &vPtr->freeHalfEdges);
    halfPtr->edgePtr = edgePtr;
    halfPtr->pm = pm;
    halfPtr->pqNext = NULL;
    halfPtr->vertex = NULL;
    halfPtr->refCount = 0;
    return halfPtr;
}


static void
ElInitialize(Voronoi *vPtr)
{
    FreeInit(&vPtr->freeHalfEdges, sizeof(HalfEdge));

    vPtr->elHashsize = 2 * vPtr->sqrtNumSites;
    vPtr->elHash = AllocMemory(vPtr, vPtr->elHashsize * sizeof(HalfEdge *));
    assert(vPtr->elHash);
    memset(vPtr->elHash, 0, vPtr->elHashsize * sizeof(HalfEdge *));

    vPtr->elLeftEnd = CreateHalfEdge(vPtr, (Edge *)NULL, 0);
    vPtr->elRightEnd = CreateHalfEdge(vPtr, (Edge *)NULL, 0);
    vPtr->elLeftEnd->leftPtr = NULL;
    vPtr->elLeftEnd->rightPtr = vPtr->elRightEnd;
    vPtr->elRightEnd->leftPtr = vPtr->elLeftEnd;
    vPtr->elRightEnd->rightPtr = NULL;
    vPtr->elHash[0] = vPtr->elLeftEnd;
    vPtr->elHash[vPtr->elHashsize - 1] = vPtr->elRightEnd;
}

INLINE static void
ElInsert(HalfEdge *lb, HalfEdge *edgePtr)
{
    edgePtr->leftPtr = lb;
    edgePtr->rightPtr = lb->rightPtr;
    lb->rightPtr->leftPtr = edgePtr;
    lb->rightPtr = edgePtr;
}

static HalfEdge *
ElGetHash(Voronoi *vPtr, int b)
{
    HalfEdge *halfPtr;

    if ((b < 0) || (b >= vPtr->elHashsize)) {
	return NULL;
    }
    halfPtr = vPtr->elHash[b];
    if ((halfPtr == NULL) || (halfPtr->edgePtr != DELETED)) {
	return halfPtr;
    }
    /* Hash table points to deleted half edge.  Patch as necessary. */

    vPtr->elHash[b] = NULL;
    halfPtr->refCount--;
    if (halfPtr->refCount == 0) {
	MakeFree(halfPtr, &vPtr->freeHalfEdges);
    }
    return NULL;
}

static int
RightOf(HalfEdge *halfPtr, Point2d *p)
{
    Edge *e;
    Site *topsite;
    int rightOfSite, above, fast;
    double dxp, dyp, dxs, t1, t2, t3, yl;

    e = halfPtr->edgePtr;
    topsite = e->rightReg;
    rightOfSite = p->x > topsite->point.x;
    if ((rightOfSite) && (halfPtr->pm == LE)) {
	return 1;
    }
    if ((!rightOfSite) && (halfPtr->pm == RE)) {
	return 0;
    }
    if (e->a == 1.0) {
	dyp = p->y - topsite->point.y;
	dxp = p->x - topsite->point.x;
	fast = 0;
	if ((!rightOfSite && (e->b < 0.0)) || (rightOfSite && (e->b >= 0.0))) {
	    above = dyp >= e->b * dxp;
	    fast = above;
	} else {
	    above = p->x + p->y * e->b > e->c;
	    if (e->b < 0.0) {
		above = !above;
	    }
	    if (!above) {
		fast = 1;
	    }
	}
	if (!fast) {
	    dxs = topsite->point.x - (e->leftReg)->point.x;
	    above = e->b * (dxp * dxp - dyp * dyp) <
		dxs * dyp * (1.0 + 2.0 * dxp / dxs + e->b * e->b);
	    if (e->b < 0.0) {
		above = !above;
	    }
	}
    } else {			/* e->b==1.0 */
	yl = e->c - e->a * p->x;
	t1 = p->y - yl;
	t2 = p->x - topsite->point.x;
	t3 = yl - topsite->point.y;
	above = t1 * t1 > t2 * t2 + t3 * t3;
    }
    return (halfPtr->pm == LE ? above : !above);
}

static HalfEdge *
ElLeftBnd(Voronoi *vPtr, Point2d *p)
{
    int i, bucket;
    HalfEdge *halfPtr;

    /* Use hash table to get close to desired halfedge */

    bucket = (p->x - vPtr->xMin) / vPtr->xDelta * vPtr->elHashsize;
    if (bucket < 0) {
	bucket = 0;
    } else if (bucket >= vPtr->elHashsize) {
	bucket = vPtr->elHashsize - 1;
    }
    halfPtr = ElGetHash(vPtr, bucket);
    if (halfPtr == NULL) {
	for (i = 1; /* empty */ ; i++) {
	    halfPtr = ElGetHash(vPtr, bucket - i);
	    if (halfPtr != NULL) {
		break;
	    }
	    halfPtr = ElGetHash(vPtr, bucket + i);
	    if (halfPtr != NULL) {
		break;
	    }
	}
    }

    /* Now search linear list of halfedges for the correct one */

    if ((halfPtr == vPtr->elLeftEnd) || 
	(halfPtr != vPtr->elRightEnd && RightOf(halfPtr, p))) {
	do {
	    halfPtr = halfPtr->rightPtr;
	} while ((halfPtr != vPtr->elRightEnd) && (RightOf(halfPtr, p)));
	halfPtr = halfPtr->leftPtr;
    } else {
	do {
	    halfPtr = halfPtr->leftPtr;
	} while ((halfPtr != vPtr->elLeftEnd) && (!RightOf(halfPtr, p)));
    }

    /* Update hash table and reference counts */

    if ((bucket > 0) && (bucket < (vPtr->elHashsize - 1))) {
	if (vPtr->elHash[bucket] != NULL) {
	    vPtr->elHash[bucket]->refCount--;
	}
	vPtr->elHash[bucket] = halfPtr;
	vPtr->elHash[bucket]->refCount++;
    }
    return halfPtr;
}

/* 
 * This delete routine can't reclaim node, since pointers from hash table may
 * be present.
 */
INLINE static void
ElDelete(HalfEdge *halfPtr)
{
    halfPtr->leftPtr->rightPtr = halfPtr->rightPtr;
    halfPtr->rightPtr->leftPtr = halfPtr->leftPtr;
    halfPtr->edgePtr = DELETED;
}

INLINE static Site *
LeftRegion(Voronoi *vPtr, HalfEdge *halfPtr)
{
    if (halfPtr->edgePtr == NULL) {
	return vPtr->bottomsite;
    }
    return (halfPtr->pm == LE) ? halfPtr->edgePtr->leftReg : halfPtr->edgePtr->rightReg;
}

INLINE static Site *
RightRegion(Voronoi *vPtr, HalfEdge *halfPtr)
{
    if (halfPtr->edgePtr == NULL) {
	return vPtr->bottomsite;
    }
    return (halfPtr->pm == LE) ? halfPtr->edgePtr->rightReg : halfPtr->edgePtr->leftReg;
}

static void
GeomInit(Voronoi *vPtr)
{
    FreeInit(&vPtr->freeEdges, sizeof(Edge));
    vPtr->numVertices = vPtr->numEdges = 0;
    vPtr->sqrtNumSites = sqrt((double)(vPtr->numSites + 4));
    vPtr->yDelta = vPtr->xMax - vPtr->xMax;
    vPtr->xDelta = vPtr->yMax - vPtr->yMin;
}

static Edge *
Bisect(Voronoi *vPtr, Site *s1, Site *s2)
{
    double dx, dy, adx, ady;
    Edge *edgePtr;

    edgePtr = GetFree(vPtr, &vPtr->freeEdges);

    edgePtr->leftReg = s1;
    edgePtr->rightReg = s2;
    IncrRefCount(s1);
    IncrRefCount(s2);
    edgePtr->ep[0] = edgePtr->ep[1] = NULL;

    dx = s2->point.x - s1->point.x;
    dy = s2->point.y - s1->point.y;
    adx = FABS(dx);
    ady = FABS(dy);
    edgePtr->c = (s1->point.x * dx) + (s1->point.y * dy) +
	((dx * dx) + (dy * dy)) * 0.5;
    if (adx > ady) {
	edgePtr->a = 1.0;
	edgePtr->b = dy / dx;
	edgePtr->c /= dx;
    } else {
	edgePtr->b = 1.0;
	edgePtr->a = dx / dy;
	edgePtr->c /= dy;
    }

    edgePtr->neighbor = vPtr->numEdges;
    vPtr->numEdges++;
    return edgePtr;
}

static Site *
Intersect(Voronoi *vPtr, HalfEdge *halfPtr1, HalfEdge *halfPtr2)
{
    Edge *e1, *e2, *e;
    HalfEdge *halfPtr;
    double d, xint, yint;
    int rightOfSite;
    Site *sitePtr;

    e1 = halfPtr1->edgePtr;
    e2 = halfPtr2->edgePtr;
    if ((e1 == NULL) || (e2 == NULL)) {
	return NULL;
    }
    if (e1->rightReg == e2->rightReg) {
	return NULL;
    }
    d = (e1->a * e2->b) - (e1->b * e2->a);
    if ((-1.0e-10 < d) && (d < 1.0e-10)) {
	return NULL;
    }
    xint = ((e1->c * e2->b) - (e2->c * e1->b)) / d;
    yint = ((e2->c * e1->a) - (e1->c * e2->a)) / d;

    if ((e1->rightReg->point.y < e2->rightReg->point.y) ||
	((e1->rightReg->point.y == e2->rightReg->point.y) &&
	    (e1->rightReg->point.x < e2->rightReg->point.x))) {
	halfPtr = halfPtr1;
	e = e1;
    } else {
	halfPtr = halfPtr2;
	e = e2;
    }
    rightOfSite = (xint >= e->rightReg->point.x);
    if ((rightOfSite && halfPtr->pm == LE) || (!rightOfSite && halfPtr->pm == RE)) {
	return NULL;
    }
    sitePtr = GetFree(vPtr, &vPtr->freeSites);
    sitePtr->refCount = 0;
    sitePtr->point.x = xint;
    sitePtr->point.y = yint;
    return sitePtr;
}

INLINE static void
EndPoint(Voronoi *vPtr, Edge *e, int lr, Site *s)
{
    e->ep[lr] = s;
    IncrRefCount(s);
    if (e->ep[RE - lr] == NULL) {
	return;
    }
    DecrRefCount(vPtr, e->leftReg);
    DecrRefCount(vPtr, e->rightReg);
    MakeFree(e, &vPtr->freeEdges);
}

INLINE static void
MakeVertex(Voronoi *vPtr, Site *vertex)
{
    vertex->neighbor = vPtr->numVertices;
    vPtr->numVertices++;
}

static int
PQBucket(Voronoi *vPtr, HalfEdge *halfPtr)
{
    int bucket;

    bucket = (halfPtr->ystar - vPtr->yMin) / vPtr->yDelta * vPtr->pqHashsize;
    if (bucket < 0) {
	bucket = 0;
    }
    if (bucket >= vPtr->pqHashsize) {
	bucket = vPtr->pqHashsize - 1;
    }
    if (bucket < vPtr->pqMin) {
	vPtr->pqMin = bucket;
    }
    return bucket;
}

static void
PQInsert(Voronoi *v, HalfEdge *halfPtr, Site *vertex, double offset)
{
    HalfEdge *last, *next;

    halfPtr->vertex = vertex;
    IncrRefCount(vertex);
    halfPtr->ystar = vertex->point.y + offset;
    last = v->pqHash + PQBucket(v, halfPtr);
    while (((next = last->pqNext) != NULL) &&
	   ((halfPtr->ystar > next->ystar) || 
	    ((halfPtr->ystar == next->ystar) &&
	     (vertex->point.x > next->vertex->point.x)))) {
	last = next;
    }
    halfPtr->pqNext = last->pqNext;
    last->pqNext = halfPtr;
    v->pqCount++;
}

static void
PQDelete(Voronoi *vPtr, HalfEdge *halfPtr)
{
    if (halfPtr->vertex != NULL) {
	HalfEdge *last;

	last = vPtr->pqHash + PQBucket(vPtr, halfPtr);
	while (last->pqNext != halfPtr) {
	    last = last->pqNext;
	}
	last->pqNext = halfPtr->pqNext;
	vPtr->pqCount--;
	DecrRefCount(vPtr, halfPtr->vertex);
	halfPtr->vertex = NULL;
    }
}

INLINE static int
PQEmpty(Voronoi *vPtr)
{
    return (vPtr->pqCount == 0);
}

INLINE static Point2d
PQMin(Voronoi *vPtr)
{
    Point2d p;

    while (vPtr->pqHash[vPtr->pqMin].pqNext == NULL) {
	vPtr->pqMin++;
    }
    p.x = vPtr->pqHash[vPtr->pqMin].pqNext->vertex->point.x;
    p.y = vPtr->pqHash[vPtr->pqMin].pqNext->ystar;
    return p;
}

INLINE static HalfEdge *
PQExtractMin(Voronoi *v)
{
    HalfEdge *curr;

    curr = v->pqHash[v->pqMin].pqNext;
    v->pqHash[v->pqMin].pqNext = curr->pqNext;
    v->pqCount--;
    return curr;
}

static void
PQInitialize(Voronoi *vPtr)
{
    size_t numBytes;

    vPtr->pqCount = vPtr->pqMin = 0;
    vPtr->pqHashsize = 4 * vPtr->sqrtNumSites;
    numBytes = vPtr->pqHashsize * sizeof(HalfEdge);
    vPtr->pqHash = AllocMemory(vPtr, numBytes);
    assert(vPtr->pqHash);
    memset(vPtr->pqHash, 0, numBytes);
}

INLINE static Site *
NextSite(Voronoi *vPtr)
{
    if (vPtr->siteIndex < vPtr->numSites) {
	Site *s;

	s = vPtr->sites + vPtr->siteIndex;
	vPtr->siteIndex++;
	return s;
    }
    return NULL;
}

static int
ComputeVoronoi(Voronoi *vPtr, MeshTriangle *triplets)
{
    Site *newsite, *bot, *top, *temp, *p;
    Site *vertex;
    Point2d newintstar;
    int pm, count = 0;
    HalfEdge *lbnd, *rbnd, *llbnd, *rrbnd, *bisector;
    Edge *e;

    PQInitialize(vPtr);
    vPtr->bottomsite = NextSite(vPtr);
    ElInitialize(vPtr);

    newsite = NextSite(vPtr);
    for (;;) {
	if (!PQEmpty(vPtr)) {
	    newintstar = PQMin(vPtr);
	}
	if ((newsite != NULL) && 
	    ((PQEmpty(vPtr)) || 
	     (newsite->point.y < newintstar.y) ||
	     ((newsite->point.y == newintstar.y) && 
	      (newsite->point.x < newintstar.x)))) {

	    /* New site is smallest */

	    lbnd = ElLeftBnd(vPtr, &newsite->point);
	    rbnd = lbnd->rightPtr;
	    bot = RightRegion(vPtr, lbnd);
	    e = Bisect(vPtr, bot, newsite);
	    bisector = CreateHalfEdge(vPtr, e, LE);
	    ElInsert(lbnd, bisector);
	    p = Intersect(vPtr, lbnd, bisector);
	    if (p != NULL) {
		PQDelete(vPtr, lbnd);
		PQInsert(vPtr, lbnd, p, Dist(p, newsite));
	    }
	    lbnd = bisector;
	    bisector = CreateHalfEdge(vPtr, e, RE);
	    ElInsert(lbnd, bisector);
	    p = Intersect(vPtr, bisector, rbnd);
	    if (p != NULL) {
		PQInsert(vPtr, bisector, p, Dist(p, newsite));
	    }
	    newsite = NextSite(vPtr);
	} else if (!PQEmpty(vPtr)) {

	    /* Intersection is smallest */

	    lbnd = PQExtractMin(vPtr);
	    llbnd = lbnd->leftPtr;
	    rbnd = lbnd->rightPtr;
	    rrbnd = rbnd->rightPtr;
	    bot = LeftRegion(vPtr, lbnd);
	    top = RightRegion(vPtr, rbnd);
	    triplets[count].a = bot->neighbor;
	    triplets[count].b = top->neighbor;
	    triplets[count].c = RightRegion(vPtr, lbnd)->neighbor;
	    ++count;
	    vertex = lbnd->vertex;
	    MakeVertex(vPtr, vertex);
	    EndPoint(vPtr, lbnd->edgePtr, lbnd->pm, vertex);
	    EndPoint(vPtr, rbnd->edgePtr, rbnd->pm, vertex);
	    ElDelete(lbnd);
	    PQDelete(vPtr, rbnd);
	    ElDelete(rbnd);
	    pm = LE;
	    if (bot->point.y > top->point.y) {
		temp = bot, bot = top, top = temp;
		pm = RE;
	    }
	    e = Bisect(vPtr, bot, top);
	    bisector = CreateHalfEdge(vPtr, e, pm);
	    ElInsert(llbnd, bisector);
	    EndPoint(vPtr, e, RE - pm, vertex);
	    DecrRefCount(vPtr, vertex);
	    p = Intersect(vPtr, llbnd, bisector);
	    if (p != NULL) {
		PQDelete(vPtr, llbnd);
		PQInsert(vPtr, llbnd, p, Dist(p, bot));
	    }
	    p = Intersect(vPtr, bisector, rrbnd);
	    if (p != NULL) {
		PQInsert(vPtr, bisector, p, Dist(p, bot));
	    }
	} else {
	    break;
	}
    }
    return count;
}

static int
CompareSites(const void *a, const void *b)
{
    const Site *s1 = a;
    const Site *s2 = b;

    if (s1->point.y < s2->point.y) {
	return -1;
    }
    if (s1->point.y > s2->point.y) {
	return 1;
    }
    if (s1->point.x < s2->point.x) {
	return -1;
    }
    if (s1->point.x > s2->point.x) {
	return 1;
    }
    return 0;
}

int
Blt_Triangulate(Tcl_Interp *interp, size_t numPoints, Point2d *points, 
		int sorted, MeshTriangle *triangles)
{
    int i, n;
    Voronoi voronoi;
    
    memset(&voronoi, 0, sizeof(voronoi));

    InitMemorySystem(&voronoi);

    voronoi.numSites = numPoints;
    voronoi.sites = AllocMemory(&voronoi, numPoints * sizeof(Site));
    for (i = 0; i < numPoints; i++) {
	voronoi.sites[i].point.x = points[i].x;
	voronoi.sites[i].point.y = points[i].y;
	voronoi.sites[i].neighbor = i;
	voronoi.sites[i].refCount = 0;
    }
    if (!sorted) {
	qsort(voronoi.sites, voronoi.numSites, sizeof(Site), CompareSites);
    }
    /* Sites are sorted by y-coordinate. */
    voronoi.yMin = voronoi.sites[0].point.y;
    voronoi.yMax = voronoi.sites[numPoints - 1].point.y;
    voronoi.xMin = voronoi.xMax = voronoi.sites[0].point.x;
    for (i = 1; i < numPoints; i++) {
	if (voronoi.sites[i].point.x < voronoi.xMin) {
	    voronoi.xMin = voronoi.sites[i].point.x;
	} else if (voronoi.sites[i].point.x > voronoi.xMax) {
	    voronoi.xMax = voronoi.sites[i].point.x;
	}
    }
    voronoi.siteIndex = 0;
    GeomInit(&voronoi);
    n = ComputeVoronoi(&voronoi, triangles);
    /* Release memory allocated for triangulation */
    ReleaseMemorySystem(&voronoi);
    return n;
}
