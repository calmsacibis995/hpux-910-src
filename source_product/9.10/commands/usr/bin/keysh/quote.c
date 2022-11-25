/*
static char rcsId[] =
  "@(#) $Header: quote.c,v 66.7 91/01/01 15:26:47 rpt Exp $";
static char copyright[] =
  "Copyright (c) HP, 1989";
*/


#include <pwd.h>
#include <sys/param.h>
#include <string.h>
#include <ctype.h>

#include "keyshell.h"
#include "kshhooks.h"


char *QuoteEnd(word)
char *word;
{
  char c;
  char *end;
  char last;
  char quote;

  end = word;
  last = '\0';
  quote = '\0';
  while (c = *word) {
    if (last != '\\' && (c == '\'' || c == '"' || c == '`')) {
      if (! quote) {
        quote = c;
      } else if (c == quote) {
        quote = '\0';
      }
    }
    word++;
    if (! isspace(c) || last == '\\') {
      end = word;
    }
    if (last == '\\') {
      last = '.';
    } else {
      last = c;
    }
  }
  if (last == '\\' || quote) {
    return NULL;
  }
  return end;
}

int QuoteSkipSpace(peekc, readc)
int (*peekc)();
int (*readc)();
{
  char c;

  while (isspace(c = (*peekc)()) || c == '#') {
    (void)(*readc)();
    if (c == '#') {
      while ((c = (*peekc)()) && c != '\n') {
        (void)(*readc)();
      }
    }
  }
  return c;
}

int QuoteBackOf(peekc, readc)
int (*peekc)();
int (*readc)();
{
  int c;
  int octal;
  int digit;

again:
  c = (*readc)();
  switch (c) {
    case 'n'  : return '\n';
    case 't'  : return '\t';
    case 'v'  : return '\v';
    case 'b'  : return '\b';
    case 'r'  : return '\r';
    case 'f'  : return '\f';
    case '0'  :
    case '1'  :
    case '2'  :
    case '3'  :
    case '4'  :
    case '5'  :
    case '6'  :
    case '7'  : digit = 0;
                octal = c - '0';
                while (digit++ < 3 && (c = (*peekc)()) >= '0' && c <= '7') {
                  c = (*readc)();
                  octal = octal*8 + c - '0';
                }
                return octal;
    case '\n' : c = (*readc)();
                if (c == '\\') {
                  goto again;
                }
                return c;
  }
  return c;
}

#define LENGTH 16384

char *QuoteReadWord(peekc, readc, extended, stopper)
int (*peekc)();
int (*readc)();
int extended;
int stopper;
{
  int c;
  int i;
  char *p;
  int nest;
  int openC;
  int closeC;
  static char word[LENGTH];

  c = QuoteSkipSpace(peekc, readc);
  if (! c || c == stopper) {
    return NULL;
  }
  if (c == '\'' || c == '"' || c == '`') {
    openC = '\0';
    closeC = c;
  } else if (extended && c == '{') {
    openC = c;
    closeC = '}';
  } else {
    i = 0;
    while (i < LENGTH-1 && (c = (*peekc)()) && ! isspace(c) && c != stopper) {
      if (c == '\\') {
        (void)(*readc)();
        word[i++] = QuoteBackOf(peekc, readc);
      } else {
        word[i++] = (*readc)();
      }
    }
    word[i] = '\0';
    return word;
  }
  i = 0;
  nest = 0;
  (void)(*readc)();
  while (i < LENGTH-1 && (c = (*readc)()) && (nest || c != closeC)) {
    if (c == '\\') {
      word[i++] = QuoteBackOf(peekc, readc);
    } else {
      word[i++] = c;
    }
    if (c == openC) {
      nest++;
    } else if (c == closeC) {
      nest--;
    }
  }
  word[i] = '\0';
  return word;
}

char *QuotePath(path)
char *path;
{
  int len;
  char *s;
  char *logDir;
  char userName[MAXPATHLEN];
  static char newPath[MAXPATHLEN];

  if (path[0] == '/') {
    return path;
  }

  if (path[0] != '~') {
absolute:
    newPath[0] = '\0';
    s = KshGetEnv("PWD");
    if (s) {
      strcpy(newPath, s);
    }
    strcat(newPath, "/");
    strcat(newPath, path);
    return newPath;
  }

  if (path[1] == '/') {
    newPath[0] = '\0';
    s = KshGetEnv("HOME");
    if (s) {
      strcpy(newPath, s);
    }
    strcat(newPath, path+1);
    return newPath;
  }

  len = strcspn(path+1, "/");
  strncpy(userName, path+1, (size_t)len);
  userName[len] = '\0';
  logDir = KshLogDir(userName);

  if (! logDir) {
    goto absolute;
  }

  strcpy(newPath, logDir);
  strcat(newPath, path+1+len);
  return newPath;
}


/*****************************************************************************/
