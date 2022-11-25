/* file sets.c */
/* @(#) $Revision: 66.6 $ */
/* KLEENIX_ID @(#)sets.c	10.1 90/02/05 */
/*======================================================================
 *
 * Procedures: Sets.c                           (Optimizer Set Routines)
 * By: Greg Lindhorst, July 1988 
 * Where: Colorado Language Lab, HP Fort Collins
 *
 * Description:
 *   This is a rewrite of the old set routines which were used to do
 *   set manipulations in the global optimizer for the Series 300.  
 *   This version assumes that almost all of our sets are sparse, and
 *   thus we use a linked list representation for the sets, where only
 *   the elements which are set are represented.
 *
 * External Routines:
 *   new_set( max_elems )                       Allocate a New Set
 *   new_set_n( max_elems, num_sets )           Multiple 'new_set'
 *
 * Internal Routines:
 *   get_sets()                                 The Free List is Empty
 *
 * Refrences:
 *   Old Series 300 'sets.s' code, by Scott Omdahl (COL), written 2/10/88.
 *   "Bit Vectors: The Set Utilities", by Karl Pettis (CAL),
 *                                     Version 1.0, 12/1/86.  
 */

/*
 * struct set_entry
 *   This structure comprises the elements for all linked lists.
 *   In normal usage, num is the element number for this particular
 *   block of data.  However, in the first block, this number is the 
 *   size of the set (which is used for error checking).
 */
struct set_entry {
    unsigned int num;                 /* element number for 'data' */
    unsigned int data;                /* the actual bits */
    struct set_entry *next;           /* pointer to next block */
    };

/* 
 * 'free_list'
 *   This pointer points to the top of the free list.  If it is empty, 
 *   then 'get_sets()' is called to replentish it.  
 */
/* static */ struct set_entry *free_list;

/*
 * 'nextset(a)'
 *   This is a simple macro to insure that there is an entry on top of
 *   the free list, and if not we get some more.  'a' is given the new
 *   node, and the free_list is updated.
 */
#define  setalloc(a) ((a = (free_list ? free_list : (get_sets(),free_list))),\
                     free_list = free_list->next /* , printf("<%x>\n",a) */ )

/*
 * 'NUM_ENTRIES'
 *   This is the number of entries to malloc at a time, when get_sets is
 *   called.
 */
# ifndef NUM_ENTRIES
#	define     NUM_ENTRIES      512
# endif NUM_ENTRIES

/*
 * 'NULL_CHECK'
 *   This flag indicates that NULL pointer checks should be made at the
 *   top of most procedure calls.
 */
#define     NULL_CHECK

/* 
 * 'NULL'
 *   The typical definition.  Don't bother changing this.
 */
#define     NULL             0

/* The following set masks have been reversed because sets.s has reversed
   the sense of the sets. Believe me, you don't want to have to debug the 
   problems caused by mixing the two sets of routines unless a consistent
   ordering is enforced. -mfm
*/
/*
 * 'bit_masks'
 *   This array contains the various bit masks for the bit numbers
 *   indicated.  0 is the MSB, 31 is the LSB.
 *
 * 'bit_low'
 *   This array contains masks for the lower part of a word.
 *
 * 'bit_high'
 *   This array contains masks for the upper part of a word.
 */
static unsigned int bit_masks[ 32 ] = 
		    { 0x00000001, 0x00000002, 0x00000004, 0x00000008,
		      0x00000010, 0x00000020, 0x00000040, 0x00000080,
		      0x00000100, 0x00000200, 0x00000400, 0x00000800,
		      0x00001000, 0x00002000, 0x00004000, 0x00008000,
		      0x00010000, 0x00020000, 0x00040000, 0x00080000,
		      0x00100000, 0x00200000, 0x00400000, 0x00800000,
		      0x01000000, 0x02000000, 0x04000000, 0x08000000,
		      0x10000000, 0x20000000, 0x40000000, 0x80000000 };
static unsigned int bits_low[ 32 ] = 
                    { 0x00000001, 0x00000003, 0x00000007, 0x0000000f,
                      0x0000001f, 0x0000003f, 0x0000007f, 0x000000ff,
                      0x000001ff, 0x000003ff, 0x000007ff, 0x00000fff,
                      0x00001fff, 0x00003fff, 0x00007fff, 0x0000ffff,
                      0x0001ffff, 0x0003ffff, 0x0007ffff, 0x000fffff,
                      0x001fffff, 0x003fffff, 0x007fffff, 0x00ffffff,
                      0x01ffffff, 0x03ffffff, 0x07ffffff, 0x0fffffff,
                      0x1fffffff, 0x3fffffff, 0x7fffffff, 0xffffffff };
