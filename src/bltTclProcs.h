/* -*- mode: c; c-basic-offset: 4; indent-tabs-mode: nil -*- */
#include "bltAlloc.h"
#include "bltChain.h"
#include "bltHash.h"
#include "bltTags.h"
#include "bltList.h"
#include "bltPool.h"
#include "bltTree.h"
#include "bltDataTable.h"
#include "bltVector.h"

/* !BEGIN!: Do not edit below this line. */

/*
 * Exported function declarations:
 */

/* Slot 0 is reserved */
#ifndef Blt_AllocInit_DECLARED
#define Blt_AllocInit_DECLARED
/* 1 */
BLT_EXTERN void		Blt_AllocInit(Blt_MallocProc *mallocProc,
				Blt_ReallocProc *reallocProc,
				Blt_FreeProc *freeProc);
#endif
#ifndef Blt_Chain_Init_DECLARED
#define Blt_Chain_Init_DECLARED
/* 2 */
BLT_EXTERN void		Blt_Chain_Init(Blt_Chain chain);
#endif
#ifndef Blt_Chain_Create_DECLARED
#define Blt_Chain_Create_DECLARED
/* 3 */
BLT_EXTERN Blt_Chain	Blt_Chain_Create(void );
#endif
#ifndef Blt_Chain_Destroy_DECLARED
#define Blt_Chain_Destroy_DECLARED
/* 4 */
BLT_EXTERN void		Blt_Chain_Destroy(Blt_Chain chain);
#endif
#ifndef Blt_Chain_NewLink_DECLARED
#define Blt_Chain_NewLink_DECLARED
/* 5 */
BLT_EXTERN Blt_ChainLink Blt_Chain_NewLink(void );
#endif
#ifndef Blt_Chain_AllocLink_DECLARED
#define Blt_Chain_AllocLink_DECLARED
/* 6 */
BLT_EXTERN Blt_ChainLink Blt_Chain_AllocLink(size_t size);
#endif
#ifndef Blt_Chain_Append_DECLARED
#define Blt_Chain_Append_DECLARED
/* 7 */
BLT_EXTERN Blt_ChainLink Blt_Chain_Append(Blt_Chain chain,
				ClientData clientData);
#endif
#ifndef Blt_Chain_Prepend_DECLARED
#define Blt_Chain_Prepend_DECLARED
/* 8 */
BLT_EXTERN Blt_ChainLink Blt_Chain_Prepend(Blt_Chain chain,
				ClientData clientData);
#endif
#ifndef Blt_Chain_Reset_DECLARED
#define Blt_Chain_Reset_DECLARED
/* 9 */
BLT_EXTERN void		Blt_Chain_Reset(Blt_Chain chain);
#endif
#ifndef Blt_Chain_InitLink_DECLARED
#define Blt_Chain_InitLink_DECLARED
/* 10 */
BLT_EXTERN void		Blt_Chain_InitLink(Blt_ChainLink link);
#endif
#ifndef Blt_Chain_LinkAfter_DECLARED
#define Blt_Chain_LinkAfter_DECLARED
/* 11 */
BLT_EXTERN void		Blt_Chain_LinkAfter(Blt_Chain chain,
				Blt_ChainLink link, Blt_ChainLink after);
#endif
#ifndef Blt_Chain_LinkBefore_DECLARED
#define Blt_Chain_LinkBefore_DECLARED
/* 12 */
BLT_EXTERN void		Blt_Chain_LinkBefore(Blt_Chain chain,
				Blt_ChainLink link, Blt_ChainLink before);
#endif
#ifndef Blt_Chain_UnlinkLink_DECLARED
#define Blt_Chain_UnlinkLink_DECLARED
/* 13 */
BLT_EXTERN void		Blt_Chain_UnlinkLink(Blt_Chain chain,
				Blt_ChainLink link);
#endif
#ifndef Blt_Chain_DeleteLink_DECLARED
#define Blt_Chain_DeleteLink_DECLARED
/* 14 */
BLT_EXTERN void		Blt_Chain_DeleteLink(Blt_Chain chain,
				Blt_ChainLink link);
#endif
#ifndef Blt_Chain_GetNthLink_DECLARED
#define Blt_Chain_GetNthLink_DECLARED
/* 15 */
BLT_EXTERN Blt_ChainLink Blt_Chain_GetNthLink(Blt_Chain chain, long position);
#endif
#ifndef Blt_Chain_Sort_DECLARED
#define Blt_Chain_Sort_DECLARED
/* 16 */
BLT_EXTERN void		Blt_Chain_Sort(Blt_Chain chain,
				Blt_ChainCompareProc *proc);
#endif
#ifndef Blt_Chain_Reverse_DECLARED
#define Blt_Chain_Reverse_DECLARED
/* 17 */
BLT_EXTERN void		Blt_Chain_Reverse(Blt_Chain chain);
#endif
#ifndef Blt_Chain_IsBefore_DECLARED
#define Blt_Chain_IsBefore_DECLARED
/* 18 */
BLT_EXTERN int		Blt_Chain_IsBefore(Blt_ChainLink first,
				Blt_ChainLink last);
#endif
#ifndef Blt_InitHashTable_DECLARED
#define Blt_InitHashTable_DECLARED
/* 19 */
BLT_EXTERN void		Blt_InitHashTable(Blt_HashTable *tablePtr,
				size_t keyType);
#endif
#ifndef Blt_InitHashTableWithPool_DECLARED
#define Blt_InitHashTableWithPool_DECLARED
/* 20 */
BLT_EXTERN void		Blt_InitHashTableWithPool(Blt_HashTable *tablePtr,
				size_t keyType);
#endif
#ifndef Blt_DeleteHashTable_DECLARED
#define Blt_DeleteHashTable_DECLARED
/* 21 */
BLT_EXTERN void		Blt_DeleteHashTable(Blt_HashTable *tablePtr);
#endif
#ifndef Blt_DeleteHashEntry_DECLARED
#define Blt_DeleteHashEntry_DECLARED
/* 22 */
BLT_EXTERN void		Blt_DeleteHashEntry(Blt_HashTable *tablePtr,
				Blt_HashEntry *entryPtr);
#endif
#ifndef Blt_FirstHashEntry_DECLARED
#define Blt_FirstHashEntry_DECLARED
/* 23 */
BLT_EXTERN Blt_HashEntry * Blt_FirstHashEntry(Blt_HashTable *tablePtr,
				Blt_HashSearch *searchPtr);
#endif
#ifndef Blt_NextHashEntry_DECLARED
#define Blt_NextHashEntry_DECLARED
/* 24 */
BLT_EXTERN Blt_HashEntry * Blt_NextHashEntry(Blt_HashSearch *srchPtr);
#endif
#ifndef Blt_HashStats_DECLARED
#define Blt_HashStats_DECLARED
/* 25 */
BLT_EXTERN const char *	 Blt_HashStats(Blt_HashTable *tablePtr);
#endif
#ifndef Blt_Tags_Create_DECLARED
#define Blt_Tags_Create_DECLARED
/* 26 */
BLT_EXTERN Blt_Tags	Blt_Tags_Create(void );
#endif
#ifndef Blt_Tags_Destroy_DECLARED
#define Blt_Tags_Destroy_DECLARED
/* 27 */
BLT_EXTERN void		Blt_Tags_Destroy(Blt_Tags tags);
#endif
#ifndef Blt_Tags_Init_DECLARED
#define Blt_Tags_Init_DECLARED
/* 28 */
BLT_EXTERN void		Blt_Tags_Init(Blt_Tags tags);
#endif
#ifndef Blt_Tags_Reset_DECLARED
#define Blt_Tags_Reset_DECLARED
/* 29 */
BLT_EXTERN void		Blt_Tags_Reset(Blt_Tags tags);
#endif
#ifndef Blt_Tags_ItemHasTag_DECLARED
#define Blt_Tags_ItemHasTag_DECLARED
/* 30 */
BLT_EXTERN int		Blt_Tags_ItemHasTag(Blt_Tags tags, ClientData item,
				const char *tag);
#endif
#ifndef Blt_Tags_AddTag_DECLARED
#define Blt_Tags_AddTag_DECLARED
/* 31 */
BLT_EXTERN void		Blt_Tags_AddTag(Blt_Tags tags, const char *tag);
#endif
#ifndef Blt_Tags_AddItemToTag_DECLARED
#define Blt_Tags_AddItemToTag_DECLARED
/* 32 */
BLT_EXTERN void		Blt_Tags_AddItemToTag(Blt_Tags tags, const char *tag,
				ClientData item);
#endif
#ifndef Blt_Tags_ForgetTag_DECLARED
#define Blt_Tags_ForgetTag_DECLARED
/* 33 */
BLT_EXTERN void		Blt_Tags_ForgetTag(Blt_Tags tags, const char *tag);
#endif
#ifndef Blt_Tags_RemoveItemFromTag_DECLARED
#define Blt_Tags_RemoveItemFromTag_DECLARED
/* 34 */
BLT_EXTERN void		Blt_Tags_RemoveItemFromTag(Blt_Tags tags,
				const char *tag, ClientData item);
#endif
#ifndef Blt_Tags_ClearTagsFromItem_DECLARED
#define Blt_Tags_ClearTagsFromItem_DECLARED
/* 35 */
BLT_EXTERN void		Blt_Tags_ClearTagsFromItem(Blt_Tags tags,
				ClientData item);
#endif
#ifndef Blt_Tags_AppendTagsToChain_DECLARED
#define Blt_Tags_AppendTagsToChain_DECLARED
/* 36 */
BLT_EXTERN void		Blt_Tags_AppendTagsToChain(Blt_Tags tags,
				ClientData item, Blt_Chain list);
#endif
#ifndef Blt_Tags_AppendTagsToObj_DECLARED
#define Blt_Tags_AppendTagsToObj_DECLARED
/* 37 */
BLT_EXTERN void		Blt_Tags_AppendTagsToObj(Blt_Tags tags,
				ClientData item, Tcl_Obj *objPtr);
#endif
#ifndef Blt_Tags_AppendAllTagsToObj_DECLARED
#define Blt_Tags_AppendAllTagsToObj_DECLARED
/* 38 */
BLT_EXTERN void		Blt_Tags_AppendAllTagsToObj(Blt_Tags tags,
				Tcl_Obj *objPtr);
#endif
#ifndef Blt_Tags_GetItemList_DECLARED
#define Blt_Tags_GetItemList_DECLARED
/* 39 */
BLT_EXTERN Blt_Chain	Blt_Tags_GetItemList(Blt_Tags tags, const char *tag);
#endif
#ifndef Blt_Tags_GetTable_DECLARED
#define Blt_Tags_GetTable_DECLARED
/* 40 */
BLT_EXTERN Blt_HashTable * Blt_Tags_GetTable(Blt_Tags tags);
#endif
#ifndef Blt_List_Init_DECLARED
#define Blt_List_Init_DECLARED
/* 41 */
BLT_EXTERN void		Blt_List_Init(Blt_List list, size_t type);
#endif
#ifndef Blt_List_Reset_DECLARED
#define Blt_List_Reset_DECLARED
/* 42 */
BLT_EXTERN void		Blt_List_Reset(Blt_List list);
#endif
#ifndef Blt_List_Create_DECLARED
#define Blt_List_Create_DECLARED
/* 43 */
BLT_EXTERN Blt_List	Blt_List_Create(size_t type);
#endif
#ifndef Blt_List_Destroy_DECLARED
#define Blt_List_Destroy_DECLARED
/* 44 */
BLT_EXTERN void		Blt_List_Destroy(Blt_List list);
#endif
#ifndef Blt_List_CreateNode_DECLARED
#define Blt_List_CreateNode_DECLARED
/* 45 */
BLT_EXTERN Blt_ListNode	 Blt_List_CreateNode(Blt_List list, const char *key);
#endif
#ifndef Blt_List_DeleteNode_DECLARED
#define Blt_List_DeleteNode_DECLARED
/* 46 */
BLT_EXTERN void		Blt_List_DeleteNode(Blt_ListNode node);
#endif
#ifndef Blt_List_Append_DECLARED
#define Blt_List_Append_DECLARED
/* 47 */
BLT_EXTERN Blt_ListNode	 Blt_List_Append(Blt_List list, const char *key,
				ClientData clientData);
#endif
#ifndef Blt_List_Prepend_DECLARED
#define Blt_List_Prepend_DECLARED
/* 48 */
BLT_EXTERN Blt_ListNode	 Blt_List_Prepend(Blt_List list, const char *key,
				ClientData clientData);
#endif
#ifndef Blt_List_LinkAfter_DECLARED
#define Blt_List_LinkAfter_DECLARED
/* 49 */
BLT_EXTERN void		Blt_List_LinkAfter(Blt_List list, Blt_ListNode node,
				Blt_ListNode afterNode);
#endif
#ifndef Blt_List_LinkBefore_DECLARED
#define Blt_List_LinkBefore_DECLARED
/* 50 */
BLT_EXTERN void		Blt_List_LinkBefore(Blt_List list, Blt_ListNode node,
				Blt_ListNode beforeNode);
#endif
#ifndef Blt_List_UnlinkNode_DECLARED
#define Blt_List_UnlinkNode_DECLARED
/* 51 */
BLT_EXTERN void		Blt_List_UnlinkNode(Blt_ListNode node);
#endif
#ifndef Blt_List_GetNode_DECLARED
#define Blt_List_GetNode_DECLARED
/* 52 */
BLT_EXTERN Blt_ListNode	 Blt_List_GetNode(Blt_List list, const char *key);
#endif
#ifndef Blt_List_DeleteNodeByKey_DECLARED
#define Blt_List_DeleteNodeByKey_DECLARED
/* 53 */
BLT_EXTERN void		Blt_List_DeleteNodeByKey(Blt_List list,
				const char *key);
#endif
#ifndef Blt_List_GetNthNode_DECLARED
#define Blt_List_GetNthNode_DECLARED
/* 54 */
BLT_EXTERN Blt_ListNode	 Blt_List_GetNthNode(Blt_List list, long position,
				int direction);
#endif
#ifndef Blt_List_Sort_DECLARED
#define Blt_List_Sort_DECLARED
/* 55 */
BLT_EXTERN void		Blt_List_Sort(Blt_List list,
				Blt_ListCompareProc *proc);
#endif
#ifndef Blt_Pool_Create_DECLARED
#define Blt_Pool_Create_DECLARED
/* 56 */
BLT_EXTERN Blt_Pool	Blt_Pool_Create(int type);
#endif
#ifndef Blt_Pool_Destroy_DECLARED
#define Blt_Pool_Destroy_DECLARED
/* 57 */
BLT_EXTERN void		Blt_Pool_Destroy(Blt_Pool pool);
#endif
#ifndef Blt_Tree_GetKey_DECLARED
#define Blt_Tree_GetKey_DECLARED
/* 58 */
BLT_EXTERN Blt_TreeKey	Blt_Tree_GetKey(Blt_Tree tree, const char *string);
#endif
#ifndef Blt_Tree_GetKeyFromNode_DECLARED
#define Blt_Tree_GetKeyFromNode_DECLARED
/* 59 */
BLT_EXTERN Blt_TreeKey	Blt_Tree_GetKeyFromNode(Blt_TreeNode node,
				const char *string);
#endif
#ifndef Blt_Tree_CreateNode_DECLARED
#define Blt_Tree_CreateNode_DECLARED
/* 60 */
BLT_EXTERN Blt_TreeNode	 Blt_Tree_CreateNode(Blt_Tree tree,
				Blt_TreeNode parent, const char *name,
				long position);
#endif
#ifndef Blt_Tree_CreateNodeWithId_DECLARED
#define Blt_Tree_CreateNodeWithId_DECLARED
/* 61 */
BLT_EXTERN Blt_TreeNode	 Blt_Tree_CreateNodeWithId(Blt_Tree tree,
				Blt_TreeNode parent, const char *name,
				long inode, long position);
#endif
#ifndef Blt_Tree_DeleteNode_DECLARED
#define Blt_Tree_DeleteNode_DECLARED
/* 62 */
BLT_EXTERN int		Blt_Tree_DeleteNode(Blt_Tree tree, Blt_TreeNode node);
#endif
#ifndef Blt_Tree_MoveNode_DECLARED
#define Blt_Tree_MoveNode_DECLARED
/* 63 */
BLT_EXTERN int		Blt_Tree_MoveNode(Blt_Tree tree, Blt_TreeNode node,
				Blt_TreeNode parent, Blt_TreeNode before);
#endif
#ifndef Blt_Tree_GetNodeFromIndex_DECLARED
#define Blt_Tree_GetNodeFromIndex_DECLARED
/* 64 */
BLT_EXTERN Blt_TreeNode	 Blt_Tree_GetNodeFromIndex(Blt_Tree tree, long inode);
#endif
#ifndef Blt_Tree_FindChild_DECLARED
#define Blt_Tree_FindChild_DECLARED
/* 65 */
BLT_EXTERN Blt_TreeNode	 Blt_Tree_FindChild(Blt_TreeNode parent,
				const char *name);
#endif
#ifndef Blt_Tree_NextNode_DECLARED
#define Blt_Tree_NextNode_DECLARED
/* 66 */
BLT_EXTERN Blt_TreeNode	 Blt_Tree_NextNode(Blt_TreeNode root,
				Blt_TreeNode node);
#endif
#ifndef Blt_Tree_PrevNode_DECLARED
#define Blt_Tree_PrevNode_DECLARED
/* 67 */
BLT_EXTERN Blt_TreeNode	 Blt_Tree_PrevNode(Blt_TreeNode root,
				Blt_TreeNode node);
#endif
#ifndef Blt_Tree_FirstChild_DECLARED
#define Blt_Tree_FirstChild_DECLARED
/* 68 */
BLT_EXTERN Blt_TreeNode	 Blt_Tree_FirstChild(Blt_TreeNode parent);
#endif
#ifndef Blt_Tree_LastChild_DECLARED
#define Blt_Tree_LastChild_DECLARED
/* 69 */
BLT_EXTERN Blt_TreeNode	 Blt_Tree_LastChild(Blt_TreeNode parent);
#endif
#ifndef Blt_Tree_IsBefore_DECLARED
#define Blt_Tree_IsBefore_DECLARED
/* 70 */
BLT_EXTERN int		Blt_Tree_IsBefore(Blt_TreeNode node1,
				Blt_TreeNode node2);
#endif
#ifndef Blt_Tree_IsAncestor_DECLARED
#define Blt_Tree_IsAncestor_DECLARED
/* 71 */
BLT_EXTERN int		Blt_Tree_IsAncestor(Blt_TreeNode node1,
				Blt_TreeNode node2);
#endif
#ifndef Blt_Tree_PrivateValue_DECLARED
#define Blt_Tree_PrivateValue_DECLARED
/* 72 */
BLT_EXTERN int		Blt_Tree_PrivateValue(Tcl_Interp *interp,
				Blt_Tree tree, Blt_TreeNode node,
				Blt_TreeKey key);
#endif
#ifndef Blt_Tree_PublicValue_DECLARED
#define Blt_Tree_PublicValue_DECLARED
/* 73 */
BLT_EXTERN int		Blt_Tree_PublicValue(Tcl_Interp *interp,
				Blt_Tree tree, Blt_TreeNode node,
				Blt_TreeKey key);
#endif
#ifndef Blt_Tree_GetValue_DECLARED
#define Blt_Tree_GetValue_DECLARED
/* 74 */
BLT_EXTERN int		Blt_Tree_GetValue(Tcl_Interp *interp, Blt_Tree tree,
				Blt_TreeNode node, const char *string,
				Tcl_Obj **valuePtr);
#endif
#ifndef Blt_Tree_ValueExists_DECLARED
#define Blt_Tree_ValueExists_DECLARED
/* 75 */
BLT_EXTERN int		Blt_Tree_ValueExists(Blt_Tree tree,
				Blt_TreeNode node, const char *string);
#endif
#ifndef Blt_Tree_SetValue_DECLARED
#define Blt_Tree_SetValue_DECLARED
/* 76 */
BLT_EXTERN int		Blt_Tree_SetValue(Tcl_Interp *interp, Blt_Tree tree,
				Blt_TreeNode node, const char *string,
				Tcl_Obj *valuePtr);
#endif
#ifndef Blt_Tree_UnsetValue_DECLARED
#define Blt_Tree_UnsetValue_DECLARED
/* 77 */
BLT_EXTERN int		Blt_Tree_UnsetValue(Tcl_Interp *interp,
				Blt_Tree tree, Blt_TreeNode node,
				const char *string);
#endif
#ifndef Blt_Tree_AppendValue_DECLARED
#define Blt_Tree_AppendValue_DECLARED
/* 78 */
BLT_EXTERN int		Blt_Tree_AppendValue(Tcl_Interp *interp,
				Blt_Tree tree, Blt_TreeNode node,
				const char *string, const char *value);
