/* -*- mode: c; c-basic-offset: 4; indent-tabs-mode: nil -*- */

/* 
 * bltHash.c --
 *
 * This module implements an in-memory hash table for the BLT toolkit.
 * Built upon the TCL hash table, it adds pool allocation 64-bit address
 * handling, improved array hash function.
 *
 *	Copyright 2001 George A Howlett.
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
 *
 * Both the MIX32 and MIX64 routine are from Bob Jenkins.
 *
 *	Bob Jenkins, 1996.  hash.c.  Public Domain.
 *	Bob Jenkins, 1997.  lookup8.c.  Public Domain.
 *
 * The hash table implementation is base upon the one in the TCL distribution.
 *
 *	Copyright (c) 1991-1993 The Regents of the University of
 *	California.
 *
 *	Copyright (c) 1994 Sun Microsystems, Inc.
 *
 *	See the file "license.terms" for information on usage and
 *	redistribution of this file, and for a DISCLAIMER OF ALL
 *	WARRANTIES.
 *
 */

#define BUILD_BLT_TCL_PROCS 1
#include <bltInt.h>

#include <stdio.h>
#include <string.h>
/* The following header is required for LP64 compilation */
#include <stdlib.h>

#include "bltAlloc.h"
#include "bltHash.h"

/*
 * When there are this many entries per bucket, on average, rebuild the
 * hash table to make it larger.
 */

#define REBUILD_MULTIPLIER	3

#if (SIZEOF_VOID_P == 8)
#define RANDOM_INDEX		HashOneWord
#define DOWNSHIFT_START		62
static Blt_Hash HashOneWord(Blt_HashTable *tablePtr, const void *key);
#else 

/*
 * The following macro takes a preliminary integer hash value and produces
 * an index into a hash tables bucket list.  The idea is to make it so that
 * preliminary values that are arbitrarily similar will end up in different
 * buckets.  The hash function was taken from a random-number generator.
 */
#define RANDOM_INDEX(tablePtr, i) \
    (((((long) (i))*1103515245) >> (tablePtr)->downShift) & (tablePtr)->mask)
#define DOWNSHIFT_START	28
#endif

/*
 * Procedure prototypes for static procedures in this file:
 */
static Blt_Hash HashArray(const void *key, size_t length);
static Blt_HashEntry *ArrayFind(Blt_HashTable *tablePtr, const void *key);
static Blt_HashEntry *ArrayCreate(Blt_HashTable *tablePtr, const void *key, 
	int *isNewPtr);
static Blt_HashEntry *BogusFind(Blt_HashTable *tablePtr, const void *key);
static Blt_HashEntry *BogusCreate(Blt_HashTable *tablePtr, const void *key, 
	int *isNewPtr);
static Blt_Hash HashString(const char *string);
static void RebuildTable(Blt_HashTable *tablePtr);
static Blt_HashEntry *StringFind(Blt_HashTable *tablePtr, const void *key);
static Blt_HashEntry *StringCreate(Blt_HashTable *tablePtr, const void *key, 
	int *isNewPtr);
static Blt_HashEntry *OneWordFind(Blt_HashTable *tablePtr, const void *key);
static Blt_HashEntry *OneWordCreate(Blt_HashTable *tablePtr, const void *key, 
	int *isNewPtr);

/*
 *---------------------------------------------------------------------------
 *
 * HashString --
 *
 *	Compute a one-word summary of a text string, which can be used to
 *	generate a hash index.
 *
 * Results:
 *	The return value is a one-word summary of the information in
 *	string.
 *
 * Side effects:
 *	None.
 *
 *---------------------------------------------------------------------------
 */
static Blt_Hash
HashString(const char *string)		/* String from which to compute
					 * hash value. */
{
    Blt_Hash result;
    Blt_Hash c;

    /*
     * I tried a zillion different hash functions and asked many other
     * people for advice.  Many people had their own favorite functions,
     * all different, but no-one had much idea why they were good ones.  I
     * chose the one below (multiply by 9 and add new character) because of
     * the following reasons:
     *
     * 1. Multiplying by 10 is perfect for keys that are decimal strings,
     *    and multiplying by 9 is just about as good.
     * 2. Times-9 is (shift-left-3) plus (old).  This means that each
     *    character's bits hang around in the low-order bits of the hash
     *    value for ever, plus they spread fairly rapidly up to the
     *    high-order bits to fill out the hash value.  This seems to work
     *    well both for decimal and non-decimal strings.
     */

    result = 0;
    while ((c = *string++) != 0) {
	result += (result << 3) + c;
    }
    return (Blt_Hash)result;
}

/*
 *---------------------------------------------------------------------------
 *
 * StringFind --
 *
 *	Given a hash table with string keys, and a string key, find the
 *	entry with a matching key.
 *
 * Results:
 *	The return value is a token for the matching entry in the hash
 *	table, or NULL if there was no matching entry.
 *
 * Side effects:
 *	None.
 *
 *---------------------------------------------------------------------------
 */
