/* -*- mode: c; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 * bltAfm.c --
 *
 * This module implements an Adobe Font Metric File (AFM) parser for the
 * purpose of determining font metrics when generating PostScript or PDF.
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

#define BUILD_BLT_TK_PROCS 1
#include "bltInt.h"
#ifdef HAVE_CTYPE_H
#  include <ctype.h>
#endif /* HAVE_CTYPE_H */
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <setjmp.h>
#include "bltAlloc.h"
#include "bltMath.h"
#include <bltHash.h>
#include <bltFont.h>
#include "bltText.h"
#include "bltFont.h"
#include "bltOp.h"
#include "bltInitCmd.h"
#include "bltAfm.h"

#define AFM_MAXSTATES	50
#define AFM_MAXLINE	512
#define FM(x)		Blt_Offset(Afm, x)
#define CM(x)		Blt_Offset(CharMetrics, x)
#define FTOA(x)		Blt_Dtoa(interp, (double)x)
    
typedef struct {
    const char *name;
    short int code;
} Symbol;

static Symbol isoLatin1Symbols[] = {
    { "A", 65 },
    { "AE", 198 },
    { "Aacute", 193 },
    { "Acircumflex", 194 },
    { "Adieresis", 196 },
    { "Agrave", 192 },
    { "Aring", 197 },
    { "Atilde", 195 },
    { "B", 66 },
    { "C", 67 },
    { "Ccedilla", 199 },
    { "D", 68 },
    { "E", 69 },
    { "Eacute", 201 },
    { "Ecircumflex", 202 },
    { "Edieresis", 203 },
    { "Egrave", 200 },
    { "Eth", 208 },
    { "F", 70 },
    { "G", 71 },
    { "H", 72 },
    { "I", 73 },
    { "Iacute", 205 },
    { "Icircumflex", 206 },
    { "Idieresis", 207 },
    { "Igrave", 204 },
    { "J", 74 },
    { "K", 75 },
    { "L", 76 },
    { "M", 77 },
    { "N", 78 },
    { "Ntilde", 209 },
    { "O", 79 },
    { "Oacute", 211 },
    { "Ocircumflex", 212 },
    { "Odieresis", 214 },
    { "Ograve", 210 },
    { "Oslash", 216 },
    { "Otilde", 213 },
    { "P", 80 },
    { "Q", 81 },
    { "R", 82 },
    { "S", 83 },
    { "T", 84 },
    { "Thorn", 222 },
    { "U", 85 },
    { "Uacute", 218 },
    { "Ucircumflex", 219 },
    { "Udieresis", 220 },
    { "Ugrave", 217 },
    { "V", 86 },
    { "W", 87 },
    { "X", 88 },
    { "Y", 89 },
    { "Yacute", 221 },
    { "Z", 90 },
    { "a", 97 },
    { "aacute", 225 },
    { "acircumflex", 226 },
    { "acute", 180 },
    { "adieresis", 228 },
    { "ae", 230 },
    { "agrave", 224 },
    { "ampersand", 38 },
    { "aring", 229 },
    { "asciicircum", 94 },
    { "asciitilde", 126 },
    { "asterisk", 42 },
    { "at", 64 },
    { "atilde", 227 },
    { "b", 98 },
    { "backslash", 92 },
    { "bar", 124 },
    { "braceleft", 123 },
    { "braceright", 125 },
    { "bracketleft", 91 },
    { "bracketright", 93 },
    { "brokenbar", 166 },
    { "florin", 166 },
    { "c", 99 },
    { "ccedilla", 231 },
    { "cedilla", 184 },
    { "cent", 162 },
    { "colon", 58 },
    { "comma", 44 },
    { "controlACK", 6 },
    { "controlBEL", 7 },
    { "controlBS", 8 },
    { "controlCAN", 24 },
    { "controlCR", 13 },
    { "controlDC1", 17 },
    { "controlDC2", 18 },
    { "controlDC3", 19 },
    { "controlDC4", 20 },
    { "controlDEL", 127 },
    { "controlDLE", 16 },
    { "controlEM", 25 },
    { "controlENQ", 5 },
    { "controlEOT", 4 },
    { "controlESC", 27 },
    { "controlETB", 23 },
    { "controlETX", 3 },
    { "controlFF", 12 },
    { "controlFS", 28 },
    { "controlGS", 29 },
    { "controlHT", 9 },
    { "controlLF", 10 },
    { "controlNAK", 21 },
    { "controlRS", 30 },
    { "controlSI", 15 },
    { "controlSO", 14 },
    { "controlSOT", 2 },
    { "controlSTX", 1 },
    { "controlSUB", 26 },
    { "controlSYN", 22 },
    { "controlUS", 31 },
    { "controlVT", 11 },
    { "copyright", 169 },
    { "currency", 164 },
    { "fraction", 164 },
    { "d", 100 },
    { "degree", 176 },
    { "dieresis", 168 },
    { "divide", 247 },
    { "dollar", 36 },
    { "e", 101 },
    { "eacute", 233 },
    { "ecircumflex", 234 },
    { "edieresis", 235 },
    { "egrave", 232 },
    { "eight", 56 },
    { "equal", 61 },
    { "eth", 240 },
    { "exclam", 33 },
    { "exclamdown", 161 },
    { "f", 102 },
    { "five", 53 },
    { "four", 52 },
    { "g", 103 },
    { "germandbls", 223 },
    { "grave", 96 },
    { "quoteleft", 96 },
    { "greater", 62 },
    { "guillemotleft", 171 },
    { "guillemotright", 187 },
    { "h", 104 },
    { "hyphen", 45 },
    { "i", 105 },
    { "iacute", 237 },
    { "icircumflex", 238 },
    { "idieresis", 239 },
    { "igrave", 236 },
    { "j", 106 },
    { "k", 107 },
    { "l", 108 },
    { "less", 60 },
    { "logicalnot", 172 },
    { "m", 109 },
    { "macron", 175 },
    { "middot", 183 },
    { "mu", 181 },
    { "mu1", 181 },
    { "multiply", 215 },
    { "n", 110 },
    { "nbspace", 160 },
    { "nine", 57 },
    { "nonbreakingspace", 160 },
    { "ntilde", 241 },
    { "numbersign", 35 },
    { "o", 111 },
    { "oacute", 243 },
    { "ocircumflex", 244 },
    { "odieresis", 246 },
    { "ograve", 242 },
    { "one", 49 },
    { "onehalf", 189 },
    { "onequarter", 188 },
    { "onesuperior", 185 },
    { "ordfeminine", 170 },
    { "ordmasculine", 186 },
    { "oslash", 248 },
    { "otilde", 245 },
    { "overscore", 175 },
    { "p", 112 },
    { "paragraph", 182 },
    { "parenleft", 40 },
    { "parenright", 41 },
    { "percent", 37 },
    { "period", 46 },
    { "periodcentered", 183 },
    { "plus", 43 },
    { "plusminus", 177 },
    { "q", 113 },
    { "question", 63 },
    { "questiondown", 191 },
    { "quotedbl", 34 },
    { "quotesingle", 39 },
    { "quoteright", 39 },
    { "r", 114 },
    { "registered", 174 },
    { "s", 115 },
    { "section", 167 },
    { "semicolon", 59 },
    { "seven", 55 },
    { "sfthyphen", 173 },
    { "six", 54 },
    { "slash", 47 },
    { "softhyphen", 173 },
    { "space", 32 },
    { "spacehackarabic", 32 },
    { "sterling", 163 },
    { "t", 116 },
    { "thorn", 254 },
    { "three", 51 },
    { "threequarters", 190 },
    { "threesuperior", 179 },
    { "two", 50 },
    { "twosuperior", 178 },
    { "u", 117 },
    { "uacute", 250 },
    { "ucircumflex", 251 },
    { "udieresis", 252 },
    { "ugrave", 249 },
    { "underscore", 95 },
    { "v", 118 },
    { "verticalbar", 124 },
    { "w", 119 },
    { "x", 120 },
    { "y", 121 },
    { "yacute", 253 },
    { "ydieresis", 255 },
    { "yen", 165 },
    { "z", 122 },
    { "zero", 48 },
    { NULL, -1 }
};

typedef struct _Parser Parser;

typedef int (ParseProc) (Parser *parserPtr, char *record, int offset);

typedef struct {
    const char *key;
    unsigned int numArgs;
    ParseProc *proc;
    size_t offset;
} ParserSpec;

typedef struct {
    float llx, lly, urx, ury;
} CharBBox;

typedef struct {
    float x, y;
} Point;

typedef struct {
    short int first, second;
} LigatureKey;

