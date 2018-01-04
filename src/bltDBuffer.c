/* -*- mode: c; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 * bltDBuffer.c --
 *
 * This module implements a dynamic buffer for the BLT toolkit.  This
 * differs from Tcl_DString's in that it handles binary data.
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

#ifdef HAVE_STRING_H
  #include <string.h>
#endif /* HAVE_STRING_H */

#include <bltAlloc.h>
#include <bltDBuffer.h>

typedef struct _Blt_DBuffer DBuffer;

void
Blt_DBuffer_Init(DBuffer *srcPtr)
{
    srcPtr->bytes = NULL;
    srcPtr->cursor = srcPtr->length = srcPtr->size = 0;
    srcPtr->chunk = 64;
    srcPtr->cursor = 0;
}

void
Blt_DBuffer_Free(DBuffer *srcPtr)
{
    if ((srcPtr->bytes != NULL) && (srcPtr->size > 0)) {
        Blt_Free(srcPtr->bytes);
    }
    Blt_DBuffer_Init(srcPtr);
}

Blt_DBuffer
Blt_DBuffer_Create(void)
{
    DBuffer *srcPtr;

    srcPtr = Blt_AssertMalloc(sizeof(DBuffer));
    Blt_DBuffer_Init(srcPtr);
    return srcPtr;
}

void 
Blt_DBuffer_Destroy(DBuffer *srcPtr) 
{
    Blt_DBuffer_Free(srcPtr);
    Blt_Free(srcPtr);
}

int
Blt_DBuffer_Resize(DBuffer *srcPtr, size_t newSize)
{
    if (srcPtr->size <= newSize) {
        size_t size, wanted;
        unsigned char *bytes;

        wanted = newSize + 1;
        size = srcPtr->chunk; 

        /* 
         * Double the buffer size until we have enough room or hit 1M.
         * After 1M, increase by multiples of 1M.
         */
        while ((size <= wanted) && (size < (1<<20))) {
            size += size;
        }    
        srcPtr->chunk = size;
        while (size <= wanted) {
            size += srcPtr->chunk;
        }
        if (srcPtr->bytes == NULL) {
            bytes = Blt_Malloc(size);
        } else {
            bytes = Blt_Realloc(srcPtr->bytes, size);
        }
        if (bytes == NULL) {
            return FALSE;
        }
        srcPtr->bytes = bytes;
        srcPtr->size = size;
    }
    return TRUE;
}

unsigned char *
Blt_DBuffer_Extend(DBuffer *srcPtr, size_t numBytes) 
{
    unsigned char *bp;

    if (!Blt_DBuffer_Resize(srcPtr, srcPtr->length + numBytes)) {
        return NULL;
    }
    bp = srcPtr->bytes + srcPtr->length;
    srcPtr->length += numBytes;
    return bp;
}

int
Blt_DBuffer_SetLength(DBuffer *srcPtr, size_t numBytes)
{
    int result;

    result = TRUE;
    if (srcPtr->size < numBytes) {
        result = Blt_DBuffer_Resize(srcPtr, numBytes);
    }
    srcPtr->length = numBytes;
    return result;
}

void
Blt_DBuffer_AppendByte(DBuffer *destPtr, unsigned char value)
{
    if (Blt_DBuffer_Resize(destPtr, destPtr->length + sizeof(value))) {
        destPtr->bytes[destPtr->length] = value;
        destPtr->length++;
    }
}

void
Blt_DBuffer_AppendShort(DBuffer *destPtr, unsigned short value)
{
    if (Blt_DBuffer_Resize(destPtr, destPtr->length + sizeof(value))) {
        unsigned char *bp;

        bp = destPtr->bytes + destPtr->length;
#ifdef WORDS_BIGENDIAN
        bp[0] = (value >> 8)  & 0xFF;
        bp[1] = (value)       & 0xFF;
#else
        bp[0] = (value)       & 0xFF;
        bp[1] = (value >> 8)  & 0xFF;
#endif
        destPtr->length += 2;
    }
}

void
Blt_DBuffer_AppendInt(DBuffer *destPtr, unsigned int value)
{
    if (Blt_DBuffer_Resize(destPtr, destPtr->length + sizeof(value))) {
        unsigned char *bp;

        bp = destPtr->bytes + destPtr->length;
#ifdef WORDS_BIGENDIAN
        bp[0] = (value >> 24) & 0xFF;
        bp[1] = (value >> 16) & 0xFF;
        bp[2] = (value >> 8)  & 0xFF;
        bp[3] = (value)       & 0xFF;
#else
        bp[0] = (value)       & 0xFF;
        bp[1] = (value >> 8)  & 0xFF;
        bp[2] = (value >> 16) & 0xFF;
        bp[3] = (value >> 24) & 0xFF;
#endif
        destPtr->length += sizeof(value);
    }
}

