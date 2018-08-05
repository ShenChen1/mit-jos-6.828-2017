/*	$NetBSD: stdarg.h,v 1.12 1995/12/25 23:15:31 mycroft Exp $	*/

#ifndef JOS_INC_STDARG_H
#define	JOS_INC_STDARG_H

#if 0

typedef __builtin_va_list va_list;

#define va_start(ap, last) __builtin_va_start(ap, last)

#define va_arg(ap, type) __builtin_va_arg(ap, type)

#define va_end(ap) __builtin_va_end(ap)

#else

typedef char *	va_list;

#define _ADDRESSOF(v)   ( &(v) )
#define _INTSIZEOF(n)   ( (sizeof(n) + sizeof(int) - 1) & ~(sizeof(int) - 1) )

#define va_start(ap,v)  ( ap = (va_list)_ADDRESSOF(v) + _INTSIZEOF(v) )
#define va_arg(ap,t)    ( *(t *)((ap += _INTSIZEOF(t)) - _INTSIZEOF(t)) )
#define va_end(ap)      ( ap = (va_list)0 )

#endif

#endif	/* !JOS_INC_STDARG_H */
