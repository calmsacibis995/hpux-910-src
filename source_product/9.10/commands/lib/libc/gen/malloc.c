/* @(#) $Revision: 72.5 $ */

/*

		New Malloc - Memory allocator

		by Edgar Circenis, HP, UDL
		Initial code			8/89
		Cartesian Tree modification	5/91
		Small block allocator		5/91

BLOCK STRUCTURE

    +-- allocptr		  +-- allocptr
    |				  |
    v				  v
  +-+-+----------------+	+-+-+----------------+
  |P|S|                |	|I|S|                |
  |R|I|                |	|N|I|                |
  |E|Z|   USED BLOCK   |	|F|Z|   FREE BLOCK   |
  |V|E|                |	|O|E|                |
  |-|-|   S=0          |	|-|-|   S=0          |
  |H|S|   U=1          |	|H|S|   U=0          |
  |-|-|                |	|-|-|                |
  |F|U|                |	|F|U|                |
  +-+-+----------------+	+-+-+----------------+

  +-- allocptr			+-- allocptr
  |				|
  v				v
  +-+----------------+		+-+----------------+
  |H|                |		|N|                |
  |E|   USED         |		|E|   FREE         |
  |A|   SMALL BLOCK  |		|X|   SMALL BLOCK  |
  |D|                |		|T|                |
  |-|   S=1          |		|-|   S=1          |
  |S|   U=1          |		|S|   U=0          |
  |-|                |		|-|                |
  |U|                |		|U|                |
  +-+----------------+		+-+----------------+
  
  SIZE:	Size of block in bytes (includes overhead).
  H:	If set, this block is being used by the small block allocator as
	either a holding block for small blocks or a holding block header
	block and should be counted as such by mallinfo.
  F:	If set, previous block is free and PREV is a pointer to that block.
  U:	If set, current block is in use.
  S:	If set, this is a small block.
  INFO:	In a free block, this points to the node element representing this
  	block in the free tree.  This field is also used as a block ID field
	for used blocks.
  HEAD: In a used small block, this field points to the holding block header.
  NEXT: When a small block is free, this field is used to implement a linked
	list of free small blocks of the same size.

  When a block is allocated, only the SIZE field is used.  Because the block
  was free, and free blocks are always coalesced, there is no need to populate
  the PREV field (but we do need to clear the F bit).  When a block is
  freed, it is inserted into the free tree of free blocks and the following
  block's PREV field and F bit are set to point back to the free block.

  Malloc block ponters always point to the word immediately preceding the
  start of user data.  This is done to simplify the algorithm, resolve the fact
  that we have two different header sizes, and to improve performance.
  
  NOTE: When reading this code, keep in mind that (header *)ptr+1 is the same
        as (int)ptr+4.

OVERVIEW

  o  This is a general purpose memory allocator.
  o  Free blocks are kept in a cartesian tree, where x=block address
     and y=block size.
  o  The tree is external from the blocks themselves.
  o  Free blocks are always coalesced.
  o  The allocator does a "left most fit" allocation (see references 1 and 3).
  o  A SVID3 small block allocator is implemented.
  o  Preservation of free blocks is always the case.

STANDARDS CONFORMANCE

  In the default configuration (#define _DEFAULT_COMPATIBILITY), the
  following malloc routines conform to the following standards:

  malloc:	XPG3, POSIX.1, FIPS 151-1, ANSI C, SVID3
  calloc:	XPG3, POSIX.1, FIPS 151-1, ANSI C, SVID3
  realloc:	XPG3, POSIX.1, FIPS 151-1, ANSI C, SVID3
  free:		XPG3, POSIX.1, FIPS 151-1, ANSI C, SVID3
  mallopt:	SVID2, SVID3, XPG2
  mallinfo:	SVID2, SVID3, XPG2
  memorymap:	NONE

LISTS

  Free blocks are kept in a cartesian tree (see references 1,3) such that
  for every block S, x=the address of the block, and y=the size of the block.
  The tree is organized so that for each S,
	1. x(l(S))  < x(S) <  x(r(S))
	2. y(l(S)) <= y(S) >= y(r(S))

  This results in a tree where an in-order search will find blocks in address
  order -- this is useful for coalescing, though we have a better way.
  The tree is also ordered from top to bottom by block size.  This is useful
  for implementing a fast, best-fit search algorithm.
  The one drawback to this data structure is that it can degrade to a linked
  list, in which case performance begins to suffer.  However, performance
  should not get any worse than the typical doubly-linked list approach
  to memory allocators.

  The tree header is called: freetree.

  In addition to this tree, all blocks can be accessed in address order by
  using the size field of the block header.  The address of a block plus
  the size of the block yields the address of the next block.  All blocks
  have sizes which are (0 mod 4) to allow the use of the two least significant
  bits in each header word for flags.

MALLOC

  The malloc function does a "leftmost fit" search of the free block tree
  attempting to find a block of suitable size  to fulfill the request.  If
  a block is found and is large enough to be split into two blocks, this
  is done.  If no block can be found, malloc increases the size of the
  arena, maintaining page alignment of the brk value and returns a block
  from the newly acquired memory.

FREE

  The free function takes the block passed to it and clears the used bit.
  Then, it attempts to coalesce the block with the previous and next blocks
  (in that order).  One of the following scenarios is possible:

  1. No coalescing possible:
	- current block is inserted into free tree.

  2. Can only coalesce with previous block:
	- block is absorbed into previous block.
	- free tree node is "bubbled" into correct tree position.

  3. Can only coalesce with following block:
	- following block is removed from free tree.
	- following block is absorbed into current block.
	- current block is inserted into free tree.

  4. Can coalesce with both previous and following blocks:
	- block is absorbed into previous block.
	- following block is removed from free tree.
	- following block is absorbed into previous block.
	- free tree node is "bubbled" into correct tree position.

REALLOC

  The realloc function either increases or decreases the size of a block.

  Decreasing the size of a block:
	- The leftover part of the block, if large enough is coalesced
	  with the following block (if free) and inserted into the free tree. 
  Increasing the size of a block:
	- First, a check is made to see if coalescing with an adjacent
	  free block will create a large enough block.  If this doesn't
	  work, malloc is called to allocate a suitable block.  After a
	  block is created, data is copied to the new block.  The amount
	  of data copied is the lesser of the original and new size.

USER SBRK

  A dummy block header (marked used) is kept at the end of the arena in case
  the user decides to increase the brk value on his own.

  If this happens, it is detected the next time malloc attempts to increase
  the size of the arena.  Malloc correctly sets the size of the dummy block
  and leaves it marked used.  This, in effect, creates a used block so that
  malloc can retains a pretty picture of who owns what memory.  Then, the
  arena is expanded past the end of user-allocated memory and another
  dummy header is set up.

  If no "brk funniness" has taken place, the original dummy header simply
  becomes part of a new free block and another is set up at the new end
  of the arena.

EXTERNAL FREE TREE

  The free block data structure in this algorithm does not reside in the
  free blocks.  Instead, memory is reserved for nodes of the free tree.
  This memory is reserved for NODE_COUNT nodes at a time.  This value
  was originally set at 409 (2 pages of nodes).  Nodes are tracked in two
  ways.  
  1. There is a pointer to a linked list of free nodes.  When a node is
     free, its first field is used as a pointer to the next free node.
     When a node is allocated from this list, it is removed from the head
     of the list.
  2. There is a pointer to virgin nodes and a count of how many virgin
     nodes are pointed to.  When a virgin node is allocated, the pointer
     is advanced to point to the next virgin node and the count of virgin
     nodes is reduced by one.
  When neither virgin nor free nodes are available, a new node block of
  NODE_COUNT nodes is allocated (using sbrk) and is designated as virgin
  node space.

  By using external node pointers, it is possible to walk the free block
  tree (searching for a block of suitable size) without walking through
  memory and causing page faults.

BLOCK IDENTIFIERS

  The INFO field of a block is used to ID a used block.  When a holding
  block header, holding block, user sbrk block, or node block are created
  and marked used, an appropriate identifier tag is placed in the INFO
  field of the block header so that the block can later be identified
  by the memorymap() routine.

  There is only one known problem with this scheme: node blocks are created
  by doing an sbrk without cleaning up.  This fools malloc into thinking that
  the user allocated memory using sbrk.  The result is that the next time
  malloc needs to grow the arena, the grow_arena routine will do the necessary
  cleanup.  However, if both a user sbrk and a node block allocation occur
  between calls to grow_arena, the identifier applied to the block will
  reflect the identity of the first part of the new memory.  If the user
  sbrk occurred before the node block allocation, the ID tag will be USER_ID.
  Otherwise, the tag would be NODE_ID.  This discrepency does not affect
  the operation of malloc.

  If two or more node block allocations occur between calls to grow_arena,
  the resulting block tag will be NODE_ID.

COALESCE AT FREE TIME

  Blocks are coalesced at free time to eliminate free block fragmentation and
  the evils it brings about.  Coalescing at free time ensures that the free
  tree is never fragmented.  This results in larger (coalesced) and fewer
  free blocks, allowing for a shorter and more efficient search when looking
  for a new block.

  By storing the state (free or used) of the previous block in the current
  block and having the a pointer to the free block free block, it is
  possible to implement coalescing of free blocks in a reasonable amount of
  time.  Because the free list is a cartesian tree, tree manipulation
  occurs every time a block is coalesced.  This involves two O(log n)
  operations where n is the depth of the free tree.

SMALL BLOCK ALLOCATOR

  The small block allocator implements the functionality described by the
  following three parameters (see SVID2):

    <maxfast>  The algorithm allocates all blocks below the size of <maxfast>
    in large groups, then doles them out very quickly.  The default value for
    <maxfast> is zero.  <maxfast> is always rounded up to the nearest <grain>
    (see below).

    <numlblks>  The above mentioned "large groups" each contain <numlblks>
    blocks.  <numlblks> must be greater than 1.  The default value for
    <numlblks> is 100.

    <grain>  The sizes of all blocks smaller than <maxfast> are considered to
    be rounded up to the nearest multiple of <grain>.  <grain> must be greater
    than zero.  The default value of <grain> is the smallest number of bytes
    that can accommodate alignment of any data type.

  The small block allocator is built on top of the normal block allocator.
  A small block is distinguished from a large block by the S bit in the
  block header.  If this bit is set, the block is considered to be a small
  block.  The remaining bits (PTR(p)) yield a pointer to the holding block
  header.

  The holding block header contains the following information:
    <blockcnt>	This is the number of small blocks belonging to this holding
		block header.
    <blksize>	This is the size of each small block in the holding block.
		This size includes the 4-byte header field per small block.
    <nextfree>	This is a block pointer which is the head of a list of free
		small blocks in this holding block.
    <numfree>	number of free small blocks in the linked list of small
		block pointed to by <nextfree>.
    <virgin>	A pointer to never-before-used small blocks.  A fresh
		holding block has all virgin small blocks.
    <virgin_cnt>A count of the number of virgin small blocks pointed to
		by <virgin>.
		
  A small block can be allocated either from virgin space or from the small
  block free list.  Preference is given to the virgin space.
  If a small block is allocated from virgin space:
   1. <virgin> is advanced to point to the next virgin small block.
   2. <virgin_cnt> is decremented.
   3. The small block header is updated to point to the holding block
      header and the USED and SMALL bits are set.
  If a small block is allocated from the free list:
   1. <nextfree> is advanced to point to the next free small block in the
      holding block.
   2. <numfree> is decremented.
   3. The small block header is updated to point to the holding block
      header and the USED and SMALL bits are set.

  Each time a small block is freed and the USED bit on the small block header
  is set:
   1. The USED bit on the small block header is cleared.
   2. Insert small block into head of holding block free list <nextfree> and
      increment <numfree>.
  If the USED bit is not set, don't do anything.

  A realloc of a small block involves the following:
  Increasing the size:
   1. Call malloc to allocate a block of new size.
   2. Copy the contents of the small block into the new block.
   3. Free the small block.
  Decreasing size:
   1. Do nothing.  The block is already supposed to be a small block.
      There is no reason to find an even smaller block.
  For realloc of a free small block:
   1. Call malloc to allocate a block of new size.
   2. If the block we get from malloc isn't the same pointer that was passed
      in (it could be because we are reallocing a free block):
      a. figure out how many bytes (up to the new block size) can be
	 copied into the new block.
      b. copy the bytes.
  
  An array of holding block headers is allocated when the first small block is
  requested.  This array contains one header for each available small
  block size (based on inspecting grain and maxfast).
  
  When a small block is requested, the holding block header for the
  appropriate size is looked at to see if either <numfree> or <virgin_cnt>
  are non-zero.  If both are zero, extra space is allocated for small blocks
  and <virgin>, <virgin_cnt>, and <blockcnt> are updated.
  
  NOTE: for PA-RISC, the minimum alignment is 8 bytes.  This poses a problem
  because small block headers are 4 bytes each.  So, what we do is make each
  small block 4 bytes larger than it needs to be.  So, where we would normally
  have a holding block header for 8-byte blocks on PA-RISC, we will actually
  have a header for 12-byte blocks.  This is more efficient.
  
  The following is an example of what things may look like if grain==4,
  maxfast=32, and num_smallblocks=4.  In this picture, the user has
  allocated 5 small blocks of size 3 and then freed three of them.  Two
  are still in use, and three small blocks have never been allocated:
  
     holdhead-----+
                  |
		  v
         +------->+--------------------------------------------------------+
      +--|--------|nextfree      |           |          |       |          |
      |  |        +--------------+           |          |       |          |
      |  |        |numfree=3     | header    | header   |       | header   |
      |  |        +--------------+ for       | for      |       | for      |
      |  |  +-----|virgin        | size=8    | size=12  |       | size=32  |
      |  |  |     +--------------+           |          |  . .  |          |
      |  |  |     |virgin_cnt=3  |           |          |       |          |
      |  |  |     +--------------+           |          |       |          |
      |  |  |     |blocksize=4   |           |          |       |          |
      |  |  |     +--------------+           |          |       |          |
      |  |  |     |blockcnt=8    |           |          |       |          |
      |  |  |     +--------------------------------------------------------+
      |  |  |     
      |  |  |     +-------------------+
      |  |--|-----|  USED             |
      |  |  |     |-------------------|
      |  |  |  +--|  FREE             |<-----+
      |  |  |  |  |-------------------|      |
      |  |  |  +->|  FREE             |-->z  |
      |  |  |     |-------------------|      |
      |  +--|-----|  USED             |      |
      |     |     +-------------------+      |
      |     |                                |
      |     |     +-------------------+      |
      +-----|---->|  FREE             |------+
            |     |-------------------|
            +---->|  VIRGIN           |
                  |-------------------|
                  |  VIRGIN           |
                  |-------------------|
                  |  VIRGIN           |
                  +-------------------+

ALIGNMENT OF LARGE BLOCKS

  For performance reasons, we align blocks 1k or larger on 16 byte boundaries.
  This only makes sense on the 68k architecture because of the mov16
  instruction.  The PA_RISC has a quad store instruction, but no complimentary
  quad load.  16 byte alignment is not necessary in this case.

  To do the alignment, we find a block which is large enough and then we
  work backwards from the end of the block until the front is aligned. 
  In other words, instead of allocating from the front end of a free block,
  we allocate from the rear and pad the block until it is properly aligned.
  
  Another approach which was considered, but causes more fragmentation is to
  first allocate a small block which is large enough to properly align the
  following block.  Then, when the following block is allocated, the small
  block is freed.  This was a kludgy method.

KNOWN PROBLEMS

  The ugly practice of realloc after free is supported by this code.

  If _ZERO_RETURNS_NULL is defined, it makes little sense to create free
  block fragments where the user data size is zero.

REFERENCES
  1. C.J. Stephenson, "Fast Fits - New methods for dynamic storage allocation",
  Proceedings of the ACM 9th Symposium on Operating Systems Review, Vol. 17,
  No. 5, Oct 1983.

  2. David G. Korn and Kiem-Phong Vo, "In Search of a Better Malloc",  USENIX
  Summer Conference Proceedings, Portland 1985.

  3. G. Bozman, et-al, "Analysis of free-storage algorithms -- revisited",
  IBM Systems Journal, Vol. 23, No. 1, 1984.

  4. Jean Vuillemin, "A Unifying Look at Data Structures".
  CACM, Vol. 23, No. 4, April 1980.


EXPLANATION OF FEATURE TEST MACROS:

DEBUG:
	Enable assertions during calls to malloc, free, and realloc.

LONGDEBUG:
	Enable full arena checking after each malloc, free, and realloc call.

PDEBUG:
	Enable printing of arena after each malloc, free, and realloc call.

PROFILE:
	Makes static functions global for profiling.

_3X_COMPATIBILITY:
	Enable 7.0 malloc(3X) compatibility.

_3C_COMPATIBILITY:
	Enable 7.0 malloc(3C) compatibility.

_NO_LUGGAGE:
	This gives you the fastest version of this algorithm by ignoring
	the compatibility issues addressed below.

_SMALL_BLOCKS:
	Adds SVID2 small block allocator functionality.

_SIGNAL_BLOCKING:
	_SIGNAL_BLOCKING is incremental functionality on top of the small
	block allocator, which allows a user to specify that all signals
	are to be blocked while in malloc routines.

_DIAGNOSTICS:
	Possible future enhancement to add user routines to help with
	debugging allocation problems.  If this is done, it should not be
	done in the standard malloc.  Instead, a separate .o file should
	be provided (in, /usr/lib/debug/malloc.o perhaps).  (NOT IMPLEMENTED)

_DONT_MUNGE_FREE_BLOCKS:
	The old malloc(3C) does not change the contents of a free block.
	The cost of this feature is 12 bytes per block overhead.

_FREE_TWICE:
	Most mallocs allow you to free blocks twice.  This feature has little
	associated cost.  If this feature is turned off, blocks may NOT be
	freed twice!  Malloc will break if this is attempted.

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

_WHAT_STRING:
	This puts a revision string (denoting 3C or 3X malloc) into the
	compiled object.

_SIGNAL_HANDLER:
	Include signal handler code which does a balance check on the
	free tree, does a dump of the arena, and also does a consistency
	check of the malloc data structures.

*/