static Blt_HashEntry *
StringFind(
    Blt_HashTable *tablePtr,		/* Table in which to lookup
					   entry. */
    const void *key)			/* Key to find matching entry. */
{
    Blt_Hash hval;
    Blt_HashEntry *hPtr;
    size_t hindex;

    hval = HashString((const char *)key);
    hindex = hval & tablePtr->mask;

    /*
     * Search all of the entries in the appropriate bucket.
     */
    for (hPtr = tablePtr->buckets[hindex]; hPtr != NULL; hPtr = hPtr->nextPtr) {
	if (hPtr->hval == hval) {
	    const char *p1, *p2;

	    for (p1 = key, p2 = hPtr->key.string; ; p1++, p2++) {
		if (*p1 != *p2) {
		    break;
		}
		if (*p1 == '\0') {
		    return hPtr;
		}
	    }
	}
    }
    return NULL;
}

/*
 *---------------------------------------------------------------------------
 *
 * StringCreate --
 *
 *	Given a hash table with string keys, and a string key, find the
 *	entry with a matching key.  If there is no matching entry, then
 *	create a new entry that does match.
 *
 * Results:
 *	The return value is a pointer to the matching entry.  If this is a
 *	newly-created entry, then *isNewPtr will be set to a non-zero
 *	value; otherwise *isNewPtr will be set to 0.  If this is a new
 *	entry the value stored in the entry will initially be 0.
 *
 * Side effects:
 *	A new entry may be added to the hash table.
 *
 *---------------------------------------------------------------------------
 */
static Blt_HashEntry *
StringCreate(
    Blt_HashTable *tablePtr,		/* Table in which to lookup
					   entry. */
    const void *key,			/* Key to use to find or create
					 * matching entry. */
    int *isNewPtr)			/* Store info here telling whether
					 * a new entry was created. */
{
    Blt_Hash hval;
    Blt_HashEntry **bucketPtr;
    Blt_HashEntry *hPtr;
    size_t size, hindex;

    hval = HashString(key);
    hindex = hval & tablePtr->mask;

    /* Search all of the entries in this bucket. */

    for (hPtr = tablePtr->buckets[hindex]; hPtr != NULL;
	    hPtr = hPtr->nextPtr) {
	if (hPtr->hval == hval) {
	    const char *p1, *p2;

	    for (p1 = key, p2 = hPtr->key.string; ; p1++, p2++) {
		if (*p1 != *p2) {
		    break;
		}
		if (*p1 == '\0') {
		    *isNewPtr = FALSE;
		    return hPtr;
		}
	    }
	}
    }

    /* Entry not found.  Add a new one to the bucket. */

    *isNewPtr = TRUE;
    size = sizeof(Blt_HashEntry) + strlen(key) - sizeof(Blt_HashKey) + 1;
    if (tablePtr->hPool != NULL) {
	hPtr = Blt_Pool_AllocItem(tablePtr->hPool, size);
    } else {
	hPtr = Blt_AssertMalloc(size);
    }
    bucketPtr = tablePtr->buckets + hindex;
    hPtr->nextPtr = *bucketPtr;
    hPtr->hval = hval;
    hPtr->clientData = 0;
    strcpy(hPtr->key.string, key);
    *bucketPtr = hPtr;
    tablePtr->numEntries++;

    /*
     * If the table has exceeded a decent size, rebuild it with many more
     * buckets.
     */

    if (tablePtr->numEntries >= tablePtr->rebuildSize) {
	RebuildTable(tablePtr);
    }
    return hPtr;
}

#if (SIZEOF_VOID_P == 8)
/*
 *---------------------------------------------------------------------------
 *
 * HashOneWord --
 *
 *	Compute a one-word hash value of a 64-bit word, which then can be
 *	used to generate a hash index.
 *
 *	From Knuth, it's a multiplicative hash.  Multiplies an unsigned
 *	64-bit value with the golden ratio (sqrt(5) - 1) / 2.  The
 *	downshift value is 64 - n, where n is the log2 of the size of the
 *	hash table.
 *		
 * Results:
 *	The return value is a one-word summary of the information in 64 bit
 *	word.
 *
 * Side effects:
 *	None.
 *
 *---------------------------------------------------------------------------
 */
static Blt_Hash
HashOneWord(
    Blt_HashTable *tablePtr,
    const void *key)
{
    uint64_t a0, a1;
    uint64_t y0, y1;
    uint64_t y2, y3; 
    uint64_t p1, p2;
    uint64_t result;
    /* Compute key * GOLDEN_RATIO in 128-bit arithmetic */
    a0 = (uint64_t)key & 0x00000000FFFFFFFF; 
    a1 = (uint64_t)key >> 32;
    
    y0 = a0 * 0x000000007f4a7c13;
    y1 = a0 * 0x000000009e3779b9; 
    y2 = a1 * 0x000000007f4a7c13;
    y3 = a1 * 0x000000009e3779b9; 
    y1 += y0 >> 32;			/* Can't carry */ 
    y1 += y2;				/* Might carry */
    if (y1 < y2) {
	y3 += (1LL << 32);		/* Propagate */ 
    }

    /* 128-bit product: p1 = loword, p2 = hiword */
    p1 = ((y1 & 0x00000000FFFFFFFF) << 32) + (y0 & 0x00000000FFFFFFFF);
    p2 = y3 + (y1 >> 32);
    
    /* Left shift the value downward by the size of the table */
    if (tablePtr->downShift > 0) { 
	if (tablePtr->downShift < 64) { 
	    result = ((p2 << (64 - tablePtr->downShift)) | 
		      (p1 >> (tablePtr->downShift & 63))); 
	} else { 
	    result = p2 >> (tablePtr->downShift & 63); 
	} 
    } else { 
	result = p1;
    } 
    /* Finally mask off the high bits */
    return (Blt_Hash)(result & tablePtr->mask);
}