int
Blt_DBuffer_Concat(DBuffer *destPtr, DBuffer *srcPtr)
{
    return Blt_DBuffer_AppendData(destPtr, srcPtr->bytes, srcPtr->length);
}

Tcl_Obj *
Blt_DBuffer_ByteArrayObj(DBuffer *srcPtr)
{
    return Tcl_NewByteArrayObj(srcPtr->bytes, srcPtr->length);
}

Tcl_Obj *
Blt_DBuffer_StringObj(DBuffer *srcPtr)
{
    return Tcl_NewStringObj((char *)srcPtr->bytes, srcPtr->length);
}

unsigned char *
Blt_DBuffer_SetFromObj(DBuffer *srcPtr, Tcl_Obj *objPtr)
{
    const char *string;
    int length;
    unsigned char *bp;
    
    string = Tcl_GetStringFromObj(objPtr, &length);
    if (!Blt_DBuffer_Resize(srcPtr, length)) {
        return NULL;
    }
    bp = Blt_DBuffer_Bytes(srcPtr);
    memcpy(bp, string, length);
    srcPtr->length = length;
    return bp;
}

/* 
 * Blt_DBuffer_String --
 *      
 *      Returns a string representing the current buffer.  A NUL value is
 *      set for the byte beyond the end of the buffer.
 */
const char *
Blt_DBuffer_String(DBuffer *srcPtr)
{
    if (srcPtr->length == srcPtr->size) {
        /* Make sure there's room for a trailing NUL byte. */
        if (!Blt_DBuffer_Resize(srcPtr, srcPtr->length + 1)) {
            return NULL;
        }
    }
    /* Set NUL byte to location just beyond the end of the buffer. */
    srcPtr->bytes[srcPtr->length] = '\0';
    return (const char *)srcPtr->bytes;
}

int
Blt_DBuffer_AppendData(DBuffer *srcPtr, const unsigned char *data, 
                       size_t numBytes)
{
    unsigned char *bp;

    bp = Blt_DBuffer_Extend(srcPtr, numBytes);
    if (bp == NULL) {
        return FALSE;
    }
    memcpy(bp, data, numBytes);
    return TRUE;
}

int
Blt_DBuffer_AppendString(DBuffer *srcPtr, const char *string, int numBytes)
{
    unsigned char *bp;

    if (numBytes < 0) {
        numBytes = strlen(string);
    }
    bp = Blt_DBuffer_Extend(srcPtr, numBytes);
    if (bp == NULL) {
        return FALSE;
    }
    memcpy(bp, string, numBytes);
    return TRUE;
}

int
Blt_DBuffer_InsertData(DBuffer *srcPtr, const unsigned char *data, 
                       size_t numBytes, size_t index)
{
    unsigned char *bp;
    size_t oldLength, newLength, trailing;
    size_t i, j, k;
    
    oldLength = Blt_DBuffer_Length(srcPtr);
    bp = Blt_DBuffer_Extend(srcPtr, numBytes);
    if (bp == NULL) {
        return FALSE;
    }
    newLength = Blt_DBuffer_Length(srcPtr);
    /* Create a hole by moving the data to the end. */
    bp = Blt_DBuffer_Bytes(srcPtr);
    trailing = oldLength - index;
    for (i = oldLength - 1, j = newLength - 1, k = 0; k < trailing;
         i--, j--, k++) {
        bp[j] = bp[i];
    }
    memcpy(bp + index, data, numBytes);
    return TRUE;
}

int
Blt_DBuffer_DeleteData(DBuffer *srcPtr, size_t index, size_t numBytes)
{
    unsigned char *bp;
    size_t oldLength, newLength, trailing;
    size_t i, j, k;
    
    oldLength = Blt_DBuffer_Length(srcPtr);
    if ((index + numBytes) > oldLength) {
        return FALSE;
    }
    bp = Blt_DBuffer_Bytes(srcPtr);
    newLength = oldLength - numBytes;
    trailing = newLength - index;
    for (i = index, j = index + numBytes, k = 0; k < trailing; i++, j++, k++) {
        bp[i] = bp[j];
    }
    Blt_DBuffer_SetLength(srcPtr, newLength);
    return TRUE;
}

void
Blt_DBuffer_VarAppend(DBuffer *srcPtr, ...)
{
    va_list args;

    va_start(args, srcPtr);
    for (;;) {
        const unsigned char *string;

        string = va_arg(args, const unsigned char *);
        if (string == NULL) {
            break;
        }
        Blt_DBuffer_AppendData(srcPtr, string, strlen((const char *)string));
    }
    va_end(args);
}

