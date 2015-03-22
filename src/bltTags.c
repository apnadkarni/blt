/* -*- mode: c; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 * bltTags.c --
 *
 * The module implements a generic tagging package.
 *
 *	Copyright 2014 George A Howlett.
 *
 *	Permission is hereby granted, free of charge, to any person obtaining
 *	a copy of this software and associated documentation files (the
 *	"Software"), to deal in the Software without restriction, including
 *	without limitation the rights to use, copy, modify, merge, publish,
 *	distribute, sublicense, and/or sell copies of the Software, and to
 *	permit persons to whom the Software is furnished to do so, subject to
 *	the following conditions:
 *
 *	The above copyright notice and this permission notice shall be
 *	included in all copies or substantial portions of the Software.
 *
 *	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 *	EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 *	MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 *	NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
 *	LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
 *	OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
 *	WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#define BUILD_BLT_TCL_PROCS 1
#include "bltInt.h"
#ifdef HAVE_STDLIB_H
#  include <stdlib.h>
#endif /* HAVE_STDLIB_H */

#include <bltAlloc.h>
#include "bltChain.h"
#include "bltHash.h"
#include "bltTags.h"

typedef struct _Blt_Tags Tags;

typedef struct {
    Blt_HashTable table;
    struct _Blt_Chain chain;
} TagTable;

/*
 *---------------------------------------------------------------------------
 *
 * NewTagTable --
 *
 *	Creates a new tag table representing a single tag.
 *
 * Results:
 *	Returns a pointer to the newly created tag table.
 *
 *---------------------------------------------------------------------------
 */
static TagTable *
NewTagTable()
{
    TagTable *tagTablePtr;

    tagTablePtr = Blt_AssertMalloc(sizeof(TagTable));
    Blt_Chain_Init(&tagTablePtr->chain);
    Blt_InitHashTable(&tagTablePtr->table, BLT_ONE_WORD_KEYS);
    return tagTablePtr;
}

/*
 *---------------------------------------------------------------------------
 *
 * FreeTagTable --
 *
 *	Frees the tag table representing a single tag.
 *
 * Results:
 *	None.
 *
 *---------------------------------------------------------------------------
 */
static void
FreeTagTable(TagTable *tagTablePtr)
{
    if (tagTablePtr != NULL) {
	Blt_Chain_Reset(&tagTablePtr->chain);
	Blt_DeleteHashTable(&tagTablePtr->table);
	Blt_Free(tagTablePtr);
    }
}

/*
 *---------------------------------------------------------------------------
 *
 * FindTagTable --
 *
 *	Searches for the tag table given by "tag".  
 *
 * Results:
 *	Returns the pointer to the tag table if found, otherwise NULL.
 *
 *---------------------------------------------------------------------------
 */
static TagTable *
FindTagTable(struct _Blt_Tags *tagsPtr, const char *tag)
{
    Blt_HashEntry *hPtr;

    hPtr = Blt_FindHashEntry(&tagsPtr->table, tag);
    if (hPtr == NULL) {
	return NULL;			/* No tag by name. */
    }
    return Blt_GetHashValue(hPtr);
}

/*
 *---------------------------------------------------------------------------
 *
 * GetTagTable --
 *
 *	Returns the tag table given by "tag".  May create the tag
 *	table is needed.
 *
 * Results:
 *	Returns the pointer to the tag table.
 *
 *---------------------------------------------------------------------------
 */
static TagTable *
GetTagTable(Tags *tagsPtr, const char *tag)
{
    Blt_HashEntry *hPtr;
    TagTable *tagTablePtr;
    int isNew;

    hPtr = Blt_CreateHashEntry(&tagsPtr->table, tag, &isNew);
    if (isNew) {
	tagTablePtr = NewTagTable();
	Blt_SetHashValue(hPtr, tagTablePtr);
    } else {
	tagTablePtr = Blt_GetHashValue(hPtr);
    }
    return tagTablePtr;
}

/*
 *---------------------------------------------------------------------------
 *
 * AddTagEntry --
 *
 *	Adds an entry for an item into the given tag table.  Tag entries
 *	consist of a hash table entry and a chain entry.
 *
 * Results:
 *	None.
 *
 *---------------------------------------------------------------------------
 */
