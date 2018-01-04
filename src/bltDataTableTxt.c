/* -*- mode: c; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 * bltDataTableTxt.c --
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
#include "config.h"
#include <tcl.h>
#include <bltSwitch.h>
#include <bltHash.h>
#include <bltDataTable.h>
#include <bltAlloc.h>

#ifdef HAVE_CTYPE_H
  #include <ctype.h>
#endif /* HAVE_CTYPE_H */

#ifdef HAVE_MEMORY_H
  #include <memory.h>
#endif /* HAVE_MEMORY_H */

DLLEXPORT extern Tcl_AppInitProc blt_table_txt_init;
DLLEXPORT extern Tcl_AppInitProc blt_table_txt_safe_init;

#define TRUE    1
#define FALSE   0

#define STATE_FIELD             1
#define STATE_SEPARATOR         0

#define EXPORT_ROWLABELS        (1<<0)
#define EXPORT_COLUMNLABELS     (1<<1)

/*
 * ImportSwitches --
 */
typedef struct {
    unsigned int flags;
    Tcl_Channel channel;                /* If non-NULL, channel to read
                                         * from. */
    char *buffer;                       /* Buffer to read data into. */
    int numBytes;                       /* # of bytes in the buffer. */
    Tcl_DString ds;                     /* Dynamic string used to read the
                                         * file line by line. */
    Tcl_Interp *interp;
    Blt_HashTable dataTable;
    Tcl_Obj *fileObjPtr;                /* Name of file representing the
                                         * channel used as the input
                                         * source. */
    Tcl_Obj *dataObjPtr;                /* If non-NULL, data object to use as
                                         * input source. */
    const char *quote;                  /* Quoted string delimiter. */
    const char *comment;                /* Comment character. */
    int maxRows;                        /* Stop processing after this many
                                         * rows have been found. */
} ImportSwitches;

static Blt_SwitchSpec importSwitches[] = 
{
    {BLT_SWITCH_STRING, "-comment",     "char", (char *)NULL,
        Blt_Offset(ImportSwitches, comment), 0},
    {BLT_SWITCH_OBJ,    "-data",      "string", (char *)NULL,
        Blt_Offset(ImportSwitches, dataObjPtr), 0, 0, NULL},
    {BLT_SWITCH_OBJ,    "-file",      "fileName", (char *)NULL,
        Blt_Offset(ImportSwitches, fileObjPtr), 0},
    {BLT_SWITCH_INT_NNEG, "-maxrows", "integer", (char *)NULL,
        Blt_Offset(ImportSwitches, maxRows), 0},
    {BLT_SWITCH_STRING, "-quote",     "char", (char *)NULL,
        Blt_Offset(ImportSwitches, quote), 0},
    {BLT_SWITCH_END}
};

/*
 * ExportSwitches --
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
    const char *quote;                  /* Quoted string delimiter. */
} ExportSwitches;

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
        Blt_Offset(ExportSwitches, ci),   0, 0, &columnIterSwitch},
    {BLT_SWITCH_OBJ,    "-file",      "fileName", (char *)NULL,
        Blt_Offset(ExportSwitches, fileObjPtr), 0},
    {BLT_SWITCH_BITS_NOARG, "-rowlabels",  "", (char *)NULL,
        Blt_Offset(ExportSwitches, flags), 0, EXPORT_ROWLABELS},
    {BLT_SWITCH_BITS_NOARG, "-columnlabels",  "", (char *)NULL,
        Blt_Offset(ExportSwitches, flags), 0, EXPORT_COLUMNLABELS},
    {BLT_SWITCH_STRING, "-quote",     "char", (char *)NULL,
        Blt_Offset(ExportSwitches, quote),   0},
    {BLT_SWITCH_CUSTOM, "-rows",      "rows", (char *)NULL,
        Blt_Offset(ExportSwitches, ri),   0, 0, &rowIterSwitch},
    {BLT_SWITCH_END}
};

static BLT_TABLE_IMPORT_PROC ImportProc;
static BLT_TABLE_EXPORT_PROC ExportProc;


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
ColumnIterFreeProc(ClientData clientData, char *record, 
                                int offset, int flags)
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
RowIterFreeProc(ClientData clientData, char *record, int offset, 
                             int flags)
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
StartRecord(ExportSwitches *exportPtr)
{
    if (exportPtr->channel != NULL) {
        Tcl_DStringSetLength(exportPtr->dsPtr, 0);
        exportPtr->length = 0;
    }
    exportPtr->count = 0;
}

