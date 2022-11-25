/* @(#) $Revision: 66.28 $ */

#define _3C_COMPATIBILITY
#ifdef __hp9000s300
#define _ALIGN_LARGE_BLOCKS
#endif

/*

		New Malloc - Memory allocator

		by Edgar Circenis, HP, UDL, 8/89

BLOCK STRUCTURE

  +-+-+-+----------------+-+
  |s| | |                | |
  |i| | |                |&|
  |z|p|n|                |h|
  |e|r|e|   block data   |e|
  |-|e|x|                |a|
  |F|v|t|                |d|
  |-| | |                |e|
  |U| | |                |r|
  +-+-+-+----------------+-+
  
  The basic block data structure is as above.
  
  size: size of block in bytes (includes overhead).
  F: if set, previous block is free. (bit 1 of size field)
  U: if set, current block is in use. (bit 0 of size field)
  prev: pointer to previous block's size field.
  next: pointer to next block's size field.
  &header: pointer to current blocks' size field.

  When a block is allocated, only the size field is used.  When a block is
  freed, it is inserted into the linked list of free blocks, its &header field
  is set to point to the start of the block, and the F bit of the next block is
  set to indicated that the block preceeding it is free.
  
  With the _DONT_MUNGE_FREE_BLOCKS #define turned on, the next, prev, and
  &header fields are not part of the block and count as additional overhead.
  With this mode off, the fields are incorporated into the data space.

  NOTE: When reading this code, keep in mind that (header *)ptr+1 is the same
        as (int)ptr+4.

LISTS

  Free blocks are kept in a doubly linked list terminated by NULL on both
  ends.  The list header is freelist.

  In addition to this list, all blocks can be accessed in address order by
  using the size field of the block header.  The address of a block plus
  the size of the block yields the address of the next block.  All blocks
  have size which are 0 mod 4 to allow the use of the two least significant
  bits in the field.

MALLOC

  The malloc function first searches the free block list attempting to find
  a block of suitable size (first fit) to fulfill the request.  If a block
  is found and is large enough to be split into two blocks, this is done.
  If no block can be found, malloc increases the size of the arena, maintaining
  page alignment of the brk value and returns a block from the newly acquired
  memory.

FREE

  The free function takes the block passed to it and clears the used bit.
  Then, it attempts to coalesce the block with the previous and next blocks
  (in that order).  One of the following scenarios is possible:

  a) No coalescing possible: insert block at head of free list.

  b) Coalesced with previous block: block is absorbed into previous block --
  insertion into free list is not necessary.

  c) Coalesced with next block: next block is absorbed into current block.
  Free list links are updated.

  d) Coalesced with both previous and next blocks: block is absorbed into
  previous block.  Then, next block is removed from free list and
  also absorbed into previous block.

REALLOC

  The realloc function either increases or decreases the size of a block.

  DECREASE: The leftover part of the block, if large enough is coalesced
  with the following block (if free) and inserted into the free list.
  INCREASE: First, a check is made to see if coalescing with a following free
  block will create a large enough block.  If this doesn't work, malloc is
  called to allocate a suitable block.

  After a block is created, data is copied to the new block.  The amount of
  data copied is the lesser of the original and new size.

USER SBRK

  A dummy block header marked used is kept at the end of the arena in case
  the user decides to increase the brk value on his own.

  If this happens, it is detected the next time malloc attempts to increase
  the size of the arena.  Malloc correctly sets the size of the psuedo-block
  and leaves is marked used.  Then, the arena is expanded past the end of
  user-allocated memory and another dummy header is set up.

  If no "brk funniness" has taken place, the original dummy header simply
  becomes part of a new free block and another is set up at the new end
  of the arena.

COALESCE AT FREE TIME

  Blocks are coalesced at free time to eliminate free list fragmentation and
  the evils it brings about.  Coalescing at free time ensures that the free
  list is never fragmented.  This results in larger (coalesced) and fewer
  free blocks, allowing for a shorter and more efficient search when looking
  for a new block.  Also, since there are fewer free blocks, the system isn't
  hopping through memory and causing as many page faults.

  By storing the state (free or used) of the previous block in the current
  block and having the last word of a free block point to itself, it is
  possible to implement coalescing of free blocks in constant time.

KNOWN PROBLEMS

  The ugly practice of realloc after free is not supported by this code.


EXPLANATION OF #defines:

DEBUG:
	Enable assertions during calls to malloc, free, and realloc.

LONGDEBUG:
	Enable full arena checking after each malloc, free, and realloc call.

PDEBUG:
	Enable printing of arena after each malloc, free, and realloc call.

_3X_COMPATIBILITY:
	Enable partial 7.0 malloc(3X) compatibility.

_3C_COMPATIBILITY:
	Enable 7.0 malloc(3C) compatibility.

_NO_LUGGAGE:
	This gives you the fastest version of this algorithm by ignoring
	the compatibility issues addressed below.

_DONT_MUNGE_FREE_BLOCKS:
	The old malloc(3C) does not change the contents of a free block.
	The cost of this feature is 12 bytes per block overhead.

_FREE_TWICE:
	The old malloc(3C) allows you to free blocks twice.  This feature
	has little associated cost.  If this feature is turned off, blocks
	may NOT be freed twice!  Malloc will break if this is attempted.

_ZERO_RETURNS_NULL:
	In the malloc(3X) package, the malloc() function returns a NULL
	pointer if passed zero as its argument.  There is no cost associated
	with this feature.  Enabling this will decrease run time if zero
	length blocks are allocated.

_REALLOC_AFTER_FREE:
	In both malloc(3C) and malloc(3X), there are provisions for realloc
	of a block freed since the last call of malloc, realloc, or calloc.
	If this option is enabled, provisions will be made to allow this.
	The size of the realloc algorithm will increase, and extra code will
	be added to malloc and free as well.

_PASCAL_KLUDGE:
	Pascal needs the global variable asm_mhfl (for some brain damaged
	reason) to denote whether memory management has taken place.

_ALIGN_LARGE_BLOCKS:
	For kernel memory-to-memory copies, it is sometimes desirable to
	have blocks aligned on boundaries other than ALIGNSIZE.  This is
	done because of instructions like the 68040 mov16 and the PA-RISC
	quad store instruction.  The bcopy performance improvement is on
	in the range of 13-100% for aligned blocks in user memory.  The
	aligned blocks are usually buffers allocated for read() or fread()
	calls.

*/