static void
AddTagEntry(TagTable *tagTablePtr, ClientData item)
{
    Blt_ChainLink link;
    Blt_HashEntry *hPtr;
    int isNew;

    hPtr = Blt_CreateHashEntry(&tagTablePtr->table, (char *)item, &isNew);
    if (!isNew) {
	return;
    }
    link = Blt_Chain_Append(&tagTablePtr->chain, item);
    Blt_SetHashValue(hPtr, link);
}

/*
 *---------------------------------------------------------------------------
 *
 * DeleteTagEntry --
 *
 *	Deletes the entry for an item in the given tag table.  
 *
 * Results:
 *	None.
 *
 *---------------------------------------------------------------------------
 */
static void
DeleteTagEntry(TagTable *tablePtr, ClientData item)
{
    Blt_HashEntry *hPtr;
    Blt_ChainLink link;

    hPtr = Blt_FindHashEntry(&tablePtr->table, (char *)item);
    if (hPtr == NULL) {
	return;
    }
    link = Blt_GetHashValue(hPtr);
    Blt_Chain_DeleteLink(&tablePtr->chain, link);
    Blt_DeleteHashEntry(&tablePtr->table, hPtr);
}

/*
 *---------------------------------------------------------------------------
 *
 * Blt_Tags_Create --
 *
 *	Creates a new tags structure and initializes its pointers;
 *
 * Results:
 *	Returns a pointer to the newly created tags structure.
 *
 *---------------------------------------------------------------------------
 */
Blt_Tags
Blt_Tags_Create(void)
{
    Tags *tagsPtr;

    tagsPtr = Blt_Malloc(sizeof(Tags));
    if (tagsPtr != NULL) {
	Blt_Tags_Init(tagsPtr);
    }
    return tagsPtr;
}

/*
 *---------------------------------------------------------------------------
 *
 * Blt_Tags_Reset
 *
 *     Frees the tags and deallocates the memory used for the tags
 *     structure itself.  It's assumed that the chain was previously
 *     allocated by Blt_Tags_Create.
 *
 * Results:
 *	None.
 *
 *---------------------------------------------------------------------------
 */
void
Blt_Tags_Reset(Tags *tagsPtr)
{
    Blt_HashEntry *hPtr;
    Blt_HashSearch iter;

    for (hPtr = Blt_FirstHashEntry(&tagsPtr->table, &iter); hPtr != NULL;
	 hPtr = Blt_NextHashEntry(&iter)) {
	TagTable *tablePtr;
	
	tablePtr = Blt_GetHashValue(hPtr);
	FreeTagTable(tablePtr);
    }
    Blt_DeleteHashTable(&tagsPtr->table);
    Blt_InitHashTable(&tagsPtr->table, BLT_STRING_KEYS);
}

/*
 *---------------------------------------------------------------------------
 *
 * Blt_Tags_Destroy
 *
 *     Frees the tags and deallocates the memory used for the tags
 *     structure itself.  It's assumed that the chain was previously
 *     allocated by Blt_Tags_Create.
 *
 * Results:
 *	None.
 *
 *---------------------------------------------------------------------------
 */
void
Blt_Tags_Destroy(Tags *tagsPtr)
{
    if (tagsPtr != NULL) {
	Blt_Tags_Reset(tagsPtr);
	Blt_Free(tagsPtr);
    }
}

/*
 *---------------------------------------------------------------------------
 *
 * Blt_Tags_Init --
 *
 *	Initializes a tags set.
 *
 * Results:
 *	None.
 *
 *---------------------------------------------------------------------------
 */
void
Blt_Tags_Init(Tags *tagsPtr)
{
    Blt_InitHashTable(&tagsPtr->table, BLT_STRING_KEYS);
}


/*
 *---------------------------------------------------------------------------
 *
 * Blt_Tags_ItemHasTag --
 *
 *	Indicates if the item has the given tag.  If the tag doesn't 
 *	exist, 0 is returned.
 *
 * Results:
 *	Returns 1 is the item has the tag, 0 otherwise.
 *
 *---------------------------------------------------------------------------
 */
