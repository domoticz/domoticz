/*	$OpenBSD: cdefs.h,v 1.32 2012/01/03 16:56:58 kettenis Exp $	*/
/*	$NetBSD: cdefs.h,v 1.16 1996/04/03 20:46:39 christos Exp $	*/

/*
 * Copyright (c) 1991, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * Berkeley Software Design, Inc.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *	@(#)cdefs.h	8.7 (Berkeley) 1/21/94
 */

#ifndef	_SYS_CDEFS_H_
#define	_SYS_CDEFS_H_

#include <machine/cdefs.h>

#if defined(__cplusplus)
#define	__BEGIN_DECLS	extern "C" {
#define	__END_DECLS	}
#else
#define	__BEGIN_DECLS
#define	__END_DECLS
#endif

/*
 * Macro to test if we're using a specific version of gcc or later.
 */
#ifdef __GNUC__
#define __GNUC_PREREQ__(ma, mi) \
	((__GNUC__ > (ma)) || (__GNUC__ == (ma) && __GNUC_MINOR__ >= (mi)))
#else
#define __GNUC_PREREQ__(ma, mi) 0
#endif

/*
 * The __CONCAT macro is used to concatenate parts of symbol names, e.g.
 * with "#define OLD(foo) __CONCAT(old,foo)", OLD(foo) produces oldfoo.
 * The __CONCAT macro is a bit tricky -- make sure you don't put spaces
 * in between its arguments.  __CONCAT can also concatenate double-quoted
 * strings produced by the __STRING macro, but this only works with ANSI C.
 */
#if defined(__STDC__) || defined(__cplusplus)
#define	__P(protos)	protos		/* full-blown ANSI C */
#define	__CONCAT(x,y)	x ## y
#define	__STRING(x)	#x

#define	__const		const		/* define reserved names to standard */
#define	__signed	signed
#define	__volatile	volatile
#if defined(__cplusplus) || defined(__PCC__)
#define	__inline	inline		/* convert to C++ keyword */
#else
#if !defined(__GNUC__) && !defined(lint)
#define	__inline			/* delete GCC keyword */
#endif /* !__GNUC__ && !lint */
#endif /* !__cplusplus */

#else	/* !(__STDC__ || __cplusplus) */
#define	__P(protos)	()		/* traditional C preprocessor */
#define	__CONCAT(x,y)	x/**/y
#define	__STRING(x)	"x"

#if defined(_MSC_VER)
#include <windows.h>
#elif !defined(__GNUC__) && !defined(lint)
#define	__const				/* delete pseudo-ANSI C keywords */
#define	__inline
#define	__signed
#define	__volatile
#endif	/* !__GNUC__ && !lint */

/*
 * In non-ANSI C environments, new programs will want ANSI-only C keywords
 * deleted from the program and old programs will want them left alone.
 * Programs using the ANSI C keywords const, inline etc. as normal
 * identifiers should define -DNO_ANSI_KEYWORDS.
 */
#ifndef	NO_ANSI_KEYWORDS
#define	const		__const		/* convert ANSI C keywords */
#define	inline		__inline
#define	signed		__signed
#define	volatile	__volatile
#endif /* !NO_ANSI_KEYWORDS */
#endif	/* !(__STDC__ || __cplusplus) */

/*
 * GCC1 and some versions of GCC2 declare dead (non-returning) and
 * pure (no side effects) functions using "volatile" and "const";
 * unfortunately, these then cause warnings under "-ansi -pedantic".
 * GCC >= 2.5 uses the __attribute__((attrs)) style.  All of these
 * work for GNU C++ (modulo a slight glitch in the C++ grammar in
 * the distribution version of 2.5.5).
 */

#if !__GNUC_PREREQ__(2, 5) && !defined(__PCC__)
#define	__attribute__(x)	/* delete __attribute__ if non-gcc or gcc1 */
#if defined(__GNUC__) && !defined(__STRICT_ANSI__)
#define	__dead		__volatile
#define	__pure		__const
#elif defined(lint)
#define __dead		/* NORETURN */
#endif
#elif !defined(__STRICT_ANSI__)
#define __dead		__attribute__((__noreturn__))
#define __pure		__attribute__((__const__))
#endif

#if __GNUC_PREREQ__(2, 7)
#define	__unused	__attribute__((__unused__))
#else
#define	__unused	/* delete */
#endif

#if __GNUC_PREREQ__(3, 1)
#define	__used		__attribute__((__used__))
#else
#define	__used		__unused	/* suppress -Wunused warnings */
#endif

/*
 * __returns_twice makes the compiler not assume the function
 * only returns once.  This affects registerisation of variables:
 * even local variables need to be in memory across such a call.
 * Example: setjmp()
 */