#endif /* SIZEOF_VOID_P == 8 */

/*
 *---------------------------------------------------------------------------
 *
 * OneWordFind --
 *
 *	Given a hash table with one-word keys, and a one-word key, find the
 *	entry with a matching key.
 *
 * Results:
 *	The return value is a token for the matching entry in the hash
 *	table, or NULL if there was no matching entry.
 *
 * Side effects:
 *	None.
 *
 *---------------------------------------------------------------------------
 */
static Blt_HashEntry *
OneWordFind(
    Blt_HashTable *tablePtr,		/* Table where to lookup entry. */
    const void *key)			/* Key that we're searching for. */
{
    Blt_HashEntry *hPtr;
    size_t hindex;

    hindex = RANDOM_INDEX(tablePtr, key);

    /* Search all of the entries in the appropriate bucket. */
    for (hPtr = tablePtr->buckets[hindex]; hPtr != NULL;
	    hPtr = hPtr->nextPtr) {
	if (hPtr->key.oneWordValue == key) {
	    return hPtr;
	}
    }
    return NULL;
}

/*
 *---------------------------------------------------------------------------
 *
 * OneWordCreate --
 *
 *	Given a hash table with one-word keys, and a one-word key, find the
 *	entry with a matching key.  If there is no matching entry, then
 *	create a new entry that does match.
 *
 * Results:
 *	The return value is a pointer to the matching entry.  If this is a
 *	newly-created entry, then *isNewPtr will be set to a non-zero
 *	value; otherwise *isNewPtr will be set to 0.  If this is a new
 *	entry the value stored in the entry will initially be 0.
 *
 * Side effects:
 *	A new entry may be added to the hash table.
 *
 *---------------------------------------------------------------------------
 */
static Blt_HashEntry *
OneWordCreate(
    Blt_HashTable *tablePtr,		/* Table in which to lookup
					   entry. */
    const void *key,			/* Key to use to find or create
					 * matching entry. */
    int *isNewPtr)			/* Store info here telling whether
					 * a new entry was created. */
{
    Blt_HashEntry **bucketPtr;
    Blt_HashEntry *hPtr;
    size_t hindex;

    hindex = RANDOM_INDEX(tablePtr, key);

    /* Search all of the entries in this bucket. */

    for (hPtr = tablePtr->buckets[hindex]; hPtr != NULL; hPtr = hPtr->nextPtr) {
	if (hPtr->key.oneWordValue == key) {
	    *isNewPtr = FALSE;
	    return hPtr;
	}
    }

    /* Entry not found.  Add a new one to the bucket. */

    *isNewPtr = TRUE;
    if (tablePtr->hPool != NULL) {
	hPtr = Blt_Pool_AllocItem(tablePtr->hPool, sizeof(Blt_HashEntry));
    } else {
	hPtr = Blt_AssertMalloc(sizeof(Blt_HashEntry));
    }	
    bucketPtr = tablePtr->buckets + hindex;
    hPtr->nextPtr = *bucketPtr;
    hPtr->hval = (Blt_Hash)key;
    hPtr->clientData = 0;
    hPtr->key.oneWordValue = (void *)key; /* const XXXX */
    *bucketPtr = hPtr;
    tablePtr->numEntries++;

    /*
     * If the table has exceeded a decent size, rebuild it with many more
     * buckets.
     */

    if (tablePtr->numEntries >= tablePtr->rebuildSize) {
	RebuildTable(tablePtr);
    }
    return hPtr;
}


#if (SIZEOF_VOID_P == 4)
/*
 *---------------------------------------------------------------------------
 *
 * MIX32 -- 
 *
 *      Bob Jenkins, 1996.  Public Domain.
 *
 *	Mix 3 32/64-bit values reversibly.  For every delta with one or two
 *	bit set, and the deltas of all three high bits or all three low
 *	bits, whether the original value of a,b,c is almost all zero or is
 *	uniformly distributed, If mix() is run forward or backward, at
 *	least 32 bits in a,b,c have at least 1/4 probability of changing.
 *	* If mix() is run forward, every bit of c will change between 1/3
 *	and 2/3 of the time.  (Well, 22/100 and 78/100 for some 2-bit
 *	deltas.)  mix() was built out of 36 single-cycle latency
 *	instructions in a structure that could supported 2x parallelism,
 *	like so:
 *
 *		a -= b; 
 *		a -= c; x = (c>>13);
 *		b -= c; a ^= x;
 *		b -= a; x = (a<<8);
 *		c -= a; b ^= x;
 *		c -= b; x = (b>>13);
 *		...
 *
 *	 Unfortunately, superscalar Pentiums and Sparcs can't take
 *	 advantage of that parallelism.  They've also turned some of those
 *	 single-cycle latency instructions into multi-cycle latency
 *	 instructions.  Still, this is the fastest good hash I could find.
 *	 There were about 2^^68 to choose from.  I only looked at a billion
 *	 or so.
 *
 * -------------------------------------------------------------------------- 
 */