#ifdef PDEBUG
#	ifndef LONGDEBUG
#	define LONGDEBUG
#	endif
#endif

#ifdef LONGDEBUG
#	ifndef DEBUG
#	define DEBUG
#	endif
#endif

#ifdef _NO_LUGGAGE
#	undef _3X_COMPATIBILITY
#	undef _3C_COMPATIBILITY
#	define _ZERO_RETURNS_NULL
#endif

#ifdef _3X_COMPATIBILITY
#	define _FREE_TWICE
#	define _ZERO_RETURNS_NULL
#	define _REALLOC_AFTER_FREE
#	define _PASCAL_KLUDGE
#endif

#ifdef _3C_COMPATIBILITY
#	define _DONT_MUNGE_FREE_BLOCKS
#	define _FREE_TWICE
#	define _REALLOC_AFTER_FREE
#	define _PASCAL_KLUDGE
#endif

/*
   Until the idea of block overhead is better coded, we need this kind
   of a kludge to prevent people from using feature test macros that
   don't work independent of each other.
*/
#ifdef _ALIGN_LARGE_BLOCKS
#	ifndef _DONT_MUNGE_FREE_BLOCKS
#		define _DONT_MUNGE_FREE_BLOCKS
#	endif
#endif


#ifdef DEBUG
#include <assert.h>
#else /* DEBUG */
#define assert(EX)
#endif /* DEBUG */

#include <errno.h>

/*  Lines added to clean up ANSI/POSIX namespace */
#ifdef _NAMESPACE_CLEAN
#   ifdef   _ANSIC_CLEAN
#       define malloc _malloc
#       define free _free
#       define realloc _realloc
#   endif  /* _ANSIC_CLEAN */
#   define sbrk _sbrk
#   define brk __brk
#   define memcpy _memcpy
#   if defined(__hp9000s300) && defined(_PASCAL_KLUDGE)
#      define asm_mhfl _asm_mhfl
#   endif /* __hp9000s300 */
#endif

/* debug defines for quickcheck routine */
#ifdef DEBUG
#	define Q_ENTRY   0
#	define Q_MALLOC  1
#	define Q_FREE    2
#	define Q_REALLOC 3
#endif /* DEBUG */

#define NULL	(header *)0
#define ALLOCSIZE 4096	/* amount to increase brk value by */
#define QUARTERSIZE (ALLOCSIZE/4)	/* don't want to waste memory */
#define MINSIZE 16	/* absolute minimum block size */

/* The series 800 needs malloc()'ed blocks aligned on 8 byte boundaries
   since the s800 floating point hardware assumes 8 byte alignment for
   floating point data types (doubles).  The problem here is that an entire
   block consists of the block header and block data.  The block header is
   aligned to ALIGNSIZE.  Because of this, FUDGE_FACTOR bytes are added in
   to get the block itself aligned on ALIGNSIZE byte boundaries.	   */
#ifdef __hp9000s800
#define ALIGNSIZE 8	/* S800 needs 8 byte alignment			   */
#define FUDGE_FACTOR 4	/* block header is offset 4 bytes from alignment   */
#else /* __hp9000s800 */
#define ALIGNSIZE 4	/* blocks must align on 4 byte boundary		   */
#define FUDGE_FACTOR 0	/* block header is aligned			   */
#endif /* __hp9000s800 */

#ifdef _ALIGN_LARGE_BLOCKS
/* For 68040, using the mov16 instruction doubles bcopy performance from
   kernel to user memory buffers.  For PA-RISC, using the quad-store
   instruction increases bcopy performance by 13%.  In each case, the
   user buffer must be 16-byte aligned.					   */
