/* $Source: /source/hpux_source/kernel/sys.SWT68K_800/dux/RCS/dux_sdo.c,v $
 * $Revision: 1.6.83.4 $	$Author: marshall $
 * $State: Exp $   	$Locker:  $
 * $Date: 93/12/15 12:02:03 $
 */

/* HPUX_ID: @(#)dux_sdo.c	55.1		88/12/23 */

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

/*
 *This file contains the code dealing with SDO's,  The main routine
 *is the lookup routine sdo_lookup.  This routine is called for ALL
 *lookups (not just SDOs), but is able to handle the SDO special cases.
 */

#include "../h/param.h"
#ifdef 	POSIX
#include "../h/time.h"
#include "../h/resource.h"
#include "../h/proc.h"
#endif	POSIX
#include "../h/user.h"
#include "../h/vnode.h"
#include "../ufs/fsdir.h"
#include "../h/buf.h"
#include "../ufs/inode.h"
#include "../ufs/fs.h"
#include "../ufs/slot.h"
#ifdef __hp9000s300
#include "../s200/vmparam.h"
#endif
#include "../dux/duxparam.h"
#include "../dux/cct.h"
#include "../h/errno.h"
#include "../dux/dux_hooks.h"
#include "../h/unistd.h"		/* for CPU_PA_RISC1_1 define */

/*
 *The code controlled by STOPHERE was in the original DUX code.  It say's
 *that a .. from an SDO that does not match the process's context should
 *only move up one entry, not all the way to the top.  I (Joel) think
 *that is an assymetry and don't like it.  However Sui-Ping feels that
 *it is needed.  It can be turned on and off with the STOPHERE flag.
 */
#define STOPHERE
/*#define STOPHERE    I agree with Joel.  With STOPHERE turned on, it is
  too confusing to user. -kao*/

extern char my_sitename[];

extern int cpu_version;			/* for build_context */

#ifdef	__hp9000s300
extern int processor;
extern int dragon_present;
extern int float_present;
extern int mc68881;
#endif	/* hp9000s300 */

struct dux_context dux_context;		/* this system's context */

/*
 *Lookup an entry in a directory, taking into account SDOs
 *
 *The algorithm is as follows:
 *
 *1)  Lookup the file with dirlook(), using the full component passed.
 *    This permits finding filenames that happen to end with the SDOCHAR,
 *    but have nothing to do with SDOs.
 *
 *2a) If the pathname was found in step 1:
 *
 *	3a)  If the inode found was not an SDO, return it.
 *	4a)  If the component ended with the SDOCHAR, return it.
 *		(This is not strictly right.  It prohibits files ending
 *		with the SDOCHAR from being made site dependent.  Strictly
 *		speaking, we should require two SDOCHARS to override this
 *		file.  However, the code to handle that is tricky, and
 *		probably not worthwhile.  Having this step supports
 *		the case of a file longer than 14 characters that was
 *		truncated, so that the SDOCHAR was not seen by dirlook().
 *	5a)  Otherwise, we need to find the context entry.  Call
 *		context_lookup() to look it up.
 *
 *2b) If the pathname was not found in step 1:
 *
 *	3b)  If the component did not end with the SDOCHAR, it is an error.
 *	4b)  Strip off the SDOCHAR and look up the resulting pathname
 *		using dirlook().
 *	5b)  If the pathname was not found, it is an error.
 *	6b)  If the pathname was found and is a hidden directory, return it.
 *	7b)  Otherwise, the pathname was found, but is not a hidden directory.
 *		This is an error.  The SDOCHAR is not considered special
 *		except when modifying an SDO.  Therefore, a given file
 *		name with the SDOCHAR does not match the same file name
 *		without it.
 *
 *Potential enhancement:
 *	This SDO implementation will negate much of the effect of the
 *	directory cache for local operation.  This is because several
 *	lookups will be necessary to get one SDO entry, and many of them
 *	will fail (the incorrect contexts).  Hopefully SDOs will be
 *	short and will fit into a single disc block to avoid multiple
 *	reads.  However, we could still improve efficiency by looking up
 *	the name in the cache and automatically traversing all the SDO
 *	entries in the single lookup.  We need to be careful though, the
 *	lookup should only succeed for this site, since other sites will
 *	not want the same entry!
 */

