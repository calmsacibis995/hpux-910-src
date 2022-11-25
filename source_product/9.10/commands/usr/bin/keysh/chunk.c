/*
static char rcsId[] =
  "@(#) $Header: chunk.c,v 66.5 90/09/26 22:09:32 rpt Exp $";
static char copyright[] =
  "Copyright (c) HP, 1989";
*/


#include <assert.h>
#include <string.h>
#include <stdlib.h>

#include "linklist.h"
#include "chunk.h"


Chunk *ChunkCreate()
{
  Chip *chip;
  Chunk *chunk;

  chunk = (Chunk *)malloc(sizeof(Chunk));
  LinkCreateBase(chunk);
  chip = (Chip *)malloc(sizeof(Chip)+CHIP);
  chip->size = CHIP;
  chip->index = 0;
  LinkAddTail(chunk, chip);
  chunk->chip = chip;
  return chunk;
}

char *ChunkMalloc(chunk, size)
Chunk *chunk;
int size;
{
  char *p;
  Chip *chip;

  size = (size+sizeof(long)-1)/sizeof(long)*sizeof(long);

  if (size > chunk->chip->size - chunk->chip->index) {
    if (size <= CHIP && LinkNext(chunk->chip)) {
      chunk->chip = LinkNext(chunk->chip);
      chunk->chip->index = 0;
    } else {
      chip = (Chip *)malloc(sizeof(Chip) + ((size>CHIP)?(size):(CHIP)));
      chip->size = ((size>CHIP)?(size):(CHIP));
      chip->index = 0;
      LinkAddAfter(chunk->chip, chip);
      chunk->chip = chip;
    }
  }

  p = (char *)(chunk->chip+1) + chunk->chip->index;
  chunk->chip->index += size;
  return p;
}

char *ChunkCalloc(chunk, size)
Chunk *chunk;
int size;
{
  char *p;

  p = ChunkMalloc(chunk, size);
  memset((void *)p, '\0', (size_t)size);
  return p;
}

char *ChunkString(chunk, s)
Chunk *chunk;
char *s;
{
  char *p;

  p = ChunkMalloc(chunk, (int)strlen(s)+1);
  strcpy(p, s);
  return p;
}

#ifndef KEYSHELL
void ChunkFree(chunk, p)
Chunk *chunk;
char *p;
{
  Chip *chip;

  for (chip = LinkHead(chunk); chip; chip = LinkNext(chip)) {
    if (p >= (char *)(chip+1) && p < (char *)(chip+1) + chip->size) {
      chunk->chip = chip;
      chip->index = p - (char *)(chip+1);
      return;
    }
  }
}
#endif

void ChunkEmpty(chunk)
Chunk *chunk;
{
  chunk->chip = LinkHead(chunk);
  chunk->chip->index = 0;
}

void ChunkDestroy(chunk)
Chunk *chunk;
{
  Chip *chip;

  while (chip = LinkTail(chunk)) {
    free(LinkRemove(chip));
  }
  free((void *)chunk);
}