#define P_ALIGN 16	/* MUST BE AT LEAST 16!!! */
#define ALIGNMENT_OFFSET(p) (P_ALIGN-((int)(p+3)&(P_ALIGN-1)))
#define PROPERLY_ALIGNED(p) (!((int)(p+3)&(P_ALIGN-1)))
#endif /* _ALIGN_LARGE_BLOCKS */

#define FREE	0x2	/* bit which signifies that previous block is free */
#define USED	0x1	/* bit which signifies that current block is used  */
#define MASK	(FREE|USED)		/* mask for size bits of header    */
#define SIZE(x)		(*(x)&~MASK)	/* returns size of block in bytes  */
					/* this includes the size of the   */
					/* header!			   */
#define FLAGS(x)	(*(x)&MASK)	/* returns FREE and USED bits	   */

/* bit manipulation for USED bit */
#define TST_USED(x)	(*(x)&USED)
#define CLR_USED(x)	(*(x)&= ~USED)
#define SET_USED(x)	(*(x)|= USED)

/* bit manipulation for FREE bit */
#define TST_FREE(x)	(*(x)&FREE)
#define CLR_FREE(x)	(*(x)&= ~FREE)
#define SET_FREE(x)	(*(x)|= FREE)

/* Pointer to previous free block in block list */
#define PREV_FREE(x)	(*(header **)((x)+1))
/* Pointer to next free block in block list */
#define NEXT_FREE(x)	(*(header **)((x)+2))

/* Pointer to last word of current block */
#define LAST_WORD(x)	(*(header **)((header)(x)+SIZE(x)-sizeof(header)))

/* Pointer to header of free block preceeding current block */
#define PREV_BLOCK(x)	((header *)(*(header *)((x)-1)))
/* Pointer to next block in memory (not necessarily free) */
#define NEXT_BLOCK(x)	(header *)((header)(x)+SIZE(x))

/* This macro will insert a previously used block at the head of the freelist */
#define ADD_TO_FREELIST(x) \
		if (NEXT_FREE(x)=freelist) \
			/* must re-link backward pointers */ \
			PREV_FREE(freelist)=x; \
		PREV_FREE(x) = NULL; \
		freelist = x;

/* This macro will remove a free block from the freelist */
#define REMOVE_FROM_FREELIST(x)	\
		if (PREV_FREE(x))	/* not first block in list */ \
			NEXT_FREE(PREV_FREE(x)) = NEXT_FREE(x); \
		else			/* first block in list */ \
			freelist = NEXT_FREE(x); \
		if (NEXT_FREE(x))	/* not last in list */ \
			PREV_FREE(NEXT_FREE(x)) = PREV_FREE(x); \
		/* must clear FREE bit on next block */ \
		CLR_FREE(NEXT_BLOCK(x));

/* This macro will remove a free block from the freelist */
/* but, it won't clear the FREE bit on the next block	 */
#define REMOVE_FROM_FREELIST2(x)	\
		if (PREV_FREE(x))	/* not first block in list */ \
			NEXT_FREE(PREV_FREE(x)) = NEXT_FREE(x); \
		else			/* first block in list */ \
			freelist = NEXT_FREE(x); \
		if (NEXT_FREE(x))	/* not last in list */ \
			PREV_FREE(NEXT_FREE(x)) = PREV_FREE(x);

/* This macro will relink block x using pointers in block y */
#define RELINK(x,y) \
		if (NEXT_FREE(x) = NEXT_FREE(y)) \
			/* re-link backward pointer */ \
			PREV_FREE(NEXT_FREE(x))=x; \
		if (PREV_FREE(x) = PREV_FREE(y)) \
			/* re-link forward pointer */ \
			NEXT_FREE(PREV_FREE(x)) = x; \
		else \
			/* x is now first in list */ \
			freelist = x;

typedef unsigned int header;	/* must be four bytes */

static header *freelist;	/* head of free block list */
static header *arenaend;	/* pointer to last (dummy) header in arena */
static header *arenastart;	/* pointer to first block in arena */
static header *lastblock;	/* pointer to last block in arena */
static header lastbrk;		/* keeps track of brk value */
static int    no_expand=0;	/* flag to inhibit expansion in malloc */
extern header _curbrk;		/* brk value set by brk() and sbrk() */
static header *grow_arena();	/* function to grow arena */

#if defined(__hp9000s300) && defined(_PASCAL_KLUDGE)
#   ifdef _NAMESPACE_CLEAN
#       undef  asm_mhfl
#       pragma _HP_SECONDARY_DEF _asm_mhfl asm_mhfl
#       define asm_mhfl _asm_mhfl
#   endif /* _NAMESPACE_CLEAN */
unsigned char asm_mhfl = 0; /* for Pascal kludge -- first time flag */
#endif /* __hp9000s300 */


