/* -*- mode: c; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 *
 * bltDataTableCsv.c --
 *
 * Copyright 2015 George A. Howlett. All rights reserved.  
 *
 *   Redistribution and use in source and binary forms, with or without
 *   modification, are permitted provided that the following conditions are
 *   met:
 *
 *   1) Redistributions of source code must retain the above copyright
 *      notice, this list of conditions and the following disclaimer.
 *   2) Redistributions in binary form must reproduce the above copyright
 *      notice, this list of conditions and the following disclaimer in the
 *      documentation and/or other materials provided with the
 *      distribution.
 *   3) Neither the name of the authors nor the names of its contributors
 *      may be used to endorse or promote products derived from this
 *      software without specific prior written permission.
 *   4) Products derived from this software may not be called "BLT" nor may
 *      "BLT" appear in their names without specific prior written
 *      permission from the author.
 *
 *   THIS SOFTWARE IS PROVIDED ''AS IS'' AND ANY EXPRESS OR IMPLIED
 *   WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 *   MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 *   DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
 *   LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 *   CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 *   SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
 *   BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 *   WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
 *   OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
 *   IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

#include <bltInt.h>
#ifndef NO_DATATABLE

#ifdef HAVE_CTYPE_H
  #include <ctype.h>
#endif /* HAVE_CTYPE_H */

#ifdef HAVE_MEMORY_H
  #include <memory.h>
#endif /* HAVE_MEMORY_H */

#ifdef HAVE_STDLIB_H
  #include <stdlib.h>
#endif /* HAVE_STDLIB_H */

#define IsSeparator(importPtr, c) ((importPtr)->separatorChar == (c))

DLLEXPORT extern Tcl_AppInitProc blt_table_csv_init;
DLLEXPORT extern Tcl_AppInitProc blt_table_csv_safe_init;

#define TRUE    1
#define FALSE   0

#define EXPORT_ROWLABELS        (1<<0)
#define EXPORT_COLUMNLABELS     (1<<1)

/*
 * Format       Import          Export
 * csv          file/data       file/data
 * tree         data            data
 * vector       data            data
 * xml          file/data       file/data
 * sql          data            data
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
    Tcl_Channel channel;                /* If non-NULL, channel to read
                                         * from. */
    Tcl_Obj *encodingObjPtr;
    const char *buffer;                 /* Buffer to read data into. */
    size_t numBytes;                    /* # of bytes in the buffer. */

    const char *next;                   /* Used for parsing data as a
                                         * single string. */
    size_t bytesLeft;                   /* Used for parsing data as a
                                         * single string. */
    
    Tcl_DString currLine;               /* Dynamic string used to read the
                                         * file line-by-line. */
    Tcl_Interp *interp;
    Tcl_Obj *fileObjPtr;                /* Name of file representing the
                                         * channel used as the input
                                         * source. */
    Tcl_Obj *dataObjPtr;                /* If non-NULL, data object to use as
                                         * input source. */
    const char *reqQuote;               /* Quoted string delimiter. */
    const char *reqSeparator;           /* Separator character. */
    const char *testSeparators;         /* Separator character. */
    const char *reqComment;             /* Comment character. */
    char separatorChar;                 /* Separator character. */
    char quoteChar;                     /* Quote character. */
    char commentChar;                   /* Comment character. */
    Tcl_Obj *emptyValueObjPtr;          /* If non-NULL, empty value. */
    int maxRows;                        /* Stop processing after this many
                                         * rows have been found. */
    const char **columnLabels;
    int nextLabel;
} ImportArgs;

static Blt_SwitchSpec importSwitches[] = 
{
    {BLT_SWITCH_LIST,   "-columnlabels",  "labelList", (char *)NULL,
        Blt_Offset(ImportArgs, columnLabels), 0},
    {BLT_SWITCH_STRING, "-comment",     "char", (char *)NULL,
        Blt_Offset(ImportArgs, reqComment), 0},
    {BLT_SWITCH_OBJ,    "-data",      "string", (char *)NULL,
        Blt_Offset(ImportArgs, dataObjPtr), 0, 0, NULL},
    {BLT_SWITCH_OBJ,    "-emptyvalue",      "string", (char *)NULL,
        Blt_Offset(ImportArgs, emptyValueObjPtr), 0},
    {BLT_SWITCH_OBJ,    "-encoding",  "string", (char *)NULL,
        Blt_Offset(ImportArgs, encodingObjPtr), 0, 0, NULL},
    {BLT_SWITCH_OBJ,    "-file",      "fileName", (char *)NULL,
        Blt_Offset(ImportArgs, fileObjPtr), 0},
    {BLT_SWITCH_INT_NNEG, "-maxrows", "numRows", (char *)NULL,
        Blt_Offset(ImportArgs, maxRows), 0},
    {BLT_SWITCH_STRING, "-possibleseparators", "string", (char *)NULL,
        Blt_Offset(ImportArgs, testSeparators), 0},
    {BLT_SWITCH_STRING, "-quote",     "char", (char *)NULL,
        Blt_Offset(ImportArgs, reqQuote), 0},
    {BLT_SWITCH_STRING, "-separator", "char", (char *)NULL,
        Blt_Offset(ImportArgs, reqSeparator), 0},
    {BLT_SWITCH_END}
};

