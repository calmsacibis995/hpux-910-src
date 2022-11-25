/* @(#) $Revision: 72.1 $ */      
/*
 *	UNIX shell
 *
 *	Bell Telephone Laboratories
 *
 */

/* To use stack as temporary workspace across
 * possible storage allocation (eg name lookup)
 * a) get ptr from `relstak'
 * b) can now use `pushstak'
 * c) then reset with `setstak'
 * d) `absstak' gives real address if needed
 */
#ifdef hp9000s200
#define		sizechk(x)	{if (brkend <= ((x)+1)) growstak(x);}
#else
#define		sizechk(x)
#endif
#define		relstak()	(staktop-stakbot)
#define		absstak(x)	(stakbot+Rcheat(x))
#define		setstak(x)	(staktop=absstak(x))
#define		pushstak(c)	sizechk(staktop+sizeof(staktop)); *staktop++=(c)
#define		zerostak()	(*staktop=0)

/* Used to address an item left on the top of
 * the stack (very temporary)
 */
#define		curstak()	(staktop)

/* `usestak' before `pushstak' then `fixstak'
 * These routines are safe against heap
 * being allocated.
 */
#define		usestak()	{locstak();}

/* for local use only since it hands
 * out a real address for the stack top
 */
extern tchar		*locstak();	

/* Will allocate the item being used and return its
 * address (safe now).
 */
#define		fixstak()	endstak(staktop)

/* For use after `locstak' to hand back
 * new stack top and then allocate item
 */
extern tchar		*endstak();

/* Copy a string onto the stack and
 * allocate the space.
 */
extern tchar		*cpystak();

/* Allocate given ammount of stack space */
extern tchar		*getstak();	

/* A chain of ptrs of stack blocks that
 * have become covered by heap allocation.
 * `tdystak' will return them to the heap.
 */
extern struct blk	*stakbsy;

/* Base of the entire stack */
extern tchar		*stakbas;

/* Top of entire stack */
extern tchar		*brkend;

/* Base of current item */
extern tchar		*stakbot;

/* Top of current item */
extern tchar		*staktop;

/* Used with tdystak */
extern tchar		*savstak();