#if defined(_NAMESPACE_CLEAN) && defined(_ANSIC_CLEAN)
#   undef  malloc
#   pragma _HP_SECONDARY_DEF _malloc malloc
#   define malloc _malloc
#endif /* _NAMESPACE_CLEAN && _ANSIC_CLEAN */

/*
 * Malloc() -- allocate a block of memory out of the arena
 *	Malloc returns a pointer to a block of the size requested.
 *	Malloc(0) returns a NULL pointer or allocated a zero size block
 *	based on whether _ZERO_RETURNS_NULL is defined.
 *	If malloc cannot allocate memory requested, it returns NULL.
 */
void *
malloc(nbytes)
unsigned int nbytes;
{
	header s;
	header *allocptr,*tempblk;
#ifdef _ALIGN_LARGE_BLOCKS
	int xbytes;	/* extra bytes needed for special alignment */
#endif

	assert(quickcheck(Q_ENTRY));
	assert(lastblock==NULL || NEXT_BLOCK(lastblock)==arenaend);
#ifdef _ZERO_RETURNS_NULL
	/* libmalloc.a malloc() returns NULL for zero size blocks */
	if (nbytes==0) return(NULL);
#endif

	if (!arenaend) {	/* first time through */
#ifdef _ALIGN_LARGE_BLOCKS
		arenastart=arenaend=(header *)(((_curbrk+ALIGNSIZE-1)&~(ALIGNSIZE-1))+P_ALIGN-3*sizeof(header));
#else
		arenastart=arenaend=(header *)(((_curbrk+ALIGNSIZE-1)&~(ALIGNSIZE-1))+FUDGE_FACTOR);
#endif /* _ALIGN_LARGE_BLOCKS */
		if (brk((char *)arenaend+sizeof(header)) == -1)
			return(arenaend=NULL);
		*arenaend=USED;	/* mark dummy as used */
		/* keep track of brk value */
		lastbrk = _curbrk;
#if defined(__hp9000s300) && defined(_PASCAL_KLUDGE)
		asm_mhfl = 1;	/* Pascal kludge */
#endif
	}

	/* make sure nbytes is 0 mod ALIGNSIZE. Also, add in header size  */
#ifdef _DONT_MUNGE_FREE_BLOCKS
	/* allocate extra space for links (i.e. 16 bytes of overhead) */
	nbytes = (nbytes+4*sizeof(header)+ALIGNSIZE-1)&~(ALIGNSIZE-1);
#else
	/* 4 bytes of overhead with minimum block size of 16 bytes */
	nbytes = (nbytes+sizeof(header)+ALIGNSIZE-1)&~(ALIGNSIZE-1);
	nbytes = (nbytes<MINSIZE)?MINSIZE:nbytes;
#endif

	allocptr = freelist;
loop:
#ifdef _ALIGN_LARGE_BLOCKS
	if (nbytes>1024+4*sizeof(header))
            while (allocptr) {
		if ((SIZE(allocptr))>=nbytes) {
		    if (PROPERLY_ALIGNED(allocptr))
		    	/* If block is aligned properly, we use it */
			goto found_aligned;
		    else {
			/* block is not properly aligned, we will create a
			   small free block before the real block is
			   allocated.  This free block will be of the proper
			   size to correctly align the following block */
			if ((SIZE(allocptr))>=nbytes+ALIGNMENT_OFFSET(allocptr)
				+4*sizeof(header))
			    goto found_aligned;
			else
			    allocptr = NEXT_FREE(allocptr);
		    }
		} else
			allocptr = NEXT_FREE(allocptr);	/* next free block */
	    }
	else
#endif /* _ALIGN_LARGE_BLOCKS */
            while (allocptr) {
		/* get size of block and compare to what's needed */
		if ((SIZE(allocptr))>=nbytes)
			goto found;
		else
			allocptr = NEXT_FREE(allocptr);	/* next free block */
	    }

        /* didn't find a block, must allocate more memory */

	if (no_expand || !(allocptr=grow_arena(nbytes))) /* did we fail? */
		return((void *)NULL);

	/* drop through with big enough block */

#ifdef _ALIGN_LARGE_BLOCKS
found_aligned:
	if (nbytes>1024+4*sizeof(header) && !PROPERLY_ALIGNED(allocptr)) {
	    xbytes=ALIGNMENT_OFFSET(allocptr);
	    /* NOTE: since we are just making an existing free block a
	       bit smaller, the forward and backward links don't change */
	    s=SIZE(allocptr)-(xbytes+4*sizeof(header));
	    /* This is a free block, and the previous block is used,
	       so we only need to set the size */
	    *allocptr=((xbytes+4*sizeof(header)));
	    LAST_WORD(allocptr) = allocptr;
	    allocptr=NEXT_BLOCK(allocptr);
	    *allocptr=(s|FREE);
	    ADD_TO_FREELIST(allocptr);
	    if (allocptr>lastblock) lastblock = allocptr;
	    /* NOTE: last word not set because we do it later */
	    assert(PROPERLY_ALIGNED(allocptr));
	    assert(SIZE(allocptr) >= nbytes);
	}
#endif /* _ALIGN_LARGE_BLOCKS */
found:  /* found a block */

	
	if (SIZE(allocptr) >= nbytes+MINSIZE) { /* split into 2 blocks */
		s = SIZE(allocptr)-nbytes;
		/* mark size of block and USED bit */
#ifdef _ALIGN_LARGE_BLOCKS
		*allocptr = (nbytes|USED|(*allocptr&FREE));
#else
		*allocptr = (nbytes|USED);
#endif /* _ALIGN_LARGE_BLOCKS */
		/* tempblk is newly split-off free block */
		tempblk = NEXT_BLOCK(allocptr);
		/* re-link freelist using tempblk instead of allocptr */
		RELINK(tempblk,allocptr);
		/* store size of block */
		*tempblk = s;
		/* last word points to start of block */
		LAST_WORD(tempblk) = tempblk;
		if (tempblk>lastblock) lastblock = tempblk;
	} else {			/* use entire block */
		/* remove block from freelist */
		REMOVE_FROM_FREELIST(allocptr);
		/* mark block as used */
		SET_USED(allocptr);
	}
	assert((tempblk=NEXT_BLOCK(allocptr))==arenaend || !TST_FREE(tempblk));
	assert(lastblock==NULL || NEXT_BLOCK(lastblock)==arenaend);
	assert(TST_USED(allocptr));
	assert(allocptr>=arenastart);
	assert(quickcheck(Q_MALLOC));
#ifdef _ALIGN_LARGE_BLOCKS
	assert(nbytes<1024+4*sizeof(header)||PROPERLY_ALIGNED(allocptr));
#endif
#ifdef _DONT_MUNGE_FREE_BLOCKS
	return(allocptr+3);	/* skip links, return beginning of data */
#else
	return(allocptr+1);	/* return beginning of data */
#endif
}