int
Blt_DBuffer_Format(DBuffer *srcPtr, const char *fmt, ...)
{
    char string[BUFSIZ+4];
    int length;
    va_list args;

    va_start(args, fmt);
    length = vsnprintf(string, BUFSIZ, fmt, args);
    if (length > BUFSIZ) {
        strcat(string, "...");
    }
    va_end(args);
    length = strlen(string);
    Blt_DBuffer_AppendString(srcPtr, string, length);
    return length;
}

#include <sys/types.h>
#include <sys/stat.h>

#ifdef notdef
int
Blt_DBuffer_LoadFile(Tcl_Interp *interp, const char *fileName, 
                     Blt_DBuffer dBuffer)
{
    FILE *f;
    size_t numBytes, numRead;
    struct stat sb;
    unsigned char *bytes;

#ifdef WIN32
  #define READ_MODE "rb"
#else 
  #define READ_MODE "r"
#endif  /* WIN32 */

    f = Blt_OpenFile(interp, fileName, READ_MODE);
    if (f == NULL) {
        return TCL_ERROR;
    }
    if (fstat(fileno(f), &sb)) {
        Tcl_AppendResult(interp, "can't stat \"", fileName, "\": ",
                Tcl_PosixError(interp), (char *)NULL);
        return TCL_ERROR;
    }   
    Blt_DBuffer_Init(dBuffer);
    numBytes = sb.st_size;              /* Size of buffer */
    if (!Blt_DBuffer_Resize(dBuffer, numBytes)) {
        fclose(f);
        return TCL_ERROR;
    }   
    bytes = Blt_DBuffer_Bytes(dBuffer);
    numRead = fread(bytes, sizeof(unsigned char), numBytes, f);
    Blt_DBuffer_SetLength(dBuffer, numRead);
    fclose(f);
    if (numRead != numBytes) {
        Tcl_AppendResult(interp, "short file \"", fileName, "\" : read ", 
                Blt_Itoa(numBytes), " bytes.", (char *)NULL); 
        Blt_DBuffer_Free(dBuffer);
        return TCL_ERROR;
    }   
    return TCL_OK;
}

#else

int
Blt_DBuffer_LoadFile(Tcl_Interp *interp, const char *fileName,
                     Blt_DBuffer dBuffer)
{
    int numBytes;
    Tcl_Channel channel;

    if (fileName[0] == '@') { 
        int mode;

        /* If the file name starts with a '@', then it represents the name
         * of a previously opened channel.  Verify that the channel was
         * opened for reading. */
        fileName++;
        channel = Tcl_GetChannel(interp, fileName, &mode);
        if ((mode & TCL_READABLE) == 0) {
            Tcl_AppendResult(interp, "can't read from \"", fileName, "\"",
                             (char *)NULL);
            return TCL_ERROR;
        }
    } else {
        channel = Tcl_OpenFileChannel(interp, fileName, "r", 0);
    }
    if (channel == NULL) {
        return TCL_ERROR;
    }
    if (Tcl_SetChannelOption(interp, channel, "-encoding", "binary")
        != TCL_OK) {
        return TCL_ERROR;
    }
    if (Tcl_SetChannelOption(interp, channel, "-translation", "binary") 
        != TCL_OK) {
        return TCL_ERROR;
    }
    Blt_DBuffer_Init(dBuffer);
    numBytes = 0;
    while (!Tcl_Eof(channel)) {
        int numRead;
#define BUFFER_SIZE (1<<16)
        char *bp;

        bp = (char *)Blt_DBuffer_Extend(dBuffer, BUFFER_SIZE);
        numRead = Tcl_ReadRaw(channel, bp, BUFFER_SIZE);
        if (numRead == -1) {
            Tcl_AppendResult(interp, "error reading ", fileName, ": ",
                        Tcl_PosixError(interp), (char *)NULL);
            Blt_DBuffer_Free(dBuffer);
            return TCL_ERROR;
        }
        numBytes += numRead;
        Blt_DBuffer_SetLength(dBuffer, numBytes);
    }
    Tcl_Close(interp, channel);
    return TCL_OK;
}

#endif

