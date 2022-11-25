/* RCS ID: @(#) $Header: clist.h,v 66.2 90/09/26 22:14:05 rpt Exp $ */
/* Copyright (c) HP, 1989 */

#ifndef CLISTincluded
#define CLISTincluded

#include "linklist.h"
#include "chunk.h"


/* local use only */
#define SEGMENT 128

/* local use only */
typedef struct Segment {
  Link link;
  int c[SEGMENT];
} Segment;

typedef struct CList {
  Link base;
  Chunk *chunk;
  Segment *inSegment;
  int inIndex;
  Segment *outSegment;
  int outIndex;
  int count;
} CList;

extern CList *CListCreate();
extern void CListAdd(/* CList *cList, int c */);
extern int CListRemove(/* CList *cList */);
extern int CListPeek(/* CList *cList */);
extern int CListLook(/* CList *cList, int n */);
extern int CListCount(/* CList *cList */);
extern void CListEmpty(/* CList *cList */);
extern void CListDestroy(/* CList *cList */);

#endif