typedef struct {
    const char *successor;
    const char *ligature;
    short int index;
} Ligature;

typedef struct {
    CharBBox bbox;
    int index;
    const char *name;
    int hasLigature;
    int hasKernPair;
    Point w;
    Point vVector;
} CharMetrics;

typedef struct {
    float x, y;
    short int first, second;
} KernPairs;

typedef struct {
    short int first, second;
} KernPairsKey;

typedef struct {
    float degree;
    float minPointSize, maxPointSize;
    float minKern, maxKern;
} TrackKern;

typedef struct {
    const char *afmVersion;
    const char *familyName;
    const char *fontName;
    const char *fullName;
    const char *weight;
    const char *comment;
    const char *copyright;
    const char *notice;
    float italicAngle;
    CharBBox fontBBox;
    float underlinePosition;
    float underlineThickness;
    const char *characterSet;
    int characters;
    const char *version;
    const char *encodingScheme;
    float capHeight;
    float xHeight;
    float ascender, descender;
    int metricSets;
    Point charWidth;
    int escChar;
    int isFixedPitch;
    int isBaseFont;
    int isCIDFont;
    int isFixedV;
    int mappingScheme;
    int numCharMetrics;
    int numComposites;
    int numDirection;
    int numKernPairs;
    int numTrackKern;

    float stdHW, stdVW;
    Point vVector;
    TrackKern *trackKern;

    KernPairs *kernPairs;
    CharMetrics metrics[256];
    Blt_HashTable metricsTable;
    Blt_HashTable kernPairsTable;
    Blt_HashTable ligatureTable;
    Blt_HashTable symbolTable;
#ifdef notdef
    Composites *composites;
    Blt_HashTable compositeTable;
#endif
    int refCount;
    Blt_HashEntry *hashPtr;
    float pointSize;			/* Current point size of font. */
} Afm;

struct _Parser {
    Tcl_Channel channel;		/* Channel to AFM file. */
    Afm *afmPtr;
    jmp_buf jmpbuf; 
    Tcl_DString errors;			/* Contains error message. */
    int numErrors;
    int argc;				/* # arguments (word) of last
					 * line.  */
    const char **argv;			/* Split of last line. */
    Tcl_DString lastLine;               /* Contains last line read from
					 * file. */
    int lineNumber;
};

static Blt_HashTable fontTable;
static int initialized;

static int ParseLine(Parser *parserPtr, ParserSpec *specs, int numSpecs, 
		     ClientData clientData);

static Tcl_Interp *afmInterp = NULL;

static INLINE int 
Points(Afm *afmPtr, double x)
{
    return ROUND((afmPtr->pointSize * x)/1000.0);
}

/*ARGSUSED*/
static void
ParserError(Parser *parserPtr, ...)
{
    const char *fmt;
    char string[BUFSIZ+4];
    int length;
    va_list args;

    va_start(args, parserPtr);
    fmt = va_arg(args, char *);
    length = vsnprintf(string, BUFSIZ, fmt, args);
    if (length > BUFSIZ) {
	strcat(string, "...");
    }
    Tcl_DStringAppend(&parserPtr->errors, "line ", 5);
    Tcl_DStringAppend(&parserPtr->errors, Blt_Itoa(parserPtr->lineNumber), -1);
    Tcl_DStringAppend(&parserPtr->errors, ": ", 2);
    Tcl_DStringAppend(&parserPtr->errors, string, -1);
    Tcl_DStringAppend(&parserPtr->errors, "\n", -1);
    va_end(args);
    longjmp(parserPtr->jmpbuf, 0);
}

static int
GetNumber(Parser *parserPtr, const char *string, float *valuePtr) 
{
    char *end;
    double d;

    errno = 0;
    d = strtod(string, &end);		/* INTL: TCL source. */
    if (end == string) {
    badDouble:
	ParserError(parserPtr, "expected floating-point number but got \"%s\"",
		    string);
    }
    if (errno != 0 && (d == HUGE_VAL || d == -HUGE_VAL || d == 0)) {
	ParserError(parserPtr, "number \"%s\" is too big to represent",
		    string);
    }
    while ((*end != 0) && isspace(UCHAR(*end))) { /* INTL: ISO space. */
	end++;
    }
    if (*end != 0) {
	goto badDouble;
    }
    *valuePtr = (float)d;
    return TCL_OK;
}

static int
GetHexNumber(Parser *parserPtr, const char *string, int *valuePtr) 
{
    char *p;
    int value;

    if (*string == '<') {
	string++;
    }
    value = strtoul(string, &p, 8);
    if ((p == string) || (*p != '>')) {
	ParserError(parserPtr, "expected hex number but got \"%s\"", string);
    }
    *valuePtr = value;
    return TCL_OK;
}

static int 
GetLine(Parser *parserPtr)
{
    Tcl_DStringSetLength(&parserPtr->lastLine, 0);
    while (!Tcl_Eof(parserPtr->channel)) {
	const char *p;
	int numChars;

	numChars = Tcl_Gets(parserPtr->channel, &parserPtr->lastLine);
	if (numChars < 0) {
	    if (Tcl_Eof(parserPtr->channel)) {
		return TCL_RETURN;
	    }
	    ParserError(parserPtr, "error reading channel: %s\n", 
		strerror(errno));
	}
	parserPtr->lineNumber++;
	for (p = Tcl_DStringValue(&parserPtr->lastLine); isspace(*p); p++) {
	    /* skip blanks */
	}
	if (*p == '\0') {
	    continue;
	}
	return TCL_OK;
    }
    return TCL_RETURN;
}

static void 
SplitLine(Parser *parserPtr, const char *line)
{
    const char *p;
    int count;
    size_t strSize, addrSize;

    if (parserPtr->argv != NULL) {
	Blt_Free((char *)parserPtr->argv);
	parserPtr->argv = NULL;
	parserPtr->argc = 0;
    }
    /* Count the # of arguments to determine what size array to allocate. */
    count = 0;
    p = line;
    while (*p != '\0') {
	while (isspace(*p)) p++;	/* Skip whitespace. */
	if (*p == '\0') {
	    break;
	}
	while (!isspace(*p) && (*p != '\0')) p++;  /* Skip the word itself. */
	count++;
    }
    if (count == 0) {
	return;				/* No arguments. */
    }
    addrSize = sizeof(char **) * (count + 1);
    strSize = p - line + 1;
    {
	char *buffer, *p;
	const char **array;

	buffer = Blt_Malloc(addrSize + strSize);
	assert(buffer);
	p = buffer + addrSize;
	strcpy(p, line);		/* Copy the string into the buffer. */
	array = (const char **)buffer;
	count = 0;
	while (*p != '\0') {
	    while (isspace(*p)) {
		*p++ = '\0';	/* Convert whitespace to NULs. */
	    }
	    if (*p == '\0') {
		break;
	    }
	    array[count] = p;
	    while (!isspace(*p) && (*p != '\0')) p++;  
	    count++;
	}
	array[count] = NULL;
	parserPtr->argv = array;
	parserPtr->argc = count;
    }
}

static int 
SplitNextLine(Parser *parserPtr)
{
    int result;

    if (parserPtr->argv != NULL) {
	Blt_Free((char *)parserPtr->argv);
	parserPtr->argv = NULL;
	parserPtr->argc = 0;
    }
    result = GetLine(parserPtr);
    if (result == TCL_OK) {
	SplitLine(parserPtr, Tcl_DStringValue(&parserPtr->lastLine));
	return TCL_OK;
    }
    return result;
}

/*
 *---------------------------------------------------------------------------
 *
 * LookupKeyword --
 *
 *---------------------------------------------------------------------------
 */
static ParserSpec *
LookupKeyword(ParserSpec *specs, int numSpecs, const char *string)
{
    char c;
    int high, low;

    low = 0;
    high = numSpecs - 1;
    c = string[0];
    while (low <= high) {
	ParserSpec *specPtr;
	int compare;
	int median;
	
	median = (low + high) >> 1;
	specPtr = specs + median;

	/* Test the first character */
	compare = c - specPtr->key[0];
	if (compare == 0) {
	    /* Now test the entire string */
	    compare = strcmp(string, specPtr->key);
	}
	if (compare < 0) {
	    high = median - 1;
	} else if (compare > 0) {
	    low = median + 1;
	} else {
	    return specPtr;
	}
    }
    return NULL;			/* Can't find operation */
}

