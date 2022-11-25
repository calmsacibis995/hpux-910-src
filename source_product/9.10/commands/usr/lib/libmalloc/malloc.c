/* @(#) $Revision: 66.8 $ */     

#ifndef debug
#   define  NDEBUG
#endif

#include "assert.h"
#include "malloc.h"
#include "mallint.h"
#include <signal.h>

#ifdef _NAMESPACE_CLEAN   /* Fix references to get primary symbols */
#   ifdef   _ANSIC_CLEAN
#      define free    _free
#      define malloc  _malloc
#      define realloc _realloc
#   endif  /* _ANSIC_CLEAN */
#   define calloc  _calloc
#   define cfree   _cfree
#   define memcpy  _memcpy
#   define memset  _memset
#   define sbrk    _sbrk
#   define sigfillset	_sigfillset
#   define sigprocmask	_sigprocmask
#endif /* _NAMESPACE_CLEAN */

/*     use level memory allocater (malloc, free, realloc)

	-malloc, free, realloc and mallopt form a memory allocator
	similar to malloc, free, and realloc.  The routines
	here are much faster than the original, with slightly worse
	space usage (a few percent difference on most input).  They
	do not have the property that data in freed blocks is left
	untouched until the space is reallocated.

	-Memory is kept in the "arena", a singly linked list of blocks.
	These blocks are of 3 types.
		1. A free block.  This is a block not in use by the
		   user.  It has a 3 word header. (See description
		   of the free queue.)
		2. An allocated block.  This is a block the user has
		   requested.  It has only a 1 word header, pointing
		   to the next block of any sort.
		3. A permanently allocated block.  This covers space
		   aquired by the user directly through sbrk().  It
		   has a 1 word header, as does 2.
	Blocks of type 1 have the lower bit of the pointer to the
	nextblock = 0.  Blocks of type 2 and 3 have that bit set,
	to mark them busy.

	-Unallocated blocks are kept on an unsorted doubly linked
	free list.  

	-Memory is allocated in blocks, with sizes specified by the
	user.  A circular first-fit startegy is used, with a roving
	head of the free queue, which prevents bunching of small
	blocks at the head of the queue.

	-Compaction is performed at free time of any blocks immediately
	following the freed block.  The freed block will be combined
	with a preceding block during the search phase of malloc.
	Since a freed block is added at the front of the free queue,
	which is moved to the end of the queue if considered and
	rejected during the search, fragmentation only occurs if
	a block with a contiguious preceding block that is free is
	freed and reallocated on the next call to malloc.  The
	time savings of this strategy is judged to be worth the
	occasional waste of memory.

	-Small blocks (of size < MAXSIZE)  are not allocated directly.
	A large "holding" block is allocated via a recursive call to
	malloc.  This block contains a header and ?????? small blocks.
	Holding blocks for a given size of small block (rounded to the
	nearest ALIGNSZ bytes) are kept on a queue with the property that any
	holding block with an unused small block is in front of any without.
	A list of free blocks is kept within the holding block.

	When increasing the size of the last used block in the arena with
	realloc, first check to see if a large enough free block exists.
	If one does not exist, expand arena to avoid a memcpy.
*/

/*
	description of arena, free queue, holding blocks etc.
*/
static struct {
	struct header arena[2];  /* the second word is a minimal block to
				   start the arena. The first is a busy
				   block to be pointed to by the last
				   block.       */
	struct header freeptr[2];/* first and last entry in free list */
} bert;
#define Arena bert.arena
#define Freeptr bert.freeptr

static struct header *arenaend; /* ptr to block marking high end of arena */
static struct header *lastblk;  /* the highest block in the arena */
static struct holdblk **holdhead; /* pointer to array of head pointers
				     to holding block chains */
/* In order to save time calculating indices, the array is 1 too
   large, and the first element is unused */
/* 
	Variables controlling algorithm, esp. how holding blocs are
	used
*/
static int numlblks = NUMLBLKS;
static int minhead = MINHEAD;	
static int change = 0;		  /* != 0, once param changes
				     are no longer allowed */
