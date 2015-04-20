/* -*- mode: c; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 * bltVecMath.c --
 *
 * This module implements mathematical expressions with vector data
 * objects.
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

#include "bltVecInt.h"
#ifdef HAVE_STDLIB_H
#  include <stdlib.h>
#endif /* HAVE_STDLIB_H */

#ifdef HAVE_STRING_H
#  include <string.h>
#endif /* HAVE_STRING_H */

#ifdef HAVE_CTYPE_H
#  include <ctype.h>
#endif /* HAVE_CTYPE_H */

#ifdef HAVE_ERRNO_H
#  include <errno.h>
#endif /* HAVE_ERRNO_H */

#include "bltAlloc.h"
#include "bltNsUtil.h"
#include "bltParse.h"


/*
 * Three types of math functions:
 *
 *	PointProc		Function is applied in multiple calls to
 *				each point of the vector.
 *	VectorProc		Entire vector is passed, each point is
 *				modified.
 *	ScalarProc		Entire vector is passed, single scalar value
 *				is returned.
 */

typedef double (PointProc1)(double value);
typedef double (PointProc0)(void);
typedef int (VectorProc)(Vector *vPtr);
typedef double (ScalarProc)(Vector *vPtr);

/*
 * Built-in math functions:
 */
#ifdef __cplusplus
typedef int (GenericMathProc)(...);
#else
typedef int (GenericMathProc)();
#endif

/*
 * MathFunction --
 *
 *	Contains information about math functions that can be called
 *	for vectors.  The table of math functions is global within the
 *	application.  So you can't define two different "sqrt"
 *	functions.
 */
typedef struct {
    const char *name;		/* Name of built-in math function.  If
				 * NULL, indicates that the function
				 * was user-defined and dynamically
				 * allocated.  Function names are
				 * global across all interpreters. */

    void *proc;			/* Procedure that implements this math
				 * function. */

    ClientData clientData;	/* Argument to pass when invoking the
				 * function. */

} MathFunction;


/*
 * Macros for testing floating-point values for certain special cases:
 *
 *	IS_NAN	Test for not-a-number by comparing a value against itself
 *	IF_INF	Test for infinity by comparing against the largest floating
 *		point value.
 */

#define IS_NAN(v) ((v) != (v))

#ifdef DBL_MAX
#   define IS_INF(v) (((v) > DBL_MAX) || ((v) < -DBL_MAX))
#else
#   define IS_INF(v) 0
#endif

/* The data structure below is used to describe an expression value,
 * which can be either a double-precision floating-point value, or a
 * string.  A given number has only one value at a time.  */

#define STATIC_STRING_SPACE 150

/*
 * Tokens --
 *
 *	The token types are defined below.  In addition, there is a
 *	table associating a precedence with each operator.  The order
 *	of types is important.  Consult the code before changing it.
 */
enum Tokens {
    VALUE, OPEN_PAREN, CLOSE_PAREN, COMMA, END, UNKNOWN,
    MULT = 8, DIVIDE, MOD, PLUS, MINUS,
    LEFT_SHIFT, RIGHT_SHIFT,
    LESS, GREATER, LEQ, GEQ, EQUAL, NEQ,
    OLD_BIT_AND, EXPONENT, OLD_BIT_OR, OLD_QUESTY, OLD_COLON,
    AND, OR, UNARY_MINUS, OLD_UNARY_PLUS, NOT, OLD_BIT_NOT
};

typedef struct {
    Vector *vPtr;
    char staticSpace[STATIC_STRING_SPACE];
    ParseValue pv;		/* Used to hold a string value, if any. */
} Value;

/*
 * ParseInfo --
 *
 *	The data structure below describes the state of parsing an
 *	expression.  It's passed among the routines in this module.
 */
typedef struct {
    const char *expr;		/* The entire right-hand side of the
				 * expression, as originally passed to
				 * Blt_ExprVector. */

    const char *nextPtr;	/* Position of the next character to
				 * be scanned from the expression
				 * string. */

    enum Tokens token;		/* Type of the last token to be parsed
				 * from nextPtr.  See below for
				 * definitions.  Corresponds to the
				 * characters just before nextPtr. */

} ParseInfo;

/*
 * Precedence table.  The values for non-operator token types are ignored.
 */
static int precTable[] =
{
    0, 0, 0, 0, 0, 0, 0, 0,
    12, 12, 12,			/* MULT, DIVIDE, MOD */
    11, 11,			/* PLUS, MINUS */
    10, 10,			/* LEFT_SHIFT, RIGHT_SHIFT */
    9, 9, 9, 9,			/* LESS, GREATER, LEQ, GEQ */
    8, 8,			/* EQUAL, NEQ */
    7,				/* OLD_BIT_AND */
    13,				/* EXPONENTIATION */
    5,				/* OLD_BIT_OR */
    4,				/* AND */
    3,				/* OR */
    2,				/* OLD_QUESTY */
    1,				/* OLD_COLON */
    14, 14, 14, 14		/* UNARY_MINUS, OLD_UNARY_PLUS, NOT,
				 * OLD_BIT_NOT */
};


/*
 * Forward declarations.
 */

static int NextValue(Tcl_Interp *interp, ParseInfo *piPtr, int prec, 
	Value *valuePtr);

#include <bltMath.h>

/*
 *---------------------------------------------------------------------------
 *
 * Sort --
 *
 *	A vector math function.  Sorts the values of the given 
 *	vector.
 *
 * Results:
 *	Always TCL_OK.
 *
 * Side Effects:
 *	The vector is sorted.
 *
 *---------------------------------------------------------------------------
 */
static int
Sort(Vector *vPtr)
{
    size_t *map;
    double *values;
    int i;
    size_t sortLength;

    sortLength = vPtr->length;
    Blt_Vec_SortMap(&vPtr, 1, &map);
    values = Blt_AssertMalloc(sizeof(double) * sortLength);
    for(i = 0; i < sortLength; i++) {
	values[i] = vPtr->valueArr[map[i]];
    }
    Blt_Vec_Reset(vPtr, values, sortLength, sortLength, TCL_DYNAMIC);
    Blt_Free(map);
    return TCL_OK;
}

static double
Length(Blt_Vector *vectorPtr)
{
    Vector *vPtr = (Vector *)vectorPtr;

    return (double)vPtr->length;
}

double
Blt_VecMax(Blt_Vector *vectorPtr)
{
    Vector *vPtr = (Vector *)vectorPtr;

    return Blt_Vec_Max(vPtr);
}

double
Blt_VecMin(Blt_Vector *vectorPtr)
{
    Vector *vPtr = (Vector *)vectorPtr;

    return Blt_Vec_Min(vPtr);
}


static double
Product(Blt_Vector *vectorPtr)
{
    Vector *vPtr = (Vector *)vectorPtr;
    double prod;
    int i;

    prod = 1.0;
    for(i = 0; i < vPtr->length; i++) {
	if (!FINITE(vPtr->valueArr[i])) {
	    continue;
	}
	prod *= vPtr->valueArr[i];
    }
    return prod;
}

static double
GetSum(Blt_Vector *vectorPtr, int *nonEmptyPtr)
{
    Vector *vPtr = (Vector *)vectorPtr;
    double sum;
    int i, count;

    /* Kahan summation algorithm */

    for (i = 0; i < vPtr->length; i++) {
	if (FINITE(vPtr->valueArr[i])) {
	    break;
	}
    }
    sum = 0.0;
    count = 0;
    if (i < vPtr->length) {
	double c;
	sum = vPtr->valueArr[i];
	c = 0.0;			/* A running compensation for lost
					 * low-order bits.*/
	count = 1;
	for (/*empty*/; i < vPtr->length; i++) {
	    double y, t;
	    
	    if (!FINITE(vPtr->valueArr[i])) {
		continue;
	    }
	    count++;
	    y = vPtr->valueArr[i] - c;	/* So far, so good: c is zero.*/
	    t = sum + y;		/* Alas, sum is big, y small, so
					 * low-order digits of y are lost.*/
	    c = (t - sum) - y;		/* (t - sum) recovers the high-order
					 * part of y; subtracting y recovers
					 * -(low part of y) */
	    sum = t;
	}
    }
    *nonEmptyPtr = count;
    return sum;
}

