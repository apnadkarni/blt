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

#include "bltInitCmd.h"

static int
ParseCsvChannel(Tcl_Interp *interp, Tcl_Channel channel)
{
    int inQuotes, isQuoted, isPath;
    char *fp, *field;
    Tcl_DString ds;
    Tcl_Obj *listObjPtr, *recordObjPtr;
    int fieldSize;

    isPath = isQuoted = inQuotes = FALSE;
    listObjPtr = Tcl_NewListObj(0, (Tcl_Obj **)NULL);
    recordObjPtr = NULL;

    Tcl_DStringInit(&ds);
    fieldSize = 128;
    Tcl_DStringSetLength(&ds, fieldSize + 1);
    fp = field = Tcl_DStringValue(&ds);
    for (;;) {
        char *bp, *bend;
#define BUFFSIZE        8191
        char buffer[BUFFSIZE+1];
        int numBytes;

        numBytes = Tcl_Read(channel, buffer, sizeof(char) * BUFFSIZE);
        for (bp = buffer, bend = bp + numBytes; bp < bend; bp++) {
            switch (*bp) {
            case '\t':
            case ' ':
                /* 
                 * Add whitespace only if it's not leading or we're inside
                 * of quotes or a path.
                 */
                if ((fp != field) || (inQuotes) || (isPath)) {
                    *fp++ = *bp; 
                }
                break;

            case '\\':
                /* 
                 * Handle special case CSV files that allow unquoted paths.
                 * Example: ...,\this\path " should\have been\quoted\,...
                 */
                if (fp == field) {
                    isPath = TRUE; 
                }
                *fp++ = *bp;
                break;

            case '"':
                if (inQuotes) {
                    if (*(bp+1) == '"') {
                        *fp++ = '"';
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
                break;

            case ',':
            case '\n':
                if (inQuotes) {
                    *fp++ = *bp;        /* Copy the comma or newline. */
                } else {
                    char *last;
                    Tcl_Obj *objPtr;

                    if ((isPath) && (*bp == ',') && (fp != field) && 
                        (*(fp - 1) != '\\')) {
                        *fp++ = *bp;    /* Copy the comma or newline. */
                        break;
                    }    
                    /* "last" points to the character after the last
                     * character in the field. */
                    last = fp;  

                    /* Remove trailing spaces only if the field wasn't
                     * quoted. */
                    if ((!isQuoted) && (!isPath)) {
                        while ((last > field) && (isspace(*(last - 1)))) {
                            last--;
                        }
                    }
                    if (recordObjPtr == NULL) {
                        if (*bp == '\n') {
                            break;      /* Ignore empty lines. */
                        }
                        recordObjPtr = Tcl_NewListObj(0, (Tcl_Obj **)NULL);
                    }
                    /* End of field. Append field to record. */
                    objPtr = Tcl_NewStringObj(field, last - field);
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
                break;

            default:
                *fp++ = *bp;            /* Copy the character. */
            }
            if ((fp - field) >= fieldSize) {
                int offset;

                /* 
                 * We've exceeded the current maximum size of the field.
                 * Double the size of the field, but make sure to reset the
                 * pointers (fp and field) to the (possibly) new memory
                 * location.
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
             * We're reached the end of input. But there may not have been
             * a final newline to trigger the final appends. So check if 1)
             * a last field is still needs appending the the last record
             * and if 2) a last record is still needs appending to the
             * list.
             */
            if (fp != field) {
                char *last;

                last = fp;
                /* Remove trailing spaces */
                while (isspace(*(last - 1))) {
                    last--;
                }
                if (last > field) {
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
    Tcl_DStringFree(&ds);
    Tcl_SetObjResult(interp, listObjPtr);
    return TCL_OK;
}

static int
ParseCsvData(Tcl_Interp *interp, Tcl_Obj *objPtr)
{
    int inQuotes, isQuoted, isPath;
    char *fp, *field;
    Tcl_DString ds;
    Tcl_Obj *listObjPtr, *recordObjPtr;
    int fieldSize;

    isPath = isQuoted = inQuotes = FALSE;
    listObjPtr = Tcl_NewListObj(0, (Tcl_Obj **)NULL);
    recordObjPtr = NULL;

    Tcl_DStringInit(&ds);
    fieldSize = 128;
    Tcl_DStringSetLength(&ds, fieldSize + 1);
    fp = field = Tcl_DStringValue(&ds);
    {
        char *bp, *bend;
        char *buffer;
        int numBytes;

        buffer = Tcl_GetStringFromObj(objPtr, &numBytes);
        for (bp = buffer, bend = bp + numBytes; bp < bend; bp++) {
            switch (*bp) {
            case '\t':
            case ' ':
                /* 
                 * Add whitespace only if it's not leading or we're inside
                 * of quotes or a path.
                 */
                if ((fp != field) || (inQuotes) || (isPath)) {
                    *fp++ = *bp; 
                }
                break;

            case '\\':
                if (fp == field) {
                    isPath = TRUE; 
                }
                *fp++ = *bp;
                break;

            case '"':
                if (inQuotes) {
                    if (*(bp+1) == '"') {
                        *fp++ = '"';
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
                break;

            case ',':
            case '\n':
                if (inQuotes) {
                    *fp++ = *bp;        /* Copy the comma or newline. */
                } else {
                    char *last;
                    Tcl_Obj *objPtr;

                    if ((isPath) && (*bp == ',') && (fp != field) && 
                        (*(fp - 1) != '\\')) {
                        *fp++ = *bp;    /* Copy the comma or newline. */
                        break;
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
                        if (*bp == '\n') {
                            break;      /* Ignore empty lines. */
                        }
                        recordObjPtr = Tcl_NewListObj(0, (Tcl_Obj **)NULL);
                    }
                    /* End of field. Append field to record. */
                    objPtr = Tcl_NewStringObj(field, last - field);
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
                break;

            default:
                *fp++ = *bp;            /* Copy the character. */
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

        /* 
         * We're reached the end of input. But there may not have been a
         * final newline to trigger the final appends. So check if 1) a
         * last field is still needs appending the the last record and if
         * 2) a last record is still needs appending to the list.
         */
        if (fp != field) {
            char *last;
            
            last = fp;
            /* Remove trailing spaces */
            while (isspace(*(last - 1))) {
                last--;
            }
            if (last > field) {
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
    }
    Tcl_DStringFree(&ds);
    Tcl_SetObjResult(interp, listObjPtr);
    return TCL_OK;
}

static int
ParseCsvFile(Tcl_Interp *interp, const char *fileName)
{
    int result;
    int closeChannel;
    Tcl_Channel channel;

    closeChannel = TRUE;
    if ((fileName[0] == '@') && (fileName[1] != '\0')) {
        int mode;
        
        channel = Tcl_GetChannel(interp, fileName+1, &mode);
        if (channel == NULL) {
            return TCL_ERROR;
        }
        if ((mode & TCL_READABLE) == 0) {
            Tcl_AppendResult(interp, "channel \"", fileName, 
                "\" not opened for reading", (char *)NULL);
            return TCL_ERROR;
        }
        closeChannel = FALSE;
    } else {
        channel = Tcl_OpenFileChannel(interp, fileName, "r", 0);
        if (channel == NULL) {
            return TCL_ERROR;           /* Can't open dump file. */
        }
    }
    result = ParseCsvChannel(interp, channel);
    if (closeChannel) {
        Tcl_Close(interp, channel);
    }
    return result;
}

static int
CsvCmd(ClientData clientData, Tcl_Interp *interp, int objc, 
       Tcl_Obj *const *objv)
{
    if (objc == 2) {
        return ParseCsvFile(interp, Tcl_GetString(objv[1]));
    }
    if (objc == 3) {
        char *string;

        string = Tcl_GetString(objv[1]);
        if (strcmp(string, "-data") == 0) {
            return  ParseCsvData(interp, objv[2]);
        } 
    }
    Tcl_AppendResult(interp, "wrong # args: should be \"", 
        Tcl_GetString(objv[0]), " ?fileName? ?-data dataString?", (char *)NULL);
    return TCL_ERROR;
}
    
int
Blt_CsvCmdInitProc(Tcl_Interp *interp)
{
    static Blt_CmdSpec cmdSpec = {"csv", CsvCmd,};

    return Blt_InitCmd(interp, "::blt", &cmdSpec);
}