static int fastct = FASTCT;
static int maxfast = MAXFAST;
/* number of small block sizes to map to one size */
static int grain = ALIGNSZ;
/* signal blocking flag */
static int sigblk = 0;
/* flag to malloc.  If set, malloc will return NULL  */
/* instead of expanding the arena.  Used by realloc. */
static int no_expand = 0;
/* Why do an sbrk(0) when you can look at the value of _curbrk? */
extern struct header *_curbrk;

#ifdef debug
#define CHECKQ  checkq();
static
checkq()
{
	register struct header *p;

	p = &(Freeptr[0]);
	/* check forward links of free list    */
	/* (also check for loops in blocklist) */
	while(p != &(Freeptr[1]))       {
		assert(p!=p->nextblk);	/* <== will freelist become circular? */
		assert(p!=p->nextfree);	/* <== or, is it already circular? */
		p = p->nextfree;
		assert(p->prevfree->nextfree == p);
	}
	/* check backward links of free list */
	while(p != &(Freeptr[0]))       {
		assert(p!=p->prevfree);
		p = p->prevfree;
		assert(p->nextfree->prevfree == p);
	}
	return(1);
}
#else
#define CHECKQ
#endif


#if defined(_NAMESPACE_CLEAN) && defined(_ANSIC_CLEAN)
#   undef  malloc
#   pragma _HP_SECONDARY_DEF _malloc malloc
#   define malloc _malloc
#endif /* _NAMESPACE_CLEAN && _ANSIC_CLEAN*/
/*
	malloc(nbytes) - give a user nbytes to use
*/
void *
malloc(nbytes)
unsigned nbytes;
{
	register struct header *blk;
	register unsigned nb;      /* size of entire block we need */
	char *sbrk();
	sigset_t set,oset;

	if (sigblk) {	/* block ALL blockable signals */
		sigfillset(&set);
		sigprocmask(SIG_SETMASK,&set,&oset);
	}

	/*      on first call, initialize       */
	if (Freeptr->nextfree == GROUND) {

		/* initialize arena */
		Arena[1].nextblk = (struct header *)BUSY;
		Arena[0].nextblk = (struct header *)BUSY;
		lastblk = arenaend = &(Arena[1]);
		/* initialize free queue */
		Freeptr[0].nextfree = &(Freeptr[1]);
		Freeptr[1].nextblk = &(Arena[0]);
		Freeptr[1].prevfree = &(Freeptr[0]);
		/* mark that small blocks not init yet */
	}
	CHECKQ;
	if (nbytes == 0) goto NULL_RETURN;
	if (nbytes <= maxfast)  {
		/*
			We can allocate out of a holding block
		*/
		register struct holdblk *holdblk; /* head of right sized queue*/
		register struct lblk *lblk;     /* pointer to a little block */
		register struct holdblk *newhold;

		if (!change)  {
			register int i;
			/*
				This allocates space for hold block
				pointers by calling malloc recursively.
				Maxfast is temporarily set to 0, to
				avoid infinite recursion.  allocate
				space for an extra ptr so that an index
				is just ->blksz/grain, with the first
				ptr unused.
			*/
			change = 1;	/* change to algorithm params
					   no longer allowed */
			/* temporarily alter maxfast, to avoid
			   infinite recursion */
			maxfast = 0;
			holdhead = (struct holdblk **)
				   malloc(sizeof(struct szholdblk *)*
				   (fastct + 1));
			for(i=1; i<fastct+1; i++)  {
				holdhead[i] = HGROUND;
			}
			maxfast = fastct*grain;
		}
		/*  Note that this uses the absolute min header size (MINHEAD)
		    unlike the large block case which uses minhead */
		/* round up to nearest multiple of grain */
		nb = (nbytes + grain - 1)/grain*grain;      /* align */
		holdblk = holdhead[nb/grain];
		nb = nb + MINHEAD;
		/*      look for space in the holding block.  Blocks with
			space will be in front of those without */
		if ((holdblk != HGROUND) && (holdblk->lfreeq != LGROUND))  {
			/* there is space */
			lblk = holdblk->lfreeq;
			/* Now make lfreeq point to a free block.
			   If lblk has been previously allocated and 
			   freed, it has a valid pointer to use.
			   Otherwise, lblk is at the beginning of
			   the unallocated blocks at the end of
			   the holding block, so, if there is room, take
			   the next space.  If not, mark holdblk full,
			   and move holdblk to the end of the queue
			*/
			if(lblk < holdblk->unused)  {
				/* move to next holdblk, if this one full */
				if((holdblk->lfreeq = 
				  CLRSMAL(lblk->header.nextfree)) == LGROUND) {
					holdhead[(nb-MINHEAD)/grain] =
							holdblk->nexthblk;
				}
			}  else  if (((char *)holdblk->unused + nb) <
				    ((char *)holdblk + HOLDSZ(nb)))  {
				holdblk->unused = (struct lblk *)
						 ((char *)holdblk->unused+nb);
				holdblk->lfreeq = holdblk->unused;
			}  else {
				holdblk->lfreeq = LGROUND;
				holdhead[(nb-MINHEAD)/grain] =
							holdblk->nexthblk;
				holdblk->unused = (struct lblk *)
						 ((char *)holdblk->unused+nb);
			}
			/* mark as busy and small */
			lblk->header.holder = (struct holdblk *)SETALL(holdblk);
		}  else  {
			/* we need a new holding block */
			newhold = (struct holdblk *)
					malloc(HOLDSZ(nb));
			if ((char *)newhold == NULL)  {
				goto NULL_RETURN;
			}
			/* add to head of free queue */
			if (holdblk != HGROUND)  {
				newhold->nexthblk = holdblk;
				newhold->prevhblk = holdblk->prevhblk;
				holdblk->prevhblk = newhold;
				newhold->prevhblk->nexthblk = newhold;
			}  else  {
				newhold->nexthblk = newhold->prevhblk = newhold;
			}
			holdhead[(nb-MINHEAD)/grain] = newhold;
			/* set up newhold */
			lblk = (struct lblk *)(newhold->space);
			newhold->lfreeq = newhold->unused =
			     (struct lblk *)((char *)newhold->space+nb);
			lblk->header.holder= (struct holdblk *)SETALL(newhold);
			newhold->blksz = nb-MINHEAD;
		}
		if (sigblk) sigprocmask(SIG_SETMASK,&oset,&set);
		CHECKQ;
		return (char *)lblk + MINHEAD;
	}  else  {
		/*
			We need an ordinary block
		*/
		register struct header *newblk; /* used for creating a block */

		/* get number of bytes we need */
		nb = nbytes + minhead;
		nb = (nb + ALIGNSZ - 1)&~(ALIGNSZ-1);	    /* align */
		nb = (nb > MINBLKSZ) ? nb : MINBLKSZ;
		/*
			see if there is a big enough block
			If none exists, you will get to Freeptr[1].  
			Freeptr[1].next = &Arena[0], so when you do the test,
			the result is a large positive number, since Arena[0]
			comes before all blocks.  Arena[0] is marked busy so
			that it will not be compacted.  This kludge is for the
			sake of the almighty efficiency.
		*/
		/*   check that a very large request won't cause an inf. loop */
		if ((Freeptr[1].nextblk-&(Freeptr[1])) < nb)  goto NULL_RETURN;
		{
		register struct header *next;  /* block following blk */
		register struct header *nextnext;  /* block after next*/

		blk = Freeptr;
		do  {

			blk = blk->nextfree;
			/* see if we can compact */
			next = blk->nextblk;
			if (!TESTBUSY(nextnext = next->nextblk))  {
				do  {
					DELFREEQ(next);
					next = nextnext;
					nextnext = next->nextblk;
				}  while  (!TESTBUSY(nextnext));
				/* next will be at most == to lastblk, but I
				   think the >= test is faster */
				if (next >= arenaend)  lastblk = blk;
				blk->nextblk = next;
			}
		}  while (((char *)(next) - (char *)blk) < nb);
		}
		/* 
			if we didn't find a block, get more memory
		*/
		if (blk == &(Freeptr[1]))  {
			register struct header *newend; /* new end of arena */
			register int nget;      /* number of words to get */

			/* Four cases -  0. no_expand is set, so we unset
					    it and return NULL.
					 1. There is space between arenaend
					    and the break value that will become
					    a permanently allocated block.
					 2. Case 1 is not true, and the last
					    block is allocated.
					 3. Case 1 is not true, and the last
					    block is free
			*/
			if (no_expand) {
				no_expand=0;
				goto NULL_RETURN;
			}
			if ((newblk = _curbrk) != 
			   (struct header *)((char *)arenaend + HEADSZ))  {
				/* case 1 */
				/* get size to fetch */
				nget = nb+HEADSZ;
				/* round up to a block */
				nget = (nget+BLOCKSZ-1)&~(BLOCKSZ-1);
				/* if not word aligned, get extra space */
				if (((int)newblk&(ALIGNSZ-1)) != 0)  {
					nget += ALIGNSZ - ((int)newblk&(ALIGNSZ-1));
				}
#ifdef pdp11
				if (newblk + nget + 64 < newblk) {
					goto NULL_RETURN;
				}
#endif
				/* get memory */
				if ((struct header *)sbrk(nget) ==
				   (struct header *)-1) {
					goto NULL_RETURN;
				}
				/* add to arena */
				newend = (struct header *)((char *)newblk + nget
					 - HEADSZ);
				/*ignore some space to make block word aligned*/
				if (((int)newblk&(ALIGNSZ-1)) != 0)  {
					newblk = (struct header *)
						 ((char *)newblk + ALIGNSZ -
						 ((int)newblk&(ALIGNSZ-1)));
				}
				newend->nextblk = SETBUSY(&(Arena[1]));
				newblk->nextblk = newend;
				arenaend->nextblk = SETBUSY(newblk);
				/* adjust other pointers */
				arenaend = newend;
				lastblk = newblk;
				blk = newblk;
			}  else  if (TESTBUSY(lastblk->nextblk))  {
				/* case 2 */
				nget = (nb+BLOCKSZ-1)&~(BLOCKSZ-1);
#ifdef pdp11
				if (newblk + nget + 64 < newblk) {
					goto NULL_RETURN;
				}
#endif
				if(sbrk(nget) == (char *)-1)  {
					goto NULL_RETURN;
				}
				/* block must be word aligned */
				assert(((int)newblk%ALIGNSZ) == 0);
				newend =
				     (struct header *)((char *)arenaend+nget);
				newend->nextblk = SETBUSY(&(Arena[1]));
				arenaend->nextblk = newend;
				lastblk = blk = arenaend;
				arenaend = newend;
			}  else  {
				/* case 3 */
				/* last block in arena is at end of memory and
				   is free */
				nget = nb - ((char *)arenaend - (char *)lastblk);
				nget = (nget+BLOCKSZ-1)&~(BLOCKSZ-1);
#ifdef pdp11
				if (newblk + nget + 64 < newblk) {
					goto NULL_RETURN;
				}
#endif
				assert(((int)newblk%ALIGNSZ) == 0);
				if (sbrk(nget) == (char *)-1)  {
					goto NULL_RETURN;
				}
				/* combine with last block, put in arena */
				newend = (struct header *)((char *)arenaend +
					  nget);
				arenaend = lastblk->nextblk = newend;
				newend->nextblk = SETBUSY(&(Arena[1]));
				/* set which block to use */
				blk = lastblk;
				DELFREEQ(blk);
			}
		}  else  {
			register struct header *nblk;      /* next block */

			/* take block found of free queue */
			DELFREEQ(blk);
			/* make head of free queue immediately follow blk, unless
			   blk was at the end of the queue */
			nblk = blk->nextfree;
			if (nblk != &(Freeptr[1]))   {
				MOVEHEAD(nblk);
			}
		}
		/*      blk now points to an adequate block     */
		if (((char *)blk->nextblk - (char *)blk) - nb >= MINBLKSZ)  {
			/* carve out the right size block */
			/* newblk will be the remainder */
			newblk = (struct header *)((char *)blk + nb);
			newblk->nextblk = blk->nextblk;
			/* mark the block busy */
			blk->nextblk = SETBUSY(newblk);
			ADDFREEQ(newblk);
			/* if blk was lastblk, make newblk lastblk */
			if(blk==lastblk) lastblk = newblk;
		}  else  {
			/* just mark the block busy */
			blk->nextblk = SETBUSY(blk->nextblk);
		}
	}
	CHECKQ;
	if (sigblk) sigprocmask(SIG_SETMASK,&oset,&set); /* unblock signals */
	return (char *)blk + minhead;
NULL_RETURN:
	CHECKQ;
	if (sigblk) sigprocmask(SIG_SETMASK,&oset,&set); /* unblock signals */
	return NULL;
}


