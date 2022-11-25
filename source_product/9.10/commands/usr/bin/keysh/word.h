/* RCS ID: @(#) $Header: word.h,v 66.3 90/08/06 13:57:54 rpt Exp $ */
/* Copyright (c) HP, 1989 */

#ifndef WORDincluded
#define WORDincluded

extern void WordInitialize();

extern void WordBegin();

extern int WordRead(/* char *word, char *newWord */);

extern int WordAvail();

extern void WordEnd();

#endif