int 
Blt_DBuffer_SaveFile(Tcl_Interp *interp, const char *fileName, 
                     Blt_DBuffer dBuffer)
{
    Tcl_Channel channel;
    size_t numWritten, numBytes;
    unsigned char *bytes;

    channel = Tcl_OpenFileChannel(interp, fileName, "w", 0660);
    if (channel == NULL) {
        return TCL_ERROR;
    }
    Tcl_SetChannelOption(interp, channel, "-translation", "binary");
    Tcl_SetChannelOption(interp, channel, "-encoding", "binary");

    numBytes = Blt_DBuffer_Length(dBuffer);
    bytes = Blt_DBuffer_Bytes(dBuffer);
    numWritten = Tcl_Write(channel, (char *)bytes, numBytes);
    Tcl_Close(interp, channel);
    if (numWritten != numBytes) {
        Tcl_AppendResult(interp, "short file \"", fileName, (char *)NULL);
        Tcl_AppendResult(interp, "\" : wrote ", Blt_Itoa(numWritten), " of ", 
                         (char *)NULL);
        Tcl_AppendResult(interp, Blt_Itoa(numBytes), " bytes.", (char *)NULL); 
        return TCL_ERROR;
    }   
    return TCL_OK;
}

#ifdef notdef
static int 
ReadNextBlock(DBuffer *srcPtr)
{
    if (srcPtr->channel == NULL) {
        return -1;
    }
    if (Tcl_Eof(srcPtr->channel)) {
        return 0;
    }
    numRead = Tcl_ReadRaw(srcPtr->channel, srcPtr->bytes, BUFFER_SIZE);
    if (numRead == -1) {
        Tcl_AppendResult(interp, "error reading channel: ",
                         Tcl_PosixError(interp), (char *)NULL);
        return -1;
    }
    srcPtr->cursor = srcPtr->bytes;
    srcPtr->length = numRead;
    return 1;
}

int
Blt_DBuffer_GetNext(DBuffer *srcPtr)
{
    int byte;

    if ((srcPtr->cursor - srcPtr->bytes) >= srcPtr->length) {
        int result;

        result = 0;
        if (srcPtr->channel != NULL) {
            result = ReadNextBlock(srcPtr);
        }
        if (result <= 0) {
            return result;
        }
    }
    byte = *srcPtr->cursor;
    srcPtr->cursor++;
    return byte;
}
#endif

int
Blt_DBuffer_Base64Decode(Tcl_Interp *interp, const char *src, size_t numChars,
                         DBuffer *destPtr)
{
    size_t numBytes, maxBytes;
    BinaryDecoder switches;

    memset(&switches, 0, sizeof(BinaryDecoder));
    maxBytes = Blt_Base64DecodeBufferSize(numChars, &switches);
    Blt_DBuffer_SetLength(destPtr, maxBytes);
    if (Blt_DecodeBase64(interp, src, numChars, destPtr->bytes, &numBytes,
                         &switches) != TCL_OK) {
        return TCL_ERROR;
    }
    Blt_DBuffer_SetLength(destPtr, numBytes);
    return TCL_OK;
}

Tcl_Obj *
Blt_DBuffer_Base64EncodeToObj(DBuffer *srcPtr) /* Input binary buffer. */
{
    return Blt_EncodeBase64ToObj(srcPtr->bytes, srcPtr->length);
}

int
Blt_DBuffer_AppendBase64(DBuffer *destPtr, const unsigned char *src, 
                         size_t numBytes) 
{
    size_t oldLength, numChars, maxChars;
    char *dest;
    BinaryEncoder switches;

    memset(&switches, 0, sizeof(BinaryEncoder));
    maxChars = Blt_Base64EncodeBufferSize(numBytes, &switches);
    oldLength = Blt_DBuffer_Length(destPtr);
    dest = (char *)Blt_DBuffer_Extend(destPtr, maxChars);
    if (dest == NULL) {
        return FALSE;
    }
    Blt_EncodeBase64(src, numBytes, dest, &numChars, &switches);
    assert(numChars < maxChars);
    Blt_DBuffer_SetLength(destPtr, oldLength + numChars);
    return TRUE;
}

int 
Blt_DBuffer_AppendBase85(DBuffer *destPtr, const unsigned char *src, 
                         size_t numBytes) 
{
    size_t oldLength, numChars, maxChars;
    char *dest;
    BinaryEncoder switches;

    memset(&switches, 0, sizeof(BinaryEncoder));
    maxChars = Blt_Base85EncodeBufferSize(numBytes, &switches);
    oldLength = Blt_DBuffer_Length(destPtr);
    dest = (char *)Blt_DBuffer_Extend(destPtr, maxChars);
    if (dest == NULL) {
        return FALSE;
    }
    Blt_EncodeBase85(src, numBytes, dest, &numChars, &switches);
    assert(numChars <= maxChars);
    Blt_DBuffer_SetLength(destPtr, oldLength + numChars);
    return TRUE;
}
