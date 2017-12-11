/* -*- mode: c; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 * bltParse.c --
 *
 * Contains a collection of procedures that are used to parse Tcl
 * commands or parts of commands (like quoted strings or nested 
 * sub-commands).
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
 * This file is copied from tclParse.c in the TCL library distribution.
 *
 * Copyright (c) 1987-1993 The Regents of the University of California.
 * Copyright (c) 1994-1998 Sun Microsystems, Inc.
 * 
 *   This software is copyrighted by the Regents of the University of
 *   California, Sun Microsystems, Inc., and other parties.  The following
 *   terms apply to all files associated with the software unless
 *   explicitly disclaimed in individual files.
 * 
 *   The authors hereby grant permission to use, copy, modify, distribute,
 *   and license this software and its documentation for any purpose,
 *   provided that existing copyright notices are retained in all copies
 *   and that this notice is included verbatim in any distributions. No
 *   written agreement, license, or royalty fee is required for any of the
 *   authorized uses.  Modifications to this software may be copyrighted by
 *   their authors and need not follow the licensing terms described here,
 *   provided that the new terms are clearly indicated on the first page of
 *   each file where they apply.
 * 
 *   IN NO EVENT SHALL THE AUTHORS OR DISTRIBUTORS BE LIABLE TO ANY PARTY
 *   FOR DIRECT, INDIRECT, SPECIAL, INCIDENTAL, OR CONSEQUENTIAL DAMAGES
 *   ARISING OUT OF THE USE OF THIS SOFTWARE, ITS DOCUMENTATION, OR ANY
 *   DERIVATIVES THEREOF, EVEN IF THE AUTHORS HAVE BEEN ADVISED OF THE
 *   POSSIBILITY OF SUCH DAMAGE.
 * 
 *   THE AUTHORS AND DISTRIBUTORS SPECIFICALLY DISCLAIM ANY WARRANTIES,
 *   INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 *   MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE, AND
 *   NON-INFRINGEMENT.  THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, AND
 *   THE AUTHORS AND DISTRIBUTORS HAVE NO OBLIGATION TO PROVIDE
 *   MAINTENANCE, SUPPORT, UPDATES, ENHANCEMENTS, OR MODIFICATIONS.
 *
 *   GOVERNMENT USE: If you are acquiring this software on behalf of the
 *   U.S. government, the Government shall have only "Restricted Rights" in
 *   the software and related documentation as defined in the Federal
 *   Acquisition Regulations (FARs) in Clause 52.227.19 (c) (2).  If you
 *   are acquiring the software on behalf of the Department of Defense, the
 *   software shall be classified as "Commercial Computer Software" and the
 *   Government shall have only "Restricted Rights" as defined in Clause
 *   252.227-7013 (b) (3) of DFARs.  Notwithstanding the foregoing, the
 *   authors grant the U.S. Government and others acting in its behalf
 *   permission to use and distribute the software in accordance with the
 *   terms specified in this license.
 */

/*
 * Since TCL 8.1.0 these routines have been replaced by ones that
 * generate byte-codes.  But since these routines are used in vector
 * expressions, where no such byte-compilation is necessary, I now
 * include them.  In fact, the byte-compiled versions would be slower
 * since the compiled code typically runs only one time.
 */
#define BUILD_BLT_TCL_PROCS 1
#include <bltInt.h>

#ifdef HAVE_STRING_H
  #include <string.h>
#endif /* HAVE_STRING_H */

#include "bltAlloc.h"
#include "bltParse.h"

/*
 * A table used to classify input characters to assist in parsing
 * TCL commands.  The table should be indexed with a signed character
 * using the CHAR_TYPE macro.  The character may have a negative
 * value.  The CHAR_TYPE macro takes a pointer to a signed character
 * and a pointer to the last character in the source string.  If the
 * src pointer is pointing at the terminating null of the string,
 * CHAR_TYPE returns TCL_COMMAND_END.
 */

#define STATIC_STRING_SPACE     150
#define UCHAR(c)                ((unsigned char) (c))
#define TCL_NORMAL              0x01
#define TCL_SPACE               0x02
#define TCL_COMMAND_END         0x04
#define TCL_QUOTE               0x08
#define TCL_OPEN_BRACKET        0x10
#define TCL_OPEN_BRACE          0x20
#define TCL_CLOSE_BRACE         0x40
#define TCL_BACKSLASH           0x80
#define TCL_DOLLAR              0x00