/*      free(ptr) - free block that user thinks starts at ptr 

	input - ptr-1 contains the block header.
		If the header points forward, we have a normal
			block pointing to the next block
		if the header points backward, we have a small
			block from a holding block.
		In both cases, the busy bit must be set
*/
#if defined(_NAMESPACE_CLEAN) && defined(_ANSIC_CLEAN)
#   undef  free
#   pragma _HP_SECONDARY_DEF _free free
#   define free _free
#endif /* _NAMESPACE_CLEAN && _ANSIC_CLEAN */
void
free(ptr)
char *ptr;
{
	register struct holdblk *holdblk;       /* block holding blk */
	register struct holdblk *oldhead;       /* former head of the hold 
						   block queue containing
						   blk's holder */
	sigset_t set,oset;


	CHECKQ;
	if (ptr==NULL)	/* malloc(0) returns NULL ptr */
		return;
	if (sigblk) {	/* block ALL blockable signals */
		sigfillset(&set);
		sigprocmask(SIG_SETMASK,&set,&oset);
	}
	if (TESTSMAL(((struct header *)(ptr - MINHEAD))->nextblk))  {
		register struct lblk *lblk;     /* pointer to freed block */
		register offset;		/* choice of header lists */

		lblk = (struct lblk *)CLRBUSY(ptr - MINHEAD);
		assert((struct header *)lblk < arenaend);
		assert((struct header *)lblk > Arena);
		/* allow twits (e.g. awk) to free a block twice */
		if (!TESTBUSY(holdblk = lblk->header.holder)) goto RETURN;
		holdblk = (struct holdblk *)CLRALL(holdblk);
		/* put lblk on its hold block's free list */
		lblk->header.nextfree = SETSMAL(holdblk->lfreeq);
		holdblk->lfreeq = lblk;
		/* move holdblk to head of queue, if its not already there */
		offset = holdblk->blksz/grain;
		oldhead = holdhead[offset];
		if (oldhead != holdblk)  {
			/* first take out of current spot */
			holdhead[offset] = holdblk;
			holdblk->nexthblk->prevhblk = holdblk->prevhblk;
			holdblk->prevhblk->nexthblk = holdblk->nexthblk;
			/* now add at front */
			holdblk->nexthblk = oldhead;
			holdblk->prevhblk = oldhead->prevhblk;
			oldhead->prevhblk = holdblk;
			holdblk->prevhblk->nexthblk = holdblk;
		}
	}  else  {
		register struct header *blk;	    /* real start of block*/
		register struct header *next;      /* next = blk->nextblk*/
		register struct header *nextnext;       /* block after next */

		blk = (struct header *)(ptr - minhead);
		next = blk->nextblk;
		/* take care of twits (e.g. awk) who return blocks twice */
		if (!TESTBUSY(next))  goto RETURN;
		blk->nextblk = next = CLRBUSY(next);
		ADDFREEQ(blk);
		/* see if we can compact */
		if (!TESTBUSY(nextnext = next->nextblk))  {
			do  {
				DELFREEQ(next);
				next = nextnext;
			}  while (!TESTBUSY(nextnext = next->nextblk));
			if (next == arenaend) lastblk = blk;
			blk->nextblk = next;
		}
	}
RETURN:
	CHECKQ
	if (sigblk) sigprocmask(SIG_SETMASK,&oset,&set); /* unblock signals */
}

