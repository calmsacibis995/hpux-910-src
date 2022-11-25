/* @(#) $Revision: 64.5 $ */     
#ifdef _NAMESPACE_CLEAN
#define sigspace _sigspace
#define sigblock __sigblock
#define sigsetmask __sigsetmask
#define sigstack _sigstack
#       ifdef   _ANSIC_CLEAN
#define free _free 
#define malloc _malloc
#       endif  /* _ANSIC_CLEAN */
#endif

#include <sys/types.h>
#include <signal.h>
#include <errno.h>

#ifdef	spectrum
/*
 * Spectrum Stacks Must Be Aligned
 *
 * Double Word:	8	(try this one for now)
 * Cache Line:	64
 * Page:	2048
 */
#define	ALIGNSIZE	8
#endif

#ifdef	hp9000s200
/*
 * For 68020, stack should optimally be 32-bit aligned
 */
#define	ALIGNSIZE	4
#endif	hp9000s200

#define	align(ptr,x)	(( ((int)(ptr)) + (((int)(x))-1) ) &~  (((int)(x))-1))

static	int	current_space = 0;
static	caddr_t	stack_buf;

#ifdef _NAMESPACE_CLEAN
#undef sigspace
#pragma _HP_SECONDARY_DEF _sigspace sigspace
#define sigspace _sigspace
#endif
sigspace (ss)
int	ss;
{
	long			save_mask;
	int			previous_space;
	caddr_t			new_buf;
	struct	sigstack	sigst;
	extern	caddr_t		malloc();
	extern	int		errno;

	/* check if currently running on the sigspace stack */
	if (current_space > 0 && (caddr_t)&ss >= stack_buf &&
	    (caddr_t)&ss <= stack_buf + current_space + ALIGNSIZE) {
		errno = EINVAL;
		return (-1);
	}

	previous_space = current_space;

	/* negative ss means just return the current allocation */
	if (ss >= 0) {

		/* make sure no one tries to use the stack */
		/* while we're switching                   */
		save_mask = sigblock (-1L);

		if (ss > 0) {
			new_buf = malloc (ss + ALIGNSIZE);
			if (new_buf == (caddr_t)0) {
				(void) sigsetmask (save_mask);
				errno = ENOMEM;	/* probably alredy is */
				return (-1);
			}

#ifdef hp9000s200
			sigst.ss_sp = (caddr_t) align (new_buf + ss, ALIGNSIZE);
#endif
#ifdef spectrum
			sigst.ss_sp = (caddr_t)align(new_buf,ALIGNSIZE);
#endif
			sigst.ss_onstack = 0;

			if (sigstack (&sigst, (struct sigstack *)0) != 0) {
				/* shouldn't ever happen - but ... */
				(void) sigsetmask (save_mask);
				free (new_buf);
				return (-1);
			}
		}

		if (previous_space > 0)
			free (stack_buf);
		stack_buf = new_buf;
		current_space = ss;
		(void) sigsetmask (save_mask);
	}
	return (previous_space);
}
