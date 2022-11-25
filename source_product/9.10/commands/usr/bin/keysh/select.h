/* RCS ID: @(#) $Header: select.h,v 66.2 90/06/05 12:34:39 rpt Exp $ */
/* Copyright (c) HP, 1989 */

#ifndef SELECTincluded
#define SELECTincluded

#include <sys/param.h>
#include <values.h>


#define SELECTfiles           NOFILE
#define SELECTints            (SELECTfiles/BITS(int) + 1)
typedef int SelectMask[SELECTints];

extern void SelectInit(/* SelectMask m */);
extern void SelectInclude(/* SelectMask m, int fd */);
extern int SelectTest(/* SelectMask m, int fd */);

#endif