static double
Sum(Blt_Vector *vectorPtr)
{
    int count;

    return GetSum(vectorPtr, &count);
}

static double
Mean(Blt_Vector *vectorPtr)
{
    double sum;
    int n;

    sum = GetSum(vectorPtr, &n);
    if (n == 0) {
	return Blt_NaN();
    }
    return sum / (double)n;
}

/*
 *  var = 1/N Sum( (x[i] - mean)^2 )
 */
static double
Variance(Blt_Vector *vectorPtr)
{
    Vector *vPtr = (Vector *)vectorPtr;
    double var, mean;
    int i, count;

    mean = Mean(vectorPtr);
    var = 0.0;
    count = 0;
    for(i = 0; i < vPtr->length; i++) {
	double dx;

	if (!FINITE(vPtr->valueArr[i])) {
	    continue;
	}
	dx = vPtr->valueArr[i] - mean;
	var += dx * dx;
	count++;
    }
    if (count < 2) {
	return 0.0;
    }
    var /= (double)(count - 1);
    return var;
}

/*
 *  skew = Sum( (x[i] - mean)^3 ) / (var^3/2)
 */
static double
Skew(Blt_Vector *vectorPtr)
{
    Vector *vPtr = (Vector *)vectorPtr;
    double var, skew, mean;
    int i, count;

    mean = Mean(vectorPtr);
    var = skew = 0.0;
    count = 0;
    for(i = 0; i < vPtr->length; i++) {
	double dx, dx2;

	if (!FINITE(vPtr->valueArr[i])) {
	    continue;
	}
	dx = vPtr->valueArr[i] - mean;
	dx = FABS(dx);
	dx2 = dx * dx;
	var += dx2;
	skew += dx2 * dx;
	count++;
    }
    if (count < 2) {
	return 0.0;
    }
    var /= (double)(count - 1);
    skew /= count * var * sqrt(var);
    return skew;
}

static double
StdDeviation(Blt_Vector *vectorPtr)
{
    double var;

    var = Variance(vectorPtr);
    if (var > 0.0) {
	return sqrt(var);
    }
    return 0.0;
}


static double
AvgDeviation(Blt_Vector *vectorPtr)
{
    Vector *vPtr = (Vector *)vectorPtr;
    double avg, mean;
    int i, count;

    mean = Mean(vectorPtr);
    avg = 0.0;
    count = 0;
    for(i = 0; i < vPtr->length; i++) {
	double diff;

	if (!FINITE(vPtr->valueArr[i])) {
	    continue;
	}
	diff = vPtr->valueArr[i] - mean;
	avg += FABS(diff);
	count++;
    }
    if (count < 2) {
	return 0.0;
    }
    avg /= (double)count;
    return avg;
}


static double
Kurtosis(Blt_Vector *vectorPtr)
{
    Vector *vPtr = (Vector *)vectorPtr;
    double kurt, var, mean;
    int i, count;

    mean = Mean(vectorPtr);
    var = kurt = 0.0;
    count = 0;
    for(i = 0; i < vPtr->length; i++) {
	double diff, diffsq;

	if (!FINITE(vPtr->valueArr[i])) {
	    continue;
	}
	diff = vPtr->valueArr[i] - mean;
	diffsq = diff * diff;
	var += diffsq;
	kurt += diffsq * diffsq;
	count++;
    }
    if (count < 2) {
	return 0.0;
    }
    var /= (double)(count - 1);
    if (var == 0.0) {
	return 0.0;
    }
    kurt /= (count * var * var);
    return kurt - 3.0;			/* Fisher Kurtosis */
}


static double
Median(Blt_Vector *vectorPtr)
{
    Vector *vPtr = (Vector *)vectorPtr;
    size_t *map;
    double q2;
    int mid;
    size_t sortLength;
    
    if (vPtr->length == 0) {
	return -DBL_MAX;
    }
    /* FIXME: compute number of nonempty slots.  */
    sortLength = vPtr->length;
    Blt_Vec_SortMap(&vPtr, 1, &map);
    mid = (sortLength - 1) / 2;

    /*  
     * Determine Q2 by checking if the number of elements [0..n-1] is
     * odd or even.  If even, we must take the average of the two
     * middle values.  
     */
    if (sortLength & 1) { /* Odd */
	q2 = vPtr->valueArr[map[mid]];
    } else {			/* Even */
	q2 = (vPtr->valueArr[map[mid]] + 
	      vPtr->valueArr[map[mid + 1]]) * 0.5;
    }
    Blt_Free(map);
    return q2;
}

static double
Q1(Blt_Vector *vectorPtr)
{
    Vector *vPtr = (Vector *)vectorPtr;
    double q1;
    size_t *map;
    size_t sortLength;

    if (vPtr->length == 0) {
	return -DBL_MAX;
    } 
    /* FIXME: compute number of nonempty slots.  */
    sortLength = vPtr->length;
    Blt_Vec_SortMap(&vPtr, 1, &map);

    if (sortLength < 4) {
	q1 = vPtr->valueArr[map[0]];
    } else {
	int mid, q;

	mid = (sortLength - 1) / 2;
	q = mid / 2;

	/* 
	 * Determine Q1 by checking if the number of elements in the
	 * bottom half [0..mid) is odd or even.   If even, we must
	 * take the average of the two middle values.
	 */
	if (mid & 1) {		/* Odd */
	    q1 = vPtr->valueArr[map[q]]; 
	} else {		/* Even */
	    q1 = (vPtr->valueArr[map[q]] + 
		  vPtr->valueArr[map[q + 1]]) * 0.5; 
	}
    }
    Blt_Free(map);
    return q1;
}

static double
Q3(Blt_Vector *vectorPtr)
{
    Vector *vPtr = (Vector *)vectorPtr;
    double q3;
    size_t *map;
    size_t sortLength;

    if (vPtr->length == 0) {
	return -DBL_MAX;
    } 
    /* FIXME: compute number of nonempty slots.  */
    sortLength = vPtr->length;
    Blt_Vec_SortMap(&vPtr, 1, &map);

    if (sortLength < 4) {
	q3 = vPtr->valueArr[map[sortLength - 1]];
    } else {
	int mid, q;

	mid = (sortLength - 1) / 2;
	q = (sortLength + mid) / 2;

	/* 
	 * Determine Q3 by checking if the number of elements in the
	 * upper half (mid..n-1] is odd or even.   If even, we must
	 * take the average of the two middle values.
	 */
	if (mid & 1) {		/* Odd */
	    q3 = vPtr->valueArr[map[q]];
	} else {		/* Even */
	    q3 = (vPtr->valueArr[map[q]] + 
		  vPtr->valueArr[map[q + 1]]) * 0.5; 
	}
    }
    Blt_Free(map);
    return q3;
}


static int
Norm(Blt_Vector *vector)
{
    Vector *vPtr = (Vector *)vector;
    double min, max;
    int i;

    min = DBL_MAX;
    max = -DBL_MAX;
    for (i = 0; i < vPtr->length; i++) {
	if (!FINITE(vPtr->valueArr[i])) {
	    continue;
	}
	if (min > vPtr->valueArr[i]) {
	    min = vPtr->valueArr[i];
	}
	if (max < vPtr->valueArr[i]) {
	    max = vPtr->valueArr[i];
	}
    }
    if (min < max) {
	double range;
	int i;

	range = max - min;
	for (i = 0; i < vPtr->length; i++) {
	    if (FINITE(vPtr->valueArr[i])) {
		double norm;

		norm = (vPtr->valueArr[i] - min) / range;
		vPtr->valueArr[i] = norm;
	    }
	}
    }
    return TCL_OK;
}

