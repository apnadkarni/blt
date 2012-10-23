
/*
 * bltList.h --
 *
 *	Copyright 1993-2004 George A Howlett.
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
#ifndef _BLT_LIST_H
#define _BLT_LIST_H

/*
 * Acceptable key types for hash tables:
 */
#ifndef BLT_STRING_KEYS
#define BLT_STRING_KEYS		0
#endif
#ifndef BLT_ONE_WORD_KEYS
#define BLT_ONE_WORD_KEYS	((size_t)-1)
#endif

typedef struct _Blt_List *Blt_List;
typedef struct _Blt_ListNode *Blt_ListNode;

typedef union {			/* Key has one of these forms: */
    const void *oneWordValue;	/* One-word value for key. */
    unsigned int words[1];	/* Multiple integer words for key.  The actual
				 * size will be as large as necessary for this
				 * table's keys. */
    char string[4];		/* String for key.  The actual size will be as
				 * large as needed to hold the key. */
} Blt_ListKey;

/*
 * A Blt_ListNode is the container structure for the Blt_List.
 */
struct _Blt_ListNode {
    Blt_ListNode prev;		/* Link to the previous node */
    Blt_ListNode next;		/* Link to the next node */
    Blt_List list;		/* List to eventually insert node */
    ClientData clientData;	/* Pointer to the data object */
    Blt_ListKey key;		/* MUST BE LAST FIELD IN RECORD!! */
};

typedef int (Blt_ListCompareProc)(Blt_ListNode *node1Ptr, 
	Blt_ListNode *node2Ptr);

/*
 * A Blt_List is a doubly chained list structure.
 */
struct _Blt_List {
    Blt_ListNode head;			/* Pointer to first element in list */
    Blt_ListNode tail;			/* Pointer to last element in list */
    size_t numNodes;			/* # of nodes currently in the list. */
    size_t type;			/* Type of keys in list. */
};

BLT_EXTERN void Blt_List_Init(Blt_List list, size_t type);
BLT_EXTERN void Blt_List_Reset(Blt_List list);
BLT_EXTERN Blt_List Blt_List_Create(size_t type);
BLT_EXTERN void Blt_List_Destroy(Blt_List list);
BLT_EXTERN Blt_ListNode Blt_List_CreateNode(Blt_List list, const char *key);
BLT_EXTERN void Blt_List_DeleteNode(Blt_ListNode node);

BLT_EXTERN Blt_ListNode Blt_List_Append(Blt_List list, const char *key, 
	ClientData clientData);
BLT_EXTERN Blt_ListNode Blt_List_Prepend(Blt_List list, const char *key, 
	ClientData clientData);
BLT_EXTERN void Blt_List_LinkAfter(Blt_List list, Blt_ListNode node, 
	Blt_ListNode afterNode);
BLT_EXTERN void Blt_List_LinkBefore(Blt_List list, Blt_ListNode node, 
	Blt_ListNode beforeNode);
BLT_EXTERN void Blt_List_UnlinkNode(Blt_ListNode node);
BLT_EXTERN Blt_ListNode Blt_List_GetNode(Blt_List list, const char *key);
BLT_EXTERN void Blt_List_DeleteNodeByKey(Blt_List list, const char *key);
BLT_EXTERN Blt_ListNode Blt_List_GetNthNode(Blt_List list, long position, 
	int direction);
BLT_EXTERN void Blt_List_Sort(Blt_List list, Blt_ListCompareProc *proc);

#define Blt_List_GetLength(list) \
	(((list) == NULL) ? 0 : ((struct _Blt_List *)list)->numNodes)
#define Blt_List_FirstNode(list) \
	(((list) == NULL) ? NULL : ((struct _Blt_List *)list)->head)
#define Blt_List_LastNode(list)	\
	(((list) == NULL) ? NULL : ((struct _Blt_List *)list)->tail)
#define Blt_List_PrevNode(node)	((node)->prev)
#define Blt_List_NextNode(node) 	((node)->next)
#define Blt_List_GetKey(node)	\
	(((node)->list->type == BLT_STRING_KEYS) \
		 ? (node)->key.string : (node)->key.oneWordValue)
#define Blt_List_GetValue(node)  	((node)->clientData)
#define Blt_List_SetValue(node, value) \
	((node)->clientData = (ClientData)(value))
#define Blt_List_AppendNode(list, node) \
	(Blt_List_LinkBefore((list), (node), (Blt_ListNode)NULL))
#define Blt_List_PrependNode(list, node) \
	(Blt_List_LinkAfter((list), (node), (Blt_ListNode)NULL))

#endif /* _BLT_LIST_H */
