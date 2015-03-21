/* -*- mode: c; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 * bltDataTable.h --
 *
 *	Copyright 1998-2004 George A. Howlett.
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

#ifndef _BLT_DATATABLE_H
#define _BLT_DATATABLE_H

#include <bltChain.h>
#include <bltHash.h>

typedef struct _BLT_TABLE_TAGS *BLT_TABLE_TAGS;

typedef enum {
    TABLE_COLUMN_TYPE_UNKNOWN=-1, 
    TABLE_COLUMN_TYPE_STRING, 
    TABLE_COLUMN_TYPE_DOUBLE, 
    TABLE_COLUMN_TYPE_LONG, 
    TABLE_COLUMN_TYPE_TIME, 
    TABLE_COLUMN_TYPE_BOOLEAN, 
    TABLE_COLUMN_TYPE_BLOB, 
} BLT_TABLE_COLUMN_TYPE;

#define TABLE_VALUE_LENGTH      16
#define TABLE_VALUE_STORE      ((char *)1)

typedef struct _BLT_TABLE_VALUE {
    union {				
	long l;
	double d;
	Tcl_WideInt w;
	void *ptr;
    } datum;				/* Internal representation of data:
					 * used to speed comparisons,
					 * sorting, etc. */
    /* 
     * Time/space tradeoff for values.  If most values are small the win
     * will be that we don't have to allocate space for each string
     * representation.
     */
    size_t length;                      /* Number of bytes in string
                                         * below. */
    const char *string;			/* String representation of the
                                         * value. If NULL, the value is
                                         * empty.  If
                                         * BLT_TABLE_VALUE_STORE, the
                                         * string is stored in *store*.
                                         * Otherwise this points to
                                         * malloc'ed memory.  Note: We
                                         * can't point directly to store,
                                         * because we realloc the column
                                         * values.  This changes the
                                         * address of store for all
                                         * values. */
    char store[TABLE_VALUE_LENGTH];
} *BLT_TABLE_VALUE;

typedef struct _BLT_TABLE_HEADER {
    const char *label;			/* Label of row or column. */
    long index;				/* Reverse lookup
					 * offset-to-index. */
    long offset;
    unsigned int flags;
} *BLT_TABLE_HEADER;

typedef struct _BLT_TABLE_ROW {
    const char *label;			/* Label of row or column. */
    long index;				/* Reverse lookup
					 * offset-to-index. */
    long offset;
    unsigned int flags;
} *BLT_TABLE_ROW;

typedef struct _BLT_TABLE_COLUMN {
    const char *label;			/* Label of row or column. */
    long index;				/* Reverse lookup
					 * offset-to-index. */
    long offset;
    unsigned short flags;
    BLT_TABLE_COLUMN_TYPE type;
} *BLT_TABLE_COLUMN;

typedef struct {
    const char *name;
    long headerSize;
} BLT_TABLE_ROWCOLUMN_CLASS;

/*
 * BLT_TABLE_ROWCOLUMN --
 *
 *	Structure representing a row or column in the table. 
 */
typedef struct _BLT_TABLE_ROWCOLUMN {
    BLT_TABLE_ROWCOLUMN_CLASS *classPtr;
    Blt_Pool headerPool;
    long numAllocated;			/* Length of allocated header array
					 * below. May exceed the number of
					 * row or column headers used. */
    long numUsed;
    BLT_TABLE_HEADER *map;		/* Array of row or column headers. */
    Blt_Chain freeList;			/* Tracks free row or column
					 * headers. */
    Blt_HashTable labelTable;		/* Hash table of labels. Maps
					 * labels to table offsets. */
    long nextId;			/* Used to generate default labels. */
} BLT_TABLE_ROWCOLUMN;

/*
 * BLT_TABLE_CORE --
 *
 *	Structure representing a core table object.  A table object may be
 *	shared by more than one client (BLT_TABLE). When a client wants to
 *	use a table object, it is given a token that represents the table.
 *	The core table object tracks its clients by their tokens.  When all
 *	the clients of a core table object have released their tokens, the
 *	table object is automatically destroyed.
 *
 *	The table object is an array of column vectors. Each vector is an
 *	array of BLT_TABLE_VALUE's, representing the data for the column.
 *	Empty row entries are designated by 0 length values.  Column
 *	vectors are allocated when needed.  Every column in the table has
 *	the same length.
 *
 *	Rows and columns are indexed by a map of pointers to headers.  This
 *	map represents the order of the rows or columns, not how the values
 *	are actually stored.  All clients see the table in the same order,
 *	it's up to the clients to manage sorting and ordering.
 */
typedef struct _BLT_TABLE_CORE {
    BLT_TABLE_ROWCOLUMN rows, columns;
    BLT_TABLE_VALUE *data;		/* Array of column vector pointers */
    unsigned int flags;			/* Internal flags. See definitions
					 * below. */
    Blt_Chain clients;			/* List of clients using this table */
    unsigned long mtime, ctime;
    unsigned int notifyFlags;		/* Notification flags. See
					 * definitions below. */
    int notifyHold;
} BLT_TABLE_CORE;

#ifndef _BLT_TAGS_H
typedef struct _Blt_Tags *Blt_Tags;
#endif  /* _BLT_TAGS_H */