static int
ParseLine(Parser *parserPtr, ParserSpec *specs, int numSpecs, 
	  ClientData clientData)
{
    ParserSpec *specPtr;

    specPtr = LookupKeyword(specs, numSpecs, parserPtr->argv[0]);
    if (specPtr == NULL) {
	ParserError(parserPtr, "unknown keyword \"%s\"", parserPtr->argv[0]);
    }
    if ((specPtr->numArgs > 0) && (specPtr->numArgs != parserPtr->argc)) {
	ParserError(parserPtr, "wrong # arguments for \"%s\"", specPtr->key);
    }
    if (specPtr->proc == NULL) {
	return TCL_OK;
    }
    return (*specPtr->proc)(parserPtr, clientData, specPtr->offset);
}

static int 
ParseEndSection(Parser *parserPtr, char *record, int offset)
{
    return TCL_CONTINUE;		/* Indicates end of section. */
}

static long
LookupSymbol(Afm *afmPtr, const char *symbol)
{
    Blt_HashEntry *hPtr;

    hPtr = Blt_FindHashEntry(&afmPtr->symbolTable, symbol);
    if (hPtr != NULL) {
	return (long)Blt_GetHashValue(hPtr);
    }
    /*Blt_Warn("unknown symbol \"%s\"\n", symbol);*/
    return -1;
}

static void
InitSymbolTable(Afm *afmPtr)
{
    Symbol *symPtr;

    Blt_InitHashTable(&afmPtr->symbolTable, BLT_STRING_KEYS);
    for (symPtr = isoLatin1Symbols; symPtr->name != NULL; symPtr++) {
	Blt_HashEntry *hPtr;
	int isNew;
	
	hPtr = Blt_CreateHashEntry(&afmPtr->symbolTable, symPtr->name, &isNew);
	Blt_SetHashValue(hPtr, (ClientData)(long)symPtr->code);
    }
}

static void
UpdateSymbol(Afm *afmPtr, long code, const char *symbol)
{
    Blt_HashEntry *hPtr;
    int isNew;
    
    hPtr = Blt_CreateHashEntry(&afmPtr->symbolTable, symbol, &isNew);
#ifdef notdef
    if (!isNew) {
	long oldCode;

	oldCode = (long)Blt_GetHashValue(hPtr);
	if (code != oldCode) {
	    fprintf(stderr, "rewriting symbol %s with %d (was %ld)\n",
		    symbol, code, oldCode);
	}
    }
#endif
    Blt_SetHashValue(hPtr, (ClientData)code);
}

static void
BuildKernPairsTable(Afm *afmPtr)
{
    KernPairs *kp, *kend;

    Blt_InitHashTable(&afmPtr->kernPairsTable, 
		      sizeof(KernPairsKey) / sizeof(int));
    for (kp = afmPtr->kernPairs, kend = kp + afmPtr->numKernPairs; kp < kend; 
	 kp++) {
	KernPairsKey key;
	Blt_HashEntry *hPtr;
	int isNew;
	
	memset(&key, 0, sizeof(key));
	key.first = kp->first;
	key.second = kp->second;
	hPtr = Blt_CreateHashEntry(&afmPtr->kernPairsTable, (char *)&key, 
		&isNew);
	Blt_SetHashValue(hPtr, (ClientData)kp);
    }
}

static KernPairs *
GetKernPairs(Afm *afmPtr, int c1, int c2)
{
    KernPairsKey key;
    Blt_HashEntry *hPtr;

    key.first = c1;
    key.second = c2;
    
    hPtr = Blt_FindHashEntry(&afmPtr->kernPairsTable, (char *)&key);
    if (hPtr == NULL) {
	return NULL;
    }
    return Blt_GetHashValue(hPtr);
}

#ifdef notdef
static KernPairs *
GetLigature(Afm *afmPtr, int c1, int c2)
{
    KernPairsKey key;
    Blt_HashEntry *hPtr;

    key.first = c1;
    key.second = c2;
    
    hPtr = Blt_FindHashEntry(&afmPtr->kernPairsTable, (char *)&key);
    if (hPtr == NULL) {
	return NULL;
    }
    return Blt_GetHashValue(hPtr);
}


static Ligature *
NewLigature(Parser *parserPtr, int first, int second)
{
    Blt_HashEntry *hPtr;
    Ligature *ligPtr;
    int isNew;
    LigatureKey key;

    key.first = first;
    key.second = second;
    ligPtr = Blt_Calloc(1, sizeof(Ligature));
    assert(ligPtr);
    hPtr = Blt_CreateHashEntry(&parserPtr->afmPtr->ligatureTable, 
	(char *)&key, &isNew);
    Tcl_SetHashValue(hPtr, ligPtr);
    return ligPtr;
}
#endif

static Parser *
NewParser(Afm *afmPtr, const char *fileName)
{
    Parser *parserPtr;
    Tcl_Channel channel;

    channel = Tcl_OpenFileChannel(NULL, fileName, "r", 0);
    if (channel == NULL) {
	Blt_Warn("can't open %s\n", fileName);
	return NULL;
    }
    if ((Tcl_SetChannelOption(NULL, channel, "-translation","auto")!=TCL_OK)||
	(Tcl_SetChannelOption(NULL, channel, "-eofchar", "\x1a") != TCL_OK)) {
	return NULL;
    }
    parserPtr = Blt_Calloc(1, sizeof(Parser));
    assert(parserPtr);
    parserPtr->channel = channel;
    parserPtr->afmPtr = afmPtr;
    InitSymbolTable(afmPtr);
    Tcl_DStringInit(&parserPtr->errors);
    Tcl_DStringAppend(&parserPtr->errors, "error reading \"", -1);
    Tcl_DStringAppend(&parserPtr->errors, fileName, -1);
    Tcl_DStringAppend(&parserPtr->errors, "\": ", -1);
    Tcl_DStringInit(&parserPtr->lastLine);
    return parserPtr;
}

static void
DestroyParser(Parser *parserPtr)
{
    if (parserPtr->argv != NULL) {
	Blt_Free((char *)parserPtr->argv);
    }
    Tcl_Close(NULL, parserPtr->channel);
    Tcl_DStringFree(&parserPtr->errors);
    Tcl_DStringFree(&parserPtr->lastLine);
    Blt_Free(parserPtr);
}

static void
AfmDestroy(Afm *afmPtr)
{
    if (afmPtr->afmVersion != NULL) {
	Blt_Free((char *)afmPtr->afmVersion);
    }
    if (afmPtr->characterSet != NULL) {
	Blt_Free((char *)afmPtr->characterSet);
    }
    if (afmPtr->comment != NULL) {
	Blt_Free((char *)afmPtr->comment);
    }
    if (afmPtr->copyright != NULL) {
	Blt_Free((char *)afmPtr->copyright);
    }
    if (afmPtr->encodingScheme != NULL) {
	Blt_Free((char *)afmPtr->encodingScheme);
    }
    if (afmPtr->familyName != NULL) {
	Blt_Free((char *)afmPtr->familyName);
    }
    if (afmPtr->fontName != NULL) {
	Blt_Free((char *)afmPtr->fontName);
    }
    if (afmPtr->fullName != NULL) {
	Blt_Free((char *)afmPtr->fullName);
    }
    if (afmPtr->notice != NULL) {
	Blt_Free((char *)afmPtr->notice);
    }
    if (afmPtr->version != NULL) {
	Blt_Free((char *)afmPtr->version);
    }
    if (afmPtr->weight != NULL) {
	Blt_Free((char *)afmPtr->weight);
    }
    if (afmPtr->hashPtr != NULL) {
	Blt_DeleteHashEntry(&fontTable, afmPtr->hashPtr);
    }
    Blt_DeleteHashTable(&afmPtr->kernPairsTable);
    Blt_DeleteHashTable(&afmPtr->metricsTable);
    Blt_DeleteHashTable(&afmPtr->symbolTable);
    Blt_DeleteHashTable(&afmPtr->ligatureTable);
    if (afmPtr->kernPairs != NULL) {
	Blt_Free(afmPtr->kernPairs);
    }
    if (afmPtr->trackKern != NULL) {
	Blt_Free(afmPtr->trackKern);
    }
    Blt_Free((char *)afmPtr);
}

static int
ParseInt(Parser *parserPtr, char *record, int offset)
{
    int *valuePtr = (int *)(record + offset);

    if (Tcl_GetInt(NULL, parserPtr->argv[1], valuePtr) != TCL_OK) {
	ParserError(parserPtr, "can't convert \"%s\" to integer.", 
		    parserPtr->argv[1]);
    }
    return TCL_OK;
}

static int
ParseHex(Parser *parserPtr, char *record, int offset)
{
    int *valuePtr = (int *)(record + offset);

    return GetHexNumber(parserPtr, parserPtr->argv[1], valuePtr);
}