sdo_lookup(dp, nm, ipp, mdvp, ldpp, realnamep)
register struct inode *dp;	/* The directory we are searching in */
register char *nm;		/* The component */
struct inode **ipp;	/* Where to put the newfound inode */
struct vnode *mdvp;	/* If path is .. and across a mount point, mdvp will
			   be different from dp.  Else, their are the same*/
struct inode **ldpp;
char	*realnamep;
{
	register int error;
	register int nmlen;
	register char save1;
	register char *lastchar;
	register int lastchar_is_sdochar;
	register int lfn;
#ifdef	POSIX
	int	no_trunc = 0;	/*can only be set in sfn and S2POSIX_NO_TRUNC
				  is set*/
#endif	POSIX

	lfn = IS_LFN_FS(dp);

	/*check to see if last character is "+" */
	nmlen = strlen(nm);
	lastchar = nm + nmlen - 1;
	if (*lastchar == SDOCHAR) {
		lastchar_is_sdochar = 1;
	}
	else {
		lastchar_is_sdochar = 0;
	}

	if (!lfn) {
#ifdef	S2POSIX_NO_TRUNC
		no_trunc = (u.u_procp->p_flag2) & S2POSIX_NO_TRUNC;
		if (no_trunc) {
		   /*ENAMETOOLONG should be return if filename too long*/
		   if (nmlen>DIRSIZ_CONSTANT) {
			/*if no_trunc is set and filename is 15 characters
			  and the last char is "+", we should go ahead
			  and see if a sdo hidden directory exists. */
		      if((!lastchar_is_sdochar) || (nmlen>(DIRSIZ_CONSTANT+1))){
			  return(ENAMETOOLONG);
		      }
		   }
		}
#endif	S2POSIX_NO_TRUNC
		/* If in short file name fs, truncate to 14 characters*/
		save1 = nm[DIRSIZ_CONSTANT];
		nm[DIRSIZ_CONSTANT] = '\000';
	}
	if (lfn || (nmlen <= DIRSIZ_CONSTANT) || (!lastchar_is_sdochar)) {
		error = dirlook(dp, nm, ipp, mdvp);
	}
	else {
		error = ENOENT;		/*we know we are not going to find it*/
	}
	/* Did we find it? */
	if (error == 0)
	{	/* Yes we did find it*/
		if (ISSDO(*ipp))
		{	/*it is an SDO*/
#ifdef	STOPHERE
			error = context_lookup(dp, nm, ipp, ldpp,realnamep);
#else not STOPHERE
			error = context_lookup(nm, ipp, ldpp,realnamep);
#endif
		}
		else
		{	/*a regular file (HO HUM), return it) */
			if (realnamep) strcpy(realnamep, nm);
			error = 0;
		}
	}
	else if (error == ENOENT)
	{	/* No we did not find it */
		/* Check for the SDO char.  Note in the degenerate case of
		 * an empty pathname, lastchar points before the nm.  Since
		 * this character could conceivably contain the SDO char,
		 * make a special check for it.
		 */
		if (lastchar_is_sdochar)
		{	/* an escaped SDO--strip off the SDOCHAR and retry*/
			*lastchar = '\0';
			error = dirlook(dp, nm, ipp, mdvp);

			/*did the search succeed? */
			if (!error) {
				if (realnamep) strcpy(realnamep, nm);
				if (ISSDO(*ipp)) {
					error = 0;	/*YES*/
				}
				else if (!lfn && (nmlen > DIRSIZ_CONSTANT)) {
/*In the case of short filename fs, if the filename passed to this routine
  is longer the DIRSIZ_CONSTANT and the last character is a sdo character
  and there is a file which name match the first DIRSIZ_CONSTANT characters
  of the filename passed, we truncate the filename to DIRSIZ_CONSTANT.  The
  sdo character does not have any special meanning. */
					error = 0;
				}
				else
				{
					/*it succeeded, but not with an sdo.
					 *release the inode
					 */
					iunlock(*ipp);
					VN_RELE(ITOV(*ipp));
					error =  ENOENT;
				}
			}
#ifdef	S2POSIX_NO_TRUNC
			if (no_trunc && (nmlen > DIRSIZ_CONSTANT)) {
				/* if nmlen is 15 and last character is "+"
				   and no_trunc, the sdo hidden directory
				   does not exist, we should return
				   ENAMETOOLONG. */
				error = ENAMETOOLONG;
			}
#endif	S2POSIX_NO_TRUNC
			*lastchar = SDOCHAR;
		}
		/*else*/
		/* we did not find it and it wasn't escaped */
	}
	if (!lfn) nm[DIRSIZ_CONSTANT] = save1;
	return (error);
}

