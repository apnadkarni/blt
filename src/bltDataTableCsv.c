/* -*- mode: c; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 *
 * bltDataTableCsv.c --
 *
 *	Copyright 1998-2005 George A Howlett.
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

#include <bltInt.h>
#ifndef NO_DATATABLE

#ifdef HAVE_CTYPE_H
#  include <ctype.h>
#endif /* HAVE_CTYPE_H */

#ifdef HAVE_MEMORY_H
#  include <memory.h>
#endif /* HAVE_MEMORY_H */

#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif /* HAVE_STDLIB_H */

#define IsSeparator(argsPtr, c) ((argsPtr)->separatorChar == (c))

DLLEXPORT extern Tcl_AppInitProc blt_table_csv_init;
DLLEXPORT extern Tcl_AppInitProc blt_table_csv_safe_init;

#define TRUE 	1
#define FALSE 	0

#define EXPORT_ROWLABELS	(1<<0)
#define EXPORT_COLUMNLABELS	(1<<1)

/*
 * Format	Import		Export
 * csv		file/data	file/data
 * tree		data		data
 * vector	data		data
 * xml		file/data	file/data
 * sql		data		data
 *
 * $table import csv -file fileName -data dataString 
 * $table export csv -file defaultFileName 
 * $table import tree $treeName $node ?switches? 
 * $table export tree $treeName $node "label" "label" "label"
 * $table import vector $vecName label $vecName label...
 * $table export vector label $vecName label $vecName..
 * $table import xml -file fileName -data dataString ?switches?
 * $table export xml -file fileName -data dataString ?switches?
 * $table import sql -host $host -password $pw -db $db -port $port 
 */

/*
 * ImportArgs --
 */
typedef struct {
    unsigned int flags;
    Tcl_Channel channel;		/* If non-NULL, channel to read
					 * from. */
    Tcl_Obj *encodingObjPtr;
    char *buffer;			/* Buffer to read data into. */
    int numBytes;                       /* # of bytes in the buffer. */
    Tcl_DString ds;			/* Dynamic string used to read the
					 * file line-by-line. */
    Tcl_Interp *interp;
    Blt_HashTable dataTable;
    Tcl_Obj *fileObjPtr;		/* Name of file representing the
					 * channel used as the input
					 * source. */
    Tcl_Obj *dataObjPtr;		/* If non-NULL, data object to use as
					 * input source. */
    const char *reqQuote;               /* Quoted string delimiter. */
    const char *reqSeparator;		/* Separator character. */
    const char *reqComment;		/* Comment character. */
    char separatorChar;                 /* Separator character. */
    char quoteChar;                     /* Quote character. */
    char commentChar;                   /* Comment character. */
    Tcl_Obj *emptyValueObjPtr;          /* If non-NULL, empty value. */
    int maxRows;			/* Stop processing after this many
					 * rows have been found. */
} ImportArgs;

static Blt_SwitchSpec importSwitches[] = 
{
    {BLT_SWITCH_STRING, "-comment",     "char", (char *)NULL,
	Blt_Offset(ImportArgs, reqComment), 0},
    {BLT_SWITCH_OBJ,	"-data",      "string", (char *)NULL,
	Blt_Offset(ImportArgs, dataObjPtr), 0, 0, NULL},
    {BLT_SWITCH_OBJ,	"-encoding",  "string", (char *)NULL,
	Blt_Offset(ImportArgs, encodingObjPtr), 0, 0, NULL},
    {BLT_SWITCH_OBJ,	"-file",      "fileName", (char *)NULL,
	Blt_Offset(ImportArgs, fileObjPtr), 0},
    {BLT_SWITCH_INT_NNEG, "-maxrows", "integer", (char *)NULL,
	Blt_Offset(ImportArgs, maxRows), 0},
    {BLT_SWITCH_STRING, "-quote",     "char", (char *)NULL,
	Blt_Offset(ImportArgs, reqQuote), 0},
    {BLT_SWITCH_STRING, "-separator", "char", (char *)NULL,
	Blt_Offset(ImportArgs, reqSeparator), 0},
    {BLT_SWITCH_OBJ,    "-empty",      "string", (char *)NULL,
	Blt_Offset(ImportArgs, emptyValueObjPtr), 0},
    {BLT_SWITCH_END}
};