static unsigned int bits_high[ 32 ] = 
                    { 0xffffffff, 0xfffffffe, 0xfffffffc, 0xfffffff8,
                      0xfffffff0, 0xffffffe0, 0xffffffc0, 0xffffff80,
                      0xffffff00, 0xfffffe00, 0xfffffc00, 0xfffff800, 
                      0xfffff000, 0xffffe000, 0xffffc000, 0xffff8000,
                      0xffff0000, 0xfffe0000, 0xfffc0000, 0xfff80000,
                      0xfff00000, 0xffe00000, 0xffc00000, 0xff800000,
                      0xff000000, 0xfe000000, 0xfc000000, 0xf8000000,
                      0xf0000000, 0xe0000000, 0xc0000000, 0x80000000 };

# include "mfile2"
# ifdef MDEBUGGING
# 	include "chkmem.h"
# endif MDEBUGGING

/*
 * Function types:
 */
struct set_entry *new_set();
void free_set();
struct set_entry **new_set_n();
void free_seet_n();
int xin();
void adelement();
void delelement();
void setassign();
void setclear();
void addsetrange();
void setunion();
void intersect();
void difference();
int setcompare();
int differentsets();
int nextel();
int isemptyset();
int set_size();
void change_set_size();
/* static */ void get_sets();
/* static */ void trim();
/* static */ void error();

/*----------------------------------------------------------------------
 *
 * Procedure: new_set                               (Allocate a New Set)
 * 
 * Description:
 *   This procedure allocates a new_set.  All that is done is that 
 *   a header block is allocated, and filled in with the appropriate 
 *   maximum number of elements.
 */
struct set_entry *new_set( max_elems )
unsigned int max_elems;
{
	register struct set_entry *alloced;

	/* Get the one off the top of the free list */
	setalloc( alloced );
# ifdef DEBUGGING
	if (mdebug>3)
		fprintf(debugp,"new_set(%d) returns 0x%x\n", max_elems,
			alloced);
# endif DEBUGGING

	/* Fill in information */
	alloced->num = max_elems;
	alloced->next = NULL;

	/* Return the Set */
	return( alloced );
}

/*----------------------------------------------------------------------
 *
 * Procedure: free_set                                      (free a set)
 *
 * Description:
 *   Free a set which has been allocated with 'new_set'.
 */
void free_set( set )
struct set_entry *set;
{
	register struct set_entry *scan;

	/* Error Check */
#ifdef  NULL_CHECK
	if( !set )
		error( "Null set passed to free_set()" );
#endif

# ifdef DEBUGGING
	if (mdebug>3)
		fprintf(debugp,"free_set(0x%x) called.\n", set);
# endif DEBUGGING
	
	/* Find the end of the set */
	for( scan = set; scan->next; scan = scan->next ) ;
	
	/* Patch into the free list */
	scan->next = free_list;
	free_list = set;
}

/*----------------------------------------------------------------------
 *
 * Procedure: new_set_n                             (Multiple 'new_set')
 *
 * Description:
 *   This routine does the same thing as new_set does, except that it
 *   returns 'num_sets' number of sets, in an array of vectors.  
 */
struct set_entry **new_set_n( max_elems, num_sets )
unsigned int max_elems, num_sets;
{
	register struct set_entry **vector, **save;
# ifdef DEBUGGING
	int nsets = num_sets;
# endif DEBUGGING

	/* Malloc up a vector space for the set pointers */
	if( !(save = (struct set_entry **)
                      malloc( sizeof(struct set_entry *) * (num_sets+1) )) )
	{
		error( "Out of Memory in 'new_set_n'" );
	}
	
	/* For Each Set... */
	vector = save;
	for( ; num_sets--; vector++ )
	{
		/* Get the top guy */
		setalloc( *vector );

		/* Fill in information */
		(*vector)->num = max_elems;
		(*vector)->next = NULL;
	}
	*vector = NULL;

	/* Return the array of vectors */
# ifdef DEBUGGING
	if (mdebug>3)
		fprintf(debugp,"new_set_n(max_elems=%d,num_sets=%d) returns 0x%x\n",
			max_elems, nsets, save);
# endif DEBUGGING
	return( save );
}

/*----------------------------------------------------------------------
 *
 * Procedure: free_set_n                           (multiple 'free_set')
 *
 * Description:
 *   This procedure will free n sets, which were allocated with the
 *   new_set_n call, in the same way that free_set works.
 */
void free_set_n( sets )
register struct set_entry **sets;
{
	register struct set_entry *scan;
	struct set_entry **sp = sets;

#ifdef   NULL_CHECK
	if( !sets )
		error( "Null pointer sent into free_set_n()" );
#endif
# ifdef DEBUGGING
	if (mdebug>3)
		fprintf(debugp,"free_set_n(0x%x) called.\n", sets);
# endif DEBUGGING

	/* Go through each of them, and look for a NULL pointer */
	for( ; *sets; sets++ )
	{
		/* Scan through and look for the last block */
		for( scan = *sets; scan->next; scan = scan->next ) ;

		/* Patch into the free list */
		scan->next = free_list;
		free_list = *sets;
	}
	free(sp);
}
		
/*----------------------------------------------------------------------
 *
 * Procedure: xin                                     (Check an element)
 *
 * Description: 
 *   This procedure will return the value of a particular element in the
 *   given set.
 */
