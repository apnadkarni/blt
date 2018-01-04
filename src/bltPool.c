/* -*- mode: c; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 * bltPool.c --
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

#define BUILD_BLT_TCL_PROCS 1
#include "bltInt.h"
#include "bltAlloc.h"
#include "bltPool.h"

/*
 * Blt_Pool --
 *
 *      Implements a pool memory allocator. 
 *
 *        + It's faster allocating memory since malloc/free are called
 *          only a fraction of the normal times.  Fixed size items can 
 *          be reused without deallocating/reallocating memory.
 *        + You don't have the extra 8-16 byte overhead per malloc. 
 *        - Memory is freed only when the entire pool is destroyed.
 *        - Memory is allocated in chunks. More memory is allocated 
 *          than used.  
 *        0 Depending upon allocation/deallocation patterns, locality
 *          may be improved or degraded.
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
 *      The number of items doubles until a maximum size is reached
 *      (each subsequent new chunk will be the maximum).  Chunks
 *      are allocated only when needed (no more space is available
 *      in the last chunk).
 *
 *      The chain of blocks is only freed when the entire pool is
 *      destroyed.  
 *
 *      A freelist of unused items also maintained. Each freed item
 *      is prepended to a free list.  Before allocating new chunks
 *      the freelist is examined to see if any unused items exist.
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

#define POOL_MAX_CHUNK_SIZE      ((1<<16) - sizeof(MemoryChain))

/* Align memory addresses to the size of a pointer. */
#ifndef ALIGN
#define ALIGN(a) \
        (((size_t)a + (sizeof(void *) - 1)) & (~(sizeof(void *) - 1)))
#endif /* ALIGN */

typedef struct _MemoryChain {
   struct _MemoryChain *nextPtr;
} MemoryChain;

typedef struct _FreeItem {
   struct _FreeItem *nextPtr;
} FreeItem;

