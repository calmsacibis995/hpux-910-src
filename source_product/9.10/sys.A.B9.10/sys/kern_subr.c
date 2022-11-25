/* $Source: /source/hpux_source/kernel/sys.SWT68K_800/sys/RCS/kern_subr.c,v $
 * $Revision: 1.20.83.4 $	$Author: rpc $
 * $State: Exp $   	$Locker:  $
 * $Date: 93/10/18 13:44:34 $
 */

#if defined(MODULEID) && !defined(lint)
static char *rcsid="@(#) kern_subr.c $Revision: 1.20.83.4 $ $Date: 93/10/18 13:44:34 $";
#endif

/*
(c) Copyright 1983, 1984, 1985, 1986, 1987, 1988 Hewlett-Packard Company.
(c) Copyright 1979 The Regents of the University of Colorado, a body corporate
(c) Copyright 1979, 1980, 1983 The Regents of the University of California
(c) Copyright 1980, 1984, 1986 AT&T Technologies.  All Rights Reserved.
The contents of this software are proprietary and confidential to the Hewlett-
Packard Company, and are limited in distribution to those with a direct need
to know.  Individuals having access to this software are responsible for main-
taining the confidentiality of the content and for keeping the software secure
when not in use.  Transfer to any party is strictly forbidden other than as
expressly permitted in writing by Hewlett-Packard Company.  Unauthorized trans-
fer to or possession by any unauthorized party may be a criminal offense.

                    RESTRICTED RIGHTS LEGEND

          Use,  duplication,  or disclosure by the Government  is
          subject to restrictions as set forth in subdivision (b)
          (3)  (ii)  of the Rights in Technical Data and Computer
          Software clause at 52.227-7013.

                     HEWLETT-PACKARD COMPANY
                        3000 Hanover St.
                      Palo Alto, CA  94304
*/

#include "../h/param.h"
#include "../h/systm.h"
#ifdef hp9000s800
#include "../h/dir.h"
#endif
#include "../h/user.h"
#include "../h/uio.h"
#ifdef hp9000s800
#include "../machine/vmparam.h"
/* NOTE: uiomove() is called from rwip() with asynchronous kernel preemption
 *	 enabled if option KPREEMPT is used to compile the kernel.
 */