int xin( set, elem )
struct set_entry *set;
unsigned int elem;
{
	register unsigned int block_num;
	register struct set_entry *scan;

#ifdef   NULL_CHECK
	if( !set )
		error( "Null Pointer passed in as src to 'xin'" );
#endif

	/* Error Checking */
	if( elem >= set->num )
		error( "element number too large in 'xin'" );

	/* Trivial Case */
	if( !set->next )
		return( 0 );

	/* Set up some things for the scan */
	block_num = elem >> 5;                         /* Divide by 32 */
	
	/* Scan forward, checking for a block number that is too large */
	for( scan = set->next; scan && scan->num < block_num; scan = scan->next ) ;

	/* Did we pass it, or did we find it */
	if( scan && block_num == scan->num )
		return( (bit_masks[ elem & 0x1f ] & scan->data) ? 1 : 0 );
	else
		return( 0 );
}

/*----------------------------------------------------------------------
 *
 * Procedure: adelement                       (Add an Element to a set)
 *
 * Description: 
 *   Adds an element to a set, by copying the set to another set, 
 *   and then either simply setting a bit in a currnet block, or
 *   adding a new block.
 */
void adelement( elem, src_set, dest_set )
unsigned int elem;
struct set_entry *src_set, *dest_set;
{
	register struct set_entry *src_scan, *dest_scan, *temp_scan;
	register unsigned int block_num, notdone;

#ifdef   NULL_CHECK
	if( !src_set || !dest_set )
		error( "Null Sets passed in to 'adelement'" );
#endif

	/* Copy over some values (into regs) */
	block_num = elem >> 5;                        /* Divide by 32 */

	/* Error Check */
	if( elem >= src_set->num )
		error( "element number too large in 'adelement'" );
	if( src_set->num != dest_set->num )
		error( "sets have different sizes in 'adelement'" );

	/* Two cases: 1) src_set == dest_set --or-- 2) src_set != dest_set */
	if( src_set == dest_set )
	{
		/* Scan forward till we find the block (or not find it) */
		for( src_scan = src_set->next, temp_scan = src_set; 
                  src_scan && src_scan->num < block_num; 
			              temp_scan = src_scan, src_scan = src_scan->next ) ;

		/* If we already have the block, set the bit */
		if( src_scan && src_scan->num == block_num )
		{
			src_scan->data |= bit_masks[ elem & 0x1f ];
		}
		else
		{
			/* Get a new block, place in the list */
			setalloc( dest_scan );
			temp_scan->next = dest_scan;
			dest_scan->next = src_scan;

			/* Set the bit, and block number */
			dest_scan->data = bit_masks[ elem & 0x1f ];
			dest_scan->num = block_num;
		}
	}
	else  /* 2) src_set != dest_set */
	{
		/* Scan through the src blocks, copying to dest */
		dest_scan = dest_set;
		src_scan = src_set->next;
		for( notdone = 1; src_scan || notdone; )
		{
			/* Do we have a block to put it? */
			if( !dest_scan->next )
			{
				setalloc( dest_scan->next );
				dest_scan->next->next = NULL;
			}
			dest_scan = dest_scan->next;
			
			/* Did we pass it? */
			if( !src_scan || (src_scan->num > block_num && notdone) )
			{
				dest_scan->num = block_num;
				dest_scan->data = bit_masks[ elem & 0x1f ];
				notdone = 0;
				continue;
			}
			/* Are we on the proper block? */
			else if( src_scan->num == block_num )
			{
				dest_scan->num = src_scan->num;
				dest_scan->data = src_scan->data | bit_masks[ elem & 0x1f ];
				notdone = 0;
			}
			/* A block we are not interested in */
			else
			{
				dest_scan->num = src_scan->num;
				dest_scan->data = src_scan->data;
			}
			
			/* Update the pointer */
			src_scan = src_scan->next;
		}

		/* If there were more blocks in the destination, throw them away */
		trim( dest_scan );
	}
}

/*----------------------------------------------------------------------
 *
 * Procedure: delelement                  (Delete an Element from a set)
 *
 * Description: 
 *   Deletes an element from a set, by copying the set to another set, 
 *   and then either simply clearing a bit in a currnet block, or
 *   deleting a block.
 */
void delelement( elem, src_set, dest_set )
unsigned int elem;
struct set_entry *src_set, *dest_set;
{
	register struct set_entry *src_scan, *dest_scan, *temp_scan;
	register unsigned int block_num, temp;

#ifdef   NULL_CHECK
	if( !src_set || !dest_set )
		error( "Null sets passed to 'delelement'" );
#endif

	/* Copy over some values (into regs) */
	block_num = elem >> 5;                        /* Divide by 32 */

	/* Error Check */
	if( elem >= src_set->num )
		error( "element number too large in 'delelement'" );
	if( src_set->num != dest_set->num )
		error( "sets have different sizes in 'delelement'" );

