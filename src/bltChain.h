/* -*- mode: c; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 * bltChain.h --
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
#ifndef _BLT_CHAIN_H
#define _BLT_CHAIN_H

typedef struct _Blt_Chain *Blt_Chain;
typedef struct _Blt_ChainLink *Blt_ChainLink;

/*
 * A Blt_ChainLink is the container structure for the Blt_Chain.
 */

struct _Blt_ChainLink {
    Blt_ChainLink prev;                 /* Link to the previous link */
    Blt_ChainLink next;                 /* Link to the next link */
    ClientData clientData;              /* Pointer to the data object */
};

typedef int (Blt_ChainCompareProc)(Blt_ChainLink *l1Ptr, Blt_ChainLink *l2Ptr);

/*
 * A Blt_Chain is a doubly chained list structure.
 */
struct _Blt_Chain {
    Blt_ChainLink head;                 /* Pointer to first element in
					 * chain. */
    Blt_ChainLink tail;                 /* Pointer to last element in
					 * chain. */
    long numLinks;                      /* Number of elements in chain. */
};

BLT_EXTERN void Blt_Chain_Init(Blt_Chain chain);
BLT_EXTERN Blt_Chain Blt_Chain_Create(void);
BLT_EXTERN void Blt_Chain_Destroy(Blt_Chain chain);
BLT_EXTERN Blt_ChainLink Blt_Chain_NewLink(void);
BLT_EXTERN Blt_ChainLink Blt_Chain_AllocLink(size_t size);
BLT_EXTERN Blt_ChainLink Blt_Chain_Append(Blt_Chain chain, 
	ClientData clientData);
BLT_EXTERN Blt_ChainLink Blt_Chain_Prepend(Blt_Chain chain, 
	ClientData clientData);
BLT_EXTERN void Blt_Chain_Reset(Blt_Chain chain);
BLT_EXTERN void Blt_Chain_InitLink(Blt_ChainLink link);
BLT_EXTERN void Blt_Chain_LinkAfter(Blt_Chain chain, Blt_ChainLink link, 
	Blt_ChainLink after);
BLT_EXTERN void Blt_Chain_LinkBefore(Blt_Chain chain, Blt_ChainLink link, 
	Blt_ChainLink before);
BLT_EXTERN void Blt_Chain_UnlinkLink(Blt_Chain chain, Blt_ChainLink link);
BLT_EXTERN void Blt_Chain_DeleteLink(Blt_Chain chain, Blt_ChainLink link);
BLT_EXTERN Blt_ChainLink Blt_Chain_GetNthLink(Blt_Chain chain, long position);
BLT_EXTERN void Blt_Chain_Sort(Blt_Chain chain, Blt_ChainCompareProc *proc);
BLT_EXTERN void Blt_Chain_Reverse(Blt_Chain chain);
BLT_EXTERN int Blt_Chain_IsBefore(Blt_ChainLink first, Blt_ChainLink last);

#define Blt_Chain_GetLength(c)	(((c) == NULL) ? 0 : (c)->numLinks)
#define Blt_Chain_FirstLink(c)	(((c) == NULL) ? NULL : (c)->head)
#define Blt_Chain_LastLink(c)	(((c) == NULL) ? NULL : (c)->tail)
#define Blt_Chain_PrevLink(l)	((l)->prev)
#define Blt_Chain_NextLink(l) 	((l)->next)
#define Blt_Chain_GetValue(l)  	((l)->clientData)
#define Blt_Chain_FirstValue(c)	\
	(((c)->head == NULL) ? NULL : (c)->head->clientData)
#define Blt_Chain_SetValue(l, value) ((l)->clientData = (ClientData)(value))
#define Blt_Chain_AppendLink(c, l) \
	(Blt_Chain_LinkAfter((c), (l), (Blt_ChainLink)NULL))
#define Blt_Chain_PrependLink(c, l) \
	(Blt_Chain_LinkBefore((c), (l), (Blt_ChainLink)NULL))

#endif /* _BLT_CHAIN_H */