#endif
uiomove(cp, n, flg, uio)
	register caddr_t cp;
	register int n;
	int flg;
	register struct uio *uio;
{
	register struct iovec *iov;
	register u_int cnt;
	u_int recnt;
	caddr_t up;
	enum uio_rw rw;
	int error = 0;
#ifdef hp9000s800
	space_t sid;
#endif

	rw = (enum uio_rw)(flg & UIO_RW);

	while (n > 0 && uio->uio_resid) {
		iov = uio->uio_iov;
		cnt = iov->iov_len;
		if (cnt == 0) {
			uio->uio_iov++;
			uio->uio_iovcnt--;
			continue;
		}
		if (cnt > n)
			cnt = n;

		switch (uio->uio_seg) {

		case UIOSEG_USER:
#ifdef	FDDI_VM
#ifdef	hp9000s800
			/*
			 *  Copies and/or moves data between the current
			 *  user's buffer and a kernel buffer.
			 *
			 *  If the caller requests that the data be moved (ie,
			 *  a copy of the data is not retained in the source
			 *  buffer), and if the data is being moved out of
			 *  the kernel buffer and into the user buffer, and
			 *  if both the kernel buffer and the user buffer are
			 *  page-aligned, and if there is at least a page 
			 *  worth of data to be moved, then the data is moved
			 *  by swapping physical pages associated with the 
			 *  two virtual buffers.  Otherwise, the data is 
			 *  copied.
			 *
			 *  In particular, it should be noted that page 
			 *  swapping cannot be used to move data from the
			 *  user buffer into a kernel buffer because the
			 *  the kernel pages that are swapped with the user
			 *  buffer may contain data that the user is not
			 *  privileged to read. 
			 */
			up = iov->iov_base;
			if (rw == UIO_READ) {
				if ((flg & UIO_AVOID_COPY) && (cnt >= NBPG) &&
					    (((int)up&(NBPG-1)) == 0) &&
					    (((int)cp&(NBPG-1)) == 0))
					error = remapout(cp, up, cnt);
				else
					error = copyout(cp, up, cnt);
			}
			else
				error = copyin(up, cp, cnt);
			if (error)
				return (error);
			break;
#endif	/* hp9000s800 */
#endif	/* FDDI_VM */

		case UIOSEG_INSTR:
			if (rw == UIO_READ)
				error = copyout(cp, iov->iov_base, cnt);
			else
				error = copyin(iov->iov_base, cp, cnt);
			if (error)
				return (error);
			break;

		case UIOSEG_KERNEL:
			/* case UIOSEG_KERNEL:
			 * This is an intraspace move in quadrant one (ex.
			 * buffer to u_area move), sid = KERNELSPACE.
			 */
#ifdef	hp9000s800
			sid = ldsid(iov->iov_base);
			up  = iov->iov_base;
#ifdef	FDDI_VM
			/*
			 *  If the caller requests that the data be moved, 
			 *  and if both kernel buffers are page-aligned, and 
			 *  if there is at least a page worth of data to be 
			 *  moved, then the greatest integral number of pages
			 *  worth of data is moved by swapping physical 
			 *  pages associated with the two virtual buffers;
			 *  remaining data is copied by the next pass of the
			 *  outer while loop.  If any of the aforementioned 
			 *  conditions are not met, all the data is copied.
			 */
			if ((flg & UIO_AVOID_COPY) && 
				    ((recnt=cnt&~(NBPG-1)) > 0) &&
				    (((int)up&(NBPG-1)) == 0) &&
				    (((int)cp&(NBPG-1)) == 0)) {
				if (rw == UIO_READ)
					cnt = lkernelremap(
						    (space_t)KERNELSPACE, cp,
							sid, up, recnt);
				else
					cnt = lkernelremap(sid, up,
						    (space_t)KERNELSPACE, cp,
							recnt);
			}
			else 
#endif	/* FDDI_VM */
			if (rw == UIO_READ)
				lbcopy((space_t)KERNELSPACE, cp, sid, up, cnt);
			else
				lbcopy(sid, up, (space_t)KERNELSPACE, cp, cnt);
			break;
#endif	/* hp9000s800 */
#ifdef __hp9000s300
			if (rw == UIO_READ)
				bcopy((caddr_t)cp, iov->iov_base, cnt);
			else
				bcopy(iov->iov_base, (caddr_t)cp, cnt);
			break;
#endif /* __hp9000s300 */
#ifdef hp9000s800
		case UIOSEG_LBCOPY:
			/*
			 * case UIOSEG_LBCOPY:
			 * Calculate the space contents of the space id given
			 * the offset. If the offset is in the first quadrant
			 * then get the sid from sr4 of the save state.
			 *
			 */
			sid = ldusid(iov->iov_base);
			if (rw == UIO_READ)
				lbcopy((space_t)KERNELSPACE, (caddr_t)cp,
					sid, iov->iov_base, cnt);
			else
				lbcopy(sid, iov->iov_base,
					(space_t)KERNELSPACE, (caddr_t)cp, cnt);
			break;
#endif /* hp9000s800 */
#ifdef hp9000s800
		case UIOSEG_KFLUSH:
		case UIOSEG_LBFLUSH:
			/* case UIOSEG_KFLUSH:
			 * This is an intraspace move in quadrant one (ex.
			 * buffer to u_area move), sid = KERNELSPACE.
			 * Flush the data cache after the move.
			 *
			 * case UIOSEG_LBFLUSH:
			 * Calculate the space contents of the space id given
			 * the offset. If the offset is in the first quadrant
			 * then get the sid from sr4 of the save state.
			 * Flush the data cache after the move.
			 *
			 */
			if (uio->uio_seg == UIOSEG_LBFLUSH)
				sid = ldusid(iov->iov_base);
			else
				sid = ldsid(iov->iov_base);
			if (rw == UIO_READ) {
				lbcopy((space_t)KERNELSPACE, (caddr_t)cp,
						sid, iov->iov_base, cnt);
				fdcache(sid, iov->iov_base, cnt);
			}
			else {
				lbcopy(sid, iov->iov_base,
				       (space_t)KERNELSPACE, (caddr_t)cp, cnt);
				fdcache((space_t)KERNELSPACE, (caddr_t)cp, cnt);
			}
			break;
#endif /* hp9000s800 */

                case UIOSEG_PAGEIN:
                        /*
                         * This code "knows" the internals of what
                         * hdl_user_protect is going to do on any given
                         * architecture.  This is gross, but alas what else
                         * could we do!  This code only works when the
                         * calling process is the process that own the pages
                         * in question.
                         */
#ifdef __hp9000s300
                        if (rw == UIO_READ)
                                bcopy((caddr_t)cp, iov->iov_base, cnt);
                        else
                                bcopy(iov->iov_base, (caddr_t)cp, cnt);
                        break;
#endif
#ifdef __hp9000s800
                        sid = ldusid(iov->iov_base);
                        if (rw == UIO_READ) {
                                error = privlbcopy(KERNELSPACE, cp,
                                                   sid, iov->iov_base, cnt);
				fdcache(sid, iov->iov_base, cnt);
                        } else {
                                error = privlbcopy(sid, iov->iov_base,
                                                   KERNELSPACE, cp, cnt);
				fdcache((space_t)KERNELSPACE, (caddr_t)cp, cnt);
			}
                        if (error)
                                return (error);
                        break;
#endif

		default:
			panic("uiomove: uio->uio_seg is invalid");
		}
		iov->iov_base += cnt;
		iov->iov_len -= cnt;
		uio->uio_resid -= cnt;
		uio->uio_offset += cnt;
		cp += cnt;
		n -= cnt;
	}
	return (error);
}