/*
 *We have found a (non escaped) hidden directory.  Traverse down the
 *entries until we either find a normal file or get an error.
 *There are two special cases:
 *  If the original name looked up is '.', we just return the inode directly.
 *  If the original name looked up is '..', move UP the tree instead of
 *  down, and don't use the context.
 */
#ifdef	STOPHERE
context_lookup(dp, nm, ipp, ldpp, realnamep)
struct inode *dp;		/*the original directory inode*/
#else	STOPHERE
context_lookup(nm, ipp, ldpp, realnamep)
#endif	STOPHERE
register char *nm;		/*the original SDO*/
register struct inode **ipp;	/*used for both input and output*/
struct inode **ldpp;
char *realnamep;
{
	register struct inode *oldip;
	register char **cntxpp;
	register dotdot = 0;
	register error;

	if (nm[0] == '.' && nm[1] == '\0')
		return (0);
	if (nm[0] == '.' && nm[1] == '.' && nm[2] == '\0')
		dotdot = 1;
	if(realnamep) strcpy(realnamep, nm);
	error = 0;	/*prime the pump*/
	while (!error && ISSDO(*ipp))
	{
		oldip = *ipp;
		iunlock(oldip);
		if (dotdot)
		{
#ifdef	STOPHERE
			char cname[MAXNAMLEN+1];


			*cname = '\0';
			getcname(cname, dp->i_number, oldip);
			if (*cname == '\0' || stophere_sdo(cname))
			{
				ilock(oldip);	/*relock it*/
				return(0);
			}
			else
				dp = oldip;
#endif	STOPHERE
			error = dirlook(oldip, nm, ipp, ITOV(oldip));
		}
		else
		{
			for (error = ENOENT, cntxpp = u.u_cntxp;
			     ((error == ENOENT) && (**cntxpp != '\0'));
			     cntxpp++ ) {
				if(realnamep) strcpy(realnamep, *cntxpp);
				error = dirlook(oldip, *cntxpp, ipp, ITOV(oldip));
			}
		}
		/* release the old inode */
		if (error || ISSDO(*ipp) || !ldpp) {
			VN_RELE (ITOV(oldip));
		}
		else {
			*ldpp = oldip;
			break;
		}
	}
	return (error);
}

/*
 *This function is calls dircheckforname.  If no SDOs are involved, it
 *just returns, so calling this function is equivalent to calling
 *dircheckforname.  However, it has the special cases needed for handling
 *SDOs.
 *
 *Unfortunately, it is necessary to duplicate much of the logic of sdo_lookup.
 *This is because the outer level lookuppn knows nothing about SDOs, and
 *it does not know about the relationship between the original parent
 *directory, and the final SDO, which, although they may be separated
 *in the directory tree, were obtained with just one call to VOP_LOOKUP.
 *
 *The following algorithm is similar to that of sdo_lookup():
 *
 *1)  Lookup the entry with dircheckforname(), using the full component passed.
 *
 *2a) If the pathname was found in step 1:
 *
 *	3a)  If the inode found was not an SDO, return it.
 *	4a)  If the component ended with the SDOCHAR, return it.
 *	5a)  Otherwise, we need to find the context entry.  Call
 *		dircheck_context() to look it up.
 *
 *2b) If the pathname was not found in step 1:
 *
 *	3b)  If the component did not end with the SDOCHAR, return.
 *	4b)  Strip off the SDOCHAR and call dircheckforname() using it.
 *		However, save the slot information first, in case of error.
 *	5b)  If the pathname was not found, restore the SDOCHAR, and the
 *		slot information.
 *	6b)  If the pathname was found and is a hidden directory, return it.
 *	7b)  Otherwise, the pathname was found, but is not a hidden directory.
 *		This is not the entry we were looking for so restore the
 *		SDOCHAR and the saved slot info.
 *
 *NOTE:  The tdpp, namepp, and namlenp are pointers here rather than
 *direct values as in dircheckforname because they can be changed for
 *SDOs.
 */
