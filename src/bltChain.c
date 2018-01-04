/* -*- mode: c; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 * bltChain.c --
 *
 * The module implements a generic linked list package.
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
#ifdef HAVE_STDLIB_H
  #include <stdlib.h>
#endif /* HAVE_STDLIB_H */

#include <bltAlloc.h>
#include "bltChain.h"

#ifndef ALIGN
  #define ALIGN(a) \
        (((size_t)a + (sizeof(double) - 1)) & (~(sizeof(double) - 1)))
#endif /* ALIGN */

typedef struct _Blt_ChainLink ChainLink;
typedef struct _Blt_Chain Chain;

/*
 *---------------------------------------------------------------------------
 *
 * Blt_Chain_Create --
 *
 *      Creates a new linked list (chain) structure and initializes its
 *      pointers;
 *
 * Results:
 *      Returns a pointer to the newly created chain structure.
 *
 *---------------------------------------------------------------------------
 */
Blt_Chain
Blt_Chain_Create(void)
{
    Chain *chainPtr;

    chainPtr = Blt_Malloc(sizeof(Chain));
    if (chainPtr != NULL) {
        Blt_Chain_Init(chainPtr);
    }
    return chainPtr;
}

/*
 *---------------------------------------------------------------------------
 *
 * Blt_Chain_AllocLink --
 *
 *      Creates a new chain link.  Unlink Blt_Chain_NewLink, this routine
 *      also allocates extra memory in the node for data.
 *
 * Results:
 *      The return value is the pointer to the newly created entry.
 *
 *---------------------------------------------------------------------------
 */
Blt_ChainLink
Blt_Chain_AllocLink(size_t extraSize)
{
    ChainLink *linkPtr;
    size_t linkSize;

    linkSize = ALIGN(sizeof(ChainLink));
    linkPtr = Blt_AssertCalloc(1, linkSize + extraSize);
    if (extraSize > 0) {
        /* Point clientData at the memory beyond the normal structure. */
        linkPtr->clientData = (ClientData)((char *)linkPtr + linkSize);
    }
    return linkPtr;
}

/*
 *---------------------------------------------------------------------------
 *
 * Blt_Chain_InitLink --
 *
 *      Initializes the new link.  This routine is for applications that
 *      use their own memory allocation procedures to allocate links.
 *
 * Results:
 *      None.
 *
 *---------------------------------------------------------------------------
 */
void
Blt_Chain_InitLink(ChainLink *linkPtr)
{
    linkPtr->clientData = NULL;
    linkPtr->next = linkPtr->prev = NULL;
}

/*
 *---------------------------------------------------------------------------
 *
 * Blt_Chain_NewLink --
 *
 *      Creates a new link.
 *
 * Results:
 *      The return value is the pointer to the newly created link.
 *
 *---------------------------------------------------------------------------
 */
Blt_ChainLink
Blt_Chain_NewLink(void)
{
    ChainLink *linkPtr;

    linkPtr = Blt_AssertMalloc(sizeof(ChainLink));
    linkPtr->clientData = NULL;
    linkPtr->next = linkPtr->prev = NULL;
    return linkPtr;
}

/*
 *---------------------------------------------------------------------------
 *
 * Blt_Chain_Reset --
 *
 *      Removes all the links in the chain, freeing the memory used for
 *      each link.  Memory pointed to by the link (clientData) is not
 *      freed.  It's the caller's responsibility to deallocate it.
 *
 * Results:
 *      None.
 *
 *---------------------------------------------------------------------------
 */
void
Blt_Chain_Reset(Chain *chainPtr)        /* Chain to clear */
{
    if (chainPtr != NULL) {
        ChainLink *linkPtr = chainPtr->head;

        while (linkPtr != NULL) {
            ChainLink *oldPtr;

            oldPtr = linkPtr;
            linkPtr = linkPtr->next;
            Blt_Free(oldPtr);
        }
        Blt_Chain_Init(chainPtr);
    }
}

/*
 *---------------------------------------------------------------------------
 *
 * Blt_Chain_Destroy
 *
 *     Frees all the nodes in the chain and deallocates the memory used for
 *     the chain structure itself.  It's assumed that the chain was
 *     previously allocated by Blt_Chain_Create.
 *
 * Results:
 *      None.
 *
 *---------------------------------------------------------------------------
 */
void
Blt_Chain_Destroy(Chain *chainPtr)
{
    if (chainPtr != NULL) {
        Blt_Chain_Reset(chainPtr);
        Blt_Free(chainPtr);
    }
}

/*
 *---------------------------------------------------------------------------
 *
 * Blt_Chain_Init --
 *
 *      Initializes a linked list.
 *
 * Results:
 *      None.
 *
 *---------------------------------------------------------------------------
 */
void
Blt_Chain_Init(Chain *chainPtr)
{
    chainPtr->numLinks = 0;
    chainPtr->head = chainPtr->tail = NULL;
}