	/* Two cases: 1) src_set == dest_set --or-- 2) src_set != dest_set */
	if( src_set == dest_set )
	{
		/* Scan forward till we find the block (or not find it) */
		for( temp_scan = src_set, src_scan = src_set->next;
                   src_scan && src_scan->num < block_num;
                        temp_scan = src_scan, src_scan = src_scan->next ) ;
		
		/* If we already have the block, set the bit */
		if( src_scan && src_scan->num == block_num )
		{
			/* Mask out the proper bit */
			src_scan->data &= ~bit_masks[ elem & 0x1f ];

			/* If there are no more bits, delete the block */
			if( !src_scan->data )
			{
				temp_scan->next = src_scan->next;
				src_scan->next = free_list;
				free_list = src_scan;
			}
		}
	}
	else  /* 2) src_set != dest_set */
	{
		/* Scan through the src blocks, copying to dest */
		dest_scan = dest_set;
		src_scan = src_set->next;
		for( ; src_scan; src_scan = src_scan->next )
		{
			/* Is this the one to change */
			temp = src_scan->data;
			if( src_scan->num == block_num )
				if( !(temp = temp & ~bit_masks[ elem & 0x1f ]) )
					continue;

			/* Do we have a block to put it? */
			if( !dest_scan->next )
			{
				setalloc( dest_scan->next );
				dest_scan->next->next = NULL;
			}
			dest_scan = dest_scan->next;

			/* Copy it over */
			dest_scan->num = src_scan->num;
			dest_scan->data = temp;
		}

		/* If there were more blocks in the destination, throw them away */
		trim( dest_scan );
	}
}


/*----------------------------------------------------------------------
 *
 * Procedure: setassign                                (copy sets)
 *
 * Description: 
 *   Copies the source set to the destination set.
 */
void setassign( src, dest )
struct set_entry *src, *dest;
{
	struct set_entry *src_scan, *dest_scan;

#ifdef   NULL_CHECK
	if( !src || !dest )
		error( "Null pointers passed to 'setassign'" );
#endif

	/* Error Checking */
	if( src->num != dest->num )
		error( "sets have different size in 'setassign'" );
	
	/* Trivial Case */
	if( src == dest )
		return;

	/* Scan through the src, copying to dest */
	for( src_scan = src->next, dest_scan = dest; 
		                            src_scan; src_scan = src_scan->next )
	{
		/* Do we have a block to put it? */
		if( !dest_scan->next )
		{
			setalloc( dest_scan->next );
			dest_scan->next->next = NULL;
		}
		dest_scan = dest_scan->next;

		/* Copy it over */
		dest_scan->num = src_scan->num;
		dest_scan->data = src_scan->data;
	}	

	/* If there were more blocks in the destination, throw them away */
	trim( dest_scan );
}

/*----------------------------------------------------------------------
 *
 * Procedure: clearset                                    (Clear a set)
 *
 * Description:
 *   Clears all data from a set.  The set header block is still there.
 */
void clearset( src )
struct set_entry *src;
{
	register struct set_entry *src_scan;

#ifdef   NULL_CHECK
	if( !src )
		error( "Null pointer passed to 'setclear'" );
#endif

	if( (src_scan = src->next) )
	{
		/* Find the end of the list */
		for( ; src_scan->next; src_scan = src_scan->next ) ;

		/* Put in the free list */
		src_scan->next = free_list;
		free_list = src->next;	

		/* Cap off the old set */
		src->next = NULL;
	}
}

/*----------------------------------------------------------------------
 *
 * Procedure: addsetrange                        (add range of elements)
 *
 * Description:
 *   This procedure will add a range of elements to a set.  Note that this
 *   procedure does not have a destination, the source is modified.
 */
void addsetrange( low, high, set )
unsigned int low, high;
struct set_entry *set;
{
	register struct set_entry *scan, *old;
	register unsigned int block_low, block_high;

	/* Do some Error Checking */
#ifdef     NULL_CHECK
	if( !set )
		cerror( "Null pointer sent to addsetrange()" );
#endif

	if( low > high )
		cerror( "'low' higher than 'high' in addsetrange()" );

	if( low >= set->num || high >= set->num )
		cerror( "low/high too large for set in addsetrange()" );

	/* Some Calculations */
	block_low = low >> 5;                         /* Divide by 32 */
	block_high = high >> 5;

	/* Scan, find the low block number (if it exists) */
	for( scan = set->next, old = set; 
		            scan && scan->num < block_low; 
                            old = scan, scan = scan->next ) ;

	/* If the low block is not present */
	if( !scan || scan->num != block_low )
	{
		/* Get a new block */
		setalloc( old->next );
		old = old->next;
		old->next = scan;

		/* Fill in some information */
		scan = old;
		scan->num = block_low;
		scan->data = 0;
	}

	/* Set the bits in the low end */
	if( block_low == block_high )
	{
		scan->data |= (bits_high[ low & 0x1f ] & bits_low[ high & 0x1f ]);
		return;                       /* There's a RETURN here */
	}
	else
		scan->data |= bits_high[ low & 0x1f ];

