/* -*- mode: c; c-basic-offset: 4; indent-tabs-mode: nil -*- */

/*
 * bltPool.c --
 *
 *	Copyright 2001-2004 George A Howlett.
 *
 *	Permission is hereby granted, free of charge, to any person
 *	obtaining a copy of this software and associated documentation
 *	files (the "Software"), to deal in the Software without
 *	restriction, including without limitation the rights to use,
 *	copy, modify, merge, publish, distribute, sublicense, and/or
 *	sell copies of the Software, and to permit persons to whom the
 *	Software is furnished to do so, subject to the following
 *	conditions:
 *
 *	The above copyright notice and this permission notice shall be
 *	included in all copies or substantial portions of the
 *	Software.
 *
 *	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY
 *	KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
 *	WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR
 *	PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS
 *	OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR
 *	OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
 *	OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 *	SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#define BUILD_BLT_TCL_PROCS 1
#include "bltInt.h"
#include "bltAlloc.h"
#include "bltPool.h"

/*
 * Blt_Pool --
 *
 *	Implements a pool memory allocator. 
 *
 *	  + It's faster allocating memory since malloc/free are called
 *	    only a fraction of the normal times.  Fixed size items can 
 *	    be reused without deallocating/reallocating memory.
 *	  + You don't have the extra 8-16 byte overhead per malloc. 
 *	  - Memory is freed only when the entire pool is destroyed.
 *	  - Memory is allocated in chunks. More memory is allocated 
 *	    than used.  
 *	  0 Depending upon allocation/deallocation patterns, locality
 *	    may be improved or degraded.
 *
 *      The pool is a chain of malloc'ed blocks.
 *
 *             +---------+  +---------+  +---------+  
 *       NULL<-| nextPtr |<-| nextPtr |<-| nextPtr |<- headPtr
 *             |---------|  |---------|  |---------|  
 *             | chunk1  |  | chunk2  |  | chunk3  |  
 *             +---------+  |         |  |         |  
 *                          +---------+  |         |  
 *                                       |         |  
 *                                       |         |  
 *                                       +---------+  
 *
 *      Each chunk contains an integral number of fixed size items.
 *	The number of items doubles until a maximum size is reached
 *      (each subsequent new chunk will be the maximum).  Chunks
 *	are allocated only when needed (no more space is available
 *	in the last chunk).
 *
 *	The chain of blocks is only freed when the entire pool is
 *	destroyed.  
 *
 *      A freelist of unused items also maintained. Each freed item
 *	is prepended to a free list.  Before allocating new chunks
 *	the freelist is examined to see if any unused items exist.
 *
 *               chunk1       chunk2       chunk3
 *            +---------+  +---------+  +---------+  
 *      NULL<-| unused  |  |         |  |         |
 *            +----^----+  +---------+  +---------+  
 *            | unused  |<-| unused  |<-| unused  |       
 *            +---------+  +---------+  +----^----+  
 *            |         |  |         |  | unused  |
 *            +---------+  |         |  +----^----+
 *                         |         |  |    |    |
 *                         +---------+  +----|----+
 *                                      | usused  |<- freePtr
 *                                      +---------+  
 */

#define POOL_MAX_CHUNK_SIZE      ((1<<16) - sizeof(PoolChain))

#ifndef ALIGN
#define ALIGN(a) \
	(((size_t)a + (sizeof(void *) - 1)) & (~(sizeof(void *) - 1)))
#endif /* ALIGN */

typedef struct _PoolChain {
   struct _PoolChain *nextPtr;
} PoolChain;

typedef struct {
    Blt_PoolAllocProc *allocProc;
    Blt_PoolFreeProc *freeProc;

    PoolChain *headPtr;		/* Chain of malloc'ed chunks. */
    PoolChain *freePtr; 	/* List of deleted items. This is only used
				 * for fixed size items. */
    size_t poolSize;		/* Log2 of # of items in the current block. */
    size_t itemSize;		/* Size of an item. */
    size_t bytesLeft;		/* # of bytes left in the current chunk. */
    size_t waste;
} Pool;