int
Blt_Tags_ItemHasTag(Tags *tagsPtr, ClientData item, const char *tag)
{
    Blt_HashEntry *hPtr;
    TagTable *tagTablePtr;

    tagTablePtr = FindTagTable(tagsPtr, tag);
    if (tagTablePtr == NULL) {
	return FALSE;
    }
    hPtr = Blt_FindHashEntry(&tagTablePtr->table, (char *)item);
    if (hPtr == NULL) {
	return FALSE;
    }
    return TRUE;
}

/*
 *---------------------------------------------------------------------------
 *
 * Blt_Tags_AddTag --
 *
 *	Add the tag to the table of tags.  There doesn't have to be any
 *	item associated with the tag.  It is not an error if the tag
 *	already exists.
 *
 * Results:
 *	None.
 *
 *---------------------------------------------------------------------------
 */
void
Blt_Tags_AddTag(Tags *tagsPtr, const char *tag)
{
    GetTagTable(tagsPtr, tag);
}

/*
 *---------------------------------------------------------------------------
 *
 * Blt_Tags_AddItemToTag --
 *
 *	Add the item to the given tag.  If the tag doesn't already exist,
 *	it is automatically created.
 *
 * Results:
 *	None.
 *
 *---------------------------------------------------------------------------
 */
void
Blt_Tags_AddItemToTag(Tags *tagsPtr, const char *tag, ClientData item)
{
    TagTable *tagTablePtr;

    tagTablePtr = GetTagTable(tagsPtr, tag);
    assert(item != NULL);
    AddTagEntry(tagTablePtr, item);
}

/*
 *---------------------------------------------------------------------------
 *
 * Blt_Tags_ForgetTag --
 *
 *	Remove the tag for the tag table.  It is not an error if the
 *	the tag doesn't exist.
 *
 * Results:
 *	None.
 *
 *---------------------------------------------------------------------------
 */
void
Blt_Tags_ForgetTag(Tags *tagsPtr, const char *tag)
{
    Blt_HashEntry *hPtr;
    TagTable *tagTablePtr;

    hPtr = Blt_FindHashEntry(&tagsPtr->table, tag);
    if (hPtr == NULL) {
	return;				/* No tag by name. */
    }
    tagTablePtr = Blt_GetHashValue(hPtr);
    FreeTagTable(tagTablePtr);
    Blt_DeleteHashEntry(&tagsPtr->table, hPtr);
}

/*
 *---------------------------------------------------------------------------
 *
 * Blt_Tags_RemoveItemFromTag --
 *
 *	Removes the item from for the tag's list of items.  It is not an
 *	error if the the tag doesn't have the item or if the tag doesn't
 *	exist.
 *
 * Results:
 *	None.
 *
 *---------------------------------------------------------------------------
 */
void
Blt_Tags_RemoveItemFromTag(Tags *tagsPtr, const char *tag, ClientData item)
{
    Blt_HashEntry *hPtr;
    
    hPtr = Blt_FindHashEntry(&tagsPtr->table, tag);
    if (hPtr != NULL) {
        TagTable *tagTablePtr;
    
        tagTablePtr = Blt_GetHashValue(hPtr);
	DeleteTagEntry(tagTablePtr, item);
    }
}

/*
 *---------------------------------------------------------------------------
 *
 * Blt_Tags_ClearTagsFromItem --
 *
 *	Removes all the item from all the tag tables.  It is not an
 *	error if the the tag doesn't belong to any tag.
 *
 * Results:
 *	None.
 *
 *---------------------------------------------------------------------------
 */
void
Blt_Tags_ClearTagsFromItem(Tags *tagsPtr, ClientData item)
{
    Blt_HashEntry *hPtr;
    Blt_HashSearch iter;

    for (hPtr = Blt_FirstHashEntry(&tagsPtr->table, &iter); hPtr != NULL;
	 hPtr = Blt_NextHashEntry(&iter)) {
	TagTable *tagTablePtr;
	
	tagTablePtr = Blt_GetHashValue(hPtr);
	DeleteTagEntry(tagTablePtr, item);
    }
}