/*
 * ExportArgs --
 */
typedef struct {
    BLT_TABLE_ITERATOR ri, ci;
    unsigned int flags;
    Tcl_Obj *fileObjPtr;
    Tcl_Channel channel;		/* If non-NULL, channel to write
					 * output to. */
    Tcl_DString *dsPtr;
    int length;				/* Length of dynamic string. */
    int count;				/* # of fields in current record. */
    Tcl_Interp *interp;
    const char *reqQuote;		/* Quoted string delimiter. */
    const char *reqSeparator;           /* Separator character. */
    char separatorChar;
    char quoteChar;
} ExportArgs;

static Blt_SwitchFreeProc ColumnIterFreeProc;
static Blt_SwitchParseProc ColumnIterSwitchProc;
static Blt_SwitchCustom columnIterSwitch = {
    ColumnIterSwitchProc, NULL, ColumnIterFreeProc, 0,
};
static Blt_SwitchFreeProc RowIterFreeProc;
static Blt_SwitchParseProc RowIterSwitchProc;
static Blt_SwitchCustom rowIterSwitch = {
    RowIterSwitchProc, NULL, RowIterFreeProc, 0,
};

static Blt_SwitchSpec exportSwitches[] = 
{
    {BLT_SWITCH_CUSTOM, "-columns",   "columns" ,(char *)NULL,
	Blt_Offset(ExportArgs, ci),   0, 0, &columnIterSwitch},
    {BLT_SWITCH_OBJ,    "-file",      "fileName", (char *)NULL,
	Blt_Offset(ExportArgs, fileObjPtr), 0},
    {BLT_SWITCH_BITMASK, "-rowlabels",  "", (char *)NULL,
	Blt_Offset(ExportArgs, flags), 0, EXPORT_ROWLABELS},
    {BLT_SWITCH_BITMASK, "-columnlabels",  "", (char *)NULL,
	Blt_Offset(ExportArgs, flags), 0, EXPORT_COLUMNLABELS},
    {BLT_SWITCH_STRING, "-quote",     "char", (char *)NULL,
	Blt_Offset(ExportArgs, reqQuote),   0},
    {BLT_SWITCH_CUSTOM, "-rows",      "rows", (char *)NULL,
	Blt_Offset(ExportArgs, ri),   0, 0, &rowIterSwitch},
    {BLT_SWITCH_STRING, "-separator", "char", (char *)NULL,
	Blt_Offset(ExportArgs, reqSeparator),     0},
    {BLT_SWITCH_END}
};

static BLT_TABLE_IMPORT_PROC ImportCsvProc;
static BLT_TABLE_EXPORT_PROC ExportCsvProc;

/*
 *---------------------------------------------------------------------------
 *
 * ColumnIterFreeProc --
 *
 *	Free the storage associated with the -columns switch.
 *
 * Results:
 *	None.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static void
ColumnIterFreeProc(ClientData clientData, char *record, int offset, int flags)
{
    BLT_TABLE_ITERATOR *iterPtr = (BLT_TABLE_ITERATOR *)(record + offset);

    blt_table_free_iterator_objv(iterPtr);
}

/*
 *---------------------------------------------------------------------------
 *
 * ColumnIterSwitchProc --
 *
 *	Convert a Tcl_Obj representing an offset in the table.
 *
 * Results:
 *	The return value is a standard TCL result.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
ColumnIterSwitchProc(
    ClientData clientData,		/* Flag indicating if the node is
					 * considered before or after the
					 * insertion position. */
    Tcl_Interp *interp,			/* Interpreter to report results. */
    const char *switchName,		/* Not used. */
    Tcl_Obj *objPtr,			/* String representation */
    char *record,			/* Structure record */
    int offset,				/* Offset to field in structure */
    int flags)				/* Not used. */
{
    BLT_TABLE_ITERATOR *iterPtr = (BLT_TABLE_ITERATOR *)(record + offset);
    BLT_TABLE table;
    Tcl_Obj **objv;
    int objc;

    table = clientData;
    if (Tcl_ListObjGetElements(interp, objPtr, &objc, &objv) != TCL_OK) {
	return TCL_ERROR;
    }
    if (blt_table_iterate_column_objv(interp, table, objc, objv, iterPtr)
	!= TCL_OK) {
	return TCL_ERROR;
    }
    return TCL_OK;
}