/*
   If 3X or 3C compatibility are defined, they override the default.
   Notice that the default is to provide a combination of 3C and 3X
   behavior, the intent being to eliminate malloc(3X) entirely.  With
   the default configuration, a small block allocator is provided (ala 3X)
   and requests for zero size blocks return a real pointer (ala 3C).  So,
   now, this version can be put into libc.a and libmalloc.a can become a
   link to libempty.a.
*/
#if !defined(_3C_COMPATIBILITY) && !defined(_3X_COMPATIBILITY)
#	define _DEFAULT_COMPATIBILITY
#endif

#ifdef __hp9000s300
#define _ALIGN_LARGE_BLOCKS
#endif

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
#	define _SMALL_BLOCKS
#	define _ZERO_RETURNS_NULL
#	define _FREE_TWICE
#	define _REALLOC_AFTER_FREE
#	define _PASCAL_KLUDGE
#	define _SIGNAL_BLOCKING
#endif

#ifdef _3C_COMPATIBILITY
#	define _DONT_MUNGE_FREE_BLOCKS
#	define _FREE_TWICE
#	define _REALLOC_AFTER_FREE
#	define _PASCAL_KLUDGE
#endif

#ifdef _DEFAULT_COMPATIBILITY
#	define _SMALL_BLOCKS
#	define _FREE_TWICE
#	define _REALLOC_AFTER_FREE
#	define _PASCAL_KLUDGE
#	define _SIGNAL_BLOCKING
#endif

/*
   Until the idea of block overhead is better coded, we need this kind
   of a kludge to prevent people from using feature test macros that
   don't work independent of each other.
#ifdef _ALIGN_LARGE_BLOCKS
#	ifndef _DONT_MUNGE_FREE_BLOCKS
#		define _DONT_MUNGE_FREE_BLOCKS
#	endif
#endif
*/

#ifdef _WHAT_STRING
#  ifdef _3X_COMPATIBILITY
static char ___malloc_id[]="@(#) malloc(3X) $Revision: 72.5 $";
#  else
static char ___malloc_id[]="@(#) malloc(3C) $Revision: 72.5 $";
#  endif
#endif

/*  Lines added to clean up ANSI/POSIX namespace */
#ifdef _NAMESPACE_CLEAN
#   ifdef   _ANSIC_CLEAN
#       define malloc _malloc
#       define free _free
#       define realloc _realloc
#   endif
#   ifdef _SMALL_BLOCKS
#	define mallopt _mallopt
#	define mallinfo _mallinfo
#   endif
#   ifdef _SIGNAL_BLOCKING
#	define sigfillset   _sigfillset
#	define sigprocmask  _sigprocmask
#   endif
#   define memorymap _memorymap
#   define sbrk _sbrk
#   define brk __brk
#   define memcpy _memcpy
#   define memset _memset
#   define printf _printf
#   if defined(__hp9000s300) && defined(_PASCAL_KLUDGE)
#      define asm_mhfl _asm_mhfl
#   endif
#endif

#ifdef DEBUG
#include <assert.h>
#else
#define assert(EX)
#endif

#if defined(_SIGNAL_BLOCKING) || defined(_SIGNAL_HANDLER)
#include <signal.h>
#   ifdef _SIGNAL_HANDLER
void sig_handler();
#   endif
#endif

#ifdef _SMALL_BLOCKS
#include <stdlib.h>
#endif

#include <errno.h>
#ifdef _THREAD_SAFE
#include "rec_mutex.h"

extern REC_MUTEX _mem_rmutex;
#endif

/* debug defines for quickcheck routine */
#if defined(DEBUG) || defined(_SIGNAL_HANDLER)
#	define Q_ENTRY   0
#	define Q_MALLOC  1
#	define Q_FREE    2
#	define Q_REALLOC 3
#	define Q_GROW	 4
#endif