	/* Loop until all the interior blocks have been filled */
	while( ++block_low < block_high )
	{
		/* Scan, for the next block number */
		old = scan;
		scan = scan->next;
		
		/* Get a new block if we need one */
		if( !scan || scan->num != block_low )
		{
			/* Get a new block */
			setalloc( old->next );
			old = old->next;
			old->next = scan;

			/* Fill in some information */
			scan = old;
			scan->num = block_low;
		}			

		/* Fill in the data */
		scan->data = 0xffffffff;
	}

	/* Fill in the high end */
	old = scan;
	scan = scan->next;
	
	if( !scan || scan->num != block_high )
	{
		/* Get a new block */
		setalloc( old->next );
		old = old->next;
		old->next = scan;

		/* Fill in some information */
		scan = old;
		scan->num = block_high;
		scan->data = 0;
	}

	scan->data |= bits_low[ high & 0x1f ];
}
	
/*----------------------------------------------------------------------
 *
 * Procedure: setunion                               (union of two sets)	
 *
 * Description:
 *   This procedure will take the union of two sets, and return this
 *   in yet another set.
 */
void setunion( set1, set2, dest )
struct set_entry *set1, *set2, *dest;
{
	register struct set_entry *scan1, *scan2, *old;

	/* Error Checking */
#ifdef  NULL_CHECK
	if( !set1 || !set2 || !dest )
		error( "Null pointer sent into setunion()" );
#endif
	if( set1->num != set2->num || set2->num != dest->num )
		error( "Sets with wrong size in setunion()" );

	/* Trivial Cases */
	if( set1 == set2 && set2 == dest )
	{
		return;
	}
	else if( set1 == set2 )
	{
		setassign( set1, dest );
		return;
	}

	/* Make set1 be the one that is dest */
	if( set2 == dest )
	{
		scan2 = set2;
		set2 = set1;
		set1 = scan2;
	}
	
	/* Two cases: 1) set1 == dest  --or--  2) set1 != dest */
	if( set1 == dest )
	{
		/* Scan through set2 */
		scan2 = set2->next;
		old = set1;
		scan1 = set1->next;
		for( ; scan2; scan2 = scan2->next )
		{
			/* Scan through set1 */
			for( ; scan1 && scan1->num < scan2->num;
                                          old = scan1, scan1 = scan1->next ) ;

			/* If we have the same block */
			if( scan1 && scan1->num == scan2->num )
			{
				scan1->data |= scan2->data;
			}
			/* We need to add a block */
			else
			{
				setalloc( old->next );
				old = old->next;
				old->next = scan1;
				old->num = scan2->num;
				old->data = scan2->data;
			}
		}
	}
	else
	/* 2) set1 != dest */
	{
		/* Scan through set1/set2 for blocks to add to destination */
		scan2 = set2->next;
		scan1 = set1->next;
		old = dest;
		while( scan2 || scan1 )
		{
			/* Get a block to put it in */
			if( !old->next )
			{
				setalloc( old->next );
				old->next->next = NULL;
			}
			old = old->next;
			
			/* Store the data, and update the source pointers */
			if( scan2 && scan1 )
			{
				if( scan2->num == scan1->num )
				{
					old->num = scan2->num;
					old->data = scan1->data | scan2->data;

					scan2 = scan2->next;
					scan1 = scan1->next;
				}
				else if( scan2->num < scan1->num )
				{
					old->num = scan2->num;
					old->data = scan2->data;

					scan2 = scan2->next;
				}
				else /* if( scan2->num > scan1->num ) */
				{
					old->num = scan1->num;
					old->data = scan1->data;

					scan1 = scan1->next;
				}
			}
			else if( scan2 )
			{
				old->num = scan2->num;
				old->data = scan2->data;

				scan2 = scan2->next;
			}
			else
			{
				old->num = scan1->num;
				old->data = scan1->data;

				scan1 = scan1->next;
			}
		}

		/* Are there any blocks left over */
		trim( old );
	}
}

/*----------------------------------------------------------------------
 *
 * Procedure: intersect                       (intersection of two sets)
 *
 * Description:
 *   This code will take the intersect of two sets, and return the 
 *   result in dest.
 */		
void intersect( set1, set2, dest )
struct set_entry *set1, *set2, *dest;
{
	register struct set_entry *scan1, *scan2, *old;

	/* Error Checking */
#ifdef  NULL_CHECK
	if( !set1 || !set2 || !dest )
		error( "Null pointer sent into intersect()" );
#endif
	if( set1->num != set2->num || set2->num != dest->num )
		error( "Sets with wrong size in intersect()" );

	/* Trivial Cases */
	if( set1 == set2 && set2 == dest )
	{
		return;
	}
	else if( set1 == set2 )
	{
		setassign( set1, dest );
		return;
	}

	/* Make set1 be the one that is dest */
	if( set2 == dest )
	{
		scan2 = set2;
		set2 = set1;
		set1 = scan2;
	}
	