/*
 *---------------------------------------------------------------------------
 *
 * RowIterFreeProc --
 *
 *	Free the storage associated with the -rows switch.
 *
 * Results:
 *	None.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static void
RowIterFreeProc(ClientData clientData, char *record, int offset, int flags)
{
    BLT_TABLE_ITERATOR *iterPtr = (BLT_TABLE_ITERATOR *)(record + offset);

    blt_table_free_iterator_objv(iterPtr);
}

/*
 *---------------------------------------------------------------------------
 *
 * RowIterSwitchProc --
 *
 *	Convert a Tcl_Obj representing an offset in the table.
 *
 * Results:
 *	The return value is a standard TCL result.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
RowIterSwitchProc(
    ClientData clientData,		/* Flag indicating if the node is
					 * considered before or after the
					 * insertion position. */
    Tcl_Interp *interp,			/* Interpreter to report results. */
    const char *switchName,		/* Not used. */
    Tcl_Obj *objPtr,			/* String representation */
    char *record,			/* Structure record */
    int offset,				/* Offset to field in structure */
    int flags)				/* Not used. */
{
    BLT_TABLE_ITERATOR *iterPtr = (BLT_TABLE_ITERATOR *)(record + offset);
    BLT_TABLE table;
    Tcl_Obj **objv;
    int objc;

    table = clientData;
    if (Tcl_ListObjGetElements(interp, objPtr, &objc, &objv) != TCL_OK) {
	return TCL_ERROR;
    }
    if (blt_table_iterate_row_objv(interp, table, objc, objv, iterPtr)
	!= TCL_OK) {
	return TCL_ERROR;
    }
    return TCL_OK;
}

static int charCounts[4];
static const char *sepTokens = { ",\t|;"};

static int
CompareCounts(const void *t1, const void *t2)
{
    int a = *(int *)t1;
    int b = *(int *)t2;
    return (charCounts[b] - charCounts[a]);
}

static void
GuessSeparator(ImportArgs *argsPtr)
{
    long pos;
    int map[4];
    const char *string, *p;
    int i;
    int length;
    Tcl_Obj *objPtr;
    
    pos = Tcl_Tell(argsPtr->channel);
    string = NULL;                      /* Suppress compiler warning. */
    if (argsPtr->channel != NULL) {
	objPtr = Tcl_NewStringObj("", -1);
	Tcl_ReadChars(argsPtr->channel, objPtr, 2000, FALSE);
	string = Tcl_GetStringFromObj(objPtr, &length);
    } else if (argsPtr->numBytes > 0) {
	string = argsPtr->buffer;
    }
    for (i = 0; i < 4; i++) {
	charCounts[i] = 0;
	map[i] = i;
    }
    for (p = string; (*p != '\0') && (p - string) < 2000; p++) {
	switch (*p) {
	case ',':                       /* Comma */
	    charCounts[0]++;         break;
	case '\t':                      /* Tab */
	    charCounts[1]++;         break;
	case '|':                       /* Pipe */
	    charCounts[2]++;         break;
	case ';':                       /* Semicolon */
	    charCounts[3]++;         break;
	}
    }
    if (argsPtr->channel != NULL) {
	Tcl_Seek(argsPtr->channel, pos, SEEK_SET);
	Tcl_DecrRefCount(objPtr);
    }
    qsort(map, 4, sizeof(int), CompareCounts);
    argsPtr->separatorChar = sepTokens[map[0]];
}
	
static void
StartCsvRecord(ExportArgs *argsPtr)
{
    if (argsPtr->channel != NULL) {
	Tcl_DStringSetLength(argsPtr->dsPtr, 0);
	argsPtr->length = 0;
    }
    argsPtr->count = 0;
}

static int
EndCsvRecord(ExportArgs *argsPtr)
{
    int numWritten;
    char *line;

    Tcl_DStringAppend(argsPtr->dsPtr, "\n", 1);
    argsPtr->length++;
    line = Tcl_DStringValue(argsPtr->dsPtr);
    if (argsPtr->channel != NULL) {
	numWritten = Tcl_Write(argsPtr->channel, line, argsPtr->length);
	if (numWritten != argsPtr->length) {
	    Tcl_AppendResult(argsPtr->interp, "can't write csv record: ",
			     Tcl_PosixError(argsPtr->interp), (char *)NULL);
	    return TCL_ERROR;
	}
    }
    return TCL_OK;
}