#ifdef PROFILE
#	define static
#endif

#define MAX(a,b) (((a)>(b))?(a):(b))

#define NULL		0
#define ALLOCSIZE	4096	/* amount to increase brk value by */
#define OVERHEAD	(2*sizeof(header))	/* normal block overhead */
#define SM_OVERHEAD	sizeof(header)		/* small block overhead */
#define HEADER_OFFSET	sizeof(header)	/* header offset from user data */

/*
   The series 800 needs malloc()'ed blocks aligned on 8 byte boundaries
   since the s800 floating point hardware assumes 8 byte alignment for
   floating point data types (doubles).
*/
#if defined(__hp9000s800) || defined(__hp9000s700)
#define ALIGNSIZE 8	/* PA_RISC needs 8 byte alignment */
#else
#define ALIGNSIZE 16	/* 68k needs 4 byte alignment */
#endif

#ifdef _SMALL_BLOCKS
/*
   For small blocks, we need a bit of a kludge for PA-RISC.  Because the
   minimum grain is 8 and the alignsize is 8 and the small block header
   size is 4, we cannot guarantee that two consecutive small blocks of
   size 8 (which is actually 12 when the header is added in) will both be
   aligned.  So, we add ALIGNMENT_OFFSET to the size of each small block
   (bumping 12-->16).  This causes consecutive small blocks to have the
   same alignment.
*/
#define ALIGNMENT_OFFSET ((ALIGNSIZE-SM_OVERHEAD)%ALIGNSIZE)
#endif

/* absolute minimum block size */
#define MINSIZE MAX((2*sizeof(header)+ALIGNSIZE-1)&~(ALIGNSIZE-1),OVERHEAD)
/* minimum free block fragment size */
#define MINFRAGSIZE MINSIZE
/* threshold block size for large block alignment */
#define THRESHOLD 1024+MINSIZE

#ifdef _ALIGN_LARGE_BLOCKS
/* For 68040, using the mov16 instruction doubles bcopy performance from
   kernel to user memory buffers.  For PA-RISC, using the quad-store
   instruction increases bcopy performance by 13%.  In each case, the
   user buffer must be 16-byte aligned.					   */
#define P_ALIGN 16	/* must be a power of two and least 16!!! */
#define P_ALIGNMASK (~(P_ALIGN-1))
#define PROPERLY_ALIGNED(p) (!(((header)p+HEADER_OFFSET)&(P_ALIGN-1)))
#endif /* _ALIGN_LARGE_BLOCKS */

#define NODE_COUNT	409	/* number of nodes to allocate at one time */
				/* this should fit on two pages. */

#define NODE_ID	0xfffffffc	/* ID for node block */
#define USER_ID	0xfffffff8	/* ID for user block */
#define HHED_ID	0xfffffff4	/* ID for holding block header */
#define HBLK_ID	0xfffffff0	/* ID for holding block */

#define BITS_31_AND_30	0xc0000000	/* Mask for bits 31 and 32 */
#define HBLK	0x2	/* Mask for the H bit of the block header */
#define FREE	0x1	/* Mask for the F bit of the block header */
#define SMALL	0x2	/* Mask for the S bit of the block header */
#define USED	0x1	/* Mask for the U bit of the block header */
#define FLAGMASK	(SMALL|USED)	/* mask for S/U bits of header       */
#define SIZEMASK	(~(SMALL|USED))	/* mask for SIZE bits of header      */
#define FREEMASK	(~FREE)		/* mask for PREV/INFO bits of header */
#define SIZE(x)	(*(x)&SIZEMASK)		/* returns size of block in bytes.   */
					/* this includes the size of the     */
					/* header!                           */
#define PTR(x)	(*(x)&SIZEMASK)		/* returns the HEAD field of a small */
					/* block header.		     */
#define PREV(x)  (header *)(*(x-1)&SIZEMASK)/* returns the PREV field of a   */
					/* normal block header.              */
#define INFO(x)  ((node *)(*(x-1)))     /* returns the INFO field of a       */
					/* normal free block header.         */
#define ID(x)	((header)(*(x-1))&SIZEMASK)	/* return the block ID */
#define SET_SIZE(x,s,f)	(*(x)=(s)|(f))	/* set the SIZE and S/U bits	     */
#define SET_HEAD(x,s,f)	(*(x)=(s)|(f))	/* set the HEAD and S/U bits	     */
#ifdef _SMALL_BLOCKS
#define SET_PREV(x,s)	(*(x-1)=((header)(s))|FREE|TST_HBLK(x))/* set the PREV and F bit */
#else
#define SET_PREV(x,s)	(*(x-1)=((header)(s))|FREE)/* set the PREV and F bit */
#endif
#define SET_INFO(x,s)	(*(x-1)=(s))    /* set the INFO field		     */
#define SET_ID(x,s,f)	(*(x-1)=(s)|(f))    /* set the INFO field with a block ID */

/* bit manipulation for USED bit */
#define TST_USED(x)	(*(x)&USED)
#define CLR_USED(x)	(*(x)&= ~USED)
#define SET_USED(x)	(*(x)|= USED)

/* bit manipulation for SMALL bit */
#define TST_SMALL(x)	(*(x)&SMALL)
#define CLR_SMALL(x)	(*(x)&= ~SMALL)
#define SET_SMALL(x)	(*(x)|= SMALL)

/* bit manipulation for FREE bit */
#define TST_FREE(x)	(*(x-1)&FREE)
#define CLR_FREE(x)	(*(x-1)&= ~FREE)
#define SET_FREE(x)	(*(x-1)|= FREE)

/* bit manipulation for HBLK bit */
#define TST_HBLK(x)	(*(x-1)&HBLK)
#define CLR_HBLK(x)	(*(x-1)&= ~HBLK)
#define SET_HBLK(x)	(*(x-1)|= HBLK)

/* Pointer to parent free block in free block tree */
#define PARENT_FREE(x)	(*(header **)((x)+1))
/* Pointer to left subtree in free block tree */
#define LEFT_FREE(x)	(*(header **)((x)+2))
/* Pointer to right subtree in free block tree */
#define RIGHT_FREE(x)	(*(header **)((x)+3))

/* Pointer to next block in memory (not necessarily free) */
#define NEXT_BLOCK(x)   (header *)((header)(x)+SIZE(x))

/* This macro will insert a previously used block at the head of the freetree */
#define ADD_TO_FREELIST(allocptr) \
{ \
	node *n; \
	if (free_node) { \
		n=free_node; \
		free_node=free_node->parent; /* next node */ \
	} else if (virgin_cnt) { \
		n=virgin_node++; \
		virgin_cnt--; \
	} else {\
		if ( node_alloc()==0 ) {     /* if this fails, we're HOSED! */ \
		   errno=ENOMEM; \
		   return ; \
		} \
		n=virgin_node++; \
		virgin_cnt--; \
	} \
	n->size=SIZE(allocptr); \
	n->block=allocptr; \
	SET_INFO(allocptr,(header)n); \
	assert(checktree(freetree)); \
	tree_insert(freetree,n); \
	assert(checktree(freetree)); \
	SET_PREV(NEXT_BLOCK(allocptr),(header)allocptr); \
}

/* This macro will remove a free block from the freetree */
#define REMOVE_FROM_FREELIST(x) \
	assert(checktree(freetree)); \
	tree_delete((node *)INFO(x),1); \
	assert(checktree(freetree));

#define ADJUST(x) \
		tree_adjust((node *)INFO(x)); \
		assert(checktree(freetree));

typedef unsigned int header;	/* must be four bytes */
/*
   NOTE: since a node is by definition a free block, we can be assured that
	 the size field actually represents the size of the block.  The flag
	 bits are both zero.
*/
typedef struct nodetype node;
struct nodetype {	/* free block header data structure */
	node	*parent;	/* parent ptr XOR'ed with block size */
	node	*left;		/* left child */
	node	*right;		/* right child */
	header	size;		/* size of block */
	header	*block;		/* pointer to block */
};

static node   *freetree;	/* root of free block tree */
static header *arenaend;	/* pointer to last (dummy) header in arena */
static header *arenastart;	/* pointer to first block in arena */
static header *lastblock;	/* pointer to last block in arena */
static header lastbrk;		/* keeps track of brk value */
static int    no_expand;	/* flag to inhibit expansion in malloc */
extern header _curbrk;		/* brk value set by brk() and sbrk() */
static header *grow_arena();	/* function to grow arena */
static int    node_alloc();	/* function to allocate nodes */
static node   *free_node;	/* pointer to free tree nodes */
static node   *virgin_node;	/* pointer to virgin tree nodes */
static unsigned int    virgin_cnt;	/* number of virgin tree nodes */
#ifdef DEBUG
static header total_node_count;	/* total number of tree nodes */
#endif

#if defined(__hp9000s300) && defined(_PASCAL_KLUDGE)
#   ifdef _NAMESPACE_CLEAN
#       undef  asm_mhfl
#       pragma _HP_SECONDARY_DEF _asm_mhfl asm_mhfl
#       define asm_mhfl _asm_mhfl
#   endif /* _NAMESPACE_CLEAN */
unsigned char asm_mhfl = 0; /* for Pascal kludge -- first time flag */
#endif /* __hp9000s300 */

#ifdef _SMALL_BLOCKS
typedef struct holdheadtype holding_head;
struct holdheadtype {
	header blksize;		/* size of each small block (incl. header)  */
	header *nextfree;	/* head of linked list of free small blocks */
	header	numfree;	/* number of free blocks in <nextfree> list */
	header *virgin;		/* pointer to first virgin small block      */
	header virgin_cnt;	/* number of virgin small blocks            */
	header blockcnt;	/* number of small blocks of this size      */
};

static holding_head *holdhead;	/* array of holding block headers */
static header fastct;		/* number of holding block headers */
static int change;		/* flag to tell us if small blocks allocated */
static int m_keep=0;		/* flag for m_keep option.	   */

static header num_smallblocks=100;     /* SVID default -- DO NOT CHANGE */
static header grain=ALIGNSIZE;         /* SVID default -- DO NOT CHANGE */
static header maxfast=0;               /* SVID default -- DO NOT CHANGE */
#endif /* _SMALL_BLOCKS */

#ifdef _SIGNAL_BLOCKING
static int sigblk;		/* signal blocking flag */

#define UNBLOCK \
	if (sigblk) sigprocmask(SIG_SETMASK,&oset,&set);

#define BLOCK \
	if (sigblk) { \
                sigfillset(&set); \
		sigprocmask(SIG_SETMASK,&set,&oset); \
	}
#else
#define BLOCK
#define UNBLOCK
#endif


/*-----------------------------------------------------------------------------
 *
 * Support routines for cartesian tree manipulation
 *	
 *
 * tree_cut() -- Cartesian tree support routine.
 *	Cut a cartesian tree into two cartesian subtrees such that all
 *	x in the left subtree are < all x in the right subtree.
 *	recursive definition:
 *		1. start with node = root.
 *		2. if node(x)>=n, node belongs to right subtree and cut
 *		   proceeds to examine left branch off of node.
 *		3. else if node(x)<n, node belongs to left subtree
 *		   and cut proceeds to examine right branch off of node.
 *----------------------------------------------------------------------------*/
