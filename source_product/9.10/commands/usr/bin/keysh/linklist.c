/*
static char rcsId[] =
  "@(#) $Header: linklist.c,v 66.5 90/09/26 22:57:01 rpt Exp $";
static char copyright[] =
  "Copyright (c) HP, 1989";
*/


#include "linklist.h"

#define NULL 0


#ifndef KEYSHELL
/*VARARGS*/
void *LinkNext(link)
Link *link;
{
  link = link->next;
  if (link == link->base) {
    return NULL;
  }
  return link;
}
#endif

/*VARARGS*/
void *LinkPrev(link)
Link *link;
{
  link = link->prev;
  if (link == link->base) {
    return NULL;
  }
  return link;
}

/*VARARGS*/
void *LinkBase(link)
Link *link;
{
  return link->base;
}

/*VARARGS*/
void *LinkHead(base)
Link *base;
{
  return LinkNext(base);
}

/*VARARGS*/
void *LinkTail(base)
Link *base;
{
  return LinkPrev(base);
}

/*VARARGS*/
void *LinkNth(base, n)
Link *base;
int n;
{
  Link *link;

  link = base->next;
  while ((n-- > 0) && link != base) {
    link = link->next;
  }
  if (link == base) {
    return NULL;
  }
  return link;
}

/*VARARGS*/
void LinkCreateBase(base)
Link *base;
{
  base->next = base;
  base->prev = base;
  base->base = base;
}

/*VARARGS*/
void LinkAddBefore(exist, link)
Link *exist;
Link *link;
{
  link->prev = exist->prev;
  exist->prev->next = link;
  link->next = exist;
  exist->prev = link;
  link->base = exist->base;
}

/*VARARGS*/
void LinkAddAfter(exist, link)
Link *exist;
Link *link;
{
  link->next = exist->next;
  exist->next->prev = link;
  link->prev = exist;
  exist->next = link;
  link->base = exist->base;
}

#ifndef KEYSHELL
/*VARARGS*/
void LinkAddHead(base, link)
Link *base;
Link *link;
{
  LinkAddAfter(base, link);
}
#endif

/*VARARGS*/
void LinkAddTail(base, link)
Link *base;
Link *link;
{
  LinkAddBefore(base, link);
}

/*VARARGS*/
void *LinkRemove(exist)
Link *exist;
{
  exist->prev->next = exist->next;
  exist->next->prev = exist->prev;
  exist->base = 0;
  return exist;
}

/*VARARGS*/
int LinkCount(base)
Link *base;
{
  int n;

  n = 0;
  while (base = LinkNext(base)) {
    n++;
  }
  return n;
}

