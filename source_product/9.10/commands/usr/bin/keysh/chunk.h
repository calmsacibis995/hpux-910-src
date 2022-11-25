/* RCS ID: @(#) $Header: chunk.h,v 66.3 90/09/26 22:10:58 rpt Exp $ */
/* Copyright (c) HP, 1989 */

#ifndef CHUNKincluded
#define CHUNKincluded

#include "linklist.h"


/* local use only */
#define CHIP 960

/* local use only */
typedef struct Chip {
  Link link;
  int size;
  int index;
} Chip;

typedef struct Chunk {
  Link base;
  Chip *chip;
} Chunk;

extern Chunk *ChunkCreate();
extern char *ChunkMalloc(/* Chunk *chunk, int size */);
extern char *ChunkCalloc(/* Chunk *chunk, int size */);
extern char *ChunkString(/* Chunk *chunk, char *s */);
extern void ChunkFree(/* Chunk *chunk, char *p */);
extern void ChunkEmpty(/* Chunk *chunk */);
extern void ChunkDestroy(/* Chunk *chunk */);

#endif
