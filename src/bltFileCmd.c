/* -*- mode: c; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 * bltFileCmd.c --
 *
 * This module implements extra file commands for the BLT toolkit.
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

#ifndef NO_FILECMD

#ifdef HAVE_CTYPE_H
  #include <ctype.h>
#endif /* HAVE_CTYPE_H */

#ifdef HAVE_STRING_H
  #include <string.h>
#endif /* HAVE_STRING_H */

#include "bltOp.h"
#include "bltInitCmd.h"

#define BUFFSIZE        8191

/* open a file
 * calculate the CRC32 of the entire contents
 * return the CRC
 * if there is an error rdet the global variable Crcerror
 */

/* -------------------------------------------------------------------------- */

/* 
 * this is the CRC32 lookup table
 * thanks Gary S. Brown 
 * 64 lines of 4 values for a 256 dword table (1024 bytes)
 */
static unsigned long crc32[256] =
{                               /* CRC polynomial 0xedb88320 */
    0x00000000, 0x77073096, 0xEE0E612C, 0x990951BA, 0x076DC419, 0x706AF48F, 
    0xE963A535, 0x9E6495A3, 0x0EDB8832, 0x79DCB8A4, 0xE0D5E91E, 0x97D2D988,
    0x09B64C2B, 0x7EB17CBD, 0xE7B82D07, 0x90BF1D91, 0x1DB71064, 0x6AB020F2, 
    0xF3B97148, 0x84BE41DE, 0x1ADAD47D, 0x6DDDE4EB, 0xF4D4B551, 0x83D385C7,
    0x136C9856, 0x646BA8C0, 0xFD62F97A, 0x8A65C9EC, 0x14015C4F, 0x63066CD9, 
    0xFA0F3D63, 0x8D080DF5, 0x3B6E20C8, 0x4C69105E, 0xD56041E4, 0xA2677172,
    0x3C03E4D1, 0x4B04D447, 0xD20D85FD, 0xA50AB56B, 0x35B5A8FA, 0x42B2986C, 
    0xDBBBC9D6, 0xACBCF940, 0x32D86CE3, 0x45DF5C75, 0xDCD60DCF, 0xABD13D59,
    0x26D930AC, 0x51DE003A, 0xC8D75180, 0xBFD06116, 0x21B4F4B5, 0x56B3C423, 
    0xCFBA9599, 0xB8BDA50F, 0x2802B89E, 0x5F058808, 0xC60CD9B2, 0xB10BE924,
    0x2F6F7C87, 0x58684C11, 0xC1611DAB, 0xB6662D3D, 0x76DC4190, 0x01DB7106, 
    0x98D220BC, 0xEFD5102A, 0x71B18589, 0x06B6B51F, 0x9FBFE4A5, 0xE8B8D433,
    0x7807C9A2, 0x0F00F934, 0x9609A88E, 0xE10E9818, 0x7F6A0DBB, 0x086D3D2D, 
    0x91646C97, 0xE6635C01, 0x6B6B51F4, 0x1C6C6162, 0x856530D8, 0xF262004E,
    0x6C0695ED, 0x1B01A57B, 0x8208F4C1, 0xF50FC457, 0x65B0D9C6, 0x12B7E950, 
    0x8BBEB8EA, 0xFCB9887C, 0x62DD1DDF, 0x15DA2D49, 0x8CD37CF3, 0xFBD44C65,
    0x4DB26158, 0x3AB551CE, 0xA3BC0074, 0xD4BB30E2, 0x4ADFA541, 0x3DD895D7, 
    0xA4D1C46D, 0xD3D6F4FB, 0x4369E96A, 0x346ED9FC, 0xAD678846, 0xDA60B8D0, 
    0x44042D73, 0x33031DE5, 0xAA0A4C5F, 0xDD0D7CC9, 0x5005713C, 0x270241AA, 
    0xBE0B1010, 0xC90C2086, 0x5768B525, 0x206F85B3, 0xB966D409, 0xCE61E49F, 
    0x5EDEF90E, 0x29D9C998, 0xB0D09822, 0xC7D7A8B4, 0x59B33D17, 0x2EB40D81, 
    0xB7BD5C3B, 0xC0BA6CAD, 0xEDB88320, 0x9ABFB3B6, 0x03B6E20C, 0x74B1D29A,
    0xEAD54739, 0x9DD277AF, 0x04DB2615, 0x73DC1683, 0xE3630B12, 0x94643B84, 
    0x0D6D6A3E, 0x7A6A5AA8, 0xE40ECF0B, 0x9309FF9D, 0x0A00AE27, 0x7D079EB1,
    0xF00F9344, 0x8708A3D2, 0x1E01F268, 0x6906C2FE, 0xF762575D, 0x806567CB, 
    0x196C3671, 0x6E6B06E7, 0xFED41B76, 0x89D32BE0, 0x10DA7A5A, 0x67DD4ACC,
    0xF9B9DF6F, 0x8EBEEFF9, 0x17B7BE43, 0x60B08ED5, 0xD6D6A3E8, 0xA1D1937E, 
    0x38D8C2C4, 0x4FDFF252, 0xD1BB67F1, 0xA6BC5767, 0x3FB506DD, 0x48B2364B,
    0xD80D2BDA, 0xAF0A1B4C, 0x36034AF6, 0x41047A60, 0xDF60EFC3, 0xA867DF55, 
    0x316E8EEF, 0x4669BE79, 0xCB61B38C, 0xBC66831A, 0x256FD2A0, 0x5268E236,
    0xCC0C7795, 0xBB0B4703, 0x220216B9, 0x5505262F, 0xC5BA3BBE, 0xB2BD0B28, 
    0x2BB45A92, 0x5CB36A04, 0xC2D7FFA7, 0xB5D0CF31, 0x2CD99E8B, 0x5BDEAE1D,
    0x9B64C2B0, 0xEC63F226, 0x756AA39C, 0x026D930A, 0x9C0906A9, 0xEB0E363F, 
    0x72076785, 0x05005713, 0x95BF4A82, 0xE2B87A14, 0x7BB12BAE, 0x0CB61B38,
    0x92D28E9B, 0xE5D5BE0D, 0x7CDCEFB7, 0x0BDBDF21, 0x86D3D2D4, 0xF1D4E242, 
    0x68DDB3F8, 0x1FDA836E, 0x81BE16CD, 0xF6B9265B, 0x6FB077E1, 0x18B74777,
    0x88085AE6, 0xFF0F6A70, 0x66063BCA, 0x11010B5C, 0x8F659EFF, 0xF862AE69, 
    0x616BFFD3, 0x166CCF45, 0xA00AE278, 0xD70DD2EE, 0x4E048354, 0x3903B3C2,
    0xA7672661, 0xD06016F7, 0x4969474D, 0x3E6E77DB, 0xAED16A4A, 0xD9D65ADC, 
    0x40DF0B66, 0x37D83BF0, 0xA9BCAE53, 0xDEBB9EC5, 0x47B2CF7F, 0x30B5FFE9,
    0xBDBDF21C, 0xCABAC28A, 0x53B39330, 0x24B4A3A6, 0xBAD03605, 0xCDD70693, 
    0x54DE5729, 0x23D967BF, 0xB3667A2E, 0xC4614AB8, 0x5D681B02, 0x2A6F2B94,
    0xB40BBE37, 0xC30C8EA1, 0x5A05DF1B, 0x2D02EF8D
}; 

