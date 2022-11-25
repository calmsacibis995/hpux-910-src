/* RCS ID: @(#) $Header: string2.h,v 66.5 90/11/13 13:56:22 rpt Exp $ */
/* Copyright (c) HP, 1989 */


#ifndef STRING2included
#define STRING2included

extern char *StringEnd();
extern char *StringBeginSig();
extern char *StringEndSig();
extern int StringSig();
extern char *StringTail();

extern char *StringCapitalize();
extern char *StringUnCapitalize();
extern int StringCompareLower();

extern char *StringExtract();
extern char *StringPad();
extern char *StringRepeat();
extern char *StringConcat();

extern int StringUpdate();
extern int StringDiff();
extern int StringDiff6();

extern char *StringVisual();

#endif