dircheckwithsdo(tdpp, namepp, namlenp, slotp, tipp, realnamep)
	register struct inode **tdpp;	/* inode of directory being checked */
	char **namepp;			/* name we're checking for */
	register int *namlenp;		/* length of name */
	register struct slot *slotp;	/* slot structure */
	struct inode **tipp;		/* return inode if we find one */
	char *realnamep;		/* when we find one, *realnamep has
					   the real filename. (for SDO).
					   realnamep should point to a buffer
					   size of MAXNAMLEN+1 */
{
	int error;
	struct slot savedslot;
	enum slotstatus savedstatus;
	register int lastchar_is_sdochar;
	register char save;
	register char *lastcharp;
	register int namelen;
	register int lfn;
	int	longnamewithsdoc = 0;

	lfn = IS_LFN_FS(*tdpp);
	namelen = *namlenp;
	savedstatus = slotp->status;	/* save it for subsequent calls */
	lastcharp = *namepp + namelen - 1;
	if (*lastcharp == SDOCHAR) {
		lastchar_is_sdochar = 1;
	}
	else {
		lastchar_is_sdochar = 0;
	}
	if (!lfn) {
		save = (*namepp)[DIRSIZ_CONSTANT];
		(*namepp)[DIRSIZ_CONSTANT] = '\000';
	}
	if (lfn || (namelen <= DIRSIZ_CONSTANT) || (!lastchar_is_sdochar)) {
		if (!lfn && (namelen > DIRSIZ_CONSTANT)) {
			namelen = DIRSIZ_CONSTANT;
		}
		error = dircheckforname(*tdpp, *namepp, namelen, slotp, tipp);
		if (error)  {
			goto out;
		}
	}
	else {
		/* In the case of short filename file systems and filename
		   is longer than DIRSIZ_CONSTANT.*/
		namelen = DIRSIZ_CONSTANT;
		longnamewithsdoc = 1;
		error = 0;
		*tipp = 0;
	}
	if (*tipp)	/*found an inode */
	{
		if (ISSDO(*tipp))	/*it's an SDO*/
		{
			error = dircheck_context(tdpp, namepp, namlenp,
				slotp, tipp, savedstatus, realnamep);
		}
		else { 	/*regular file, return it*/
			strcpy(realnamep, *namepp);
		}
	}
	else	/*didn't find it*/
	{
		if (lastchar_is_sdochar)
		{
			/*an escaped SDO.  Save the slot, strip the SDOCHAR,
			 *and retry.
			 */
			*lastcharp = '\000';
			savedslot = *slotp;
			slotp->status = savedstatus;
			slotp->bp = NULL;
			/*if name length (include sdochar) is less or equal than
			  DIRSIZ_CONSTANT, sdochar is zeroed out and namelen should
			  reflect the fact*/
			if (lfn || (*namlenp <= DIRSIZ_CONSTANT)) namelen -= 1;
			error = dircheckforname(*tdpp, *namepp, namelen,
				slotp, tipp);
			strcpy(realnamep, *namepp);
			if (!error && (*tipp) && !ISSDO(*tipp)
			    && longnamewithsdoc) {
				*lastcharp = SDOCHAR;
				goto out;
			}
			else if (error || !(*tipp) || !(ISSDO(*tipp)))
			{
				/*various error conditions.  restore
				 *the original info and return*/
/*If longnamewithsdoc is not set, slot information is obtained and buffer
  was allocated by dircheckforname(), we need to release it*/
				if (!longnamewithsdoc) {
					if (slotp->bp)
					{
						brelse (slotp->bp);
						slotp->bp = NULL;
					}
					*slotp = savedslot;
				}
/* since slot->bp was cleared by callers to dircheckwithsdo(), no need to
   relase bp.
				else {
					if (savedslot.bp)
					{
						brelse (savedslot.bp);
						savedslot.bp = NULL;
					}
				}
*/
				*lastcharp = SDOCHAR;
				if (*tipp) {
					if (*tdpp == *tipp) {
/*If the file name is ".+" which is now "." due to temporarily removed "+",
  we don't use iput, (iput() unlock the inode which is not appropriate here.)
  We simply decrement the reference count. */
						VN_RELE(ITOV(*tipp));
					}
					else {
						iput (*tipp);
					}
					/* Since this is not what we intend to
					   look for, clear tipp so the caller
					   won't think we find it.  -kao*/
					*tipp = 0;
				}
				error = 0;	/*we do not return error
						 *because there was no
						 *error on the original
						 *lookup
						 */
			}
			else	/* found the escaped SDO */
			{
				if (savedslot.bp)
				{
					brelse (savedslot.bp);
					savedslot.bp = NULL;
				}
				/* return as we are, leaving the modified
				 * pathname
				 */
				return(0);
			}
		}
	}
out:
	if (!lfn) (*namepp)[DIRSIZ_CONSTANT] = save;
	return(error);
}

