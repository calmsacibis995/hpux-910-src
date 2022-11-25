/* @(#) $Revision: 1.3.83.4 $ */

#ifndef _SYS_POLL_INCLUDED
#define _SYS_POLL_INCLUDED

#ifdef _KERNEL_BUILD
#include "../h/stdsyms.h"
#else  /* ! _KERNEL_BUILD */
#include <sys/stdsyms.h>
#endif /* _KERNEL_BUILD */

#ifdef _INCLUDE_AES_SOURCE

#define POLLIN		00001	/* Check for input */
#define POLLNORM	POLLIN	/* Ask Norm what he'd like */
#define POLLPRI		00002
#define POLLOUT		00004	/* Check for output */
#define POLLERR		00010	/* Check for error */
#define POLLHUP		00020	/* Check for hangup */
#define POLLNVAL	00040	/* Check for an invalid file descriptor */

#define POLLRDNORM	00100
#define POLLRDBAND	00200
#define POLLWRNORM	00400
#define POLLWRBAND	01000
#define POLLMSG		02000

#define INFTIM (-1)		/* Infinite timeout */

/* array of file descriptors to poll */
struct pollfd {
	int	fd;		/* file descriptor to poll */
	short	events;		/* events we are interested in */
	short	revents;	/* returned mask of events that occurred */
};

/*
 * Due to kernel implementation concerns in poll(), the above flags must not
 * conflict with FREAD (01) and FWRITE (02).  So, naturally, they do:
 * POLLPRI==FWRITE.  To cure this, we map POLLPRI to another value in poll().
 * User programs continue to use the same old value for POLLPRI.
 */

#ifdef _KERNEL
#undef POLLPRI
#define POLLPRI  004000000000	/* The kernel will use this internally. */
#define USER_POLLPRI 00002	/* The value that user programs use. */
#define POLLDONE 001000000000	/* Signal last poll event */
				/* POLLDONE must be the largest POLL* value */
#endif

#ifndef _KERNEL
#ifdef __cplusplus
extern "C" {
#endif

#ifdef _PROTOTYPES
	extern int poll(struct pollfd *, int, int);
#else /* not _PROTOTYPES */
	extern int poll();
#endif /* not _PROTOTYPES */

#ifdef __cplusplus
}
#endif
#endif /* _KERNEL */

#endif /* _INCLUDE_AES_SOURCE */

#endif /* _SYS_POLL_INCLUDED */
