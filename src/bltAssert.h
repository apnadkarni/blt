#ifndef _BLT_ASSERT_H
#define _BLT_ASSERT_H

/*
 * Since the Tcl/Tk distribution doesn't perform any asserts, dynamic
 * loading can fail to find the __assert function.  As a workaround,
 * we'll include our own.
 */
#undef	assert

#ifdef	NDEBUG
#  define	assert(EX) ((void)0)
#else

BLT_EXTERN void Blt_Assert(const char *expr, const char *file, int line);

#ifdef __STDC__
#  define assert(EX) (void)((EX) || (Blt_Assert(#EX, __FILE__, __LINE__), 0))
#else
#  define assert(EX) (void)((EX) || (Blt_Assert("EX", __FILE__, __LINE__), 0))
#endif /* __STDC__ */

#endif /* NDEBUG */

#endif /* _BLT_ASSERT_H */