#define MIX32(a,b,c) \
	a -= b, a -= c, a ^= (c >> 13), \
	b -= c, b -= a, b ^= (a <<  8), \
	c -= a, c -= b, c ^= (b >> 13), \
	a -= b, a -= c, a ^= (c >> 12), \
	b -= c, b -= a, b ^= (a << 16), \
	c -= a, c -= b, c ^= (b >>  5), \
	a -= b, a -= c, a ^= (c >>  3), \
	b -= c, b -= a, b ^= (a << 10), \
	c -= a, c -= b, c ^= (b >> 15)

#define GOLDEN_RATIO32	0x9e3779b9	/* An arbitrary value */

/*
 *---------------------------------------------------------------------------
 *
 * HashArray --
 *
 *      Bob Jenkins, 1996.  Public Domain.
 *
 *	This works on all machines.  Length has to be measured in unsigned
 *	longs instead of bytes.  It requires that
 *
 *	  o The key be an array of unsigned ints.
 *	  o All your machines have the same endianness
 *	  o The length be the number of unsigned ints in the key.
 *
 *---------------------------------------------------------------------------
 */
static Blt_Hash
HashArray(const void *key, size_t length)
{
    uint32_t a, b, c, len;
    uint32_t *arrayPtr = (uint32_t *)key;
    /* Set up the internal state */
    len = length;
    a = b = GOLDEN_RATIO32;		/* An arbitrary value */
    c = 0;				/* Previous hash value */

    while (len >= 3) {			/* Handle most of the key */
	a += arrayPtr[0];
	b += arrayPtr[1];
	c += arrayPtr[2];
	MIX32(a, b, c);
	arrayPtr += 3; len -= 3;
    }
    c += length;		
    /* And now the last 2 words */
    /* Note that all the case statements fall through */
    switch(len) {
	/* c is reserved for the length */
    case 2 : b += arrayPtr[1];
    case 1 : a += arrayPtr[0];
	/* case 0: nothing left to add */
    }
    MIX32(a, b, c);
    return (Blt_Hash)c;
}
#endif /* SIZEOF_VOID_P == 4 */

#if (SIZEOF_VOID_P == 8)

/* 
 *---------------------------------------------------------------------------
 *
 * MIX64 --
 *
 *	Bob Jenkins, January 4 1997, Public Domain.  You can use this free
 *	for any purpose.  It has no warranty.
 *
 *	Returns a 64-bit value.  Every bit of the key affects every bit of
 *	the return value.  No funnels.  Every 1-bit and 2-bit delta
 *	achieves avalanche.  About 41+5len instructions.
 *
 *	The best hash table sizes are powers of 2.  There is no need to do
 *	mod a prime (mod is sooo slow!).  If you need less than 64 bits,
 *	use a bitmask.  For example, if you need only 10 bits, do h = (h &
 *	hashmask(10)); In which case, the hash table should have
 *	hashsize(10) elements.
 *
 *	By Bob Jenkins, Jan 4 1997.  bob_jenkins@burtleburtle.net.  You may
 *	use this code any way you wish, private, educational, or
 *	commercial, as long as this whole comment accompanies it.
 * 
 *	See http://burtleburtle.net/bob/hash/evahash.html Use for hash
 *	table lookup, or anything where one collision in 2^^64 * is
 *	acceptable.  Do NOT use for cryptographic purposes.
 *
 *---------------------------------------------------------------------------
 */

#define MIX64(a,b,c) \
	a -= b, a -= c, a ^= (c >> 43), \
	b -= c, b -= a, b ^= (a <<  9), \
	c -= a, c -= b, c ^= (b >>  8), \
	a -= b, a -= c, a ^= (c >> 38), \
	b -= c, b -= a, b ^= (a << 23), \
	c -= a, c -= b, c ^= (b >>  5), \
	a -= b, a -= c, a ^= (c >> 35), \
	b -= c, b -= a, b ^= (a << 49), \
	c -= a, c -= b, c ^= (b >> 11), \
	a -= b, a -= c, a ^= (c >> 12), \
	b -= c, b -= a, b ^= (a << 18), \
	c -= a, c -= b, c ^= (b >> 22)

#define GOLDEN_RATIO64	0x9e3779b97f4a7c13LL

/*
 *---------------------------------------------------------------------------
 *
 * HashArray --
 *
 *	Bob Jenkins, January 4 1997, Public Domain.  You can use this free
 *	for any purpose.  It has no warranty.
 *
 *	This works on all machines.  The length has to be measured in 64
 *	bit words, instead of bytes.  It requires that
 *
 *	  o The key be an array of 64 bit words (unsigned longs).
 *	  o All your machines have the same endianness.
 *	  o The length be the number of 64 bit words in the key.
 *
 *---------------------------------------------------------------------------
 */
