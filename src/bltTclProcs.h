#include "bltAlloc.h"
#include "bltChain.h"
#include "bltList.h"
#include "bltDataTable.h"
#include "bltPool.h"
#include "bltTree.h"
#include "bltHash.h.in"
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
#ifndef Blt_Chain_IsBefore_DECLARED
#define Blt_Chain_IsBefore_DECLARED
/* 17 */
BLT_EXTERN int		Blt_Chain_IsBefore(Blt_ChainLink first,
				Blt_ChainLink last);
#endif
#ifndef Blt_List_Init_DECLARED
#define Blt_List_Init_DECLARED
/* 18 */
BLT_EXTERN void		Blt_List_Init(Blt_List list, size_t type);
#endif
#ifndef Blt_List_Reset_DECLARED
#define Blt_List_Reset_DECLARED
/* 19 */
BLT_EXTERN void		Blt_List_Reset(Blt_List list);
#endif
#ifndef Blt_List_Create_DECLARED
#define Blt_List_Create_DECLARED
/* 20 */
BLT_EXTERN Blt_List	Blt_List_Create(size_t type);
#endif
#ifndef Blt_List_Destroy_DECLARED
#define Blt_List_Destroy_DECLARED
/* 21 */
BLT_EXTERN void		Blt_List_Destroy(Blt_List list);
#endif
#ifndef Blt_List_CreateNode_DECLARED
#define Blt_List_CreateNode_DECLARED
/* 22 */
BLT_EXTERN Blt_ListNode	 Blt_List_CreateNode(Blt_List list, const char *key);
#endif
#ifndef Blt_List_DeleteNode_DECLARED
#define Blt_List_DeleteNode_DECLARED
/* 23 */
BLT_EXTERN void		Blt_List_DeleteNode(Blt_ListNode node);
#endif
#ifndef Blt_List_Append_DECLARED
#define Blt_List_Append_DECLARED
/* 24 */
BLT_EXTERN Blt_ListNode	 Blt_List_Append(Blt_List list, const char *key,
				ClientData clientData);
#endif
#ifndef Blt_List_Prepend_DECLARED
#define Blt_List_Prepend_DECLARED
/* 25 */
BLT_EXTERN Blt_ListNode	 Blt_List_Prepend(Blt_List list, const char *key,
				ClientData clientData);
#endif
#ifndef Blt_List_LinkAfter_DECLARED
#define Blt_List_LinkAfter_DECLARED
/* 26 */
BLT_EXTERN void		Blt_List_LinkAfter(Blt_List list, Blt_ListNode node,
				Blt_ListNode afterNode);
#endif
#ifndef Blt_List_LinkBefore_DECLARED
#define Blt_List_LinkBefore_DECLARED
/* 27 */
BLT_EXTERN void		Blt_List_LinkBefore(Blt_List list, Blt_ListNode node,
				Blt_ListNode beforeNode);
#endif
#ifndef Blt_List_UnlinkNode_DECLARED
#define Blt_List_UnlinkNode_DECLARED
/* 28 */
BLT_EXTERN void		Blt_List_UnlinkNode(Blt_ListNode node);
#endif
#ifndef Blt_List_GetNode_DECLARED
#define Blt_List_GetNode_DECLARED
/* 29 */
BLT_EXTERN Blt_ListNode	 Blt_List_GetNode(Blt_List list, const char *key);
#endif
#ifndef Blt_List_DeleteNodeByKey_DECLARED
#define Blt_List_DeleteNodeByKey_DECLARED
/* 30 */
BLT_EXTERN void		Blt_List_DeleteNodeByKey(Blt_List list,
				const char *key);
#endif
#ifndef Blt_List_GetNthNode_DECLARED
#define Blt_List_GetNthNode_DECLARED
/* 31 */
BLT_EXTERN Blt_ListNode	 Blt_List_GetNthNode(Blt_List list, long position,
				int direction);
#endif
#ifndef Blt_List_Sort_DECLARED
#define Blt_List_Sort_DECLARED
/* 32 */
BLT_EXTERN void		Blt_List_Sort(Blt_List list,
				Blt_ListCompareProc *proc);
#endif
#ifndef blt_table_release_tags_DECLARED
#define blt_table_release_tags_DECLARED
/* 33 */
BLT_EXTERN void		blt_table_release_tags(BLT_TABLE table);
#endif
#ifndef blt_table_exists_DECLARED
#define blt_table_exists_DECLARED
/* 34 */
BLT_EXTERN int		blt_table_exists(Tcl_Interp *interp,
				const char *name);
#endif
#ifndef blt_table_create_DECLARED
#define blt_table_create_DECLARED
/* 35 */
BLT_EXTERN int		blt_table_create(Tcl_Interp *interp,
				const char *name, BLT_TABLE *tablePtr);
#endif
#ifndef blt_table_open_DECLARED
#define blt_table_open_DECLARED
/* 36 */
BLT_EXTERN int		blt_table_open(Tcl_Interp *interp, const char *name,
				BLT_TABLE *tablePtr);
#endif
#ifndef blt_table_close_DECLARED
#define blt_table_close_DECLARED
/* 37 */
BLT_EXTERN void		blt_table_close(BLT_TABLE table);
#endif
#ifndef blt_table_same_object_DECLARED
#define blt_table_same_object_DECLARED
/* 38 */
BLT_EXTERN int		blt_table_same_object(BLT_TABLE table1,
				BLT_TABLE table2);
#endif
#ifndef blt_table_row_get_label_table_DECLARED
#define blt_table_row_get_label_table_DECLARED
/* 39 */
BLT_EXTERN Blt_HashTable * blt_table_row_get_label_table(BLT_TABLE table,
				const char *label);
#endif
#ifndef blt_table_column_get_label_table_DECLARED
#define blt_table_column_get_label_table_DECLARED
/* 40 */
BLT_EXTERN Blt_HashTable * blt_table_column_get_label_table(BLT_TABLE table,
				const char *label);
#endif
#ifndef blt_table_get_row_by_label_DECLARED
#define blt_table_get_row_by_label_DECLARED
/* 41 */
BLT_EXTERN BLT_TABLE_ROW blt_table_get_row_by_label(BLT_TABLE table,
				const char *label);
#endif
#ifndef blt_table_get_column_by_label_DECLARED
#define blt_table_get_column_by_label_DECLARED
/* 42 */
BLT_EXTERN BLT_TABLE_COLUMN blt_table_get_column_by_label(BLT_TABLE table,
				const char *label);
#endif
#ifndef blt_table_get_row_by_index_DECLARED
#define blt_table_get_row_by_index_DECLARED
/* 43 */
BLT_EXTERN BLT_TABLE_ROW blt_table_get_row_by_index(BLT_TABLE table,
				long index);
#endif
#ifndef blt_table_get_column_by_index_DECLARED
#define blt_table_get_column_by_index_DECLARED
/* 44 */
BLT_EXTERN BLT_TABLE_COLUMN blt_table_get_column_by_index(BLT_TABLE table,
				long index);
#endif
#ifndef blt_table_set_row_label_DECLARED
#define blt_table_set_row_label_DECLARED
/* 45 */
BLT_EXTERN int		blt_table_set_row_label(Tcl_Interp *interp,
				BLT_TABLE table, BLT_TABLE_ROW row,
				const char *label);
#endif
#ifndef blt_table_set_column_label_DECLARED
#define blt_table_set_column_label_DECLARED
/* 46 */
BLT_EXTERN int		blt_table_set_column_label(Tcl_Interp *interp,
				BLT_TABLE table, BLT_TABLE_COLUMN column,
				const char *label);
#endif
#ifndef blt_table_name_to_column_type_DECLARED
#define blt_table_name_to_column_type_DECLARED
/* 47 */
BLT_EXTERN BLT_TABLE_COLUMN_TYPE blt_table_name_to_column_type(
				const char *typeName);
#endif
#ifndef blt_table_set_column_type_DECLARED
#define blt_table_set_column_type_DECLARED
/* 48 */
BLT_EXTERN int		blt_table_set_column_type(BLT_TABLE table,
				BLT_TABLE_COLUMN column,
				BLT_TABLE_COLUMN_TYPE type);
#endif
#ifndef blt_table_column_type_to_name_DECLARED
#define blt_table_column_type_to_name_DECLARED
/* 49 */
BLT_EXTERN const char *	 blt_table_column_type_to_name(BLT_TABLE_COLUMN_TYPE type);
#endif
#ifndef blt_table_set_column_tag_DECLARED
#define blt_table_set_column_tag_DECLARED
/* 50 */
BLT_EXTERN int		blt_table_set_column_tag(Tcl_Interp *interp,
				BLT_TABLE table, BLT_TABLE_COLUMN column,
				const char *tagName);
#endif
#ifndef blt_table_set_row_tag_DECLARED
#define blt_table_set_row_tag_DECLARED
/* 51 */
BLT_EXTERN int		blt_table_set_row_tag(Tcl_Interp *interp,
				BLT_TABLE table, BLT_TABLE_ROW row,
				const char *tagName);
#endif
#ifndef blt_table_create_row_DECLARED
#define blt_table_create_row_DECLARED
/* 52 */
BLT_EXTERN BLT_TABLE_ROW blt_table_create_row(Tcl_Interp *interp,
				BLT_TABLE table, const char *label);
#endif
#ifndef blt_table_create_column_DECLARED
#define blt_table_create_column_DECLARED
/* 53 */
BLT_EXTERN BLT_TABLE_COLUMN blt_table_create_column(Tcl_Interp *interp,
				BLT_TABLE table, const char *label);
#endif
#ifndef blt_table_extend_rows_DECLARED
#define blt_table_extend_rows_DECLARED
/* 54 */
BLT_EXTERN int		blt_table_extend_rows(Tcl_Interp *interp,
				BLT_TABLE table, size_t n,
				BLT_TABLE_ROW *rows);
#endif
#ifndef blt_table_extend_columns_DECLARED
#define blt_table_extend_columns_DECLARED
/* 55 */
BLT_EXTERN int		blt_table_extend_columns(Tcl_Interp *interp,
				BLT_TABLE table, size_t n,
				BLT_TABLE_COLUMN *columms);
#endif
#ifndef blt_table_delete_row_DECLARED
#define blt_table_delete_row_DECLARED
/* 56 */
BLT_EXTERN int		blt_table_delete_row(BLT_TABLE table,
				BLT_TABLE_ROW row);
#endif
#ifndef blt_table_delete_column_DECLARED
#define blt_table_delete_column_DECLARED
/* 57 */
BLT_EXTERN int		blt_table_delete_column(BLT_TABLE table,
				BLT_TABLE_COLUMN column);
#endif
#ifndef blt_table_move_row_DECLARED
#define blt_table_move_row_DECLARED
/* 58 */
BLT_EXTERN int		blt_table_move_row(Tcl_Interp *interp,
				BLT_TABLE table, BLT_TABLE_ROW from,
				BLT_TABLE_ROW to, size_t n);
#endif
#ifndef blt_table_move_column_DECLARED
#define blt_table_move_column_DECLARED
/* 59 */
BLT_EXTERN int		blt_table_move_column(Tcl_Interp *interp,
				BLT_TABLE table, BLT_TABLE_COLUMN from,
				BLT_TABLE_COLUMN to, size_t n);
#endif
#ifndef blt_table_get_obj_DECLARED
#define blt_table_get_obj_DECLARED
/* 60 */
BLT_EXTERN Tcl_Obj *	blt_table_get_obj(BLT_TABLE table, BLT_TABLE_ROW row,
				BLT_TABLE_COLUMN column);
#endif
#ifndef blt_table_set_obj_DECLARED
#define blt_table_set_obj_DECLARED
/* 61 */
BLT_EXTERN int		blt_table_set_obj(BLT_TABLE table, BLT_TABLE_ROW row,
				BLT_TABLE_COLUMN column, Tcl_Obj *objPtr);
#endif
#ifndef blt_table_get_string_DECLARED
#define blt_table_get_string_DECLARED
/* 62 */
BLT_EXTERN const char *	 blt_table_get_string(BLT_TABLE table,
				BLT_TABLE_ROW row, BLT_TABLE_COLUMN column);
#endif
#ifndef blt_table_set_string_DECLARED
#define blt_table_set_string_DECLARED
/* 63 */
BLT_EXTERN int		blt_table_set_string(BLT_TABLE table,
				BLT_TABLE_ROW row, BLT_TABLE_COLUMN column,
				const char *string, int length);
#endif
#ifndef blt_table_append_string_DECLARED
#define blt_table_append_string_DECLARED
/* 64 */
BLT_EXTERN int		blt_table_append_string(Tcl_Interp *interp,
				BLT_TABLE table, BLT_TABLE_ROW row,
				BLT_TABLE_COLUMN column, const char *string,
				int length);
#endif
#ifndef blt_table_get_double_DECLARED
#define blt_table_get_double_DECLARED
/* 65 */
BLT_EXTERN double	blt_table_get_double(BLT_TABLE table,
				BLT_TABLE_ROW row, BLT_TABLE_COLUMN column);
#endif
#ifndef blt_table_set_double_DECLARED
#define blt_table_set_double_DECLARED
/* 66 */
BLT_EXTERN int		blt_table_set_double(BLT_TABLE table,
				BLT_TABLE_ROW row, BLT_TABLE_COLUMN column,
				double value);
#endif
#ifndef blt_table_get_long_DECLARED
#define blt_table_get_long_DECLARED
/* 67 */
BLT_EXTERN long		blt_table_get_long(BLT_TABLE table,
				BLT_TABLE_ROW row, BLT_TABLE_COLUMN column,
				long defValue);
#endif
#ifndef blt_table_set_long_DECLARED
#define blt_table_set_long_DECLARED
/* 68 */
BLT_EXTERN int		blt_table_set_long(BLT_TABLE table,
				BLT_TABLE_ROW row, BLT_TABLE_COLUMN column,
				long value);
#endif
#ifndef blt_table_get_value_DECLARED
#define blt_table_get_value_DECLARED
/* 69 */
BLT_EXTERN BLT_TABLE_VALUE blt_table_get_value(BLT_TABLE table,
				BLT_TABLE_ROW row, BLT_TABLE_COLUMN column);
#endif
#ifndef blt_table_set_value_DECLARED
#define blt_table_set_value_DECLARED
/* 70 */
BLT_EXTERN int		blt_table_set_value(BLT_TABLE table,
				BLT_TABLE_ROW row, BLT_TABLE_COLUMN column,
				BLT_TABLE_VALUE value);
#endif
#ifndef blt_table_unset_value_DECLARED
#define blt_table_unset_value_DECLARED
/* 71 */
BLT_EXTERN int		blt_table_unset_value(BLT_TABLE table,
				BLT_TABLE_ROW row, BLT_TABLE_COLUMN column);
#endif
#ifndef blt_table_value_exists_DECLARED
#define blt_table_value_exists_DECLARED
/* 72 */
BLT_EXTERN int		blt_table_value_exists(BLT_TABLE table,
				BLT_TABLE_ROW row, BLT_TABLE_COLUMN column);
#endif
#ifndef blt_table_get_row_tag_table_DECLARED
#define blt_table_get_row_tag_table_DECLARED
/* 73 */
BLT_EXTERN Blt_HashTable * blt_table_get_row_tag_table(BLT_TABLE table,
				const char *tagName);
#endif
#ifndef blt_table_get_column_tag_table_DECLARED
#define blt_table_get_column_tag_table_DECLARED
/* 74 */
BLT_EXTERN Blt_HashTable * blt_table_get_column_tag_table(BLT_TABLE table,
				const char *tagName);
#endif
#ifndef blt_table_row_tags_DECLARED
#define blt_table_row_tags_DECLARED
/* 75 */
BLT_EXTERN Blt_Chain	blt_table_row_tags(BLT_TABLE table,
				BLT_TABLE_ROW row);
#endif
#ifndef blt_table_column_tags_DECLARED
#define blt_table_column_tags_DECLARED
/* 76 */
BLT_EXTERN Blt_Chain	blt_table_column_tags(BLT_TABLE table,
				BLT_TABLE_COLUMN column);
#endif
#ifndef blt_table_tags_are_shared_DECLARED
#define blt_table_tags_are_shared_DECLARED
/* 77 */
BLT_EXTERN int		blt_table_tags_are_shared(BLT_TABLE table);
#endif
#ifndef blt_table_row_has_tag_DECLARED
#define blt_table_row_has_tag_DECLARED
/* 78 */
BLT_EXTERN int		blt_table_row_has_tag(BLT_TABLE table,
				BLT_TABLE_ROW row, const char *tagName);
#endif
#ifndef blt_table_column_has_tag_DECLARED
#define blt_table_column_has_tag_DECLARED
/* 79 */
BLT_EXTERN int		blt_table_column_has_tag(BLT_TABLE table,
				BLT_TABLE_COLUMN column, const char *tagName);
#endif
#ifndef blt_table_forget_row_tag_DECLARED
#define blt_table_forget_row_tag_DECLARED
/* 80 */
BLT_EXTERN int		blt_table_forget_row_tag(Tcl_Interp *interp,
				BLT_TABLE table, const char *tagName);