static void
AppendCsvRecord(ExportArgs *argsPtr, const char *field, int length, 
		BLT_TABLE_COLUMN_TYPE type)
{
    const char *fp;
    char *p;
    int extra, doQuote;

    doQuote = (type == TABLE_COLUMN_TYPE_STRING);
    extra = 0;
    if (field == NULL) {
	length = 0;
    } else {
	for (fp = field; *fp != '\0'; fp++) {
	    if ((*fp == '\n') || (*fp == argsPtr->separatorChar) || 
		(*fp == ' ') || (*fp == '\t')) {
		doQuote = TRUE;
	    } else if (*fp == argsPtr->quoteChar) {
		doQuote = TRUE;
		extra++;
	    }
	}
	if (doQuote) {
	    extra += 2;
	}
	if (length < 0) {
	    length = fp - field;
	}
    }
    if (argsPtr->count > 0) {
	Tcl_DStringAppend(argsPtr->dsPtr, &argsPtr->separatorChar, 1);
	argsPtr->length++;
    }
    length = length + extra + argsPtr->length;
    Tcl_DStringSetLength(argsPtr->dsPtr, length);
    p = Tcl_DStringValue(argsPtr->dsPtr) + argsPtr->length;
    argsPtr->length = length;
    if (field != NULL) {
	if (doQuote) {
	    *p++ = argsPtr->quoteChar;
	}
	for (fp = field; *fp != '\0'; fp++) {
	    if (*fp == argsPtr->quoteChar) {
		*p++ = argsPtr->quoteChar;
	    }
	    *p++ = *fp;
	}
	if (doQuote) {
	    *p++ = argsPtr->quoteChar;
	}
    }
    argsPtr->count++;
}

static int
ExportCsvRows(BLT_TABLE table, ExportArgs *argsPtr)
{
    BLT_TABLE_ROW row;

    for (row = blt_table_first_tagged_row(&argsPtr->ri); row != NULL; 
	 row = blt_table_next_tagged_row(&argsPtr->ri)) {
	BLT_TABLE_COLUMN col;
	    
	StartCsvRecord(argsPtr);
	if (argsPtr->flags & EXPORT_ROWLABELS) {
	    const char *field;

	    field = blt_table_row_label(row);
	    AppendCsvRecord(argsPtr, field, -1, TABLE_COLUMN_TYPE_STRING);
	}
	for (col = blt_table_first_tagged_column(&argsPtr->ci); col != NULL; 
	     col = blt_table_next_tagged_column(&argsPtr->ci)) {
	    const char *string;
	    BLT_TABLE_COLUMN_TYPE type;
		
	    type = blt_table_column_type(col);
	    string = blt_table_get_string(table, row, col);
	    AppendCsvRecord(argsPtr, string, -1, type);
	}
	if (EndCsvRecord(argsPtr) != TCL_OK) {
	    return TCL_ERROR;
	}
    }
    return TCL_OK;
}

static int
ExportCsvColumns(ExportArgs *argsPtr)
{
    if (argsPtr->flags & EXPORT_COLUMNLABELS) {
	BLT_TABLE_COLUMN col;

	StartCsvRecord(argsPtr);
	if (argsPtr->flags & EXPORT_ROWLABELS) {
	    AppendCsvRecord(argsPtr, "*BLT*", 5, TABLE_COLUMN_TYPE_STRING);
	}
	for (col = blt_table_first_tagged_column(&argsPtr->ci); col != NULL; 
	     col = blt_table_next_tagged_column(&argsPtr->ci)) {
	    AppendCsvRecord(argsPtr, blt_table_column_label(col), -1, 
		TABLE_COLUMN_TYPE_STRING);
	}
	return EndCsvRecord(argsPtr);
    }
    return TCL_OK;
}

/* 
 * $table exportfile fileName ?switches...?
 * $table exportdata ?switches...?
 */
