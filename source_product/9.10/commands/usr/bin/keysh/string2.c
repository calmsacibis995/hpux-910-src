/*
static char rcsId[] =
  "@(#) $Header: string2.c,v 66.6 90/11/13 13:54:42 rpt Exp $";
static char copyright[] =
  "Copyright (c) HP, 1989";
*/


#include <ctype.h>
#include <string.h>
#include "string2.h"

#define NULL 0


static char buffer[1024];


/*****************************************************************************/


char *StringEnd(s)
char *s;
{
  return strchr(s, '\0');
}


char *StringBeginSig(source)
char *source;
{
  if (source) {
    while (isspace(*source)) {
      source++;
    }
  }
  return source;
}


char *StringEndSig(source)
char *source;
{
  char *end = NULL;

  if (source) {
    end = StringEnd(source);
    while (end > source && isspace(*(end-1))) {
      end--;
    }
  }
  return end;
}


int StringSig(s)
char *s;
{
  char *ends;

  ends = StringEndSig(s);
  if (ends == s) {
    return 0;
  }
  return ends - StringBeginSig(s);
}


char *StringTail(source, tail)
char *source;
char *tail;
{
  int n;
  int t;

  n = strlen(source);
  t = strlen(tail);
  if (t > n) {
    return NULL;
  }
  if (strcmp(source+n-t, tail)) {
    return NULL;
  }
  return source+n-t;
}


char *StringCapitalize(s)
char *s;
{
  char *p;

  p = s;
  if (*p) {
    *p = _toupper(*p);
    while (*++p) {
      *p = _tolower(*p);
    }
  }
  return s;
}


char *StringUnCapitalize(s)
char *s;
{
  char *p;

  p = s;
  while (*p) {
    *p = _tolower(*p);
    p++;
  }
  return s;
}


int StringCompareLower(s1, s2)
char *s1;
char *s2;
{
  while (*s1 && _tolower(*s1) == _tolower(*s2)) {
    s1++;
    s2++;
  }
  return *s1 - *s2;
}


char *StringExtract(dest, source, end)
char *dest;
char *source;
char *end;
{
  char *result;

  if (dest == NULL) {
    dest = buffer;
  }
  result = dest;
  if (source) {
    while (*source && source != end) {
      *dest++ = *source++;
    }
  }
  *dest = '\0';
  return result;
}


char *StringPad(s, n)
char *s;
int n;
{
  int i;

  for (i = 0; s[i] && i < n; i++) {
    buffer[i] = s[i];
  }
  while (i < n) {
    buffer[i++] = ' ';
  }
  buffer[i] = '\0';
  return buffer;
}


char *StringRepeat(c, n)
char c;
int n;
{
  int i;

  for (i = 0; i < n; i++) {
    buffer[i] = c;
  }
  buffer[i] = '\0';
  return buffer;
}


char *StringConcat(s1, s2)
char *s1;
char *s2;
{
  strcpy(buffer, s1);
  strcat(buffer, s2);
  return buffer;
}


int StringUpdate(new, old, length)
char *new;
char *old;
int *length;
{
  int last;
  int first;
  int count;

  last = first = count = 0;
  if (*new != *old) {
    first = 0;
    last = 1;
  }
  while (*new || *old) {
    if (*new != *old) {
      if (! last) {
        first = count;
      }
      last = count+1;
    }
    if (*new) {
      new++;
    }
    if (*old) {
      old++;
    }
    count++;
  }
  *length = last-first;
  return first;
}


int StringDiff(new, old, newLength, oldLength)
char *new;
char *old;
int *newLength;
int *oldLength;
{
  int first;
  int oldLast;
  int newLast;

  /* pretty poor performance... */
  if (strcmp(new, old) == 0) {
    *newLength = 0;
    *oldLength = 0;
    return -1;
  }
  first = 0;
  oldLast = strlen(old);
  newLast = strlen(new);
  while (new[first] == old[first]) {
    first++;
  }
  if (! new[first]) {
    *newLength = 0;
    *oldLength = oldLast-first;
    return first;
  }
  if (! old[first]) {
    *oldLength = 0;
    *newLength = newLast-first;
    return first;
  }
  while (newLast > first && oldLast > first) {
    if (new[newLast-1] != old[oldLast-1]) {
      break;
    }
    newLast--;
    oldLast--;
  }
  *newLength = newLast-first;
  *oldLength = oldLast-first;
  return first;
}



int StringDiff6(new, old, limit, length, newLength, oldLength)
char *new;
char *old;
int limit;
int length;
int *newLength;
int *oldLength;
{
  int ni;
  int oi;
  int nlen;
  int olen;
  int sum;
  int first;

  first = 0;
  while (*new == *old) {
    if (! *new) {
      return -1;
    }
    new++;
    old++;
    first++;
  }
  nlen = strlen(new);
  olen = strlen(old);
  for (sum = 1; sum < limit; sum++) {
    for (ni = 0; ni <= sum; ni++) {
      oi = sum - ni;
      if (ni <= nlen && oi <= olen && new[ni] == old[oi] &&
          strncmp(new+ni, old+oi, length-first-((ni>oi)?(ni):(oi))) == 0) {
        *newLength = ni;
        *oldLength = oi;
        return first;
      }
    }
  }
  return -1;
}


char *StringVisual(dest, source, length, point)
char *dest;
char *source;
int length;
char *point;
{
  int c;
  int i;
  char *translate;

  i = 0;
  translate = NULL;
  while (i < length && (c = *source)) {
    if (iscntrl(c)) {
      dest[i++] = '^';
      if (i >= length) {
        continue;
      }
      dest[i++] = c ^ '@';
    } else {
      dest[i++] = c;
    }
    if (source++ == point) {
      translate = dest + i-1;
    }
  }
  if (i < length) {
    dest[i] = '\0';
    if (! *point) {
      translate = dest + i;
    }
  }
  return translate;
}


/*****************************************************************************/