#endif
#ifndef blt_table_forget_column_tag_DECLARED
#define blt_table_forget_column_tag_DECLARED
/* 81 */
BLT_EXTERN int		blt_table_forget_column_tag(Tcl_Interp *interp,
				BLT_TABLE table, const char *tagName);
#endif
#ifndef blt_table_unset_row_tag_DECLARED
#define blt_table_unset_row_tag_DECLARED
/* 82 */
BLT_EXTERN int		blt_table_unset_row_tag(Tcl_Interp *interp,
				BLT_TABLE table, BLT_TABLE_ROW row,
				const char *tagName);
#endif
#ifndef blt_table_unset_column_tag_DECLARED
#define blt_table_unset_column_tag_DECLARED
/* 83 */
BLT_EXTERN int		blt_table_unset_column_tag(Tcl_Interp *interp,
				BLT_TABLE table, BLT_TABLE_COLUMN column,
				const char *tagName);
#endif
#ifndef blt_table_first_row_tag_DECLARED
#define blt_table_first_row_tag_DECLARED
/* 84 */
BLT_EXTERN Blt_HashEntry * blt_table_first_row_tag(BLT_TABLE table,
				Blt_HashSearch *cursorPtr);
#endif
#ifndef blt_table_first_column_tag_DECLARED
#define blt_table_first_column_tag_DECLARED
/* 85 */
BLT_EXTERN Blt_HashEntry * blt_table_first_column_tag(BLT_TABLE table,
				Blt_HashSearch *cursorPtr);
#endif
#ifndef blt_table_first_column_DECLARED
#define blt_table_first_column_DECLARED
/* 86 */
BLT_EXTERN BLT_TABLE_COLUMN blt_table_first_column(BLT_TABLE table);
#endif
#ifndef blt_table_next_column_DECLARED
#define blt_table_next_column_DECLARED
/* 87 */
BLT_EXTERN BLT_TABLE_COLUMN blt_table_next_column(BLT_TABLE table,
				BLT_TABLE_COLUMN column);
#endif
#ifndef blt_table_first_row_DECLARED
#define blt_table_first_row_DECLARED
/* 88 */
BLT_EXTERN BLT_TABLE_ROW blt_table_first_row(BLT_TABLE table);
#endif
#ifndef blt_table_next_row_DECLARED
#define blt_table_next_row_DECLARED
/* 89 */
BLT_EXTERN BLT_TABLE_ROW blt_table_next_row(BLT_TABLE table,
				BLT_TABLE_ROW row);
#endif
#ifndef blt_table_row_spec_DECLARED
#define blt_table_row_spec_DECLARED
/* 90 */
BLT_EXTERN BLT_TABLE_ROWCOLUMN_SPEC blt_table_row_spec(BLT_TABLE table,
				Tcl_Obj *objPtr, const char **sp);
#endif
#ifndef blt_table_column_spec_DECLARED
#define blt_table_column_spec_DECLARED
/* 91 */
BLT_EXTERN BLT_TABLE_ROWCOLUMN_SPEC blt_table_column_spec(BLT_TABLE table,
				Tcl_Obj *objPtr, const char **sp);
#endif
#ifndef blt_table_iterate_row_DECLARED
#define blt_table_iterate_row_DECLARED
/* 92 */
BLT_EXTERN int		blt_table_iterate_row(Tcl_Interp *interp,
				BLT_TABLE table, Tcl_Obj *objPtr,
				BLT_TABLE_ITERATOR *iter);
#endif
#ifndef blt_table_iterate_column_DECLARED
#define blt_table_iterate_column_DECLARED
/* 93 */
BLT_EXTERN int		blt_table_iterate_column(Tcl_Interp *interp,
				BLT_TABLE table, Tcl_Obj *objPtr,
				BLT_TABLE_ITERATOR *iter);
#endif
#ifndef blt_table_iterate_row_objv_DECLARED
#define blt_table_iterate_row_objv_DECLARED
/* 94 */
BLT_EXTERN int		blt_table_iterate_row_objv(Tcl_Interp *interp,
				BLT_TABLE table, int objc,
				Tcl_Obj *const *objv,
				BLT_TABLE_ITERATOR *iterPtr);
#endif
#ifndef blt_table_iterate_column_objv_DECLARED
#define blt_table_iterate_column_objv_DECLARED
/* 95 */
BLT_EXTERN int		blt_table_iterate_column_objv(Tcl_Interp *interp,
				BLT_TABLE table, int objc,
				Tcl_Obj *const *objv,
				BLT_TABLE_ITERATOR *iterPtr);
#endif
#ifndef blt_table_free_iterator_objv_DECLARED
#define blt_table_free_iterator_objv_DECLARED
/* 96 */
BLT_EXTERN void		blt_table_free_iterator_objv(
				BLT_TABLE_ITERATOR *iterPtr);
#endif
#ifndef blt_table_iterate_all_rows_DECLARED
#define blt_table_iterate_all_rows_DECLARED
/* 97 */
BLT_EXTERN void		blt_table_iterate_all_rows(BLT_TABLE table,
				BLT_TABLE_ITERATOR *iterPtr);
#endif
#ifndef blt_table_iterate_all_columns_DECLARED
#define blt_table_iterate_all_columns_DECLARED
/* 98 */
BLT_EXTERN void		blt_table_iterate_all_columns(BLT_TABLE table,
				BLT_TABLE_ITERATOR *iterPtr);
#endif
#ifndef blt_table_first_tagged_row_DECLARED
#define blt_table_first_tagged_row_DECLARED
/* 99 */
BLT_EXTERN BLT_TABLE_ROW blt_table_first_tagged_row(BLT_TABLE_ITERATOR *iter);
#endif
#ifndef blt_table_first_tagged_column_DECLARED
#define blt_table_first_tagged_column_DECLARED
/* 100 */
BLT_EXTERN BLT_TABLE_COLUMN blt_table_first_tagged_column(
				BLT_TABLE_ITERATOR *iter);
#endif
#ifndef blt_table_next_tagged_row_DECLARED
#define blt_table_next_tagged_row_DECLARED
/* 101 */
BLT_EXTERN BLT_TABLE_ROW blt_table_next_tagged_row(BLT_TABLE_ITERATOR *iter);
#endif
#ifndef blt_table_next_tagged_column_DECLARED
#define blt_table_next_tagged_column_DECLARED
/* 102 */
BLT_EXTERN BLT_TABLE_COLUMN blt_table_next_tagged_column(
				BLT_TABLE_ITERATOR *iter);
#endif
#ifndef blt_table_get_row_DECLARED
#define blt_table_get_row_DECLARED
/* 103 */
BLT_EXTERN BLT_TABLE_ROW blt_table_get_row(Tcl_Interp *interp,
				BLT_TABLE table, Tcl_Obj *objPtr);
#endif
#ifndef blt_table_get_column_DECLARED
#define blt_table_get_column_DECLARED
/* 104 */
BLT_EXTERN BLT_TABLE_COLUMN blt_table_get_column(Tcl_Interp *interp,
				BLT_TABLE table, Tcl_Obj *objPtr);
#endif
#ifndef blt_table_list_rows_DECLARED
#define blt_table_list_rows_DECLARED
/* 105 */
BLT_EXTERN int		blt_table_list_rows(Tcl_Interp *interp,
				BLT_TABLE table, int objc,
				Tcl_Obj *const *objv, Blt_Chain chain);
#endif
#ifndef blt_table_list_columns_DECLARED
#define blt_table_list_columns_DECLARED
/* 106 */
BLT_EXTERN int		blt_table_list_columns(Tcl_Interp *interp,
				BLT_TABLE table, int objc,
				Tcl_Obj *const *objv, Blt_Chain chain);
#endif
#ifndef blt_table_clear_row_tags_DECLARED
#define blt_table_clear_row_tags_DECLARED
/* 107 */
BLT_EXTERN void		blt_table_clear_row_tags(BLT_TABLE table,
				BLT_TABLE_ROW row);
#endif
#ifndef blt_table_clear_column_tags_DECLARED
#define blt_table_clear_column_tags_DECLARED
/* 108 */
BLT_EXTERN void		blt_table_clear_column_tags(BLT_TABLE table,
				BLT_TABLE_COLUMN column);
#endif
#ifndef blt_table_clear_row_traces_DECLARED
#define blt_table_clear_row_traces_DECLARED
/* 109 */
BLT_EXTERN void		blt_table_clear_row_traces(BLT_TABLE table,
				BLT_TABLE_ROW row);
#endif
#ifndef blt_table_clear_column_traces_DECLARED
#define blt_table_clear_column_traces_DECLARED
/* 110 */
BLT_EXTERN void		blt_table_clear_column_traces(BLT_TABLE table,
				BLT_TABLE_COLUMN column);
#endif
#ifndef blt_table_create_trace_DECLARED
#define blt_table_create_trace_DECLARED
/* 111 */
BLT_EXTERN BLT_TABLE_TRACE blt_table_create_trace(BLT_TABLE table,
				BLT_TABLE_ROW row, BLT_TABLE_COLUMN column,
				const char *rowTag, const char *columnTag,
				unsigned int mask,
				BLT_TABLE_TRACE_PROC *proc,
				BLT_TABLE_TRACE_DELETE_PROC *deleteProc,
				ClientData clientData);
#endif
#ifndef blt_table_set_column_trace_DECLARED
#define blt_table_set_column_trace_DECLARED
/* 112 */
BLT_EXTERN BLT_TABLE_TRACE blt_table_set_column_trace(BLT_TABLE table,
				BLT_TABLE_COLUMN column, unsigned int mask,
				BLT_TABLE_TRACE_PROC *proc,
				BLT_TABLE_TRACE_DELETE_PROC *deleteProc,
				ClientData clientData);
#endif
#ifndef blt_table_create_column_tag_trace_DECLARED
#define blt_table_create_column_tag_trace_DECLARED
/* 113 */
BLT_EXTERN BLT_TABLE_TRACE blt_table_create_column_tag_trace(BLT_TABLE table,
				const char *tag, unsigned int mask,
				BLT_TABLE_TRACE_PROC *proc,
				BLT_TABLE_TRACE_DELETE_PROC *deleteProc,
				ClientData clientData);
#endif
#ifndef blt_table_create_row_trace_DECLARED
#define blt_table_create_row_trace_DECLARED
/* 114 */
BLT_EXTERN BLT_TABLE_TRACE blt_table_create_row_trace(BLT_TABLE table,
				BLT_TABLE_ROW row, unsigned int mask,
				BLT_TABLE_TRACE_PROC *proc,
				BLT_TABLE_TRACE_DELETE_PROC *deleteProc,
				ClientData clientData);
#endif
#ifndef blt_table_create_row_tag_trace_DECLARED
#define blt_table_create_row_tag_trace_DECLARED
/* 115 */
BLT_EXTERN BLT_TABLE_TRACE blt_table_create_row_tag_trace(BLT_TABLE table,
				const char *tag, unsigned int mask,
				BLT_TABLE_TRACE_PROC *proc,
				BLT_TABLE_TRACE_DELETE_PROC *deleteProc,
				ClientData clientData);
#endif
#ifndef blt_table_delete_trace_DECLARED
#define blt_table_delete_trace_DECLARED
/* 116 */
BLT_EXTERN void		blt_table_delete_trace(BLT_TABLE_TRACE trace);
#endif
#ifndef blt_table_create_row_notifier_DECLARED
#define blt_table_create_row_notifier_DECLARED
/* 117 */
BLT_EXTERN BLT_TABLE_NOTIFIER blt_table_create_row_notifier(
				Tcl_Interp *interp, BLT_TABLE table,
				BLT_TABLE_ROW row, unsigned int mask,
				BLT_TABLE_NOTIFY_EVENT_PROC *proc,
				BLT_TABLE_NOTIFIER_DELETE_PROC *deleteProc,
				ClientData clientData);
#endif
#ifndef blt_table_create_row_tag_notifier_DECLARED
#define blt_table_create_row_tag_notifier_DECLARED
/* 118 */
BLT_EXTERN BLT_TABLE_NOTIFIER blt_table_create_row_tag_notifier(
				Tcl_Interp *interp, BLT_TABLE table,
				const char *tag, unsigned int mask,
				BLT_TABLE_NOTIFY_EVENT_PROC *proc,
				BLT_TABLE_NOTIFIER_DELETE_PROC *deleteProc,
				ClientData clientData);
#endif
#ifndef blt_table_create_column_notifier_DECLARED
#define blt_table_create_column_notifier_DECLARED
/* 119 */
BLT_EXTERN BLT_TABLE_NOTIFIER blt_table_create_column_notifier(
				Tcl_Interp *interp, BLT_TABLE table,
				BLT_TABLE_COLUMN column, unsigned int mask,
				BLT_TABLE_NOTIFY_EVENT_PROC *proc,
				BLT_TABLE_NOTIFIER_DELETE_PROC *deleteProc,
				ClientData clientData);
#endif
#ifndef blt_table_create_column_tag_notifier_DECLARED
#define blt_table_create_column_tag_notifier_DECLARED
/* 120 */
BLT_EXTERN BLT_TABLE_NOTIFIER blt_table_create_column_tag_notifier(
				Tcl_Interp *interp, BLT_TABLE table,
				const char *tag, unsigned int mask,
				BLT_TABLE_NOTIFY_EVENT_PROC *proc,
				BLT_TABLE_NOTIFIER_DELETE_PROC *deleteProc,
				ClientData clientData);
#endif
#ifndef blt_table_delete_notifier_DECLARED
#define blt_table_delete_notifier_DECLARED
/* 121 */
BLT_EXTERN void		blt_table_delete_notifier(
				BLT_TABLE_NOTIFIER notifier);
#endif
#ifndef blt_table_sort_init_DECLARED
#define blt_table_sort_init_DECLARED
/* 122 */
BLT_EXTERN void		blt_table_sort_init(BLT_TABLE table,
				BLT_TABLE_SORT_ORDER *order,
				size_t numCompares, unsigned int flags);
#endif
#ifndef blt_table_sort_rows_DECLARED
#define blt_table_sort_rows_DECLARED
/* 123 */
BLT_EXTERN BLT_TABLE_ROW * blt_table_sort_rows(BLT_TABLE table);
#endif
#ifndef blt_table_sort_rows_subset_DECLARED
#define blt_table_sort_rows_subset_DECLARED
/* 124 */
BLT_EXTERN void		blt_table_sort_rows_subset(BLT_TABLE table,
				long numRows, BLT_TABLE_ROW *rows);
#endif
#ifndef blt_table_sort_finish_DECLARED
#define blt_table_sort_finish_DECLARED
/* 125 */
BLT_EXTERN void		blt_table_sort_finish(void );
#endif
#ifndef blt_table_get_compare_proc_DECLARED
#define blt_table_get_compare_proc_DECLARED
/* 126 */
BLT_EXTERN BLT_TABLE_COMPARE_PROC * blt_table_get_compare_proc(
				BLT_TABLE table, BLT_TABLE_COLUMN column,
				unsigned int flags);
#endif
#ifndef blt_table_get_row_map_DECLARED
#define blt_table_get_row_map_DECLARED
/* 127 */
BLT_EXTERN BLT_TABLE_ROW * blt_table_get_row_map(BLT_TABLE table);
#endif
#ifndef blt_table_get_column_map_DECLARED
#define blt_table_get_column_map_DECLARED
/* 128 */
BLT_EXTERN BLT_TABLE_COLUMN * blt_table_get_column_map(BLT_TABLE table);
#endif
#ifndef blt_table_set_row_map_DECLARED
#define blt_table_set_row_map_DECLARED
/* 129 */
BLT_EXTERN void		blt_table_set_row_map(BLT_TABLE table,
				BLT_TABLE_ROW *map);
#endif
#ifndef blt_table_set_column_map_DECLARED
#define blt_table_set_column_map_DECLARED
/* 130 */
BLT_EXTERN void		blt_table_set_column_map(BLT_TABLE table,
				BLT_TABLE_COLUMN *map);
#endif
#ifndef blt_table_restore_DECLARED
#define blt_table_restore_DECLARED
/* 131 */
BLT_EXTERN int		blt_table_restore(Tcl_Interp *interp,
				BLT_TABLE table, char *string,
				unsigned int flags);
#endif
#ifndef blt_table_file_restore_DECLARED
#define blt_table_file_restore_DECLARED
/* 132 */
BLT_EXTERN int		blt_table_file_restore(Tcl_Interp *interp,
				BLT_TABLE table, const char *fileName,
				unsigned int flags);
#endif
#ifndef blt_table_register_format_DECLARED
#define blt_table_register_format_DECLARED
/* 133 */
BLT_EXTERN int		blt_table_register_format(Tcl_Interp *interp,
				const char *name,
				BLT_TABLE_IMPORT_PROC *importProc,
				BLT_TABLE_EXPORT_PROC *exportProc);
#endif
#ifndef blt_table_unset_keys_DECLARED
#define blt_table_unset_keys_DECLARED
/* 134 */
BLT_EXTERN void		blt_table_unset_keys(BLT_TABLE table);
#endif
#ifndef blt_table_get_keys_DECLARED
#define blt_table_get_keys_DECLARED
/* 135 */
BLT_EXTERN Blt_Chain	blt_table_get_keys(BLT_TABLE table);
#endif
#ifndef blt_table_set_keys_DECLARED
#define blt_table_set_keys_DECLARED
/* 136 */
BLT_EXTERN int		blt_table_set_keys(BLT_TABLE table, Blt_Chain keys,
				int unique);