	/* Two cases: 1) set1 == dest  --or--  2) set1 != dest */
	if( set1 == dest )
	{
		/* Scan through set2 */
		scan2 = set2->next;
		old = set1;
		scan1 = set1->next;
		for( ; scan1 && scan2; scan2 = scan2->next )
		{
			/* Scan through set1, deleteing blocks */
			while( scan1 && scan1->num < scan2->num ) 
			{
				old->next = scan1->next;
				scan1->next = free_list;
				free_list = scan1;
				scan1 = old->next;
			}

			/* If we have the same block */
			if( scan1 && scan1->num == scan2->num )
			{
				/* If there is data, we need a block */
				if( scan1->data & scan2->data )
				{
					/* Update the Block */
					scan1->data &= scan2->data;

					/* Update the pointers */
					old = scan1;
					scan1 = scan1->next;
				}
				else
				/* If there is no data, we delete the block */
				{
					old->next = scan1->next;
					scan1->next = free_list;
					free_list = scan1->next;
					scan1 = old->next;
				}
			}
		}

		/* Delete any blocks left in set1 */
		trim( old );
	}
	else
	/* 2) set1 != dest */
	{
		/* Scan through set1/set2 for blocks to add to destination */
		scan2 = set2->next;
		scan1 = set1->next;
		old = dest;
		while( scan2 && scan1 )
		{
			/* Store the data, and update the source pointers */
			if( scan2->num == scan1->num )
			{
				if( scan1->data & scan2->data )
				{
					/* Get a block to put it in */
					if( !old->next )
					{
						setalloc( old->next );
						old->next->next = NULL;
					}
					old = old->next;

					old->num = scan2->num;
					old->data = scan1->data & scan2->data;
				}
				scan2 = scan2->next;
				scan1 = scan1->next;
			}
			else if( scan2->num < scan1->num )
			{
				scan2 = scan2->next;
			}
			else /* if( scan2->num > scan1->num ) */
			{
				scan1 = scan1->next;
			}
		}

		/* Are there any blocks left over */
		trim( old );
	}
}

/*----------------------------------------------------------------------
 *
 * Procedure: difference               (find the difference of two sets)
 *
 * Description: 
 *    Destination set = set1 - set2.  Note that the numbers are switched
 *    in the calling sequence.
 */
void difference( set2, set1, dest )
struct set_entry *set1, *set2, *dest;
{
	register struct set_entry *scan1, *scan2, *old;
	register unsigned int temp;

	/* Error Checking */
#ifdef  NULL_CHECK
	if( !set1 || !set2 || !dest )
		error( "Null pointer sent into difference()" );
#endif
	if( set1->num != set2->num || set2->num != dest->num )
		error( "Sets with wrong size in difference()" );

	/* Trivial Cases */
	if( set1 == set2 )
	{
		clearset( dest );
		return;
	}

	/* Three cases: 1) set1 == dest, 2) set2 == dest, 3) set1,2 != dest */
	if( set1 == dest )
	{
		/* Scan through set2, looking for bits to delete from set1 */
		scan2 = set2->next;
		scan1 = set1->next;
		old = set1;
		for( ; scan1 && scan2; scan2 = scan2->next )
		{
			/* Scan theough set1, looking for a corresponding block */
			for( ; scan1 && scan1->num < scan2->num;
				                            old=scan1, scan1=scan1->next ) ;
			
			/* If same, check for empty block now */
			if( scan1->num == scan2->num )
			{
				if( (temp = (scan1->data ^ scan2->data) & scan1->data) )
				{
					/* Store the new data */
					scan1->data = temp;
				}
				else
				{
					/* Delete the block */
					old->next = scan1->next;
					scan1->next = free_list;
					free_list = scan1;
					scan1 = old->next;
				}
			}
		}
	}
	/* 2) set2 == dest */
	else if( set2 == dest )
	{
		/* Scan through set1, looking for blocks */
		scan2 = set2->next;
		old = set2;
		scan1 = set1->next;
		for( ; scan1; scan1 = scan1->next )
		{
			/* Scan through set2, looking for a match to scan1 */
			while( scan2 && scan2->num < scan1->num )
			{
				/* While lower, delete blocks */
				old->next = scan2->next;
				scan2->next = free_list;
				free_list = scan2;
				scan2 = old->next;
			}

			/* If equal, check for a possible deletion of the block */
			if( scan2 && scan1->num == scan2->num )
			{
				if( (temp = (scan1->data ^ scan2->data) & scan1->data) )
				{
					scan2->data = temp;

					/* Increment so we don't delete this guy */
					old = scan2;
					scan2 = scan2->next;
				}
				else
				{
					/* Delete the block */
					old->next = scan2->next;
					scan2->next = free_list;
					free_list = scan2;
					scan2 = old->next;
				}
			}
			/* We need to add this block */
			else
			{
				setalloc( old->next );
				old = old->next;
				old->next = scan2;

				old->num = scan1->num;
				old->data = scan1->data;
			}
		}

		/* Check for extra blocks in set2 */
		trim( old );
	}
	/* 3) set1,2 != dest */
	else
	{
		/* Scan through set1 for blocks to add to destination */
		scan2 = set2->next;
		scan1 = set1->next;
		old = dest;
		for( ; scan1; scan1 = scan1->next )
		{
			/* Look for scan2 blocks that match */
			for( ; scan2 && scan2->num < scan1->num; scan2 = scan2->next ) ;

			/* Store the data, and update the source pointers */
			if( scan2 && scan2->num == scan1->num )
			{
				if( (temp = ((scan1->data ^ scan2->data) & scan1->data) ) )
				{
					/* Get a block to put it in */
					if( !old->next )
					{
						setalloc( old->next );
						old->next->next = NULL;
					}
					old = old->next;

					old->num = scan2->num;
					old->data = temp;
				}
			}
			else 
			{
				/* Get a block to put it in */
				if( !old->next )
				{
					setalloc( old->next );
					old->next->next = NULL;
				}				
				old = old->next;

				old->num = scan1->num;
				old->data = scan1->data;
			}
		}

		/* Delete extra blocks */
		trim( old );
	}
}

