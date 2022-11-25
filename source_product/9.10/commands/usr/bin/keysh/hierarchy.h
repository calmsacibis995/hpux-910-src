/* RCS ID: @(#) $Header: hierarchy.h,v 66.4 90/09/26 22:38:45 rpt Exp $ */
/* Copyright (c) HP, 1989 */

#ifndef HIERARCHYincluded
#define HIERARCHYincluded

#include "linklist.h"


typedef struct Hier {
  struct Link siblings;
  struct Link children; /* base */
  struct Hier *parent;
  struct Hier *root;
} Hier;


extern void *HierRoot(/* Hier *hier */);
extern void *HierParent(/* Hier *hier */);
extern void *HierHeadChild(/* Hier *hier */);
extern void *HierTailChild(/* Hier *hier */);
extern void *HierNthChild(/* Hier *hier, int n */);
extern void *HierNextSibling(/* Hier *hier */);
extern void *HierPrevSibling(/* Hier *hier */);

#ifdef KEYSHELL
#define HierNextSibling(hier) (LinkNext(&((Hier *)hier)->siblings))
#endif

extern void HierCreateRoot(/* Hier *root */);

extern void HierAddHeadChild(/* Hier *exist, Hier *hier, int preserve */);
extern void HierAddTailChild(/* Hier *exist, Hier *hier, int preserve */);
extern void HierAddBeforeSibling(/* Hier *exist, Hier *hier, int preserve */);
extern void HierAddAfterSibling(/* Hier *exist, Hier *hier, int preserve */);

extern void *HierRemove(/* Hier *exist */);

extern int HierChildCount(/* Hier *exist */);

#endif
