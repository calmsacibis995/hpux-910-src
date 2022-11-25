/* RCS ID: @(#) $Header: intrinsics.h,v 66.3 90/07/22 22:38:34 rpt Exp $ */
/* Copyright (c) HP, 1989 */

#ifndef INTRINSICSincluded
#define INTRINSICSincluded

extern void IntrinsicsInitialize();

extern void IntrinsicsExecute(/* char *word; int code */);

extern int IntrinsicsSelected(/* GlobalSoftKey *sk */);

extern void IntrinsicsBegin();

/* call intrinsic functions */

extern void IntrinsicsEnd();

#endif
