library blt
interface bltTcl
hooks bltTclInt
declare 1 generic {
   void Blt_AllocInit(Blt_MallocProc *mallocProc, 
	Blt_ReallocProc *reallocProc, Blt_FreeProc *freeProc)
}
declare 2 generic {
   void Blt_Chain_Init(Blt_Chain chain)
}
declare 3 generic {
   Blt_Chain Blt_Chain_Create(void)
}
declare 4 generic {
   void Blt_Chain_Destroy(Blt_Chain chain)
}
declare 5 generic {
   Blt_ChainLink Blt_Chain_NewLink(void)
}
declare 6 generic {
   Blt_ChainLink Blt_Chain_AllocLink(size_t size)
}
declare 7 generic {
   Blt_ChainLink Blt_Chain_Append(Blt_Chain chain, 
	ClientData clientData)
}
declare 8 generic {
   Blt_ChainLink Blt_Chain_Prepend(Blt_Chain chain, 
	ClientData clientData)
}
declare 9 generic {
   void Blt_Chain_Reset(Blt_Chain chain)
}
declare 10 generic {
   void Blt_Chain_InitLink(Blt_ChainLink link)
}
declare 11 generic {
   void Blt_Chain_LinkAfter(Blt_Chain chain, Blt_ChainLink link, 
	Blt_ChainLink after)
}
declare 12 generic {
   void Blt_Chain_LinkBefore(Blt_Chain chain, Blt_ChainLink link, 
	Blt_ChainLink before)
}
declare 13 generic {
   void Blt_Chain_UnlinkLink(Blt_Chain chain, Blt_ChainLink link)
}
declare 14 generic {
   void Blt_Chain_DeleteLink(Blt_Chain chain, Blt_ChainLink link)
}
declare 15 generic {
   Blt_ChainLink Blt_Chain_GetNthLink(Blt_Chain chain, long position)
}
declare 16 generic {
   void Blt_Chain_Sort(Blt_Chain chain, Blt_ChainCompareProc *proc)
}
declare 17 generic {
   int Blt_Chain_IsBefore(Blt_ChainLink first, Blt_ChainLink last)
}
declare 18 generic {
   void Blt_List_Init(Blt_List list, size_t type)
}
declare 19 generic {
   void Blt_List_Reset(Blt_List list)
}
declare 20 generic {
   Blt_List Blt_List_Create(size_t type)
}
declare 21 generic {
   void Blt_List_Destroy(Blt_List list)
}
declare 22 generic {
   Blt_ListNode Blt_List_CreateNode(Blt_List list, const char *key)
}
declare 23 generic {
   void Blt_List_DeleteNode(Blt_ListNode node)
}
declare 24 generic {
   Blt_ListNode Blt_List_Append(Blt_List list, const char *key, 
	ClientData clientData)
}
declare 25 generic {
   Blt_ListNode Blt_List_Prepend(Blt_List list, const char *key, 
	ClientData clientData)
}
declare 26 generic {
   void Blt_List_LinkAfter(Blt_List list, Blt_ListNode node, 
	Blt_ListNode afterNode)
}
declare 27 generic {
   void Blt_List_LinkBefore(Blt_List list, Blt_ListNode node, 
	Blt_ListNode beforeNode)
}
declare 28 generic {
   void Blt_List_UnlinkNode(Blt_ListNode node)
}
declare 29 generic {
   Blt_ListNode Blt_List_GetNode(Blt_List list, const char *key)
}
declare 30 generic {
   void Blt_List_DeleteNodeByKey(Blt_List list, const char *key)
}
declare 31 generic {
   Blt_ListNode Blt_List_GetNthNode(Blt_List list, long position, 
	int direction)
}
declare 32 generic {
   void Blt_List_Sort(Blt_List list, Blt_ListCompareProc *proc)
}
declare 33 generic {
   void blt_table_release_tags(BLT_TABLE table)
}
declare 34 generic {
   int blt_table_exists(Tcl_Interp *interp, const char *name)
}
declare 35 generic {
   int blt_table_create(Tcl_Interp *interp, const char *name, 
	BLT_TABLE *tablePtr)
}
declare 36 generic {
   int blt_table_open(Tcl_Interp *interp, const char *name, 
	BLT_TABLE *tablePtr)
}
declare 37 generic {
   void blt_table_close(BLT_TABLE table)
}
declare 38 generic {
   int blt_table_same_object(BLT_TABLE table1, BLT_TABLE table2)
}
declare 39 generic {
   Blt_HashTable *blt_table_row_get_label_table(BLT_TABLE table, 
	const char *label)
}
declare 40 generic {
   Blt_HashTable *blt_table_column_get_label_table(BLT_TABLE table, 
	const char *label)
}
declare 41 generic {
   BLT_TABLE_ROW blt_table_row_find_by_label(BLT_TABLE table, 
	const char *label)
}
declare 42 generic {
   BLT_TABLE_COLUMN blt_table_column_find_by_label(BLT_TABLE table, 
	const char *label)
}
declare 43 generic {
   BLT_TABLE_ROW blt_table_row_find_by_index(BLT_TABLE table, long index)
}
declare 44 generic {
   BLT_TABLE_COLUMN blt_table_column_find_by_index(BLT_TABLE table, 
	long index)
}
declare 45 generic {
   int blt_table_row_set_label(Tcl_Interp *interp, BLT_TABLE table, 
	BLT_TABLE_ROW row, const char *label)
}
declare 46 generic {
   int blt_table_column_set_label(Tcl_Interp *interp, BLT_TABLE table, 
	BLT_TABLE_COLUMN column, const char *label)
}
declare 47 generic {
   BLT_TABLE_COLUMN_TYPE blt_table_column_convert_name_to_type(const char *typeName)
}
declare 48 generic {
   int blt_table_column_set_type(BLT_TABLE table, BLT_TABLE_COLUMN column,
	BLT_TABLE_COLUMN_TYPE type)
}
declare 49 generic {
   const char *blt_table_name_of_type(BLT_TABLE_COLUMN_TYPE type)
}
declare 50 generic {
   int blt_table_column_set_tag(Tcl_Interp *interp, BLT_TABLE table, 
	BLT_TABLE_COLUMN column, const char *tagName)
}
declare 51 generic {
   int blt_table_row_set_tag(Tcl_Interp *interp, BLT_TABLE table, 
	BLT_TABLE_ROW row, const char *tagName)
}
declare 52 generic {
   BLT_TABLE_ROW blt_table_row_create(Tcl_Interp *interp, BLT_TABLE table,
	const char *label)
}
declare 53 generic {
   BLT_TABLE_COLUMN blt_table_column_create(Tcl_Interp *interp, 
	BLT_TABLE table, const char *label)
}
declare 54 generic {
   int blt_table_row_extend(Tcl_Interp *interp, BLT_TABLE table,
	size_t n, BLT_TABLE_ROW *rows)
}
declare 55 generic {
   int blt_table_column_extend(Tcl_Interp *interp, BLT_TABLE table, 
	size_t n, BLT_TABLE_COLUMN *columms)
}
declare 56 generic {
   int blt_table_row_delete(BLT_TABLE table, BLT_TABLE_ROW row)
}
declare 57 generic {
   int blt_table_column_delete(BLT_TABLE table, BLT_TABLE_COLUMN column)
}
declare 58 generic {
   int blt_table_row_move(Tcl_Interp *interp, BLT_TABLE table, 
	BLT_TABLE_ROW from, BLT_TABLE_ROW to, size_t n)
}
declare 59 generic {
   int blt_table_column_move(Tcl_Interp *interp, BLT_TABLE table, 
	BLT_TABLE_COLUMN from, BLT_TABLE_COLUMN to, size_t n)
}
declare 60 generic {
   Tcl_Obj *blt_table_get_obj(BLT_TABLE table, BLT_TABLE_ROW row, 
	BLT_TABLE_COLUMN column)
}
declare 61 generic {
   int blt_table_set_obj(BLT_TABLE table, BLT_TABLE_ROW row,
	BLT_TABLE_COLUMN column, Tcl_Obj *objPtr)
}
declare 62 generic {
   const char *blt_table_get_string(BLT_TABLE table, BLT_TABLE_ROW row, 
	BLT_TABLE_COLUMN column)
}
declare 63 generic {
   int blt_table_set_string(BLT_TABLE table, BLT_TABLE_ROW row,
	BLT_TABLE_COLUMN column, const char *string, int length)
}
declare 64 generic {
   int blt_table_append_string(Tcl_Interp *interp, BLT_TABLE table, 
	BLT_TABLE_ROW row, BLT_TABLE_COLUMN column, const char *string, 
	int length)
}
declare 65 generic {
   double blt_table_get_double(BLT_TABLE table, BLT_TABLE_ROW row, 
	BLT_TABLE_COLUMN column)
}
declare 66 generic {
   int blt_table_set_double(BLT_TABLE table, BLT_TABLE_ROW row, 
	BLT_TABLE_COLUMN column, double value)
}
declare 67 generic {
   long blt_table_get_long(BLT_TABLE table, BLT_TABLE_ROW row, 
	BLT_TABLE_COLUMN column, long defValue)
}
declare 68 generic {
   int blt_table_set_long(BLT_TABLE table, BLT_TABLE_ROW row, 
	BLT_TABLE_COLUMN column, long value)
}
declare 69 generic {
   BLT_TABLE_VALUE blt_table_get_value(BLT_TABLE table, 
	BLT_TABLE_ROW row, BLT_TABLE_COLUMN column)
}
declare 70 generic {
   int blt_table_set_value(BLT_TABLE table, BLT_TABLE_ROW row, 
	BLT_TABLE_COLUMN column, BLT_TABLE_VALUE value)
}
declare 71 generic {
   int blt_table_unset_value(BLT_TABLE table, BLT_TABLE_ROW row, 
	BLT_TABLE_COLUMN column)
}
declare 72 generic {
   int blt_table_value_exists(BLT_TABLE table, BLT_TABLE_ROW row, 
	BLT_TABLE_COLUMN column)
}
declare 73 generic {
   Blt_HashTable *blt_table_row_find_tag_table(BLT_TABLE table, 
	const char *tagName)
}
declare 74 generic {
   Blt_HashTable *blt_table_column_find_tag_table(BLT_TABLE table, 
	const char *tagName)
}
declare 75 generic {
   Blt_Chain blt_table_row_tags(BLT_TABLE table, BLT_TABLE_ROW row)
}
declare 76 generic {
   Blt_Chain blt_table_column_tags(BLT_TABLE table, 
	BLT_TABLE_COLUMN column)
}
declare 77 generic {
   int blt_table_tags_are_shared(BLT_TABLE table)
}
declare 78 generic {
   int blt_table_row_has_tag(BLT_TABLE table, BLT_TABLE_ROW row, 
	const char *tagName)
}
declare 79 generic {
   int blt_table_column_has_tag(BLT_TABLE table, 
	BLT_TABLE_COLUMN column, const char *tagName)
}
declare 80 generic {
   int blt_table_row_forget_tag(Tcl_Interp *interp, BLT_TABLE table, 
	const char *tagName)
}
declare 81 generic {
   int blt_table_column_forget_tag(Tcl_Interp *interp, BLT_TABLE table, 
	const char *tagName)
}
declare 82 generic {
   int blt_table_row_unset_tag(Tcl_Interp *interp, BLT_TABLE table, 
	BLT_TABLE_ROW row, const char *tagName)
}
declare 83 generic {
   int blt_table_column_unset_tag(Tcl_Interp *interp, BLT_TABLE table, 
	BLT_TABLE_COLUMN column, const char *tagName)
}
declare 84 generic {
   Blt_HashEntry *blt_table_row_first_tag(BLT_TABLE table, 
	Blt_HashSearch *cursorPtr)
}
declare 85 generic {
   Blt_HashEntry *blt_table_column_first_tag(BLT_TABLE table, 
	Blt_HashSearch *cursorPtr)
}
declare 86 generic {
   BLT_TABLE_COLUMN blt_table_column_first(BLT_TABLE table)
}
declare 87 generic {
   BLT_TABLE_COLUMN blt_table_column_next(BLT_TABLE table, 
	BLT_TABLE_COLUMN column)
}
declare 88 generic {
   BLT_TABLE_ROW blt_table_row_first(BLT_TABLE table)
}
declare 89 generic {
   BLT_TABLE_ROW blt_table_row_next(BLT_TABLE table, BLT_TABLE_ROW row)
}
declare 90 generic {
   BLT_TABLE_ROWCOLUMN_SPEC blt_table_row_spec(BLT_TABLE table, 
	Tcl_Obj *objPtr, const char **sp)
}
declare 91 generic {
   BLT_TABLE_ROWCOLUMN_SPEC blt_table_column_spec(BLT_TABLE table, 
	Tcl_Obj *objPtr, const char **sp)
}
declare 92 generic {
   int blt_table_row_iterate(Tcl_Interp *interp, BLT_TABLE table, 
	Tcl_Obj *objPtr, BLT_TABLE_ITERATOR *iter)
}
declare 93 generic {
   int blt_table_column_iterate(Tcl_Interp *interp, BLT_TABLE table, 
	Tcl_Obj *objPtr, BLT_TABLE_ITERATOR *iter)
}
declare 94 generic {
   int blt_table_row_iterate_objv(Tcl_Interp *interp, BLT_TABLE table, 
	int objc, Tcl_Obj *const *objv, BLT_TABLE_ITERATOR *iterPtr)
}
declare 95 generic {
   int blt_table_column_iterate_objv(Tcl_Interp *interp, 
	BLT_TABLE table, int objc, Tcl_Obj *const *objv, 
	BLT_TABLE_ITERATOR *iterPtr)
}
declare 96 generic {
   void blt_table_free_iterator_objv(BLT_TABLE_ITERATOR *iterPtr)
}
declare 97 generic {
   void blt_table_row_iterate_all(BLT_TABLE table, 
	BLT_TABLE_ITERATOR *iterPtr)
}
declare 98 generic {
   void blt_table_column_iterate_all(BLT_TABLE table, 
	BLT_TABLE_ITERATOR *iterPtr)
}
declare 99 generic {
   BLT_TABLE_ROW blt_table_row_first_tagged(BLT_TABLE_ITERATOR *iter)
}
declare 100 generic {
   BLT_TABLE_COLUMN blt_table_column_first_tagged(
	BLT_TABLE_ITERATOR *iter)
}
declare 101 generic {
   BLT_TABLE_ROW blt_table_row_next_tagged(BLT_TABLE_ITERATOR *iter)
}
declare 102 generic {
   BLT_TABLE_COLUMN blt_table_column_next_tagged(
	BLT_TABLE_ITERATOR *iter)
}
declare 103 generic {
   BLT_TABLE_ROW blt_table_row_find(Tcl_Interp *interp, 
	BLT_TABLE table, Tcl_Obj *objPtr)
}
declare 104 generic {
   BLT_TABLE_COLUMN blt_table_column_find(Tcl_Interp *interp, 
	BLT_TABLE table, Tcl_Obj *objPtr)
}
declare 105 generic {
   int blt_table_list_rows(Tcl_Interp *interp, BLT_TABLE table, 
	int objc, Tcl_Obj *const *objv, Blt_Chain chain)
}
declare 106 generic {
   int blt_table_list_columns(Tcl_Interp *interp, BLT_TABLE table, 
	int objc, Tcl_Obj *const *objv, Blt_Chain chain)
}
declare 107 generic {
   void blt_table_row_clear_tags(BLT_TABLE table, BLT_TABLE_ROW row)
}
declare 108 generic {
   void blt_table_column_clear_tags(BLT_TABLE table, 
	BLT_TABLE_COLUMN column)
}
declare 109 generic {
   void blt_table_row_clear_traces(BLT_TABLE table, BLT_TABLE_ROW row)
}
declare 110 generic {
   void blt_table_column_clear_traces(BLT_TABLE table, 
	BLT_TABLE_COLUMN column)
}
declare 111 generic {
   BLT_TABLE_TRACE blt_table_create_trace(BLT_TABLE table, 
	BLT_TABLE_ROW row, BLT_TABLE_COLUMN column, const char *rowTag, 
	const char *columnTag, unsigned int mask, BLT_TABLE_TRACE_PROC *proc, 
	BLT_TABLE_TRACE_DELETE_PROC *deleteProc, ClientData clientData)
}
declare 112 generic {
   BLT_TABLE_TRACE blt_table_column_create_trace(BLT_TABLE table, 
	BLT_TABLE_COLUMN column, unsigned int mask, BLT_TABLE_TRACE_PROC *proc, 
	BLT_TABLE_TRACE_DELETE_PROC *deleteProc, ClientData clientData)
}
declare 113 generic {
   BLT_TABLE_TRACE blt_table_column_create_tag_trace(BLT_TABLE table, 
	const char *tag, unsigned int mask, BLT_TABLE_TRACE_PROC *proc, 
	BLT_TABLE_TRACE_DELETE_PROC *deleteProc, ClientData clientData)
}
declare 114 generic {
   BLT_TABLE_TRACE blt_table_row_create_trace(BLT_TABLE table,
	BLT_TABLE_ROW row, unsigned int mask, BLT_TABLE_TRACE_PROC *proc, 
	BLT_TABLE_TRACE_DELETE_PROC *deleteProc, ClientData clientData)
}
declare 115 generic {
   BLT_TABLE_TRACE blt_table_row_create_tag_trace(BLT_TABLE table, 
	const char *tag, unsigned int mask, BLT_TABLE_TRACE_PROC *proc, 
	BLT_TABLE_TRACE_DELETE_PROC *deleteProc, ClientData clientData)
}
declare 116 generic {
   void blt_table_delete_trace(BLT_TABLE_TRACE trace)
}
declare 117 generic {
   BLT_TABLE_NOTIFIER blt_table_row_create_notifier(Tcl_Interp *interp, 
	BLT_TABLE table, BLT_TABLE_ROW row, unsigned int mask, 
	BLT_TABLE_NOTIFY_EVENT_PROC *proc, 
	BLT_TABLE_NOTIFIER_DELETE_PROC *deleteProc,
	ClientData clientData)
}
declare 118 generic {
   BLT_TABLE_NOTIFIER blt_table_row_create_tag_notifier(
	Tcl_Interp *interp,
	BLT_TABLE table, const char *tag, unsigned int mask, 
	BLT_TABLE_NOTIFY_EVENT_PROC *proc, 
	BLT_TABLE_NOTIFIER_DELETE_PROC *deleteProc,
	ClientData clientData)
}
declare 119 generic {
   BLT_TABLE_NOTIFIER blt_table_column_create_notifier(
	Tcl_Interp *interp, BLT_TABLE table, BLT_TABLE_COLUMN column, 
	unsigned int mask, BLT_TABLE_NOTIFY_EVENT_PROC *proc, 
	BLT_TABLE_NOTIFIER_DELETE_PROC *deleteProc, ClientData clientData)
}
declare 120 generic {
   BLT_TABLE_NOTIFIER blt_table_column_create_tag_notifier(
	Tcl_Interp *interp, BLT_TABLE table, const char *tag, 
	unsigned int mask, BLT_TABLE_NOTIFY_EVENT_PROC *proc, 
	BLT_TABLE_NOTIFIER_DELETE_PROC *deleteProc, ClientData clientData)
}
declare 121 generic {
   void blt_table_delete_notifier(BLT_TABLE_NOTIFIER notifier)
}
declare 122 generic {
   void blt_table_sort_init(BLT_TABLE table, 
	BLT_TABLE_SORT_ORDER *order, size_t numCompares, unsigned int flags)
}
declare 123 generic {
   BLT_TABLE_ROW *blt_table_sort_rows(BLT_TABLE table)
}
declare 124 generic {
   void blt_table_sort_rows_subset(BLT_TABLE table, long numRows, 
	BLT_TABLE_ROW *rows)
}
declare 125 generic {
   void blt_table_sort_finish(void)
}
declare 126 generic {
   BLT_TABLE_COMPARE_PROC *blt_table_get_compare_proc(BLT_TABLE table, 
	BLT_TABLE_COLUMN column, unsigned int flags)
}
declare 127 generic {
   BLT_TABLE_ROW *blt_table_row_get_map(BLT_TABLE table)
}
declare 128 generic {
   BLT_TABLE_COLUMN *blt_table_column_get_map(BLT_TABLE table)
}
declare 129 generic {
   void blt_table_row_set_map(BLT_TABLE table, BLT_TABLE_ROW *map)
}
declare 130 generic {
   void blt_table_column_set_map(BLT_TABLE table, BLT_TABLE_COLUMN *map)
}
declare 131 generic {
   int blt_table_restore(Tcl_Interp *interp, BLT_TABLE table, 
	char *string, unsigned int flags)
}
declare 132 generic {
   int blt_table_file_restore(Tcl_Interp *interp, BLT_TABLE table, 
	const char *fileName, unsigned int flags)
}
declare 133 generic {
   int blt_table_register_format(Tcl_Interp *interp, const char *name, 
	BLT_TABLE_IMPORT_PROC *importProc, BLT_TABLE_EXPORT_PROC *exportProc)
}
declare 134 generic {
   void blt_table_unset_keys(BLT_TABLE table)
}
declare 135 generic {
   Blt_Chain blt_table_get_keys(BLT_TABLE table)
}
declare 136 generic {
   int blt_table_set_keys(BLT_TABLE table, Blt_Chain keys, int unique)
}
declare 137 generic {
   int blt_table_key_lookup(Tcl_Interp *interp, BLT_TABLE table,
	int objc, Tcl_Obj *const *objv, BLT_TABLE_ROW *rowPtr)
}
declare 138 generic {
   int blt_table_column_get_limits(Tcl_Interp *interp, BLT_TABLE table, 
	BLT_TABLE_COLUMN col, Tcl_Obj **minObjPtrPtr, Tcl_Obj **maxObjPtrPtr)
}
declare 139 generic {
   Blt_Pool Blt_Pool_Create(int type)
}
declare 140 generic {
   void Blt_Pool_Destroy(Blt_Pool pool)
}
declare 141 generic {
   Blt_TreeKey Blt_Tree_GetKey(Blt_Tree tree, const char *string)
}
declare 142 generic {
   Blt_TreeKey Blt_Tree_GetKeyFromNode(Blt_TreeNode node, 
	const char *string)
}
declare 143 generic {
   Blt_TreeKey Blt_Tree_GetKeyFromInterp(Tcl_Interp *interp,
	const char *string)
}
declare 144 generic {
   Blt_TreeNode Blt_Tree_CreateNode(Blt_Tree tree, Blt_TreeNode parent, 
	const char *name, long position)
}
declare 145 generic {
   Blt_TreeNode Blt_Tree_CreateNodeWithId(Blt_Tree tree, 
	Blt_TreeNode parent, const char *name, long inode, long position)
}
declare 146 generic {
   int Blt_Tree_DeleteNode(Blt_Tree tree, Blt_TreeNode node)
}
declare 147 generic {
   int Blt_Tree_MoveNode(Blt_Tree tree, Blt_TreeNode node, 
	Blt_TreeNode parent, Blt_TreeNode before)
}
declare 148 generic {
   Blt_TreeNode Blt_Tree_GetNode(Blt_Tree tree, long inode)
}
declare 149 generic {
   Blt_TreeNode Blt_Tree_FindChild(Blt_TreeNode parent, 
	const char *name)
}
declare 150 generic {
   Blt_TreeNode Blt_Tree_NextNode(Blt_TreeNode root, Blt_TreeNode node)
}
declare 151 generic {
   Blt_TreeNode Blt_Tree_PrevNode(Blt_TreeNode root, Blt_TreeNode node)
}
declare 152 generic {
   Blt_TreeNode Blt_Tree_FirstChild(Blt_TreeNode parent)
}
declare 153 generic {
   Blt_TreeNode Blt_Tree_LastChild(Blt_TreeNode parent)
}
declare 154 generic {
   int Blt_Tree_IsBefore(Blt_TreeNode node1, Blt_TreeNode node2)
}
declare 155 generic {
   int Blt_Tree_IsAncestor(Blt_TreeNode node1, Blt_TreeNode node2)
}
declare 156 generic {
   int Blt_Tree_PrivateValue(Tcl_Interp *interp, Blt_Tree tree, 
	Blt_TreeNode node, Blt_TreeKey key)
}
declare 157 generic {
   int Blt_Tree_PublicValue(Tcl_Interp *interp, Blt_Tree tree, 
	Blt_TreeNode node, Blt_TreeKey key)
}
declare 158 generic {
   int Blt_Tree_GetValue(Tcl_Interp *interp, Blt_Tree tree, 
	Blt_TreeNode node, const char *string, Tcl_Obj **valuePtr)
}
declare 159 generic {
   int Blt_Tree_ValueExists(Blt_Tree tree, Blt_TreeNode node, 
	const char *string)
}
declare 160 generic {
   int Blt_Tree_SetValue(Tcl_Interp *interp, Blt_Tree tree, 
	Blt_TreeNode node, const char *string, Tcl_Obj *valuePtr)
}
declare 161 generic {
   int Blt_Tree_UnsetValue(Tcl_Interp *interp, Blt_Tree tree, 
	Blt_TreeNode node, const char *string)
}
declare 162 generic {
   int Blt_Tree_AppendValue(Tcl_Interp *interp, Blt_Tree tree, 
	Blt_TreeNode node, const char *string, const char *value)
}
declare 163 generic {
   int Blt_Tree_ListAppendValue(Tcl_Interp *interp, Blt_Tree tree, 
	Blt_TreeNode node, const char *string, Tcl_Obj *valuePtr)
}
declare 164 generic {
   int Blt_Tree_GetArrayValue(Tcl_Interp *interp, Blt_Tree tree, 
	Blt_TreeNode node, const char *arrayName, const char *elemName, 
	Tcl_Obj **valueObjPtrPtr)
}
declare 165 generic {
   int Blt_Tree_SetArrayValue(Tcl_Interp *interp, Blt_Tree tree, 
	Blt_TreeNode node, const char *arrayName, const char *elemName, 
	Tcl_Obj *valueObjPtr)
}
declare 166 generic {
   int Blt_Tree_UnsetArrayValue(Tcl_Interp *interp, Blt_Tree tree, 
	Blt_TreeNode node, const char *arrayName, const char *elemName)
}
declare 167 generic {
   int Blt_Tree_AppendArrayValue(Tcl_Interp *interp, Blt_Tree tree, 
	Blt_TreeNode node, const char *arrayName, const char *elemName, 
	const char *value)
}
declare 168 generic {
   int Blt_Tree_ListAppendArrayValue(Tcl_Interp *interp, Blt_Tree tree, 
	Blt_TreeNode node, const char *arrayName, const char *elemName, 
	Tcl_Obj *valueObjPtr)
}
declare 169 generic {
   int Blt_Tree_ArrayValueExists(Blt_Tree tree, Blt_TreeNode node, 
	const char *arrayName, const char *elemName)
}
declare 170 generic {
   int Blt_Tree_ArrayNames(Tcl_Interp *interp, Blt_Tree tree, 
	Blt_TreeNode node, const char *arrayName, Tcl_Obj *listObjPtr)
}
declare 171 generic {
   int Blt_Tree_GetValueByKey(Tcl_Interp *interp, Blt_Tree tree, 
	Blt_TreeNode node, Blt_TreeKey key, Tcl_Obj **valuePtr)
}
declare 172 generic {
   int Blt_Tree_SetValueByKey(Tcl_Interp *interp, Blt_Tree tree, 
	Blt_TreeNode node, Blt_TreeKey key, Tcl_Obj *valuePtr)
}
declare 173 generic {
   int Blt_Tree_UnsetValueByKey(Tcl_Interp *interp, Blt_Tree tree, 
	Blt_TreeNode node, Blt_TreeKey key)
}
declare 174 generic {
   int Blt_Tree_AppendValueByKey(Tcl_Interp *interp, Blt_Tree tree, 
	Blt_TreeNode node, Blt_TreeKey key, const char *value)
}
declare 175 generic {
   int Blt_Tree_ListAppendValueByKey(Tcl_Interp *interp, Blt_Tree tree, 
	Blt_TreeNode node, Blt_TreeKey key, Tcl_Obj *valuePtr)
}
declare 176 generic {
   int Blt_Tree_ValueExistsByKey(Blt_Tree tree, Blt_TreeNode node, 
	Blt_TreeKey key)
}
declare 177 generic {
   Blt_TreeKey Blt_Tree_FirstKey(Blt_Tree tree, Blt_TreeNode node, 
	Blt_TreeKeyIterator *iterPtr)
}
declare 178 generic {
   Blt_TreeKey Blt_Tree_NextKey(Blt_Tree tree, 
	Blt_TreeKeyIterator *iterPtr)
}
declare 179 generic {
   int Blt_Tree_Apply(Blt_TreeNode root, Blt_TreeApplyProc *proc, 
	ClientData clientData)
}
declare 180 generic {
   int Blt_Tree_ApplyDFS(Blt_TreeNode root, Blt_TreeApplyProc *proc, 
	ClientData clientData, int order)
}
declare 181 generic {
   int Blt_Tree_ApplyBFS(Blt_TreeNode root, Blt_TreeApplyProc *proc, 
	ClientData clientData)
}
declare 182 generic {
   int Blt_Tree_SortNode(Blt_Tree tree, Blt_TreeNode node, 
	Blt_TreeCompareNodesProc *proc)
}
declare 183 generic {
   int Blt_Tree_Exists(Tcl_Interp *interp, const char *name)
}
declare 184 generic {
   Blt_Tree Blt_Tree_Open(Tcl_Interp *interp, const char *name, 
	int flags)
}
declare 185 generic {
   void Blt_Tree_Close(Blt_Tree tree)
}
declare 186 generic {
   int Blt_Tree_Attach(Tcl_Interp *interp, Blt_Tree tree, 
	const char *name)
}
declare 187 generic {
   Blt_Tree Blt_Tree_GetFromObj(Tcl_Interp *interp, Tcl_Obj *objPtr)
}
declare 188 generic {
   int Blt_Tree_Size(Blt_TreeNode node)
}
declare 189 generic {
   Blt_TreeTrace Blt_Tree_CreateTrace(Blt_Tree tree, Blt_TreeNode node, 
	const char *keyPattern, const char *tagName, unsigned int mask, 
	Blt_TreeTraceProc *proc, ClientData clientData)
}
declare 190 generic {
   void Blt_Tree_DeleteTrace(Blt_TreeTrace token)
}
declare 191 generic {
   void Blt_Tree_CreateEventHandler(Blt_Tree tree, unsigned int mask, 
	Blt_TreeNotifyEventProc *proc, ClientData clientData)
}
declare 192 generic {
   void Blt_Tree_DeleteEventHandler(Blt_Tree tree, unsigned int mask, 
	Blt_TreeNotifyEventProc *proc, ClientData clientData)
}
declare 193 generic {
   void Blt_Tree_RelabelNode(Blt_Tree tree, Blt_TreeNode node, 
	const char *string)
}
declare 194 generic {
   void Blt_Tree_RelabelNodeWithoutNotify(Blt_TreeNode node,
	const char *string)
}
declare 195 generic {
   const char *Blt_Tree_NodeIdAscii(Blt_TreeNode node)
}
declare 196 generic {
   const char *Blt_Tree_NodePath(Blt_TreeNode node, 
	Tcl_DString *resultPtr)
}
declare 197 generic {
   const char *Blt_Tree_NodeRelativePath(Blt_TreeNode root, 
	Blt_TreeNode node, const char *separator, unsigned int flags, 
	Tcl_DString *resultPtr)
}
declare 198 generic {
   long Blt_Tree_NodePosition(Blt_TreeNode node)
}
declare 199 generic {
   void Blt_Tree_ClearTags(Blt_Tree tree, Blt_TreeNode node)
}
declare 200 generic {
   int Blt_Tree_HasTag(Blt_Tree tree, Blt_TreeNode node, 
	const char *tagName)
}
declare 201 generic {
   void Blt_Tree_AddTag(Blt_Tree tree, Blt_TreeNode node, 
	const char *tagName)
}
declare 202 generic {
   void Blt_Tree_RemoveTag(Blt_Tree tree, Blt_TreeNode node,
	const char *tagName)
}
declare 203 generic {
   void Blt_Tree_ForgetTag(Blt_Tree tree, const char *tagName)
}
declare 204 generic {
   Blt_HashTable *Blt_Tree_TagHashTable(Blt_Tree tree, 
	const char *tagName)
}
declare 205 generic {
   int Blt_Tree_TagTableIsShared(Blt_Tree tree)
}
declare 206 generic {
   void Blt_Tree_NewTagTable(Blt_Tree tree)
}
declare 207 generic {
   Blt_HashEntry *Blt_Tree_FirstTag(Blt_Tree tree, 
	Blt_HashSearch *searchPtr)
}
declare 208 generic {
   void Blt_Tree_DumpNode(Blt_Tree tree, Blt_TreeNode root, 
	Blt_TreeNode node, Tcl_DString *resultPtr)
}
declare 209 generic {
   int Blt_Tree_Dump(Blt_Tree tree, Blt_TreeNode root, 
	Tcl_DString *resultPtr)
}
declare 210 generic {
   int Blt_Tree_DumpToFile(Tcl_Interp *interp, Blt_Tree tree, 
	Blt_TreeNode root, const char *fileName)
}
declare 211 generic {
   int Blt_Tree_Restore(Tcl_Interp *interp, Blt_Tree tree, 
	Blt_TreeNode root, const char *string, unsigned int flags)
}
declare 212 generic {
   int Blt_Tree_RestoreFromFile(Tcl_Interp *interp, Blt_Tree tree, 
	Blt_TreeNode root, const char *fileName, unsigned int flags)
}
declare 213 generic {
   long Blt_Tree_Depth(Blt_Tree tree)
}
declare 214 generic {
   int Blt_Tree_RegisterFormat(Tcl_Interp *interp, const char *fmtName, 
	Blt_TreeImportProc *importProc, Blt_TreeExportProc *exportProc)
}
declare 215 generic {
   Blt_TreeTagEntry *Blt_Tree_RememberTag(Blt_Tree tree, 
	const char *name)
}
declare 216 generic {
   void Blt_InitHashTable(Blt_HashTable *tablePtr, size_t keyType)
}
declare 217 generic {
   void Blt_InitHashTableWithPool(Blt_HashTable *tablePtr, 
	size_t keyType)
}
declare 218 generic {
   void Blt_DeleteHashTable(Blt_HashTable *tablePtr)
}
declare 219 generic {
   void Blt_DeleteHashEntry(Blt_HashTable *tablePtr, 
	Blt_HashEntry *entryPtr)
}
declare 220 generic {
   Blt_HashEntry *Blt_FirstHashEntry(Blt_HashTable *tablePtr, 
	Blt_HashSearch *searchPtr)
}
declare 221 generic {
   Blt_HashEntry *Blt_NextHashEntry(Blt_HashSearch *srchPtr)
}
declare 222 generic {
   const char *Blt_HashStats(Blt_HashTable *tablePtr)
}
declare 223 generic {
   double Blt_VecMin(Blt_Vector *vPtr)
}
declare 224 generic {
   double Blt_VecMax(Blt_Vector *vPtr)
}
declare 225 generic {
   Blt_VectorId Blt_AllocVectorId(Tcl_Interp *interp, 
	const char *vecName)
}
declare 226 generic {
   void Blt_SetVectorChangedProc(Blt_VectorId clientId, 
	Blt_VectorChangedProc *proc, ClientData clientData)
}
declare 227 generic {
   void Blt_FreeVectorId(Blt_VectorId clientId)
}
declare 228 generic {
   int Blt_GetVectorById(Tcl_Interp *interp, Blt_VectorId clientId, 
	Blt_Vector **vecPtrPtr)
}
declare 229 generic {
   const char *Blt_NameOfVectorId(Blt_VectorId clientId)
}
declare 230 generic {
   const char *Blt_NameOfVector(Blt_Vector *vecPtr)
}
declare 231 generic {
   int Blt_VectorNotifyPending(Blt_VectorId clientId)
}
declare 232 generic {
   int Blt_CreateVector(Tcl_Interp *interp, const char *vecName, 
	int size, Blt_Vector ** vecPtrPtr)
}
declare 233 generic {
   int Blt_CreateVector2(Tcl_Interp *interp, const char *vecName, 
	const char *cmdName, const char *varName, int initialSize, 
	Blt_Vector **vecPtrPtr)
}
declare 234 generic {
   int Blt_GetVector(Tcl_Interp *interp, const char *vecName, 
	Blt_Vector **vecPtrPtr)
}
declare 235 generic {
   int Blt_GetVectorFromObj(Tcl_Interp *interp, Tcl_Obj *objPtr, 
	Blt_Vector **vecPtrPtr)
}
declare 236 generic {
   int Blt_VectorExists(Tcl_Interp *interp, const char *vecName)
}
declare 237 generic {
   int Blt_ResetVector(Blt_Vector *vecPtr, double *dataArr, int n, 
	int arraySize, Tcl_FreeProc *freeProc)
}
declare 238 generic {
   int Blt_ResizeVector(Blt_Vector *vecPtr, int n)
}
declare 239 generic {
   int Blt_DeleteVectorByName(Tcl_Interp *interp, const char *vecName)
}
declare 240 generic {
   int Blt_DeleteVector(Blt_Vector *vecPtr)
}
declare 241 generic {
   int Blt_ExprVector(Tcl_Interp *interp, char *expr, 
	Blt_Vector *vecPtr)
}
declare 242 generic {
   void Blt_InstallIndexProc(Tcl_Interp *interp, const char *indexName,
	Blt_VectorIndexProc * procPtr)
}
declare 243 generic {
   int Blt_VectorExists2(Tcl_Interp *interp, const char *vecName)
}