static Blt_Hash
HashArray(const void *key, size_t length)
{
    uint64_t a, b, c, len;
    uint32_t *ip = (uint32_t *)key;

#ifdef WORDS_BIGENDIAN
#define PACK(a,b)	((uint64_t)(b) | ((uint64_t)(a) << 32))
#else
#define PACK(a,b)	((uint64_t)(a) | ((uint64_t)(b) << 32))
#endif
    /* Set up the internal state */
    len = length;			/* Length is the number of 64-bit
					 * words. */
    a = b = GOLDEN_RATIO64;		/* An arbitrary value */
    c = 0;				/* Previous hash value */

    while (len >= 6) {			/* Handle most of the key */
	a += PACK(ip[0], ip[1]);
	b += PACK(ip[2], ip[3]);
	c += PACK(ip[4], ip[5]);
	MIX64(a,b,c);
	ip += 6; len -= 6;
    }
    c += length;		
    /* And now the last 2 words */
    /* Note that all the case statements fall through */
    switch(len) {
	/* c is reserved for the length */
    case 5 : 
    case 4 : 
	a += PACK(ip[0], ip[1]);
	b += PACK(ip[2], ip[3]);
	ip += 4; len -= 4;
	break;
    case 3 : 
    case 2 : 
	a += PACK(ip[0], ip[1]);
	ip += 2; len -= 2;
 /* case 0: nothing left to add */
    }
    if (len > 0) {		
	b += ip[0];
    }
    MIX64(a,b,c);
    return (Blt_Hash)c;
}
#endif /* SIZEOF_VOID_P == 8 */

/*
 *---------------------------------------------------------------------------
 *
 * ArrayFind --
 *
 *	Given a hash table with array-of-int keys, and a key, find the
 *	entry with a matching key.
 *
 * Results:
 *	The return value is a token for the matching entry in the hash
 *	table, or NULL if there was no matching entry.
 *
 * Side effects:
 *	None.
 *
 *---------------------------------------------------------------------------
 */
static Blt_HashEntry *
ArrayFind(
    Blt_HashTable *tablePtr,		/* Table where to lookup entry. */
    const void *key)			/* Key to find matching entry. */
{
    Blt_Hash hval;
    Blt_HashEntry *hPtr;
    size_t hindex;

    hval = HashArray(key, tablePtr->keyType);
    hindex = hval & tablePtr->mask;

    /* Search all of the entries in the appropriate bucket. */
    for (hPtr = tablePtr->buckets[hindex]; hPtr != NULL;
	    hPtr = hPtr->nextPtr) {
	if (hPtr->hval == hval) {
	    uint32_t *ip1, *ip2;
	    size_t count;

	    for (ip1 = (uint32_t *)key, ip2 = (uint32_t *)hPtr->key.words,
		     count = tablePtr->keyType; ; count--, ip1++, ip2++) {
		if (count == 0) {
		    return hPtr;
		}
		if (*ip1 != *ip2) {
		    break;
		}
	    }
	}
    }
    return NULL;
}

/*
 *---------------------------------------------------------------------------
 *
 * ArrayCreate --
 *
 *	Given a hash table with one-word keys, and a one-word key, find the
 *	entry with a matching key.  If there is no matching entry, then
 *	create a new entry that does match.
 *
 * Results:
 *	The return value is a pointer to the matching entry.  If this is a
 *	newly-created entry, then *isNewPtr will be set to a non-zero
 *	value; otherwise *isNewPtr will be set to 0.  If this is a new
 *	entry the value stored in the entry will initially be 0.
 *
 * Side effects:
 *	A new entry may be added to the hash table.
 *
 *---------------------------------------------------------------------------
 */
static Blt_HashEntry *
ArrayCreate(
    Blt_HashTable *tablePtr,		/* Table where to lookup entry. */
    const void *key,			/* Key to find or create matching
					 * entry. */
    int *isNewPtr)			/* Store info here telling whether
					 * a new entry was created. */
{
    Blt_Hash hval;
    Blt_HashEntry **bucketPtr;
    size_t count;
    Blt_HashEntry *hPtr;
    uint32_t *ip1, *ip2;
    size_t size, hindex;

    hval = HashArray(key, tablePtr->keyType);
    hindex = hval & tablePtr->mask;

    /* Search all of the entries in the appropriate bucket. */

    for (hPtr = tablePtr->buckets[hindex]; hPtr != NULL;
	    hPtr = hPtr->nextPtr) {
	if (hPtr->hval == hval) {
	    for (ip1 = (uint32_t *)key, ip2 = (uint32_t *)hPtr->key.words,
		     count = tablePtr->keyType; ; count--, ip1++, ip2++) {
		if (count == 0) {
		    *isNewPtr = FALSE;
		    return hPtr;
		}
		if (*ip1 != *ip2) {
		    break;
		}
	    }
	}
    }

    /* Entry not found.  Add a new one to the bucket. */

    *isNewPtr = TRUE;
    /* We assume here that the size of the key is at least 2 words */
    size = sizeof(Blt_HashEntry) + tablePtr->keyType * sizeof(uint32_t) - 
	sizeof(Blt_HashKey);
    if (tablePtr->hPool != NULL) {
	hPtr = Blt_Pool_AllocItem(tablePtr->hPool, size);
    } else {
	hPtr = Blt_AssertMalloc(size);
    }
    bucketPtr = tablePtr->buckets + hindex;
    hPtr->nextPtr = *bucketPtr;
    hPtr->hval = hval;
    hPtr->clientData = 0;
    count = tablePtr->keyType;
    for (ip1 = (uint32_t *)key, ip2 = (uint32_t *)hPtr->key.words; 
	 count > 0; count--, ip1++, ip2++) {
	*ip2 = *ip1;
    }
    *bucketPtr = hPtr;
    tablePtr->numEntries++;

    /*
     * If the table has exceeded a decent size, rebuild it with many more
     * buckets.
     */
    if (tablePtr->numEntries >= tablePtr->rebuildSize) {
	RebuildTable(tablePtr);
    }
    return hPtr;
}

