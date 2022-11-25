/*
static char rcsId[] =
  "@(#) $Header: clist.c,v 66.5 90/09/26 22:12:48 rpt Exp $";
static char copyright[] =
  "Copyright (c) HP, 1989";
*/


#include "chunk.h"
#include "linklist.h"
#include "clist.h"


static Segment *NextSegment(segment)
Segment *segment;
{
  Segment *next;

  next = LinkNext(segment);
  if (! next) {
    next = LinkHead(LinkBase(segment));
  }
  return next;
}

CList *CListCreate()
{
  CList *cList;
  Chunk *chunk;
  Segment *segment;

  chunk = ChunkCreate();
  cList = (CList *)ChunkMalloc(chunk, sizeof(*cList));
  cList->chunk = chunk;
  LinkCreateBase(cList);
  segment = (Segment *)ChunkMalloc(chunk, sizeof(*segment));
  LinkAddTail(cList, segment);
  cList->inSegment = segment;
  cList->inIndex = 0;
  cList->outSegment = segment;
  cList->outIndex = 0;
  cList->count = 0;
  return cList;
}

void CListAdd(cList, c)
CList *cList;
int c;
{
  Segment *segment;

  cList->inSegment->c[cList->inIndex++] = c;
  cList->count++;
  if (cList->inIndex >= SEGMENT) {
    if (NextSegment(cList->inSegment) == cList->outSegment) {
      segment = (Segment *)ChunkMalloc(cList->chunk, sizeof(*segment));
      LinkAddAfter(cList->inSegment, segment);
    }
    cList->inSegment = NextSegment(cList->inSegment);
    cList->inIndex = 0;
  }
}

int CListRemove(cList)
CList *cList;
{
  int c;

  if (! cList->count) {
    return 0;
  }
  c = cList->outSegment->c[cList->outIndex++];
  cList->count--;
  if (cList->outIndex >= SEGMENT) {
    cList->outSegment = NextSegment(cList->outSegment);
    cList->outIndex = 0;
  }
  return c;
}

int CListPeek(cList)
CList *cList;
{
  return CListLook(cList, 0);
}

int CListLook(cList, n)
CList *cList;
int n;
{
  Segment *lookSegment;
  int lookIndex;

  if (n >= cList->count) {
    return 0;
  }
  lookSegment = cList->outSegment;
  lookIndex = cList->outIndex;
  while (n) {
    if (n >= SEGMENT-lookIndex) {
      n -= SEGMENT-lookIndex;
      lookSegment = NextSegment(lookSegment);
      lookIndex = 0;
    } else {
      lookIndex += n;
      n = 0;
    }
  }
  return lookSegment->c[lookIndex];
}

int CListCount(cList)
CList *cList;
{
  return cList->count;
}

void CListEmpty(cList)
CList *cList;
{
  Segment *segment;

  segment = LinkHead(cList);
  cList->inSegment = segment;
  cList->inIndex = 0;
  cList->outSegment = segment;
  cList->outIndex = 0;
  cList->count = 0;
}

#ifndef KEYSHELL
void CListDestroy(cList)
CList *cList;
{
  ChunkDestroy(cList->chunk);
}
#endif