/*
 * BLT_TABLE --
 *
 *	A BLT_TABLE is a structure each client has which points to a core
 *	table object.  It is opaque.  There are accessor functions for its
 *	data members. The client is uniquely identified by a combination of
 *	the table name and the originating namespace.  Two table objects in
 *	the same interpreter can have similar names but must reside in
 *	different namespaces.
 *
 *	Two or more BLT_TABLEs can share the same core table object.  Each
 *	structure acts as a ticket for the underlying core table object.
 *	Clients can designate notifier routines that are invoked when the
 *	core table object is changed is specific ways (especially by other
 *	clients).
 */
typedef struct _BLT_TABLE {
    unsigned int magic;			/* Magic value indicating whether a
					 * generic pointer is really a
					 * datatable token or not. */
    const char *name;			/* Fully namespace-qualified name of
					 * the client. */
    BLT_TABLE_CORE *corePtr;		/* Pointer to the structure
					 * containing the master
					 * information about the table used
					 * by the client.  If NULL, this
					 * indicates that the table has
					 * been destroyed (but as of yet,
					 * this client hasn't recognized
					 * it). */
    Tcl_Interp *interp;
    Blt_HashTable *clientTablePtr;	/* Interpreter-specific global hash
					 * table of all datatable clients.
					 * Each entry is a chain of clients
					 * that are sharing the same core
					 * datatable. */
    Blt_HashEntry *hPtr;		/* This client's entry into a list
					 * stored in the above
					 * interpreter-specific hash
					 * table. */
    Blt_ChainLink link;			/* Pointer into the core server's
					 * chain of clients. */
    Blt_ChainLink link2;		/* Pointer into the list of clients
					 * using the same table name. */

    Blt_Tags rowTags;
    Blt_Tags columnTags;

    Blt_HashTable traces;		/* Hash table of valid traces */

    Blt_Chain readTraces;		/* List of read traces. */
    Blt_Chain writeTraces;		/* List of write, create, unset
					 * traces. */
    Blt_HashTable notifiers;		/* Hash table of valid notifiers */
    Blt_Chain columnNotifiers;		/* Chain of event handlers. */
    Blt_Chain rowNotifiers;		/* Chain of event handlers. */
    BLT_TABLE_TAGS tags;

    Blt_HashTable *keyTables;		/* Array of primary key
					 * hashtables. */
    BLT_TABLE_ROW *masterKey;		/* Master key entry. */
    Blt_HashTable masterKeyTable;
    BLT_TABLE_COLUMN *primaryKeys;      /* Array of columns acting as
					 * primary keys for table
					 * lookups. */
    int numKeys;			/* # of primary keys. */

    unsigned int flags;
} *BLT_TABLE;

BLT_EXTERN void blt_table_release_tags(BLT_TABLE table);
BLT_EXTERN void blt_table_new_tags(BLT_TABLE table);

BLT_EXTERN int blt_table_exists(Tcl_Interp *interp, const char *name);
BLT_EXTERN int blt_table_create(Tcl_Interp *interp, const char *name, 
	BLT_TABLE *tablePtr);
BLT_EXTERN int blt_table_open(Tcl_Interp *interp, const char *name, 
	BLT_TABLE *tablePtr);
BLT_EXTERN void blt_table_close(BLT_TABLE table);

BLT_EXTERN int blt_table_same_object(BLT_TABLE table1, BLT_TABLE table2);

BLT_EXTERN Blt_HashTable *blt_table_row_get_label_table(BLT_TABLE table, 
	const char *label);
BLT_EXTERN Blt_HashTable *blt_table_column_get_label_table(BLT_TABLE table, 
	const char *label);

BLT_EXTERN BLT_TABLE_ROW blt_table_get_row(Tcl_Interp *interp, 
	BLT_TABLE table, Tcl_Obj *objPtr);
BLT_EXTERN BLT_TABLE_COLUMN blt_table_get_column(Tcl_Interp *interp, 
	BLT_TABLE table, Tcl_Obj *objPtr);
BLT_EXTERN BLT_TABLE_ROW blt_table_get_row_by_label(BLT_TABLE table, 
	const char *label);
BLT_EXTERN BLT_TABLE_COLUMN blt_table_get_column_by_label(BLT_TABLE table, 
	const char *label);
BLT_EXTERN BLT_TABLE_ROW blt_table_get_row_by_index(BLT_TABLE table, 
	long index);
BLT_EXTERN BLT_TABLE_COLUMN blt_table_get_column_by_index(BLT_TABLE table, 
	long index);

BLT_EXTERN int blt_table_set_row_label(Tcl_Interp *interp, BLT_TABLE table, 
	BLT_TABLE_ROW row, const char *label);
BLT_EXTERN int blt_table_set_column_label(Tcl_Interp *interp, BLT_TABLE table, 
	BLT_TABLE_COLUMN column, const char *label);

BLT_EXTERN BLT_TABLE_COLUMN_TYPE blt_table_name_to_column_type(
	const char *typeName);
BLT_EXTERN int blt_table_set_column_type(Tcl_Interp *interp, BLT_TABLE table, 
	BLT_TABLE_COLUMN column, BLT_TABLE_COLUMN_TYPE type);
BLT_EXTERN const char *blt_table_column_type_to_name(
	BLT_TABLE_COLUMN_TYPE type);

BLT_EXTERN int blt_table_set_column_tag(Tcl_Interp *interp, BLT_TABLE table, 
	BLT_TABLE_COLUMN column, const char *tag);