#endif
#ifndef blt_table_key_lookup_DECLARED
#define blt_table_key_lookup_DECLARED
/* 137 */
BLT_EXTERN int		blt_table_key_lookup(Tcl_Interp *interp,
				BLT_TABLE table, int objc,
				Tcl_Obj *const *objv, BLT_TABLE_ROW *rowPtr);
#endif
#ifndef blt_table_get_column_limits_DECLARED
#define blt_table_get_column_limits_DECLARED
/* 138 */
BLT_EXTERN int		blt_table_get_column_limits(Tcl_Interp *interp,
				BLT_TABLE table, BLT_TABLE_COLUMN col,
				Tcl_Obj **minObjPtrPtr,
				Tcl_Obj **maxObjPtrPtr);
#endif
#ifndef Blt_Pool_Create_DECLARED
#define Blt_Pool_Create_DECLARED
/* 139 */
BLT_EXTERN Blt_Pool	Blt_Pool_Create(int type);
#endif
#ifndef Blt_Pool_Destroy_DECLARED
#define Blt_Pool_Destroy_DECLARED
/* 140 */
BLT_EXTERN void		Blt_Pool_Destroy(Blt_Pool pool);
#endif
#ifndef Blt_Tree_GetKey_DECLARED
#define Blt_Tree_GetKey_DECLARED
/* 141 */
BLT_EXTERN Blt_TreeKey	Blt_Tree_GetKey(Blt_Tree tree, const char *string);
#endif
#ifndef Blt_Tree_GetKeyFromNode_DECLARED
#define Blt_Tree_GetKeyFromNode_DECLARED
/* 142 */
BLT_EXTERN Blt_TreeKey	Blt_Tree_GetKeyFromNode(Blt_TreeNode node,
				const char *string);
#endif
#ifndef Blt_Tree_GetKeyFromInterp_DECLARED
#define Blt_Tree_GetKeyFromInterp_DECLARED
/* 143 */
BLT_EXTERN Blt_TreeKey	Blt_Tree_GetKeyFromInterp(Tcl_Interp *interp,
				const char *string);
#endif
#ifndef Blt_Tree_CreateNode_DECLARED
#define Blt_Tree_CreateNode_DECLARED
/* 144 */
BLT_EXTERN Blt_TreeNode	 Blt_Tree_CreateNode(Blt_Tree tree,
				Blt_TreeNode parent, const char *name,
				long position);
#endif
#ifndef Blt_Tree_CreateNodeWithId_DECLARED
#define Blt_Tree_CreateNodeWithId_DECLARED
/* 145 */
BLT_EXTERN Blt_TreeNode	 Blt_Tree_CreateNodeWithId(Blt_Tree tree,
				Blt_TreeNode parent, const char *name,
				long inode, long position);
#endif
#ifndef Blt_Tree_DeleteNode_DECLARED
#define Blt_Tree_DeleteNode_DECLARED
/* 146 */
BLT_EXTERN int		Blt_Tree_DeleteNode(Blt_Tree tree, Blt_TreeNode node);
#endif
#ifndef Blt_Tree_MoveNode_DECLARED
#define Blt_Tree_MoveNode_DECLARED
/* 147 */
BLT_EXTERN int		Blt_Tree_MoveNode(Blt_Tree tree, Blt_TreeNode node,
				Blt_TreeNode parent, Blt_TreeNode before);
#endif
#ifndef Blt_Tree_GetNode_DECLARED
#define Blt_Tree_GetNode_DECLARED
/* 148 */
BLT_EXTERN Blt_TreeNode	 Blt_Tree_GetNode(Blt_Tree tree, long inode);
#endif
#ifndef Blt_Tree_FindChild_DECLARED
#define Blt_Tree_FindChild_DECLARED
/* 149 */
BLT_EXTERN Blt_TreeNode	 Blt_Tree_FindChild(Blt_TreeNode parent,
				const char *name);
#endif
#ifndef Blt_Tree_NextNode_DECLARED
#define Blt_Tree_NextNode_DECLARED
/* 150 */
BLT_EXTERN Blt_TreeNode	 Blt_Tree_NextNode(Blt_TreeNode root,
				Blt_TreeNode node);
#endif
#ifndef Blt_Tree_PrevNode_DECLARED
#define Blt_Tree_PrevNode_DECLARED
/* 151 */
BLT_EXTERN Blt_TreeNode	 Blt_Tree_PrevNode(Blt_TreeNode root,
				Blt_TreeNode node);
#endif
#ifndef Blt_Tree_FirstChild_DECLARED
#define Blt_Tree_FirstChild_DECLARED
/* 152 */
BLT_EXTERN Blt_TreeNode	 Blt_Tree_FirstChild(Blt_TreeNode parent);
#endif
#ifndef Blt_Tree_LastChild_DECLARED
#define Blt_Tree_LastChild_DECLARED
/* 153 */
BLT_EXTERN Blt_TreeNode	 Blt_Tree_LastChild(Blt_TreeNode parent);
#endif
#ifndef Blt_Tree_IsBefore_DECLARED
#define Blt_Tree_IsBefore_DECLARED
/* 154 */
BLT_EXTERN int		Blt_Tree_IsBefore(Blt_TreeNode node1,
				Blt_TreeNode node2);
#endif
#ifndef Blt_Tree_IsAncestor_DECLARED
#define Blt_Tree_IsAncestor_DECLARED
/* 155 */
BLT_EXTERN int		Blt_Tree_IsAncestor(Blt_TreeNode node1,
				Blt_TreeNode node2);
#endif
#ifndef Blt_Tree_PrivateValue_DECLARED
#define Blt_Tree_PrivateValue_DECLARED
/* 156 */
BLT_EXTERN int		Blt_Tree_PrivateValue(Tcl_Interp *interp,
				Blt_Tree tree, Blt_TreeNode node,
				Blt_TreeKey key);
#endif
#ifndef Blt_Tree_PublicValue_DECLARED
#define Blt_Tree_PublicValue_DECLARED
/* 157 */
BLT_EXTERN int		Blt_Tree_PublicValue(Tcl_Interp *interp,
				Blt_Tree tree, Blt_TreeNode node,
				Blt_TreeKey key);
#endif
#ifndef Blt_Tree_GetValue_DECLARED
#define Blt_Tree_GetValue_DECLARED
/* 158 */
BLT_EXTERN int		Blt_Tree_GetValue(Tcl_Interp *interp, Blt_Tree tree,
				Blt_TreeNode node, const char *string,
				Tcl_Obj **valuePtr);
#endif
#ifndef Blt_Tree_ValueExists_DECLARED
#define Blt_Tree_ValueExists_DECLARED
/* 159 */
BLT_EXTERN int		Blt_Tree_ValueExists(Blt_Tree tree,
				Blt_TreeNode node, const char *string);
#endif
#ifndef Blt_Tree_SetValue_DECLARED
#define Blt_Tree_SetValue_DECLARED
/* 160 */
BLT_EXTERN int		Blt_Tree_SetValue(Tcl_Interp *interp, Blt_Tree tree,
				Blt_TreeNode node, const char *string,
				Tcl_Obj *valuePtr);
#endif
#ifndef Blt_Tree_UnsetValue_DECLARED
#define Blt_Tree_UnsetValue_DECLARED
/* 161 */
BLT_EXTERN int		Blt_Tree_UnsetValue(Tcl_Interp *interp,
				Blt_Tree tree, Blt_TreeNode node,
				const char *string);
#endif
#ifndef Blt_Tree_AppendValue_DECLARED
#define Blt_Tree_AppendValue_DECLARED
/* 162 */
BLT_EXTERN int		Blt_Tree_AppendValue(Tcl_Interp *interp,
				Blt_Tree tree, Blt_TreeNode node,
				const char *string, const char *value);
#endif
#ifndef Blt_Tree_ListAppendValue_DECLARED
#define Blt_Tree_ListAppendValue_DECLARED
/* 163 */
BLT_EXTERN int		Blt_Tree_ListAppendValue(Tcl_Interp *interp,
				Blt_Tree tree, Blt_TreeNode node,
				const char *string, Tcl_Obj *valuePtr);
#endif
#ifndef Blt_Tree_GetArrayValue_DECLARED
#define Blt_Tree_GetArrayValue_DECLARED
/* 164 */
BLT_EXTERN int		Blt_Tree_GetArrayValue(Tcl_Interp *interp,
				Blt_Tree tree, Blt_TreeNode node,
				const char *arrayName, const char *elemName,
				Tcl_Obj **valueObjPtrPtr);
#endif
#ifndef Blt_Tree_SetArrayValue_DECLARED
#define Blt_Tree_SetArrayValue_DECLARED
/* 165 */
BLT_EXTERN int		Blt_Tree_SetArrayValue(Tcl_Interp *interp,
				Blt_Tree tree, Blt_TreeNode node,
				const char *arrayName, const char *elemName,
				Tcl_Obj *valueObjPtr);
#endif
#ifndef Blt_Tree_UnsetArrayValue_DECLARED
#define Blt_Tree_UnsetArrayValue_DECLARED
/* 166 */
BLT_EXTERN int		Blt_Tree_UnsetArrayValue(Tcl_Interp *interp,
				Blt_Tree tree, Blt_TreeNode node,
				const char *arrayName, const char *elemName);
#endif
#ifndef Blt_Tree_AppendArrayValue_DECLARED
#define Blt_Tree_AppendArrayValue_DECLARED
/* 167 */
BLT_EXTERN int		Blt_Tree_AppendArrayValue(Tcl_Interp *interp,
				Blt_Tree tree, Blt_TreeNode node,
				const char *arrayName, const char *elemName,
				const char *value);
#endif
#ifndef Blt_Tree_ListAppendArrayValue_DECLARED
#define Blt_Tree_ListAppendArrayValue_DECLARED
/* 168 */
BLT_EXTERN int		Blt_Tree_ListAppendArrayValue(Tcl_Interp *interp,
				Blt_Tree tree, Blt_TreeNode node,
				const char *arrayName, const char *elemName,
				Tcl_Obj *valueObjPtr);
#endif
#ifndef Blt_Tree_ArrayValueExists_DECLARED
#define Blt_Tree_ArrayValueExists_DECLARED
/* 169 */
BLT_EXTERN int		Blt_Tree_ArrayValueExists(Blt_Tree tree,
				Blt_TreeNode node, const char *arrayName,
				const char *elemName);
#endif
#ifndef Blt_Tree_ArrayNames_DECLARED
#define Blt_Tree_ArrayNames_DECLARED
/* 170 */
BLT_EXTERN int		Blt_Tree_ArrayNames(Tcl_Interp *interp,
				Blt_Tree tree, Blt_TreeNode node,
				const char *arrayName, Tcl_Obj *listObjPtr);
#endif
#ifndef Blt_Tree_GetValueByKey_DECLARED
#define Blt_Tree_GetValueByKey_DECLARED
/* 171 */
BLT_EXTERN int		Blt_Tree_GetValueByKey(Tcl_Interp *interp,
				Blt_Tree tree, Blt_TreeNode node,
				Blt_TreeKey key, Tcl_Obj **valuePtr);
#endif
#ifndef Blt_Tree_SetValueByKey_DECLARED
#define Blt_Tree_SetValueByKey_DECLARED
/* 172 */
BLT_EXTERN int		Blt_Tree_SetValueByKey(Tcl_Interp *interp,
				Blt_Tree tree, Blt_TreeNode node,
				Blt_TreeKey key, Tcl_Obj *valuePtr);
#endif
#ifndef Blt_Tree_UnsetValueByKey_DECLARED
#define Blt_Tree_UnsetValueByKey_DECLARED
/* 173 */
BLT_EXTERN int		Blt_Tree_UnsetValueByKey(Tcl_Interp *interp,
				Blt_Tree tree, Blt_TreeNode node,
				Blt_TreeKey key);
#endif
#ifndef Blt_Tree_AppendValueByKey_DECLARED
#define Blt_Tree_AppendValueByKey_DECLARED
/* 174 */
BLT_EXTERN int		Blt_Tree_AppendValueByKey(Tcl_Interp *interp,
				Blt_Tree tree, Blt_TreeNode node,
				Blt_TreeKey key, const char *value);
#endif
#ifndef Blt_Tree_ListAppendValueByKey_DECLARED
#define Blt_Tree_ListAppendValueByKey_DECLARED
/* 175 */
BLT_EXTERN int		Blt_Tree_ListAppendValueByKey(Tcl_Interp *interp,
				Blt_Tree tree, Blt_TreeNode node,
				Blt_TreeKey key, Tcl_Obj *valuePtr);
#endif
#ifndef Blt_Tree_ValueExistsByKey_DECLARED
#define Blt_Tree_ValueExistsByKey_DECLARED
/* 176 */
BLT_EXTERN int		Blt_Tree_ValueExistsByKey(Blt_Tree tree,
				Blt_TreeNode node, Blt_TreeKey key);
#endif
#ifndef Blt_Tree_FirstKey_DECLARED
#define Blt_Tree_FirstKey_DECLARED
/* 177 */
BLT_EXTERN Blt_TreeKey	Blt_Tree_FirstKey(Blt_Tree tree, Blt_TreeNode node,
				Blt_TreeKeyIterator *iterPtr);
#endif
#ifndef Blt_Tree_NextKey_DECLARED
#define Blt_Tree_NextKey_DECLARED
/* 178 */
BLT_EXTERN Blt_TreeKey	Blt_Tree_NextKey(Blt_Tree tree,
				Blt_TreeKeyIterator *iterPtr);
#endif
#ifndef Blt_Tree_Apply_DECLARED
#define Blt_Tree_Apply_DECLARED
/* 179 */
BLT_EXTERN int		Blt_Tree_Apply(Blt_TreeNode root,
				Blt_TreeApplyProc *proc,
				ClientData clientData);
#endif
#ifndef Blt_Tree_ApplyDFS_DECLARED
#define Blt_Tree_ApplyDFS_DECLARED
/* 180 */
BLT_EXTERN int		Blt_Tree_ApplyDFS(Blt_TreeNode root,
				Blt_TreeApplyProc *proc,
				ClientData clientData, int order);
#endif
#ifndef Blt_Tree_ApplyBFS_DECLARED
#define Blt_Tree_ApplyBFS_DECLARED
/* 181 */
BLT_EXTERN int		Blt_Tree_ApplyBFS(Blt_TreeNode root,
				Blt_TreeApplyProc *proc,
				ClientData clientData);
#endif
#ifndef Blt_Tree_SortNode_DECLARED
#define Blt_Tree_SortNode_DECLARED
/* 182 */
BLT_EXTERN int		Blt_Tree_SortNode(Blt_Tree tree, Blt_TreeNode node,
				Blt_TreeCompareNodesProc *proc);
#endif
#ifndef Blt_Tree_Exists_DECLARED
#define Blt_Tree_Exists_DECLARED
/* 183 */
BLT_EXTERN int		Blt_Tree_Exists(Tcl_Interp *interp, const char *name);
#endif
#ifndef Blt_Tree_Open_DECLARED
#define Blt_Tree_Open_DECLARED
/* 184 */
BLT_EXTERN Blt_Tree	Blt_Tree_Open(Tcl_Interp *interp, const char *name,
				int flags);
#endif
#ifndef Blt_Tree_Close_DECLARED
#define Blt_Tree_Close_DECLARED
/* 185 */
BLT_EXTERN void		Blt_Tree_Close(Blt_Tree tree);
#endif
#ifndef Blt_Tree_Attach_DECLARED
#define Blt_Tree_Attach_DECLARED
/* 186 */
BLT_EXTERN int		Blt_Tree_Attach(Tcl_Interp *interp, Blt_Tree tree,
				const char *name);
#endif
#ifndef Blt_Tree_GetFromObj_DECLARED
#define Blt_Tree_GetFromObj_DECLARED
/* 187 */
BLT_EXTERN Blt_Tree	Blt_Tree_GetFromObj(Tcl_Interp *interp,
				Tcl_Obj *objPtr);
#endif
#ifndef Blt_Tree_Size_DECLARED
#define Blt_Tree_Size_DECLARED
/* 188 */
BLT_EXTERN int		Blt_Tree_Size(Blt_TreeNode node);
#endif
#ifndef Blt_Tree_CreateTrace_DECLARED
#define Blt_Tree_CreateTrace_DECLARED
/* 189 */
BLT_EXTERN Blt_TreeTrace Blt_Tree_CreateTrace(Blt_Tree tree,
				Blt_TreeNode node, const char *keyPattern,
				const char *tagName, unsigned int mask,
				Blt_TreeTraceProc *proc,
				ClientData clientData);
#endif
#ifndef Blt_Tree_DeleteTrace_DECLARED
#define Blt_Tree_DeleteTrace_DECLARED
/* 190 */
BLT_EXTERN void		Blt_Tree_DeleteTrace(Blt_TreeTrace token);
#endif
#ifndef Blt_Tree_CreateEventHandler_DECLARED
#define Blt_Tree_CreateEventHandler_DECLARED
/* 191 */
BLT_EXTERN void		Blt_Tree_CreateEventHandler(Blt_Tree tree,
				unsigned int mask,
				Blt_TreeNotifyEventProc *proc,
				ClientData clientData);