#if defined(_NAMESPACE_CLEAN) && defined(_ANSIC_CLEAN)
#   undef  free
#   pragma _HP_SECONDARY_DEF _free free
#   define free _free
#endif /* _NAMESPACE_CLEAN && _ANSIC_CLEAN */
/*
 * Free() -- Free a block allocated by malloc().
 *	Free(NULL) does nothing.
 *	If the pointer passed to free() is not a pointer to a block
 *	 allocated by malloc(), or points to a block that had previously
 *	 been freed, the linked list structure will be corrupted.
 */
void
free(p)
void *p;
{
	header *allocptr;
	header *nextblk;
	header s;
	int coalesced=0;

	assert(quickcheck(Q_ENTRY));
	assert(lastblock==NULL || NEXT_BLOCK(lastblock)==arenaend);
	if (p==(char *)NULL)	/* free() of NULL pointer does nothing */
		return;
	assert((header)p%4 == 0);
#ifdef _DONT_MUNGE_FREE_BLOCKS
	allocptr=(header *)p-3;
#else
	allocptr=(header *)p-1;
#endif
	assert(allocptr>=arenastart);
	assert(allocptr<=(header *)_curbrk);
#ifdef _FREE_TWICE
	if (!TST_USED(allocptr)) return;	/* return if already free */
#else
	assert(TST_USED(allocptr));
#endif
	CLR_USED(allocptr);	/* so that we can detect multiple free's
				   even after block has been coalesced */
#ifdef _REALLOC_AFTER_FREE
	/* even free blocks within free blocks need this info */
	LAST_WORD(allocptr)=allocptr;
	SET_FREE(NEXT_BLOCK(allocptr));	/* set free bit so we can find head */
#endif
	/* get size of block to be freed */
	s=SIZE(allocptr);
	if (TST_FREE(allocptr)) {	/* coalesce with previous block */
		if (allocptr==lastblock) { /* need to reset lastblock */
			/* get previous block's header */
			lastblock = allocptr = PREV_BLOCK(allocptr);
		} else
			/* get previous block's header */
			allocptr = PREV_BLOCK(allocptr);
		/* add size of previous block */
		s += SIZE(allocptr);
		*allocptr = s;
		/* set FREE bit on next block */
		SET_FREE(NEXT_BLOCK(allocptr));
		coalesced++;		/* block is part of free list now */
		assert(lastblock==NULL || NEXT_BLOCK(lastblock)==arenaend);
	}
	if (!TST_USED(nextblk=NEXT_BLOCK(allocptr))) {
		if (lastblock == nextblk)	/* reset lastblock */
			lastblock = allocptr;
		if (coalesced) { /* already coalesced with previous block */
			/* remove nextblk from free list */
			/* but, don't clear the FREE bit on next block */
			REMOVE_FROM_FREELIST2(nextblk);
		} else {	/* forward coalesce only */
			/* re-link freelist using allocptr instead of nextblk */
			RELINK(allocptr,nextblk);
			coalesced++;
		}
		/* store new size (previous block will always be in use) */
		*allocptr = s+SIZE(nextblk);
		assert(lastblock==NULL || NEXT_BLOCK(lastblock)==arenaend);
	}
	if (!coalesced) {
		/* insert at head of free list */
		ADD_TO_FREELIST(allocptr);
		/* set FREE bit on next block */
		SET_FREE(NEXT_BLOCK(allocptr));
	}
	/* last word of free block contains addr */
	LAST_WORD(allocptr) = allocptr;
	assert(!TST_USED(allocptr));
#ifdef _DONT_MUNGE_FREE_BLOCKS
	assert(!TST_USED((header *)p-3));  /* input block marked free */
#else
	assert(!TST_USED((header *)p-1));
#endif
	assert((nextblk=NEXT_BLOCK(allocptr))==arenaend || TST_FREE(nextblk));
	assert(lastblock==NULL || NEXT_BLOCK(lastblock)==arenaend);
	assert(quickcheck(Q_FREE));
}