BLT_EXTERN int blt_table_set_row_tag(Tcl_Interp *interp, BLT_TABLE table, 
	BLT_TABLE_ROW row, const char *tag);

BLT_EXTERN BLT_TABLE_ROW blt_table_create_row(Tcl_Interp *interp, 
	BLT_TABLE table, const char *label);
BLT_EXTERN BLT_TABLE_COLUMN blt_table_create_column(Tcl_Interp *interp, 
	BLT_TABLE table, const char *label);

BLT_EXTERN int blt_table_extend_rows(Tcl_Interp *interp, BLT_TABLE table,
	size_t n, BLT_TABLE_ROW *rows);
BLT_EXTERN int blt_table_extend_columns(Tcl_Interp *interp, BLT_TABLE table, 
	size_t n, BLT_TABLE_COLUMN *columms);
BLT_EXTERN int blt_table_delete_row(BLT_TABLE table, BLT_TABLE_ROW row);
BLT_EXTERN int blt_table_delete_column(BLT_TABLE table, BLT_TABLE_COLUMN column);
BLT_EXTERN int blt_table_move_row(Tcl_Interp *interp, BLT_TABLE table, 
	BLT_TABLE_ROW from, BLT_TABLE_ROW to, size_t n);
BLT_EXTERN int blt_table_move_column(Tcl_Interp *interp, BLT_TABLE table, 
	BLT_TABLE_COLUMN from, BLT_TABLE_COLUMN to, size_t n);

BLT_EXTERN Tcl_Obj *blt_table_get_obj(BLT_TABLE table, BLT_TABLE_ROW row, 
	BLT_TABLE_COLUMN column);
BLT_EXTERN int blt_table_set_obj(Tcl_Interp *interp, BLT_TABLE table,
        BLT_TABLE_ROW row, BLT_TABLE_COLUMN column, Tcl_Obj *objPtr);

BLT_EXTERN const char *blt_table_get_string(BLT_TABLE table,
        BLT_TABLE_ROW row, BLT_TABLE_COLUMN column);
BLT_EXTERN int blt_table_set_string_rep(Tcl_Interp *interp, BLT_TABLE table,
        BLT_TABLE_ROW row, BLT_TABLE_COLUMN column, const char *string,
        int length);
BLT_EXTERN int blt_table_set_string(Tcl_Interp *interp, BLT_TABLE table,
        BLT_TABLE_ROW row, BLT_TABLE_COLUMN column, const char *string,
        int length);
BLT_EXTERN int blt_table_append_string(Tcl_Interp *interp, BLT_TABLE table, 
	BLT_TABLE_ROW row, BLT_TABLE_COLUMN column, const char *string, 
	int length);
BLT_EXTERN int blt_table_set_bytes(Tcl_Interp *interp, BLT_TABLE table,
        BLT_TABLE_ROW row, BLT_TABLE_COLUMN column,
        const unsigned char *string, int length);

BLT_EXTERN double blt_table_get_double(Tcl_Interp *interp, BLT_TABLE table,
        BLT_TABLE_ROW row, BLT_TABLE_COLUMN column);
BLT_EXTERN int blt_table_set_double(Tcl_Interp *interp, BLT_TABLE table,
        BLT_TABLE_ROW row, BLT_TABLE_COLUMN column, double value);
BLT_EXTERN long blt_table_get_long(Tcl_Interp *interp, BLT_TABLE table,
        BLT_TABLE_ROW row, BLT_TABLE_COLUMN column, long defValue);
BLT_EXTERN int blt_table_set_long(Tcl_Interp *interp, BLT_TABLE table,
        BLT_TABLE_ROW row, BLT_TABLE_COLUMN column, long value);
BLT_EXTERN int blt_table_get_boolean(Tcl_Interp *interp, BLT_TABLE table,
        BLT_TABLE_ROW row, BLT_TABLE_COLUMN column, int defValue);
BLT_EXTERN int blt_table_set_boolean(Tcl_Interp *interp, BLT_TABLE table,
        BLT_TABLE_ROW row, BLT_TABLE_COLUMN column, int value);

BLT_EXTERN BLT_TABLE_VALUE blt_table_get_value(BLT_TABLE table, 
	BLT_TABLE_ROW row, BLT_TABLE_COLUMN column);
BLT_EXTERN int blt_table_set_value(BLT_TABLE table, BLT_TABLE_ROW row, 
	BLT_TABLE_COLUMN column, BLT_TABLE_VALUE value);
BLT_EXTERN int blt_table_unset_value(BLT_TABLE table, BLT_TABLE_ROW row, 
	BLT_TABLE_COLUMN column);
BLT_EXTERN int blt_table_value_exists(BLT_TABLE table, BLT_TABLE_ROW row, 
	BLT_TABLE_COLUMN column);
BLT_EXTERN const char *blt_table_value_string(BLT_TABLE_VALUE value);
BLT_EXTERN const unsigned char *blt_table_value_bytes(BLT_TABLE_VALUE value);
BLT_EXTERN size_t blt_table_value_length(BLT_TABLE_VALUE value);

BLT_EXTERN int blt_table_tags_are_shared(BLT_TABLE table);
BLT_EXTERN void blt_table_clear_row_tags(BLT_TABLE table, BLT_TABLE_ROW row);
BLT_EXTERN void blt_table_clear_column_tags(BLT_TABLE table, 
	BLT_TABLE_COLUMN col);
