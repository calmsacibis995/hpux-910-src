/* RCS ID: @(#) $Header: display.h,v 66.4 90/07/13 11:05:17 rpt Exp $ */
/* Copyright (c) HP, 1989 */

#ifndef DISPLAYincluded
#define DISPLAYincluded


#define DISPLAYlength           160
#define DISPLAYminLines         8
#define DISPLAYminColumns       80

#define DISPLAYsoftKeyLines     2
#define DISPLAYsoftKeyLength    8
#define DISPLAYsoftKeyCount     8


extern void DisplayInitialize();
extern void DisplayStatusFeeder(/* char *(*feeder)() */);

extern void DisplayBegin();
extern void DisplayResume();

extern void DisplayRefresh();
extern void DisplayUpdate(/* int full */);

extern void DisplayBell();
extern void DisplayStatus();
extern void DisplayHint(/* char *hint, int bell */);
extern void DisplayHelp(/* char *label, char *help */);
extern void DisplayPrintf(/* char *s, ... */);
extern void DisplayClear();
extern void DisplayFlush();

extern void DisplayScroll(/* int n */);

extern void DisplayAside();
extern void DisplayEnd();

#endif
