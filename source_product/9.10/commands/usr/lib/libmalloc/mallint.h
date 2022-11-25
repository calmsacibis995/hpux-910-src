/* @(#) $Revision: 66.4 $ */      
/*
	template for the header
*/
struct header {
	struct header *nextblk;
	struct header *nextfree;
	struct header *prevfree;
#if defined (hpux) || (pdp11)		/* ensure doubleword alignment */
	struct header *dummy;	   /* pad to a multple of 4 bytes */
#endif
};
/*
	template for a small block
*/
struct lblk  {
	union {
		struct lblk *nextfree;  /* the next free little block in this
					   holding block.  This field is used
					   when the block is free */
		struct holdblk *holder; /* the holding block containing this
					   little block.  This field is used
					   when the block is allocated */
	}  header;
	char byte;		    /* There is no telling how big this
					   field freally is.  */
};
/* 
	template for holding block
*/
struct holdblk {
	struct holdblk *nexthblk;   /* next holding block */
	struct holdblk *prevhblk;   /* previous holding block */
	struct lblk *lfreeq;	/* head of free queue within block */
	struct lblk *unused;	/* pointer to 1st little block never used */
	int blksz;		/* size of little blocks contained */
#if defined (hpux) || (pdp11)
	int pad;		/* pad to an even # of words */
#endif
	char space[1];		/* start of space to allocate.
				   This must be on a word boundary */
};

/*
	template for holding block.  used in HOLDSZ define and for
	sizing of holdblk strcuture.
*/
struct szholdblk {
	struct holdblk *nexthblk;   /* next holding block */
	struct holdblk *prevhblk;   /* previous holding block */
	struct lblk *lfreeq;	/* head of free queue within block */
	struct lblk *unused;	/* pointer to 1st little block never used */
	int blksz;		/* size of little blocks contained */
#if defined (hpux) || (pdp11)
	int pad;		/* pad to an even # of words */
#endif
};

/*
	The following manipulate the free queue

		DELFREEQ will remove x from the free queue
		ADDFREEQ will add an element to the free queue.
		ADDTOHEAD will add an element to the head
			 of the free queue.
		ADDTOTAIL will add an element to the tail
			 of the free queue.
		MOVEHEAD will move the free pointers so that
			 x is at the front of the queue
*/
#define ADDFREEQ(x)	   ADDTOTAIL(x)
#define ADDTOTAIL(x)       (x)->prevfree = Freeptr[1].prevfree;\
				(x)->nextfree = &(Freeptr[1]);\
				Freeptr[1].prevfree->nextfree = (x);\
				Freeptr[1].prevfree = (x);\
				assert((x)->nextfree != (x));\
				assert((x)->prevfree != (x));
#define ADDTOHEAD(x)       (x)->prevfree = &(Freeptr[0]);\
				(x)->nextfree = Freeptr[0].nextfree;\
				Freeptr[0].nextfree->prevfree = (x);\
				Freeptr[0].nextfree = (x);\
				assert((x)->nextfree != (x));\
				assert((x)->prevfree != (x));
#define DELFREEQ(x)       (x)->prevfree->nextfree = (x)->nextfree;\
				(x)->nextfree->prevfree = (x)->prevfree;\
				assert((x)->nextfree != (x));\
				assert((x)->prevfree != (x));
#define MOVEHEAD(x)       Freeptr[1].prevfree->nextfree = \
					Freeptr[0].nextfree;\
				Freeptr[0].nextfree->prevfree = \
					Freeptr[1].prevfree;\
				(x)->prevfree->nextfree = &(Freeptr[1]);\
				Freeptr[1].prevfree = (x)->prevfree;\
				(x)->prevfree = &(Freeptr[0]);\
				Freeptr[0].nextfree = (x);\
				assert((x)->nextfree != (x));\
				assert((x)->prevfree != (x));
/*
	The following manipulate the busy flag
*/
#define BUSY	1
#define SETBUSY(x)      ((struct header *)((int)(x) | BUSY))
#define CLRBUSY(x)      ((struct header *)((int)(x) & ~BUSY))
#define TESTBUSY(x)     ((int)(x) & BUSY)
/*
	The following manipulate the small block flag
*/
#define SMAL	2
#define SETSMAL(x)      ((struct lblk *)((int)(x) | SMAL))
#define CLRSMAL(x)      ((struct lblk *)((int)(x) & ~SMAL))
#define TESTSMAL(x)     ((int)(x) & SMAL)
/*
	The following manipulate both flags.  They must be 
	type coerced
*/
#define SETALL(x)       ((int)(x) | (SMAL | BUSY))
#define CLRALL(x)       ((int)(x) & ~(SMAL | BUSY))
/*
	Other useful constants
*/
#define TRUE    1
#define FALSE   0
#define HEADSZ  sizeof(struct header)   /* size of unallocated block header */
/*      MINHEAD is the minimum size of an allocated block header */
#ifdef pdp11
#define MINHEAD 4
#else
#define MINHEAD 8
#endif
#define MINBLKSZ 12		/* min. block size must as big as
				   HEADSZ */
#define BLOCKSZ 2048		/* memory is gotten from sbrk in
				   multiples of BLOCKSZ */
#define GROUND  (struct header *)0
#define LGROUND (struct lblk *)0	/* ground for a queue within a holding
					   block	*/
#define HGROUND (struct holdblk *)0     /* ground for the holding block queue */
/* number of bytes to align to  (must be at least 4, because lower 2 bits
   are used for flags */
#define ALIGNSZ 8
#ifndef NULL
#define NULL    (char *)0
#endif
/*
	Structures and constants describing the holding blocks
*/
#define NUMLBLKS  100   /* default number of small blocks per holding block */
/* size of a holding block with small blocks of size blksz */
#define HOLDSZ(blksz)  (sizeof(struct szholdblk) + blksz*numlblks)


#define FASTCT 0	/* for XPG2 and SVID compliance */


#define MAXFAST   ALIGNSZ*FASTCT  /* default maximum size block for fast
				     allocation */