static double
Count(Blt_Vector *vector)
{
    Vector *vPtr = (Vector *)vector;
    int count;
    int i;


    count = 0;
    for (i = 0; i < vPtr->length; i++) {
	if (FINITE(vPtr->valueArr[i])) {
	    count++;
	}
    }
    return (double) count;
}


static double
Nonzeros(Blt_Vector *vector)
{
    Vector *vPtr = (Vector *)vector;
    int count;
    int i;

    count = 0;
    for (i = 0; i < vPtr->length; i++) {
	if ((FINITE(vPtr->valueArr[i])) && (vPtr->valueArr[i] != 0.0)) {
	    count++;
	}
    }
    return (double)count;
}

static double
Fabs(double value)
{
    if (!FINITE(value)) {
	return Blt_NaN();
    }
    if (value < 0.0) {
	return -value;
    }
    return value;
}

static double
Round(double value)
{
    if (!FINITE(value)) {
	return Blt_NaN();
    }
    if (value < 0.0) {
	return ceil(value - 0.5);
    } else {
	return floor(value + 0.5);
    }
}

static double
Fmod(double x, double y)
{
    if ((!FINITE(y)) || (!FINITE(x))) {
	return Blt_NaN();
    }
    if (y == 0.0) {
	return 0.0;
    }
    return x - (floor(x / y) * y);
}

/*
 *---------------------------------------------------------------------------
 *
 * MathError --
 *
 *	This procedure is called when an error occurs during a floating-point
 *	operation.  It reads errno and sets interp->result accordingly.
 *
 * Results:
 *	Interp->result is set to hold an error message.
 *
 * Side effects:
 *	None.
 *
 *---------------------------------------------------------------------------
 */
static void
MathError(
    Tcl_Interp *interp,			/* Where to store error message. */
    double value)			/* Value returned after error; used to
					 * distinguish underflows from
					 * overflows. */
{
    if ((errno == EDOM) || (value != value)) {
	const char *result;

	Tcl_AppendResult(interp, "domain error: argument not in valid range",
			 (char *)NULL);
	result = Tcl_GetString(Tcl_GetObjResult(interp));
	Tcl_SetErrorCode(interp, "ARITH", "DOMAIN", result, (char *)NULL);
    } else if ((errno == ERANGE) || IS_INF(value)) {
	if (value == 0.0) {
	    const char *result;

	    Tcl_AppendResult(interp, 
			     "floating-point value too small to represent",
		(char *)NULL);
	    result = Tcl_GetString(Tcl_GetObjResult(interp));
	    Tcl_SetErrorCode(interp, "ARITH", "UNDERFLOW", result,(char *)NULL);
	} else {
	    const char *result;

	    Tcl_AppendResult(interp, 
			     "floating-point value too large to represent",
		(char *)NULL);
	    result = Tcl_GetString(Tcl_GetObjResult(interp));
	    Tcl_SetErrorCode(interp, "ARITH", "OVERFLOW", result, (char *)NULL);
	}
    } else {
	const char *result;

	Tcl_AppendResult(interp, "unknown floating-point error, ",
		"errno = ", Blt_Itoa(errno), (char *)NULL);
	result = Tcl_GetString(Tcl_GetObjResult(interp));
	Tcl_SetErrorCode(interp, "ARITH", "UNKNOWN", result, (char *)NULL);
    }
}

/*
 *---------------------------------------------------------------------------
 *
 * ParseString --
 *
 *	Given a string (such as one coming from command or variable
 *	substitution), make a Value based on the string.  The value will be
 *	a floating-point or integer, if possible, or else it will just be a
 *	copy of the string.
 *
 * Results:
 *	TCL_OK is returned under normal circumstances, and TCL_ERROR is
 *	returned if a floating-point overflow or underflow occurred while
 *	reading in a number.  The value at *valuePtr is modified to hold a
 *	number, if possible.
 *
 * Side effects:
 *	None.
 *
 *---------------------------------------------------------------------------
 */