static Blt_PoolAllocProc VariablePoolAllocItem;
static Blt_PoolFreeProc  VariablePoolFreeItem;
static Blt_PoolAllocProc FixedPoolAllocItem;
static Blt_PoolFreeProc  FixedPoolFreeItem;
static Blt_PoolAllocProc StringPoolAllocItem;
static Blt_PoolFreeProc  StringPoolFreeItem;

/*
 *---------------------------------------------------------------------------
 *
 * VariablePoolAllocItem --
 *
 *      Returns a new item.  First check if there is any more space 
 *	left in the current chunk.  If there isn't then next check
 *	the free list for unused items.  Finally allocate a new 
 *	chunk and return its first item.
 *
 * Results:
 *      Returns a new (possible reused) item.
 *
 * Side Effects:
 *	A new memory chunk may be allocated.
 *
 *---------------------------------------------------------------------------
 */
static void *
VariablePoolAllocItem(
    Blt_Pool pool,		      
    size_t size)		/* Number of bytes to allocate. */
{
    Pool *poolPtr = (Pool *)pool;
    PoolChain *chainPtr;
    void *memory;

    size = ALIGN(size);
    if (size >= POOL_MAX_CHUNK_SIZE) {
	/* 
	 * Handle oversized requests by allocating a chunk to hold the
	 * single item and immediately placing it into the in-use list.
	 */
	chainPtr = Blt_AssertMalloc(sizeof(PoolChain) + size);
	if (poolPtr->headPtr == NULL) {
	    poolPtr->headPtr = chainPtr;
	} else {
	    chainPtr->nextPtr = poolPtr->headPtr->nextPtr;
	    poolPtr->headPtr->nextPtr = chainPtr;
	}
	memory = (void *)chainPtr;
    } else {
	if (poolPtr->bytesLeft >= size) {
	    poolPtr->bytesLeft -= size;
	    memory = (char *)(poolPtr->headPtr + 1) + poolPtr->bytesLeft;
	} else {
	    poolPtr->waste += poolPtr->bytesLeft;
	    /* Create a new block of items and prepend it to the in-use list */
	    poolPtr->bytesLeft = POOL_MAX_CHUNK_SIZE;
	    /* Allocate the requested chunk size, plus the header */
	    chainPtr = Blt_AssertMalloc(sizeof(PoolChain) + poolPtr->bytesLeft);
	    chainPtr->nextPtr = poolPtr->headPtr;
	    poolPtr->headPtr = chainPtr;
	    /* Peel off a new item. */
	    poolPtr->bytesLeft -= size;
	    memory = (char *)(chainPtr + 1) + poolPtr->bytesLeft;
	}
    }
    return memory;
}

/*
 *---------------------------------------------------------------------------
 *
 * VariablePoolFreeItem --
 *
 *      Placeholder for freeProc routine.  The pool memory is 
 *	not reclaimed or freed until the entire pool is released.
 *
 * Results:
 *      None.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static void
VariablePoolFreeItem(Blt_Pool pool, void *item)
{
    /* Does nothing */
}

/*
 *---------------------------------------------------------------------------
 *
 * StringPoolAllocItem --
 *
 *      Returns a new item.  First check if there is any more space 
 *	left in the current chunk.  If there isn't then next check
 *	the free list for unused items.  Finally allocate a new 
 *	chunk and return its first item.
 *
 * Results:
 *      Returns a new (possible reused) item.
 *
 * Side Effects:
 *	A new memory chunk may be allocated.
 *
 *---------------------------------------------------------------------------
 */