#endif
#ifndef Blt_Tree_DeleteEventHandler_DECLARED
#define Blt_Tree_DeleteEventHandler_DECLARED
/* 192 */
BLT_EXTERN void		Blt_Tree_DeleteEventHandler(Blt_Tree tree,
				unsigned int mask,
				Blt_TreeNotifyEventProc *proc,
				ClientData clientData);
#endif
#ifndef Blt_Tree_RelabelNode_DECLARED
#define Blt_Tree_RelabelNode_DECLARED
/* 193 */
BLT_EXTERN void		Blt_Tree_RelabelNode(Blt_Tree tree,
				Blt_TreeNode node, const char *string);
#endif
#ifndef Blt_Tree_RelabelNodeWithoutNotify_DECLARED
#define Blt_Tree_RelabelNodeWithoutNotify_DECLARED
/* 194 */
BLT_EXTERN void		Blt_Tree_RelabelNodeWithoutNotify(Blt_TreeNode node,
				const char *string);
#endif
#ifndef Blt_Tree_NodeIdAscii_DECLARED
#define Blt_Tree_NodeIdAscii_DECLARED
/* 195 */
BLT_EXTERN const char *	 Blt_Tree_NodeIdAscii(Blt_TreeNode node);
#endif
#ifndef Blt_Tree_NodePath_DECLARED
#define Blt_Tree_NodePath_DECLARED
/* 196 */
BLT_EXTERN const char *	 Blt_Tree_NodePath(Blt_TreeNode node,
				Tcl_DString *resultPtr);
#endif
#ifndef Blt_Tree_NodeRelativePath_DECLARED
#define Blt_Tree_NodeRelativePath_DECLARED
/* 197 */
BLT_EXTERN const char *	 Blt_Tree_NodeRelativePath(Blt_TreeNode root,
				Blt_TreeNode node, const char *separator,
				unsigned int flags, Tcl_DString *resultPtr);
#endif
#ifndef Blt_Tree_NodePosition_DECLARED
#define Blt_Tree_NodePosition_DECLARED
/* 198 */
BLT_EXTERN long		Blt_Tree_NodePosition(Blt_TreeNode node);
#endif
#ifndef Blt_Tree_ClearTags_DECLARED
#define Blt_Tree_ClearTags_DECLARED
/* 199 */
BLT_EXTERN void		Blt_Tree_ClearTags(Blt_Tree tree, Blt_TreeNode node);
#endif
#ifndef Blt_Tree_HasTag_DECLARED
#define Blt_Tree_HasTag_DECLARED
/* 200 */
BLT_EXTERN int		Blt_Tree_HasTag(Blt_Tree tree, Blt_TreeNode node,
				const char *tagName);
#endif
#ifndef Blt_Tree_AddTag_DECLARED
#define Blt_Tree_AddTag_DECLARED
/* 201 */
BLT_EXTERN void		Blt_Tree_AddTag(Blt_Tree tree, Blt_TreeNode node,
				const char *tagName);
#endif
#ifndef Blt_Tree_RemoveTag_DECLARED
#define Blt_Tree_RemoveTag_DECLARED
/* 202 */
BLT_EXTERN void		Blt_Tree_RemoveTag(Blt_Tree tree, Blt_TreeNode node,
				const char *tagName);
#endif
#ifndef Blt_Tree_ForgetTag_DECLARED
#define Blt_Tree_ForgetTag_DECLARED
/* 203 */
BLT_EXTERN void		Blt_Tree_ForgetTag(Blt_Tree tree,
				const char *tagName);
#endif
#ifndef Blt_Tree_TagHashTable_DECLARED
#define Blt_Tree_TagHashTable_DECLARED
/* 204 */
BLT_EXTERN Blt_HashTable * Blt_Tree_TagHashTable(Blt_Tree tree,
				const char *tagName);
#endif
#ifndef Blt_Tree_TagTableIsShared_DECLARED
#define Blt_Tree_TagTableIsShared_DECLARED
/* 205 */
BLT_EXTERN int		Blt_Tree_TagTableIsShared(Blt_Tree tree);
#endif
#ifndef Blt_Tree_NewTagTable_DECLARED
#define Blt_Tree_NewTagTable_DECLARED
/* 206 */
BLT_EXTERN void		Blt_Tree_NewTagTable(Blt_Tree tree);
#endif
#ifndef Blt_Tree_FirstTag_DECLARED
#define Blt_Tree_FirstTag_DECLARED
/* 207 */
BLT_EXTERN Blt_HashEntry * Blt_Tree_FirstTag(Blt_Tree tree,
				Blt_HashSearch *searchPtr);
#endif
#ifndef Blt_Tree_DumpNode_DECLARED
#define Blt_Tree_DumpNode_DECLARED
/* 208 */
BLT_EXTERN void		Blt_Tree_DumpNode(Blt_Tree tree, Blt_TreeNode root,
				Blt_TreeNode node, Tcl_DString *resultPtr);
#endif
#ifndef Blt_Tree_Dump_DECLARED
#define Blt_Tree_Dump_DECLARED
/* 209 */
BLT_EXTERN int		Blt_Tree_Dump(Blt_Tree tree, Blt_TreeNode root,
				Tcl_DString *resultPtr);
#endif
#ifndef Blt_Tree_DumpToFile_DECLARED
#define Blt_Tree_DumpToFile_DECLARED
/* 210 */
BLT_EXTERN int		Blt_Tree_DumpToFile(Tcl_Interp *interp,
				Blt_Tree tree, Blt_TreeNode root,
				const char *fileName);
#endif
#ifndef Blt_Tree_Restore_DECLARED
#define Blt_Tree_Restore_DECLARED
/* 211 */
BLT_EXTERN int		Blt_Tree_Restore(Tcl_Interp *interp, Blt_Tree tree,
				Blt_TreeNode root, const char *string,
				unsigned int flags);
#endif
#ifndef Blt_Tree_RestoreFromFile_DECLARED
#define Blt_Tree_RestoreFromFile_DECLARED
/* 212 */
BLT_EXTERN int		Blt_Tree_RestoreFromFile(Tcl_Interp *interp,
				Blt_Tree tree, Blt_TreeNode root,
				const char *fileName, unsigned int flags);
#endif
#ifndef Blt_Tree_Depth_DECLARED
#define Blt_Tree_Depth_DECLARED
/* 213 */
BLT_EXTERN long		Blt_Tree_Depth(Blt_Tree tree);
#endif
#ifndef Blt_Tree_RegisterFormat_DECLARED
#define Blt_Tree_RegisterFormat_DECLARED
/* 214 */
BLT_EXTERN int		Blt_Tree_RegisterFormat(Tcl_Interp *interp,
				const char *fmtName,
				Blt_TreeImportProc *importProc,
				Blt_TreeExportProc *exportProc);
#endif
#ifndef Blt_Tree_RememberTag_DECLARED
#define Blt_Tree_RememberTag_DECLARED
/* 215 */
BLT_EXTERN Blt_TreeTagEntry * Blt_Tree_RememberTag(Blt_Tree tree,
				const char *name);
#endif
#ifndef Blt_InitHashTable_DECLARED
#define Blt_InitHashTable_DECLARED
/* 216 */
BLT_EXTERN void		Blt_InitHashTable(Blt_HashTable *tablePtr,
				size_t keyType);
#endif
#ifndef Blt_InitHashTableWithPool_DECLARED
#define Blt_InitHashTableWithPool_DECLARED
/* 217 */
BLT_EXTERN void		Blt_InitHashTableWithPool(Blt_HashTable *tablePtr,
				size_t keyType);
#endif
#ifndef Blt_DeleteHashTable_DECLARED
#define Blt_DeleteHashTable_DECLARED
/* 218 */
BLT_EXTERN void		Blt_DeleteHashTable(Blt_HashTable *tablePtr);
#endif
#ifndef Blt_DeleteHashEntry_DECLARED
#define Blt_DeleteHashEntry_DECLARED
/* 219 */
BLT_EXTERN void		Blt_DeleteHashEntry(Blt_HashTable *tablePtr,
				Blt_HashEntry *entryPtr);
#endif
#ifndef Blt_FirstHashEntry_DECLARED
#define Blt_FirstHashEntry_DECLARED
/* 220 */
BLT_EXTERN Blt_HashEntry * Blt_FirstHashEntry(Blt_HashTable *tablePtr,
				Blt_HashSearch *searchPtr);
#endif
#ifndef Blt_NextHashEntry_DECLARED
#define Blt_NextHashEntry_DECLARED
/* 221 */
BLT_EXTERN Blt_HashEntry * Blt_NextHashEntry(Blt_HashSearch *srchPtr);
#endif
#ifndef Blt_HashStats_DECLARED
#define Blt_HashStats_DECLARED
/* 222 */
BLT_EXTERN const char *	 Blt_HashStats(Blt_HashTable *tablePtr);
#endif
#ifndef Blt_VecMin_DECLARED
#define Blt_VecMin_DECLARED
/* 223 */
BLT_EXTERN double	Blt_VecMin(Blt_Vector *vPtr);
#endif
#ifndef Blt_VecMax_DECLARED
#define Blt_VecMax_DECLARED
/* 224 */
BLT_EXTERN double	Blt_VecMax(Blt_Vector *vPtr);
#endif
#ifndef Blt_AllocVectorId_DECLARED
#define Blt_AllocVectorId_DECLARED
/* 225 */
BLT_EXTERN Blt_VectorId	 Blt_AllocVectorId(Tcl_Interp *interp,
				const char *vecName);
#endif
#ifndef Blt_SetVectorChangedProc_DECLARED
#define Blt_SetVectorChangedProc_DECLARED
/* 226 */
BLT_EXTERN void		Blt_SetVectorChangedProc(Blt_VectorId clientId,
				Blt_VectorChangedProc *proc,
				ClientData clientData);
#endif
#ifndef Blt_FreeVectorId_DECLARED
#define Blt_FreeVectorId_DECLARED
/* 227 */
BLT_EXTERN void		Blt_FreeVectorId(Blt_VectorId clientId);
#endif
#ifndef Blt_GetVectorById_DECLARED
#define Blt_GetVectorById_DECLARED
/* 228 */
BLT_EXTERN int		Blt_GetVectorById(Tcl_Interp *interp,
				Blt_VectorId clientId,
				Blt_Vector **vecPtrPtr);
#endif
#ifndef Blt_NameOfVectorId_DECLARED
#define Blt_NameOfVectorId_DECLARED
/* 229 */
BLT_EXTERN const char *	 Blt_NameOfVectorId(Blt_VectorId clientId);
#endif
#ifndef Blt_NameOfVector_DECLARED
#define Blt_NameOfVector_DECLARED
/* 230 */
BLT_EXTERN const char *	 Blt_NameOfVector(Blt_Vector *vecPtr);
#endif
#ifndef Blt_VectorNotifyPending_DECLARED
#define Blt_VectorNotifyPending_DECLARED
/* 231 */
BLT_EXTERN int		Blt_VectorNotifyPending(Blt_VectorId clientId);
#endif
#ifndef Blt_CreateVector_DECLARED
#define Blt_CreateVector_DECLARED
/* 232 */
BLT_EXTERN int		Blt_CreateVector(Tcl_Interp *interp,
				const char *vecName, int size,
				Blt_Vector **vecPtrPtr);
#endif
#ifndef Blt_CreateVector2_DECLARED
#define Blt_CreateVector2_DECLARED
/* 233 */
BLT_EXTERN int		Blt_CreateVector2(Tcl_Interp *interp,
				const char *vecName, const char *cmdName,
				const char *varName, int initialSize,
				Blt_Vector **vecPtrPtr);
#endif
#ifndef Blt_GetVector_DECLARED
#define Blt_GetVector_DECLARED
/* 234 */
BLT_EXTERN int		Blt_GetVector(Tcl_Interp *interp,
				const char *vecName, Blt_Vector **vecPtrPtr);
#endif
#ifndef Blt_GetVectorFromObj_DECLARED
#define Blt_GetVectorFromObj_DECLARED
/* 235 */
BLT_EXTERN int		Blt_GetVectorFromObj(Tcl_Interp *interp,
				Tcl_Obj *objPtr, Blt_Vector **vecPtrPtr);
#endif
#ifndef Blt_VectorExists_DECLARED
#define Blt_VectorExists_DECLARED
/* 236 */
BLT_EXTERN int		Blt_VectorExists(Tcl_Interp *interp,
				const char *vecName);
#endif
#ifndef Blt_ResetVector_DECLARED
#define Blt_ResetVector_DECLARED
/* 237 */
BLT_EXTERN int		Blt_ResetVector(Blt_Vector *vecPtr, double *dataArr,
				int n, int arraySize, Tcl_FreeProc *freeProc);
#endif
#ifndef Blt_ResizeVector_DECLARED
#define Blt_ResizeVector_DECLARED
/* 238 */
BLT_EXTERN int		Blt_ResizeVector(Blt_Vector *vecPtr, int n);
#endif
#ifndef Blt_DeleteVectorByName_DECLARED
#define Blt_DeleteVectorByName_DECLARED
/* 239 */
BLT_EXTERN int		Blt_DeleteVectorByName(Tcl_Interp *interp,
				const char *vecName);
#endif
#ifndef Blt_DeleteVector_DECLARED
#define Blt_DeleteVector_DECLARED
/* 240 */
BLT_EXTERN int		Blt_DeleteVector(Blt_Vector *vecPtr);
#endif
#ifndef Blt_ExprVector_DECLARED
#define Blt_ExprVector_DECLARED
/* 241 */
BLT_EXTERN int		Blt_ExprVector(Tcl_Interp *interp, char *expr,
				Blt_Vector *vecPtr);
#endif
#ifndef Blt_InstallIndexProc_DECLARED
#define Blt_InstallIndexProc_DECLARED
/* 242 */
BLT_EXTERN void		Blt_InstallIndexProc(Tcl_Interp *interp,
				const char *indexName,
				Blt_VectorIndexProc *procPtr);