static int
ParseBoolean(Parser *parserPtr, char *record, int offset)
{
    int *valuePtr = (int *)(record + offset);

    if (Tcl_GetBoolean(NULL, parserPtr->argv[1], valuePtr) != TCL_OK) {
	ParserError(parserPtr, "can't convert \"%s\" to boolean.", 
		    parserPtr->argv[1]);
    }
    return TCL_OK;
}


static int
ParseNumber(Parser *parserPtr, char *record, int offset)
{
    float *valuePtr = (float *)(record + offset);

    return GetNumber(parserPtr, parserPtr->argv[1], valuePtr);
}


static int
ParsePoint(Parser *parserPtr, char *record, int offset)
{
    Point *pointPtr = (Point *)(record + offset);

    if ((GetNumber(parserPtr, parserPtr->argv[1], &pointPtr->x) != TCL_OK) ||
	(GetNumber(parserPtr, parserPtr->argv[2], &pointPtr->y) != TCL_OK)) {
	return TCL_ERROR;
    }
    return TCL_OK;
}

static int
ParseBBox(Parser *parserPtr, char *record, int offset)
{
    CharBBox *bboxPtr = (CharBBox *)(record + offset);
    
    if ((GetNumber(parserPtr, parserPtr->argv[1], &bboxPtr->llx) != TCL_OK) ||
	(GetNumber(parserPtr, parserPtr->argv[2], &bboxPtr->lly) != TCL_OK) || 
	(GetNumber(parserPtr, parserPtr->argv[3], &bboxPtr->urx) != TCL_OK) || 
	(GetNumber(parserPtr, parserPtr->argv[4], &bboxPtr->ury) != TCL_OK)) {
	return TCL_ERROR;
    }
    return TCL_OK;
}

static int
ParseString(Parser *parserPtr, char *record, int offset)
{
    char **args = (char **)(record + offset);

    if (*args != NULL) {
	Blt_Free(*args);
	*args = NULL;
    }
    *args = Tcl_Merge(parserPtr->argc - 1 , parserPtr->argv + 1);
    if (*args == NULL) {
	ParserError(parserPtr, "can't merge \"%s\" string.", 
		    parserPtr->argv[0]);
    }
    return TCL_OK;
}

static int
ParseName(Parser *parserPtr, char *record, int offset)
{
    const char **valuePtr = (const char **)(record + offset);

    if (*valuePtr != NULL) {
	Blt_Free((char *)*valuePtr);
    }
    *valuePtr = Blt_Strdup(parserPtr->argv[1]);
    return TCL_OK;
}


static int
ParseStartComposites(Parser *parserPtr, char *record, int offset)
{
    int *valuePtr = (int *)(record + offset);
    int n;

    assert(*valuePtr == 0);
    if (Tcl_GetInt(NULL, parserPtr->argv[1], &n) != TCL_OK) {
	ParserError(parserPtr, "can't convert \"%s\" to integer", 
		    parserPtr->argv[1]);
    }
    n++;
    *valuePtr = n;
    for (;;) {
	if (SplitNextLine(parserPtr) == TCL_RETURN) {
	    ParserError(parserPtr, "unexpected EOF in StartComposites");
	}
	if (strcmp(parserPtr->argv[0], "EndComposites") == 0) {
	    return TCL_OK;
	}
    }
    /*notreached*/
    return TCL_ERROR;
}


static ParserSpec directionSpecs[] = {
    { "CharWidth",	   3, ParsePoint,      FM(charWidth)	      },
    { "EndDirection",	   1, ParseEndSection, 0		      },
    { "IsFixedPitch",	   2, ParseBoolean,    FM(isFixedPitch)       },
    { "ItalicAngle",	   2, ParseNumber,     FM(italicAngle)	      },
    { "UnderlinePosition", 2, ParseNumber,     FM(underlinePosition)  },
    { "UnderlineThickness",2, ParseNumber,     FM(underlineThickness) }
};
static int numDirectionSpecs = sizeof(directionSpecs) / sizeof(ParserSpec);


static int
ParseStartDirection(Parser *parserPtr, char *record, int offset)
{
    Afm *afmPtr = (Afm *)record;
    int *valuePtr = (int *)(record + offset);
    int n;
    int result;

    assert(*valuePtr == 0);
    if (Tcl_GetInt(NULL, parserPtr->argv[1], &n) != TCL_OK) {
	ParserError(parserPtr, "can't convert \"%s\" to integer.", 
		    parserPtr->argv[1]);
    }
    do {
	if (SplitNextLine(parserPtr) == TCL_RETURN) {
	    ParserError(parserPtr, "unexpected EOF in StartDirection");
	}
	result = ParseLine(parserPtr, directionSpecs, numDirectionSpecs, afmPtr);
    } while (result == TCL_OK);
    if (result == TCL_CONTINUE) {
	return TCL_OK;			/* Found EndKernPairs */
    }
    return TCL_ERROR;
}

static int 
ParseTrackKern(Parser *parserPtr, char *record, int offset)
{
    TrackKern *tp = (TrackKern *)(record + offset);

    if ((GetNumber(parserPtr, parserPtr->argv[1], &tp->degree) != TCL_OK) ||
	(GetNumber(parserPtr, parserPtr->argv[2], &tp->minPointSize)!=TCL_OK) ||
	(GetNumber(parserPtr, parserPtr->argv[3], &tp->minKern) != TCL_OK) ||
	(GetNumber(parserPtr, parserPtr->argv[4], &tp->maxPointSize)!=TCL_OK) ||
	(GetNumber(parserPtr, parserPtr->argv[5], &tp->maxKern) != TCL_OK)) {
	return TCL_ERROR;
    }
    return TCL_OK;
}

static ParserSpec trackKernSpecs[] = {
    { "EndTrackKern",	 1, ParseEndSection, 0 },
    { "TrackKern",	 6, ParseTrackKern,  0 },
};
static int numTrackKernSpecs = sizeof(trackKernSpecs) / sizeof(ParserSpec);

static int
ParseStartTrackKern(Parser *parserPtr, char *record, int offset)
{
    Afm *afmPtr = (Afm *)record;
    TrackKern *tp;
    int *valuePtr = (int *)(record + offset);
    int n;
    int result;

    assert(*valuePtr == 0);
    if (Tcl_GetInt(NULL, parserPtr->argv[1], &n) != TCL_OK) {
	ParserError(parserPtr, "can't convert \"%s\" to integer.", 
		parserPtr->argv[1]);
    }
    n++;
    *valuePtr = n;
    afmPtr->trackKern = Blt_Calloc(n, sizeof(TrackKern));
    assert(afmPtr->trackKern);
    tp = afmPtr->trackKern;
    do {
	if (SplitNextLine(parserPtr) == TCL_RETURN) {
	    ParserError(parserPtr, "unexpected EOF in StartTrackKern");
	}
	result = ParseLine(parserPtr, trackKernSpecs, numTrackKernSpecs, tp);
	tp++;
    } while (result == TCL_OK);
    if (result == TCL_CONTINUE) {
	assert((tp - afmPtr->trackKern) == n);
	return TCL_OK;			/* Found EndTrackKern */
    }
    return TCL_ERROR;
}


static int 
ParseKP(Parser *parserPtr, char *record, int offset)
{
    KernPairs *pairPtr = (KernPairs *)(record + offset);

    pairPtr->first = LookupSymbol(parserPtr->afmPtr, parserPtr->argv[1]);
    pairPtr->second = LookupSymbol(parserPtr->afmPtr, parserPtr->argv[2]);
    if ((GetNumber(parserPtr, parserPtr->argv[3], &pairPtr->x) != TCL_OK) ||
	(GetNumber(parserPtr, parserPtr->argv[4], &pairPtr->y) != TCL_OK)) {
	return TCL_ERROR;
    }
    return TCL_OK;
}

static int 
ParseKPH(Parser *parserPtr, char *record, int offset)
{
    KernPairs *pairPtr = (KernPairs *)(record + offset);
    int x, y;

    pairPtr->first = LookupSymbol(parserPtr->afmPtr, parserPtr->argv[1]);
    pairPtr->second = LookupSymbol(parserPtr->afmPtr, parserPtr->argv[2]);
    if ((GetHexNumber(parserPtr, parserPtr->argv[3], &x) != TCL_OK) ||
	(GetHexNumber(parserPtr, parserPtr->argv[4], &y) != TCL_OK)) {
	return TCL_ERROR;
    }
    pairPtr->x = (float)x;
    pairPtr->y = (float)y;
    return TCL_OK;
}