#if defined(_NAMESPACE_CLEAN) && defined(_ANSIC_CLEAN)
#   undef  realloc
#   pragma _HP_SECONDARY_DEF _realloc realloc
#   define realloc _realloc
#endif /* _NAMESPACE_CLEAN && _ANSIC_CLEAN */
/*
 * Realloc() -- change the size of a block allocated by malloc().
 *	Realloc(NULL,n) will do a malloc(n).
 *	Realloc(p,0) will do a free(p) and return NULL.
 *	Realloc(p,size+n) will increase the size of the block pointed
 *	 to by _p_ by _n_ bytes and will preserve the first _size_ bytes
 *	 of data.
 *	Realloc(p,size-n) will decrease the size of the block pointed
 *	 to by _p_ by _n_ bytes and will preserve any remaining bytes.
 */
void *
realloc(p,n)
void *p;
header n;
{
	void *ptr;
	header s,nbytes,newsize;
	header *allocptr,*tempblk,*newfree;

	assert(quickcheck(Q_ENTRY));
	assert(lastblock==NULL || NEXT_BLOCK(lastblock)==arenaend);
	/* If the input pointer is NULL, then malloc nbytes */
	if (p==(char *)NULL)
		return(malloc(n));
	/* If the input pointer is not NULL and nbytes is 0, free the pointer */
	if (n==0) {
		free(p);
		return((void *)NULL);
	}
	assert((header)p%ALIGNSIZE == 0);
#ifdef _DONT_MUNGE_FREE_BLOCKS
	allocptr=(header *)p-3;
#else
	allocptr=(header *)p-1;
#endif
#ifdef _REALLOC_AFTER_FREE
	if (!TST_USED(allocptr)) {  /* need to uncoalesce a free block */
		if (!TST_FREE(allocptr)) {  /* block is start of free block */
			REMOVE_FROM_FREELIST(allocptr);
			SET_USED(allocptr);
		} else { /* block embedded in free block, need to split */
			/* find the head of the free block */
			tempblk=PREV_BLOCK(allocptr);
			while (TST_FREE(tempblk)) {
				tempblk=PREV_BLOCK(tempblk);
			}
			s=SIZE(tempblk);
			/* set size of split off block, mark correctly */
			*allocptr=(s-((header)allocptr-(header)tempblk))|USED|FREE;
			/* reset size of remaining free block */
			*tempblk=(s-SIZE(allocptr));
		}
	}
#endif
	assert(allocptr>=arenastart);
	assert(TST_USED(allocptr));
#ifdef _DONT_MUNGE_FREE_BLOCKS
	/* add space for links (16 bytes of overhead) */
	nbytes = (n+4*sizeof(header)+ALIGNSIZE-1)&~(ALIGNSIZE-1);
#else
	/* 4 bytes overhead with minimum block size of 16 bytes */
	nbytes = (n+sizeof(header)+ALIGNSIZE-1)&~(ALIGNSIZE-1);
	nbytes = (nbytes<MINSIZE)?MINSIZE:nbytes;
#endif
	s = SIZE(allocptr);
	if (nbytes > s) {	/* increasing size of block */
increase:
		if ((tempblk=NEXT_BLOCK(allocptr))!=arenaend
			&& !TST_USED(tempblk)
			&& (newsize=s+SIZE(tempblk)) >= nbytes) {
			/* realloc into next block */
			if (newsize-nbytes>=MINSIZE) { /* split in two */
				/* set new size of used block */
				*allocptr=nbytes|FLAGS(allocptr);
				newfree = NEXT_BLOCK(allocptr);
				RELINK(newfree,tempblk);
				/* save size of split-off block */
				*newfree = newsize-nbytes;
				LAST_WORD(newfree) = newfree;
				if (newfree>lastblock) lastblock=newfree;
			} else	{ /* use entire free block */
				*allocptr=newsize|FLAGS(allocptr);
				/* remove tempblk from free list */
				REMOVE_FROM_FREELIST(tempblk);
				if (tempblk == lastblock ) lastblock=allocptr;
			}
			assert(TST_USED(allocptr));
			assert(allocptr>=arenastart);
			assert(lastblock==NULL || NEXT_BLOCK(lastblock)==arenaend);
			assert(quickcheck(Q_REALLOC));
			return((void *)p);	/* same block returned */

		} else if ((allocptr==lastblock || ((tempblk=NEXT_BLOCK(allocptr))==lastblock && !TST_USED(tempblk))) && lastbrk==_curbrk) {
		/* everything past the current block is free, so if we can't
		   get a block from malloc, we can grow the arena and expand
		   the current block */
			no_expand=1;
			if (ptr=malloc(n)) { /* does a suitable block exist? */
				no_expand=0;
				goto copy;   /* yes */
			} else {
				no_expand=0; /* no */
				/* if next block is free, merge blocks */
				if (!TST_USED(tempblk=NEXT_BLOCK(allocptr))) {
					s+=SIZE(tempblk);
					*allocptr=s|FLAGS(allocptr);
					/* remove tempblk from free list */
					REMOVE_FROM_FREELIST(tempblk);
					if (tempblk == lastblock )
						lastblock=allocptr;
				}
				/* at this point, allocptr is the last block
				   in the arena */
				if (!(tempblk=grow_arena(nbytes-s)))
					return((void *)NULL);
				goto increase;
			}
		} else if (ptr=malloc(n)) { /* allocate more memory */
copy:
#ifdef _3C_COMPATIBILITY
			/* don't copy links! */
			(void)memcpy(ptr,p,s-4*sizeof(header));
#else
			(void)memcpy(ptr,p,s-sizeof(header));
#endif
			free(p);
			return((void *)ptr);

		} else	/* all else fails */
			return((void *)NULL);

	} else {		/* decrease size of block */
		if (s >= nbytes+MINSIZE) { /* split off extra part of block */
			/* point to next (already existing) block */
			tempblk = NEXT_BLOCK(allocptr);
			/* set new size of used block */
			*allocptr = nbytes|FLAGS(allocptr);
			/* point to new free block */
			newfree = NEXT_BLOCK(allocptr);
			/* if next block is already free... */
			if (!TST_USED(tempblk)) {
				/* calculate size for new free block */
				*newfree = s-nbytes+SIZE(tempblk);
				/* coalesce with next free block */
				RELINK(newfree,tempblk);
				if (tempblk == lastblock) lastblock = newfree;
			} else { /* next block is used or doesn't exist */
				/* store size of new free block */
				*newfree = s-nbytes;
				/* insert block at head of free list */
				ADD_TO_FREELIST(newfree);
				/* set FREE bit of next block */
				SET_FREE(tempblk);
				if (tempblk==arenaend)/* we are last block */
					lastblock = newfree;
			}
			/* last word of block points to beginning of block */
			LAST_WORD(newfree) = newfree;
			assert(allocptr>=arenastart);
			assert(TST_USED(allocptr));
			assert(lastblock==NULL || NEXT_BLOCK(lastblock)==arenaend);
			}
		assert(quickcheck(Q_REALLOC));
		return((void *)p);
	}
}