BLT_EXTERN Blt_Chain blt_table_get_row_tags(BLT_TABLE table, BLT_TABLE_ROW row);
BLT_EXTERN Blt_Chain blt_table_get_column_tags(BLT_TABLE table, 
	BLT_TABLE_COLUMN column);
BLT_EXTERN Blt_Chain blt_table_get_tagged_rows(BLT_TABLE table, 
	const char *tag);
BLT_EXTERN Blt_Chain blt_table_get_tagged_columns(BLT_TABLE table, 
	const char *tag);
BLT_EXTERN int blt_table_row_has_tag(BLT_TABLE table, BLT_TABLE_ROW row, 
	const char *tag);
BLT_EXTERN int blt_table_column_has_tag(BLT_TABLE table, 
	BLT_TABLE_COLUMN column, const char *tag);
BLT_EXTERN int blt_table_forget_row_tag(Tcl_Interp *interp, BLT_TABLE table, 
	const char *tag);
BLT_EXTERN int blt_table_forget_column_tag(Tcl_Interp *interp, BLT_TABLE table, 
	const char *tag);
BLT_EXTERN int blt_table_unset_row_tag(Tcl_Interp *interp, BLT_TABLE table, 
	BLT_TABLE_ROW row, const char *tag);
BLT_EXTERN int blt_table_unset_column_tag(Tcl_Interp *interp, BLT_TABLE table, 
	BLT_TABLE_COLUMN column, const char *tag);

BLT_EXTERN BLT_TABLE_COLUMN blt_table_first_column(BLT_TABLE table);
BLT_EXTERN BLT_TABLE_COLUMN blt_table_next_column(BLT_TABLE table, 
	BLT_TABLE_COLUMN column);
BLT_EXTERN BLT_TABLE_ROW blt_table_first_row(BLT_TABLE table);
BLT_EXTERN BLT_TABLE_ROW blt_table_next_row(BLT_TABLE table, BLT_TABLE_ROW row);

typedef enum { 
    TABLE_SPEC_UNKNOWN,                 /* 0 */
    TABLE_SPEC_INDEX,                   /* 1 */
    TABLE_SPEC_RANGE,                   /* 2 */
    TABLE_SPEC_LABEL,                   /* 3 */
    TABLE_SPEC_TAG,                     /* 4 */
} BLT_TABLE_ROWCOLUMN_SPEC;

BLT_EXTERN BLT_TABLE_ROWCOLUMN_SPEC blt_table_row_spec(BLT_TABLE table, 
	Tcl_Obj *objPtr, const char **sp);
BLT_EXTERN BLT_TABLE_ROWCOLUMN_SPEC blt_table_column_spec(BLT_TABLE table, 
	Tcl_Obj *objPtr, const char **sp);

/*
 * BLT_TABLE_ITERATOR --
 *
 *	Structure representing a trace used by a client of the table.
 *
 *	Table rows and columns may be tagged with strings.  A row may have
 *	many tags.  The same tag may be used for many rows.  Tags are used
 *	and stored by clients of a table.  Tags can also be shared between
 *	clients of the same table.
 *	
 *	Both rowTable and columnTable are hash tables keyed by the physical
 *	row or column location in the table respectively.  This is not the
 *	same as the client's view (the order of rows or columns as seen by
 *	the client).  This is so that clients (which may have different
 *	views) can share tags without sharing the same view.
 */


typedef enum { 
    TABLE_ITERATOR_INDEX, 
    TABLE_ITERATOR_LABEL, 
    TABLE_ITERATOR_TAG, 
    TABLE_ITERATOR_RANGE, 
    TABLE_ITERATOR_ALL, 
    TABLE_ITERATOR_CHAIN
} BLT_TABLE_ITERATOR_TYPE;

typedef struct _BLT_TABLE_ITERATOR {
    BLT_TABLE table;			/* Table that we're iterating over. */

    BLT_TABLE_ITERATOR_TYPE type;	/* Type of iteration:
					 * TABLE_ITERATOR_TAG by row or
					 * column tag.  TABLE_ITERATOR_ALL
					 * by every row or column.
					 * TABLE_ITERATOR_INDEX single
					 * item: either label or index.
					 * TABLE_ITERATOR_RANGE over a
					 * consecutive range of indices.
					 * TABLE_ITERATOR_CHAIN over an
					 * expanded, non-overlapping list
					 * of tags, labels, and indices.
					 */

    const char *tag;                    /* Used by notification routines to
					 * determine if a tag is being
					 * used. */
    long start;				/* Starting index.  Starting point
					 * of search, saved if iterator is
					 * reused.  Used for
					 * TABLE_ITERATOR_ALL and
					 * TABLE_ITERATOR_INDEX
					 * searches. */
    long end;				/* Ending index (inclusive). */

    long next;				/* Next index. */

    size_t numEntries;			/* Number of entries found. */

    /* For tag-based searches. */
    Blt_HashTable *tablePtr;		/* Pointer to tag hash table. */
    Blt_HashSearch cursor;		/* Iterator for tag hash table. */

    /* For chain-based searches (multiple tags). */
    Blt_Chain chain;			/* This chain, unlike the above
					 * hash table must be freed after
					 * its use. */
    Blt_ChainLink link;			/* Search iterator for chain. */
} BLT_TABLE_ITERATOR;