/*
 *---------------------------------------------------------------------------
 *
 * Blt_Chain_LinkAfter --
 *
 *      Inserts a link after another link.  If afterPtr is NULL, then the
 *      new link is prepended to the beginning of the chain.
 *
 * Results:
 *      None.
 *
 *---------------------------------------------------------------------------
 */
void
Blt_Chain_LinkAfter(Chain *chainPtr, ChainLink *linkPtr, ChainLink *afterPtr)
{
    if (chainPtr->head == NULL) {
        chainPtr->tail = chainPtr->head = linkPtr;
    } else {
        if (afterPtr == NULL) {
            /* Append to the end of the chain. */
            linkPtr->next = NULL;
            linkPtr->prev = chainPtr->tail;
            if (chainPtr->tail != NULL) {
                chainPtr->tail->next = linkPtr;
            }
            chainPtr->tail = linkPtr;
        } else {
            linkPtr->next = afterPtr->next;
            linkPtr->prev = afterPtr;
            if (afterPtr == chainPtr->tail) {
                chainPtr->tail = linkPtr;
            } else {
                afterPtr->next->prev = linkPtr;
            }
            afterPtr->next = linkPtr;
        }
    }
    chainPtr->numLinks++;
}

/*
 *---------------------------------------------------------------------------
 *
 * Blt_Chain_LinkBefore --
 *
 *      Inserts a new link preceding a given link in a chain.  If beforePtr
 *      is NULL, then the new link is placed at the beginning of the list.
 *
 * Results:
 *      None.
 *
 *---------------------------------------------------------------------------
 */
void
Blt_Chain_LinkBefore(Chain *chainPtr, ChainLink *linkPtr, ChainLink *beforePtr)
{
    if (chainPtr->head == NULL) {
        chainPtr->tail = chainPtr->head = linkPtr;
    } else {
        if (beforePtr == NULL) {
            /* Prepend to the front of the chain */
            linkPtr->next = chainPtr->head;
            linkPtr->prev = NULL;
            if (chainPtr->head != NULL) {
                chainPtr->head->prev = linkPtr;
            }
            chainPtr->head = linkPtr;
        } else {
            linkPtr->prev = beforePtr->prev;
            linkPtr->next = beforePtr;
            if (beforePtr == chainPtr->head) {
                chainPtr->head = linkPtr;
            } else {
                beforePtr->prev->next = linkPtr;
            }
            beforePtr->prev = linkPtr;
        }
    }
    chainPtr->numLinks++;
}

/*
 *---------------------------------------------------------------------------
 *
 * Blt_Chain_UnlinkLink --
 *
 *      Unlinks a link from the chain. The link is not deallocated, but
 *      only removed from the chain.
 *
 * Results:
 *      None.
 *
 *---------------------------------------------------------------------------
 */
void
Blt_Chain_UnlinkLink(Chain *chainPtr, ChainLink *linkPtr)
{
    int unlinked;                       /* Indicates if the link is
                                         * actually removed from the
                                         * chain. */
    unlinked = FALSE;
    if (chainPtr->head == linkPtr) {
        chainPtr->head = linkPtr->next;
        unlinked = TRUE;
    }
    if (chainPtr->tail == linkPtr) {
        chainPtr->tail = linkPtr->prev;
        unlinked = TRUE;
    }
    if (linkPtr->next != NULL) {
        linkPtr->next->prev = linkPtr->prev;
        unlinked = TRUE;
    }
    if (linkPtr->prev != NULL) {
        linkPtr->prev->next = linkPtr->next;
        unlinked = TRUE;
    }
    if (unlinked) {
        chainPtr->numLinks--;
    }
    linkPtr->prev = linkPtr->next = NULL;
}

/*
 *---------------------------------------------------------------------------
 *
 * Blt_Chain_DeleteLink --
 *
 *      Unlinks and frees the given link from the chain.  It's assumed that
 *      the link belong to the chain. No error checking is performed to
 *      verify this.
 *
 * Results:
 *      None.
 *
 *---------------------------------------------------------------------------
 */
void
Blt_Chain_DeleteLink(Blt_Chain chain, Blt_ChainLink link)
{
    Blt_Chain_UnlinkLink(chain, link);
    Blt_Free(link);
}

/*
 *---------------------------------------------------------------------------
 *
 * Blt_Chain_Append
 *
 *      Creates and new link with the given data and appends it to the end
 *      of the chain.
 *
 * Results:
 *      Returns a pointer to the link created.
 *
 *---------------------------------------------------------------------------
 */
Blt_ChainLink
Blt_Chain_Append(Blt_Chain chain, ClientData clientData)
{
    Blt_ChainLink link;

    link = Blt_Chain_NewLink();
    Blt_Chain_LinkAfter(chain, link, (Blt_ChainLink)NULL);
    Blt_Chain_SetValue(link, clientData);
    return link;
}