#ifdef  hp9000s800
/* 
 * BEGIN_DESC
 *
 * struct ioops uio_noops
 *
 * Description:
 *	References a nonoperational function.  This struct is used when 
 *	uiowrite_avoid_copy() and the functions it calls are unable to create
 *	copy-on-write references.
 *
 * END_DESC
 */
uionoop()
{
	return(0);
}

struct ioops uio_noops = {
	/*    freebuf */ uionoop,
	/*     copyto */ uionoop,
	/*   copyfrom */ uionoop,
	/*   physaddr */ uionoop,
	/* flushcache */ uionoop,
	/* purgecache */ uionoop,
};

/* 
 * BEGIN_DESC
 *
 * uiowrite_avoid_copy()
 *
 * Input Parameters:    
 *	np:		References the maximum number of bytes to retrieve 
 *			from the user.
 *
 *	flg:		No values defined; caller should set flg to 0.
 *
 *	uio:		References and describes one or more pieces of memory.
 *
 *      argsbuf_addr_ptr: Address of memory through which uiowrite_avoid_copy() 
 *			and its caller specify where the description of the 
 *			copy-on-write reference is located.  If the parm 
 *			argsbuf_size is smaller than the amount of memory 
 *			needed for the copy-on-write reference description, 
 *			uiowrite_avoid_copy() will allocate memory to hold the 
 *			description and will overwrite the address referenced 
 *			by parm	argsbuf_addr_ptr with the address of allocated 
 *			memory.  Otherwise, uiowrite_avoid_copy() will write 
 *			the description at the address referenced by parm 
 *			argsbuf_addr_ptr; in which case, the address referenced
 *			by parm argsbuf_addr_ptr must be word-aligned.
 *
 *			If the caller wishes for uiowrite_avoid_copy() to 
 *			allocate memory	to hold the description of the 
 *			copy-on-write reference, it should set parm 
 *			argsbuf_size to zero.
 *
 *			If argsbuf_addr_ptr references a null, non-word
 *			aligned, or otherwise invalid address, and if parm 
 *			argsbuf_size is *not* smaller than the amount of 
 *			memory needed for the copy-on-write reference
 *			description, uiowrite_avoid_copy() will panic.
 *
 *			If uiowrite_avoid_copy() returns a nonzero value, the 
 *			value of the memory referenced by parm argsbuf_addr_ptr
 *			is undefined.
 *
 *	argsbuf_size:	Number of bytes of memory referenced by the address
 *			referenced by parm args_addr_ptr (see description
 *			of argsbuf_addr_ptr above).
 *
 * Output Parameters:
 *	cpp:		Address of memory through which uiowrite_avoid_copy()
 *			returns the address of the user data it retrieved.
 *			If uiowrite_avoid_copy() returns a nonzero value, the
 *			value of the memory referenced by cpp is undefined.
 *
 *	np:		References the actual number of bytes retrieved from
 *			the user, if uiowrite_avoid_copy() returns zero.  If
 *			uiowrite_avoid_copy() returns a nonzero value, the
 *			value of memory referenced by np on input will not be
 *			modified.
 *
 *	uio:		References and describes the memory remaining to be
 *			retrieved.
 *
 *	ops_addr_ptr:	Address of memory through which uiowrite_avoid_copy() 
 *			returns a reference to the struct ioops operations that
 *			the caller of uiowrite_avoid_copy() should use to 
 *			free, flush, etc., the buffer returned by
 *			uiowrite_avoid_copy().  The value referenced by
 *			ops_addr_ptr on input is ignored.  If 
 *			uiowrite_avoid_copy() returns a nonzero value, the 
 *			value of the memory referenced by ops_addr_ptr is 
 *			undefined.
 *
 *      argsbuf_addr_ptr: (see above)
 *
 * Return Value:	Zero if at least one byte was successfully retrieved
 *			from the user without copying; EAGAIN if the caller
 *			should attempt to retrieve the user data via a copy
 *			(ie, by calling uiomove); other nonzero values indicate
 *			a severe error has occurred (no need to try to copy).
 *
 * Globals Referenced:
 *	uio_noops
 *
 * Description:
 *	Retrieve user data without copying it by creating a copy-on-write
 *	reference to the user data.
 *
 *	Via the ops_addr_ptr parameter, uiowrite_avoid_copy() returns a set 
 *	of operations to act upon the retrieved data; the defined operations 
 *	are ioo_freebuf (frees the retrieved data), ioo_physaddr (returns 
 *	physical address of retrieved data), ioo_flushcache (flushes the
 *	retrieved data from the processors' caches) and ioo_purgecache (purges
 *	the retrieved data from the processors' caches).  The ops_addr_ptr
 *	parameter also references operations ioo_copyto (copy data into the
 *	retrieved data buffer) and ioo_copyfrom (copy data from the retrieved
 *	data buffer); these operations are currently undefined for copy-on-
 *	write references.
 *
 *	Via the argsbuf_addr_ptr parameter, uiowrite_avoid_copy() also returns 
 *	an opaque description of the retrieved data, which its caller must 
 *	retain.  This description must be passed to the operations referenced
 *	via the ops_addr_ptr.  The caller may supply memory to hold this 
 *	description or it may rely on uiowrite_avoid_copy() to allocate memory 
 *	for this description.  If uiowrite_avoid_copy() allocates this memory, 
 *	(*(*ops_addr_ptr)->ioo_freebuf)() will free it.
 *
 * Algorithm:
 *	Determine if the memory referenced by parm uio meets the necessary
 *	conditions for copy-on-write.  Copy-on-write requires that the uio
 *	struct references at least a page of user data that begins on a
 *	page boundary.  If not, return either zero if no data existed or
 *	EAGAIN if data existed but did not meet the necessary conditions for
 *	copy-on-write.  If the memory referenced by parm uio does meet the
 *	necessary conditions, call cowin() to create copy-on-write references
 *	to the memory.
 *
 * In/Out conditions:
 *      Uiowrite_copy_avoid() must be called on the kernel stack of the user 
 *	from whom the data is retrieved.
 *
 *	Parameters cpp, np, ops_addr_ptr and argsbuf_addr_ptr must be
 *	word-aligned.
 *
 *	The address referenced by argsbuf_addr_ptr is undefined if 
 *	uiowrite_copy_avoid() returns a non-zero value; accordingly, the caller 
 *	should never assume that the address referenced by argsbuf_addr_ptr
 *	will not be modified (no matter how big argsbuf_size is).
 *
 *	The memory referenced by *argsbuf_addr_ptr must not be modified, and
 *	should not be accessed, by the caller.
 *
 *	(See the description of argsbuf_addr_ptr above for more restrictions).
 *
 * END_DESC
 */
