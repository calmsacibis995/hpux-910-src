/* RCS ID: @(#) $Header: edit.h,v 70.1 94/09/29 16:04:58 hmgr Exp $ */
/* Copyright (c) HP, 1989 */

#ifndef EDITincluded
#define EDITincluded

#define EDITlength  258      /* Modified for DSDe409947 */

typedef enum EditMode {
  editRaw,
  editEmacs,
  editVi
} EditMode;

extern EditMode editMode;
extern int editOverwrite;

extern char editLine[];
extern int editIndex;
extern int editEOL;

extern void EditInitialize();

extern void EditBegin();

extern int EditRead(/* char *editWord */);
extern int EditAvail();

extern void EditEnd();

#endif