typedef struct {
    Blt_PoolAllocProc *allocProc;
    Blt_PoolFreeProc *freeProc;

    MemoryChain *headPtr;       /* Chain of malloc'ed chunks. */
    FreeItem *freePtr;          /* List of deleted items. This is only used
                                 * for fixed size items. */
    size_t poolSize;            /* Log2 of # of items in the current block. */
    size_t itemSize;            /* Size of an item. */
    size_t bytesLeft;           /* # of bytes left in the current chunk. */
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
 *      left in the current chunk.  If there isn't then next check
 *      the free list for unused items.  Finally allocate a new 
 *      chunk and return its first item.
 *
 * Results:
 *      Returns a new (possible reused) item.
 *
 * Side Effects:
 *      A new memory chunk may be allocated.
 *
 *---------------------------------------------------------------------------
 */
static void *
VariablePoolAllocItem(
    Blt_Pool pool,                    
    size_t size)                /* Number of bytes to allocate. */
{
    Pool *poolPtr = (Pool *)pool;
    MemoryChain *chainPtr;
    void *memory;

    size = ALIGN(size);
    if (size >= POOL_MAX_CHUNK_SIZE) {
        /* 
         * Handle oversized requests by allocating a chunk to hold the
         * single item and immediately placing it into the in-use list.
         */
        chainPtr = Blt_AssertMalloc(sizeof(MemoryChain) + size);
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
            chainPtr = Blt_AssertMalloc(sizeof(MemoryChain)+poolPtr->bytesLeft);
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
 *      not reclaimed or freed until the entire pool is released.
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
 *      left in the current chunk.  If there isn't then next check
 *      the free list for unused items.  Finally allocate a new 
 *      chunk and return its first item.
 *
 * Results:
 *      Returns a new (possible reused) item.
 *
 * Side Effects:
 *      A new memory chunk may be allocated.
 *
 *---------------------------------------------------------------------------
 */
static void *
StringPoolAllocItem(Blt_Pool pool, size_t size)
{
    Pool *poolPtr = (Pool *)pool;
    void *memory;

    if (size >= POOL_MAX_CHUNK_SIZE) {
        MemoryChain *chainPtr;

        /* 
         * Handle oversized requests by allocating a chunk to hold the
         * single item and immediately placing it into the in-use list.
         */
        chainPtr = Blt_AssertMalloc(sizeof(MemoryChain) + size);
        if (poolPtr->headPtr == NULL) {
            poolPtr->headPtr = chainPtr;
        } else {
            /* Place the memory block after the current. */
            chainPtr->nextPtr = poolPtr->headPtr->nextPtr;
            poolPtr->headPtr->nextPtr = chainPtr;
        }
        memory = (char *)(chainPtr + 1);
    } else {
        if (poolPtr->bytesLeft >= size) {
            poolPtr->bytesLeft -= size;
            memory = (char *)(poolPtr->headPtr + 1) + poolPtr->bytesLeft;
        } else {
            MemoryChain *chainPtr;

            poolPtr->waste += poolPtr->bytesLeft;
            /* Create a new block of items and prepend it to the
             * in-use list */
            poolPtr->bytesLeft = POOL_MAX_CHUNK_SIZE;
            /* Allocate the requested chunk size, plus the header */
            chainPtr = Blt_AssertMalloc(sizeof(MemoryChain)+poolPtr->bytesLeft);
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
 *      Placeholder for freeProc routine.  String pool memory is not
 *      reclaimed or freed until the entire pool is released.
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
 *      The number of items doubles until a maximum size is reached
 *      (each subsequent new chunk will be the maximum).  Chunks
 *      are allocated only when needed (no more space is available
 *      in the last chunk).
 *
 *      The chain of blocks is only freed when the entire pool is
 *      destroyed.  
 *
 *      A freelist of unused items also maintained. Each freed item
 *      is prepended to a free list.  Before allocating new chunks
 *      the freelist is examined to see if an unused items exist.
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
 *      left in the current chunk.  If there isn't then next check
 *      the free list for unused items.  Finally allocate a new 
 *      chunk and return its first item.
 *
 * Results:
 *      Returns a new (possible reused) item.
 *
 * Side Effects:
 *      A new memory chunk may be allocated.
 *
 *---------------------------------------------------------------------------
 */
static void *
FixedPoolAllocItem(Blt_Pool pool, size_t size)
{
    Pool *poolPtr = (Pool *)pool;
    void *memory;

    size = ALIGN(size);
    if (poolPtr->itemSize == 0) {
        poolPtr->itemSize = size;
    }
    assert(size == poolPtr->itemSize);

    if (poolPtr->bytesLeft > 0) {
        poolPtr->bytesLeft -= poolPtr->itemSize;
        memory = (char *)(poolPtr->headPtr + 1) + poolPtr->bytesLeft;
    } else if (poolPtr->freePtr != NULL) { /* Reuse from the free list. */
        FreeItem *itemPtr;

        /* Reuse items on the free list */
        itemPtr = poolPtr->freePtr;
        poolPtr->freePtr = itemPtr->nextPtr;
        memory = itemPtr;
    } else {                    /* No space left in chunk and no free
                                 * items, allocate another block. */
        MemoryChain *chainPtr;

        /* Create a new block of items and prepend it to the in-use list */
        poolPtr->bytesLeft = poolPtr->itemSize * (1 << poolPtr->poolSize);
        if (poolPtr->bytesLeft < POOL_MAX_CHUNK_SIZE) {
            poolPtr->poolSize++; /* Keep doubling the size of the new 
                                  * chunk up to a maximum size. */
        }
        /* Allocate the requested chunk size, plus the header */
        chainPtr = Blt_AssertMalloc(sizeof(MemoryChain) + poolPtr->bytesLeft);
        chainPtr->nextPtr = poolPtr->headPtr;
        poolPtr->headPtr = chainPtr;

        /* Peel off a new item.  From the back of the block. */
        poolPtr->bytesLeft -= poolPtr->itemSize;
        memory = (char *)(poolPtr->headPtr + 1) + poolPtr->bytesLeft;
    }
    return memory;
}

/*
 *---------------------------------------------------------------------------
 *
 * FixedPoolFreeItem --
 *
 *      Frees an item.  The actual memory is not freed.  The item
 *      instead is prepended to a freelist where it may be reclaimed
 *      and used again.
 *
 * Results:
 *      None.
 *
 * Side Effects:
 *      Item is placed on the pool's free list.
 *
 *---------------------------------------------------------------------------
 */
static void
FixedPoolFreeItem(Blt_Pool pool, void *item) 
{
    Pool *poolPtr = (Pool *)pool;
    FreeItem *itemPtr = item;
    
    /* Prepend the newly deallocated item to the free list. */
    itemPtr->nextPtr = poolPtr->freePtr;
    poolPtr->freePtr = itemPtr;
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
    poolPtr->headPtr = NULL;
    poolPtr->freePtr = NULL;
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
 *      are freed.  The is the only time that memory is actually freed.
 *
 * Results:
 *      None.
 *
 * Side Effects:
 *      All memory used by the pool is freed.
 *
 *---------------------------------------------------------------------------
 */
void  
Blt_Pool_Destroy(Blt_Pool pool)
{
    Pool *poolPtr = (Pool *)pool;
    MemoryChain *chainPtr, *nextPtr;
    
    for (chainPtr = poolPtr->headPtr; chainPtr != NULL; chainPtr = nextPtr) {
        nextPtr = chainPtr->nextPtr;
        Blt_Free(chainPtr);
    }
    Blt_Free(poolPtr);
}