static node
tree_cut(CT,c)	/* cut the cartesian tree so that v(L),x<c and v(R),x>=c */
node *CT;
header *c;
{
	node LR;
	node *lparent,*rparent;

	LR.left=LR.right=NULL;
	lparent=rparent=NULL;
	while (CT) {
		if (c<CT->block) {
			if (rparent) {
				rparent->left=CT;
				CT->parent=rparent;
			}
			if (!LR.right) LR.right=CT;
			rparent=CT;
			CT=CT->left;
		} else {
			if (lparent) {
				lparent->right=CT;
				CT->parent=lparent;
			}
			if (!LR.left) LR.left=CT;
			lparent=CT;
			CT=CT->right;
		}
	}
	if (rparent) rparent->left=NULL;
	if (lparent) lparent->right=NULL;
	return LR;
}

/*-----------------------------------------------------------------------------
 * tree_insert() -- Cartesian tree support routine.
 *	Insert a free block into a cartesian tree.
 *	Procedure:
 *		Using x coordinate (address), follow tree until the
 *		y coordinate (size) of the block to be inserted is
 *		larger than the block we're at.  Then, cut the current node
 *		using x=address of node to be inserted.  This produces
 *		a left and right subtree which we can be linked on to
 *		the node we're inserting.
 *----------------------------------------------------------------------------*/
static void
tree_insert(CT,p)	/* insert the node p into cartesian tree CT */
node *CT,*p;
{
	node **link;
	node *parent;
	node LR;

	link=&(freetree);	/* our default parent is the tree pointer */
	parent=NULL;
	while (CT) {
		if (p->size >= CT->size) {
			*link=p;	/* link to parent */
			p->parent=parent;
			LR=tree_cut(CT,p->block);
			if (p->left=LR.left)
				LR.left->parent=p;
			if (p->right=LR.right)
				LR.right->parent=p;
			return;
		} else if (p->block < CT->block) {
			parent=CT;
			link=&(CT->left);
			CT=CT->left;
		} else /* p->block>=CT->block */ {
			parent=CT;
			link=&(CT->right);
			CT=CT->right;
		}
	}
	*link=p;
	p->parent=parent;
	p->left=p->right=NULL;
}

/*-----------------------------------------------------------------------------
 * tree_concatenate() -- Cartesian tree support routine.
 *	Given two cartesian subtrees, such that all x in the left subtree
 *	are less than all x in the right subtree, merge the two trees
 *	into one cartesian tree.
 *
 *	This code is confusing -- it implements a non-recusrsive version of
 *	the following:
 *
 *	static node *tree_concatenate(L,R)
 *	node *L,*R;
 *	{
 *	    if (!L) return R;
 *	    else if (!R) return L;
 *	    else if (L->size > R->size) {
 *		if (L->right=tree_concatenate(L->right,R)) L->right->parent=L;
 *		return L;
 *	    } else if (L->size <= R->size) {
 *		if (R->left=tree_concatenate(L,R->left)) R->left->parent=R;
 *		return R;
 *	    }
 *	}
 *----------------------------------------------------------------------------*/
#define LEFT=0
#define RIGHT=1
static node *
tree_concatenate(L,R)	/* Concatenate Left and Right subtrees */
node *L,*R;		/* all x in L must be less than all x in R */
{
	node *parent,*tree,*mover,*other;
	int dir;

	if (!L) return R;
	if (!R) return L;
	if (L->size > R->size) {
		tree=parent=L;
		other=L->right;
		mover=R;
		dir=RIGHT;
	} else {
		tree=parent=R;
		other=R->left;
		mover=L;
		dir=LEFT;
	}
	while (other) {
		if (dir==RIGHT) {
			if (!other || mover->size>other->size) {
				mover->parent=parent;
				parent->right=mover;
				parent=mover;
				mover=other;
				other=parent->left;
				dir=LEFT;
			} else {
				parent=parent->right;
				other=other->right;
			}
		} else /* LEFT */{
			if (!other || mover->size>other->size) {
				mover->parent=parent;
				parent->left=mover;
				parent=mover;
				mover=other;
				other=parent->right;
				dir=RIGHT;
			} else {
				parent=parent->left;
				other=other->left;
			}
		}
	}
	if (dir==RIGHT) {
		mover->parent=parent;
		parent->right=mover;
	} else /* LEFT */ {
		mover->parent=parent;
		parent->left=mover;
	}
	return tree;
}

/*-----------------------------------------------------------------------------
 * tree_delete() -- Cartesian tree support routine.
 *	Delete a node from a cartesian tree.  The basic idea here is that
 *	the left and right subtrees of <p> are themselves cartesian trees.
 *	Further, all x in the left subtree are less than all x in the
 *	right subtree.  So, we concatenate the two subtrees and link to
 *	the parent of the node we are removing.
 *----------------------------------------------------------------------------*/
static void
tree_delete(p,del)	/* delete a node from a cartesian tree */
node *p;
int del;	/* if non zero, return node to free pool */
{
	node *x;

	if ((!p->left) || (!p->right))
		x=(node *)((int)(p->left)|(int)(p->right));
	else
		x=tree_concatenate(p->left,p->right);
	if (p==freetree)
		freetree=x;
	else if (p->parent->left == p)
		p->parent->left=x;
	else
		p->parent->right=x;
	if (x)
		x->parent=p->parent;
	if (del) {
		/* put deleted node on free list */
		p->parent=free_node;
		free_node=p;
	}
}

/*-----------------------------------------------------------------------------
 * tree_adjust() -- Cartesian tree support routine.
 *	After increasing the size of a node, the node needs to be adjusted
 *	so that the tree retains its cartesian properties.
 *----------------------------------------------------------------------------*/
static void
tree_adjust(p) /* make sure a node p is in correct position after re-size */
node *p;
{
	node *parent,*grandparent;

	while ((parent=p->parent) && p->size > p->parent->size) {
		/* tree not cartesian */
		grandparent=parent->parent;
		/* p->parent=NULL; */
		if (parent->left == p) {
			if (!p->right) {
				p->right=parent;
				parent->left=NULL;
			} else {
				p->right->parent=parent;
				parent->left=p->right;
				p->right=parent;
			}
		} else /* parent->right == p */ {
			if (!p->left) {
				p->left=parent;
				parent->right=NULL;
			} else {
				p->left->parent=parent;
				parent->right=p->left;
				p->left=parent;
			}
		}
		parent->parent=p;
		if (grandparent) {
			if (grandparent->left==parent) {
				grandparent->left=p;
			} else {
				grandparent->right=p;
			}
			p->parent=grandparent;
		} else {
			freetree=p;
			p->parent=NULL;
			return;
		}
	}
}

#if defined(_NAMESPACE_CLEAN) && defined(_ANSIC_CLEAN)
#   undef  malloc
#   pragma _HP_SECONDARY_DEF _malloc malloc
#   define malloc _malloc
#endif /* _NAMESPACE_CLEAN && _ANSIC_CLEAN */

/*-----------------------------------------------------------------------------
 * Malloc() -- allocate a block of memory out of the arena
 *	Malloc returns a pointer to a block of the size requested.
 *	Malloc(0) returns a NULL pointer or allocated a zero size block
 *	based on whether _ZERO_RETURNS_NULL is defined.
 *	If malloc cannot allocate memory requested, it returns NULL.
 *----------------------------------------------------------------------------*/