/*----------------------------------------------------------------------
 *
 * Procedure: setcompare                                  (compare sets)
 *
 * Description:
 *   This procedure returns an integer (0-4) which indicates the following:
 *                 0 -- disjoint sets
 *                 1 -- set1 IN set2
 *                 2 -- set2 IN set1
 *                 3 -- set1 == set2
 *                 4 -- set1 and set2 overlap, but no containment
 */
int setcompare( set1, set2 )
struct set_entry *set1, *set2;
{
	register struct set_entry *scan1, *scan2;
	register unsigned int temp;
	register char overlap = 0, onlyin1 = 0, onlyin2 = 0;

	/* Error Checking */
#ifdef   NULL_CHECK
	if( !set1 || !set2 )
		error( "Null sets sent into setcompare()" );
#endif
	if( set1->num != set2->num )
		error( "Sets of different size sent into setcompare()" );

	/* Scan through both sets, looking for bits */
	scan1 = set1->next;
	scan2 = set2->next;
	while( scan1 && scan2 )
	{
		/* If same block, and them; otherwise block gives an 'only' stat */
		if( scan1->num == scan2->num )
		{
			if( scan1->data & scan2->data )
				overlap = 1;

			if( (temp = scan1->data ^ scan2->data) )
			{
				if( scan1->data & temp )
					onlyin1 = 1;
				if( scan2->data & temp )
					onlyin2 = 1;
			}
			
			scan1 = scan1->next;
			scan2 = scan2->next;
		}
		else if( scan1->num < scan2->num )
		{
			onlyin1 = 1;

			scan1 = scan1->next;
		}
		else
		{
			onlyin2 = 1;

			scan2 = scan2->next;
		}
	}

	/* Left over blocks indicate things which are only in one */
	if( scan1 )
		onlyin1 = 1;
	else if( scan2 )
		onlyin2 = 1;

	/* Return a value based on the info we know */
	if( overlap )
	{
		if( onlyin1 && onlyin2 )
			return( 4 );
		else if( onlyin1 )
			return( 2 );
		else if( onlyin2 )
			return( 1 );
		else
			return( 3 );
	}
	else if( onlyin1 || onlyin2 )
		return( 0 );
	else
		return( 3 );
}

/*----------------------------------------------------------------------
 * 
 * Procedure: differentsets        (compares two sets for equality only)
 *
 * Description:
 *   This is a modified version of setcompare, which checks only for
 *   two sets equality.  1 = not equal, 0 = equal.
 */
int differentsets( set1, set2 )
struct set_entry *set1, *set2;
{
	struct set_entry *scan1, *scan2;

	/* Error Checking */
#ifdef   NULL_CHECK
	if( !set1 || !set2 )
		error( "Null sets sent into setcompare()" );
#endif
	if( set1->num != set2->num )
		error( "Sets of different size sent into setcompare()" );

	/* Scan through both sets, looking for bits */
	scan1 = set1->next;
	scan2 = set2->next;
	for( ; scan1 && scan2; scan1 = scan1->next, scan2 = scan2->next )
	{
		/* If same block, and them; otherwise block gives an 'only' stat */
		if( scan1->num == scan2->num )
		{
			if( scan1->data ^ scan2->data )
				return( 1 );
		}
		else 
			return( 1 );
	}
	
	/* If blocks left over, not equal */
	if( scan1 || scan2 )
		return( 1 );

	return( 0 );
}

/*----------------------------------------------------------------------
 *
 * Procedure: nextel                  (find the next element in the set)
 *
 * Description:
 *   Finds the next element which is set.  If -1 is sent in, then search
 *   begins at the beginning, otherwise the search begins AFTER the 
 *   element specified.  If no elements are left, -1 is returned.
 *
 *   This version is a very crude attempt, which will no doubt be very,
 *   very slow.  It uses a very niave approach.
 */