/*
 * grow_arena() -- we have run out of room and need to increase
 *	the size of the data segment.  Increase by at least nbytes
 *	bytes and return a pointer to the last free block in the
 *	new arena.
 */
static header *
grow_arena(nbytes)
header nbytes;
{
	header s,brkinc;
	header *allocptr;

	assert(TST_USED(arenaend));
	/* check to see if user has done sbrk */
	if (lastbrk != _curbrk) {
		/* if user released our memory, complain! */
		/* malloc'ed space is trashed */
		if (lastbrk > _curbrk) {
			errno=EINVAL;	/* memory detectably corrupted */
			return(NULL);
		}
		/* calculate the amount of memory allocated by user */
		/* round up to align correctly */
		s = (((_curbrk+ALIGNSIZE-1)&~(ALIGNSIZE-1))-lastbrk)|FUDGE_FACTOR;
		/* save size and mark block as used */
		*arenaend = (s+sizeof(header))|FLAGS(arenaend)|USED;
		/* calculate new brk value rounding up to a page boundary */
		brkinc = (lastbrk+s+nbytes+sizeof(header)+ALLOCSIZE-1)&~(ALLOCSIZE-1);
		if (brk(brkinc) == -1) /* get more memory */
			return(NULL);	/* failed to get memory */
		/* move arenaend to point to dummy header */
		arenaend=(header *)(_curbrk-sizeof(header));
		/* block before arenaend is a free block */
		/* arenaend itself is a used dummy header */
		*arenaend=FREE|USED;
		/* calculate start of new free block */
		lastblock=allocptr=(header *)(lastbrk+s);
		/* reset my brk pointer */
		lastbrk=_curbrk;
		*allocptr=(header)_curbrk-(header)allocptr-sizeof(header);
		LAST_WORD(allocptr)=allocptr;
		ADD_TO_FREELIST(allocptr);
		assert(lastblock==NULL || NEXT_BLOCK(lastblock)==arenaend);
	} else {	/* brk value didn't move */
		/* keep in mind that there is already a dummy header
		 * past the arenaend.  If we use sbrk to increase the
		 * size of the arena, this extra header will still
		 * exist. */
		/* calculate brk increment and end it on a page boundary */
		brkinc = (ALLOCSIZE<nbytes)?nbytes:ALLOCSIZE;
		brkinc = ((_curbrk+brkinc+ALLOCSIZE-1)&~(ALLOCSIZE-1))-_curbrk;
        	if (sbrk(brkinc) == -1)      /* get more memory */
                	return(NULL);           /* failed to get memory */
		/* reset our brk sentinel */
		lastbrk=_curbrk;
		assert(lastblock==NULL || NEXT_BLOCK(lastblock)==arenaend);
		if (lastblock && !TST_USED(lastblock)) {
			/* coalesce new memory with last free block */
			allocptr=lastblock;
			/* calculate size of new free block */
			*allocptr = SIZE(allocptr)+brkinc;
		} else {	/* need to create new free block out of new memory */
			/* block begins at arenaend (new last block) */
			lastblock = allocptr = arenaend;
			/* store size of block */
			*allocptr = brkinc;
			/* insert block into head of free list */
			ADD_TO_FREELIST(allocptr);
		}
		/* reset arenaend to point to dummy header */
		arenaend = (header *)((header)arenaend+brkinc);
		/* block preceeding arenaend is a free block */
		/* arenaend is a used dummy block header */
		*arenaend=FREE|USED;
		assert(lastblock==NULL || NEXT_BLOCK(lastblock)==arenaend);
		LAST_WORD(allocptr) = allocptr;
	}
	assert(TST_USED(arenaend));
	return(allocptr);	/* big free block */
}


