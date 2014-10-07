/* -*- mode: c; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 * bltTags.h --
 *
 *	Copyright 2014 George A Howlett.
 *
 *	Permission is hereby granted, free of charge, to any person
 *	obtaining a copy of this software and associated documentation
 *	files (the "Software"), to deal in the Software without
 *	restriction, including without limitation the rights to use, copy,
 *	modify, merge, publish, distribute, sublicense, and/or sell copies
 *	of the Software, and to permit persons to whom the Software is
 *	furnished to do so, subject to the following conditions:
 *
 *	The above copyright notice and this permission notice shall be
 *	included in all copies or substantial portions of the Software.
 *
 *	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 *	EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 *	MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 *	NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
 *	BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
 *	ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 *	CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 *	SOFTWARE.
 */
#ifndef _BLT_TAGS_H
#define _BLT_TAGS_H

/*
 * A Blt_Tags structure holds the tags for a data object or widget.
 */
typedef struct _Blt_Tags {
    Blt_HashTable table;		/* Hash table of tag tables */
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

#endif /* _BLT_TAGS_H */