void *
malloc(nbytes)
unsigned int nbytes;
{
	node *freeblock;
	header s;
	header *allocptr,*tempblk;
#ifdef _SMALL_BLOCKS
	int i,index;
	holding_head *hblk;
#endif
#ifdef _SIGNAL_BLOCKING
	sigset_t set,oset;
#endif

	/* check if the amount of memory requested is legal value */
	if ( nbytes & BITS_31_AND_30 ) {
	    errno=ENOMEM;
	    return(NULL);
	}

	assert(quickcheck(Q_ENTRY));
	assert(lastblock==NULL || NEXT_BLOCK(lastblock)==arenaend);
#ifdef _ZERO_RETURNS_NULL
	/* libmalloc.a malloc() returns NULL for zero size blocks */
	if (nbytes==0) return(NULL);
#endif

#ifdef _THREAD_SAFE
	_rec_mutex_lock(&_mem_rmutex);
#endif
	BLOCK;
	if (!arenaend) {	/* first time through */
#ifdef _ALIGN_LARGE_BLOCKS
		arenastart=arenaend=(header *)(((_curbrk+OVERHEAD+ALIGNSIZE-1)&~(ALIGNSIZE-1))+P_ALIGN-HEADER_OFFSET);
#else
		arenastart=arenaend=lastblock=(header *)(((_curbrk+OVERHEAD+ALIGNSIZE-1)&~(ALIGNSIZE-1))-HEADER_OFFSET);
#endif /* _ALIGN_LARGE_BLOCKS */
		if (brk((char *)arenaend+HEADER_OFFSET) == -1) {
			UNBLOCK;
#ifdef _THREAD_SAFE
			arenaend=NULL;
			_rec_mutex_unlock(&_mem_rmutex);
			return(NULL);
#else
			return(arenaend=NULL);
#endif
		}
		SET_SIZE(arenaend,0,USED);	/* mark dummy as used */
		/* keep track of brk value */
		lastbrk = _curbrk;
#if defined(__hp9000s300) && defined(_PASCAL_KLUDGE)
		asm_mhfl = 1;	/* Pascal kludge */
#endif
#ifdef _SIGNAL_HANDLER
		signal(SIGUSR2,sig_handler);
#endif
		if (!node_alloc()) {
			UNBLOCK;
#ifdef _THREAD_SAFE
			_rec_mutex_unlock(&_mem_rmutex);
#endif
			return NULL;
		}
	}

#ifdef _SMALL_BLOCKS
	if (maxfast && nbytes<=maxfast) {
		if (!change) { /* initialize small block allocator first time */
			/* temporarily alter maxfast to avoid recursion */
			i=maxfast; maxfast=0;
			/* allocate space for holding block header lists */
			holdhead=(holding_head *)malloc(sizeof(holding_head)*fastct);
			/* reset maxfast */
			maxfast=i;
			/* do some error checking */
			if (!holdhead) {
				UNBLOCK;
#ifdef _THREAD_SAFE
				_rec_mutex_unlock(&_mem_rmutex);
#endif
				return NULL;
				/*
			   	We weren't able to allocate space for the
				holding block headers.  So, we keep change==0
				and return NULL; errno set to whatever malloc
				set it to.
				*/
			}
			/* set H bit so that this block is counted correctly */
			/* also set holding block header ID */
			SET_ID((header *)((header)holdhead-HEADER_OFFSET),HHED_ID,HBLK);
			/* NULL the holding block headers */
			for (i=0;i<fastct;i++) {
				holdhead[i].nextfree=NULL;
				holdhead[i].numfree=0;
				holdhead[i].virgin_cnt=0;
				holdhead[i].blockcnt=0;
				holdhead[i].blksize=(i+1)*grain+ALIGNMENT_OFFSET+SM_OVERHEAD;
			}
			/* mallopt can no longer be used to change allocator */
			change=1;
		}
		/* figure out which holding block list to look in */
		index=(nbytes+grain-1-ALIGNMENT_OFFSET)/grain-1;
		if (index<0) index=0;
		/* check to see if we have free blocks in holding blocks */
		hblk=&holdhead[index];
		if (!hblk->nextfree && !hblk->virgin_cnt) {
			/* temporarily alter maxfast to avoid recursion */
			i=maxfast; maxfast=0;
			/* allocate space for holding block header lists */
			tempblk = (header *)malloc(hblk->blksize*num_smallblocks+ALIGNMENT_OFFSET);
			/* reset maxfast */
			maxfast=i;
			/* do some error checking */
			if (!tempblk) {
				UNBLOCK;
#ifdef _THREAD_SAFE
				_rec_mutex_unlock(&_mem_rmutex);
#endif
				return NULL;
				/*
			   	We weren't able to allocate space for a holding
			   	block.  So, we keep change==1 because the small
			   	block allocator is already active.  We return
			   	NULL; errno set to whatever malloc set it to.
				*/
			}
			/* set H bit so that this block is counted correctly */
			/* also set holding block ID. */
			SET_ID((header *)((header)tempblk-HEADER_OFFSET),HBLK_ID,HBLK);
			/* update holding block header */
			holdhead[index].virgin=(header *)((header)tempblk+ALIGNMENT_OFFSET);
			holdhead[index].virgin_cnt=num_smallblocks;
			holdhead[index].blockcnt+=num_smallblocks;
			hblk=&holdhead[index];
		}
		/* get either a free or virgin small block */
		if (hblk->nextfree) { /* get a free block */
			allocptr=hblk->nextfree;
			hblk->numfree--;
			/* remove small block from free list */
			hblk->nextfree=(header *)PTR(allocptr);
		} else { /* allocate a virgin block */
			allocptr=hblk->virgin;
			hblk->virgin_cnt--;
			/* advance virgin pointer */
			hblk->virgin = (header *)((header)hblk->virgin+hblk->blksize);
		}
		/* set pointer, SMALL, and USED bit on small block */
		SET_HEAD(allocptr,(header)hblk,USED|SMALL);
		assert(hblk->numfree<=hblk->blockcnt);
		assert(hblk->virgin_cnt<=num_smallblocks);
		assert(hblk->virgin_cnt+hblk->numfree<=hblk->blockcnt);
		assert((((header)allocptr+SM_OVERHEAD)%ALIGNSIZE)==0);
		UNBLOCK;
#ifdef _THREAD_SAFE
		_rec_mutex_unlock(&_mem_rmutex);
#endif
		return (void *)((header)allocptr+SM_OVERHEAD);
	}
#endif /* _SMALL_BLOCKS */
	/* make sure nbytes is 0 mod ALIGNSIZE. Also, add in header size  */
	/* allocate extra space for links (i.e. overhead) */
	nbytes = (nbytes+OVERHEAD+ALIGNSIZE-1)&~(ALIGNSIZE-1);
#ifndef _DONT_MUNGE_FREE_BLOCKS
	nbytes = (nbytes<MINSIZE)?MINSIZE:nbytes;
#endif

	freeblock = freetree;
loop:
#ifdef _ALIGN_LARGE_BLOCKS
	if (nbytes>=THRESHOLD)
	    while (freeblock) {
		if (freeblock->left && freeblock->left->size >= nbytes)
			freeblock=freeblock->left;
		else if (freeblock->size >= nbytes) {
		    do {
		    	/*
			   If block is aligned properly, we use it.
			   If block is not properly aligned, we will 
			   allocate from the end of the block, pushing
			   the non-aligned block backwards until it is
			   aligned.  The remaining portion of the block is
			   a free block.
			*/
			if (PROPERLY_ALIGNED(freeblock->block) ||
			freeblock->size>=nbytes+P_ALIGN+sizeof(header)) {
			    allocptr=freeblock->block;
			    goto found_aligned;
			} else
			    freeblock = freeblock->parent;
		    } while (freeblock);
		} else
			freeblock=NULL;
	    }
	else
#endif /* _ALIGN_LARGE_BLOCKS */
	    while (freeblock) {
		if (freeblock->left && freeblock->left->size >= nbytes)
		    	freeblock=freeblock->left;
		else if (freeblock->size >= nbytes) {
			allocptr=freeblock->block;
			goto found;
		} else
			freeblock=NULL;
	    }

        /* didn't find a block, must allocate more memory */

#ifdef _ALIGN_LARGE_BLOCKS
	s=nbytes+P_ALIGN+sizeof(header);
#else
	s=nbytes;
#endif
	/* We are expanding the arena to accomodate <s> bytes.  Take
	   into account that there may be a free block at the end of the
	   arena that we can also use. */
	if (!TST_USED(lastblock) && lastbrk==_curbrk) {
		/* lastblock will be merged with new memory */
		s -= SIZE(lastblock);
	}
	if (no_expand || !(allocptr=grow_arena(s))) {
		UNBLOCK;
#ifdef _THREAD_SAFE
		_rec_mutex_unlock(&_mem_rmutex);
#endif
		return((void *)NULL);	/* arena expansion failed */
	}
	assert((((header)allocptr+HEADER_OFFSET)%ALIGNSIZE)==0);

	freeblock=INFO(allocptr);

	/* drop through with big enough block */

#ifdef _ALIGN_LARGE_BLOCKS
found_aligned:
	if (nbytes>=THRESHOLD && !PROPERLY_ALIGNED(allocptr)) {
	    /*
		To properly align the block, we allocate from the end of the
		free block, pading the end of the new block until the start
		of the user data is properly aligned.
	    */
	    /* remove from free tree */
	    assert(checktree(freetree));
	    tree_delete(freeblock,0);
	    assert(checktree(freetree));
	    /* how big does block need to be to force alignment? */
	    tempblk=NEXT_BLOCK(allocptr); /* point to next block */
	    /* tempblk points to block at end of allocptr block */
	    tempblk=(header *)((header)tempblk-nbytes);
	    /* align tempblk so that user data is aligned */
	    tempblk=(header *)((header)(tempblk+1)&P_ALIGNMASK)-1;
	    assert(tempblk>allocptr);
	    /* clear F bit on following block */
	    CLR_FREE(NEXT_BLOCK(allocptr));
	    /* set size of newly allocated block */
	    s=(header)NEXT_BLOCK(allocptr)-(header)tempblk;
	    assert(s>=nbytes);
	    SET_SIZE(tempblk,s,USED);
	    /* set size of remaining free block */
	    s=(header)tempblk-(header)allocptr;
	    assert(s>=OVERHEAD);
	    SET_SIZE(allocptr,s,0);
	    /* Set PREV and F bits on allocated block */
	    SET_PREV(tempblk,allocptr);
	    /* put free block back into tree */
	    freeblock->size=s;
	    tree_insert(freetree,freeblock);
	    if (tempblk>lastblock) lastblock = tempblk;
	    assert(PROPERLY_ALIGNED(tempblk));
	    assert(SIZE(tempblk) >= nbytes);
	    assert(lastblock==NULL || NEXT_BLOCK(lastblock)==arenaend);
	    assert(TST_USED(tempblk));
	    assert(allocptr>=arenastart);
	    assert(quickcheck(Q_MALLOC));
#ifdef _ALIGN_LARGE_BLOCKS
	    assert(nbytes<THRESHOLD||PROPERLY_ALIGNED(tempblk));
#endif
	    assert((((header)tempblk+HEADER_OFFSET)%ALIGNSIZE)==0);
	    /* skip links, return beginning of data */
	    UNBLOCK;
#ifdef _THREAD_SAFE
	    _rec_mutex_unlock(&_mem_rmutex);
#endif
	    return((header *)((header)tempblk+HEADER_OFFSET));
	}
#endif /* _ALIGN_LARGE_BLOCKS */
found:  /* found a block */
	if (freeblock->size >= nbytes+MINFRAGSIZE) { /* split into 2 blocks */
	    	assert(checktree(freetree));
		tree_delete(freeblock,0);
	    	assert(checktree(freetree));
		s = SIZE(allocptr);
		/* mark size of block and USED bit */
		SET_SIZE(allocptr,nbytes,USED);
		/* tempblk is newly split-off free block */
		tempblk = NEXT_BLOCK(allocptr);
		/* store size of block */
		SET_SIZE(tempblk,s-nbytes,0);
		/* set PREV and F bit on following block */
		SET_PREV(NEXT_BLOCK(tempblk),(header)tempblk);
		/* set INFO field on this free block */
		SET_INFO(tempblk,(header)freeblock);
		freeblock->size=s-nbytes;
		freeblock->block=tempblk;
		/* add tempblk back to free list */
		tree_insert(freetree,freeblock);
		if (tempblk>lastblock) lastblock = tempblk;
	} else {			/* use entire block */
		/* remove block from freetree */
	    	assert(checktree(freetree));
		tree_delete(freeblock,1);
	    	assert(checktree(freetree));
		/* clear F bit on following block */
		CLR_FREE(NEXT_BLOCK(allocptr));
		/* mark block as used */
		SET_USED(allocptr);
	}
	assert((tempblk=NEXT_BLOCK(allocptr))==arenaend || !TST_FREE(tempblk));
	assert(lastblock==NULL || NEXT_BLOCK(lastblock)==arenaend);
	assert(TST_USED(allocptr));
	assert(allocptr>=arenastart);
	assert(quickcheck(Q_MALLOC));
#ifdef _ALIGN_LARGE_BLOCKS
	assert(nbytes<THRESHOLD||PROPERLY_ALIGNED(allocptr));
#endif
	assert((((header)allocptr+HEADER_OFFSET)%ALIGNSIZE)==0);
	/* skip links, return beginning of data */
	UNBLOCK;
#ifdef _THREAD_SAFE
	_rec_mutex_unlock(&_mem_rmutex);
#endif
	return((header *)((header)allocptr+HEADER_OFFSET));
}


#if defined(_NAMESPACE_CLEAN) && defined(_ANSIC_CLEAN)
#   undef  free
#   pragma _HP_SECONDARY_DEF _free free
#   define free _free
#endif /* _NAMESPACE_CLEAN && _ANSIC_CLEAN */
/*-----------------------------------------------------------------------------
 * Free() -- Free a block allocated by malloc().
 *	Free(NULL) does nothing.
 *	If the pointer passed to free() is not a pointer to a block
 *	allocated by malloc() the linked list structure will be corrupted.
 *
 * History:
 *	5/26/93 	jlee	See history for node_alloc and mallopt for 
 *			changes in free() behavior.  (Implemented
 *			M_KEEP option).
 *
 *----------------------------------------------------------------------------*/