#ifdef DEBUG
#ifdef LONGDEBUG
/*
 *  Check for the following error conditions:
 *	- incorrect links in block list
 *	- infinite loops in free list
 *	- incorrect block alignment
 *	- incorrect end of arena
 *	- continguous free blocks
 *	- incorrect use of FREE and USED bits
 */
int
quickcheck(id)
int id;
{
	header *allocptr,*tempptr,*prev;
	unsigned int free_count=0,i;

	assert(lastbrk!=_curbrk || arenaend+1 == _curbrk);
	prev=NULL;
	allocptr=arenastart;
	while (allocptr != arenaend) {
		/* check to see if block is correctly aligned */
		assert((((int)allocptr+sizeof(header))&(ALIGNSIZE-1))==0);
		/* count free blocks in block list */
		if (!TST_USED(allocptr)) free_count++;
		tempptr = NEXT_BLOCK(allocptr);
		if (allocptr>=tempptr)
			return(0);
		prev = allocptr;
		allocptr = tempptr;
		/* check for two contiguous free blocks */
		assert(!(TST_FREE(prev) && TST_FREE(allocptr)));
		/* check to see if FLAG bits make sense */
		if (TST_FREE(allocptr)) assert(!TST_USED(prev));
		if (!TST_FREE(allocptr)) assert(TST_USED(prev));
	}
	if (prev!=NULL && prev!=lastblock)
		return(0);
	/* check forward links in freelist */
	prev=NULL;
	i=0;
	for (allocptr=freelist;allocptr;allocptr=NEXT_FREE(allocptr)) {
		prev=allocptr;
		i++;
	}
	assert(i==free_count);
	/* check backward links in freelist */
	if (prev) {
		i=0;
		for (allocptr=prev;allocptr;allocptr=PREV_FREE(allocptr))
			i++;
		assert(i==free_count);
	}
	if (arenaend)
		assert(TST_USED(arenaend));
#ifdef PDEBUG
        if (id!=0)
                printf("-------------------- after ");
        switch(id) {
                case Q_MALLOC:
                        printf("malloc\n"); break;
                case Q_FREE:
                        printf("free\n"); break;
                case Q_REALLOC:
                        printf("realloc\n"); break;
        }
        if (id!=0)
                list_arena();
#endif /* PDEBUG */

	return(1);
}
#else /* LONGDEBUG */
int quickcheck() { return(1); }
#endif /* LONGDEBUG */

unsigned int num_free,size_free;
unsigned int num_used,size_used;

int
list_arena()
{
	header *allocptr,*tempptr;

	num_free=size_free=num_used=size_used=0;

	printf("%10d: arenastart\n",arenastart);
	allocptr=arenastart;
	while (allocptr != arenaend) {
		if (TST_USED(allocptr)) {
			num_used++;
			size_used += SIZE(allocptr);
			printf("%10d: used %d\n",allocptr,SIZE(allocptr));
		} else {
			num_free++;
			size_free += SIZE(allocptr);
			printf("%10d: free %d\n",allocptr,SIZE(allocptr));
		}
		tempptr = NEXT_BLOCK(allocptr);
		if (allocptr>=tempptr)
			return(0);
		allocptr = tempptr;
	}
	printf("%10d: arenaend\n",arenaend);
	printf("%10d: _curbrk\n",_curbrk);
	return(1);
}
#endif /* DEBUG */