static void *
StringPoolAllocItem(Blt_Pool pool, size_t size)
{
    Pool *poolPtr = (Pool *)pool;
    PoolChain *chainPtr;
    void *memory;

    if (size >= POOL_MAX_CHUNK_SIZE) {
	/* 
	 * Handle oversized requests by allocating a chunk to hold the
	 * single item and immediately placing it into the in-use list.
	 */
	chainPtr = Blt_AssertMalloc(sizeof(PoolChain) + size);
	if (poolPtr->headPtr == NULL) {
	    poolPtr->headPtr = chainPtr;
	} else {
	    chainPtr->nextPtr = poolPtr->headPtr->nextPtr;
	    poolPtr->headPtr->nextPtr = chainPtr;
	}
	memory = (void *)chainPtr;
    } else {
	if (poolPtr->bytesLeft >= size) {
	    poolPtr->bytesLeft -= size;
	    memory = (char *)(poolPtr->headPtr + 1) + poolPtr->bytesLeft;
	} else {
	    poolPtr->waste += poolPtr->bytesLeft;
	    /* Create a new block of items and prepend it to the
	     * in-use list */
	    poolPtr->bytesLeft = POOL_MAX_CHUNK_SIZE;
	    /* Allocate the requested chunk size, plus the header */
	    chainPtr = Blt_AssertMalloc(sizeof(PoolChain) + poolPtr->bytesLeft);
	    chainPtr->nextPtr = poolPtr->headPtr;
	    poolPtr->headPtr = chainPtr;
	    /* Peel off a new item. */
	    poolPtr->bytesLeft -= size;
	    memory = (char *)(chainPtr + 1) + poolPtr->bytesLeft;
	}
    }
    return memory;
}

/*
 *---------------------------------------------------------------------------
 *
 * StringPoolFreeItem --
 *
 *      Placeholder for freeProc routine.  String pool memory is 
 *	not reclaimed or freed until the entire pool is released.
 *
 * Results:
 *      None.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static void
StringPoolFreeItem(Blt_Pool pool, void *item) 
{
    /* Does nothing */
}

/*
 *       The fixed size pool is a chain of malloc'ed blocks.
 *
 *             +---------+  +---------+  +---------+  
 *       NULL<-| nextPtr |<-| nextPtr |<-| nextPtr |<- headPtr
 *             |---------|  |---------|  |---------|  
 *             | chunk1  |  | chunk2  |  | chunk3  |  
 *             +---------+  |         |  |         |  
 *                          +---------+  |         |  
 *                                       |         |  
 *                                       |         |  
 *                                       +---------+  
 *
 *      Each chunk contains an integral number of fixed size items.
 *	The number of items doubles until a maximum size is reached
 *      (each subsequent new chunk will be the maximum).  Chunks
 *	are allocated only when needed (no more space is available
 *	in the last chunk).
 *
 *	The chain of blocks is only freed when the entire pool is
 *	destroyed.  
 *
 *      A freelist of unused items also maintained. Each freed item
 *	is prepended to a free list.  Before allocating new chunks
 *	the freelist is examined to see if an unused items exist.
 *
 *               chunk1       chunk2       chunk3
 *            +---------+  +---------+  +---------+  
 *      NULL<-| unused  |  |         |  |         |
 *            +----^----+  +---------+  +---------+  
 *            | unused  |<-| unused  |<-| unused  |       
 *            +---------+  +---------+  +----^----+  
 *            |         |  |         |  | unused  |
 *            +---------+  |         |  +----^----+
 *                         |         |  |    |    |
 *                         +---------+  +----|----+
 *                                      | usused  |<- freePtr
 *                                      +---------+  
 */

/*
 *---------------------------------------------------------------------------
 *
 * FixedPoolFreeItem --
 *
 *      Returns a new item.  First check if there is any more space 
 *	left in the current chunk.  If there isn't then next check
 *	the free list for unused items.  Finally allocate a new 
 *	chunk and return its first item.
 *
 * Results:
 *      Returns a new (possible reused) item.
 *
 * Side Effects:
 *	A new memory chunk may be allocated.
 *
 *---------------------------------------------------------------------------
 */