void
free(p)
void *p;
{
	header *allocptr;
	header *nextblk;
	header s;
	int on_free = 0;
	node *nodeptr;
#ifdef _SIGNAL_BLOCKING
	sigset_t set,oset;
#endif

	assert(quickcheck(Q_ENTRY));
	assert(lastblock==NULL || NEXT_BLOCK(lastblock)==arenaend);
	if (p==(char *)NULL)	/* free() of NULL pointer does nothing */
		return;
#ifdef _THREAD_SAFE
	_rec_mutex_lock(&_mem_rmutex);
#endif
	BLOCK;
	assert((header)p%ALIGNSIZE == 0);
	allocptr=(header *)((header)p-HEADER_OFFSET);
	assert(allocptr>=arenastart);
	assert(allocptr<=(header *)_curbrk);
#ifdef _FREE_TWICE
        if ( !(arenastart <= p && p <= arenaend) ||
             !TST_USED(allocptr)) {
                        /* return if already free */
# ifdef _THREAD_SAFE
                _rec_mutex_unlock(&_mem_rmutex);
# endif
                return;
        }
#else
           assert(TST_USED(allocptr));
        if ( !(arenastart <= p && p <= arenaend) ||
             !TST_USED(allocptr)) {
              errno=EINVAL;
# ifdef _THREAD_SAFE
              _rec_mutex_unlock(&_mem_rmutex);
# endif
              return;
	}
#endif
	CLR_USED(allocptr);	/* so that we can detect multiple free's
				   even after block has been coalesced */
#ifdef _SMALL_BLOCKS
	if (TST_SMALL(allocptr)) { /* This is a small block. */
		holding_head   *hblk;
		
		/* find holding block header */
		hblk=(holding_head *)PTR(allocptr);
		/* insert allocptr into linked list of small blocks */
		*allocptr=(header)hblk->nextfree|SMALL;
		hblk->nextfree=allocptr;
		hblk->numfree++;
		assert(hblk->numfree<=hblk->blockcnt);
		UNBLOCK;
# ifdef _THREAD_SAFE
		_rec_mutex_unlock(&_mem_rmutex);
# endif
		return;
	}
#endif /* _SMALL_BLOCKS */
	/* get size of block to be freed */
	s=SIZE(allocptr);
	if (TST_FREE(allocptr)) {	/* coalesce with previous block */
		if (allocptr==lastblock) { /* need to reset lastblock */
			/* get previous block's header */
			lastblock = allocptr = PREV(allocptr);
		} else {
			/* get previous block's header */
			allocptr = PREV(allocptr);
		}
		/* add size of previous block */
		s += SIZE(allocptr);
		/* increase the size of the free block to absorb new block */
		SET_SIZE(allocptr,s,0);
		on_free++;
		assert(lastblock==NULL || NEXT_BLOCK(lastblock)==arenaend);
	}
	if (!TST_USED(nextblk=NEXT_BLOCK(allocptr))) { /* coalesce with next */
		if (lastblock == nextblk)	/* reset lastblock */
			lastblock = allocptr;
		/* remove nextblk from free list */
		/* but, don't clear the FREE bit on next block */
		tree_delete((node *)INFO(nextblk),1);
#ifdef _REALLOC_AFTER_FREE
		/* set PREV and F bit on next block so we can walk backwards */
		SET_PREV(nextblk,(header)allocptr);
#endif
		/* store new size (previous block will always be in use) */
		SET_SIZE(allocptr,s+SIZE(nextblk),0);
	}
	if (on_free) {
		INFO(allocptr)->size=SIZE(allocptr);
		/* readjust cartesian tree */
		ADJUST(allocptr);
	} else {
	 	/* insert into free tree */
		ADD_TO_FREELIST(allocptr); 
	}
	/* We just freed a block, so set PREV and F bit on next block */
	SET_PREV(NEXT_BLOCK(allocptr),(header)allocptr);
	assert(!TST_USED(allocptr));
	assert(!TST_USED((header *)((header)p-HEADER_OFFSET)));  /* input block marked free */
	assert((nextblk=NEXT_BLOCK(allocptr))==arenaend || TST_FREE(nextblk));
	assert(lastblock==NULL || NEXT_BLOCK(lastblock)==arenaend);
	assert(quickcheck(Q_FREE));
	UNBLOCK;
#ifdef _THREAD_SAFE
	_rec_mutex_unlock(&_mem_rmutex);
#endif
}


#if defined(_NAMESPACE_CLEAN) && defined(_ANSIC_CLEAN)
#   undef  realloc
#   pragma _HP_SECONDARY_DEF _realloc realloc
#   define realloc _realloc
#endif /* _NAMESPACE_CLEAN && _ANSIC_CLEAN */
/*-----------------------------------------------------------------------------
 * Realloc() -- change the size of a block allocated by malloc().
 *	Realloc(NULL,n) will do a malloc(n).
 *	Realloc(p,0) will do a free(p) and return NULL.
 *	Realloc(p,size+n) will increase the size of the block pointed
 *	 to by _p_ by _n_ bytes and will preserve the first _size_ bytes
 *	 of data.
 *	Realloc(p,size-n) will decrease the size of the block pointed
 *	 to by _p_ by _n_ bytes and will preserve any remaining bytes.
 *----------------------------------------------------------------------------*/
void *
realloc(p,n)
void *p;
header n;
{
	void *ptr;
	header s,nbytes,newsize;
	header *allocptr,*tempblk,*newfree;
#ifdef _SMALL_BLOCKS
	holding_head *hblk;
#endif
#ifdef _SIGNAL_BLOCKING
	sigset_t set,oset;
#endif

	assert(quickcheck(Q_ENTRY));
	assert(lastblock==NULL || NEXT_BLOCK(lastblock)==arenaend);
	/* If the input pointer is NULL, then malloc nbytes */
	if (p==(char *)NULL)
		return(malloc(n));
#ifdef _THREAD_SAFE
	_rec_mutex_lock(&_mem_rmutex);
#endif
	BLOCK;
	/* If the input pointer is not NULL and nbytes is 0, free the pointer */
	if (n==0) {
		free(p);
		UNBLOCK;
#ifdef _THREAD_SAFE
		_rec_mutex_unlock(&_mem_rmutex);
#endif
		return((void *)NULL);
	}
	assert((header)p%ALIGNSIZE == 0);
	allocptr=(header *)((header)p-HEADER_OFFSET);
#ifdef _SMALL_BLOCKS
	if (TST_SMALL(allocptr)) { /* this is a small block. */
		/* find holding block header */
		hblk=(holding_head *)PTR(allocptr);
		/* round up allocation request to nearest grain */
		n=((n+grain-1)/grain)*grain;
		if (n+SM_OVERHEAD>hblk->blksize) { /* increasing size */
			/* get a new block */
			ptr=malloc(n);
			/* copy contents to new block */
			memcpy(ptr,p,hblk->blksize-SM_OVERHEAD);
			/* free old block if it isn't already free */
			if (TST_USED(allocptr))
				free(p);
			UNBLOCK;
#ifdef _THREAD_SAFE
			_rec_mutex_unlock(&_mem_rmutex);
#endif
			return ptr;
#ifdef _REALLOC_AFTER_FREE
		} else if (!TST_USED(allocptr)) { /* realloc of free block */
			/* get a new block */
			ptr=malloc(n);
			/*
		   	Figure out how much we can copy.  This is kludgy and
		   	non-efficient, but we don't care because the act of
		   	reallocing a free block is not only anti social, but
		   	also bad programming.  The user deserves what they get.
			*/
			if (ptr != p) { /* because p was free, ptr may be p */
				if ((header)p+n>_curbrk)
					n=_curbrk-(header)p;
				/* copy contents to new block */
				memcpy(ptr,p,n);
			}
			/* we don't free old block -- it's already free */
			UNBLOCK;
#ifdef _THREAD_SAFE
			_rec_mutex_unlock(&_mem_rmutex);
#endif
			return ptr;
#endif
		} else {
			/* do nothing, used block is getting
			   smaller or not changing */
			UNBLOCK;
#ifdef _THREAD_SAFE
			_rec_mutex_unlock(&_mem_rmutex);
#endif
			return p;
		}
	}
#endif /* _SMALL_BLOCKS */
#ifdef _REALLOC_AFTER_FREE
	if (!TST_USED(allocptr)) {  /* need to uncoalesce a free block */
		if (!TST_FREE(allocptr)) {  /* block is start of free block */
			REMOVE_FROM_FREELIST(allocptr);
			CLR_FREE(NEXT_BLOCK(allocptr));
			SET_USED(allocptr);
		} else { /* block embedded in free block, need to split */
			/* find the head of the free block */
			tempblk=PREV(allocptr);
			while (TST_FREE(tempblk)) {
				tempblk=PREV(tempblk);
			}
			s=SIZE(tempblk);
			/* set size of split off block, mark correctly */
			SET_SIZE(allocptr,s-((header)allocptr-(header)tempblk),USED);
			/* remove block from free list */
			REMOVE_FROM_FREELIST(tempblk);
			/* reset size of remaining free block */
			SET_SIZE(tempblk,s-SIZE(allocptr),0);
			ADD_TO_FREELIST(tempblk);
		}
	}
#endif
	assert(allocptr>=arenastart);
	assert(TST_USED(allocptr));
	/* add space for any extra block overhead */
	nbytes = (n+OVERHEAD+ALIGNSIZE-1)&~(ALIGNSIZE-1);
#ifndef _DONT_MUNGE_FREE_BLOCKS
	nbytes = (nbytes<MINSIZE)?MINSIZE:nbytes;
#endif
	s = SIZE(allocptr);
	if (nbytes > s) {	/* increasing size of block */
increase:
		if ((tempblk=NEXT_BLOCK(allocptr))!=arenaend
			&& !TST_USED(tempblk)
			&& (newsize=s+SIZE(tempblk)) >= nbytes) {
			/* realloc into next block */
			if (newsize-nbytes>=MINFRAGSIZE) { /* split in two */

				REMOVE_FROM_FREELIST(tempblk);
				/* set new size of used block */
				SET_SIZE(allocptr,nbytes,USED);
				newfree = NEXT_BLOCK(allocptr);
				/* save size of split-off block */
				SET_SIZE(newfree,newsize-nbytes,0);
				ADD_TO_FREELIST(newfree);
				if (newfree>lastblock) lastblock=newfree;
			} else	{ /* use entire free block */
				SET_SIZE(allocptr,newsize,USED);
				/* remove tempblk from free list */
				REMOVE_FROM_FREELIST(tempblk);
				CLR_FREE(NEXT_BLOCK(tempblk));
				if (tempblk == lastblock ) lastblock=allocptr;
			}
			assert(TST_USED(allocptr));
			assert(allocptr>=arenastart);
			assert(lastblock==NULL || NEXT_BLOCK(lastblock)==arenaend);
			assert(quickcheck(Q_REALLOC));
			UNBLOCK;
#ifdef _THREAD_SAFE
			_rec_mutex_unlock(&_mem_rmutex);
#endif
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
					SET_SIZE(allocptr,s,USED);
					/* remove tempblk from free list */
					REMOVE_FROM_FREELIST(tempblk);
					CLR_FREE(NEXT_BLOCK(tempblk));
					if (tempblk == lastblock )
						lastblock=allocptr;
				}
				/* at this point, allocptr is the last block
				   in the arena */
				if (!(tempblk=grow_arena(nbytes-s))) {
					UNBLOCK;
#ifdef _THREAD_SAFE
					_rec_mutex_unlock(&_mem_rmutex);
#endif
					return((void *)NULL);
				}
				goto increase;
			}
		} else if (ptr=malloc(n)) { /* allocate more memory */
copy:
#ifdef _3C_COMPATIBILITY
			/* don't copy links! */
			(void)memcpy(ptr,p,s-OVERHEAD);
#else
			(void)memcpy(ptr,p,s-sizeof(header));
#endif
			free(p);
			assert((((header)ptr)%ALIGNSIZE)==0);
			UNBLOCK;
#ifdef _THREAD_SAFE
			_rec_mutex_unlock(&_mem_rmutex);
#endif
			return((void *)ptr);

		} else {	/* all else fails */
			UNBLOCK;
#ifdef _THREAD_SAFE
			_rec_mutex_unlock(&_mem_rmutex);
#endif
			return((void *)NULL);
		}

	} else {		/* decrease size of block */
		if (s >= nbytes+MINFRAGSIZE) { /* split off extra part of block */
			/* point to next (already existing) block */
			tempblk = NEXT_BLOCK(allocptr);
			/* set new size of used block */
			SET_SIZE(allocptr,nbytes,USED);
			/* point to new free block */
			newfree = NEXT_BLOCK(allocptr);
			/* if next block is already free... */
			if (!TST_USED(tempblk)) {
				/* calculate size for new free block */
				SET_SIZE(newfree,s-nbytes+SIZE(tempblk),0);
				/* coalesce with next free block */
				REMOVE_FROM_FREELIST(tempblk);
				ADD_TO_FREELIST(newfree);
				if (tempblk == lastblock) lastblock = newfree;
			} else { /* next block is used or doesn't exist */
				/* store size of new free block */
				SET_SIZE(newfree,s-nbytes,0);
				/* insert block at head of free list */
				ADD_TO_FREELIST(newfree);
				/* set FREE bit of next block */
				SET_FREE(tempblk);
				if (tempblk==arenaend)/* we are last block */
					lastblock = newfree;
			}
		}
		assert(allocptr>=arenastart);
		assert(TST_USED(allocptr));
		assert(lastblock==NULL || NEXT_BLOCK(lastblock)==arenaend);
		assert(quickcheck(Q_REALLOC));
		assert((((header)p)%ALIGNSIZE)==0);
		UNBLOCK;
#ifdef _THREAD_SAFE
		_rec_mutex_unlock(&_mem_rmutex);
#endif
		return((void *)p);
	}
}