/*      realloc(ptr,size) - give the user a block of size "size", with
			    the contents pointed to by ptr.  Free ptr.
*/
#if defined(_NAMESPACE_CLEAN) && defined(_ANSIC_CLEAN)
#   undef  realloc
#   pragma _HP_SECONDARY_DEF _realloc realloc
#   define realloc _realloc
#endif /* _NAMESPACE_CLEAN && _ANSIC_CLEAN */
void *
realloc(ptr,size)
char *ptr;		    /* block to change size of */
unsigned size;	    /* size to change to */
{
	register struct header *blk;    /* block ptr is contained in */
	register unsigned trusize;      /* size of block, as allocaters see it*/
	char *newptr;	      /* pointer to user's new block */
	register unsigned cpysize;      /* amount to copy */
	register struct header *next;   /* block after blk */
	sigset_t set,oset;


	CHECKQ;
	if (size==0) {
		free(ptr);
		return(NULL);
	}
	if (ptr == NULL) return(malloc(size));

	if (sigblk) {	/* block ALL blockable signals */
		sigfillset(&set);
		sigprocmask(SIG_SETMASK,&set,&oset);
	}
	if (TESTSMAL(((struct lblk *)(ptr - MINHEAD))->header.holder))  {
		/* we have a special small block which can't be expanded */
		/* This makes the assumption that even if the user is 
		   reallocating a free block, malloc doesn't alter the contents
		   of small blocks */
		newptr = (char *)malloc(size);
		if (newptr == NULL)  goto RNULL_RETURN;
		/* Copy correct amount of data from old block into new block */
		trusize=(unsigned)(((struct holdblk *)CLRALL(((struct lblk *)(ptr - MINHEAD))->header.holder))->blksz);
		trusize = (trusize < size) ? trusize : size;
		if(ptr != newptr)  {
			(void)memcpy(newptr,ptr,(int)trusize);
			free(ptr);
		}
	}  else  {
		blk = (struct header *)(ptr - minhead);
		next = blk->nextblk;
		/* deal with twits who reallocate free blocks */
		/* if they haven't reset minblk via getopt, that's
		   thier problem */
		if (!TESTBUSY(next))  {
			DELFREEQ(blk);
			blk->nextblk = SETBUSY(next);
		}
		next = CLRBUSY(next);
		/* make blk as big as possible */
		if (!TESTBUSY(next->nextblk))  {
			do {
				DELFREEQ(next);
				next = next->nextblk;
			}  while (!TESTBUSY(next->nextblk));
			blk->nextblk = SETBUSY(next);
			if (next >= arenaend) lastblk = blk;
		}  
		/* get size we really need */
		trusize = size+minhead;
		trusize = (trusize + ALIGNSZ - 1)&~(ALIGNSZ-1);
		trusize = (trusize >= MINBLKSZ) ? trusize : MINBLKSZ;
		/* see if we have enough */
		/* this isn't really the copy size, but I need a register */
		cpysize = (char *)next - (char *)blk;
		if (cpysize >= trusize)  {
			/* carve out the size we need */
			register struct header *newblk; /* remainder */
carve:
			if (cpysize - trusize >= MINBLKSZ)
				{
				/* carve out the right size block */
				/* newblk will be the remainder */
				newblk = (struct header *)((char *)blk 
					  + trusize);
				newblk->nextblk = next;
				blk->nextblk = SETBUSY(newblk);
				/* at this point, next is invalid */
				ADDFREEQ(newblk);
				/* if blk was lastblk, make newblk lastblk */
				if(blk==lastblk) lastblk = newblk;
			}
			newptr = ptr;
		}  else  {
			if (next==arenaend &&  _curbrk == (struct header *)((char *)arenaend + HEADSZ)) {
				no_expand=1;	/* just looking ... */
				if (!(newptr=(char *)malloc(size))) {
					/* expand the arena, avoid a memcpy() */
					register int nget;

					nget=trusize-cpysize;
					nget=(nget+BLOCKSZ-1)&~(BLOCKSZ-1);
					if (sbrk(nget)==(char *)-1) {
						goto RNULL_RETURN;
					}
					arenaend=(struct header *)((char *)arenaend+nget);
					arenaend->nextblk = SETBUSY(&(Arena[1]));
					next=arenaend;
					blk->nextblk = SETBUSY(arenaend);
					cpysize += nget;
					goto carve;	/* split off unused */
				} else {
					no_expand=0;
					goto the_bullet;
				}
			} else {
				/* bite the bullet, and call malloc */
				newptr = (char *)malloc(size);
the_bullet:
				cpysize = (size > cpysize) ? cpysize : size;
				if (newptr == NULL) goto RNULL_RETURN;
				(void)memcpy(newptr,ptr,(int)cpysize);
				free(ptr);
			}
		}       
	}
	CHECKQ;
	if (sigblk) sigprocmask(SIG_SETMASK,&oset,&set); /* unblock signals */
	return newptr;
RNULL_RETURN:
	CHECKQ;
	if (sigblk) sigprocmask(SIG_SETMASK,&oset,&set); /* unblock signals */
	return NULL;
}