BLT_EXTERN int blt_table_iterate_rows(Tcl_Interp *interp, BLT_TABLE table, 
	Tcl_Obj *objPtr, BLT_TABLE_ITERATOR *iter);

BLT_EXTERN int blt_table_iterate_columns(Tcl_Interp *interp, BLT_TABLE table, 
	Tcl_Obj *objPtr, BLT_TABLE_ITERATOR *iter);

BLT_EXTERN int blt_table_iterate_rows_objv(Tcl_Interp *interp, BLT_TABLE table, 
	int objc, Tcl_Obj *const *objv, BLT_TABLE_ITERATOR *iterPtr);

BLT_EXTERN int blt_table_iterate_columns_objv(Tcl_Interp *interp, 
	BLT_TABLE table, int objc, Tcl_Obj *const *objv, 
	BLT_TABLE_ITERATOR *iterPtr);

BLT_EXTERN void blt_table_free_iterator_objv(BLT_TABLE_ITERATOR *iterPtr);

BLT_EXTERN void blt_table_iterate_all_rows(BLT_TABLE table, 
	BLT_TABLE_ITERATOR *iterPtr);

BLT_EXTERN void blt_table_iterate_all_columns(BLT_TABLE table, 
	BLT_TABLE_ITERATOR *iterPtr);

BLT_EXTERN BLT_TABLE_ROW blt_table_first_tagged_row(BLT_TABLE_ITERATOR *iter);

BLT_EXTERN BLT_TABLE_COLUMN blt_table_first_tagged_column(
	BLT_TABLE_ITERATOR *iter);

BLT_EXTERN BLT_TABLE_ROW blt_table_next_tagged_row(BLT_TABLE_ITERATOR *iter);

BLT_EXTERN BLT_TABLE_COLUMN blt_table_next_tagged_column(
	BLT_TABLE_ITERATOR *iter);

BLT_EXTERN int blt_table_list_rows(Tcl_Interp *interp, BLT_TABLE table, 
	int objc, Tcl_Obj *const *objv, Blt_Chain chain);

BLT_EXTERN int blt_table_list_columns(Tcl_Interp *interp, BLT_TABLE table, 
	int objc, Tcl_Obj *const *objv, Blt_Chain chain);

/*
 * BLT_TABLE_TRACE_EVENT --
 *
 *	Structure representing an event matching a trace set by a client of
 *	the table.
 *
 *	Table rows and columns may be tagged with strings.  A row may have
 *	many tags.  The same tag may be used for many rows.  Tags are used
 *	and stored by clients of a table.  Tags can also be shared between
 *	clients of the same table.
 *	
 *	Both rowTable and columnTable are hash tables keyed by the physical
 *	row or column location in the table respectively.  This is not the
 *	same as the client's view (the order of rows or columns as seen by
 *	the client).  This is so that clients (which may have different
 *	views) can share tags without sharing the same view.
 */
typedef struct {
    Tcl_Interp *interp;			/* Interpreter to report to */
    BLT_TABLE table;			/* Table object client that
					 * received the event. */
    BLT_TABLE_ROW row;			/* Matching row and column. */
    BLT_TABLE_COLUMN column;
    unsigned int mask;			/* Type of event received. */
} BLT_TABLE_TRACE_EVENT;

typedef int (BLT_TABLE_TRACE_PROC)(ClientData clientData, 
	BLT_TABLE_TRACE_EVENT *eventPtr);

typedef void (BLT_TABLE_TRACE_DELETE_PROC)(ClientData clientData);

/*
 * BLT_TABLE_TRACE --
 *
 *	Structure representing a trace used by a client of the table.
 *
 *	Table rows and columns may be tagged with strings.  A row may have
 *	many tags.  The same tag may be used for many rows.  Tags are used
 *	and stored by clients of a table.  Tags can also be shared between
 *	clients of the same table.
 *	
 *	Both rowTable and columnTable are hash tables keyed by the physical
 *	row or column location in the table respectively.  This is not the
 *	same as the client's view (the order of rows or columns as seen by
 *	the client).  This is so that clients (which may have different
 *	views) can share tags without sharing the same view.
 */
typedef struct _BLT_TABLE_TRACE {
    unsigned int flags;
    const char *rowTag, *colTag;
    BLT_TABLE_TRACE_EVENT event;
    BLT_TABLE_ROW row;
    BLT_TABLE_COLUMN column;
    BLT_TABLE_TRACE_PROC *proc;
    BLT_TABLE_TRACE_DELETE_PROC *deleteProc;
    ClientData clientData;
    BLT_TABLE table;
    Blt_ChainLink readLink;		/* Pointer to this entry in a list
					 * of read traces */
    Blt_ChainLink writeLink;		/* Pointer to this entry in a list
					 * of write, create, or unset
					 * traces. */
} *BLT_TABLE_TRACE;


#define TABLE_TRACE_READS	(1<<0)
#define TABLE_TRACE_CREATES	(1<<1)
#define TABLE_TRACE_WRITES	(1<<2)
#define TABLE_TRACE_UNSETS	(1<<3)
#define TABLE_TRACE_WCU		(TABLE_TRACE_UNSETS | TABLE_TRACE_WRITES | \
				 TABLE_TRACE_CREATES)
