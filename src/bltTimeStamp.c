/* -*- mode: c; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 * bltTimeStamp.c --
 *
 *      This module implements a timestamp parser for the BLT toolkit.  It
 *      differs from the new TCL "clock" command.  It was built to
 *      programatically convert thousands of timestamps in files to seconds
 *      (especially dates that may include fractional seconds).
 *      
 *      For example, if the year, month, or day are not specified, it is
 *      assumed that the date is the first day of the epoch, not the
 *      current date. The may seem strange but if you are parsing
 *      time/dates from a file you don't want to get a different time value
 *      each time you parse it.  It also handles different date formats
 *      (including fractional seconds).
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

#define _DEFAULT_SOURCE 1
#define BUILD_BLT_TCL_PROCS 1
#include "bltInt.h"

#ifdef HAVE_STDLIB_H
  #include <stdlib.h>
#endif /* HAVE_STDLIB_H */

#ifdef HAVE_CTYPE_H
  #include <ctype.h>
#endif /* HAVE_CTYPE_H */

#ifdef HAVE_TIME_H
  #include <time.h>
#endif /* HAVE_TIME_H */

#ifdef HAVE_SYS_TIME_H
  #include <sys/time.h>
#endif

#include <stdint.h>
#include "bltAlloc.h"
#include "bltString.h"
#include "bltSwitch.h"
#include "bltInitCmd.h"
#include "bltOp.h"
#include <bltMath.h>

#define DEBUG           0
#define MAXTOKENS       64              /* Specifies the maximum number of
                                         * tokens that have been
                                         * pre-allocated */
#define TRACE_ALL \
    (TCL_TRACE_WRITES | TCL_TRACE_UNSETS | TCL_TRACE_READS | TCL_GLOBAL_ONLY)

#define TZOFFSET(x) \
    ((((x)/100) * SECONDS_HOUR) + (((x) % 100) * SECONDS_MINUTE))

#define EPOCH           1970
#define EPOCH_WDAY      4               /* Thursday. */
#define IsLeapYear(y) \
        ((((y) % 4) == 0) && ((((y) % 100) != 0) || (((y) % 400) == 0)))

static const char *tokenNames[] = {
    "end",
    "month", "wday", "yday", "day", "year", "week",
    "hours", "seconds", "minutes", "ampm", 
    "tz", "dst", 
    "/", "-", ",", ":", "+", ".", "'", "(", ")", 
    "number", "iso6", "iso7", "iso8", 
    "unknown"
};

typedef enum {
    _END,
    _MONTH, _WDAY, _YDAY, _MDAY, _YEAR, _WEEK,
    _HOUR, _SECOND, _MINUTE, _AMPM,
    _TZ, _DST, 
    _SLASH, _DASH, _COMMA, _COLON, _PLUS, _DOT, _QUOTE, _LPAREN, _RPAREN,
    _NUMBER, _ISO6, _ISO7, _ISO8, _UNKNOWN,
} ParserTokenId;

typedef struct {
    int std;
    int dst;
} TimeZone;

typedef struct _ParserToken ParserToken;

struct _ParserToken {
    const char *string;                 /* String representing this
                                         * token. */
    uint64_t lvalue;                    /* Numeric value of token. */
    Tcl_Obj *objPtr;                    /* Points to the timezone offsets. */
    ParserTokenId id;                   /* Serial ID of token. */
    int length;                         /* Length of the string. */
    ParserToken *nextPtr;
    ParserToken *prevPtr;
};

typedef struct {
    Blt_DateTime date;
    ParserToken tokens[MAXTOKENS];      /* Pool of tokens. */
    ParserToken *curTokenPtr;           /* Current token being
                                         * processed. */
    const char *buffer;                 /* Buffer holding parsed string. */
    const char *nextCharPtr;            /* Points to the next character
                                         * to parse.*/
    ParserToken *headPtr;               /* First token list.*/
    ParserToken *tailPtr;               /* Last token in list. */
    unsigned short int numTokens;
    unsigned short int nextFreeToken;
    unsigned int flags;                 /* Flags: see below. */
} TimeStampParser;

#define PARSE_DATE      (1<<0)
#define PARSE_TIME      (1<<1)
#define PARSE_TZ        (1<<2)
#define PARSE_DST       (1<<4)
#define PARSE_OFFSET    (1<<5)

#define PARSE_YDAY      (1<<6)           /* Day of year has been set. */
#define PARSE_MDAY      (1<<7)           /* Day of month has been set. */
#define PARSE_WDAY      (1<<8)           /* Day of week has been set. */
#define PARSE_WEEK      (1<<8)           /* Ordinal week has been set. */
#define PARSE_WYEAR     (1<<10)          /* Year of ordinal week has been
                                          * set. */

typedef struct {
    const char *string;                 /* Name of identifier. */
    ParserTokenId id;                     /* Serial ID of identifier. */
    int value;                          /* Value associated with
                                         * identifier. */
} IdentTable;

#define SECONDS_SECOND        (1)
#define SECONDS_MINUTE        (60)
#define SECONDS_HOUR          (SECONDS_MINUTE * 60)
#define SECONDS_DAY           (SECONDS_HOUR * 24)
#define SECONDS_MONTH         (SECONDS_DAY * 30)
#define SECONDS_YEAR          (SECONDS_DAY * 365)

static const char *monthNames[] = {
    "January", "February", "March", "April", "May", "June", 
    "July", "August", "September", "October", "November", "December"
};
static int numMonths = sizeof(monthNames) / sizeof(const char *);

static const int numDaysMonth[2][13] = {
    {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31, 31},
    {31, 29, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31, 31}
} ;

static const int numDaysToMonth[2][13] = {
    { 0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334 },
    { 0, 31, 60, 91, 121, 152, 182, 213, 244, 274, 305, 335 }
};
static const int numDaysYear[2] = { 365, 366 };

static const char *weekdayNames[] = {
    "Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday",  
    "Saturday"
};
static int numWeekdays = sizeof(weekdayNames) / sizeof(const char *);

static const char *meridianNames[] = {
    "am",  "pm"
};
static int numMeridians = sizeof(meridianNames) / sizeof(const char *);

typedef struct {
    int numIds;
    ParserTokenId ids[6];
} Pattern;

static Pattern datePatterns[] = {
    { 2, {_ISO7 }},                     /* 0. yyyyddd (2012100) */
    { 2, {_ISO8 }},                     /* 1. yyyymmdd (20120131) */
    { 2, {_MONTH}},                     /* 2. mon (Jan) */
    { 2, {_WEEK }},                     /* 3. www (W01)*/
    { 2, {_YEAR }},                     /* 4. yy (1)*/
    { 2, {_YDAY }},                     /* 5. ddd:hh:mm:ss */
    { 3, {_MONTH, _MDAY }},             /* 6. mon dd */
    { 3, {_MDAY,  _MONTH}},             /* 7. dd mon yyyy (31 Jan 2012) */
    { 3, {_WEEK,  _WDAY }},             /* 8. yyyywwwd (2012W017) */
    { 4, {_MONTH, _MDAY,  _YEAR}},      /* 9. mm-dd-yy (01-31-99) */
    { 4, {_MDAY,  _MONTH, _YEAR}},      /* 10. dd-mon-yy (31-Jan-99) */
    { 4, {_MDAY,  _DOT,   _MONTH}},     /* 11. dd.mon (31.Jan) */
    { 4, {_MONTH, _DOT,   _MDAY}},      /* 12. mon.dd (Jan.31) */
    { 5, {_MDAY, _DOT,   _MONTH, _DOT}}, /* 12. dd.mon.yyyy (31.Jan.1999) */
    { 5, {_DOT, _MONTH, _DOT,   _MDAY}}, /* 13. .mon.dd (.Jan.31) */
    { 6, {_YEAR, _DOT, _MONTH, _DOT, _MDAY}}, /* 14. yy.mon.dd (1999.Jan.31) */
    { 6, {_YEAR, _COLON,  _MONTH, _COLON, _MDAY}}, /* 15. yyyy:mm:dd hh:mm:ss */
    { 1, {}}
};

static int numDatePatterns = sizeof(datePatterns) / sizeof(Pattern);

typedef struct {
    Tcl_Obj *tzObjPtr;
    Tcl_Obj *fmtObjPtr;
} FormatSwitches;

static Blt_SwitchSpec formatSwitches[] = 
{
    {BLT_SWITCH_OBJ, "-timezone", "",  (char *)NULL,
        Blt_Offset(FormatSwitches, tzObjPtr), 0},
    {BLT_SWITCH_OBJ, "-format", "",  (char *)NULL,
        Blt_Offset(FormatSwitches, fmtObjPtr), 0},
    {BLT_SWITCH_END}
};

static Tcl_ObjCmdProc TimeStampCmd;

/*
 *---------------------------------------------------------------------------
 *
 * GetTimeZoneOffset --
 *
 *      Converts a string in the format "+/-hh:mm:ss" into the number
 *      seconds representing the offset from UTC.  
 *
 * Results:
 *      A standard TCL result.  The offset is returned via *offsetPtr*.
 *
 *---------------------------------------------------------------------------
 */
