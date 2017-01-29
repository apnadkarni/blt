/* -*- mode: c; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 * bltCsv.c --
 *
 * This module implements a CSV reader procedure for the BLT toolkit.
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

#define BUILD_BLT_TCL_PROCS 1
#include "bltInt.h"

#ifdef HAVE_CTYPE_H
  #include <ctype.h>
#endif /* HAVE_CTYPE_H */

#ifdef HAVE_STRING_H
  #include <string.h>
#endif /* HAVE_STRING_H */

#ifdef HAVE_STDLIB_H
  #include <stdlib.h>
#endif /* HAVE_STDLIB_H */

#include "bltInitCmd.h"

#define IsSeparator(importPtr, c) ((importPtr)->separatorChar == (c))

/*
 * ImportArgs --
 */
typedef struct {
    unsigned int flags;
    Tcl_Channel channel;                /* If non-NULL, channel to read
                                         * from. */
    Tcl_Obj *encodingObjPtr;
    const char *buffer;                 /* Buffer to read data into. */
    ssize_t numBytes;                   /* # of bytes in the buffer. */
    const char *next;
    ssize_t bytesLeft;
    Tcl_DString nextLine;               /* Dynamic string used to read the
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

static Blt_SwitchSpec parseSwitches[] = 
{
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

static Blt_SwitchSpec guessSwitches[] = 
{
    {BLT_SWITCH_STRING, "-comment",     "char", (char *)NULL,
        Blt_Offset(ImportArgs, reqComment), 0},
    {BLT_SWITCH_OBJ,    "-data",      "string", (char *)NULL,
        Blt_Offset(ImportArgs, dataObjPtr), 0, 0, NULL},
    {BLT_SWITCH_OBJ,    "-encoding",  "string", (char *)NULL,
        Blt_Offset(ImportArgs, encodingObjPtr), 0, 0, NULL},
    {BLT_SWITCH_OBJ,    "-file",      "fileName", (char *)NULL,
        Blt_Offset(ImportArgs, fileObjPtr), 0},
    {BLT_SWITCH_INT_NNEG, "-maxrows", "numRows", (char *)NULL,
        Blt_Offset(ImportArgs, maxRows), 0},
    {BLT_SWITCH_STRING, "-possibleseparators", "string", (char *)NULL,
        Blt_Offset(ImportArgs, testSeparators), 0},
    {BLT_SWITCH_END}
};

/* 
 * GetLine -- 
 *
 *      Get a single line from the input buffer or file.  We don't remove
 *      newlines.  The parser needs them to determine if we are really at
 *      the end of a row or in a quoted field.  So the resulting line
 *      always contains a new line unless an error occurs or we hit EOF.
 *
 */
static int
GetLine(Tcl_Interp *interp, ImportArgs *importPtr, const char **bufferPtr,
        size_t *numBytesPtr)
{
    if (importPtr->channel != NULL) {
        int numChars;

        if (Tcl_Eof(importPtr->channel)) {
            *numBytesPtr = 0;
            return TCL_OK;
        }
        Tcl_DStringSetLength(&importPtr->nextLine, 0);
        numChars = Tcl_Gets(importPtr->channel, &importPtr->nextLine);
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
        Tcl_DStringAppend(&importPtr->nextLine, "\n", 1);
        *numBytesPtr = Tcl_DStringLength(&importPtr->nextLine);
        *bufferPtr = Tcl_DStringValue(&importPtr->nextLine);
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
                Tcl_DStringSetLength(&importPtr->nextLine, 0);
                Tcl_DStringAppend(&importPtr->nextLine, importPtr->next, delta);
                Tcl_DStringAppend(&importPtr->nextLine, "\n", 1);
                *numBytesPtr = Tcl_DStringLength(&importPtr->nextLine);
                *bufferPtr = Tcl_DStringValue(&importPtr->nextLine);
            } else {
                importPtr->next += delta;
            }
        }
    }
    return TCL_OK;
}

/* 
 * IsEmpty -- 
 *
 */
static int
IsEmpty(ImportArgs *importPtr, const char *field, size_t fieldLength)
{
    int emptyLength;
    const char *value;

    if (importPtr->emptyValueObjPtr == NULL) {
        return FALSE;                   /* No empty value defined. */
    }
    value = Tcl_GetStringFromObj(importPtr->emptyValueObjPtr, &emptyLength);
    if (fieldLength == 0) {
        return (fieldLength == emptyLength); /* Check if field==empty==0 */
    }
    if (emptyLength != fieldLength) {
        return 0;
    }
    return (strncmp(field, value, fieldLength) == 0); /* Matches empty value. */
}