#define TABLE_TRACE_ALL		(TABLE_TRACE_UNSETS | TABLE_TRACE_WRITES | \
				 TABLE_TRACE_READS  | TABLE_TRACE_CREATES)
#define TABLE_TRACE_MASK	(TRACE_ALL)
#define TABLE_TRACE_ALL_ROWS	(BLT_TABLE_ROW)0
#define TABLE_TRACE_ALL_COLUMNS	(BLT_TABLE_COLUMN)0

#define TABLE_TRACE_FOREIGN_ONLY (1<<8)
#define TABLE_TRACE_ACTIVE	(1<<9)
#define TABLE_TRACE_SELF	(1<<10)
#define TABLE_TRACE_DESTROYED	(1<<11)
#define TABLE_TRACE_PENDING	(1<<12)
#define TABLE_TRACE_WHENIDLE	(1<<13)

BLT_EXTERN void blt_table_clear_row_traces(BLT_TABLE table, BLT_TABLE_ROW row);

BLT_EXTERN void blt_table_clear_column_traces(BLT_TABLE table, 
	BLT_TABLE_COLUMN column);

BLT_EXTERN BLT_TABLE_TRACE blt_table_create_trace(BLT_TABLE table, 
	BLT_TABLE_ROW row, BLT_TABLE_COLUMN column, const char *rowTag, 
	const char *columnTag, unsigned int mask, BLT_TABLE_TRACE_PROC *proc, 
	BLT_TABLE_TRACE_DELETE_PROC *deleteProc, ClientData clientData);

BLT_EXTERN void blt_table_trace_column(BLT_TABLE table, 
	BLT_TABLE_COLUMN column, unsigned int mask, BLT_TABLE_TRACE_PROC *proc, 
	BLT_TABLE_TRACE_DELETE_PROC *deleteProc, ClientData clientData);

BLT_EXTERN void blt_table_trace_row(BLT_TABLE table,
	BLT_TABLE_ROW row, unsigned int mask, BLT_TABLE_TRACE_PROC *proc, 
	BLT_TABLE_TRACE_DELETE_PROC *deleteProc, ClientData clientData);

BLT_EXTERN BLT_TABLE_TRACE blt_table_create_column_trace(BLT_TABLE table, 
	BLT_TABLE_COLUMN column, unsigned int mask, BLT_TABLE_TRACE_PROC *proc, 
	BLT_TABLE_TRACE_DELETE_PROC *deleteProc, ClientData clientData);

BLT_EXTERN BLT_TABLE_TRACE blt_table_create_column_tag_trace(BLT_TABLE table, 
	const char *tag, unsigned int mask, BLT_TABLE_TRACE_PROC *proc, 
	BLT_TABLE_TRACE_DELETE_PROC *deleteProc, ClientData clientData);

BLT_EXTERN BLT_TABLE_TRACE blt_table_create_row_trace(BLT_TABLE table,
	BLT_TABLE_ROW row, unsigned int mask, BLT_TABLE_TRACE_PROC *proc, 
	BLT_TABLE_TRACE_DELETE_PROC *deleteProc, ClientData clientData);

BLT_EXTERN BLT_TABLE_TRACE blt_table_create_row_tag_trace(BLT_TABLE table, 
	const char *tag, unsigned int mask, BLT_TABLE_TRACE_PROC *proc, 
	BLT_TABLE_TRACE_DELETE_PROC *deleteProc, ClientData clientData);

BLT_EXTERN void blt_table_delete_trace(BLT_TABLE table, BLT_TABLE_TRACE trace);

/*
 * BLT_TABLE_NOTIFY_EVENT --
 *
 *	Structure representing a trace used by a client of the table.
 *
 *	Table rows and columns may be tagged with strings.  A row may have
 *	many tags.  The same tag may be used for many rows.  Tags are used
 *	and stored by clients of a table.  Tags can also be shared between
 *	clients of the same table.
 *	
 *	Both rowTable and columnTable are hash tables keyed by the physical
 *	row or column location in the table respectively.  This is not the
 *	same as the client's view (the order of rows or columns as seen by
 *	the client).  This is so that clients (which may have different
 *	views) can share tags without sharing the same view.
 */
typedef struct {
    Tcl_Interp *interp;			/* Interpreter to report results. */
    BLT_TABLE table;			/* Table object client that
					 * received the event. */
    int self;				/* Indicates if this table client
					 * generated the event. */
    int type;			        /* Indicates type of event
					 * received. */
    BLT_TABLE_ROW row;			/* If NULL, indicates all rows have
					 * changed. */
    BLT_TABLE_COLUMN column;		/* If NULL, indicates all columns
					 * have changed. */
} BLT_TABLE_NOTIFY_EVENT;

typedef int (BLT_TABLE_NOTIFY_EVENT_PROC)(ClientData clientData, 
	BLT_TABLE_NOTIFY_EVENT *eventPtr);

typedef void (BLT_TABLE_NOTIFIER_DELETE_PROC)(ClientData clientData);

typedef struct _BLT_TABLE_NOTIFIER {
    BLT_TABLE table;
    Blt_ChainLink link;
    Blt_Chain chain;
    BLT_TABLE_NOTIFY_EVENT event;
    BLT_TABLE_NOTIFY_EVENT_PROC *proc;
    BLT_TABLE_NOTIFIER_DELETE_PROC *deleteProc;
    ClientData clientData;
    Tcl_Interp *interp;
    BLT_TABLE_ROW row;
    BLT_TABLE_COLUMN column;
    char *tag;
    unsigned int flags;
} *BLT_TABLE_NOTIFIER;