#endif
#ifndef Blt_Tree_ListAppendValue_DECLARED
#define Blt_Tree_ListAppendValue_DECLARED
/* 79 */
BLT_EXTERN int		Blt_Tree_ListAppendValue(Tcl_Interp *interp,
				Blt_Tree tree, Blt_TreeNode node,
				const char *string, Tcl_Obj *valuePtr);
#endif
#ifndef Blt_Tree_GetArrayValue_DECLARED
#define Blt_Tree_GetArrayValue_DECLARED
/* 80 */
BLT_EXTERN int		Blt_Tree_GetArrayValue(Tcl_Interp *interp,
				Blt_Tree tree, Blt_TreeNode node,
				const char *arrayName, const char *elemName,
				Tcl_Obj **valueObjPtrPtr);
#endif
#ifndef Blt_Tree_SetArrayValue_DECLARED
#define Blt_Tree_SetArrayValue_DECLARED
/* 81 */
BLT_EXTERN int		Blt_Tree_SetArrayValue(Tcl_Interp *interp,
				Blt_Tree tree, Blt_TreeNode node,
				const char *arrayName, const char *elemName,
				Tcl_Obj *valueObjPtr);
#endif
#ifndef Blt_Tree_UnsetArrayValue_DECLARED
#define Blt_Tree_UnsetArrayValue_DECLARED
/* 82 */
BLT_EXTERN int		Blt_Tree_UnsetArrayValue(Tcl_Interp *interp,
				Blt_Tree tree, Blt_TreeNode node,
				const char *arrayName, const char *elemName);
#endif
#ifndef Blt_Tree_AppendArrayValue_DECLARED
#define Blt_Tree_AppendArrayValue_DECLARED
/* 83 */
BLT_EXTERN int		Blt_Tree_AppendArrayValue(Tcl_Interp *interp,
				Blt_Tree tree, Blt_TreeNode node,
				const char *arrayName, const char *elemName,
				const char *value);
#endif
#ifndef Blt_Tree_ListAppendArrayValue_DECLARED
#define Blt_Tree_ListAppendArrayValue_DECLARED
/* 84 */
BLT_EXTERN int		Blt_Tree_ListAppendArrayValue(Tcl_Interp *interp,
				Blt_Tree tree, Blt_TreeNode node,
				const char *arrayName, const char *elemName,
				Tcl_Obj *valueObjPtr);
#endif
#ifndef Blt_Tree_ArrayValueExists_DECLARED
#define Blt_Tree_ArrayValueExists_DECLARED
/* 85 */
BLT_EXTERN int		Blt_Tree_ArrayValueExists(Blt_Tree tree,
				Blt_TreeNode node, const char *arrayName,
				const char *elemName);
#endif
#ifndef Blt_Tree_ArrayNames_DECLARED
#define Blt_Tree_ArrayNames_DECLARED
/* 86 */
BLT_EXTERN int		Blt_Tree_ArrayNames(Tcl_Interp *interp,
				Blt_Tree tree, Blt_TreeNode node,
				const char *arrayName, Tcl_Obj *listObjPtr);
#endif
#ifndef Blt_Tree_GetValueByKey_DECLARED
#define Blt_Tree_GetValueByKey_DECLARED
/* 87 */
BLT_EXTERN int		Blt_Tree_GetValueByKey(Tcl_Interp *interp,
				Blt_Tree tree, Blt_TreeNode node,
				Blt_TreeKey key, Tcl_Obj **valuePtr);
#endif
#ifndef Blt_Tree_SetValueByKey_DECLARED
#define Blt_Tree_SetValueByKey_DECLARED
/* 88 */
BLT_EXTERN int		Blt_Tree_SetValueByKey(Tcl_Interp *interp,
				Blt_Tree tree, Blt_TreeNode node,
				Blt_TreeKey key, Tcl_Obj *valuePtr);
#endif
#ifndef Blt_Tree_UnsetValueByKey_DECLARED
#define Blt_Tree_UnsetValueByKey_DECLARED
/* 89 */
BLT_EXTERN int		Blt_Tree_UnsetValueByKey(Tcl_Interp *interp,
				Blt_Tree tree, Blt_TreeNode node,
				Blt_TreeKey key);
#endif
#ifndef Blt_Tree_AppendValueByKey_DECLARED
#define Blt_Tree_AppendValueByKey_DECLARED
/* 90 */
BLT_EXTERN int		Blt_Tree_AppendValueByKey(Tcl_Interp *interp,
				Blt_Tree tree, Blt_TreeNode node,
				Blt_TreeKey key, const char *value);
#endif
#ifndef Blt_Tree_ListAppendValueByKey_DECLARED
#define Blt_Tree_ListAppendValueByKey_DECLARED
/* 91 */
BLT_EXTERN int		Blt_Tree_ListAppendValueByKey(Tcl_Interp *interp,
				Blt_Tree tree, Blt_TreeNode node,
				Blt_TreeKey key, Tcl_Obj *valuePtr);
#endif
#ifndef Blt_Tree_ValueExistsByKey_DECLARED
#define Blt_Tree_ValueExistsByKey_DECLARED
/* 92 */
BLT_EXTERN int		Blt_Tree_ValueExistsByKey(Blt_Tree tree,
				Blt_TreeNode node, Blt_TreeKey key);
#endif
#ifndef Blt_Tree_FirstKey_DECLARED
#define Blt_Tree_FirstKey_DECLARED
/* 93 */
BLT_EXTERN Blt_TreeKey	Blt_Tree_FirstKey(Blt_Tree tree, Blt_TreeNode node,
				Blt_TreeKeyIterator *iterPtr);
#endif
#ifndef Blt_Tree_NextKey_DECLARED
#define Blt_Tree_NextKey_DECLARED
/* 94 */
BLT_EXTERN Blt_TreeKey	Blt_Tree_NextKey(Blt_Tree tree,
				Blt_TreeKeyIterator *iterPtr);
#endif
#ifndef Blt_Tree_Apply_DECLARED
#define Blt_Tree_Apply_DECLARED
/* 95 */
BLT_EXTERN int		Blt_Tree_Apply(Blt_TreeNode root,
				Blt_TreeApplyProc *proc,
				ClientData clientData);
#endif
#ifndef Blt_Tree_ApplyDFS_DECLARED
#define Blt_Tree_ApplyDFS_DECLARED
/* 96 */
BLT_EXTERN int		Blt_Tree_ApplyDFS(Blt_TreeNode root,
				Blt_TreeApplyProc *proc,
				ClientData clientData, int order);
#endif
#ifndef Blt_Tree_ApplyBFS_DECLARED
#define Blt_Tree_ApplyBFS_DECLARED
/* 97 */
BLT_EXTERN int		Blt_Tree_ApplyBFS(Blt_TreeNode root,
				Blt_TreeApplyProc *proc,
				ClientData clientData);
#endif
#ifndef Blt_Tree_SortNode_DECLARED
#define Blt_Tree_SortNode_DECLARED
/* 98 */
BLT_EXTERN int		Blt_Tree_SortNode(Blt_Tree tree, Blt_TreeNode node,
				Blt_TreeCompareNodesProc *proc);
#endif
#ifndef Blt_Tree_Exists_DECLARED
#define Blt_Tree_Exists_DECLARED
/* 99 */
BLT_EXTERN int		Blt_Tree_Exists(Tcl_Interp *interp, const char *name);
#endif
#ifndef Blt_Tree_Open_DECLARED
#define Blt_Tree_Open_DECLARED
/* 100 */
BLT_EXTERN Blt_Tree	Blt_Tree_Open(Tcl_Interp *interp, const char *name,
				int flags);
#endif
#ifndef Blt_Tree_Close_DECLARED
#define Blt_Tree_Close_DECLARED
/* 101 */
BLT_EXTERN void		Blt_Tree_Close(Blt_Tree tree);
#endif
#ifndef Blt_Tree_Attach_DECLARED
#define Blt_Tree_Attach_DECLARED
/* 102 */
BLT_EXTERN int		Blt_Tree_Attach(Tcl_Interp *interp, Blt_Tree tree,
				const char *name);
#endif
#ifndef Blt_Tree_GetFromObj_DECLARED
#define Blt_Tree_GetFromObj_DECLARED
/* 103 */
BLT_EXTERN Blt_Tree	Blt_Tree_GetFromObj(Tcl_Interp *interp,
				Tcl_Obj *objPtr);
#endif
#ifndef Blt_Tree_Size_DECLARED
#define Blt_Tree_Size_DECLARED
/* 104 */
BLT_EXTERN int		Blt_Tree_Size(Blt_TreeNode node);
#endif
#ifndef Blt_Tree_CreateTrace_DECLARED
#define Blt_Tree_CreateTrace_DECLARED
/* 105 */
BLT_EXTERN Blt_TreeTrace Blt_Tree_CreateTrace(Blt_Tree tree,
				Blt_TreeNode node, const char *keyPattern,
				const char *tagName, unsigned int mask,
				Blt_TreeTraceProc *proc,
				ClientData clientData);
#endif
#ifndef Blt_Tree_DeleteTrace_DECLARED
#define Blt_Tree_DeleteTrace_DECLARED
/* 106 */
BLT_EXTERN void		Blt_Tree_DeleteTrace(Blt_TreeTrace token);
#endif
#ifndef Blt_Tree_CreateEventHandler_DECLARED
#define Blt_Tree_CreateEventHandler_DECLARED
/* 107 */
BLT_EXTERN void		Blt_Tree_CreateEventHandler(Blt_Tree tree,
				unsigned int mask,
				Blt_TreeNotifyEventProc *proc,
				ClientData clientData);
#endif
#ifndef Blt_Tree_DeleteEventHandler_DECLARED
#define Blt_Tree_DeleteEventHandler_DECLARED
/* 108 */
BLT_EXTERN void		Blt_Tree_DeleteEventHandler(Blt_Tree tree,
				unsigned int mask,
				Blt_TreeNotifyEventProc *proc,
				ClientData clientData);
#endif
#ifndef Blt_Tree_RelabelNode_DECLARED
#define Blt_Tree_RelabelNode_DECLARED
/* 109 */
BLT_EXTERN void		Blt_Tree_RelabelNode(Blt_Tree tree,
				Blt_TreeNode node, const char *string);
#endif
#ifndef Blt_Tree_RelabelNodeWithoutNotify_DECLARED
#define Blt_Tree_RelabelNodeWithoutNotify_DECLARED
/* 110 */
BLT_EXTERN void		Blt_Tree_RelabelNodeWithoutNotify(Blt_TreeNode node,
				const char *string);
#endif
#ifndef Blt_Tree_NodeIdAscii_DECLARED
#define Blt_Tree_NodeIdAscii_DECLARED
/* 111 */
BLT_EXTERN const char *	 Blt_Tree_NodeIdAscii(Blt_TreeNode node);
#endif
#ifndef Blt_Tree_NodePath_DECLARED
#define Blt_Tree_NodePath_DECLARED
/* 112 */
BLT_EXTERN const char *	 Blt_Tree_NodePath(Blt_TreeNode node,
				Tcl_DString *resultPtr);
#endif
#ifndef Blt_Tree_NodeRelativePath_DECLARED
#define Blt_Tree_NodeRelativePath_DECLARED
/* 113 */
BLT_EXTERN const char *	 Blt_Tree_NodeRelativePath(Blt_TreeNode root,
				Blt_TreeNode node, const char *separator,
				unsigned int flags, Tcl_DString *resultPtr);
#endif
#ifndef Blt_Tree_NodePosition_DECLARED
#define Blt_Tree_NodePosition_DECLARED
/* 114 */
BLT_EXTERN long		Blt_Tree_NodePosition(Blt_TreeNode node);
#endif
#ifndef Blt_Tree_ClearTags_DECLARED
#define Blt_Tree_ClearTags_DECLARED
/* 115 */
BLT_EXTERN void		Blt_Tree_ClearTags(Blt_Tree tree, Blt_TreeNode node);
#endif
#ifndef Blt_Tree_HasTag_DECLARED
#define Blt_Tree_HasTag_DECLARED
/* 116 */
BLT_EXTERN int		Blt_Tree_HasTag(Blt_Tree tree, Blt_TreeNode node,
				const char *tagName);
#endif
#ifndef Blt_Tree_AddTag_DECLARED
#define Blt_Tree_AddTag_DECLARED
/* 117 */
BLT_EXTERN void		Blt_Tree_AddTag(Blt_Tree tree, Blt_TreeNode node,
				const char *tagName);
#endif
#ifndef Blt_Tree_RemoveTag_DECLARED
#define Blt_Tree_RemoveTag_DECLARED
/* 118 */
BLT_EXTERN void		Blt_Tree_RemoveTag(Blt_Tree tree, Blt_TreeNode node,
				const char *tagName);
#endif
#ifndef Blt_Tree_ForgetTag_DECLARED
#define Blt_Tree_ForgetTag_DECLARED
/* 119 */
BLT_EXTERN void		Blt_Tree_ForgetTag(Blt_Tree tree,
				const char *tagName);
#endif
#ifndef Blt_Tree_TagHashTable_DECLARED
#define Blt_Tree_TagHashTable_DECLARED
/* 120 */
BLT_EXTERN Blt_HashTable * Blt_Tree_TagHashTable(Blt_Tree tree,
				const char *tagName);
#endif
#ifndef Blt_Tree_TagTableIsShared_DECLARED
#define Blt_Tree_TagTableIsShared_DECLARED
/* 121 */
BLT_EXTERN int		Blt_Tree_TagTableIsShared(Blt_Tree tree);
#endif
#ifndef Blt_Tree_NewTagTable_DECLARED
#define Blt_Tree_NewTagTable_DECLARED
/* 122 */
BLT_EXTERN void		Blt_Tree_NewTagTable(Blt_Tree tree);
#endif
#ifndef Blt_Tree_FirstTag_DECLARED
#define Blt_Tree_FirstTag_DECLARED
/* 123 */
BLT_EXTERN Blt_HashEntry * Blt_Tree_FirstTag(Blt_Tree tree,
				Blt_HashSearch *searchPtr);
#endif
#ifndef Blt_Tree_Depth_DECLARED
#define Blt_Tree_Depth_DECLARED
/* 124 */
BLT_EXTERN long		Blt_Tree_Depth(Blt_Tree tree);
#endif
#ifndef Blt_Tree_RegisterFormat_DECLARED
#define Blt_Tree_RegisterFormat_DECLARED
/* 125 */
BLT_EXTERN int		Blt_Tree_RegisterFormat(Tcl_Interp *interp,
				const char *fmtName,
				Blt_TreeImportProc *importProc,
				Blt_TreeExportProc *exportProc);
#endif
#ifndef Blt_Tree_RememberTag_DECLARED
#define Blt_Tree_RememberTag_DECLARED
/* 126 */
BLT_EXTERN Blt_TreeTagEntry * Blt_Tree_RememberTag(Blt_Tree tree,
				const char *name);
#endif
#ifndef Blt_Tree_GetNodeFromObj_DECLARED
#define Blt_Tree_GetNodeFromObj_DECLARED
/* 127 */
BLT_EXTERN int		Blt_Tree_GetNodeFromObj(Tcl_Interp *interp,
				Blt_Tree tree, Tcl_Obj *objPtr,
				Blt_TreeNode *nodePtr);
#endif
#ifndef Blt_Tree_GetNodeIterator_DECLARED
#define Blt_Tree_GetNodeIterator_DECLARED
/* 128 */
BLT_EXTERN int		Blt_Tree_GetNodeIterator(Tcl_Interp *interp,
				Blt_Tree tree, Tcl_Obj *objPtr,
				Blt_TreeIterator *iterPtr);
#endif
#ifndef Blt_Tree_FirstTaggedNode_DECLARED
#define Blt_Tree_FirstTaggedNode_DECLARED
/* 129 */
BLT_EXTERN Blt_TreeNode	 Blt_Tree_FirstTaggedNode(Blt_TreeIterator *iterPtr);
#endif
#ifndef Blt_Tree_NextTaggedNode_DECLARED
#define Blt_Tree_NextTaggedNode_DECLARED
/* 130 */
BLT_EXTERN Blt_TreeNode	 Blt_Tree_NextTaggedNode(Blt_TreeIterator *iterPtr);
#endif
#ifndef blt_table_release_tags_DECLARED
#define blt_table_release_tags_DECLARED
/* 131 */
BLT_EXTERN void		blt_table_release_tags(BLT_TABLE table);
#endif
#ifndef blt_table_new_tags_DECLARED
#define blt_table_new_tags_DECLARED
/* 132 */
BLT_EXTERN void		blt_table_new_tags(BLT_TABLE table);
#endif
#ifndef blt_table_get_column_tag_table_DECLARED
#define blt_table_get_column_tag_table_DECLARED
/* 133 */
BLT_EXTERN Blt_HashTable * blt_table_get_column_tag_table(BLT_TABLE table);
#endif
#ifndef blt_table_get_row_tag_table_DECLARED
#define blt_table_get_row_tag_table_DECLARED
/* 134 */
BLT_EXTERN Blt_HashTable * blt_table_get_row_tag_table(BLT_TABLE table);
#endif
#ifndef blt_table_exists_DECLARED
#define blt_table_exists_DECLARED
/* 135 */
BLT_EXTERN int		blt_table_exists(Tcl_Interp *interp,
				const char *name);
#endif
#ifndef blt_table_create_DECLARED
#define blt_table_create_DECLARED
/* 136 */
BLT_EXTERN int		blt_table_create(Tcl_Interp *interp,
				const char *name, BLT_TABLE *tablePtr);
#endif
#ifndef blt_table_open_DECLARED
#define blt_table_open_DECLARED
/* 137 */
BLT_EXTERN int		blt_table_open(Tcl_Interp *interp, const char *name,
				BLT_TABLE *tablePtr);
#endif
#ifndef blt_table_close_DECLARED
#define blt_table_close_DECLARED
/* 138 */
BLT_EXTERN void		blt_table_close(BLT_TABLE table);
#endif
#ifndef blt_table_clear_DECLARED
#define blt_table_clear_DECLARED
/* 139 */
BLT_EXTERN void		blt_table_clear(BLT_TABLE table);
#endif
#ifndef blt_table_pack_DECLARED
#define blt_table_pack_DECLARED
/* 140 */
BLT_EXTERN void		blt_table_pack(BLT_TABLE table);
#endif
#ifndef blt_table_same_object_DECLARED
#define blt_table_same_object_DECLARED
/* 141 */
BLT_EXTERN int		blt_table_same_object(BLT_TABLE table1,
				BLT_TABLE table2);
#endif
#ifndef blt_table_row_get_label_table_DECLARED
#define blt_table_row_get_label_table_DECLARED
/* 142 */
BLT_EXTERN Blt_HashTable * blt_table_row_get_label_table(BLT_TABLE table,
				const char *label);
#endif
#ifndef blt_table_column_get_label_table_DECLARED
#define blt_table_column_get_label_table_DECLARED
/* 143 */
BLT_EXTERN Blt_HashTable * blt_table_column_get_label_table(BLT_TABLE table,
				const char *label);
#endif
#ifndef blt_table_get_row_DECLARED
#define blt_table_get_row_DECLARED
/* 144 */
BLT_EXTERN BLT_TABLE_ROW blt_table_get_row(Tcl_Interp *interp,
				BLT_TABLE table, Tcl_Obj *objPtr);
#endif
#ifndef blt_table_get_column_DECLARED
#define blt_table_get_column_DECLARED
/* 145 */
BLT_EXTERN BLT_TABLE_COLUMN blt_table_get_column(Tcl_Interp *interp,
				BLT_TABLE table, Tcl_Obj *objPtr);
#endif
#ifndef blt_table_get_row_by_label_DECLARED
#define blt_table_get_row_by_label_DECLARED
/* 146 */
BLT_EXTERN BLT_TABLE_ROW blt_table_get_row_by_label(BLT_TABLE table,
				const char *label);
#endif
#ifndef blt_table_get_column_by_label_DECLARED
#define blt_table_get_column_by_label_DECLARED
/* 147 */
BLT_EXTERN BLT_TABLE_COLUMN blt_table_get_column_by_label(BLT_TABLE table,
				const char *label);
#endif
#ifndef blt_table_get_row_by_index_DECLARED
#define blt_table_get_row_by_index_DECLARED
/* 148 */
BLT_EXTERN BLT_TABLE_ROW blt_table_get_row_by_index(BLT_TABLE table,
				long index);
#endif
#ifndef blt_table_get_column_by_index_DECLARED
#define blt_table_get_column_by_index_DECLARED
/* 149 */
BLT_EXTERN BLT_TABLE_COLUMN blt_table_get_column_by_index(BLT_TABLE table,
				long index);
#endif
#ifndef blt_table_set_row_label_DECLARED
#define blt_table_set_row_label_DECLARED
/* 150 */
BLT_EXTERN int		blt_table_set_row_label(Tcl_Interp *interp,
				BLT_TABLE table, BLT_TABLE_ROW row,
				const char *label);
