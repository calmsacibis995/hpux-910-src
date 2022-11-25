/* RCS ID: @(#) $Header: vi.h,v 66.2 90/06/05 12:38:32 rpt Exp $ */
/* Copyright (c) HP, 1989 */

#ifndef VIincluded
#define VIincluded

extern int viEscape;

extern void ViInitialize();
extern void ViBegin();
extern int ViRead();
extern int ViAvail();
extern void ViEnd();

#endif

