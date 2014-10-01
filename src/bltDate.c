/* -*- mode: c; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 * bltDate.c --
 *
 *      This module implements a date parser for the BLT toolkit.  It
 *      differs slightly from the TCL "clock" command.  It was built to
 *      programatically convert thousands of dates in files to seconds
 *      (especially dates that may include fractional seconds).
 *      
 *      For example, if the year, month, or day are not specified, it is
 *      assumed that the date is the first day of the epoch, not the
 *      current date. The may seem strange but if you are parsing
 *      time/dates from a file you don't want to get a different time value
 *      each time you parse it.  It also handles different date formats
 *      (including fractional seconds) and is faster (C implementation
 *      vs. TCL).
 *
 *	Copyright 1993-2004 George A Howlett.
 *
 *	Permission is hereby granted, free of charge, to any person
 *	obtaining a copy of this software and associated documentation
 *	files (the "Software"), to deal in the Software without
 *	restriction, including without limitation the rights to use, copy,
 *	modify, merge, publish, distribute, sublicense, and/or sell copies
 *	of the Software, and to permit persons to whom the Software is
 *	furnished to do so, subject to the following conditions:
 *
 *	The above copyright notice and this permission notice shall be
 *	included in all copies or substantial portions of the Software.
 *
 *	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 *	EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 *	MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 *	NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
 *	BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
 *	ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 *	CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 *	SOFTWARE.
 */

#define _BSD_SOURCE 1
#define BUILD_BLT_TCL_PROCS 1
#include "bltInt.h"
#ifdef HAVE_STDLIB_H
#  include <stdlib.h>
#endif /* HAVE_STDLIB_H */
#ifdef HAVE_CTYPE_H
#  include <ctype.h>
#endif /* HAVE_CTYPE_H */
#ifdef HAVE_TIME_H
#  include <time.h>
#endif /* HAVE_TIME_H */
#ifdef HAVE_SYS_TIME_H
#include <sys/time.h>
#endif
#include <stdint.h>
#include "bltAlloc.h"
#include "bltString.h"
#include "bltSwitch.h"
#include "bltChain.h"
#include "bltInitCmd.h"
#include "bltOp.h"
#include <bltMath.h>

#define HRS2MINS(x)         ((int) (60 * x))

#if defined(MAC_TCL) && !defined(TCL_MAC_USE_MSL_EPOCH)
#   define EPOCH           1904
#   define START_OF_TIME   1904
#   define END_OF_TIME     2039
#else
#   define EPOCH           1970
#   define START_OF_TIME   1902
#   define END_OF_TIME     2037
#endif
#   define EPOCH_WDAY      4            /* Thursday. */

#define MER_AM		0
#define MER_PM		1
#define MER_24		2

#define IsLeapYear(y) \
        ((((y) % 4) == 0) && ((((y) % 100) != 0) || (((y) % 400) == 0)))

static const char *tokenNames[] = {
    "end",
    "month", "wday", "yday", "day", "year", "week",
    "hours", "seconds", "minutes", "ampm", 
    "tz_std", "tz_dst", 
    "/", "-", ",", ":", "+", ".", "'",
    "number", "iso6", "iso7", "iso8", 
    "unknown"
};

typedef enum {
    _END,
    _MONTH, _WDAY, _YDAY, _MDAY, _YEAR, _WEEK,
    _HOUR, _SECOND, _MINUTE, _AMPM,
    _STD, _DST,
    _SLASH, _DASH, _COMMA, _COLON, _PLUS, _DOT, _QUOTE,
    _NUMBER, _ISO6, _ISO7, _ISO8, _UNKNOWN,
} TokenId;

typedef struct _Token Token;

struct _Token {
    Blt_ChainLink link;			/* If non-NULL, pointer this entry
					 * in the list of tokens. */
    Blt_Chain chain;			/* Pointer to list of tokens. */
    const char *ident;			/* String representing this
                                         * token. */
    int length;				/* # of bytes in string. */
    TokenId id;                         /* Serial ID of token. */
    uint64_t lvalue;			/* Numeric value of token. */
    float frac;				/* Fraction of seconds. */
};

typedef struct {
    Tcl_Interp *interp;			/* Interpreter associated with the
					 * parser. */
    unsigned int flags;			/* Flags: see below. */
    Blt_DateTime date;
    int ampm;				/* Meridian. */
    char *nextCharPtr;			/* Points to the next character
					 * to parse.*/
    Blt_Chain tokens;			/* List of parsed tokens. */
    Token *currentPtr;			/* Current token being processed. */
    char buffer[BUFSIZ];		/* Buffer holding parsed string. */
} DateParser;

#define PARSE_DATE      (1<<0)
#define PARSE_TIME      (1<<1)
#define PARSE_TZ        (1<<2)

#define PARSE_YDAY      (1<<3)           /* Day of year has been set. */
#define PARSE_MDAY      (1<<4)           /* Day of month has been set. */
#define PARSE_WDAY      (1<<5)           /* Day of week has been set. */
#define PARSE_WEEK      (1<<6)           /* Ordinal week has been set. */
#define PARSE_WYEAR     (1<<7)           /* Year of ordinal week has been
                                          * set. */