#define CRC32(c, b) (crc32[((int)(c) ^ (b)) & 0xff] ^ ((c) >> 8))

static int 
Crc32FromObj(Tcl_Obj *objPtr, unsigned long *sumPtr)
{
    char *bp, *bend, *buffer;
    int numBytes;
    unsigned long sum;
    
    buffer = Tcl_GetStringFromObj(objPtr, &numBytes);
    sum = *sumPtr;
    for (bp = buffer, bend = bp + numBytes; bp < bend; bp++) {
        sum = CRC32(sum, *bp);
    }
    *sumPtr = sum;
    return TCL_OK;
}

static int 
Crc32File(Tcl_Interp *interp, char *fileName, unsigned long *sumPtr)
{
    Tcl_Channel channel;
    int closeChannel;
    int done;
    unsigned long sum;

    closeChannel = TRUE;
    if ((fileName[0] == '@') && (fileName[1] != '\0')) {
        int mode;
        
        channel = Tcl_GetChannel(interp, fileName+1, &mode);
        if (channel == NULL) {
            return TCL_ERROR;
        }
        if ((mode & TCL_WRITABLE) == 0) {
            Tcl_AppendResult(interp, "channel \"", fileName, 
                "\" not opened for writing", (char *)NULL);
            return TCL_ERROR;
        }
        closeChannel = FALSE;
    } else {
        channel = Tcl_OpenFileChannel(interp, fileName, "r", 0);
        if (channel == NULL) {
            return TCL_ERROR;
        }
    }
    if (Tcl_SetChannelOption(interp, channel, "-translation", "binary") 
        != TCL_OK) {
        return TCL_ERROR;
    }
    done = FALSE;
    sum = *sumPtr;
    while (!done) {
        char *bp, *bend;
        int numBytes;
        char buffer[BUFFSIZE+1];

        numBytes = Tcl_Read(channel, buffer, sizeof(char) * BUFFSIZE);
        if (numBytes < 0) {
            Tcl_AppendResult(interp, "\nread error: ", Tcl_PosixError(interp),
                             (char *)NULL);
            if (closeChannel) {
                Tcl_Close(interp, channel);
            }
            return TCL_ERROR;
        }
        done = Tcl_Eof(channel);
        for (bp = buffer, bend = bp + numBytes; bp < bend; bp++) {
            sum = CRC32(sum, *bp);
        }
    }
    if (closeChannel) {
        Tcl_Close(interp, channel);
    }
    *sumPtr = sum;
    return TCL_OK;
}