#endif
#ifndef blt_table_set_column_label_DECLARED
#define blt_table_set_column_label_DECLARED
/* 151 */
BLT_EXTERN int		blt_table_set_column_label(Tcl_Interp *interp,
				BLT_TABLE table, BLT_TABLE_COLUMN column,
				const char *label);
#endif
#ifndef blt_table_name_to_column_type_DECLARED
#define blt_table_name_to_column_type_DECLARED
/* 152 */
BLT_EXTERN BLT_TABLE_COLUMN_TYPE blt_table_name_to_column_type(
				const char *typeName);
#endif
#ifndef blt_table_set_column_type_DECLARED
#define blt_table_set_column_type_DECLARED
/* 153 */
BLT_EXTERN int		blt_table_set_column_type(Tcl_Interp *interp,
				BLT_TABLE table, BLT_TABLE_COLUMN column,
				BLT_TABLE_COLUMN_TYPE type);
#endif
#ifndef blt_table_column_type_to_name_DECLARED
#define blt_table_column_type_to_name_DECLARED
/* 154 */
BLT_EXTERN const char *	 blt_table_column_type_to_name(
				BLT_TABLE_COLUMN_TYPE type);
#endif
#ifndef blt_table_set_column_tag_DECLARED
#define blt_table_set_column_tag_DECLARED
/* 155 */
BLT_EXTERN int		blt_table_set_column_tag(Tcl_Interp *interp,
				BLT_TABLE table, BLT_TABLE_COLUMN column,
				const char *tag);
#endif
#ifndef blt_table_set_row_tag_DECLARED
#define blt_table_set_row_tag_DECLARED
/* 156 */
BLT_EXTERN int		blt_table_set_row_tag(Tcl_Interp *interp,
				BLT_TABLE table, BLT_TABLE_ROW row,
				const char *tag);
#endif
#ifndef blt_table_create_row_DECLARED
#define blt_table_create_row_DECLARED
/* 157 */
BLT_EXTERN BLT_TABLE_ROW blt_table_create_row(Tcl_Interp *interp,
				BLT_TABLE table, const char *label);
#endif
#ifndef blt_table_create_column_DECLARED
#define blt_table_create_column_DECLARED
/* 158 */
BLT_EXTERN BLT_TABLE_COLUMN blt_table_create_column(Tcl_Interp *interp,
				BLT_TABLE table, const char *label);
#endif
#ifndef blt_table_extend_rows_DECLARED
#define blt_table_extend_rows_DECLARED
/* 159 */
BLT_EXTERN int		blt_table_extend_rows(Tcl_Interp *interp,
				BLT_TABLE table, size_t n,
				BLT_TABLE_ROW *rows);
#endif
#ifndef blt_table_extend_columns_DECLARED
#define blt_table_extend_columns_DECLARED
/* 160 */
BLT_EXTERN int		blt_table_extend_columns(Tcl_Interp *interp,
				BLT_TABLE table, size_t n,
				BLT_TABLE_COLUMN *columms);
#endif
#ifndef blt_table_delete_row_DECLARED
#define blt_table_delete_row_DECLARED
/* 161 */
BLT_EXTERN int		blt_table_delete_row(BLT_TABLE table,
				BLT_TABLE_ROW row);
#endif
#ifndef blt_table_delete_column_DECLARED
#define blt_table_delete_column_DECLARED
/* 162 */
BLT_EXTERN int		blt_table_delete_column(BLT_TABLE table,
				BLT_TABLE_COLUMN column);
#endif
#ifndef blt_table_move_rows_DECLARED
#define blt_table_move_rows_DECLARED
/* 163 */
BLT_EXTERN int		blt_table_move_rows(Tcl_Interp *interp,
				BLT_TABLE table, BLT_TABLE_ROW destRow,
				BLT_TABLE_ROW firstRow,
				BLT_TABLE_ROW lastRow, int after);
#endif
#ifndef blt_table_move_columns_DECLARED
#define blt_table_move_columns_DECLARED
/* 164 */
BLT_EXTERN int		blt_table_move_columns(Tcl_Interp *interp,
				BLT_TABLE table, BLT_TABLE_COLUMN destColumn,
				BLT_TABLE_COLUMN firstColumn,
				BLT_TABLE_COLUMN lastColumn, int after);
#endif
#ifndef blt_table_get_obj_DECLARED
#define blt_table_get_obj_DECLARED
/* 165 */
BLT_EXTERN Tcl_Obj *	blt_table_get_obj(BLT_TABLE table, BLT_TABLE_ROW row,
				BLT_TABLE_COLUMN column);
#endif
#ifndef blt_table_set_obj_DECLARED
#define blt_table_set_obj_DECLARED
/* 166 */
BLT_EXTERN int		blt_table_set_obj(Tcl_Interp *interp,
				BLT_TABLE table, BLT_TABLE_ROW row,
				BLT_TABLE_COLUMN column, Tcl_Obj *objPtr);
#endif
#ifndef blt_table_get_string_DECLARED
#define blt_table_get_string_DECLARED
/* 167 */
BLT_EXTERN const char *	 blt_table_get_string(BLT_TABLE table,
				BLT_TABLE_ROW row, BLT_TABLE_COLUMN column);
#endif
#ifndef blt_table_set_string_rep_DECLARED
#define blt_table_set_string_rep_DECLARED
/* 168 */
BLT_EXTERN int		blt_table_set_string_rep(Tcl_Interp *interp,
				BLT_TABLE table, BLT_TABLE_ROW row,
				BLT_TABLE_COLUMN column, const char *string,
				int length);
#endif
#ifndef blt_table_set_string_DECLARED
#define blt_table_set_string_DECLARED
/* 169 */
BLT_EXTERN int		blt_table_set_string(Tcl_Interp *interp,
				BLT_TABLE table, BLT_TABLE_ROW row,
				BLT_TABLE_COLUMN column, const char *string,
				int length);
#endif
#ifndef blt_table_append_string_DECLARED
#define blt_table_append_string_DECLARED
/* 170 */
BLT_EXTERN int		blt_table_append_string(Tcl_Interp *interp,
				BLT_TABLE table, BLT_TABLE_ROW row,
				BLT_TABLE_COLUMN column, const char *string,
				int length);
#endif
#ifndef blt_table_set_bytes_DECLARED
#define blt_table_set_bytes_DECLARED
/* 171 */
BLT_EXTERN int		blt_table_set_bytes(Tcl_Interp *interp,
				BLT_TABLE table, BLT_TABLE_ROW row,
				BLT_TABLE_COLUMN column,
				const unsigned char *string, int length);
#endif
#ifndef blt_table_get_double_DECLARED
#define blt_table_get_double_DECLARED
/* 172 */
BLT_EXTERN double	blt_table_get_double(Tcl_Interp *interp,
				BLT_TABLE table, BLT_TABLE_ROW row,
				BLT_TABLE_COLUMN column);
#endif
#ifndef blt_table_set_double_DECLARED
#define blt_table_set_double_DECLARED
/* 173 */
BLT_EXTERN int		blt_table_set_double(Tcl_Interp *interp,
				BLT_TABLE table, BLT_TABLE_ROW row,
				BLT_TABLE_COLUMN column, double value);
#endif
#ifndef blt_table_get_long_DECLARED
#define blt_table_get_long_DECLARED
/* 174 */
BLT_EXTERN long		blt_table_get_long(Tcl_Interp *interp,
				BLT_TABLE table, BLT_TABLE_ROW row,
				BLT_TABLE_COLUMN column, long defValue);
#endif
#ifndef blt_table_set_long_DECLARED
#define blt_table_set_long_DECLARED
/* 175 */
BLT_EXTERN int		blt_table_set_long(Tcl_Interp *interp,
				BLT_TABLE table, BLT_TABLE_ROW row,
				BLT_TABLE_COLUMN column, long value);
#endif
#ifndef blt_table_get_boolean_DECLARED
#define blt_table_get_boolean_DECLARED
/* 176 */
BLT_EXTERN int		blt_table_get_boolean(Tcl_Interp *interp,
				BLT_TABLE table, BLT_TABLE_ROW row,
				BLT_TABLE_COLUMN column, int defValue);
#endif
#ifndef blt_table_set_boolean_DECLARED
#define blt_table_set_boolean_DECLARED
/* 177 */
BLT_EXTERN int		blt_table_set_boolean(Tcl_Interp *interp,
				BLT_TABLE table, BLT_TABLE_ROW row,
				BLT_TABLE_COLUMN column, int value);
#endif
#ifndef blt_table_get_value_DECLARED
#define blt_table_get_value_DECLARED
/* 178 */
BLT_EXTERN BLT_TABLE_VALUE blt_table_get_value(BLT_TABLE table,
				BLT_TABLE_ROW row, BLT_TABLE_COLUMN column);
#endif
#ifndef blt_table_set_value_DECLARED
#define blt_table_set_value_DECLARED
/* 179 */
BLT_EXTERN int		blt_table_set_value(BLT_TABLE table,
				BLT_TABLE_ROW row, BLT_TABLE_COLUMN column,
				BLT_TABLE_VALUE value);
#endif
#ifndef blt_table_unset_value_DECLARED
#define blt_table_unset_value_DECLARED
/* 180 */
BLT_EXTERN int		blt_table_unset_value(BLT_TABLE table,
				BLT_TABLE_ROW row, BLT_TABLE_COLUMN column);
#endif
#ifndef blt_table_value_exists_DECLARED
#define blt_table_value_exists_DECLARED
/* 181 */
BLT_EXTERN int		blt_table_value_exists(BLT_TABLE table,
				BLT_TABLE_ROW row, BLT_TABLE_COLUMN column);
#endif
#ifndef blt_table_value_string_DECLARED
#define blt_table_value_string_DECLARED
/* 182 */
BLT_EXTERN const char *	 blt_table_value_string(BLT_TABLE_VALUE value);
#endif
#ifndef blt_table_value_bytes_DECLARED
#define blt_table_value_bytes_DECLARED
/* 183 */
BLT_EXTERN const unsigned char * blt_table_value_bytes(BLT_TABLE_VALUE value);
#endif
#ifndef blt_table_value_length_DECLARED
#define blt_table_value_length_DECLARED
/* 184 */
BLT_EXTERN size_t	blt_table_value_length(BLT_TABLE_VALUE value);
#endif
#ifndef blt_table_tags_are_shared_DECLARED
#define blt_table_tags_are_shared_DECLARED
/* 185 */
BLT_EXTERN int		blt_table_tags_are_shared(BLT_TABLE table);
#endif
#ifndef blt_table_clear_row_tags_DECLARED
#define blt_table_clear_row_tags_DECLARED
/* 186 */
BLT_EXTERN void		blt_table_clear_row_tags(BLT_TABLE table,
				BLT_TABLE_ROW row);
#endif
#ifndef blt_table_clear_column_tags_DECLARED
#define blt_table_clear_column_tags_DECLARED
/* 187 */
BLT_EXTERN void		blt_table_clear_column_tags(BLT_TABLE table,
				BLT_TABLE_COLUMN col);
#endif
#ifndef blt_table_get_row_tags_DECLARED
#define blt_table_get_row_tags_DECLARED
/* 188 */
BLT_EXTERN Blt_Chain	blt_table_get_row_tags(BLT_TABLE table,
				BLT_TABLE_ROW row);
#endif
#ifndef blt_table_get_column_tags_DECLARED
#define blt_table_get_column_tags_DECLARED
/* 189 */
BLT_EXTERN Blt_Chain	blt_table_get_column_tags(BLT_TABLE table,
				BLT_TABLE_COLUMN column);
#endif
#ifndef blt_table_get_tagged_rows_DECLARED
#define blt_table_get_tagged_rows_DECLARED
/* 190 */
BLT_EXTERN Blt_Chain	blt_table_get_tagged_rows(BLT_TABLE table,
				const char *tag);
#endif
#ifndef blt_table_get_tagged_columns_DECLARED
#define blt_table_get_tagged_columns_DECLARED
/* 191 */
BLT_EXTERN Blt_Chain	blt_table_get_tagged_columns(BLT_TABLE table,
				const char *tag);
#endif
#ifndef blt_table_row_has_tag_DECLARED
#define blt_table_row_has_tag_DECLARED
/* 192 */
BLT_EXTERN int		blt_table_row_has_tag(BLT_TABLE table,
				BLT_TABLE_ROW row, const char *tag);
#endif
#ifndef blt_table_column_has_tag_DECLARED
#define blt_table_column_has_tag_DECLARED
/* 193 */
BLT_EXTERN int		blt_table_column_has_tag(BLT_TABLE table,
				BLT_TABLE_COLUMN column, const char *tag);
#endif
#ifndef blt_table_forget_row_tag_DECLARED
#define blt_table_forget_row_tag_DECLARED
/* 194 */
BLT_EXTERN int		blt_table_forget_row_tag(Tcl_Interp *interp,
				BLT_TABLE table, const char *tag);
#endif
#ifndef blt_table_forget_column_tag_DECLARED
#define blt_table_forget_column_tag_DECLARED
/* 195 */
BLT_EXTERN int		blt_table_forget_column_tag(Tcl_Interp *interp,
				BLT_TABLE table, const char *tag);
#endif
#ifndef blt_table_unset_row_tag_DECLARED
#define blt_table_unset_row_tag_DECLARED
/* 196 */
BLT_EXTERN int		blt_table_unset_row_tag(Tcl_Interp *interp,
				BLT_TABLE table, BLT_TABLE_ROW row,
				const char *tag);
#endif
#ifndef blt_table_unset_column_tag_DECLARED
#define blt_table_unset_column_tag_DECLARED
/* 197 */
BLT_EXTERN int		blt_table_unset_column_tag(Tcl_Interp *interp,
				BLT_TABLE table, BLT_TABLE_COLUMN column,
				const char *tag);
#endif
#ifndef blt_table_first_column_DECLARED
#define blt_table_first_column_DECLARED
/* 198 */
BLT_EXTERN BLT_TABLE_COLUMN blt_table_first_column(BLT_TABLE table);
#endif
#ifndef blt_table_last_column_DECLARED
#define blt_table_last_column_DECLARED
/* 199 */
BLT_EXTERN BLT_TABLE_COLUMN blt_table_last_column(BLT_TABLE table);
#endif
#ifndef blt_table_next_column_DECLARED
#define blt_table_next_column_DECLARED
/* 200 */
BLT_EXTERN BLT_TABLE_COLUMN blt_table_next_column(BLT_TABLE_COLUMN column);
#endif
#ifndef blt_table_previous_column_DECLARED
#define blt_table_previous_column_DECLARED
/* 201 */
BLT_EXTERN BLT_TABLE_COLUMN blt_table_previous_column(
				BLT_TABLE_COLUMN column);
#endif
#ifndef blt_table_first_row_DECLARED
#define blt_table_first_row_DECLARED
/* 202 */
BLT_EXTERN BLT_TABLE_ROW blt_table_first_row(BLT_TABLE table);
#endif
#ifndef blt_table_last_row_DECLARED
#define blt_table_last_row_DECLARED
/* 203 */
BLT_EXTERN BLT_TABLE_ROW blt_table_last_row(BLT_TABLE table);
#endif
#ifndef blt_table_next_row_DECLARED
#define blt_table_next_row_DECLARED
/* 204 */
BLT_EXTERN BLT_TABLE_ROW blt_table_next_row(BLT_TABLE_ROW row);
#endif
#ifndef blt_table_previous_row_DECLARED
#define blt_table_previous_row_DECLARED
/* 205 */
BLT_EXTERN BLT_TABLE_ROW blt_table_previous_row(BLT_TABLE_ROW row);
#endif
#ifndef blt_table_row_spec_DECLARED
#define blt_table_row_spec_DECLARED
/* 206 */
BLT_EXTERN BLT_TABLE_ROWCOLUMN_SPEC blt_table_row_spec(BLT_TABLE table,
				Tcl_Obj *objPtr, const char **sp);
#endif
#ifndef blt_table_column_spec_DECLARED
#define blt_table_column_spec_DECLARED
/* 207 */
BLT_EXTERN BLT_TABLE_ROWCOLUMN_SPEC blt_table_column_spec(BLT_TABLE table,
				Tcl_Obj *objPtr, const char **sp);
#endif
#ifndef blt_table_iterate_rows_DECLARED
#define blt_table_iterate_rows_DECLARED
/* 208 */
BLT_EXTERN int		blt_table_iterate_rows(Tcl_Interp *interp,
				BLT_TABLE table, Tcl_Obj *objPtr,
				BLT_TABLE_ITERATOR *iter);
#endif
#ifndef blt_table_iterate_columns_DECLARED
#define blt_table_iterate_columns_DECLARED
/* 209 */
BLT_EXTERN int		blt_table_iterate_columns(Tcl_Interp *interp,
				BLT_TABLE table, Tcl_Obj *objPtr,
				BLT_TABLE_ITERATOR *iter);
#endif
#ifndef blt_table_iterate_rows_objv_DECLARED
#define blt_table_iterate_rows_objv_DECLARED
/* 210 */
BLT_EXTERN int		blt_table_iterate_rows_objv(Tcl_Interp *interp,
				BLT_TABLE table, int objc,
				Tcl_Obj *const *objv,
				BLT_TABLE_ITERATOR *iterPtr);
#endif
#ifndef blt_table_iterate_columns_objv_DECLARED
#define blt_table_iterate_columns_objv_DECLARED
/* 211 */
BLT_EXTERN int		blt_table_iterate_columns_objv(Tcl_Interp *interp,
				BLT_TABLE table, int objc,
				Tcl_Obj *const *objv,
				BLT_TABLE_ITERATOR *iterPtr);
#endif
#ifndef blt_table_free_iterator_objv_DECLARED
#define blt_table_free_iterator_objv_DECLARED
/* 212 */
BLT_EXTERN void		blt_table_free_iterator_objv(
				BLT_TABLE_ITERATOR *iterPtr);
#endif
#ifndef blt_table_iterate_all_rows_DECLARED
#define blt_table_iterate_all_rows_DECLARED
/* 213 */
BLT_EXTERN void		blt_table_iterate_all_rows(BLT_TABLE table,
				BLT_TABLE_ITERATOR *iterPtr);
#endif
#ifndef blt_table_iterate_all_columns_DECLARED
#define blt_table_iterate_all_columns_DECLARED
/* 214 */
BLT_EXTERN void		blt_table_iterate_all_columns(BLT_TABLE table,
				BLT_TABLE_ITERATOR *iterPtr);
#endif
#ifndef blt_table_first_tagged_row_DECLARED
#define blt_table_first_tagged_row_DECLARED
/* 215 */
BLT_EXTERN BLT_TABLE_ROW blt_table_first_tagged_row(BLT_TABLE_ITERATOR *iter);
#endif
#ifndef blt_table_first_tagged_column_DECLARED
#define blt_table_first_tagged_column_DECLARED
/* 216 */
BLT_EXTERN BLT_TABLE_COLUMN blt_table_first_tagged_column(
				BLT_TABLE_ITERATOR *iter);
#endif
#ifndef blt_table_next_tagged_row_DECLARED
#define blt_table_next_tagged_row_DECLARED
/* 217 */
BLT_EXTERN BLT_TABLE_ROW blt_table_next_tagged_row(BLT_TABLE_ITERATOR *iter);
#endif
#ifndef blt_table_next_tagged_column_DECLARED
#define blt_table_next_tagged_column_DECLARED
/* 218 */
BLT_EXTERN BLT_TABLE_COLUMN blt_table_next_tagged_column(
				BLT_TABLE_ITERATOR *iter);
#endif
#ifndef blt_table_list_rows_DECLARED
#define blt_table_list_rows_DECLARED
/* 219 */
BLT_EXTERN int		blt_table_list_rows(Tcl_Interp *interp,
				BLT_TABLE table, int objc,
				Tcl_Obj *const *objv, Blt_Chain chain);
#endif
#ifndef blt_table_list_columns_DECLARED
#define blt_table_list_columns_DECLARED
/* 220 */
BLT_EXTERN int		blt_table_list_columns(Tcl_Interp *interp,
				BLT_TABLE table, int objc,
				Tcl_Obj *const *objv, Blt_Chain chain);
#endif
#ifndef blt_table_clear_row_traces_DECLARED
#define blt_table_clear_row_traces_DECLARED
/* 221 */
BLT_EXTERN void		blt_table_clear_row_traces(BLT_TABLE table,
				BLT_TABLE_ROW row);
#endif
#ifndef blt_table_clear_column_traces_DECLARED
#define blt_table_clear_column_traces_DECLARED
/* 222 */
BLT_EXTERN void		blt_table_clear_column_traces(BLT_TABLE table,
				BLT_TABLE_COLUMN column);
#endif
#ifndef blt_table_create_trace_DECLARED
#define blt_table_create_trace_DECLARED
/* 223 */
BLT_EXTERN BLT_TABLE_TRACE blt_table_create_trace(BLT_TABLE table,
				BLT_TABLE_ROW row, BLT_TABLE_COLUMN column,
				const char *rowTag, const char *columnTag,
				unsigned int mask,
				BLT_TABLE_TRACE_PROC *proc,
				BLT_TABLE_TRACE_DELETE_PROC *deleteProc,
				ClientData clientData);