static int
GetTimeZoneOffset(Tcl_Interp *interp, Tcl_Obj *objPtr, int *offsetPtr)
{
    char buffer[8];
    const char *p;
    int sign, hours, minutes, seconds;
    int count;
    
    sign = -1;
    p = Tcl_GetString(objPtr);
    if ((*p == '+') || (*p == '-')) {
        if (*p == '-') {
            sign = -1;
        }
        p++;
    }
    for (count = 0; (*p != '\0') || (count > 7); p++) {
        if (isdigit(*p)) {
            buffer[count] = *p;
            count++;
        } else if (*p != ':') {
            break;                      /* Stop if it's not a digit or
                                         * colon. */
        }
    }
    hours = seconds = minutes = 0;
    buffer[count] = '\0';
    switch (count) {
    case 6:
        seconds = (buffer[4] - '0') * 10 + (buffer[5] - '0');
        /* fallthru */
    case 4:                             /* Hours and minutes */
        minutes = (buffer[2] - '0') * 10  + (buffer[3] - '0');
        /* fallthru */
    case 2:                             /* Hours only */
        hours =   (buffer[0] - '0') * 10  + (buffer[1] - '0');
        break;
    default:
        Tcl_AppendResult(interp, "unknown timezone string \"",
                Tcl_GetString(objPtr), "\"", (char *)NULL);
        return TCL_ERROR;
    }
    *offsetPtr = sign * ((hours * SECONDS_HOUR) +
                         (minutes * SECONDS_MINUTE) +
                         seconds);
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * GetTimeZoneOffsets --
 *
 *      Converts a string in the format "+/-hh:mm:ss +/-hh:mm:ss" that
 *      represent the standard and daylight offsets of the timezone.
 *      
 * Results:
 *      A standard TCL result.  The offsets are returned via *zonePtr*.
 *
 *---------------------------------------------------------------------------
 */
static int
GetTimeZoneOffsets(Tcl_Interp *interp, Tcl_Obj *objPtr, TimeZone *zonePtr)
{
    Tcl_Obj **objv;
    int objc;
    int result;

    if (Tcl_ListObjGetElements(interp, objPtr, &objc, &objv) != TCL_OK) {
        return TCL_ERROR;
    }
    if (objc != 2) {
        return TCL_ERROR;
    }
    result = GetTimeZoneOffset(interp, objv[0], &zonePtr->std);
    if (result == TCL_OK) {
        result = GetTimeZoneOffset(interp, objv[1], &zonePtr->dst);
    }
    return TCL_OK;
}

static Tcl_Obj *
FindTimeZone(Tcl_Interp *interp, const char *string, int length)
{
    const char *copy;
    char buffer[64];
    Tcl_Obj *objPtr;
    
    if (length < 0) {
        length = strlen(string);
    }
    if (length < 64) {
        copy = buffer;
        strncpy(buffer, string, length);
        buffer[length] = '\0';
    } else {
        copy = Blt_Strndup(string, length);
    }
    /* Don't leave an error message if you don't find the timezone. */
    objPtr = Tcl_GetVar2Ex(interp, "blt::timezones", copy, 0);
    if (objPtr == NULL) {
        /* Try it again, all uppercase. */
        Blt_UpperCase((char *)copy);
        objPtr = Tcl_GetVar2Ex(interp, "blt::timezones", copy, 0);
    }
    if (copy != buffer) {
        Blt_Free(copy);
    }
    return objPtr;
}

static int
GetTwoDigitCentury(Tcl_Interp *interp)
{
    int century;
    Tcl_Obj *objPtr;
    
    /* Don't leave an error message if you don't find the variable. */
    objPtr = Tcl_GetVar2Ex(interp, "blt::timestamp", "century", 0);
    if (objPtr == NULL) {
        return 1900;
    }
    if (Tcl_GetIntFromObj(interp, objPtr, &century) != TCL_OK) {
        return 1900;
    }
    if (century < 1) {
        return 1900;
    }
    if (century < 100) {
        return (century - 1) * 100;
    }
    return century;
}

/* 
 *-----------------------------------------------------------------------------
 *
 * InitParser --
 *
 *      Initializes the data structure used in parsing date/time strings.
 *
 * Returns:
 *      None.
 *
 * Side Effects:
 *      Initialized the parser date structure.
 *
 *-----------------------------------------------------------------------------
 */
static void
InitParser(Tcl_Interp *interp, TimeStampParser *parserPtr, const char *string) 
{
    memset(parserPtr, 0, sizeof(TimeStampParser));
    parserPtr->nextCharPtr = parserPtr->buffer = string; 
    /* Default date is the EPOCH which is 0 seconds. */
    parserPtr->date.year = EPOCH;
    parserPtr->date.mday = 1;
}

/* 
 *-----------------------------------------------------------------------------
 *
 * FreeParser --
 *
 * Returns:
 *      None.
 *
 *-----------------------------------------------------------------------------
 */
static void
FreeParser(TimeStampParser *parserPtr) 
{

}

/* 
 *-----------------------------------------------------------------------------
 *
 * ParseError --
 *
 *      Formats and saves a formatted error message in the interpreter
 *      result.
 *
 * Returns:
 *      None.
 *
 *-----------------------------------------------------------------------------
 */
static void
ParseError(Tcl_Interp *interp, const char *fmt, ...)
{
    char string[BUFSIZ+4];
    int length;
    va_list args;

    va_start(args, fmt);
    length = vsnprintf(string, BUFSIZ, fmt, args);
    if (length > BUFSIZ) {
        strcat(string, "...");
    }
    Tcl_AppendResult(interp, string, (char *)NULL);
    va_end(args);
}

static void
ParseWarning(Tcl_Interp *interp, const char *fmt, ...)
{
    char string[BUFSIZ+4];
    int length;
    va_list args;

    va_start(args, fmt);
    length = vsnprintf(string, BUFSIZ, fmt, args);
    if (length > BUFSIZ) {
        strcat(string, "...");
    }
    Tcl_ResetResult(interp);
    Tcl_AppendResult(interp, string, (char *)NULL);
    va_end(args);
}

/* 
 *-----------------------------------------------------------------------------
 *
 * DeleteToken --
 *
 *      Removes the token from the list of tokens.  We don't actually
 *      free the memory. That happens when the parser is freed.
 *
 * Returns:
 *      None.
 *
 *-----------------------------------------------------------------------------
 */
static void
DeleteToken(TimeStampParser *parserPtr, ParserToken *tokenPtr) 
{
    int unlinked;                       /* Indicates if the link is
                                         * actually removed from the
                                         * list. */
    unlinked = FALSE;
    if (parserPtr->headPtr == tokenPtr) {
        parserPtr->headPtr = tokenPtr->nextPtr;
        unlinked = TRUE;
    }
    if (parserPtr->tailPtr == tokenPtr) {
        parserPtr->tailPtr = tokenPtr->prevPtr;
        unlinked = TRUE;
    }
    if (tokenPtr->nextPtr != NULL) {
        tokenPtr->nextPtr->prevPtr = tokenPtr->prevPtr;
        unlinked = TRUE;
    }
    if (tokenPtr->prevPtr != NULL) {
        tokenPtr->prevPtr->nextPtr = tokenPtr->nextPtr;
        unlinked = TRUE;
    }
    if (unlinked) {
        parserPtr->numTokens--;
    }
    tokenPtr->prevPtr = tokenPtr->nextPtr = NULL;
}

/* 
 *-----------------------------------------------------------------------------
 *
 * ParseNumber --
 *
 *      Parse the given string as a number.
 *
 * Returns:
 *      Returns the token _NUMBER if successful, _END otherwise.
 *
 *-----------------------------------------------------------------------------
 */
static int
ParseNumber(Tcl_Interp *interp, const char *string, ParserToken *tokenPtr)
{
    const char *p;
    long lvalue;
    int length, result;
    Tcl_Obj *objPtr;

    p = string;
    while (isdigit(*p)) {
        p++;
    }
    length = p - string;
    objPtr = Tcl_NewStringObj(string, length);
    Tcl_IncrRefCount(objPtr);
    result = Blt_GetLongFromObj(interp, objPtr, &lvalue);
    Tcl_DecrRefCount(objPtr);
    if (result != TCL_OK) {
        if (interp != NULL) {
            ParseError(interp, "error parsing \"%*s\" as number", 
                       length, string);
        }
        return TCL_ERROR;
    }
    tokenPtr->lvalue = lvalue;
    tokenPtr->string = string;
    tokenPtr->length = p - string;
    tokenPtr->id = _NUMBER;
    return TCL_OK;
}

/* 
 *-----------------------------------------------------------------------------
 *
 * ParseString --
 *
 *      Parses the given string and tries to match against different kinds
 *      of identifiers to determine the ID. Months, weekdays, timezones, and
 *      meridian can be distinctly identified.  
 *
 * Returns:
 *      Returns the appropiate token.
 *
 *-----------------------------------------------------------------------------
 */
static int
ParseString(Tcl_Interp *interp, TimeStampParser *parserPtr, int length,
            const char *string)
{
    ParserToken *tokenPtr;
    char c;
    int i;
    Tcl_Obj *objPtr;
    
    c = tolower(string[0]);
    tokenPtr = parserPtr->curTokenPtr;
    /* Step 1. Test of month and weekday names (may be abbreviated). */
    if (length >= 3) {
        /* Test for months. Allow abbreviations greater than 2 characters. */
        for (i = 0; i < numMonths; i++) {
            const char *p;

            p = monthNames[i];
            if ((c == tolower(*p)) && (strncasecmp(p, string, length) == 0)) {
                tokenPtr->lvalue = i + 1;      /* Month 1-12 */
                tokenPtr->string = p;
                tokenPtr->length = 0;
                tokenPtr->id = _MONTH;
                return TCL_OK;
            }
        }
        /* Test for weekdays. Allow abbreviations greater than 2 characters. */
        for (i = 0; i < numWeekdays; i++) {
            const char *p;

            p = weekdayNames[i];
            if ((c == tolower(*p)) && (strncasecmp(p, string, length) == 0)) {
                tokenPtr->lvalue = i + 1;      /* Weekday 1-7 */
                tokenPtr->length = 0;
                tokenPtr->string = p;
                tokenPtr->id = _WDAY;
                return TCL_OK;
            }
        }
    }

    /* Step 2. Test for DST string. */
    if ((length == 3) && (strncasecmp(string, "DST", length) == 0)) {
        tokenPtr->lvalue = 0;
        tokenPtr->string = "DST";
        tokenPtr->length = 3;
        tokenPtr->id = _DST;
        parserPtr->date.isdst = TRUE;
        parserPtr->flags |= PARSE_DST;
        return TCL_OK;
    }
    
    /* Step 3. Test for meridian: am or pm. */
    if (length == 2) {
        int i;
        
        /* Test of meridian. */
        for (i = 0; i < numMeridians; i++) {
            const char *p;
            
            p = meridianNames[i];
            if ((c == *p) && (strncasecmp(p, string, length) == 0)) {
                tokenPtr->lvalue = i;
                tokenPtr->length = 2;
                tokenPtr->id = _AMPM;
                return TCL_OK;
            }
        }
    }
    /* Step 5. Test for timezone name. Check against because there may
     *         have been a number, -, /, or _ next to it. */
    objPtr = FindTimeZone(interp, string, length);
    if (objPtr != NULL) {
        tokenPtr->objPtr = objPtr;
        Tcl_IncrRefCount(objPtr);
        tokenPtr->lvalue = length;
        tokenPtr->string = string;
        tokenPtr->length = length;
        tokenPtr->id = _TZ;
        parserPtr->flags |= PARSE_TZ;
        return TCL_OK;
    }
    tokenPtr->string = string;
    tokenPtr->length = length;
    tokenPtr->id = _UNKNOWN;
    return TCL_ERROR;
}

/* 
 *-----------------------------------------------------------------------------
 *
 * ParseTimeZone --
 *
 *      Parses the given string and tries to match against timezones.
 *
 * Returns:
 *      Returns the appropiate token.
 *
 *-----------------------------------------------------------------------------
 */
static int
ParseTimeZone(Tcl_Interp *interp, TimeStampParser *parserPtr, int length,
              const char *string)
{
    ParserToken *tokenPtr;
    Tcl_Obj *objPtr;
    
    tokenPtr = parserPtr->curTokenPtr;
    objPtr = FindTimeZone(interp, string, length);
    if (objPtr != NULL) {
        tokenPtr->objPtr = objPtr;
        Tcl_IncrRefCount(objPtr);
        tokenPtr->lvalue = length;
        tokenPtr->string = string;
        tokenPtr->length = length;
        tokenPtr->id = _TZ;
        parserPtr->flags |= PARSE_TZ;
        return TCL_OK;
    }
    tokenPtr->string = string;
    tokenPtr->length = length;
    tokenPtr->id = _UNKNOWN;
    return TCL_ERROR;
}

static ParserToken *
FindFirstToken(TimeStampParser *parserPtr, int id)
{
    ParserToken *tokenPtr;
    
    for (tokenPtr = parserPtr->headPtr; tokenPtr != NULL;
         tokenPtr = tokenPtr->nextPtr) {
        if (tokenPtr->id == id) {
            return tokenPtr;
        }
    }
    return NULL;
}

/* 
 *-----------------------------------------------------------------------------
 *
 * NumberTokens --
 *
 *      Returns the number of tokens left in the list of tokens.
 *
 *-----------------------------------------------------------------------------
 */
static INLINE int
NumberTokens(TimeStampParser *parserPtr) 
{
    return parserPtr->numTokens;
}

/* 
 *-----------------------------------------------------------------------------
 *
 * TokenSymbol --
 *
 *      Returns the symbol name of the given token.
 *
 *-----------------------------------------------------------------------------
 */
static const char *
TokenSymbol(ParserToken *t) 
{
    if (t == NULL) {
        return tokenNames[_END];
    } 
    return tokenNames[t->id];
}


static Tcl_Obj *
PrintTokens(Tcl_Interp *interp, TimeStampParser *parserPtr) 
{
    ParserToken *tokenPtr;
    Tcl_Obj *listObjPtr;
    
    listObjPtr = Tcl_NewListObj(0, (Tcl_Obj **)NULL);
    for (tokenPtr = parserPtr->headPtr; tokenPtr != NULL;
         tokenPtr = tokenPtr->nextPtr) {
        Tcl_Obj *objPtr;

        objPtr = Tcl_NewStringObj(TokenSymbol(tokenPtr), -1);
        Tcl_ListObjAppendElement(interp, listObjPtr, objPtr);
    }
    return listObjPtr;
}

/*
 *---------------------------------------------------------------------------
 *
 * AppendToken --
 *
 *      Inserts a link after another link.  If afterPtr is NULL, then the
 *      new link is prepended to the beginning of the list.
 *
 * Results:
 *      None.
 *
 *---------------------------------------------------------------------------
 */
static void
AppendToken(TimeStampParser *parserPtr, ParserToken *tokenPtr)
{
    if (parserPtr->headPtr == NULL) {
        parserPtr->tailPtr = parserPtr->headPtr = tokenPtr;
    } else {
        tokenPtr->nextPtr = NULL;
        tokenPtr->prevPtr = parserPtr->tailPtr;
        if (parserPtr->tailPtr != NULL) {
            parserPtr->tailPtr->nextPtr = tokenPtr;
        }
        parserPtr->tailPtr = tokenPtr;
    }
    parserPtr->numTokens++;
}

/*
 *---------------------------------------------------------------------------
 *
 * AllocToken --
 *
 *      Inserts a link after another link.  If afterPtr is NULL, then the
 *      new link is appended to the end of the list.
 *
 * Results:
 *      None.
 *
 *---------------------------------------------------------------------------
 */
static ParserToken *
AllocToken(TimeStampParser *parserPtr)
{
    ParserToken *tokenPtr;

    tokenPtr = parserPtr->tokens + parserPtr->nextFreeToken;
    parserPtr->nextFreeToken++;
    AppendToken(parserPtr, tokenPtr);
    return tokenPtr;
}
    
/* 
 *-----------------------------------------------------------------------------
 *
 * GetNextToken --
 *
 *      Parses the next token from the date string.
 *
 *-----------------------------------------------------------------------------
 */
static int
GetNextToken(Tcl_Interp *interp, TimeStampParser *parserPtr)
{
    const char *p;
    ParserToken *tokenPtr;

    tokenPtr = AllocToken(parserPtr);
    parserPtr->curTokenPtr = tokenPtr;
    p = parserPtr->nextCharPtr;

    while (isspace(*p)) {
        p++;                            /* Skip leading spaces. */
    }

    /* Handle single character tokens. */
    switch (*p) {
    case '/':
        tokenPtr->id = _SLASH;
        break;
    case '+':
        tokenPtr->id = _PLUS;
        break;
    case '\'':                          /* Single quote */
        tokenPtr->id = _QUOTE;
        break;
    case ':':
        tokenPtr->id = _COLON;
        break;
    case '.':
        tokenPtr->id = _DOT;
        break;
    case ',':
        tokenPtr->id = _COMMA;
        break;
    case '-':
        tokenPtr->id = _DASH;
        break;
    case '(':
        tokenPtr->id = _LPAREN;
        break;
    case ')':
        tokenPtr->id = _RPAREN;
        break;
    default:
        goto next;
    }
    parserPtr->nextCharPtr = p + 1;
    return TCL_OK;

 next:
    /* Week number Wnn. */
    if ((*p == 'W') && (isdigit(*(p+1))) && (isdigit(*(p+2)))) {
        tokenPtr->string = p;
        tokenPtr->lvalue = (p[1] - '0') * 10 + (p[2] - '0');
        tokenPtr->id = _WEEK;           /* Www week number. */
        tokenPtr->length = 3;
        p += 3;
    } else if (isdigit(*p)) {
        const char *first;
        char save, *q;
        
        first = p;
        while (isdigit(*p)) {
            p++;
        }
        q = (char *)p;
        save = *p;
        *q = '\0';
        if (ParseNumber(interp, first, tokenPtr) != TCL_OK) {
            Tcl_AppendResult(interp, "unknown token found", (char *)NULL);
            return TCL_ERROR;
        }
        *q = save;                      /* Restore last chararacter. */
        if (*p != '\0') {
            char c, d;

            c = tolower(p[0]);
            d = tolower(p[1]);
            if (((c == 't') && (d == 'h')) || /* 9th */
                ((c == 's') && (d == 't')) || /* 1st */
                ((c == 'n') && (d == 'd')) || /* 2nd */
                ((c == 'r') && (d == 'd'))) { /* 3rd */
                p += 2;
                tokenPtr->id = _MDAY;       /* Month day. */
            }
        }
    } else if (isalpha(*p)) {
        const char *name;
        
        /* Special parsing rules for timezones.  They start with a letter
         * and may contain one or more letters, digits underscores,
         * slashes, dashes, or periods. */
        name = p;
        for (/*empty*/; *p != '\0'; p++) {
            if ((!isalnum(*p)) && (*p != '_') && (*p != '-') && (*p != '/')) {
                break;
            }
        }
        if (ParseTimeZone(interp, parserPtr, p - name, name) != TCL_OK) {
            /* String identifiers contain only letters. */
            p = name;
            for (p = name; *p != '\0'; p++) {
                if (!isalpha(*p)) {
                    break;
                }
            }
            ParseString(interp, parserPtr, p - name, name);
        }
    } else if (*p == '\0') {
        tokenPtr->id = _END;
        p++;
    } else {
        tokenPtr->id = _UNKNOWN;
        p++;
    }
    parserPtr->nextCharPtr = p;
    return TCL_OK;
}

/* 
 *-----------------------------------------------------------------------------
 *
 * ProcessTokens --
 *
 *      Parses the date string into a list of tokens.  This lets us
 *      look forward and backward as tokens to discern different 
 *      patterns.
 *
 *-----------------------------------------------------------------------------
 */
static int
ProcessTokens(Tcl_Interp *interp, TimeStampParser *parserPtr)
{
    do {
        if (parserPtr->nextFreeToken >= MAXTOKENS) {
            Tcl_AppendResult(interp, "too many tokens found in \"",
                             parserPtr->buffer, "\"", (char *)NULL);
            return TCL_ERROR;
        }
        if (GetNextToken(interp, parserPtr) != TCL_OK) {
            return TCL_ERROR;
        }
    } while (parserPtr->curTokenPtr->id != _END);
    return TCL_OK;
}

/* 
 *-----------------------------------------------------------------------------
 *
 * MatchDatePattern --
 *
 *      After having extracted the time, timezone, etc. from the list of
 *      tokens, try to match the remaining tokens with known date patterns.
 *
 * Returns:
 *      Return the index of the matching pattern or -1 if no pattern
 *      matches.
 *
 *-----------------------------------------------------------------------------
 */
static int
MatchDatePattern(Tcl_Interp *interp, TimeStampParser *parserPtr)
{
    int i;

    /* Find the first colon. */
    for (i = 0; i < numDatePatterns; i++) {
        int j;
        ParserToken *t;
        Pattern *patPtr;

        patPtr = datePatterns + i;
        if (patPtr->numIds != NumberTokens(parserPtr)) {
            continue;
        }
        for (j = 0, t = parserPtr->headPtr; 
             (t != NULL) && (j < patPtr->numIds); t = t->nextPtr, j++) {
            ParserTokenId id;

            id = patPtr->ids[j];
            if (t->id != id) {
                if ((t->id == _NUMBER) && (t->length <= 2)){
                    /* One or two digit number. */
                    switch (id) {
                    case _WDAY:
                        if ((t->lvalue > 0) && (t->lvalue <= 7)) {
                            continue;
                        }
                        ParseWarning(interp, "weekday \"%d\" is out of range",
                                t->lvalue);
                        break;

                    case _MONTH:
                        if ((t->lvalue > 0) && (t->lvalue <= 12)) {
                            continue;
                        }
                        ParseWarning(interp, "month \"%d\" is out of range",
                                t->lvalue);
                        break;

                    case _MDAY:
                        if ((t->lvalue > 0) && (t->lvalue <= 31)) {
                            continue;
                        }
                        ParseWarning(interp, "day \"%d\" is out of range",
                                t->lvalue);
                        break;

                    case _YEAR:
                        continue;

                    default:
                        break;
                    }
                }
                break;
            }
            if (id == _END) {
                return i;
            }
        }
    }
    return -1;
}

/* 
 *-----------------------------------------------------------------------------
 *
 * ExtractDST --
 *
 *      Finds and extracts the DST token.  This token indicates that the
 *      timezone should be offset by one hour for daylight time.
 *
 *      For example: "America/New_York  DST" is the same as "EDT"
 *
 * Returns:
 *      None.
 *
 * Side Effects:
 *      The DST token is remove from the list of tokens.
 *
 *-----------------------------------------------------------------------------
 */
static void
ExtractDST(TimeStampParser *parserPtr)
{
    ParserToken *t;

    t = FindFirstToken(parserPtr, _DST);
    if (t != NULL) {
        parserPtr->date.isdst = TRUE;
        DeleteToken(parserPtr, t);
    }
}

/* 
 *-----------------------------------------------------------------------------
 *
 * ExtractTimeZoneAndOffset --
 *
 *      Searches and removes (if found) the timezone token from the parser
 *      list.  The timezone name (different from the timezone offset)
 *      can't be confused with other parts of the date and time.
 *
 *      We only extract the offset is we find a timezone.  Otherwise
 *      we'll look for it later after the time.
 *
 *      Need to handle the following cases.
 *      1) "EST" or "America/New_York"    (Eastern Standard Time)
 *      2) "UTC-0500"   
 *      3) "America/New_York DST          (Eastern Daylight Time) OK
 *      4) "US/Pacific PDT                (Pacific Daylight Time) 
 *      5) -05:00                         (Eastern Standard Time)
 *
 *      Weird cases:
 *      1) EST-0300                       (Pacific Standard Time)
 *      2) EDT DST                        (invalid)
 *      3) US/Pacific EDT                 (invalid)
 *      
 * Returns:
 *      A standard TCL result.  Always TCL_OK.
 *
 * Side Effects:
 *      The *tzoffset* and *isdst* fields are set if timezone tokens
 *      are found.
 *
 *-----------------------------------------------------------------------------
 */
static int
ExtractTimeZoneAndOffset(Tcl_Interp *interp, TimeStampParser *parserPtr,
                         ParserToken *tzPtr, ParserToken **nextPtrPtr)
{
    ParserToken *t, *end, *next;
    int tzoffset, offset, sign, allowNNNN;
    TimeZone tz;
    
    /* Process the timezone offset. */

    /* 
     * Check for a timezone offset following the timezone (Z07:00) 
     *   +-NNNN   plus/minus + 4 digit token
     *   +-NN:NN  plus/minus + 2 digit token + colon + 2 digit token
     *   NN:NN    plus/minus + 2 digit token + colon + 2 digit token
     *
     * The NN:NN is peculiar to Go.  We can't allow NNNN because this
     * gets confused with 4 digit years.
     */
    end = t = tzPtr->nextPtr;
    offset = 0;
    sign = 1;
    allowNNNN = FALSE;
    if ((t->id == _PLUS) || (t->id == _DASH)) {
        sign = (t->id == _DASH) ? -1 : 1;
        t = t->nextPtr;
        allowNNNN = TRUE;
    }
    if (t->id == _NUMBER) {
        parserPtr->flags |= PARSE_OFFSET;
        /* Looking for NN:NN or NNNN */
        if ((t->length == 4) && (allowNNNN)) {
            offset = TZOFFSET(t->lvalue);
            end = t->nextPtr;
        } else if (t->length < 3) {
            offset = t->lvalue * SECONDS_HOUR;
            t = t->nextPtr;
            if (t->id != _COLON) {
                goto found;             /* Only found NN */
            }
            t = t->nextPtr;
            if ((t->id != _NUMBER) || (t->length > 2)) {
                Tcl_AppendResult(interp, "bad token: expecting a number",
                        (char *)NULL);
                return TCL_ERROR;       /* The token following the colon
                                         * isn't a 2 digit number.  */
            }
            offset += t->lvalue * SECONDS_MINUTE;
            end = t->nextPtr;
        } 
    }
 found:
    if (GetTimeZoneOffsets(interp, tzPtr->objPtr, &tz) != TCL_OK) {
        return TCL_ERROR;
    }
    Tcl_DecrRefCount(tzPtr->objPtr);
    tzoffset = (parserPtr->date.isdst) ? -tz.dst : -tz.std;
    parserPtr->date.tzoffset = tzoffset + (sign * offset);
    parserPtr->flags |= PARSE_TZ;

    /* Remove the timezone and timezone offset tokens. */
    for (t = tzPtr; t != end; t = next) {
        next = t->nextPtr;
        DeleteToken(parserPtr, t);
    }
    *nextPtrPtr = end;
    return TCL_OK;
}

/* 
 *-----------------------------------------------------------------------------
 *
 * ExtractTimeZoneAndOffset --
 *
 *      Searches and removes (if found) the timezone tokens from the parser
 *      list.  The timezone name (different from the timezone offset) can't
 *      be confused with other parts of the date and time.
 *
 *      This handles the cases like 
 *
 *              2007-09-02 12:00 America/Los_Angeles PST
 *
 *      We let the last timezone found win.  Ideally "America/Los_Angeles"
 *      should let us distinguish between Pacific Standard Time (USA) and
 *      Pakistan Standard Time.  
 *
 *-----------------------------------------------------------------------------
 */
static int
ExtractTimeZonesAndOffset(Tcl_Interp *interp, TimeStampParser *parserPtr)
{
    ParserToken *tokenPtr, *nextPtr;
    
    /* Now parse out the timezone, time, and date. */
    for (tokenPtr = parserPtr->headPtr; tokenPtr != NULL; tokenPtr = nextPtr) {
        if (tokenPtr->id == _TZ) {
            int result;

            result = ExtractTimeZoneAndOffset(interp, parserPtr, tokenPtr,
                &nextPtr);
            if (result != TCL_OK) {
                return result;
            }
        } else {
            nextPtr = tokenPtr->nextPtr;
        }
    }
    return TCL_OK;
}

/* 
 *-----------------------------------------------------------------------------
 *
 * ParseTimeZoneOffset --
 *
 *      Parses the group of tokens forming a timezone offset.  The 
 *      offset may be in any of the following forms:
 *
 *      +-NNNN   plus/minus + 4 digit token
 *      +-NN:NN  plus/minus + 2 digit token + colon + 2 digit token
 *      NN:NN    plus/minus + 2 digit token + colon + 2 digit token
 *
 *      The NN:NN is peculiar to Go.  We can't allow NNNN because this gets
 *      confused with 4 digit years.  If you what to use NNNN, then
 *      prefix is with a plus sign.
 *      
 * Returns:
 *      A standard TCL result.  
 *
 * Side Effects:
 *      The *tzoffset* and *isdst* fields are set if timezone tokens
 *      are found.
 *
 *-----------------------------------------------------------------------------
 */
static int
ParseTimeZoneOffset(Tcl_Interp *interp, TimeStampParser *parserPtr,
                    ParserToken *tokenPtr, ParserToken **nextPtrPtr)
{
    int sign, offset;

    sign = (tokenPtr->id == _DASH) ? 1 : -1;

    /* Verify the next token after the +/-/' is a number.  */
    tokenPtr = tokenPtr->nextPtr;
    if (tokenPtr->id != _NUMBER) {
        if (interp != NULL) {
            Tcl_AppendResult(interp, "bad token in timezone: not a number",
                         (char *)NULL);
        }
        return TCL_ERROR;
    }
    /* The timezone is in the form NN:NN or NNNN. */
    if (tokenPtr->length == 4) {
        offset = TZOFFSET(tokenPtr->lvalue);
    } else if (tokenPtr->length < 3) {
        offset = tokenPtr->lvalue * SECONDS_HOUR;
        tokenPtr = tokenPtr->nextPtr;
        if (tokenPtr->id != _COLON) {
            goto found;
        }
        tokenPtr = tokenPtr->nextPtr;
        if ((tokenPtr->id != _NUMBER) || (tokenPtr->length > 2)) {
            if (interp != NULL) {
                Tcl_AppendResult(interp, "bad token in timezone: not a number",
                                 (char *)NULL);
            }
            return TCL_ERROR;
        }
        offset += tokenPtr->lvalue * SECONDS_MINUTE;
    } else {
        if (interp != NULL) {
            Tcl_AppendResult(interp, "bad token in timezone: not a number",
                         (char *)NULL);
        }
        return TCL_ERROR;               /* Error: expecting 2 digit or 4
                                         * digit number after plus or
                                         * minus. */
    }
    tokenPtr = tokenPtr->nextPtr;
 found:
    parserPtr->flags |= PARSE_OFFSET;
    *nextPtrPtr = tokenPtr;
    /* Subtract or add the offset to the current timezone offset. */
    parserPtr->date.tzoffset += sign * offset;
    parserPtr->flags |= PARSE_TZ;
    return TCL_OK;
}

/* 
 *-----------------------------------------------------------------------------
 *
 * ExtractWeekday --
 *
 *      Searches for a weekday token and removes it from the parser token
 *      list.  The weekday would have been specified as a weekday name such
 *      as "Tuesday".  Also remove a following comma if one exists.
 *      Tuesday. We don't use the weekday. The weekday is always computed
 *      from the year day. No checking is done to see if these match.
 * 
 * Returns: None.
 *
 * Side Effects:
 *      The *wday* field is  set if a weekday token is found.
 *
 *-----------------------------------------------------------------------------
 */
static void
ExtractWeekday(TimeStampParser *parserPtr)
{
    ParserToken *tokenPtr;

    tokenPtr = FindFirstToken(parserPtr, _WDAY);
    if (tokenPtr != NULL) {
        ParserToken *nextPtr;

        parserPtr->date.wday = tokenPtr->lvalue - 1; /* 0-6 */
        nextPtr = tokenPtr->nextPtr;
        DeleteToken(parserPtr, tokenPtr);
        if ((nextPtr->id == _COMMA) || (nextPtr->id == _DOT)) {
            DeleteToken(parserPtr, nextPtr);
        }
    }
}

/* 
 *-----------------------------------------------------------------------------
 *
 * ExtractYear --
 *
 *      Searches for a year token and removes it from the parser list.
 *      This simplifies the number of possible date patterns.
 *
 * Returns:
 *      None.
 *
 * Side Effects:
 *      The *wday* field is  set if a weekday token is found.
 *
 *-----------------------------------------------------------------------------
 */
static void
ExtractYear(TimeStampParser *parserPtr)
{
    ParserToken *tokenPtr;

    tokenPtr = FindFirstToken(parserPtr, _YEAR);
    if (tokenPtr != NULL) {
        parserPtr->date.year = tokenPtr->lvalue;
        if (tokenPtr->length == 2) {
            
            parserPtr->date.year += 1900;
        }
        DeleteToken(parserPtr, tokenPtr);
    }
}

/* 
 *-----------------------------------------------------------------------------
 *
 * ParseTimeZoneOffset --
 *
 *      ISO format allows for the date and time to be separated by a "T".
 *      This can get confused with the military time zone.  It's assumed
 *      that if the "T" is the last token, it is probably a timezone
 *      designation, not a separator.  If we find the separator, remove it.
 *      
 * Returns:
 *      None.
 *
 * Side Effects:
 *      The separator (as TZ token) is removed from the parser's list
 *      of tokens.
 *
 *-----------------------------------------------------------------------------
 */
static void
ExtractDateTimeSeparator(TimeStampParser *parserPtr)
{
    ParserToken *tokenPtr;

    /* Find the date/time separator "t" and remove it. */
    for (tokenPtr = parserPtr->headPtr; tokenPtr != NULL;
         tokenPtr = tokenPtr->nextPtr) {
        char c;

        if (tokenPtr->id != _TZ) {
            continue;
        }
        c = tolower(tokenPtr->string[0]);
        if ((tokenPtr->length == 1) && (c == 't')) {
            /* Check if the 't' is a military timezone or a separator. */
            if (tokenPtr->nextPtr->id != _END) {
                DeleteToken(parserPtr, tokenPtr);
            }
            return;
        }
    }
}

/*
 *---------------------------------------------------------------------------
 *
 * FixParserTokens --
 *
 *      Convert 3, 4, and 8 digit number token IDs to more specific IDs.
 *      This is done for pattern matching. 
 *
 *      3 digit number          oridinal day of year (ddd)
 *      4 digit number          year (yyyy)
 *      7 digit number          year (yyyyddd)
 *      8 digit number          ISO date format (yyyymmdd)
 *
 * Results:
 *      None.
 *
 *---------------------------------------------------------------------------
 */
static void
FixParserTokens(TimeStampParser *parserPtr)
{
    ParserToken *tokenPtr, *next;

    for (tokenPtr = parserPtr->headPtr; tokenPtr != NULL; tokenPtr = next) {
        next = tokenPtr->nextPtr;
        switch (tokenPtr->id) {
        case _NUMBER:
            if (tokenPtr->length == 2) {
                if (tokenPtr->lvalue > 31) {
                    tokenPtr->id = _YEAR;
                }
            } else if (tokenPtr->length == 3) {
                tokenPtr->id = _YDAY;
            } else if ((tokenPtr->length == 4) || (tokenPtr->length == 5)) {
                tokenPtr->id = _YEAR;
            } else if (tokenPtr->length == 7) {
                tokenPtr->id = _ISO7;
            } else if (tokenPtr->length == 8) {
                tokenPtr->id = _ISO8;
            }
            break;
        case _PLUS:
        case _DASH:
        case _SLASH:
        case _COMMA:
        case _COLON:
        case _LPAREN:
        case _RPAREN:
            /* Remove date separator tokens. */
            DeleteToken(parserPtr, tokenPtr);
            break;
        default:
            break;
        }
    }
}

static ParserToken *
FindTimeSequence(ParserToken *tokenPtr, ParserToken **tokens)
{
    tokens[0] = tokens[1] = tokens[2] = NULL;

    /* Find the pattern "NN [:.] NN"  or "NN [:.] NN [:.] NN". */
    if ((tokenPtr->id != _NUMBER) || (tokenPtr->length > 2) ||
        (tokenPtr->lvalue > 23)) {
        return NULL;                    /* Not a valid hour spec. */
    }
    if ((tokenPtr->prevPtr != NULL) &&
        ((tokenPtr->prevPtr->id == _DOT) || tokenPtr->prevPtr->id == _COLON)) {
        return NULL;                    /* Previous token can't be a
                                         * dot. */
    }
    tokens[0] = tokenPtr;               /* Save hour token */
    tokenPtr = tokenPtr->nextPtr;
    if ((tokenPtr->id != _COLON) && (tokenPtr->id != _DOT)) {
        return NULL;                    /* Must be separated by : or . */
    }
    tokenPtr = tokenPtr->nextPtr;
    if ((tokenPtr->id != _NUMBER) || (tokenPtr->length > 2) ||
        (tokenPtr->lvalue > 59)) {
        return NULL;                    /* Not a valid minute spec. */
    }
    tokens[1] = tokenPtr;               /* Save the minute token. */
    tokenPtr = tokenPtr->nextPtr;
    if ((tokenPtr->id != _COLON) && (tokenPtr->id != _DOT)) {
        return tokenPtr;                /* No second spec. */
    }
    tokenPtr = tokenPtr->nextPtr;
    if ((tokenPtr->id != _NUMBER) || (tokenPtr->length > 2) ||
        (tokenPtr->lvalue > 60)) {
        return NULL;                    /* Not a valid second spec. */
    }
    tokens[2] = tokenPtr;               /* Save second token. */
    return tokenPtr->nextPtr;
}

/*
 *---------------------------------------------------------------------------
 *
 * ExtractTime --
 *
 *      Find and extract the time related tokens in the list.  
 *      The time format is
 *
 *              time + ampm + timezone
 *
 *      where ampm and timezone are optional.  It's easier to extract
 *      the meridian and timezone once we already found a time.
 *
 *      The following time patterns are accepted:
 *
 *      hh:mm                                           10:21
 *      hh:mm:ss                                        10:21:00
 *      hh:mm:ss:fff    fff is milliseconds.            10:21:00:001
 *      hh:mm:ss.f+     f+ is fraction of seconds.      10:21:00.001
 *      hh:mm:ss,f+     f+ is fraction of seconds.      10:21:00,001
 *      hhmmss          ISO time format                 102100
 *
 * Results:
 *      None.
 *
 * Side Effects:
 *      The matching tokens are removed from the list and the parser
 *      structure is updated with the new information.
 *
 *---------------------------------------------------------------------------
 */
static int
ExtractTime(Tcl_Interp *interp, TimeStampParser *parserPtr)
{
    ParserToken *next, *first, *last;
    ParserToken *tokenPtr, *tokens[3];

#if DEBUG
    fprintf(stderr, "ExtractTime (%s)\n", 
            Tcl_GetString(PrintTokens(interp, parserPtr)));
#endif
    first = last = NULL;

    for (tokenPtr = parserPtr->headPtr; tokenPtr != NULL;
         tokenPtr = tokenPtr->nextPtr) {
        first = tokenPtr;
        /* Assume that any 6-digit number is ISO time format (hhmmss). */
        if ((tokenPtr->id == _NUMBER) && (tokenPtr->length == 6)) {
            long value;

            value = tokenPtr->lvalue;
            parserPtr->date.sec = value % 100;
            value /= 100;
            parserPtr->date.min = value % 100;
            value /= 100;
            parserPtr->date.hour = value % 100;
            first = tokenPtr;
            next = tokenPtr = tokenPtr->nextPtr;
            goto done;
        }
        next = FindTimeSequence(tokenPtr, tokens);
        if (next != NULL) {
            first = tokenPtr;
            break;                      /* Found the time pattern */
        }
    }
    if (tokenPtr == NULL) {
        /* Could not window a time sequence. */
        for (tokenPtr = parserPtr->headPtr; tokenPtr != NULL;
             tokenPtr = tokenPtr->nextPtr) {
            if ((tokenPtr->id == _NUMBER) && ((tokenPtr->length == 6) ||
                                              (tokenPtr->length == 14))) {
                long value;
                /* Iso time format hhmmss */
                value = tokenPtr->lvalue;
                
                parserPtr->date.sec = value % 100;
                value /= 100;
                parserPtr->date.min = value % 100;
                value /= 100;
                parserPtr->date.hour = value % 100;
                if (tokenPtr->length == 14) {
                    value /= 100;
                    tokenPtr->length = 8;  /* Convert to ISO8 */
                    tokenPtr->lvalue = value;
                    first = tokenPtr = tokenPtr->nextPtr;
                } else {
                    first = tokenPtr;
                    tokenPtr = tokenPtr->nextPtr;
                }
                goto done;
            }
        }
        return TCL_OK;                  /* No time tokens found. */
    }
    if (next != NULL) {
        parserPtr->date.hour = tokens[0]->lvalue;
        parserPtr->date.min = tokens[1]->lvalue;
        tokenPtr = next;
        if (tokens[2] != NULL) {
            parserPtr->date.sec = tokens[2]->lvalue;
            if ((tokenPtr->id == _COLON) || (tokenPtr->id == _DOT) ||
                (tokenPtr->id == _COMMA)) {
                tokenPtr = tokenPtr->nextPtr;
                if ((tokenPtr->id == _NUMBER)) {
                    double d;
                    
                    d = pow(10.0, tokenPtr->length);
                    parserPtr->date.frac = (double)tokenPtr->lvalue / d;
                    tokenPtr = tokenPtr->nextPtr;
                }
            }
        }
    }
 done:
    /* Look for AMPM designation. */
    if (tokenPtr->id == _AMPM) {
        /* 12am == 00, 12pm == 12, 13-23am/pm invalid */
        if (parserPtr->date.hour > 12) {
            fprintf(stderr, "invalid am/pm, already in 24hr format\n");
        } else {
            if (tokenPtr->lvalue) {     /* PM */
                if (parserPtr->date.hour < 12) {
                    parserPtr->date.hour += 12;
                }
            } else {                        /* AM */
                if (parserPtr->date.hour == 12) {
                    parserPtr->date.hour = 0;
                }
            }
        }
        tokenPtr = tokenPtr->nextPtr;
    }
    /* Check for the timezone offset. */
    if ((tokenPtr->id == _PLUS) || (tokenPtr->id == _DASH)) {
        if (ParseTimeZoneOffset(interp, parserPtr, tokenPtr, &next) != TCL_OK) {
            return TCL_ERROR;
        }
        tokenPtr = next;
    }
    /* Remove the time-related tokens from the list. */
    last = tokenPtr;
    for (tokenPtr = first; tokenPtr != NULL; tokenPtr = next) {
        next = tokenPtr->nextPtr;
        if (tokenPtr == last) {
            break;
        }
        DeleteToken(parserPtr, tokenPtr);
    }
    parserPtr->flags |= PARSE_TIME;
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * ExtractDate --
 *
 *      Find and extract the date related tokens in the list.  
 *      The possible date formats are
 *
 *      +ddd:hh:mm:ss           +000:00:00:00
 *      dd mon yyyy             (31 Jan 2012) 
 *      dd-mm-yyyy              (31-Jan-2012) 
 *      dd.mm yyyy              (18. Feb 1921) 
 *      dd.mm.yyy               (31.Jan.2012) 
 *      dd/mm/yyyy              (31/Jan/2012) 
 *      ddd:hh:mm:ss            000:00:00:00
 *      mm dd, yyyy             (Jan 31, 2012) 
 *      mm-dd-yyyy              (Jan-31-2012) 
 *      mm.dd.yyyy              (Jan.31.2012) 
 *      mm/dd/yyyy              (Jan/31/2012) 
 *      mon dd yyyy             (Jan 31 2012) 
 *      mon yyyy                (Jan 2012) 
 *      mon/dd                  (12/23) 
 *      wday, dd, mon, yy       (Thu, 01 Jan 04) 
 *      yyyy                    (2012) 
 *      yyyy mon                (2012 Jan) 
 *      yyyy-Www                (2012-W01) 
 *      yyyy-Www-d              (2012-W50-7) 
 *      yyyy-ddd                (2012-100) 
 *      yyyy-mm                 (1957-Dec) 
 *      yyyy-mm-dd              (2012-Jan-31) 
 *      yyyy/mm/dd              (2012.Jan.31) 
 *      yyyyWww                 (2012W01)
 *      yyyyWwwd                (2012W017) 
 *      yyyyddd                 (2012100) 
 *      yyyymmdd                (20120131) 
 *
 * Results:
 *      None.
 *
 * Side Effects:
 *      The matching tokens are removed from the list and the parser
 *      structure is updated with the new information.
 *
 *---------------------------------------------------------------------------
 */
static int
ExtractDate(Tcl_Interp *interp, TimeStampParser *parserPtr)
{
    ParserToken *next, *tokenPtr;
    int i, patternIndex;
    Pattern *patPtr;

    if (NumberTokens(parserPtr) == 1) {
        return TCL_OK;
    }
    /* Remove the weekday description. */
    ExtractWeekday(parserPtr);
    /*  */
    FixParserTokens(parserPtr);
    ExtractYear(parserPtr);
#if DEBUG
    fprintf(stderr, "ExtractDate (%s)\n", 
            Tcl_GetString(PrintTokens(interp, parserPtr)));
#endif
    patternIndex = MatchDatePattern(interp, parserPtr);
#if DEBUG
    fprintf(stderr, "matching pattern is %d\n", patternIndex);
#endif
    if (patternIndex < 0) {
        if (interp != NULL) {
            ParseError(interp, 
                   "no matching date pattern \"%s\" for \"%s\"", 
                       Tcl_GetString(PrintTokens(interp, parserPtr)),
                   parserPtr->buffer);
        }
        return TCL_ERROR;
    }
    /* Process the list against the matching pattern. */
    patPtr = datePatterns + patternIndex;
    assert(patPtr->numIds == NumberTokens(parserPtr));
    tokenPtr = parserPtr->headPtr;
    for (i = 0; i < patPtr->numIds; i++, tokenPtr = tokenPtr->nextPtr) {
        ParserTokenId id;
        
        id = patPtr->ids[i];
        switch (id) {
        case _MONTH:                   /* 1-12 converted to 0-11 */
            parserPtr->date.mon = tokenPtr->lvalue - 1;
            break;

        case _YEAR:                    /* 0000-9999 or 00-99 */
            parserPtr->date.year = tokenPtr->lvalue;
            if (tokenPtr->length == 2) {

                parserPtr->date.year += 1900;
            }
            break;
        case _WEEK:                    /* 1-53 converted to 0-52 */
            parserPtr->flags |= PARSE_WEEK;
            parserPtr->date.week = tokenPtr->lvalue - 1;
            break;
        case _WDAY:                    /* 1-7 converted to 0-6 */
            parserPtr->flags |= PARSE_WDAY;
            parserPtr->date.wday = tokenPtr->lvalue - 1;
            break;
        case _MDAY:                    /* 1-31 */
            parserPtr->flags |= PARSE_MDAY;
            parserPtr->date.mday = tokenPtr->lvalue;
            break;
        case _YDAY:                    /* 1-366 converted to 0-365 */
            parserPtr->flags |= PARSE_YDAY;
            parserPtr->date.yday = tokenPtr->lvalue - 1;
            break;

        case _ISO7:                    /* 0000-9999,1-366  */
            {
                long value;
                /* Iso date format */
                value = tokenPtr->lvalue;
                
                parserPtr->date.yday = (value % 1000) - 1;
                value /= 1000;
                parserPtr->date.year = value;
                parserPtr->flags |= PARSE_YDAY;
            }
            break;

        case _ISO8:                    /* 0000-9999,1-12,1-31 */
            {
                long value;
                /* Iso date format */
                value = tokenPtr->lvalue;
                
                parserPtr->date.mday = value % 100;
                value /= 100;
                parserPtr->date.mon = (value % 100) - 1;
                value /= 100;
                parserPtr->date.year = value;
                parserPtr->flags |= PARSE_MDAY;
            }
            break;
        case _COMMA:
        case _DASH:
        case _DOT:
            continue;
        case _END:
        default:
            break;
        }
    }
    for (tokenPtr = parserPtr->headPtr; tokenPtr != NULL; tokenPtr = next) {
        next = tokenPtr->nextPtr;
        DeleteToken(parserPtr, tokenPtr);
    }
    parserPtr->flags |= PARSE_DATE;
    return TCL_OK;
}

/* 
 *-----------------------------------------------------------------------------
 *
 * NumberDaysFromStartOfYear --
 *
 *      Computes the number of days from the beginning of the given year to
 *      the given date.  Note that mday is 1-31, not 0-30.
 *
 * Returns:
 *      Returns the number of days from beginning of the given year to
 *      the given date.
 *
 *-----------------------------------------------------------------------------
 */
static int
NumberDaysFromStartOfYear(int year, int month, int mday)
{
    int numDays;

    numDays = numDaysToMonth[IsLeapYear(year)][month];
    return numDays + (mday - 1);
}

/* 
 *-----------------------------------------------------------------------------
 *
 * NumberDaysFromEpoch --
 *
 *      Computes the number of days from the epoch (0 secs) to the given
 *      year.  Note that returned value may be negative if the give year
 *      occurs before the epoch.
 *
 * Returns:
 *      Returns the number of days since the epoch to the beginning of the
 *      given year.
 *
 *-----------------------------------------------------------------------------
 */
static long
NumberDaysFromEpoch(int year)
{
    int y;
    long numDays;

    numDays = 0;
    if (year >= EPOCH) {
        for (y = EPOCH; y < year; y++) {
            numDays += numDaysYear[IsLeapYear(y)];
        }
    } else {
        for (y = year; y < EPOCH; y++)
            numDays -= numDaysYear[IsLeapYear(y)];
    }
    return numDays;
}

/* 
 *-----------------------------------------------------------------------------
 *
 * GetWeek --
 *
 *      Computes the US week number give the year, month, and day. The
 *      first week of the year contains Jan 1st.  Weeks start on Sunday and
 *      are numbered 0-52.
 *
 * Returns:
 *      Returns the week number for the date.  
 *
 *-----------------------------------------------------------------------------
 */
static int
GetWeek(int year, int mon, int mday)
{
    long numDays;
    int wdayJan1st;                     /* Weekday of Jan 1st. */

    numDays = NumberDaysFromEpoch(year);
    wdayJan1st = ((numDays % 7) + EPOCH_WDAY) % 7;
    numDays = NumberDaysFromStartOfYear(year, mon, mday) + wdayJan1st;
    if (numDays < 0) {
        return 0;
    }  
    return (numDays / 7);
}

/* 
 *-----------------------------------------------------------------------------
 *
 * GetIsoWeek --
 *
 *      Computes the ISO week number give the year, month, and day. The
 *      first week of the year contains Jan 4th.  Weeks start on Monday and
 *      are numbered 0-52.
 *
 * Returns:
 *      Returns the week number for the date.  
 *
 *-----------------------------------------------------------------------------
 */
static int
GetIsoWeek(int year, int mon, int day, int *wyearPtr)
{
    long a, b, c, s, e, f, d, g, n;
    int week, wyear;

    if (mon < 2) {
        a = year - 1;
        b = a / 4 -  a / 100 +  a / 400;
        c = (a-1)/4 - (a-1)/100 + (a-1)/400;
        s = b - c;
        e = 0;
        f = day - 1 +  31 * (mon-1);
    } else {
        a = year;
        b = a/4 - a/100 + a/400;
        c = (a-1)/4 - (a-1)/100 + (a-1)/400;

        s = b-c;
        e = s+1;
        f = day + (153*(mon-3)+2)/5 + 58 + s;
    }
    g = (a + b) % 7;
    d = (f + g - e) % 7;
    n = f + 3 - d;

    if (n < 0) {
        week = 53-(g-s)/5;
        wyear = year-1;
    }
    else if (n>364+s) {
        week = 1;
        wyear = year+1;
    }
    else {
        week = n/7 + 1;
        wyear = year;
    }
    *wyearPtr = wyear;
    return week;
}

#ifdef notdef
static int
GetWeekDay(int year, int mon, int mday)
{
    int century;
    
    /* Adjust months so February is the last one */
    mon -= 2;
    if (mon < 1) {
        mon += 12;
        year--;
    }
    /* Split by century */
    century = year / 100;
    year %= 100;
    return (((26*mon - 2)/10 + mday + year + year/4 + 
             century/4 + 5*century) % 7);
}
#endif

/* 
 *-----------------------------------------------------------------------------
 *
 * GetDateFromOrdinalDay --
 *
 *      Computes the month and day of the month from the given year and 
 *      ordinal day (day of the year).
 *
 * Returns:
 *      Returns the day of the month.  
 *
 * Side Effects:
 *      The month is returned via *monthPtr*.
 *
 *-----------------------------------------------------------------------------
 */
static int
GetDateFromOrdinalDay(int year, int yday, int *monthPtr)
{
    int numDays, mon;

    /* Keep subtracting the number of days in the month from the ordinal
     * day until it fits within the month.  */
    numDays = yday;
    mon = 0;
    while (numDays >= numDaysMonth[IsLeapYear(year)][mon]) {
        numDays -= numDaysMonth[IsLeapYear(year)][mon];
        mon++;
    }
    *monthPtr = mon;
    return numDays + 1;                 /* mday is 1-31 */
}

static Tcl_Obj *
DateToListObj(Tcl_Interp *interp, TimeStampParser *parserPtr)
{
    Tcl_Obj *objPtr, *listObjPtr;
    Blt_DateTime *datePtr;

    datePtr = &parserPtr->date;
    listObjPtr = Tcl_NewListObj(0, (Tcl_Obj **)NULL);
    /* Year */
    objPtr = Tcl_NewStringObj("year", 4);
    Tcl_ListObjAppendElement(interp, listObjPtr, objPtr);
    objPtr = Tcl_NewIntObj(datePtr->year);
    Tcl_ListObjAppendElement(interp, listObjPtr, objPtr);
    /* Month */
    objPtr = Tcl_NewStringObj("month", 5);
    Tcl_ListObjAppendElement(interp, listObjPtr, objPtr);
    objPtr = Tcl_NewStringObj(monthNames[datePtr->mon], -1);
    Tcl_ListObjAppendElement(interp, listObjPtr, objPtr);

    if (parserPtr->flags & PARSE_MDAY) { /* Date of month. */
        objPtr = Tcl_NewStringObj("mday", 4);
        Tcl_ListObjAppendElement(interp, listObjPtr, objPtr);
        objPtr = Tcl_NewIntObj(datePtr->mday);
        Tcl_ListObjAppendElement(interp, listObjPtr, objPtr);
    }
    if (parserPtr->flags & PARSE_WDAY) { /* Week Day */
        objPtr = Tcl_NewStringObj("wday", 4);
        Tcl_ListObjAppendElement(interp, listObjPtr, objPtr);
        objPtr = Tcl_NewStringObj(weekdayNames[datePtr->wday], -1);
        Tcl_ListObjAppendElement(interp, listObjPtr, objPtr);
    }
    if (parserPtr->flags & PARSE_YDAY) { /* Year Day */
        objPtr = Tcl_NewStringObj("yday", 4);
        Tcl_ListObjAppendElement(interp, listObjPtr, objPtr);
        objPtr = Tcl_NewIntObj(datePtr->yday);
        Tcl_ListObjAppendElement(interp, listObjPtr, objPtr);
    }
    if (parserPtr->flags & PARSE_WEEK) { /* Week */
        objPtr = Tcl_NewStringObj("week", 4);
        Tcl_ListObjAppendElement(interp, listObjPtr, objPtr);
        objPtr = Tcl_NewIntObj(datePtr->week);
        Tcl_ListObjAppendElement(interp, listObjPtr, objPtr);
        /* Week Year */
        objPtr = Tcl_NewStringObj("wyear", 5);
        Tcl_ListObjAppendElement(interp, listObjPtr, objPtr);
        objPtr = Tcl_NewIntObj(datePtr->wyear);
        Tcl_ListObjAppendElement(interp, listObjPtr, objPtr);
    }
    /* Leap year */
    objPtr = Tcl_NewStringObj("isleapyear", 10);
    Tcl_ListObjAppendElement(interp, listObjPtr, objPtr);
    objPtr = Tcl_NewBooleanObj(datePtr->isLeapYear);
    Tcl_ListObjAppendElement(interp, listObjPtr, objPtr);
    /* Hour */
    objPtr = Tcl_NewStringObj("hour", 4);
    Tcl_ListObjAppendElement(interp, listObjPtr, objPtr);
    objPtr = Tcl_NewIntObj(datePtr->hour);
    Tcl_ListObjAppendElement(interp, listObjPtr, objPtr);
    /* Minute */
    objPtr = Tcl_NewStringObj("minute", 6);
    Tcl_ListObjAppendElement(interp, listObjPtr, objPtr);
    objPtr = Tcl_NewIntObj(datePtr->min);
    Tcl_ListObjAppendElement(interp, listObjPtr, objPtr);
    /* Second */
    objPtr = Tcl_NewStringObj("second", 6);
    Tcl_ListObjAppendElement(interp, listObjPtr, objPtr);
    objPtr = Tcl_NewDoubleObj(datePtr->sec + datePtr->frac);
    Tcl_ListObjAppendElement(interp, listObjPtr, objPtr);
    /* Daylight Savings Time */
    objPtr = Tcl_NewStringObj("isdst", 5);
    Tcl_ListObjAppendElement(interp, listObjPtr, objPtr);
    objPtr = Tcl_NewBooleanObj(datePtr->isdst);
    Tcl_ListObjAppendElement(interp, listObjPtr, objPtr);
    /* Timezone Offset */
    objPtr = Tcl_NewStringObj("tzoffset", 8);
    Tcl_ListObjAppendElement(interp, listObjPtr, objPtr);
    objPtr = Tcl_NewIntObj(datePtr->tzoffset);
    Tcl_ListObjAppendElement(interp, listObjPtr, objPtr);
    return listObjPtr;
}

#ifdef notdef
static int
CheckDateAgainstMktime(Tcl_Interp *interp, TimeStampParser *parserPtr,
                       double seconds)
{
    Blt_DateTime *datePtr;
    struct tm tm;
    time_t ticks;
    long sec;
    extern char *tzname[2];
    extern long timezone;
    extern int daylight;

    datePtr = &parserPtr->date;
    tm.tm_sec  = datePtr->sec;          /* 0-59 */
    tm.tm_min  = datePtr->min;          /* 0-59 */
    tm.tm_hour = datePtr->hour;         /* 0-23 */
    tm.tm_mday = datePtr->mday;         /* 1-31 */
    tm.tm_mon  = datePtr->mon;          /* 0-11 */
    tm.tm_year = datePtr->year - 1900;  /* since 1900 */

    tm.tm_isdst = -1;
    setenv("TZ", "UTC", 1);
    if (parserPtr->tmZone != NULL) {
        setenv("TZ", parserPtr->tmZone, 1);
    }
    tzset();
    ticks = mktime(&tm);
    setenv("TZ", "UTC", 1);
    tzset();
    sec = (long)seconds;
    if ((sec - ticks) != 0) {
        fprintf(stderr, "tmzone=%s tzname[0]=%s tzname[1]=%s timezone=%ld daylight=%d\n", parserPtr->tmZone, tzname[0], tzname[1], timezone, daylight);
        fprintf(stderr, "sec=%d min=%d hour=%d mday=%d mon=%d year=%d\n",
                datePtr->sec, datePtr->min, datePtr->hour, datePtr->mday,
                datePtr->mon, datePtr->year);
        Tcl_AppendResult(interp, "conflict w/ mktime (timezone is ",
                (parserPtr->tmZone == NULL) ? "null" : parserPtr->tmZone,
                         " ", tzname[0], 
                ") difference = ", Blt_Dtoa(interp, sec - ticks), (char *)NULL);
        return TCL_ERROR;
    }
    return TCL_OK;
}
#endif

/*
 *-----------------------------------------------------------------------------
 *
 * ComputeTime --
 *
 *      Convert a {month, day, year, hours, minutes, seconds, meridian, dst}
 *      tuple into a clock seconds value.
 *
 * Results:
 *      A standard TCL result.  If no error occurred, TCL_OK is returned.
 *
 * Side effects:
 *      Returns number of seconds in *secondPtr*..
 *
 *-----------------------------------------------------------------------------
 */
static int
ComputeTime(Tcl_Interp *interp, TimeStampParser *parserPtr, double *secondsPtr)
{
    int isLeapYear;
    Blt_DateTime *datePtr;

    datePtr = &parserPtr->date;
#ifdef notdef
    fprintf(stderr, "ComputeDate: (%s) year=%d mon=%d mday=%d week=%d, %dh%dm%ds.%g, tz=%d\n", 
            parserPtr->buffer, 
            datePtr->year, datePtr->mon, datePtr->mday, datePtr->week,
            datePtr->hour, datePtr->min, datePtr->sec, datePtr->frac,
            datePtr->tzoffset);
#endif
    isLeapYear = IsLeapYear(datePtr->year);
    /* Check the inputs for validity */
    if ((datePtr->year < 0) || (datePtr->year > 99999)) {
        if (interp != NULL) {
            ParseError(interp, "year \"%d\" is out of range.", 
                       datePtr->year);
        }
        return TCL_ERROR;
    }
    if ((datePtr->mon < 0) || (datePtr->mon > 11)) { /* 0..11 */
        if (interp != NULL) {
            ParseError(interp, "month \"%d\" is out of range.", 
                       datePtr->mon + 1);
        }
        return TCL_ERROR;
    }
    if (parserPtr->flags & PARSE_WEEK) {
        if ((datePtr->week < 0) || (datePtr->week > 53)) {
            if (interp != NULL) {
                ParseError(interp, "week \"%d\" is out of range.", 
                           datePtr->week + 1);
            }
            return TCL_ERROR;
        }
    }
    if (parserPtr->flags & PARSE_MDAY) { 
        if ((datePtr->mday < 0) || 
            (datePtr->mday > numDaysMonth[isLeapYear][datePtr->mon])) {
            if (interp != NULL) {
                ParseError(interp, 
                           "day \"%d\" is out of range for month \"%s\"",
                           datePtr->mday, monthNames[datePtr->mon]);
            }
            return TCL_ERROR;
        }
    }
    if (parserPtr->flags & PARSE_YDAY) {
        if ((datePtr->yday < 0) || (datePtr->yday > numDaysYear[isLeapYear])) {
            if (interp != NULL) {
                ParseError(interp, 
                           "day of year \"%d\" is out of range for \"%d\"",
                           datePtr->yday, datePtr->year);
            }
            return TCL_ERROR;
        }
    }
    if (parserPtr->flags & PARSE_WDAY) { 
        if ((datePtr->wday < 0) || (datePtr->wday > 6)) {
            if (interp != NULL) {
                ParseError(interp, "day of week \"%d\" is out of range",
                           datePtr->wday);
            }
            return TCL_ERROR;
        }
    }

    if ((datePtr->hour < 0) || (datePtr->hour > 24)) {
        if (interp != NULL) {
            ParseError(interp, "hour \"%d\" is out of range.", datePtr->hour); 
        }
        return TCL_ERROR;
    }
    if ((datePtr->min < 0) || (datePtr->min > 59)) {
        if (interp != NULL) {
            ParseError(interp, "minute \"%d\", is out of range.", datePtr->min);
        }
        return TCL_ERROR;
    }
    if ((datePtr->sec < 0) || (datePtr->sec > 60)) {
        if (interp != NULL) {
            ParseError(interp, "second \"%d\" is out of range.", datePtr->sec);
        }
        return TCL_ERROR;
    }
    if (parserPtr->flags & PARSE_YDAY) {
        if ((datePtr->yday < 0) || (datePtr->yday > 366)) {
            if (interp != NULL) {
                ParseError(interp, "day of year \"%d\" is out of range.", 
                           datePtr->yday);
            }
            return TCL_ERROR;
        }
    }
    if ((parserPtr->flags & PARSE_YDAY) && (datePtr->yday > 0)) {
        datePtr->mday = GetDateFromOrdinalDay(datePtr->year, datePtr->yday,
                &datePtr->mon);
#ifdef notdef
        fprintf(stderr, "parse yday: yday=%d year=%d mon=%d mday=%d\n",
                datePtr->yday, datePtr->year, datePtr->mon, datePtr->mday);
#endif
    }         
    if (parserPtr->flags & PARSE_WEEK) {
        int numDays, wdayJan1, numDaysToFirstSunday;
        
        /* If the date was given by week number + week day, convert to
         * month, and mday. Note that the year may change. */

        /* Start by computing # of days until the start of the year. */
        numDays = NumberDaysFromEpoch(datePtr->year);
        /* The epoch started on a Thursday, so figure out what the offset
         * is to the first Sunday of the year.  Note it could be in the
         * previous year. */
        wdayJan1 = ((ABS(numDays) % 7) + EPOCH_WDAY) % 7;
        /* Jan 1st
         *  Mon, Tue, Wed, or Thu (1-4):  Week 01 (0).  
         *  Fri (5):                      Week 53 (52) of previous year.
         *  Sat (6):                      Week 52 or 53 of previous year. 
         *  Sun (0):                      Week 52 (51) of previous year. 
         * 
         * Which means if Jan 1st isn't 1-4, then the first week is +7 days.
         */
        numDaysToFirstSunday = numDays - wdayJan1;
        if ((wdayJan1 < 1) || (wdayJan1 > 4)) {
            numDaysToFirstSunday += 7;  /* Week 1 is the next week. */
        }
        datePtr->yday = (numDaysToFirstSunday + (datePtr->week * 7) + 
                         datePtr->wday + 1) - numDays;

        /* If the day of the year is negative, that means the day is in the
         * last year. */
        if (datePtr->yday < 0) {
            datePtr->year--;
            datePtr->yday += numDaysYear[IsLeapYear(datePtr->year)];
        }
       /* If the week was 52 or 53, then you might cross into the next year. */
        if (datePtr->yday >= numDaysYear[IsLeapYear(datePtr->year)]) {
            datePtr->yday -= numDaysYear[IsLeapYear(datePtr->year)];
            datePtr->year++;
        }
        datePtr->mday = GetDateFromOrdinalDay(datePtr->year, datePtr->yday,
                &datePtr->mon);
    }
    Blt_DateToSeconds(datePtr, secondsPtr);
#ifdef notdef
    return CheckDateAgainstMktime(interp, parserPtr, *secondsPtr);
#endif
    return TCL_OK;
}

/* 
 * Date and time formats:
 *
 *      <date> <time>
 *      <date>T<time>
 */
/* 
 * Date formats:
 *
 *      wday, month day, year 
 *      wday month day, year
 *      wday, day. month year
 *      wday, day month year
 *      wday day month year
 *      month/day/year
 *      day/month/year
 *      year/month/day
 *      day-month-year
 *      year-month-day
 *      month day
 *      month-day
 *      wday month day
 *      day. month
 *      wday day. month
 *      day month
 *      wday day month
 *      year-month-day
 *      year-month-day
 *      month, year
 *      month year
 */

/* 
 * Time formats:
 *
 *      time <meridian> <tz>
 *      hh
 *      hh:mm
 *      hh:mm:ss
 *      hhmm
 *      hhmmss
 *      ss.frac                 - a single number integer or floating point
 *      hh:mm:ss:mmm
 *      hh:mm
 */

/* 
 * Timezone
 *
 *      timezone
 *      '-+1234
 *      a-z     
 */

/* 
 * Date formats:
 *
 *      day month yyyy          01 January 1970
 *      dd month                01 January
 *      dd - month-yyyy         01-Jan-1970
 *      dd . month              01.Jan
 *      dd . month yyyy         01.Jan.1970
 *      dd / month / yyyy       01/Jan/1970
 *      month day               January 01
 *      month dd                Jan     01
 *      month dd yyyy           January 01 1970
 *      month dd , yyyy         January 01, 1970
 *      month yyyy              January 1970
 *      month , yyyy            January, 1970
 *      month - dd              January-01
 *      month - dd-yyyy         January-01-1970
 *      month / dd / yyyy       January/01/1970
 *      yyyy                    1970
 *      yyyy - Www              1970-W01
 *      yyyy - Www - D          1970-W02-1
 *      yyyy - mm               1970-01
 *      yyyy - mm - dd          1970-01-01
 *      yyyy - month            1970-January
 *      yyyy - month - dd       1970-January-01
 *      yyyy / mm / dd          1970/January/01
 *      yyyyWww                 1970W01
 *      yyyyWwwD                1970W011
 *      yyyymmdd                10700101
 *
 * mm yy
 * mm yyyy
 * mm dd yy
 * mm dd yyyy
 * mm dd, yyyy
 * mm / dd / yyyy
 * mm - dd - yyyy
 * mm . dd . yyyy
 * yyyy mm dd
 * yyyy - mm - dd
 * yyyy . mm . dd
 * dd mm yyyy
 * dd . mm . yyyy
 * yyyymmdd
 * 
 * dd yyyy
 * mm dd yy
 * mm/dd/yy
 * mm/dd/yyyy
 * yy.mm.dd
 * yyyy.mm.dd
 * dd/mm/yy
 * yyyy-mm-dd
 * yyyymmmdd 8 digit int.
 * dd 
        on DD YYYY HH:MIAM      Jan 1 2005 1:29PM 
        MM/DD/YY                11/23/98
        MM/DD/YYYY              11/23/1998
        YY.MM.DD                72.01.01
        YYYY.MM.DD              1972.01.01
        DD/MM/YY                19/02/72
        DD/MM/YYYY              19/02/1972
        DD.MM.YY                25.12.05
        DD.MM.YYYY              25.12.2005
        DD-MM-YY                24-01-98
        DD-MM-YYYY              24-01-1998
        DD Mon YY               04 Jul 06 1
        DD Mon YYYY             04 Jul 2006 1
        Mon DD, YY              Jan 24, 98 1
        Mon DD, YYYY            Jan 24, 1998 1
        HH:MM:SS                03:24:53
        Mon DD YYYY HH:MI:SS:MMMAM Apr 28 2006 12:32:29:253PM 1
        MM-DD-YY                01-01-06
        MM-DD-YYYY              01-01-2006
        YY/MM/DD                98/11/23
        YYYY/MM/DD              1998/11/23
        YYMMDD                  980124
        YYYYMMDD                19980124
        DD Mon YYYY HH:MM:SS:MMM        28 Apr 2006 00:34:55:190 
        HH:MI:SS:MMM            11:34:23:013
        YYYY-MM-DD HH:MI:SS     1972-01-01 13:42:24
        YYYY-MM-DD HH:MI:SS.MMM 1972-02-19 06:35:24.489
        YYYY-MM-DDTHH:MM:SS:MMM 1998-11-23T11:25:43:250
        DD Mon YYYY HH:MI:SS:MMMAM      28 Apr 2006 12:39:32:429AM 1
        DD/MM/YYYY HH:MI:SS:MMMAM       28/04/2006 12:39:32:429AM

        YY-MM-DD                99-01-24
        YYYY-MM-DD              1999-01-24
        MM/YY                   08/99
        MM/YYYY                 12/2005
        YY/MM                   99/08
        YYYY/MM                 2005/12
        Month DD, YYYY          July 04, 2006
        Mon YYYY                Apr 2006 1
        Month YYYY              February 2006
        DD Month                11 September 
        Month DD                September 11
        DD Month YY             19 February 72 
        DD Month YYYY           11 September 2002 
        MM-YY                   12-92
        MM-YYYY                 05-2006
        YY-MM                   92-12
        YYYY-MM                 2006-05
        MMDDYY                  122506
        MMDDYYYY                12252006
        DDMMYY                  240702
        DDMMYYYY                24072002
        Mon-YY                  Sep-02 1
        Mon-YYYY                Sep-2002 
        DD-Mon-YY               25-Dec-05 
        DD-Mon-YYYY 

        mm/dd/yy                11/23/98
        mm/dd/yyyy              11/23/1998
        yy.mm.dd                72.01.01
        yyyy.mm.dd              1972.01.01
        dd/mm/yy                19/02/72
        dd/mm/yyyy              19/02/1972
        dd.mm.yy                25.12.05
        dd.mm.yyyy              25.12.2005
        dd-mm-yy                24-01-98
        dd-mm-yyyy              24-01-1998
        dd mon yy               04 Jul 06 1
        dd mon yyyy             04 Jul 2006 1
        mon dd, yy              Jan 24, 98 1
        mon dd, yyyy            Jan 24, 1998 1
        mm-dd-yy                01-01-06
        mm-dd-yyyy              01-01-2006
        yy/mm/dd                98/11/23
        yyyy/mm/dd              1998/11/23
        yymmdd                  980124
        yyyymmdd                19980124
        dd mon yyyy HH:MM:SS:MMM        28 Apr 2006 00:34:55:190 
        hh:mi:ss:mmm            11:34:23:013
        yyyy-mm-dd HH:MI:SS     1972-01-01 13:42:24
        yyyy-mm-dd HH:MI:SS.MMM 1972-02-19 06:35:24.489
        yyyy-mm-ddthh:MM:SS:MMM 1998-11-23T11:25:43:250
        dd mon yyyy HH:MI:SS:MMMAM      28 Apr 2006 12:39:32:429AM 1
        dd/mm/yyyy HH:MI:SS:MMMAM       28/04/2006 12:39:32:429AM

        yy-mm-dd                99-01-24
        yyyy-mm-dd              1999-01-24
        mm/yy                   08/99
        mm/yyyy                 12/2005
        yy/mm                   99/08
        yyyy/mm                 2005/12
        month dd, yyyy          July 04, 2006
        mon yyyy                Apr 2006 1
        month yyyy              February 2006
        dd month                11 September 
        month dd                September 11
        dd month yy             19 February 72 
        dd month yyyy           11 September 2002 
        mm-yy                   12-92
        mm-yyyy                 05-2006
        yy-mm                   92-12
        yyyy-mm                 2006-05
        mmddyy                  122506
        mmddyyyy                12252006
        ddmmyy                  240702
        ddmmyyyy                24072002
        mon-yy                  Sep-02
        mon-yyyy                Sep-2002 
        dd-mon-yy               25-Dec-05 
        dd-mon-yyyy 

        D:yyyymmddhhmmss+'HHMM
 */

/* 
 *-----------------------------------------------------------------------------
 *
 * ParseTimeStamp --
 *
 *      Parses the date/time string into the number of seconds since the
 *      epoch.  The date string can be in one many formats accepted.
 *
 *      We're trying to parse somewhat standard timestamps, not validate
 *      any timestamp.  This is an easier requirement.  Our purpose here is
 *      to parse known formats quickly. For example this might be used to
 *      process measurement data where there may be hundreds of thousands
 *      of timestamps.  If there are invalid patterns that are also
 *      accepted, that's OK.  We won't see them.
 *
 *      The idea here is to remove the unambiguous parts first an then
 *      attack the harder patterns.  We can remove the time separator, DST,
 *      timezone, tnime, tokens without much problem.  This leaves a smaller
 *      number of accepted date patterns to match.
 *
 * Returns:
 *      A standard TCL result.  If the string was successfully parsed,
 *      TCL_OK is returned.  Otherwise TCL_ERROR, and an error message is
 *      left as the interpreter's result.
 *
 * Side Effects:
 *      The time as the number of seconds since the epoch is saved in 
 *      *secondsPtr*.
 *
 *-----------------------------------------------------------------------------
 */
static int
ParseTimeStamp(Tcl_Interp *interp, const char *string, double *secondsPtr)
{
    TimeStampParser parser;
    int result;
    
    InitParser(interp, &parser, string);

    /* Create list of tokens from date string. */
    result = ProcessTokens(interp, &parser);
    if (result == TCL_OK) {
        /* Remove the time/date 'T' separator if one exists. */
        ExtractDateTimeSeparator(&parser);
        /* Remove the DST indicator if one exists. */
        if (parser.flags & PARSE_DST) {
            ExtractDST(&parser);
        }
        /* Now parse out the timezone, time, and date. */
        result = ExtractTimeZonesAndOffset(interp, &parser);
    }
    if (result == TCL_OK) {
        result = ExtractTime(interp, &parser);
    }
    if (result == TCL_OK) {
        result = ExtractDate(interp, &parser);
    }
    if (result == TCL_OK) {
        result = ComputeTime(interp, &parser, secondsPtr);
    }
    FreeParser(&parser);
    return result;
}

static int
FormatOp(ClientData clientData, Tcl_Interp *interp, int objc, 
       Tcl_Obj *const *objv)
{
    FormatSwitches switches;
    Blt_DateTime date;
    double seconds;
    Tcl_DString ds;

    if (Tcl_GetDoubleFromObj(interp, objv[2], &seconds) != TCL_OK) {
        return TCL_ERROR;
    }
    /* Process switches  */
    memset(&switches, 0, sizeof(switches));
    if (Blt_ParseSwitches(interp, formatSwitches, objc - 3, objv + 3,
                &switches, BLT_SWITCH_DEFAULTS) < 0) {
        return TCL_ERROR;
    }
    Blt_SecondsToDate(seconds, &date);
    Tcl_DStringInit(&ds);
    if (switches.fmtObjPtr == NULL) {
        Blt_FormatDate(&date, "%a %b %d %H:%M:%S %z %Y", &ds);
    } else {
        Blt_FormatDate(&date, Tcl_GetString(switches.fmtObjPtr), &ds);
    }
    Tcl_DStringResult(interp, &ds);
    Tcl_DStringFree(&ds);
    Blt_FreeSwitches(formatSwitches, (char *)&switches, 0);
    return TCL_OK;
}

static int
ParseOp(ClientData clientData, Tcl_Interp *interp, int objc,
        Tcl_Obj *const *objv)
{
    int result;
    TimeStampParser parser;

    InitParser(interp, &parser, Tcl_GetString(objv[2]));

    /* Create list of tokens from date string. */
    result = ProcessTokens(interp, &parser);
    if (result == TCL_OK) {
        /* Remove the time/date 'T' separator if one exists. */
        ExtractDateTimeSeparator(&parser);
        /* Remove the DST indicator if one exists. */
        if (parser.flags & PARSE_DST) {
            ExtractDST(&parser);
        }
        /* Now parse out the timezone, time, and date. */
        result = ExtractTimeZonesAndOffset(interp, &parser);
    }
    if (result == TCL_OK) {
        result = ExtractTime(interp, &parser);
    }
    if (result == TCL_OK) {
        result = ExtractDate(interp, &parser);
    }
    if (result == TCL_OK) {
        Tcl_SetObjResult(interp, DateToListObj(interp, &parser));
    }
    FreeParser(&parser);
    return result;
}


static int
ScanOp(ClientData clientData, Tcl_Interp *interp, int objc, 
       Tcl_Obj *const *objv)
{
    double seconds;

    if (Blt_GetTimeFromObj(interp, objv[2], &seconds) != TCL_OK) {
        return TCL_ERROR;
    }
    Tcl_SetDoubleObj(Tcl_GetObjResult(interp), seconds);
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * TimeStampCmd --
 *
 *---------------------------------------------------------------------------
 */
static Blt_OpSpec timeStampCmdOps[] =
{
    {"format",  1, FormatOp,      3, 0, "seconds ?switches ...?",},
    {"parse",   1, ParseOp,       3, 3, "timeStamp",},
    {"scan",    1, ScanOp,        3, 3, "timeStamp",},
};

static int numCmdOps = sizeof(timeStampCmdOps) / sizeof(Blt_OpSpec);

/*ARGSUSED*/
static int
TimeStampCmd(
    ClientData clientData,              /* Interpreter-specific data. */
    Tcl_Interp *interp,
    int objc,
    Tcl_Obj *const *objv)
{
    Tcl_ObjCmdProc *proc;

    proc = Blt_GetOpFromObj(interp, numCmdOps, timeStampCmdOps, BLT_OP_ARG1,
        objc, objv, 0);
    if (proc == NULL) {
        return TCL_ERROR;
    }
    return  (*proc) (clientData, interp, objc, objv);
}
/* Public routines. */

/*
 *-----------------------------------------------------------------------------
 *
 * Blt_DateToSeconds --
 *
 *      Converts a Blt_DateTime structure into the number of second since
 *      the epoch.  The date can contain fractional seconds.  The date
 *      fields year, mon, and mday are used to compute the time, while
 *      week, wday, and yday and ignored.  You must have previously
 *      converted the them to year, mon, and mday.
 *
 * Results:
 *      None.
 *
 * Side effects:
 *      Returns number of seconds in *secondPtr*.
 *
 *-----------------------------------------------------------------------------
 */
void
Blt_DateToSeconds(Blt_DateTime *datePtr, double *secondsPtr)
{
    double t;
    long numDays;

#ifdef notdef
    fprintf(stderr, "Entering Blt_DateToSeconds: year=%d mon=%d mday=%d week=%d hour=%d min=%d sec=%d, frac=%.15g\n", 
            datePtr->year, datePtr->mon, datePtr->mday, datePtr->week,
            datePtr->hour, datePtr->min, datePtr->sec, datePtr->frac);
#endif
    datePtr->isLeapYear = IsLeapYear(datePtr->year);
    /* Compute the number of seconds. */

    /* Step 1: Determine the number of days since the epoch to the
     *         beginning of the given year. */
    numDays = NumberDaysFromEpoch(datePtr->year);

    /* Step 2: Add in the number of days from the start of the year. Use
     *         the day of year if it was supplied, otherwise compute it. */
    if (datePtr->mday > 0) {            
        int n;                          

        n = NumberDaysFromStartOfYear(datePtr->year, datePtr->mon, 
                datePtr->mday);
        datePtr->yday = n;              
        numDays += n;
    } else if (datePtr->yday > 0) {
        numDays += datePtr->yday;
    }
    /* Step 3: Convert days to seconds. */
    t = numDays * SECONDS_DAY;          

    /* Step 4: Add in the timezone offset. */
    t += datePtr->tzoffset;

#ifdef notdef
    if (datePtr->isdst > 0) {
        datePtr->hour++; 
    }
#endif
    /* Step 5. Add in the time, including the fractional seconds. */
    t += (datePtr->hour * SECONDS_HOUR) + (datePtr->min * SECONDS_MINUTE) + 
        datePtr->sec;
    t += datePtr->frac; 

    *secondsPtr = t;
#ifdef notdef
    fprintf(stderr, "Leaving Blt_DateToSeconds: seconds=%.15g\n", t);
#endif
}

/*
 *-----------------------------------------------------------------------------
 *
 * Blt_SecondsToDate --
 *
 *      Converts a double value representing the number of seconds since
 *      the epoch into a date structure. The date can contain fractional
 *      seconds.  
 *
 * Results:
 *      None.
 *
 * Side effects:
 *      Returns the date in *datePtr*..
 *
 *-----------------------------------------------------------------------------
 */
void
Blt_SecondsToDate(double seconds, Blt_DateTime *datePtr)
{
    int mon, year;
    long rem;
    long numDays;

#ifdef notdef
    fprintf(stderr, "Entering Blt_SecondsToDate: seconds=%.15g\n", seconds);
#endif
    memset(datePtr, 0, sizeof(Blt_DateTime));
    numDays = ((long)seconds) / SECONDS_DAY;
    rem  = ((long)seconds) % SECONDS_DAY;
    if (rem < 0) {
#ifdef notdef
 fprintf(stderr, "rem=%ld numDays=%ld rem < 0\n", rem, numDays);
#endif
        rem += SECONDS_DAY;
        numDays--;
    }
    if (rem >= SECONDS_DAY) {
 fprintf(stderr, "rem=%ld numDays=%ld rem >= SECONDS_DAY\n", 
                rem, numDays);
        rem -= SECONDS_DAY;
        numDays++;
    }
    memset(datePtr, 0, sizeof(Blt_DateTime));

    /* Step 1: Extract the time: hours, minutes, and seconds. */
    datePtr->hour = (int) (rem / SECONDS_HOUR);
    rem %= SECONDS_HOUR;
    datePtr->min = (int) (rem / SECONDS_MINUTE);
    datePtr->sec = (int) (rem % SECONDS_MINUTE);
    datePtr->frac = seconds - floor(seconds);

    /* Step 2: Compute the day of week: 0=Sunday 6=Saturday. */
    datePtr->wday = (EPOCH_WDAY + numDays) % 7;
    if (datePtr->wday < 0) {
        datePtr->wday += 7;
    }
    /* Step 3: Compute the year from the total number of days. Subtract the
     *         number of days from the epoch until you get to the start of
     *         the year. */
    year = EPOCH;
    if (numDays >= 0) {
        while (numDays >= numDaysYear[IsLeapYear(year)]) {
            numDays -= numDaysYear[IsLeapYear(year)];
            year++;
        }
    } else {
        do {
            --year;
             numDays += numDaysYear[IsLeapYear(year)];
       } while (numDays < 0);
    }
    datePtr->year = year;
    datePtr->isLeapYear = IsLeapYear(year);
    datePtr->yday = numDays;            /* The days remaining are the
                                         * number of days from the start of
                                         * the year. */
    mon = 0;
    /* Step 4: Compute the month and the number of days from the beginning
     *         of the month. */
    while (numDays >= numDaysMonth[datePtr->isLeapYear][mon]) {
        numDays -= numDaysMonth[datePtr->isLeapYear][mon];
        mon++;
    }
    datePtr->mon = mon;
    datePtr->mday = numDays + 1;        /* mday is 1-31 */

    /* Step 5: Lastly, compute the ISO week and week year. */
    datePtr->week = GetIsoWeek(year, mon + 1, numDays, &datePtr->wyear);

#ifdef notdef
    fprintf(stderr, "Leaving Blt_SecondsToDate: y=%d m=%d mday=%d wday=%d yday=%d week=%d wyear=%d hour=%d min=%d sec=%d usaweek=%d\n",
            datePtr->year, datePtr->mon, datePtr->mday, datePtr->wday,
            datePtr->yday, datePtr->week, datePtr->wyear, datePtr->hour,
            datePtr->min, datePtr->sec, 
            GetWeek(datePtr->year, datePtr->mon, datePtr->mday));
#endif
    datePtr->isdst = 0;
}

/* 
 *-----------------------------------------------------------------------------
 *
 * Blt_GetTime --
 *
 *      Converts a date string into the number of seconds from the epoch.
 *
 *-----------------------------------------------------------------------------
 */
int
Blt_GetTime(Tcl_Interp *interp, const char *string, double *secondsPtr)
{
    if (ParseTimeStamp(interp, string, secondsPtr) == TCL_OK) {
        return TCL_OK;
    }
    return TCL_ERROR;
}

/* 
 *-----------------------------------------------------------------------------
 *
 * Blt_GetTimeFromObj --
 *
 *      Converts a Tcl_Obj representing a date string into the number of
 *      seconds from the epoch (GMT time).
 *
 *-----------------------------------------------------------------------------
 */
int
Blt_GetTimeFromObj(Tcl_Interp *interp, Tcl_Obj *objPtr, double *secondsPtr)
{
    if (ParseTimeStamp(interp, Tcl_GetString(objPtr), secondsPtr) == TCL_OK) {
        return TCL_OK;
    }
    return TCL_ERROR;
}

/*
 *-----------------------------------------------------------------------------
 *
 * Blt_FormatDate --
 *
 *      Formats the date structure using the given format specifications
 *      into a string.  The string is saved in the dynamic string. It is
 *      assumed that the dynamic string has already been initialized.
 *
 * Results:
 *      None.
 *
 * Side effects:
 *      The formatted date string in returned in *resultsPtr*. 
 *
 *-----------------------------------------------------------------------------
 */
void
Blt_FormatDate(Blt_DateTime *datePtr, const char *fmt, Tcl_DString *resultPtr)
{
    size_t count, numBytes;
    char *buffer, *bp;
    const char *p;
    double seconds;

#ifdef notdef
    fprintf(stderr, "Entering Blt_FormatDate: year=%d mon=%d mday=%d week=%d hour=%d min=%d sec=%d, frac=%.15g\n", 
            datePtr->year, datePtr->mon, datePtr->mday, datePtr->week,
            datePtr->hour, datePtr->min, datePtr->sec, datePtr->frac);
#endif
    /* Pass 1: Compute the size of the resulting string. */
    count = 0;
    for (p = fmt; *p != '\0'; p++) {
        if (*p != '%') {
            count++;
            continue;
        }
        p++;
        switch (*p) {
        case '%':                       /* "%%" is converted to "%". */
            count++;                    
            break;
        case 'a':                       /* Abbreviated weekday (Sun) */
            count += 3 ;                
            break;
        case 'A':                       /* Weekday */
            count += strlen(weekdayNames[datePtr->wday]);
            break;
        case 'b':                       /* Abbreviated month (Jan) */
        case 'h':
            count += 3;
            break;
        case 'B':                       /* Month */
            count += strlen(monthNames[datePtr->mon]);
            break;
        case 'c':                       /* Date and time (Thu Mar 3
                                         * 23:05:25 2005)". */
            count += 4 + 4 + 3 + 9 + 4;
            break;
        case 'C':                       /* Century without last two digits
                                         * (20) */
            count += 2;
            break;
        case 'd':                       /* Day of month. */
            count += 2;
            break;
        case 'D':                       /* mm/dd/yy */
            count += 8;
            break;
        case 'e':                       /* Day of month, space padded */
            count += 3;
            break;
        case 'F':                       /* Full date yyyy-mm-dd */
            count += 10;
            break;
        case 'g':                       /* Last 2 digits of ISO wyear */
            count += 2;
            break;
        case 'G':                       /* ISO wyear */
            count += 4;
            break;
        case 'H':                       /* Hour (0-23) */
        case 'I':                       /* Hour (0-12) */
            count += 2;
            break;
        case 'j':                       /* Day of year */
            count += 3;
            break;
        case 'k':                       /* Hour, space padded */
        case 'l':                       /* Hour, space padded */
            count += 3;
            break;
        case 'm':                       /* Month */
            count += 2;
            break;
        case 'M':                       /* Minute */
            count += 2;
            break;
        case 'N':                       /* nanoseconds (000000000..999999999) */
            count += 25;
            break;
        case 'P':
        case 'p':                       /* Equivalent of either AM or PM
                                         * blank if not known */
            count += 2;
            break;
        case 'R':                       /* 24 hour clock time (hh:mm) */
            count += 5;
            break;
        case 'r':                       /* 12 hour clock time (hh:mm:ss AM) */
            count += 11;
            break;
        case 's':                       /* Seconds since epoch, may contain
                                         * fraction. */
            count += 17;
            break;
        case 'S':                       /* Second (ss) */
            count += 2;
            break;
        case 'T':                       /* The time as "%H:%M:%S". */
            count += 8;
            break;
        case 'w':                       /* Day of week 0-6 */
        case 'u':                       /* Day of week 1-7 */
            count += 1;
            break;
        case 'U':                       /* Week (Sunday first day)*/
        case 'W':                       /* Week */
        case 'V':                       /* ISO Week (Monday first day) */
            count += 2;
            break;
        case 'x':                       /* Date representation mm/dd/yy */
            count += 8;
            break;
        case 'y':                       /* Year, last 2 digits (99) */
            count += 2;
            break;
        case 'Y':                       /* Year, 4 digits (1999) */
            count += 4;
            if (datePtr->year > 9999) {
                count++;
            }
            break;
        case 'z':                       /* Numeric timezone, +hhmm */
            count += 5;
            break;
        default:
            count += 2;
            break;
        }
    }
    if (count == 0) {
        return;
    }
    count++;                            /* NUL byte */

    /* Make sure the result dynamic string has enough space to hold the
     * formatted date.  */
    Tcl_DStringSetLength(resultPtr, count);
    buffer = Tcl_DStringValue(resultPtr);

    /* Pass 2: Fill in the allocated string with the date. */
    bp = buffer;
    for (p = fmt; *p != '\0'; p++) {
        if (*p != '%') {
            *bp++ = *p;
            continue;
        }
        p++;
        switch (*p) {
        case '%':                       /* "%%" is converted to "%". */
            *bp++ = '%';                    
            break;
        case 'a':                       /* Abbreviated weekday (Sun) */
            sprintf(bp, "%.3s", weekdayNames[datePtr->wday]);                
            bp += 3;
            break;
        case 'A':                       /* Weekday */
            numBytes = sprintf(bp, "%s", weekdayNames[datePtr->wday]); 
            bp += numBytes;
            break;
        case 'b':                       /* Abbreviated month (Jan) */
        case 'h':
            sprintf(bp, "%.3s", monthNames[datePtr->mon]);                
            bp += 3;
            break;
        case 'B':                       /* Month */
            numBytes = sprintf(bp, "%s", monthNames[datePtr->mon]);                
            bp += numBytes;
            break;
        case 'c':                       /* Date and time (Thu Mar 3
                                         * 23:05:25 2005)". */
            numBytes = sprintf(bp, "%.3s %.3s %d %02d:%02d:%02d %4d",
                            weekdayNames[datePtr->wday],
                            monthNames[datePtr->mon],
                            datePtr->mday,
                            datePtr->hour,
                            datePtr->min,
                            datePtr->sec,
                            datePtr->year);
            bp += numBytes;
            break;
        case 'C':                       /* Century without last two digits
                                         * (20) */
            sprintf(bp, "%2d", datePtr->year / 100);
            bp += 2;
            break;
        case 'd':                       /* Day of month (01-31). */
            sprintf(bp, "%02d", datePtr->mday);
            bp += 2;
            break;
        case 'D':                       /* mm/dd/yy */
            sprintf(bp, "%02d/%02d/%02d", 
                    datePtr->mon + 1, 
                    datePtr->mday,
                    datePtr->year % 100);
            bp += 8;
            break;
        case 'e':                       /* Day of month, space padded */
            sprintf(bp, "%2d", datePtr->mday);
            bp += 2;
            break;
        case 'F':                       /* Full date yyyy-mm-dd */
            sprintf(bp, "%04d-%02d-%02d", datePtr->year, datePtr->mon + 1, 
                    datePtr->mday);
            bp += 10;
            break;
        case 'g':                       /* Last 2 digits of ISO wyear */
            sprintf(bp, "%02d", datePtr->wyear % 100);
            bp += 2;
            break;
        case 'G':                       /* ISO year */
            sprintf(bp, "%04d", datePtr->wyear);
            bp += 4;
            break;
        case 'H':                       /* Hour (0-23) */
            sprintf(bp, "%02d", datePtr->hour);
            bp += 2;
            break;
        case 'I':                       /* Hour (01-12) */
            sprintf(bp, "%02d", ((datePtr->hour == 0) || (datePtr->hour == 12))
                    ? 12 : (datePtr->hour % 12));
            bp += 2;
            break;
        case 'j':                       /* Day of year 001-366 */
            sprintf(bp, "%03d", datePtr->yday + 1);
            bp += 3;
            break;
        case 'k':                       /* Hour, space padded */
            sprintf(bp, "%2d", datePtr->hour);
            bp += 2;
            break;
        case 'l':                       /* Hour, space padded */
            sprintf(bp, "%2d", ((datePtr->hour == 0) || (datePtr->hour == 12))
                    ? 12 : (datePtr->hour % 12));
            bp += 2;
            break;
        case 'm':                       /* Month (01-12) */
            sprintf(bp, "%02d", datePtr->mon + 1);
            bp += 2;
            break;
        case 'M':                       /* Minute (00-59) */
            sprintf(bp, "%02d", datePtr->min);
            bp += 2;
            break;
        case 'N':                       /* nanoseconds (000000000..999999999) */
            Blt_DateToSeconds(datePtr, &seconds);
            numBytes = sprintf(bp, "%ld", (long)(seconds * 1e9));
            bp += numBytes;
            break;
        case 'P':
            strcpy (bp, (datePtr->hour > 11) ? "pm" : "am");
            bp += 2;
            break;
        case 'p':                       /* Equivalent of either AM or PM */
            strcpy (bp, (datePtr->hour > 11) ? "PM" : "AM");
            bp += 2;
            break;
        case 'r':                       /* 12 hour clock time (hh:mm:ss AM) */
            sprintf(bp, "%02d:%02d:%02d %2s",
                    ((datePtr->hour == 0) || (datePtr->hour == 12)) 
                    ? 12 : (datePtr->hour % 12), 
                    datePtr->min, datePtr->sec,
                    (datePtr->hour > 11) ? "PM" : "AM");
            bp += 11;
            break;
        case 'R':                       /* 24 hour clock time (hh:mm) */
            sprintf(bp, "%02d:%02d", datePtr->hour, datePtr->min);
            bp += 5;
            break;
        case 's':                       /* Seconds since epoch, may contain
                                         * fraction. */
            if (datePtr->sec < 10) {
                numBytes = sprintf(bp, "0%01.6g", datePtr->sec + datePtr->frac);
            } else {
                numBytes = sprintf(bp, "%02.6g", datePtr->sec + datePtr->frac);
            }
            bp += numBytes;
            break;
        case 'S':                       /* Second (00-60) */
            sprintf(bp, "%02d", datePtr->sec);
            bp += 2;
            break;
        case 'T':                       /* The time as "hh:mm:ss". */
            sprintf(bp, "%02d:%02d:%02d",
                    datePtr->hour, datePtr->min, datePtr->sec);
            bp += 8;
            break;
        case 'u':                       /* Day of week 1-7 */
            sprintf(bp, "%1d", datePtr->wday + 1);
            bp += 1;
            break;
        case 'U':                       /* Week (10-53). Sunday is first
                                         * day of week. */
            sprintf(bp, "%02d", 
                    GetWeek(datePtr->year, datePtr->mon, datePtr->mday) + 1);
            bp += 2;
            break;
        case 'V':                       /* ISO Week (01-53). Monday is
                                         * first day of week. */
            sprintf(bp, "%02d", datePtr->week);
            bp += 2;
            break;
        case 'w':                       /* Week day (0-6). Sunday is 0. */
            sprintf(bp, "%1d", datePtr->wday);
            bp += 1;
            break;
        case 'W':                       /* Week (00-53). Monday is the
                                         * first day of week. (I don't know
                                         * what this is.) */
            sprintf(bp, "%02d", datePtr->week);
            bp += 2;
            break;
        case 'x':                       /* Date representation mm/dd/yy */
            sprintf(bp, "%02d/%02d/%02d", 
                    datePtr->mon + 1, datePtr->mday, datePtr->year % 100);
            bp += 8;
            break;
        case 'y':                       /* Year, last 2 digits (yy) */
            sprintf(bp, "%02d", datePtr->year % 100);
            bp += 2;
            break;
        case 'Y':                       /* Year, 4 digits (yyyy) */
            numBytes = sprintf(bp, (datePtr->year > 9999) ? "%05d" : "%04d", 
                datePtr->year);
            bp += numBytes;
            break;
        case 'z':                       /* Numeric timezone, +-hhmm */
            if (datePtr->tzoffset < 0) {
                sprintf(bp, "%05d", datePtr->tzoffset);
            } else {
                sprintf(bp, "+%04d", datePtr->tzoffset);
            }
            bp += 5;
            break;
        default:                        /* Not a substitution. */
            sprintf(bp, "%%%c", *p);
            bp += 2;
            break;
        }
        assert((bp - buffer) < count);
    }
    *bp = '\0';    
}

/*
 *---------------------------------------------------------------------------
 *
 * Blt_TimeStampCmdInitProc --
 *
 *      This procedure is invoked to initialize the "timestamp" command.
 *
 * Results:
 *      None.
 *
 * Side effects:
 *      Creates the new command.
 *
 *---------------------------------------------------------------------------
 */
int
Blt_TimeStampCmdInitProc(Tcl_Interp *interp)
{
    static Blt_CmdSpec cmdSpec = { 
        "timestamp", TimeStampCmd
    };
    /*
     * Source in the file contains the blt::timezones array. This array
     * contains the names of timezones and their offset.  We do it now so
     * that the user can change entries if he/she wants.
     */
    if (Tcl_GlobalEval(interp,
                "source [file join $blt_library bltTimeStamp.tcl]") != TCL_OK) {
        const char *errmsg =
            "\n    (while loading timezones for timestamp command)";
        Tcl_AddErrorInfo(interp, errmsg);
        return TCL_ERROR;
    }
    return Blt_InitCmd(interp, "::blt", &cmdSpec);
}


#ifdef notdef 
/* 
 * Duplicate abbreviations:
 */
    { "amst",    "+500", "+500", }, /* Armenia Summer Time */
    { "amst",    "-300", "-300", }, /* Amazon Summer Time */

    { "amt",     "+400", "+400", }, /* Armenia Time */
    { "amt",     "-400", "-400", }, /* Amazon Time */

    { "bst",     "+100", "+100", }, /* British Summer Time */
    { "bst",     "-300", "-300", }, /* Brazil Standard Time */
    { "bst",    "-1100", "-1100", }, /* Bering Summer Time */

    { "cast",    "+800", "+800", }, /* Casey Time (Antarctica) ! */
    { "cast",    "+930", "+930", }, /* Central Australian Standard */

    { "cat",     "+200", "+200", }, /* Central Africa Time  ! */
    { "cat",    "-1000", "-1000", }, /* Central Alaska (until 1967) */

    { "cct",     "+630", "+630", }, /* Cocos Islands Time (Indian ocean)  ? */
    { "cct",     "+800", "+800", }, /* China Coast Time  ? */

    { "ect",     "-500", "-500", }, /* Ecuador Time ! */
    { "ect",     "-400", "-400", }, /* Eastern Caribbean Time */

    { "edt",     "-400", "-400", }, /* Eastern Daylight Time (North America and Caribbean) ! */
    { "edt",    "+1100", "+1100", }, /*Eastern Daylight Time (Australia) */
    { "fst",     "+200", "+200", }, /* French Summer ! */
    { "fst",     "-200", "-200", }, /* Fernando de Noronha Standard Time (Brazil) */
    { "gst",     "+400", "+400", }, /* Gulf Standard Time ! */
    { "gst",    "+1000", "+1000", }, /* Guam Standard Time */
    { "gst",     "-300", "-300", }, /* Greenland Standard Time */
    { "gst",     "-200", "-200", }, /* South Georgia Time (South Georgia and the South Sandwich Islands) */
    { "idt",     "+300", "+300", }, /* Israel Daylight Time ! */
    { "idt",     "+430", "+430", }, /* Iran Daylight Time */
    { "idt",     "+630", "+630", }, /* Indian Daylight Time */
    { "it",      "+330", "+330", }, /* Iran Time ??? */

    { "ist",     "+330", "+330", }, /* India Standard Time ! */
    { "ist",     "+200", "+200", }, /* Israel Standard Time */
    { "ist",     "+330", "+330", }, /* Iran Standard Time */

    { "nt",      "-330", "-330", }, /* Newfoundland Time */
    { "nt",      "-1100", "-1100", }, /* Nome */

    { "sst",    "-1100", "-1100", }, /* Samoa Standard Time (American Samoa) ! */
    { "sst",     "+200", "+200", }, /* Swedish Summer */
    { "sst",     "+100", "+100", }, /* Swedish Summer */
    { "sst",     "+700", "+700", }, /* South Sumatra Time */
    { "sst",     "+800", "+800", }, /* Singapore Standard Time */

    { "wast",    "+200", "+200", }, /* West Africa Summer Time ! */
    { "wast",    "+700", "+700", }, /* West Australian Standard */

    { "wst",    "+1300", "+1300", }, /* Western Samoa Time (Standard Time)! */
    { "wst",     "+800", "+800", }, /* Western Standard Time (Australia) !*/
    { "wst",     "+100", "+100", }, /* Western Sahara Summer Time */
#endif