static int
ParseString(
    Tcl_Interp *interp,                 /* Where to store error message. */
    const char *string,                 /* String to turn into value. */
    Value *valuePtr)                    /* Where to store value
                                         * information.  Caller must have
                                         * initialized pv field. */
{
    const char *endPtr;
    double value;

    errno = 0;

    /*   
     * The string can be either a number or a vector.  First try to
     * convert the string to a number.  If that fails then see if
     * we can find a vector by that name.
     */

    value = strtod(string, (char **)&endPtr);
    if ((endPtr != string) && (*endPtr == '\0')) {
	if (errno != 0) {
	    Tcl_ResetResult(interp);
	    MathError(interp, value);
	    return TCL_ERROR;
	}
	/* Numbers are stored as single element vectors. */
	if (Blt_Vec_ChangeLength(interp, valuePtr->vPtr, 1) != TCL_OK) {
	    return TCL_ERROR;
	}
	valuePtr->vPtr->valueArr[0] = value;
	return TCL_OK;
    } else {
	Vector *vPtr;

	while (isspace(UCHAR(*string))) {
	    string++;		/* Skip spaces leading the vector name. */    
	}
	vPtr = Blt_Vec_ParseElement(interp, valuePtr->vPtr->dataPtr, 
		string, &endPtr, NS_SEARCH_BOTH);
	if (vPtr == NULL) {
	    return TCL_ERROR;
	}
	if (*endPtr != '\0') {
	    Tcl_AppendResult(interp, "extra characters after vector", 
			     (char *)NULL);
	    return TCL_ERROR;
	}
	/* Copy the designated vector to our temporary. */
	Blt_Vec_Duplicate(valuePtr->vPtr, vPtr);
    }
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * ParseMathFunction --
 *
 *	This procedure is invoked to parse a math function from an
 *	expression string, carry out the function, and return the value
 *	computed.
 *
 * Results:
 *	TCL_OK is returned if all went well and the function's value was
 *	computed successfully.  If the name doesn't match any known math
 *	function, returns TCL_RETURN. And if a format error was found,
 *	TCL_ERROR is returned and an error message is left in
 *	interp->result.
 *
 *	After a successful return piPtr will be updated to point to the
 *	character just after the function call, the token is set to VALUE,
 *	and the value is stored in valuePtr.
 *
 * Side effects:
 *	Embedded commands could have arbitrary side-effects.
 *
 *---------------------------------------------------------------------------
 */
static int
ParseMathFunction(
    Tcl_Interp *interp,                 /* Interpreter to use for error
                                         * reporting. */
    const char *start,                  /* Start of string to parse */
    ParseInfo *piPtr,                   /* Describes the state of the
                                         * parse.  piPtr->nextPtr must
                                         * point to the first character of
                                         * the function's name. */
    Value *valuePtr)                    /* Where to store value, if that is
                                         * what's parsed from string.
                                         * Caller must have initialized pv
                                         * field correctly. */
{
    Blt_HashEntry *hPtr;
    MathFunction *mathPtr;              /* Info about math function. */
    char *p;
    VectorCmdInterpData *dataPtr;          /* Interpreter-specific data. */
    GenericMathProc *proc;

    /*
     * Find the end of the math function's name and lookup the record for
     * the function.
     */
    p = (char *)start;
    while (isspace(UCHAR(*p))) {
	p++;
    }
    piPtr->nextPtr = p;
    while (isalnum(UCHAR(*p)) || (*p == '_')) {
	p++;
    }
    if (*p != '(') {
	return TCL_RETURN;              /* Must start with open
                                         * parenthesis */
    }
    dataPtr = valuePtr->vPtr->dataPtr;
    *p = '\0';
    hPtr = Blt_FindHashEntry(&dataPtr->mathProcTable, piPtr->nextPtr);
    *p = '(';
    if (hPtr == NULL) {
	return TCL_RETURN;              /* Name doesn't match any known
                                         * function */
    }
    /* Pick up the single value as the argument to the function */
    piPtr->token = OPEN_PAREN;
    piPtr->nextPtr = p + 1;
    valuePtr->pv.next = valuePtr->pv.buffer;
    if (NextValue(interp, piPtr, -1, valuePtr) != TCL_OK) {
	return TCL_ERROR;               /* Parse error */
    }
    if (piPtr->token != CLOSE_PAREN) {
	Tcl_AppendResult(interp, "unmatched parentheses in expression \"",
	    piPtr->expr, "\"", (char *)NULL);
	return TCL_ERROR;               /* Missing right parenthesis */
    }
    mathPtr = Blt_GetHashValue(hPtr);
    proc = mathPtr->proc;
    if ((*proc) (mathPtr->clientData, interp, valuePtr->vPtr) != TCL_OK) {
	return TCL_ERROR;               /* Function invocation error */
    }
    piPtr->token = VALUE;
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * NextToken --
 *
 *	Lexical analyzer for expression parser: parses a single value,
 *	operator, or other syntactic element from an expression string.
 *
 * Results:
 *	TCL_OK is returned unless an error occurred while doing lexical
 *	analysis or executing an embedded command.  In that case a standard
 *	TCL error is returned, using interp->result to hold an error
 *	message.  In the event of a successful return, the token and field
 *	in piPtr is updated to refer to the next symbol in the expression
 *	string, and the expr field is advanced past that token; if the
 *	token is a value, then the value is stored at valuePtr.
 *
 * Side effects:
 *	None.
 *
 *---------------------------------------------------------------------------
 */
static int
NextToken(
    Tcl_Interp *interp,                 /* Interpreter to use for error
                                         * reporting. */
    ParseInfo *piPtr,                   /* Describes the state of the
                                         * parser. */
    Value *valuePtr)                    /* Where to store value, if that is
                                         * what's parsed from string.
                                         * Caller must have initialized pv
                                         * field correctly. */
{
    const char *p;
    const char *endPtr;
    const char *var;
    int result;

    p = piPtr->nextPtr;
    while (isspace(UCHAR(*p))) {
	p++;
    }
    if (*p == '\0') {
	piPtr->token = END;
	piPtr->nextPtr = p;
	return TCL_OK;
    }
    /*
     * Try to parse the token as a floating-point number. But check that
     * the first character isn't a "-" or "+", which "strtod" will happily
     * accept as an unary operator.  Otherwise, we might accidently treat a
     * binary operator as unary by mistake, which will eventually cause a
     * syntax error.
     */
    if ((*p != '-') && (*p != '+')) {
	double value;

	errno = 0;
	value = strtod(p, (char **)&endPtr);
	if (endPtr != p) {
	    if (errno != 0) {
		MathError(interp, value);
		return TCL_ERROR;
	    }
	    piPtr->token = VALUE;
	    piPtr->nextPtr = endPtr;

	    /*
	     * Save the single floating-point value as an 1-point vector.
	     */
	    if (Blt_Vec_ChangeLength(interp, valuePtr->vPtr, 1) != TCL_OK) {
		return TCL_ERROR;
	    }
	    valuePtr->vPtr->valueArr[0] = value;
	    return TCL_OK;
	}
    }
    piPtr->nextPtr = p + 1;
    switch (*p) {
    case '$':
	piPtr->token = VALUE;
	var = Tcl_ParseVar(interp, p, &endPtr);
	if (var == NULL) {
	    return TCL_ERROR;
	}
	piPtr->nextPtr = endPtr;
	Tcl_ResetResult(interp);
	result = ParseString(interp, var, valuePtr);
	return result;

    case '[':
	piPtr->token = VALUE;
	result = Blt_ParseNestedCmd(interp, p + 1, 0, &endPtr, &valuePtr->pv);
	if (result != TCL_OK) {
	    return result;
	}
	piPtr->nextPtr = endPtr;
	Tcl_ResetResult(interp);
	result = ParseString(interp, valuePtr->pv.buffer, valuePtr);
	return result;

    case '"':
	piPtr->token = VALUE;
	result = Blt_ParseQuotes(interp, p + 1, '"', 0, &endPtr, &valuePtr->pv);
	if (result != TCL_OK) {
	    return result;
	}
	piPtr->nextPtr = endPtr;
	Tcl_ResetResult(interp);
	result = ParseString(interp, valuePtr->pv.buffer, valuePtr);
	return result;

    case '{':
	piPtr->token = VALUE;
	result = Blt_ParseBraces(interp, p + 1, &endPtr, &valuePtr->pv);
	if (result != TCL_OK) {
	    return result;
	}
	piPtr->nextPtr = endPtr;
	Tcl_ResetResult(interp);
	result = ParseString(interp, valuePtr->pv.buffer, valuePtr);
	return result;

    case '(':
	piPtr->token = OPEN_PAREN;
	break;

    case ')':
	piPtr->token = CLOSE_PAREN;
	break;

    case ',':
	piPtr->token = COMMA;
	break;

    case '*':
	piPtr->token = MULT;
	break;

    case '/':
	piPtr->token = DIVIDE;
	break;

    case '%':
	piPtr->token = MOD;
	break;

    case '+':
	piPtr->token = PLUS;
	break;

    case '-':
	piPtr->token = MINUS;
	break;

    case '^':
	piPtr->token = EXPONENT;
	break;

    case '<':
	switch (*(p + 1)) {
	case '<':
	    piPtr->nextPtr = p + 2;
	    piPtr->token = LEFT_SHIFT;
	    break;
	case '=':
	    piPtr->nextPtr = p + 2;
	    piPtr->token = LEQ;
	    break;
	default:
	    piPtr->token = LESS;
	    break;
	}
	break;

    case '>':
	switch (*(p + 1)) {
	case '>':
	    piPtr->nextPtr = p + 2;
	    piPtr->token = RIGHT_SHIFT;
	    break;
	case '=':
	    piPtr->nextPtr = p + 2;
	    piPtr->token = GEQ;
	    break;
	default:
	    piPtr->token = GREATER;
	    break;
	}
	break;

    case '=':
	if (*(p + 1) == '=') {
	    piPtr->nextPtr = p + 2;
	    piPtr->token = EQUAL;
	} else {
	    piPtr->token = UNKNOWN;
	}
	break;

    case '&':
	if (*(p + 1) == '&') {
	    piPtr->nextPtr = p + 2;
	    piPtr->token = AND;
	} else {
	    piPtr->token = UNKNOWN;
	}
	break;

    case '|':
	if (*(p + 1) == '|') {
	    piPtr->nextPtr = p + 2;
	    piPtr->token = OR;
	} else {
	    piPtr->token = UNKNOWN;
	}
	break;

    case '!':
	if (*(p + 1) == '=') {
	    piPtr->nextPtr = p + 2;
	    piPtr->token = NEQ;
	} else {
	    piPtr->token = NOT;
	}
	break;

    default:
	piPtr->token = VALUE;
	result = ParseMathFunction(interp, p, piPtr, valuePtr);
	if ((result == TCL_OK) || (result == TCL_ERROR)) {
	    return result;
	} else {
	    Vector *vPtr;

	    while (isspace(UCHAR(*p))) {
		p++;		/* Skip spaces leading the vector name. */    
	    }
	    vPtr = Blt_Vec_ParseElement(interp, valuePtr->vPtr->dataPtr, 
			p, &endPtr, NS_SEARCH_BOTH);
	    if (vPtr == NULL) {
		return TCL_ERROR;
	    }
	    Blt_Vec_Duplicate(valuePtr->vPtr, vPtr);
	    piPtr->nextPtr = endPtr;
	}
    }
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * NextValue --
 *
 *	Parse a "value" from the remainder of the expression in piPtr.
 *
 * Results:
 *	Normally TCL_OK is returned.  The value of the expression is
 *	returned in *valuePtr.  If an error occurred, then interp->result
 *	contains an error message and TCL_ERROR is returned.
 *	InfoPtr->token will be left pointing to the token AFTER the
 *	expression, and piPtr->nextPtr will point to the character just
 *	after the terminating token.
 *
 * Side effects:
 *	None.
 *
 *---------------------------------------------------------------------------
 */
static int
NextValue(
    Tcl_Interp *interp,			/* Interpreter to use for error
					 * reporting. */
    ParseInfo *piPtr,			/* Describes the state of the parse
					 * just before the value
					 * (i.e. NextToken will be called to
					 * get first token of value). */
    int prec,				/* Treat any un-parenthesized operator
					 * with precedence <= this as the end
					 * of the expression. */
    Value *valuePtr)			/* Where to store the value of the
					 * expression.  Caller must have
					 * initialized * pv field. */
{
    Value value2;			/* Second operand for current
					 * operator.  */
    int operator;			/* Current operator (either unary or
					 * binary). */
    int gotOp;				/* Non-zero means already lexed the
					 * operator (while picking up value
					 * for unary operator).  Don't lex
					 * again. */
    int result;
    Vector *vPtr, *v2Ptr;
    int i;
    double *values;

    /*
     * There are two phases to this procedure.  First, pick off an initial
     * value.  Then, parse (binary operator, value) pairs until done.
     */

    vPtr = valuePtr->vPtr;
    v2Ptr = Blt_Vec_New(vPtr->dataPtr);
    gotOp = FALSE;
    value2.vPtr = v2Ptr;
    value2.pv.buffer = value2.pv.next = value2.staticSpace;
    value2.pv.end = value2.pv.buffer + STATIC_STRING_SPACE - 1;
    value2.pv.expandProc = Blt_ExpandParseValue;
    value2.pv.clientData = NULL;

    values = NULL;
    result = NextToken(interp, piPtr, valuePtr);
    if (result != TCL_OK) {
	goto done;
    }
    if (piPtr->token == OPEN_PAREN) {

	/* Parenthesized sub-expression. */

	result = NextValue(interp, piPtr, -1, valuePtr);
	if (result != TCL_OK) {
	    goto done;
	}
	if (piPtr->token != CLOSE_PAREN) {
	    Tcl_AppendResult(interp, "unmatched parentheses in expression \"",
		piPtr->expr, "\"", (char *)NULL);
	    result = TCL_ERROR;
	    goto done;
	}
    } else {
	if (piPtr->token == MINUS) {
	    piPtr->token = UNARY_MINUS;
	}
	if (piPtr->token >= UNARY_MINUS) {
	    operator = piPtr->token;
	    result = NextValue(interp, piPtr, precTable[operator], valuePtr);
	    if (result != TCL_OK) {
		goto done;
	    }
	    gotOp = TRUE;
	    /* Process unary operators. */
	    switch (operator) {
	    case UNARY_MINUS:
		for(i = 0; i < vPtr->length; i++) {
		    if (!FINITE(vPtr->valueArr[i])) {
			continue;
		    }
		    vPtr->valueArr[i] = -(vPtr->valueArr[i]);
		}
		break;

	    case NOT:
		for(i = 0; i < vPtr->length; i++) {
		    if (!FINITE(vPtr->valueArr[i])) {
			continue;
		    }
		    vPtr->valueArr[i] = (double)(!vPtr->valueArr[i]);
		}
		break;
	    default:
		Tcl_AppendResult(interp, "unknown operator", (char *)NULL);
		goto error;
	    }
	} else if (piPtr->token != VALUE) {
	    Tcl_AppendResult(interp, "missing operand", (char *)NULL);
	    goto error;
	}
    }
    if (!gotOp) {
	result = NextToken(interp, piPtr, &value2);
	if (result != TCL_OK) {
	    goto done;
	}
    }
    /*
     * Got the first operand.  Now fetch (operator, operand) pairs.
     */
    for (;;) {
	operator = piPtr->token;
	int length;

	values = NULL;
	value2.pv.next = value2.pv.buffer;
	if ((operator < MULT) || (operator >= UNARY_MINUS)) {
	    if ((operator == END) || (operator == CLOSE_PAREN) || 
		(operator == COMMA)) {
		result = TCL_OK;
		goto done;
	    } else {
		Tcl_AppendResult(interp, "bad operator", (char *)NULL);
		goto error;
	    }
	}
	if (precTable[operator] <= prec) {
	    result = TCL_OK;
	    goto done;
	}
	result = NextValue(interp, piPtr, precTable[operator], &value2);
	if (result != TCL_OK) {
	    goto done;
	}
	if ((piPtr->token < MULT) && (piPtr->token != VALUE) &&
	    (piPtr->token != END) && (piPtr->token != CLOSE_PAREN) &&
	    (piPtr->token != COMMA)) {
	    Tcl_AppendResult(interp, "unexpected token in expression",
		(char *)NULL);
	    goto error;
	}
	/*
	 * At this point we have two vectors and an operator.
	 */
	errno = 0;
	if (v2Ptr->length == 1) {
	    double scalar;

	    /*
	     * 2nd operand is a scalar.
	     */
	    scalar = v2Ptr->valueArr[0];
	    length = vPtr->length;
	    values = Blt_AssertMalloc(sizeof(double) * vPtr->length);
	    memcpy(values, vPtr->valueArr, sizeof(double) * vPtr->length);
	    switch (operator) {
	    case MULT:
		for(i = 0; i < vPtr->length; i++) {
		    if (!FINITE(values[i])) {
			continue;
		    }
		    values[i] *= scalar;
		    if ((!FINITE(values[i])) || (errno != 0)) {
			MathError(interp, values[i]);
			goto error;
		    }
		}
		break;

	    case DIVIDE:
		if (scalar == 0.0) {
		    Tcl_AppendResult(interp, "divide by zero", (char *)NULL);
		    goto error;
		}
		for(i = 0; i < vPtr->length; i++) {
		    if (FINITE(values[i])) {
			values[i] /= scalar;
			if ((!FINITE(values[i])) || (errno != 0)) {
			    MathError(interp, values[i]);
			    goto error;
			}
		    }
		}
		break;

	    case PLUS:
		for(i = 0; i < vPtr->length; i++) {
		    if (!FINITE(values[i])) {
			continue;
		    }
		    values[i] += scalar;
		    if ((!FINITE(values[i])) || (errno != 0)) {
			MathError(interp, values[i]);
			goto error;
		    }
		}
		break;

	    case MINUS:
		for(i = 0; i < vPtr->length; i++) {
		    if (!FINITE(values[i])) {
			continue;
		    }
		    values[i] -= scalar;
		    if ((!FINITE(values[i])) || (errno != 0)) {
			MathError(interp, values[i]);
			goto error;
		    }
		}
		break;

	    case EXPONENT:
		for(i = 0; i < vPtr->length; i++) {
		    if (!FINITE(values[i])) {
			continue;
		    }
		    values[i] = pow(values[i], scalar);
		    if ((!FINITE(values[i])) || (errno != 0)) {
			MathError(interp, values[i]);
			goto error;
		    }
		}
		break;

	    case MOD:
		for(i = 0; i < vPtr->length; i++) {
		    if (!FINITE(values[i])) {
			continue;
		    }
		    values[i] = Fmod(values[i], scalar);
		    if ((!FINITE(values[i])) || (errno != 0)) {
			MathError(interp, values[i]);
			goto error;
		    }
		}
		break;

	    case LESS:
		for(i = 0; i < vPtr->length; i++) {
		    if (!FINITE(values[i])) {
			continue;
		    }
		    values[i] = (double)(values[i] < scalar);
		}
		break;

	    case GREATER:
		for(i = 0; i < vPtr->length; i++) {
		    if (!FINITE(values[i])) {
			continue;
		    }
		    values[i] = (double)(values[i] > scalar);
		}
		break;

	    case LEQ:
		for(i = 0; i < vPtr->length; i++) {
		    if (!FINITE(values[i])) {
			continue;
		    }
		    values[i] = (double)(values[i] <= scalar);
		}
		break;

	    case GEQ:
		for(i = 0; i < vPtr->length; i++) {
		    if (!FINITE(values[i])) {
			continue;
		    }
		    values[i] = (double)(values[i] >= scalar);
		}
		break;

	    case EQUAL:
		for(i = 0; i < vPtr->length; i++) {
		    if (!FINITE(values[i])) {
			continue;
		    }
		    values[i] = (double)(values[i] == scalar);
		}
		break;

	    case NEQ:
		for(i = 0; i < vPtr->length; i++) {
		    if (!FINITE(values[i])) {
			continue;
		    }
		    values[i] = (double)(values[i] != scalar);
		}
		break;

	    case AND:
		for(i = 0; i < vPtr->length; i++) {
		    if (!FINITE(values[i])) {
			continue;
		    }
		    values[i] = (double)(values[i] && scalar);
		}
		break;

	    case OR:
		for(i = 0; i < vPtr->length; i++) {
		    if (!FINITE(values[i])) {
			continue;
		    }
		    values[i] = (double)(values[i] || scalar);
		}
		break;

	    case LEFT_SHIFT:
		{
		    int offset;

		    offset = (int)scalar % vPtr->length;
		    if (offset > 0) {
			double *hold;
			int j;

			hold = Blt_AssertMalloc(sizeof(double) * offset);
			for (i = 0; i < offset; i++) {
			    hold[i] = values[i];
			}
			for (i = offset, j = 0; i < vPtr->length; i++, j++) {
			    values[j] = values[i];
			}
			for (i = 0, j = vPtr->length - offset;
			     j < vPtr->length; i++, j++) {
			    values[j] = hold[i];
			}
			Blt_Free(hold);
		    }
		}
		break;

	    case RIGHT_SHIFT:
		{
		    int offset;

		    offset = (int)scalar % vPtr->length;
		    if (offset > 0) {
			double *hold;
			int j;
			
			hold = Blt_AssertMalloc(sizeof(double) * offset);
			for (i = vPtr->length - offset, j = 0; 
			     i < vPtr->length; i++, j++) {
			    hold[j] = values[i];
			}
			for (i = vPtr->length - offset - 1, 
				 j = vPtr->length - 1; i >= 0; i--, j--) {
			    values[j] = values[i];
			}
			for (i = 0; i < offset; i++) {
			    values[i] = hold[i];
			}
			Blt_Free(hold);
		    }
		}
		break;

	    default:
		Tcl_AppendResult(interp, "unknown operator in expression",
		    (char *)NULL);
		goto error;
	    }

	} else if (vPtr->length == 1) {
	    double scalar;

	    /*
	     * 1st operand is a scalar.
	     */
	    scalar = vPtr->valueArr[0];
	    length = v2Ptr->length;
	    values = Blt_AssertMalloc(sizeof(double) * length);
	    memcpy(values, v2Ptr->valueArr, sizeof(double) * length);
	    switch (operator) {
	    case MULT:
		for(i = 0; i < length; i++) {
		    if (!FINITE(values[i])) {
			continue;
		    }
		    values[i] *= scalar;
		    if (!FINITE(values[i])) {
			MathError(interp, values[i]);
			goto error;
		    }
		}
		break;

	    case PLUS:
		for(i = 0; i < length; i++) {
		    if (!FINITE(values[i])) {
			continue;
		    }
		    values[i] += scalar;
		    if (!FINITE(values[i])) {
			MathError(interp, values[i]);
			goto error;
		    }
		}
		break;

	    case DIVIDE:
		for(i = 0; i < length; i++) {
		    if (!FINITE(values[i])) {
			continue;
		    }
		    if (values[i] == 0.0) {
			Tcl_AppendResult(interp, "divide by zero", 
					 (char *)NULL);
			goto error;
		    }
		    values[i] = scalar / values[i];
		    if (!FINITE(values[i])) {
			MathError(interp, values[i]);
			goto error;
		    }
		}
		break;

	    case MINUS:
		for(i = 0; i < length; i++) {
		    if (!FINITE(values[i])) {
			continue;
		    }
		    values[i] = scalar - values[i];
		    if (!FINITE(values[i])) {
			MathError(interp, values[i]);
			goto error;
		    }
		}
		break;

	    case EXPONENT:
		for(i = 0; i < length; i++) {
		    if (!FINITE(values[i])) {
			continue;
		    }
		    values[i] = pow(scalar, values[i]);
		    if ((!FINITE(values[i])) || (errno != 0)) {
			MathError(interp, values[i]);
			goto error;
		    }
		}
		break;

	    case MOD:
		for(i = 0; i < length; i++) {
		    if (!FINITE(values[i])) {
			continue;
		    }
		    values[i] = Fmod(scalar, values[i]);
		    if ((!FINITE(values[i])) || (errno != 0)) {
			MathError(interp, values[i]);
			goto error;
		    }
		}
		break;

	    case LESS:
		for(i = 0; i < length; i++) {
		    if (!FINITE(values[i])) {
			continue;
		    }
		    values[i] = (double)(scalar < values[i]);
		}
		break;

	    case GREATER:
		for(i = 0; i < length; i++) {
		    if (!FINITE(values[i])) {
			continue;
		    }
		    values[i] = (double)(scalar > values[i]);
		}
		break;

	    case LEQ:
		for(i = 0; i < length; i++) {
		    if (!FINITE(values[i])) {
			continue;
		    }
		    values[i] = (double)(scalar >= values[i]);
		}
		break;

	    case GEQ:
		for(i = 0; i < length; i++) {
		    if (!FINITE(values[i])) {
			continue;
		    }
		    values[i] = (double)(scalar <= values[i]);
		}
		break;

	    case EQUAL:
		for(i = 0; i < length; i++) {
		    if (!FINITE(values[i])) {
			continue;
		    }
		    values[i] = (double)(values[i] == scalar);
		}
		break;

	    case NEQ:
		for(i = 0; i < length; i++) {
		    if (!FINITE(values[i])) {
			continue;
		    }
		    values[i] = (double)(values[i] != scalar);
		}
		break;

	    case AND:
		for(i = 0; i < length; i++) {
		    if (!FINITE(values[i])) {
			continue;
		    }
		    values[i] = (double)(values[i] && scalar);
		}
		break;

	    case OR:
		for(i = 0; i < length; i++) {
		    if (!FINITE(values[i])) {
			continue;
		    }
		    values[i] = (double)(values[i] || scalar);
		}
		break;

	    case LEFT_SHIFT:
	    case RIGHT_SHIFT:
		Tcl_AppendResult(interp, "second shift operand must be scalar",
		    (char *)NULL);
		goto error;

	    default:
		Tcl_AppendResult(interp, "unknown operator in expression",
		    (char *)NULL);
		goto error;
	    }
	} else {
	    double *opnd2;
	    /*
	     * Carry out the function of the specified operator.
	     */
	    if (vPtr->length != v2Ptr->length) {
		Tcl_AppendResult(interp, "vectors are different lengths",
		    (char *)NULL);
		goto error;
	    }
	    errno = 0;
	    opnd2 = v2Ptr->valueArr;
	    length = vPtr->length;
	    values = Blt_AssertMalloc(sizeof(double) * length);
	    memcpy(values, vPtr->valueArr, sizeof(double) * length);
	    switch (operator) {
	    case MULT:
		for (i = 0; i < length; i++) {
		    if ((!FINITE(values[i])) || (!FINITE(opnd2[i]))) {
			continue;
		    }
		    values[i] *= opnd2[i];
		    if (!FINITE(values[i])) {
			MathError(interp, values[i]);
			goto error;
		    }
		}
		break;

	    case DIVIDE:
		for (i = 0; i < length; i++) {
		    if ((!FINITE(values[i])) || (!FINITE(opnd2[i]))) {
			continue;
		    }
		    if (opnd2[i] == 0.0) {
			Tcl_AppendResult(interp,
				"can't divide by 0.0 vector point",
				(char *)NULL);
			goto error;
		    }
		    values[i] /= opnd2[i];
		    if (!FINITE(values[i])) {
			MathError(interp, values[i]);
			goto error;
		    }
		}
		break;

	    case PLUS:
		for (i = 0; i < length; i++) {
		    if ((!FINITE(values[i])) || (!FINITE(opnd2[i]))) {
			continue;
		    }
		    values[i] += opnd2[i];
		    if (!FINITE(values[i])) {
			MathError(interp, values[i]);
			goto error;
		    }
		}
		break;

	    case MINUS:
		for (i = 0; i < length; i++) {
		    if ((!FINITE(values[i])) || (!FINITE(opnd2[i]))) {
			continue;
		    }
		    values[i] -= opnd2[i];
		    if (!FINITE(values[i])) {
			MathError(interp, values[i]);
			goto error;
		    }
		}
		break;

	    case MOD:
		for (i = 0; i < length; i++) {
		    if ((!FINITE(values[i])) || (!FINITE(opnd2[i]))) {
			continue;
		    }
		    values[i] = Fmod(values[i], opnd2[i]);
		    if (!FINITE(values[i])) {
			MathError(interp, values[i]);
			goto error;
		    }
		}
		break;

	    case EXPONENT:
		for (i = 0; i < length; i++) {
		    if ((!FINITE(values[i])) || (!FINITE(opnd2[i]))) {
			continue;
		    }
		    values[i] = pow(values[i], opnd2[i]);
		    if (!FINITE(values[i])) {
			MathError(interp, values[i]);
			goto error;
		    }
		}
		break;

	    case LESS:
		for (i = 0; i < length; i++) {
		    if ((!FINITE(values[i])) || (!FINITE(opnd2[i]))) {
			continue;
		    }
		    values[i] = (double)(values[i] < opnd2[i]);
		}
		break;

	    case GREATER:
		for (i = 0; i < length; i++) {
		    if ((!FINITE(values[i])) || (!FINITE(opnd2[i]))) {
			continue;
		    }
		    values[i] = (double)(values[i] > opnd2[i]);
		}
		break;

	    case LEQ:
		for (i = 0; i < length; i++) {
		    if ((!FINITE(values[i])) || (!FINITE(opnd2[i]))) {
			continue;
		    }
		    values[i] = (double)(values[i] <= opnd2[i]);
		}
		break;

	    case GEQ:
		for (i = 0; i < length; i++) {
		    if ((!FINITE(values[i])) || (!FINITE(opnd2[i]))) {
			continue;
		    }
		    values[i] = (double)(values[i] >= opnd2[i]);
		}
		break;

	    case EQUAL:
		for (i = 0; i < length; i++) {
		    if ((!FINITE(values[i])) || (!FINITE(opnd2[i]))) {
			continue;
		    }
		    values[i] = (double)(values[i] == opnd2[i]);
		}
		break;

	    case NEQ:
		for (i = 0; i < length; i++) {
		    if ((!FINITE(values[i])) || (!FINITE(opnd2[i]))) {
			continue;
		    }
		    values[i] = (double)(values[i] != opnd2[i]);
		}
		break;

	    case AND:
		for (i = 0; i < length; i++) {
		    if ((!FINITE(values[i])) || (!FINITE(opnd2[i]))) {
			continue;
		    }
		    values[i] = (double)(values[i] && opnd2[i]);
		}
		break;

	    case OR:
		for (i = 0; i < length; i++) {
		    if ((!FINITE(values[i])) || (!FINITE(opnd2[i]))) {
			continue;
		    }
		    values[i] = (double)(values[i] || opnd2[i]);
		}
		break;

	    case LEFT_SHIFT:
	    case RIGHT_SHIFT:
		Tcl_AppendResult(interp, "second shift operand must be scalar",
		    (char *)NULL);
		goto error;

	    default:
		Tcl_AppendResult(interp, "unknown operator in expression",
		    (char *)NULL);
		goto error;
	    }
	}
	if (values != NULL) {
	    Blt_Vec_Reset(vPtr, values, length, length, TCL_DYNAMIC);
	}
    }
  done:
    if (value2.pv.buffer != value2.staticSpace) {
	Blt_Free(value2.pv.buffer);
    }
    Blt_Vec_Free(v2Ptr);
    return result;

  error:
    if (value2.pv.buffer != value2.staticSpace) {
	Blt_Free(value2.pv.buffer);
    }
    Blt_Vec_Free(v2Ptr);
    if (values != NULL) {
	Blt_Free(values);
    }
    return TCL_ERROR;
}

/*
 *---------------------------------------------------------------------------
 *
 * EvaluateExpression --
 *
 *	This procedure provides top-level functionality shared by
 *	procedures like Tcl_ExprInt, Tcl_ExprDouble, etc.
 *
 * Results:
 *	The result is a standard TCL return value.  If an error occurs then
 *	an error message is left in interp->result.  The value of the
 *	expression is returned in *valuePtr, in whatever form it ends up in
 *	(could be string or integer or double).  Caller may need to convert
 *	result.  Caller is also responsible for freeing string memory in
 *	*valuePtr, if any was allocated.
 *
 * Side effects:
 *	None.
 *
 *---------------------------------------------------------------------------
 */
static int
EvaluateExpression(
    Tcl_Interp *interp,			/* Context in which to evaluate the
					 * expression. */
    char *string,			/* Expression to evaluate. */
    Value *valuePtr)			/* Where to store result.  Should
					 * not be initialized by caller. */
{
    ParseInfo info;
    int result;

    errno = 0;
    info.expr = info.nextPtr = string;
    valuePtr->pv.buffer = valuePtr->pv.next = valuePtr->staticSpace;
    valuePtr->pv.end = valuePtr->pv.buffer + STATIC_STRING_SPACE - 1;
    valuePtr->pv.expandProc = Blt_ExpandParseValue;
    valuePtr->pv.clientData = NULL;

    result = NextValue(interp, &info, -1, valuePtr);
    if (result != TCL_OK) {
	return result;
    }
    if (info.token != END) {
	Tcl_AppendResult(interp, ": syntax error in expression \"",
	    string, "\"", (char *)NULL);
	return TCL_ERROR;
    }
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * Math Functions --
 *
 *	This page contains the procedures that implement all of the
 *	built-in math functions for expressions.
 *
 * Results:
 *	Each procedure returns TCL_OK if it succeeds and places result
 *	information at *resultPtr.  If it fails it returns TCL_ERROR and
 *	leaves an error message in interp->result.
 *
 * Side effects:
 *	None.
 *
 *---------------------------------------------------------------------------
 */
static int
PointFunc(
    ClientData clientData,		/* Contains address of procedure
					 * that takes one double argument
					 * and returns a double result. */
    Tcl_Interp *interp,
    Vector *vPtr)
{
    PointProc1 *procPtr = (PointProc1 *) clientData;
    int i;
    double *values;

    values = Blt_AssertMalloc(sizeof(double) * vPtr->length);
    memcpy(values, vPtr->valueArr, sizeof(double) * vPtr->length);

    for(i = 0; i < vPtr->length; i++) {
	if (!FINITE(values[i])) {
	    continue;			/* There is a hole in vector. */
	}
	values[i] = (*procPtr) (values[i]);
	if ((!FINITE(values[i])) || (errno != 0)) {
	    MathError(interp, values[i]);
	    Blt_Free(values);
	    return TCL_ERROR;
	}
    }
    Blt_Vec_Reset(vPtr, values, vPtr->length, vPtr->length, TCL_DYNAMIC);
    return TCL_OK;
}
/*
 *---------------------------------------------------------------------------
 *
 * Math Functions --
 *
 *	This page contains the procedures that implement all of the
 *	built-in math functions for expressions.
 *
 * Results:
 *	Each procedure returns TCL_OK if it succeeds and places result
 *	information at *resultPtr.  If it fails it returns TCL_ERROR and
 *	leaves an error message in interp->result.
 *
 * Side effects:
 *	None.
 *
 *---------------------------------------------------------------------------
 */
static int
PointNoArgsFunc(
    ClientData clientData,              /* Contains address of procedure
					 * that takes one double argument
					 * and returns a double result. */
    Tcl_Interp *interp,
    Vector *vPtr)
{
    PointProc0 *procPtr = (PointProc0 *) clientData;
    int i;
    double *values;

    values = Blt_AssertMalloc(sizeof(double) * vPtr->length);
    memcpy(values, vPtr->valueArr, sizeof(double) * vPtr->length);
    for(i = 0; i < vPtr->length; i++) {
	values[i] = (*procPtr) ();
    }
    Blt_Vec_Reset(vPtr, values, vPtr->length, vPtr->length, TCL_DYNAMIC);
    return TCL_OK;
}


static int
ScalarFunc(ClientData clientData, Tcl_Interp *interp, Vector *vPtr)
{
    double value;
    ScalarProc *procPtr = (ScalarProc *) clientData;

    errno = 0;
    value = (*procPtr) (vPtr);
    if ((errno != 0) || (!FINITE(value))) {
	MathError(interp, value);
	return TCL_ERROR;
    }
    if (Blt_Vec_ChangeLength(interp, vPtr, 1) != TCL_OK) {
	return TCL_ERROR;
    }
    vPtr->valueArr[0] = value;
    return TCL_OK;
}

/*ARGSUSED*/
static int
VectorFunc(ClientData clientData, Tcl_Interp *interp, Vector *vPtr)
{
    VectorProc *procPtr = (VectorProc *) clientData;

    return (*procPtr) (vPtr);
}


static MathFunction mathFunctions[] =
{
    {"abs",     PointFunc,      Fabs},
    {"acos",	PointFunc,      acos},
    {"adev",	ScalarFunc,     AvgDeviation},
    {"asin",	PointFunc,      asin},
    {"asinh",	PointFunc,      asinh},
    {"atan",	PointFunc,      atan},
    {"ceil",	PointFunc,      ceil},
    {"cos",	PointFunc,      cos},
    {"cosh",	PointFunc,      cosh},
    {"exp",	PointFunc,      exp},
    {"floor",	PointFunc,      floor},
    {"kurtosis",ScalarFunc,     Kurtosis},
    {"length",	ScalarFunc,     Length},
    {"log",	PointFunc,      log},
    {"log10",	PointFunc,      log10},
    {"max",	ScalarFunc,     Blt_VecMax},
    {"mean",	ScalarFunc,     Mean},
    {"median",	ScalarFunc,     Median},
    {"min",	ScalarFunc,     Blt_VecMin},
    {"nonempty",ScalarFunc,     Count},
    {"nonzero",	ScalarFunc,     Nonzeros},
    {"norm",	VectorFunc,     Norm},
    {"prod",	ScalarFunc,     Product},
    {"q1",	ScalarFunc,     Q1},
    {"q2",	ScalarFunc,     Mean},
    {"q3",	ScalarFunc,     Q3},
    {"random",	PointNoArgsFunc ,drand48},
    {"round",	PointFunc,      Round},
    {"sdev",	ScalarFunc,     StdDeviation},
    {"sin",	PointFunc,      sin},
    {"sinh",	PointFunc,      sinh},
    {"skew",	ScalarFunc,     Skew},
    {"sort",	VectorFunc,     Sort},
    {"sqrt",	PointFunc,      sqrt},
    {"sum",	ScalarFunc,     Sum},
    {"tan",	PointFunc,      tan},
    {"tanh",	PointFunc,      tanh},
    {"var",	ScalarFunc,     Variance},
    {(char *)NULL,},
};

void
Blt_Vec_InstallMathFunctions(Blt_HashTable *tablePtr)
{
    MathFunction *mathPtr;

    for (mathPtr = mathFunctions; mathPtr->name != NULL; mathPtr++) {
	Blt_HashEntry *hPtr;
	int isNew;

	hPtr = Blt_CreateHashEntry(tablePtr, mathPtr->name, &isNew);
	Blt_SetHashValue(hPtr, (ClientData)mathPtr);
    }
}

void
Blt_Vec_UninstallMathFunctions(Blt_HashTable *tablePtr)
{
    Blt_HashEntry *hPtr;
    Blt_HashSearch cursor;

    for (hPtr = Blt_FirstHashEntry(tablePtr, &cursor); hPtr != NULL; 
	hPtr = Blt_NextHashEntry(&cursor)) {
	MathFunction *mathPtr;

	mathPtr = Blt_GetHashValue(hPtr);
	if (mathPtr->name == NULL) {
	    Blt_Free(mathPtr);
	}
    }
}


static void
InstallIndexProc(
    Blt_HashTable *tablePtr,
    const char *string,
    Blt_VectorIndexProc *procPtr)	/* Pointer to function to be called
					 * when the vector finds the named
					 * index.  If NULL, this indicates
					 * to remove the index from the
					 * table. */
{
    Blt_HashEntry *hPtr;
    int dummy;

    hPtr = Blt_CreateHashEntry(tablePtr, string, &dummy);
    if (procPtr == NULL) {
	Blt_DeleteHashEntry(tablePtr, hPtr);
    } else {
	Blt_SetHashValue(hPtr, (ClientData)procPtr);
    }
}

void
Blt_Vec_InstallSpecialIndices(Blt_HashTable *tablePtr)
{
    InstallIndexProc(tablePtr, "min",  Blt_VecMin);
    InstallIndexProc(tablePtr, "max",  Blt_VecMax);
    InstallIndexProc(tablePtr, "mean", Mean);
    InstallIndexProc(tablePtr, "sum",  Sum);
    InstallIndexProc(tablePtr, "prod", Product);
}


/*
 *---------------------------------------------------------------------------
 *
 * Blt_ExprVector --
 *
 *	Evaluates an vector expression and returns its value(s).
 *
 * Results:
 *	Each of the procedures below returns a standard TCL result.  If an
 *	error occurs then an error message is left in interp->result.
 *	Otherwise the value of the expression, in the appropriate form, is
 *	stored at *resultPtr.  If the expression had a result that was
 *	incompatible with the desired form then an error is returned.
 *
 * Side effects:
 *	None.
 *
 *---------------------------------------------------------------------------
 */
int
Blt_ExprVector(
    Tcl_Interp *interp,			/* Context in which to evaluate the
					 * expression. */
    char *string,			/* Expression to evaluate. */
    Blt_Vector *vector)			/* Where to store result. */
{
    VectorCmdInterpData *dataPtr;       /* Interpreter-specific data. */
    Vector *vPtr = (Vector *)vector;
    Value value;

    dataPtr = (vector != NULL) 
	? vPtr->dataPtr : Blt_Vec_GetInterpData(interp);
    value.vPtr = Blt_Vec_New(dataPtr);
    if (EvaluateExpression(interp, string, &value) != TCL_OK) {
	Blt_Vec_Free(value.vPtr);
	return TCL_ERROR;
    }
    if (vPtr != NULL) {
	Blt_Vec_Duplicate(vPtr, value.vPtr);
    } else {
	Tcl_Obj *listObjPtr;
	int i;

	/* No result vector.  Put values in interp->result.  */
	listObjPtr = Tcl_NewListObj(0, (Tcl_Obj **) NULL);
	for (i = 0; i < value.vPtr->length; i++) {
	    Tcl_Obj *objPtr;

	    objPtr = Tcl_NewDoubleObj(value.vPtr->valueArr[i]);
	    Tcl_ListObjAppendElement(interp, listObjPtr, objPtr);
	}
	Tcl_SetObjResult(interp, listObjPtr);
    }
    Blt_Vec_Free(value.vPtr);
    return TCL_OK;
}