#endif
#ifndef blt_table_trace_column_DECLARED
#define blt_table_trace_column_DECLARED
/* 224 */
BLT_EXTERN void		blt_table_trace_column(BLT_TABLE table,
				BLT_TABLE_COLUMN column, unsigned int mask,
				BLT_TABLE_TRACE_PROC *proc,
				BLT_TABLE_TRACE_DELETE_PROC *deleteProc,
				ClientData clientData);
#endif
#ifndef blt_table_trace_row_DECLARED
#define blt_table_trace_row_DECLARED
/* 225 */
BLT_EXTERN void		blt_table_trace_row(BLT_TABLE table,
				BLT_TABLE_ROW row, unsigned int mask,
				BLT_TABLE_TRACE_PROC *proc,
				BLT_TABLE_TRACE_DELETE_PROC *deleteProc,
				ClientData clientData);
#endif
#ifndef blt_table_create_column_trace_DECLARED
#define blt_table_create_column_trace_DECLARED
/* 226 */
BLT_EXTERN BLT_TABLE_TRACE blt_table_create_column_trace(BLT_TABLE table,
				BLT_TABLE_COLUMN column, unsigned int mask,
				BLT_TABLE_TRACE_PROC *proc,
				BLT_TABLE_TRACE_DELETE_PROC *deleteProc,
				ClientData clientData);
#endif
#ifndef blt_table_create_column_tag_trace_DECLARED
#define blt_table_create_column_tag_trace_DECLARED
/* 227 */
BLT_EXTERN BLT_TABLE_TRACE blt_table_create_column_tag_trace(BLT_TABLE table,
				const char *tag, unsigned int mask,
				BLT_TABLE_TRACE_PROC *proc,
				BLT_TABLE_TRACE_DELETE_PROC *deleteProc,
				ClientData clientData);
#endif
#ifndef blt_table_create_row_trace_DECLARED
#define blt_table_create_row_trace_DECLARED
/* 228 */
BLT_EXTERN BLT_TABLE_TRACE blt_table_create_row_trace(BLT_TABLE table,
				BLT_TABLE_ROW row, unsigned int mask,
				BLT_TABLE_TRACE_PROC *proc,
				BLT_TABLE_TRACE_DELETE_PROC *deleteProc,
				ClientData clientData);
#endif
#ifndef blt_table_create_row_tag_trace_DECLARED
#define blt_table_create_row_tag_trace_DECLARED
/* 229 */
BLT_EXTERN BLT_TABLE_TRACE blt_table_create_row_tag_trace(BLT_TABLE table,
				const char *tag, unsigned int mask,
				BLT_TABLE_TRACE_PROC *proc,
				BLT_TABLE_TRACE_DELETE_PROC *deleteProc,
				ClientData clientData);
#endif
#ifndef blt_table_delete_trace_DECLARED
#define blt_table_delete_trace_DECLARED
/* 230 */
BLT_EXTERN void		blt_table_delete_trace(BLT_TABLE table,
				BLT_TABLE_TRACE trace);
#endif
#ifndef blt_table_create_notifier_DECLARED
#define blt_table_create_notifier_DECLARED
/* 231 */
BLT_EXTERN BLT_TABLE_NOTIFIER blt_table_create_notifier(Tcl_Interp *interp,
				BLT_TABLE table, unsigned int mask,
				BLT_TABLE_NOTIFY_EVENT_PROC *proc,
				BLT_TABLE_NOTIFIER_DELETE_PROC *deleteProc,
				ClientData clientData);
#endif
#ifndef blt_table_create_row_notifier_DECLARED
#define blt_table_create_row_notifier_DECLARED
/* 232 */
BLT_EXTERN BLT_TABLE_NOTIFIER blt_table_create_row_notifier(
				Tcl_Interp *interp, BLT_TABLE table,
				BLT_TABLE_ROW row, unsigned int mask,
				BLT_TABLE_NOTIFY_EVENT_PROC *proc,
				BLT_TABLE_NOTIFIER_DELETE_PROC *deleteProc,
				ClientData clientData);
#endif
#ifndef blt_table_create_row_tag_notifier_DECLARED
#define blt_table_create_row_tag_notifier_DECLARED
/* 233 */
BLT_EXTERN BLT_TABLE_NOTIFIER blt_table_create_row_tag_notifier(
				Tcl_Interp *interp, BLT_TABLE table,
				const char *tag, unsigned int mask,
				BLT_TABLE_NOTIFY_EVENT_PROC *proc,
				BLT_TABLE_NOTIFIER_DELETE_PROC *deleteProc,
				ClientData clientData);
#endif
#ifndef blt_table_create_column_notifier_DECLARED
#define blt_table_create_column_notifier_DECLARED
/* 234 */
BLT_EXTERN BLT_TABLE_NOTIFIER blt_table_create_column_notifier(
				Tcl_Interp *interp, BLT_TABLE table,
				BLT_TABLE_COLUMN column, unsigned int mask,
				BLT_TABLE_NOTIFY_EVENT_PROC *proc,
				BLT_TABLE_NOTIFIER_DELETE_PROC *deleteProc,
				ClientData clientData);
#endif
#ifndef blt_table_create_column_tag_notifier_DECLARED
#define blt_table_create_column_tag_notifier_DECLARED
/* 235 */
BLT_EXTERN BLT_TABLE_NOTIFIER blt_table_create_column_tag_notifier(
				Tcl_Interp *interp, BLT_TABLE table,
				const char *tag, unsigned int mask,
				BLT_TABLE_NOTIFY_EVENT_PROC *proc,
				BLT_TABLE_NOTIFIER_DELETE_PROC *deleteProc,
				ClientData clientData);
#endif
#ifndef blt_table_delete_notifier_DECLARED
#define blt_table_delete_notifier_DECLARED
/* 236 */
BLT_EXTERN void		blt_table_delete_notifier(BLT_TABLE table,
				BLT_TABLE_NOTIFIER notifier);
#endif
#ifndef blt_table_sort_init_DECLARED
#define blt_table_sort_init_DECLARED
/* 237 */
BLT_EXTERN void		blt_table_sort_init(BLT_TABLE table,
				BLT_TABLE_SORT_ORDER *order,
				size_t numCompares, unsigned int flags);
#endif
#ifndef blt_table_sort_rows_DECLARED
#define blt_table_sort_rows_DECLARED
/* 238 */
BLT_EXTERN BLT_TABLE_ROW * blt_table_sort_rows(BLT_TABLE table);
#endif
#ifndef blt_table_sort_rows_subset_DECLARED
#define blt_table_sort_rows_subset_DECLARED
/* 239 */
BLT_EXTERN void		blt_table_sort_rows_subset(BLT_TABLE table,
				long numRows, BLT_TABLE_ROW *rows);
#endif
#ifndef blt_table_sort_finish_DECLARED
#define blt_table_sort_finish_DECLARED
/* 240 */
BLT_EXTERN void		blt_table_sort_finish(void );
#endif
#ifndef blt_table_get_compare_proc_DECLARED
#define blt_table_get_compare_proc_DECLARED
/* 241 */
BLT_EXTERN BLT_TABLE_COMPARE_PROC * blt_table_get_compare_proc(
				BLT_TABLE table, BLT_TABLE_COLUMN column,
				unsigned int flags);
#endif
#ifndef blt_table_get_row_map_DECLARED
#define blt_table_get_row_map_DECLARED
/* 242 */
BLT_EXTERN BLT_TABLE_ROW * blt_table_get_row_map(BLT_TABLE table);
#endif
#ifndef blt_table_get_column_map_DECLARED
#define blt_table_get_column_map_DECLARED
/* 243 */
BLT_EXTERN BLT_TABLE_COLUMN * blt_table_get_column_map(BLT_TABLE table);
#endif
#ifndef blt_table_set_row_map_DECLARED
#define blt_table_set_row_map_DECLARED
/* 244 */
BLT_EXTERN void		blt_table_set_row_map(BLT_TABLE table,
				BLT_TABLE_ROW *map);
#endif
#ifndef blt_table_set_column_map_DECLARED
#define blt_table_set_column_map_DECLARED
/* 245 */
BLT_EXTERN void		blt_table_set_column_map(BLT_TABLE table,
				BLT_TABLE_COLUMN *map);
#endif
#ifndef blt_table_restore_DECLARED
#define blt_table_restore_DECLARED
/* 246 */
BLT_EXTERN int		blt_table_restore(Tcl_Interp *interp,
				BLT_TABLE table, char *string,
				unsigned int flags);
#endif
#ifndef blt_table_file_restore_DECLARED
#define blt_table_file_restore_DECLARED
/* 247 */
BLT_EXTERN int		blt_table_file_restore(Tcl_Interp *interp,
				BLT_TABLE table, const char *fileName,
				unsigned int flags);
#endif
#ifndef blt_table_register_format_DECLARED
#define blt_table_register_format_DECLARED
/* 248 */
BLT_EXTERN int		blt_table_register_format(Tcl_Interp *interp,
				const char *name,
				BLT_TABLE_IMPORT_PROC *importProc,
				BLT_TABLE_EXPORT_PROC *exportProc);
#endif
#ifndef blt_table_unset_keys_DECLARED
#define blt_table_unset_keys_DECLARED
/* 249 */
BLT_EXTERN void		blt_table_unset_keys(BLT_TABLE table);
#endif
#ifndef blt_table_get_keys_DECLARED
#define blt_table_get_keys_DECLARED
/* 250 */
BLT_EXTERN int		blt_table_get_keys(BLT_TABLE table,
				BLT_TABLE_COLUMN **keysPtr);
#endif
#ifndef blt_table_set_keys_DECLARED
#define blt_table_set_keys_DECLARED
/* 251 */
BLT_EXTERN int		blt_table_set_keys(BLT_TABLE table, int numKeys,
				BLT_TABLE_COLUMN *keys, int unique);
#endif
#ifndef blt_table_key_lookup_DECLARED
#define blt_table_key_lookup_DECLARED
/* 252 */
BLT_EXTERN int		blt_table_key_lookup(Tcl_Interp *interp,
				BLT_TABLE table, int objc,
				Tcl_Obj *const *objv, BLT_TABLE_ROW *rowPtr);
#endif
#ifndef blt_table_get_column_limits_DECLARED
#define blt_table_get_column_limits_DECLARED
/* 253 */
BLT_EXTERN int		blt_table_get_column_limits(Tcl_Interp *interp,
				BLT_TABLE table, BLT_TABLE_COLUMN col,
				Tcl_Obj **minObjPtrPtr,
				Tcl_Obj **maxObjPtrPtr);
#endif
#ifndef blt_table_row_DECLARED
#define blt_table_row_DECLARED
/* 254 */
BLT_EXTERN BLT_TABLE_ROW blt_table_row(BLT_TABLE table, long index);
#endif
#ifndef blt_table_column_DECLARED
#define blt_table_column_DECLARED
/* 255 */
BLT_EXTERN BLT_TABLE_COLUMN blt_table_column(BLT_TABLE table, long index);
#endif
#ifndef blt_table_row_index_DECLARED
#define blt_table_row_index_DECLARED
/* 256 */
BLT_EXTERN long		blt_table_row_index(BLT_TABLE table,
				BLT_TABLE_ROW row);
#endif
#ifndef blt_table_column_index_DECLARED
#define blt_table_column_index_DECLARED
/* 257 */
BLT_EXTERN long		blt_table_column_index(BLT_TABLE table,
				BLT_TABLE_COLUMN column);
#endif
#ifndef Blt_VecMin_DECLARED
#define Blt_VecMin_DECLARED
/* 258 */
BLT_EXTERN double	Blt_VecMin(Blt_Vector *vPtr);
#endif
#ifndef Blt_VecMax_DECLARED
#define Blt_VecMax_DECLARED
/* 259 */
BLT_EXTERN double	Blt_VecMax(Blt_Vector *vPtr);
#endif
#ifndef Blt_AllocVectorId_DECLARED
#define Blt_AllocVectorId_DECLARED
/* 260 */
BLT_EXTERN Blt_VectorId	 Blt_AllocVectorId(Tcl_Interp *interp,
				const char *vecName);
#endif
#ifndef Blt_SetVectorChangedProc_DECLARED
#define Blt_SetVectorChangedProc_DECLARED
/* 261 */
BLT_EXTERN void		Blt_SetVectorChangedProc(Blt_VectorId clientId,
				Blt_VectorChangedProc *proc,
				ClientData clientData);
#endif
#ifndef Blt_FreeVectorId_DECLARED
#define Blt_FreeVectorId_DECLARED
/* 262 */
BLT_EXTERN void		Blt_FreeVectorId(Blt_VectorId clientId);
#endif
#ifndef Blt_GetVectorById_DECLARED
#define Blt_GetVectorById_DECLARED
/* 263 */
BLT_EXTERN int		Blt_GetVectorById(Tcl_Interp *interp,
				Blt_VectorId clientId,
				Blt_Vector **vecPtrPtr);
#endif
#ifndef Blt_NameOfVectorId_DECLARED
#define Blt_NameOfVectorId_DECLARED
/* 264 */
BLT_EXTERN const char *	 Blt_NameOfVectorId(Blt_VectorId clientId);
#endif
#ifndef Blt_NameOfVector_DECLARED
#define Blt_NameOfVector_DECLARED
/* 265 */
BLT_EXTERN const char *	 Blt_NameOfVector(Blt_Vector *vecPtr);
#endif
#ifndef Blt_VectorNotifyPending_DECLARED
#define Blt_VectorNotifyPending_DECLARED
/* 266 */
BLT_EXTERN int		Blt_VectorNotifyPending(Blt_VectorId clientId);
#endif
#ifndef Blt_CreateVector_DECLARED
#define Blt_CreateVector_DECLARED
/* 267 */
BLT_EXTERN int		Blt_CreateVector(Tcl_Interp *interp,
				const char *vecName, size_t size,
				Blt_Vector **vecPtrPtr);
#endif
#ifndef Blt_CreateVector2_DECLARED
#define Blt_CreateVector2_DECLARED
/* 268 */
BLT_EXTERN int		Blt_CreateVector2(Tcl_Interp *interp,
				const char *vecName, const char *cmdName,
				const char *varName, size_t initialSize,
				Blt_Vector **vecPtrPtr);
#endif
#ifndef Blt_GetVector_DECLARED
#define Blt_GetVector_DECLARED
/* 269 */
BLT_EXTERN int		Blt_GetVector(Tcl_Interp *interp,
				const char *vecName, Blt_Vector **vecPtrPtr);
#endif
#ifndef Blt_GetVectorFromObj_DECLARED
#define Blt_GetVectorFromObj_DECLARED
/* 270 */
BLT_EXTERN int		Blt_GetVectorFromObj(Tcl_Interp *interp,
				Tcl_Obj *objPtr, Blt_Vector **vecPtrPtr);
#endif
#ifndef Blt_VectorExists_DECLARED
#define Blt_VectorExists_DECLARED
/* 271 */
BLT_EXTERN int		Blt_VectorExists(Tcl_Interp *interp,
				const char *vecName);
#endif
#ifndef Blt_ResetVector_DECLARED
#define Blt_ResetVector_DECLARED
/* 272 */
BLT_EXTERN int		Blt_ResetVector(Blt_Vector *vecPtr, double *dataArr,
        			size_t n, size_t arraySize,
                                Tcl_FreeProc *freeProc);
#endif
#ifndef Blt_ResizeVector_DECLARED
#define Blt_ResizeVector_DECLARED
/* 273 */
BLT_EXTERN int		Blt_ResizeVector(Blt_Vector *vecPtr, size_t n);
#endif
#ifndef Blt_DeleteVectorByName_DECLARED
#define Blt_DeleteVectorByName_DECLARED
/* 274 */
BLT_EXTERN int		Blt_DeleteVectorByName(Tcl_Interp *interp,
				const char *vecName);
#endif
#ifndef Blt_DeleteVector_DECLARED
#define Blt_DeleteVector_DECLARED
/* 275 */
BLT_EXTERN int		Blt_DeleteVector(Blt_Vector *vecPtr);
#endif
#ifndef Blt_ExprVector_DECLARED
#define Blt_ExprVector_DECLARED
/* 276 */
BLT_EXTERN int		Blt_ExprVector(Tcl_Interp *interp, char *expr,
				Blt_Vector *vecPtr);
#endif
#ifndef Blt_InstallIndexProc_DECLARED
#define Blt_InstallIndexProc_DECLARED
/* 277 */
BLT_EXTERN void		Blt_InstallIndexProc(Tcl_Interp *interp,
				const char *indexName,
				Blt_VectorIndexProc *procPtr);
#endif
#ifndef Blt_VectorExists2_DECLARED
#define Blt_VectorExists2_DECLARED
/* 278 */
BLT_EXTERN int		Blt_VectorExists2(Tcl_Interp *interp,
				const char *vecName);
#endif

typedef struct BltTclStubHooks {
    struct BltTclIntProcs *bltTclIntProcs;
} BltTclStubHooks;