static int
ExportCsvProc(BLT_TABLE table, Tcl_Interp *interp, int objc, 
	      Tcl_Obj *const *objv)
{
    ExportArgs args;
    Tcl_Channel channel;
    Tcl_DString ds;
    int closeChannel;
    int result;

    closeChannel = FALSE;
    channel = NULL;

    Tcl_DStringInit(&ds);
    memset(&args, 0, sizeof(args));
    args.separatorChar = ',';
    args.quoteChar = '\"';
    args.flags = EXPORT_COLUMNLABELS;
    rowIterSwitch.clientData = table;
    columnIterSwitch.clientData = table;
    blt_table_iterate_all_rows(table, &args.ri);
    blt_table_iterate_all_columns(table, &args.ci);
    if (Blt_ParseSwitches(interp, exportSwitches, objc - 3, objv + 3, &args,
	BLT_SWITCH_DEFAULTS) < 0) {
	return TCL_ERROR;
    }
    result = TCL_ERROR;
    if (args.fileObjPtr != NULL) {
	const char *fileName;

	closeChannel = TRUE;
	fileName = Tcl_GetString(args.fileObjPtr);
	if ((fileName[0] == '@') && (fileName[1] != '\0')) {
	    int mode;
	    
	    channel = Tcl_GetChannel(interp, fileName+1, &mode);
	    if (channel == NULL) {
		goto error;
	    }
	    if ((mode & TCL_WRITABLE) == 0) {
		Tcl_AppendResult(interp, "channel \"", fileName, 
				 "\" not opened for writing", (char *)NULL);
		goto error;
	    }
	    closeChannel = FALSE;
	} else {
	    channel = Tcl_OpenFileChannel(interp, fileName, "w", 0666);
	    if (channel == NULL) {
		goto error;	/* Can't open export file. */
	    }
	}
    }
    args.interp = interp;
    args.dsPtr = &ds;
    args.channel = channel;
    if ((args.reqSeparator != NULL) && (args.reqSeparator[0] != '\0')) {
	args.separatorChar = args.reqSeparator[0];
    }
    if ((args.reqQuote != NULL) && (args.reqQuote[0] != '\0')) {
	args.quoteChar = args.reqQuote[0];
    }
    result = ExportCsvColumns(&args); 
    if (result == TCL_OK) {
	result = ExportCsvRows(table, &args);
    }
    if ((args.channel == NULL) && (result == TCL_OK)) {
	Tcl_DStringResult(interp, &ds);
    } 
 error:
    Tcl_DStringFree(&ds);
    if (closeChannel) {
	Tcl_Close(interp, channel);
    }
    Blt_FreeSwitches(exportSwitches, (char *)&args, 0);
    return result;
}

/* 
 * IsEmpty -- 
 *
 */
static INLINE int
IsEmpty(ImportArgs *argsPtr, const char *field, size_t count)
{
    const char *value;
    int length;

    if (argsPtr->emptyValueObjPtr == NULL) {
	return FALSE;                   /* No empty value defined. */
    }
    value = Tcl_GetStringFromObj(argsPtr->emptyValueObjPtr, &length);
    if (count > 0) {
	return (strncmp(field, value, count) == 0); /* Matches empty value. */
    }
    return (length == 0);               /* Check if count==length==0 */
}

/* 
 * ImportGetLine -- 
 *
 *	Get a single line from the input buffer or file.  We don't remove
 *      newlines.  The parser needs them to determine if we are really at
 *      the end of a row or in a quoted field.  So the resulting line
 *      always contains a new line unless an error occurs or we hit EOF.
 *
 */