#endif
#ifndef Blt_VectorExists2_DECLARED
#define Blt_VectorExists2_DECLARED
/* 243 */
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
    int (*blt_Chain_IsBefore) (Blt_ChainLink first, Blt_ChainLink last); /* 17 */
    void (*blt_List_Init) (Blt_List list, size_t type); /* 18 */
    void (*blt_List_Reset) (Blt_List list); /* 19 */
    Blt_List (*blt_List_Create) (size_t type); /* 20 */
    void (*blt_List_Destroy) (Blt_List list); /* 21 */
    Blt_ListNode (*blt_List_CreateNode) (Blt_List list, const char *key); /* 22 */
    void (*blt_List_DeleteNode) (Blt_ListNode node); /* 23 */
    Blt_ListNode (*blt_List_Append) (Blt_List list, const char *key, ClientData clientData); /* 24 */
    Blt_ListNode (*blt_List_Prepend) (Blt_List list, const char *key, ClientData clientData); /* 25 */
    void (*blt_List_LinkAfter) (Blt_List list, Blt_ListNode node, Blt_ListNode afterNode); /* 26 */
    void (*blt_List_LinkBefore) (Blt_List list, Blt_ListNode node, Blt_ListNode beforeNode); /* 27 */
    void (*blt_List_UnlinkNode) (Blt_ListNode node); /* 28 */
    Blt_ListNode (*blt_List_GetNode) (Blt_List list, const char *key); /* 29 */
    void (*blt_List_DeleteNodeByKey) (Blt_List list, const char *key); /* 30 */
    Blt_ListNode (*blt_List_GetNthNode) (Blt_List list, long position, int direction); /* 31 */
    void (*blt_List_Sort) (Blt_List list, Blt_ListCompareProc *proc); /* 32 */
    void (*blt_table_release_tags) (BLT_TABLE table); /* 33 */
    int (*blt_table_exists) (Tcl_Interp *interp, const char *name); /* 34 */
    int (*blt_table_create) (Tcl_Interp *interp, const char *name, BLT_TABLE *tablePtr); /* 35 */
    int (*blt_table_open) (Tcl_Interp *interp, const char *name, BLT_TABLE *tablePtr); /* 36 */
    void (*blt_table_close) (BLT_TABLE table); /* 37 */
    int (*blt_table_same_object) (BLT_TABLE table1, BLT_TABLE table2); /* 38 */
    Blt_HashTable * (*blt_table_row_get_label_table) (BLT_TABLE table, const char *label); /* 39 */
    Blt_HashTable * (*blt_table_column_get_label_table) (BLT_TABLE table, const char *label); /* 40 */
    BLT_TABLE_ROW (*blt_table_get_row_by_label) (BLT_TABLE table, const char *label); /* 41 */
    BLT_TABLE_COLUMN (*blt_table_get_column_by_label) (BLT_TABLE table, const char *label); /* 42 */
    BLT_TABLE_ROW (*blt_table_get_row_by_index) (BLT_TABLE table, long index); /* 43 */
    BLT_TABLE_COLUMN (*blt_table_get_column_by_index) (BLT_TABLE table, long index); /* 44 */
    int (*blt_table_set_row_label) (Tcl_Interp *interp, BLT_TABLE table, BLT_TABLE_ROW row, const char *label); /* 45 */
    int (*blt_table_set_column_label) (Tcl_Interp *interp, BLT_TABLE table, BLT_TABLE_COLUMN column, const char *label); /* 46 */
    BLT_TABLE_COLUMN_TYPE (*blt_table_name_to_column_type) (const char *typeName); /* 47 */
    int (*blt_table_set_column_type) (BLT_TABLE table, BLT_TABLE_COLUMN column, BLT_TABLE_COLUMN_TYPE type); /* 48 */
    const char * (*blt_table_column_type_to_name) (BLT_TABLE_COLUMN_TYPE type); /* 49 */
    int (*blt_table_set_column_tag) (Tcl_Interp *interp, BLT_TABLE table, BLT_TABLE_COLUMN column, const char *tagName); /* 50 */
    int (*blt_table_set_row_tag) (Tcl_Interp *interp, BLT_TABLE table, BLT_TABLE_ROW row, const char *tagName); /* 51 */
    BLT_TABLE_ROW (*blt_table_create_row) (Tcl_Interp *interp, BLT_TABLE table, const char *label); /* 52 */
    BLT_TABLE_COLUMN (*blt_table_create_column) (Tcl_Interp *interp, BLT_TABLE table, const char *label); /* 53 */
    int (*blt_table_extend_rows) (Tcl_Interp *interp, BLT_TABLE table, size_t n, BLT_TABLE_ROW *rows); /* 54 */
    int (*blt_table_extend_columns) (Tcl_Interp *interp, BLT_TABLE table, size_t n, BLT_TABLE_COLUMN *columms); /* 55 */
    int (*blt_table_delete_row) (BLT_TABLE table, BLT_TABLE_ROW row); /* 56 */
    int (*blt_table_delete_column) (BLT_TABLE table, BLT_TABLE_COLUMN column); /* 57 */
    int (*blt_table_move_row) (Tcl_Interp *interp, BLT_TABLE table, BLT_TABLE_ROW from, BLT_TABLE_ROW to, size_t n); /* 58 */
    int (*blt_table_move_column) (Tcl_Interp *interp, BLT_TABLE table, BLT_TABLE_COLUMN from, BLT_TABLE_COLUMN to, size_t n); /* 59 */
    Tcl_Obj * (*blt_table_get_obj) (BLT_TABLE table, BLT_TABLE_ROW row, BLT_TABLE_COLUMN column); /* 60 */
    int (*blt_table_set_obj) (BLT_TABLE table, BLT_TABLE_ROW row, BLT_TABLE_COLUMN column, Tcl_Obj *objPtr); /* 61 */
    const char * (*blt_table_get_string) (BLT_TABLE table, BLT_TABLE_ROW row, BLT_TABLE_COLUMN column); /* 62 */
    int (*blt_table_set_string) (BLT_TABLE table, BLT_TABLE_ROW row, BLT_TABLE_COLUMN column, const char *string, int length); /* 63 */
    int (*blt_table_append_string) (Tcl_Interp *interp, BLT_TABLE table, BLT_TABLE_ROW row, BLT_TABLE_COLUMN column, const char *string, int length); /* 64 */
    double (*blt_table_get_double) (BLT_TABLE table, BLT_TABLE_ROW row, BLT_TABLE_COLUMN column); /* 65 */
    int (*blt_table_set_double) (BLT_TABLE table, BLT_TABLE_ROW row, BLT_TABLE_COLUMN column, double value); /* 66 */
    long (*blt_table_get_long) (BLT_TABLE table, BLT_TABLE_ROW row, BLT_TABLE_COLUMN column, long defValue); /* 67 */
    int (*blt_table_set_long) (BLT_TABLE table, BLT_TABLE_ROW row, BLT_TABLE_COLUMN column, long value); /* 68 */
    BLT_TABLE_VALUE (*blt_table_get_value) (BLT_TABLE table, BLT_TABLE_ROW row, BLT_TABLE_COLUMN column); /* 69 */
    int (*blt_table_set_value) (BLT_TABLE table, BLT_TABLE_ROW row, BLT_TABLE_COLUMN column, BLT_TABLE_VALUE value); /* 70 */
    int (*blt_table_unset_value) (BLT_TABLE table, BLT_TABLE_ROW row, BLT_TABLE_COLUMN column); /* 71 */
    int (*blt_table_value_exists) (BLT_TABLE table, BLT_TABLE_ROW row, BLT_TABLE_COLUMN column); /* 72 */
    Blt_HashTable * (*blt_table_get_row_tag_table) (BLT_TABLE table, const char *tagName); /* 73 */
    Blt_HashTable * (*blt_table_get_column_tag_table) (BLT_TABLE table, const char *tagName); /* 74 */
    Blt_Chain (*blt_table_row_tags) (BLT_TABLE table, BLT_TABLE_ROW row); /* 75 */
    Blt_Chain (*blt_table_column_tags) (BLT_TABLE table, BLT_TABLE_COLUMN column); /* 76 */
    int (*blt_table_tags_are_shared) (BLT_TABLE table); /* 77 */
    int (*blt_table_row_has_tag) (BLT_TABLE table, BLT_TABLE_ROW row, const char *tagName); /* 78 */
    int (*blt_table_column_has_tag) (BLT_TABLE table, BLT_TABLE_COLUMN column, const char *tagName); /* 79 */
    int (*blt_table_forget_row_tag) (Tcl_Interp *interp, BLT_TABLE table, const char *tagName); /* 80 */
    int (*blt_table_forget_column_tag) (Tcl_Interp *interp, BLT_TABLE table, const char *tagName); /* 81 */
    int (*blt_table_unset_row_tag) (Tcl_Interp *interp, BLT_TABLE table, BLT_TABLE_ROW row, const char *tagName); /* 82 */
    int (*blt_table_unset_column_tag) (Tcl_Interp *interp, BLT_TABLE table, BLT_TABLE_COLUMN column, const char *tagName); /* 83 */
    Blt_HashEntry * (*blt_table_first_row_tag) (BLT_TABLE table, Blt_HashSearch *cursorPtr); /* 84 */
    Blt_HashEntry * (*blt_table_first_column_tag) (BLT_TABLE table, Blt_HashSearch *cursorPtr); /* 85 */
    BLT_TABLE_COLUMN (*blt_table_first_column) (BLT_TABLE table); /* 86 */
    BLT_TABLE_COLUMN (*blt_table_next_column) (BLT_TABLE table, BLT_TABLE_COLUMN column); /* 87 */
    BLT_TABLE_ROW (*blt_table_first_row) (BLT_TABLE table); /* 88 */
    BLT_TABLE_ROW (*blt_table_next_row) (BLT_TABLE table, BLT_TABLE_ROW row); /* 89 */
    BLT_TABLE_ROWCOLUMN_SPEC (*blt_table_row_spec) (BLT_TABLE table, Tcl_Obj *objPtr, const char **sp); /* 90 */
    BLT_TABLE_ROWCOLUMN_SPEC (*blt_table_column_spec) (BLT_TABLE table, Tcl_Obj *objPtr, const char **sp); /* 91 */
    int (*blt_table_iterate_row) (Tcl_Interp *interp, BLT_TABLE table, Tcl_Obj *objPtr, BLT_TABLE_ITERATOR *iter); /* 92 */
    int (*blt_table_iterate_column) (Tcl_Interp *interp, BLT_TABLE table, Tcl_Obj *objPtr, BLT_TABLE_ITERATOR *iter); /* 93 */
    int (*blt_table_iterate_row_objv) (Tcl_Interp *interp, BLT_TABLE table, int objc, Tcl_Obj *const *objv, BLT_TABLE_ITERATOR *iterPtr); /* 94 */
    int (*blt_table_iterate_column_objv) (Tcl_Interp *interp, BLT_TABLE table, int objc, Tcl_Obj *const *objv, BLT_TABLE_ITERATOR *iterPtr); /* 95 */
    void (*blt_table_free_iterator_objv) (BLT_TABLE_ITERATOR *iterPtr); /* 96 */
    void (*blt_table_iterate_all_rows) (BLT_TABLE table, BLT_TABLE_ITERATOR *iterPtr); /* 97 */
    void (*blt_table_iterate_all_columns) (BLT_TABLE table, BLT_TABLE_ITERATOR *iterPtr); /* 98 */
    BLT_TABLE_ROW (*blt_table_first_tagged_row) (BLT_TABLE_ITERATOR *iter); /* 99 */
    BLT_TABLE_COLUMN (*blt_table_first_tagged_column) (BLT_TABLE_ITERATOR *iter); /* 100 */
    BLT_TABLE_ROW (*blt_table_next_tagged_row) (BLT_TABLE_ITERATOR *iter); /* 101 */
    BLT_TABLE_COLUMN (*blt_table_next_tagged_column) (BLT_TABLE_ITERATOR *iter); /* 102 */
    BLT_TABLE_ROW (*blt_table_get_row) (Tcl_Interp *interp, BLT_TABLE table, Tcl_Obj *objPtr); /* 103 */
    BLT_TABLE_COLUMN (*blt_table_get_column) (Tcl_Interp *interp, BLT_TABLE table, Tcl_Obj *objPtr); /* 104 */
    int (*blt_table_list_rows) (Tcl_Interp *interp, BLT_TABLE table, int objc, Tcl_Obj *const *objv, Blt_Chain chain); /* 105 */
    int (*blt_table_list_columns) (Tcl_Interp *interp, BLT_TABLE table, int objc, Tcl_Obj *const *objv, Blt_Chain chain); /* 106 */
    void (*blt_table_clear_row_tags) (BLT_TABLE table, BLT_TABLE_ROW row); /* 107 */
    void (*blt_table_clear_column_tags) (BLT_TABLE table, BLT_TABLE_COLUMN column); /* 108 */
    void (*blt_table_clear_row_traces) (BLT_TABLE table, BLT_TABLE_ROW row); /* 109 */
    void (*blt_table_clear_column_traces) (BLT_TABLE table, BLT_TABLE_COLUMN column); /* 110 */
    BLT_TABLE_TRACE (*blt_table_create_trace) (BLT_TABLE table, BLT_TABLE_ROW row, BLT_TABLE_COLUMN column, const char *rowTag, const char *columnTag, unsigned int mask, BLT_TABLE_TRACE_PROC *proc, BLT_TABLE_TRACE_DELETE_PROC *deleteProc, ClientData clientData); /* 111 */
    BLT_TABLE_TRACE (*blt_table_set_column_trace) (BLT_TABLE table, BLT_TABLE_COLUMN column, unsigned int mask, BLT_TABLE_TRACE_PROC *proc, BLT_TABLE_TRACE_DELETE_PROC *deleteProc, ClientData clientData); /* 112 */
    BLT_TABLE_TRACE (*blt_table_create_column_tag_trace) (BLT_TABLE table, const char *tag, unsigned int mask, BLT_TABLE_TRACE_PROC *proc, BLT_TABLE_TRACE_DELETE_PROC *deleteProc, ClientData clientData); /* 113 */
    BLT_TABLE_TRACE (*blt_table_create_row_trace) (BLT_TABLE table, BLT_TABLE_ROW row, unsigned int mask, BLT_TABLE_TRACE_PROC *proc, BLT_TABLE_TRACE_DELETE_PROC *deleteProc, ClientData clientData); /* 114 */
    BLT_TABLE_TRACE (*blt_table_create_row_tag_trace) (BLT_TABLE table, const char *tag, unsigned int mask, BLT_TABLE_TRACE_PROC *proc, BLT_TABLE_TRACE_DELETE_PROC *deleteProc, ClientData clientData); /* 115 */
    void (*blt_table_delete_trace) (BLT_TABLE_TRACE trace); /* 116 */
    BLT_TABLE_NOTIFIER (*blt_table_create_row_notifier) (Tcl_Interp *interp, BLT_TABLE table, BLT_TABLE_ROW row, unsigned int mask, BLT_TABLE_NOTIFY_EVENT_PROC *proc, BLT_TABLE_NOTIFIER_DELETE_PROC *deleteProc, ClientData clientData); /* 117 */
    BLT_TABLE_NOTIFIER (*blt_table_create_row_tag_notifier) (Tcl_Interp *interp, BLT_TABLE table, const char *tag, unsigned int mask, BLT_TABLE_NOTIFY_EVENT_PROC *proc, BLT_TABLE_NOTIFIER_DELETE_PROC *deleteProc, ClientData clientData); /* 118 */
    BLT_TABLE_NOTIFIER (*blt_table_create_column_notifier) (Tcl_Interp *interp, BLT_TABLE table, BLT_TABLE_COLUMN column, unsigned int mask, BLT_TABLE_NOTIFY_EVENT_PROC *proc, BLT_TABLE_NOTIFIER_DELETE_PROC *deleteProc, ClientData clientData); /* 119 */
    BLT_TABLE_NOTIFIER (*blt_table_create_column_tag_notifier) (Tcl_Interp *interp, BLT_TABLE table, const char *tag, unsigned int mask, BLT_TABLE_NOTIFY_EVENT_PROC *proc, BLT_TABLE_NOTIFIER_DELETE_PROC *deleteProc, ClientData clientData); /* 120 */
    void (*blt_table_delete_notifier) (BLT_TABLE_NOTIFIER notifier); /* 121 */
    void (*blt_table_sort_init) (BLT_TABLE table, BLT_TABLE_SORT_ORDER *order, size_t numCompares, unsigned int flags); /* 122 */
    BLT_TABLE_ROW * (*blt_table_sort_rows) (BLT_TABLE table); /* 123 */
    void (*blt_table_sort_rows_subset) (BLT_TABLE table, long numRows, BLT_TABLE_ROW *rows); /* 124 */
    void (*blt_table_sort_finish) (void); /* 125 */
    BLT_TABLE_COMPARE_PROC * (*blt_table_get_compare_proc) (BLT_TABLE table, BLT_TABLE_COLUMN column, unsigned int flags); /* 126 */
    BLT_TABLE_ROW * (*blt_table_get_row_map) (BLT_TABLE table); /* 127 */
    BLT_TABLE_COLUMN * (*blt_table_get_column_map) (BLT_TABLE table); /* 128 */
    void (*blt_table_set_row_map) (BLT_TABLE table, BLT_TABLE_ROW *map); /* 129 */
    void (*blt_table_set_column_map) (BLT_TABLE table, BLT_TABLE_COLUMN *map); /* 130 */
    int (*blt_table_restore) (Tcl_Interp *interp, BLT_TABLE table, char *string, unsigned int flags); /* 131 */
    int (*blt_table_file_restore) (Tcl_Interp *interp, BLT_TABLE table, const char *fileName, unsigned int flags); /* 132 */
    int (*blt_table_register_format) (Tcl_Interp *interp, const char *name, BLT_TABLE_IMPORT_PROC *importProc, BLT_TABLE_EXPORT_PROC *exportProc); /* 133 */
    void (*blt_table_unset_keys) (BLT_TABLE table); /* 134 */
    Blt_Chain (*blt_table_get_keys) (BLT_TABLE table); /* 135 */
    int (*blt_table_set_keys) (BLT_TABLE table, Blt_Chain keys, int unique); /* 136 */
    int (*blt_table_key_lookup) (Tcl_Interp *interp, BLT_TABLE table, int objc, Tcl_Obj *const *objv, BLT_TABLE_ROW *rowPtr); /* 137 */
    int (*blt_table_get_column_limits) (Tcl_Interp *interp, BLT_TABLE table, BLT_TABLE_COLUMN col, Tcl_Obj **minObjPtrPtr, Tcl_Obj **maxObjPtrPtr); /* 138 */
    Blt_Pool (*blt_Pool_Create) (int type); /* 139 */
    void (*blt_Pool_Destroy) (Blt_Pool pool); /* 140 */
    Blt_TreeKey (*blt_Tree_GetKey) (Blt_Tree tree, const char *string); /* 141 */
    Blt_TreeKey (*blt_Tree_GetKeyFromNode) (Blt_TreeNode node, const char *string); /* 142 */
    Blt_TreeKey (*blt_Tree_GetKeyFromInterp) (Tcl_Interp *interp, const char *string); /* 143 */
    Blt_TreeNode (*blt_Tree_CreateNode) (Blt_Tree tree, Blt_TreeNode parent, const char *name, long position); /* 144 */
    Blt_TreeNode (*blt_Tree_CreateNodeWithId) (Blt_Tree tree, Blt_TreeNode parent, const char *name, long inode, long position); /* 145 */
    int (*blt_Tree_DeleteNode) (Blt_Tree tree, Blt_TreeNode node); /* 146 */
    int (*blt_Tree_MoveNode) (Blt_Tree tree, Blt_TreeNode node, Blt_TreeNode parent, Blt_TreeNode before); /* 147 */
    Blt_TreeNode (*blt_Tree_GetNode) (Blt_Tree tree, long inode); /* 148 */
    Blt_TreeNode (*blt_Tree_FindChild) (Blt_TreeNode parent, const char *name); /* 149 */
    Blt_TreeNode (*blt_Tree_NextNode) (Blt_TreeNode root, Blt_TreeNode node); /* 150 */
    Blt_TreeNode (*blt_Tree_PrevNode) (Blt_TreeNode root, Blt_TreeNode node); /* 151 */
    Blt_TreeNode (*blt_Tree_FirstChild) (Blt_TreeNode parent); /* 152 */
    Blt_TreeNode (*blt_Tree_LastChild) (Blt_TreeNode parent); /* 153 */
    int (*blt_Tree_IsBefore) (Blt_TreeNode node1, Blt_TreeNode node2); /* 154 */
    int (*blt_Tree_IsAncestor) (Blt_TreeNode node1, Blt_TreeNode node2); /* 155 */
    int (*blt_Tree_PrivateValue) (Tcl_Interp *interp, Blt_Tree tree, Blt_TreeNode node, Blt_TreeKey key); /* 156 */
    int (*blt_Tree_PublicValue) (Tcl_Interp *interp, Blt_Tree tree, Blt_TreeNode node, Blt_TreeKey key); /* 157 */
    int (*blt_Tree_GetValue) (Tcl_Interp *interp, Blt_Tree tree, Blt_TreeNode node, const char *string, Tcl_Obj **valuePtr); /* 158 */
    int (*blt_Tree_ValueExists) (Blt_Tree tree, Blt_TreeNode node, const char *string); /* 159 */
    int (*blt_Tree_SetValue) (Tcl_Interp *interp, Blt_Tree tree, Blt_TreeNode node, const char *string, Tcl_Obj *valuePtr); /* 160 */
    int (*blt_Tree_UnsetValue) (Tcl_Interp *interp, Blt_Tree tree, Blt_TreeNode node, const char *string); /* 161 */
    int (*blt_Tree_AppendValue) (Tcl_Interp *interp, Blt_Tree tree, Blt_TreeNode node, const char *string, const char *value); /* 162 */
    int (*blt_Tree_ListAppendValue) (Tcl_Interp *interp, Blt_Tree tree, Blt_TreeNode node, const char *string, Tcl_Obj *valuePtr); /* 163 */
    int (*blt_Tree_GetArrayValue) (Tcl_Interp *interp, Blt_Tree tree, Blt_TreeNode node, const char *arrayName, const char *elemName, Tcl_Obj **valueObjPtrPtr); /* 164 */
    int (*blt_Tree_SetArrayValue) (Tcl_Interp *interp, Blt_Tree tree, Blt_TreeNode node, const char *arrayName, const char *elemName, Tcl_Obj *valueObjPtr); /* 165 */
    int (*blt_Tree_UnsetArrayValue) (Tcl_Interp *interp, Blt_Tree tree, Blt_TreeNode node, const char *arrayName, const char *elemName); /* 166 */
    int (*blt_Tree_AppendArrayValue) (Tcl_Interp *interp, Blt_Tree tree, Blt_TreeNode node, const char *arrayName, const char *elemName, const char *value); /* 167 */
    int (*blt_Tree_ListAppendArrayValue) (Tcl_Interp *interp, Blt_Tree tree, Blt_TreeNode node, const char *arrayName, const char *elemName, Tcl_Obj *valueObjPtr); /* 168 */
    int (*blt_Tree_ArrayValueExists) (Blt_Tree tree, Blt_TreeNode node, const char *arrayName, const char *elemName); /* 169 */
    int (*blt_Tree_ArrayNames) (Tcl_Interp *interp, Blt_Tree tree, Blt_TreeNode node, const char *arrayName, Tcl_Obj *listObjPtr); /* 170 */
    int (*blt_Tree_GetValueByKey) (Tcl_Interp *interp, Blt_Tree tree, Blt_TreeNode node, Blt_TreeKey key, Tcl_Obj **valuePtr); /* 171 */
    int (*blt_Tree_SetValueByKey) (Tcl_Interp *interp, Blt_Tree tree, Blt_TreeNode node, Blt_TreeKey key, Tcl_Obj *valuePtr); /* 172 */
    int (*blt_Tree_UnsetValueByKey) (Tcl_Interp *interp, Blt_Tree tree, Blt_TreeNode node, Blt_TreeKey key); /* 173 */
    int (*blt_Tree_AppendValueByKey) (Tcl_Interp *interp, Blt_Tree tree, Blt_TreeNode node, Blt_TreeKey key, const char *value); /* 174 */
    int (*blt_Tree_ListAppendValueByKey) (Tcl_Interp *interp, Blt_Tree tree, Blt_TreeNode node, Blt_TreeKey key, Tcl_Obj *valuePtr); /* 175 */
    int (*blt_Tree_ValueExistsByKey) (Blt_Tree tree, Blt_TreeNode node, Blt_TreeKey key); /* 176 */
    Blt_TreeKey (*blt_Tree_FirstKey) (Blt_Tree tree, Blt_TreeNode node, Blt_TreeKeyIterator *iterPtr); /* 177 */
    Blt_TreeKey (*blt_Tree_NextKey) (Blt_Tree tree, Blt_TreeKeyIterator *iterPtr); /* 178 */
    int (*blt_Tree_Apply) (Blt_TreeNode root, Blt_TreeApplyProc *proc, ClientData clientData); /* 179 */
    int (*blt_Tree_ApplyDFS) (Blt_TreeNode root, Blt_TreeApplyProc *proc, ClientData clientData, int order); /* 180 */
    int (*blt_Tree_ApplyBFS) (Blt_TreeNode root, Blt_TreeApplyProc *proc, ClientData clientData); /* 181 */
    int (*blt_Tree_SortNode) (Blt_Tree tree, Blt_TreeNode node, Blt_TreeCompareNodesProc *proc); /* 182 */
    int (*blt_Tree_Exists) (Tcl_Interp *interp, const char *name); /* 183 */
    Blt_Tree (*blt_Tree_Open) (Tcl_Interp *interp, const char *name, int flags); /* 184 */
    void (*blt_Tree_Close) (Blt_Tree tree); /* 185 */
    int (*blt_Tree_Attach) (Tcl_Interp *interp, Blt_Tree tree, const char *name); /* 186 */
    Blt_Tree (*blt_Tree_GetFromObj) (Tcl_Interp *interp, Tcl_Obj *objPtr); /* 187 */
    int (*blt_Tree_Size) (Blt_TreeNode node); /* 188 */
    Blt_TreeTrace (*blt_Tree_CreateTrace) (Blt_Tree tree, Blt_TreeNode node, const char *keyPattern, const char *tagName, unsigned int mask, Blt_TreeTraceProc *proc, ClientData clientData); /* 189 */
    void (*blt_Tree_DeleteTrace) (Blt_TreeTrace token); /* 190 */
    void (*blt_Tree_CreateEventHandler) (Blt_Tree tree, unsigned int mask, Blt_TreeNotifyEventProc *proc, ClientData clientData); /* 191 */
    void (*blt_Tree_DeleteEventHandler) (Blt_Tree tree, unsigned int mask, Blt_TreeNotifyEventProc *proc, ClientData clientData); /* 192 */
    void (*blt_Tree_RelabelNode) (Blt_Tree tree, Blt_TreeNode node, const char *string); /* 193 */
    void (*blt_Tree_RelabelNodeWithoutNotify) (Blt_TreeNode node, const char *string); /* 194 */
    const char * (*blt_Tree_NodeIdAscii) (Blt_TreeNode node); /* 195 */
    const char * (*blt_Tree_NodePath) (Blt_TreeNode node, Tcl_DString *resultPtr); /* 196 */
    const char * (*blt_Tree_NodeRelativePath) (Blt_TreeNode root, Blt_TreeNode node, const char *separator, unsigned int flags, Tcl_DString *resultPtr); /* 197 */
    long (*blt_Tree_NodePosition) (Blt_TreeNode node); /* 198 */
    void (*blt_Tree_ClearTags) (Blt_Tree tree, Blt_TreeNode node); /* 199 */
    int (*blt_Tree_HasTag) (Blt_Tree tree, Blt_TreeNode node, const char *tagName); /* 200 */
    void (*blt_Tree_AddTag) (Blt_Tree tree, Blt_TreeNode node, const char *tagName); /* 201 */
    void (*blt_Tree_RemoveTag) (Blt_Tree tree, Blt_TreeNode node, const char *tagName); /* 202 */
    void (*blt_Tree_ForgetTag) (Blt_Tree tree, const char *tagName); /* 203 */
    Blt_HashTable * (*blt_Tree_TagHashTable) (Blt_Tree tree, const char *tagName); /* 204 */
    int (*blt_Tree_TagTableIsShared) (Blt_Tree tree); /* 205 */
    void (*blt_Tree_NewTagTable) (Blt_Tree tree); /* 206 */
    Blt_HashEntry * (*blt_Tree_FirstTag) (Blt_Tree tree, Blt_HashSearch *searchPtr); /* 207 */
    void (*blt_Tree_DumpNode) (Blt_Tree tree, Blt_TreeNode root, Blt_TreeNode node, Tcl_DString *resultPtr); /* 208 */
    int (*blt_Tree_Dump) (Blt_Tree tree, Blt_TreeNode root, Tcl_DString *resultPtr); /* 209 */
    int (*blt_Tree_DumpToFile) (Tcl_Interp *interp, Blt_Tree tree, Blt_TreeNode root, const char *fileName); /* 210 */
    int (*blt_Tree_Restore) (Tcl_Interp *interp, Blt_Tree tree, Blt_TreeNode root, const char *string, unsigned int flags); /* 211 */
    int (*blt_Tree_RestoreFromFile) (Tcl_Interp *interp, Blt_Tree tree, Blt_TreeNode root, const char *fileName, unsigned int flags); /* 212 */
    long (*blt_Tree_Depth) (Blt_Tree tree); /* 213 */
    int (*blt_Tree_RegisterFormat) (Tcl_Interp *interp, const char *fmtName, Blt_TreeImportProc *importProc, Blt_TreeExportProc *exportProc); /* 214 */
    Blt_TreeTagEntry * (*blt_Tree_RememberTag) (Blt_Tree tree, const char *name); /* 215 */
    void (*blt_InitHashTable) (Blt_HashTable *tablePtr, size_t keyType); /* 216 */
    void (*blt_InitHashTableWithPool) (Blt_HashTable *tablePtr, size_t keyType); /* 217 */
    void (*blt_DeleteHashTable) (Blt_HashTable *tablePtr); /* 218 */
    void (*blt_DeleteHashEntry) (Blt_HashTable *tablePtr, Blt_HashEntry *entryPtr); /* 219 */
    Blt_HashEntry * (*blt_FirstHashEntry) (Blt_HashTable *tablePtr, Blt_HashSearch *searchPtr); /* 220 */
    Blt_HashEntry * (*blt_NextHashEntry) (Blt_HashSearch *srchPtr); /* 221 */
    const char * (*blt_HashStats) (Blt_HashTable *tablePtr); /* 222 */
    double (*blt_VecMin) (Blt_Vector *vPtr); /* 223 */
    double (*blt_VecMax) (Blt_Vector *vPtr); /* 224 */
    Blt_VectorId (*blt_AllocVectorId) (Tcl_Interp *interp, const char *vecName); /* 225 */
    void (*blt_SetVectorChangedProc) (Blt_VectorId clientId, Blt_VectorChangedProc *proc, ClientData clientData); /* 226 */
    void (*blt_FreeVectorId) (Blt_VectorId clientId); /* 227 */
    int (*blt_GetVectorById) (Tcl_Interp *interp, Blt_VectorId clientId, Blt_Vector **vecPtrPtr); /* 228 */
    const char * (*blt_NameOfVectorId) (Blt_VectorId clientId); /* 229 */
    const char * (*blt_NameOfVector) (Blt_Vector *vecPtr); /* 230 */
    int (*blt_VectorNotifyPending) (Blt_VectorId clientId); /* 231 */
    int (*blt_CreateVector) (Tcl_Interp *interp, const char *vecName, int size, Blt_Vector **vecPtrPtr); /* 232 */
    int (*blt_CreateVector2) (Tcl_Interp *interp, const char *vecName, const char *cmdName, const char *varName, int initialSize, Blt_Vector **vecPtrPtr); /* 233 */
    int (*blt_GetVector) (Tcl_Interp *interp, const char *vecName, Blt_Vector **vecPtrPtr); /* 234 */
    int (*blt_GetVectorFromObj) (Tcl_Interp *interp, Tcl_Obj *objPtr, Blt_Vector **vecPtrPtr); /* 235 */
    int (*blt_VectorExists) (Tcl_Interp *interp, const char *vecName); /* 236 */
    int (*blt_ResetVector) (Blt_Vector *vecPtr, double *dataArr, int n, int arraySize, Tcl_FreeProc *freeProc); /* 237 */
    int (*blt_ResizeVector) (Blt_Vector *vecPtr, int n); /* 238 */
    int (*blt_DeleteVectorByName) (Tcl_Interp *interp, const char *vecName); /* 239 */
    int (*blt_DeleteVector) (Blt_Vector *vecPtr); /* 240 */
    int (*blt_ExprVector) (Tcl_Interp *interp, char *expr, Blt_Vector *vecPtr); /* 241 */
    void (*blt_InstallIndexProc) (Tcl_Interp *interp, const char *indexName, Blt_VectorIndexProc *procPtr); /* 242 */
    int (*blt_VectorExists2) (Tcl_Interp *interp, const char *vecName); /* 243 */
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
#ifndef Blt_Chain_IsBefore
#define Blt_Chain_IsBefore \
	(bltTclProcsPtr->blt_Chain_IsBefore) /* 17 */