typedef struct BltTclProcs {
    int magic;
    struct BltTclStubHooks *hooks;

    void *reserved0;
    void (*blt_AllocInit) (Blt_MallocProc *mallocProc, Blt_ReallocProc *reallocProc, Blt_FreeProc *freeProc); /* 1 */
    void (*blt_Chain_Init) (Blt_Chain chain); /* 2 */
    Blt_Chain (*blt_Chain_Create) (void); /* 3 */
    void (*blt_Chain_Destroy) (Blt_Chain chain); /* 4 */
    Blt_ChainLink (*blt_Chain_NewLink) (void); /* 5 */
    Blt_ChainLink (*blt_Chain_AllocLink) (size_t size); /* 6 */
    Blt_ChainLink (*blt_Chain_Append) (Blt_Chain chain, ClientData clientData); /* 7 */
    Blt_ChainLink (*blt_Chain_Prepend) (Blt_Chain chain, ClientData clientData); /* 8 */
    void (*blt_Chain_Reset) (Blt_Chain chain); /* 9 */
    void (*blt_Chain_InitLink) (Blt_ChainLink link); /* 10 */
    void (*blt_Chain_LinkAfter) (Blt_Chain chain, Blt_ChainLink link, Blt_ChainLink after); /* 11 */
    void (*blt_Chain_LinkBefore) (Blt_Chain chain, Blt_ChainLink link, Blt_ChainLink before); /* 12 */
    void (*blt_Chain_UnlinkLink) (Blt_Chain chain, Blt_ChainLink link); /* 13 */
    void (*blt_Chain_DeleteLink) (Blt_Chain chain, Blt_ChainLink link); /* 14 */
    Blt_ChainLink (*blt_Chain_GetNthLink) (Blt_Chain chain, long position); /* 15 */
    void (*blt_Chain_Sort) (Blt_Chain chain, Blt_ChainCompareProc *proc); /* 16 */
    void (*blt_Chain_Reverse) (Blt_Chain chain); /* 17 */
    int (*blt_Chain_IsBefore) (Blt_ChainLink first, Blt_ChainLink last); /* 18 */
    void (*blt_InitHashTable) (Blt_HashTable *tablePtr, size_t keyType); /* 19 */
    void (*blt_InitHashTableWithPool) (Blt_HashTable *tablePtr, size_t keyType); /* 20 */
    void (*blt_DeleteHashTable) (Blt_HashTable *tablePtr); /* 21 */
    void (*blt_DeleteHashEntry) (Blt_HashTable *tablePtr, Blt_HashEntry *entryPtr); /* 22 */
    Blt_HashEntry * (*blt_FirstHashEntry) (Blt_HashTable *tablePtr, Blt_HashSearch *searchPtr); /* 23 */
    Blt_HashEntry * (*blt_NextHashEntry) (Blt_HashSearch *srchPtr); /* 24 */
    const char * (*blt_HashStats) (Blt_HashTable *tablePtr); /* 25 */
    Blt_Tags (*blt_Tags_Create) (void); /* 26 */
    void (*blt_Tags_Destroy) (Blt_Tags tags); /* 27 */
    void (*blt_Tags_Init) (Blt_Tags tags); /* 28 */
    void (*blt_Tags_Reset) (Blt_Tags tags); /* 29 */
    int (*blt_Tags_ItemHasTag) (Blt_Tags tags, ClientData item, const char *tag); /* 30 */
    void (*blt_Tags_AddTag) (Blt_Tags tags, const char *tag); /* 31 */
    void (*blt_Tags_AddItemToTag) (Blt_Tags tags, const char *tag, ClientData item); /* 32 */
    void (*blt_Tags_ForgetTag) (Blt_Tags tags, const char *tag); /* 33 */
    void (*blt_Tags_RemoveItemFromTag) (Blt_Tags tags, const char *tag, ClientData item); /* 34 */
    void (*blt_Tags_ClearTagsFromItem) (Blt_Tags tags, ClientData item); /* 35 */
    void (*blt_Tags_AppendTagsToChain) (Blt_Tags tags, ClientData item, Blt_Chain list); /* 36 */
    void (*blt_Tags_AppendTagsToObj) (Blt_Tags tags, ClientData item, Tcl_Obj *objPtr); /* 37 */
    void (*blt_Tags_AppendAllTagsToObj) (Blt_Tags tags, Tcl_Obj *objPtr); /* 38 */
    Blt_Chain (*blt_Tags_GetItemList) (Blt_Tags tags, const char *tag); /* 39 */
    Blt_HashTable * (*blt_Tags_GetTable) (Blt_Tags tags); /* 40 */
    void (*blt_List_Init) (Blt_List list, size_t type); /* 41 */
    void (*blt_List_Reset) (Blt_List list); /* 42 */
    Blt_List (*blt_List_Create) (size_t type); /* 43 */
    void (*blt_List_Destroy) (Blt_List list); /* 44 */
    Blt_ListNode (*blt_List_CreateNode) (Blt_List list, const char *key); /* 45 */
    void (*blt_List_DeleteNode) (Blt_ListNode node); /* 46 */
    Blt_ListNode (*blt_List_Append) (Blt_List list, const char *key, ClientData clientData); /* 47 */
    Blt_ListNode (*blt_List_Prepend) (Blt_List list, const char *key, ClientData clientData); /* 48 */
    void (*blt_List_LinkAfter) (Blt_List list, Blt_ListNode node, Blt_ListNode afterNode); /* 49 */
    void (*blt_List_LinkBefore) (Blt_List list, Blt_ListNode node, Blt_ListNode beforeNode); /* 50 */
    void (*blt_List_UnlinkNode) (Blt_ListNode node); /* 51 */
    Blt_ListNode (*blt_List_GetNode) (Blt_List list, const char *key); /* 52 */
    void (*blt_List_DeleteNodeByKey) (Blt_List list, const char *key); /* 53 */
    Blt_ListNode (*blt_List_GetNthNode) (Blt_List list, long position, int direction); /* 54 */
    void (*blt_List_Sort) (Blt_List list, Blt_ListCompareProc *proc); /* 55 */
    Blt_Pool (*blt_Pool_Create) (int type); /* 56 */
    void (*blt_Pool_Destroy) (Blt_Pool pool); /* 57 */
    Blt_TreeKey (*blt_Tree_GetKey) (Blt_Tree tree, const char *string); /* 58 */
    Blt_TreeKey (*blt_Tree_GetKeyFromNode) (Blt_TreeNode node, const char *string); /* 59 */
    Blt_TreeNode (*blt_Tree_CreateNode) (Blt_Tree tree, Blt_TreeNode parent, const char *name, long position); /* 60 */
    Blt_TreeNode (*blt_Tree_CreateNodeWithId) (Blt_Tree tree, Blt_TreeNode parent, const char *name, long inode, long position); /* 61 */
    int (*blt_Tree_DeleteNode) (Blt_Tree tree, Blt_TreeNode node); /* 62 */
    int (*blt_Tree_MoveNode) (Blt_Tree tree, Blt_TreeNode node, Blt_TreeNode parent, Blt_TreeNode before); /* 63 */
    Blt_TreeNode (*blt_Tree_GetNodeFromIndex) (Blt_Tree tree, long inode); /* 64 */
    Blt_TreeNode (*blt_Tree_FindChild) (Blt_TreeNode parent, const char *name); /* 65 */
    Blt_TreeNode (*blt_Tree_NextNode) (Blt_TreeNode root, Blt_TreeNode node); /* 66 */
    Blt_TreeNode (*blt_Tree_PrevNode) (Blt_TreeNode root, Blt_TreeNode node); /* 67 */
    Blt_TreeNode (*blt_Tree_FirstChild) (Blt_TreeNode parent); /* 68 */
    Blt_TreeNode (*blt_Tree_LastChild) (Blt_TreeNode parent); /* 69 */
    int (*blt_Tree_IsBefore) (Blt_TreeNode node1, Blt_TreeNode node2); /* 70 */
    int (*blt_Tree_IsAncestor) (Blt_TreeNode node1, Blt_TreeNode node2); /* 71 */
    int (*blt_Tree_PrivateValue) (Tcl_Interp *interp, Blt_Tree tree, Blt_TreeNode node, Blt_TreeKey key); /* 72 */
    int (*blt_Tree_PublicValue) (Tcl_Interp *interp, Blt_Tree tree, Blt_TreeNode node, Blt_TreeKey key); /* 73 */
    int (*blt_Tree_GetValue) (Tcl_Interp *interp, Blt_Tree tree, Blt_TreeNode node, const char *string, Tcl_Obj **valuePtr); /* 74 */
    int (*blt_Tree_ValueExists) (Blt_Tree tree, Blt_TreeNode node, const char *string); /* 75 */
    int (*blt_Tree_SetValue) (Tcl_Interp *interp, Blt_Tree tree, Blt_TreeNode node, const char *string, Tcl_Obj *valuePtr); /* 76 */
    int (*blt_Tree_UnsetValue) (Tcl_Interp *interp, Blt_Tree tree, Blt_TreeNode node, const char *string); /* 77 */
    int (*blt_Tree_AppendValue) (Tcl_Interp *interp, Blt_Tree tree, Blt_TreeNode node, const char *string, const char *value); /* 78 */
    int (*blt_Tree_ListAppendValue) (Tcl_Interp *interp, Blt_Tree tree, Blt_TreeNode node, const char *string, Tcl_Obj *valuePtr); /* 79 */
    int (*blt_Tree_GetArrayValue) (Tcl_Interp *interp, Blt_Tree tree, Blt_TreeNode node, const char *arrayName, const char *elemName, Tcl_Obj **valueObjPtrPtr); /* 80 */
    int (*blt_Tree_SetArrayValue) (Tcl_Interp *interp, Blt_Tree tree, Blt_TreeNode node, const char *arrayName, const char *elemName, Tcl_Obj *valueObjPtr); /* 81 */
    int (*blt_Tree_UnsetArrayValue) (Tcl_Interp *interp, Blt_Tree tree, Blt_TreeNode node, const char *arrayName, const char *elemName); /* 82 */
    int (*blt_Tree_AppendArrayValue) (Tcl_Interp *interp, Blt_Tree tree, Blt_TreeNode node, const char *arrayName, const char *elemName, const char *value); /* 83 */
    int (*blt_Tree_ListAppendArrayValue) (Tcl_Interp *interp, Blt_Tree tree, Blt_TreeNode node, const char *arrayName, const char *elemName, Tcl_Obj *valueObjPtr); /* 84 */
    int (*blt_Tree_ArrayValueExists) (Blt_Tree tree, Blt_TreeNode node, const char *arrayName, const char *elemName); /* 85 */
    int (*blt_Tree_ArrayNames) (Tcl_Interp *interp, Blt_Tree tree, Blt_TreeNode node, const char *arrayName, Tcl_Obj *listObjPtr); /* 86 */
    int (*blt_Tree_GetValueByKey) (Tcl_Interp *interp, Blt_Tree tree, Blt_TreeNode node, Blt_TreeKey key, Tcl_Obj **valuePtr); /* 87 */
    int (*blt_Tree_SetValueByKey) (Tcl_Interp *interp, Blt_Tree tree, Blt_TreeNode node, Blt_TreeKey key, Tcl_Obj *valuePtr); /* 88 */
    int (*blt_Tree_UnsetValueByKey) (Tcl_Interp *interp, Blt_Tree tree, Blt_TreeNode node, Blt_TreeKey key); /* 89 */
    int (*blt_Tree_AppendValueByKey) (Tcl_Interp *interp, Blt_Tree tree, Blt_TreeNode node, Blt_TreeKey key, const char *value); /* 90 */
    int (*blt_Tree_ListAppendValueByKey) (Tcl_Interp *interp, Blt_Tree tree, Blt_TreeNode node, Blt_TreeKey key, Tcl_Obj *valuePtr); /* 91 */
    int (*blt_Tree_ValueExistsByKey) (Blt_Tree tree, Blt_TreeNode node, Blt_TreeKey key); /* 92 */
    Blt_TreeKey (*blt_Tree_FirstKey) (Blt_Tree tree, Blt_TreeNode node, Blt_TreeKeyIterator *iterPtr); /* 93 */
    Blt_TreeKey (*blt_Tree_NextKey) (Blt_Tree tree, Blt_TreeKeyIterator *iterPtr); /* 94 */
    int (*blt_Tree_Apply) (Blt_TreeNode root, Blt_TreeApplyProc *proc, ClientData clientData); /* 95 */
    int (*blt_Tree_ApplyDFS) (Blt_TreeNode root, Blt_TreeApplyProc *proc, ClientData clientData, int order); /* 96 */
    int (*blt_Tree_ApplyBFS) (Blt_TreeNode root, Blt_TreeApplyProc *proc, ClientData clientData); /* 97 */
    int (*blt_Tree_SortNode) (Blt_Tree tree, Blt_TreeNode node, Blt_TreeCompareNodesProc *proc); /* 98 */
    int (*blt_Tree_Exists) (Tcl_Interp *interp, const char *name); /* 99 */
    Blt_Tree (*blt_Tree_Open) (Tcl_Interp *interp, const char *name, int flags); /* 100 */
    void (*blt_Tree_Close) (Blt_Tree tree); /* 101 */
    int (*blt_Tree_Attach) (Tcl_Interp *interp, Blt_Tree tree, const char *name); /* 102 */
    Blt_Tree (*blt_Tree_GetFromObj) (Tcl_Interp *interp, Tcl_Obj *objPtr); /* 103 */
    int (*blt_Tree_Size) (Blt_TreeNode node); /* 104 */
    Blt_TreeTrace (*blt_Tree_CreateTrace) (Blt_Tree tree, Blt_TreeNode node, const char *keyPattern, const char *tagName, unsigned int mask, Blt_TreeTraceProc *proc, ClientData clientData); /* 105 */
    void (*blt_Tree_DeleteTrace) (Blt_TreeTrace token); /* 106 */
    void (*blt_Tree_CreateEventHandler) (Blt_Tree tree, unsigned int mask, Blt_TreeNotifyEventProc *proc, ClientData clientData); /* 107 */
    void (*blt_Tree_DeleteEventHandler) (Blt_Tree tree, unsigned int mask, Blt_TreeNotifyEventProc *proc, ClientData clientData); /* 108 */
    void (*blt_Tree_RelabelNode) (Blt_Tree tree, Blt_TreeNode node, const char *string); /* 109 */
    void (*blt_Tree_RelabelNodeWithoutNotify) (Blt_TreeNode node, const char *string); /* 110 */
    const char * (*blt_Tree_NodeIdAscii) (Blt_TreeNode node); /* 111 */
    const char * (*blt_Tree_NodePath) (Blt_TreeNode node, Tcl_DString *resultPtr); /* 112 */
    const char * (*blt_Tree_NodeRelativePath) (Blt_TreeNode root, Blt_TreeNode node, const char *separator, unsigned int flags, Tcl_DString *resultPtr); /* 113 */
    long (*blt_Tree_NodePosition) (Blt_TreeNode node); /* 114 */
    void (*blt_Tree_ClearTags) (Blt_Tree tree, Blt_TreeNode node); /* 115 */
    int (*blt_Tree_HasTag) (Blt_Tree tree, Blt_TreeNode node, const char *tagName); /* 116 */
    void (*blt_Tree_AddTag) (Blt_Tree tree, Blt_TreeNode node, const char *tagName); /* 117 */
    void (*blt_Tree_RemoveTag) (Blt_Tree tree, Blt_TreeNode node, const char *tagName); /* 118 */
    void (*blt_Tree_ForgetTag) (Blt_Tree tree, const char *tagName); /* 119 */
    Blt_HashTable * (*blt_Tree_TagHashTable) (Blt_Tree tree, const char *tagName); /* 120 */
    int (*blt_Tree_TagTableIsShared) (Blt_Tree tree); /* 121 */
    void (*blt_Tree_NewTagTable) (Blt_Tree tree); /* 122 */
    Blt_HashEntry * (*blt_Tree_FirstTag) (Blt_Tree tree, Blt_HashSearch *searchPtr); /* 123 */
    long (*blt_Tree_Depth) (Blt_Tree tree); /* 124 */
    int (*blt_Tree_RegisterFormat) (Tcl_Interp *interp, const char *fmtName, Blt_TreeImportProc *importProc, Blt_TreeExportProc *exportProc); /* 125 */
    Blt_TreeTagEntry * (*blt_Tree_RememberTag) (Blt_Tree tree, const char *name); /* 126 */
    int (*blt_Tree_GetNodeFromObj) (Tcl_Interp *interp, Blt_Tree tree, Tcl_Obj *objPtr, Blt_TreeNode *nodePtr); /* 127 */
    int (*blt_Tree_GetNodeIterator) (Tcl_Interp *interp, Blt_Tree tree, Tcl_Obj *objPtr, Blt_TreeIterator *iterPtr); /* 128 */
    Blt_TreeNode (*blt_Tree_FirstTaggedNode) (Blt_TreeIterator *iterPtr); /* 129 */
    Blt_TreeNode (*blt_Tree_NextTaggedNode) (Blt_TreeIterator *iterPtr); /* 130 */
    void (*blt_table_release_tags) (BLT_TABLE table); /* 131 */
    void (*blt_table_new_tags) (BLT_TABLE table); /* 132 */
    Blt_HashTable * (*blt_table_get_column_tag_table) (BLT_TABLE table); /* 133 */
    Blt_HashTable * (*blt_table_get_row_tag_table) (BLT_TABLE table); /* 134 */
    int (*blt_table_exists) (Tcl_Interp *interp, const char *name); /* 135 */
    int (*blt_table_create) (Tcl_Interp *interp, const char *name, BLT_TABLE *tablePtr); /* 136 */
    int (*blt_table_open) (Tcl_Interp *interp, const char *name, BLT_TABLE *tablePtr); /* 137 */
    void (*blt_table_close) (BLT_TABLE table); /* 138 */
    void (*blt_table_clear) (BLT_TABLE table); /* 139 */
    void (*blt_table_pack) (BLT_TABLE table); /* 140 */
    int (*blt_table_same_object) (BLT_TABLE table1, BLT_TABLE table2); /* 141 */
    Blt_HashTable * (*blt_table_row_get_label_table) (BLT_TABLE table, const char *label); /* 142 */
    Blt_HashTable * (*blt_table_column_get_label_table) (BLT_TABLE table, const char *label); /* 143 */
    BLT_TABLE_ROW (*blt_table_get_row) (Tcl_Interp *interp, BLT_TABLE table, Tcl_Obj *objPtr); /* 144 */
    BLT_TABLE_COLUMN (*blt_table_get_column) (Tcl_Interp *interp, BLT_TABLE table, Tcl_Obj *objPtr); /* 145 */
    BLT_TABLE_ROW (*blt_table_get_row_by_label) (BLT_TABLE table, const char *label); /* 146 */
    BLT_TABLE_COLUMN (*blt_table_get_column_by_label) (BLT_TABLE table, const char *label); /* 147 */
    BLT_TABLE_ROW (*blt_table_get_row_by_index) (BLT_TABLE table, long index); /* 148 */
    BLT_TABLE_COLUMN (*blt_table_get_column_by_index) (BLT_TABLE table, long index); /* 149 */
    int (*blt_table_set_row_label) (Tcl_Interp *interp, BLT_TABLE table, BLT_TABLE_ROW row, const char *label); /* 150 */
    int (*blt_table_set_column_label) (Tcl_Interp *interp, BLT_TABLE table, BLT_TABLE_COLUMN column, const char *label); /* 151 */
    BLT_TABLE_COLUMN_TYPE (*blt_table_name_to_column_type) (const char *typeName); /* 152 */
    int (*blt_table_set_column_type) (Tcl_Interp *interp, BLT_TABLE table, BLT_TABLE_COLUMN column, BLT_TABLE_COLUMN_TYPE type); /* 153 */
    const char * (*blt_table_column_type_to_name) (BLT_TABLE_COLUMN_TYPE type); /* 154 */
    int (*blt_table_set_column_tag) (Tcl_Interp *interp, BLT_TABLE table, BLT_TABLE_COLUMN column, const char *tag); /* 155 */
    int (*blt_table_set_row_tag) (Tcl_Interp *interp, BLT_TABLE table, BLT_TABLE_ROW row, const char *tag); /* 156 */
    BLT_TABLE_ROW (*blt_table_create_row) (Tcl_Interp *interp, BLT_TABLE table, const char *label); /* 157 */
    BLT_TABLE_COLUMN (*blt_table_create_column) (Tcl_Interp *interp, BLT_TABLE table, const char *label); /* 158 */
    int (*blt_table_extend_rows) (Tcl_Interp *interp, BLT_TABLE table, size_t n, BLT_TABLE_ROW *rows); /* 159 */
    int (*blt_table_extend_columns) (Tcl_Interp *interp, BLT_TABLE table, size_t n, BLT_TABLE_COLUMN *columms); /* 160 */
    int (*blt_table_delete_row) (BLT_TABLE table, BLT_TABLE_ROW row); /* 161 */
    int (*blt_table_delete_column) (BLT_TABLE table, BLT_TABLE_COLUMN column); /* 162 */
    int (*blt_table_move_rows) (Tcl_Interp *interp, BLT_TABLE table, BLT_TABLE_ROW destRow, BLT_TABLE_ROW firstRow, BLT_TABLE_ROW lastRow, int after); /* 163 */
    int (*blt_table_move_columns) (Tcl_Interp *interp, BLT_TABLE table, BLT_TABLE_COLUMN destColumn, BLT_TABLE_COLUMN firstColumn, BLT_TABLE_COLUMN lastColumn, int after); /* 164 */
    Tcl_Obj * (*blt_table_get_obj) (BLT_TABLE table, BLT_TABLE_ROW row, BLT_TABLE_COLUMN column); /* 165 */
    int (*blt_table_set_obj) (Tcl_Interp *interp, BLT_TABLE table, BLT_TABLE_ROW row, BLT_TABLE_COLUMN column, Tcl_Obj *objPtr); /* 166 */
    const char * (*blt_table_get_string) (BLT_TABLE table, BLT_TABLE_ROW row, BLT_TABLE_COLUMN column); /* 167 */
    int (*blt_table_set_string_rep) (Tcl_Interp *interp, BLT_TABLE table, BLT_TABLE_ROW row, BLT_TABLE_COLUMN column, const char *string, int length); /* 168 */
    int (*blt_table_set_string) (Tcl_Interp *interp, BLT_TABLE table, BLT_TABLE_ROW row, BLT_TABLE_COLUMN column, const char *string, int length); /* 169 */
    int (*blt_table_append_string) (Tcl_Interp *interp, BLT_TABLE table, BLT_TABLE_ROW row, BLT_TABLE_COLUMN column, const char *string, int length); /* 170 */
    int (*blt_table_set_bytes) (Tcl_Interp *interp, BLT_TABLE table, BLT_TABLE_ROW row, BLT_TABLE_COLUMN column, const unsigned char *string, int length); /* 171 */
    double (*blt_table_get_double) (Tcl_Interp *interp, BLT_TABLE table, BLT_TABLE_ROW row, BLT_TABLE_COLUMN column); /* 172 */
    int (*blt_table_set_double) (Tcl_Interp *interp, BLT_TABLE table, BLT_TABLE_ROW row, BLT_TABLE_COLUMN column, double value); /* 173 */
    long (*blt_table_get_long) (Tcl_Interp *interp, BLT_TABLE table, BLT_TABLE_ROW row, BLT_TABLE_COLUMN column, long defValue); /* 174 */
    int (*blt_table_set_long) (Tcl_Interp *interp, BLT_TABLE table, BLT_TABLE_ROW row, BLT_TABLE_COLUMN column, long value); /* 175 */
    int (*blt_table_get_boolean) (Tcl_Interp *interp, BLT_TABLE table, BLT_TABLE_ROW row, BLT_TABLE_COLUMN column, int defValue); /* 176 */
    int (*blt_table_set_boolean) (Tcl_Interp *interp, BLT_TABLE table, BLT_TABLE_ROW row, BLT_TABLE_COLUMN column, int value); /* 177 */
    BLT_TABLE_VALUE (*blt_table_get_value) (BLT_TABLE table, BLT_TABLE_ROW row, BLT_TABLE_COLUMN column); /* 178 */
    int (*blt_table_set_value) (BLT_TABLE table, BLT_TABLE_ROW row, BLT_TABLE_COLUMN column, BLT_TABLE_VALUE value); /* 179 */
    int (*blt_table_unset_value) (BLT_TABLE table, BLT_TABLE_ROW row, BLT_TABLE_COLUMN column); /* 180 */
    int (*blt_table_value_exists) (BLT_TABLE table, BLT_TABLE_ROW row, BLT_TABLE_COLUMN column); /* 181 */
    const char * (*blt_table_value_string) (BLT_TABLE_VALUE value); /* 182 */
    const unsigned char * (*blt_table_value_bytes) (BLT_TABLE_VALUE value); /* 183 */
    size_t (*blt_table_value_length) (BLT_TABLE_VALUE value); /* 184 */
    int (*blt_table_tags_are_shared) (BLT_TABLE table); /* 185 */
    void (*blt_table_clear_row_tags) (BLT_TABLE table, BLT_TABLE_ROW row); /* 186 */
    void (*blt_table_clear_column_tags) (BLT_TABLE table, BLT_TABLE_COLUMN col); /* 187 */
    Blt_Chain (*blt_table_get_row_tags) (BLT_TABLE table, BLT_TABLE_ROW row); /* 188 */
    Blt_Chain (*blt_table_get_column_tags) (BLT_TABLE table, BLT_TABLE_COLUMN column); /* 189 */
    Blt_Chain (*blt_table_get_tagged_rows) (BLT_TABLE table, const char *tag); /* 190 */
    Blt_Chain (*blt_table_get_tagged_columns) (BLT_TABLE table, const char *tag); /* 191 */
    int (*blt_table_row_has_tag) (BLT_TABLE table, BLT_TABLE_ROW row, const char *tag); /* 192 */
    int (*blt_table_column_has_tag) (BLT_TABLE table, BLT_TABLE_COLUMN column, const char *tag); /* 193 */
    int (*blt_table_forget_row_tag) (Tcl_Interp *interp, BLT_TABLE table, const char *tag); /* 194 */
    int (*blt_table_forget_column_tag) (Tcl_Interp *interp, BLT_TABLE table, const char *tag); /* 195 */
    int (*blt_table_unset_row_tag) (Tcl_Interp *interp, BLT_TABLE table, BLT_TABLE_ROW row, const char *tag); /* 196 */
    int (*blt_table_unset_column_tag) (Tcl_Interp *interp, BLT_TABLE table, BLT_TABLE_COLUMN column, const char *tag); /* 197 */
    BLT_TABLE_COLUMN (*blt_table_first_column) (BLT_TABLE table); /* 198 */
    BLT_TABLE_COLUMN (*blt_table_last_column) (BLT_TABLE table); /* 199 */
    BLT_TABLE_COLUMN (*blt_table_next_column) (BLT_TABLE_COLUMN column); /* 200 */
    BLT_TABLE_COLUMN (*blt_table_previous_column) (BLT_TABLE_COLUMN column); /* 201 */
    BLT_TABLE_ROW (*blt_table_first_row) (BLT_TABLE table); /* 202 */
    BLT_TABLE_ROW (*blt_table_last_row) (BLT_TABLE table); /* 203 */
    BLT_TABLE_ROW (*blt_table_next_row) (BLT_TABLE_ROW row); /* 204 */
    BLT_TABLE_ROW (*blt_table_previous_row) (BLT_TABLE_ROW row); /* 205 */
    BLT_TABLE_ROWCOLUMN_SPEC (*blt_table_row_spec) (BLT_TABLE table, Tcl_Obj *objPtr, const char **sp); /* 206 */
    BLT_TABLE_ROWCOLUMN_SPEC (*blt_table_column_spec) (BLT_TABLE table, Tcl_Obj *objPtr, const char **sp); /* 207 */
    int (*blt_table_iterate_rows) (Tcl_Interp *interp, BLT_TABLE table, Tcl_Obj *objPtr, BLT_TABLE_ITERATOR *iter); /* 208 */
    int (*blt_table_iterate_columns) (Tcl_Interp *interp, BLT_TABLE table, Tcl_Obj *objPtr, BLT_TABLE_ITERATOR *iter); /* 209 */
    int (*blt_table_iterate_rows_objv) (Tcl_Interp *interp, BLT_TABLE table, int objc, Tcl_Obj *const *objv, BLT_TABLE_ITERATOR *iterPtr); /* 210 */
    int (*blt_table_iterate_columns_objv) (Tcl_Interp *interp, BLT_TABLE table, int objc, Tcl_Obj *const *objv, BLT_TABLE_ITERATOR *iterPtr); /* 211 */
    void (*blt_table_free_iterator_objv) (BLT_TABLE_ITERATOR *iterPtr); /* 212 */
    void (*blt_table_iterate_all_rows) (BLT_TABLE table, BLT_TABLE_ITERATOR *iterPtr); /* 213 */
    void (*blt_table_iterate_all_columns) (BLT_TABLE table, BLT_TABLE_ITERATOR *iterPtr); /* 214 */
    BLT_TABLE_ROW (*blt_table_first_tagged_row) (BLT_TABLE_ITERATOR *iter); /* 215 */
    BLT_TABLE_COLUMN (*blt_table_first_tagged_column) (BLT_TABLE_ITERATOR *iter); /* 216 */
    BLT_TABLE_ROW (*blt_table_next_tagged_row) (BLT_TABLE_ITERATOR *iter); /* 217 */
    BLT_TABLE_COLUMN (*blt_table_next_tagged_column) (BLT_TABLE_ITERATOR *iter); /* 218 */
    int (*blt_table_list_rows) (Tcl_Interp *interp, BLT_TABLE table, int objc, Tcl_Obj *const *objv, Blt_Chain chain); /* 219 */
    int (*blt_table_list_columns) (Tcl_Interp *interp, BLT_TABLE table, int objc, Tcl_Obj *const *objv, Blt_Chain chain); /* 220 */
    void (*blt_table_clear_row_traces) (BLT_TABLE table, BLT_TABLE_ROW row); /* 221 */
    void (*blt_table_clear_column_traces) (BLT_TABLE table, BLT_TABLE_COLUMN column); /* 222 */
    BLT_TABLE_TRACE (*blt_table_create_trace) (BLT_TABLE table, BLT_TABLE_ROW row, BLT_TABLE_COLUMN column, const char *rowTag, const char *columnTag, unsigned int mask, BLT_TABLE_TRACE_PROC *proc, BLT_TABLE_TRACE_DELETE_PROC *deleteProc, ClientData clientData); /* 223 */
    void (*blt_table_trace_column) (BLT_TABLE table, BLT_TABLE_COLUMN column, unsigned int mask, BLT_TABLE_TRACE_PROC *proc, BLT_TABLE_TRACE_DELETE_PROC *deleteProc, ClientData clientData); /* 224 */
    void (*blt_table_trace_row) (BLT_TABLE table, BLT_TABLE_ROW row, unsigned int mask, BLT_TABLE_TRACE_PROC *proc, BLT_TABLE_TRACE_DELETE_PROC *deleteProc, ClientData clientData); /* 225 */
    BLT_TABLE_TRACE (*blt_table_create_column_trace) (BLT_TABLE table, BLT_TABLE_COLUMN column, unsigned int mask, BLT_TABLE_TRACE_PROC *proc, BLT_TABLE_TRACE_DELETE_PROC *deleteProc, ClientData clientData); /* 226 */
    BLT_TABLE_TRACE (*blt_table_create_column_tag_trace) (BLT_TABLE table, const char *tag, unsigned int mask, BLT_TABLE_TRACE_PROC *proc, BLT_TABLE_TRACE_DELETE_PROC *deleteProc, ClientData clientData); /* 227 */
    BLT_TABLE_TRACE (*blt_table_create_row_trace) (BLT_TABLE table, BLT_TABLE_ROW row, unsigned int mask, BLT_TABLE_TRACE_PROC *proc, BLT_TABLE_TRACE_DELETE_PROC *deleteProc, ClientData clientData); /* 228 */
    BLT_TABLE_TRACE (*blt_table_create_row_tag_trace) (BLT_TABLE table, const char *tag, unsigned int mask, BLT_TABLE_TRACE_PROC *proc, BLT_TABLE_TRACE_DELETE_PROC *deleteProc, ClientData clientData); /* 229 */
    void (*blt_table_delete_trace) (BLT_TABLE table, BLT_TABLE_TRACE trace); /* 230 */
    BLT_TABLE_NOTIFIER (*blt_table_create_notifier) (Tcl_Interp *interp, BLT_TABLE table, unsigned int mask, BLT_TABLE_NOTIFY_EVENT_PROC *proc, BLT_TABLE_NOTIFIER_DELETE_PROC *deleteProc, ClientData clientData); /* 231 */
    BLT_TABLE_NOTIFIER (*blt_table_create_row_notifier) (Tcl_Interp *interp, BLT_TABLE table, BLT_TABLE_ROW row, unsigned int mask, BLT_TABLE_NOTIFY_EVENT_PROC *proc, BLT_TABLE_NOTIFIER_DELETE_PROC *deleteProc, ClientData clientData); /* 232 */
    BLT_TABLE_NOTIFIER (*blt_table_create_row_tag_notifier) (Tcl_Interp *interp, BLT_TABLE table, const char *tag, unsigned int mask, BLT_TABLE_NOTIFY_EVENT_PROC *proc, BLT_TABLE_NOTIFIER_DELETE_PROC *deleteProc, ClientData clientData); /* 233 */
    BLT_TABLE_NOTIFIER (*blt_table_create_column_notifier) (Tcl_Interp *interp, BLT_TABLE table, BLT_TABLE_COLUMN column, unsigned int mask, BLT_TABLE_NOTIFY_EVENT_PROC *proc, BLT_TABLE_NOTIFIER_DELETE_PROC *deleteProc, ClientData clientData); /* 234 */
    BLT_TABLE_NOTIFIER (*blt_table_create_column_tag_notifier) (Tcl_Interp *interp, BLT_TABLE table, const char *tag, unsigned int mask, BLT_TABLE_NOTIFY_EVENT_PROC *proc, BLT_TABLE_NOTIFIER_DELETE_PROC *deleteProc, ClientData clientData); /* 235 */
    void (*blt_table_delete_notifier) (BLT_TABLE table, BLT_TABLE_NOTIFIER notifier); /* 236 */
    void (*blt_table_sort_init) (BLT_TABLE table, BLT_TABLE_SORT_ORDER *order, size_t numCompares, unsigned int flags); /* 237 */
    BLT_TABLE_ROW * (*blt_table_sort_rows) (BLT_TABLE table); /* 238 */
    void (*blt_table_sort_rows_subset) (BLT_TABLE table, long numRows, BLT_TABLE_ROW *rows); /* 239 */
    void (*blt_table_sort_finish) (void); /* 240 */
    BLT_TABLE_COMPARE_PROC * (*blt_table_get_compare_proc) (BLT_TABLE table, BLT_TABLE_COLUMN column, unsigned int flags); /* 241 */
    BLT_TABLE_ROW * (*blt_table_get_row_map) (BLT_TABLE table); /* 242 */
    BLT_TABLE_COLUMN * (*blt_table_get_column_map) (BLT_TABLE table); /* 243 */
    void (*blt_table_set_row_map) (BLT_TABLE table, BLT_TABLE_ROW *map); /* 244 */
    void (*blt_table_set_column_map) (BLT_TABLE table, BLT_TABLE_COLUMN *map); /* 245 */
    int (*blt_table_restore) (Tcl_Interp *interp, BLT_TABLE table, char *string, unsigned int flags); /* 246 */
    int (*blt_table_file_restore) (Tcl_Interp *interp, BLT_TABLE table, const char *fileName, unsigned int flags); /* 247 */
    int (*blt_table_register_format) (Tcl_Interp *interp, const char *name, BLT_TABLE_IMPORT_PROC *importProc, BLT_TABLE_EXPORT_PROC *exportProc); /* 248 */
    void (*blt_table_unset_keys) (BLT_TABLE table); /* 249 */
    int (*blt_table_get_keys) (BLT_TABLE table, BLT_TABLE_COLUMN **keysPtr); /* 250 */
    int (*blt_table_set_keys) (BLT_TABLE table, int numKeys, BLT_TABLE_COLUMN *keys, int unique); /* 251 */
    int (*blt_table_key_lookup) (Tcl_Interp *interp, BLT_TABLE table, int objc, Tcl_Obj *const *objv, BLT_TABLE_ROW *rowPtr); /* 252 */
    int (*blt_table_get_column_limits) (Tcl_Interp *interp, BLT_TABLE table, BLT_TABLE_COLUMN col, Tcl_Obj **minObjPtrPtr, Tcl_Obj **maxObjPtrPtr); /* 253 */
    BLT_TABLE_ROW (*blt_table_row) (BLT_TABLE table, long index); /* 254 */
    BLT_TABLE_COLUMN (*blt_table_column) (BLT_TABLE table, long index); /* 255 */
    long (*blt_table_row_index) (BLT_TABLE table, BLT_TABLE_ROW row); /* 256 */
    long (*blt_table_column_index) (BLT_TABLE table, BLT_TABLE_COLUMN column); /* 257 */
    double (*blt_VecMin) (Blt_Vector *vPtr); /* 258 */
    double (*blt_VecMax) (Blt_Vector *vPtr); /* 259 */
    Blt_VectorId (*blt_AllocVectorId) (Tcl_Interp *interp, const char *vecName); /* 260 */
    void (*blt_SetVectorChangedProc) (Blt_VectorId clientId, Blt_VectorChangedProc *proc, ClientData clientData); /* 261 */
    void (*blt_FreeVectorId) (Blt_VectorId clientId); /* 262 */
    int (*blt_GetVectorById) (Tcl_Interp *interp, Blt_VectorId clientId, Blt_Vector **vecPtrPtr); /* 263 */
    const char * (*blt_NameOfVectorId) (Blt_VectorId clientId); /* 264 */
    const char * (*blt_NameOfVector) (Blt_Vector *vecPtr); /* 265 */
    int (*blt_VectorNotifyPending) (Blt_VectorId clientId); /* 266 */
    int (*blt_CreateVector) (Tcl_Interp *interp, const char *vecName, size_t size, Blt_Vector **vecPtrPtr); /* 267 */
    int (*blt_CreateVector2) (Tcl_Interp *interp, const char *vecName, const char *cmdName, const char *varName, size_t initialSize, Blt_Vector **vecPtrPtr); /* 268 */
    int (*blt_GetVector) (Tcl_Interp *interp, const char *vecName, Blt_Vector **vecPtrPtr); /* 269 */
    int (*blt_GetVectorFromObj) (Tcl_Interp *interp, Tcl_Obj *objPtr, Blt_Vector **vecPtrPtr); /* 270 */
    int (*blt_VectorExists) (Tcl_Interp *interp, const char *vecName); /* 271 */
    int (*blt_ResetVector) (Blt_Vector *vecPtr, double *dataArr, size_t n, size_t arraySize, Tcl_FreeProc *freeProc); /* 272 */
    int (*blt_ResizeVector) (Blt_Vector *vecPtr, size_t n); /* 273 */
    int (*blt_DeleteVectorByName) (Tcl_Interp *interp, const char *vecName); /* 274 */
    int (*blt_DeleteVector) (Blt_Vector *vecPtr); /* 275 */
    int (*blt_ExprVector) (Tcl_Interp *interp, char *expr, Blt_Vector *vecPtr); /* 276 */
    void (*blt_InstallIndexProc) (Tcl_Interp *interp, const char *indexName, Blt_VectorIndexProc *procPtr); /* 277 */
    int (*blt_VectorExists2) (Tcl_Interp *interp, const char *vecName); /* 278 */
} BltTclProcs;