#define TABLE_NOTIFY_CREATE	    (1<<0)
#define TABLE_NOTIFY_DELETE	    (1<<1)
#define TABLE_NOTIFY_MOVE	    (1<<2)
#define TABLE_NOTIFY_RELABEL	    (1<<3)
#define TABLE_NOTIFY_ROW	    (1<<4)
#define TABLE_NOTIFY_COLUMN	    (1<<5)
#define TABLE_NOTIFY_ROWS_CREATED    (TABLE_NOTIFY_CREATE|TABLE_NOTIFY_ROW)
#define TABLE_NOTIFY_ROWS_DELETED    (TABLE_NOTIFY_DELETE|TABLE_NOTIFY_ROW)
#define TABLE_NOTIFY_ROWS_RELABEL    (TABLE_NOTIFY_RELABEL|TABLE_NOTIFY_ROW)
#define TABLE_NOTIFY_ROWS_MOVED      (TABLE_NOTIFY_MOVE|TABLE_NOTIFY_ROW)
#define TABLE_NOTIFY_COLUMNS_CREATED (TABLE_NOTIFY_CREATE|TABLE_NOTIFY_COLUMN)
#define TABLE_NOTIFY_COLUMNS_DELETED (TABLE_NOTIFY_DELETE|TABLE_NOTIFY_COLUMN)
#define TABLE_NOTIFY_COLUMNS_RELABEL (TABLE_NOTIFY_RELABEL|TABLE_NOTIFY_COLUMN)
#define TABLE_NOTIFY_COLUMNS_MOVED   (TABLE_NOTIFY_MOVE|TABLE_NOTIFY_COLUMN)

#define TABLE_NOTIFY_COLUMN_CHANGED \
	(TABLE_NOTIFY_CREATE | TABLE_NOTIFY_DELETE | \
	 TABLE_NOTIFY_MOVE | TABLE_NOTIFY_RELABEL | TABLE_NOTIFY_COLUMN)
#define TABLE_NOTIFY_ROW_CHANGED \
	(TABLE_NOTIFY_CREATE | TABLE_NOTIFY_DELETE | \
	 TABLE_NOTIFY_MOVE | TABLE_NOTIFY_RELABEL | TABLE_NOTIFY_ROW)
    
#define TABLE_NOTIFY_ALL_EVENTS \
	(TABLE_NOTIFY_ROW_CHANGED | TABLE_NOTIFY_COLUMN_CHANGED)

#define TABLE_NOTIFY_TYPE_MASK	(TABLE_NOTIFY_ROW | TABLE_NOTIFY_COLUMN)

#define TABLE_NOTIFY_EVENT_MASK	TABLE_NOTIFY_ALL_EVENTS
#define TABLE_NOTIFY_MASK	(TABLE_NOTIFY_EVENT_MASK | \
				 TABLE_NOTIFY_TYPE_MASK)

#define TABLE_NOTIFY_WHENIDLE      (1<<10)
#define TABLE_NOTIFY_FOREIGN_ONLY  (1<<11)
#define TABLE_NOTIFY_PENDING       (1<<12)
#define TABLE_NOTIFY_ACTIVE        (1<<13)
#define TABLE_NOTIFY_DESTROYED     (1<<14)

#define TABLE_NOTIFY_ALL	   (NULL)

BLT_EXTERN BLT_TABLE_NOTIFIER blt_table_create_notifier(Tcl_Interp *interp, 
	BLT_TABLE table, unsigned int mask, 
	BLT_TABLE_NOTIFY_EVENT_PROC *proc, 
	BLT_TABLE_NOTIFIER_DELETE_PROC *deleteProc,
	ClientData clientData);

BLT_EXTERN BLT_TABLE_NOTIFIER blt_table_create_row_notifier(Tcl_Interp *interp, 
	BLT_TABLE table, BLT_TABLE_ROW row, unsigned int mask, 
	BLT_TABLE_NOTIFY_EVENT_PROC *proc, 
	BLT_TABLE_NOTIFIER_DELETE_PROC *deleteProc,
	ClientData clientData);

BLT_EXTERN BLT_TABLE_NOTIFIER blt_table_create_row_tag_notifier(
	Tcl_Interp *interp,
	BLT_TABLE table, const char *tag, unsigned int mask, 
	BLT_TABLE_NOTIFY_EVENT_PROC *proc, 
	BLT_TABLE_NOTIFIER_DELETE_PROC *deleteProc,
	ClientData clientData);

BLT_EXTERN BLT_TABLE_NOTIFIER blt_table_create_column_notifier(
	Tcl_Interp *interp, BLT_TABLE table, BLT_TABLE_COLUMN column, 
	unsigned int mask, BLT_TABLE_NOTIFY_EVENT_PROC *proc, 
	BLT_TABLE_NOTIFIER_DELETE_PROC *deleteProc, ClientData clientData);

BLT_EXTERN BLT_TABLE_NOTIFIER blt_table_create_column_tag_notifier(
	Tcl_Interp *interp, BLT_TABLE table, const char *tag, 
	unsigned int mask, BLT_TABLE_NOTIFY_EVENT_PROC *proc, 
	BLT_TABLE_NOTIFIER_DELETE_PROC *deleteProc, ClientData clientData);