/*
 *Find the appropriate directory and directory entry given an SDO.  This
 *function is called from dircheckwithsdo when an SDO has been found.
 *Replace the original directory with the new SDO as the directory, and
 *find the appropriate slot.  If one of the contexts matches, check to
 *see if we need to loop (because of another SDO).  If not, return the
 *inode we found.  If we don't find a matching context, change the name
 *to refer to the site name.  This permits creates of SDOs to work, even
 *if there is no SDO corresponding to the machine.
 */
dircheck_context(tdpp, namepp, namlenp, slotp, tipp, slotstatus, realnamep)
	register struct inode **tdpp;	/* inode of directory being checked */
	char **namepp;			/* name we're checking for */
	int *namlenp;			/* length of name */
	register struct slot *slotp;	/* slot structure */
	struct inode **tipp;		/* return inode if we find one */
	enum slotstatus slotstatus;	/* initial value for slotp->status */
	char *realnamep;		/* when we found one, *realnamp should
					   have the real file name (for SDO)*/
{
	register int error;
	register char **cntxpp;
	struct inode *otdpp = *tdpp;	/*save original vnode */
	char *cp;

	if (**namepp == '.') {
/*I am making an assumption here: The only time ".." can be passed to
  this routine is when super-user is doing link xxx .. and ".." exists.
  There is no need to find out the real "..", we simply return 0 here.
  If this assumption is not true in the future, we need to change this
  routine to find out the real "..".  Take a look at context_lookup() for
  reference. */
		if ((*namlenp == 1) ||
                    ((*namlenp == 2) && ((*namepp)[1] == '.'))) {
			strcpy(realnamep, *namepp);
			return (0);
		}
	}

	error = 0;	/*prime the pump*/
	while (!error && (*tipp != NULL) && ISSDO(*tipp))
	{
		/*replace the old directory with the new.  If this is not
		 *the original starting directory, release it too.  (If this
		 *is the original starting directory, it will be released
		 *by the system call.
		 */
		iunlock (*tdpp);
		if (*tdpp != otdpp)
			VN_RELE(ITOV(*tdpp));
		*tdpp = *tipp;
		*tipp = NULL;

		for (cntxpp = u.u_cntxp;
		     (!error) && (*tipp == NULL) && (**cntxpp != '\0');
		     cntxpp++ ) {
			strcpy(realnamep,*cntxpp);

			/* Reset the  slot structure */
			slotp->status = slotstatus;
			if (slotp->bp) {
				brelse(slotp->bp);
				slotp->bp = 0;
			}
			/* Look it up */
			error = dircheckforname(*tdpp, realnamep,
					strlen(realnamep), slotp, tipp);
		}
		/*If we never found it, reset name to machine name and relook*/
		if (!error && *tipp == NULL)
		{
			*namepp = *(u.u_cntxp);	/* auto create default */
			*namlenp = strlen(*namepp);
			for (cp = *namepp; *realnamep++ = *cp++;);
			/* Reset the  slot structure */
			slotp->status = slotstatus;
			if (slotp->bp)
			{
				brelse(slotp->bp);
				slotp->bp = 0;
			}
			/* Look it up */
			error = dircheckforname(*tdpp, *namepp, *namlenp,
				slotp, tipp);
		}
	}
	return (error);
}


/*
 * getcontext(buffer, sizeofbuf)
 *
 * system call to get the per-process context
 */
getcontext(uap)
	struct a {
		char *buf;
		int len;
		int flag;
	} *uap;
{
	register char **cntxpp;
	register char *s, *d;
	register len;
	char tmp[CNTX_BUF_SIZE];

	d = tmp;
	for (cntxpp = u.u_cntxp; **cntxpp != '\0'; cntxpp++) {
		s = *cntxpp;		/* 's' points to context element */
		while(*(d++) = *(s++))	/* copy context element to tmp */
			;
		--d;			/* back up to re-do trailing null */
		*(d++) = ' ';		/* separate elements with a blank */
	}
	/* we are assuming at least one context element */
	*(d-1) = '\0';		/* overwrite final blank with terminator */
	len = d - tmp;
	u.u_r.r_val1 = len;
	if (len > uap->len)
		len = uap->len;

	u.u_error = copyout((caddr_t) tmp,
			    (caddr_t) uap->buf,
			    len);
}