/*LINTLIBRARY*/
/*      calloc - allocate and clear memory block
*/


#if defined(_NAMESPACE_CLEAN)
#undef calloc
#pragma _HP_SECONDARY_DEF _calloc calloc
#define calloc _calloc
#endif /* _NAMESPACE_CLEAN */
void *
calloc(num, size)
register unsigned num, size;
{
	register char *mp;
	char *b;

	b=(char *)_curbrk;	/* get brk value before malloc */
	num *= size;
	mp = (char *)malloc(num);
	if(mp == NULL)
		return(NULL);
	num=(b>mp && b-mp<num)?b-mp:num; /* don't zero new bytes... */
	(void)memset(mp,0,(int)num);	 /* they are already zeroed */
	return(mp);


}

/*      Mallopt - set options for allocation

	Mallopt provides for   control over the allocation algorithm.
	The cmds available are:

	M_MXFAST Set maxfast to value.  Maxfast is the size of the
		 largest small, quickly allocated block.  Maxfast
		 may be set to 0 to disable fast allocation entirely.

	M_NLBLKS Set numlblks   to value.  Numlblks is the number of
		 small blocks per holding block.  Value must be
		 greater than 0.

	M_GRAIN  Set grain to value.  The sizes of all blocks
		 smaller than maxfast are considered to be rounded
		 up to the nearest multiple of grain.    The default
		 value of grain is the smallest number of bytes
		 which will allow alignment of any data type.    Grain
		 will   be rounded up to a multiple of its default,
		 and maxsize will be rounded up to a multiple   of
		 grain.  Value must be greater than 0.

	M_KEEP   Retain data in freed   block until the next malloc,
		 realloc, or calloc.  Value is ignored.
		 This option is provided only for compatibility with
		 the old version of malloc, and is not recommended.

	returns - 0, upon successful completion
		  1, if malloc has previously been called or
		     if value or cmd have illegal values
*/