/*
 *---------------------------------------------------------------------------
 *
 * Blt_Chain_Prepend
 *
 *      Creates and new link with the given data and prepends it to
 *      beginning of the chain.
 *
 * Results:
 *      Returns a pointer to the link created.
 *
 *---------------------------------------------------------------------------
 */
Blt_ChainLink 
Blt_Chain_Prepend(Blt_Chain chain, ClientData clientData)
{
    Blt_ChainLink link;

    link = Blt_Chain_NewLink();
    Blt_Chain_LinkBefore(chain, link, (Blt_ChainLink)NULL);
    Blt_Chain_SetValue(link, clientData);
    return link;
}

/*
 *---------------------------------------------------------------------------
 *
 * Blt_Chain_GetNthLink --
 *
 *      Find the link at the given position in the chain.  The position is
 *      number from 0.  If position is negative is returns the nth link
 *      from the back of the chain.
 *
 * Results:
 *      Returns the pointer to the link, if that numbered link
 *      exists. Otherwise NULL.
 *
 *---------------------------------------------------------------------------
 */
Blt_ChainLink
Blt_Chain_GetNthLink(Chain *chainPtr, long position)
{
    if (chainPtr != NULL) {
        if (position < 0) {
            ChainLink *linkPtr;
            int i;

            position = -position;
            for (i = 0, linkPtr = chainPtr->tail; linkPtr != NULL; 
                 linkPtr = linkPtr->prev, i++) {
                if (i == position) {
                    return linkPtr;
                }
            }
        } else {
            ChainLink *linkPtr;
            int i;

            for (i = 0, linkPtr = chainPtr->head; linkPtr != NULL; 
                 linkPtr = linkPtr->next, i++) {
                if (i == position) {
                    return linkPtr;
                }
            }
        }
    }
    return NULL;
}

/*
 *---------------------------------------------------------------------------
 *
 * Blt_Chain_Sort --
 *
 *      Sorts the chain according to the given comparison routine.  
 *
 * Results:
 *      None.
 *
 * Side Effects:
 *      The chain is reordered.
 *
 *---------------------------------------------------------------------------
 */
void
Blt_Chain_Sort(Chain *chainPtr, Blt_ChainCompareProc *proc)
{
    ChainLink **linkArr;
    ChainLink *linkPtr;
    long i;

    if (chainPtr->numLinks < 2) {
        return;
    }
    linkArr = Blt_Malloc(sizeof(Blt_ChainLink) * (chainPtr->numLinks + 1));
    if (linkArr == NULL) {
        return;                         /* Out of memory. */
    }
    i = 0;
    for (linkPtr = chainPtr->head; linkPtr != NULL; 
         linkPtr = linkPtr->next) { 
        linkArr[i++] = linkPtr;
    }
    qsort((char *)linkArr, chainPtr->numLinks, sizeof(Blt_ChainLink),
        (QSortCompareProc *)proc);

    /* Rethread the chain. */
    linkPtr = linkArr[0];
    chainPtr->head = linkPtr;
    linkPtr->prev = NULL;
    for (i = 1; i < chainPtr->numLinks; i++) {
        linkPtr->next = linkArr[i];
        linkPtr->next->prev = linkPtr;
        linkPtr = linkPtr->next;
    }
    chainPtr->tail = linkPtr;
    linkPtr->next = NULL;
    Blt_Free(linkArr);
}

/*
 *---------------------------------------------------------------------------
 *
 * Blt_Chain_Reverse --
 *
 *      Reverse the order of the chain.  
 *
 * Results:
 *      None.
 *
 * Side Effects:
 *      The chain is reordered.
 *
 *---------------------------------------------------------------------------
 */
void
Blt_Chain_Reverse(Chain *chainPtr)
{
    ChainLink *lastPtr, *linkPtr, *nextPtr;

    lastPtr = NULL;
    for (linkPtr = chainPtr->head; linkPtr != NULL; linkPtr = nextPtr) { 
        nextPtr = linkPtr->next;
        linkPtr->next = lastPtr;
        linkPtr->prev = nextPtr;
        lastPtr = linkPtr;
    }
    /* Swap head and tail pointers. */
    linkPtr = chainPtr->head;
    chainPtr->head = chainPtr->tail;
    chainPtr->tail = linkPtr;
}


/*
 *---------------------------------------------------------------------------
 *
 * Blt_Chain_IsBefore --
 *
 *
 * Results:
 *      Return boolean value if the first link comes before the second.
 *
 *---------------------------------------------------------------------------
 */
int
Blt_Chain_IsBefore(ChainLink *firstPtr, ChainLink *lastPtr)
{
    ChainLink *linkPtr;

    for (linkPtr = firstPtr; linkPtr != NULL; linkPtr = linkPtr->next) {
        if (linkPtr == lastPtr) {
            return TRUE;
        }
    }
    return FALSE;
}