#ifdef __cplusplus
extern "C" {
#endif
extern BltTclProcs *bltTclProcsPtr;
#ifdef __cplusplus
}
#endif

#if defined(USE_BLT_STUBS) && !defined(BUILD_BLT_TCL_PROCS)

/*
 * Inline function declarations:
 */

/* Slot 0 is reserved */
#ifndef Blt_AllocInit
#define Blt_AllocInit \
	(bltTclProcsPtr->blt_AllocInit) /* 1 */
#endif
#ifndef Blt_Chain_Init
#define Blt_Chain_Init \
	(bltTclProcsPtr->blt_Chain_Init) /* 2 */
#endif
#ifndef Blt_Chain_Create
#define Blt_Chain_Create \
	(bltTclProcsPtr->blt_Chain_Create) /* 3 */
#endif
#ifndef Blt_Chain_Destroy
#define Blt_Chain_Destroy \
	(bltTclProcsPtr->blt_Chain_Destroy) /* 4 */
#endif
#ifndef Blt_Chain_NewLink
#define Blt_Chain_NewLink \
	(bltTclProcsPtr->blt_Chain_NewLink) /* 5 */
#endif
#ifndef Blt_Chain_AllocLink
#define Blt_Chain_AllocLink \
	(bltTclProcsPtr->blt_Chain_AllocLink) /* 6 */
#endif
#ifndef Blt_Chain_Append
#define Blt_Chain_Append \
	(bltTclProcsPtr->blt_Chain_Append) /* 7 */
#endif
#ifndef Blt_Chain_Prepend
#define Blt_Chain_Prepend \
	(bltTclProcsPtr->blt_Chain_Prepend) /* 8 */
#endif
#ifndef Blt_Chain_Reset
#define Blt_Chain_Reset \
	(bltTclProcsPtr->blt_Chain_Reset) /* 9 */
#endif
#ifndef Blt_Chain_InitLink
#define Blt_Chain_InitLink \
	(bltTclProcsPtr->blt_Chain_InitLink) /* 10 */
#endif
#ifndef Blt_Chain_LinkAfter
#define Blt_Chain_LinkAfter \
	(bltTclProcsPtr->blt_Chain_LinkAfter) /* 11 */
#endif
#ifndef Blt_Chain_LinkBefore
#define Blt_Chain_LinkBefore \
	(bltTclProcsPtr->blt_Chain_LinkBefore) /* 12 */
#endif
#ifndef Blt_Chain_UnlinkLink
#define Blt_Chain_UnlinkLink \
	(bltTclProcsPtr->blt_Chain_UnlinkLink) /* 13 */
#endif
#ifndef Blt_Chain_DeleteLink
#define Blt_Chain_DeleteLink \
	(bltTclProcsPtr->blt_Chain_DeleteLink) /* 14 */
#endif
#ifndef Blt_Chain_GetNthLink
#define Blt_Chain_GetNthLink \
	(bltTclProcsPtr->blt_Chain_GetNthLink) /* 15 */
#endif
#ifndef Blt_Chain_Sort
#define Blt_Chain_Sort \
	(bltTclProcsPtr->blt_Chain_Sort) /* 16 */
#endif
#ifndef Blt_Chain_Reverse
#define Blt_Chain_Reverse \
	(bltTclProcsPtr->blt_Chain_Reverse) /* 17 */
#endif
#ifndef Blt_Chain_IsBefore
#define Blt_Chain_IsBefore \
	(bltTclProcsPtr->blt_Chain_IsBefore) /* 18 */
#endif
#ifndef Blt_InitHashTable
#define Blt_InitHashTable \
	(bltTclProcsPtr->blt_InitHashTable) /* 19 */
#endif
#ifndef Blt_InitHashTableWithPool
#define Blt_InitHashTableWithPool \
	(bltTclProcsPtr->blt_InitHashTableWithPool) /* 20 */
#endif
#ifndef Blt_DeleteHashTable
#define Blt_DeleteHashTable \
	(bltTclProcsPtr->blt_DeleteHashTable) /* 21 */
#endif
#ifndef Blt_DeleteHashEntry
#define Blt_DeleteHashEntry \
	(bltTclProcsPtr->blt_DeleteHashEntry) /* 22 */
#endif
#ifndef Blt_FirstHashEntry
#define Blt_FirstHashEntry \
	(bltTclProcsPtr->blt_FirstHashEntry) /* 23 */
#endif
#ifndef Blt_NextHashEntry
#define Blt_NextHashEntry \
	(bltTclProcsPtr->blt_NextHashEntry) /* 24 */
#endif
#ifndef Blt_HashStats
#define Blt_HashStats \
	(bltTclProcsPtr->blt_HashStats) /* 25 */
#endif
#ifndef Blt_Tags_Create
#define Blt_Tags_Create \
	(bltTclProcsPtr->blt_Tags_Create) /* 26 */
#endif
#ifndef Blt_Tags_Destroy
#define Blt_Tags_Destroy \
	(bltTclProcsPtr->blt_Tags_Destroy) /* 27 */
#endif
#ifndef Blt_Tags_Init
#define Blt_Tags_Init \
	(bltTclProcsPtr->blt_Tags_Init) /* 28 */
#endif
#ifndef Blt_Tags_Reset
#define Blt_Tags_Reset \
	(bltTclProcsPtr->blt_Tags_Reset) /* 29 */
#endif
#ifndef Blt_Tags_ItemHasTag
#define Blt_Tags_ItemHasTag \
	(bltTclProcsPtr->blt_Tags_ItemHasTag) /* 30 */
#endif
#ifndef Blt_Tags_AddTag
#define Blt_Tags_AddTag \
	(bltTclProcsPtr->blt_Tags_AddTag) /* 31 */