/*-----------------------------------------------------------------------------
 *  node_alloc() -- allocate more space for free tree nodes.
 *	0 - operation failed
 *	1 - operation succeeded
 *  NOTE: this may seem really stupid, but because of SVID compliance, we
 *	are not allowed to disturb free block contents unless there is an
 *	intervening call to malloc, calloc, or realloc.  That is why we
 *	get to increase the size of the data segment during a call to free.
 *
 *	History:
 *
 *	   5/26/93 	jlee	Implemented M_KEEP flag.  Node alloc will now
 *				use memory that has been Freed if M_KEEP has
 *				not been set (value of 0)  and node_alloc is 
 *				unable to get additional memory from the
 *				kernel  
 * 
 *				If M_KEEP is set and MAXDSIZ amount of storage 
 *				has allocated to the application, then 
 *				node_alloc will ask for more space by doing
 *				a sbrk() function call.  Unfortunately if the
 *				sbrk call should fail, then we will return
 *				a error message (EINVAL) via the value of 
 *				errno.  NOTE:  This method WILL generate 
 *				memory leaks.  Because will not know where this
 *				newly freed block structure is.
 *
 *----------------------------------------------------------------------------*/
static int
node_alloc()
{
	node *ptr, *new_node ;
	header *allocptr;

	ptr=(node *)_curbrk;
	if (lastbrk==_curbrk) {
		/*
		   If there hasn't been any sbrk activity, we can be sure
		   that there is a dummy header at the end of memory.  We
		   will set its ID field to NODE_ID so that grow_arena will
		   not reset it to USER_ID later on.  The only problem here
		   is that in the very unlikely case that a user does an
		   sbrk followed by a free() which causes node_alloc to be
		   called, the entire block (user sbrk memory and the node
		   block) will be marked with USER_ID).  This does not in
		   any way affect the operation of malloc.
		*/
		allocptr=(header *)((header)ptr-HEADER_OFFSET);
		SET_ID(allocptr,NODE_ID,TST_FREE(allocptr));
	}
	if (sbrk(NODE_COUNT*sizeof(node))==-1) {
	   if ( !m_keep ) { /* If m_keep is not set then we can allocate */
			    /* from the free block list. 		 */

	      new_node=(node *)malloc(NODE_COUNT*sizeof(node)); 
	      if ( new_node ) { /* If allocation is successful, then set   */
				/* the pointers for the free tree nodes    */
				/* area	and set the virgin_cnt to 	   */
				/* represent the number of nodes allocated */
	         virgin_node=new_node;
	         virgin_cnt=NODE_COUNT;
	      } else { /* Initial allocate of large block of nodes failed, */
		       /* so now we'll just allocate one node.		   */
	         new_node=(node *)malloc(sizeof(node));
	         if ( new_node ) {
		    virgin_node=new_node;
		    virgin_cnt=1;
	         } else  /* The second malloced failed, so we'll need to */
			 /* return a error condition.			 */
		    return 0; /* Return failure */
	       }  /* end of "if (new_node) then ... else...  " from 	 */
		  /* NODE_COUNT allocation   				 */

#ifdef DEBUG 
	       total_node_count +=NODE_COUNT;
#endif
	       return 1; /* Return successful */

	   } else 
	      return 0; /* Return failure to allocate new node */

	} /* end of sbrk condition */

	/*
	   NOTICE that we don't reset lastblock, arenaend, or do any kind
	   of cleanup.  We don't want to mess with free blocks while we're
	   in this routine.  The cleanup is handled in grow_arena when it
	   detects that someone resized the data segment behind its back.
	*/
	virgin_node=ptr;
	virgin_cnt=NODE_COUNT;
#ifdef DEBUG
	total_node_count+=NODE_COUNT;
#endif
	return 1;
}


/*-----------------------------------------------------------------------------
 * grow_arena() -- we have run out of room and need to increase
 *	the size of the data segment.  Increase by at least nbytes
 *	bytes and return a pointer to the last free block in the
 *	new arena.
 *----------------------------------------------------------------------------*/
static header *
grow_arena(nbytes)
header nbytes;
{
	header s,brkinc;
	header *allocptr;

	assert(quickcheck(Q_ENTRY));
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
		s = (_curbrk-lastbrk+OVERHEAD+ALIGNSIZE-1)&~(ALIGNSIZE-1);
		/* check to see if this is a node block or a user sbrk */
		if (ID(arenaend)!=NODE_ID) {
			/* mark appropriately */
			SET_ID(arenaend,USER_ID,TST_FREE(arenaend));
		};
		/* save size and mark block as used */
		SET_SIZE(arenaend,s,USED);
		/* point to where we will get more memory */
		allocptr=NEXT_BLOCK(arenaend);
		/* calculate new brk value rounding up to a page boundary */
		brkinc = (lastbrk+s+nbytes+ALLOCSIZE-1)&~(ALLOCSIZE-1);
		if (brk(brkinc) == -1) /* get more memory */
			return(NULL);	/* failed to get memory */
		/* move arenaend to point to dummy header */
		arenaend=(header *)(_curbrk-HEADER_OFFSET);
		/* block before arenaend is a free block */
		/* arenaend itself is a used dummy header */
		SET_SIZE(arenaend,0,USED);
		/* allocptr is the last block */
		lastblock=allocptr;
		/* reset my brk pointer */
		lastbrk=_curbrk;
		SET_SIZE(allocptr,(header)_curbrk-(header)allocptr-HEADER_OFFSET,0);
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
			REMOVE_FROM_FREELIST(allocptr);
			CLR_FREE(NEXT_BLOCK(allocptr));
			/* calculate size of new free block */
			SET_SIZE(allocptr,SIZE(allocptr)+brkinc,0);
		} else {	/* need to create new free block out of new memory */
			/* block begins at arenaend (new last block) */
			lastblock = allocptr = arenaend;
			/* store size of block */
			SET_SIZE(allocptr,brkinc,0);
		}
		/* insert block into free list */
		ADD_TO_FREELIST(allocptr);
		/* reset arenaend to point to dummy header */
		arenaend = (header *)((header)arenaend+brkinc);
		/* block preceeding arenaend is a free block */
		/* arenaend is a used dummy block header */
		SET_SIZE(arenaend,0,USED);
		assert(lastblock==NULL || NEXT_BLOCK(lastblock)==arenaend);
	}
	assert(TST_USED(arenaend));
	assert((((header)allocptr+HEADER_OFFSET)%ALIGNSIZE)==0);
	assert(quickcheck(Q_ENTRY));
	return(allocptr);	/* big free block */
}

#ifdef _SMALL_BLOCKS
#ifdef _NAMESPACE_CLEAN
#   undef  mallopt
#   pragma _HP_SECONDARY_DEF _mallopt mallopt
#   define mallopt _mallopt
#endif /* _NAMESPACE_CLEAN */
/*-----------------------------------------------------------------------------
 *  mallopt -- allow user to change small block allocator configuration
 *
 *  History:
 *
 *	5/26/93		jlee	Added processing of the M_KEEP flag. priot to
 *				this patch, M_KEEP was always asssembed off.  
 *				Please see commanet header for node_alloc() for
 *				details regarding the impact of this change.
 * 
 ----------------------------------------------------------------------------*/