/*
 * setcontext(newcontext)
 *
 * NOTE: This system call is UNSUPPORTED.  As such, we restrict its use
 *	 to super-user only and do not document it or provide a system
 *	 call stub in libc for it.
 */

/*ARGSUSED*/
setcontext(uap)
	struct a {
		char *buf;
	} *uap;
{
    register char	*cp1;
    register char	*cp2;
    register int	num_cntx;
    int			len;
    struct dux_context	*new_contextp;
    struct dux_context	*old_contextp;
    auto char		tmp_buf[CNTX_BUF_SIZE];

    if (!suser())	/* Super user only */
	return;

    /*
     * If the context buffer address is NULL, change the context
     * back to the system default.
     */
    old_contextp = (struct dux_context *)u.u_cntxp;
    if (uap->buf == (char *)NULL) {
	/*
	 * If the current context is not the default context, we
	 * release the context buffer before switch to default.
	 */
	if (old_contextp != &dux_context) {
	    kmem_free(old_contextp, sizeof(struct dux_context));
	    u.u_cntxp = (char **)&dux_context;
	}
	return;
    }

    /*
     * Read in the context specified by users.	copyinstr() does
     * copy the trailing '\0'.
     */
    u.u_error = copyinstr((caddr_t)uap->buf, tmp_buf, CNTX_BUF_SIZE, &len);
    if (u.u_error != 0) {
	if (u.u_error == ENOENT)
	    u.u_error = EINVAL; /* map error to something meaningful */
	return;
    }

    /*
     * If the current context is the default context, which is shared
     * among all processes except those use setcontext, a new context
     * buffer is allocated.
     */
    if (old_contextp == &dux_context) {
	new_contextp =
	    (struct dux_context *)kmem_alloc(sizeof (struct dux_context));

	if (new_contextp == (struct dux_context *)0) {
	    u.u_error = ENOMEM;
	    return;
	}
    }

    /*
     * Initialize the context to all zeros, this makes debugging nice
     * and also simplifies some of our string copies, below.
     */
    bzero(new_contextp, sizeof (struct dux_context));

    /*
     * Set up the new context.
     */
    cp1 = tmp_buf;
    cp2 = new_contextp->buf;

    num_cntx = 0;
    for (;;) {
	int elem_len;

	/*
	 * First skip leading white space.
	 */
	while (*cp1 == ' ' || *cp1 == '\t')
	    cp1++;

	/*
	 * If we hit the end of the new context string, we are done.
	 */
	if (*cp1 == '\0') {
	    if (num_cntx != 0)
		cp2--;		/* back cp2 up to the '\0' */
	    break;		/* no more context, out of here */
	}

	/*
	 * Setup the pointer for this new context element.
	 */
	new_contextp->ptr[num_cntx++] = cp2;

	/*
	 * Make sure we do not have too many context elements.
	 */
	if (num_cntx >= NUM_CNTX_PTR)
	    goto invalid;

	/*
	 * Copy this context string
	 */
	elem_len = 0;
	while (*cp1 != '\0' && *cp1 != ' ' && *cp1 != '\t') {
	    if (*cp1 == '/')
		goto invalid; /* You cannot have a '/' in a context! */

	    *cp2++ = *cp1++;
	    elem_len++;
	}
	cp2++; /* advance cp2 past terminating '\0' */

	/*
	 * Ensure that this context element is not longer than 14 bytes
	 * (a restriction imposed for "short" filename filesystems).
	 */
	if (elem_len > 14) {
	    kmem_free(new_contextp, sizeof (struct dux_context));
	    u.u_error = ENAMETOOLONG;
	    return;
	}

	/*
	 * A context element cannot be "." or ".."
	 */
	if (*(cp2-2) == '.') {
	    if (elem_len == 1 || (elem_len == 2 && *(cp2-3) == '.'))
		goto invalid;
	}

	/*
	 * A context element cannot end in '+'
	 */
	if (*(cp2-2) == '+')
	    goto invalid;
    }

    /*
     * You are not allowed to have an empty context!
     */
    if (num_cntx == 0)
	goto invalid;

    /*
     * Ensure that *all* unused context element pointers point
     * to a NULL string.  This is mainly so that when these
     * pointers are sent to another node and then adjusted they
     * end up pointing to something reasonable -- aids in
     * debugging.  This is *not* a performance critical area,
     * so this extra stuff is worthwhile.
     */
    while (num_cntx < NUM_CNTX_PTR)
	new_contextp->ptr[num_cntx++] = cp2;

    u.u_cntxp = (char **)new_contextp;
    return;

invalid:
    kmem_free(new_contextp, sizeof (struct dux_context));
    u.u_error = EINVAL;
    return;
}