#endif
#ifndef Blt_List_Init
#define Blt_List_Init \
	(bltTclProcsPtr->blt_List_Init) /* 18 */
#endif
#ifndef Blt_List_Reset
#define Blt_List_Reset \
	(bltTclProcsPtr->blt_List_Reset) /* 19 */
#endif
#ifndef Blt_List_Create
#define Blt_List_Create \
	(bltTclProcsPtr->blt_List_Create) /* 20 */
#endif
#ifndef Blt_List_Destroy
#define Blt_List_Destroy \
	(bltTclProcsPtr->blt_List_Destroy) /* 21 */
#endif
#ifndef Blt_List_CreateNode
#define Blt_List_CreateNode \
	(bltTclProcsPtr->blt_List_CreateNode) /* 22 */
#endif
#ifndef Blt_List_DeleteNode
#define Blt_List_DeleteNode \
	(bltTclProcsPtr->blt_List_DeleteNode) /* 23 */
#endif
#ifndef Blt_List_Append
#define Blt_List_Append \
	(bltTclProcsPtr->blt_List_Append) /* 24 */
#endif
#ifndef Blt_List_Prepend
#define Blt_List_Prepend \
	(bltTclProcsPtr->blt_List_Prepend) /* 25 */
#endif
#ifndef Blt_List_LinkAfter
#define Blt_List_LinkAfter \
	(bltTclProcsPtr->blt_List_LinkAfter) /* 26 */
#endif
#ifndef Blt_List_LinkBefore
#define Blt_List_LinkBefore \
	(bltTclProcsPtr->blt_List_LinkBefore) /* 27 */
#endif
#ifndef Blt_List_UnlinkNode
#define Blt_List_UnlinkNode \
	(bltTclProcsPtr->blt_List_UnlinkNode) /* 28 */
#endif
#ifndef Blt_List_GetNode
#define Blt_List_GetNode \
	(bltTclProcsPtr->blt_List_GetNode) /* 29 */
#endif
#ifndef Blt_List_DeleteNodeByKey
#define Blt_List_DeleteNodeByKey \
	(bltTclProcsPtr->blt_List_DeleteNodeByKey) /* 30 */
#endif
#ifndef Blt_List_GetNthNode
#define Blt_List_GetNthNode \
	(bltTclProcsPtr->blt_List_GetNthNode) /* 31 */
#endif
#ifndef Blt_List_Sort
#define Blt_List_Sort \
	(bltTclProcsPtr->blt_List_Sort) /* 32 */
#endif
#ifndef blt_table_release_tags
#define blt_table_release_tags \
	(bltTclProcsPtr->blt_table_release_tags) /* 33 */
#endif
#ifndef blt_table_exists
#define blt_table_exists \
	(bltTclProcsPtr->blt_table_exists) /* 34 */
#endif
#ifndef blt_table_create
#define blt_table_create \
	(bltTclProcsPtr->blt_table_create) /* 35 */
#endif
#ifndef blt_table_open
#define blt_table_open \
	(bltTclProcsPtr->blt_table_open) /* 36 */
#endif
#ifndef blt_table_close
#define blt_table_close \
	(bltTclProcsPtr->blt_table_close) /* 37 */
#endif
#ifndef blt_table_same_object
#define blt_table_same_object \
	(bltTclProcsPtr->blt_table_same_object) /* 38 */
#endif
#ifndef blt_table_row_get_label_table
#define blt_table_row_get_label_table \
	(bltTclProcsPtr->blt_table_row_get_label_table) /* 39 */
#endif
#ifndef blt_table_column_get_label_table
#define blt_table_column_get_label_table \
	(bltTclProcsPtr->blt_table_column_get_label_table) /* 40 */
#endif
#ifndef blt_table_get_row_by_label
#define blt_table_get_row_by_label \
	(bltTclProcsPtr->blt_table_get_row_by_label) /* 41 */
#endif
#ifndef blt_table_get_column_by_label
#define blt_table_get_column_by_label \
	(bltTclProcsPtr->blt_table_get_column_by_label) /* 42 */
