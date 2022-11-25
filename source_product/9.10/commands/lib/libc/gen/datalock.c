/* @(#) $Revision: 64.6 $ */    
/*
 * Generic stack allocator algorithm:
 *	Break stack size requests into four buckets,
 *	in 2^3*n-1 increments.  Allocate bucket-size,
 *	check residual to determine next bucket.
 */

#ifdef _NAMESPACE_CLEAN
#define datalock _datalock
#define plock _plock
#define abs _abs
#       ifdef   _ANSIC_CLEAN
#define malloc _malloc
#define free _free
#       endif  /* _ANSIC_CLEAN */
#endif

#include <sys/lock.h>

extern char *malloc();
static int *stack_addr;
static int counter;

/*  Lines added to clean up ANSI/POSIX namespace */
#ifdef _NAMESPACE_CLEAN
#undef datalock
#pragma _HP_SECONDARY_DEF _datalock datalock
#define datalock _datalock
#endif

datalock(datsiz, stsiz)
{
	char *m;

	if ((m = malloc(datsiz)) == 0)
		return (-1);
	stack_addr = &stsiz;
	if (stsiz & ~077777) {
		if (s_alloc3(stsiz) == -1)
			return (-1);
	} else if (stsiz & ~07777) {
		if (s_alloc2(stsiz) == -1)
			return (-1);
	} else if (stsiz & ~0777) {
		if (s_alloc1(stsiz) == -1)
			return (-1);
	} else if (s_alloc(stsiz) == -1)
		return (-1);
	free(m);
	return(counter);	/* if debug defined, number */
				/* of trips through allocator */
}

static
s_alloc3(size)
{
/*	Assumes size is stored near stack pointer. */
	register int residual = size - abs((int)&size - (int)stack_addr);
	char a[0100000];

#ifdef debug
	counter += 1;
#endif debug
	if (residual <= 0)
		return(plock(PROCLOCK));
	else if (residual & ~077777)
		return (s_alloc3(size));
	else if (residual & ~07777)
		return (s_alloc2(size));
	else if (residual & ~0777)
		return (s_alloc1(size));
	else return (s_alloc(size));
}

static
s_alloc2(size)
{
	register int residual = size - abs((int)&size - (int)stack_addr);
	char a[010000];

#ifdef debug
	counter += 1;
#endif debug
	if (residual <= 0)
		return(plock(PROCLOCK));
	else if (residual & ~07777)
		return (s_alloc2(size));
	else if (residual & ~0777)
		return (s_alloc1(size));
	else return (s_alloc(size));
}

static
s_alloc1(size)
{
	register int residual = size - abs((int)&size - (int)stack_addr);
	char a[01000];

#ifdef debug
	counter += 1;
#endif debug
	if (residual <= 0)
		return(plock(PROCLOCK));
	else if (residual & ~0777)
		return (s_alloc1(size));
	else return (s_alloc(size));
}

static
s_alloc(size)
{
	register int residual = size - abs((int)&size - (int)stack_addr);
	char a[0100];

#ifdef debug
	counter += 1;
#endif debug
	if (residual <= 0)
		return(plock(PROCLOCK));
	else
		return(s_alloc(size));
}