static int 
ParseKPX(Parser *parserPtr, char *record, int offset)
{
    KernPairs *pairPtr = (KernPairs *)(record + offset);

    pairPtr->first = LookupSymbol(parserPtr->afmPtr, parserPtr->argv[1]);
    pairPtr->second = LookupSymbol(parserPtr->afmPtr, parserPtr->argv[2]);
    if (GetNumber(parserPtr, parserPtr->argv[3], &pairPtr->x) != TCL_OK) {
	return TCL_ERROR;
    }
    pairPtr->y = 0;
    return TCL_OK;
}

static int 
ParseKPY(Parser *parserPtr, char *record, int offset)
{
    KernPairs *pairPtr = (KernPairs *)(record + offset);

    pairPtr->first = LookupSymbol(parserPtr->afmPtr, parserPtr->argv[1]);
    pairPtr->second = LookupSymbol(parserPtr->afmPtr, parserPtr->argv[2]);
    if (GetNumber(parserPtr, parserPtr->argv[3], &pairPtr->y) != TCL_OK) {
	return TCL_ERROR;
    }
    pairPtr->x = 0;
    return TCL_OK;
}

static ParserSpec kernPairsSpecs[] = {
    { "EndKernPairs",	1, ParseEndSection, 0 },
    { "KP",		5, ParseKP,	    0 },
    { "KPH",		5, ParseKPH,	    0 },
    { "KPX",		4, ParseKPX,	    0 },
    { "KPY",		4, ParseKPY,	    0 },
};
static int numKernPairsSpecs = sizeof(kernPairsSpecs) / sizeof(ParserSpec);

static int
ParseStartKernPairs(Parser *parserPtr, char *record, int offset)
{
    Afm *afmPtr = (Afm *)record;
    int *valuePtr = (int *)(record + offset);
    int n;
    int result;
    KernPairs *kp;

    assert(*valuePtr == 0);
    if (Tcl_GetInt(NULL, parserPtr->argv[1], &n) != TCL_OK) {
	ParserError(parserPtr, "can't convert \"%s\" to integer.", 
		    parserPtr->argv[1]);
    }
    n++;
    *valuePtr = n;
    afmPtr->kernPairs = Blt_Calloc(n, sizeof(KernPairs));
    assert(afmPtr->kernPairs);
    kp = afmPtr->kernPairs;
    do {
	if (SplitNextLine(parserPtr) == TCL_RETURN) {
	    ParserError(parserPtr, "unexpected EOF in StartKernPairs");
	}
	result = ParseLine(parserPtr, kernPairsSpecs, numKernPairsSpecs, kp);
	kp++;
    } while (result == TCL_OK);
    if (result == TCL_CONTINUE) {
	assert((kp - afmPtr->kernPairs) == *valuePtr);
	return TCL_OK;			/* Found EndKernPairs */
    }
    return TCL_ERROR;
}


static ParserSpec kernDataSpecs[] = {
    { "EndKernData",	 1, ParseEndSection,      0		  },
    { "StartKernPairs",	 2, ParseStartKernPairs,  FM(numKernPairs)  },
    { "StartKernPairs0", 2, ParseStartKernPairs,  FM(numKernPairs)  },
    { "StartKernPairs1", 2, ParseStartKernPairs,  FM(numKernPairs)  },
    { "StartTrackKern",	 2, ParseStartTrackKern,  FM(numTrackKern)  },
};
static int numKernDataSpecs = sizeof(kernDataSpecs) / sizeof(ParserSpec);

static int
ParseStartKernData(Parser *parserPtr, char *record, int offset)
{
    Afm *afmPtr = (Afm *)record;
    int result;

    do {
	if (SplitNextLine(parserPtr) == TCL_RETURN) {
	    ParserError(parserPtr, "unexpected EOF in StartKernPairs");
	}
	result = ParseLine(parserPtr, kernDataSpecs, numKernDataSpecs, afmPtr);
    } while (result == TCL_OK);
    if (result == TCL_CONTINUE) {
	return TCL_OK;			/* Found EndKernData */
    }
    return TCL_ERROR;
}


static int
ParseLigature(Parser *parserPtr, char *record, int offset)
{
#ifdef notdef
    CharMetrics *cmPtr = (CharMetrics *)record;
    int successor;

    cmPtr->ligature = TRUE;
    successor = LookupSymbol(parserPtr->afmPtr, parserPtr->argv[1]);
    ligPtr = NewLigature(cmPtr->index, successor);
    ligPtr->name = Blt_Strdup(parserPtr->argv[2]);
#endif
    return TCL_OK;
}

static ParserSpec charMetricsSpecs[] = {
    { "B",		5, ParseBBox,	    CM(bbox)	    },
    { "C",		2, ParseInt,	    CM(index)	    },
    { "CH",		2, ParseHex,	    CM(index)	    },
    { "EndCharMetrics",	1, ParseEndSection, 0		    },
    { "L",		3, ParseLigature,   CM(hasLigature) },
    { "N",		2, ParseName,	    CM(name)	    },
    { "VV",		3, ParsePoint,      CM(vVector)     },
    { "W",		3, ParsePoint,      CM(w)           },
    { "W0",		3, ParsePoint,      CM(w)           },
    { "W1",		3, ParsePoint,      CM(w)           },
    { "W0X",		2, ParseNumber,     CM(w.x)	    },
    { "W0Y",		2, ParseNumber,     CM(w.y)	    },
    { "W1X",		2, ParseNumber,     CM(w.x)	    },
    { "W1Y",		2, ParseNumber,     CM(w.y)	    },
    { "WX",		2, ParseNumber,     CM(w.x)	    },
    { "WY",		2, ParseNumber,     CM(w.y)	    }
};
static int numCharMetricsSpecs = sizeof(charMetricsSpecs) / sizeof(ParserSpec);

static int
ParseStartCharMetrics(Parser *parserPtr, char *record, int offset)
{
    Afm *afmPtr = (Afm *)record;
    int *valuePtr = (int *)(record + offset);
    int count;
    int i;

    assert(*valuePtr == 0);
    if (Tcl_GetInt(NULL, parserPtr->argv[1], &i) != TCL_OK) {
	ParserError(parserPtr, "can't convert \"%s\" to integer.", 
		    parserPtr->argv[1]);
    }
    i++;
    *valuePtr = i;
    for(i = 0; i < 256; i++) {
	afmPtr->metrics[i].index = -1;
    }
    count = 0;
    for (;;) {
	int result;
	CharMetrics cm;
	const char *p;

	result = GetLine(parserPtr);
	if (result == TCL_RETURN) {
	    ParserError(parserPtr, "unexpected EOF in StartCharMetrics");
	}
	memset(&cm, 0, sizeof(CharMetrics));
	for(p = strtok(Tcl_DStringValue(&parserPtr->lastLine), ";"); p != NULL; 
	    p = strtok(NULL, ";")) {
	    SplitLine(parserPtr, p);
	    if (parserPtr->argc == 0) {
		continue;
	    }
	    result = ParseLine(parserPtr, charMetricsSpecs, numCharMetricsSpecs, 
			       &cm);
	    if (result != TCL_OK) {
		break;
	    }
	}
	count++;
	if (cm.index != -1) {
	    if (cm.name != NULL) {
		UpdateSymbol(parserPtr->afmPtr, cm.index, cm.name);
	    }
	    afmPtr->metrics[cm.index] = cm;
	}
	if (result == TCL_ERROR) {
	    return TCL_ERROR;
	}
	if (result == TCL_CONTINUE) {
	    assert(count == *valuePtr);
	    return TCL_OK;		/* Found EndCharMetrics */
	}
    }
    return TCL_ERROR;
}