/*
 *---------------------------------------------------------------------------
 *
 * BogusFind --
 *
 *	This procedure is invoked when an Blt_FindHashEntry is called on a
 *	table that has been deleted.
 *
 * Results:
 *	If panic returns (which it shouldn't) this procedure returns NULL.
 *
 * Side effects:
 *	Generates a panic.
 *
 *---------------------------------------------------------------------------
 */
/* ARGSUSED */
static Blt_HashEntry *
BogusFind(
    Blt_HashTable *tablePtr,		/* Not used. */
    const void *key)			/* Not used.*/
{
    Blt_Panic("called Blt_FindHashEntry on deleted table");
    return NULL;
}

/*
 *---------------------------------------------------------------------------
 *
 * BogusCreate --
 *
 *	This procedure is invoked when an Blt_CreateHashEntry is called on
 *	a table that has been deleted.
 *
 * Results:
 *	If panic returns (which it shouldn't) this procedure returns NULL.
 *
 * Side effects:
 *	Generates a panic.
 *
 *---------------------------------------------------------------------------
 */
/* ARGSUSED */
static Blt_HashEntry *
BogusCreate(
    Blt_HashTable *tablePtr,		/* Not used. */
    const void *key,			/* Not used. */
    int *isNewPtr)			/* Not used. */
{
    Blt_Panic("called Blt_CreateHashEntry on deleted table");
    return NULL;
}

/*
 *---------------------------------------------------------------------------
 *
 * RebuildTable --
 *
 *	This procedure is invoked when the ratio of entries to hash buckets
 *	becomes too large.  It creates a new table with a larger bucket
 *	array and moves all of the entries into the new table.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Memory gets reallocated and entries get re-hashed to new buckets.
 *
 *---------------------------------------------------------------------------
 */
static void
RebuildTable(Blt_HashTable *tablePtr)	/* Table to enlarge. */
{
    Blt_HashEntry **oldBuckets;
    int oldNumBuckets;

    oldBuckets = tablePtr->buckets;
    oldNumBuckets = tablePtr->numBuckets;
    /*
     * Allocate and initialize the new bucket array, and set up hashing
     * constants for new array size.
     */
    tablePtr->numBuckets <<= 2;
    tablePtr->buckets = Blt_AssertCalloc(tablePtr->numBuckets, 
		sizeof(Blt_HashEntry *));
    tablePtr->rebuildSize <<= 2;
    tablePtr->downShift -= 2;
    tablePtr->mask = tablePtr->numBuckets - 1;

    /*
     * Move all of the existing entries into the new bucket array, based on
     * their hash values.
     */
    if (tablePtr->keyType == BLT_ONE_WORD_KEYS) {
	Blt_HashEntry **hpp, **hend;

	/* 
	 * BLT_ONE_WORD_KEYS are handled slightly differently because they
	 * use the current table size (number of buckets) to distribute the
	 * entries.
	 */
	for (hpp = oldBuckets, hend = hpp + oldNumBuckets; hpp < hend; hpp++) {
	    Blt_HashEntry *hPtr, *nextPtr;

	    for (hPtr = *hpp; hPtr != NULL; hPtr = nextPtr) {
		Blt_HashEntry **bucketPtr;
		size_t hindex;

		nextPtr = hPtr->nextPtr;
		hindex = RANDOM_INDEX(tablePtr, hPtr->key.oneWordValue);
		bucketPtr = tablePtr->buckets + hindex;
		hPtr->nextPtr = *bucketPtr;
		*bucketPtr = hPtr;
	    }
	}
    } else {
	Blt_HashEntry **hpp, **hend;

	for (hpp = oldBuckets, hend = hpp + oldNumBuckets; hpp < hend; hpp++) {
	    Blt_HashEntry *hPtr, *nextPtr;

	    for (hPtr = *hpp; hPtr != NULL; hPtr = nextPtr) {
		Blt_HashEntry **bucketPtr;
		size_t hindex;

		nextPtr = hPtr->nextPtr;
		hindex = hPtr->hval & tablePtr->mask;
		bucketPtr = tablePtr->buckets + hindex;
		hPtr->nextPtr = *bucketPtr;
		*bucketPtr = hPtr;
	    }
	}
    }

    /*
     * Free up the old bucket array, if it was dynamically allocated.
     */
    if (oldBuckets != tablePtr->staticBuckets) {
	Blt_Free(oldBuckets);
    }
}


/* Public hash table routines */

/*
 *---------------------------------------------------------------------------
 *
 * Blt_InitHashTable --
 *
 *	Given storage for a hash table, set up the fields to prepare the
 *	hash table for use.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	TablePtr is now ready to be passed to Blt_FindHashEntry and
 *	Blt_CreateHashEntry.
 *
 *---------------------------------------------------------------------------
 */
