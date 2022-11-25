/* RCS ID: @(#) $Header: linklist.h,v 66.4 90/09/26 23:01:07 rpt Exp $ */
/* Copyright (c) HP, 1989 */

#ifndef LINKLISTincluded
#define LINKLISTincluded

typedef struct Link {
  struct Link *next;
  struct Link *prev;
  struct Link *base;
} Link;

extern void *LinkNext(/* Link *link */);
extern void *LinkPrev(/* Link *link */);
extern void *LinkBase(/* Link *link */);
extern void *LinkHead(/* Link *base */);
extern void *LinkTail(/* Link *base */);
extern void *LinkNth(/* Link *base, int n */);

#ifdef KEYSHELL
#define LinkNext(link) ((((Link *)link)->next == ((Link *)link)->base)? \
                        ((void *)0): \
                        ((void *)((Link *)link)->next))
#endif

extern void LinkCreateBase(/* Link *base */);

extern void LinkAddBefore(/* Link *exist, Link *link */);
extern void LinkAddAfter(/* Link *exist, Link *link */);
extern void LinkAddHead(/* Link *base, Link *link */);
extern void LinkAddTail(/* Link *base, Link *link */);

extern void *LinkRemove(/* Link *link */);

extern int LinkCount(/* Link *base */);

#endif