static ParserSpec fontMetricsSpecs[] = {
    { "Ascender",	    2, ParseNumber,	      FM(ascender)  	     }, 
    { "CapHeight",	    2, ParseNumber,	      FM(capHeight)	     },
    { "CharWidth",	    3, ParsePoint,	      FM(charWidth)	     },
    { "CharacterSet",	    0, ParseString,	      FM(characterSet)       },
    { "Characters",	    2, ParseInt,	      FM(characters)	     },
    { "Comment",	    0, NULL,		      FM(comment)	     },
    { "Copyright",	    0, NULL,		      FM(copyright)	     },
    { "Descender",	    2, ParseNumber,	      FM(descender)	     },
    { "EncodingScheme",	    0, ParseString,	      FM(encodingScheme)     },
    { "EndFontMetrics",	    1, ParseEndSection,	      0		             },
    { "EscChar",	    2, ParseInt,	      FM(escChar)	     },
    { "FamilyName",	    0, ParseString,	      FM(familyName)	     },
    { "FontBBox",	    5, ParseBBox,	      FM(fontBBox)	     },
    { "FontName",	    0, ParseString,	      FM(fontName)	     },
    { "FullName",	    0, ParseString,	      FM(fullName)	     },
    { "IsBaseFont",	    2, ParseBoolean,          FM(isBaseFont)	     },
    { "IsCIDFont",	    2, ParseBoolean,          FM(isCIDFont)	     },
    { "IsFixedPitch",	    2, ParseBoolean,          FM(isFixedPitch)       },
    { "IsFixedV",	    2, ParseBoolean,          FM(isFixedV)	     },
    { "ItalicAngle",	    2, ParseNumber,	      FM(italicAngle)        },
    { "MappingScheme",	    2, ParseInt,	      FM(mappingScheme)      },
    { "MetricSets",	    2, ParseInt,	      FM(metricSets)         },
    { "Notice",		    0, NULL,		      FM(notice)	     },
    { "StartCharMetrics",   2, ParseStartCharMetrics, FM(numCharMetrics)       },
    { "StartComposites",    2, ParseStartComposites,  FM(numComposites)        },
    { "StartDirection",	    2, ParseStartDirection,   FM(numDirection)         },
    { "StartKernData",	    1, ParseStartKernData,    0                      },
    { "StdHW",		    2, ParseNumber,           FM(stdHW)              },
    { "StdVW",		    2, ParseNumber,           FM(stdVW)              },
    { "UnderlinePosition",  2, ParseNumber,           FM(underlinePosition)  },
    { "UnderlineThickness", 2, ParseNumber,           FM(underlineThickness) },
    { "VVector",	    2, ParsePoint,            FM(vVector)            },
    { "Version",	    0, ParseString,           FM(version)            },
    { "Weight",		    0, ParseString,           FM(weight)             },
    { "XHeight",	    2, ParseNumber,           FM(xHeight)            }
};
static int numFontMetricsSpecs = sizeof(fontMetricsSpecs) / sizeof(ParserSpec);

static int 
ParseStartFontMetrics(Parser *parserPtr, char *record, int offset)
{
    Afm *afmPtr = (Afm *)record;
    const char **versionPtr = (const char **)(record + offset);
    int result;

    assert(*versionPtr == NULL);
    *versionPtr = Blt_Strdup(parserPtr->argv[1]);
    do {
	if (SplitNextLine(parserPtr) == TCL_RETURN) {
	    ParserError(parserPtr, "unexpected EOF in StartFontMetrics");
	}
	result = ParseLine(parserPtr, fontMetricsSpecs, numFontMetricsSpecs,
			   afmPtr);
    } while (result == TCL_OK);
    if (result == TCL_CONTINUE) {
	return TCL_OK;			/* Found EndFontMetrics */
    }
    return TCL_ERROR;
}

static ParserSpec afmSpecs[] = {
    { "StartFontMetrics", 2, ParseStartFontMetrics, FM(afmVersion)  },
};
static int numAfmSpecs = sizeof(afmSpecs) / sizeof(ParserSpec);

static Afm *
AfmParseFile(Tcl_Interp *interp, const char *fileName)
{
    Afm *afmPtr;
    Parser *parserPtr;
    int result;

    afmPtr = Blt_Calloc(1, sizeof(Afm));
    assert(afmPtr);
    parserPtr = NewParser(afmPtr, fileName);
    if (parserPtr == NULL) {
	Blt_Free(afmPtr);
	return NULL;
    }
    /* Set up jump for errors. */
    if (setjmp(parserPtr->jmpbuf)) {
	Blt_Warn("%s\n", Tcl_DStringValue(&parserPtr->errors));
	DestroyParser(parserPtr);
	AfmDestroy(afmPtr);
	return NULL;
    }
    for (;;) {
	result = SplitNextLine(parserPtr);
	if (result == TCL_RETURN) {
	    break;
	}
	result = ParseLine(parserPtr, afmSpecs, numAfmSpecs, afmPtr);
    }
    DestroyParser(parserPtr);
    if (result != TCL_RETURN) {
	AfmDestroy(afmPtr);
	return NULL;
    }
    BuildKernPairsTable(afmPtr);
    return afmPtr;
}

typedef struct {
    const char *alias;
    const char *fontName;
} FontMap;

static FontMap fontMap[] =
{
    { "Arial",		        "Helvetica"	   },
    { "AvantGarde",             "AvantGarde"       },
    { "Bookman",                "Bookman"          },
    { "Courier New",            "Courier"          },
    { "Courier",                "Courier"          },
    { "Geneva",                 "Helvetica"        },
    { "Helvetica",              "Helvetica"        },
    { "Mathematica1",		"Helvetica"	   },
    { "Monaco",                 "Courier"          },
    { "New Century Schoolbook", "NewCenturySchlbk" },
    { "New York",               "Times"            },
    { "Nimbus Roman No9 L"	"Times"		   },
    { "Nimbus Sans L Condensed","Helvetica"        },
    { "Nimbus Sans L",		"Helvetica"        },
    { "Palatino",               "Palatino"         },
    { "Standard Symbols L",	"Symbol"           },
    { "Symbol",                 "Symbol"           },
    { "Times New Roman",        "Times"            },
    { "Times Roman",            "Times"            },
    { "Times",                  "Times"            },
    { "ZapfChancery",           "ZapfChancery"     },
    { "ZapfDingbats",           "ZapfDingbats"     }
};
static int numFonts = sizeof(fontMap) / sizeof(FontMap);
/*
 *---------------------------------------------------------------------------
 *
 * LookupFontName --
 *
 *---------------------------------------------------------------------------
 */
static const char *
LookupFontName(const char *string)
{
    char c;
    int high, low;

    low = 0;
    high = numFonts - 1;
    c = string[0];
    while (low <= high) {
	FontMap *mapPtr;
	int compare;
	int median;
	
	median = (low + high) >> 1;
	mapPtr = fontMap + median;

	/* Test the first character */
	compare = c - mapPtr->alias[0];
	if (compare == 0) {
	    /* Now test the entire string */
	    compare = strcmp(string, mapPtr->alias);
	}
	if (compare < 0) {
	    high = median - 1;
	} else if (compare > 0) {
	    low = median + 1;
	} else {
	    return mapPtr->fontName;
	}
    }
    return "Helvetica";			/* Can't find font. */
}

static Afm *
AfmGetMetrics(Tcl_Interp *interp, const char *psFontName) 
{
    Afm *afmPtr;
    Blt_HashEntry *hPtr;
    int isNew;

    if (!initialized) {
	Blt_InitHashTable(&fontTable, BLT_STRING_KEYS);
	initialized = TRUE;
    }
#ifdef notdef
    fprintf(stderr, "Lookup for %s\n", psFontName);
#endif
    psFontName = LookupFontName(psFontName);
#ifdef notdef
    fprintf(stderr, "returning %s\n", psFontName);
#endif
    hPtr = Blt_CreateHashEntry(&fontTable, psFontName, &isNew);
    if (isNew) {
	const char *path;
	Tcl_DString ds;

	path = Tcl_GetVar(interp, "blt_library", TCL_GLOBAL_ONLY);
	if (path == NULL) {
	    Tcl_AppendResult(interp, "can't find \"blt_library\" variable", 
			     (char *)NULL);
	    Blt_DeleteHashEntry(&fontTable, hPtr);
	    return NULL;
	}
	Tcl_DStringInit(&ds);
	Tcl_DStringAppend(&ds, path, -1);
	Tcl_DStringAppend(&ds, "/afm/", 5);
	Tcl_DStringAppend(&ds, psFontName, -1);
	Tcl_DStringAppend(&ds, ".afm", 4);
	afmPtr = AfmParseFile(interp, Tcl_DStringValue(&ds));
	Tcl_DStringFree(&ds);
	if (afmPtr == NULL) {
	    Blt_DeleteHashEntry(&fontTable, hPtr);
	    return NULL;
	}
	Blt_SetHashValue(hPtr, afmPtr);
	afmPtr->hashPtr = hPtr;
    } else {
	afmPtr = Blt_GetHashValue(hPtr);
    }
    return afmPtr;
}

static Afm *
AfmGetMetricsFromFont(Blt_Font font) 
{
    Afm *afmPtr;
    Tcl_DString ds;
    double pointSize;
    Tcl_Interp *interp;

    Tcl_DStringInit(&ds);
    pointSize = Blt_Font_PostscriptName(font, &ds);
    interp = Blt_Font_GetInterp(font);
    afmPtr = AfmGetMetrics(interp, Tcl_DStringValue(&ds));
    Tcl_DStringFree(&ds);
    if (afmPtr != NULL) {
	afmPtr->pointSize = pointSize;
	return afmPtr;
    }
    return NULL;
}    