#endif
#ifndef blt_table_get_row_by_index
#define blt_table_get_row_by_index \
	(bltTclProcsPtr->blt_table_get_row_by_index) /* 43 */
#endif
#ifndef blt_table_get_column_by_index
#define blt_table_get_column_by_index \
	(bltTclProcsPtr->blt_table_get_column_by_index) /* 44 */
#endif
#ifndef blt_table_set_row_label
#define blt_table_set_row_label \
	(bltTclProcsPtr->blt_table_set_row_label) /* 45 */
#endif
#ifndef blt_table_set_column_label
#define blt_table_set_column_label \
	(bltTclProcsPtr->blt_table_set_column_label) /* 46 */
#endif
#ifndef blt_table_name_to_column_type
#define blt_table_name_to_column_type \
	(bltTclProcsPtr->blt_table_name_to_column_type) /* 47 */
#endif
#ifndef blt_table_set_column_type
#define blt_table_set_column_type \
	(bltTclProcsPtr->blt_table_set_column_type) /* 48 */
#endif
#ifndef blt_table_column_type_to_name
#define blt_table_column_type_to_name \
	(bltTclProcsPtr->blt_table_column_type_to_name) /* 49 */
#endif
#ifndef blt_table_set_column_tag
#define blt_table_set_column_tag \
	(bltTclProcsPtr->blt_table_set_column_tag) /* 50 */
#endif
#ifndef blt_table_set_row_tag
#define blt_table_set_row_tag \
	(bltTclProcsPtr->blt_table_set_row_tag) /* 51 */
#endif
#ifndef blt_table_create_row
#define blt_table_create_row \
	(bltTclProcsPtr->blt_table_create_row) /* 52 */
#endif
#ifndef blt_table_create_column
#define blt_table_create_column \
	(bltTclProcsPtr->blt_table_create_column) /* 53 */
#endif
#ifndef blt_table_extend_rows
#define blt_table_extend_rows \
	(bltTclProcsPtr->blt_table_extend_rows) /* 54 */
#endif
#ifndef blt_table_extend_columns
#define blt_table_extend_columns \
	(bltTclProcsPtr->blt_table_extend_columns) /* 55 */
#endif
#ifndef blt_table_delete_row
#define blt_table_delete_row \
	(bltTclProcsPtr->blt_table_delete_row) /* 56 */
#endif
#ifndef blt_table_delete_column
#define blt_table_delete_column \
	(bltTclProcsPtr->blt_table_delete_column) /* 57 */
#endif
#ifndef blt_table_move_row
#define blt_table_move_row \
	(bltTclProcsPtr->blt_table_move_row) /* 58 */
#endif
#ifndef blt_table_move_column
#define blt_table_move_column \
	(bltTclProcsPtr->blt_table_move_column) /* 59 */
#endif
#ifndef blt_table_get_obj
#define blt_table_get_obj \
	(bltTclProcsPtr->blt_table_get_obj) /* 60 */
#endif
#ifndef blt_table_set_obj
#define blt_table_set_obj \
	(bltTclProcsPtr->blt_table_set_obj) /* 61 */
#endif
#ifndef blt_table_get_string
#define blt_table_get_string \
	(bltTclProcsPtr->blt_table_get_string) /* 62 */
#endif
#ifndef blt_table_set_string
#define blt_table_set_string \
	(bltTclProcsPtr->blt_table_set_string) /* 63 */
#endif
#ifndef blt_table_append_string
#define blt_table_append_string \
	(bltTclProcsPtr->blt_table_append_string) /* 64 */
#endif
#ifndef blt_table_get_double
#define blt_table_get_double \
	(bltTclProcsPtr->blt_table_get_double) /* 65 */
#endif
#ifndef blt_table_set_double
#define blt_table_set_double \
	(bltTclProcsPtr->blt_table_set_double) /* 66 */
#endif
#ifndef blt_table_get_long
#define blt_table_get_long \
	(bltTclProcsPtr->blt_table_get_long) /* 67 */
#endif
#ifndef blt_table_set_long
#define blt_table_set_long \
	(bltTclProcsPtr->blt_table_set_long) /* 68 */
#endif
#ifndef blt_table_get_value
#define blt_table_get_value \
	(bltTclProcsPtr->blt_table_get_value) /* 69 */
#endif
#ifndef blt_table_set_value
#define blt_table_set_value \
	(bltTclProcsPtr->blt_table_set_value) /* 70 */
#endif
#ifndef blt_table_unset_value
#define blt_table_unset_value \
	(bltTclProcsPtr->blt_table_unset_value) /* 71 */
#endif
#ifndef blt_table_value_exists
#define blt_table_value_exists \
	(bltTclProcsPtr->blt_table_value_exists) /* 72 */
#endif
#ifndef blt_table_get_row_tag_table
#define blt_table_get_row_tag_table \
	(bltTclProcsPtr->blt_table_get_row_tag_table) /* 73 */
#endif
#ifndef blt_table_get_column_tag_table
#define blt_table_get_column_tag_table \
	(bltTclProcsPtr->blt_table_get_column_tag_table) /* 74 */
#endif
#ifndef blt_table_row_tags
#define blt_table_row_tags \
	(bltTclProcsPtr->blt_table_row_tags) /* 75 */
#endif
#ifndef blt_table_column_tags
#define blt_table_column_tags \
	(bltTclProcsPtr->blt_table_column_tags) /* 76 */
#endif
#ifndef blt_table_tags_are_shared
#define blt_table_tags_are_shared \
	(bltTclProcsPtr->blt_table_tags_are_shared) /* 77 */
#endif
#ifndef blt_table_row_has_tag
#define blt_table_row_has_tag \
	(bltTclProcsPtr->blt_table_row_has_tag) /* 78 */
#endif
#ifndef blt_table_column_has_tag
#define blt_table_column_has_tag \
	(bltTclProcsPtr->blt_table_column_has_tag) /* 79 */
#endif
#ifndef blt_table_forget_row_tag
#define blt_table_forget_row_tag \
	(bltTclProcsPtr->blt_table_forget_row_tag) /* 80 */
#endif
#ifndef blt_table_forget_column_tag
#define blt_table_forget_column_tag \
	(bltTclProcsPtr->blt_table_forget_column_tag) /* 81 */
#endif
#ifndef blt_table_unset_row_tag
#define blt_table_unset_row_tag \
	(bltTclProcsPtr->blt_table_unset_row_tag) /* 82 */
#endif
#ifndef blt_table_unset_column_tag
#define blt_table_unset_column_tag \
	(bltTclProcsPtr->blt_table_unset_column_tag) /* 83 */
#endif
#ifndef blt_table_first_row_tag
#define blt_table_first_row_tag \
	(bltTclProcsPtr->blt_table_first_row_tag) /* 84 */
#endif
#ifndef blt_table_first_column_tag
#define blt_table_first_column_tag \
	(bltTclProcsPtr->blt_table_first_column_tag) /* 85 */
#endif
#ifndef blt_table_first_column
#define blt_table_first_column \
	(bltTclProcsPtr->blt_table_first_column) /* 86 */
#endif
#ifndef blt_table_next_column
#define blt_table_next_column \
	(bltTclProcsPtr->blt_table_next_column) /* 87 */
#endif
#ifndef blt_table_first_row
#define blt_table_first_row \
	(bltTclProcsPtr->blt_table_first_row) /* 88 */
#endif
#ifndef blt_table_next_row
#define blt_table_next_row \
	(bltTclProcsPtr->blt_table_next_row) /* 89 */
#endif
#ifndef blt_table_row_spec
#define blt_table_row_spec \
	(bltTclProcsPtr->blt_table_row_spec) /* 90 */
#endif
#ifndef blt_table_column_spec
#define blt_table_column_spec \
	(bltTclProcsPtr->blt_table_column_spec) /* 91 */
#endif
#ifndef blt_table_iterate_row
#define blt_table_iterate_row \
	(bltTclProcsPtr->blt_table_iterate_row) /* 92 */
#endif
#ifndef blt_table_iterate_column
#define blt_table_iterate_column \
	(bltTclProcsPtr->blt_table_iterate_column) /* 93 */
#endif
#ifndef blt_table_iterate_row_objv
#define blt_table_iterate_row_objv \
	(bltTclProcsPtr->blt_table_iterate_row_objv) /* 94 */
#endif
#ifndef blt_table_iterate_column_objv
#define blt_table_iterate_column_objv \
	(bltTclProcsPtr->blt_table_iterate_column_objv) /* 95 */
#endif
#ifndef blt_table_free_iterator_objv
#define blt_table_free_iterator_objv \
	(bltTclProcsPtr->blt_table_free_iterator_objv) /* 96 */
#endif
#ifndef blt_table_iterate_all_rows
#define blt_table_iterate_all_rows \
	(bltTclProcsPtr->blt_table_iterate_all_rows) /* 97 */
#endif
#ifndef blt_table_iterate_all_columns
#define blt_table_iterate_all_columns \
	(bltTclProcsPtr->blt_table_iterate_all_columns) /* 98 */
#endif
#ifndef blt_table_first_tagged_row
#define blt_table_first_tagged_row \
	(bltTclProcsPtr->blt_table_first_tagged_row) /* 99 */
#endif
#ifndef blt_table_first_tagged_column
#define blt_table_first_tagged_column \
	(bltTclProcsPtr->blt_table_first_tagged_column) /* 100 */
#endif
#ifndef blt_table_next_tagged_row
#define blt_table_next_tagged_row \
	(bltTclProcsPtr->blt_table_next_tagged_row) /* 101 */
#endif
#ifndef blt_table_next_tagged_column
#define blt_table_next_tagged_column \
	(bltTclProcsPtr->blt_table_next_tagged_column) /* 102 */
#endif
#ifndef blt_table_get_row
#define blt_table_get_row \
	(bltTclProcsPtr->blt_table_get_row) /* 103 */
#endif
#ifndef blt_table_get_column
#define blt_table_get_column \
	(bltTclProcsPtr->blt_table_get_column) /* 104 */
#endif
#ifndef blt_table_list_rows
#define blt_table_list_rows \
	(bltTclProcsPtr->blt_table_list_rows) /* 105 */
#endif
#ifndef blt_table_list_columns
#define blt_table_list_columns \
	(bltTclProcsPtr->blt_table_list_columns) /* 106 */
#endif
#ifndef blt_table_clear_row_tags
#define blt_table_clear_row_tags \
	(bltTclProcsPtr->blt_table_clear_row_tags) /* 107 */
#endif
#ifndef blt_table_clear_column_tags
#define blt_table_clear_column_tags \
	(bltTclProcsPtr->blt_table_clear_column_tags) /* 108 */
#endif
#ifndef blt_table_clear_row_traces
#define blt_table_clear_row_traces \
	(bltTclProcsPtr->blt_table_clear_row_traces) /* 109 */
#endif
#ifndef blt_table_clear_column_traces
#define blt_table_clear_column_traces \
	(bltTclProcsPtr->blt_table_clear_column_traces) /* 110 */
#endif
#ifndef blt_table_create_trace
#define blt_table_create_trace \
	(bltTclProcsPtr->blt_table_create_trace) /* 111 */
#endif
#ifndef blt_table_set_column_trace
#define blt_table_set_column_trace \
	(bltTclProcsPtr->blt_table_set_column_trace) /* 112 */
#endif
#ifndef blt_table_create_column_tag_trace
#define blt_table_create_column_tag_trace \
	(bltTclProcsPtr->blt_table_create_column_tag_trace) /* 113 */
#endif
#ifndef blt_table_create_row_trace
#define blt_table_create_row_trace \
	(bltTclProcsPtr->blt_table_create_row_trace) /* 114 */
#endif
#ifndef blt_table_create_row_tag_trace
#define blt_table_create_row_tag_trace \
	(bltTclProcsPtr->blt_table_create_row_tag_trace) /* 115 */
#endif
#ifndef blt_table_delete_trace
#define blt_table_delete_trace \
	(bltTclProcsPtr->blt_table_delete_trace) /* 116 */
#endif
#ifndef blt_table_create_row_notifier
#define blt_table_create_row_notifier \
	(bltTclProcsPtr->blt_table_create_row_notifier) /* 117 */
#endif
#ifndef blt_table_create_row_tag_notifier
#define blt_table_create_row_tag_notifier \
	(bltTclProcsPtr->blt_table_create_row_tag_notifier) /* 118 */
#endif
#ifndef blt_table_create_column_notifier
#define blt_table_create_column_notifier \
	(bltTclProcsPtr->blt_table_create_column_notifier) /* 119 */
#endif
#ifndef blt_table_create_column_tag_notifier
#define blt_table_create_column_tag_notifier \
	(bltTclProcsPtr->blt_table_create_column_tag_notifier) /* 120 */
#endif
#ifndef blt_table_delete_notifier
#define blt_table_delete_notifier \
	(bltTclProcsPtr->blt_table_delete_notifier) /* 121 */
#endif
#ifndef blt_table_sort_init
#define blt_table_sort_init \
	(bltTclProcsPtr->blt_table_sort_init) /* 122 */
#endif
#ifndef blt_table_sort_rows
#define blt_table_sort_rows \
	(bltTclProcsPtr->blt_table_sort_rows) /* 123 */
#endif
#ifndef blt_table_sort_rows_subset
#define blt_table_sort_rows_subset \
	(bltTclProcsPtr->blt_table_sort_rows_subset) /* 124 */
#endif
#ifndef blt_table_sort_finish
#define blt_table_sort_finish \
	(bltTclProcsPtr->blt_table_sort_finish) /* 125 */
#endif
#ifndef blt_table_get_compare_proc
#define blt_table_get_compare_proc \
	(bltTclProcsPtr->blt_table_get_compare_proc) /* 126 */
#endif
#ifndef blt_table_get_row_map
#define blt_table_get_row_map \
	(bltTclProcsPtr->blt_table_get_row_map) /* 127 */
#endif
#ifndef blt_table_get_column_map
#define blt_table_get_column_map \
	(bltTclProcsPtr->blt_table_get_column_map) /* 128 */
#endif
#ifndef blt_table_set_row_map
#define blt_table_set_row_map \
	(bltTclProcsPtr->blt_table_set_row_map) /* 129 */
#endif
#ifndef blt_table_set_column_map
#define blt_table_set_column_map \
	(bltTclProcsPtr->blt_table_set_column_map) /* 130 */
#endif
#ifndef blt_table_restore
#define blt_table_restore \
	(bltTclProcsPtr->blt_table_restore) /* 131 */
#endif
#ifndef blt_table_file_restore
#define blt_table_file_restore \
	(bltTclProcsPtr->blt_table_file_restore) /* 132 */
#endif
#ifndef blt_table_register_format
#define blt_table_register_format \
	(bltTclProcsPtr->blt_table_register_format) /* 133 */
#endif
#ifndef blt_table_unset_keys
#define blt_table_unset_keys \
	(bltTclProcsPtr->blt_table_unset_keys) /* 134 */
#endif
#ifndef blt_table_get_keys
#define blt_table_get_keys \
	(bltTclProcsPtr->blt_table_get_keys) /* 135 */
#endif
#ifndef blt_table_set_keys
#define blt_table_set_keys \
	(bltTclProcsPtr->blt_table_set_keys) /* 136 */
#endif
#ifndef blt_table_key_lookup
#define blt_table_key_lookup \
	(bltTclProcsPtr->blt_table_key_lookup) /* 137 */
#endif
#ifndef blt_table_get_column_limits
#define blt_table_get_column_limits \
	(bltTclProcsPtr->blt_table_get_column_limits) /* 138 */
#endif
#ifndef Blt_Pool_Create
#define Blt_Pool_Create \
	(bltTclProcsPtr->blt_Pool_Create) /* 139 */
#endif
#ifndef Blt_Pool_Destroy
#define Blt_Pool_Destroy \
	(bltTclProcsPtr->blt_Pool_Destroy) /* 140 */
#endif
#ifndef Blt_Tree_GetKey
#define Blt_Tree_GetKey \
	(bltTclProcsPtr->blt_Tree_GetKey) /* 141 */
#endif
#ifndef Blt_Tree_GetKeyFromNode
#define Blt_Tree_GetKeyFromNode \
	(bltTclProcsPtr->blt_Tree_GetKeyFromNode) /* 142 */
#endif
#ifndef Blt_Tree_GetKeyFromInterp
#define Blt_Tree_GetKeyFromInterp \
	(bltTclProcsPtr->blt_Tree_GetKeyFromInterp) /* 143 */
#endif
#ifndef Blt_Tree_CreateNode
#define Blt_Tree_CreateNode \
	(bltTclProcsPtr->blt_Tree_CreateNode) /* 144 */
#endif
#ifndef Blt_Tree_CreateNodeWithId
#define Blt_Tree_CreateNodeWithId \
	(bltTclProcsPtr->blt_Tree_CreateNodeWithId) /* 145 */
#endif
#ifndef Blt_Tree_DeleteNode
#define Blt_Tree_DeleteNode \
	(bltTclProcsPtr->blt_Tree_DeleteNode) /* 146 */
#endif
#ifndef Blt_Tree_MoveNode
#define Blt_Tree_MoveNode \
	(bltTclProcsPtr->blt_Tree_MoveNode) /* 147 */