static int
ImportGetLine(Tcl_Interp *interp, ImportArgs *argsPtr, char **bufferPtr,
		int *numBytesPtr)
{
    if (argsPtr->channel != NULL) {
	int numChars;

	if (Tcl_Eof(argsPtr->channel)) {
	    *numBytesPtr = 0;
	    return TCL_OK;
	}
	Tcl_DStringSetLength(&argsPtr->ds, 0);
	numChars = Tcl_Gets(argsPtr->channel, &argsPtr->ds);
	if (numChars < 0) {
	    if (Tcl_Eof(argsPtr->channel)) {
		*numBytesPtr = 0;
		return TCL_OK;
	    }
	    *numBytesPtr = numChars;
	    Tcl_AppendResult(interp, "error reading file: ", 
			     Tcl_PosixError(interp), (char *)NULL);
	    return TCL_ERROR;
	}
	/* Put back the newline. */
	Tcl_DStringAppend(&argsPtr->ds, "\n", 1);
	*numBytesPtr = Tcl_DStringLength(&argsPtr->ds);
	*bufferPtr = Tcl_DStringValue(&argsPtr->ds);
    } else {
	char *bp, *bend;
	int numBytes;

	for (bp = argsPtr->buffer, bend = bp + argsPtr->numBytes; bp < bend;
	     bp++) {
	    if (*bp == '\n') {
		bp++;			/* Keep the newline. */
		break;
	    }
	}
	/* Do bookkeeping on buffer pointer and size. */
	*bufferPtr = argsPtr->buffer;
	numBytes = bp - argsPtr->buffer;
	*numBytesPtr = numBytes;
	argsPtr->numBytes -= numBytes; /* Important to reduce bytes left
					  * regardless of trailing
					  * newline. */
	if (numBytes > 0) {
	    if (*(bp-1) != '\n') {
		/* The last newline has been trimmed.  Append a newline.
		 * Don't change the data object's string
		 * representation. Copy the line and append the newline. */
		assert(*bp == '\0');
		Tcl_DStringSetLength(&argsPtr->ds, 0);
		Tcl_DStringAppend(&argsPtr->ds, argsPtr->buffer, numBytes);
		Tcl_DStringAppend(&argsPtr->ds, "\n", 1);
		*numBytesPtr = Tcl_DStringLength(&argsPtr->ds);
		*bufferPtr = Tcl_DStringValue(&argsPtr->ds);
	    } else {
		argsPtr->buffer += numBytes;
	    }
	}
    }
    return TCL_OK;
}