static int
LoadOp(ClientData clientData, Tcl_Interp *interp, int objc, 
       Tcl_Obj *const *objv)
{
    Afm *afmPtr;

    afmPtr = AfmParseFile(interp, Tcl_GetString(objv[2]));
    if (afmPtr == NULL) {
	return TCL_ERROR;
    }
    return TCL_OK;
}

static int
DumpOp(ClientData clientData, Tcl_Interp *interp, int objc, 
       Tcl_Obj *const *objv)
{
    Afm *afmPtr;
    const char *fileName;
    int i;

    fileName = Tcl_GetString(objv[2]);
    afmPtr = AfmParseFile(interp, fileName);
    if (afmPtr == NULL) {
	return TCL_ERROR;
    }
    if (afmPtr->familyName != NULL) {
	Tcl_AppendElement(interp, "familyName");
	Tcl_AppendElement(interp, afmPtr->familyName);
    }
    if (afmPtr->fontName != NULL) {
	Tcl_AppendElement(interp, "fontName");
	Tcl_AppendElement(interp, afmPtr->fontName);
    }
    if (afmPtr->fullName != NULL) {
	Tcl_AppendElement(interp, "fullName");
	Tcl_AppendElement(interp, afmPtr->fullName);
    }
    if (afmPtr->version != NULL) {
	Tcl_AppendElement(interp, "version");
	Tcl_AppendElement(interp, afmPtr->version);
    }
    if (afmPtr->weight != NULL) {
	Tcl_AppendElement(interp, "weight");
	Tcl_AppendElement(interp, afmPtr->weight);
    }
    if (afmPtr->comment != NULL) {
	Tcl_AppendElement(interp, "comment");
	Tcl_AppendElement(interp, afmPtr->comment);
    }
    if (afmPtr->notice != NULL) {
	Tcl_AppendElement(interp, "notice");
	Tcl_AppendElement(interp, afmPtr->notice);
    }
    if (afmPtr->characterSet != NULL) {
	Tcl_AppendElement(interp, "characterSet");
	Tcl_AppendElement(interp, afmPtr->characterSet);
    }
    if (afmPtr->encodingScheme != NULL) {
	Tcl_AppendElement(interp, "encodingScheme");
	Tcl_AppendElement(interp, afmPtr->encodingScheme);
    }
    Tcl_AppendElement(interp, "underlinePosition");
    Tcl_AppendElement(interp, FTOA(afmPtr->underlinePosition));
    Tcl_AppendElement(interp, "underlineThickness");
    Tcl_AppendElement(interp, FTOA(afmPtr->underlineThickness));
    Tcl_AppendElement(interp, "italicAngle");
    Tcl_AppendElement(interp, FTOA(afmPtr->italicAngle));
    Tcl_AppendElement(interp, "capHeight");
    Tcl_AppendElement(interp, FTOA(afmPtr->capHeight));
    Tcl_AppendElement(interp, "xHeight");
    Tcl_AppendElement(interp, FTOA(afmPtr->xHeight));
    Tcl_AppendElement(interp, "ascender");
    Tcl_AppendElement(interp, FTOA(afmPtr->ascender));
    Tcl_AppendElement(interp, "descender");
    Tcl_AppendElement(interp, FTOA(afmPtr->descender));

    Tcl_AppendElement(interp, "isFixedPitch");
    Tcl_AppendElement(interp, Blt_Itoa(afmPtr->isFixedPitch));
    Tcl_AppendElement(interp, "isBaseFont");
    Tcl_AppendElement(interp, Blt_Itoa(afmPtr->isBaseFont));
    Tcl_AppendElement(interp, "isCIDFont");
    Tcl_AppendElement(interp, Blt_Itoa(afmPtr->isCIDFont));
    Tcl_AppendElement(interp, "isFixedV");
    Tcl_AppendElement(interp, Blt_Itoa(afmPtr->isFixedV));
    Tcl_AppendElement(interp, "numCharMetrics");
    Tcl_AppendElement(interp, Blt_Itoa(afmPtr->numCharMetrics));
    Tcl_AppendElement(interp, "numComposites");
    Tcl_AppendElement(interp, Blt_Itoa(afmPtr->numComposites));
    Tcl_AppendElement(interp, "numDirection");
    Tcl_AppendElement(interp, Blt_Itoa(afmPtr->numDirection));
    Tcl_AppendElement(interp, "numKernPairs");
    Tcl_AppendElement(interp, Blt_Itoa(afmPtr->numKernPairs));
    Tcl_AppendElement(interp, "numTrackKern");
    Tcl_AppendElement(interp, Blt_Itoa(afmPtr->numTrackKern));
    Tcl_AppendElement(interp, "escChar");
    Tcl_AppendElement(interp, Blt_Itoa(afmPtr->escChar));
    Tcl_AppendElement(interp, "vvector x");
    Tcl_AppendElement(interp, FTOA(afmPtr->vVector.x));
    Tcl_AppendElement(interp, "vvector y");
    Tcl_AppendElement(interp, FTOA(afmPtr->vVector.y));
    Tcl_AppendElement(interp, "stdHW");
    Tcl_AppendElement(interp, FTOA(afmPtr->stdHW));
    Tcl_AppendElement(interp, "stdVW");
    Tcl_AppendElement(interp, FTOA(afmPtr->stdVW));
    for (i = 0; i < 256; i++) {
	if (afmPtr->metrics[i].index >= 0) {
	    if (afmPtr->metrics[i].hasLigature) {
		
	    Tcl_AppendElement(interp, "index");
	    Tcl_AppendElement(interp, "x");
	    Tcl_AppendElement(interp, FTOA(afmPtr->metrics[i].w.x));
	    Tcl_AppendElement(interp, "y");
	    Tcl_AppendElement(interp, FTOA(afmPtr->metrics[i].w.y));
	    Tcl_AppendElement(interp, FTOA(afmPtr->metrics[i].index));
	    if (afmPtr->metrics[i].name != NULL) {
#ifdef notdef
		int code;
		code = LookupSymbol(afmPtr, afmPtr->metrics[i].name);
		if (code != afmPtr->metrics[i].index) {
		    fprintf(stderr, "index=%d, code=%d, name=%s\n", 
			afmPtr->metrics[i].index, code, 
			afmPtr->metrics[i].name);
		}
#endif
		Tcl_AppendElement(interp, "name");
		Tcl_AppendElement(interp, afmPtr->metrics[i].name);
	    }
	    Tcl_AppendElement(interp, "llx");
	    Tcl_AppendElement(interp, FTOA(afmPtr->metrics[i].bbox.llx));
	    Tcl_AppendElement(interp, "lly");
	    Tcl_AppendElement(interp, FTOA(afmPtr->metrics[i].bbox.lly));
	    Tcl_AppendElement(interp, "urx");
	    Tcl_AppendElement(interp, FTOA(afmPtr->metrics[i].bbox.urx));
	    Tcl_AppendElement(interp, "ury");
	    Tcl_AppendElement(interp, FTOA(afmPtr->metrics[i].bbox.ury));
	    }
	}
    }
    AfmDestroy(afmPtr);
    return TCL_OK;
}

static Blt_OpSpec afmOps[] =
{
    {"dump",    1, DumpOp,  3, 3, "fileName",},
    {"load",    1, LoadOp,  3, 3, "fileName",},
};
static int numAfmOps = sizeof(afmOps) / sizeof(Blt_OpSpec);


static int
AfmCmdProc(ClientData clientData, Tcl_Interp *interp, int objc, 
	   Tcl_Obj *const *objv)
{
    Tcl_ObjCmdProc *proc;
    int result;

    proc = Blt_GetOpFromObj(interp, numAfmOps, afmOps, BLT_OP_ARG1, 
			    objc, objv, 0);
    if (proc == NULL) {
	return TCL_ERROR;
    }
    result = (*proc) (clientData, interp, objc, objv);
    return result;
}


int
Blt_AfmCmdInitProc(Tcl_Interp *interp)
{
    static Blt_CmdSpec cmdSpec = { 
	"afm", AfmCmdProc, 
    };
    return Blt_InitCmd(interp, "::blt", &cmdSpec);
}


#ifdef notdef
static int
AfmStringWidthOp(ClientData clientData, Tcl_Interp *interp, int objc, 
		 Tcl_Obj *const *objv)
{
    Afm *afmPtr;

    afmPtr = AfmParseFile(interp, Tcl_GetString(objv[2]));
    if (afmPtr == NULL) {
	return TCL_ERROR;
    }
    return TCL_OK;
}
#endif