#endif
#ifndef Blt_Tree_GetNode
#define Blt_Tree_GetNode \
	(bltTclProcsPtr->blt_Tree_GetNode) /* 148 */
#endif
#ifndef Blt_Tree_FindChild
#define Blt_Tree_FindChild \
	(bltTclProcsPtr->blt_Tree_FindChild) /* 149 */
#endif
#ifndef Blt_Tree_NextNode
#define Blt_Tree_NextNode \
	(bltTclProcsPtr->blt_Tree_NextNode) /* 150 */
#endif
#ifndef Blt_Tree_PrevNode
#define Blt_Tree_PrevNode \
	(bltTclProcsPtr->blt_Tree_PrevNode) /* 151 */
#endif
#ifndef Blt_Tree_FirstChild
#define Blt_Tree_FirstChild \
	(bltTclProcsPtr->blt_Tree_FirstChild) /* 152 */
#endif
#ifndef Blt_Tree_LastChild
#define Blt_Tree_LastChild \
	(bltTclProcsPtr->blt_Tree_LastChild) /* 153 */
#endif
#ifndef Blt_Tree_IsBefore
#define Blt_Tree_IsBefore \
	(bltTclProcsPtr->blt_Tree_IsBefore) /* 154 */
#endif
#ifndef Blt_Tree_IsAncestor
#define Blt_Tree_IsAncestor \
	(bltTclProcsPtr->blt_Tree_IsAncestor) /* 155 */
#endif
#ifndef Blt_Tree_PrivateValue
#define Blt_Tree_PrivateValue \
	(bltTclProcsPtr->blt_Tree_PrivateValue) /* 156 */
#endif
#ifndef Blt_Tree_PublicValue
#define Blt_Tree_PublicValue \
	(bltTclProcsPtr->blt_Tree_PublicValue) /* 157 */
#endif
#ifndef Blt_Tree_GetValue
#define Blt_Tree_GetValue \
	(bltTclProcsPtr->blt_Tree_GetValue) /* 158 */
#endif
#ifndef Blt_Tree_ValueExists
#define Blt_Tree_ValueExists \
	(bltTclProcsPtr->blt_Tree_ValueExists) /* 159 */
#endif
#ifndef Blt_Tree_SetValue
#define Blt_Tree_SetValue \
	(bltTclProcsPtr->blt_Tree_SetValue) /* 160 */
#endif
#ifndef Blt_Tree_UnsetValue
#define Blt_Tree_UnsetValue \
	(bltTclProcsPtr->blt_Tree_UnsetValue) /* 161 */
#endif
#ifndef Blt_Tree_AppendValue
#define Blt_Tree_AppendValue \
	(bltTclProcsPtr->blt_Tree_AppendValue) /* 162 */
#endif
#ifndef Blt_Tree_ListAppendValue
#define Blt_Tree_ListAppendValue \
	(bltTclProcsPtr->blt_Tree_ListAppendValue) /* 163 */
#endif
#ifndef Blt_Tree_GetArrayValue
#define Blt_Tree_GetArrayValue \
	(bltTclProcsPtr->blt_Tree_GetArrayValue) /* 164 */
#endif
#ifndef Blt_Tree_SetArrayValue
#define Blt_Tree_SetArrayValue \
	(bltTclProcsPtr->blt_Tree_SetArrayValue) /* 165 */
#endif
#ifndef Blt_Tree_UnsetArrayValue
#define Blt_Tree_UnsetArrayValue \
	(bltTclProcsPtr->blt_Tree_UnsetArrayValue) /* 166 */
#endif
#ifndef Blt_Tree_AppendArrayValue
#define Blt_Tree_AppendArrayValue \
	(bltTclProcsPtr->blt_Tree_AppendArrayValue) /* 167 */
#endif
#ifndef Blt_Tree_ListAppendArrayValue
#define Blt_Tree_ListAppendArrayValue \
	(bltTclProcsPtr->blt_Tree_ListAppendArrayValue) /* 168 */
#endif
#ifndef Blt_Tree_ArrayValueExists
#define Blt_Tree_ArrayValueExists \
	(bltTclProcsPtr->blt_Tree_ArrayValueExists) /* 169 */
#endif
#ifndef Blt_Tree_ArrayNames
#define Blt_Tree_ArrayNames \
	(bltTclProcsPtr->blt_Tree_ArrayNames) /* 170 */
#endif
#ifndef Blt_Tree_GetValueByKey
#define Blt_Tree_GetValueByKey \
	(bltTclProcsPtr->blt_Tree_GetValueByKey) /* 171 */
#endif
#ifndef Blt_Tree_SetValueByKey
#define Blt_Tree_SetValueByKey \
	(bltTclProcsPtr->blt_Tree_SetValueByKey) /* 172 */
#endif
#ifndef Blt_Tree_UnsetValueByKey
#define Blt_Tree_UnsetValueByKey \
	(bltTclProcsPtr->blt_Tree_UnsetValueByKey) /* 173 */
#endif
#ifndef Blt_Tree_AppendValueByKey
#define Blt_Tree_AppendValueByKey \
	(bltTclProcsPtr->blt_Tree_AppendValueByKey) /* 174 */
#endif
#ifndef Blt_Tree_ListAppendValueByKey
#define Blt_Tree_ListAppendValueByKey \
	(bltTclProcsPtr->blt_Tree_ListAppendValueByKey) /* 175 */
#endif
#ifndef Blt_Tree_ValueExistsByKey
#define Blt_Tree_ValueExistsByKey \
	(bltTclProcsPtr->blt_Tree_ValueExistsByKey) /* 176 */
#endif
#ifndef Blt_Tree_FirstKey
#define Blt_Tree_FirstKey \
	(bltTclProcsPtr->blt_Tree_FirstKey) /* 177 */
#endif
#ifndef Blt_Tree_NextKey
#define Blt_Tree_NextKey \
	(bltTclProcsPtr->blt_Tree_NextKey) /* 178 */
#endif
#ifndef Blt_Tree_Apply
#define Blt_Tree_Apply \
	(bltTclProcsPtr->blt_Tree_Apply) /* 179 */
#endif
#ifndef Blt_Tree_ApplyDFS
#define Blt_Tree_ApplyDFS \
	(bltTclProcsPtr->blt_Tree_ApplyDFS) /* 180 */
#endif
#ifndef Blt_Tree_ApplyBFS
#define Blt_Tree_ApplyBFS \
	(bltTclProcsPtr->blt_Tree_ApplyBFS) /* 181 */
#endif
#ifndef Blt_Tree_SortNode
#define Blt_Tree_SortNode \
	(bltTclProcsPtr->blt_Tree_SortNode) /* 182 */
#endif
#ifndef Blt_Tree_Exists
#define Blt_Tree_Exists \
	(bltTclProcsPtr->blt_Tree_Exists) /* 183 */
#endif
#ifndef Blt_Tree_Open
#define Blt_Tree_Open \
	(bltTclProcsPtr->blt_Tree_Open) /* 184 */
#endif
#ifndef Blt_Tree_Close
#define Blt_Tree_Close \
	(bltTclProcsPtr->blt_Tree_Close) /* 185 */
#endif
#ifndef Blt_Tree_Attach
#define Blt_Tree_Attach \
	(bltTclProcsPtr->blt_Tree_Attach) /* 186 */
#endif
#ifndef Blt_Tree_GetFromObj
#define Blt_Tree_GetFromObj \
	(bltTclProcsPtr->blt_Tree_GetFromObj) /* 187 */
#endif
#ifndef Blt_Tree_Size
#define Blt_Tree_Size \
	(bltTclProcsPtr->blt_Tree_Size) /* 188 */
#endif
#ifndef Blt_Tree_CreateTrace
#define Blt_Tree_CreateTrace \
	(bltTclProcsPtr->blt_Tree_CreateTrace) /* 189 */
#endif
#ifndef Blt_Tree_DeleteTrace
#define Blt_Tree_DeleteTrace \
	(bltTclProcsPtr->blt_Tree_DeleteTrace) /* 190 */
#endif
#ifndef Blt_Tree_CreateEventHandler
#define Blt_Tree_CreateEventHandler \
	(bltTclProcsPtr->blt_Tree_CreateEventHandler) /* 191 */
#endif
#ifndef Blt_Tree_DeleteEventHandler
#define Blt_Tree_DeleteEventHandler \
	(bltTclProcsPtr->blt_Tree_DeleteEventHandler) /* 192 */
#endif
#ifndef Blt_Tree_RelabelNode
#define Blt_Tree_RelabelNode \
	(bltTclProcsPtr->blt_Tree_RelabelNode) /* 193 */
#endif
#ifndef Blt_Tree_RelabelNodeWithoutNotify
#define Blt_Tree_RelabelNodeWithoutNotify \
	(bltTclProcsPtr->blt_Tree_RelabelNodeWithoutNotify) /* 194 */
#endif
#ifndef Blt_Tree_NodeIdAscii
#define Blt_Tree_NodeIdAscii \
	(bltTclProcsPtr->blt_Tree_NodeIdAscii) /* 195 */
#endif
#ifndef Blt_Tree_NodePath
#define Blt_Tree_NodePath \
	(bltTclProcsPtr->blt_Tree_NodePath) /* 196 */
#endif
#ifndef Blt_Tree_NodeRelativePath
#define Blt_Tree_NodeRelativePath \
	(bltTclProcsPtr->blt_Tree_NodeRelativePath) /* 197 */
#endif
#ifndef Blt_Tree_NodePosition
#define Blt_Tree_NodePosition \
	(bltTclProcsPtr->blt_Tree_NodePosition) /* 198 */
#endif
#ifndef Blt_Tree_ClearTags
#define Blt_Tree_ClearTags \
	(bltTclProcsPtr->blt_Tree_ClearTags) /* 199 */
#endif
#ifndef Blt_Tree_HasTag
#define Blt_Tree_HasTag \
	(bltTclProcsPtr->blt_Tree_HasTag) /* 200 */
#endif
#ifndef Blt_Tree_AddTag
#define Blt_Tree_AddTag \
	(bltTclProcsPtr->blt_Tree_AddTag) /* 201 */
#endif
#ifndef Blt_Tree_RemoveTag
#define Blt_Tree_RemoveTag \
	(bltTclProcsPtr->blt_Tree_RemoveTag) /* 202 */
#endif
#ifndef Blt_Tree_ForgetTag
#define Blt_Tree_ForgetTag \
	(bltTclProcsPtr->blt_Tree_ForgetTag) /* 203 */
#endif
#ifndef Blt_Tree_TagHashTable
#define Blt_Tree_TagHashTable \
	(bltTclProcsPtr->blt_Tree_TagHashTable) /* 204 */
#endif
#ifndef Blt_Tree_TagTableIsShared
#define Blt_Tree_TagTableIsShared \
	(bltTclProcsPtr->blt_Tree_TagTableIsShared) /* 205 */
#endif
#ifndef Blt_Tree_NewTagTable
#define Blt_Tree_NewTagTable \
	(bltTclProcsPtr->blt_Tree_NewTagTable) /* 206 */
#endif
#ifndef Blt_Tree_FirstTag
#define Blt_Tree_FirstTag \
	(bltTclProcsPtr->blt_Tree_FirstTag) /* 207 */
#endif
#ifndef Blt_Tree_DumpNode
#define Blt_Tree_DumpNode \
	(bltTclProcsPtr->blt_Tree_DumpNode) /* 208 */
#endif
#ifndef Blt_Tree_Dump
#define Blt_Tree_Dump \
	(bltTclProcsPtr->blt_Tree_Dump) /* 209 */
#endif
#ifndef Blt_Tree_DumpToFile
#define Blt_Tree_DumpToFile \
	(bltTclProcsPtr->blt_Tree_DumpToFile) /* 210 */
#endif
#ifndef Blt_Tree_Restore
#define Blt_Tree_Restore \
	(bltTclProcsPtr->blt_Tree_Restore) /* 211 */
#endif
#ifndef Blt_Tree_RestoreFromFile
#define Blt_Tree_RestoreFromFile \
	(bltTclProcsPtr->blt_Tree_RestoreFromFile) /* 212 */
#endif
#ifndef Blt_Tree_Depth
#define Blt_Tree_Depth \
	(bltTclProcsPtr->blt_Tree_Depth) /* 213 */
#endif
#ifndef Blt_Tree_RegisterFormat
#define Blt_Tree_RegisterFormat \
	(bltTclProcsPtr->blt_Tree_RegisterFormat) /* 214 */
#endif
#ifndef Blt_Tree_RememberTag
#define Blt_Tree_RememberTag \
	(bltTclProcsPtr->blt_Tree_RememberTag) /* 215 */
#endif
#ifndef Blt_InitHashTable
#define Blt_InitHashTable \
	(bltTclProcsPtr->blt_InitHashTable) /* 216 */
#endif
#ifndef Blt_InitHashTableWithPool
#define Blt_InitHashTableWithPool \
	(bltTclProcsPtr->blt_InitHashTableWithPool) /* 217 */
#endif
#ifndef Blt_DeleteHashTable
#define Blt_DeleteHashTable \
	(bltTclProcsPtr->blt_DeleteHashTable) /* 218 */
#endif
#ifndef Blt_DeleteHashEntry
#define Blt_DeleteHashEntry \
	(bltTclProcsPtr->blt_DeleteHashEntry) /* 219 */
#endif
#ifndef Blt_FirstHashEntry
#define Blt_FirstHashEntry \
	(bltTclProcsPtr->blt_FirstHashEntry) /* 220 */
#endif
#ifndef Blt_NextHashEntry
#define Blt_NextHashEntry \
	(bltTclProcsPtr->blt_NextHashEntry) /* 221 */
#endif
#ifndef Blt_HashStats
#define Blt_HashStats \
	(bltTclProcsPtr->blt_HashStats) /* 222 */
#endif
#ifndef Blt_VecMin
#define Blt_VecMin \
	(bltTclProcsPtr->blt_VecMin) /* 223 */
#endif
#ifndef Blt_VecMax
#define Blt_VecMax \
	(bltTclProcsPtr->blt_VecMax) /* 224 */
#endif
#ifndef Blt_AllocVectorId
#define Blt_AllocVectorId \
	(bltTclProcsPtr->blt_AllocVectorId) /* 225 */
#endif
#ifndef Blt_SetVectorChangedProc
#define Blt_SetVectorChangedProc \
	(bltTclProcsPtr->blt_SetVectorChangedProc) /* 226 */
#endif
#ifndef Blt_FreeVectorId
#define Blt_FreeVectorId \
	(bltTclProcsPtr->blt_FreeVectorId) /* 227 */
#endif
#ifndef Blt_GetVectorById
#define Blt_GetVectorById \
	(bltTclProcsPtr->blt_GetVectorById) /* 228 */
#endif
#ifndef Blt_NameOfVectorId
#define Blt_NameOfVectorId \
	(bltTclProcsPtr->blt_NameOfVectorId) /* 229 */
#endif
#ifndef Blt_NameOfVector
#define Blt_NameOfVector \
	(bltTclProcsPtr->blt_NameOfVector) /* 230 */
#endif
#ifndef Blt_VectorNotifyPending
#define Blt_VectorNotifyPending \
	(bltTclProcsPtr->blt_VectorNotifyPending) /* 231 */
#endif
#ifndef Blt_CreateVector
#define Blt_CreateVector \
	(bltTclProcsPtr->blt_CreateVector) /* 232 */
#endif
#ifndef Blt_CreateVector2
#define Blt_CreateVector2 \
	(bltTclProcsPtr->blt_CreateVector2) /* 233 */
#endif
#ifndef Blt_GetVector
#define Blt_GetVector \
	(bltTclProcsPtr->blt_GetVector) /* 234 */
#endif
#ifndef Blt_GetVectorFromObj
#define Blt_GetVectorFromObj \
	(bltTclProcsPtr->blt_GetVectorFromObj) /* 235 */
#endif
#ifndef Blt_VectorExists
#define Blt_VectorExists \
	(bltTclProcsPtr->blt_VectorExists) /* 236 */
#endif
#ifndef Blt_ResetVector
#define Blt_ResetVector \
	(bltTclProcsPtr->blt_ResetVector) /* 237 */
#endif
#ifndef Blt_ResizeVector
#define Blt_ResizeVector \
	(bltTclProcsPtr->blt_ResizeVector) /* 238 */
#endif
#ifndef Blt_DeleteVectorByName
#define Blt_DeleteVectorByName \
	(bltTclProcsPtr->blt_DeleteVectorByName) /* 239 */
#endif
#ifndef Blt_DeleteVector
#define Blt_DeleteVector \
	(bltTclProcsPtr->blt_DeleteVector) /* 240 */
#endif
#ifndef Blt_ExprVector
#define Blt_ExprVector \
	(bltTclProcsPtr->blt_ExprVector) /* 241 */
#endif
#ifndef Blt_InstallIndexProc
#define Blt_InstallIndexProc \
	(bltTclProcsPtr->blt_InstallIndexProc) /* 242 */
#endif
#ifndef Blt_VectorExists2
#define Blt_VectorExists2 \
	(bltTclProcsPtr->blt_VectorExists2) /* 243 */
#endif

#endif /* defined(USE_BLT_STUBS) && !defined(BUILD_BLT_TCL_PROCS) */

/* !END!: Do not edit above this line. */