#if __GNUC_PREREQ__(4, 1)
#define __returns_twice	__attribute__((returns_twice))
#else
#define __returns_twice
#endif

/*
 * __only_inline makes the compiler only use this function definition
 * for inlining; references that can't be inlined will be left as
 * external references instead of generating a local copy.  The
 * matching library should include a simple extern definition for
 * the function to handle those references.  c.f. ctype.h
 */
#ifdef __GNUC__
#  if __GNUC_PREREQ__(4, 2)
#define __only_inline	extern __inline __attribute__((__gnu_inline__))
#  else
#define __only_inline	extern __inline
#  endif
#else
#define __only_inline	static __inline
#endif

/*
 * GNU C version 2.96 adds explicit branch prediction so that
 * the CPU back-end can hint the processor and also so that
 * code blocks can be reordered such that the predicted path
 * sees a more linear flow, thus improving cache behavior, etc.
 *
 * The following two macros provide us with a way to utilize this
 * compiler feature.  Use __predict_true() if you expect the expression
 * to evaluate to true, and __predict_false() if you expect the
 * expression to evaluate to false.
 *
 * A few notes about usage:
 *
 *	* Generally, __predict_false() error condition checks (unless
 *	  you have some _strong_ reason to do otherwise, in which case
 *	  document it), and/or __predict_true() `no-error' condition
 *	  checks, assuming you want to optimize for the no-error case.
 *
 *	* Other than that, if you don't know the likelihood of a test
 *	  succeeding from empirical or other `hard' evidence, don't
 *	  make predictions.
 *
 *	* These are meant to be used in places that are run `a lot'.
 *	  It is wasteful to make predictions in code that is run
 *	  seldomly (e.g. at subsystem initialization time) as the
 *	  basic block reordering that this affects can often generate
 *	  larger code.
 */
#if __GNUC_PREREQ__(2, 96)
#define __predict_true(exp)	__builtin_expect(((exp) != 0), 1)
#define __predict_false(exp)	__builtin_expect(((exp) != 0), 0)
#else
#define __predict_true(exp)	((exp) != 0)
#define __predict_false(exp)	((exp) != 0)
#endif

/* Delete pseudo-keywords wherever they are not available or needed. */
#ifndef __dead
#define	__dead
#define	__pure
#endif

#if __GNUC_PREREQ__(2, 7) || defined(__PCC__)
#define	__packed	__attribute__((__packed__))
#elif defined(lint)
#define	__packed
#endif

#if !__GNUC_PREREQ__(2, 8)
#define	__extension__
#endif

#if __GNUC_PREREQ__(2, 8) || defined(__PCC__)
#define __statement(x)	__extension__(x)
#elif defined(lint)
#define __statement(x)	(0)
#else
#define __statement(x)	(x)
#endif

#if __GNUC_PREREQ__(3, 0)
#define	__malloc	__attribute__((__malloc__))
#else
#define	__malloc
#endif

/*
 * "The nice thing about standards is that there are so many to choose from."
 * There are a number of "feature test macros" specified by (different)
 * standards that determine which interfaces and types the header files
 * should expose.
 *
 * Because of inconsistencies in these macros, we define our own
 * set in the private name space that end in _VISIBLE.  These are
 * always defined and so headers can test their values easily.
 * Things can get tricky when multiple feature macros are defined.
 * We try to take the union of all the features requested.
 *
 * The following macros are guaranteed to have a value after cdefs.h
 * has been included:
 *	__POSIX_VISIBLE
 *	__XPG_VISIBLE
 *	__ISO_C_VISIBLE
 *	__BSD_VISIBLE
 */

/*
 * X/Open Portability Guides and Single Unix Specifications.
 * _XOPEN_SOURCE				XPG3
 * _XOPEN_SOURCE && _XOPEN_VERSION = 4		XPG4
 * _XOPEN_SOURCE && _XOPEN_SOURCE_EXTENDED = 1	XPG4v2
 * _XOPEN_SOURCE == 500				XPG5
 * _XOPEN_SOURCE == 520				XPG5v2
 * _XOPEN_SOURCE == 600				POSIX 1003.1-2001 with XSI
 * _XOPEN_SOURCE == 700				POSIX 1003.1-2008 with XSI
 *
 * The XPG spec implies a specific value for _POSIX_C_SOURCE.
 */
