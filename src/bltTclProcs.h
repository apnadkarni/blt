/* -*- mode: c; c-basic-offset: 4; indent-tabs-mode: nil -*- */
#include "bltAlloc.h"
#include "bltChain.h"
#include "bltDataTable.h"
#include "bltHash.h.in"
#include "bltList.h"
#include "bltPool.h"
#include "bltTags.h"
#include "bltTree.h"
#include "bltVector.h"

/* !BEGIN!: Do not edit below this line. */

/*
 * Exported function declarations:
 */

/* Slot 0 is reserved */
#ifndef Blt_AllocInit_DECLARED
#define Blt_AllocInit_DECLARED
/* 1 */
BLT_EXTERN void         Blt_AllocInit(Blt_MallocProc *mallocProc,
                                Blt_ReallocProc *reallocProc,
                                Blt_FreeProc *freeProc);
#endif
#ifndef Blt_Chain_Init_DECLARED
#define Blt_Chain_Init_DECLARED
/* 2 */
BLT_EXTERN void         Blt_Chain_Init(Blt_Chain chain);
#endif
#ifndef Blt_Chain_Create_DECLARED
#define Blt_Chain_Create_DECLARED
/* 3 */
BLT_EXTERN Blt_Chain    Blt_Chain_Create(void );
#endif
#ifndef Blt_Chain_Destroy_DECLARED
#define Blt_Chain_Destroy_DECLARED
/* 4 */
BLT_EXTERN void         Blt_Chain_Destroy(Blt_Chain chain);
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
BLT_EXTERN void         Blt_Chain_Reset(Blt_Chain chain);
#endif
#ifndef Blt_Chain_InitLink_DECLARED
#define Blt_Chain_InitLink_DECLARED
/* 10 */
BLT_EXTERN void         Blt_Chain_InitLink(Blt_ChainLink link);
#endif
#ifndef Blt_Chain_LinkAfter_DECLARED
#define Blt_Chain_LinkAfter_DECLARED
/* 11 */
BLT_EXTERN void         Blt_Chain_LinkAfter(Blt_Chain chain,
                                Blt_ChainLink link, Blt_ChainLink after);
#endif
#ifndef Blt_Chain_LinkBefore_DECLARED
#define Blt_Chain_LinkBefore_DECLARED
/* 12 */
BLT_EXTERN void         Blt_Chain_LinkBefore(Blt_Chain chain,
                                Blt_ChainLink link, Blt_ChainLink before);
#endif
#ifndef Blt_Chain_UnlinkLink_DECLARED
#define Blt_Chain_UnlinkLink_DECLARED
/* 13 */
BLT_EXTERN void         Blt_Chain_UnlinkLink(Blt_Chain chain,
                                Blt_ChainLink link);