struct dux_context *
build_context(context)
struct dux_context *context;
{
	register i;
	register char *s, *d;
	register char **cntxpp;

	bzero(context, sizeof context);

	cntxpp = &(context->ptr[0]);
	d = context->buf;
	for (i=0; i<NUM_CNTX_PTR; i++) {
		*cntxpp = d;
		s = "";
/*
 * Please forgive the "clever" programming style.
 * Essentially, each case is executed sequentially.
 */
		switch (i) {
#ifndef HPUXBOOT
	case 0:		s = my_sitename;		/* cnode name */
#else HPUXBOOT
	case 0:		s = "localroot";		/* cnode name */
#endif HPUXBOOT
			break;
#ifdef	__hp9000s300
	case 1:		if (processor == M68040)	/* cpu type */
				s = "HP-MC68040";
			break;
	case 2:		if (dragon_present)		/* fpp type */
				s = "HP98248A";
			break;
	case 3:		if (mc68881 || (processor == M68040))
				s = "HP-MC68881";
			break;
	case 4:		if (float_present)
				s = "HP98653A";
			break;
	case 5:		if (processor != M68010)	/* cpu type */
				s = "HP-MC68020";
			break;
	case 6:		s = "HP-MC68010";
			break;
#endif	/* hp9000s300 */
#ifdef	__hp9000s800
	case 1:
	case 2:
	case 3:
	case 4:
		break;
	case 5:
		if (cpu_version == CPU_PA_RISC1_1)
			s = "PA-RISC1.1";
		break;
	case 6:
		s = "HP-PA";
		break;
#endif	/* hp9000s800 */
	case 7:		if (my_site_status & CCT_ROOT)
				s = "localroot";
			else
				s = "remoteroot";
			break;
	case 8:		s = "default";
			break;
/* Potential BUG: must ensure that 16 * (#_cntx_entries) < CNTX_BUF_SIZE
   #_cntx_entries includes default case, due to terminator.  Need to
   #define CNTX_BUFSIZE (16*NUM_CNTX_PTR) */
	default:	i = NUM_CNTX_PTR;	/* done - bail out */
			break;
		} /* case */

		if (*s != '\0') {
			strncpy(d,s,DIRSIZ+1);
			*(cntxpp++) = d;
/*
 * The path lookup algorithm for long filename truncation assumes the name
 * buffer is at least DIRSIZ+1 (15).  We use "16" because it is a nice number.
 * When long filenames are supported, this restriction may disappear.
 */
			d += 16;
		}
	} /* for */
	return(context);
}


#ifdef STOPHERE
/*
 *This routine was renamed from stophere due to a name conflict in pagein
 *   ---joel
 */
stophere_sdo(name)
register char *name;
{
	register char **cntxpp;


	for (cntxpp = u.u_cntxp; **cntxpp != '\0'; cntxpp++)
		if (strcmp(*cntxpp,name) == 0)
			return(0);
	return(1);
}

getcname(name, ino, dp)
char *name;
ino_t ino;
struct inode *dp;
{
	int dirsize;
	struct buf *bp = 0;
	struct direct *ep;
	int entryoffsetinblock = 0;
	int offset = 0;
	extern struct buf *blkatoff();

	dirsize = roundup(dp->i_size, DIRBLKSIZ);
	while (offset < dirsize) {
		if (blkoff(dp->i_fs, offset) == 0) {
			if (bp != NULL)
				brelse(bp);
			bp = blkatoff(dp, offset, (char **)0);
			if (bp == NULL)
				return;
			entryoffsetinblock = 0;
		}
		ep = (struct direct *)(bp->b_un.b_addr + entryoffsetinblock);
		if (ep->d_ino == ino) {
			bcopy(ep->d_name, name, DIRSIZ+1);
			if (bp != NULL)
				brelse(bp);
			return;
		}
		offset += ep->d_reclen;
		entryoffsetinblock += ep->d_reclen;
	}
	if (bp != NULL)
		brelse(bp);
}
#endif	STOPHERE