BLT_EXTERN void blt_table_delete_notifier(BLT_TABLE table, 
	BLT_TABLE_NOTIFIER notifier);

/*
 * BLT_TABLE_SORT_ORDER --
 *
 */
typedef int (BLT_TABLE_COMPARE_PROC)(ClientData clientData, 
	BLT_TABLE_COLUMN col, BLT_TABLE_ROW row1, BLT_TABLE_ROW row2);

typedef struct {
    int type;				/* Type of sort to be performed:
					 * see flags below. */
    BLT_TABLE_COMPARE_PROC *cmpProc;	/* Procedures to be called to
					 * compare two rows in same
					 * column. */
    BLT_TABLE_COMPARE_PROC *userProc;	/* Procedures to be called to
					 * compare two entries in the same
					 * row or column. */
    ClientData clientData;		/* One word of data passed to the
					 * sort comparison procedure
					 * above. */
    BLT_TABLE_COLUMN column;		/* Column to be compared. */
} BLT_TABLE_SORT_ORDER;


#define TABLE_SORT_DECREASING		(1<<0)

#define TABLE_SORT_TYPE_MASK		(3<<2)
#define TABLE_SORT_AUTO			(0)
#define TABLE_SORT_ASCII		(1<<2)
#define TABLE_SORT_DICTIONARY		(2<<2)
#define TABLE_SORT_FREQUENCY		(3<<2)

BLT_EXTERN void blt_table_sort_init(BLT_TABLE table, 
	BLT_TABLE_SORT_ORDER *order, size_t numCompares, unsigned int flags);
BLT_EXTERN BLT_TABLE_ROW *blt_table_sort_rows(BLT_TABLE table);
BLT_EXTERN void blt_table_sort_rows_subset(BLT_TABLE table, long numRows, 
	BLT_TABLE_ROW *rows);
BLT_EXTERN void blt_table_sort_finish(void);
BLT_EXTERN BLT_TABLE_COMPARE_PROC *blt_table_get_compare_proc(BLT_TABLE table, 
	BLT_TABLE_COLUMN column, unsigned int flags);

BLT_EXTERN BLT_TABLE_ROW *blt_table_get_row_map(BLT_TABLE table);
BLT_EXTERN BLT_TABLE_COLUMN *blt_table_get_column_map(BLT_TABLE table);

BLT_EXTERN void blt_table_set_row_map(BLT_TABLE table, BLT_TABLE_ROW *map);
BLT_EXTERN void blt_table_set_column_map(BLT_TABLE table, BLT_TABLE_COLUMN *map);

#define TABLE_RESTORE_NO_TAGS	    (1<<0)
#define TABLE_RESTORE_OVERWRITE	    (1<<1)

BLT_EXTERN int blt_table_restore(Tcl_Interp *interp, BLT_TABLE table, 
	char *string, unsigned int flags);
BLT_EXTERN int blt_table_file_restore(Tcl_Interp *interp, BLT_TABLE table, 
	const char *fileName, unsigned int flags);

typedef int (BLT_TABLE_IMPORT_PROC)(BLT_TABLE table, Tcl_Interp *interp, 
	int objc, Tcl_Obj *const *objv);

typedef int (BLT_TABLE_EXPORT_PROC)(BLT_TABLE table, Tcl_Interp *interp,
	int objc, Tcl_Obj *const *objv);

BLT_EXTERN int blt_table_register_format(Tcl_Interp *interp, const char *name, 
	BLT_TABLE_IMPORT_PROC *importProc, BLT_TABLE_EXPORT_PROC *exportProc);

BLT_EXTERN void blt_table_unset_keys(BLT_TABLE table);
BLT_EXTERN int blt_table_get_keys(BLT_TABLE table, BLT_TABLE_COLUMN **keysPtr);
BLT_EXTERN int blt_table_set_keys(BLT_TABLE table, int numKeys,
	BLT_TABLE_COLUMN *keys, int unique);
BLT_EXTERN int blt_table_key_lookup(Tcl_Interp *interp, BLT_TABLE table,
	int objc, Tcl_Obj *const *objv, BLT_TABLE_ROW *rowPtr);

BLT_EXTERN int blt_table_get_column_limits(Tcl_Interp *interp, BLT_TABLE table, 
	BLT_TABLE_COLUMN col, Tcl_Obj **minObjPtrPtr, Tcl_Obj **maxObjPtrPtr);

#define blt_table_num_rows(t)		((t)->corePtr->rows.numUsed)
#define blt_table_row_index(r)		((r)->index)
#define blt_table_row_label(r)		((r)->label)
#define blt_table_row(t,i)  \
    (BLT_TABLE_ROW)((t)->corePtr->rows.map[(i)])

#define blt_table_num_columns(t)	((t)->corePtr->columns.numUsed)
#define blt_table_column_index(c)	((c)->index)
#define blt_table_column_label(c)	((c)->label)
#define blt_table_column(t,i) \
	(BLT_TABLE_COLUMN)((t)->corePtr->columns.map[(i)])

#define blt_table_name(t)		((t)->name)
#define blt_table_empty_value(t)	((t)->emptyValue)
#define blt_table_column_type(c)	((c)->type)

#endif /* BLT_DATATABLE_H */