static int
EndRecord(ExportSwitches *exportPtr)
{
    Tcl_DStringAppend(exportPtr->dsPtr, "\n", 1);
    exportPtr->length++;
    if (exportPtr->channel != NULL) {
        int numWritten;
        char *line;

        line = Tcl_DStringValue(exportPtr->dsPtr);
        numWritten = Tcl_Write(exportPtr->channel, line, exportPtr->length);
        if (numWritten != exportPtr->length) {
            Tcl_AppendResult(exportPtr->interp, "can't write txt record: ",
                Tcl_PosixError(exportPtr->interp), (char *)NULL);
            return TCL_ERROR;
        }
    }
    return TCL_OK;
}

static void
AppendRecord(ExportSwitches *exportPtr, const char *field, int length, 
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
            if (isspace(*fp)) {
                doQuote = TRUE;
            } else if (*fp == exportPtr->quote[0]) {
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
        Tcl_DStringAppend(exportPtr->dsPtr, "\t", 1);
        exportPtr->length++;
    }
    length = length + extra + exportPtr->length;
    Tcl_DStringSetLength(exportPtr->dsPtr, length);
    p = Tcl_DStringValue(exportPtr->dsPtr) + exportPtr->length;
    exportPtr->length = length;
    if (field != NULL) {
        if (doQuote) {
            *p++ = exportPtr->quote[0];
        }
        for (fp = field; *fp != '\0'; fp++) {
            if (*fp == exportPtr->quote[0]) {
                *p++ = exportPtr->quote[0];
            }
            *p++ = *fp;
        }
        if (doQuote) {
            *p++ = exportPtr->quote[0];
        }
    }
    exportPtr->count++;
}

static int
ExportRows(BLT_TABLE table, ExportSwitches *exportPtr)
{
    BLT_TABLE_ROW row;

    for (row = blt_table_first_tagged_row(&exportPtr->ri); row != NULL; 
         row = blt_table_next_tagged_row(&exportPtr->ri)) {
        BLT_TABLE_COLUMN col;
            
        StartRecord(exportPtr);
        if (exportPtr->flags & EXPORT_ROWLABELS) {
            const char *field;

            field = blt_table_row_label(row);
            AppendRecord(exportPtr, field, -1, TABLE_COLUMN_TYPE_STRING);
        }
        for (col = blt_table_first_tagged_column(&exportPtr->ci); col != NULL; 
             col = blt_table_next_tagged_column(&exportPtr->ci)) {
            const char *string;
            BLT_TABLE_COLUMN_TYPE type;
                
            type = blt_table_column_type(col);
            string = blt_table_get_string(table, row, col);
            AppendRecord(exportPtr, string, -1, type);
        }
        if (EndRecord(exportPtr) != TCL_OK) {
            return TCL_ERROR;
        }
    }
    return TCL_OK;
}

static int
ExportColumns(ExportSwitches *exportPtr)
{
    if (exportPtr->flags & EXPORT_COLUMNLABELS) {
        BLT_TABLE_COLUMN col;

        StartRecord(exportPtr);
        if (exportPtr->flags & EXPORT_ROWLABELS) {
            AppendRecord(exportPtr, "*BLT*", 5, TABLE_COLUMN_TYPE_STRING);
        }
        for (col = blt_table_first_tagged_column(&exportPtr->ci); col != NULL; 
             col = blt_table_next_tagged_column(&exportPtr->ci)) {
            AppendRecord(exportPtr, blt_table_column_label(col), -1, 
                TABLE_COLUMN_TYPE_STRING);
        }
        return EndRecord(exportPtr);
    }
    return TCL_OK;
}

/* 
 * $table export -file fileName ?switches...?
 * $table export -data ?switches...?
 */