/*
 * The following table assigns a type to each character. Only types
 * meaningful to TCL parsing are represented here. The table is
 * designed to be referenced with either signed or unsigned characters,
 * so it has 384 entries. The first 128 entries correspond to negative
 * character values, the next 256 correspond to positive character
 * values. The last 128 entries are identical to the first 128. The
 * table is always indexed with a 128-byte offset (the 128th entry
 * corresponds to a 0 character value).
 */

static unsigned char tclTypeTable[] =
{
 /*
     * Negative character values, from -128 to -1:
     */

    TCL_NORMAL, TCL_NORMAL, TCL_NORMAL, TCL_NORMAL,
    TCL_NORMAL, TCL_NORMAL, TCL_NORMAL, TCL_NORMAL,
    TCL_NORMAL, TCL_NORMAL, TCL_NORMAL, TCL_NORMAL,
    TCL_NORMAL, TCL_NORMAL, TCL_NORMAL, TCL_NORMAL,
    TCL_NORMAL, TCL_NORMAL, TCL_NORMAL, TCL_NORMAL,
    TCL_NORMAL, TCL_NORMAL, TCL_NORMAL, TCL_NORMAL,
    TCL_NORMAL, TCL_NORMAL, TCL_NORMAL, TCL_NORMAL,
    TCL_NORMAL, TCL_NORMAL, TCL_NORMAL, TCL_NORMAL,
    TCL_NORMAL, TCL_NORMAL, TCL_NORMAL, TCL_NORMAL,
    TCL_NORMAL, TCL_NORMAL, TCL_NORMAL, TCL_NORMAL,
    TCL_NORMAL, TCL_NORMAL, TCL_NORMAL, TCL_NORMAL,
    TCL_NORMAL, TCL_NORMAL, TCL_NORMAL, TCL_NORMAL,
    TCL_NORMAL, TCL_NORMAL, TCL_NORMAL, TCL_NORMAL,
    TCL_NORMAL, TCL_NORMAL, TCL_NORMAL, TCL_NORMAL,
    TCL_NORMAL, TCL_NORMAL, TCL_NORMAL, TCL_NORMAL,
    TCL_NORMAL, TCL_NORMAL, TCL_NORMAL, TCL_NORMAL,
    TCL_NORMAL, TCL_NORMAL, TCL_NORMAL, TCL_NORMAL,
    TCL_NORMAL, TCL_NORMAL, TCL_NORMAL, TCL_NORMAL,
    TCL_NORMAL, TCL_NORMAL, TCL_NORMAL, TCL_NORMAL,
    TCL_NORMAL, TCL_NORMAL, TCL_NORMAL, TCL_NORMAL,
    TCL_NORMAL, TCL_NORMAL, TCL_NORMAL, TCL_NORMAL,
    TCL_NORMAL, TCL_NORMAL, TCL_NORMAL, TCL_NORMAL,
    TCL_NORMAL, TCL_NORMAL, TCL_NORMAL, TCL_NORMAL,
    TCL_NORMAL, TCL_NORMAL, TCL_NORMAL, TCL_NORMAL,
    TCL_NORMAL, TCL_NORMAL, TCL_NORMAL, TCL_NORMAL,
    TCL_NORMAL, TCL_NORMAL, TCL_NORMAL, TCL_NORMAL,
    TCL_NORMAL, TCL_NORMAL, TCL_NORMAL, TCL_NORMAL,
    TCL_NORMAL, TCL_NORMAL, TCL_NORMAL, TCL_NORMAL,
    TCL_NORMAL, TCL_NORMAL, TCL_NORMAL, TCL_NORMAL,
    TCL_NORMAL, TCL_NORMAL, TCL_NORMAL, TCL_NORMAL,
    TCL_NORMAL, TCL_NORMAL, TCL_NORMAL, TCL_NORMAL,
    TCL_NORMAL, TCL_NORMAL, TCL_NORMAL, TCL_NORMAL,

    /*
     * Positive character values, from 0-127:
     */

    TCL_NORMAL, TCL_NORMAL, TCL_NORMAL, TCL_NORMAL,
    TCL_NORMAL, TCL_NORMAL, TCL_NORMAL, TCL_NORMAL,
    TCL_NORMAL, TCL_SPACE, TCL_COMMAND_END, TCL_SPACE,
    TCL_SPACE, TCL_SPACE, TCL_NORMAL, TCL_NORMAL,
    TCL_NORMAL, TCL_NORMAL, TCL_NORMAL, TCL_NORMAL,
    TCL_NORMAL, TCL_NORMAL, TCL_NORMAL, TCL_NORMAL,
    TCL_NORMAL, TCL_NORMAL, TCL_NORMAL, TCL_NORMAL,
    TCL_NORMAL, TCL_NORMAL, TCL_NORMAL, TCL_NORMAL,
    TCL_SPACE, TCL_NORMAL, TCL_QUOTE, TCL_NORMAL,
    TCL_DOLLAR, TCL_NORMAL, TCL_NORMAL, TCL_NORMAL,
    TCL_NORMAL, TCL_NORMAL, TCL_NORMAL, TCL_NORMAL,
    TCL_NORMAL, TCL_NORMAL, TCL_NORMAL, TCL_NORMAL,
    TCL_NORMAL, TCL_NORMAL, TCL_NORMAL, TCL_NORMAL,
    TCL_NORMAL, TCL_NORMAL, TCL_NORMAL, TCL_NORMAL,
    TCL_NORMAL, TCL_NORMAL, TCL_NORMAL, TCL_COMMAND_END,
    TCL_NORMAL, TCL_NORMAL, TCL_NORMAL, TCL_NORMAL,
    TCL_NORMAL, TCL_NORMAL, TCL_NORMAL, TCL_NORMAL,
    TCL_NORMAL, TCL_NORMAL, TCL_NORMAL, TCL_NORMAL,
    TCL_NORMAL, TCL_NORMAL, TCL_NORMAL, TCL_NORMAL,
    TCL_NORMAL, TCL_NORMAL, TCL_NORMAL, TCL_NORMAL,
    TCL_NORMAL, TCL_NORMAL, TCL_NORMAL, TCL_NORMAL,
    TCL_NORMAL, TCL_NORMAL, TCL_NORMAL, TCL_NORMAL,
    TCL_NORMAL, TCL_NORMAL, TCL_NORMAL, TCL_OPEN_BRACKET,
    TCL_BACKSLASH, TCL_COMMAND_END, TCL_NORMAL, TCL_NORMAL,
    TCL_NORMAL, TCL_NORMAL, TCL_NORMAL, TCL_NORMAL,
    TCL_NORMAL, TCL_NORMAL, TCL_NORMAL, TCL_NORMAL,
    TCL_NORMAL, TCL_NORMAL, TCL_NORMAL, TCL_NORMAL,
    TCL_NORMAL, TCL_NORMAL, TCL_NORMAL, TCL_NORMAL,
    TCL_NORMAL, TCL_NORMAL, TCL_NORMAL, TCL_NORMAL,
    TCL_NORMAL, TCL_NORMAL, TCL_NORMAL, TCL_NORMAL,
    TCL_NORMAL, TCL_NORMAL, TCL_NORMAL, TCL_OPEN_BRACE,
    TCL_NORMAL, TCL_CLOSE_BRACE, TCL_NORMAL, TCL_NORMAL,

    /*
     * Large unsigned character values, from 128-255:
     */

    TCL_NORMAL, TCL_NORMAL, TCL_NORMAL, TCL_NORMAL,
    TCL_NORMAL, TCL_NORMAL, TCL_NORMAL, TCL_NORMAL,
    TCL_NORMAL, TCL_NORMAL, TCL_NORMAL, TCL_NORMAL,
    TCL_NORMAL, TCL_NORMAL, TCL_NORMAL, TCL_NORMAL,
    TCL_NORMAL, TCL_NORMAL, TCL_NORMAL, TCL_NORMAL,
    TCL_NORMAL, TCL_NORMAL, TCL_NORMAL, TCL_NORMAL,
    TCL_NORMAL, TCL_NORMAL, TCL_NORMAL, TCL_NORMAL,
    TCL_NORMAL, TCL_NORMAL, TCL_NORMAL, TCL_NORMAL,
    TCL_NORMAL, TCL_NORMAL, TCL_NORMAL, TCL_NORMAL,
    TCL_NORMAL, TCL_NORMAL, TCL_NORMAL, TCL_NORMAL,
    TCL_NORMAL, TCL_NORMAL, TCL_NORMAL, TCL_NORMAL,
    TCL_NORMAL, TCL_NORMAL, TCL_NORMAL, TCL_NORMAL,
    TCL_NORMAL, TCL_NORMAL, TCL_NORMAL, TCL_NORMAL,
    TCL_NORMAL, TCL_NORMAL, TCL_NORMAL, TCL_NORMAL,
    TCL_NORMAL, TCL_NORMAL, TCL_NORMAL, TCL_NORMAL,
    TCL_NORMAL, TCL_NORMAL, TCL_NORMAL, TCL_NORMAL,
    TCL_NORMAL, TCL_NORMAL, TCL_NORMAL, TCL_NORMAL,
    TCL_NORMAL, TCL_NORMAL, TCL_NORMAL, TCL_NORMAL,
    TCL_NORMAL, TCL_NORMAL, TCL_NORMAL, TCL_NORMAL,
    TCL_NORMAL, TCL_NORMAL, TCL_NORMAL, TCL_NORMAL,
    TCL_NORMAL, TCL_NORMAL, TCL_NORMAL, TCL_NORMAL,
    TCL_NORMAL, TCL_NORMAL, TCL_NORMAL, TCL_NORMAL,
    TCL_NORMAL, TCL_NORMAL, TCL_NORMAL, TCL_NORMAL,
    TCL_NORMAL, TCL_NORMAL, TCL_NORMAL, TCL_NORMAL,
    TCL_NORMAL, TCL_NORMAL, TCL_NORMAL, TCL_NORMAL,
    TCL_NORMAL, TCL_NORMAL, TCL_NORMAL, TCL_NORMAL,
    TCL_NORMAL, TCL_NORMAL, TCL_NORMAL, TCL_NORMAL,
    TCL_NORMAL, TCL_NORMAL, TCL_NORMAL, TCL_NORMAL,
    TCL_NORMAL, TCL_NORMAL, TCL_NORMAL, TCL_NORMAL,
    TCL_NORMAL, TCL_NORMAL, TCL_NORMAL, TCL_NORMAL,
    TCL_NORMAL, TCL_NORMAL, TCL_NORMAL, TCL_NORMAL,
    TCL_NORMAL, TCL_NORMAL, TCL_NORMAL, TCL_NORMAL,
};