static int
ImportCsv(Tcl_Interp *interp, BLT_TABLE table, ImportArgs *argsPtr)
{
    Tcl_DString ds;
    char *fp, *field;
    int fieldSize;
    int inQuotes, isQuoted, isPath;
    int result;
    long i;
    BLT_TABLE_ROW row;
    BLT_TABLE_COLUMN col;

    result = TCL_ERROR;
    isPath = isQuoted = inQuotes = FALSE;
    row = NULL;
    i = 0;
    Tcl_DStringInit(&ds);
    fieldSize = 128;
    Tcl_DStringSetLength(&ds, fieldSize + 1);
    fp = field = Tcl_DStringValue(&ds);
    for (;;) {
	char *bp, *bend;
	int numBytes;

	result = ImportGetLine(interp, argsPtr, &bp, &numBytes);
	if (result != TCL_OK) {
	    goto error;			/* I/O Error. */
	}
	if (numBytes == 0) {
	    break;			/* EOF */
	}
	bend = bp + numBytes;
	while ((bp < bend) && (isspace(*bp)) && (!IsSeparator(argsPtr, *bp))){
	    bp++;			/* Skip leading spaces. */
	}
	if ((*bp == '\0') || (*bp == argsPtr->commentChar)) {
	    continue;			/* Ignore blank or comment lines */
	}
	for (/*empty*/; bp < bend; bp++) {
	    if ((IsSeparator(argsPtr, *bp)) || (*bp == '\n')) {
		if (inQuotes) {
		    *fp++ = *bp;	/* Copy the separator or newline. */
		} else {
		    BLT_TABLE_COLUMN col;
		    char *last;

		    if ((isPath) && (IsSeparator(argsPtr, *bp)) && 
			(fp != field) && (*(fp - 1) != '\\')) {
			*fp++ = *bp;	/* Copy the separator or newline. */
			goto done;
		    }    
		    /* "last" points to the character after the last character
		     * in the field. */
		    last = fp;	

		    /* Remove trailing spaces only if the field wasn't
		     * quoted. */
		    if ((!isQuoted) && (!isPath)) {
			while ((last > field) && (isspace(*(last - 1)))) {
			    last--;
			}
		    }
		    if (row == NULL) {
			if ((*bp == '\n') &&  (fp == field)) {
			    goto done;	/* Ignore empty lines. */
			}
			if (blt_table_extend_rows(interp, table, 1, &row) 
			    != TCL_OK) {
			    goto error;
			}
			if ((argsPtr->maxRows > 0) && 
			    (blt_table_num_rows(table) > argsPtr->maxRows)) {
			    bp = bend;
			    goto done;
			}
		    }
		    /* End of field. Append field to row. */
		    if (i >= blt_table_num_columns(table)) {
			if (blt_table_extend_columns(interp, table, 1, &col) 
			    != TCL_OK) {
			    goto error;
			}
		    } else {
			col = blt_table_column(table, i);
		    }
		    if (((last > field) || (isQuoted)) && 
			(!IsEmpty(argsPtr, field, last - field))) {
			if (blt_table_set_string_rep(table, row, col, field,
				last - field) != TCL_OK) {             
			    goto error;
			}
		    }
		    i++;
		    if (*bp == '\n') {
			row = NULL;
			i = 0;
		    }
		    fp = field;
		    isPath = isQuoted = FALSE;
		}
	    done:
		;
	    } else if ((*bp == ' ') || (*bp == '\t')) {
		/* 
		 * Include whitespace in the field only if it's not leading or
		 * we're inside of quotes or a path.
		 */
		if ((fp != field) || (inQuotes) || (isPath)) {
		    *fp++ = *bp; 
		}
	    } else if (*bp == '\\') {
		/* 
		 * Handle special case CSV files that allow unquoted paths.
		 * Example:  ...,\this\path " should\have been\quoted\,...
		 */
		if (fp == field) {
		    isPath = TRUE; 
		}
		*fp++ = *bp;
	    } else if (*bp == argsPtr->quoteChar ) {
		if (inQuotes) {
		    if (*(bp+1) == argsPtr->quoteChar) {
			*fp++ = argsPtr->quoteChar;
			bp++;
		    } else {
			inQuotes = FALSE;
		    }
		} else {
		    /* 
		     * If the quote doesn't start a field, then treat all
		     * quotes in the field as ordinary characters.
		     */
		    if (fp == field) {
			isQuoted = inQuotes = TRUE; 
		    } else {
			*fp++ = *bp;
		    }
		}
	    } else {
		*fp++ = *bp;	/* Copy the character. */
	    }
	    if ((fp - field) >= fieldSize) {
		int offset;

		/* 
		 * We've exceeded the current maximum size of the field.
		 * Double the size of the field, but make sure to reset the
		 * pointers to the (possibly) new memory location.
		 */
		offset = fp - field;
		fieldSize += fieldSize;
		Tcl_DStringSetLength(&ds, fieldSize + 1);
		field = Tcl_DStringValue(&ds);
		fp = field + offset;
	    }
	}
	if (numBytes < 1) {
	    /* 
	     * We're reached the end of input. But there may not have been a
	     * final newline to trigger the final append. So check if a last
	     * field is still needs appending the the last row.
	     */
	    if (fp != field) {
		char *last;

		last = fp;
		/* Remove trailing spaces */
		while (isspace(*(last - 1))) {
		    last--;
		}
		if (row == NULL) {
		    if (blt_table_extend_rows(interp, table, 1, &row) != TCL_OK){
			goto error;
		    }
		}
		col = blt_table_get_column_by_index(table, i);
		if (col == NULL) {
		    if (blt_table_extend_columns(interp, table, 1, &col) 
			!= TCL_OK) {
			goto error;
		    }
		}			
		if (((last > field) || (isQuoted)) && 
		    (!IsEmpty(argsPtr, field, last - field))) {
		    if (blt_table_set_string(table, row, col, field, 
			last - field) != TCL_OK) {
			goto error;
		    }
		}		
	    }    
	    break;
	}
    }
    result = TCL_OK;
 error:
    Tcl_DStringFree(&ds);
    return result;
}