/*
 * ExportArgs --
 */
typedef struct {
    BLT_TABLE_ITERATOR ri, ci;
    unsigned int flags;
    Tcl_Obj *fileObjPtr;
    Tcl_Channel channel;                /* If non-NULL, channel to write
                                         * output to. */
    Tcl_DString *dsPtr;
    int length;                         /* Length of dynamic string. */
    int count;                          /* # of fields in current record. */
    Tcl_Interp *interp;
    const char *reqQuote;               /* Quoted string delimiter. */
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
    {BLT_SWITCH_BITS_NOARG, "-rowlabels",  "", (char *)NULL,
        Blt_Offset(ExportArgs, flags), 0, EXPORT_ROWLABELS},
    {BLT_SWITCH_BITS_NOARG, "-columnlabels",  "", (char *)NULL,
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
 *      Free the storage associated with the -columns switch.
 *
 * Results:
 *      None.
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
 *      Convert a Tcl_Obj representing an offset in the table.
 *
 * Results:
 *      The return value is a standard TCL result.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
ColumnIterSwitchProc(
    ClientData clientData,              /* Flag indicating if the node is
                                         * considered before or after the
                                         * insertion position. */
    Tcl_Interp *interp,                 /* Interpreter to report results. */
    const char *switchName,             /* Not used. */
    Tcl_Obj *objPtr,                    /* String representation */
    char *record,                       /* Structure record */
    int offset,                         /* Offset to field in structure */
    int flags)                          /* Not used. */
{
    BLT_TABLE_ITERATOR *iterPtr = (BLT_TABLE_ITERATOR *)(record + offset);
    BLT_TABLE table;
    Tcl_Obj **objv;
    int objc;

    table = clientData;
    if (Tcl_ListObjGetElements(interp, objPtr, &objc, &objv) != TCL_OK) {
        return TCL_ERROR;
    }
    if (blt_table_iterate_columns_objv(interp, table, objc, objv, iterPtr)
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
 *      Free the storage associated with the -rows switch.
 *
 * Results:
 *      None.
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
 *      Convert a Tcl_Obj representing an offset in the table.
 *
 * Results:
 *      The return value is a standard TCL result.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
RowIterSwitchProc(
    ClientData clientData,              /* Flag indicating if the node is
                                         * considered before or after the
                                         * insertion position. */
    Tcl_Interp *interp,                 /* Interpreter to report results. */
    const char *switchName,             /* Not used. */
    Tcl_Obj *objPtr,                    /* String representation */
    char *record,                       /* Structure record */
    int offset,                         /* Offset to field in structure */
    int flags)                          /* Not used. */
{
    BLT_TABLE_ITERATOR *iterPtr = (BLT_TABLE_ITERATOR *)(record + offset);
    BLT_TABLE table;
    Tcl_Obj **objv;
    int objc;

    table = clientData;
    if (Tcl_ListObjGetElements(interp, objPtr, &objc, &objv) != TCL_OK) {
        return TCL_ERROR;
    }
    if (blt_table_iterate_rows_objv(interp, table, objc, objv, iterPtr)
        != TCL_OK) {
        return TCL_ERROR;
    }
    return TCL_OK;
}

static void
StartCsvRecord(ExportArgs *exportPtr)
{
    if (exportPtr->channel != NULL) {
        Tcl_DStringSetLength(exportPtr->dsPtr, 0);
        exportPtr->length = 0;
    }
    exportPtr->count = 0;
}

static int
EndCsvRecord(ExportArgs *exportPtr)
{
    Tcl_DStringAppend(exportPtr->dsPtr, "\n", 1);
    exportPtr->length++;
    if (exportPtr->channel != NULL) {
        int numWritten;
        char *line;

        line = Tcl_DStringValue(exportPtr->dsPtr);
        numWritten = Tcl_Write(exportPtr->channel, line, exportPtr->length);
        if (numWritten != exportPtr->length) {
            Tcl_AppendResult(exportPtr->interp, "can't write csv record: ",
                             Tcl_PosixError(exportPtr->interp), (char *)NULL);
            return TCL_ERROR;
        }
    }
    return TCL_OK;
}

static void
AppendCsvRecord(ExportArgs *exportPtr, const char *field, int length, 
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
            if ((*fp == '\n') || (*fp == exportPtr->separatorChar) || 
                (*fp == ' ') || (*fp == '\t')) {
                doQuote = TRUE;
            } else if (*fp == exportPtr->quoteChar) {
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
    if (exportPtr->count > 0) {
        Tcl_DStringAppend(exportPtr->dsPtr, &exportPtr->separatorChar, 1);
        exportPtr->length++;
    }
    length = length + extra + exportPtr->length;
    Tcl_DStringSetLength(exportPtr->dsPtr, length);
    p = Tcl_DStringValue(exportPtr->dsPtr) + exportPtr->length;
    exportPtr->length = length;
    if (field != NULL) {
        if (doQuote) {
            *p++ = exportPtr->quoteChar;
        }
        for (fp = field; *fp != '\0'; fp++) {
            if (*fp == exportPtr->quoteChar) {
                *p++ = exportPtr->quoteChar;
            }
            *p++ = *fp;
        }
        if (doQuote) {
            *p++ = exportPtr->quoteChar;
        }
    }
    exportPtr->count++;
}

static int
ExportCsvRows(BLT_TABLE table, ExportArgs *exportPtr)
{
    BLT_TABLE_ROW row;

    for (row = blt_table_first_tagged_row(&exportPtr->ri); row != NULL; 
         row = blt_table_next_tagged_row(&exportPtr->ri)) {
        BLT_TABLE_COLUMN col;
            
        StartCsvRecord(exportPtr);
        if (exportPtr->flags & EXPORT_ROWLABELS) {
            const char *field;

            field = blt_table_row_label(row);
            AppendCsvRecord(exportPtr, field, -1, TABLE_COLUMN_TYPE_STRING);
        }
        for (col = blt_table_first_tagged_column(&exportPtr->ci); col != NULL; 
             col = blt_table_next_tagged_column(&exportPtr->ci)) {
            const char *string;
            BLT_TABLE_COLUMN_TYPE type;
                
            type = blt_table_column_type(col);
            string = blt_table_get_string(table, row, col);
            AppendCsvRecord(exportPtr, string, -1, type);
        }
        if (EndCsvRecord(exportPtr) != TCL_OK) {
            return TCL_ERROR;
        }
    }
    return TCL_OK;
}

static int
ExportCsvColumns(ExportArgs *exportPtr)
{
    if (exportPtr->flags & EXPORT_COLUMNLABELS) {
        BLT_TABLE_COLUMN col;

        StartCsvRecord(exportPtr);
        if (exportPtr->flags & EXPORT_ROWLABELS) {
            AppendCsvRecord(exportPtr, "*BLT*", 5, TABLE_COLUMN_TYPE_STRING);
        }
        for (col = blt_table_first_tagged_column(&exportPtr->ci); col != NULL; 
             col = blt_table_next_tagged_column(&exportPtr->ci)) {
            AppendCsvRecord(exportPtr, blt_table_column_label(col), -1, 
                TABLE_COLUMN_TYPE_STRING);
        }
        return EndCsvRecord(exportPtr);
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
                goto error;     /* Can't open export file. */
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
IsEmpty(ImportArgs *importPtr, const char *field, size_t count)
{
    const char *value;
    int length;

    if (importPtr->emptyValueObjPtr == NULL) {
        return FALSE;                   /* No empty value defined. */
    }
    value = Tcl_GetStringFromObj(importPtr->emptyValueObjPtr, &length);
    if (count > 0) {
        return (strncmp(field, value, count) == 0); /* Matches empty value. */
    }
    return (length == 0);               /* Check if count==length==0 */
}

/* 
 * ImportGetLine -- 
 *
 *      Gets a single line from the input buffer or file.  We don't remove
 *      newlines.  The parser needs them to determine if we are really at
 *      the end of a row or in a quoted field.  So the resulting line
 *      always contains a new line unless an error occurs or we hit EOF.
 *
 */
static int
ImportGetLine(Tcl_Interp *interp, ImportArgs *importPtr, const char **bufferPtr,
              size_t *numBytesPtr)
{
    if (importPtr->channel != NULL) {
        int numChars;

        if (Tcl_Eof(importPtr->channel)) {
            *numBytesPtr = 0;
            return TCL_OK;
        }
        Tcl_DStringSetLength(&importPtr->currLine, 0);
        numChars = Tcl_Gets(importPtr->channel, &importPtr->currLine);
        if (numChars < 0) {
            if (Tcl_Eof(importPtr->channel)) {
                *numBytesPtr = 0;
                return TCL_OK;
            }
            *numBytesPtr = numChars;
            Tcl_AppendResult(interp, "error reading file: ", 
                             Tcl_PosixError(interp), (char *)NULL);
            return TCL_ERROR;
        }
        /* Put back the newline. */
        Tcl_DStringAppend(&importPtr->currLine, "\n", 1);
        *numBytesPtr = Tcl_DStringLength(&importPtr->currLine);
        *bufferPtr = Tcl_DStringValue(&importPtr->currLine);
    } else {
        const char *bp, *bend;
        ssize_t delta;

        for (bp = importPtr->next, bend = bp + importPtr->bytesLeft; bp < bend;
             bp++) {
            if (*bp == '\n') {
                bp++;                   /* Keep the newline. */
                break;
            }
        }
        /* Do bookkeeping on buffer pointer and size. */
        *bufferPtr = importPtr->next;
        delta = bp - importPtr->next;
        *numBytesPtr = delta;
        importPtr->bytesLeft -= delta;  /* Important to reduce bytes left
                                         * regardless of trailing
                                         * newline. */
        if (delta > 0) {
            if (*(bp-1) != '\n') {
                /* The last newline has been trimmed.  Append a newline.
                 * Don't change the data object's string
                 * representation. Copy the line and append the newline. */
                assert(*bp == '\0');
                Tcl_DStringSetLength(&importPtr->currLine, 0);
                Tcl_DStringAppend(&importPtr->currLine, importPtr->next, delta);
                Tcl_DStringAppend(&importPtr->currLine, "\n", 1);
                *numBytesPtr = Tcl_DStringLength(&importPtr->currLine);
                *bufferPtr = Tcl_DStringValue(&importPtr->currLine);
            } else {
                importPtr->next += delta;
            }
        }
    }
    return TCL_OK;
}

static int
GuessSeparator(Tcl_Interp *interp, int maxRows, ImportArgs *importPtr)
{
    int charCounts[10];
    int i, numSeparators;
    off_t pos;
    const char defSepTokens[] = { ",\t|;" };
    const char *sepTokens;

    sepTokens = (importPtr->testSeparators != NULL) ? 
        importPtr->testSeparators : defSepTokens;
    pos = 0;                            /* Suppress compiler warning. */
    if (importPtr->channel != NULL) {
        pos = Tcl_Tell(importPtr->channel);
    } 
    numSeparators = strlen(sepTokens);
    if (numSeparators > 10) {
        numSeparators = 10;
    }
    for (i = 0; i < numSeparators; i++) {
        charCounts[i] = 0;
    }
    for (i = 0; i < importPtr->maxRows; i++) {
        const char *bp, *bend;
        size_t numBytes;
        int result;
        
        result = ImportGetLine(interp, importPtr, &bp, &numBytes);
        if (result != TCL_OK) {
            return TCL_ERROR;           /* I/O Error. */
        }
        if (numBytes == 0) {
            break;                      /* EOF */
        }
        for (i = 0; i < numSeparators; i++) {
            for (bend = bp + numBytes; bp < bend; bp++) {
                if (*bp == sepTokens[i]) {
                    charCounts[i]++;
                }
            }
        }
    }
    if (importPtr->channel != NULL) {
        Tcl_Seek(importPtr->channel, pos, SEEK_SET);
    } else {
        importPtr->next = importPtr->buffer;
        importPtr->bytesLeft = importPtr->numBytes;
    }

    {
	int maxCount;

        maxCount = -1;
        for (i = 0; i < numSeparators; i++) {
            if (maxCount < charCounts[i]) {
	        maxCount = charCounts[i];
                importPtr->separatorChar = sepTokens[i];
            }
        }
    }
    return importPtr->separatorChar;
}

static const char *
GetNextLabel(ImportArgs *importPtr)
{
    const char *label;
    
    if (importPtr->columnLabels == NULL) {
        return NULL;
    }
    label = importPtr->columnLabels[importPtr->nextLabel];
    if (label != NULL) {
        return NULL;
    }
    importPtr->nextLabel++;
    return label;
}

static int
ImportCsv(Tcl_Interp *interp, BLT_TABLE table, ImportArgs *importPtr)
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
        const char *bp, *bend;
        size_t numBytes;

        result = ImportGetLine(interp, importPtr, &bp, &numBytes);
        if (result != TCL_OK) {
            goto error;                 /* I/O Error. */
        }
        if (numBytes == 0) {
            break;                      /* EOF */
        }
        bend = bp + numBytes;
        while ((bp < bend) && (isspace(*bp)) && (!IsSeparator(importPtr, *bp))){
            bp++;                       /* Skip leading spaces. */
        }
        if ((*bp == '\0') || (*bp == importPtr->commentChar)) {
            continue;                   /* Ignore blank or comment lines */
        }
        for (/*empty*/; bp < bend; bp++) {
            if ((IsSeparator(importPtr, *bp)) || (*bp == '\n')) {
                if (inQuotes) {
                    *fp++ = *bp;        /* Copy the separator or newline. */
                } else {
                    BLT_TABLE_COLUMN col;
                    char *last;

                    if ((isPath) && (IsSeparator(importPtr, *bp)) && 
                        (fp != field) && (*(fp - 1) != '\\')) {
                        *fp++ = *bp;    /* Copy the separator or newline. */
                        goto currLine;
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
                            goto currLine;  /* Ignore empty lines. */
                        }
                        if (blt_table_extend_rows(interp, table, 1, &row) 
                            != TCL_OK) {
                            goto error;
                        }
                        if ((importPtr->maxRows > 0) && 
                            (blt_table_num_rows(table) > importPtr->maxRows)) {
                            bp = bend;
                            goto currLine;
                        }
                    }
                    /* End of field. Append field to row. */
                    if (i >= blt_table_num_columns(table)) {
                        const char *label;
                        
                        if (blt_table_extend_columns(interp, table, 1, &col) 
                            != TCL_OK) {
                            goto error;
                        }
                        label = GetNextLabel(importPtr);
                        if ((label != NULL) && 
                            (blt_table_set_column_label(interp, table, col,
                                                        label) != TCL_OK)) {
                            goto error;
                        }
                    } else {
                        col = blt_table_column(table, i);
                    }
                    if (((last > field) || (isQuoted)) && 
                        (!IsEmpty(importPtr, field, last - field))) {
                        if (blt_table_set_string_rep(interp, table, row, col,
                                field, last - field) != TCL_OK) {             
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
            currLine:
                if ((importPtr->maxRows > 0) && 
                    (blt_table_num_rows(table) > importPtr->maxRows)) {
                    break;
                }
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
            } else if (*bp == importPtr->quoteChar ) {
                if (inQuotes) {
                    if (*(bp+1) == importPtr->quoteChar) {
                        *fp++ = importPtr->quoteChar;
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
                *fp++ = *bp;    /* Copy the character. */
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
                const char *label;

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
                label = GetNextLabel(importPtr);
                if ((label != NULL) &&
                    (blt_table_set_column_label(interp, table, col,
                                                label) != TCL_OK)) {
                    goto error;
                }
                if (((last > field) || (isQuoted)) && 
                    (!IsEmpty(importPtr, field, last - field))) {
                    if (blt_table_set_string(interp, table, row, col, field, 
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
#define MAX_LINES        20

    memset(&args, 0, sizeof(args));
    args.quoteChar   = '\"';
    args.separatorChar = ',';
    args.commentChar = '\0';
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
        args.next = args.buffer;
        args.bytesLeft = args.numBytes = numBytes;
        args.fileObjPtr = NULL;
        if ((args.reqSeparator == NULL) || (args.reqSeparator[0] == '\0')) {
            args.separatorChar = GuessSeparator(interp, MAX_LINES, &args);
        } else {
            args.separatorChar = args.reqSeparator[0];
        }
        Tcl_DStringInit(&args.currLine);
        result = ImportCsv(interp, table, &args);
        Tcl_DStringFree(&args.currLine);
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
        Tcl_DStringInit(&args.currLine);
        if ((args.reqSeparator == NULL) || (args.reqSeparator[0] == '\0')) {
            args.separatorChar = GuessSeparator(interp, MAX_LINES, &args);
        } else {
            args.separatorChar = args.reqSeparator[0];
        }
        result = ImportCsv(interp, table, &args);
        Tcl_DStringFree(&args.currLine);
        if (closeChannel) {
            Tcl_Close(interp, channel);
        }
    }
 error:
    Blt_FreeSwitches(importSwitches, (char *)&args, 0);
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
        "csv",                  /* Name of format. */
        ImportCsvProc,          /* Import procedure. */
        ExportCsvProc);         /* Export procedure. */

}

int 
blt_table_csv_safe_init(Tcl_Interp *interp)
{
    return blt_table_csv_init(interp);
}

#endif /* NO_DATATABLE */