#ifdef _XOPEN_SOURCE
# if (_XOPEN_SOURCE - 0 >= 700)
#  define __XPG_VISIBLE		700
#  undef _POSIX_C_SOURCE
#  define _POSIX_C_SOURCE	200809L
# elif (_XOPEN_SOURCE - 0 >= 600)
#  define __XPG_VISIBLE		600
#  undef _POSIX_C_SOURCE
#  define _POSIX_C_SOURCE	200112L
# elif (_XOPEN_SOURCE - 0 >= 520)
#  define __XPG_VISIBLE		520
#  undef _POSIX_C_SOURCE
#  define _POSIX_C_SOURCE	199506L
# elif (_XOPEN_SOURCE - 0 >= 500)
#  define __XPG_VISIBLE		500
#  undef _POSIX_C_SOURCE
#  define _POSIX_C_SOURCE	199506L
# elif (_XOPEN_SOURCE_EXTENDED - 0 == 1)
#  define __XPG_VISIBLE		420
# elif (_XOPEN_VERSION - 0 >= 4)
#  define __XPG_VISIBLE		400
# else
#  define __XPG_VISIBLE		300
# endif
#endif

/*
 * POSIX macros, these checks must follow the XOPEN ones above.
 *
 * _POSIX_SOURCE == 1		1003.1-1988 (superseded by _POSIX_C_SOURCE)
 * _POSIX_C_SOURCE == 1		1003.1-1990
 * _POSIX_C_SOURCE == 2		1003.2-1992
 * _POSIX_C_SOURCE == 199309L	1003.1b-1993
 * _POSIX_C_SOURCE == 199506L   1003.1c-1995, 1003.1i-1995,
 *				and the omnibus ISO/IEC 9945-1:1996
 * _POSIX_C_SOURCE == 200112L   1003.1-2001
 * _POSIX_C_SOURCE == 200809L   1003.1-2008
 *
 * The POSIX spec implies a specific value for __ISO_C_VISIBLE, though
 * this may be overridden by the _ISOC99_SOURCE macro later.
 */
#ifdef _POSIX_C_SOURCE
# if (_POSIX_C_SOURCE - 0 >= 200809)
#  define __POSIX_VISIBLE	200809
#  define __ISO_C_VISIBLE	1999
# elif (_POSIX_C_SOURCE - 0 >= 200112)
#  define __POSIX_VISIBLE	200112
#  define __ISO_C_VISIBLE	1999
# elif (_POSIX_C_SOURCE - 0 >= 199506)
#  define __POSIX_VISIBLE	199506
#  define __ISO_C_VISIBLE	1990
# elif (_POSIX_C_SOURCE - 0 >= 199309)
#  define __POSIX_VISIBLE	199309
#  define __ISO_C_VISIBLE	1990
# elif (_POSIX_C_SOURCE - 0 >= 2)
#  define __POSIX_VISIBLE	199209
#  define __ISO_C_VISIBLE	1990
# else
#  define __POSIX_VISIBLE	199009
#  define __ISO_C_VISIBLE	1990
# endif
#elif defined(_POSIX_SOURCE)
# define __POSIX_VISIBLE	198808
#  define __ISO_C_VISIBLE	0
#endif

/*
 * _ANSI_SOURCE means to expose ANSI C89 interfaces only.
 * If the user defines it in addition to one of the POSIX or XOPEN
 * macros, assume the POSIX/XOPEN macro(s) should take precedence.
 */
#if defined(_ANSI_SOURCE) && !defined(__POSIX_VISIBLE) && \
    !defined(__XPG_VISIBLE)
# define __POSIX_VISIBLE	0
# define __XPG_VISIBLE		0
# define __ISO_C_VISIBLE	1990
#endif

/*
 * _ISOC99_SOURCE and __STDC_VERSION__ override any of the other macros since
 * they are non-exclusive.
 */
#if defined(_ISOC99_SOURCE) || \
    (defined(__STDC_VERSION__) && __STDC_VERSION__ >= 199901) || \
    (defined(__cplusplus) && __cplusplus >= 201103)
# undef __ISO_C_VISIBLE
# define __ISO_C_VISIBLE	1999
#endif

/*
 * Finally deal with BSD-specific interfaces that are not covered
 * by any standards.  We expose these when none of the POSIX or XPG
 * macros is defined or if the user explicitly asks for them.
 */
#if !defined(_BSD_SOURCE) && \
   (defined(_ANSI_SOURCE) || defined(__XPG_VISIBLE) || defined(__POSIX_VISIBLE))
# define __BSD_VISIBLE		0
#endif

/*
 * Default values.
 */
#ifndef __XPG_VISIBLE
# define __XPG_VISIBLE		700
#endif
#ifndef __POSIX_VISIBLE
# define __POSIX_VISIBLE	200809
#endif
#ifndef __ISO_C_VISIBLE
# define __ISO_C_VISIBLE	1999
#endif
#ifndef __BSD_VISIBLE
# define __BSD_VISIBLE		1
#endif

#endif /* !_SYS_CDEFS_H_ */
