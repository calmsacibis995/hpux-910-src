/*
 * @(#)sem_beta.h: $Revision: 1.7.83.4 $ $Date: 93/09/17 18:34:02 $
 * $Locker:  $
 */

#ifndef _SEM_BETA_INCLUDED
#define _SEM_BETA_INCLUDED

#ifdef _KERNEL_BUILD
#include "../h/semglobal.h"
#include "../h/spinlock.h"
#include "../h/mp.h"
#else  /* ! _KERNEL_BUILD */
#include <sys/semglobal.h>
#include <sys/spinlock.h>
#include <sys/mp.h>
#endif /* _KERNEL_BUILD */


/*
 *  Beta class semaphore structure.
 */
typedef	struct b_sema {
#ifdef	SEMAPHORE_DEBUG
	unsigned	char	b_type;
#endif	/* SEMAPHORE_DEBUG */
	unsigned	char	b_lock;
	unsigned 	short	order;
	struct 	proc	*owner;		/* proc structure pointer	*/
#ifdef	SEMAPHORE_DEBUG
	struct 	proc	*last_proc;	/* proc structure pointer	*/
	struct 	b_sema	*next;		/* next semaphore in per-process list */
	struct 	b_sema	*prev;		/* prev semaphore in per-process list */
	struct 	b_sema	*link;		/* link of all beta semaphores	*/
	u_int		b_pcaller;	/* rp of last thread to do p op	*/
	u_int		b_vcaller;	/* rp of last thread to do v op	*/
	char	*name;			/* name of semaphore */
	unsigned char b_deadlock_safe;	/* Can order be violated ? */
	unsigned char	max_misses;	/* Max times the semaphore was stolen */
#endif	/* SEMAPHORE_DEBUG */
} b_sema_t;

typedef	b_sema_t	vm_sema_t;

/*
 *  Beta-hash semaphore
 *    semaphores hashed to by beta class routines
 */
typedef struct bh_sema {
	struct proc *fp, *bp;
#ifdef	SEMAPHORE_DEBUG
	struct bh_sema *link;
#endif	/* SEMAPHORE_DEBUG */
} bh_sema_t;

#define	B_SEMA_HTBL_SIZE	64	/* a power of 2 */

void b_sema_htbl_init();	

#define SEM_LOCKED	0x1
#define SEM_WANT	0x2

#define BETA_MISS_LIMIT	10

#ifdef SEMAPHORE_DEBUG
#define B_TERMSEMA(x) b_termsema(x)
#else
#define B_TERMSEMA(x)
#endif /* SEMAPHORE_DEBUG */

/*
 *  Routine calling macros for Beta class semaphores
 */
#define vm_psema(b_sema, pri)		b_psema(b_sema)
#define vm_vsema(b_sema, junk)		b_vsema(b_sema)
#define vm_cpsema			b_cpsema
#define vm_disownsema			b_disownsema
#define vm_valusema			b_valusema
#define vm_initsema(SEMA, VAL, ORDER, NAME) \
					b_initsema(SEMA, VAL, ORDER, NAME)
#define vm_termsema			B_TERMSEMA


#endif /* _SEM_BETA_INCLUDED */