int
mallopt(cmd,value)
int cmd;
int value;
{
	int i;
	static int user_maxfast=0;

#ifdef _THREAD_SAFE
	_rec_mutex_lock(&_mem_rmutex);
#endif
#ifdef _SIGNAL_BLOCKING
	if (change && cmd!=M_BLOCK && cmd!=M_UBLOCK)
#else
	if (change)
#endif
#ifdef _THREAD_SAFE
		{ _rec_mutex_unlock(&_mem_rmutex); return 1; }
#else
		return 1;
#endif
	switch(cmd) {
		case M_MXFAST:
			if (value<0)
#ifdef _THREAD_SAFE
				{ _rec_mutex_unlock(&_mem_rmutex); return 1; }
#else
				return 1;
#endif
			user_maxfast=value;
			maxfast=((user_maxfast+grain-1)/grain)*grain;
			break;
		case M_NLBLKS:
#ifdef _THREAD_SAFE
			if (value<=1) { _rec_mutex_unlock(&_mem_rmutex); return 1; }
#else
			if (value<=1) return 1;
#endif
			num_smallblocks=value;
			break;
		case M_GRAIN:
			if (value<0)
#ifdef _THREAD_SAFE
				{ _rec_mutex_unlock(&_mem_rmutex); return 1; }
#else
				return 1;
#endif
			i=(value+ALIGNSIZE-1)&~(ALIGNSIZE-1);
#ifdef _THREAD_SAFE
			if (!i) { _rec_mutex_unlock(&_mem_rmutex); return 1; }
#else
			if (!i) return 1;
#endif
			grain=i;
			maxfast=((user_maxfast+grain-1)/grain)*grain;
			break;
		case M_KEEP:
			/* Do nothing.  M_KEEP is always on */
			m_keep = value;
			break;
#ifdef _SIGNAL_BLOCKING
		case M_BLOCK:
			sigblk=1;
			break;
		case M_UBLOCK:
			sigblk=0;
			break;
#endif
		default:
#ifdef _THREAD_SAFE
			_rec_mutex_unlock(&_mem_rmutex);
#endif
			return 1;
	}
	fastct=maxfast/grain;
#ifdef _THREAD_SAFE
	_rec_mutex_unlock(&_mem_rmutex);
#endif
	return 0;
}
#endif /* _SMALL_BLOCKS */

#ifdef _SMALL_BLOCKS
#ifdef _NAMESPACE_CLEAN
#   undef  mallinfo
#   pragma _HP_SECONDARY_DEF _mallinfo mallinfo
#   define mallinfo _mallinfo
#endif /* _NAMESPACE_CLEAN */
/*-----------------------------------------------------------------------------
 *  mallinfo -- report statistics on memory use
 *----------------------------------------------------------------------------*/
struct mallinfo
mallinfo()
{
	struct mallinfo	m;
	holding_head	*hblk;
	header		*allocptr;
	int		i;

	memset(&m,0,sizeof(m));
#ifdef _THREAD_SAFE
	_rec_mutex_lock(&_mem_rmutex);
#endif
	m.arena=(header)arenaend-(header)arenastart;
	/* follow block list in arena counting used and free blocks */
	allocptr=arenastart;
	while (allocptr != arenaend) {
		if (TST_HBLK(allocptr) && TST_USED(allocptr)) {
			m.hblkhd += SIZE(allocptr);
		} else if (TST_USED(allocptr)) {
			m.uordblks += SIZE(allocptr);
			m.ordblks++;
		} else {
			m.fordblks += SIZE(allocptr);
			m.ordblks++;
		}
		allocptr = NEXT_BLOCK(allocptr);
	}
	/* the keepcost is zero since we never trample free space */
	m.keepcost=0;
	/* if small blocks have been enabled, fill in the statistics */
	if (change) {
		/* for each holding block, count overhead, and used/free */
		for (i=0;i<fastct;i++) {
			hblk=&holdhead[i];
			m.hblks += hblk->blockcnt/num_smallblocks;
			m.smblks += hblk->blockcnt;
			m.fsmblks += (hblk->numfree+hblk->virgin_cnt)*hblk->blksize;
			m.usmblks += (hblk->blockcnt-hblk->numfree-hblk->virgin_cnt)*hblk->blksize;
		}
		m.hblkhd -= m.fsmblks+m.usmblks;
	}
	assert(m.hblkhd+m.usmblks+m.fsmblks+m.uordblks+m.fordblks==m.arena);
#ifdef _THREAD_SAFE
	_rec_mutex_unlock(&_mem_rmutex);
#endif
	return m;
}
#endif /* _SMALL_BLOCKS */

#ifdef _THREAD_SAFE
/*
 * memorymap really should be protected using _mem_rmutex, but
 * since it calls printf, we have a lock ordering problem.
 * We'd like _mem_rmutex to be the lowest order lock, but
 * calling printf would invalidate that.  So for now, memorymap
 * will not be thread-safe.
 */
#endif
#ifdef _NAMESPACE_CLEAN
#   undef  memorymap
#   pragma _HP_SECONDARY_DEF _memorymap memorymap
#   define memorymap _memorymap
#endif /* _NAMESPACE_CLEAN */
/*-----------------------------------------------------------------------------
 *  memorymap -- print a map of the heap to standard out.
 *	flag:	0 - just print a map of the heap.
 *		1 - add auditing info.
 *----------------------------------------------------------------------------*/
void
memorymap(flag)
int flag;
{
	header *allocptr,*tempptr;
	unsigned int num_free,size_free;
	unsigned int num_used,size_used;
	header id;
#ifdef _SMALL_BLOCKS
	unsigned int num_hblk,size_hblk;
#endif

	num_free=size_free=num_used=size_used=0;
#ifdef _SMALL_BLOCKS
	num_hblk=size_hblk=0;
#endif

	allocptr=arenastart;
	while (allocptr != arenaend) {
#ifdef _SMALL_BLOCKS
		if (TST_HBLK(allocptr) && TST_USED(allocptr)) {
			num_hblk++;
			size_hblk += SIZE(allocptr);
			id=ID(allocptr);
			switch (id) {
			case HHED_ID:
			    printf("%10u: holding block header %u\n",allocptr,SIZE(allocptr));
			    break;
			case HBLK_ID:
			default:
			    printf("%10u: holding block %u\n",allocptr,SIZE(allocptr));
			    break;
			}
		} else
#endif
		if (TST_USED(allocptr)) {
			num_used++;
			size_used += SIZE(allocptr);
			id=ID(allocptr);
			switch (id) {
			case NODE_ID:
			    printf("%10u: node block %u\n",allocptr,SIZE(allocptr));
			    break;
			case USER_ID:
			    printf("%10u: user sbrk memory %u\n",allocptr,SIZE(allocptr));
			    break;
			default:
			    printf("%10u: used %u\n",allocptr,SIZE(allocptr));
			    break;
			}
		} else {
			num_free++;
			size_free += SIZE(allocptr);
			printf("%10u: free %u\n",allocptr,SIZE(allocptr));
		}
		tempptr = NEXT_BLOCK(allocptr);
		if (allocptr>=tempptr) {
			printf("Malloc arena corrupted -- aborting; check your code\n");
			return;
		}
		allocptr = tempptr;
	}
	if (flag) {
		printf("\n");
		printf("arenastart               : %10u\n",arenastart);
		printf("arenaend                 : %10u\n",arenaend);
		printf("# of blocks              : %10u\n",num_free+num_used+num_hblk);
		printf("# of free blocks         : %10u\n",num_free);
		printf("# of used blocks         : %10u\n",num_used);
#ifdef _SMALL_BLOCKS
		printf("# of smblk blks          : %10u\n",num_hblk);
#endif
		printf("total space used         : %10u\n",size_free+size_used+size_hblk);
		printf("space in free blocks     : %10u\n",size_free);
		printf("space in used blocks     : %10u\n",size_used);
#ifdef _SMALL_BLOCKS
		printf("space in smblk blocks    : %10u\n",size_hblk);
#endif
	}
	return;
}


#ifdef DEBUG
/*-----------------------------------------------------------------------------
 *  checktree() -- Do an integrity check on the free block tree
 *----------------------------------------------------------------------------*/
int
checktree(t)
node *t;
{
	if (!t) return 1;
	assert (INFO(t->block) == t);
	assert (SIZE(t->block) == t->size);
	if (t->left) {
		if (t->left->parent != t) return 0;
		if (t->left->block >= t->block) return 0;
		if (t->left->size > t->size) return 0;
	}
	if (t->right) {
		if (t->right->parent != t) return 0;
		if (t->right->block <= t->block) return 0;
		if (t->right->size > t->size) return 0;
	}
	return (checktree(t->left)&&checktree(t->right));
}
#endif /* DEBUG */

#if defined(LONGDEBUG) || defined(_SIGNAL_HANDLER)
/*-----------------------------------------------------------------------------
 *  countfree() -- count the number of blocks in the free list
 *----------------------------------------------------------------------------*/int
countfree(t)
node *t;
{
	if (t)
		return 1+countfree(t->left)+countfree(t->right);
	else
		return 0;
}

#ifdef _SMALL_BLOCKS
/*-----------------------------------------------------------------------------
 *  check_small_blocks -- run a consistency check on the small block allocator
 *----------------------------------------------------------------------------*/
static int
check_small_blocks()
{
	holding_head	*hblk;
	header		*ptr,*ptr2;
	unsigned int	i,count;

	if (change) {
		for (i=0;i<fastct;i++) {
			hblk=&holdhead[i];
			ptr=hblk->nextfree;
			count=0;
			while (ptr) {
				count++;
				ptr=(header *)PTR(ptr);
			}
			assert(count==hblk->numfree);
		}
	}
	return 1;
}
#endif /* _SMALL_BLOCKS */

/*-----------------------------------------------------------------------------
 *  quickcheck() -- Check for the following error conditions:
 *	- incorrect links in block list
 *	- incorrect block alignment
 *	- incorrect end of arena
 *	- continguous free blocks
 *	- incorrect use of FREE and USED bits
 *	- bad links in free block tree
 *----------------------------------------------------------------------------*/
int
quickcheck(id)
int id;
{
	header *allocptr,*tempptr,*prev;
	unsigned int free_count=0,i;
	node *n;

	assert(!arenaend || (((header)arenaend+HEADER_OFFSET)%ALIGNSIZE)==0);
	assert(!arenaend || lastbrk!=_curbrk || (header)arenaend+HEADER_OFFSET == _curbrk);
	prev=NULL;
	allocptr=arenastart;
	while (allocptr != arenaend) {
		/* NOTE: we don't check to see if block is correctly aligned
		   because block could be a block created through sbrk in
		   which case, it most certainly will not be correctly
		   aligned since we always end the arena on a page
		   boundary. */
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
	assert(countfree(freetree)==free_count);
	/* Check the free list tree structure */
	assert(checktree(freetree));
	/* Check for a node memory leak */
	i=virgin_cnt+countfree(freetree);
	n=free_node;
	while (n) {
		i++;
		n=n->parent;
	}
	assert(i==total_node_count);
	if (arenaend)
		assert(TST_USED(arenaend));
#ifdef _SMALL_BLOCKS
	assert(check_small_blocks());
#endif
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
                memorymap(1);
#endif /* PDEBUG */

	return(1);
}
#else
#    ifdef DEBUG
	static int
	quickcheck() { return(1); }
#    endif
#endif

#ifdef _SIGNAL_HANDLER
/*-----------------------------------------------------------------------------
 *  sig_handler() -- allow user to interrupt program to get malloc info.
 *	NOTE:	- calling this routine may cause a core dump.
 *		- calling this routine may cause an infinite loop.
 *		- upon return from this handler, errors may occur.
 *----------------------------------------------------------------------------*/
void
sig_handler()
{
	memorymap(1);
	if (!quickcheck(Q_ENTRY))
		printf("consistency check failed\n");
	else
		printf("consistency check passed\n");
}
#endif