static int
ParseCsv(Tcl_Interp *interp, Tcl_Obj *listObjPtr, ImportArgs *importPtr)
{
    Tcl_DString ds;
    char *fp, *field;
    int fieldSize;
    int inQuotes, isQuoted, isPath;
    int result;
    int numRows;
    Tcl_Obj *objPtr, *recordObjPtr;

    result = TCL_ERROR;
    isPath = isQuoted = inQuotes = FALSE;
    recordObjPtr = NULL;
    Tcl_DStringInit(&ds);
    fieldSize = 128;
    numRows = 0;
    Tcl_DStringSetLength(&ds, fieldSize + 1);
    fp = field = Tcl_DStringValue(&ds);
    for (;;) {
        const char *bp, *bend;
        size_t numBytes;

        result = GetLine(interp, importPtr, &bp, &numBytes);
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
                    char *last;

                    if ((isPath) && (IsSeparator(importPtr, *bp)) && 
                        (fp != field) && (*(fp - 1) != '\\')) {
                        *fp++ = *bp;    /* Copy the separator or newline. */
                        goto nextLine;
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
                    if (recordObjPtr == NULL) {
                        if ((*bp == '\n') &&  (fp == field)) {
                            goto nextLine;  /* Ignore empty lines. */
                        }
                        recordObjPtr = Tcl_NewListObj(0, (Tcl_Obj **)NULL);
                        numRows++;
                        if ((importPtr->maxRows > 0) && 
                            (numRows > importPtr->maxRows)) {
                            bp = bend;
                            goto nextLine;
                        }
                    }
                    if (IsEmpty(importPtr, field, last - field)) {
                        objPtr = Tcl_NewStringObj("", -1);
                    } else {
                        objPtr = Tcl_NewStringObj(field, last - field);
                    }
                    /* End of field. Append field to record. */
                    Tcl_ListObjAppendElement(interp, recordObjPtr, objPtr);
                    if (*bp == '\n') {
                        /* On newlines append the record to the list. */
                        Tcl_ListObjAppendElement(interp, listObjPtr, 
                                                 recordObjPtr);
                        recordObjPtr = NULL;
                    }
                    fp = field;
                    isPath = isQuoted = FALSE;
                }
            nextLine:
                if ((importPtr->maxRows > 0) && (numRows > importPtr->maxRows)){
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
        if ((numBytes < 1) && ((importPtr->maxRows == 0) ||
                               (numRows <= importPtr->maxRows))) {
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
                if (((last > field) || (isQuoted)) && 
                    (!IsEmpty(importPtr, field, last - field))) {
                    Tcl_Obj *objPtr;

                    objPtr = Tcl_NewStringObj(field, last - field);
                    if (recordObjPtr == NULL) {
                        recordObjPtr = Tcl_NewListObj(0, (Tcl_Obj **)NULL);
                    } 
                    Tcl_ListObjAppendElement(interp, recordObjPtr, objPtr);
                }
            }    
            if (recordObjPtr != NULL) {
                Tcl_ListObjAppendElement(interp, listObjPtr, recordObjPtr);
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
GuessSeparator(Tcl_Interp *interp, int maxRows, ImportArgs *importPtr,
               Tcl_Obj *listObjPtr)
{
    int charCounts[10];
    int i, numSeparators, numRows;
    off_t pos;
    const char defSepTokens[] = { ",\t|;"};
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
    if (numSeparators == 0) {
        return '.';
    }
    for (i = 0; i < numSeparators; i++) {
        charCounts[i] = 0;
    }
    numRows = 0;
    for (;;) {
        const char *line, *bp, *bend;
        size_t numBytes;
        int result;

        result = GetLine(interp, importPtr, &line, &numBytes);
        if (result != TCL_OK) {
            return TCL_ERROR;           /* I/O Error. */
        }
        if (numBytes == 0) {
            break;                      /* EOF */
        }
        if (*line == '\n') {
            continue;                   /* Skip blank lines. */
        }
        numRows++;
        if (numRows > importPtr->maxRows) {
            break;
        }
        for (i = 0; i < numSeparators; i++) {
            for (bp = line, bend = bp + numBytes; bp < bend; bp++) {
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
    if (listObjPtr != NULL) {
        for (i = 0; i < numSeparators; i++) {
            Tcl_Obj *objPtr;
            char string[3];
            
            sprintf(string, "%c", sepTokens[i]);
            objPtr = Tcl_NewStringObj(string, -1);
            Tcl_ListObjAppendElement(interp, listObjPtr, objPtr);
            objPtr = Tcl_NewIntObj(charCounts[i]);
            Tcl_ListObjAppendElement(interp, listObjPtr, objPtr);
        }
    }
    return importPtr->separatorChar;
}

static int
CsvParseOp(ClientData clientData, Tcl_Interp *interp, int objc,
           Tcl_Obj *const *objv)
{
    int result;
    ImportArgs args;
    Tcl_Obj *listObjPtr;
    
    memset(&args, 0, sizeof(args));
    args.quoteChar   = '\"';
    args.separatorChar = ',';
    args.commentChar = '\0';
    if (Blt_ParseSwitches(interp, parseSwitches, objc - 2 , objv + 2, 
        &args, BLT_SWITCH_DEFAULTS) < 0) {
        return TCL_ERROR;
    }
    result = TCL_ERROR;
    listObjPtr = Tcl_NewListObj(0, (Tcl_Obj **)NULL);
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
            args.separatorChar = GuessSeparator(interp, 20, &args, NULL);
        } else {
            args.separatorChar = args.reqSeparator[0];
        }
        Tcl_DStringInit(&args.nextLine);
        result = ParseCsv(interp, listObjPtr, &args);
        Tcl_DStringFree(&args.nextLine);
    } else if (args.fileObjPtr != NULL) {
        int closeChannel;
        Tcl_Channel channel;
        const char *fileName;

        closeChannel = TRUE;
        fileName = Tcl_GetString(args.fileObjPtr);
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
        Tcl_DStringInit(&args.nextLine);
        if ((args.reqSeparator == NULL) || (args.reqSeparator[0] == '\0')) {
            args.separatorChar = GuessSeparator(interp, 20, &args, NULL);
        } else {
            args.separatorChar = args.reqSeparator[0];
        }
        result = ParseCsv(interp, listObjPtr, &args);
        Tcl_DStringFree(&args.nextLine);
        if (closeChannel) {
            Tcl_Close(interp, channel);
        }
    }
 error:
    Blt_FreeSwitches(parseSwitches, (char *)&args, 0);
    if (result == TCL_OK) {
        Tcl_SetObjResult(interp, listObjPtr);
    } else {
        Tcl_DecrRefCount(listObjPtr);
    }
    return result;
}

static int
CsvGuessOp(ClientData clientData, Tcl_Interp *interp, int objc,
           Tcl_Obj *const *objv)
{
    int result;
    ImportArgs args;
    Tcl_DString ds;
    Tcl_Obj *listObjPtr;
    
    memset(&args, 0, sizeof(args));
    args.quoteChar   = '\"';
    args.separatorChar = ',';
    args.commentChar = '\0';
    args.maxRows = 20;
    if (Blt_ParseSwitches(interp, guessSwitches, objc - 2 , objv + 2, 
        &args, BLT_SWITCH_DEFAULTS) < 0) {
        return TCL_ERROR;
    }
    result = TCL_OK;
    listObjPtr = Tcl_NewListObj(0, (Tcl_Obj **)NULL);
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
    Tcl_DStringInit(&ds);
    if (args.dataObjPtr != NULL) {
        int numBytes;

        args.channel = NULL;
        args.buffer = Tcl_GetStringFromObj(args.dataObjPtr, &numBytes);
        args.next = args.buffer;
        args.bytesLeft = args.numBytes = numBytes;
        args.fileObjPtr = NULL;
        GuessSeparator(interp, args.maxRows, &args, listObjPtr);
    } else if (args.fileObjPtr != NULL) {
        int closeChannel;
        Tcl_Channel channel;
        const char *fileName;

        closeChannel = TRUE;
        fileName = Tcl_GetString(args.fileObjPtr);
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
        Tcl_DStringInit(&args.nextLine);
        GuessSeparator(interp, args.maxRows, &args, listObjPtr);
        Tcl_DStringFree(&args.nextLine);
        if (closeChannel) {
            Tcl_Close(interp, channel);
        }
    }
 error:
    Blt_FreeSwitches(guessSwitches, (char *)&args, 0);
    Tcl_DStringFree(&ds);
    if (result == TCL_OK) {
        Tcl_SetObjResult(interp, listObjPtr);
    } else {
        Tcl_DecrRefCount(listObjPtr);
    }
    return result;
}

/*
 *---------------------------------------------------------------------------
 *
 * CsvObjCmd --
 *
 *---------------------------------------------------------------------------
 */
static Blt_OpSpec csvCmdOps[] =
{
    {"guess",  1, CsvGuessOp,    2, 0, "?switches?",},
    {"parse",  1, CsvParseOp,    2, 0, "?switches?",},
};

static int numCmdOps = sizeof(csvCmdOps) / sizeof(Blt_OpSpec);

/*ARGSUSED*/
static int
CsvObjCmd(ClientData clientData, Tcl_Interp *interp, int objc,
           Tcl_Obj *const *objv)
{
    Tcl_ObjCmdProc *proc;

    proc = Blt_GetOpFromObj(interp, numCmdOps, csvCmdOps, BLT_OP_ARG1, objc, 
        objv, 0);
    if (proc == NULL) {
        return TCL_ERROR;
    }
    return (*proc) (clientData, interp, objc, objv);
}

int
Blt_CsvCmdInitProc(Tcl_Interp *interp)
{
    static Blt_CmdSpec cmdSpec = {"csv", CsvObjCmd,};

    return Blt_InitCmd(interp, "::blt", &cmdSpec);
}