#define CHAR_TYPE(src,last) \
        (((src)==(last))?TCL_COMMAND_END:(tclTypeTable+128)[(int)*(src)])

/*
 *---------------------------------------------------------------------------
 *
 * Blt_ParseNestedCmd --
 *
 *      This procedure parses a nested TCL command between
 *      brackets, returning the result of the command.
 *
 * Results:
 *      The return value is a standard TCL result, which is
 *      TCL_OK unless there was an error while executing the
 *      nested command.  If an error occurs then interp->result
 *      contains a standard error message.  *TermPtr is filled
 *      in with the address of the character just after the
 *      last one processed;  this is usually the character just
 *      after the matching close-bracket, or the null character
 *      at the end of the string if the close-bracket was missing
 *      (a missing close bracket is an error).  The result returned
 *      by the command is stored in standard fashion in *parsePtr,
 *      null-terminated, with parsePtr->next pointing to the null
 *      character.
 *
 * Side effects:
 *      The storage space at *parsePtr may be expanded.
 *
 *---------------------------------------------------------------------------
 */
int
Blt_ParseNestedCmd(
    Tcl_Interp *interp,         /* Interpreter to use for nested command
                                 * evaluations and error messages. */
    const char *string,         /* Character just after opening bracket. */
    int flags,                  /* Flags to pass to nested Tcl_Eval. */
    const char **termPtr,       /* Store address of terminating character
                                 * here. */
    ParseValue *parsePtr)       /* Information about where to place
                                 * result of command. */
{
    int result, length, shortfall;
    Interp *iPtr = (Interp *)interp;

    iPtr->evalFlags = flags | TCL_BRACKET_TERM;
    result = Tcl_Eval(interp, string);
    *termPtr = (string + iPtr->termOffset);
    if (result != TCL_OK) {
        /*
         * The increment below results in slightly cleaner message in
         * the errorInfo variable (the close-bracket will appear).
         */

        if (**termPtr == ']') {
            *termPtr += 1;
        }
        return result;
    }
    (*termPtr) += 1;
    length = (int)strlen(iPtr->result);
    shortfall = length + 1 - (parsePtr->end - parsePtr->next);
    if (shortfall > 0) {
        (*parsePtr->expandProc) (parsePtr, shortfall);
    }
    strcpy(parsePtr->next, iPtr->result);
    parsePtr->next += length;

    Tcl_FreeResult(interp);
    iPtr->result = iPtr->resultSpace;
    iPtr->resultSpace[0] = '\0';
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * Blt_ParseBraces --
 *
 *      This procedure scans the information between matching
 *      curly braces.
 *
 * Results:
 *      The return value is a standard TCL result, which is
 *      TCL_OK unless there was an error while parsing string.
 *      If an error occurs then interp->result contains a
 *      standard error message.  *TermPtr is filled
 *      in with the address of the character just after the
 *      last one successfully processed;  this is usually the
 *      character just after the matching close-brace.  The
 *      information between curly braces is stored in standard
 *      fashion in *parsePtr, null-terminated with parsePtr->next
 *      pointing to the terminating null character.
 *
 * Side effects:
 *      The storage space at *parsePtr may be expanded.
 *
 *---------------------------------------------------------------------------
 */

int
Blt_ParseBraces(
    Tcl_Interp *interp,         /* Interpreter to use for nested command
                                 * evaluations and error messages. */
    const char *string,         /* Character just after opening bracket. */
    const char **termPtr,       /* Store address of terminating character
                                 * here. */
    ParseValue *parsePtr)       /* Information about where to place
                                 * result of command. */
{
    int level;
    const char *src;
    char *dest, *end;
    const char *lastChar = string + strlen(string);

    src = string;
    dest = parsePtr->next;
    end = parsePtr->end;
    level = 1;

    /*
     * Copy the characters one at a time to the result area, stopping
     * when the matching close-brace is found.
     */

    for (;;) {
        char c;

        c = *src;
        src++;

        if (dest == end) {
            parsePtr->next = dest;
            (*parsePtr->expandProc) (parsePtr, 20);
            dest = parsePtr->next;
            end = parsePtr->end;
        }
        *dest = c;
        dest++;

        if (CHAR_TYPE(src - 1, lastChar) == TCL_NORMAL) {
            continue;
        } else if (c == '{') {
            level++;
        } else if (c == '}') {
            level--;
            if (level == 0) {
                dest--;         /* Don't copy the last close brace. */
                break;
            }
        } else if (c == '\\') {
            int count;

            /*
             * Must always squish out backslash-newlines, even when in
             * braces.  This is needed so that this sequence can appear
             * anywhere in a command, such as the middle of an expression.
             */

            if (*src == '\n') {
                dest[-1] = Tcl_Backslash(src - 1, &count);
                src += count - 1;
            } else {
                Tcl_Backslash(src - 1, &count);
                while (count > 1) {
                    if (dest == end) {
                        parsePtr->next = dest;
                        (*parsePtr->expandProc) (parsePtr, 20);
                        dest = parsePtr->next;
                        end = parsePtr->end;
                    }
                    *dest = *src;
                    dest++;
                    src++;
                    count--;
                }
            }
        } else if (c == '\0') {
            Tcl_AppendResult(interp, "missing close-brace", (char *)NULL);
            *termPtr = string - 1;
            return TCL_ERROR;
        }
    }

    *dest = '\0';
    parsePtr->next = dest;
    *termPtr = src;
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * Blt_ExpandParseValue --
 *
 *      This procedure is commonly used as the value of the
 *      expandProc in a ParseValue.  It uses malloc to allocate
 *      more space for the result of a parse.
 *
 * Results:
 *      The buffer space in *parsePtr is reallocated to something
 *      larger, and if parsePtr->clientData is non-zero the old
 *      buffer is freed.  Information is copied from the old
 *      buffer to the new one.
 *
 * Side effects:
 *      None.
 *
 *---------------------------------------------------------------------------
 */
void
Blt_ExpandParseValue(
    ParseValue *parsePtr,       /* Information about buffer that
                                 * must be expanded.  If the clientData
                                 * in the structure is non-zero, it
                                 * means that the current buffer is
                                 * dynamically allocated. */
    int needed)                 /* Minimum amount of additional space
                                 * to allocate. */
{
    int size;
    char *buffer;

    /*
     * Either double the size of the buffer or add enough new space
     * to meet the demand, whichever produces a larger new buffer.
     */
    size = (parsePtr->end - parsePtr->buffer) + 1;
    if (size < needed) {
        size += needed;
    } else {
        size += size;
    }
    buffer = Blt_AssertMalloc((unsigned int)size);

    /*
     * Copy from old buffer to new, free old buffer if needed, and
     * mark new buffer as malloc-ed.
     */
    memcpy((VOID *) buffer, (VOID *) parsePtr->buffer,
        (size_t) (parsePtr->next - parsePtr->buffer));
    parsePtr->next = buffer + (parsePtr->next - parsePtr->buffer);
    if (parsePtr->clientData != 0) {
        Blt_Free(parsePtr->buffer);
    }
    parsePtr->buffer = buffer;
    parsePtr->end = buffer + size - 1;
    parsePtr->clientData = (ClientData)1;
}

/*
 *---------------------------------------------------------------------------
 *
 * Blt_ParseQuotes --
 *
 *      This procedure parses a double-quoted string such as a
 *      quoted TCL command argument or a quoted value in a Tcl
 *      expression.  This procedure is also used to parse array
 *      element names within parentheses, or anything else that
 *      needs all the substitutions that happen in quotes.
 *
 * Results:
 *      The return value is a standard TCL result, which is
 *      TCL_OK unless there was an error while parsing the
 *      quoted string.  If an error occurs then interp->result
 *      contains a standard error message.  *TermPtr is filled
 *      in with the address of the character just after the
 *      last one successfully processed;  this is usually the
 *      character just after the matching close-quote.  The
 *      fully-substituted contents of the quotes are stored in
 *      standard fashion in *parsePtr, null-terminated with
 *      parsePtr->next pointing to the terminating null character.
 *
 * Side effects:
 *      The buffer space in parsePtr may be enlarged by calling its
 *      expandProc.
 *
 *---------------------------------------------------------------------------
 */
int
Blt_ParseQuotes(
    Tcl_Interp *interp,         /* Interpreter to use for nested command
                                 * evaluations and error messages. */
    const char *string,         /* Character just after opening double-
                                 * quote. */
    int termChar,               /* Character that terminates "quoted" string
                                 * (usually double-quote, but sometimes
                                 * right-paren or something else). */
    int flags,                  /* Flags to pass to nested Tcl_Eval calls. */
    const char **termPtr,       /* Store address of terminating character
                                 * here. */
    ParseValue *parsePtr)       /* Information about where to place
                                 * fully-substituted result of parse. */
{
    const char *src;
    char *dest, c;
    const char *lastChar = string + strlen(string);

    src = string;
    dest = parsePtr->next;

    for (;;) {
        if (dest == parsePtr->end) {
            /*
             * Target buffer space is about to run out.  Make more space.
             */
            parsePtr->next = dest;
            (*parsePtr->expandProc) (parsePtr, 1);
            dest = parsePtr->next;
        }
        c = *src;
        src++;
        if (c == termChar) {
            *dest = '\0';
            parsePtr->next = dest;
            *termPtr = src;
            return TCL_OK;
        } else if (CHAR_TYPE(src - 1, lastChar) == TCL_NORMAL) {
          copy:
            *dest = c;
            dest++;
            continue;
        } else if (c == '$') {
            int length;
            const char *value;

            value = Tcl_ParseVar(interp, src - 1, termPtr);
            if (value == NULL) {
                return TCL_ERROR;
            }
            src = *termPtr;
            length = strlen(value);
            if ((parsePtr->end - dest) <= length) {
                parsePtr->next = dest;
                (*parsePtr->expandProc) (parsePtr, length);
                dest = parsePtr->next;
            }
            strcpy(dest, value);
            dest += length;
            continue;
        } else if (c == '[') {
            int result;

            parsePtr->next = dest;
            result = Blt_ParseNestedCmd(interp, src, flags, termPtr, parsePtr);
            if (result != TCL_OK) {
                return result;
            }
            src = *termPtr;
            dest = parsePtr->next;
            continue;
        } else if (c == '\\') {
            int numRead;

            src--;
            *dest = Tcl_Backslash(src, &numRead);
            dest++;
            src += numRead;
            continue;
        } else if (c == '\0') {
            char buf[10];

            Tcl_ResetResult(interp);
            Blt_FormatString(buf, 10, "missing %c", termChar);
            Tcl_SetStringObj(Tcl_GetObjResult(interp), buf, 9);
            *termPtr = string - 1;
            return TCL_ERROR;
        } else {
            goto copy;
        }
    }
}