#endif
#ifndef Blt_Chain_DeleteLink_DECLARED
#define Blt_Chain_DeleteLink_DECLARED
/* 14 */
BLT_EXTERN void         Blt_Chain_DeleteLink(Blt_Chain chain,
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
BLT_EXTERN void         Blt_Chain_Sort(Blt_Chain chain,
                                Blt_ChainCompareProc *proc);
#endif
#ifndef Blt_Chain_Reverse_DECLARED
#define Blt_Chain_Reverse_DECLARED
/* 17 */
BLT_EXTERN void         Blt_Chain_Reverse(Blt_Chain chain);
#endif
#ifndef Blt_Chain_IsBefore_DECLARED
#define Blt_Chain_IsBefore_DECLARED
/* 18 */
BLT_EXTERN int          Blt_Chain_IsBefore(Blt_ChainLink first,
                                Blt_ChainLink last);
#endif
#ifndef blt_table_release_tags_DECLARED
#define blt_table_release_tags_DECLARED
/* 19 */
BLT_EXTERN void         blt_table_release_tags(BLT_TABLE table);
#endif
#ifndef blt_table_new_tags_DECLARED
#define blt_table_new_tags_DECLARED
/* 20 */
BLT_EXTERN void         blt_table_new_tags(BLT_TABLE table);
#endif
#ifndef blt_table_get_column_tag_table_DECLARED
#define blt_table_get_column_tag_table_DECLARED
/* 21 */
BLT_EXTERN Blt_HashTable * blt_table_get_column_tag_table(BLT_TABLE table);
#endif
#ifndef blt_table_get_row_tag_table_DECLARED
#define blt_table_get_row_tag_table_DECLARED
/* 22 */
BLT_EXTERN Blt_HashTable * blt_table_get_row_tag_table(BLT_TABLE table);
#endif
#ifndef blt_table_exists_DECLARED
#define blt_table_exists_DECLARED
/* 23 */
BLT_EXTERN int          blt_table_exists(Tcl_Interp *interp,
                                const char *name);
#endif
#ifndef blt_table_create_DECLARED
#define blt_table_create_DECLARED
/* 24 */
BLT_EXTERN int          blt_table_create(Tcl_Interp *interp,
                                const char *name, BLT_TABLE *tablePtr);
#endif
#ifndef blt_table_open_DECLARED
#define blt_table_open_DECLARED
/* 25 */
BLT_EXTERN int          blt_table_open(Tcl_Interp *interp, const char *name,
                                BLT_TABLE *tablePtr);
#endif
#ifndef blt_table_close_DECLARED
#define blt_table_close_DECLARED
/* 26 */
BLT_EXTERN void         blt_table_close(BLT_TABLE table);
#endif
#ifndef blt_table_same_object_DECLARED
#define blt_table_same_object_DECLARED
/* 27 */
BLT_EXTERN int          blt_table_same_object(BLT_TABLE table1,
                                BLT_TABLE table2);
#endif
#ifndef blt_table_row_get_label_table_DECLARED
#define blt_table_row_get_label_table_DECLARED
/* 28 */
BLT_EXTERN Blt_HashTable * blt_table_row_get_label_table(BLT_TABLE table,
                                const char *label);
#endif
#ifndef blt_table_column_get_label_table_DECLARED
#define blt_table_column_get_label_table_DECLARED
/* 29 */
BLT_EXTERN Blt_HashTable * blt_table_column_get_label_table(BLT_TABLE table,
                                const char *label);
#endif
#ifndef blt_table_get_row_DECLARED
#define blt_table_get_row_DECLARED
/* 30 */
BLT_EXTERN BLT_TABLE_ROW blt_table_get_row(Tcl_Interp *interp,
                                BLT_TABLE table, Tcl_Obj *objPtr);
#endif
#ifndef blt_table_get_column_DECLARED
#define blt_table_get_column_DECLARED
/* 31 */
BLT_EXTERN BLT_TABLE_COLUMN blt_table_get_column(Tcl_Interp *interp,
                                BLT_TABLE table, Tcl_Obj *objPtr);
#endif
#ifndef blt_table_get_row_by_label_DECLARED
#define blt_table_get_row_by_label_DECLARED
/* 32 */
BLT_EXTERN BLT_TABLE_ROW blt_table_get_row_by_label(BLT_TABLE table,
                                const char *label);
#endif
#ifndef blt_table_get_column_by_label_DECLARED
#define blt_table_get_column_by_label_DECLARED
/* 33 */
BLT_EXTERN BLT_TABLE_COLUMN blt_table_get_column_by_label(BLT_TABLE table,
                                const char *label);
#endif
#ifndef blt_table_get_row_by_index_DECLARED
#define blt_table_get_row_by_index_DECLARED
/* 34 */
BLT_EXTERN BLT_TABLE_ROW blt_table_get_row_by_index(BLT_TABLE table,
                                long index);
#endif
#ifndef blt_table_get_column_by_index_DECLARED
#define blt_table_get_column_by_index_DECLARED
/* 35 */
BLT_EXTERN BLT_TABLE_COLUMN blt_table_get_column_by_index(BLT_TABLE table,
                                long index);
#endif
#ifndef blt_table_set_row_label_DECLARED
#define blt_table_set_row_label_DECLARED
/* 36 */
BLT_EXTERN int          blt_table_set_row_label(Tcl_Interp *interp,
                                BLT_TABLE table, BLT_TABLE_ROW row,
                                const char *label);
#endif
#ifndef blt_table_set_column_label_DECLARED
#define blt_table_set_column_label_DECLARED
/* 37 */
BLT_EXTERN int          blt_table_set_column_label(Tcl_Interp *interp,
                                BLT_TABLE table, BLT_TABLE_COLUMN column,
                                const char *label);
#endif
#ifndef blt_table_name_to_column_type_DECLARED
#define blt_table_name_to_column_type_DECLARED
/* 38 */
BLT_EXTERN BLT_TABLE_COLUMN_TYPE blt_table_name_to_column_type(
                                const char *typeName);
#endif
#ifndef blt_table_set_column_type_DECLARED
#define blt_table_set_column_type_DECLARED
/* 39 */
BLT_EXTERN int          blt_table_set_column_type(Tcl_Interp *interp,
                                BLT_TABLE table, BLT_TABLE_COLUMN column,
                                BLT_TABLE_COLUMN_TYPE type);
#endif
#ifndef blt_table_column_type_to_name_DECLARED
#define blt_table_column_type_to_name_DECLARED
/* 40 */
BLT_EXTERN const char *  blt_table_column_type_to_name(
                                BLT_TABLE_COLUMN_TYPE type);
#endif
#ifndef blt_table_set_column_tag_DECLARED
#define blt_table_set_column_tag_DECLARED
/* 41 */
BLT_EXTERN int          blt_table_set_column_tag(Tcl_Interp *interp,
                                BLT_TABLE table, BLT_TABLE_COLUMN column,
                                const char *tag);
#endif
#ifndef blt_table_set_row_tag_DECLARED
#define blt_table_set_row_tag_DECLARED
/* 42 */
BLT_EXTERN int          blt_table_set_row_tag(Tcl_Interp *interp,
                                BLT_TABLE table, BLT_TABLE_ROW row,
                                const char *tag);
#endif
#ifndef blt_table_create_row_DECLARED
#define blt_table_create_row_DECLARED
/* 43 */
BLT_EXTERN BLT_TABLE_ROW blt_table_create_row(Tcl_Interp *interp,
                                BLT_TABLE table, const char *label);
#endif
#ifndef blt_table_create_column_DECLARED
#define blt_table_create_column_DECLARED
/* 44 */
BLT_EXTERN BLT_TABLE_COLUMN blt_table_create_column(Tcl_Interp *interp,
                                BLT_TABLE table, const char *label);
#endif
#ifndef blt_table_extend_rows_DECLARED
#define blt_table_extend_rows_DECLARED
/* 45 */
BLT_EXTERN int          blt_table_extend_rows(Tcl_Interp *interp,
                                BLT_TABLE table, size_t n,
                                BLT_TABLE_ROW *rows);
#endif
#ifndef blt_table_extend_columns_DECLARED
#define blt_table_extend_columns_DECLARED
/* 46 */
BLT_EXTERN int          blt_table_extend_columns(Tcl_Interp *interp,
                                BLT_TABLE table, size_t n,
                                BLT_TABLE_COLUMN *columms);
#endif
#ifndef blt_table_delete_row_DECLARED
#define blt_table_delete_row_DECLARED
/* 47 */
BLT_EXTERN int          blt_table_delete_row(BLT_TABLE table,
                                BLT_TABLE_ROW row);
#endif
#ifndef blt_table_delete_column_DECLARED
#define blt_table_delete_column_DECLARED
/* 48 */
BLT_EXTERN int          blt_table_delete_column(BLT_TABLE table,
                                BLT_TABLE_COLUMN column);
#endif
#ifndef blt_table_move_row_DECLARED
#define blt_table_move_row_DECLARED
/* 49 */
BLT_EXTERN int          blt_table_move_row(Tcl_Interp *interp,
                                BLT_TABLE table, BLT_TABLE_ROW from,
                                BLT_TABLE_ROW to, size_t n);
#endif
#ifndef blt_table_move_column_DECLARED
#define blt_table_move_column_DECLARED
/* 50 */
BLT_EXTERN int          blt_table_move_column(Tcl_Interp *interp,
                                BLT_TABLE table, BLT_TABLE_COLUMN from,
                                BLT_TABLE_COLUMN to, size_t n);
#endif
#ifndef blt_table_get_obj_DECLARED
#define blt_table_get_obj_DECLARED
/* 51 */
BLT_EXTERN Tcl_Obj *    blt_table_get_obj(BLT_TABLE table, BLT_TABLE_ROW row,
                                BLT_TABLE_COLUMN column);
#endif
#ifndef blt_table_set_obj_DECLARED
#define blt_table_set_obj_DECLARED
/* 52 */
BLT_EXTERN int          blt_table_set_obj(Tcl_Interp *interp,
                                BLT_TABLE table, BLT_TABLE_ROW row,
                                BLT_TABLE_COLUMN column, Tcl_Obj *objPtr);
#endif
#ifndef blt_table_get_string_DECLARED
#define blt_table_get_string_DECLARED
/* 53 */
BLT_EXTERN const char *  blt_table_get_string(BLT_TABLE table,
                                BLT_TABLE_ROW row, BLT_TABLE_COLUMN column);
#endif
#ifndef blt_table_set_string_rep_DECLARED
#define blt_table_set_string_rep_DECLARED
/* 54 */
BLT_EXTERN int          blt_table_set_string_rep(Tcl_Interp *interp,
                                BLT_TABLE table, BLT_TABLE_ROW row,
                                BLT_TABLE_COLUMN column, const char *string,
                                int length);
#endif
#ifndef blt_table_set_string_DECLARED
#define blt_table_set_string_DECLARED
/* 55 */
BLT_EXTERN int          blt_table_set_string(Tcl_Interp *interp,
                                BLT_TABLE table, BLT_TABLE_ROW row,
                                BLT_TABLE_COLUMN column, const char *string,
                                int length);
#endif
#ifndef blt_table_append_string_DECLARED
#define blt_table_append_string_DECLARED
/* 56 */
BLT_EXTERN int          blt_table_append_string(Tcl_Interp *interp,
                                BLT_TABLE table, BLT_TABLE_ROW row,
                                BLT_TABLE_COLUMN column, const char *string,
                                int length);
#endif
#ifndef blt_table_set_bytes_DECLARED
#define blt_table_set_bytes_DECLARED
/* 57 */
BLT_EXTERN int          blt_table_set_bytes(Tcl_Interp *interp,
                                BLT_TABLE table, BLT_TABLE_ROW row,
                                BLT_TABLE_COLUMN column,
                                const unsigned char *string, int length);
#endif
#ifndef blt_table_get_double_DECLARED
#define blt_table_get_double_DECLARED
/* 58 */
BLT_EXTERN double       blt_table_get_double(Tcl_Interp *interp,
                                BLT_TABLE table, BLT_TABLE_ROW row,
                                BLT_TABLE_COLUMN column);
#endif
#ifndef blt_table_set_double_DECLARED
#define blt_table_set_double_DECLARED
/* 59 */
BLT_EXTERN int          blt_table_set_double(Tcl_Interp *interp,
                                BLT_TABLE table, BLT_TABLE_ROW row,
                                BLT_TABLE_COLUMN column, double value);
#endif
#ifndef blt_table_get_long_DECLARED
#define blt_table_get_long_DECLARED
/* 60 */
BLT_EXTERN long         blt_table_get_long(Tcl_Interp *interp,
                                BLT_TABLE table, BLT_TABLE_ROW row,
                                BLT_TABLE_COLUMN column, long defValue);
#endif
#ifndef blt_table_set_long_DECLARED
#define blt_table_set_long_DECLARED
/* 61 */
BLT_EXTERN int          blt_table_set_long(Tcl_Interp *interp,
                                BLT_TABLE table, BLT_TABLE_ROW row,
                                BLT_TABLE_COLUMN column, long value);
#endif
#ifndef blt_table_get_boolean_DECLARED
#define blt_table_get_boolean_DECLARED
/* 62 */
BLT_EXTERN int          blt_table_get_boolean(Tcl_Interp *interp,
                                BLT_TABLE table, BLT_TABLE_ROW row,
                                BLT_TABLE_COLUMN column, int defValue);
#endif
#ifndef blt_table_set_boolean_DECLARED
#define blt_table_set_boolean_DECLARED
/* 63 */
BLT_EXTERN int          blt_table_set_boolean(Tcl_Interp *interp,
                                BLT_TABLE table, BLT_TABLE_ROW row,
                                BLT_TABLE_COLUMN column, int value);
#endif
#ifndef blt_table_get_value_DECLARED
#define blt_table_get_value_DECLARED
/* 64 */
BLT_EXTERN BLT_TABLE_VALUE blt_table_get_value(BLT_TABLE table,
                                BLT_TABLE_ROW row, BLT_TABLE_COLUMN column);
#endif
#ifndef blt_table_set_value_DECLARED
#define blt_table_set_value_DECLARED
/* 65 */
BLT_EXTERN int          blt_table_set_value(BLT_TABLE table,
                                BLT_TABLE_ROW row, BLT_TABLE_COLUMN column,
                                BLT_TABLE_VALUE value);
#endif
#ifndef blt_table_unset_value_DECLARED
#define blt_table_unset_value_DECLARED
/* 66 */
BLT_EXTERN int          blt_table_unset_value(BLT_TABLE table,
                                BLT_TABLE_ROW row, BLT_TABLE_COLUMN column);
#endif
#ifndef blt_table_value_exists_DECLARED
#define blt_table_value_exists_DECLARED
/* 67 */
BLT_EXTERN int          blt_table_value_exists(BLT_TABLE table,
                                BLT_TABLE_ROW row, BLT_TABLE_COLUMN column);
#endif
#ifndef blt_table_value_string_DECLARED
#define blt_table_value_string_DECLARED
/* 68 */
BLT_EXTERN const char *  blt_table_value_string(BLT_TABLE_VALUE value);
#endif
#ifndef blt_table_value_bytes_DECLARED
#define blt_table_value_bytes_DECLARED
/* 69 */
BLT_EXTERN const unsigned char * blt_table_value_bytes(BLT_TABLE_VALUE value);
#endif
#ifndef blt_table_value_length_DECLARED
#define blt_table_value_length_DECLARED
/* 70 */
BLT_EXTERN size_t       blt_table_value_length(BLT_TABLE_VALUE value);
#endif
#ifndef blt_table_tags_are_shared_DECLARED
#define blt_table_tags_are_shared_DECLARED
/* 71 */
BLT_EXTERN int          blt_table_tags_are_shared(BLT_TABLE table);
#endif
#ifndef blt_table_clear_row_tags_DECLARED
#define blt_table_clear_row_tags_DECLARED
/* 72 */
BLT_EXTERN void         blt_table_clear_row_tags(BLT_TABLE table,
                                BLT_TABLE_ROW row);
#endif
#ifndef blt_table_clear_column_tags_DECLARED
#define blt_table_clear_column_tags_DECLARED
/* 73 */
BLT_EXTERN void         blt_table_clear_column_tags(BLT_TABLE table,
                                BLT_TABLE_COLUMN col);
#endif
#ifndef blt_table_get_row_tags_DECLARED
#define blt_table_get_row_tags_DECLARED
/* 74 */
BLT_EXTERN Blt_Chain    blt_table_get_row_tags(BLT_TABLE table,
                                BLT_TABLE_ROW row);
#endif
#ifndef blt_table_get_column_tags_DECLARED
#define blt_table_get_column_tags_DECLARED
/* 75 */
BLT_EXTERN Blt_Chain    blt_table_get_column_tags(BLT_TABLE table,
                                BLT_TABLE_COLUMN column);
#endif
#ifndef blt_table_get_tagged_rows_DECLARED
#define blt_table_get_tagged_rows_DECLARED
/* 76 */
BLT_EXTERN Blt_Chain    blt_table_get_tagged_rows(BLT_TABLE table,
                                const char *tag);
#endif
#ifndef blt_table_get_tagged_columns_DECLARED
#define blt_table_get_tagged_columns_DECLARED
/* 77 */
BLT_EXTERN Blt_Chain    blt_table_get_tagged_columns(BLT_TABLE table,
                                const char *tag);
#endif
#ifndef blt_table_row_has_tag_DECLARED
#define blt_table_row_has_tag_DECLARED
/* 78 */
BLT_EXTERN int          blt_table_row_has_tag(BLT_TABLE table,
                                BLT_TABLE_ROW row, const char *tag);
#endif
#ifndef blt_table_column_has_tag_DECLARED
#define blt_table_column_has_tag_DECLARED
/* 79 */
BLT_EXTERN int          blt_table_column_has_tag(BLT_TABLE table,
                                BLT_TABLE_COLUMN column, const char *tag);
#endif
#ifndef blt_table_forget_row_tag_DECLARED
#define blt_table_forget_row_tag_DECLARED
/* 80 */
BLT_EXTERN int          blt_table_forget_row_tag(Tcl_Interp *interp,
                                BLT_TABLE table, const char *tag);
#endif
#ifndef blt_table_forget_column_tag_DECLARED
#define blt_table_forget_column_tag_DECLARED
/* 81 */
BLT_EXTERN int          blt_table_forget_column_tag(Tcl_Interp *interp,
                                BLT_TABLE table, const char *tag);
#endif
#ifndef blt_table_unset_row_tag_DECLARED
#define blt_table_unset_row_tag_DECLARED
/* 82 */
BLT_EXTERN int          blt_table_unset_row_tag(Tcl_Interp *interp,
                                BLT_TABLE table, BLT_TABLE_ROW row,
                                const char *tag);
#endif
#ifndef blt_table_unset_column_tag_DECLARED
#define blt_table_unset_column_tag_DECLARED
/* 83 */
BLT_EXTERN int          blt_table_unset_column_tag(Tcl_Interp *interp,
                                BLT_TABLE table, BLT_TABLE_COLUMN column,
                                const char *tag);
#endif
#ifndef blt_table_first_column_DECLARED
#define blt_table_first_column_DECLARED
/* 84 */
BLT_EXTERN BLT_TABLE_COLUMN blt_table_first_column(BLT_TABLE table);
#endif
#ifndef blt_table_next_column_DECLARED
#define blt_table_next_column_DECLARED
/* 85 */
BLT_EXTERN BLT_TABLE_COLUMN blt_table_next_column(BLT_TABLE table,
                                BLT_TABLE_COLUMN column);
#endif
#ifndef blt_table_first_row_DECLARED
#define blt_table_first_row_DECLARED
/* 86 */
BLT_EXTERN BLT_TABLE_ROW blt_table_first_row(BLT_TABLE table);
#endif
#ifndef blt_table_next_row_DECLARED
#define blt_table_next_row_DECLARED
/* 87 */
BLT_EXTERN BLT_TABLE_ROW blt_table_next_row(BLT_TABLE table,
                                BLT_TABLE_ROW row);
#endif
#ifndef blt_table_row_spec_DECLARED
#define blt_table_row_spec_DECLARED
/* 88 */
BLT_EXTERN BLT_TABLE_ROWCOLUMN_SPEC blt_table_row_spec(BLT_TABLE table,
                                Tcl_Obj *objPtr, const char **sp);
#endif
#ifndef blt_table_column_spec_DECLARED
#define blt_table_column_spec_DECLARED
/* 89 */
BLT_EXTERN BLT_TABLE_ROWCOLUMN_SPEC blt_table_column_spec(BLT_TABLE table,
                                Tcl_Obj *objPtr, const char **sp);
#endif
#ifndef blt_table_iterate_rows_DECLARED
#define blt_table_iterate_rows_DECLARED
/* 90 */
BLT_EXTERN int          blt_table_iterate_rows(Tcl_Interp *interp,
                                BLT_TABLE table, Tcl_Obj *objPtr,
                                BLT_TABLE_ITERATOR *iter);
#endif
#ifndef blt_table_iterate_columns_DECLARED
#define blt_table_iterate_columns_DECLARED
/* 91 */
BLT_EXTERN int          blt_table_iterate_columns(Tcl_Interp *interp,
                                BLT_TABLE table, Tcl_Obj *objPtr,
                                BLT_TABLE_ITERATOR *iter);
#endif
#ifndef blt_table_iterate_rows_objv_DECLARED
#define blt_table_iterate_rows_objv_DECLARED
/* 92 */
BLT_EXTERN int          blt_table_iterate_rows_objv(Tcl_Interp *interp,
                                BLT_TABLE table, int objc,
                                Tcl_Obj *const *objv,
                                BLT_TABLE_ITERATOR *iterPtr);
#endif
#ifndef blt_table_iterate_columns_objv_DECLARED
#define blt_table_iterate_columns_objv_DECLARED
/* 93 */
BLT_EXTERN int          blt_table_iterate_columns_objv(Tcl_Interp *interp,
                                BLT_TABLE table, int objc,
                                Tcl_Obj *const *objv,
                                BLT_TABLE_ITERATOR *iterPtr);
#endif
#ifndef blt_table_free_iterator_objv_DECLARED
#define blt_table_free_iterator_objv_DECLARED
/* 94 */
BLT_EXTERN void         blt_table_free_iterator_objv(
                                BLT_TABLE_ITERATOR *iterPtr);
#endif
#ifndef blt_table_iterate_all_rows_DECLARED
#define blt_table_iterate_all_rows_DECLARED
/* 95 */
BLT_EXTERN void         blt_table_iterate_all_rows(BLT_TABLE table,
                                BLT_TABLE_ITERATOR *iterPtr);
#endif
#ifndef blt_table_iterate_all_columns_DECLARED
#define blt_table_iterate_all_columns_DECLARED
/* 96 */
BLT_EXTERN void         blt_table_iterate_all_columns(BLT_TABLE table,
                                BLT_TABLE_ITERATOR *iterPtr);
#endif
#ifndef blt_table_first_tagged_row_DECLARED
#define blt_table_first_tagged_row_DECLARED
/* 97 */
BLT_EXTERN BLT_TABLE_ROW blt_table_first_tagged_row(BLT_TABLE_ITERATOR *iter);
#endif
#ifndef blt_table_first_tagged_column_DECLARED
#define blt_table_first_tagged_column_DECLARED
/* 98 */
BLT_EXTERN BLT_TABLE_COLUMN blt_table_first_tagged_column(
                                BLT_TABLE_ITERATOR *iter);
#endif
#ifndef blt_table_next_tagged_row_DECLARED
#define blt_table_next_tagged_row_DECLARED
/* 99 */
BLT_EXTERN BLT_TABLE_ROW blt_table_next_tagged_row(BLT_TABLE_ITERATOR *iter);
#endif
#ifndef blt_table_next_tagged_column_DECLARED
#define blt_table_next_tagged_column_DECLARED
/* 100 */
BLT_EXTERN BLT_TABLE_COLUMN blt_table_next_tagged_column(
                                BLT_TABLE_ITERATOR *iter);
#endif
#ifndef blt_table_list_rows_DECLARED
#define blt_table_list_rows_DECLARED
/* 101 */
BLT_EXTERN int          blt_table_list_rows(Tcl_Interp *interp,
                                BLT_TABLE table, int objc,
                                Tcl_Obj *const *objv, Blt_Chain chain);
#endif
#ifndef blt_table_list_columns_DECLARED
#define blt_table_list_columns_DECLARED
/* 102 */
BLT_EXTERN int          blt_table_list_columns(Tcl_Interp *interp,
                                BLT_TABLE table, int objc,
                                Tcl_Obj *const *objv, Blt_Chain chain);
#endif
#ifndef blt_table_clear_row_traces_DECLARED
#define blt_table_clear_row_traces_DECLARED
/* 103 */
BLT_EXTERN void         blt_table_clear_row_traces(BLT_TABLE table,
                                BLT_TABLE_ROW row);
#endif
#ifndef blt_table_clear_column_traces_DECLARED
#define blt_table_clear_column_traces_DECLARED
/* 104 */
BLT_EXTERN void         blt_table_clear_column_traces(BLT_TABLE table,
                                BLT_TABLE_COLUMN column);
#endif
#ifndef blt_table_create_trace_DECLARED
#define blt_table_create_trace_DECLARED
/* 105 */
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
/* 106 */
BLT_EXTERN void         blt_table_trace_column(BLT_TABLE table,
                                BLT_TABLE_COLUMN column, unsigned int mask,
                                BLT_TABLE_TRACE_PROC *proc,
                                BLT_TABLE_TRACE_DELETE_PROC *deleteProc,
                                ClientData clientData);
#endif
#ifndef blt_table_trace_row_DECLARED
#define blt_table_trace_row_DECLARED
/* 107 */
BLT_EXTERN void         blt_table_trace_row(BLT_TABLE table,
                                BLT_TABLE_ROW row, unsigned int mask,
                                BLT_TABLE_TRACE_PROC *proc,
                                BLT_TABLE_TRACE_DELETE_PROC *deleteProc,
                                ClientData clientData);
#endif
#ifndef blt_table_create_column_trace_DECLARED
#define blt_table_create_column_trace_DECLARED
/* 108 */
BLT_EXTERN BLT_TABLE_TRACE blt_table_create_column_trace(BLT_TABLE table,
                                BLT_TABLE_COLUMN column, unsigned int mask,
                                BLT_TABLE_TRACE_PROC *proc,
                                BLT_TABLE_TRACE_DELETE_PROC *deleteProc,
                                ClientData clientData);
#endif
#ifndef blt_table_create_column_tag_trace_DECLARED
#define blt_table_create_column_tag_trace_DECLARED
/* 109 */
BLT_EXTERN BLT_TABLE_TRACE blt_table_create_column_tag_trace(BLT_TABLE table,
                                const char *tag, unsigned int mask,
                                BLT_TABLE_TRACE_PROC *proc,
                                BLT_TABLE_TRACE_DELETE_PROC *deleteProc,
                                ClientData clientData);
#endif
#ifndef blt_table_create_row_trace_DECLARED
#define blt_table_create_row_trace_DECLARED
/* 110 */
BLT_EXTERN BLT_TABLE_TRACE blt_table_create_row_trace(BLT_TABLE table,
                                BLT_TABLE_ROW row, unsigned int mask,
                                BLT_TABLE_TRACE_PROC *proc,
                                BLT_TABLE_TRACE_DELETE_PROC *deleteProc,
                                ClientData clientData);
#endif
#ifndef blt_table_create_row_tag_trace_DECLARED
#define blt_table_create_row_tag_trace_DECLARED
/* 111 */
BLT_EXTERN BLT_TABLE_TRACE blt_table_create_row_tag_trace(BLT_TABLE table,
                                const char *tag, unsigned int mask,
                                BLT_TABLE_TRACE_PROC *proc,
                                BLT_TABLE_TRACE_DELETE_PROC *deleteProc,
                                ClientData clientData);
#endif
#ifndef blt_table_delete_trace_DECLARED
#define blt_table_delete_trace_DECLARED
/* 112 */
BLT_EXTERN void         blt_table_delete_trace(BLT_TABLE table,
                                BLT_TABLE_TRACE trace);
#endif
#ifndef blt_table_create_notifier_DECLARED
#define blt_table_create_notifier_DECLARED
/* 113 */
BLT_EXTERN BLT_TABLE_NOTIFIER blt_table_create_notifier(Tcl_Interp *interp,
                                BLT_TABLE table, unsigned int mask,
                                BLT_TABLE_NOTIFY_EVENT_PROC *proc,
                                BLT_TABLE_NOTIFIER_DELETE_PROC *deleteProc,
                                ClientData clientData);
#endif
#ifndef blt_table_create_row_notifier_DECLARED
#define blt_table_create_row_notifier_DECLARED
/* 114 */
BLT_EXTERN BLT_TABLE_NOTIFIER blt_table_create_row_notifier(
                                Tcl_Interp *interp, BLT_TABLE table,
                                BLT_TABLE_ROW row, unsigned int mask,
                                BLT_TABLE_NOTIFY_EVENT_PROC *proc,
                                BLT_TABLE_NOTIFIER_DELETE_PROC *deleteProc,
                                ClientData clientData);
#endif
#ifndef blt_table_create_row_tag_notifier_DECLARED
#define blt_table_create_row_tag_notifier_DECLARED
/* 115 */
BLT_EXTERN BLT_TABLE_NOTIFIER blt_table_create_row_tag_notifier(
                                Tcl_Interp *interp, BLT_TABLE table,
                                const char *tag, unsigned int mask,
                                BLT_TABLE_NOTIFY_EVENT_PROC *proc,
                                BLT_TABLE_NOTIFIER_DELETE_PROC *deleteProc,
                                ClientData clientData);
#endif
#ifndef blt_table_create_column_notifier_DECLARED
#define blt_table_create_column_notifier_DECLARED
/* 116 */
BLT_EXTERN BLT_TABLE_NOTIFIER blt_table_create_column_notifier(
                                Tcl_Interp *interp, BLT_TABLE table,
                                BLT_TABLE_COLUMN column, unsigned int mask,
                                BLT_TABLE_NOTIFY_EVENT_PROC *proc,
                                BLT_TABLE_NOTIFIER_DELETE_PROC *deleteProc,
                                ClientData clientData);
#endif
#ifndef blt_table_create_column_tag_notifier_DECLARED
#define blt_table_create_column_tag_notifier_DECLARED
/* 117 */
BLT_EXTERN BLT_TABLE_NOTIFIER blt_table_create_column_tag_notifier(
                                Tcl_Interp *interp, BLT_TABLE table,
                                const char *tag, unsigned int mask,
                                BLT_TABLE_NOTIFY_EVENT_PROC *proc,
                                BLT_TABLE_NOTIFIER_DELETE_PROC *deleteProc,
                                ClientData clientData);
#endif
#ifndef blt_table_delete_notifier_DECLARED
#define blt_table_delete_notifier_DECLARED
/* 118 */
BLT_EXTERN void         blt_table_delete_notifier(BLT_TABLE table,
                                BLT_TABLE_NOTIFIER notifier);
#endif
#ifndef blt_table_sort_init_DECLARED
#define blt_table_sort_init_DECLARED
/* 119 */
BLT_EXTERN void         blt_table_sort_init(BLT_TABLE table,
                                BLT_TABLE_SORT_ORDER *order,
                                size_t numCompares, unsigned int flags);
#endif
#ifndef blt_table_sort_rows_DECLARED
#define blt_table_sort_rows_DECLARED
/* 120 */
BLT_EXTERN BLT_TABLE_ROW * blt_table_sort_rows(BLT_TABLE table);
#endif
#ifndef blt_table_sort_rows_subset_DECLARED
#define blt_table_sort_rows_subset_DECLARED
/* 121 */
BLT_EXTERN void         blt_table_sort_rows_subset(BLT_TABLE table,
                                long numRows, BLT_TABLE_ROW *rows);
#endif
#ifndef blt_table_sort_finish_DECLARED
#define blt_table_sort_finish_DECLARED
/* 122 */
BLT_EXTERN void         blt_table_sort_finish(void );
#endif
#ifndef blt_table_get_compare_proc_DECLARED
#define blt_table_get_compare_proc_DECLARED
/* 123 */
BLT_EXTERN BLT_TABLE_COMPARE_PROC * blt_table_get_compare_proc(
                                BLT_TABLE table, BLT_TABLE_COLUMN column,
                                unsigned int flags);
#endif
#ifndef blt_table_get_row_map_DECLARED
#define blt_table_get_row_map_DECLARED
/* 124 */
BLT_EXTERN BLT_TABLE_ROW * blt_table_get_row_map(BLT_TABLE table);
#endif
#ifndef blt_table_get_column_map_DECLARED
#define blt_table_get_column_map_DECLARED
/* 125 */
BLT_EXTERN BLT_TABLE_COLUMN * blt_table_get_column_map(BLT_TABLE table);
#endif
#ifndef blt_table_set_row_map_DECLARED
#define blt_table_set_row_map_DECLARED
/* 126 */
BLT_EXTERN void         blt_table_set_row_map(BLT_TABLE table,
                                BLT_TABLE_ROW *map);
#endif
#ifndef blt_table_set_column_map_DECLARED
#define blt_table_set_column_map_DECLARED
/* 127 */
BLT_EXTERN void         blt_table_set_column_map(BLT_TABLE table,
                                BLT_TABLE_COLUMN *map);
#endif
#ifndef blt_table_restore_DECLARED
#define blt_table_restore_DECLARED
/* 128 */
BLT_EXTERN int          blt_table_restore(Tcl_Interp *interp,
                                BLT_TABLE table, char *string,
                                unsigned int flags);
#endif
#ifndef blt_table_file_restore_DECLARED
#define blt_table_file_restore_DECLARED
/* 129 */
BLT_EXTERN int          blt_table_file_restore(Tcl_Interp *interp,
                                BLT_TABLE table, const char *fileName,
                                unsigned int flags);
#endif
#ifndef blt_table_register_format_DECLARED
#define blt_table_register_format_DECLARED
/* 130 */
BLT_EXTERN int          blt_table_register_format(Tcl_Interp *interp,
                                const char *name,
                                BLT_TABLE_IMPORT_PROC *importProc,
                                BLT_TABLE_EXPORT_PROC *exportProc);
#endif
#ifndef blt_table_unset_keys_DECLARED
#define blt_table_unset_keys_DECLARED
/* 131 */
BLT_EXTERN void         blt_table_unset_keys(BLT_TABLE table);
#endif
#ifndef blt_table_get_keys_DECLARED
#define blt_table_get_keys_DECLARED
/* 132 */
BLT_EXTERN int          blt_table_get_keys(BLT_TABLE table,
                                BLT_TABLE_COLUMN **keysPtr);
#endif
#ifndef blt_table_set_keys_DECLARED
#define blt_table_set_keys_DECLARED
/* 133 */
BLT_EXTERN int          blt_table_set_keys(BLT_TABLE table, int numKeys,
                                BLT_TABLE_COLUMN *keys, int unique);
#endif
#ifndef blt_table_key_lookup_DECLARED
#define blt_table_key_lookup_DECLARED
/* 134 */
BLT_EXTERN int          blt_table_key_lookup(Tcl_Interp *interp,
                                BLT_TABLE table, int objc,
                                Tcl_Obj *const *objv, BLT_TABLE_ROW *rowPtr);
#endif
#ifndef blt_table_get_column_limits_DECLARED
#define blt_table_get_column_limits_DECLARED
/* 135 */
BLT_EXTERN int          blt_table_get_column_limits(Tcl_Interp *interp,
                                BLT_TABLE table, BLT_TABLE_COLUMN col,
                                Tcl_Obj **minObjPtrPtr,
                                Tcl_Obj **maxObjPtrPtr);
#endif
#ifndef Blt_InitHashTable_DECLARED
#define Blt_InitHashTable_DECLARED
/* 136 */
BLT_EXTERN void         Blt_InitHashTable(Blt_HashTable *tablePtr,
                                size_t keyType);
#endif
#ifndef Blt_InitHashTableWithPool_DECLARED
#define Blt_InitHashTableWithPool_DECLARED
/* 137 */
BLT_EXTERN void         Blt_InitHashTableWithPool(Blt_HashTable *tablePtr,
                                size_t keyType);
#endif
#ifndef Blt_DeleteHashTable_DECLARED
#define Blt_DeleteHashTable_DECLARED
/* 138 */
BLT_EXTERN void         Blt_DeleteHashTable(Blt_HashTable *tablePtr);
#endif
#ifndef Blt_DeleteHashEntry_DECLARED
#define Blt_DeleteHashEntry_DECLARED
/* 139 */
BLT_EXTERN void         Blt_DeleteHashEntry(Blt_HashTable *tablePtr,
                                Blt_HashEntry *entryPtr);
#endif
#ifndef Blt_FirstHashEntry_DECLARED
#define Blt_FirstHashEntry_DECLARED
/* 140 */
BLT_EXTERN Blt_HashEntry * Blt_FirstHashEntry(Blt_HashTable *tablePtr,
                                Blt_HashSearch *searchPtr);
#endif
#ifndef Blt_NextHashEntry_DECLARED
#define Blt_NextHashEntry_DECLARED
/* 141 */
BLT_EXTERN Blt_HashEntry * Blt_NextHashEntry(Blt_HashSearch *srchPtr);
#endif
#ifndef Blt_HashStats_DECLARED
#define Blt_HashStats_DECLARED
/* 142 */
BLT_EXTERN const char *  Blt_HashStats(Blt_HashTable *tablePtr);
#endif
#ifndef Blt_List_Init_DECLARED
#define Blt_List_Init_DECLARED
/* 143 */
BLT_EXTERN void         Blt_List_Init(Blt_List list, size_t type);
#endif
#ifndef Blt_List_Reset_DECLARED
#define Blt_List_Reset_DECLARED
/* 144 */
BLT_EXTERN void         Blt_List_Reset(Blt_List list);
#endif
#ifndef Blt_List_Create_DECLARED
#define Blt_List_Create_DECLARED
/* 145 */
BLT_EXTERN Blt_List     Blt_List_Create(size_t type);
#endif
#ifndef Blt_List_Destroy_DECLARED
#define Blt_List_Destroy_DECLARED
/* 146 */
BLT_EXTERN void         Blt_List_Destroy(Blt_List list);
#endif
#ifndef Blt_List_CreateNode_DECLARED
#define Blt_List_CreateNode_DECLARED
/* 147 */
BLT_EXTERN Blt_ListNode  Blt_List_CreateNode(Blt_List list, const char *key);
#endif
#ifndef Blt_List_DeleteNode_DECLARED
#define Blt_List_DeleteNode_DECLARED
/* 148 */
BLT_EXTERN void         Blt_List_DeleteNode(Blt_ListNode node);
#endif
#ifndef Blt_List_Append_DECLARED
#define Blt_List_Append_DECLARED
/* 149 */
BLT_EXTERN Blt_ListNode  Blt_List_Append(Blt_List list, const char *key,
                                ClientData clientData);
#endif
#ifndef Blt_List_Prepend_DECLARED
#define Blt_List_Prepend_DECLARED
/* 150 */
BLT_EXTERN Blt_ListNode  Blt_List_Prepend(Blt_List list, const char *key,
                                ClientData clientData);
#endif
#ifndef Blt_List_LinkAfter_DECLARED
#define Blt_List_LinkAfter_DECLARED
/* 151 */
BLT_EXTERN void         Blt_List_LinkAfter(Blt_List list, Blt_ListNode node,
                                Blt_ListNode afterNode);
#endif
#ifndef Blt_List_LinkBefore_DECLARED
#define Blt_List_LinkBefore_DECLARED
/* 152 */
BLT_EXTERN void         Blt_List_LinkBefore(Blt_List list, Blt_ListNode node,
                                Blt_ListNode beforeNode);
#endif
#ifndef Blt_List_UnlinkNode_DECLARED
#define Blt_List_UnlinkNode_DECLARED
/* 153 */
BLT_EXTERN void         Blt_List_UnlinkNode(Blt_ListNode node);
#endif
#ifndef Blt_List_GetNode_DECLARED
#define Blt_List_GetNode_DECLARED
/* 154 */
BLT_EXTERN Blt_ListNode  Blt_List_GetNode(Blt_List list, const char *key);
#endif
#ifndef Blt_List_DeleteNodeByKey_DECLARED
#define Blt_List_DeleteNodeByKey_DECLARED
/* 155 */
BLT_EXTERN void         Blt_List_DeleteNodeByKey(Blt_List list,
                                const char *key);
#endif
#ifndef Blt_List_GetNthNode_DECLARED
#define Blt_List_GetNthNode_DECLARED
/* 156 */
BLT_EXTERN Blt_ListNode  Blt_List_GetNthNode(Blt_List list, long position,
                                int direction);
#endif
#ifndef Blt_List_Sort_DECLARED
#define Blt_List_Sort_DECLARED
/* 157 */
BLT_EXTERN void         Blt_List_Sort(Blt_List list,
                                Blt_ListCompareProc *proc);
#endif
#ifndef Blt_Pool_Create_DECLARED
#define Blt_Pool_Create_DECLARED
/* 158 */
BLT_EXTERN Blt_Pool     Blt_Pool_Create(int type);
#endif
#ifndef Blt_Pool_Destroy_DECLARED
#define Blt_Pool_Destroy_DECLARED
/* 159 */
BLT_EXTERN void         Blt_Pool_Destroy(Blt_Pool pool);
#endif
#ifndef Blt_Tags_Create_DECLARED
#define Blt_Tags_Create_DECLARED
/* 160 */
BLT_EXTERN Blt_Tags     Blt_Tags_Create(void );
#endif
#ifndef Blt_Tags_Destroy_DECLARED
#define Blt_Tags_Destroy_DECLARED
/* 161 */
BLT_EXTERN void         Blt_Tags_Destroy(Blt_Tags tags);
#endif
#ifndef Blt_Tags_Init_DECLARED
#define Blt_Tags_Init_DECLARED
/* 162 */
BLT_EXTERN void         Blt_Tags_Init(Blt_Tags tags);
#endif
#ifndef Blt_Tags_Reset_DECLARED
#define Blt_Tags_Reset_DECLARED
/* 163 */
BLT_EXTERN void         Blt_Tags_Reset(Blt_Tags tags);
#endif
#ifndef Blt_Tags_ItemHasTag_DECLARED
#define Blt_Tags_ItemHasTag_DECLARED
/* 164 */
BLT_EXTERN int          Blt_Tags_ItemHasTag(Blt_Tags tags, ClientData item,
                                const char *tag);
#endif
#ifndef Blt_Tags_AddTag_DECLARED
#define Blt_Tags_AddTag_DECLARED
/* 165 */
BLT_EXTERN void         Blt_Tags_AddTag(Blt_Tags tags, const char *tag);
#endif
#ifndef Blt_Tags_AddItemToTag_DECLARED
#define Blt_Tags_AddItemToTag_DECLARED
/* 166 */
BLT_EXTERN void         Blt_Tags_AddItemToTag(Blt_Tags tags, const char *tag,
                                ClientData item);
#endif
#ifndef Blt_Tags_ForgetTag_DECLARED
#define Blt_Tags_ForgetTag_DECLARED
/* 167 */
BLT_EXTERN void         Blt_Tags_ForgetTag(Blt_Tags tags, const char *tag);
#endif
#ifndef Blt_Tags_RemoveItemFromTag_DECLARED
#define Blt_Tags_RemoveItemFromTag_DECLARED
/* 168 */
BLT_EXTERN void         Blt_Tags_RemoveItemFromTag(Blt_Tags tags,
                                const char *tag, ClientData item);
#endif
#ifndef Blt_Tags_ClearTagsFromItem_DECLARED
#define Blt_Tags_ClearTagsFromItem_DECLARED
/* 169 */
BLT_EXTERN void         Blt_Tags_ClearTagsFromItem(Blt_Tags tags,
                                ClientData item);
#endif
#ifndef Blt_Tags_AppendTagsToChain_DECLARED
#define Blt_Tags_AppendTagsToChain_DECLARED
/* 170 */
BLT_EXTERN void         Blt_Tags_AppendTagsToChain(Blt_Tags tags,
                                ClientData item, Blt_Chain list);
#endif
#ifndef Blt_Tags_AppendTagsToObj_DECLARED
#define Blt_Tags_AppendTagsToObj_DECLARED
/* 171 */
BLT_EXTERN void         Blt_Tags_AppendTagsToObj(Blt_Tags tags,
                                ClientData item, Tcl_Obj *objPtr);
#endif
#ifndef Blt_Tags_AppendAllTagsToObj_DECLARED
#define Blt_Tags_AppendAllTagsToObj_DECLARED
/* 172 */
BLT_EXTERN void         Blt_Tags_AppendAllTagsToObj(Blt_Tags tags,
                                Tcl_Obj *objPtr);
#endif
#ifndef Blt_Tags_GetItemList_DECLARED
#define Blt_Tags_GetItemList_DECLARED
/* 173 */
BLT_EXTERN Blt_Chain    Blt_Tags_GetItemList(Blt_Tags tags, const char *tag);
#endif
#ifndef Blt_Tags_GetTable_DECLARED
#define Blt_Tags_GetTable_DECLARED
/* 174 */
BLT_EXTERN Blt_HashTable * Blt_Tags_GetTable(Blt_Tags tags);
#endif
#ifndef Blt_Tree_GetKey_DECLARED
#define Blt_Tree_GetKey_DECLARED
/* 175 */
BLT_EXTERN Blt_TreeKey  Blt_Tree_GetKey(Blt_Tree tree, const char *string);
#endif
#ifndef Blt_Tree_GetKeyFromNode_DECLARED
#define Blt_Tree_GetKeyFromNode_DECLARED
/* 176 */
BLT_EXTERN Blt_TreeKey  Blt_Tree_GetKeyFromNode(Blt_TreeNode node,
                                const char *string);
#endif
#ifndef Blt_Tree_GetKeyFromInterp_DECLARED
#define Blt_Tree_GetKeyFromInterp_DECLARED
/* 177 */
BLT_EXTERN Blt_TreeKey  Blt_Tree_GetKeyFromInterp(Tcl_Interp *interp,
                                const char *string);
#endif
#ifndef Blt_Tree_CreateNode_DECLARED
#define Blt_Tree_CreateNode_DECLARED
/* 178 */
BLT_EXTERN Blt_TreeNode  Blt_Tree_CreateNode(Blt_Tree tree,
                                Blt_TreeNode parent, const char *name,
                                long position);
#endif
#ifndef Blt_Tree_CreateNodeWithId_DECLARED
#define Blt_Tree_CreateNodeWithId_DECLARED
/* 179 */
BLT_EXTERN Blt_TreeNode  Blt_Tree_CreateNodeWithId(Blt_Tree tree,
                                Blt_TreeNode parent, const char *name,
                                long inode, long position);
#endif
#ifndef Blt_Tree_DeleteNode_DECLARED
#define Blt_Tree_DeleteNode_DECLARED
/* 180 */
BLT_EXTERN int          Blt_Tree_DeleteNode(Blt_Tree tree, Blt_TreeNode node);
#endif
#ifndef Blt_Tree_MoveNode_DECLARED
#define Blt_Tree_MoveNode_DECLARED
/* 181 */
BLT_EXTERN int          Blt_Tree_MoveNode(Blt_Tree tree, Blt_TreeNode node,
                                Blt_TreeNode parent, Blt_TreeNode before);
#endif
#ifndef Blt_Tree_GetNodeFromIndex_DECLARED
#define Blt_Tree_GetNodeFromIndex_DECLARED
/* 182 */
BLT_EXTERN Blt_TreeNode  Blt_Tree_GetNodeFromIndex(Blt_Tree tree, long inode);
#endif
#ifndef Blt_Tree_FindChild_DECLARED
#define Blt_Tree_FindChild_DECLARED
/* 183 */
BLT_EXTERN Blt_TreeNode  Blt_Tree_FindChild(Blt_TreeNode parent,
                                const char *name);
#endif
#ifndef Blt_Tree_NextNode_DECLARED
#define Blt_Tree_NextNode_DECLARED
/* 184 */
BLT_EXTERN Blt_TreeNode  Blt_Tree_NextNode(Blt_TreeNode root,
                                Blt_TreeNode node);
#endif
#ifndef Blt_Tree_PrevNode_DECLARED
#define Blt_Tree_PrevNode_DECLARED
/* 185 */
BLT_EXTERN Blt_TreeNode  Blt_Tree_PrevNode(Blt_TreeNode root,
                                Blt_TreeNode node);
#endif
#ifndef Blt_Tree_FirstChild_DECLARED
#define Blt_Tree_FirstChild_DECLARED
/* 186 */
BLT_EXTERN Blt_TreeNode  Blt_Tree_FirstChild(Blt_TreeNode parent);
#endif
#ifndef Blt_Tree_LastChild_DECLARED
#define Blt_Tree_LastChild_DECLARED
/* 187 */
BLT_EXTERN Blt_TreeNode  Blt_Tree_LastChild(Blt_TreeNode parent);
#endif
#ifndef Blt_Tree_IsBefore_DECLARED
#define Blt_Tree_IsBefore_DECLARED
/* 188 */
BLT_EXTERN int          Blt_Tree_IsBefore(Blt_TreeNode node1,
                                Blt_TreeNode node2);
#endif
#ifndef Blt_Tree_IsAncestor_DECLARED
#define Blt_Tree_IsAncestor_DECLARED
/* 189 */
BLT_EXTERN int          Blt_Tree_IsAncestor(Blt_TreeNode node1,
                                Blt_TreeNode node2);
#endif
#ifndef Blt_Tree_PrivateValue_DECLARED
#define Blt_Tree_PrivateValue_DECLARED
/* 190 */
BLT_EXTERN int          Blt_Tree_PrivateValue(Tcl_Interp *interp,
                                Blt_Tree tree, Blt_TreeNode node,
                                Blt_TreeKey key);
#endif
#ifndef Blt_Tree_PublicValue_DECLARED
#define Blt_Tree_PublicValue_DECLARED
/* 191 */
BLT_EXTERN int          Blt_Tree_PublicValue(Tcl_Interp *interp,
                                Blt_Tree tree, Blt_TreeNode node,
                                Blt_TreeKey key);
#endif
#ifndef Blt_Tree_GetValue_DECLARED
#define Blt_Tree_GetValue_DECLARED
/* 192 */
BLT_EXTERN int          Blt_Tree_GetValue(Tcl_Interp *interp, Blt_Tree tree,
                                Blt_TreeNode node, const char *string,
                                Tcl_Obj **valuePtr);
#endif
#ifndef Blt_Tree_ValueExists_DECLARED
#define Blt_Tree_ValueExists_DECLARED
/* 193 */
BLT_EXTERN int          Blt_Tree_ValueExists(Blt_Tree tree,
                                Blt_TreeNode node, const char *string);
#endif
#ifndef Blt_Tree_SetValue_DECLARED
#define Blt_Tree_SetValue_DECLARED
/* 194 */
BLT_EXTERN int          Blt_Tree_SetValue(Tcl_Interp *interp, Blt_Tree tree,
                                Blt_TreeNode node, const char *string,
                                Tcl_Obj *valuePtr);
#endif
#ifndef Blt_Tree_UnsetValue_DECLARED
#define Blt_Tree_UnsetValue_DECLARED
/* 195 */
BLT_EXTERN int          Blt_Tree_UnsetValue(Tcl_Interp *interp,
                                Blt_Tree tree, Blt_TreeNode node,
                                const char *string);
#endif
#ifndef Blt_Tree_AppendValue_DECLARED
#define Blt_Tree_AppendValue_DECLARED
/* 196 */
BLT_EXTERN int          Blt_Tree_AppendValue(Tcl_Interp *interp,
                                Blt_Tree tree, Blt_TreeNode node,
                                const char *string, const char *value);
#endif
#ifndef Blt_Tree_ListAppendValue_DECLARED
#define Blt_Tree_ListAppendValue_DECLARED
/* 197 */
BLT_EXTERN int          Blt_Tree_ListAppendValue(Tcl_Interp *interp,
                                Blt_Tree tree, Blt_TreeNode node,
                                const char *string, Tcl_Obj *valuePtr);
#endif
#ifndef Blt_Tree_GetArrayValue_DECLARED
#define Blt_Tree_GetArrayValue_DECLARED
/* 198 */
BLT_EXTERN int          Blt_Tree_GetArrayValue(Tcl_Interp *interp,
                                Blt_Tree tree, Blt_TreeNode node,
                                const char *arrayName, const char *elemName,
                                Tcl_Obj **valueObjPtrPtr);
#endif
#ifndef Blt_Tree_SetArrayValue_DECLARED
#define Blt_Tree_SetArrayValue_DECLARED
/* 199 */
BLT_EXTERN int          Blt_Tree_SetArrayValue(Tcl_Interp *interp,
                                Blt_Tree tree, Blt_TreeNode node,
                                const char *arrayName, const char *elemName,
                                Tcl_Obj *valueObjPtr);
#endif
#ifndef Blt_Tree_UnsetArrayValue_DECLARED
#define Blt_Tree_UnsetArrayValue_DECLARED
/* 200 */
BLT_EXTERN int          Blt_Tree_UnsetArrayValue(Tcl_Interp *interp,
                                Blt_Tree tree, Blt_TreeNode node,
                                const char *arrayName, const char *elemName);
#endif
#ifndef Blt_Tree_AppendArrayValue_DECLARED
#define Blt_Tree_AppendArrayValue_DECLARED
/* 201 */
BLT_EXTERN int          Blt_Tree_AppendArrayValue(Tcl_Interp *interp,
                                Blt_Tree tree, Blt_TreeNode node,
                                const char *arrayName, const char *elemName,
                                const char *value);
#endif
#ifndef Blt_Tree_ListAppendArrayValue_DECLARED
#define Blt_Tree_ListAppendArrayValue_DECLARED
/* 202 */
BLT_EXTERN int          Blt_Tree_ListAppendArrayValue(Tcl_Interp *interp,
                                Blt_Tree tree, Blt_TreeNode node,
                                const char *arrayName, const char *elemName,
                                Tcl_Obj *valueObjPtr);
#endif
#ifndef Blt_Tree_ArrayValueExists_DECLARED
#define Blt_Tree_ArrayValueExists_DECLARED
/* 203 */
BLT_EXTERN int          Blt_Tree_ArrayValueExists(Blt_Tree tree,
                                Blt_TreeNode node, const char *arrayName,
                                const char *elemName);
#endif
#ifndef Blt_Tree_ArrayNames_DECLARED
#define Blt_Tree_ArrayNames_DECLARED
/* 204 */
BLT_EXTERN int          Blt_Tree_ArrayNames(Tcl_Interp *interp,
                                Blt_Tree tree, Blt_TreeNode node,
                                const char *arrayName, Tcl_Obj *listObjPtr);
#endif
#ifndef Blt_Tree_GetValueByKey_DECLARED
#define Blt_Tree_GetValueByKey_DECLARED
/* 205 */
BLT_EXTERN int          Blt_Tree_GetValueByKey(Tcl_Interp *interp,
                                Blt_Tree tree, Blt_TreeNode node,
                                Blt_TreeKey key, Tcl_Obj **valuePtr);
#endif
#ifndef Blt_Tree_SetValueByKey_DECLARED
#define Blt_Tree_SetValueByKey_DECLARED
/* 206 */
BLT_EXTERN int          Blt_Tree_SetValueByKey(Tcl_Interp *interp,
                                Blt_Tree tree, Blt_TreeNode node,
                                Blt_TreeKey key, Tcl_Obj *valuePtr);
#endif
#ifndef Blt_Tree_UnsetValueByKey_DECLARED
#define Blt_Tree_UnsetValueByKey_DECLARED
/* 207 */
BLT_EXTERN int          Blt_Tree_UnsetValueByKey(Tcl_Interp *interp,
                                Blt_Tree tree, Blt_TreeNode node,
                                Blt_TreeKey key);
#endif
#ifndef Blt_Tree_AppendValueByKey_DECLARED
#define Blt_Tree_AppendValueByKey_DECLARED
/* 208 */
BLT_EXTERN int          Blt_Tree_AppendValueByKey(Tcl_Interp *interp,
                                Blt_Tree tree, Blt_TreeNode node,
                                Blt_TreeKey key, const char *value);
#endif
#ifndef Blt_Tree_ListAppendValueByKey_DECLARED
#define Blt_Tree_ListAppendValueByKey_DECLARED
/* 209 */
BLT_EXTERN int          Blt_Tree_ListAppendValueByKey(Tcl_Interp *interp,
                                Blt_Tree tree, Blt_TreeNode node,
                                Blt_TreeKey key, Tcl_Obj *valuePtr);
#endif
#ifndef Blt_Tree_ValueExistsByKey_DECLARED
#define Blt_Tree_ValueExistsByKey_DECLARED
/* 210 */
BLT_EXTERN int          Blt_Tree_ValueExistsByKey(Blt_Tree tree,
                                Blt_TreeNode node, Blt_TreeKey key);
#endif
#ifndef Blt_Tree_FirstKey_DECLARED
#define Blt_Tree_FirstKey_DECLARED
/* 211 */
BLT_EXTERN Blt_TreeKey  Blt_Tree_FirstKey(Blt_Tree tree, Blt_TreeNode node,
                                Blt_TreeKeyIterator *iterPtr);
#endif
#ifndef Blt_Tree_NextKey_DECLARED
#define Blt_Tree_NextKey_DECLARED
/* 212 */
BLT_EXTERN Blt_TreeKey  Blt_Tree_NextKey(Blt_Tree tree,
                                Blt_TreeKeyIterator *iterPtr);
#endif
#ifndef Blt_Tree_Apply_DECLARED
#define Blt_Tree_Apply_DECLARED
/* 213 */
BLT_EXTERN int          Blt_Tree_Apply(Blt_TreeNode root,
                                Blt_TreeApplyProc *proc,
                                ClientData clientData);
#endif
#ifndef Blt_Tree_ApplyDFS_DECLARED
#define Blt_Tree_ApplyDFS_DECLARED
/* 214 */
BLT_EXTERN int          Blt_Tree_ApplyDFS(Blt_TreeNode root,
                                Blt_TreeApplyProc *proc,
                                ClientData clientData, int order);
#endif
#ifndef Blt_Tree_ApplyBFS_DECLARED
#define Blt_Tree_ApplyBFS_DECLARED
/* 215 */
BLT_EXTERN int          Blt_Tree_ApplyBFS(Blt_TreeNode root,
                                Blt_TreeApplyProc *proc,
                                ClientData clientData);
#endif
#ifndef Blt_Tree_SortNode_DECLARED
#define Blt_Tree_SortNode_DECLARED
/* 216 */
BLT_EXTERN int          Blt_Tree_SortNode(Blt_Tree tree, Blt_TreeNode node,
                                Blt_TreeCompareNodesProc *proc);
#endif
#ifndef Blt_Tree_Exists_DECLARED
#define Blt_Tree_Exists_DECLARED
/* 217 */
BLT_EXTERN int          Blt_Tree_Exists(Tcl_Interp *interp, const char *name);
#endif
#ifndef Blt_Tree_Open_DECLARED
#define Blt_Tree_Open_DECLARED
/* 218 */
BLT_EXTERN Blt_Tree     Blt_Tree_Open(Tcl_Interp *interp, const char *name,
                                int flags);
#endif
#ifndef Blt_Tree_Close_DECLARED
#define Blt_Tree_Close_DECLARED
/* 219 */
BLT_EXTERN void         Blt_Tree_Close(Blt_Tree tree);
#endif
#ifndef Blt_Tree_Attach_DECLARED
#define Blt_Tree_Attach_DECLARED
/* 220 */
BLT_EXTERN int          Blt_Tree_Attach(Tcl_Interp *interp, Blt_Tree tree,
                                const char *name);
#endif
#ifndef Blt_Tree_GetFromObj_DECLARED
#define Blt_Tree_GetFromObj_DECLARED
/* 221 */
BLT_EXTERN Blt_Tree     Blt_Tree_GetFromObj(Tcl_Interp *interp,
                                Tcl_Obj *objPtr);
#endif
#ifndef Blt_Tree_Size_DECLARED
#define Blt_Tree_Size_DECLARED
/* 222 */
BLT_EXTERN int          Blt_Tree_Size(Blt_TreeNode node);
#endif
#ifndef Blt_Tree_CreateTrace_DECLARED
#define Blt_Tree_CreateTrace_DECLARED
/* 223 */
BLT_EXTERN Blt_TreeTrace Blt_Tree_CreateTrace(Blt_Tree tree,
                                Blt_TreeNode node, const char *keyPattern,
                                const char *tagName, unsigned int mask,
                                Blt_TreeTraceProc *proc,
                                ClientData clientData);
#endif
#ifndef Blt_Tree_DeleteTrace_DECLARED
#define Blt_Tree_DeleteTrace_DECLARED
/* 224 */
BLT_EXTERN void         Blt_Tree_DeleteTrace(Blt_TreeTrace token);
#endif
#ifndef Blt_Tree_CreateEventHandler_DECLARED
#define Blt_Tree_CreateEventHandler_DECLARED
/* 225 */
BLT_EXTERN void         Blt_Tree_CreateEventHandler(Blt_Tree tree,
                                unsigned int mask,
                                Blt_TreeNotifyEventProc *proc,
                                ClientData clientData);
#endif
#ifndef Blt_Tree_DeleteEventHandler_DECLARED
#define Blt_Tree_DeleteEventHandler_DECLARED
/* 226 */
BLT_EXTERN void         Blt_Tree_DeleteEventHandler(Blt_Tree tree,
                                unsigned int mask,
                                Blt_TreeNotifyEventProc *proc,
                                ClientData clientData);
#endif
#ifndef Blt_Tree_RelabelNode_DECLARED
#define Blt_Tree_RelabelNode_DECLARED
/* 227 */
BLT_EXTERN void         Blt_Tree_RelabelNode(Blt_Tree tree,
                                Blt_TreeNode node, const char *string);
#endif
#ifndef Blt_Tree_RelabelNodeWithoutNotify_DECLARED
#define Blt_Tree_RelabelNodeWithoutNotify_DECLARED
/* 228 */
BLT_EXTERN void         Blt_Tree_RelabelNodeWithoutNotify(Blt_TreeNode node,
                                const char *string);
#endif
#ifndef Blt_Tree_NodeIdAscii_DECLARED
#define Blt_Tree_NodeIdAscii_DECLARED
/* 229 */
BLT_EXTERN const char *  Blt_Tree_NodeIdAscii(Blt_TreeNode node);
#endif
#ifndef Blt_Tree_NodePath_DECLARED
#define Blt_Tree_NodePath_DECLARED
/* 230 */
BLT_EXTERN const char *  Blt_Tree_NodePath(Blt_TreeNode node,
                                Tcl_DString *resultPtr);
#endif
#ifndef Blt_Tree_NodeRelativePath_DECLARED
#define Blt_Tree_NodeRelativePath_DECLARED
/* 231 */
BLT_EXTERN const char *  Blt_Tree_NodeRelativePath(Blt_TreeNode root,
                                Blt_TreeNode node, const char *separator,
                                unsigned int flags, Tcl_DString *resultPtr);
#endif
#ifndef Blt_Tree_NodePosition_DECLARED
#define Blt_Tree_NodePosition_DECLARED
/* 232 */
BLT_EXTERN long         Blt_Tree_NodePosition(Blt_TreeNode node);
#endif
#ifndef Blt_Tree_ClearTags_DECLARED
#define Blt_Tree_ClearTags_DECLARED
/* 233 */
BLT_EXTERN void         Blt_Tree_ClearTags(Blt_Tree tree, Blt_TreeNode node);
#endif
#ifndef Blt_Tree_HasTag_DECLARED
#define Blt_Tree_HasTag_DECLARED
/* 234 */
BLT_EXTERN int          Blt_Tree_HasTag(Blt_Tree tree, Blt_TreeNode node,
                                const char *tagName);
#endif
#ifndef Blt_Tree_AddTag_DECLARED
#define Blt_Tree_AddTag_DECLARED
/* 235 */
BLT_EXTERN void         Blt_Tree_AddTag(Blt_Tree tree, Blt_TreeNode node,
                                const char *tagName);
#endif
#ifndef Blt_Tree_RemoveTag_DECLARED
#define Blt_Tree_RemoveTag_DECLARED
/* 236 */
BLT_EXTERN void         Blt_Tree_RemoveTag(Blt_Tree tree, Blt_TreeNode node,
                                const char *tagName);
#endif
#ifndef Blt_Tree_ForgetTag_DECLARED
#define Blt_Tree_ForgetTag_DECLARED
/* 237 */
BLT_EXTERN void         Blt_Tree_ForgetTag(Blt_Tree tree,
                                const char *tagName);
#endif
#ifndef Blt_Tree_TagHashTable_DECLARED
#define Blt_Tree_TagHashTable_DECLARED
/* 238 */
BLT_EXTERN Blt_HashTable * Blt_Tree_TagHashTable(Blt_Tree tree,
                                const char *tagName);
#endif
#ifndef Blt_Tree_TagTableIsShared_DECLARED
#define Blt_Tree_TagTableIsShared_DECLARED
/* 239 */
BLT_EXTERN int          Blt_Tree_TagTableIsShared(Blt_Tree tree);
#endif
#ifndef Blt_Tree_NewTagTable_DECLARED
#define Blt_Tree_NewTagTable_DECLARED
/* 240 */
BLT_EXTERN void         Blt_Tree_NewTagTable(Blt_Tree tree);
#endif
#ifndef Blt_Tree_FirstTag_DECLARED
#define Blt_Tree_FirstTag_DECLARED
/* 241 */
BLT_EXTERN Blt_HashEntry * Blt_Tree_FirstTag(Blt_Tree tree,
                                Blt_HashSearch *searchPtr);
#endif
#ifndef Blt_Tree_DumpNode_DECLARED
#define Blt_Tree_DumpNode_DECLARED
/* 242 */
BLT_EXTERN void         Blt_Tree_DumpNode(Blt_Tree tree, Blt_TreeNode root,
                                Blt_TreeNode node, Tcl_DString *resultPtr);
#endif
#ifndef Blt_Tree_Dump_DECLARED
#define Blt_Tree_Dump_DECLARED
/* 243 */
BLT_EXTERN int          Blt_Tree_Dump(Blt_Tree tree, Blt_TreeNode root,
                                Tcl_DString *resultPtr);
#endif
#ifndef Blt_Tree_DumpToFile_DECLARED
#define Blt_Tree_DumpToFile_DECLARED
/* 244 */
BLT_EXTERN int          Blt_Tree_DumpToFile(Tcl_Interp *interp,
                                Blt_Tree tree, Blt_TreeNode root,
                                const char *fileName);
#endif
#ifndef Blt_Tree_Restore_DECLARED
#define Blt_Tree_Restore_DECLARED
/* 245 */
BLT_EXTERN int          Blt_Tree_Restore(Tcl_Interp *interp, Blt_Tree tree,
                                Blt_TreeNode root, const char *string,
                                unsigned int flags);
#endif
#ifndef Blt_Tree_RestoreFromFile_DECLARED
#define Blt_Tree_RestoreFromFile_DECLARED
/* 246 */
BLT_EXTERN int          Blt_Tree_RestoreFromFile(Tcl_Interp *interp,
                                Blt_Tree tree, Blt_TreeNode root,
                                const char *fileName, unsigned int flags);
#endif
#ifndef Blt_Tree_Depth_DECLARED
#define Blt_Tree_Depth_DECLARED
/* 247 */
BLT_EXTERN long         Blt_Tree_Depth(Blt_Tree tree);
#endif
#ifndef Blt_Tree_RegisterFormat_DECLARED
#define Blt_Tree_RegisterFormat_DECLARED
/* 248 */
BLT_EXTERN int          Blt_Tree_RegisterFormat(Tcl_Interp *interp,
                                const char *fmtName,
                                Blt_TreeImportProc *importProc,
                                Blt_TreeExportProc *exportProc);
#endif
#ifndef Blt_Tree_RememberTag_DECLARED
#define Blt_Tree_RememberTag_DECLARED
/* 249 */
BLT_EXTERN Blt_TreeTagEntry * Blt_Tree_RememberTag(Blt_Tree tree,
                                const char *name);
#endif
#ifndef Blt_Tree_GetNodeFromObj_DECLARED
#define Blt_Tree_GetNodeFromObj_DECLARED
/* 250 */
BLT_EXTERN int          Blt_Tree_GetNodeFromObj(Tcl_Interp *interp,
                                Blt_Tree tree, Tcl_Obj *objPtr,
                                Blt_TreeNode *nodePtr);
#endif
#ifndef Blt_Tree_GetNodeIterator_DECLARED
#define Blt_Tree_GetNodeIterator_DECLARED
/* 251 */
BLT_EXTERN int          Blt_Tree_GetNodeIterator(Tcl_Interp *interp,
                                Blt_Tree tree, Tcl_Obj *objPtr,
                                Blt_TreeIterator *iterPtr);
#endif
#ifndef Blt_Tree_FirstTaggedNode_DECLARED
#define Blt_Tree_FirstTaggedNode_DECLARED
/* 252 */
BLT_EXTERN Blt_TreeNode  Blt_Tree_FirstTaggedNode(Blt_TreeIterator *iterPtr);
#endif
#ifndef Blt_Tree_NextTaggedNode_DECLARED
#define Blt_Tree_NextTaggedNode_DECLARED
/* 253 */
BLT_EXTERN Blt_TreeNode  Blt_Tree_NextTaggedNode(Blt_TreeIterator *iterPtr);
#endif
#ifndef Blt_VecMin_DECLARED
#define Blt_VecMin_DECLARED
/* 254 */
BLT_EXTERN double       Blt_VecMin(Blt_Vector *vPtr);
#endif
#ifndef Blt_VecMax_DECLARED
#define Blt_VecMax_DECLARED
/* 255 */
BLT_EXTERN double       Blt_VecMax(Blt_Vector *vPtr);
#endif
#ifndef Blt_AllocVectorId_DECLARED
#define Blt_AllocVectorId_DECLARED
/* 256 */
BLT_EXTERN Blt_VectorId  Blt_AllocVectorId(Tcl_Interp *interp,
                                const char *vecName);
#endif
#ifndef Blt_SetVectorChangedProc_DECLARED
#define Blt_SetVectorChangedProc_DECLARED
/* 257 */
BLT_EXTERN void         Blt_SetVectorChangedProc(Blt_VectorId clientId,
                                Blt_VectorChangedProc *proc,
                                ClientData clientData);
#endif
#ifndef Blt_FreeVectorId_DECLARED
#define Blt_FreeVectorId_DECLARED
/* 258 */
BLT_EXTERN void         Blt_FreeVectorId(Blt_VectorId clientId);
#endif
#ifndef Blt_GetVectorById_DECLARED
#define Blt_GetVectorById_DECLARED
/* 259 */
BLT_EXTERN int          Blt_GetVectorById(Tcl_Interp *interp,
                                Blt_VectorId clientId,
                                Blt_Vector **vecPtrPtr);
#endif
#ifndef Blt_NameOfVectorId_DECLARED
#define Blt_NameOfVectorId_DECLARED
/* 260 */
BLT_EXTERN const char *  Blt_NameOfVectorId(Blt_VectorId clientId);
#endif
#ifndef Blt_NameOfVector_DECLARED
#define Blt_NameOfVector_DECLARED
/* 261 */
BLT_EXTERN const char *  Blt_NameOfVector(Blt_Vector *vecPtr);
#endif
#ifndef Blt_VectorNotifyPending_DECLARED
#define Blt_VectorNotifyPending_DECLARED
/* 262 */
BLT_EXTERN int          Blt_VectorNotifyPending(Blt_VectorId clientId);
#endif
#ifndef Blt_CreateVector_DECLARED
#define Blt_CreateVector_DECLARED
/* 263 */
BLT_EXTERN int          Blt_CreateVector(Tcl_Interp *interp,
                                const char *vecName, int size,
                                Blt_Vector **vecPtrPtr);
#endif
#ifndef Blt_CreateVector2_DECLARED
#define Blt_CreateVector2_DECLARED
/* 264 */
BLT_EXTERN int          Blt_CreateVector2(Tcl_Interp *interp,
                                const char *vecName, const char *cmdName,
                                const char *varName, int initialSize,
                                Blt_Vector **vecPtrPtr);
#endif
#ifndef Blt_GetVector_DECLARED
#define Blt_GetVector_DECLARED
/* 265 */
BLT_EXTERN int          Blt_GetVector(Tcl_Interp *interp,
                                const char *vecName, Blt_Vector **vecPtrPtr);
#endif
#ifndef Blt_GetVectorFromObj_DECLARED
#define Blt_GetVectorFromObj_DECLARED
/* 266 */
BLT_EXTERN int          Blt_GetVectorFromObj(Tcl_Interp *interp,
                                Tcl_Obj *objPtr, Blt_Vector **vecPtrPtr);
#endif
#ifndef Blt_VectorExists_DECLARED
#define Blt_VectorExists_DECLARED
/* 267 */
BLT_EXTERN int          Blt_VectorExists(Tcl_Interp *interp,
                                const char *vecName);
#endif
#ifndef Blt_ResetVector_DECLARED
#define Blt_ResetVector_DECLARED
/* 268 */
BLT_EXTERN int          Blt_ResetVector(Blt_Vector *vecPtr, double *dataArr,
                                int n, int arraySize, Tcl_FreeProc *freeProc);
#endif
#ifndef Blt_ResizeVector_DECLARED
#define Blt_ResizeVector_DECLARED
/* 269 */
BLT_EXTERN int          Blt_ResizeVector(Blt_Vector *vecPtr, int n);
#endif
#ifndef Blt_DeleteVectorByName_DECLARED
#define Blt_DeleteVectorByName_DECLARED
/* 270 */
BLT_EXTERN int          Blt_DeleteVectorByName(Tcl_Interp *interp,
                                const char *vecName);
#endif
#ifndef Blt_DeleteVector_DECLARED
#define Blt_DeleteVector_DECLARED
/* 271 */
BLT_EXTERN int          Blt_DeleteVector(Blt_Vector *vecPtr);
#endif
#ifndef Blt_ExprVector_DECLARED
#define Blt_ExprVector_DECLARED
/* 272 */
BLT_EXTERN int          Blt_ExprVector(Tcl_Interp *interp, char *expr,
                                Blt_Vector *vecPtr);
#endif
#ifndef Blt_InstallIndexProc_DECLARED
#define Blt_InstallIndexProc_DECLARED
/* 273 */
BLT_EXTERN void         Blt_InstallIndexProc(Tcl_Interp *interp,
                                const char *indexName,
                                Blt_VectorIndexProc *procPtr);
#endif
#ifndef Blt_VectorExists2_DECLARED
#define Blt_VectorExists2_DECLARED
/* 274 */
BLT_EXTERN int          Blt_VectorExists2(Tcl_Interp *interp,
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
    void (*blt_table_release_tags) (BLT_TABLE table); /* 19 */
    void (*blt_table_new_tags) (BLT_TABLE table); /* 20 */
    Blt_HashTable * (*blt_table_get_column_tag_table) (BLT_TABLE table); /* 21 */
    Blt_HashTable * (*blt_table_get_row_tag_table) (BLT_TABLE table); /* 22 */
    int (*blt_table_exists) (Tcl_Interp *interp, const char *name); /* 23 */
    int (*blt_table_create) (Tcl_Interp *interp, const char *name, BLT_TABLE *tablePtr); /* 24 */
    int (*blt_table_open) (Tcl_Interp *interp, const char *name, BLT_TABLE *tablePtr); /* 25 */
    void (*blt_table_close) (BLT_TABLE table); /* 26 */
    int (*blt_table_same_object) (BLT_TABLE table1, BLT_TABLE table2); /* 27 */
    Blt_HashTable * (*blt_table_row_get_label_table) (BLT_TABLE table, const char *label); /* 28 */
    Blt_HashTable * (*blt_table_column_get_label_table) (BLT_TABLE table, const char *label); /* 29 */
    BLT_TABLE_ROW (*blt_table_get_row) (Tcl_Interp *interp, BLT_TABLE table, Tcl_Obj *objPtr); /* 30 */
    BLT_TABLE_COLUMN (*blt_table_get_column) (Tcl_Interp *interp, BLT_TABLE table, Tcl_Obj *objPtr); /* 31 */
    BLT_TABLE_ROW (*blt_table_get_row_by_label) (BLT_TABLE table, const char *label); /* 32 */
    BLT_TABLE_COLUMN (*blt_table_get_column_by_label) (BLT_TABLE table, const char *label); /* 33 */
    BLT_TABLE_ROW (*blt_table_get_row_by_index) (BLT_TABLE table, long index); /* 34 */
    BLT_TABLE_COLUMN (*blt_table_get_column_by_index) (BLT_TABLE table, long index); /* 35 */
    int (*blt_table_set_row_label) (Tcl_Interp *interp, BLT_TABLE table, BLT_TABLE_ROW row, const char *label); /* 36 */
    int (*blt_table_set_column_label) (Tcl_Interp *interp, BLT_TABLE table, BLT_TABLE_COLUMN column, const char *label); /* 37 */
    BLT_TABLE_COLUMN_TYPE (*blt_table_name_to_column_type) (const char *typeName); /* 38 */
    int (*blt_table_set_column_type) (Tcl_Interp *interp, BLT_TABLE table, BLT_TABLE_COLUMN column, BLT_TABLE_COLUMN_TYPE type); /* 39 */
    const char * (*blt_table_column_type_to_name) (BLT_TABLE_COLUMN_TYPE type); /* 40 */
    int (*blt_table_set_column_tag) (Tcl_Interp *interp, BLT_TABLE table, BLT_TABLE_COLUMN column, const char *tag); /* 41 */
    int (*blt_table_set_row_tag) (Tcl_Interp *interp, BLT_TABLE table, BLT_TABLE_ROW row, const char *tag); /* 42 */
    BLT_TABLE_ROW (*blt_table_create_row) (Tcl_Interp *interp, BLT_TABLE table, const char *label); /* 43 */
    BLT_TABLE_COLUMN (*blt_table_create_column) (Tcl_Interp *interp, BLT_TABLE table, const char *label); /* 44 */
    int (*blt_table_extend_rows) (Tcl_Interp *interp, BLT_TABLE table, size_t n, BLT_TABLE_ROW *rows); /* 45 */
    int (*blt_table_extend_columns) (Tcl_Interp *interp, BLT_TABLE table, size_t n, BLT_TABLE_COLUMN *columms); /* 46 */
    int (*blt_table_delete_row) (BLT_TABLE table, BLT_TABLE_ROW row); /* 47 */
    int (*blt_table_delete_column) (BLT_TABLE table, BLT_TABLE_COLUMN column); /* 48 */
    int (*blt_table_move_row) (Tcl_Interp *interp, BLT_TABLE table, BLT_TABLE_ROW from, BLT_TABLE_ROW to, size_t n); /* 49 */
    int (*blt_table_move_column) (Tcl_Interp *interp, BLT_TABLE table, BLT_TABLE_COLUMN from, BLT_TABLE_COLUMN to, size_t n); /* 50 */
    Tcl_Obj * (*blt_table_get_obj) (BLT_TABLE table, BLT_TABLE_ROW row, BLT_TABLE_COLUMN column); /* 51 */
    int (*blt_table_set_obj) (Tcl_Interp *interp, BLT_TABLE table, BLT_TABLE_ROW row, BLT_TABLE_COLUMN column, Tcl_Obj *objPtr); /* 52 */
    const char * (*blt_table_get_string) (BLT_TABLE table, BLT_TABLE_ROW row, BLT_TABLE_COLUMN column); /* 53 */
    int (*blt_table_set_string_rep) (Tcl_Interp *interp, BLT_TABLE table, BLT_TABLE_ROW row, BLT_TABLE_COLUMN column, const char *string, int length); /* 54 */
    int (*blt_table_set_string) (Tcl_Interp *interp, BLT_TABLE table, BLT_TABLE_ROW row, BLT_TABLE_COLUMN column, const char *string, int length); /* 55 */
    int (*blt_table_append_string) (Tcl_Interp *interp, BLT_TABLE table, BLT_TABLE_ROW row, BLT_TABLE_COLUMN column, const char *string, int length); /* 56 */
    int (*blt_table_set_bytes) (Tcl_Interp *interp, BLT_TABLE table, BLT_TABLE_ROW row, BLT_TABLE_COLUMN column, const unsigned char *string, int length); /* 57 */
    double (*blt_table_get_double) (Tcl_Interp *interp, BLT_TABLE table, BLT_TABLE_ROW row, BLT_TABLE_COLUMN column); /* 58 */
    int (*blt_table_set_double) (Tcl_Interp *interp, BLT_TABLE table, BLT_TABLE_ROW row, BLT_TABLE_COLUMN column, double value); /* 59 */
    long (*blt_table_get_long) (Tcl_Interp *interp, BLT_TABLE table, BLT_TABLE_ROW row, BLT_TABLE_COLUMN column, long defValue); /* 60 */
    int (*blt_table_set_long) (Tcl_Interp *interp, BLT_TABLE table, BLT_TABLE_ROW row, BLT_TABLE_COLUMN column, long value); /* 61 */
    int (*blt_table_get_boolean) (Tcl_Interp *interp, BLT_TABLE table, BLT_TABLE_ROW row, BLT_TABLE_COLUMN column, int defValue); /* 62 */
    int (*blt_table_set_boolean) (Tcl_Interp *interp, BLT_TABLE table, BLT_TABLE_ROW row, BLT_TABLE_COLUMN column, int value); /* 63 */
    BLT_TABLE_VALUE (*blt_table_get_value) (BLT_TABLE table, BLT_TABLE_ROW row, BLT_TABLE_COLUMN column); /* 64 */
    int (*blt_table_set_value) (BLT_TABLE table, BLT_TABLE_ROW row, BLT_TABLE_COLUMN column, BLT_TABLE_VALUE value); /* 65 */
    int (*blt_table_unset_value) (BLT_TABLE table, BLT_TABLE_ROW row, BLT_TABLE_COLUMN column); /* 66 */
    int (*blt_table_value_exists) (BLT_TABLE table, BLT_TABLE_ROW row, BLT_TABLE_COLUMN column); /* 67 */
    const char * (*blt_table_value_string) (BLT_TABLE_VALUE value); /* 68 */
    const unsigned char * (*blt_table_value_bytes) (BLT_TABLE_VALUE value); /* 69 */
    size_t (*blt_table_value_length) (BLT_TABLE_VALUE value); /* 70 */
    int (*blt_table_tags_are_shared) (BLT_TABLE table); /* 71 */
    void (*blt_table_clear_row_tags) (BLT_TABLE table, BLT_TABLE_ROW row); /* 72 */
    void (*blt_table_clear_column_tags) (BLT_TABLE table, BLT_TABLE_COLUMN col); /* 73 */
    Blt_Chain (*blt_table_get_row_tags) (BLT_TABLE table, BLT_TABLE_ROW row); /* 74 */
    Blt_Chain (*blt_table_get_column_tags) (BLT_TABLE table, BLT_TABLE_COLUMN column); /* 75 */
    Blt_Chain (*blt_table_get_tagged_rows) (BLT_TABLE table, const char *tag); /* 76 */
    Blt_Chain (*blt_table_get_tagged_columns) (BLT_TABLE table, const char *tag); /* 77 */
    int (*blt_table_row_has_tag) (BLT_TABLE table, BLT_TABLE_ROW row, const char *tag); /* 78 */
    int (*blt_table_column_has_tag) (BLT_TABLE table, BLT_TABLE_COLUMN column, const char *tag); /* 79 */
    int (*blt_table_forget_row_tag) (Tcl_Interp *interp, BLT_TABLE table, const char *tag); /* 80 */
    int (*blt_table_forget_column_tag) (Tcl_Interp *interp, BLT_TABLE table, const char *tag); /* 81 */
    int (*blt_table_unset_row_tag) (Tcl_Interp *interp, BLT_TABLE table, BLT_TABLE_ROW row, const char *tag); /* 82 */
    int (*blt_table_unset_column_tag) (Tcl_Interp *interp, BLT_TABLE table, BLT_TABLE_COLUMN column, const char *tag); /* 83 */
    BLT_TABLE_COLUMN (*blt_table_first_column) (BLT_TABLE table); /* 84 */
    BLT_TABLE_COLUMN (*blt_table_next_column) (BLT_TABLE table, BLT_TABLE_COLUMN column); /* 85 */
    BLT_TABLE_ROW (*blt_table_first_row) (BLT_TABLE table); /* 86 */
    BLT_TABLE_ROW (*blt_table_next_row) (BLT_TABLE table, BLT_TABLE_ROW row); /* 87 */
    BLT_TABLE_ROWCOLUMN_SPEC (*blt_table_row_spec) (BLT_TABLE table, Tcl_Obj *objPtr, const char **sp); /* 88 */
    BLT_TABLE_ROWCOLUMN_SPEC (*blt_table_column_spec) (BLT_TABLE table, Tcl_Obj *objPtr, const char **sp); /* 89 */
    int (*blt_table_iterate_rows) (Tcl_Interp *interp, BLT_TABLE table, Tcl_Obj *objPtr, BLT_TABLE_ITERATOR *iter); /* 90 */
    int (*blt_table_iterate_columns) (Tcl_Interp *interp, BLT_TABLE table, Tcl_Obj *objPtr, BLT_TABLE_ITERATOR *iter); /* 91 */
    int (*blt_table_iterate_rows_objv) (Tcl_Interp *interp, BLT_TABLE table, int objc, Tcl_Obj *const *objv, BLT_TABLE_ITERATOR *iterPtr); /* 92 */
    int (*blt_table_iterate_columns_objv) (Tcl_Interp *interp, BLT_TABLE table, int objc, Tcl_Obj *const *objv, BLT_TABLE_ITERATOR *iterPtr); /* 93 */
    void (*blt_table_free_iterator_objv) (BLT_TABLE_ITERATOR *iterPtr); /* 94 */
    void (*blt_table_iterate_all_rows) (BLT_TABLE table, BLT_TABLE_ITERATOR *iterPtr); /* 95 */
    void (*blt_table_iterate_all_columns) (BLT_TABLE table, BLT_TABLE_ITERATOR *iterPtr); /* 96 */
    BLT_TABLE_ROW (*blt_table_first_tagged_row) (BLT_TABLE_ITERATOR *iter); /* 97 */
    BLT_TABLE_COLUMN (*blt_table_first_tagged_column) (BLT_TABLE_ITERATOR *iter); /* 98 */
    BLT_TABLE_ROW (*blt_table_next_tagged_row) (BLT_TABLE_ITERATOR *iter); /* 99 */
    BLT_TABLE_COLUMN (*blt_table_next_tagged_column) (BLT_TABLE_ITERATOR *iter); /* 100 */
    int (*blt_table_list_rows) (Tcl_Interp *interp, BLT_TABLE table, int objc, Tcl_Obj *const *objv, Blt_Chain chain); /* 101 */
    int (*blt_table_list_columns) (Tcl_Interp *interp, BLT_TABLE table, int objc, Tcl_Obj *const *objv, Blt_Chain chain); /* 102 */
    void (*blt_table_clear_row_traces) (BLT_TABLE table, BLT_TABLE_ROW row); /* 103 */
    void (*blt_table_clear_column_traces) (BLT_TABLE table, BLT_TABLE_COLUMN column); /* 104 */
    BLT_TABLE_TRACE (*blt_table_create_trace) (BLT_TABLE table, BLT_TABLE_ROW row, BLT_TABLE_COLUMN column, const char *rowTag, const char *columnTag, unsigned int mask, BLT_TABLE_TRACE_PROC *proc, BLT_TABLE_TRACE_DELETE_PROC *deleteProc, ClientData clientData); /* 105 */
    void (*blt_table_trace_column) (BLT_TABLE table, BLT_TABLE_COLUMN column, unsigned int mask, BLT_TABLE_TRACE_PROC *proc, BLT_TABLE_TRACE_DELETE_PROC *deleteProc, ClientData clientData); /* 106 */
    void (*blt_table_trace_row) (BLT_TABLE table, BLT_TABLE_ROW row, unsigned int mask, BLT_TABLE_TRACE_PROC *proc, BLT_TABLE_TRACE_DELETE_PROC *deleteProc, ClientData clientData); /* 107 */
    BLT_TABLE_TRACE (*blt_table_create_column_trace) (BLT_TABLE table, BLT_TABLE_COLUMN column, unsigned int mask, BLT_TABLE_TRACE_PROC *proc, BLT_TABLE_TRACE_DELETE_PROC *deleteProc, ClientData clientData); /* 108 */
    BLT_TABLE_TRACE (*blt_table_create_column_tag_trace) (BLT_TABLE table, const char *tag, unsigned int mask, BLT_TABLE_TRACE_PROC *proc, BLT_TABLE_TRACE_DELETE_PROC *deleteProc, ClientData clientData); /* 109 */
    BLT_TABLE_TRACE (*blt_table_create_row_trace) (BLT_TABLE table, BLT_TABLE_ROW row, unsigned int mask, BLT_TABLE_TRACE_PROC *proc, BLT_TABLE_TRACE_DELETE_PROC *deleteProc, ClientData clientData); /* 110 */
    BLT_TABLE_TRACE (*blt_table_create_row_tag_trace) (BLT_TABLE table, const char *tag, unsigned int mask, BLT_TABLE_TRACE_PROC *proc, BLT_TABLE_TRACE_DELETE_PROC *deleteProc, ClientData clientData); /* 111 */
    void (*blt_table_delete_trace) (BLT_TABLE table, BLT_TABLE_TRACE trace); /* 112 */
    BLT_TABLE_NOTIFIER (*blt_table_create_notifier) (Tcl_Interp *interp, BLT_TABLE table, unsigned int mask, BLT_TABLE_NOTIFY_EVENT_PROC *proc, BLT_TABLE_NOTIFIER_DELETE_PROC *deleteProc, ClientData clientData); /* 113 */
    BLT_TABLE_NOTIFIER (*blt_table_create_row_notifier) (Tcl_Interp *interp, BLT_TABLE table, BLT_TABLE_ROW row, unsigned int mask, BLT_TABLE_NOTIFY_EVENT_PROC *proc, BLT_TABLE_NOTIFIER_DELETE_PROC *deleteProc, ClientData clientData); /* 114 */
    BLT_TABLE_NOTIFIER (*blt_table_create_row_tag_notifier) (Tcl_Interp *interp, BLT_TABLE table, const char *tag, unsigned int mask, BLT_TABLE_NOTIFY_EVENT_PROC *proc, BLT_TABLE_NOTIFIER_DELETE_PROC *deleteProc, ClientData clientData); /* 115 */
    BLT_TABLE_NOTIFIER (*blt_table_create_column_notifier) (Tcl_Interp *interp, BLT_TABLE table, BLT_TABLE_COLUMN column, unsigned int mask, BLT_TABLE_NOTIFY_EVENT_PROC *proc, BLT_TABLE_NOTIFIER_DELETE_PROC *deleteProc, ClientData clientData); /* 116 */
    BLT_TABLE_NOTIFIER (*blt_table_create_column_tag_notifier) (Tcl_Interp *interp, BLT_TABLE table, const char *tag, unsigned int mask, BLT_TABLE_NOTIFY_EVENT_PROC *proc, BLT_TABLE_NOTIFIER_DELETE_PROC *deleteProc, ClientData clientData); /* 117 */
    void (*blt_table_delete_notifier) (BLT_TABLE table, BLT_TABLE_NOTIFIER notifier); /* 118 */
    void (*blt_table_sort_init) (BLT_TABLE table, BLT_TABLE_SORT_ORDER *order, size_t numCompares, unsigned int flags); /* 119 */
    BLT_TABLE_ROW * (*blt_table_sort_rows) (BLT_TABLE table); /* 120 */
    void (*blt_table_sort_rows_subset) (BLT_TABLE table, long numRows, BLT_TABLE_ROW *rows); /* 121 */
    void (*blt_table_sort_finish) (void); /* 122 */
    BLT_TABLE_COMPARE_PROC * (*blt_table_get_compare_proc) (BLT_TABLE table, BLT_TABLE_COLUMN column, unsigned int flags); /* 123 */
    BLT_TABLE_ROW * (*blt_table_get_row_map) (BLT_TABLE table); /* 124 */
    BLT_TABLE_COLUMN * (*blt_table_get_column_map) (BLT_TABLE table); /* 125 */
    void (*blt_table_set_row_map) (BLT_TABLE table, BLT_TABLE_ROW *map); /* 126 */
    void (*blt_table_set_column_map) (BLT_TABLE table, BLT_TABLE_COLUMN *map); /* 127 */
    int (*blt_table_restore) (Tcl_Interp *interp, BLT_TABLE table, char *string, unsigned int flags); /* 128 */
    int (*blt_table_file_restore) (Tcl_Interp *interp, BLT_TABLE table, const char *fileName, unsigned int flags); /* 129 */
    int (*blt_table_register_format) (Tcl_Interp *interp, const char *name, BLT_TABLE_IMPORT_PROC *importProc, BLT_TABLE_EXPORT_PROC *exportProc); /* 130 */
    void (*blt_table_unset_keys) (BLT_TABLE table); /* 131 */
    int (*blt_table_get_keys) (BLT_TABLE table, BLT_TABLE_COLUMN **keysPtr); /* 132 */
    int (*blt_table_set_keys) (BLT_TABLE table, int numKeys, BLT_TABLE_COLUMN *keys, int unique); /* 133 */
    int (*blt_table_key_lookup) (Tcl_Interp *interp, BLT_TABLE table, int objc, Tcl_Obj *const *objv, BLT_TABLE_ROW *rowPtr); /* 134 */
    int (*blt_table_get_column_limits) (Tcl_Interp *interp, BLT_TABLE table, BLT_TABLE_COLUMN col, Tcl_Obj **minObjPtrPtr, Tcl_Obj **maxObjPtrPtr); /* 135 */
    void (*blt_InitHashTable) (Blt_HashTable *tablePtr, size_t keyType); /* 136 */
    void (*blt_InitHashTableWithPool) (Blt_HashTable *tablePtr, size_t keyType); /* 137 */
    void (*blt_DeleteHashTable) (Blt_HashTable *tablePtr); /* 138 */
    void (*blt_DeleteHashEntry) (Blt_HashTable *tablePtr, Blt_HashEntry *entryPtr); /* 139 */
    Blt_HashEntry * (*blt_FirstHashEntry) (Blt_HashTable *tablePtr, Blt_HashSearch *searchPtr); /* 140 */
    Blt_HashEntry * (*blt_NextHashEntry) (Blt_HashSearch *srchPtr); /* 141 */
    const char * (*blt_HashStats) (Blt_HashTable *tablePtr); /* 142 */
    void (*blt_List_Init) (Blt_List list, size_t type); /* 143 */
    void (*blt_List_Reset) (Blt_List list); /* 144 */
    Blt_List (*blt_List_Create) (size_t type); /* 145 */
    void (*blt_List_Destroy) (Blt_List list); /* 146 */
    Blt_ListNode (*blt_List_CreateNode) (Blt_List list, const char *key); /* 147 */
    void (*blt_List_DeleteNode) (Blt_ListNode node); /* 148 */
    Blt_ListNode (*blt_List_Append) (Blt_List list, const char *key, ClientData clientData); /* 149 */
    Blt_ListNode (*blt_List_Prepend) (Blt_List list, const char *key, ClientData clientData); /* 150 */
    void (*blt_List_LinkAfter) (Blt_List list, Blt_ListNode node, Blt_ListNode afterNode); /* 151 */
    void (*blt_List_LinkBefore) (Blt_List list, Blt_ListNode node, Blt_ListNode beforeNode); /* 152 */
    void (*blt_List_UnlinkNode) (Blt_ListNode node); /* 153 */
    Blt_ListNode (*blt_List_GetNode) (Blt_List list, const char *key); /* 154 */
    void (*blt_List_DeleteNodeByKey) (Blt_List list, const char *key); /* 155 */
    Blt_ListNode (*blt_List_GetNthNode) (Blt_List list, long position, int direction); /* 156 */
    void (*blt_List_Sort) (Blt_List list, Blt_ListCompareProc *proc); /* 157 */
    Blt_Pool (*blt_Pool_Create) (int type); /* 158 */
    void (*blt_Pool_Destroy) (Blt_Pool pool); /* 159 */
    Blt_Tags (*blt_Tags_Create) (void); /* 160 */
    void (*blt_Tags_Destroy) (Blt_Tags tags); /* 161 */
    void (*blt_Tags_Init) (Blt_Tags tags); /* 162 */
    void (*blt_Tags_Reset) (Blt_Tags tags); /* 163 */
    int (*blt_Tags_ItemHasTag) (Blt_Tags tags, ClientData item, const char *tag); /* 164 */
    void (*blt_Tags_AddTag) (Blt_Tags tags, const char *tag); /* 165 */
    void (*blt_Tags_AddItemToTag) (Blt_Tags tags, const char *tag, ClientData item); /* 166 */
    void (*blt_Tags_ForgetTag) (Blt_Tags tags, const char *tag); /* 167 */
    void (*blt_Tags_RemoveItemFromTag) (Blt_Tags tags, const char *tag, ClientData item); /* 168 */
    void (*blt_Tags_ClearTagsFromItem) (Blt_Tags tags, ClientData item); /* 169 */
    void (*blt_Tags_AppendTagsToChain) (Blt_Tags tags, ClientData item, Blt_Chain list); /* 170 */
    void (*blt_Tags_AppendTagsToObj) (Blt_Tags tags, ClientData item, Tcl_Obj *objPtr); /* 171 */
    void (*blt_Tags_AppendAllTagsToObj) (Blt_Tags tags, Tcl_Obj *objPtr); /* 172 */
    Blt_Chain (*blt_Tags_GetItemList) (Blt_Tags tags, const char *tag); /* 173 */
    Blt_HashTable * (*blt_Tags_GetTable) (Blt_Tags tags); /* 174 */
    Blt_TreeKey (*blt_Tree_GetKey) (Blt_Tree tree, const char *string); /* 175 */
    Blt_TreeKey (*blt_Tree_GetKeyFromNode) (Blt_TreeNode node, const char *string); /* 176 */
    Blt_TreeKey (*blt_Tree_GetKeyFromInterp) (Tcl_Interp *interp, const char *string); /* 177 */
    Blt_TreeNode (*blt_Tree_CreateNode) (Blt_Tree tree, Blt_TreeNode parent, const char *name, long position); /* 178 */
    Blt_TreeNode (*blt_Tree_CreateNodeWithId) (Blt_Tree tree, Blt_TreeNode parent, const char *name, long inode, long position); /* 179 */
    int (*blt_Tree_DeleteNode) (Blt_Tree tree, Blt_TreeNode node); /* 180 */
    int (*blt_Tree_MoveNode) (Blt_Tree tree, Blt_TreeNode node, Blt_TreeNode parent, Blt_TreeNode before); /* 181 */
    Blt_TreeNode (*blt_Tree_GetNodeFromIndex) (Blt_Tree tree, long inode); /* 182 */
    Blt_TreeNode (*blt_Tree_FindChild) (Blt_TreeNode parent, const char *name); /* 183 */
    Blt_TreeNode (*blt_Tree_NextNode) (Blt_TreeNode root, Blt_TreeNode node); /* 184 */
    Blt_TreeNode (*blt_Tree_PrevNode) (Blt_TreeNode root, Blt_TreeNode node); /* 185 */
    Blt_TreeNode (*blt_Tree_FirstChild) (Blt_TreeNode parent); /* 186 */
    Blt_TreeNode (*blt_Tree_LastChild) (Blt_TreeNode parent); /* 187 */
    int (*blt_Tree_IsBefore) (Blt_TreeNode node1, Blt_TreeNode node2); /* 188 */
    int (*blt_Tree_IsAncestor) (Blt_TreeNode node1, Blt_TreeNode node2); /* 189 */
    int (*blt_Tree_PrivateValue) (Tcl_Interp *interp, Blt_Tree tree, Blt_TreeNode node, Blt_TreeKey key); /* 190 */
    int (*blt_Tree_PublicValue) (Tcl_Interp *interp, Blt_Tree tree, Blt_TreeNode node, Blt_TreeKey key); /* 191 */
    int (*blt_Tree_GetValue) (Tcl_Interp *interp, Blt_Tree tree, Blt_TreeNode node, const char *string, Tcl_Obj **valuePtr); /* 192 */
    int (*blt_Tree_ValueExists) (Blt_Tree tree, Blt_TreeNode node, const char *string); /* 193 */
    int (*blt_Tree_SetValue) (Tcl_Interp *interp, Blt_Tree tree, Blt_TreeNode node, const char *string, Tcl_Obj *valuePtr); /* 194 */
    int (*blt_Tree_UnsetValue) (Tcl_Interp *interp, Blt_Tree tree, Blt_TreeNode node, const char *string); /* 195 */
    int (*blt_Tree_AppendValue) (Tcl_Interp *interp, Blt_Tree tree, Blt_TreeNode node, const char *string, const char *value); /* 196 */
    int (*blt_Tree_ListAppendValue) (Tcl_Interp *interp, Blt_Tree tree, Blt_TreeNode node, const char *string, Tcl_Obj *valuePtr); /* 197 */
    int (*blt_Tree_GetArrayValue) (Tcl_Interp *interp, Blt_Tree tree, Blt_TreeNode node, const char *arrayName, const char *elemName, Tcl_Obj **valueObjPtrPtr); /* 198 */
    int (*blt_Tree_SetArrayValue) (Tcl_Interp *interp, Blt_Tree tree, Blt_TreeNode node, const char *arrayName, const char *elemName, Tcl_Obj *valueObjPtr); /* 199 */
    int (*blt_Tree_UnsetArrayValue) (Tcl_Interp *interp, Blt_Tree tree, Blt_TreeNode node, const char *arrayName, const char *elemName); /* 200 */
    int (*blt_Tree_AppendArrayValue) (Tcl_Interp *interp, Blt_Tree tree, Blt_TreeNode node, const char *arrayName, const char *elemName, const char *value); /* 201 */
    int (*blt_Tree_ListAppendArrayValue) (Tcl_Interp *interp, Blt_Tree tree, Blt_TreeNode node, const char *arrayName, const char *elemName, Tcl_Obj *valueObjPtr); /* 202 */
    int (*blt_Tree_ArrayValueExists) (Blt_Tree tree, Blt_TreeNode node, const char *arrayName, const char *elemName); /* 203 */
    int (*blt_Tree_ArrayNames) (Tcl_Interp *interp, Blt_Tree tree, Blt_TreeNode node, const char *arrayName, Tcl_Obj *listObjPtr); /* 204 */
    int (*blt_Tree_GetValueByKey) (Tcl_Interp *interp, Blt_Tree tree, Blt_TreeNode node, Blt_TreeKey key, Tcl_Obj **valuePtr); /* 205 */
    int (*blt_Tree_SetValueByKey) (Tcl_Interp *interp, Blt_Tree tree, Blt_TreeNode node, Blt_TreeKey key, Tcl_Obj *valuePtr); /* 206 */
    int (*blt_Tree_UnsetValueByKey) (Tcl_Interp *interp, Blt_Tree tree, Blt_TreeNode node, Blt_TreeKey key); /* 207 */
    int (*blt_Tree_AppendValueByKey) (Tcl_Interp *interp, Blt_Tree tree, Blt_TreeNode node, Blt_TreeKey key, const char *value); /* 208 */
    int (*blt_Tree_ListAppendValueByKey) (Tcl_Interp *interp, Blt_Tree tree, Blt_TreeNode node, Blt_TreeKey key, Tcl_Obj *valuePtr); /* 209 */
    int (*blt_Tree_ValueExistsByKey) (Blt_Tree tree, Blt_TreeNode node, Blt_TreeKey key); /* 210 */
    Blt_TreeKey (*blt_Tree_FirstKey) (Blt_Tree tree, Blt_TreeNode node, Blt_TreeKeyIterator *iterPtr); /* 211 */
    Blt_TreeKey (*blt_Tree_NextKey) (Blt_Tree tree, Blt_TreeKeyIterator *iterPtr); /* 212 */
    int (*blt_Tree_Apply) (Blt_TreeNode root, Blt_TreeApplyProc *proc, ClientData clientData); /* 213 */
    int (*blt_Tree_ApplyDFS) (Blt_TreeNode root, Blt_TreeApplyProc *proc, ClientData clientData, int order); /* 214 */
    int (*blt_Tree_ApplyBFS) (Blt_TreeNode root, Blt_TreeApplyProc *proc, ClientData clientData); /* 215 */
    int (*blt_Tree_SortNode) (Blt_Tree tree, Blt_TreeNode node, Blt_TreeCompareNodesProc *proc); /* 216 */
    int (*blt_Tree_Exists) (Tcl_Interp *interp, const char *name); /* 217 */
    Blt_Tree (*blt_Tree_Open) (Tcl_Interp *interp, const char *name, int flags); /* 218 */
    void (*blt_Tree_Close) (Blt_Tree tree); /* 219 */
    int (*blt_Tree_Attach) (Tcl_Interp *interp, Blt_Tree tree, const char *name); /* 220 */
    Blt_Tree (*blt_Tree_GetFromObj) (Tcl_Interp *interp, Tcl_Obj *objPtr); /* 221 */
    int (*blt_Tree_Size) (Blt_TreeNode node); /* 222 */
    Blt_TreeTrace (*blt_Tree_CreateTrace) (Blt_Tree tree, Blt_TreeNode node, const char *keyPattern, const char *tagName, unsigned int mask, Blt_TreeTraceProc *proc, ClientData clientData); /* 223 */
    void (*blt_Tree_DeleteTrace) (Blt_TreeTrace token); /* 224 */
    void (*blt_Tree_CreateEventHandler) (Blt_Tree tree, unsigned int mask, Blt_TreeNotifyEventProc *proc, ClientData clientData); /* 225 */
    void (*blt_Tree_DeleteEventHandler) (Blt_Tree tree, unsigned int mask, Blt_TreeNotifyEventProc *proc, ClientData clientData); /* 226 */
    void (*blt_Tree_RelabelNode) (Blt_Tree tree, Blt_TreeNode node, const char *string); /* 227 */
    void (*blt_Tree_RelabelNodeWithoutNotify) (Blt_TreeNode node, const char *string); /* 228 */
    const char * (*blt_Tree_NodeIdAscii) (Blt_TreeNode node); /* 229 */
    const char * (*blt_Tree_NodePath) (Blt_TreeNode node, Tcl_DString *resultPtr); /* 230 */
    const char * (*blt_Tree_NodeRelativePath) (Blt_TreeNode root, Blt_TreeNode node, const char *separator, unsigned int flags, Tcl_DString *resultPtr); /* 231 */
    long (*blt_Tree_NodePosition) (Blt_TreeNode node); /* 232 */
    void (*blt_Tree_ClearTags) (Blt_Tree tree, Blt_TreeNode node); /* 233 */
    int (*blt_Tree_HasTag) (Blt_Tree tree, Blt_TreeNode node, const char *tagName); /* 234 */
    void (*blt_Tree_AddTag) (Blt_Tree tree, Blt_TreeNode node, const char *tagName); /* 235 */
    void (*blt_Tree_RemoveTag) (Blt_Tree tree, Blt_TreeNode node, const char *tagName); /* 236 */
    void (*blt_Tree_ForgetTag) (Blt_Tree tree, const char *tagName); /* 237 */
    Blt_HashTable * (*blt_Tree_TagHashTable) (Blt_Tree tree, const char *tagName); /* 238 */
    int (*blt_Tree_TagTableIsShared) (Blt_Tree tree); /* 239 */
    void (*blt_Tree_NewTagTable) (Blt_Tree tree); /* 240 */
    Blt_HashEntry * (*blt_Tree_FirstTag) (Blt_Tree tree, Blt_HashSearch *searchPtr); /* 241 */
    void (*blt_Tree_DumpNode) (Blt_Tree tree, Blt_TreeNode root, Blt_TreeNode node, Tcl_DString *resultPtr); /* 242 */
    int (*blt_Tree_Dump) (Blt_Tree tree, Blt_TreeNode root, Tcl_DString *resultPtr); /* 243 */
    int (*blt_Tree_DumpToFile) (Tcl_Interp *interp, Blt_Tree tree, Blt_TreeNode root, const char *fileName); /* 244 */
    int (*blt_Tree_Restore) (Tcl_Interp *interp, Blt_Tree tree, Blt_TreeNode root, const char *string, unsigned int flags); /* 245 */
    int (*blt_Tree_RestoreFromFile) (Tcl_Interp *interp, Blt_Tree tree, Blt_TreeNode root, const char *fileName, unsigned int flags); /* 246 */
    long (*blt_Tree_Depth) (Blt_Tree tree); /* 247 */
    int (*blt_Tree_RegisterFormat) (Tcl_Interp *interp, const char *fmtName, Blt_TreeImportProc *importProc, Blt_TreeExportProc *exportProc); /* 248 */
    Blt_TreeTagEntry * (*blt_Tree_RememberTag) (Blt_Tree tree, const char *name); /* 249 */
    int (*blt_Tree_GetNodeFromObj) (Tcl_Interp *interp, Blt_Tree tree, Tcl_Obj *objPtr, Blt_TreeNode *nodePtr); /* 250 */
    int (*blt_Tree_GetNodeIterator) (Tcl_Interp *interp, Blt_Tree tree, Tcl_Obj *objPtr, Blt_TreeIterator *iterPtr); /* 251 */
    Blt_TreeNode (*blt_Tree_FirstTaggedNode) (Blt_TreeIterator *iterPtr); /* 252 */
    Blt_TreeNode (*blt_Tree_NextTaggedNode) (Blt_TreeIterator *iterPtr); /* 253 */
    double (*blt_VecMin) (Blt_Vector *vPtr); /* 254 */
    double (*blt_VecMax) (Blt_Vector *vPtr); /* 255 */
    Blt_VectorId (*blt_AllocVectorId) (Tcl_Interp *interp, const char *vecName); /* 256 */
    void (*blt_SetVectorChangedProc) (Blt_VectorId clientId, Blt_VectorChangedProc *proc, ClientData clientData); /* 257 */
    void (*blt_FreeVectorId) (Blt_VectorId clientId); /* 258 */
    int (*blt_GetVectorById) (Tcl_Interp *interp, Blt_VectorId clientId, Blt_Vector **vecPtrPtr); /* 259 */
    const char * (*blt_NameOfVectorId) (Blt_VectorId clientId); /* 260 */
    const char * (*blt_NameOfVector) (Blt_Vector *vecPtr); /* 261 */
    int (*blt_VectorNotifyPending) (Blt_VectorId clientId); /* 262 */
    int (*blt_CreateVector) (Tcl_Interp *interp, const char *vecName, int size, Blt_Vector **vecPtrPtr); /* 263 */
    int (*blt_CreateVector2) (Tcl_Interp *interp, const char *vecName, const char *cmdName, const char *varName, int initialSize, Blt_Vector **vecPtrPtr); /* 264 */
    int (*blt_GetVector) (Tcl_Interp *interp, const char *vecName, Blt_Vector **vecPtrPtr); /* 265 */
    int (*blt_GetVectorFromObj) (Tcl_Interp *interp, Tcl_Obj *objPtr, Blt_Vector **vecPtrPtr); /* 266 */
    int (*blt_VectorExists) (Tcl_Interp *interp, const char *vecName); /* 267 */
    int (*blt_ResetVector) (Blt_Vector *vecPtr, double *dataArr, int n, int arraySize, Tcl_FreeProc *freeProc); /* 268 */
    int (*blt_ResizeVector) (Blt_Vector *vecPtr, int n); /* 269 */
    int (*blt_DeleteVectorByName) (Tcl_Interp *interp, const char *vecName); /* 270 */
    int (*blt_DeleteVector) (Blt_Vector *vecPtr); /* 271 */
    int (*blt_ExprVector) (Tcl_Interp *interp, char *expr, Blt_Vector *vecPtr); /* 272 */
    void (*blt_InstallIndexProc) (Tcl_Interp *interp, const char *indexName, Blt_VectorIndexProc *procPtr); /* 273 */
    int (*blt_VectorExists2) (Tcl_Interp *interp, const char *vecName); /* 274 */
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
#ifndef blt_table_release_tags
#define blt_table_release_tags \
        (bltTclProcsPtr->blt_table_release_tags) /* 19 */
#endif
#ifndef blt_table_new_tags
#define blt_table_new_tags \
        (bltTclProcsPtr->blt_table_new_tags) /* 20 */
#endif
#ifndef blt_table_get_column_tag_table
#define blt_table_get_column_tag_table \
        (bltTclProcsPtr->blt_table_get_column_tag_table) /* 21 */
#endif
#ifndef blt_table_get_row_tag_table
#define blt_table_get_row_tag_table \
        (bltTclProcsPtr->blt_table_get_row_tag_table) /* 22 */
#endif
#ifndef blt_table_exists
#define blt_table_exists \
        (bltTclProcsPtr->blt_table_exists) /* 23 */
#endif
#ifndef blt_table_create
#define blt_table_create \
        (bltTclProcsPtr->blt_table_create) /* 24 */
#endif
#ifndef blt_table_open
#define blt_table_open \
        (bltTclProcsPtr->blt_table_open) /* 25 */
#endif
#ifndef blt_table_close
#define blt_table_close \
        (bltTclProcsPtr->blt_table_close) /* 26 */
#endif
#ifndef blt_table_same_object
#define blt_table_same_object \
        (bltTclProcsPtr->blt_table_same_object) /* 27 */
#endif
#ifndef blt_table_row_get_label_table
#define blt_table_row_get_label_table \
        (bltTclProcsPtr->blt_table_row_get_label_table) /* 28 */
#endif
#ifndef blt_table_column_get_label_table
#define blt_table_column_get_label_table \
        (bltTclProcsPtr->blt_table_column_get_label_table) /* 29 */
#endif
#ifndef blt_table_get_row
#define blt_table_get_row \
        (bltTclProcsPtr->blt_table_get_row) /* 30 */
#endif
#ifndef blt_table_get_column
#define blt_table_get_column \
        (bltTclProcsPtr->blt_table_get_column) /* 31 */
#endif
#ifndef blt_table_get_row_by_label
#define blt_table_get_row_by_label \
        (bltTclProcsPtr->blt_table_get_row_by_label) /* 32 */
#endif
#ifndef blt_table_get_column_by_label
#define blt_table_get_column_by_label \
        (bltTclProcsPtr->blt_table_get_column_by_label) /* 33 */
#endif
#ifndef blt_table_get_row_by_index
#define blt_table_get_row_by_index \
        (bltTclProcsPtr->blt_table_get_row_by_index) /* 34 */
#endif
#ifndef blt_table_get_column_by_index
#define blt_table_get_column_by_index \
        (bltTclProcsPtr->blt_table_get_column_by_index) /* 35 */
#endif
#ifndef blt_table_set_row_label
#define blt_table_set_row_label \
        (bltTclProcsPtr->blt_table_set_row_label) /* 36 */
#endif
#ifndef blt_table_set_column_label
#define blt_table_set_column_label \
        (bltTclProcsPtr->blt_table_set_column_label) /* 37 */
#endif
#ifndef blt_table_name_to_column_type
#define blt_table_name_to_column_type \
        (bltTclProcsPtr->blt_table_name_to_column_type) /* 38 */
#endif
#ifndef blt_table_set_column_type
#define blt_table_set_column_type \
        (bltTclProcsPtr->blt_table_set_column_type) /* 39 */
#endif
#ifndef blt_table_column_type_to_name
#define blt_table_column_type_to_name \
        (bltTclProcsPtr->blt_table_column_type_to_name) /* 40 */
#endif
#ifndef blt_table_set_column_tag
#define blt_table_set_column_tag \
        (bltTclProcsPtr->blt_table_set_column_tag) /* 41 */
#endif
#ifndef blt_table_set_row_tag
#define blt_table_set_row_tag \
        (bltTclProcsPtr->blt_table_set_row_tag) /* 42 */
#endif
#ifndef blt_table_create_row
#define blt_table_create_row \
        (bltTclProcsPtr->blt_table_create_row) /* 43 */
#endif
#ifndef blt_table_create_column
#define blt_table_create_column \
        (bltTclProcsPtr->blt_table_create_column) /* 44 */
#endif
#ifndef blt_table_extend_rows
#define blt_table_extend_rows \
        (bltTclProcsPtr->blt_table_extend_rows) /* 45 */
#endif
#ifndef blt_table_extend_columns
#define blt_table_extend_columns \
        (bltTclProcsPtr->blt_table_extend_columns) /* 46 */
#endif
#ifndef blt_table_delete_row
#define blt_table_delete_row \
        (bltTclProcsPtr->blt_table_delete_row) /* 47 */
#endif
#ifndef blt_table_delete_column
#define blt_table_delete_column \
        (bltTclProcsPtr->blt_table_delete_column) /* 48 */
#endif
#ifndef blt_table_move_row
#define blt_table_move_row \
        (bltTclProcsPtr->blt_table_move_row) /* 49 */
#endif
#ifndef blt_table_move_column
#define blt_table_move_column \
        (bltTclProcsPtr->blt_table_move_column) /* 50 */
#endif
#ifndef blt_table_get_obj
#define blt_table_get_obj \
        (bltTclProcsPtr->blt_table_get_obj) /* 51 */
#endif
#ifndef blt_table_set_obj
#define blt_table_set_obj \
        (bltTclProcsPtr->blt_table_set_obj) /* 52 */
#endif
#ifndef blt_table_get_string
#define blt_table_get_string \
        (bltTclProcsPtr->blt_table_get_string) /* 53 */
#endif
#ifndef blt_table_set_string_rep
#define blt_table_set_string_rep \
        (bltTclProcsPtr->blt_table_set_string_rep) /* 54 */
#endif
#ifndef blt_table_set_string
#define blt_table_set_string \
        (bltTclProcsPtr->blt_table_set_string) /* 55 */
#endif
#ifndef blt_table_append_string
#define blt_table_append_string \
        (bltTclProcsPtr->blt_table_append_string) /* 56 */
#endif
#ifndef blt_table_set_bytes
#define blt_table_set_bytes \
        (bltTclProcsPtr->blt_table_set_bytes) /* 57 */
#endif
#ifndef blt_table_get_double
#define blt_table_get_double \
        (bltTclProcsPtr->blt_table_get_double) /* 58 */
#endif
#ifndef blt_table_set_double
#define blt_table_set_double \
        (bltTclProcsPtr->blt_table_set_double) /* 59 */
#endif
#ifndef blt_table_get_long
#define blt_table_get_long \
        (bltTclProcsPtr->blt_table_get_long) /* 60 */
#endif
#ifndef blt_table_set_long
#define blt_table_set_long \
        (bltTclProcsPtr->blt_table_set_long) /* 61 */
#endif
#ifndef blt_table_get_boolean
#define blt_table_get_boolean \
        (bltTclProcsPtr->blt_table_get_boolean) /* 62 */
#endif
#ifndef blt_table_set_boolean
#define blt_table_set_boolean \
        (bltTclProcsPtr->blt_table_set_boolean) /* 63 */
#endif
#ifndef blt_table_get_value
#define blt_table_get_value \
        (bltTclProcsPtr->blt_table_get_value) /* 64 */
#endif
#ifndef blt_table_set_value
#define blt_table_set_value \
        (bltTclProcsPtr->blt_table_set_value) /* 65 */
#endif
#ifndef blt_table_unset_value
#define blt_table_unset_value \
        (bltTclProcsPtr->blt_table_unset_value) /* 66 */
#endif
#ifndef blt_table_value_exists
#define blt_table_value_exists \
        (bltTclProcsPtr->blt_table_value_exists) /* 67 */
#endif
#ifndef blt_table_value_string
#define blt_table_value_string \
        (bltTclProcsPtr->blt_table_value_string) /* 68 */
#endif
#ifndef blt_table_value_bytes
#define blt_table_value_bytes \
        (bltTclProcsPtr->blt_table_value_bytes) /* 69 */
#endif
#ifndef blt_table_value_length
#define blt_table_value_length \
        (bltTclProcsPtr->blt_table_value_length) /* 70 */
#endif
#ifndef blt_table_tags_are_shared
#define blt_table_tags_are_shared \
        (bltTclProcsPtr->blt_table_tags_are_shared) /* 71 */
#endif
#ifndef blt_table_clear_row_tags
#define blt_table_clear_row_tags \
        (bltTclProcsPtr->blt_table_clear_row_tags) /* 72 */
#endif
#ifndef blt_table_clear_column_tags
#define blt_table_clear_column_tags \
        (bltTclProcsPtr->blt_table_clear_column_tags) /* 73 */
#endif
#ifndef blt_table_get_row_tags
#define blt_table_get_row_tags \
        (bltTclProcsPtr->blt_table_get_row_tags) /* 74 */
#endif
#ifndef blt_table_get_column_tags
#define blt_table_get_column_tags \
        (bltTclProcsPtr->blt_table_get_column_tags) /* 75 */
#endif
#ifndef blt_table_get_tagged_rows
#define blt_table_get_tagged_rows \
        (bltTclProcsPtr->blt_table_get_tagged_rows) /* 76 */
#endif
#ifndef blt_table_get_tagged_columns
#define blt_table_get_tagged_columns \
        (bltTclProcsPtr->blt_table_get_tagged_columns) /* 77 */
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
#ifndef blt_table_first_column
#define blt_table_first_column \
        (bltTclProcsPtr->blt_table_first_column) /* 84 */
#endif
#ifndef blt_table_next_column
#define blt_table_next_column \
        (bltTclProcsPtr->blt_table_next_column) /* 85 */
#endif
#ifndef blt_table_first_row
#define blt_table_first_row \
        (bltTclProcsPtr->blt_table_first_row) /* 86 */
#endif
#ifndef blt_table_next_row
#define blt_table_next_row \
        (bltTclProcsPtr->blt_table_next_row) /* 87 */
#endif
#ifndef blt_table_row_spec
#define blt_table_row_spec \
        (bltTclProcsPtr->blt_table_row_spec) /* 88 */
#endif
#ifndef blt_table_column_spec
#define blt_table_column_spec \
        (bltTclProcsPtr->blt_table_column_spec) /* 89 */
#endif
#ifndef blt_table_iterate_rows
#define blt_table_iterate_rows \
        (bltTclProcsPtr->blt_table_iterate_rows) /* 90 */
#endif
#ifndef blt_table_iterate_columns
#define blt_table_iterate_columns \
        (bltTclProcsPtr->blt_table_iterate_columns) /* 91 */
#endif
#ifndef blt_table_iterate_rows_objv
#define blt_table_iterate_rows_objv \
        (bltTclProcsPtr->blt_table_iterate_rows_objv) /* 92 */
#endif
#ifndef blt_table_iterate_columns_objv
#define blt_table_iterate_columns_objv \
        (bltTclProcsPtr->blt_table_iterate_columns_objv) /* 93 */
#endif
#ifndef blt_table_free_iterator_objv
#define blt_table_free_iterator_objv \
        (bltTclProcsPtr->blt_table_free_iterator_objv) /* 94 */
#endif
#ifndef blt_table_iterate_all_rows
#define blt_table_iterate_all_rows \
        (bltTclProcsPtr->blt_table_iterate_all_rows) /* 95 */
#endif
#ifndef blt_table_iterate_all_columns
#define blt_table_iterate_all_columns \
        (bltTclProcsPtr->blt_table_iterate_all_columns) /* 96 */
#endif
#ifndef blt_table_first_tagged_row
#define blt_table_first_tagged_row \
        (bltTclProcsPtr->blt_table_first_tagged_row) /* 97 */
#endif
#ifndef blt_table_first_tagged_column
#define blt_table_first_tagged_column \
        (bltTclProcsPtr->blt_table_first_tagged_column) /* 98 */
#endif
#ifndef blt_table_next_tagged_row
#define blt_table_next_tagged_row \
        (bltTclProcsPtr->blt_table_next_tagged_row) /* 99 */
#endif
#ifndef blt_table_next_tagged_column
#define blt_table_next_tagged_column \
        (bltTclProcsPtr->blt_table_next_tagged_column) /* 100 */
#endif
#ifndef blt_table_list_rows
#define blt_table_list_rows \
        (bltTclProcsPtr->blt_table_list_rows) /* 101 */
#endif
#ifndef blt_table_list_columns
#define blt_table_list_columns \
        (bltTclProcsPtr->blt_table_list_columns) /* 102 */
#endif
#ifndef blt_table_clear_row_traces
#define blt_table_clear_row_traces \
        (bltTclProcsPtr->blt_table_clear_row_traces) /* 103 */
#endif
#ifndef blt_table_clear_column_traces
#define blt_table_clear_column_traces \
        (bltTclProcsPtr->blt_table_clear_column_traces) /* 104 */
#endif
#ifndef blt_table_create_trace
#define blt_table_create_trace \
        (bltTclProcsPtr->blt_table_create_trace) /* 105 */
#endif
#ifndef blt_table_trace_column
#define blt_table_trace_column \
        (bltTclProcsPtr->blt_table_trace_column) /* 106 */
#endif
#ifndef blt_table_trace_row
#define blt_table_trace_row \
        (bltTclProcsPtr->blt_table_trace_row) /* 107 */
#endif
#ifndef blt_table_create_column_trace
#define blt_table_create_column_trace \
        (bltTclProcsPtr->blt_table_create_column_trace) /* 108 */
#endif
#ifndef blt_table_create_column_tag_trace
#define blt_table_create_column_tag_trace \
        (bltTclProcsPtr->blt_table_create_column_tag_trace) /* 109 */
#endif
#ifndef blt_table_create_row_trace
#define blt_table_create_row_trace \
        (bltTclProcsPtr->blt_table_create_row_trace) /* 110 */
#endif
#ifndef blt_table_create_row_tag_trace
#define blt_table_create_row_tag_trace \
        (bltTclProcsPtr->blt_table_create_row_tag_trace) /* 111 */
#endif
#ifndef blt_table_delete_trace
#define blt_table_delete_trace \
        (bltTclProcsPtr->blt_table_delete_trace) /* 112 */
#endif
#ifndef blt_table_create_notifier
#define blt_table_create_notifier \
        (bltTclProcsPtr->blt_table_create_notifier) /* 113 */
#endif
#ifndef blt_table_create_row_notifier
#define blt_table_create_row_notifier \
        (bltTclProcsPtr->blt_table_create_row_notifier) /* 114 */
#endif
#ifndef blt_table_create_row_tag_notifier
#define blt_table_create_row_tag_notifier \
        (bltTclProcsPtr->blt_table_create_row_tag_notifier) /* 115 */
#endif
#ifndef blt_table_create_column_notifier
#define blt_table_create_column_notifier \
        (bltTclProcsPtr->blt_table_create_column_notifier) /* 116 */
#endif
#ifndef blt_table_create_column_tag_notifier
#define blt_table_create_column_tag_notifier \
        (bltTclProcsPtr->blt_table_create_column_tag_notifier) /* 117 */
#endif
#ifndef blt_table_delete_notifier
#define blt_table_delete_notifier \
        (bltTclProcsPtr->blt_table_delete_notifier) /* 118 */
#endif
#ifndef blt_table_sort_init
#define blt_table_sort_init \
        (bltTclProcsPtr->blt_table_sort_init) /* 119 */
#endif
#ifndef blt_table_sort_rows
#define blt_table_sort_rows \
        (bltTclProcsPtr->blt_table_sort_rows) /* 120 */
#endif
#ifndef blt_table_sort_rows_subset
#define blt_table_sort_rows_subset \
        (bltTclProcsPtr->blt_table_sort_rows_subset) /* 121 */
#endif
#ifndef blt_table_sort_finish
#define blt_table_sort_finish \
        (bltTclProcsPtr->blt_table_sort_finish) /* 122 */
#endif
#ifndef blt_table_get_compare_proc
#define blt_table_get_compare_proc \
        (bltTclProcsPtr->blt_table_get_compare_proc) /* 123 */
#endif
#ifndef blt_table_get_row_map
#define blt_table_get_row_map \
        (bltTclProcsPtr->blt_table_get_row_map) /* 124 */
#endif
#ifndef blt_table_get_column_map
#define blt_table_get_column_map \
        (bltTclProcsPtr->blt_table_get_column_map) /* 125 */
#endif
#ifndef blt_table_set_row_map
#define blt_table_set_row_map \
        (bltTclProcsPtr->blt_table_set_row_map) /* 126 */
#endif
#ifndef blt_table_set_column_map
#define blt_table_set_column_map \
        (bltTclProcsPtr->blt_table_set_column_map) /* 127 */
#endif
#ifndef blt_table_restore
#define blt_table_restore \
        (bltTclProcsPtr->blt_table_restore) /* 128 */
#endif
#ifndef blt_table_file_restore
#define blt_table_file_restore \
        (bltTclProcsPtr->blt_table_file_restore) /* 129 */
#endif
#ifndef blt_table_register_format
#define blt_table_register_format \
        (bltTclProcsPtr->blt_table_register_format) /* 130 */
#endif
#ifndef blt_table_unset_keys
#define blt_table_unset_keys \
        (bltTclProcsPtr->blt_table_unset_keys) /* 131 */
#endif
#ifndef blt_table_get_keys
#define blt_table_get_keys \
        (bltTclProcsPtr->blt_table_get_keys) /* 132 */
#endif
#ifndef blt_table_set_keys
#define blt_table_set_keys \
        (bltTclProcsPtr->blt_table_set_keys) /* 133 */
#endif
#ifndef blt_table_key_lookup
#define blt_table_key_lookup \
        (bltTclProcsPtr->blt_table_key_lookup) /* 134 */
#endif
#ifndef blt_table_get_column_limits
#define blt_table_get_column_limits \
        (bltTclProcsPtr->blt_table_get_column_limits) /* 135 */
#endif
#ifndef Blt_InitHashTable
#define Blt_InitHashTable \
        (bltTclProcsPtr->blt_InitHashTable) /* 136 */
#endif
#ifndef Blt_InitHashTableWithPool
#define Blt_InitHashTableWithPool \
        (bltTclProcsPtr->blt_InitHashTableWithPool) /* 137 */
#endif
#ifndef Blt_DeleteHashTable
#define Blt_DeleteHashTable \
        (bltTclProcsPtr->blt_DeleteHashTable) /* 138 */
#endif
#ifndef Blt_DeleteHashEntry
#define Blt_DeleteHashEntry \
        (bltTclProcsPtr->blt_DeleteHashEntry) /* 139 */
#endif
#ifndef Blt_FirstHashEntry
#define Blt_FirstHashEntry \
        (bltTclProcsPtr->blt_FirstHashEntry) /* 140 */
#endif
#ifndef Blt_NextHashEntry
#define Blt_NextHashEntry \
        (bltTclProcsPtr->blt_NextHashEntry) /* 141 */
#endif
#ifndef Blt_HashStats
#define Blt_HashStats \
        (bltTclProcsPtr->blt_HashStats) /* 142 */
#endif
#ifndef Blt_List_Init
#define Blt_List_Init \
        (bltTclProcsPtr->blt_List_Init) /* 143 */
#endif
#ifndef Blt_List_Reset
#define Blt_List_Reset \
        (bltTclProcsPtr->blt_List_Reset) /* 144 */
#endif
#ifndef Blt_List_Create
#define Blt_List_Create \
        (bltTclProcsPtr->blt_List_Create) /* 145 */
#endif
#ifndef Blt_List_Destroy
#define Blt_List_Destroy \
        (bltTclProcsPtr->blt_List_Destroy) /* 146 */
#endif
#ifndef Blt_List_CreateNode
#define Blt_List_CreateNode \
        (bltTclProcsPtr->blt_List_CreateNode) /* 147 */
#endif
#ifndef Blt_List_DeleteNode
#define Blt_List_DeleteNode \
        (bltTclProcsPtr->blt_List_DeleteNode) /* 148 */
#endif
#ifndef Blt_List_Append
#define Blt_List_Append \
        (bltTclProcsPtr->blt_List_Append) /* 149 */
#endif
#ifndef Blt_List_Prepend
#define Blt_List_Prepend \
        (bltTclProcsPtr->blt_List_Prepend) /* 150 */
#endif
#ifndef Blt_List_LinkAfter
#define Blt_List_LinkAfter \
        (bltTclProcsPtr->blt_List_LinkAfter) /* 151 */
#endif
#ifndef Blt_List_LinkBefore
#define Blt_List_LinkBefore \
        (bltTclProcsPtr->blt_List_LinkBefore) /* 152 */
#endif
#ifndef Blt_List_UnlinkNode
#define Blt_List_UnlinkNode \
        (bltTclProcsPtr->blt_List_UnlinkNode) /* 153 */
#endif
#ifndef Blt_List_GetNode
#define Blt_List_GetNode \
        (bltTclProcsPtr->blt_List_GetNode) /* 154 */
#endif
#ifndef Blt_List_DeleteNodeByKey
#define Blt_List_DeleteNodeByKey \
        (bltTclProcsPtr->blt_List_DeleteNodeByKey) /* 155 */
#endif
#ifndef Blt_List_GetNthNode
#define Blt_List_GetNthNode \
        (bltTclProcsPtr->blt_List_GetNthNode) /* 156 */
#endif
#ifndef Blt_List_Sort
#define Blt_List_Sort \
        (bltTclProcsPtr->blt_List_Sort) /* 157 */
#endif
#ifndef Blt_Pool_Create
#define Blt_Pool_Create \
        (bltTclProcsPtr->blt_Pool_Create) /* 158 */
#endif
#ifndef Blt_Pool_Destroy
#define Blt_Pool_Destroy \
        (bltTclProcsPtr->blt_Pool_Destroy) /* 159 */
#endif
#ifndef Blt_Tags_Create
#define Blt_Tags_Create \
        (bltTclProcsPtr->blt_Tags_Create) /* 160 */
#endif
#ifndef Blt_Tags_Destroy
#define Blt_Tags_Destroy \
        (bltTclProcsPtr->blt_Tags_Destroy) /* 161 */
#endif
#ifndef Blt_Tags_Init
#define Blt_Tags_Init \
        (bltTclProcsPtr->blt_Tags_Init) /* 162 */
#endif
#ifndef Blt_Tags_Reset
#define Blt_Tags_Reset \
        (bltTclProcsPtr->blt_Tags_Reset) /* 163 */
#endif
#ifndef Blt_Tags_ItemHasTag
#define Blt_Tags_ItemHasTag \
        (bltTclProcsPtr->blt_Tags_ItemHasTag) /* 164 */
#endif
#ifndef Blt_Tags_AddTag
#define Blt_Tags_AddTag \
        (bltTclProcsPtr->blt_Tags_AddTag) /* 165 */
#endif
#ifndef Blt_Tags_AddItemToTag
#define Blt_Tags_AddItemToTag \
        (bltTclProcsPtr->blt_Tags_AddItemToTag) /* 166 */
#endif
#ifndef Blt_Tags_ForgetTag
#define Blt_Tags_ForgetTag \
        (bltTclProcsPtr->blt_Tags_ForgetTag) /* 167 */
#endif
#ifndef Blt_Tags_RemoveItemFromTag
#define Blt_Tags_RemoveItemFromTag \
        (bltTclProcsPtr->blt_Tags_RemoveItemFromTag) /* 168 */
#endif
#ifndef Blt_Tags_ClearTagsFromItem
#define Blt_Tags_ClearTagsFromItem \
        (bltTclProcsPtr->blt_Tags_ClearTagsFromItem) /* 169 */
#endif
#ifndef Blt_Tags_AppendTagsToChain
#define Blt_Tags_AppendTagsToChain \
        (bltTclProcsPtr->blt_Tags_AppendTagsToChain) /* 170 */
#endif
#ifndef Blt_Tags_AppendTagsToObj
#define Blt_Tags_AppendTagsToObj \
        (bltTclProcsPtr->blt_Tags_AppendTagsToObj) /* 171 */
#endif
#ifndef Blt_Tags_AppendAllTagsToObj
#define Blt_Tags_AppendAllTagsToObj \
        (bltTclProcsPtr->blt_Tags_AppendAllTagsToObj) /* 172 */
#endif
#ifndef Blt_Tags_GetItemList
#define Blt_Tags_GetItemList \
        (bltTclProcsPtr->blt_Tags_GetItemList) /* 173 */
#endif
#ifndef Blt_Tags_GetTable
#define Blt_Tags_GetTable \
        (bltTclProcsPtr->blt_Tags_GetTable) /* 174 */
#endif
#ifndef Blt_Tree_GetKey
#define Blt_Tree_GetKey \
        (bltTclProcsPtr->blt_Tree_GetKey) /* 175 */
#endif
#ifndef Blt_Tree_GetKeyFromNode
#define Blt_Tree_GetKeyFromNode \
        (bltTclProcsPtr->blt_Tree_GetKeyFromNode) /* 176 */
#endif
#ifndef Blt_Tree_GetKeyFromInterp
#define Blt_Tree_GetKeyFromInterp \
        (bltTclProcsPtr->blt_Tree_GetKeyFromInterp) /* 177 */
#endif
#ifndef Blt_Tree_CreateNode
#define Blt_Tree_CreateNode \
        (bltTclProcsPtr->blt_Tree_CreateNode) /* 178 */
#endif
#ifndef Blt_Tree_CreateNodeWithId
#define Blt_Tree_CreateNodeWithId \
        (bltTclProcsPtr->blt_Tree_CreateNodeWithId) /* 179 */
#endif
#ifndef Blt_Tree_DeleteNode
#define Blt_Tree_DeleteNode \
        (bltTclProcsPtr->blt_Tree_DeleteNode) /* 180 */
#endif
#ifndef Blt_Tree_MoveNode
#define Blt_Tree_MoveNode \
        (bltTclProcsPtr->blt_Tree_MoveNode) /* 181 */
#endif
#ifndef Blt_Tree_GetNodeFromIndex
#define Blt_Tree_GetNodeFromIndex \
        (bltTclProcsPtr->blt_Tree_GetNodeFromIndex) /* 182 */
#endif
#ifndef Blt_Tree_FindChild
#define Blt_Tree_FindChild \
        (bltTclProcsPtr->blt_Tree_FindChild) /* 183 */
#endif
#ifndef Blt_Tree_NextNode
#define Blt_Tree_NextNode \
        (bltTclProcsPtr->blt_Tree_NextNode) /* 184 */
#endif
#ifndef Blt_Tree_PrevNode
#define Blt_Tree_PrevNode \
        (bltTclProcsPtr->blt_Tree_PrevNode) /* 185 */
#endif
#ifndef Blt_Tree_FirstChild
#define Blt_Tree_FirstChild \
        (bltTclProcsPtr->blt_Tree_FirstChild) /* 186 */
#endif
#ifndef Blt_Tree_LastChild
#define Blt_Tree_LastChild \
        (bltTclProcsPtr->blt_Tree_LastChild) /* 187 */
#endif
#ifndef Blt_Tree_IsBefore
#define Blt_Tree_IsBefore \
        (bltTclProcsPtr->blt_Tree_IsBefore) /* 188 */
#endif
#ifndef Blt_Tree_IsAncestor
#define Blt_Tree_IsAncestor \
        (bltTclProcsPtr->blt_Tree_IsAncestor) /* 189 */
#endif
#ifndef Blt_Tree_PrivateValue
#define Blt_Tree_PrivateValue \
        (bltTclProcsPtr->blt_Tree_PrivateValue) /* 190 */
#endif
#ifndef Blt_Tree_PublicValue
#define Blt_Tree_PublicValue \
        (bltTclProcsPtr->blt_Tree_PublicValue) /* 191 */
#endif
#ifndef Blt_Tree_GetValue
#define Blt_Tree_GetValue \
        (bltTclProcsPtr->blt_Tree_GetValue) /* 192 */
#endif
#ifndef Blt_Tree_ValueExists
#define Blt_Tree_ValueExists \
        (bltTclProcsPtr->blt_Tree_ValueExists) /* 193 */
#endif
#ifndef Blt_Tree_SetValue
#define Blt_Tree_SetValue \
        (bltTclProcsPtr->blt_Tree_SetValue) /* 194 */
#endif
#ifndef Blt_Tree_UnsetValue
#define Blt_Tree_UnsetValue \
        (bltTclProcsPtr->blt_Tree_UnsetValue) /* 195 */
#endif
#ifndef Blt_Tree_AppendValue
#define Blt_Tree_AppendValue \
        (bltTclProcsPtr->blt_Tree_AppendValue) /* 196 */
#endif
#ifndef Blt_Tree_ListAppendValue
#define Blt_Tree_ListAppendValue \
        (bltTclProcsPtr->blt_Tree_ListAppendValue) /* 197 */
#endif
#ifndef Blt_Tree_GetArrayValue
#define Blt_Tree_GetArrayValue \
        (bltTclProcsPtr->blt_Tree_GetArrayValue) /* 198 */
#endif
#ifndef Blt_Tree_SetArrayValue
#define Blt_Tree_SetArrayValue \
        (bltTclProcsPtr->blt_Tree_SetArrayValue) /* 199 */
#endif
#ifndef Blt_Tree_UnsetArrayValue
#define Blt_Tree_UnsetArrayValue \
        (bltTclProcsPtr->blt_Tree_UnsetArrayValue) /* 200 */
#endif
#ifndef Blt_Tree_AppendArrayValue
#define Blt_Tree_AppendArrayValue \
        (bltTclProcsPtr->blt_Tree_AppendArrayValue) /* 201 */
#endif
#ifndef Blt_Tree_ListAppendArrayValue
#define Blt_Tree_ListAppendArrayValue \
        (bltTclProcsPtr->blt_Tree_ListAppendArrayValue) /* 202 */
#endif
#ifndef Blt_Tree_ArrayValueExists
#define Blt_Tree_ArrayValueExists \
        (bltTclProcsPtr->blt_Tree_ArrayValueExists) /* 203 */
#endif
#ifndef Blt_Tree_ArrayNames
#define Blt_Tree_ArrayNames \
        (bltTclProcsPtr->blt_Tree_ArrayNames) /* 204 */
#endif
#ifndef Blt_Tree_GetValueByKey
#define Blt_Tree_GetValueByKey \
        (bltTclProcsPtr->blt_Tree_GetValueByKey) /* 205 */
#endif
#ifndef Blt_Tree_SetValueByKey
#define Blt_Tree_SetValueByKey \
        (bltTclProcsPtr->blt_Tree_SetValueByKey) /* 206 */
#endif
#ifndef Blt_Tree_UnsetValueByKey
#define Blt_Tree_UnsetValueByKey \
        (bltTclProcsPtr->blt_Tree_UnsetValueByKey) /* 207 */
#endif
#ifndef Blt_Tree_AppendValueByKey
#define Blt_Tree_AppendValueByKey \
        (bltTclProcsPtr->blt_Tree_AppendValueByKey) /* 208 */
#endif
#ifndef Blt_Tree_ListAppendValueByKey
#define Blt_Tree_ListAppendValueByKey \
        (bltTclProcsPtr->blt_Tree_ListAppendValueByKey) /* 209 */
#endif
#ifndef Blt_Tree_ValueExistsByKey
#define Blt_Tree_ValueExistsByKey \
        (bltTclProcsPtr->blt_Tree_ValueExistsByKey) /* 210 */
#endif
#ifndef Blt_Tree_FirstKey
#define Blt_Tree_FirstKey \
        (bltTclProcsPtr->blt_Tree_FirstKey) /* 211 */
#endif
#ifndef Blt_Tree_NextKey
#define Blt_Tree_NextKey \
        (bltTclProcsPtr->blt_Tree_NextKey) /* 212 */
#endif
#ifndef Blt_Tree_Apply
#define Blt_Tree_Apply \
        (bltTclProcsPtr->blt_Tree_Apply) /* 213 */
#endif
#ifndef Blt_Tree_ApplyDFS
#define Blt_Tree_ApplyDFS \
        (bltTclProcsPtr->blt_Tree_ApplyDFS) /* 214 */
#endif
#ifndef Blt_Tree_ApplyBFS
#define Blt_Tree_ApplyBFS \
        (bltTclProcsPtr->blt_Tree_ApplyBFS) /* 215 */
#endif
#ifndef Blt_Tree_SortNode
#define Blt_Tree_SortNode \
        (bltTclProcsPtr->blt_Tree_SortNode) /* 216 */
#endif
#ifndef Blt_Tree_Exists
#define Blt_Tree_Exists \
        (bltTclProcsPtr->blt_Tree_Exists) /* 217 */
#endif
#ifndef Blt_Tree_Open
#define Blt_Tree_Open \
        (bltTclProcsPtr->blt_Tree_Open) /* 218 */
#endif
#ifndef Blt_Tree_Close
#define Blt_Tree_Close \
        (bltTclProcsPtr->blt_Tree_Close) /* 219 */
#endif
#ifndef Blt_Tree_Attach
#define Blt_Tree_Attach \
        (bltTclProcsPtr->blt_Tree_Attach) /* 220 */
#endif
#ifndef Blt_Tree_GetFromObj
#define Blt_Tree_GetFromObj \
        (bltTclProcsPtr->blt_Tree_GetFromObj) /* 221 */
#endif
#ifndef Blt_Tree_Size
#define Blt_Tree_Size \
        (bltTclProcsPtr->blt_Tree_Size) /* 222 */
#endif
#ifndef Blt_Tree_CreateTrace
#define Blt_Tree_CreateTrace \
        (bltTclProcsPtr->blt_Tree_CreateTrace) /* 223 */
#endif
#ifndef Blt_Tree_DeleteTrace
#define Blt_Tree_DeleteTrace \
        (bltTclProcsPtr->blt_Tree_DeleteTrace) /* 224 */
#endif
#ifndef Blt_Tree_CreateEventHandler
#define Blt_Tree_CreateEventHandler \
        (bltTclProcsPtr->blt_Tree_CreateEventHandler) /* 225 */
#endif
#ifndef Blt_Tree_DeleteEventHandler
#define Blt_Tree_DeleteEventHandler \
        (bltTclProcsPtr->blt_Tree_DeleteEventHandler) /* 226 */
#endif
#ifndef Blt_Tree_RelabelNode
#define Blt_Tree_RelabelNode \
        (bltTclProcsPtr->blt_Tree_RelabelNode) /* 227 */
#endif
#ifndef Blt_Tree_RelabelNodeWithoutNotify
#define Blt_Tree_RelabelNodeWithoutNotify \
        (bltTclProcsPtr->blt_Tree_RelabelNodeWithoutNotify) /* 228 */
#endif
#ifndef Blt_Tree_NodeIdAscii
#define Blt_Tree_NodeIdAscii \
        (bltTclProcsPtr->blt_Tree_NodeIdAscii) /* 229 */
#endif
#ifndef Blt_Tree_NodePath
#define Blt_Tree_NodePath \
        (bltTclProcsPtr->blt_Tree_NodePath) /* 230 */
#endif
#ifndef Blt_Tree_NodeRelativePath
#define Blt_Tree_NodeRelativePath \
        (bltTclProcsPtr->blt_Tree_NodeRelativePath) /* 231 */
#endif
#ifndef Blt_Tree_NodePosition
#define Blt_Tree_NodePosition \
        (bltTclProcsPtr->blt_Tree_NodePosition) /* 232 */
#endif
#ifndef Blt_Tree_ClearTags
#define Blt_Tree_ClearTags \
        (bltTclProcsPtr->blt_Tree_ClearTags) /* 233 */
#endif
#ifndef Blt_Tree_HasTag
#define Blt_Tree_HasTag \
        (bltTclProcsPtr->blt_Tree_HasTag) /* 234 */
#endif
#ifndef Blt_Tree_AddTag
#define Blt_Tree_AddTag \
        (bltTclProcsPtr->blt_Tree_AddTag) /* 235 */
#endif
#ifndef Blt_Tree_RemoveTag
#define Blt_Tree_RemoveTag \
        (bltTclProcsPtr->blt_Tree_RemoveTag) /* 236 */
#endif
#ifndef Blt_Tree_ForgetTag
#define Blt_Tree_ForgetTag \
        (bltTclProcsPtr->blt_Tree_ForgetTag) /* 237 */
#endif
#ifndef Blt_Tree_TagHashTable
#define Blt_Tree_TagHashTable \
        (bltTclProcsPtr->blt_Tree_TagHashTable) /* 238 */
#endif
#ifndef Blt_Tree_TagTableIsShared
#define Blt_Tree_TagTableIsShared \
        (bltTclProcsPtr->blt_Tree_TagTableIsShared) /* 239 */
#endif
#ifndef Blt_Tree_NewTagTable
#define Blt_Tree_NewTagTable \
        (bltTclProcsPtr->blt_Tree_NewTagTable) /* 240 */
#endif
#ifndef Blt_Tree_FirstTag
#define Blt_Tree_FirstTag \
        (bltTclProcsPtr->blt_Tree_FirstTag) /* 241 */
#endif
#ifndef Blt_Tree_DumpNode
#define Blt_Tree_DumpNode \
        (bltTclProcsPtr->blt_Tree_DumpNode) /* 242 */
#endif
#ifndef Blt_Tree_Dump
#define Blt_Tree_Dump \
        (bltTclProcsPtr->blt_Tree_Dump) /* 243 */
#endif
#ifndef Blt_Tree_DumpToFile
#define Blt_Tree_DumpToFile \
        (bltTclProcsPtr->blt_Tree_DumpToFile) /* 244 */
#endif
#ifndef Blt_Tree_Restore
#define Blt_Tree_Restore \
        (bltTclProcsPtr->blt_Tree_Restore) /* 245 */
#endif
#ifndef Blt_Tree_RestoreFromFile
#define Blt_Tree_RestoreFromFile \
        (bltTclProcsPtr->blt_Tree_RestoreFromFile) /* 246 */
#endif
#ifndef Blt_Tree_Depth
#define Blt_Tree_Depth \
        (bltTclProcsPtr->blt_Tree_Depth) /* 247 */
#endif
#ifndef Blt_Tree_RegisterFormat
#define Blt_Tree_RegisterFormat \
        (bltTclProcsPtr->blt_Tree_RegisterFormat) /* 248 */
#endif
#ifndef Blt_Tree_RememberTag
#define Blt_Tree_RememberTag \
        (bltTclProcsPtr->blt_Tree_RememberTag) /* 249 */
#endif
#ifndef Blt_Tree_GetNodeFromObj
#define Blt_Tree_GetNodeFromObj \
        (bltTclProcsPtr->blt_Tree_GetNodeFromObj) /* 250 */
#endif
#ifndef Blt_Tree_GetNodeIterator
#define Blt_Tree_GetNodeIterator \
        (bltTclProcsPtr->blt_Tree_GetNodeIterator) /* 251 */
#endif
#ifndef Blt_Tree_FirstTaggedNode
#define Blt_Tree_FirstTaggedNode \
        (bltTclProcsPtr->blt_Tree_FirstTaggedNode) /* 252 */
#endif
#ifndef Blt_Tree_NextTaggedNode
#define Blt_Tree_NextTaggedNode \
        (bltTclProcsPtr->blt_Tree_NextTaggedNode) /* 253 */
#endif
#ifndef Blt_VecMin
#define Blt_VecMin \
        (bltTclProcsPtr->blt_VecMin) /* 254 */
#endif
#ifndef Blt_VecMax
#define Blt_VecMax \
        (bltTclProcsPtr->blt_VecMax) /* 255 */
#endif
#ifndef Blt_AllocVectorId
#define Blt_AllocVectorId \
        (bltTclProcsPtr->blt_AllocVectorId) /* 256 */
#endif
#ifndef Blt_SetVectorChangedProc
#define Blt_SetVectorChangedProc \
        (bltTclProcsPtr->blt_SetVectorChangedProc) /* 257 */
#endif
#ifndef Blt_FreeVectorId
#define Blt_FreeVectorId \
        (bltTclProcsPtr->blt_FreeVectorId) /* 258 */
#endif
#ifndef Blt_GetVectorById
#define Blt_GetVectorById \
        (bltTclProcsPtr->blt_GetVectorById) /* 259 */
#endif
#ifndef Blt_NameOfVectorId
#define Blt_NameOfVectorId \
        (bltTclProcsPtr->blt_NameOfVectorId) /* 260 */
#endif
#ifndef Blt_NameOfVector
#define Blt_NameOfVector \
        (bltTclProcsPtr->blt_NameOfVector) /* 261 */
#endif
#ifndef Blt_VectorNotifyPending
#define Blt_VectorNotifyPending \
        (bltTclProcsPtr->blt_VectorNotifyPending) /* 262 */
#endif
#ifndef Blt_CreateVector
#define Blt_CreateVector \
        (bltTclProcsPtr->blt_CreateVector) /* 263 */
#endif
#ifndef Blt_CreateVector2
#define Blt_CreateVector2 \
        (bltTclProcsPtr->blt_CreateVector2) /* 264 */
#endif
#ifndef Blt_GetVector
#define Blt_GetVector \
        (bltTclProcsPtr->blt_GetVector) /* 265 */
#endif
#ifndef Blt_GetVectorFromObj
#define Blt_GetVectorFromObj \
        (bltTclProcsPtr->blt_GetVectorFromObj) /* 266 */
#endif
#ifndef Blt_VectorExists
#define Blt_VectorExists \
        (bltTclProcsPtr->blt_VectorExists) /* 267 */
#endif
#ifndef Blt_ResetVector
#define Blt_ResetVector \
        (bltTclProcsPtr->blt_ResetVector) /* 268 */
#endif
#ifndef Blt_ResizeVector
#define Blt_ResizeVector \
        (bltTclProcsPtr->blt_ResizeVector) /* 269 */
#endif
#ifndef Blt_DeleteVectorByName
#define Blt_DeleteVectorByName \
        (bltTclProcsPtr->blt_DeleteVectorByName) /* 270 */
#endif
#ifndef Blt_DeleteVector
#define Blt_DeleteVector \
        (bltTclProcsPtr->blt_DeleteVector) /* 271 */
#endif
#ifndef Blt_ExprVector
#define Blt_ExprVector \
        (bltTclProcsPtr->blt_ExprVector) /* 272 */
#endif
#ifndef Blt_InstallIndexProc
#define Blt_InstallIndexProc \
        (bltTclProcsPtr->blt_InstallIndexProc) /* 273 */
#endif
#ifndef Blt_VectorExists2
#define Blt_VectorExists2 \
        (bltTclProcsPtr->blt_VectorExists2) /* 274 */
#endif

#endif /* defined(USE_BLT_STUBS) && !defined(BUILD_BLT_TCL_PROCS) */

/* !END!: Do not edit above this line. */