void
Blt_InitHashTable(Blt_HashTable *tablePtr, size_t keyType)
{
#if (BLT_SMALL_HASH_TABLE != 4) 
    Blt_Panic("Blt_InitHashTable: BLT_SMALL_HASH_TABLE is %d, not 4\n",
	    BLT_SMALL_HASH_TABLE);
#endif
    tablePtr->buckets = tablePtr->staticBuckets;
    tablePtr->numBuckets = BLT_SMALL_HASH_TABLE;
    tablePtr->staticBuckets[0] = tablePtr->staticBuckets[1] = 0;
    tablePtr->staticBuckets[2] = tablePtr->staticBuckets[3] = 0;
    tablePtr->numEntries = 0;
    tablePtr->rebuildSize = BLT_SMALL_HASH_TABLE * REBUILD_MULTIPLIER;
    tablePtr->downShift = DOWNSHIFT_START;

    /* The number of buckets is always a power of 2, so we can generate the
     * mask by simply subtracting 1 from the number of buckets. */
    tablePtr->mask = (Blt_Hash)(tablePtr->numBuckets - 1);
    tablePtr->keyType = keyType;

    switch (keyType) {
    case BLT_STRING_KEYS:		/* NUL terminated string keys. */
	tablePtr->findProc = StringFind;
	tablePtr->createProc = StringCreate;
	break;

    case BLT_ONE_WORD_KEYS:		/* 32 or 64-bit atomic keys. */
	tablePtr->findProc = OneWordFind;
	tablePtr->createProc = OneWordCreate;
	break;

    default:				/* Structures/arrays. */
	if (keyType == 0) {
	    Blt_Panic("Blt_InitHashTable: Key size can't be %d, must be > 0\n",
		  keyType);
	}
	tablePtr->findProc = ArrayFind;
	tablePtr->createProc = ArrayCreate;
	break;
    }
    tablePtr->hPool = NULL;
}

/*
 *---------------------------------------------------------------------------
 *
 * Blt_InitHashTableWithPool --
 *
 *	Given storage for a hash table, set up the fields to prepare the
 *	hash table for use.  The only difference between this routine and
 *	Blt_InitHashTable is that is uses a pool allocator to allocate
 *	memory for hash table entries.  The type of pool is either fixed or
 *	variable size (string) keys.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	TablePtr is now ready to be passed to Blt_FindHashEntry and
 *	Blt_CreateHashEntry.
 *
 *---------------------------------------------------------------------------
 */
void
Blt_InitHashTableWithPool(Blt_HashTable *tablePtr, size_t keyType)
{
    Blt_InitHashTable(tablePtr, keyType);
    if (keyType == BLT_STRING_KEYS) {
	tablePtr->hPool = Blt_Pool_Create(BLT_VARIABLE_SIZE_ITEMS);
    } else {
	tablePtr->hPool = Blt_Pool_Create(BLT_FIXED_SIZE_ITEMS);
    }
}

/*
 *---------------------------------------------------------------------------
 *
 * Blt_DeleteHashEntry --
 *
 *	Remove a single entry from a hash table.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The entry given by entryPtr is deleted from its table and should
 *	never again be used by the caller.  It is up to the caller to free
 *	the clientData field of the entry, if that is relevant.
 *
 *---------------------------------------------------------------------------
 */
void
Blt_DeleteHashEntry(Blt_HashTable *tablePtr, Blt_HashEntry *entryPtr)
{
    Blt_HashEntry **bucketPtr;
    size_t hindex;

    if (tablePtr->keyType == BLT_ONE_WORD_KEYS) {
	hindex = RANDOM_INDEX(tablePtr, (const void *)entryPtr->hval);
    } else {
	hindex = (entryPtr->hval & tablePtr->mask);
    }	
    bucketPtr = tablePtr->buckets + hindex;
    if (*bucketPtr == entryPtr) {
	*bucketPtr = entryPtr->nextPtr;
    } else {
	Blt_HashEntry *prevPtr;

	for (prevPtr = *bucketPtr; /*empty*/; prevPtr = prevPtr->nextPtr) {
	    if (prevPtr == NULL) {
		Blt_Panic("malformed bucket chain in Blt_DeleteHashEntry");
	    } else if (prevPtr->nextPtr == entryPtr) {
		prevPtr->nextPtr = entryPtr->nextPtr;
		break;
	    }
	}
    }
    tablePtr->numEntries--;
    if (tablePtr->hPool != NULL) {
	Blt_Pool_FreeItem(tablePtr->hPool, entryPtr);
    } else {
	Blt_Free(entryPtr);
    }
}

/*
 *---------------------------------------------------------------------------
 *
 * Blt_DeleteHashTable --
 *
 *	Free up everything associated with a hash table except for the
 *	record for the table itself.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The hash table is no longer useable.
 *
 *---------------------------------------------------------------------------
 */
