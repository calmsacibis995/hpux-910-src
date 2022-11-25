/*
static char rcsId[] =
  "@(#) $Header: select.c,v 66.4 90/08/06 13:51:04 rpt Exp $";
static char copyright[] =
  "Copyright (c) HP, 1989";
*/


#include "select.h"


void SelectInit(m)
SelectMask m;
{
  int i;

  for (i = 0; i < SELECTints; i++) {
    m[i] = 0;
  }
}

void SelectInclude(m, fd)
SelectMask m;
int fd;
{
  m[fd/BITS(int)] |= 1 << (fd%BITS(int));
}

#ifndef KEYSHELL
int SelectTest(m, fd)
SelectMask m;
int fd;
{
  return m[fd/BITS(int)] & (1 << (fd%BITS(int)));
}
#endif