static int
ImportCsvProc(BLT_TABLE table, Tcl_Interp *interp, int objc, 
	      Tcl_Obj *const *objv)
{
    int result;
    ImportArgs args;

    memset(&args, 0, sizeof(args));
    args.quoteChar   = '\"';
    args.separatorChar = ',';
    args.commentChar = '\0';
    Blt_InitHashTable(&args.dataTable, BLT_STRING_KEYS);
    if (Blt_ParseSwitches(interp, importSwitches, objc - 3 , objv + 3, 
	&args, BLT_SWITCH_DEFAULTS) < 0) {
	return TCL_ERROR;
    }
    result = TCL_ERROR;
    if ((args.dataObjPtr != NULL) && (args.fileObjPtr != NULL)) {
	Tcl_AppendResult(interp, "can't set both -file and -data switches.",
			 (char *)NULL);
	goto error;
    }
    if ((args.reqQuote != NULL) && (args.reqQuote[0] != '\0')) {
	args.quoteChar = args.reqQuote[0];
    }
    if ((args.reqComment != NULL) && (args.reqComment[0] != '\0')) {
	args.commentChar = args.reqComment[0];
    }
    if (args.dataObjPtr != NULL) {
	int numBytes;

	args.channel = NULL;
	args.buffer = Tcl_GetStringFromObj(args.dataObjPtr, &numBytes);
	args.numBytes = numBytes;
	args.fileObjPtr = NULL;
	if ((args.reqSeparator == NULL) || (args.reqSeparator[0] == '\0')) {
	    GuessSeparator(&args);
	} else {
	    args.separatorChar = args.reqSeparator[0];
	}
	Tcl_DStringInit(&args.ds);
	result = ImportCsv(interp, table, &args);
	Tcl_DStringFree(&args.ds);
    } else {
	int closeChannel;
	Tcl_Channel channel;
	const char *fileName;

	closeChannel = TRUE;
	if (args.fileObjPtr == NULL) {
	    fileName = "out.csv";
	} else {
	    fileName = Tcl_GetString(args.fileObjPtr);
	}
	if ((fileName[0] == '@') && (fileName[1] != '\0')) {
	    int mode;
	    
	    channel = Tcl_GetChannel(interp, fileName+1, &mode);
	    if (channel == NULL) {
		goto error;
	    }
	    if ((mode & TCL_READABLE) == 0) {
		Tcl_AppendResult(interp, "channel \"", fileName, 
				 "\" not opened for reading", (char *)NULL);
		goto error;
	    }
	    closeChannel = FALSE;
	} else {
	    channel = Tcl_OpenFileChannel(interp, fileName, "r", 0);
	    if (channel == NULL) {
		goto error;
	    }
	}
	if (args.encodingObjPtr != NULL) {
	    if (Tcl_SetChannelOption(interp, channel, "-encoding", 
		Tcl_GetString(args.encodingObjPtr)) != TCL_OK) {
		goto error;
	    }
	}
	args.channel = channel;
	Tcl_DStringInit(&args.ds);
	if ((args.reqSeparator == NULL) || (args.reqSeparator[0] == '\0')) {
	    GuessSeparator(&args);
	} else {
	    args.separatorChar = args.reqSeparator[0];
	}
	result = ImportCsv(interp, table, &args);
	Tcl_DStringFree(&args.ds);
	if (closeChannel) {
	    Tcl_Close(interp, channel);
	}
    }
 error:
    Blt_FreeSwitches(importSwitches, (char *)&args, 0);
    Blt_DeleteHashTable(&args.dataTable);
    return result;
}
    
int 
blt_table_csv_init(Tcl_Interp *interp)
{
#ifdef USE_TCL_STUBS
    if (Tcl_InitStubs(interp, TCL_VERSION_COMPILED, PKG_ANY) == NULL) {
	return TCL_ERROR;
    };
#endif
#ifdef USE_BLT_STUBS
    if (Blt_InitTclStubs(interp, BLT_VERSION, PKG_EXACT) == NULL) {
	return TCL_ERROR;
    };
#else
    if (Tcl_PkgRequire(interp, "blt_tcl", BLT_VERSION, PKG_EXACT) == NULL) {
	return TCL_ERROR;
    }
#endif    
    if (Tcl_PkgProvide(interp, "blt_datatable_csv", BLT_VERSION) != TCL_OK) { 
	return TCL_ERROR;
    }
    return blt_table_register_format(interp,
	"csv",			/* Name of format. */
	ImportCsvProc,		/* Import procedure. */
	ExportCsvProc);		/* Export procedure. */

}

int 
blt_table_csv_safe_init(Tcl_Interp *interp)
{
    return blt_table_csv_init(interp);
}

#endif /* NO_DATATABLE */