typedef struct {
    const char *name;			/* Name of identifier. */
    TokenId id;                         /* Serial ID of identifier. */
    int value;				/* Value associated with
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
    "July", "August", "September", "October", "November", "December",
    "Off by one month"
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
    "Saturday", "Off by one weekday"
};

static int numWeekdays = sizeof(weekdayNames) / sizeof(const char *);

static const char *meridianNames[] = {
    "am",  "pm"
};
static int numMeridians = sizeof(meridianNames) / sizeof(const char *);

/*
 * The timezone table.  (Note: This table was modified to not use any floating
 * point constants to work around an SGI compiler bug).
 */
static IdentTable tzTable[] = {
    { "gmt",  _STD, HRS2MINS(0) },	/* Greenwich Mean */
    { "ut",   _STD, HRS2MINS(0) },	/* Universal (Coordinated) */
    { "utc",  _STD, HRS2MINS(0) },
    { "uct",  _STD, HRS2MINS(0) },	/* Universal Coordinated Time */
    { "wet",  _STD, HRS2MINS(0) },	/* Western European */
    { "bst",  _DST, HRS2MINS(0) },	/* British Summer */
    { "wat",  _STD, HRS2MINS(1) },	/* West Africa */
    { "at",   _STD, HRS2MINS(2) },	/* Azores */
#if     0
    /* For completeness.  BST is also British Summer, and GST is also Guam
     * Standard. */
    { "bst",  _STD, HRS2MINS(3) },	/* Brazil Standard */
    { "gst",  _STD, HRS2MINS(3) },	/* Greenland Standard */
#endif
    { "nft",  _STD, HRS2MINS(7/2) },	/* Newfoundland */
    { "nst",  _STD, HRS2MINS(7/2) },	/* Newfoundland Standard */
    { "ndt",  _DST, HRS2MINS(7/2) },	/* Newfoundland Daylight */
    { "ast",  _STD, HRS2MINS(4) },	/* Atlantic Standard */
    { "adt",  _DST, HRS2MINS(4) },	/* Atlantic Daylight */
    { "est",  _STD, HRS2MINS(5) },	/* Eastern Standard */
    { "edt",  _DST, HRS2MINS(5) },	/* Eastern Daylight */
    { "cst",  _STD, HRS2MINS(6) },	/* Central Standard */
    { "cdt",  _DST, HRS2MINS(6) },	/* Central Daylight */
    { "mst",  _STD, HRS2MINS(7) },	/* Mountain Standard */
    { "mdt",  _DST, HRS2MINS(7) },	/* Mountain Daylight */
    { "pst",  _STD, HRS2MINS(8) },	/* Pacific Standard */
    { "pdt",  _DST, HRS2MINS(8) },	/* Pacific Daylight */
    { "yst",  _STD, HRS2MINS(9) },	/* Yukon Standard */
    { "ydt",  _DST, HRS2MINS(9) },	/* Yukon Daylight */
    { "hst",  _STD, HRS2MINS(10) },	/* Hawaii Standard */
    { "hdt",  _DST, HRS2MINS(10) },	/* Hawaii Daylight */
    { "cat",  _STD, HRS2MINS(10) },	/* Central Alaska */
    { "ahst", _STD, HRS2MINS(10) },	/* Alaska-Hawaii Standard */
    { "nt",   _STD, HRS2MINS(11) },	/* Nome */
    { "idlw", _STD, HRS2MINS(12) },	/* International Date Line West */
    { "cet",  _STD, -HRS2MINS(1) },	/* Central European */
    { "cest", _DST, -HRS2MINS(1) },	/* Central European Summer */
    { "met",  _STD, -HRS2MINS(1) },	/* Middle European */
    { "mewt", _STD, -HRS2MINS(1) },	/* Middle European Winter */
    { "mest", _DST, -HRS2MINS(1) },	/* Middle European Summer */
    { "swt",  _STD, -HRS2MINS(1) },	/* Swedish Winter */
    { "sst",  _DST, -HRS2MINS(1) },	/* Swedish Summer */
    { "fwt",  _STD, -HRS2MINS(1) },	/* French Winter */
    { "fst",  _DST, -HRS2MINS(1) },	/* French Summer */
    { "eet",  _STD, -HRS2MINS(2) },	/* Eastern Europe, USSR Zone 1 */
    { "bt",   _STD, -HRS2MINS(3) },	/* Baghdad, USSR Zone 2 */
    { "it",   _STD, -HRS2MINS(7/2) },	/* Iran */
    { "zp4",  _STD, -HRS2MINS(4) },	/* USSR Zone 3 */
    { "zp5",  _STD, -HRS2MINS(5) },	/* USSR Zone 4 */
    { "ist",  _STD, -HRS2MINS(11/2) },    /* Indian Standard */
    { "zp6",  _STD, -HRS2MINS(6) },	/* USSR Zone 5 */
#if     0
    /* For completeness.  NST is also Newfoundland Stanard, nad SST is also
     * Swedish Summer. */
    { "nst",  _STD, -HRS2MINS(13/2) },    /* North Sumatra */
    { "sst",  _STD, -HRS2MINS( 7) },      /* south Sumatra, USSR Zone 6 */
#endif  /* 0 */ 
    { "wast", _STD, -HRS2MINS( 7) },      /* West Australian Standard */
    { "wadt", _DST, -HRS2MINS( 7) },      /* West Australian Daylight */
    { "jt",   _STD, -HRS2MINS(15/2) },    /* Java (3pm in Cronusland!) */
    { "cct",  _STD, -HRS2MINS( 8) },      /* China Coast, USSR Zone 7 */
    { "jst",  _STD, -HRS2MINS( 9) },      /* Japan Standard, USSR Zone 8 */
    { "jdt",  _DST, -HRS2MINS( 9) },      /* Japan Daylight */
    { "kst",  _STD, -HRS2MINS( 9) },      /* Korea Standard */
    { "kdt",  _DST, -HRS2MINS( 9) },      /* Korea Daylight */
    { "cast", _STD, -HRS2MINS(19/2) },    /* Central Australian Standard */
    { "cadt", _DST, -HRS2MINS(19/2) },    /* Central Australian Daylight */
    { "east", _STD, -HRS2MINS(10) },      /* Eastern Australian Standard */
    { "eadt", _DST, -HRS2MINS(10) },      /* Eastern Australian Daylight */
    { "gst",  _STD, -HRS2MINS(10) },      /* Guam Standard, USSR Zone 9 */
    { "nzt",  _STD, -HRS2MINS(12) },      /* New Zealand */
    { "nzst", _STD, -HRS2MINS(12) },      /* New Zealand Standard */
    { "nzdt", _DST, -HRS2MINS(12) },      /* New Zealand Daylight */
    { "idle", _STD, -HRS2MINS(12) },      /* International Date Line East */
    /* ADDED BY Marco Nijdam */
    { "dst",  _DST, HRS2MINS( 0) },       /* DST on (hour is ignored) */
    /* End ADDED */
};
static int numTimezones = sizeof(tzTable) / sizeof(IdentTable);

/*
 * Military timezone table.
 */
static IdentTable milTzTable[] = {
    { "a",  _STD,  HRS2MINS(1)   },
    { "b",  _STD,  HRS2MINS(2)   },
    { "c",  _STD,  HRS2MINS(3)   },
    { "d",  _STD,  HRS2MINS(4)   },
    { "e",  _STD,  HRS2MINS(5)   },
    { "f",  _STD,  HRS2MINS(6)   },
    { "g",  _STD,  HRS2MINS(7)   },
    { "h",  _STD,  HRS2MINS(8)   },
    { "i",  _STD,  HRS2MINS(9)   },
    { "k",  _STD,  HRS2MINS(10)  },
    { "l",  _STD,  HRS2MINS(11)  },
    { "m",  _STD,  HRS2MINS(12)  },
    { "n",  _STD,  HRS2MINS(-1)  },
    { "o",  _STD,  HRS2MINS(-2)  },
    { "p",  _STD,  HRS2MINS(-3)  },
    { "q",  _STD,  HRS2MINS(-4)  },
    { "r",  _STD,  HRS2MINS(-5)  },
    { "s",  _STD,  HRS2MINS(-6)  },
    { "t",  _STD,  HRS2MINS(-7)  },
    { "u",  _STD,  HRS2MINS(-8)  },
    { "v",  _STD,  HRS2MINS(-9)  },
    { "w",  _STD,  HRS2MINS(-10) },
    { "x",  _STD,  HRS2MINS(-11) },
    { "y",  _STD,  HRS2MINS(-12) },
    { "z",  _STD,  HRS2MINS(0)   },
};
static int numMilitaryTimezones = sizeof(milTzTable) / sizeof(IdentTable);

typedef struct {
    int numIds;
    TokenId ids[6];
} Pattern;

static Pattern datePatterns[] = {
    { 2, {_ISO7} },			/* yyyyddd (2012100) */
    { 2, {_ISO8} },			/* yyyymmdd (20120131) */
    { 2, {_YEAR} },			/* yyyy (2012) */
    { 3, {_MONTH, _YEAR} },		/* mon yyyy (Jan 2012) */
    { 3, {_YEAR, _MONTH} },		/* yyyy mon (2012 Jan) */
    { 3, {_YEAR, _WEEK} },		/* yyyywww (2012W01)*/
    { 3, {_YDAY, _COLON} },		/* ddd:hh:mm:ss */
    { 4, {_MDAY, _MONTH, _YEAR} },	/* dd mon yyyy (31 Jan 2012) */
    { 4, {_MONTH, _MDAY, _YEAR} },	/* mon dd yyyy (Jan 31 2012) */
    { 4, {_MONTH, _SLASH, _MDAY} },	/* mon/dd (12/23) */
    { 4, {_YEAR, _DASH, _WEEK} },	/* yyyy-www (2012-W01) */
    { 4, {_YEAR, _DASH, _YDAY} },	/* yyyy-ddd (2012-100) */
    { 4, {_YEAR, _DASH, _MONTH} },	/* yyyy-mm (12/23) */
    { 4, {_YEAR, _MONTH, _MDAY} },	/* yyyymmdd (20120131) */
    { 4, {_YEAR, _WEEK, _WDAY} },	/* yyyywwwd (2012W017) */
    { 4, {_PLUS, _YDAY, _COLON} },	/* +ddd:hh:mm:ss */
    { 5, {_MDAY, _DOT, _MONTH, _YEAR} },  /* (18. Feb 1921) */
    { 5, {_MONTH, _MDAY, _COMMA, _YEAR} },  /* (Jan 31, 2012) */
    { 5, {_COMMA, _MDAY, _MONTH, _YEAR } }, /* (Thu, 01 Jan 04) */
    { 6, {_MDAY, _DOT, _MONTH, _DOT, _YEAR} },  /* (31.Jan.2012) */
    { 6, {_MDAY, _DASH, _MONTH, _DASH, _YEAR} }, /* (31-Jan-2012) */
    { 6, {_MONTH, _DOT, _MDAY, _DOT, _YEAR} },  /* (Jan.31.2012) */
    { 6, {_MONTH, _DASH, _MDAY, _DASH, _YEAR} }, /* (Jan-31-2012) */
    { 6, {_MONTH, _SLASH, _MDAY, _SLASH, _YEAR} }, /* (Jan/31/2012) */
    { 6, {_MDAY, _SLASH, _MONTH, _SLASH, _YEAR} }, /* (31/Jan/2012) */
    { 6, {_YEAR, _SLASH, _MONTH, _SLASH, _MDAY} }, /* (2012/Jan/31) */
    { 6, {_YEAR, _DOT, _MONTH, _DOT, _MDAY} },  /* (2012.Jan.31) */
    { 6, {_YEAR, _DASH, _MONTH, _DASH, _MDAY} }, /* (2012-Jan-31) */
    { 6, {_YEAR, _DASH, _WEEK, _DASH, _WDAY} }, /* (2012-W50-7) */
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

#ifdef notdef
typedef struct {
    Tcl_Obj *fmtObjPtr;
} ScanSwitches;

static Blt_SwitchSpec scanSwitches[] = 
{
    {BLT_SWITCH_OBJ, "-format", "",  (char *)NULL,
	Blt_Offset(ScanSwitches, fmtObjPtr), 0},
    {BLT_SWITCH_END}
};
#endif

/* 
 *-----------------------------------------------------------------------------
 *
 * InitParser --
 *
 *	Initializes the data structure used in parsing date/time strings.
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
InitParser(Tcl_Interp *interp, DateParser *parserPtr, const char *string) 
{
    const char *p;
    char *q;

    memset(parserPtr, 0, sizeof(DateParser));
    parserPtr->interp = interp;
    for (p = string, q = parserPtr->buffer; *p != '\0'; p++, q++) {
	if (isalpha(*p)) {
	    *q = tolower(*p);
	} else {
	    *q = *p;
	}
    }
    *q = '\0';
    parserPtr->nextCharPtr = parserPtr->buffer;
    parserPtr->tokens = Blt_Chain_Create();
}

/* 
 *-----------------------------------------------------------------------------
 *
 * FreeParser --
 *
 *	Releases the chain of tokens in the parser.
 *
 * Returns:
 *      None.
 *
 *-----------------------------------------------------------------------------
 */
static void
FreeParser(DateParser *parserPtr) 
{
    Blt_Chain_Destroy(parserPtr->tokens);
}

/* 
 *-----------------------------------------------------------------------------
 *
 * ParseError --
 *
 *	Formats and saves a formatted error message in the interpreter
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
 * GetId --
 *
 *	Returns the ID of the token.
 *
 * Returns:
 *	Returns the ID of the token.  If *tokenPtr* is NULL, _END is
 *	returned.
 *
 *-----------------------------------------------------------------------------
 */
static TokenId
GetId(Token *tokenPtr) 
{
    if (tokenPtr == NULL) {
	return _END;
    }
    return tokenPtr->id;
}

/* 
 *-----------------------------------------------------------------------------
 *
 * DeleteToken --
 *
 *	Removed the token from the chain of tokens.
 *
 * Returns:
 *	None.
 *
 *-----------------------------------------------------------------------------
 */
static void
DeleteToken(Token *tokenPtr) 
{
    if (tokenPtr->link != NULL) {
	Blt_Chain_DeleteLink(tokenPtr->chain, tokenPtr->link);
    }
}

/* 
 *-----------------------------------------------------------------------------
 *
 * ParseNumber --
 *
 *	Parse the given string as a number.
 *
 * Returns:
 *	Returns the token _NUMBER if successful, _END otherwise.
 *
 *-----------------------------------------------------------------------------
 */
static TokenId
ParseNumber(DateParser *parserPtr, const char *string)
{
    const char *p;
    long lvalue;
    int length, result;
    Token *tokenPtr;
    Tcl_Obj *objPtr;

    p = string;
    tokenPtr = parserPtr->currentPtr;
    while (isdigit(*p)) {
	p++;
    }
    length = p - string;
    objPtr = Tcl_NewStringObj(string, length);
    Tcl_IncrRefCount(objPtr);
    result = Blt_GetLongFromObj(parserPtr->interp, objPtr, &lvalue);
    Tcl_DecrRefCount(objPtr);
    if (result != TCL_OK) {
	ParseError(parserPtr->interp, "error parsing \"%*s\" as number", 
                length, string);
	return _UNKNOWN;
    }
    tokenPtr->lvalue = lvalue;
    tokenPtr->ident = string;
    tokenPtr->length = p - string;
    return tokenPtr->id = _NUMBER;
}

/* 
 *-----------------------------------------------------------------------------
 *
 * ParseString --
 *
 *	Parse the given string and tries to match against different
 *      identifiers to determine the ID.
 *
 * Returns:
 *	Returns the appropiate token.
 *
 *-----------------------------------------------------------------------------
 */
static TokenId
ParseString(DateParser *parserPtr, int length, const char *string)
{
    char c;
    int i;
    Token *tokenPtr;

    tokenPtr = parserPtr->currentPtr;
    c = tolower(string[0]);
    /* Month and weekday names (may be abbreviated). */
    if (length >= 3) {
	int i;

	/* Test for months. Allow abbreviations greater than 2 characters. */
	for (i = 0; i < numMonths; i++) {
	    const char *p;

	    p = monthNames[i];
	    if ((c == tolower(*p)) && (strncasecmp(p, string, length) == 0)) {
		tokenPtr->lvalue = i + 1;
		tokenPtr->ident = p;
		tokenPtr->length = 0;
		return tokenPtr->id = _MONTH;
	    }
	}
	/* Test for weekdays. Allow abbreviations greater than 2 characters. */
	for (i = 0; i < numWeekdays; i++) {
	    const char *p;

	    p = weekdayNames[i];
	    if ((c == tolower(*p)) && (strncasecmp(p, string, length) == 0)) {
		tokenPtr->lvalue = i;
		tokenPtr->length = 0;
		tokenPtr->ident = p;
		return tokenPtr->id = _WDAY;
	    }
	}
    }
    /* Timezone. */
    for (i = 0; i < numTimezones; i++) {
	IdentTable *p;

	p = tzTable + i;
	/* Test of timezomes. No abbreviations. */
	if ((c == p->name[0]) && (strncmp(p->name, string, length) == 0)) {
	    tokenPtr->lvalue = p->value;
	    tokenPtr->ident = p->name;
	    tokenPtr->length = 0;
	    return  tokenPtr->id = p->id;
	}
    }
    /* Meridian: am or pm. */
    if (length == 2) {
	int i;

	/* Test of meridian. */
	for (i = 0; i < numMeridians; i++) {
	    const char *p;

	    p = meridianNames[i];
	    if ((c == tolower(*p)) && (strncasecmp(p, string, length) == 0)) {
		tokenPtr->lvalue = i;
		tokenPtr->length = 0;
		return tokenPtr->id = _AMPM;
	    }
	}
    }
    /* Military timezone. Single letter a-z. */
    if (length == 1) {
	int i;

	/* Test of military timezones. Only one character wide. */
	for (i = 0; i < numMilitaryTimezones; i++) {
	    IdentTable *p;

	    p = milTzTable + i;
	    if (c == p->name[0]) {
		tokenPtr->ident = p->name;
		tokenPtr->length = 1;
		tokenPtr->lvalue = p->value;
		return tokenPtr->id = _STD;
	    }
	}
    }
    ParseError(parserPtr->interp, "unknown token \"%.*s\"", length, string);
    return _UNKNOWN;
}

/* 
 *-----------------------------------------------------------------------------
 *
 * FirstToken --
 *
 *	Returns the first token in the chain of token.
 *
 *-----------------------------------------------------------------------------
 */
static Token *
FirstToken(DateParser *parserPtr)
{
    Blt_ChainLink link;

    link = Blt_Chain_FirstLink(parserPtr->tokens);
    if (link == NULL) {
	return NULL;
    }
    return Blt_Chain_GetValue(link);
}

/* 
 *-----------------------------------------------------------------------------
 *
 * LastToken --
 *
 *	Returns the last token in the chain of token.
 *
 *-----------------------------------------------------------------------------
 */
static Token *
LastToken(DateParser *parserPtr)
{
    Blt_ChainLink link;

    link = Blt_Chain_LastLink(parserPtr->tokens);
    if (link == NULL) {
	return NULL;
    }
    return Blt_Chain_GetValue(link);
}

/* 
 *-----------------------------------------------------------------------------
 *
 * NextToken --
 *
 *	Returns the next token in the chain of token.
 *
 *-----------------------------------------------------------------------------
 */
static Token *
NextToken(Token *tokenPtr)
{
    Blt_ChainLink link;

    if (tokenPtr->link == NULL) {
	return NULL;
    }
    link = Blt_Chain_NextLink(tokenPtr->link);
    if (link == NULL) {
	return NULL;
    }
    return Blt_Chain_GetValue(link);
}

/* 
 *-----------------------------------------------------------------------------
 *
 * PrevToken --
 *
 *	Returns the previous token in the chain of token.
 *
 *-----------------------------------------------------------------------------
 */
static Token *
PrevToken(Token *tokenPtr)
{
    Blt_ChainLink link;

    if (tokenPtr->link == NULL) {
	return NULL;
    }
    link = Blt_Chain_PrevLink(tokenPtr->link);
    if (link == NULL) {
	return NULL;
    }
    return Blt_Chain_GetValue(link);
}

/* 
 *-----------------------------------------------------------------------------
 *
 * NumberTokens --
 *
 *	Returns the number of tokens left in the chain of tokens.
 *
 *-----------------------------------------------------------------------------
 */
static int
NumberTokens(DateParser *parserPtr) 
{
    return Blt_Chain_GetLength(parserPtr->tokens);
}

/* 
 *-----------------------------------------------------------------------------
 *
 * NumberTokens --
 *
 *	Returns the symbol name of the given token.
 *
 *-----------------------------------------------------------------------------
 */
static const char *
TokenName(Token *tokenPtr) 
{
    if (tokenPtr == NULL) {
	return tokenNames[_END];
    } 
    return tokenNames[tokenPtr->id];
}


static void
PrintTokens(DateParser *parserPtr) 
{
    Token *tokenPtr;

    fprintf(stderr, "tokens = [ ");
    for (tokenPtr = FirstToken(parserPtr); tokenPtr != NULL;
	 tokenPtr = NextToken(tokenPtr)) {
	fprintf(stderr, "%s ",TokenName(tokenPtr));
    }
    fprintf(stderr, "]\n");
}

/* 
 *-----------------------------------------------------------------------------
 *
 * GetNextToken --
 *
 *	Parses the next token from the date string.
 *
 *-----------------------------------------------------------------------------
 */
static int
GetNextToken(DateParser *parserPtr, TokenId *idPtr)
{
    char *p;
    TokenId id;
    Token *tokenPtr;
    Blt_ChainLink link;

    link = Blt_Chain_AllocLink(sizeof(Token));
    Blt_Chain_LinkAfter(parserPtr->tokens, link, NULL);
    tokenPtr = Blt_Chain_GetValue(link);
    tokenPtr->link = link;
    tokenPtr->chain = parserPtr->tokens;
    parserPtr->currentPtr = tokenPtr;
    p = parserPtr->nextCharPtr;
    while (isspace(*p)) {
	p++;				/* Skip leading spaces. */
    }
    if (*p == '/') {
	id = _SLASH;
	p++;
    } else if (*p == '+') {
	id = _PLUS;
	p++;
    } else if (*p == '\'') {
	id = _QUOTE;
	p++;
    } else if (*p == ':') {
	id = _COLON;
	p++;
    } else if (*p == '.') {
	id = _DOT;
	p++;
    } else if (*p == ',') {
	id = _COMMA;
	p++;
    } else if (*p == '-') {
	id = _DASH;
	p++;
    } else if ((*p == 'w') && (isdigit(*(p+1))) && (isdigit(*(p+2)))) {
	tokenPtr->ident = p;
	tokenPtr->lvalue = (p[1] - '0') * 10 + (p[2] - '0');
	id = _WEEK;
	tokenPtr->length = 3;
	p += tokenPtr->length;
    } else if (isdigit(*p)) {
	char *start;
	char save;

	start = p;
	while (isdigit(*p)) {
	    p++;
	}
	save = *p;
	*p = '\0';
	id = ParseNumber(parserPtr, start);
	if (id == _UNKNOWN) {
	    return TCL_ERROR;
	}
	*p = save;			/* Restore last chararacter. */
    } else if (isalpha(*p)) {
	char name[BUFSIZ];
	int i;
	
	for (i = 0; ((isalpha(*p)) || (*p == '.')) && (i < 200); p++) {
	    if (*p != '.') {
		name[i] = *p;
		i++;
	    }
	}
	id = ParseString(parserPtr, i, name);
	if (id == _UNKNOWN) {
	    return TCL_ERROR;
	}
    } else if (*p == '\0') {
	id = _END;
    } else {
	id = _UNKNOWN;
    }
    *idPtr = parserPtr->currentPtr->id = id;
    parserPtr->nextCharPtr = p;
    return TCL_OK;
}

/* 
 *-----------------------------------------------------------------------------
 *
 * ProcessTokens --
 *
 *	Parses the date string into a chain of tokens.
 *
 *-----------------------------------------------------------------------------
 */
static int
ProcessTokens(DateParser *parserPtr)
{
    TokenId id;

    id = _END;
    do {
	if (GetNextToken(parserPtr, &id) != TCL_OK) {
	    return TCL_ERROR;
	}
    } while (id != _END);
    return TCL_OK;
}

/* 
 *-----------------------------------------------------------------------------
 *
 * MatchDatePattern --
 *
 *	After having extracted the time, timezone, etc. from the chain
 *      of tokens, try to match the remaining tokens with known date 
 *      patterns.
 *
 * Returns:
 *      Return the index of the matching pattern or -1 if no pattern
 *      matches.
 *
 *-----------------------------------------------------------------------------
 */
static int
MatchDatePattern(DateParser *parserPtr)
{
    int i;

    /* Find the first colon. */
    for (i = 0; i < numDatePatterns; i++) {
	int j;
	Token *tokenPtr;
	Pattern *patPtr;

	patPtr = datePatterns + i;
	if (patPtr->numIds != NumberTokens(parserPtr)) {
	    continue;
	}
	for (j = 0, tokenPtr = FirstToken(parserPtr); 
	     (tokenPtr != NULL) && (j < patPtr->numIds); 
	     tokenPtr = NextToken(tokenPtr), j++) {
	    TokenId id;

	    id = patPtr->ids[j];
	    if (tokenPtr->id != id) {
		if ((tokenPtr->id == _NUMBER) && (tokenPtr->length <= 2)){
		    switch (id) {
		    case _WDAY:
			if ((tokenPtr->lvalue > 0) && (tokenPtr->lvalue <= 7)) {
			    continue;
			}
			ParseWarning(parserPtr->interp, 
                                "weekday \"%d\" is out of range", 
                                tokenPtr->lvalue);
			break;

		    case _MONTH:
			if (tokenPtr->lvalue <= 12) {
			    continue;
			}
			ParseWarning(parserPtr->interp, 
                                "month \"%d\" is out of range", 
                                tokenPtr->lvalue);
			break;

		    case _MDAY:
			if (tokenPtr->lvalue <= 31) {
			    continue;
			}
			ParseWarning(parserPtr->interp, 
                                "day \"%d\" is out of range", 
                                tokenPtr->lvalue);
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
 * GetTz --
 *
 *	Trys to parse a timezone specification from the current
 *      point in the parser.
 *
 * Returns:
 *      Return a pointer to the timezone token or NULL is no timezone
 *      specification is found.
 *
 *-----------------------------------------------------------------------------
 */
static Token *
GetTz(Token *tokenPtr, int *mPtr)
{
    int m, sign;
    
    sign = 1;
    if (GetId(tokenPtr) == _DASH) {
	sign = -1;
    }
    /* Verify the next token after the +/-/' is a number.  */
    tokenPtr = NextToken(tokenPtr);
    if (GetId(tokenPtr) != _NUMBER) {
	return NULL;
    }
    /* The timezone is in the form NN:NN or NNNN. */
    if (tokenPtr->length == 4) {
	m = ((tokenPtr->lvalue / 100) * 60) + (tokenPtr->lvalue % 100);
    } else if (tokenPtr->length == 2) {
	m = HRS2MINS(tokenPtr->lvalue);
	tokenPtr = NextToken(tokenPtr);
	if (GetId(tokenPtr) != _COLON) {
	    *mPtr = sign * m;
	    return tokenPtr;
	}
	tokenPtr = NextToken(tokenPtr);
	if ((GetId(tokenPtr) != _NUMBER) || (tokenPtr->length != 2)) {
	    return NULL;
	}
	m += tokenPtr->lvalue;
    } else {
	return NULL;                    /* Error: expecting 2 digit or 4
                                         * digit number after plus or
                                         * minus. */
    }
    tokenPtr = NextToken(tokenPtr);
    *mPtr = sign * m;
    return tokenPtr;
}

/* 
 *-----------------------------------------------------------------------------
 *
 * ExtractTimezone --
 *
 *	Searches for a possible timezone token (or serier of token) and
 *      removes them from the parser chain.
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
ExtractTimezone(DateParser *parserPtr)
{
    int isdst, m;
    Token *tokenPtr;

    m = 0;
    isdst = 0;
    for (tokenPtr = LastToken(parserPtr); tokenPtr != NULL;
	 tokenPtr = PrevToken(tokenPtr)) {
	switch (tokenPtr->id) {
	case _DST:			/* (EDT) */
	    isdst = 1;
	    m = HRS2MINS(tokenPtr->lvalue);
	    isdst = TRUE;
	    goto found;
	    
	case _STD:			/* (EST) */
	    m = HRS2MINS(tokenPtr->lvalue);
	    goto found;

	break;
	default:
	    break;
	}
    }
    if (tokenPtr == NULL) {
	return TCL_OK;			/* No timezone found. */
    }
found:
    DeleteToken(tokenPtr);
    parserPtr->date.tzoffset = m;
    parserPtr->date.isdst = isdst;
    parserPtr->flags |= PARSE_TZ;
    return TCL_OK;
}


static int
GetTimezone(DateParser *parserPtr, Token *tokenPtr, Token **resPtrPtr)
{
    int isdst, m;

    isdst = 0;
    switch (GetId(tokenPtr)) {
    case _PLUS:                         /* +00:00 */
    case _DASH:                         /* -00:00 */
	tokenPtr = GetTz(tokenPtr, &m);
	break;

    case _DST:				/* EDT */
	m = HRS2MINS(tokenPtr->lvalue);
	isdst = TRUE;
	tokenPtr = NextToken(tokenPtr);
	goto done;

    case _STD:				/* EST */
	m = HRS2MINS(tokenPtr->lvalue);
	tokenPtr = NextToken(tokenPtr);
	goto done;

    default:
	return TCL_CONTINUE;
    }
    if (tokenPtr == NULL) {
	return TCL_ERROR;
    }
 done:
    *resPtrPtr = tokenPtr;
    parserPtr->date.tzoffset = m;
    parserPtr->date.isdst = isdst;
    parserPtr->flags |= PARSE_TZ;
    return TCL_OK;
}

/* 
 *-----------------------------------------------------------------------------
 *
 * ExtractWeekday --
 *
 *	Searches for a weekday token and removes it from the parser chain.
 *      The weekday would have been specified are a weekday name
 *      (e.g. Tuesday).
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
ExtractWeekday(DateParser *parserPtr)
{
    Token *tokenPtr;

    for (tokenPtr = FirstToken(parserPtr); tokenPtr != NULL; 
	 tokenPtr = NextToken(tokenPtr)) {
	if (tokenPtr->id == _WDAY) {
	    parserPtr->date.wday = tokenPtr->lvalue; /* 0-6 */
	    DeleteToken(tokenPtr);
	    return;
	}
    }
}

static int
ExtractSeparator(DateParser *parserPtr)
{
    Token *tokenPtr;

    /* Find the date/time separator "t" and remove it. */
    for (tokenPtr = FirstToken(parserPtr); tokenPtr != NULL; 
	 tokenPtr = NextToken(tokenPtr)) {
	if ((tokenPtr->id == _STD) && (tokenPtr->length == 1) &&
	    (tokenPtr->ident[0] == 't')) {
	    Token *nextPtr;

	    /* Check if the 't' is a military timezone or a separator. */
	    nextPtr = NextToken(tokenPtr);
	    if (GetId(nextPtr) != _END) {
		DeleteToken(tokenPtr);
	    }
	    return TCL_OK;
	}
    }
    return TCL_ERROR;
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

static int
GetDateFromOrdinalDay(Tcl_Interp *interp, int year, int yday, int *monthPtr)
{
    int isLeapYear, numDays, mon;

    numDays = yday;
    mon = 0;
    isLeapYear = IsLeapYear(year);
    while (numDays >= numDaysMonth[IsLeapYear(year)][mon]) {
        numDays -= numDaysMonth[IsLeapYear(year)][mon];
        mon++;
    }
    *monthPtr = mon;
    return numDays + 1;                 /* mday is 1-31 */
}

#endif

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

    
/*
 *---------------------------------------------------------------------------
 *
 * FixTokens --
 *
 *	Convert 3, 4, and 8 digit number token IDs to more specific IDs.
 *	This is done for pattern matching. 
 *
 *	3 digit number		oridinal day of year (ddd)
 *	4 digit number		year (yyyy)
 *	7 digit number		year (yyyyddd)
 *	8 digit number		ISO date format (yyyymmdd)
 *
 * Results:
 *	None.
 *
 *---------------------------------------------------------------------------
 */
static void
FixTokens(DateParser *parserPtr)
{
    Token *tokenPtr;

    for (tokenPtr = FirstToken(parserPtr); tokenPtr != NULL; 
	 tokenPtr = NextToken(tokenPtr)) {
	if (tokenPtr->id == _NUMBER) {
	    if (tokenPtr->length == 3) {
		tokenPtr->id = _YDAY;
	    } else if (tokenPtr->length == 4) {
		tokenPtr->id = _YEAR;
	    } else if (tokenPtr->length == 7) {
		tokenPtr->id = _ISO7;
	    } else if (tokenPtr->length == 8) {
		tokenPtr->id = _ISO8;
	    }
	}
    }
}

/*
 *---------------------------------------------------------------------------
 *
 * ExtractTime --
 *
 *	Find and extract the time related tokens in the list.  
 *	The time format is
 *
 *		time + ampm + timezone
 *
 *	where ampm and timezone are optional.  It's easier to extract
 *	the meridian and timezone once we already found a time.
 *
 *	The following time patterns are accepted:
 *
 *	hh:mm						10:21
 *	hh:mm:ss					10:21:00
 *	hh:mm:ss:fff	fff is milliseconds.		10:21:00:001
 *	hh:mm:ss.f+	f+ is fraction of seconds.	10:21:00.001
 *	hh:mm:ss,f+	f+ is fraction of seconds.	10:21:00,001
 *	hhmmss		ISO time format			102100
 *
 * Results:
 *	None.
 *
 * Side Effects:
 *	The matching tokens are removed from the list and the parser
 *	structure is updated with the new information.
 *
 *---------------------------------------------------------------------------
 */
static int
ExtractTime(DateParser *parserPtr)
{
    int result;
    Token *nextPtr, *firstPtr, *lastPtr;
    Token *tokenPtr, *hourPtr, *minPtr;

    firstPtr = lastPtr = NULL;
    /* Find the starting pattern "num colon num". */
    for (tokenPtr = FirstToken(parserPtr); tokenPtr != NULL; 
	 tokenPtr = NextToken(tokenPtr)) {
	Token *colonPtr;

	hourPtr = firstPtr = tokenPtr;
	if ((GetId(tokenPtr) == _NUMBER) && (tokenPtr->length == 6)) {
	    long value;

	    /* Assuming that any 6-digit number is a time. */
	    /* ISO time format hhmmss */
	    value = tokenPtr->lvalue;
	    parserPtr->date.sec = value % 100;
	    value /= 100;
	    parserPtr->date.min = value % 100;
	    value /= 100;
	    parserPtr->date.hour = value % 100;
	    firstPtr = tokenPtr;
	    tokenPtr = NextToken(tokenPtr);
	    goto done;
	}
	if ((GetId(hourPtr) != _NUMBER) || (hourPtr->length > 2)) {
	    continue;
	}
	colonPtr = NextToken(tokenPtr);
	if (GetId(colonPtr) != _COLON) {
	    continue;
	}
	minPtr = NextToken(colonPtr);
	if ((GetId(minPtr) != _NUMBER) || (minPtr->length > 2)) {
	    continue;
	}
	tokenPtr = NextToken(minPtr);
	break;				/* Found the starting pattern */
    }
    if (tokenPtr == NULL) {
	for (tokenPtr = FirstToken(parserPtr); tokenPtr != NULL; 
	     tokenPtr = NextToken(tokenPtr)) {
	    if ((GetId(tokenPtr) == _NUMBER) && 
		((tokenPtr->length == 6) || (tokenPtr->length == 14))) {
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
		    firstPtr = tokenPtr = NextToken(tokenPtr);
		} else {
		    firstPtr = tokenPtr;
		    tokenPtr = NextToken(tokenPtr);
		}
		goto done;
	    }
	}
	return TCL_OK;			/* No time tokens found. */
    }
    parserPtr->date.hour = hourPtr->lvalue;
    parserPtr->date.min = minPtr->lvalue;
    if (GetId(tokenPtr) != _COLON) {
	goto done;
    }
    tokenPtr = NextToken(tokenPtr);
    if ((GetId(tokenPtr) != _NUMBER) || (tokenPtr->length > 2)) {
	goto done;
    }
    parserPtr->date.sec = tokenPtr->lvalue;
    tokenPtr = NextToken(tokenPtr);
    if (GetId(tokenPtr) == _COLON) {
	tokenPtr = NextToken(tokenPtr);
	if ((GetId(tokenPtr) == _NUMBER) && (tokenPtr->length == 3)) {
	    parserPtr->date.frac = tokenPtr->lvalue * 1e-3;
	    tokenPtr = NextToken(tokenPtr);
	}
    } else if ((GetId(tokenPtr) == _DOT) || (GetId(tokenPtr) == _COMMA)) {
	tokenPtr = NextToken(tokenPtr);
	if ((GetId(tokenPtr) == _NUMBER)) {
	    double d;

	    d = pow(10.0, tokenPtr->length);
	    parserPtr->date.frac = tokenPtr->lvalue / d;
	    tokenPtr = NextToken(tokenPtr);
	}
    }
 done:
    /* Look for AMPM designation. */
    if (GetId(tokenPtr) == _AMPM) {
	if (parserPtr->date.hour > 12) {
	    fprintf(stderr, "invalid am/pm, already in 24hr format\n");
	}
	if (tokenPtr->lvalue) {
	    parserPtr->date.hour += 12;
	}
	tokenPtr = NextToken(tokenPtr);
    }
    result = GetTimezone(parserPtr, tokenPtr, &nextPtr);
    if (result == TCL_ERROR) {
	return TCL_ERROR;
    }
    if (result == TCL_OK) {
	tokenPtr = nextPtr;
    }
    /* Remove the time-related tokens from the list. */
    lastPtr = tokenPtr;
    for (tokenPtr = firstPtr; tokenPtr != NULL; tokenPtr = nextPtr) {
	nextPtr = NextToken(tokenPtr);
	if (tokenPtr == lastPtr) {
	    break;
	}
	DeleteToken(tokenPtr);
    }
    parserPtr->flags |= PARSE_TIME;
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * ExtractDate --
 *
 *	Find and extract the date related tokens in the list.  
 *	The possible date formats are
 *
 *	+ddd:hh:mm:ss		+000:00:00:00
 *	dd mon yyyy		(31 Jan 2012) 
 *	dd-mm-yyyy		(31-Jan-2012) 
 *	dd.mm yyyy		(18. Feb 1921) 
 *	dd.mm.yyy		(31.Jan.2012) 
 *	dd/mm/yyyy		(31/Jan/2012) 
 *	ddd:hh:mm:ss		000:00:00:00
 *	mm dd, yyyy		(Jan 31, 2012) 
 *	mm-dd-yyyy		(Jan-31-2012) 
 *	mm.dd.yyyy		(Jan.31.2012) 
 *	mm/dd/yyyy		(Jan/31/2012) 
 *	mon dd yyyy		(Jan 31 2012) 
 *	mon yyyy		(Jan 2012) 
 *	mon/dd			(12/23) 
 *	wday, dd, mon, yy	(Thu, 01 Jan 04) 
 *	yyyy			(2012) 
 *	yyyy mon		(2012 Jan) 
 *	yyyy-Www		(2012-W01) 
 *	yyyy-Www-d		(2012-W50-7) 
 *	yyyy-ddd		(2012-100) 
 *	yyyy-mm			(1957-Dec) 
 *	yyyy-mm-dd		(2012-Jan-31) 
 *	yyyy/mm/dd		(2012.Jan.31) 
 *	yyyyWww			(2012W01)
 *	yyyyWwwd		(2012W017) 
 *	yyyyddd			(2012100) 
 *	yyyymmdd		(20120131) 
 *
 * Results:
 *	None.
 *
 * Side Effects:
 *	The matching tokens are removed from the list and the parser
 *	structure is updated with the new information.
 *
 *---------------------------------------------------------------------------
 */
static int
ExtractDate(DateParser *parserPtr)
{
    Token *nextPtr, *tokenPtr;
    int i, patternIndex;
    Pattern *patPtr;

    if (NumberTokens(parserPtr) == 1) {
	return TCL_OK;
    }
    /* Remove the weekday description. */
    ExtractWeekday(parserPtr);
    /*  */
    FixTokens(parserPtr);
    patternIndex = MatchDatePattern(parserPtr);
    if (patternIndex < 0) {
	PrintTokens(parserPtr);
	ParseError(parserPtr->interp, "\nNo matching date pattern %s", 
                parserPtr->buffer);
	return TCL_ERROR;
    }
    Tcl_ResetResult(parserPtr->interp);
    /* Process the list against the matching pattern. */
    patPtr = datePatterns + patternIndex;
    assert(patPtr->numIds == NumberTokens(parserPtr));
    tokenPtr = FirstToken(parserPtr); 
    for (i = 0; i < patPtr->numIds; i++, tokenPtr = NextToken(tokenPtr)) {
	TokenId id;
	
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
    for (tokenPtr = FirstToken(parserPtr); tokenPtr != NULL; 
	 tokenPtr = nextPtr) {
	nextPtr = NextToken(tokenPtr);
	DeleteToken(tokenPtr);
    }
    parserPtr->flags |= PARSE_DATE;
    return TCL_OK;
}


static int
NumberDaysFromStartOfYear(int year, int month, int mday)
{
    int numDays;

    numDays = numDaysToMonth[IsLeapYear(year)][month];
    return numDays + (mday - 1);
}

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
 * ConvertDate --
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
ConvertDate(Tcl_Interp *interp, DateParser *parserPtr, double *secondsPtr)
{
    int isLeapYear;
    Blt_DateTime *datePtr;

    datePtr = &parserPtr->date;
#ifdef notdef
    fprintf(stderr, "ConvertDate: year=%d mon=%d mday=%d week=%d\n", 
            datePtr->year, datePtr->mon, datePtr->mday, datePtr->week);
#endif
    isLeapYear = IsLeapYear(datePtr->year);
    /* Check the inputs for validity */
    if ((datePtr->year < 0) || (datePtr->year > 9999)) {
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
                           datePtr->week);
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
                           "day of year \"%d\" is out of range for \"%s\"",
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
            ParseError(interp, "hour \"%d\" is out of range.",
                       datePtr->hour); 
        }
        return TCL_ERROR;
    }
    if ((datePtr->min < 0) || (datePtr->min > 59)) {
        if (interp != NULL) {
            ParseError(interp, "minute \"%d\", is out of range.", 
                       datePtr->min);
        }
        return TCL_ERROR;
    }
    if ((datePtr->sec < 0) || (datePtr->sec > 60)) {
        if (interp != NULL) {
            ParseError(interp, "second \"%d\" is out of range.", 
                       datePtr->sec);
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

#ifdef notdef
    if ((parserPtr->flags & PARSE_WEEK) && (datePtr->week > 0)) {
	int mday, mon, wday, corr, numDays;
        
        /* If the date was given by week number + week day, convert to
         * month, and mday. Note that the year may change. */

        /* Get the week day of the first week of the year. */
	wday = GetWeekDay(datePtr->year, 1, 4);
	corr = wday + 3;
	numDays = ((datePtr->week) * 7) + (datePtr->wday) + 
            IsLeapYear(datePtr->year) - corr;
	mday = GetDateFromOrdinalDay(interp, datePtr->year, numDays, &mon);
        datePtr->yday = numDays;
        datePtr->mday = mday;
        datePtr->mon = mon;
        fprintf(stderr, "yday=%d week=%d wday=%d mday=%d\n",
                datePtr->yday, datePtr->week, datePtr->wday, datePtr->mday);
    }
#endif
    Blt_DateToSeconds(datePtr, secondsPtr);
    return TCL_OK;
}

/* 
 * Date and time formats:
 *
 *	<date> <time>
 *	<date>T<time>
 */
/* 
 * Date formats:
 *
 *	wday, month day, year 
 *	wday month day, year
 *	wday, day. month year
 *	wday, day month year
 *	wday day month year
 *	month/day/year
 *	day/month/year
 *	year/month/day
 *	day-month-year
 *	year-month-day
 *	month day
 *	month-day
 *	wday month day
 *	day. month
 *	wday day. month
 *	day month
 *	wday day month
 *	year-month-day
 *	year-month-day
 *	month, year
 *	month year
 */

/* 
 * Time formats:
 *
 *	time <meridian> <tz>
 *	hh
 *	hh:mm
 *	hh:mm:ss
 *	hhmm
 *	hhmmss
 *	ss.frac			- a single number interger or floating point
 *	hh:mm:ss:mmm
 *	hh:mm
 */

/* 
 * Timezone
 *
 *	timezone
 *	'-+1234
 *	a-z	
 */

/* 
 * Date formats:
 *
 *	day month yyyy
 *	dd month
 *	dd - month-yyyy
 *	dd . month
 *	dd . month yyyy
 *	dd / month / yyyy
 *	month day
 *	month dd
 *	month dd yyyy
 *	month dd , yyyy
 *	month yyyy
 *	month , yyyy
 *	month - dd
 *	month - dd-yyyy
 *	month / dd / yyyy
 *	yyyy
 *	yyyy - Www
 *	yyyy - Www - D
 *	yyyy - mm
 *	yyyy - mm - dd
 *	yyyy - month
 *	yyyy - month - dd
 *	yyyy / mm / dd
 *	yyyyWww
 *	yyyyWwwD
 *	yyyymmdd

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
	on DD YYYY HH:MIAM 	Jan 1 2005 1:29PM 1
	MM/DD/YY	 	11/23/98
	MM/DD/YYYY		11/23/1998
	YY.MM.DD	 	72.01.01
	YYYY.MM.DD	 	1972.01.01
	DD/MM/YY	 	19/02/72
	DD/MM/YYYY	 	19/02/1972
	DD.MM.YY	 	25.12.05
	DD.MM.YYYY		25.12.2005
	DD-MM-YY		24-01-98
	DD-MM-YYYY 		24-01-1998
	DD Mon YY	 	04 Jul 06 1
	DD Mon YYYY	 	04 Jul 2006 1
	Mon DD, YY	 	Jan 24, 98 1
	Mon DD, YYYY	 	Jan 24, 1998 1
	HH:MM:SS	 	03:24:53
	Mon DD YYYY HH:MI:SS:MMMAM Apr 28 2006 12:32:29:253PM 1
	MM-DD-YY		01-01-06
	MM-DD-YYYY		01-01-2006
	YY/MM/DD	 	98/11/23
	YYYY/MM/DD	 	1998/11/23
	YYMMDD			980124
	YYYYMMDD 	 	19980124
	DD Mon YYYY HH:MM:SS:MMM 	28 Apr 2006 00:34:55:190 
	HH:MI:SS:MMM	  	11:34:23:013
	YYYY-MM-DD HH:MI:SS 	1972-01-01 13:42:24
	YYYY-MM-DD HH:MI:SS.MMM	1972-02-19 06:35:24.489
	YYYY-MM-DDTHH:MM:SS:MMM 1998-11-23T11:25:43:250
	DD Mon YYYY HH:MI:SS:MMMAM	28 Apr 2006 12:39:32:429AM 1
	DD/MM/YYYY HH:MI:SS:MMMAM  	28/04/2006 12:39:32:429AM

	YY-MM-DD		99-01-24
	YYYY-MM-DD		1999-01-24
	MM/YY		 	08/99
	MM/YYYY		 	12/2005
	YY/MM		 	99/08
	YYYY/MM			2005/12
	Month DD, YYYY		July 04, 2006
	Mon YYYY		Apr 2006 1
	Month YYYY 		February 2006
	DD Month		11 September 
	Month DD		September 11
	DD Month YY		19 February 72 
	DD Month YYYY		11 September 2002 
	MM-YY		 	12-92
	MM-YYYY		 	05-2006
	YY-MM		 	92-12
	YYYY-MM		 	2006-05
	MMDDYY		 	122506
	MMDDYYYY 	 	12252006
	DDMMYY		 	240702
	DDMMYYYY 	 	24072002
	Mon-YY			Sep-02 1
	Mon-YYYY		Sep-2002 
	DD-Mon-YY		25-Dec-05 
	DD-Mon-YYYY 

	mm/dd/yy	 	11/23/98
	mm/dd/yyyy		11/23/1998
	yy.mm.dd	 	72.01.01
	yyyy.mm.dd	 	1972.01.01
	dd/mm/yy	 	19/02/72
	dd/mm/yyyy	 	19/02/1972
	dd.mm.yy	 	25.12.05
	dd.mm.yyyy		25.12.2005
	dd-mm-yy		24-01-98
	dd-mm-yyyy 		24-01-1998
	dd mon yy	 	04 Jul 06 1
	dd mon yyyy	 	04 Jul 2006 1
	mon dd, yy	 	Jan 24, 98 1
	mon dd, yyyy	 	Jan 24, 1998 1
	mm-dd-yy		01-01-06
	mm-dd-yyyy		01-01-2006
	yy/mm/dd	 	98/11/23
	yyyy/mm/dd	 	1998/11/23
	yymmdd			980124
	yyyymmdd 	 	19980124
	dd mon yyyy HH:MM:SS:MMM 	28 Apr 2006 00:34:55:190 
	hh:mi:ss:mmm	  	11:34:23:013
	yyyy-mm-dd HH:MI:SS 	1972-01-01 13:42:24
	yyyy-mm-dd HH:MI:SS.MMM	1972-02-19 06:35:24.489
	yyyy-mm-ddthh:MM:SS:MMM 1998-11-23T11:25:43:250
	dd mon yyyy HH:MI:SS:MMMAM	28 Apr 2006 12:39:32:429AM 1
	dd/mm/yyyy HH:MI:SS:MMMAM  	28/04/2006 12:39:32:429AM

	yy-mm-dd		99-01-24
	yyyy-mm-dd		1999-01-24
	mm/yy		 	08/99
	mm/yyyy		 	12/2005
	yy/mm		 	99/08
	yyyy/mm			2005/12
	month dd, yyyy		July 04, 2006
	mon yyyy		Apr 2006 1
	month yyyy 		February 2006
	dd month		11 September 
	month dd		September 11
	dd month yy		19 February 72 
	dd month yyyy		11 September 2002 
	mm-yy		 	12-92
	mm-yyyy		 	05-2006
	yy-mm		 	92-12
	yyyy-mm		 	2006-05
	mmddyy		 	122506
	mmddyyyy 	 	12252006
	ddmmyy		 	240702
	ddmmyyyy 	 	24072002
	mon-yy			Sep-02
	mon-yyyy		Sep-2002 
	dd-mon-yy		25-Dec-05 
	dd-mon-yyyy 

	D:yyyymmddhhmmss+'HHMM
 */

/* 
 *-----------------------------------------------------------------------------
 *
 * ParseDate --
 *
 *      Parses the date string into the number of seconds since the epoch.
 *      The date string can be in one many formats accepted.
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
ParseDate(Tcl_Interp *interp, const char *string, double *secondsPtr)
{
    DateParser parser;

    InitParser(interp, &parser, string);

    /* Create list of tokens from date string. */
    if (ProcessTokens(&parser) != TCL_OK) {
        goto error;
    }

    /* Remove the time/date 'T' separator if one exists. */
    ExtractSeparator(&parser);
    /* Now parse out the time, timezone, and then date. */
    if ((ExtractTime(&parser) != TCL_OK) ||
	(ExtractTimezone(&parser) != TCL_OK) ||
	(ExtractDate(&parser) != TCL_OK)) {
	goto error;
    }
    if (ConvertDate(interp, &parser, secondsPtr) != TCL_OK) {
	goto error;
    }
    FreeParser(&parser);
    return TCL_OK;
 error:
    FreeParser(&parser);
    return TCL_ERROR;
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
    if (Blt_ParseSwitches(interp, formatSwitches, objc - 3, objv + 3, &switches,
			  BLT_SWITCH_DEFAULTS) < 0) {
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
ScanOp(ClientData clientData, Tcl_Interp *interp, int objc, 
       Tcl_Obj *const *objv)
{
    double t;
    int result;

    result = Blt_GetTimeFromObj(interp, objv[2], &t);
    if (result == TCL_OK) {
	Tcl_SetDoubleObj(Tcl_GetObjResult(interp), t);
    }
    return result;
}

/*
 *---------------------------------------------------------------------------
 *
 * DateObjCmd --
 *
 *---------------------------------------------------------------------------
 */
static Blt_OpSpec dateCmdOps[] =
{
    {"format",  1, FormatOp,      3, 0, "seconds ?switches?",},
    {"scan",    1, ScanOp,        3, 3, "date",},
};

static int numCmdOps = sizeof(dateCmdOps) / sizeof(Blt_OpSpec);

/*ARGSUSED*/
static int
DateObjCmd(
    ClientData clientData,              /* Interpreter-specific data. */
    Tcl_Interp *interp,
    int objc,
    Tcl_Obj *const *objv)
{
    Tcl_ObjCmdProc *proc;

    proc = Blt_GetOpFromObj(interp, numCmdOps, dateCmdOps, BLT_OP_ARG1, objc, 
	objv, 0);
    if (proc == NULL) {
	return TCL_ERROR;
    }
    return (*proc) (clientData, interp, objc, objv);
}

/*
 *---------------------------------------------------------------------------
 *
 * Blt_DateScanCmdInitProc --
 *
 *	This procedure is invoked to initialize the "date" command.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Creates the new command.
 *
 *---------------------------------------------------------------------------
 */
int
Blt_DateScanCmdInitProc(Tcl_Interp *interp)
{
    static Blt_CmdSpec cmdSpec = { 
	"date", DateObjCmd, 
    };
    return Blt_InitCmd(interp, "::blt", &cmdSpec);
}

/* Public routines. */

/*
 *-----------------------------------------------------------------------------
 *
 * Blt_DateToSeconds --
 *
 *      Converts a date structure into the number of second since the
 *      epoch.  The date can contain fractional seconds.  The date fields
 *      year, mon, and mday are used to compute the time, while week, wday,
 *      and yday and ignored.  You much have previously converted the them
 *      to year, mon, and mday.
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
    fprintf(stderr, "Entering Blt_DateToSeconds: year=%d mon=%d mday=%d week=%d hour=%d min=%d sec=%d\n", 
            datePtr->year, datePtr->mon, datePtr->mday, datePtr->week,
            datePtr->hour, datePtr->min, datePtr->sec);
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

    if (datePtr->isdst > 0) {
	datePtr->hour++; 
    }
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
    while (rem < 0) {
 fprintf(stderr, "rem=%ld numDays=%ld rem < 0\n", rem, numDays);
        rem += SECONDS_DAY;
        numDays--;
    }
    while (rem >= SECONDS_DAY) {
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
    fprintf(stderr, "Leaving Blt_SecondsToDate: y=%d m=%d mday=%d wday=%d yday=%d week=%d wyear=%d hour=%d min=%d sec=%d\n",
            datePtr->year, datePtr->mon, datePtr->mday, datePtr->wday,
            datePtr->yday, datePtr->week, datePtr->wyear, datePtr->hour,
            datePtr->min, datePtr->sec);
#endif
    datePtr->isdst = 0;
}

/* 
 *-----------------------------------------------------------------------------
 *
 * Blt_GetTime --
 *
 *	Converts a date string into the number of seconds from the epoch.
 *
 *-----------------------------------------------------------------------------
 */
int
Blt_GetTime(Tcl_Interp *interp, const char *string, double *secondsPtr)
{
    if (ParseDate(interp, string, secondsPtr) == TCL_OK) {
        return TCL_OK;
    }
    return TCL_ERROR;
}

/* 
 *-----------------------------------------------------------------------------
 *
 * Blt_GetTimeFromObj --
 *
 *	Converts a Tcl_Obj representing a date string into the number of
 *	seconds from the epoch (GMT time).
 *
 *-----------------------------------------------------------------------------
 */
int
Blt_GetTimeFromObj(Tcl_Interp *interp, Tcl_Obj *objPtr, double *secondsPtr)
{
    if (ParseDate(interp, Tcl_GetString(objPtr), secondsPtr) == TCL_OK) {
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
            count += 3 + 3 + 2 + 8 + 4 + 4;
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
        case 'G':                       /* ISO week */
            count += 2;
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
            count += 17;
            break;
        case 'P':
        case 'p':                       /* Equivalent of either AM or PM
                                         * blank if not known */
            count += 2;
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
        case 'u':                       /* Day of week 1-7 */
            count += 1;
            break;
        case 'U':                       /* Week (Sunday first day)*/
        case 'W':                       /* Week */
        case 'V':                       /* ISO Week (Monday first day) */
            count += 2;
            break;
        case 'w':                       /* Week day */
            count += 1;
            break;
        case 'x':                       /* Date representation mm/dd/yy */
            count += 8;
            break;
        case 'y':                       /* Year, last 2 digits (99) */
            count += 2;
            break;
        case 'Y':                       /* Year, 4 digits (1999) */
            count += 4;
            break;
        case 'z':                       /* Numeric timezone, +hhmm */
            count += 5;
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
            sprintf(bp, "%3s", weekdayNames[datePtr->wday]);                
            bp += 3;
            break;
        case 'A':                       /* Weekday */
            numBytes = sprintf(bp, "%s", weekdayNames[datePtr->wday]); 
            bp += numBytes;
            break;
        case 'b':                       /* Abbreviated month (Jan) */
        case 'h':
            sprintf(bp, "%3s", monthNames[datePtr->mon]);                
            bp += 3;
            break;
        case 'B':                       /* Month */
            numBytes = sprintf(bp, "%s", monthNames[datePtr->mon]);                
            bp += numBytes;
            break;
        case 'c':                       /* Date and time (Thu Mar 3
                                         * 23:05:25 2005)". */
            numBytes = sprintf(bp, "%3s %3s %d %02d:%02d:%02d %4d",
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
            sprintf(bp, "%2d", datePtr->year % 100);
            bp += 2;
            break;
        case 'd':                       /* Day of month. */
            sprintf(bp, "%02d", datePtr->mday);
            bp += 2;
            break;
        case 'D':                       /* mm/dd/yy */
            sprintf(bp, "%02d/%02d/%02d", 
                    datePtr->mon, 
                    datePtr->mday,
                    datePtr->year % 100);
            bp += 8;
            break;
        case 'e':                       /* Day of month, space padded */
            sprintf(bp, "%2d", datePtr->mon);
            bp += 2;
            break;
        case 'F':                       /* Full date yyyy-mm-dd */
            sprintf(bp, "%4d-%2d-%2d", datePtr->year, datePtr->mon, 
                    datePtr->mday);
            bp += 10;
            break;
        case 'g':                       /* Last 2 digits of ISO wyear */
            sprintf(bp, "%2d", datePtr->wyear % 100);
            bp += 2;
            break;
        case 'G':                       /* ISO week */
            sprintf(bp, "%2d", datePtr->week);
            bp += 2;
            break;
        case 'H':                       /* Hour (0-23) */
            sprintf(bp, "%02d", datePtr->hour);
            bp += 2;
            break;
        case 'I':                       /* Hour (0-12) */
            if (datePtr->hour > 12) {
                sprintf(bp, "%2d", datePtr->hour - 12);
            } else {
                sprintf(bp, "%2d", datePtr->hour);
            }
            bp += 2;
            break;
        case 'j':                       /* Day of year */
            sprintf(bp, "%03d", datePtr->yday);
            bp += 3;
            break;
        case 'k':                       /* Hour, space padded */
            sprintf(bp, "%2d", datePtr->hour);
            bp += 2;
            break;
        case 'l':                       /* Hour, space padded */
            if (datePtr->hour > 12) {
                sprintf(bp, "%2d", datePtr->hour - 12);
            } else {
                sprintf(bp, "%2d", datePtr->hour);
            }
            bp += 2;
            break;
        case 'm':                       /* Month */
            sprintf(bp, "%02d", datePtr->mon + 1);
            bp += 2;
            break;
        case 'M':                       /* Minute */
            sprintf(bp, "%02d", datePtr->min);
            bp += 2;
            break;
        case 'N':                       /* nanoseconds (000000000..999999999) */
            Blt_DateToSeconds(datePtr, &seconds);
            numBytes = sprintf(bp, "%ld%ld", 
                               (long)seconds, 
                               (long)(datePtr->frac * 1e9));
            bp += numBytes;
            break;
        case 'P':
            if (datePtr->hour > 11) {
                strcpy(bp, "pm");                
            } else {
                strcpy(bp, "am");
            }
            bp += 2;
            break;
        case 'p':                       /* Equivalent of either AM or PM */
            if (datePtr->hour > 11) {
                strcpy(bp, "PM");                
            } else {
                strcpy(bp, "AM");
            }
            bp += 2;
            break;
        case 'r':                       /* 12 hour clock time (hh:mm:ss AM) */
            sprintf(bp, "%02d:%02d:%02d %2s",
                    datePtr->hour, datePtr->min, datePtr->sec,
                    (datePtr->hour > 11) ? "PM" : "AM");
            bp += 11;
            break;
        case 's':                       /* Seconds since epoch, may contain
                                         * fraction. */
            Blt_DateToSeconds(datePtr, &seconds);
            numBytes = sprintf(bp, "%.15g", seconds);
            bp += numBytes;
            break;
        case 'S':                       /* Second (ss) */
            sprintf(bp, "%02d", datePtr->sec);
            bp += 2;
            break;
        case 'T':                       /* The time as "hh:%mm:ss". */
            sprintf(bp, "%02d:%02d:%02d",
                    datePtr->hour, datePtr->min, datePtr->sec);
            bp += 8;
            break;
        case 'u':                       /* Day of week 1-7 */
            sprintf(bp, "%1d", datePtr->wday);
            bp += 1;
            break;
        case 'U':                       /* Week (Sunday first day)*/
            sprintf(bp, "%02d", datePtr->week);
            bp += 2;
            break;
        case 'V':                       /* ISO Week (Monday first day) */
            sprintf(bp, "%02d", datePtr->week);
            bp += 2;
            break;
        case 'w':                       /* Week day */
            sprintf(bp, "%02d", datePtr->wday);
            bp += 2;
            break;
        case 'x':                       /* Date representation mm/dd/yy */
            sprintf(bp, "%02d/%02d/%02d", 
                    datePtr->mon, datePtr->mday, datePtr->year % 100);
            bp += 8;
            break;
        case 'y':                       /* Year, last 2 digits (yy) */
            sprintf(bp, "%02d", datePtr->year % 100);
            bp += 2;
            break;
        case 'Y':                       /* Year, 4 digits (yyyy) */
            sprintf(bp, "%04d", datePtr->year);
            bp += 4;
            break;
        case 'z':                       /* Numeric timezone, +hhmm */
            sprintf(bp, "+%04ld", datePtr->tzoffset);
            bp += 5;
            break;
        }
        assert((bp - buffer) < count);
    }
    *bp = '\0';    
}