void
Blt_DeleteHashTable(Blt_HashTable *tablePtr)	/* Table to delete. */
{
    /* Free up all the entries in the table. */
    if (tablePtr->hPool != NULL) {
	Blt_Pool_Destroy(tablePtr->hPool);
	tablePtr->hPool = NULL;
    } else {
	size_t i;

	for (i = 0; i < tablePtr->numBuckets; i++) {
	    Blt_HashEntry *hPtr;

	    hPtr = tablePtr->buckets[i];
	    while (hPtr != NULL) {
		Blt_HashEntry *nextPtr;

		nextPtr = hPtr->nextPtr;
		Blt_Free(hPtr);
		hPtr = nextPtr;
	    }
	}
    }
    
    /* Free up the bucket array, if it was dynamically allocated. */
    if (tablePtr->buckets != tablePtr->staticBuckets) {
	Blt_Free(tablePtr->buckets);
    }

    /*
     * Arrange for panics if the table is used again without
     * re-initialization.
     */
    tablePtr->findProc = BogusFind;
    tablePtr->createProc = BogusCreate;
}

/*
 *---------------------------------------------------------------------------
 *
 * Blt_FirstHashEntry --
 *
 *	Locate the first entry in a hash table and set up a record that can
 *	be used to step through all the remaining entries of the table.
 *
 * Results:
 *	The return value is a pointer to the first entry in tablePtr, or
 *	NULL if tablePtr has no entries in it.  The memory at *searchPtr is
 *	initialized so that subsequent calls to Blt_NextHashEntry will
 *	return all of the entries in the table, one at a time.
 *
 * Side effects:
 *	None.
 *
 *---------------------------------------------------------------------------
 */
Blt_HashEntry *
Blt_FirstHashEntry(
    Blt_HashTable *tablePtr,		/* Table to search. */
    Blt_HashSearch *searchPtr)		/* Place to store information about
					 * progress through the table. */
{
    searchPtr->tablePtr = tablePtr;
    searchPtr->nextIndex = 0;
    searchPtr->nextEntryPtr = NULL;
    return Blt_NextHashEntry(searchPtr);
}

/*
 *---------------------------------------------------------------------------
 *
 * Blt_NextHashEntry --
 *
 *	Once a hash table enumeration has been initiated by calling
 *	Blt_FirstHashEntry, this procedure may be called to return
 *	successive elements of the table.
 *
 * Results:
 *	The return value is the next entry in the hash table being
 *	enumerated, or NULL if the end of the table is reached.
 *
 * Side effects:
 *	None.
 *
 *---------------------------------------------------------------------------
 */
Blt_HashEntry *
Blt_NextHashEntry(Blt_HashSearch *searchPtr)
{
    Blt_HashEntry *hPtr;

    while (searchPtr->nextEntryPtr == NULL) {
	if (searchPtr->nextIndex >= searchPtr->tablePtr->numBuckets) {
	    return NULL;
	}
	searchPtr->nextEntryPtr =
		searchPtr->tablePtr->buckets[searchPtr->nextIndex];
	searchPtr->nextIndex++;
    }
    hPtr = searchPtr->nextEntryPtr;
    searchPtr->nextEntryPtr = hPtr->nextPtr;
    return hPtr;
}

/*
 *---------------------------------------------------------------------------
 *
 * Blt_HashStats --
 *
 *	Return statistics describing the layout of the hash table in its
 *	hash buckets.
 *
 * Results:
 *	The return value is a malloc-ed string containing information about
 *	tablePtr.  It is the caller's responsibility to free this string.
 *
 * Side effects:
 *	None.
 *
 *---------------------------------------------------------------------------
 */
const char *
Blt_HashStats(Blt_HashTable *tablePtr)  /* Table where to collect stats. */
{
    Blt_HashEntry **bp, **bend;
    char *result, *p;
    double average, tmp;
#define NUM_COUNTERS 10
    size_t count[NUM_COUNTERS], overflow, i, max;

    /* Compute a histogram of bucket usage. */
    for (i = 0; i < NUM_COUNTERS; i++) {
	count[i] = 0;
    }
    overflow = 0;
    average = 0.0;
    max = 0;

    for (bp = tablePtr->buckets, bend = bp + tablePtr->numBuckets; bp < bend; 
	 bp++) {
	Blt_HashEntry *hPtr;
	size_t j;

	j = 0;
	for (hPtr = *bp; hPtr != NULL; hPtr = hPtr->nextPtr) {
	    j++;
	}
	if (j > max) {
	    max = j;
	}
	if (j < NUM_COUNTERS) {
	    count[j]++;
	} else {
	    overflow++;
	}
	tmp = j;
	average += (tmp+1.0)*(tmp/tablePtr->numEntries)/2.0;
    }

    /* Print out the histogram and a few other pieces of information. */
    result = Blt_AssertMalloc((unsigned) ((NUM_COUNTERS*60) + 300));
    sprintf(result, "%lu entries in table, %lu buckets\n",
	    (unsigned long)tablePtr->numEntries, 
	    (unsigned long)tablePtr->numBuckets);
    p = result + strlen(result);
    for (i = 0; i < NUM_COUNTERS; i++) {
	sprintf(p, "number of buckets with %lu entries: %lu\n",
		(unsigned long)i, (unsigned long)count[i]);
	p += strlen(p);
    }
    sprintf(p, "number of buckets with %d or more entries: %lu\n",
	    NUM_COUNTERS, (unsigned long)overflow);
    p += strlen(p);
    sprintf(p, "average search distance for entry: %.2f\n", average);
    p += strlen(p);
    sprintf(p, "maximum search distance for entry: %lu", (unsigned long)max);
    return result;
}