mallopt(cmd, value)
register int cmd;	       /* specifies option to set */
register int value;	     /* value of option */
{
	/* disallow changes once a small block is allocated */
	/* unless M_BLOCK or M_UBLOCK specified */
	if (change && cmd!=M_BLOCK && cmd!=M_UBLOCK)  {
		return 1;
	}
	switch (cmd)  {
	    case M_MXFAST:
		if (value < 0)  {
			return 1;
		}
		fastct = (value + grain - 1)/grain;
		maxfast = grain*fastct;
		break;
	    case M_NLBLKS:
		if (value <= 1)  {
			return 1;
		}
		numlblks = value;
		break;
	    case M_GRAIN:
		if (value <= 0)  {
			return 1;
		}
		/* round grain up to a multiple of ALIGNSZ */
		grain = (value + ALIGNSZ - 1)&~(ALIGNSZ-1);
		/* reduce fastct appropriately */
		fastct = (maxfast + grain - 1)/grain;
		maxfast = grain*fastct;
		break;
	    case M_KEEP:
		minhead = HEADSZ;
		break;
	    case M_BLOCK:
		sigblk=1;
		break;
	    case M_UBLOCK:
		sigblk=0;
		break;
	    default:
		return 1;
	}
	return 0;
}

/*	mallinfo-provide information about space usage

	input - max; mallinfo will return the size of the
		largest block < max.

	output - a structure containing a description of
		 of space usage, defined in malloc.h
*/
struct mallinfo
mallinfo()
{
	struct header *blk, *next;	/* ptr to ordinary blocks */
	struct holdblk *hblk;		/* ptr to holding blocks */
	struct mallinfo inf;		/* return value */
	register i;			/* the ubiquitous counter */
	int size;			/* size of a block */
	int fsp;			/* free space in 1 hold block */

	(void)memset (&inf, 0, sizeof(struct mallinfo));
	if(Freeptr->nextfree == GROUND)
		return(inf);
	blk = CLRBUSY(Arena[1].nextblk);
	/* return total space used */
	inf.arena = (char *)arenaend - (char *)blk;
	/* loop through arena, counting # of blocks, and
	   and space used by blocks */
	next = CLRBUSY(blk->nextblk);
	while (next != &(Arena[1])) {
		inf.ordblks++;
		size = (char *)next - (char *)blk;
		if (TESTBUSY(blk->nextblk))  {
			inf.uordblks += size;
			inf.keepcost += HEADSZ-MINHEAD;
		}  else  {
			inf.fordblks += size;
		}
		blk = next;
		next = CLRBUSY(blk->nextblk);
	}
	/* 	examine space in holding blks	*/
      if(change) {
	for (i=fastct; i>0; i--)  {  /* loop thru ea. chain */
	    hblk = holdhead[i];
	    if (hblk != HGROUND)  {  /* do only if chain not empty */
	    	size = hblk->blksz + MINHEAD;
		do  {		     /* loop thru 1 hold blk chain */
		    inf.hblks++;    
		    fsp = freespace(hblk);
		    inf.fsmblks += fsp;
		    inf.usmblks += numlblks*size - fsp;
		    inf.smblks += numlblks;
		    hblk = hblk->nexthblk;
		}  while (hblk != holdhead[i]);
	    }
	}
      }
	inf.hblkhd = (inf.smblks/numlblks)*sizeof(struct szholdblk);
	/* holding block were counted in ordblks, so subtract off */
	inf.ordblks -= inf.hblks;
	inf.uordblks -= inf.hblkhd + inf.usmblks + inf.fsmblks;
	inf.keepcost -= inf.hblks*(HEADSZ - MINHEAD);
	return inf;
}

