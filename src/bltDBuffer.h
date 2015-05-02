/* -*- mode: c; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 * bltDBuffer.h --
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

#ifndef _BLT_DBUFFER_H
#define _BLT_DBUFFER_H

typedef struct _Blt_DBuffer {
    unsigned char *bytes;		/* Buffer (malloc-ed).*/
    size_t size;			/* Size of dynamically allocated
					 * buffer. */
    size_t length;			/* # of bytes read into the *
					 * buffer. Marks the current fill
					 * point of the * buffer. */
    size_t cursor;			/* Current position in buffer. */
    size_t chunk;			/* Buffer growth size. */
} *Blt_DBuffer;


BLT_EXTERN void Blt_DBuffer_VarAppend(Blt_DBuffer buffer, ...);

BLT_EXTERN int Blt_DBuffer_Format(Blt_DBuffer buffer, const char *fmt, ...);

BLT_EXTERN void Blt_DBuffer_Init(Blt_DBuffer buffer);
BLT_EXTERN void Blt_DBuffer_Free(Blt_DBuffer buffer);
BLT_EXTERN unsigned char *Blt_DBuffer_Extend(Blt_DBuffer buffer, size_t extra);
BLT_EXTERN int Blt_DBuffer_AppendData(Blt_DBuffer buffer, 
	const unsigned char *bytes, size_t extra);
BLT_EXTERN int Blt_DBuffer_AppendString(Blt_DBuffer buffer, const char *string,
        int length);
BLT_EXTERN int Blt_DBuffer_DeleteData(Blt_DBuffer buffer, size_t index,
        size_t numBytes);
BLT_EXTERN int Blt_DBuffer_InsertData(Blt_DBuffer buffer, 
        const unsigned char *bytes, size_t extra, size_t index);
BLT_EXTERN unsigned char *Blt_DBuffer_SetFromObj(Blt_DBuffer buffer,
        Tcl_Obj *objPtr);
BLT_EXTERN int Blt_DBuffer_Concat(Blt_DBuffer dest, Blt_DBuffer src);
BLT_EXTERN int Blt_DBuffer_Resize(Blt_DBuffer buffer, size_t length);
BLT_EXTERN int Blt_DBuffer_SetLength(Blt_DBuffer buffer, size_t length);
BLT_EXTERN Blt_DBuffer Blt_DBuffer_Create(void);
BLT_EXTERN void Blt_DBuffer_Destroy(Blt_DBuffer buffer);

BLT_EXTERN int Blt_DBuffer_LoadFile(Tcl_Interp *interp, const char *fileName, 
	Blt_DBuffer buffer); 
BLT_EXTERN int Blt_DBuffer_SaveFile(Tcl_Interp *interp, const char *fileName, 
	Blt_DBuffer buffer); 

BLT_EXTERN void Blt_DBuffer_AppendByte(Blt_DBuffer buffer, unsigned char byte);
BLT_EXTERN void Blt_DBuffer_AppendShort(Blt_DBuffer buffer, 
	unsigned short value);
BLT_EXTERN void Blt_DBuffer_AppendInt(Blt_DBuffer buffer, unsigned int value);
BLT_EXTERN Tcl_Obj *Blt_DBuffer_ByteArrayObj(Blt_DBuffer buffer);
BLT_EXTERN Tcl_Obj *Blt_DBuffer_StringObj(Blt_DBuffer buffer);
BLT_EXTERN const char *Blt_DBuffer_String(Blt_DBuffer buffer);

#define Blt_DBuffer_Bytes(s)		((s)->bytes)
#define Blt_DBuffer_Size(s)		((s)->size)

#define Blt_DBuffer_BytesLeft(s)	((s)->length - (s)->cursor)
#define Blt_DBuffer_NextByte(s)		((s)->bytes[(s)->cursor++])
#define Blt_DBuffer_Pointer(s)		((s)->bytes + (s)->cursor)
#define Blt_DBuffer_SetPointer(s,p)	((s)->cursor = (p) - (s)->bytes)

#define Blt_DBuffer_Rewind(s)		((s)->cursor = 0)
#define Blt_DBuffer_Cursor(s)		((s)->cursor)
#define Blt_DBuffer_SetCursor(s,n)	((s)->cursor = (n))
#define Blt_DBuffer_IncrCursor(s,i)	((s)->cursor += (i))

#define Blt_DBuffer_End(s)		((s)->bytes + (s)->length)
#define Blt_DBuffer_Length(s)		((s)->length)
#define Blt_DBuffer_SetLengthFromPointer(s,p) \
	((s)->length = ((p) - (s)->bytes))
#define Blt_DBuffer_IncrLength(s,i)	((s)->length += (i))

BLT_EXTERN int Blt_DBuffer_Base64Decode(Tcl_Interp *interp, 
	const char *string, size_t length, Blt_DBuffer buffer);
BLT_EXTERN Tcl_Obj *Blt_DBuffer_Base64EncodeToObj(Blt_DBuffer buffer);
BLT_EXTERN int Blt_DBuffer_AppendBase85(Blt_DBuffer buffer, 
	const unsigned char *bytes, size_t numBytes);
BLT_EXTERN int Blt_DBuffer_AppendBase64(Blt_DBuffer buffer, 
	const unsigned char *bytes, size_t numBytes);

#endif /*_BLT_DBUFFER_H*/
