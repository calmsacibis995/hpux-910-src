/*
static char rcsId[] =
  "@(#) $Header: buffer.c,v 66.7 91/01/01 15:08:09 rpt Exp $";
static char copyright[] =
  "Copyright (c) HP, 1989";
*/


#include <sys/types.h>
#include <unistd.h>

#include "buffer.h"


#define TRUE   1
#define FALSE  0


static int readFd;
static int readEOF;
static char readBuffer[4096];
static char *readNext = readBuffer;
static char *readLimit = readBuffer;

static int writeFd;
static char writeBuffer[4096];
static char *writeNext = writeBuffer;


/*****************************************************************************/

int bufferLine;
int bufferChar;

int BufferPeekChar()
{
  int len;

  if (readNext == readLimit && ! readEOF) {
    len = read(readFd, readBuffer, sizeof(readBuffer));
    if (len < sizeof(readBuffer)) {
      if (len < 0) {
        len = 0;
      }
      readBuffer[len] = '\0';
      readEOF = TRUE;
    }
    readNext = readBuffer;
    readLimit = readBuffer+len;
  }
  return *readNext;
}

int BufferReadChar()
{
  int c;

  c = BufferPeekChar();
  if (c) {
    readNext++;
    bufferChar++;
    if (c == '\n') {
      bufferLine++;
      bufferChar = 1;
    }
  }
  return c;
}

off_t BufferReadOffset()
{
  off_t offset;

  offset = lseek(readFd, (off_t)0, SEEK_CUR);
  return offset - (readLimit-readNext);
}

int BufferReadFd(fd)
int fd;
{
  int oldFd;

  oldFd = readFd;
  readFd = fd;
  readEOF = FALSE;
  readNext = readBuffer;
  readLimit = readBuffer;
  bufferLine = 1;
  bufferChar = 1;
  return oldFd;
}

/*****************************************************************************/

void BufferWriteFlush()
{
  if (writeNext-writeBuffer) {
    write(writeFd, writeBuffer, (unsigned)(writeNext-writeBuffer));
  }
  writeNext = writeBuffer;
}

void BufferWriteChar(c)
int c;
{
  *writeNext++ = c;
  if (writeNext == writeBuffer+sizeof(writeBuffer)) {
    BufferWriteFlush();
  }
}

void BufferWriteString(s)
char *s;
{
  while (*s) {
    BufferWriteChar(*s++);
  }
}

int BufferWriteFd(fd)
int fd;
{
  int oldFd;

  oldFd = writeFd;
  BufferWriteFlush();
  writeFd = fd;
  return oldFd;
}

/*****************************************************************************/
