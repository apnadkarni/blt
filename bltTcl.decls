library blt
interface bltTcl
hooks bltTclInt
declare 1 generic {
   void Blt_AllocInit(Blt_MallocProc *mallocProc, 
	Blt_ReallocProc *reallocProc, Blt_FreeProc *freeProc)
}
declare 2 generic {
   void *Blt_Malloc(size_t size)
}
declare 3 generic {
   void *Blt_Realloc(void *ptr, size_t size)
}
declare 4 generic {
   void Blt_Free(const void *ptr)
}
declare 5 generic {
   void *Blt_Calloc(size_t numElem, size_t size)
}
declare 6 generic {
   char *Blt_Strdup(const char *string)
}
declare 7 generic {
   void *Blt_MallocAbortOnError(size_t size, const char *file,int line)
}
declare 8 generic {
   void *Blt_CallocAbortOnError(size_t numElem, size_t size, 
	const char *file, int line)
}
declare 9 generic {
   char *Blt_StrdupAbortOnError(const char *ptr, const char *file, 
	int line)
}
declare 10 generic {
   void Blt_Chain_Init(Blt_Chain chain)
}
declare 11 generic {
   Blt_Chain Blt_Chain_Create(void)
}
declare 12 generic {
   void Blt_Chain_Destroy(Blt_Chain chain)
}
declare 13 generic {
   Blt_ChainLink Blt_Chain_NewLink(void)
}
declare 14 generic {
   Blt_ChainLink Blt_Chain_AllocLink(size_t size)
}
declare 15 generic {
   Blt_ChainLink Blt_Chain_Append(Blt_Chain chain, 
	ClientData clientData)
}
declare 16 generic {
   Blt_ChainLink Blt_Chain_Prepend(Blt_Chain chain, 
	ClientData clientData)
}
declare 17 generic {
   void Blt_Chain_Reset(Blt_Chain chain)
}
declare 18 generic {
   void Blt_Chain_InitLink(Blt_ChainLink link)
}
declare 19 generic {
   void Blt_Chain_LinkAfter(Blt_Chain chain, Blt_ChainLink link, 
	Blt_ChainLink after)
}
declare 20 generic {
   void Blt_Chain_LinkBefore(Blt_Chain chain, Blt_ChainLink link, 
	Blt_ChainLink before)
}
declare 21 generic {
   void Blt_Chain_UnlinkLink(Blt_Chain chain, Blt_ChainLink link)
}
declare 22 generic {
   void Blt_Chain_DeleteLink(Blt_Chain chain, Blt_ChainLink link)
}
declare 23 generic {
   Blt_ChainLink Blt_Chain_GetNthLink(Blt_Chain chain, long position)
}
declare 24 generic {
   void Blt_Chain_Sort(Blt_Chain chain, Blt_ChainCompareProc *proc)
}
declare 25 generic {
   int Blt_Chain_IsBefore(Blt_ChainLink first, Blt_ChainLink last)
}
declare 26 generic {
   void Blt_List_Init(Blt_List list, size_t type)
}
declare 27 generic {
   void Blt_List_Reset(Blt_List list)
}
declare 28 generic {
   Blt_List Blt_List_Create(size_t type)
}
declare 29 generic {
   void Blt_List_Destroy(Blt_List list)
}
declare 30 generic {
   Blt_ListNode Blt_List_CreateNode(Blt_List list, const char *key)
}
declare 31 generic {
   void Blt_List_DeleteNode(Blt_ListNode node)
}
declare 32 generic {
   Blt_ListNode Blt_List_Append(Blt_List list, const char *key, 
	ClientData clientData)
}
declare 33 generic {
   Blt_ListNode Blt_List_Prepend(Blt_List list, const char *key, 
	ClientData clientData)
}
declare 34 generic {
   void Blt_List_LinkAfter(Blt_List list, Blt_ListNode node, 
	Blt_ListNode afterNode)
}
declare 35 generic {
   void Blt_List_LinkBefore(Blt_List list, Blt_ListNode node, 
	Blt_ListNode beforeNode)
}
declare 36 generic {
   void Blt_List_UnlinkNode(Blt_ListNode node)
}
declare 37 generic {
   Blt_ListNode Blt_List_GetNode(Blt_List list, const char *key)
}
declare 38 generic {
   void Blt_List_DeleteNodeByKey(Blt_List list, const char *key)
}
declare 39 generic {
   Blt_ListNode Blt_List_GetNthNode(Blt_List list, long position, 
	int direction)
}
declare 40 generic {
   void Blt_List_Sort(Blt_List list, Blt_ListCompareProc *proc)
}
declare 41 generic {
   void blt_table_release_tags(BLT_TABLE table)
}
declare 42 generic {
   int blt_table_exists(Tcl_Interp *interp, const char *name)
}
declare 43 generic {
   int blt_table_create(Tcl_Interp *interp, const char *name, 
	BLT_TABLE *tablePtr)
}
declare 44 generic {
   int blt_table_open(Tcl_Interp *interp, const char *name, 
	BLT_TABLE *tablePtr)
}
declare 45 generic {
   void blt_table_close(BLT_TABLE table)
}
declare 46 generic {
   int blt_table_same_object(BLT_TABLE table1, BLT_TABLE table2)
}
declare 47 generic {
   Blt_HashTable *blt_table_row_get_label_table(BLT_TABLE table, 
	const char *label)
}
declare 48 generic {
   Blt_HashTable *blt_table_column_get_label_table(BLT_TABLE table, 
	const char *label)
}
declare 49 generic {
   BLT_TABLE_ROW blt_table_row_find_by_label(BLT_TABLE table, 
	const char *label)
}
declare 50 generic {
   BLT_TABLE_COLUMN blt_table_column_find_by_label(BLT_TABLE table, 
	const char *label)
}
declare 51 generic {
   BLT_TABLE_ROW blt_table_row_find_by_index(BLT_TABLE table, long index)
}
declare 52 generic {
   BLT_TABLE_COLUMN blt_table_column_find_by_index(BLT_TABLE table, 
	long index)
}
declare 53 generic {
   int blt_table_row_set_label(Tcl_Interp *interp, BLT_TABLE table, 
	BLT_TABLE_ROW row, const char *label)
}
declare 54 generic {
   int blt_table_column_set_label(Tcl_Interp *interp, BLT_TABLE table, 
	BLT_TABLE_COLUMN column, const char *label)
}
declare 55 generic {
   BLT_TABLE_COLUMN_TYPE blt_table_column_convert_name_to_type(const char *typeName)
}
declare 56 generic {
   int blt_table_column_set_type(BLT_TABLE table, BLT_TABLE_COLUMN column,
	BLT_TABLE_COLUMN_TYPE type)
}
declare 57 generic {
   const char *blt_table_name_of_type(BLT_TABLE_COLUMN_TYPE type)
}
declare 58 generic {
   int blt_table_column_set_tag(Tcl_Interp *interp, BLT_TABLE table, 
	BLT_TABLE_COLUMN column, const char *tagName)
}
declare 59 generic {
   int blt_table_row_set_tag(Tcl_Interp *interp, BLT_TABLE table, 
	BLT_TABLE_ROW row, const char *tagName)
}
declare 60 generic {
   BLT_TABLE_ROW blt_table_row_create(Tcl_Interp *interp, BLT_TABLE table,
	const char *label)
}
declare 61 generic {
   BLT_TABLE_COLUMN blt_table_column_create(Tcl_Interp *interp, 
	BLT_TABLE table, const char *label)
}
declare 62 generic {
   int blt_table_row_extend(Tcl_Interp *interp, BLT_TABLE table,
	size_t n, BLT_TABLE_ROW *rows)
}
declare 63 generic {
   int blt_table_column_extend(Tcl_Interp *interp, BLT_TABLE table, 
	size_t n, BLT_TABLE_COLUMN *columms)
}
declare 64 generic {
   int blt_table_row_delete(BLT_TABLE table, BLT_TABLE_ROW row)
}
declare 65 generic {
   int blt_table_column_delete(BLT_TABLE table, BLT_TABLE_COLUMN column)
}
declare 66 generic {
   int blt_table_row_move(Tcl_Interp *interp, BLT_TABLE table, 
	BLT_TABLE_ROW from, BLT_TABLE_ROW to, size_t n)
}
declare 67 generic {
   int blt_table_column_move(Tcl_Interp *interp, BLT_TABLE table, 
	BLT_TABLE_COLUMN from, BLT_TABLE_COLUMN to, size_t n)
}
declare 68 generic {
   Tcl_Obj *blt_table_get_obj(BLT_TABLE table, BLT_TABLE_ROW row, 
	BLT_TABLE_COLUMN column)
}
declare 69 generic {
   int blt_table_set_obj(BLT_TABLE table, BLT_TABLE_ROW row,
	BLT_TABLE_COLUMN column, Tcl_Obj *objPtr)
}
declare 70 generic {
   const char *blt_table_get_string(BLT_TABLE table, BLT_TABLE_ROW row, 
	BLT_TABLE_COLUMN column)
}
declare 71 generic {
   int blt_table_set_string(BLT_TABLE table, BLT_TABLE_ROW row,
	BLT_TABLE_COLUMN column, const char *string, int length)
}
declare 72 generic {
   int blt_table_append_string(Tcl_Interp *interp, BLT_TABLE table, 
	BLT_TABLE_ROW row, BLT_TABLE_COLUMN column, const char *string, 
	int length)
}
declare 73 generic {
   double blt_table_get_double(BLT_TABLE table, BLT_TABLE_ROW row, 
	BLT_TABLE_COLUMN column)
}
declare 74 generic {
   int blt_table_set_double(BLT_TABLE table, BLT_TABLE_ROW row, 
	BLT_TABLE_COLUMN column, double value)
}
declare 75 generic {
   long blt_table_get_long(BLT_TABLE table, BLT_TABLE_ROW row, 
	BLT_TABLE_COLUMN column, long defValue)
}
declare 76 generic {
   int blt_table_set_long(BLT_TABLE table, BLT_TABLE_ROW row, 
	BLT_TABLE_COLUMN column, long value)
}
declare 77 generic {
   BLT_TABLE_VALUE blt_table_get_value(BLT_TABLE table, 
	BLT_TABLE_ROW row, BLT_TABLE_COLUMN column)
}
declare 78 generic {
   int blt_table_set_value(BLT_TABLE table, BLT_TABLE_ROW row, 
	BLT_TABLE_COLUMN column, BLT_TABLE_VALUE value)
}
declare 79 generic {
   int blt_table_unset_value(BLT_TABLE table, BLT_TABLE_ROW row, 
	BLT_TABLE_COLUMN column)
}
declare 80 generic {
   int blt_table_value_exists(BLT_TABLE table, BLT_TABLE_ROW row, 
	BLT_TABLE_COLUMN column)
}
declare 81 generic {
   Blt_HashTable *blt_table_row_find_tag_table(BLT_TABLE table, 
	const char *tagName)
}
declare 82 generic {
   Blt_HashTable *blt_table_column_find_tag_table(BLT_TABLE table, 
	const char *tagName)
}
declare 83 generic {
   Blt_Chain blt_table_row_tags(BLT_TABLE table, BLT_TABLE_ROW row)
}
declare 84 generic {
   Blt_Chain blt_table_column_tags(BLT_TABLE table, 
	BLT_TABLE_COLUMN column)
}
declare 85 generic {
   int blt_table_tags_are_shared(BLT_TABLE table)
}
declare 86 generic {
   int blt_table_row_has_tag(BLT_TABLE table, BLT_TABLE_ROW row, 
	const char *tagName)
}
declare 87 generic {
   int blt_table_column_has_tag(BLT_TABLE table, 
	BLT_TABLE_COLUMN column, const char *tagName)
}
declare 88 generic {
   int blt_table_row_forget_tag(Tcl_Interp *interp, BLT_TABLE table, 
	const char *tagName)
}
declare 89 generic {
   int blt_table_column_forget_tag(Tcl_Interp *interp, BLT_TABLE table, 
	const char *tagName)
}
declare 90 generic {
   int blt_table_row_unset_tag(Tcl_Interp *interp, BLT_TABLE table, 
	BLT_TABLE_ROW row, const char *tagName)
}
declare 91 generic {
   int blt_table_column_unset_tag(Tcl_Interp *interp, BLT_TABLE table, 
	BLT_TABLE_COLUMN column, const char *tagName)
}
declare 92 generic {
   Blt_HashEntry *blt_table_row_first_tag(BLT_TABLE table, 
	Blt_HashSearch *cursorPtr)
}
declare 93 generic {
   Blt_HashEntry *blt_table_column_first_tag(BLT_TABLE table, 
	Blt_HashSearch *cursorPtr)
}
declare 94 generic {
   BLT_TABLE_COLUMN blt_table_column_first(BLT_TABLE table)
}
declare 95 generic {
   BLT_TABLE_COLUMN blt_table_column_next(BLT_TABLE table, 
	BLT_TABLE_COLUMN column)
}
declare 96 generic {
   BLT_TABLE_ROW blt_table_row_first(BLT_TABLE table)
}
declare 97 generic {
   BLT_TABLE_ROW blt_table_row_next(BLT_TABLE table, BLT_TABLE_ROW row)
}
declare 98 generic {
   BLT_TABLE_ROWCOLUMN_SPEC blt_table_row_spec(BLT_TABLE table, 
	Tcl_Obj *objPtr, const char **sp)
}
declare 99 generic {
   BLT_TABLE_ROWCOLUMN_SPEC blt_table_column_spec(BLT_TABLE table, 
	Tcl_Obj *objPtr, const char **sp)
}
declare 100 generic {
   int blt_table_row_iterate(Tcl_Interp *interp, BLT_TABLE table, 
	Tcl_Obj *objPtr, BLT_TABLE_ITERATOR *iter)
}
declare 101 generic {
   int blt_table_column_iterate(Tcl_Interp *interp, BLT_TABLE table, 
	Tcl_Obj *objPtr, BLT_TABLE_ITERATOR *iter)
}
declare 102 generic {
   int blt_table_row_iterate_objv(Tcl_Interp *interp, BLT_TABLE table, 
	int objc, Tcl_Obj *const *objv, BLT_TABLE_ITERATOR *iterPtr)
}
declare 103 generic {
   int blt_table_column_iterate_objv(Tcl_Interp *interp, 
	BLT_TABLE table, int objc, Tcl_Obj *const *objv, 
	BLT_TABLE_ITERATOR *iterPtr)
}
declare 104 generic {
   void blt_table_free_iterator_objv(BLT_TABLE_ITERATOR *iterPtr)
}
declare 105 generic {
   void blt_table_row_iterate_all(BLT_TABLE table, 
	BLT_TABLE_ITERATOR *iterPtr)
}
declare 106 generic {
   void blt_table_column_iterate_all(BLT_TABLE table, 
	BLT_TABLE_ITERATOR *iterPtr)
}
declare 107 generic {
   BLT_TABLE_ROW blt_table_row_first_tagged(BLT_TABLE_ITERATOR *iter)
}
declare 108 generic {
   BLT_TABLE_COLUMN blt_table_column_first_tagged(
	BLT_TABLE_ITERATOR *iter)
}
declare 109 generic {
   BLT_TABLE_ROW blt_table_row_next_tagged(BLT_TABLE_ITERATOR *iter)
}
declare 110 generic {
   BLT_TABLE_COLUMN blt_table_column_next_tagged(
	BLT_TABLE_ITERATOR *iter)
}
declare 111 generic {
   BLT_TABLE_ROW blt_table_row_find(Tcl_Interp *interp, 
	BLT_TABLE table, Tcl_Obj *objPtr)
}
declare 112 generic {
   BLT_TABLE_COLUMN blt_table_column_find(Tcl_Interp *interp, 
	BLT_TABLE table, Tcl_Obj *objPtr)
}
declare 113 generic {
   int blt_table_list_rows(Tcl_Interp *interp, BLT_TABLE table, 
	int objc, Tcl_Obj *const *objv, Blt_Chain chain)
}
declare 114 generic {
   int blt_table_list_columns(Tcl_Interp *interp, BLT_TABLE table, 
	int objc, Tcl_Obj *const *objv, Blt_Chain chain)
}
declare 115 generic {
   void blt_table_row_clear_tags(BLT_TABLE table, BLT_TABLE_ROW row)
}
declare 116 generic {
   void blt_table_column_clear_tags(BLT_TABLE table, 
	BLT_TABLE_COLUMN column)
}
declare 117 generic {
   void blt_table_row_clear_traces(BLT_TABLE table, BLT_TABLE_ROW row)
}
declare 118 generic {
   void blt_table_column_clear_traces(BLT_TABLE table, 
	BLT_TABLE_COLUMN column)
}
declare 119 generic {
   BLT_TABLE_TRACE blt_table_create_trace(BLT_TABLE table, 
	BLT_TABLE_ROW row, BLT_TABLE_COLUMN column, const char *rowTag, 
	const char *columnTag, unsigned int mask, BLT_TABLE_TRACE_PROC *proc, 
	BLT_TABLE_TRACE_DELETE_PROC *deleteProc, ClientData clientData)
}
declare 120 generic {
   BLT_TABLE_TRACE blt_table_column_create_trace(BLT_TABLE table, 
	BLT_TABLE_COLUMN column, unsigned int mask, BLT_TABLE_TRACE_PROC *proc, 
	BLT_TABLE_TRACE_DELETE_PROC *deleteProc, ClientData clientData)
}
declare 121 generic {
   BLT_TABLE_TRACE blt_table_column_create_tag_trace(BLT_TABLE table, 
	const char *tag, unsigned int mask, BLT_TABLE_TRACE_PROC *proc, 
	BLT_TABLE_TRACE_DELETE_PROC *deleteProc, ClientData clientData)
}
declare 122 generic {
   BLT_TABLE_TRACE blt_table_row_create_trace(BLT_TABLE table,
	BLT_TABLE_ROW row, unsigned int mask, BLT_TABLE_TRACE_PROC *proc, 
	BLT_TABLE_TRACE_DELETE_PROC *deleteProc, ClientData clientData)
}
declare 123 generic {
   BLT_TABLE_TRACE blt_table_row_create_tag_trace(BLT_TABLE table, 
	const char *tag, unsigned int mask, BLT_TABLE_TRACE_PROC *proc, 
	BLT_TABLE_TRACE_DELETE_PROC *deleteProc, ClientData clientData)
}
declare 124 generic {
   void blt_table_delete_trace(BLT_TABLE_TRACE trace)
}
declare 125 generic {
   BLT_TABLE_NOTIFIER blt_table_row_create_notifier(Tcl_Interp *interp, 
	BLT_TABLE table, BLT_TABLE_ROW row, unsigned int mask, 
	BLT_TABLE_NOTIFY_EVENT_PROC *proc, 
	BLT_TABLE_NOTIFIER_DELETE_PROC *deleteProc,
	ClientData clientData)
}
declare 126 generic {
   BLT_TABLE_NOTIFIER blt_table_row_create_tag_notifier(
	Tcl_Interp *interp,
	BLT_TABLE table, const char *tag, unsigned int mask, 
	BLT_TABLE_NOTIFY_EVENT_PROC *proc, 
	BLT_TABLE_NOTIFIER_DELETE_PROC *deleteProc,
	ClientData clientData)
}
declare 127 generic {
   BLT_TABLE_NOTIFIER blt_table_column_create_notifier(
	Tcl_Interp *interp, BLT_TABLE table, BLT_TABLE_COLUMN column, 
	unsigned int mask, BLT_TABLE_NOTIFY_EVENT_PROC *proc, 
	BLT_TABLE_NOTIFIER_DELETE_PROC *deleteProc, ClientData clientData)
}
declare 128 generic {
   BLT_TABLE_NOTIFIER blt_table_column_create_tag_notifier(
	Tcl_Interp *interp, BLT_TABLE table, const char *tag, 
	unsigned int mask, BLT_TABLE_NOTIFY_EVENT_PROC *proc, 
	BLT_TABLE_NOTIFIER_DELETE_PROC *deleteProc, ClientData clientData)
}
declare 129 generic {
   void blt_table_delete_notifier(BLT_TABLE_NOTIFIER notifier)
}
declare 130 generic {
   void blt_table_sort_init(BLT_TABLE table, 
	BLT_TABLE_SORT_ORDER *order, size_t numCompares, unsigned int flags)
}
declare 131 generic {
   BLT_TABLE_ROW *blt_table_sort_rows(BLT_TABLE table)
}
declare 132 generic {
   void blt_table_sort_rows_subset(BLT_TABLE table, long numRows, 
	BLT_TABLE_ROW *rows)
}
declare 133 generic {
   void blt_table_sort_finish(void)
}
declare 134 generic {
   BLT_TABLE_COMPARE_PROC *blt_table_get_compare_proc(BLT_TABLE table, 
	BLT_TABLE_COLUMN column, unsigned int flags)
}
declare 135 generic {
   BLT_TABLE_ROW *blt_table_row_get_map(BLT_TABLE table)
}
declare 136 generic {
   BLT_TABLE_COLUMN *blt_table_column_get_map(BLT_TABLE table)
}
declare 137 generic {
   void blt_table_row_set_map(BLT_TABLE table, BLT_TABLE_ROW *map)
}
declare 138 generic {
   void blt_table_column_set_map(BLT_TABLE table, BLT_TABLE_COLUMN *map)
}
declare 139 generic {
   int blt_table_restore(Tcl_Interp *interp, BLT_TABLE table, 
	char *string, unsigned int flags)
}
declare 140 generic {
   int blt_table_file_restore(Tcl_Interp *interp, BLT_TABLE table, 
	const char *fileName, unsigned int flags)
}
declare 141 generic {
   int blt_table_register_format(Tcl_Interp *interp, const char *name, 
	BLT_TABLE_IMPORT_PROC *importProc, BLT_TABLE_EXPORT_PROC *exportProc)
}
declare 142 generic {
   void blt_table_unset_keys(BLT_TABLE table)
}
declare 143 generic {
   Blt_Chain blt_table_get_keys(BLT_TABLE table)
}
declare 144 generic {
   int blt_table_set_keys(BLT_TABLE table, Blt_Chain keys, int unique)
}
declare 145 generic {
   int blt_table_key_lookup(Tcl_Interp *interp, BLT_TABLE table,
	int objc, Tcl_Obj *const *objv, BLT_TABLE_ROW *rowPtr)
}
declare 146 generic {
   int blt_table_column_get_limits(Tcl_Interp *interp, BLT_TABLE table, 
	BLT_TABLE_COLUMN col, Tcl_Obj **minObjPtrPtr, Tcl_Obj **maxObjPtrPtr)
}
declare 147 generic {
   Blt_Pool Blt_Pool_Create(int type)
}
declare 148 generic {
   void Blt_Pool_Destroy(Blt_Pool pool)
}
declare 149 generic {
   Blt_TreeKey Blt_Tree_GetKey(Blt_Tree tree, const char *string)
}
declare 150 generic {
   Blt_TreeKey Blt_Tree_GetKeyFromNode(Blt_TreeNode node, 
	const char *string)
}
declare 151 generic {
   Blt_TreeKey Blt_Tree_GetKeyFromInterp(Tcl_Interp *interp,
	const char *string)
}
declare 152 generic {
   Blt_TreeNode Blt_Tree_CreateNode(Blt_Tree tree, Blt_TreeNode parent, 
	const char *name, long position)
}
declare 153 generic {
   Blt_TreeNode Blt_Tree_CreateNodeWithId(Blt_Tree tree, 
	Blt_TreeNode parent, const char *name, long inode, long position)
}
declare 154 generic {
   int Blt_Tree_DeleteNode(Blt_Tree tree, Blt_TreeNode node)
}
declare 155 generic {
   int Blt_Tree_MoveNode(Blt_Tree tree, Blt_TreeNode node, 
	Blt_TreeNode parent, Blt_TreeNode before)
}
declare 156 generic {
   Blt_TreeNode Blt_Tree_GetNode(Blt_Tree tree, long inode)
}
declare 157 generic {
   Blt_TreeNode Blt_Tree_FindChild(Blt_TreeNode parent, 
	const char *name)
}
declare 158 generic {
   Blt_TreeNode Blt_Tree_NextNode(Blt_TreeNode root, Blt_TreeNode node)
}
declare 159 generic {
   Blt_TreeNode Blt_Tree_PrevNode(Blt_TreeNode root, Blt_TreeNode node)
}
declare 160 generic {
   Blt_TreeNode Blt_Tree_FirstChild(Blt_TreeNode parent)
}
declare 161 generic {
   Blt_TreeNode Blt_Tree_LastChild(Blt_TreeNode parent)
}
declare 162 generic {
   int Blt_Tree_IsBefore(Blt_TreeNode node1, Blt_TreeNode node2)
}
declare 163 generic {
   int Blt_Tree_IsAncestor(Blt_TreeNode node1, Blt_TreeNode node2)
}
declare 164 generic {
   int Blt_Tree_PrivateValue(Tcl_Interp *interp, Blt_Tree tree, 
	Blt_TreeNode node, Blt_TreeKey key)
}
declare 165 generic {
   int Blt_Tree_PublicValue(Tcl_Interp *interp, Blt_Tree tree, 
	Blt_TreeNode node, Blt_TreeKey key)
}
declare 166 generic {
   int Blt_Tree_GetValue(Tcl_Interp *interp, Blt_Tree tree, 
	Blt_TreeNode node, const char *string, Tcl_Obj **valuePtr)
}
declare 167 generic {
   int Blt_Tree_ValueExists(Blt_Tree tree, Blt_TreeNode node, 
	const char *string)
}
declare 168 generic {
   int Blt_Tree_SetValue(Tcl_Interp *interp, Blt_Tree tree, 
	Blt_TreeNode node, const char *string, Tcl_Obj *valuePtr)
}
declare 169 generic {
   int Blt_Tree_UnsetValue(Tcl_Interp *interp, Blt_Tree tree, 
	Blt_TreeNode node, const char *string)
}
declare 170 generic {
   int Blt_Tree_AppendValue(Tcl_Interp *interp, Blt_Tree tree, 
	Blt_TreeNode node, const char *string, const char *value)
}
declare 171 generic {
   int Blt_Tree_ListAppendValue(Tcl_Interp *interp, Blt_Tree tree, 
	Blt_TreeNode node, const char *string, Tcl_Obj *valuePtr)
}
declare 172 generic {
   int Blt_Tree_GetArrayValue(Tcl_Interp *interp, Blt_Tree tree, 
	Blt_TreeNode node, const char *arrayName, const char *elemName, 
	Tcl_Obj **valueObjPtrPtr)
}
declare 173 generic {
   int Blt_Tree_SetArrayValue(Tcl_Interp *interp, Blt_Tree tree, 
	Blt_TreeNode node, const char *arrayName, const char *elemName, 
	Tcl_Obj *valueObjPtr)
}
declare 174 generic {
   int Blt_Tree_UnsetArrayValue(Tcl_Interp *interp, Blt_Tree tree, 
	Blt_TreeNode node, const char *arrayName, const char *elemName)
}
declare 175 generic {
   int Blt_Tree_AppendArrayValue(Tcl_Interp *interp, Blt_Tree tree, 
	Blt_TreeNode node, const char *arrayName, const char *elemName, 
	const char *value)
}
declare 176 generic {
   int Blt_Tree_ListAppendArrayValue(Tcl_Interp *interp, Blt_Tree tree, 
	Blt_TreeNode node, const char *arrayName, const char *elemName, 
	Tcl_Obj *valueObjPtr)
}
declare 177 generic {
   int Blt_Tree_ArrayValueExists(Blt_Tree tree, Blt_TreeNode node, 
	const char *arrayName, const char *elemName)
}
declare 178 generic {
   int Blt_Tree_ArrayNames(Tcl_Interp *interp, Blt_Tree tree, 
	Blt_TreeNode node, const char *arrayName, Tcl_Obj *listObjPtr)
}
declare 179 generic {
   int Blt_Tree_GetValueByKey(Tcl_Interp *interp, Blt_Tree tree, 
	Blt_TreeNode node, Blt_TreeKey key, Tcl_Obj **valuePtr)
}
declare 180 generic {
   int Blt_Tree_SetValueByKey(Tcl_Interp *interp, Blt_Tree tree, 
	Blt_TreeNode node, Blt_TreeKey key, Tcl_Obj *valuePtr)
}
declare 181 generic {
   int Blt_Tree_UnsetValueByKey(Tcl_Interp *interp, Blt_Tree tree, 
	Blt_TreeNode node, Blt_TreeKey key)
}
declare 182 generic {
   int Blt_Tree_AppendValueByKey(Tcl_Interp *interp, Blt_Tree tree, 
	Blt_TreeNode node, Blt_TreeKey key, const char *value)
}
declare 183 generic {
   int Blt_Tree_ListAppendValueByKey(Tcl_Interp *interp, Blt_Tree tree, 
	Blt_TreeNode node, Blt_TreeKey key, Tcl_Obj *valuePtr)
}
declare 184 generic {
   int Blt_Tree_ValueExistsByKey(Blt_Tree tree, Blt_TreeNode node, 
	Blt_TreeKey key)
}
declare 185 generic {
   Blt_TreeKey Blt_Tree_FirstKey(Blt_Tree tree, Blt_TreeNode node, 
	Blt_TreeKeyIterator *iterPtr)
}
declare 186 generic {
   Blt_TreeKey Blt_Tree_NextKey(Blt_Tree tree, 
	Blt_TreeKeyIterator *iterPtr)
}
declare 187 generic {
   int Blt_Tree_Apply(Blt_TreeNode root, Blt_TreeApplyProc *proc, 
	ClientData clientData)
}
declare 188 generic {
   int Blt_Tree_ApplyDFS(Blt_TreeNode root, Blt_TreeApplyProc *proc, 
	ClientData clientData, int order)
}
declare 189 generic {
   int Blt_Tree_ApplyBFS(Blt_TreeNode root, Blt_TreeApplyProc *proc, 
	ClientData clientData)
}
declare 190 generic {
   int Blt_Tree_SortNode(Blt_Tree tree, Blt_TreeNode node, 
	Blt_TreeCompareNodesProc *proc)
}
declare 191 generic {
   int Blt_Tree_Exists(Tcl_Interp *interp, const char *name)
}
declare 192 generic {
   Blt_Tree Blt_Tree_Open(Tcl_Interp *interp, const char *name, 
	int flags)
}
declare 193 generic {
   void Blt_Tree_Close(Blt_Tree tree)
}
declare 194 generic {
   int Blt_Tree_Attach(Tcl_Interp *interp, Blt_Tree tree, 
	const char *name)
}
declare 195 generic {
   Blt_Tree Blt_Tree_GetFromObj(Tcl_Interp *interp, Tcl_Obj *objPtr)
}
declare 196 generic {
   int Blt_Tree_Size(Blt_TreeNode node)
}
declare 197 generic {
   Blt_TreeTrace Blt_Tree_CreateTrace(Blt_Tree tree, Blt_TreeNode node, 
	const char *keyPattern, const char *tagName, unsigned int mask, 
	Blt_TreeTraceProc *proc, ClientData clientData)
}
declare 198 generic {
   void Blt_Tree_DeleteTrace(Blt_TreeTrace token)
}
declare 199 generic {
   void Blt_Tree_CreateEventHandler(Blt_Tree tree, unsigned int mask, 
	Blt_TreeNotifyEventProc *proc, ClientData clientData)
}
declare 200 generic {
   void Blt_Tree_DeleteEventHandler(Blt_Tree tree, unsigned int mask, 
	Blt_TreeNotifyEventProc *proc, ClientData clientData)
}
declare 201 generic {
   void Blt_Tree_RelabelNode(Blt_Tree tree, Blt_TreeNode node, 
	const char *string)
}
declare 202 generic {
   void Blt_Tree_RelabelNodeWithoutNotify(Blt_TreeNode node,
	const char *string)
}
declare 203 generic {
   const char *Blt_Tree_NodeIdAscii(Blt_TreeNode node)
}
declare 204 generic {
   const char *Blt_Tree_NodePath(Blt_TreeNode node, 
	Tcl_DString *resultPtr)
}
declare 205 generic {
   const char *Blt_Tree_NodeRelativePath(Blt_TreeNode root, 
	Blt_TreeNode node, const char *separator, unsigned int flags, 
	Tcl_DString *resultPtr)
}
declare 206 generic {
   long Blt_Tree_NodePosition(Blt_TreeNode node)
}
declare 207 generic {
   void Blt_Tree_ClearTags(Blt_Tree tree, Blt_TreeNode node)
}
declare 208 generic {
   int Blt_Tree_HasTag(Blt_Tree tree, Blt_TreeNode node, 
	const char *tagName)
}
declare 209 generic {
   void Blt_Tree_AddTag(Blt_Tree tree, Blt_TreeNode node, 
	const char *tagName)
}
declare 210 generic {
   void Blt_Tree_RemoveTag(Blt_Tree tree, Blt_TreeNode node,
	const char *tagName)
}
declare 211 generic {
   void Blt_Tree_ForgetTag(Blt_Tree tree, const char *tagName)
}
declare 212 generic {
   Blt_HashTable *Blt_Tree_TagHashTable(Blt_Tree tree, 
	const char *tagName)
}
declare 213 generic {
   int Blt_Tree_TagTableIsShared(Blt_Tree tree)
}
declare 214 generic {
   void Blt_Tree_NewTagTable(Blt_Tree tree)
}
declare 215 generic {
   Blt_HashEntry *Blt_Tree_FirstTag(Blt_Tree tree, 
	Blt_HashSearch *searchPtr)
}
declare 216 generic {
   void Blt_Tree_DumpNode(Blt_Tree tree, Blt_TreeNode root, 
	Blt_TreeNode node, Tcl_DString *resultPtr)
}
declare 217 generic {
   int Blt_Tree_Dump(Blt_Tree tree, Blt_TreeNode root, 
	Tcl_DString *resultPtr)
}
declare 218 generic {
   int Blt_Tree_DumpToFile(Tcl_Interp *interp, Blt_Tree tree, 
	Blt_TreeNode root, const char *fileName)
}
declare 219 generic {
   int Blt_Tree_Restore(Tcl_Interp *interp, Blt_Tree tree, 
	Blt_TreeNode root, const char *string, unsigned int flags)
}
declare 220 generic {
   int Blt_Tree_RestoreFromFile(Tcl_Interp *interp, Blt_Tree tree, 
	Blt_TreeNode root, const char *fileName, unsigned int flags)
}
declare 221 generic {
   long Blt_Tree_Depth(Blt_Tree tree)
}
declare 222 generic {
   int Blt_Tree_RegisterFormat(Tcl_Interp *interp, const char *fmtName, 
	Blt_TreeImportProc *importProc, Blt_TreeExportProc *exportProc)
}
declare 223 generic {
   Blt_TreeTagEntry *Blt_Tree_RememberTag(Blt_Tree tree, 
	const char *name)
}
declare 224 generic {
   void Blt_InitHashTable(Blt_HashTable *tablePtr, size_t keyType)
}
declare 225 generic {
   void Blt_InitHashTableWithPool(Blt_HashTable *tablePtr, 
	size_t keyType)
}
declare 226 generic {
   void Blt_DeleteHashTable(Blt_HashTable *tablePtr)
}
declare 227 generic {
   void Blt_DeleteHashEntry(Blt_HashTable *tablePtr, 
	Blt_HashEntry *entryPtr)
}
declare 228 generic {
   Blt_HashEntry *Blt_FirstHashEntry(Blt_HashTable *tablePtr, 
	Blt_HashSearch *searchPtr)
}
declare 229 generic {
   Blt_HashEntry *Blt_NextHashEntry(Blt_HashSearch *srchPtr)
}
declare 230 generic {
   const char *Blt_HashStats(Blt_HashTable *tablePtr)
}
declare 231 generic {
   double Blt_VecMin(Blt_Vector *vPtr)
}
declare 232 generic {
   double Blt_VecMax(Blt_Vector *vPtr)
}
declare 233 generic {
   Blt_VectorId Blt_AllocVectorId(Tcl_Interp *interp, 
	const char *vecName)
}
declare 234 generic {
   void Blt_SetVectorChangedProc(Blt_VectorId clientId, 
	Blt_VectorChangedProc *proc, ClientData clientData)
}
declare 235 generic {
   void Blt_FreeVectorId(Blt_VectorId clientId)
}
declare 236 generic {
   int Blt_GetVectorById(Tcl_Interp *interp, Blt_VectorId clientId, 
	Blt_Vector **vecPtrPtr)
}
declare 237 generic {
   const char *Blt_NameOfVectorId(Blt_VectorId clientId)
}
declare 238 generic {
   const char *Blt_NameOfVector(Blt_Vector *vecPtr)
}
declare 239 generic {
   int Blt_VectorNotifyPending(Blt_VectorId clientId)
}
declare 240 generic {
   int Blt_CreateVector(Tcl_Interp *interp, const char *vecName, 
	int size, Blt_Vector ** vecPtrPtr)
}
declare 241 generic {
   int Blt_CreateVector2(Tcl_Interp *interp, const char *vecName, 
	const char *cmdName, const char *varName, int initialSize, 
	Blt_Vector **vecPtrPtr)
}
declare 242 generic {
   int Blt_GetVector(Tcl_Interp *interp, const char *vecName, 
	Blt_Vector **vecPtrPtr)
}
declare 243 generic {
   int Blt_GetVectorFromObj(Tcl_Interp *interp, Tcl_Obj *objPtr, 
	Blt_Vector **vecPtrPtr)
}
declare 244 generic {
   int Blt_VectorExists(Tcl_Interp *interp, const char *vecName)
}
declare 245 generic {
   int Blt_ResetVector(Blt_Vector *vecPtr, double *dataArr, int n, 
	int arraySize, Tcl_FreeProc *freeProc)
}
declare 246 generic {
   int Blt_ResizeVector(Blt_Vector *vecPtr, int n)
}
declare 247 generic {
   int Blt_DeleteVectorByName(Tcl_Interp *interp, const char *vecName)
}
declare 248 generic {
   int Blt_DeleteVector(Blt_Vector *vecPtr)
}
declare 249 generic {
   int Blt_ExprVector(Tcl_Interp *interp, char *expr, 
	Blt_Vector *vecPtr)
}
declare 250 generic {
   void Blt_InstallIndexProc(Tcl_Interp *interp, const char *indexName,
	Blt_VectorIndexProc * procPtr)
}
declare 251 generic {
   int Blt_VectorExists2(Tcl_Interp *interp, const char *vecName)
}