/*	freespace - calc. how much space is used in the free
		    small blocks in a given holding block

	input - hblk = given holding block

	returns space used in free small blocks of hblk
*/
freespace(holdblk)  
register struct holdblk *holdblk;
{
	register struct lblk *lblk;
	register int space = 0;
	register int size;
	register struct lblk *unused;

	lblk = CLRSMAL(holdblk->lfreeq);
	size = holdblk->blksz + MINHEAD;
	unused = CLRSMAL(holdblk->unused);
	/* follow free chain */
	while ((lblk != LGROUND) && (lblk != unused))  {
		space += size;
		lblk = CLRSMAL(lblk->header.nextfree);
	}
	space += ((char *)holdblk + HOLDSZ(size)) - (char *)unused;
	return space;
}
#ifdef RSTALLOC
/*      rstalloc - reset alloc routines

	description - return allocated memory and reset
		      allocation pointers.

	Warning - This is for debugging purposes only.
		  It will return all memory allocated after
		  the first call to malloc, even if some
		  of it was fetched by a user's sbrk().
*/
rstalloc()
{
	struct header *temp;

	temp = Arena;
	minhead = MINHEAD;
	grain = ALIGNSZ;
	numlblks = NUMLBLKS;
	fastct = FASTCT;
	maxfast = MAXFAST;
	change = 0;
	sigblk = 0;
	if(Freeptr->nextfree == GROUND) return;
	brk(CLRBUSY(Arena[1].nextblk));
	Freeptr->nextfree = GROUND;
}
#endif /*RSTALLOC*/

#if defined(_NAMESPACE_CLEAN)
#   undef  cfree
#   pragma _HP_SECONDARY_DEF _cfree cfree
#   define cfree _cfree
#endif /* _NAMESPACE_CLEAN */
/*
 * cfree(p, num, size) - free block that user thinks starts at p
 */
/*ARGSUSED*/
void
cfree(p, num, size)
char *p;
unsigned num, size;
{
        free(p);
}