uiowrite_avoid_copy(cpp, np, flg, uio, ops_addr_ptr, 
		argsbuf_addr_ptr, argsbuf_size)
	caddr_t *cpp;
	int *np;
	int flg;
	struct uio *uio;
	struct ioops **ops_addr_ptr;
	caddr_t *argsbuf_addr_ptr;
	int argsbuf_size;
{
	int n, cnt, error;
	struct iovec *iov;
	caddr_t up;

	if ((n = *np) <= 0)
		goto zero;
	if (uio->uio_resid == 0)
		goto zero;
	if (uio->uio_seg != UIOSEG_USER)
		goto move;

	iov = uio->uio_iov;
	while ((cnt = iov->iov_len) == 0) {
		iov = ++uio->uio_iov;
		uio->uio_iovcnt--;
	}
	if (cnt > n)
		cnt = n;

	/* Can only map integral number of page.  If at least one page worth 
	 * of data to be moved, will create copy-on-write references to the 
	 * greatest number of pages less than the amount of data available 
	 * to be moved.
	 */
	if ((cnt &= ~(NBPG-1)) == 0)
		goto move;
	up = iov->iov_base;
	if ((int)up & (NBPG-1))
		goto move;

	error = cowin(up, cpp, cnt, 
			ops_addr_ptr, argsbuf_addr_ptr, argsbuf_size);
	if (error)
		goto exit;

	iov->iov_base += cnt;
	iov->iov_len -= cnt;
	uio->uio_resid -= cnt;
	uio->uio_offset += cnt;
	*np = cnt;
	goto exit;

zero:	*np   = 0;
	*cpp  = 0;
	error = 0;
	goto null;

move:   error = EAGAIN;
null:	*ops_addr_ptr = &uio_noops;
exit:   return (error);
}
#endif  /* hp9000s800 */