int 
Blt_Afm_GetMetrics(Blt_Font font, Blt_FontMetrics *fmPtr) 
{
    Afm *afmPtr;

    afmPtr = AfmGetMetricsFromFont(font);
    if (afmPtr == NULL) {
	return TCL_ERROR;
    }
#ifndef notdef
    fmPtr->ascent    = Points(afmPtr, afmPtr->ascender);
#else
    fmPtr->ascent    = Points(afmPtr, afmPtr->capHeight);
#endif
    fmPtr->descent   = Points(afmPtr, -afmPtr->descender);
    fmPtr->linespace = Points(afmPtr, afmPtr->ascender - afmPtr->descender);
#ifdef notdef
    fprintf(stderr, "GetMetrics(%s), ascent=%d descent=%d linespace=%d\n",
	    Blt_Font_Name(font), fmPtr->ascent, fmPtr->descent, 
	    fmPtr->linespace);
#endif
    return TCL_OK;
}

int 
Blt_Afm_TextWidth(Blt_Font font, const char *string, int numBytes) 
{
    Afm *afmPtr;
    const char *p, *pend;
    float width;

#ifdef notdef
    fprintf(stderr, "Afm_TextWidth(%s,\"%s\")\n", Blt_Font_Name(font),  
	    string);
#endif
    afmPtr = AfmGetMetricsFromFont(font);
    if (afmPtr == NULL) {
	Blt_Warn("can't find font\n");
	return -1;
    }
    width = 0;
#ifdef notdef
    fprintf(stderr, "string=\"%s\"\n", string);
#endif
    for (p = string, pend = string + numBytes; p < pend; /*empty*/) {
	CharMetrics *cmPtr;
	unsigned char c;
	Tcl_UniChar ch;

	p += Tcl_UtfToUniChar(p, &ch);
	c = (unsigned char)(ch & 0xff);
	cmPtr = afmPtr->metrics + c;
	if (cmPtr->index < 0) {
	    continue;			/* Ignore unencoded characters. */
	}
#ifdef notdef
	fprintf(stderr, "width=%g, incr=%g, char=%c\n",
		width, cmPtr->w.x, c);
#endif
	width += cmPtr->w.x;
    }
    {
	/* Kerning */
	unsigned char c1, c2;
	Tcl_UniChar ch;

	p = string;
	p += Tcl_UtfToUniChar(string, &ch);
	c1 = (unsigned char)(ch & 0xff);
	while (p < pend) {
	    p += Tcl_UtfToUniChar(p, &ch);
	    c2 = (unsigned char)(ch & 0xff);
	    if (afmPtr->metrics[c1].hasKernPair) {
		KernPairs *kp;
		
		kp = GetKernPairs(afmPtr, c1, c2);
		width += kp->x;
	    }
	    c1 = c2;
	}
    }
#ifdef notdef
    fprintf(stderr, "StringWidth of \"%s\" is %d (ps=%f)\n",
	    string, Points(afmPtr, width), afmPtr->pointSize);
#endif
    return Points(afmPtr, width);
}

void 
Blt_Afm_SetPrinting(Tcl_Interp *interp, int state)
{
    afmInterp = ((state) && (interp != NULL)) ? interp : NULL;
}

int
Blt_Afm_IsPrinting(void)
{
    return (afmInterp != NULL);
}

typedef struct {
    const char *alias;
    const char *fontName;
} FamilyMap;

static FamilyMap familyMap[] =
{
    { "Arial",		        "Helvetica"	   },
    { "AvantGarde",             "AvantGarde"       },
    { "Bookman",                "Bookman"          },
    { "Courier New",            "Courier"          },
    { "Courier",                "Courier"          },
    { "Geneva",                 "Helvetica"        },
    { "Helvetica",              "Helvetica"        },
    { "Mathematica1",		"Helvetica"	   },
    { "Monaco",                 "Courier"          },
    { "New Century Schoolbook", "NewCenturySchlbk" },
    { "New York",               "Times"            },
    { "Nimbus Roman No9 L"	"Times"		   },
    { "Nimbus Sans L Condensed","Helvetica"        },
    { "Nimbus Sans L",		"Helvetica"        },
    { "Palatino",               "Palatino"         },
    { "Standard Symbols L",	"Symbol"           },
    { "Swiss 721",              "Helvetica"        },
    { "Symbol",                 "Symbol"           },
    { "Times New Roman",        "Times"            },
    { "Times Roman",            "Times"            },
    { "Times",                  "Times"            },
    { "ZapfChancery",           "ZapfChancery"     },
    { "ZapfDingbats",           "ZapfDingbats"     }
};

static int numFamilyNames = (sizeof(familyMap) / sizeof(FamilyMap));

const char *
Blt_Afm_GetPostscriptFamily(const char *family) 
{
    FamilyMap *fp, *fend;

    if (strncasecmp(family, "itc ", 4) == 0) {
	family += 4;
    }
    for (fp = familyMap, fend = fp + numFamilyNames; fp < fend; fp++) {
	if (strcasecmp(fp->alias, family) == 0) {
	    return fp->fontName;
	}
    }
    return NULL;
}

void
Blt_Afm_GetPostscriptName(const char *family, int flags, 
			   Tcl_DString *resultPtr)
{
    const char *familyName, *weightName, *slantName;
    int len;

    len = Tcl_DStringLength(resultPtr);

    familyName = Blt_Afm_GetPostscriptFamily(family);
    if (familyName == NULL) {
	Tcl_UniChar ch;
	char *src, *dest;
	int upper;

	/*
	 * Inline, capitalize the first letter of each word, lowercase the
	 * rest of the letters in each word, and then take out the spaces
	 * between the words.  This may make the DString shorter, which is
	 * safe to do.
	 */
	Tcl_DStringAppend(resultPtr, family, -1);
	src = dest = Tcl_DStringValue(resultPtr) + len;
	upper = TRUE;
	while (*src != '\0') {
	    while (isspace(*src)) { /* INTL: ISO space */
		src++;
		upper = TRUE;
	    }
	    src += Tcl_UtfToUniChar(src, &ch);
	    if (upper) {
		ch = Tcl_UniCharToUpper(ch);
		upper = FALSE;
	    } else {
		ch = Tcl_UniCharToLower(ch);
	    }
	    dest += Tcl_UniCharToUtf(ch, dest);
	}
	*dest = '\0';
	Tcl_DStringSetLength(resultPtr, dest - Tcl_DStringValue(resultPtr));
	familyName = Tcl_DStringValue(resultPtr) + len;
    }
    if (familyName != Tcl_DStringValue(resultPtr) + len) {
	Tcl_DStringAppend(resultPtr, familyName, -1);
	familyName = Tcl_DStringValue(resultPtr) + len;
    }
    if (strcasecmp(familyName, "NewCenturySchoolbook") == 0) {
	Tcl_DStringSetLength(resultPtr, len);
	Tcl_DStringAppend(resultPtr, "NewCenturySchlbk", -1);
	familyName = Tcl_DStringValue(resultPtr) + len;
    }

    /* Get the string to use for the weight. */
    weightName = NULL;
    if (flags & FONT_BOLD) {
	if ((strcmp(familyName, "Bookman") == 0) || 
	    (strcmp(familyName, "AvantGarde") == 0)) {
	    weightName = "Demi";
	} else {
	    weightName = "Bold";
	}
    } else {
	if (strcmp(familyName, "Bookman") == 0) {
	    weightName = "Light";
	} else if (strcmp(familyName, "AvantGarde") == 0) {
	    weightName = "Book";
	} else if (strcmp(familyName, "ZapfChancery") == 0) {
	    weightName = "Medium";
	}
    }

    /* Get the string to use for the slant. */
    slantName = NULL;
    if (flags & FONT_ITALIC) {
	if ((strcmp(familyName, "Helvetica") == 0) || 
	    (strcmp(familyName, "Courier") == 0) || 
	    (strcmp(familyName, "AvantGarde") == 0)) {
	    slantName = "Oblique";
	} else {
	    slantName = "Italic";
	}
    }

    /*
     * The string "Roman" needs to be added to some fonts that are not bold
     * and not italic.
     */
    if ((slantName == NULL) && (weightName == NULL)) {
	if ((strcmp(familyName, "Times") == 0) || 
	    (strcmp(familyName, "NewCenturySchlbk") == 0) || 
	    (strcmp(familyName, "Palatino") == 0)) {
	    Tcl_DStringAppend(resultPtr, "-Roman", -1);
	}
    } else {
	Tcl_DStringAppend(resultPtr, "-", -1);
	if (weightName != NULL) {
	    Tcl_DStringAppend(resultPtr, weightName, -1);
	}
	if (slantName != NULL) {
	    Tcl_DStringAppend(resultPtr, slantName, -1);
	}
    }
}