/*
 *---------------------------------------------------------------------------
 *
 * Blt_Tags_AppendTagsToChain --
 *
 *	Appends to a list of all the tags an item is associated with.
 *
 * Results:
 *	None.
 *
 *---------------------------------------------------------------------------
 */
void
Blt_Tags_AppendTagsToChain(Tags *tagsPtr, ClientData item, Blt_Chain list)
{
    Blt_HashEntry *hPtr;
    Blt_HashSearch iter;

    for (hPtr = Blt_FirstHashEntry(&tagsPtr->table, &iter); hPtr != NULL;
	 hPtr = Blt_NextHashEntry(&iter)) {
	TagTable *tagTablePtr;
        const char *tag;
	
	tagTablePtr = Blt_GetHashValue(hPtr);
        tag = Blt_GetHashKey(&tagsPtr->table, hPtr);
	if (Blt_FindHashEntry(&tagTablePtr->table, (char *)item) != NULL) {
	    Blt_Chain_Append(list, (ClientData)tag);
	}
    }
}

/*
 *---------------------------------------------------------------------------
 *
 * Blt_Tags_AppendTagsToObj --
 *
 *	Appends to a Tcl_Obj list all the tags an item is associated
 *	with.
 *
 * Results:
 *	None.
 *
 *---------------------------------------------------------------------------
 */
void
Blt_Tags_AppendTagsToObj(Tags *tagsPtr, ClientData item, 
			  Tcl_Obj *listObjPtr)
{
    Blt_HashEntry *hPtr;
    Blt_HashSearch iter;

    for (hPtr = Blt_FirstHashEntry(&tagsPtr->table, &iter); hPtr != NULL;
	 hPtr = Blt_NextHashEntry(&iter)) {
	TagTable *tagTablePtr;
	
	tagTablePtr = Blt_GetHashValue(hPtr);
	if (Blt_FindHashEntry(&tagTablePtr->table, (char *)item) != NULL) {
	    Tcl_Obj *objPtr;
	    const char *name;
	    
	    name = Blt_GetHashKey(&tagsPtr->table, hPtr);
	    objPtr = Tcl_NewStringObj(name, -1);
	    Tcl_ListObjAppendElement(NULL, listObjPtr, objPtr);
	}
    }
}

Blt_HashTable *
Blt_Tags_GetTable(Tags *tagsPtr)
{
    return &tagsPtr->table;
}

/*
 *---------------------------------------------------------------------------
 *
 * Blt_Tags_AppendAllTagsToObj --
 *
 *	Appends to a Tcl_Obj list all the tags in the tag table.
 *
 * Results:
 *	None.
 *
 *---------------------------------------------------------------------------
 */
void
Blt_Tags_AppendAllTagsToObj(Tags *tagsPtr, Tcl_Obj *listObjPtr)
{
    Blt_HashEntry *hPtr;
    Blt_HashSearch iter;

    for (hPtr = Blt_FirstHashEntry(&tagsPtr->table, &iter); hPtr != NULL;
	 hPtr = Blt_NextHashEntry(&iter)) {
	const char *name;
	Tcl_Obj *objPtr;
	
	name = Blt_GetHashKey(&tagsPtr->table, hPtr);
	objPtr = Tcl_NewStringObj(name, -1);
	Tcl_ListObjAppendElement(NULL, listObjPtr, objPtr);
    }
}

/*
 *---------------------------------------------------------------------------
 *
 * Blt_Tags_GetItemList --
 *
 *	Returns the chain of all the items corresponding to a specific
 *	tag.  If the tag doesn't exist, NULL is return;
 *
 * Results:
 *	A chain contains all the items that have the tag.  If no
 *	tag exists, then NULL is returned.
 *
 *---------------------------------------------------------------------------
 */
Blt_Chain
Blt_Tags_GetItemList(Tags *tagsPtr, const char *tagName)
{
    Blt_HashEntry *hPtr;
    TagTable *tagTablePtr;

    hPtr = Blt_FindHashEntry(&tagsPtr->table, tagName);
    if (hPtr == NULL) {
	return NULL;				/* No tag by name. */
    }
    tagTablePtr = Blt_GetHashValue(hPtr);
    return &tagTablePtr->chain;
}