/*
 * Give next character to user as result of read.
 */
ureadc(c, uio)
	register int c;
	register struct uio *uio;
{
	register struct iovec *iov;

again:
	if (uio->uio_iovcnt == 0)
		panic("ureadc");
	iov = uio->uio_iov;
	if (iov->iov_len <= 0 || uio->uio_resid <= 0) {
		uio->uio_iovcnt--;
		uio->uio_iov++;
		goto again;
	}
	switch (uio->uio_seg) {

	case UIOSEG_USER:
		if (subyte(iov->iov_base, c) < 0)
			return (EFAULT);
		break;

	case UIOSEG_KERNEL:
		*iov->iov_base = c;
		break;

	case UIOSEG_INSTR:
		if (suibyte(iov->iov_base, c) < 0)
			return (EFAULT);
		break;
	}
	iov->iov_base++;
	iov->iov_len--;
	uio->uio_resid--;
	uio->uio_offset++;
	return (0);
}

#ifdef __hp9000s300
#ifdef NEVER_CALLED
/*
 * Get next character written in by user from uio.
 */
uwritec(uio)
	struct uio *uio;
{
	register struct iovec *iov;
	register int c;

again:
	if (uio->uio_iovcnt <= 0 || uio->uio_resid <= 0)
		panic("uwritec");
	iov = uio->uio_iov;
	if (iov->iov_len == 0) {
		uio->uio_iovcnt--;
		uio->uio_iov++;
		goto again;
	}
	switch (uio->uio_seg) {

	case UIOSEG_USER:
		c = fubyte(iov->iov_base);
		break;

	case UIOSEG_KERNEL:
		c = *iov->iov_base & 0377;
		break;

	case UIOSEG_INSTR:
		c = fuibyte(iov->iov_base);
		break;
	}
	if (c < 0)
		return (-1);
	iov->iov_base++;
	iov->iov_len--;
	uio->uio_resid--;
	uio->uio_offset++;
	return (c & 0377);
}
#endif /* NEVER_CALLED */
#endif /* __hp9000s300 */

#ifdef hp9000s800
/*
 * strcpy() - Copy String S2 to S1
 * Series 300 uses libc versions since they are in assembly language.
 */
char *
strcpy(s1, s2)
register char *s1, *s2;
{
	register char *os1;

	os1 = s1;
	while (*s1++ = *s2++)
		;
	return os1;
}

/*
 * strncpy() - Copy String S2 to S1, up to N bytes
 */
char *
strncpy(s1,s2,n)
register char *s1,*s2;
int n;
{
	register char *os1;

	os1 = s1;
	while (*os1++ = *s2++)
	    if (--n <= 0) break;
	return os1;
}

/*
 * strcmp() - Compare String S1 and S2
 */
int
strcmp(s1, s2)
    char *s1, *s2;
{
    if (s1 == s2)
	return(0);
    if (s1 == NULL) {
	if (s2 == NULL)
	    return(0);
	else
	    return(-*s2);
    }
    else if (s2 == NULL)
	return(*s1);

    while (*s1 == *s2++)
	if (*s1++ == '\0')
	    return(0);
    return(*s1 - *--s2);
}

strncmp(s1, s2, n)
	register char *s1, *s2;
	register int n;
{
	while (--n >= 0 && *s1 == *s2++)
		if (*s1++ == '\0')
			return (0);
	return (n<0 ? 0 : *s1 - *--s2);
}
#endif /* hp9000s800 */

/*
 * dux_notconfigured() and returnzero() moved here from init_sent.c
 */

dux_notconfigured()
{
	u.u_error = ENODEV;
}

returnzero()
{
	u.u_rval1 = 0;
}