int nextel( elem, set )
int elem;
struct set_entry *set;
{
	register struct set_entry *scan;
	register unsigned int temp, count;

#ifdef  NULL_CHECK
	if( !set )
		error( "Null set passed to nextel()" );
#endif

	if( elem >= (int) set->num )
		error( "element number to high in nextel()" );

	/* We want to start looking on the NEXT bit */
	elem++;                                 

	/* Get to The proper Block Number (to start) */
	temp = elem >> 5;                         /* Divide by 32 */
	for( scan = set->next; scan && scan->num < temp; scan = scan->next ) ;

	/* If no more blocks */
	if( !scan )
		return( -1 );

	/* If the correct block */
	if( scan->num == temp )
	{
		if( !(temp = bits_high[ elem & 0x1f ] & scan->data) )
		{
			if( scan->next )
			{
				scan = scan->next;
				temp = scan->data;
			}
			else
				return( -1 );
		}
	}
	else
		temp = scan->data;

	/* Get the bit */
	for( count = 32; temp; count--, temp <<= 1 ) ;

	return( (scan->num << 5) + count );
}

/*----------------------------------------------------------------------
 *
 * Procedure: isemptyset                             (is the set empty?)
 *
 * Description:
 *   Simply checks to see if a set has been empty.
 */
int isemptyset( set )
struct set_entry *set;
{
#ifdef   NULL_CHECK
	if( !set )
		error( "Null pointer sent to 'isemptyset'" );
#endif

	if( set->next )
		return( 0 );
	else
		return( 1 );
}

/*----------------------------------------------------------------------
 *
 * Procedure: set_size                        (return max size of a set)
 *
 * Description:
 *   Returns the size of a set, right out of the header.
 */
int set_size( set )
struct set_entry *set;
{
#ifdef  NULL_CHECK
	if( !set )
		error( "Null pointer sent into 'set_size'" );
#endif

	return( set->num );
}

/*----------------------------------------------------------------------
 * 
 * Procedure: change_set_size             (modify the max size of a set)
 *
 * Description:
 *    Will change the maximum size of a set.  Note that no scan is made
 *    to insure that there are no blocks which would be invalid under
 *    the new size (when reducing the size).  Care should be taken.
 */
void change_set_size( set, newsize )
struct set_entry *set;
int newsize;
{
#ifdef   NULL_CHECK
	if( !set )
		error( "Null pointer sent into 'change_set_size'" );
#endif

	if( newsize < 0 )
		error( "New size is too small in 'change_set_size'" );

	set->num = newsize;
}

/*----------------------------------------------------------------------
 * 
 * Internal Procedure: get_sets            (allocate internal free list)
 *
 * Description:
 *    This routine is called when the free list is empty, in order to
 *    allocate more memory for sets.  NUM_ENTRIES are malloced at a 
 *    time.
 */
/* static */ void get_sets()
{
	register struct set_entry *scan;
	register int t;

	if( free_list != NULL )
		error( "Sets Free List not NULL in get_sets()" );

	scan = (struct set_entry *) malloc(sizeof(struct set_entry)*NUM_ENTRIES);
	if( scan == NULL )
		error( "Out of memory in get_sets()" );
	
	free_list = scan;
	for( t = NUM_ENTRIES-1; t; scan->next = (scan+1), scan++, t-- ) ;
	scan->next = NULL;
}

/*---------------------------------------------------------------------- 
 * 
 * Internal Procedure: trim                 (trim the rest of this list)
 *
 * Description:
 *   This routine will put all of set->next on the free list, and make
 *   set->next = NULL.  If set is NULL, or set->next is NULL, then a
 *   return is done.
 */
/* static */ void trim( set )
struct set_entry *set;
{
	register struct set_entry *scan;

	/* Trivial Case, checks for NULL */
	if( !set || !set->next )
		return;

	/* Scan for the last block */
	for( scan = set->next; scan->next; scan = scan->next ) ;
	scan->next = free_list;
	free_list = set->next;

	/* Plug the list */
	set->next = NULL;
}

/*----------------------------------------------------------------------
 * 
 * Internal Procedure: error                 (error handler)
 *
 * Dscription:
 *   This routine will produce an error message, and then die.
 */
/* static */ void error( msg )
char *msg;
{
	cerror( msg );         /* Should not return */
	exit( 1 );             /* Just in case we get back here */
}







void set_elem_vector(setp, vecp) SET *setp; long *vecp;
{
	register i;
	for (i= -1; (i = nextel(i, setp)) >= 0; )
			*vecp++ = i;
	*vecp = -1;
}

void kill_free_sets()
{
	register struct set_entry *fp, *sp;
	fp = free_list;
	while (fp)
		{
		sp = fp;
		fp = fp->next;
		free(sp);
		}
	free_list = NULL;
}
/* <<<<<<<<<<<<<<<<<<<<<<<<< End of Sets.c >>>>>>>>>>>>>>>>>>>>>>>>> */