static int
ExportProc(BLT_TABLE table, Tcl_Interp *interp, int objc, Tcl_Obj *const *objv)
{
    ExportSwitches switches;
    Tcl_Channel channel;
    Tcl_DString ds;
    int closeChannel;
    int result;

    closeChannel = FALSE;
    channel = NULL;

    Tcl_DStringInit(&ds);
    memset(&switches, 0, sizeof(switches));
    switches.quote = Blt_AssertStrdup("\"");
    rowIterSwitch.clientData = table;
    columnIterSwitch.clientData = table;
    blt_table_iterate_all_rows(table, &switches.ri);
    blt_table_iterate_all_columns(table, &switches.ci);
    if (Blt_ParseSwitches(interp, exportSwitches, objc - 3, objv + 3, &switches,
        BLT_SWITCH_DEFAULTS) < 0) {
        return TCL_ERROR;
    }
    result = TCL_ERROR;
    if (switches.fileObjPtr != NULL) {
        const char *fileName;

        closeChannel = TRUE;
        fileName = Tcl_GetString(switches.fileObjPtr);
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
    switches.interp = interp;
    switches.dsPtr = &ds;
    switches.channel = channel;
    result = ExportColumns(&switches); 
    if (result == TCL_OK) {
        result = ExportRows(table, &switches);
    }
    if ((switches.channel == NULL) && (result == TCL_OK)) {
        Tcl_DStringResult(interp, &ds);
    } 
 error:
    Tcl_DStringFree(&ds);
    if (closeChannel) {
        Tcl_Close(interp, channel);
    }
    Blt_FreeSwitches(exportSwitches, (char *)&switches, 0);
    return result;
}

/* 
 * ImportGetLine -- 
 *
 *      Get a single line from the input buffer or file.  We don't remove
 *      newlines.  The parser needs them to determine if we are really 
 *      at the end of a row or in a quoted field.  So the resulting line
 *      always contains a new line unless an error occurs or we hit EOF.
 *
 */
static int
ImportGetLine(Tcl_Interp *interp, ImportSwitches *importPtr, char **bufferPtr,
                int *numBytesPtr)
{
    if (importPtr->channel != NULL) {
        int numChars;

        if (Tcl_Eof(importPtr->channel)) {
            *numBytesPtr = 0;
            return TCL_OK;
        }
        Tcl_DStringSetLength(&importPtr->ds, 0);
        numChars = Tcl_Gets(importPtr->channel, &importPtr->ds);
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
        Tcl_DStringAppend(&importPtr->ds, "\n", 1);
        *numBytesPtr = Tcl_DStringLength(&importPtr->ds);
        *bufferPtr = Tcl_DStringValue(&importPtr->ds);
    } else {
        char *bp, *bend;
        int numBytes;

        for (bp = importPtr->buffer, bend = bp + importPtr->numBytes; bp < bend;
             bp++) {
            if (*bp == '\n') {
                bp++;                   /* Keep the newline. */
                break;
            }
        }
        /* Do bookkeeping on buffer pointer and size. */
        *bufferPtr = importPtr->buffer;
        numBytes = bp - importPtr->buffer;
        *numBytesPtr = numBytes;
        importPtr->numBytes -= numBytes; /* Important to reduce bytes left
                                          * regardless of trailing newline. */
        if (numBytes > 0) {
            if (*(bp-1) != '\n') {
                /* The last newline has been trimmed.  Append a newline.
                 * Don't change the data object's string representation. Copy
                 * the line and append the newline. */
                assert(*bp == '\0');
                Tcl_DStringSetLength(&importPtr->ds, 0);
                Tcl_DStringAppend(&importPtr->ds, importPtr->buffer, numBytes);
                Tcl_DStringAppend(&importPtr->ds, "\n", 1);
                *numBytesPtr = Tcl_DStringLength(&importPtr->ds);
                *bufferPtr = Tcl_DStringValue(&importPtr->ds);
            } else {
                importPtr->buffer += numBytes;
            }
        }
    }
    return TCL_OK;
}

static int
Import(Tcl_Interp *interp, BLT_TABLE table, ImportSwitches *importPtr)
{
    Tcl_DString ds;
    char *fp, *field;
    int fieldSize;
    int inQuotes, state;
    int result;
    long i;
    BLT_TABLE_ROW row;
    BLT_TABLE_COLUMN col;
    const char quote = importPtr->quote[0];
    const char comment = importPtr->comment[0];

    result = TCL_ERROR;
    inQuotes = FALSE;
    row = NULL;
    i = 0;
    state = STATE_SEPARATOR;
    Tcl_DStringInit(&ds);
    fieldSize = 128;
    Tcl_DStringSetLength(&ds, fieldSize + 1);
    fp = field = Tcl_DStringValue(&ds);
    for (;;) {
        char *bp, *bend;
        int numBytes;
        
        result = ImportGetLine(interp, importPtr, &bp, &numBytes);
        if (result != TCL_OK) {
            goto error;                 /* I/O Error. */
        }
        if (numBytes == 0) {
            break;                      /* EOF */
        }
        bend = bp + numBytes;
        while ((bp < bend) && (isspace(*bp))) {
            bp++;                       /* Skip leading spaces. */
        }
        if ((*bp == '\0') || (*bp == comment)) {
            continue;                   /* Ignore blank or comment lines */
        }
        for (/*empty*/; bp < bend; bp++) {
            if (*bp == '\\') {
                bp++;
                if (bp == bend) {
                    goto done;          /* Ignore trailing escape. */
                }
                goto collect;
            }
            if (isspace(*bp)) {
                if (inQuotes) {
                    goto collect;
                } else {
                    /* Transition from value to whitespace. */
                    if ((state == STATE_FIELD) && (fp > field)) {
                        *fp++ = '\0';
                        if (row == NULL) {
                            if (blt_table_extend_rows(interp, table, 1, &row)
                                != TCL_OK) {
                                goto error;
                            }
                            if ((importPtr->maxRows > 0) && 
                                (blt_table_num_rows(table)>importPtr->maxRows)){
                                bp = bend;
                                goto done;
                            }
                        } 
                        if (i >= blt_table_num_columns(table)) {
                            if (blt_table_extend_columns(interp, table, 1, &col) 
                                != TCL_OK) {
                                goto error;
                            }
                        } else {
                            col = blt_table_column(table, i);
                        }
                        i++;
                        if (blt_table_set_string_rep(interp, table, row, col,
                                field, fp - field) != TCL_OK) {
                            goto error;
                        }
                    }
                    /* Transition from value to newline. */
                    if (*bp == '\n') {
                        state = STATE_SEPARATOR;
                        if (fp == field) {
                            continue;   /* Ignore empty lines. */
                        } 
                        row = NULL;     /* Indicate end of record. */
                        i = 0;
                    } 
                }
                state = STATE_SEPARATOR;
            } else if (*bp == quote) {
                inQuotes ^= 1;
                continue;
            } else {
            collect:
                /* Transition from whitespace to value. */
                if (state == STATE_SEPARATOR) {
                    state = STATE_FIELD;
                    fp = field;
                }
                *fp++ = *bp;
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
        }
    done:
        if (numBytes < 1) {
            /* 
             * We're reached the end of input. But there may not have been a
             * final newline to trigger the final append. So check if a last
             * field is still needs appending the the last row.
             */
            if (fp > field) {
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
                if (blt_table_set_string_rep(interp, table, row, col, field,
                        fp - field) != TCL_OK) {
                    goto error;
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
ImportProc(BLT_TABLE table, Tcl_Interp *interp, int objc, Tcl_Obj *const *objv)
{
    int result;
    ImportSwitches switches;

    memset(&switches, 0, sizeof(switches));
    switches.quote      = Blt_AssertStrdup("\"");
    switches.comment    = Blt_AssertStrdup("#");
    Blt_InitHashTable(&switches.dataTable, BLT_STRING_KEYS);
    if (Blt_ParseSwitches(interp, importSwitches, objc - 3 , objv + 3, 
        &switches, BLT_SWITCH_DEFAULTS) < 0) {
        return TCL_ERROR;
    }
    result = TCL_ERROR;
    if ((switches.dataObjPtr != NULL) && (switches.fileObjPtr != NULL)) {
        Tcl_AppendResult(interp, "can't set both -file and -data switches.",
                         (char *)NULL);
        goto error;
    }
    if (switches.dataObjPtr != NULL) {
        int numBytes;

        switches.channel = NULL;
        switches.buffer = Tcl_GetStringFromObj(switches.dataObjPtr, &numBytes);
        switches.numBytes = numBytes;
        switches.fileObjPtr = NULL;
        Tcl_DStringInit(&switches.ds);
        result = Import(interp, table, &switches);
        Tcl_DStringFree(&switches.ds);
    } else {
        int closeChannel;
        Tcl_Channel channel;
        const char *fileName;

        closeChannel = TRUE;
        if (switches.fileObjPtr == NULL) {
            fileName = "out.txt";
        } else {
            fileName = Tcl_GetString(switches.fileObjPtr);
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
        switches.channel = channel;
        Tcl_DStringInit(&switches.ds);
        result = Import(interp, table, &switches);
        Tcl_DStringFree(&switches.ds);
        if (closeChannel) {
            Tcl_Close(interp, channel);
        }
    }
 error:
    Blt_FreeSwitches(importSwitches, (char *)&switches, 0);
    Blt_DeleteHashTable(&switches.dataTable);
    return result;
}
    
int 
blt_table_txt_init(Tcl_Interp *interp)
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
#endif    
    if (Tcl_PkgRequire(interp, "blt_tcl", BLT_VERSION, PKG_EXACT) == NULL) {
        return TCL_ERROR;
    }
    if (Tcl_PkgProvide(interp, "blt_datatable_txt", BLT_VERSION) != TCL_OK) { 
        return TCL_ERROR;
    }
    return blt_table_register_format(interp,
        "txt",                  /* Name of format. */
        ImportProc,             /* Import procedure. */
        ExportProc);            /* Export procedure. */

}

int 
blt_table_txt_safe_init(Tcl_Interp *interp)
{
    return blt_table_txt_init(interp);
}

#endif /* NO_DATATABLE */