#endif
#ifndef Blt_Tags_AddItemToTag
#define Blt_Tags_AddItemToTag \
	(bltTclProcsPtr->blt_Tags_AddItemToTag) /* 32 */
#endif
#ifndef Blt_Tags_ForgetTag
#define Blt_Tags_ForgetTag \
	(bltTclProcsPtr->blt_Tags_ForgetTag) /* 33 */
#endif
#ifndef Blt_Tags_RemoveItemFromTag
#define Blt_Tags_RemoveItemFromTag \
	(bltTclProcsPtr->blt_Tags_RemoveItemFromTag) /* 34 */
#endif
#ifndef Blt_Tags_ClearTagsFromItem
#define Blt_Tags_ClearTagsFromItem \
	(bltTclProcsPtr->blt_Tags_ClearTagsFromItem) /* 35 */
#endif
#ifndef Blt_Tags_AppendTagsToChain
#define Blt_Tags_AppendTagsToChain \
	(bltTclProcsPtr->blt_Tags_AppendTagsToChain) /* 36 */
#endif
#ifndef Blt_Tags_AppendTagsToObj
#define Blt_Tags_AppendTagsToObj \
	(bltTclProcsPtr->blt_Tags_AppendTagsToObj) /* 37 */
#endif
#ifndef Blt_Tags_AppendAllTagsToObj
#define Blt_Tags_AppendAllTagsToObj \
	(bltTclProcsPtr->blt_Tags_AppendAllTagsToObj) /* 38 */
#endif
#ifndef Blt_Tags_GetItemList
#define Blt_Tags_GetItemList \
	(bltTclProcsPtr->blt_Tags_GetItemList) /* 39 */
#endif
#ifndef Blt_Tags_GetTable
#define Blt_Tags_GetTable \
	(bltTclProcsPtr->blt_Tags_GetTable) /* 40 */
#endif
#ifndef Blt_List_Init
#define Blt_List_Init \
	(bltTclProcsPtr->blt_List_Init) /* 41 */
#endif
#ifndef Blt_List_Reset
#define Blt_List_Reset \
	(bltTclProcsPtr->blt_List_Reset) /* 42 */
#endif
#ifndef Blt_List_Create
#define Blt_List_Create \
	(bltTclProcsPtr->blt_List_Create) /* 43 */
#endif
#ifndef Blt_List_Destroy
#define Blt_List_Destroy \
	(bltTclProcsPtr->blt_List_Destroy) /* 44 */
#endif
#ifndef Blt_List_CreateNode
#define Blt_List_CreateNode \
	(bltTclProcsPtr->blt_List_CreateNode) /* 45 */
#endif
#ifndef Blt_List_DeleteNode
#define Blt_List_DeleteNode \
	(bltTclProcsPtr->blt_List_DeleteNode) /* 46 */
#endif
#ifndef Blt_List_Append
#define Blt_List_Append \
	(bltTclProcsPtr->blt_List_Append) /* 47 */
#endif
#ifndef Blt_List_Prepend
#define Blt_List_Prepend \
	(bltTclProcsPtr->blt_List_Prepend) /* 48 */
#endif
#ifndef Blt_List_LinkAfter
#define Blt_List_LinkAfter \
	(bltTclProcsPtr->blt_List_LinkAfter) /* 49 */
#endif
#ifndef Blt_List_LinkBefore
#define Blt_List_LinkBefore \
	(bltTclProcsPtr->blt_List_LinkBefore) /* 50 */
#endif
#ifndef Blt_List_UnlinkNode
#define Blt_List_UnlinkNode \
	(bltTclProcsPtr->blt_List_UnlinkNode) /* 51 */
#endif
#ifndef Blt_List_GetNode
#define Blt_List_GetNode \
	(bltTclProcsPtr->blt_List_GetNode) /* 52 */
#endif
#ifndef Blt_List_DeleteNodeByKey
#define Blt_List_DeleteNodeByKey \
	(bltTclProcsPtr->blt_List_DeleteNodeByKey) /* 53 */
#endif
#ifndef Blt_List_GetNthNode
#define Blt_List_GetNthNode \
	(bltTclProcsPtr->blt_List_GetNthNode) /* 54 */
#endif
#ifndef Blt_List_Sort
#define Blt_List_Sort \
	(bltTclProcsPtr->blt_List_Sort) /* 55 */
#endif
#ifndef Blt_Pool_Create
#define Blt_Pool_Create \
	(bltTclProcsPtr->blt_Pool_Create) /* 56 */
#endif
#ifndef Blt_Pool_Destroy
#define Blt_Pool_Destroy \
	(bltTclProcsPtr->blt_Pool_Destroy) /* 57 */
#endif
#ifndef Blt_Tree_GetKey
#define Blt_Tree_GetKey \
	(bltTclProcsPtr->blt_Tree_GetKey) /* 58 */
#endif
#ifndef Blt_Tree_GetKeyFromNode
#define Blt_Tree_GetKeyFromNode \
	(bltTclProcsPtr->blt_Tree_GetKeyFromNode) /* 59 */
#endif
#ifndef Blt_Tree_CreateNode
#define Blt_Tree_CreateNode \
	(bltTclProcsPtr->blt_Tree_CreateNode) /* 60 */
#endif
#ifndef Blt_Tree_CreateNodeWithId
#define Blt_Tree_CreateNodeWithId \
	(bltTclProcsPtr->blt_Tree_CreateNodeWithId) /* 61 */
#endif
#ifndef Blt_Tree_DeleteNode
#define Blt_Tree_DeleteNode \
	(bltTclProcsPtr->blt_Tree_DeleteNode) /* 62 */
#endif
#ifndef Blt_Tree_MoveNode
#define Blt_Tree_MoveNode \
	(bltTclProcsPtr->blt_Tree_MoveNode) /* 63 */
#endif
#ifndef Blt_Tree_GetNodeFromIndex
#define Blt_Tree_GetNodeFromIndex \
	(bltTclProcsPtr->blt_Tree_GetNodeFromIndex) /* 64 */
#endif
#ifndef Blt_Tree_FindChild
#define Blt_Tree_FindChild \
	(bltTclProcsPtr->blt_Tree_FindChild) /* 65 */
#endif
#ifndef Blt_Tree_NextNode
#define Blt_Tree_NextNode \
	(bltTclProcsPtr->blt_Tree_NextNode) /* 66 */
#endif
#ifndef Blt_Tree_PrevNode
#define Blt_Tree_PrevNode \
	(bltTclProcsPtr->blt_Tree_PrevNode) /* 67 */
#endif
#ifndef Blt_Tree_FirstChild
#define Blt_Tree_FirstChild \
	(bltTclProcsPtr->blt_Tree_FirstChild) /* 68 */
#endif
#ifndef Blt_Tree_LastChild
#define Blt_Tree_LastChild \
	(bltTclProcsPtr->blt_Tree_LastChild) /* 69 */
#endif
#ifndef Blt_Tree_IsBefore
#define Blt_Tree_IsBefore \
	(bltTclProcsPtr->blt_Tree_IsBefore) /* 70 */
#endif
#ifndef Blt_Tree_IsAncestor
#define Blt_Tree_IsAncestor \
	(bltTclProcsPtr->blt_Tree_IsAncestor) /* 71 */
#endif
#ifndef Blt_Tree_PrivateValue
#define Blt_Tree_PrivateValue \
	(bltTclProcsPtr->blt_Tree_PrivateValue) /* 72 */
#endif
#ifndef Blt_Tree_PublicValue
#define Blt_Tree_PublicValue \
	(bltTclProcsPtr->blt_Tree_PublicValue) /* 73 */
#endif
#ifndef Blt_Tree_GetValue
#define Blt_Tree_GetValue \
	(bltTclProcsPtr->blt_Tree_GetValue) /* 74 */
#endif
#ifndef Blt_Tree_ValueExists
#define Blt_Tree_ValueExists \
	(bltTclProcsPtr->blt_Tree_ValueExists) /* 75 */
#endif
#ifndef Blt_Tree_SetValue
#define Blt_Tree_SetValue \
	(bltTclProcsPtr->blt_Tree_SetValue) /* 76 */
#endif
#ifndef Blt_Tree_UnsetValue
#define Blt_Tree_UnsetValue \
	(bltTclProcsPtr->blt_Tree_UnsetValue) /* 77 */
#endif
#ifndef Blt_Tree_AppendValue
#define Blt_Tree_AppendValue \
	(bltTclProcsPtr->blt_Tree_AppendValue) /* 78 */
#endif
#ifndef Blt_Tree_ListAppendValue
#define Blt_Tree_ListAppendValue \
	(bltTclProcsPtr->blt_Tree_ListAppendValue) /* 79 */
#endif
#ifndef Blt_Tree_GetArrayValue
#define Blt_Tree_GetArrayValue \
	(bltTclProcsPtr->blt_Tree_GetArrayValue) /* 80 */
#endif
#ifndef Blt_Tree_SetArrayValue
#define Blt_Tree_SetArrayValue \
	(bltTclProcsPtr->blt_Tree_SetArrayValue) /* 81 */
#endif
#ifndef Blt_Tree_UnsetArrayValue
#define Blt_Tree_UnsetArrayValue \
	(bltTclProcsPtr->blt_Tree_UnsetArrayValue) /* 82 */
#endif
#ifndef Blt_Tree_AppendArrayValue
#define Blt_Tree_AppendArrayValue \
	(bltTclProcsPtr->blt_Tree_AppendArrayValue) /* 83 */
#endif
#ifndef Blt_Tree_ListAppendArrayValue
#define Blt_Tree_ListAppendArrayValue \
	(bltTclProcsPtr->blt_Tree_ListAppendArrayValue) /* 84 */
#endif
#ifndef Blt_Tree_ArrayValueExists
#define Blt_Tree_ArrayValueExists \
	(bltTclProcsPtr->blt_Tree_ArrayValueExists) /* 85 */
#endif
#ifndef Blt_Tree_ArrayNames
#define Blt_Tree_ArrayNames \
	(bltTclProcsPtr->blt_Tree_ArrayNames) /* 86 */
#endif
#ifndef Blt_Tree_GetValueByKey
#define Blt_Tree_GetValueByKey \
	(bltTclProcsPtr->blt_Tree_GetValueByKey) /* 87 */
#endif
#ifndef Blt_Tree_SetValueByKey
#define Blt_Tree_SetValueByKey \
	(bltTclProcsPtr->blt_Tree_SetValueByKey) /* 88 */
#endif
#ifndef Blt_Tree_UnsetValueByKey
#define Blt_Tree_UnsetValueByKey \
	(bltTclProcsPtr->blt_Tree_UnsetValueByKey) /* 89 */
#endif
#ifndef Blt_Tree_AppendValueByKey
#define Blt_Tree_AppendValueByKey \
	(bltTclProcsPtr->blt_Tree_AppendValueByKey) /* 90 */
#endif
#ifndef Blt_Tree_ListAppendValueByKey
#define Blt_Tree_ListAppendValueByKey \
	(bltTclProcsPtr->blt_Tree_ListAppendValueByKey) /* 91 */
#endif
#ifndef Blt_Tree_ValueExistsByKey
#define Blt_Tree_ValueExistsByKey \
	(bltTclProcsPtr->blt_Tree_ValueExistsByKey) /* 92 */
#endif
#ifndef Blt_Tree_FirstKey
#define Blt_Tree_FirstKey \
	(bltTclProcsPtr->blt_Tree_FirstKey) /* 93 */
#endif
#ifndef Blt_Tree_NextKey
#define Blt_Tree_NextKey \
	(bltTclProcsPtr->blt_Tree_NextKey) /* 94 */
#endif
#ifndef Blt_Tree_Apply
#define Blt_Tree_Apply \
	(bltTclProcsPtr->blt_Tree_Apply) /* 95 */
#endif
#ifndef Blt_Tree_ApplyDFS
#define Blt_Tree_ApplyDFS \
	(bltTclProcsPtr->blt_Tree_ApplyDFS) /* 96 */
#endif
#ifndef Blt_Tree_ApplyBFS
#define Blt_Tree_ApplyBFS \
	(bltTclProcsPtr->blt_Tree_ApplyBFS) /* 97 */
#endif
#ifndef Blt_Tree_SortNode
#define Blt_Tree_SortNode \
	(bltTclProcsPtr->blt_Tree_SortNode) /* 98 */
#endif
#ifndef Blt_Tree_Exists
#define Blt_Tree_Exists \
	(bltTclProcsPtr->blt_Tree_Exists) /* 99 */
#endif
#ifndef Blt_Tree_Open
#define Blt_Tree_Open \
	(bltTclProcsPtr->blt_Tree_Open) /* 100 */
#endif
#ifndef Blt_Tree_Close
#define Blt_Tree_Close \
	(bltTclProcsPtr->blt_Tree_Close) /* 101 */
#endif
#ifndef Blt_Tree_Attach
#define Blt_Tree_Attach \
	(bltTclProcsPtr->blt_Tree_Attach) /* 102 */
#endif
#ifndef Blt_Tree_GetFromObj
#define Blt_Tree_GetFromObj \
	(bltTclProcsPtr->blt_Tree_GetFromObj) /* 103 */
#endif
#ifndef Blt_Tree_Size
#define Blt_Tree_Size \
	(bltTclProcsPtr->blt_Tree_Size) /* 104 */
#endif
#ifndef Blt_Tree_CreateTrace
#define Blt_Tree_CreateTrace \
	(bltTclProcsPtr->blt_Tree_CreateTrace) /* 105 */
#endif
#ifndef Blt_Tree_DeleteTrace
#define Blt_Tree_DeleteTrace \
	(bltTclProcsPtr->blt_Tree_DeleteTrace) /* 106 */
#endif
#ifndef Blt_Tree_CreateEventHandler
#define Blt_Tree_CreateEventHandler \
	(bltTclProcsPtr->blt_Tree_CreateEventHandler) /* 107 */
#endif
#ifndef Blt_Tree_DeleteEventHandler
#define Blt_Tree_DeleteEventHandler \
	(bltTclProcsPtr->blt_Tree_DeleteEventHandler) /* 108 */
#endif
#ifndef Blt_Tree_RelabelNode
#define Blt_Tree_RelabelNode \
	(bltTclProcsPtr->blt_Tree_RelabelNode) /* 109 */
#endif
#ifndef Blt_Tree_RelabelNodeWithoutNotify
#define Blt_Tree_RelabelNodeWithoutNotify \
	(bltTclProcsPtr->blt_Tree_RelabelNodeWithoutNotify) /* 110 */
#endif
#ifndef Blt_Tree_NodeIdAscii
#define Blt_Tree_NodeIdAscii \
	(bltTclProcsPtr->blt_Tree_NodeIdAscii) /* 111 */
#endif
#ifndef Blt_Tree_NodePath
#define Blt_Tree_NodePath \
	(bltTclProcsPtr->blt_Tree_NodePath) /* 112 */
#endif
#ifndef Blt_Tree_NodeRelativePath
#define Blt_Tree_NodeRelativePath \
	(bltTclProcsPtr->blt_Tree_NodeRelativePath) /* 113 */
#endif
#ifndef Blt_Tree_NodePosition
#define Blt_Tree_NodePosition \
	(bltTclProcsPtr->blt_Tree_NodePosition) /* 114 */
#endif
#ifndef Blt_Tree_ClearTags
#define Blt_Tree_ClearTags \
	(bltTclProcsPtr->blt_Tree_ClearTags) /* 115 */
#endif
#ifndef Blt_Tree_HasTag
#define Blt_Tree_HasTag \
	(bltTclProcsPtr->blt_Tree_HasTag) /* 116 */
#endif
#ifndef Blt_Tree_AddTag
#define Blt_Tree_AddTag \
	(bltTclProcsPtr->blt_Tree_AddTag) /* 117 */
#endif
#ifndef Blt_Tree_RemoveTag
#define Blt_Tree_RemoveTag \
	(bltTclProcsPtr->blt_Tree_RemoveTag) /* 118 */
#endif
#ifndef Blt_Tree_ForgetTag
#define Blt_Tree_ForgetTag \
	(bltTclProcsPtr->blt_Tree_ForgetTag) /* 119 */
#endif
#ifndef Blt_Tree_TagHashTable
#define Blt_Tree_TagHashTable \
	(bltTclProcsPtr->blt_Tree_TagHashTable) /* 120 */
#endif
#ifndef Blt_Tree_TagTableIsShared
#define Blt_Tree_TagTableIsShared \
	(bltTclProcsPtr->blt_Tree_TagTableIsShared) /* 121 */
#endif
#ifndef Blt_Tree_NewTagTable
#define Blt_Tree_NewTagTable \
	(bltTclProcsPtr->blt_Tree_NewTagTable) /* 122 */
#endif
#ifndef Blt_Tree_FirstTag
#define Blt_Tree_FirstTag \
	(bltTclProcsPtr->blt_Tree_FirstTag) /* 123 */
#endif
#ifndef Blt_Tree_Depth
#define Blt_Tree_Depth \
	(bltTclProcsPtr->blt_Tree_Depth) /* 124 */
#endif
#ifndef Blt_Tree_RegisterFormat
#define Blt_Tree_RegisterFormat \
	(bltTclProcsPtr->blt_Tree_RegisterFormat) /* 125 */
#endif
#ifndef Blt_Tree_RememberTag
#define Blt_Tree_RememberTag \
	(bltTclProcsPtr->blt_Tree_RememberTag) /* 126 */
#endif
#ifndef Blt_Tree_GetNodeFromObj
#define Blt_Tree_GetNodeFromObj \
	(bltTclProcsPtr->blt_Tree_GetNodeFromObj) /* 127 */
#endif
#ifndef Blt_Tree_GetNodeIterator
#define Blt_Tree_GetNodeIterator \
	(bltTclProcsPtr->blt_Tree_GetNodeIterator) /* 128 */
#endif
#ifndef Blt_Tree_FirstTaggedNode
#define Blt_Tree_FirstTaggedNode \
	(bltTclProcsPtr->blt_Tree_FirstTaggedNode) /* 129 */
#endif
#ifndef Blt_Tree_NextTaggedNode
#define Blt_Tree_NextTaggedNode \
	(bltTclProcsPtr->blt_Tree_NextTaggedNode) /* 130 */
#endif
#ifndef blt_table_release_tags
#define blt_table_release_tags \
	(bltTclProcsPtr->blt_table_release_tags) /* 131 */
#endif
#ifndef blt_table_new_tags
#define blt_table_new_tags \
	(bltTclProcsPtr->blt_table_new_tags) /* 132 */
#endif
#ifndef blt_table_get_column_tag_table
#define blt_table_get_column_tag_table \
	(bltTclProcsPtr->blt_table_get_column_tag_table) /* 133 */
#endif
#ifndef blt_table_get_row_tag_table
#define blt_table_get_row_tag_table \
	(bltTclProcsPtr->blt_table_get_row_tag_table) /* 134 */
#endif
#ifndef blt_table_exists
#define blt_table_exists \
	(bltTclProcsPtr->blt_table_exists) /* 135 */
#endif
#ifndef blt_table_create
#define blt_table_create \
	(bltTclProcsPtr->blt_table_create) /* 136 */
#endif
#ifndef blt_table_open
#define blt_table_open \
	(bltTclProcsPtr->blt_table_open) /* 137 */
#endif
#ifndef blt_table_close
#define blt_table_close \
	(bltTclProcsPtr->blt_table_close) /* 138 */
#endif
#ifndef blt_table_clear
#define blt_table_clear \
	(bltTclProcsPtr->blt_table_clear) /* 139 */
#endif
#ifndef blt_table_pack
#define blt_table_pack \
	(bltTclProcsPtr->blt_table_pack) /* 140 */
#endif
#ifndef blt_table_same_object
#define blt_table_same_object \
	(bltTclProcsPtr->blt_table_same_object) /* 141 */
#endif
#ifndef blt_table_row_get_label_table
#define blt_table_row_get_label_table \
	(bltTclProcsPtr->blt_table_row_get_label_table) /* 142 */
#endif
#ifndef blt_table_column_get_label_table
#define blt_table_column_get_label_table \
	(bltTclProcsPtr->blt_table_column_get_label_table) /* 143 */
#endif
#ifndef blt_table_get_row
#define blt_table_get_row \
	(bltTclProcsPtr->blt_table_get_row) /* 144 */
#endif
#ifndef blt_table_get_column
#define blt_table_get_column \
	(bltTclProcsPtr->blt_table_get_column) /* 145 */
#endif
#ifndef blt_table_get_row_by_label
#define blt_table_get_row_by_label \
	(bltTclProcsPtr->blt_table_get_row_by_label) /* 146 */
#endif
#ifndef blt_table_get_column_by_label
#define blt_table_get_column_by_label \
	(bltTclProcsPtr->blt_table_get_column_by_label) /* 147 */
#endif
#ifndef blt_table_get_row_by_index
#define blt_table_get_row_by_index \
	(bltTclProcsPtr->blt_table_get_row_by_index) /* 148 */