/*ARGSUSED*/
static int
Crc32Op(ClientData clientData, Tcl_Interp *interp, int objc, 
        Tcl_Obj *const *objv)
{
    unsigned long crc;

    crc = 0L;
    crc = crc ^ 0xffffffffUL;
    if (objc == 3) {
        if (Crc32File(interp, Tcl_GetString(objv[2]), &crc) != TCL_OK) {
            return TCL_ERROR;
        }
    } else if (objc == 4) {
        char *string;

        string = Tcl_GetString(objv[2]);
        if (strcmp(string, "-data") == 0) {
            if (Crc32FromObj(objv[3], &crc) != TCL_OK) {
                return TCL_ERROR;
            }
         } else { 
            Tcl_AppendResult(interp, "wrong # args: should be \"", 
                Tcl_GetString(objv[0]), " crc32 ?fileName? ?-data dataString?", 
                             (char *)NULL);
            return TCL_ERROR;
        }
    } else {
        Tcl_AppendResult(interp, "wrong # args: should be \"", 
                Tcl_GetString(objv[0]), " crc32 ?fileName? ?-data dataString?", 
                (char *)NULL);
            return TCL_ERROR;
    }
    crc = crc ^ 0xffffffffUL;
    {
        char buf[200];

        Blt_FormatString(buf, 200, "%lx", crc);
        Tcl_SetStringObj(Tcl_GetObjResult(interp), buf, -1);
    }
    return TCL_OK;
}

