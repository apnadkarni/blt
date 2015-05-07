/* -*- mode: c; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 * bltTags.h --
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
#ifndef _BLT_TAGS_H
#define _BLT_TAGS_H

/*
 * A Blt_Tags structure holds the tags for a data object or widget.
 */
typedef struct _Blt_Tags {
    Blt_HashTable table;                /* Hash table of tag tables */
} *Blt_Tags;

BLT_EXTERN Blt_Tags Blt_Tags_Create(void);
BLT_EXTERN void Blt_Tags_Destroy(Blt_Tags tags);
BLT_EXTERN void Blt_Tags_Init(Blt_Tags tags);
BLT_EXTERN void Blt_Tags_Reset(Blt_Tags tags);

BLT_EXTERN int Blt_Tags_ItemHasTag(Blt_Tags tags, ClientData item, 
        const char *tag);
BLT_EXTERN void Blt_Tags_AddTag(Blt_Tags tags, const char *tag);
BLT_EXTERN void Blt_Tags_AddItemToTag(Blt_Tags tags, const char *tag, 
        ClientData item);
BLT_EXTERN void Blt_Tags_ForgetTag(Blt_Tags tags, const char *tag);
BLT_EXTERN void Blt_Tags_RemoveItemFromTag(Blt_Tags tags, const char *tag, 
        ClientData item);
BLT_EXTERN void Blt_Tags_ClearTagsFromItem(Blt_Tags tags, ClientData item);
BLT_EXTERN void Blt_Tags_AppendTagsToChain(Blt_Tags tags, ClientData item,
        Blt_Chain list);
BLT_EXTERN void Blt_Tags_AppendTagsToObj(Blt_Tags tags, ClientData item, 
        Tcl_Obj *objPtr);
BLT_EXTERN void Blt_Tags_AppendAllTagsToObj(Blt_Tags tags, Tcl_Obj *objPtr);
BLT_EXTERN Blt_Chain Blt_Tags_GetItemList(Blt_Tags tags, const char *tag);
BLT_EXTERN Blt_HashTable *Blt_Tags_GetTable(Blt_Tags tags);

#endif /* _BLT_TAGS_H */
