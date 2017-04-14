/* -*- mode: c; c-basic-offset: 4; indent-tabs-mode: nil -*- */
#define BUILD_BLT_TCL_PROCS 1
#include <bltInt.h>

extern BltTclIntProcs bltTclIntProcs;

/* !BEGIN!: Do not edit below this line. */

static BltTclStubHooks bltTclStubHooks = {
    &bltTclIntProcs
};

BltTclProcs bltTclProcs = {
    TCL_STUB_MAGIC,
    &bltTclStubHooks,
    NULL, /* 0 */
    Blt_AllocInit, /* 1 */
    Blt_Chain_Init, /* 2 */
    Blt_Chain_Create, /* 3 */
    Blt_Chain_Destroy, /* 4 */
    Blt_Chain_NewLink, /* 5 */
    Blt_Chain_AllocLink, /* 6 */
    Blt_Chain_Append, /* 7 */
    Blt_Chain_Prepend, /* 8 */
    Blt_Chain_Reset, /* 9 */
    Blt_Chain_InitLink, /* 10 */
    Blt_Chain_LinkAfter, /* 11 */
    Blt_Chain_LinkBefore, /* 12 */
    Blt_Chain_UnlinkLink, /* 13 */
    Blt_Chain_DeleteLink, /* 14 */
    Blt_Chain_GetNthLink, /* 15 */
    Blt_Chain_Sort, /* 16 */
    Blt_Chain_Reverse, /* 17 */
    Blt_Chain_IsBefore, /* 18 */
    Blt_InitHashTable, /* 19 */
    Blt_InitHashTableWithPool, /* 20 */
    Blt_DeleteHashTable, /* 21 */
    Blt_DeleteHashEntry, /* 22 */
    Blt_FirstHashEntry, /* 23 */
    Blt_NextHashEntry, /* 24 */
    Blt_HashStats, /* 25 */
    Blt_Tags_Create, /* 26 */
    Blt_Tags_Destroy, /* 27 */
    Blt_Tags_Init, /* 28 */
    Blt_Tags_Reset, /* 29 */
    Blt_Tags_ItemHasTag, /* 30 */
    Blt_Tags_AddTag, /* 31 */
    Blt_Tags_AddItemToTag, /* 32 */
    Blt_Tags_ForgetTag, /* 33 */
    Blt_Tags_RemoveItemFromTag, /* 34 */
    Blt_Tags_ClearTagsFromItem, /* 35 */
    Blt_Tags_AppendTagsToChain, /* 36 */
    Blt_Tags_AppendTagsToObj, /* 37 */
    Blt_Tags_AppendAllTagsToObj, /* 38 */
    Blt_Tags_GetItemList, /* 39 */
    Blt_Tags_GetTable, /* 40 */
    Blt_List_Init, /* 41 */
    Blt_List_Reset, /* 42 */
    Blt_List_Create, /* 43 */
    Blt_List_Destroy, /* 44 */
    Blt_List_CreateNode, /* 45 */
    Blt_List_DeleteNode, /* 46 */
    Blt_List_Append, /* 47 */
    Blt_List_Prepend, /* 48 */
    Blt_List_LinkAfter, /* 49 */
    Blt_List_LinkBefore, /* 50 */
    Blt_List_UnlinkNode, /* 51 */
    Blt_List_GetNode, /* 52 */
    Blt_List_DeleteNodeByKey, /* 53 */
    Blt_List_GetNthNode, /* 54 */
    Blt_List_Sort, /* 55 */
    Blt_Pool_Create, /* 56 */
    Blt_Pool_Destroy, /* 57 */
    Blt_Tree_GetKey, /* 58 */
    Blt_Tree_GetKeyFromNode, /* 59 */
    Blt_Tree_CreateNode, /* 60 */
    Blt_Tree_CreateNodeWithId, /* 61 */
    Blt_Tree_DeleteNode, /* 62 */
    Blt_Tree_MoveNode, /* 63 */
    Blt_Tree_GetNodeFromIndex, /* 64 */
    Blt_Tree_FindChild, /* 65 */
    Blt_Tree_NextNode, /* 66 */
    Blt_Tree_PrevNode, /* 67 */
    Blt_Tree_FirstChild, /* 68 */
    Blt_Tree_LastChild, /* 69 */
    Blt_Tree_IsBefore, /* 70 */
    Blt_Tree_IsAncestor, /* 71 */
    Blt_Tree_PrivateValue, /* 72 */
    Blt_Tree_PublicValue, /* 73 */
    Blt_Tree_GetValue, /* 74 */
    Blt_Tree_ValueExists, /* 75 */
    Blt_Tree_SetValue, /* 76 */
    Blt_Tree_UnsetValue, /* 77 */
    Blt_Tree_AppendValue, /* 78 */
    Blt_Tree_ListAppendValue, /* 79 */
    Blt_Tree_GetArrayValue, /* 80 */
    Blt_Tree_SetArrayValue, /* 81 */
    Blt_Tree_UnsetArrayValue, /* 82 */
    Blt_Tree_AppendArrayValue, /* 83 */
    Blt_Tree_ListAppendArrayValue, /* 84 */
    Blt_Tree_ArrayValueExists, /* 85 */
    Blt_Tree_ArrayNames, /* 86 */
    Blt_Tree_GetValueByKey, /* 87 */
    Blt_Tree_SetValueByKey, /* 88 */
    Blt_Tree_UnsetValueByKey, /* 89 */
    Blt_Tree_AppendValueByKey, /* 90 */
    Blt_Tree_ListAppendValueByKey, /* 91 */
    Blt_Tree_ValueExistsByKey, /* 92 */
    Blt_Tree_FirstKey, /* 93 */
    Blt_Tree_NextKey, /* 94 */
    Blt_Tree_Apply, /* 95 */
    Blt_Tree_ApplyDFS, /* 96 */
    Blt_Tree_ApplyBFS, /* 97 */
    Blt_Tree_SortNode, /* 98 */
    Blt_Tree_Exists, /* 99 */
    Blt_Tree_Open, /* 100 */
    Blt_Tree_Close, /* 101 */
    Blt_Tree_Attach, /* 102 */
    Blt_Tree_GetFromObj, /* 103 */
    Blt_Tree_Size, /* 104 */
    Blt_Tree_CreateTrace, /* 105 */
    Blt_Tree_DeleteTrace, /* 106 */
    Blt_Tree_CreateEventHandler, /* 107 */
    Blt_Tree_DeleteEventHandler, /* 108 */
    Blt_Tree_RelabelNode, /* 109 */
    Blt_Tree_RelabelNodeWithoutNotify, /* 110 */
    Blt_Tree_NodeIdAscii, /* 111 */
    Blt_Tree_NodePath, /* 112 */
    Blt_Tree_NodeRelativePath, /* 113 */
    Blt_Tree_NodePosition, /* 114 */
    Blt_Tree_ClearTags, /* 115 */
    Blt_Tree_HasTag, /* 116 */
    Blt_Tree_AddTag, /* 117 */
    Blt_Tree_RemoveTag, /* 118 */
    Blt_Tree_ForgetTag, /* 119 */
    Blt_Tree_TagHashTable, /* 120 */
    Blt_Tree_TagTableIsShared, /* 121 */
    Blt_Tree_NewTagTable, /* 122 */
    Blt_Tree_FirstTag, /* 123 */
    Blt_Tree_Depth, /* 124 */
    Blt_Tree_RegisterFormat, /* 125 */
    Blt_Tree_RememberTag, /* 126 */
    Blt_Tree_GetNodeFromObj, /* 127 */
    Blt_Tree_GetNodeIterator, /* 128 */
    Blt_Tree_FirstTaggedNode, /* 129 */
    Blt_Tree_NextTaggedNode, /* 130 */
    blt_table_release_tags, /* 131 */
    blt_table_new_tags, /* 132 */
    blt_table_get_column_tag_table, /* 133 */
    blt_table_get_row_tag_table, /* 134 */
    blt_table_exists, /* 135 */
    blt_table_create, /* 136 */
    blt_table_open, /* 137 */
    blt_table_close, /* 138 */
    blt_table_clear, /* 139 */
    blt_table_pack, /* 140 */
    blt_table_same_object, /* 141 */
    blt_table_row_get_label_table, /* 142 */
    blt_table_column_get_label_table, /* 143 */
    blt_table_get_row, /* 144 */
    blt_table_get_column, /* 145 */
    blt_table_get_row_by_label, /* 146 */
    blt_table_get_column_by_label, /* 147 */
    blt_table_get_row_by_index, /* 148 */
    blt_table_get_column_by_index, /* 149 */
    blt_table_set_row_label, /* 150 */
    blt_table_set_column_label, /* 151 */
    blt_table_name_to_column_type, /* 152 */
    blt_table_set_column_type, /* 153 */
    blt_table_column_type_to_name, /* 154 */
    blt_table_set_column_tag, /* 155 */
    blt_table_set_row_tag, /* 156 */
    blt_table_create_row, /* 157 */
    blt_table_create_column, /* 158 */
    blt_table_extend_rows, /* 159 */
    blt_table_extend_columns, /* 160 */
    blt_table_delete_row, /* 161 */
    blt_table_delete_column, /* 162 */
    blt_table_move_rows, /* 163 */
    blt_table_move_columns, /* 164 */
    blt_table_get_obj, /* 165 */
    blt_table_set_obj, /* 166 */
    blt_table_get_string, /* 167 */
    blt_table_set_string_rep, /* 168 */
    blt_table_set_string, /* 169 */
    blt_table_append_string, /* 170 */
    blt_table_set_bytes, /* 171 */
    blt_table_get_double, /* 172 */
    blt_table_set_double, /* 173 */
    blt_table_get_long, /* 174 */
    blt_table_set_long, /* 175 */
    blt_table_get_boolean, /* 176 */
    blt_table_set_boolean, /* 177 */
    blt_table_get_value, /* 178 */
    blt_table_set_value, /* 179 */
    blt_table_unset_value, /* 180 */
    blt_table_value_exists, /* 181 */
    blt_table_value_string, /* 182 */
    blt_table_value_bytes, /* 183 */
    blt_table_value_length, /* 184 */
    blt_table_tags_are_shared, /* 185 */
    blt_table_clear_row_tags, /* 186 */
    blt_table_clear_column_tags, /* 187 */
    blt_table_get_row_tags, /* 188 */
    blt_table_get_column_tags, /* 189 */
    blt_table_get_tagged_rows, /* 190 */
    blt_table_get_tagged_columns, /* 191 */
    blt_table_row_has_tag, /* 192 */
    blt_table_column_has_tag, /* 193 */
    blt_table_forget_row_tag, /* 194 */
    blt_table_forget_column_tag, /* 195 */
    blt_table_unset_row_tag, /* 196 */
    blt_table_unset_column_tag, /* 197 */
    blt_table_first_column, /* 198 */
    blt_table_last_column, /* 199 */
    blt_table_next_column, /* 200 */
    blt_table_previous_column, /* 201 */
    blt_table_first_row, /* 202 */
    blt_table_last_row, /* 203 */
    blt_table_next_row, /* 204 */
    blt_table_previous_row, /* 205 */
    blt_table_row_spec, /* 206 */
    blt_table_column_spec, /* 207 */
    blt_table_iterate_rows, /* 208 */
    blt_table_iterate_columns, /* 209 */
    blt_table_iterate_rows_objv, /* 210 */
    blt_table_iterate_columns_objv, /* 211 */
    blt_table_free_iterator_objv, /* 212 */
    blt_table_iterate_all_rows, /* 213 */
    blt_table_iterate_all_columns, /* 214 */
    blt_table_first_tagged_row, /* 215 */
    blt_table_first_tagged_column, /* 216 */
    blt_table_next_tagged_row, /* 217 */
    blt_table_next_tagged_column, /* 218 */
    blt_table_list_rows, /* 219 */
    blt_table_list_columns, /* 220 */
    blt_table_clear_row_traces, /* 221 */
    blt_table_clear_column_traces, /* 222 */
    blt_table_create_trace, /* 223 */
    blt_table_trace_column, /* 224 */
    blt_table_trace_row, /* 225 */
    blt_table_create_column_trace, /* 226 */
    blt_table_create_column_tag_trace, /* 227 */
    blt_table_create_row_trace, /* 228 */
    blt_table_create_row_tag_trace, /* 229 */
    blt_table_delete_trace, /* 230 */
    blt_table_create_notifier, /* 231 */
    blt_table_create_row_notifier, /* 232 */
    blt_table_create_row_tag_notifier, /* 233 */
    blt_table_create_column_notifier, /* 234 */
    blt_table_create_column_tag_notifier, /* 235 */
    blt_table_delete_notifier, /* 236 */
    blt_table_sort_init, /* 237 */
    blt_table_sort_rows, /* 238 */
    blt_table_sort_rows_subset, /* 239 */
    blt_table_sort_finish, /* 240 */
    blt_table_get_compare_proc, /* 241 */
    blt_table_get_row_map, /* 242 */
    blt_table_get_column_map, /* 243 */
    blt_table_set_row_map, /* 244 */
    blt_table_set_column_map, /* 245 */
    blt_table_restore, /* 246 */
    blt_table_file_restore, /* 247 */
    blt_table_register_format, /* 248 */
    blt_table_unset_keys, /* 249 */
    blt_table_get_keys, /* 250 */
    blt_table_set_keys, /* 251 */
    blt_table_key_lookup, /* 252 */
    blt_table_get_column_limits, /* 253 */
    blt_table_row, /* 254 */
    blt_table_column, /* 255 */
    blt_table_row_index, /* 256 */
    blt_table_column_index, /* 257 */
    Blt_VecMin, /* 258 */
    Blt_VecMax, /* 259 */
    Blt_AllocVectorId, /* 260 */
    Blt_SetVectorChangedProc, /* 261 */
    Blt_FreeVectorId, /* 262 */
    Blt_GetVectorById, /* 263 */
    Blt_NameOfVectorId, /* 264 */
    Blt_NameOfVector, /* 265 */
    Blt_VectorNotifyPending, /* 266 */
    Blt_CreateVector, /* 267 */
    Blt_CreateVector2, /* 268 */
    Blt_GetVector, /* 269 */
    Blt_GetVectorFromObj, /* 270 */
    Blt_VectorExists, /* 271 */
    Blt_ResetVector, /* 272 */
    Blt_ResizeVector, /* 273 */
    Blt_DeleteVectorByName, /* 274 */
    Blt_DeleteVector, /* 275 */
    Blt_ExprVector, /* 276 */
    Blt_InstallIndexProc, /* 277 */
    Blt_VectorExists2, /* 278 */
};

/* !END!: Do not edit above this line. */