/*ARGSUSED*/
static int
Base64Op(ClientData clientData, Tcl_Interp *interp, int objc, 
         Tcl_Obj *const *objv)
{
    int option;
    static const char *args[] = {
        "decode", "encode",  NULL,
    };

    if (objc != 4) {
        Tcl_AppendResult(interp, "wrong # args: should be \"", 
                Tcl_GetString(objv[1]), "base64 encode|decode bytes\"", 
                (char *)NULL);
        return TCL_ERROR;
    }
    if (Tcl_GetIndexFromObj(interp, objv[2], args, "qualifier", TCL_EXACT,
                &option) != TCL_OK) {
        return TCL_ERROR;
    }
    switch (option) {
    case 0:             /* decode */
        {
            int length;
            Tcl_Obj *objPtr;
            const char *string;

            string = Tcl_GetStringFromObj(objv[3], &length);
            objPtr = Blt_DecodeBase64ToObj(interp, string, (size_t)length); 
            if (objPtr == NULL) {
                return TCL_ERROR;
            }
            Tcl_SetObjResult(interp, objPtr);
        }
        break;
    case 1:                             /* encode */
        {
            int length;
            unsigned char *bp;
            Tcl_Obj *objPtr;

            bp = Tcl_GetByteArrayFromObj(objv[3], &length);
            objPtr = Blt_EncodeBase64ToObj(bp, length);
            if (objPtr == NULL) {
                return TCL_ERROR;
            }
            Tcl_SetObjResult(interp, objPtr);
        }
        break;
    default:
        Tcl_AppendResult(interp, "bad option \"", Tcl_GetString(objv[2]), 
                         "\": should be encode or decode", (char *)NULL);
        return TCL_ERROR;
    }
    return TCL_OK;
}

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
        char buffer[BUFFSIZE+1];
        int numBytes;

        numBytes = Tcl_Read(channel, buffer, sizeof(char) * BUFFSIZE);
        for (bp = buffer, bend = bp + numBytes; bp < bend; bp++) {
            switch (*bp) {
            case '\t':
            case ' ':
                /* 
                 * Add whitespace only if it's not leading or we're inside of
                 * quotes or a path.
           b      */
                if ((fp != field) || (inQuotes) || (isPath)) {
                    *fp++ = *bp; 
                }
                break;

            case '\\':
                /* 
                 * Handle special case CSV files that allow unquoted paths.
                 * Example:  ...,\this\path " should\have been\quoted\,...
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
                    const char *last;
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
             * We're reached the end of input. But there may not have been a
             * final newline to trigger the final appends. So check if 1) a
             * last field is still needs appending the the last record and if
             * 2) a last record is still needs appending to the list.
             */
            if (fp != field) {
                const char *last;

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
                 * Add whitespace only if it's not leading or we're inside of
                 * quotes or a path.
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
         * We're reached the end of input. But there may not have been a final
         * newline to trigger the final appends. So check if 1) a last field
         * is still needs appending the the last record and if 2) a last
         * record is still needs appending to the list.
         */
        if (fp != field) {
            const char *last;
            
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
CsvOp(ClientData clientData, Tcl_Interp *interp, int objc, 
      Tcl_Obj *const *objv)
{
    if (objc == 3) {
        return ParseCsvFile(interp, Tcl_GetString(objv[2]));
    }
    if (objc == 4) {
        char *string;

        string = Tcl_GetString(objv[2]);
        if (strcmp(string, "-data") == 0) {
            return  ParseCsvData(interp, objv[3]);
        } 
    }
    Tcl_AppendResult(interp, "wrong # args: should be \"", 
        Tcl_GetString(objv[0]), "csv ?fileName? ?-data dataString?", 
        (char *)NULL);
    return TCL_ERROR;
}
    
static Blt_OpSpec fileOps[] =
{
    {"base64",    1, Base64Op,    3, 0, "encode|decode fileName",},
    {"crc32",     2, Crc32Op,     3, 4, "?fileName? ?-data string?",},
    {"csv",       2, CsvOp,       3, 4, "?fileName? ?-data string?",},
};

static int numFileOps = sizeof(fileOps) / sizeof(Blt_OpSpec);

/* ARGSUSED */
static int
FileCmd(ClientData clientData, Tcl_Interp *interp, int objc, 
        Tcl_Obj *const *objv)
{
    Tcl_ObjCmdProc *proc;
    int result;

    proc = Blt_GetOpFromObj(interp, numFileOps, fileOps, BLT_OP_ARG1,  objc, 
        objv, 0);
    if (proc == NULL) {
        return TCL_ERROR;
    }
    result = (*proc) (clientData, interp, objc, objv);
    return result;
}

int
Blt_FileCmdInitProc(Tcl_Interp *interp)
{
    static Blt_CmdSpec cmdSpec = {"file", FileCmd,};

    return Blt_InitCmd(interp, "::blt", &cmdSpec);
}

#endif /* NO_FILECMD */