static void *
FixedPoolAllocItem(Blt_Pool pool, size_t size)
{
    Pool *poolPtr = (Pool *)pool;
    PoolChain *chainPtr;
    void *newPtr;

    size = ALIGN(size);
    if (poolPtr->itemSize == 0) {
	poolPtr->itemSize = size;
    }
    assert(size == poolPtr->itemSize);

    if (poolPtr->bytesLeft > 0) {
	poolPtr->bytesLeft -= poolPtr->itemSize;
	newPtr = (char *)(poolPtr->headPtr + 1) + poolPtr->bytesLeft;
    } else if (poolPtr->freePtr != NULL) { /* Reuse from the free list. */
	/* Reuse items on the free list */
	chainPtr = poolPtr->freePtr;
	poolPtr->freePtr = chainPtr->nextPtr;
	newPtr = (void *)chainPtr;
    } else {			/* Allocate another block. */
	
	/* Create a new block of items and prepend it to the in-use list */
	poolPtr->bytesLeft = poolPtr->itemSize * (1 << poolPtr->poolSize);
	if (poolPtr->bytesLeft < POOL_MAX_CHUNK_SIZE) {
	    poolPtr->poolSize++; /* Keep doubling the size of the new 
				  * chunk up to a maximum size. */
	}
	/* Allocate the requested chunk size, plus the header */
	chainPtr = Blt_AssertMalloc(sizeof(PoolChain) + poolPtr->bytesLeft);
	chainPtr->nextPtr = poolPtr->headPtr;
	poolPtr->headPtr = chainPtr;

	/* Peel off a new item. */
	poolPtr->bytesLeft -= poolPtr->itemSize;
	newPtr = (char *)(poolPtr->headPtr + 1) + poolPtr->bytesLeft;
    }
    return newPtr;
}

/*
 *---------------------------------------------------------------------------
 *
 * FixedPoolFreeItem --
 *
 *      Frees an item.  The actual memory is not freed.  The item
 *	instead is prepended to a freelist where it may be reclaimed
 *	and used again.
 *
 * Results:
 *      None.
 *
 * Side Effects:
 *	Item is placed on the pool's free list.
 *
 *---------------------------------------------------------------------------
 */
static void
FixedPoolFreeItem(Blt_Pool pool, void *item) 
{
    Pool *poolPtr = (Pool *)pool;
    PoolChain *chainPtr = (PoolChain *)item;
    
    /* Prepend the newly deallocated item to the free list. */
    chainPtr->nextPtr = poolPtr->freePtr;
    poolPtr->freePtr = chainPtr;
}

/*
 *---------------------------------------------------------------------------
 *
 * Blt_Pool_Create --
 *
 *      Creates a new memory pool for fixed-size/variable-size/string
 *      items.  
 *
 * Results:
 *      Returns a pointer to the newly allocated pool.
 *
 *---------------------------------------------------------------------------
 */

Blt_Pool
Blt_Pool_Create(int type)
{
    Pool *poolPtr;

    poolPtr = Blt_AssertMalloc(sizeof(Pool));
    switch (type) {
    case BLT_VARIABLE_SIZE_ITEMS:
	poolPtr->allocProc = VariablePoolAllocItem;
	poolPtr->freeProc = VariablePoolFreeItem;
	break;
    case BLT_FIXED_SIZE_ITEMS:
	poolPtr->allocProc = FixedPoolAllocItem;
	poolPtr->freeProc = FixedPoolFreeItem;
	break;
    case BLT_STRING_ITEMS:
	poolPtr->allocProc = StringPoolAllocItem;
	poolPtr->freeProc = StringPoolFreeItem;
	break;
    }
    poolPtr->headPtr = poolPtr->freePtr = NULL;
    poolPtr->waste = poolPtr->bytesLeft = 0;
    poolPtr->poolSize = poolPtr->itemSize = 0;
    return (Blt_Pool)poolPtr;
}

/*
 *---------------------------------------------------------------------------
 *
 * Blt_Pool_Destroy --
 *
 *      Destroys the given memory pool.  The chain of allocated blocks
 *	are freed.  The is the only time that memory is actually freed.
 *
 * Results:
 *      None.
 *
 * Side Effects:
 *	All memory used by the pool is freed.
 *
 *---------------------------------------------------------------------------
 */
void  
Blt_Pool_Destroy(Blt_Pool pool)
{
    Pool *poolPtr = (Pool *)pool;
    PoolChain *chainPtr, *nextPtr;
    
    for (chainPtr = poolPtr->headPtr; chainPtr != NULL; chainPtr = nextPtr) {
	nextPtr = chainPtr->nextPtr;
	Blt_Free(chainPtr);
    }
    Blt_Free(poolPtr);
}