#endif
#ifndef blt_table_get_column_by_index
#define blt_table_get_column_by_index \
	(bltTclProcsPtr->blt_table_get_column_by_index) /* 149 */
#endif
#ifndef blt_table_set_row_label
#define blt_table_set_row_label \
	(bltTclProcsPtr->blt_table_set_row_label) /* 150 */
#endif
#ifndef blt_table_set_column_label
#define blt_table_set_column_label \
	(bltTclProcsPtr->blt_table_set_column_label) /* 151 */
#endif
#ifndef blt_table_name_to_column_type
#define blt_table_name_to_column_type \
	(bltTclProcsPtr->blt_table_name_to_column_type) /* 152 */
#endif
#ifndef blt_table_set_column_type
#define blt_table_set_column_type \
	(bltTclProcsPtr->blt_table_set_column_type) /* 153 */
#endif
#ifndef blt_table_column_type_to_name
#define blt_table_column_type_to_name \
	(bltTclProcsPtr->blt_table_column_type_to_name) /* 154 */
#endif
#ifndef blt_table_set_column_tag
#define blt_table_set_column_tag \
	(bltTclProcsPtr->blt_table_set_column_tag) /* 155 */
#endif
#ifndef blt_table_set_row_tag
#define blt_table_set_row_tag \
	(bltTclProcsPtr->blt_table_set_row_tag) /* 156 */
#endif
#ifndef blt_table_create_row
#define blt_table_create_row \
	(bltTclProcsPtr->blt_table_create_row) /* 157 */
#endif
#ifndef blt_table_create_column
#define blt_table_create_column \
	(bltTclProcsPtr->blt_table_create_column) /* 158 */
#endif
#ifndef blt_table_extend_rows
#define blt_table_extend_rows \
	(bltTclProcsPtr->blt_table_extend_rows) /* 159 */
#endif
#ifndef blt_table_extend_columns
#define blt_table_extend_columns \
	(bltTclProcsPtr->blt_table_extend_columns) /* 160 */
#endif
#ifndef blt_table_delete_row
#define blt_table_delete_row \
	(bltTclProcsPtr->blt_table_delete_row) /* 161 */
#endif
#ifndef blt_table_delete_column
#define blt_table_delete_column \
	(bltTclProcsPtr->blt_table_delete_column) /* 162 */
#endif
#ifndef blt_table_move_rows
#define blt_table_move_rows \
	(bltTclProcsPtr->blt_table_move_rows) /* 163 */
#endif
#ifndef blt_table_move_columns
#define blt_table_move_columns \
	(bltTclProcsPtr->blt_table_move_columns) /* 164 */
#endif
#ifndef blt_table_get_obj
#define blt_table_get_obj \
	(bltTclProcsPtr->blt_table_get_obj) /* 165 */
#endif
#ifndef blt_table_set_obj
#define blt_table_set_obj \
	(bltTclProcsPtr->blt_table_set_obj) /* 166 */
#endif
#ifndef blt_table_get_string
#define blt_table_get_string \
	(bltTclProcsPtr->blt_table_get_string) /* 167 */
#endif
#ifndef blt_table_set_string_rep
#define blt_table_set_string_rep \
	(bltTclProcsPtr->blt_table_set_string_rep) /* 168 */
#endif
#ifndef blt_table_set_string
#define blt_table_set_string \
	(bltTclProcsPtr->blt_table_set_string) /* 169 */
#endif
#ifndef blt_table_append_string
#define blt_table_append_string \
	(bltTclProcsPtr->blt_table_append_string) /* 170 */
#endif
#ifndef blt_table_set_bytes
#define blt_table_set_bytes \
	(bltTclProcsPtr->blt_table_set_bytes) /* 171 */
#endif
#ifndef blt_table_get_double
#define blt_table_get_double \
	(bltTclProcsPtr->blt_table_get_double) /* 172 */
#endif
#ifndef blt_table_set_double
#define blt_table_set_double \
	(bltTclProcsPtr->blt_table_set_double) /* 173 */
#endif
#ifndef blt_table_get_long
#define blt_table_get_long \
	(bltTclProcsPtr->blt_table_get_long) /* 174 */
#endif
#ifndef blt_table_set_long
#define blt_table_set_long \
	(bltTclProcsPtr->blt_table_set_long) /* 175 */
#endif
#ifndef blt_table_get_boolean
#define blt_table_get_boolean \
	(bltTclProcsPtr->blt_table_get_boolean) /* 176 */
#endif
#ifndef blt_table_set_boolean
#define blt_table_set_boolean \
	(bltTclProcsPtr->blt_table_set_boolean) /* 177 */
#endif
#ifndef blt_table_get_value
#define blt_table_get_value \
	(bltTclProcsPtr->blt_table_get_value) /* 178 */
#endif
#ifndef blt_table_set_value
#define blt_table_set_value \
	(bltTclProcsPtr->blt_table_set_value) /* 179 */
#endif
#ifndef blt_table_unset_value
#define blt_table_unset_value \
	(bltTclProcsPtr->blt_table_unset_value) /* 180 */
#endif
#ifndef blt_table_value_exists
#define blt_table_value_exists \
	(bltTclProcsPtr->blt_table_value_exists) /* 181 */
#endif
#ifndef blt_table_value_string
#define blt_table_value_string \
	(bltTclProcsPtr->blt_table_value_string) /* 182 */
#endif
#ifndef blt_table_value_bytes
#define blt_table_value_bytes \
	(bltTclProcsPtr->blt_table_value_bytes) /* 183 */
#endif
#ifndef blt_table_value_length
#define blt_table_value_length \
	(bltTclProcsPtr->blt_table_value_length) /* 184 */
#endif
#ifndef blt_table_tags_are_shared
#define blt_table_tags_are_shared \
	(bltTclProcsPtr->blt_table_tags_are_shared) /* 185 */
#endif
#ifndef blt_table_clear_row_tags
#define blt_table_clear_row_tags \
	(bltTclProcsPtr->blt_table_clear_row_tags) /* 186 */
#endif
#ifndef blt_table_clear_column_tags
#define blt_table_clear_column_tags \
	(bltTclProcsPtr->blt_table_clear_column_tags) /* 187 */
#endif
#ifndef blt_table_get_row_tags
#define blt_table_get_row_tags \
	(bltTclProcsPtr->blt_table_get_row_tags) /* 188 */
#endif
#ifndef blt_table_get_column_tags
#define blt_table_get_column_tags \
	(bltTclProcsPtr->blt_table_get_column_tags) /* 189 */
#endif
#ifndef blt_table_get_tagged_rows
#define blt_table_get_tagged_rows \
	(bltTclProcsPtr->blt_table_get_tagged_rows) /* 190 */
#endif
#ifndef blt_table_get_tagged_columns
#define blt_table_get_tagged_columns \
	(bltTclProcsPtr->blt_table_get_tagged_columns) /* 191 */
#endif
#ifndef blt_table_row_has_tag
#define blt_table_row_has_tag \
	(bltTclProcsPtr->blt_table_row_has_tag) /* 192 */
#endif
#ifndef blt_table_column_has_tag
#define blt_table_column_has_tag \
	(bltTclProcsPtr->blt_table_column_has_tag) /* 193 */
#endif
#ifndef blt_table_forget_row_tag
#define blt_table_forget_row_tag \
	(bltTclProcsPtr->blt_table_forget_row_tag) /* 194 */
#endif
#ifndef blt_table_forget_column_tag
#define blt_table_forget_column_tag \
	(bltTclProcsPtr->blt_table_forget_column_tag) /* 195 */
#endif
#ifndef blt_table_unset_row_tag
#define blt_table_unset_row_tag \
	(bltTclProcsPtr->blt_table_unset_row_tag) /* 196 */
#endif
#ifndef blt_table_unset_column_tag
#define blt_table_unset_column_tag \
	(bltTclProcsPtr->blt_table_unset_column_tag) /* 197 */
#endif
#ifndef blt_table_first_column
#define blt_table_first_column \
	(bltTclProcsPtr->blt_table_first_column) /* 198 */
#endif
#ifndef blt_table_last_column
#define blt_table_last_column \
	(bltTclProcsPtr->blt_table_last_column) /* 199 */
#endif
#ifndef blt_table_next_column
#define blt_table_next_column \
	(bltTclProcsPtr->blt_table_next_column) /* 200 */
#endif
#ifndef blt_table_previous_column
#define blt_table_previous_column \
	(bltTclProcsPtr->blt_table_previous_column) /* 201 */
#endif
#ifndef blt_table_first_row
#define blt_table_first_row \
	(bltTclProcsPtr->blt_table_first_row) /* 202 */
#endif
#ifndef blt_table_last_row
#define blt_table_last_row \
	(bltTclProcsPtr->blt_table_last_row) /* 203 */
#endif
#ifndef blt_table_next_row
#define blt_table_next_row \
	(bltTclProcsPtr->blt_table_next_row) /* 204 */
#endif
#ifndef blt_table_previous_row
#define blt_table_previous_row \
	(bltTclProcsPtr->blt_table_previous_row) /* 205 */
#endif
#ifndef blt_table_row_spec
#define blt_table_row_spec \
	(bltTclProcsPtr->blt_table_row_spec) /* 206 */
#endif
#ifndef blt_table_column_spec
#define blt_table_column_spec \
	(bltTclProcsPtr->blt_table_column_spec) /* 207 */
#endif
#ifndef blt_table_iterate_rows
#define blt_table_iterate_rows \
	(bltTclProcsPtr->blt_table_iterate_rows) /* 208 */
#endif
#ifndef blt_table_iterate_columns
#define blt_table_iterate_columns \
	(bltTclProcsPtr->blt_table_iterate_columns) /* 209 */
#endif
#ifndef blt_table_iterate_rows_objv
#define blt_table_iterate_rows_objv \
	(bltTclProcsPtr->blt_table_iterate_rows_objv) /* 210 */
#endif
#ifndef blt_table_iterate_columns_objv
#define blt_table_iterate_columns_objv \
	(bltTclProcsPtr->blt_table_iterate_columns_objv) /* 211 */
#endif
#ifndef blt_table_free_iterator_objv
#define blt_table_free_iterator_objv \
	(bltTclProcsPtr->blt_table_free_iterator_objv) /* 212 */
#endif
#ifndef blt_table_iterate_all_rows
#define blt_table_iterate_all_rows \
	(bltTclProcsPtr->blt_table_iterate_all_rows) /* 213 */
#endif
#ifndef blt_table_iterate_all_columns
#define blt_table_iterate_all_columns \
	(bltTclProcsPtr->blt_table_iterate_all_columns) /* 214 */
#endif
#ifndef blt_table_first_tagged_row
#define blt_table_first_tagged_row \
	(bltTclProcsPtr->blt_table_first_tagged_row) /* 215 */
#endif
#ifndef blt_table_first_tagged_column
#define blt_table_first_tagged_column \
	(bltTclProcsPtr->blt_table_first_tagged_column) /* 216 */
#endif
#ifndef blt_table_next_tagged_row
#define blt_table_next_tagged_row \
	(bltTclProcsPtr->blt_table_next_tagged_row) /* 217 */
#endif
#ifndef blt_table_next_tagged_column
#define blt_table_next_tagged_column \
	(bltTclProcsPtr->blt_table_next_tagged_column) /* 218 */
#endif
#ifndef blt_table_list_rows
#define blt_table_list_rows \
	(bltTclProcsPtr->blt_table_list_rows) /* 219 */
#endif
#ifndef blt_table_list_columns
#define blt_table_list_columns \
	(bltTclProcsPtr->blt_table_list_columns) /* 220 */
#endif
#ifndef blt_table_clear_row_traces
#define blt_table_clear_row_traces \
	(bltTclProcsPtr->blt_table_clear_row_traces) /* 221 */
#endif
#ifndef blt_table_clear_column_traces
#define blt_table_clear_column_traces \
	(bltTclProcsPtr->blt_table_clear_column_traces) /* 222 */
#endif
#ifndef blt_table_create_trace
#define blt_table_create_trace \
	(bltTclProcsPtr->blt_table_create_trace) /* 223 */
#endif
#ifndef blt_table_trace_column
#define blt_table_trace_column \
	(bltTclProcsPtr->blt_table_trace_column) /* 224 */
#endif
#ifndef blt_table_trace_row
#define blt_table_trace_row \
	(bltTclProcsPtr->blt_table_trace_row) /* 225 */
#endif
#ifndef blt_table_create_column_trace
#define blt_table_create_column_trace \
	(bltTclProcsPtr->blt_table_create_column_trace) /* 226 */
#endif
#ifndef blt_table_create_column_tag_trace
#define blt_table_create_column_tag_trace \
	(bltTclProcsPtr->blt_table_create_column_tag_trace) /* 227 */
#endif
#ifndef blt_table_create_row_trace
#define blt_table_create_row_trace \
	(bltTclProcsPtr->blt_table_create_row_trace) /* 228 */
#endif
#ifndef blt_table_create_row_tag_trace
#define blt_table_create_row_tag_trace \
	(bltTclProcsPtr->blt_table_create_row_tag_trace) /* 229 */
#endif
#ifndef blt_table_delete_trace
#define blt_table_delete_trace \
	(bltTclProcsPtr->blt_table_delete_trace) /* 230 */
#endif
#ifndef blt_table_create_notifier
#define blt_table_create_notifier \
	(bltTclProcsPtr->blt_table_create_notifier) /* 231 */
#endif
#ifndef blt_table_create_row_notifier
#define blt_table_create_row_notifier \
	(bltTclProcsPtr->blt_table_create_row_notifier) /* 232 */
#endif
#ifndef blt_table_create_row_tag_notifier
#define blt_table_create_row_tag_notifier \
	(bltTclProcsPtr->blt_table_create_row_tag_notifier) /* 233 */
#endif
#ifndef blt_table_create_column_notifier
#define blt_table_create_column_notifier \
	(bltTclProcsPtr->blt_table_create_column_notifier) /* 234 */
#endif
#ifndef blt_table_create_column_tag_notifier
#define blt_table_create_column_tag_notifier \
	(bltTclProcsPtr->blt_table_create_column_tag_notifier) /* 235 */
#endif
#ifndef blt_table_delete_notifier
#define blt_table_delete_notifier \
	(bltTclProcsPtr->blt_table_delete_notifier) /* 236 */
#endif
#ifndef blt_table_sort_init
#define blt_table_sort_init \
	(bltTclProcsPtr->blt_table_sort_init) /* 237 */
#endif
#ifndef blt_table_sort_rows
#define blt_table_sort_rows \
	(bltTclProcsPtr->blt_table_sort_rows) /* 238 */
#endif
#ifndef blt_table_sort_rows_subset
#define blt_table_sort_rows_subset \
	(bltTclProcsPtr->blt_table_sort_rows_subset) /* 239 */
#endif
#ifndef blt_table_sort_finish
#define blt_table_sort_finish \
	(bltTclProcsPtr->blt_table_sort_finish) /* 240 */
#endif
#ifndef blt_table_get_compare_proc
#define blt_table_get_compare_proc \
	(bltTclProcsPtr->blt_table_get_compare_proc) /* 241 */
#endif
#ifndef blt_table_get_row_map
#define blt_table_get_row_map \
	(bltTclProcsPtr->blt_table_get_row_map) /* 242 */
#endif
#ifndef blt_table_get_column_map
#define blt_table_get_column_map \
	(bltTclProcsPtr->blt_table_get_column_map) /* 243 */
#endif
#ifndef blt_table_set_row_map
#define blt_table_set_row_map \
	(bltTclProcsPtr->blt_table_set_row_map) /* 244 */
#endif
#ifndef blt_table_set_column_map
#define blt_table_set_column_map \
	(bltTclProcsPtr->blt_table_set_column_map) /* 245 */
#endif
#ifndef blt_table_restore
#define blt_table_restore \
	(bltTclProcsPtr->blt_table_restore) /* 246 */
#endif
#ifndef blt_table_file_restore
#define blt_table_file_restore \
	(bltTclProcsPtr->blt_table_file_restore) /* 247 */
#endif
#ifndef blt_table_register_format
#define blt_table_register_format \
	(bltTclProcsPtr->blt_table_register_format) /* 248 */
#endif
#ifndef blt_table_unset_keys
#define blt_table_unset_keys \
	(bltTclProcsPtr->blt_table_unset_keys) /* 249 */
#endif
#ifndef blt_table_get_keys
#define blt_table_get_keys \
	(bltTclProcsPtr->blt_table_get_keys) /* 250 */
#endif
#ifndef blt_table_set_keys
#define blt_table_set_keys \
	(bltTclProcsPtr->blt_table_set_keys) /* 251 */
#endif
#ifndef blt_table_key_lookup
#define blt_table_key_lookup \
	(bltTclProcsPtr->blt_table_key_lookup) /* 252 */
#endif
#ifndef blt_table_get_column_limits
#define blt_table_get_column_limits \
	(bltTclProcsPtr->blt_table_get_column_limits) /* 253 */
#endif
#ifndef blt_table_row
#define blt_table_row \
	(bltTclProcsPtr->blt_table_row) /* 254 */
#endif
#ifndef blt_table_column
#define blt_table_column \
	(bltTclProcsPtr->blt_table_column) /* 255 */
#endif
#ifndef blt_table_row_index
#define blt_table_row_index \
	(bltTclProcsPtr->blt_table_row_index) /* 256 */
#endif
#ifndef blt_table_column_index
#define blt_table_column_index \
	(bltTclProcsPtr->blt_table_column_index) /* 257 */
#endif
#ifndef Blt_VecMin
#define Blt_VecMin \
	(bltTclProcsPtr->blt_VecMin) /* 258 */
#endif
#ifndef Blt_VecMax
#define Blt_VecMax \
	(bltTclProcsPtr->blt_VecMax) /* 259 */
#endif
#ifndef Blt_AllocVectorId
#define Blt_AllocVectorId \
	(bltTclProcsPtr->blt_AllocVectorId) /* 260 */
#endif
#ifndef Blt_SetVectorChangedProc
#define Blt_SetVectorChangedProc \
	(bltTclProcsPtr->blt_SetVectorChangedProc) /* 261 */
#endif
#ifndef Blt_FreeVectorId
#define Blt_FreeVectorId \
	(bltTclProcsPtr->blt_FreeVectorId) /* 262 */
#endif
#ifndef Blt_GetVectorById
#define Blt_GetVectorById \
	(bltTclProcsPtr->blt_GetVectorById) /* 263 */
#endif
#ifndef Blt_NameOfVectorId
#define Blt_NameOfVectorId \
	(bltTclProcsPtr->blt_NameOfVectorId) /* 264 */
#endif
#ifndef Blt_NameOfVector
#define Blt_NameOfVector \
	(bltTclProcsPtr->blt_NameOfVector) /* 265 */
#endif
#ifndef Blt_VectorNotifyPending
#define Blt_VectorNotifyPending \
	(bltTclProcsPtr->blt_VectorNotifyPending) /* 266 */
#endif
#ifndef Blt_CreateVector
#define Blt_CreateVector \
	(bltTclProcsPtr->blt_CreateVector) /* 267 */
#endif
#ifndef Blt_CreateVector2
#define Blt_CreateVector2 \
	(bltTclProcsPtr->blt_CreateVector2) /* 268 */
#endif
#ifndef Blt_GetVector
#define Blt_GetVector \
	(bltTclProcsPtr->blt_GetVector) /* 269 */
#endif
#ifndef Blt_GetVectorFromObj
#define Blt_GetVectorFromObj \
	(bltTclProcsPtr->blt_GetVectorFromObj) /* 270 */
#endif
#ifndef Blt_VectorExists
#define Blt_VectorExists \
	(bltTclProcsPtr->blt_VectorExists) /* 271 */
#endif
#ifndef Blt_ResetVector
#define Blt_ResetVector \
	(bltTclProcsPtr->blt_ResetVector) /* 272 */
#endif
#ifndef Blt_ResizeVector
#define Blt_ResizeVector \
	(bltTclProcsPtr->blt_ResizeVector) /* 273 */
#endif
#ifndef Blt_DeleteVectorByName
#define Blt_DeleteVectorByName \
	(bltTclProcsPtr->blt_DeleteVectorByName) /* 274 */
#endif
#ifndef Blt_DeleteVector
#define Blt_DeleteVector \
	(bltTclProcsPtr->blt_DeleteVector) /* 275 */
#endif
#ifndef Blt_ExprVector
#define Blt_ExprVector \
	(bltTclProcsPtr->blt_ExprVector) /* 276 */
#endif
#ifndef Blt_InstallIndexProc
#define Blt_InstallIndexProc \
	(bltTclProcsPtr->blt_InstallIndexProc) /* 277 */
#endif
#ifndef Blt_VectorExists2
#define Blt_VectorExists2 \
	(bltTclProcsPtr->blt_VectorExists2) /* 278 */
#endif

#endif /* defined(USE_BLT_STUBS) && !defined(BUILD_BLT_TCL_PROCS) */

/* !END!: Do not edit above this line. */
