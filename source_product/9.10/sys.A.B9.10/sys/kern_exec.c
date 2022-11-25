/* $Source: /source/hpux_source/kernel/sys.SWT68K_800/sys/RCS/kern_exec.c,v $
 * $Revision: 1.88.83.6 $       $Author: dkm $
 * $State: Exp $        $Locker:  $
 * $Date: 94/11/17 15:32:16 $
 */

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

#include "../h/debug.h"
#include "../h/param.h"
#include "../h/user.h"
#include "../h/proc.h"
#include "../h/pathname.h"
#include "../h/vnode.h"
#include "../ufs/inode.h"
#include "../h/vfs.h"
#include "../h/uio.h"
#include "../h/acct.h"
#include "../h/magic.h"
#ifdef	SHAREMAGIC_EXEC
#include "../h/vm.h"
#include "../h/pregion.h"
#include "../h/region.h"
#include "../h/pfdat.h"
#endif	/* SHAREMAGIC_EXEC */
#include "../dux/lookupops.h"	/* for LKUP_EXEC */
#include "../netinet/in.h"	/* for sockaddr_in for nfs_clnt.h */
#include "../nfs/nfs_clnt.h"	/* for vtomi() */
#include "../h/kern_sem.h"
#include "../h/malloc.h"
#include "../h/pregion.h"

#ifdef __hp9000s300
#include "../h/systm.h"		/* for sysent tables */
#include "../machine/psl.h"
#include "../machine/reg.h"
#include "../machine/trap.h"	/* for exception_stack */
#endif

#ifdef SAR
#include "../h/sar.h"
#endif

#ifdef hp9000s800
#include "../machine/pde.h"	/* for spreg() */
#ifdef BTT
#include "../machine/btt.h"
#endif
#endif

#ifdef AUDIT
#include "../h/audit.h"
#endif

#if NBPG == 4096
#define TRAPNULLREF_CASE	(-1)
#define UNDEFINED_CASE		0
#define ZEROFILL_CASE		1
#endif

/*
 * aout_info:
 * a Hardware independent description of an executable
 */

typedef struct {
	u_short is_script;	/* true iff file starts with "#!"           */
				/*   if true, only shell field is valid     */
				/*   otherwise, only shell field is invalid */
	u_short sysid;		/* system ID (see magic.h) */
	u_short magic;		/* magic number (see magic.h) */
	char shell[SHSIZE];	/* shell command, if any ("#!" etc...) */
} aout_info_t;


/*
 * exec system call, with and without environments.
 */
struct execa {
	char *fname;
	char **argp;
	char **envp;
};

execv()
{
	((struct execa *)u.u_ap)->envp = NULL;
	execve();
}

execve()
{

	register struct execa *uap = (struct execa *)u.u_ap;

	register int  argc = 0;
	int  num_env = 0;

	/* buffer variables */
	char *buf = NULL;	/* assembly buffer for arg, env strings */
	char *kstrptr;		/* pointer to where to put next string */
	int strspace = 0;	/* space used for strings (in chars) */
	unsigned int room = NCARGS; /* maximum avail for strings (in chars) */
	char **pbuf = NULL;	/* assembly buffer for arg pointers */
	register char **kargptr; /* ptr to next string ptr to fill in */
	int ptrspace = 0;	/* space used for pointers (in chars) */

	char *usrstrbase;	/* userland pointer, destination strings copyou
t()'d */
	int total_len;	/* bytes copied out onto user's stack */
	register int reloc_offset;
#ifdef hp9000s800
	int ptr_reloc_offset;	/* for reloc'ing the next two. */
	char **argv;	/* rather than on the stack.  Keep track of where */
	char **envp;	/* they will be in these. */
#endif
	/* utility locals */
	register char **ap;
	char *ustr;
	int dummy;
	int error;
	int len;
	int i;

	/* fs stuff */
	struct pathname pn;
	struct vnode *vp;
	struct vattr vattr;
	uid_t uid;
	gid_t gid;

	aout_info_t aout_info;
	int pound_bang = 0;	/* set iff we see a "#!" script */
	int shargc = 0;	 	/* number of script related args */
	char cfarg[SHSIZE], *sharg, *execnamep;

#ifdef AUDIT
	/*
	 * Save off auditing information if enabled.
	 */
	if (AUDITEVERON()) {
		(void)save_pn_info(uap->fname);
	}
#endif

	/*
	 * Get hold of the program the user wants executed
	 */

	if (u.u_error = pn_get(uap->fname, UIOSEG_USER, &pn)) {
		return;
	}

	if (u.u_error = lookuppn(&pn, FOLLOW_LINK, (struct vnode **)0, &vp,
					LKUP_EXEC, (caddr_t)0)) {
		pn_free(&pn);
		return;
	}

	/*
	 * Call inctext to ensure no writes to file and for dux
	 * to have a positive client count to match the server's count.
	 * We always do a dectext() at the end of execve() to match
	 * this inctext().
	 */
	inctext(vp, SERVERINITIATED);

	/*
	 * Keep track of number of exec's for SAR
	 */
#ifdef SAR
	sysexec++;
#endif

	/*
	 * Determine what uid and gid the process shall have whilst
	 * running the executable
	 */

	/* default to ourself */
	uid = u.u_uid;
	gid = u.u_gid;
	if (u.u_error = VOP_GETATTR(vp, &vattr, u.u_cred, VASYNC)) {
		goto bad;
	}

	/* are set[ug]id's allowed on this FS? */
	if ((vp->v_vfsp->vfs_flag & VFS_NOSUID) == 0) {
		/* OK, but did they want one? */
		if (vattr.va_mode & VSUID) {
			uid = vattr.va_uid;
		}
		if (vattr.va_mode & VSGID) {
			gid = vattr.va_gid;
		}
	} else if ((vattr.va_mode & VSUID) || (vattr.va_mode & VSGID)) {
		/* desired, but not allowed. */

		/*
		 * Need to get the pathname again as lookuppn() may have
		 * modified it.  We want to give the user the original path.
		 */
		struct pathname tmppn;
		if (u.u_error = pn_get(uap->fname, UIOSEG_USER, &tmppn)) {
			goto bad;
		}
			uprintf("%s: Setuid execution not allowed\n",
				tmppn.pn_buf);

/* Before snakes, msg_printf on the s800 was the same as uprintf, i.e., it
printed the message to the user terminal.  At snakes we made msg_printf print
the message to the kernel message buffer like it should. */

#ifdef _WSIO
			msg_printf("%s: Setuid execution not allowed\n",
					tmppn.pn_buf);
#endif
			pn_free(&tmppn);
	}

	/*
	 * Come back here when looping to do pound_bang's
	 */

again:

	/*
	 * VOP_ACCESS (for ufs, nfs, dux, cdfs) has been changed to
	 * require at least one 'x' bit to be on a regular file for
	 * SU exec access.
	 */

	/* can we EXEC this? */
	if (u.u_error = VOP_ACCESS(vp, VEXEC, u.u_cred)) {
		goto bad;
	}

	/* SU can 'exec' any directory.  Make sure we have a regular file. */
	if (vp->v_type != VREG) {
		u.u_error = EACCES;
		goto bad;
	}

	/* if we are being ptraced, make sure we can read the image */
	if ((u.u_procp->p_flag & STRC)
	    && (u.u_error = VOP_ACCESS(vp, VREAD, u.u_cred))) {
		goto bad;
	}

	/*
	 * Read in the first few bytes of the file for useful info.
	 */
	if (u.u_error = get_aout_info(vp, &aout_info)) {
		goto bad;
	}

	/*
	 * Now deal with this being a script if it is one.
	 */
	if (aout_info.is_script) {
		struct pathname shell_pn;
		/*
		 * Now get:
		 * The name of the interpreter (execnamep)
		 * and
		 * the optional argument, if any (sharg).
		 *
		 * The parsing buffer is *VERY* small.
		 *
		 * NOTE:  Only one level of "#!" is permitted, therefore
		 *        once we see this, the next file is required to
		 *        be a native-machine a.out.
		 *
		 * only once through this path == 1 level of "#!"
		 */
		if (pound_bang) {
			u.u_error = ENOEXEC;
			goto bad;
		}
		pound_bang = 1;

		/* find (and mark) the end of the command */
		for (ustr = &aout_info.shell[2];	/* skip the "#!" */
		     ustr < &aout_info.shell[SHSIZE]; ustr++)
			if (*ustr == '\t')
				*ustr = ' ';
			else if (*ustr == '\n') {
				*ustr = '\0';
				break;
			}
		if (ustr >= &aout_info.shell[SHSIZE]) {	/* off the end? */
			u.u_error = ENOEXEC;
			goto bad;
		}

		/* find the start of the command's name */
		for (ustr = &aout_info.shell[2]; *ustr == ' '; ustr++)
			;
		execnamep = ustr;

		/* find the end of the command's name */
		while (*ustr && *ustr != ' ')
			ustr++;

		/* get the optional argument, if any */
		sharg = NULL;
		if (*ustr) {
			*ustr++ = '\0';
			while (*ustr == ' ')
				ustr++;
			if (*ustr) {
				bcopy((caddr_t)ustr, (caddr_t)cfarg, SHSIZE);
				sharg = cfarg;	/* save ptr to opt shell arg */
			}
		}

		/*
		 * Done with this file.  We release it, and turn around and
		 * grab its interpreter instead.  Note that we have *not*
		 * relinquished its (uap->) argp or envp.
		 */
		
		/* let go of this file */
		dectext(vp, 1);
		vn_rele(vp);
		vp = (struct vnode *)NULL;

		/* grab the new "shell" */
		if (u.u_error = pn_get(execnamep, UIOSEG_KERNEL, &shell_pn)) {
			goto bad;
		}
		if (u.u_error = lookuppn(&shell_pn, FOLLOW_LINK,
				(struct vnode **)0, &vp, LKUP_EXEC,(caddr_t)0)){
			vp = (struct vnode *)NULL;
			pn_free(&shell_pn);
			goto bad;
		}

		/* hold on to the new one */
		inctext(vp, SERVERINITIATED);

		/*
		 * now sync up with where we would be if this excursion never
		 * had happened.
		 */
		pn_free(&shell_pn);
		if (u.u_error = VOP_GETATTR(vp, &vattr, u.u_cred, VASYNC)) {
			goto bad;
		}

		/* start over */
		goto again;
	}



	/*
	 * Loop thru uap->argp, count argc. If we can't fuword() a ptr,
	 * error is EFAULT; bug out.
	 */

	/*
	 * If this is a script, its name will be an argument.
	 * Account for the number of special arguments associated
	 * with executing interpreter scripts (shargc) for
	 * later use.
	 */
	if (pound_bang) {
		argc++;
		shargc++;
		/* as will the interpreter arg, if it exists */
		if (sharg) {
			argc++;
			shargc++;
		}
	}
	ap = uap->argp;		/* local copy */
	if (ap) {	/* may be that we do not have ANY args. */
		while (1) {	/* get all that we can */
			if ((dummy = fuword((caddr_t)ap++)) == 0) {
				break;
			}
			if (dummy == -1) {
				u.u_error = EFAULT;
				goto bad;
			}
			argc++;
		}
	}

	/*
	 * Loop thru uap->envp, count num_env. If we can't fuword() a ptr,
	 * error is EFAULT; bug out.
	 */

	ap = uap->envp;		/* local copy */
	if (ap) {	/* may be that we do not have any env strings */
		while (1) {	/* get all that we can */
			if ((dummy = fuword((caddr_t)ap++)) == 0) {
				break;
			}
			if (dummy == -1) {
				u.u_error = EFAULT;
				goto bad;
			}
			num_env++;
		}
	}

	/*
	 * Calculate room needed for pointers
	 *
	 * Allocate room for the argv pointers, and their terminating NULL
	 *	(the NULL is always there)
	 *
	 * Allocate room for the envp pointers, and their terminating NULL
	 *	(the NULL is always there)
	 *
	 * On the 300, argc is stored in the first word of the pointer space.
	 * On the 800, there is one word of string padding between
	 *	       the strings and pointers.
	 * Either way, allocate one word more.
	 */

	ptrspace = (argc + num_env + 3) * NBPW;

	/*
	 * Get the memory.
	 *
	 * We want to keep the stack word aligned.  Thus, NCARGS needs to
	 * stay word-aligned.
	 */
	MALLOC(buf, char *, NCARGS, M_TEMP, M_WAITOK);
	kstrptr = buf;		/* working pointer */

	/*
	 * No alignment worries for the pointers
	 */
	MALLOC(pbuf, char **, ptrspace, M_TEMP, M_WAITOK);
	kargptr = pbuf;		/* working pointer */

#ifdef __hp9000s300
	/*
	 * On the 300, store argc into beginning of the pbuf.
	 *
	 * this method is a bit un-kosher, as argc isn't a (char *), but
	 * it is a 32 bit quantity....
	 */
	*kargptr++ = (char *) argc;
#endif
#ifdef hp9000s800
	/*
	 * On the 800, there is one word of string padding here.
	 */
	*kargptr++ = (char *) 0;
#endif

	/*
	 * Now go get the arguments
	 */

	i = 0;			/* total number of strings fetched */
#ifdef hp9000s800
	argv = kargptr;		/* this is what argv will point at */
#endif
	/*
	 * Do the first arg if there is one.  A null first argument
	 * violates execve calling conventions, but we have to
	 * allow it and make sure that argv[0] is a null pointer.
	 * 
	 * shargc is a count of the number of special arguments
	 * that are fabricated by execve (i.e. not supplied by the user)
	 * in the process of executing a shell script.
	 * It's value will be one of the following:
	 * 	0 - not a shell script (native executable)
	 *	1 - shell script without any interpreter args
	 *	2 - shell script with one interpreter arg (e.g. -x)
	 *
	 * If we don't have any user-supplied arguments (i.e.
	 * argc == shargc), don't attempt to copy any in.  Most likely
	 * uap->argp is NULL in this case.
	 */
	if (argc > shargc) {
		ustr = (char *)fuword(uap->argp++);
		/*
		 * We have already counted up the arg pointers.  While
		 * it is possible that the args have changed since we
		 * did the count (consider what happens if the
		 * strings/ptrs are in shared mem), the only user of
		 * ustr, get_user_str(), can handle the potentially bad
		 * pointers.
		 */
		/*
		 * if (((int)ustr == 0) || ((int)ustr == -1)) {
		 *	u.u_error = EFAULT;
		 *	goto bad;
		 * }
		 */
		*kargptr++ = kstrptr;   /* record the ptr to the new string */
		if (u.u_error = get_user_str(ustr, &kstrptr, &room)) {
			goto bad;
		}
		i++;
	}
	/*
	 * now do pound_bang stuff as needed
	 */
	if (pound_bang) {
		if (sharg) {
			VASSERT(shargc == 2);
			*kargptr++ = kstrptr;
			error = copystr(sharg, kstrptr, (unsigned int)room, 
							&len);
			if (error) {
				if (error == ENOENT) {
					error = E2BIG;
				}
				u.u_error = error;
				goto bad;
			}
			room -= (unsigned int)len;
			kstrptr += len;
			i++;
		}
		VASSERT(shargc >= 1);
		*kargptr++ = kstrptr;
		if (u.u_error = get_user_str(uap->fname, &kstrptr, &room)) {
			goto bad;
		}
		i++;
	}
	/*
	 * now finish up with the rest of the arguments (if any)
	 */
	for (; i < argc; i++) {
		ustr = (char *)fuword(uap->argp++);
		/*
		 * if (((int)ustr == 0) || ((int)ustr == -1)) {
		 *	u.u_error = EFAULT;
		 *	goto bad;
		 * }
		 */
		*kargptr++ = kstrptr;
		if (u.u_error = get_user_str(ustr, &kstrptr, &room)) {
			goto bad;
		}
	}
	/*
	 * add a terminating NULL pointer for the list.
	 * If argc == 0, argv will be pointing at this NULL.
	 */
	*kargptr++ = 0;

	/*
	 * Now go get the environment strings
	 */
#ifdef hp9000s800
	envp = kargptr;		/* this is what envp will point at */
#endif
	for (i = 0; i < num_env; i++) {		/* may be none */
		ustr = (char *)fuword(uap->envp++);
		/*
		 * if (((int)ustr == 0) || ((int)ustr == -1)) {
		 *	u.u_error = EFAULT;
		 *	goto bad;
		 * }
		 */
		*kargptr++ = kstrptr;
		if (u.u_error = get_user_str(ustr, &kstrptr, &room)) {
			goto bad;
		}
	}

	/*
	 * add a terminating NULL pointer for the list.
	 * If num_env == 0, envp will be pointing at this NULL.
	 */
	*kargptr = 0;		/* set fullword null after envp's */

	/*
	 * Calculate just how many bytes we are going to move out onto the
	 * user's stack.  We make the count word-aligned, so that the
	 * stack stays that way.  Minimum to copy is 12 bytes:
	 * 800: one word of string padding and two NULL pointers;
	 * 300:	argc and two NULL pointers.
	 *
	 * Set usrstrbase, the dest. of the strings copyout() in user space.
	 */

	strspace = (( (kstrptr - buf) + (NBPW - 1)) & ~(NBPW - 1));
	total_len = strspace + ptrspace;

	/*
	 * Strings come at beginning of the stack, but note that the stacks
	 * run in opposite directions for the 300 and 800.
	 */
#ifdef __hp9000s300
	usrstrbase = (char *) USRSTACK - strspace;
#endif
#ifdef hp9000s800
	usrstrbase = (char *) USRSTACK;
#endif

	/* 
	 * String buffer runs from 'buf' to 'buf + strspace'.  Make it as if
	 * runs from 'usrstrbase' to 'usrstrbase + strspace'.
	 */
	reloc_offset = usrstrbase - buf;
#ifdef hp9000s800
	/*
	 * For argv, envp (stored in registers) on the 800,
	 * Pointer buffer runs from 'pbuf' to 'pbuf + ptrspace'. Make it as if
	 * runs from usrstrbase + strspace to usrstrbase + strspace + ptrspace.
	 */
	ptr_reloc_offset = (usrstrbase + strspace) - (char *)pbuf;
#endif

	/*
	 * Go get the new executable
	 */

	KPREEMPTPOINT();	/* allow kernel preemption here */
	if (getxfile(vp, total_len, uid, gid, &aout_info, &vattr)) {
		goto bad;
	}
	KPREEMPTPOINT();	/* allow kernel preemption here */

	/*
	 * Let pstat() know about the new executable and args
	 */
#ifdef __hp9000s300
	pstat_cmd(u.u_procp, buf, argc, pn.pn_path);
#endif
#ifdef __hp9000s800
	pstat_cmd(u.u_procp, buf, argc, pn.pn_buf);
#endif

	/*
	 * Relocate the string pointers to user's-stack-relative addresses.
	 * For the 300, we couldn't do this until we knew strspace, (and thus
	 * the starting destination address).
	 * For the 800, we could have done this relocation on the fly in the
	 * argument fetching loop.
	 *
	 * Skip padding/argc to get to the pointers.
	 */
	for (ap = &pbuf[1]; ap < (char **)((char *)pbuf + ptrspace); ap++) {
		if (*ap) {			/* NULL ptrs stay NULL */
			*ap += reloc_offset;
		}
	}


	/*
	 * Set up some other registers
	 *
	 * On the 300, this is the stack ptr.
	 * On the 800, this is the stack ptr, flags, and the arguments.
	 */
#ifdef __hp9000s300
	/* set the stack pointer into user A.S */
	u.u_ar0[SP] = (unsigned) usrstrbase - ptrspace;
#endif
#ifdef hp9000s800
	/* set up 'arguments' to the program */
	u.u_sstatep->ss_gr26 = argc;
	u.u_sstatep->ss_gr25 = (int)argv + ptr_reloc_offset;
	u.u_sstatep->ss_gr24 = (int)envp + ptr_reloc_offset;

	/* set up stack ptr to just past this mess in user's A.S. */
	u.u_sstatep->ss_sp = (unsigned int) usrstrbase + 
			     (unsigned int) total_len;

	/* and the user's args are valid. */
	u.u_sstatep->ss_flags |= SS_ARGSVALID;
#endif

	/*
	 * Call exec_cleanup() to fix up some other things
	 */
	exec_cleanup(&aout_info);

	/*
	 * Toss the whole thing out onto the user's stack.
	 *
	 * We do this in two parts, pointers and strings.
	 * The pointers are 'later' in the stack than the strings.
	 * This means that:
	 * On the 300, the pointers are in the lower addresses and
	 *	the strings are in the higher addresses, while
	 * On the 800, the strings are in the lower addresses and
	 *	the pointers are in the higher addresses.
	 */

#ifdef __hp9000s300

	if (copyout(pbuf, usrstrbase - ptrspace, ptrspace)) {
		u.u_error = EFAULT;
		goto bad;
	}
	if (copyout(buf, usrstrbase, strspace)) {
		u.u_error = EFAULT;
		goto bad;
	}
#endif
#ifdef hp9000s800
	if (copyout(buf, usrstrbase, strspace)) {
		u.u_error = EFAULT;
		goto bad;
	}
	if (copyout(pbuf, usrstrbase + strspace, ptrspace)) {
		u.u_error = EFAULT;
		goto bad;
	}
#endif


	/*
	 * Mark program name for accounting
	 */
	u.u_acflag &= ~AFORK;

#if defined(hp9000s800) && defined(BTT)
	/*
	 * Do any Taken-Branch-Tracing monitoring.
	 */
	if (btt_control&BTT_CONTROL_TRACING) {
		btt_add_entry(BTT_TT_EXEC|
			((findpregtype(u.u_procp->p_vas,PT_TEXT))->p_space),
			u.u_comm, pn.pn_pathlen+1);
	}
#endif


	/*
	 * Fall through to do some final cleaning up.
	 */

bad:
	pn_free(&pn);

#ifdef  FSD_KI
	u.u_dev_t = (dev_t)vp; /* passed back to KI with this kludge */
#endif  /* FSD_KI */

	/*
	 * we no longer need this vnode (if we even got that far)
	 */
	if (vp) {
		dectext(vp, 1);
		vn_rele(vp);
	}

	/* get rid of our assembly buffers (if any) */
	if (buf) {
		FREE((unsigned long)buf, M_TEMP);
	}
	if (pbuf) {
		FREE((unsigned long)pbuf, M_TEMP);
	}
}

/*
 * get_aout_info() reads the a.out file header, which is machine-dependent,
 * and loads a machine-independent structure with the info so the exec
 * doesn't have to know so much.
 *
 * NOTES:  Machine-specific a.out file format checks (system ID's, etc...)
 *         are done here, rather than in the more generic routines.
 */

#ifdef __hp9000s300
int
get_aout_info(vp, ap)
	struct vnode *vp;
	aout_info_t *ap;
{
	int error;
	int resid;

	u.u_exdata.ux_shell[0] = 0;		/* for zero length files */
	if (error = vn_rdwr(UIO_READ,
			vp, (caddr_t)&u.u_exdata, sizeof(u.u_exdata),
			0, UIOSEG_KERNEL, IO_UNIT, &resid, 0)) {
		return(error);
	}
	u.u_count = resid;

	/*
	 * Did we get everything we need?
	 */
	if ((sizeof(u.u_exdata)-resid < sizeof(u.u_exdata.Ux_A))
					&& u.u_exdata.ux_shell[0] != '#') {
		return(ENOEXEC);
	}

	/* script? or native machine code? */
	ap->is_script =
		u.u_exdata.ux_shell[0] == '#' && u.u_exdata.ux_shell[1] == '!';

	if (ap->is_script)
		bcopy((caddr_t)u.u_exdata.ux_shell, (caddr_t)ap->shell, SHSIZE)
;
	else {
		ap->shell[0] = '\0';

		ap->sysid = u.u_exdata.ux_system_id;
		ap->magic = u.u_exdata.ux_mag;

		switch (ap->magic) {
		case EXEC_MAGIC:
			u.u_exdata.ux_dsize += u.u_exdata.ux_tsize;
			u.u_exdata.ux_tsize = 0;
			break;
		case SHARE_MAGIC:
		case DEMAND_MAGIC:
			if (u.u_exdata.ux_tsize == 0) {
				return(ENOEXEC);
			}
			break;
		default:
			return(ENOEXEC);
		}

		/*
		 * Heterogeneous clusters expect EINVAL rather than ENOEXEC
		 * when the system ID does match the expected value(s).
		 * Also, it is important that this test be done *after* the
		 * magic number test.
		 */
		if (ap->sysid != HP9000S200_ID && ap->sysid != HP98x6_ID) {
			return(EINVAL);
		}
	}

	return 0;	/* ok */
}
#endif /* __hp9000s300 */

#ifdef hp9000s800
int
get_aout_info(vp, ap)
	struct vnode *vp;
	aout_info_t *ap;
{
        extern int cpu_version;
	int resid;
	int error;
	union {
		struct header header;
		char ux_shell[SHSIZE];
	} som_hdr;

	som_hdr.ux_shell[0] = 0;		/* for zero length files */
	if (error = vn_rdwr(UIO_READ,
			vp, (caddr_t)&som_hdr, sizeof(som_hdr),
			0, UIOSEG_KERNEL, IO_UNIT, &resid, 0)) {
		return(error);
	}
	u.u_count = resid;

	/*
	 * Did we get everything we need?
	 */
	if ((sizeof(som_hdr)-resid < sizeof(struct header))
					&& som_hdr.ux_shell[0] != '#') {
		return(ENOEXEC);
	}

	/*
	 * Copy magic number to u_area.
	 */
	u.u_exdata.ux_mag =
		(som_hdr.header.system_id << 16) | som_hdr.header.a_magic;

	/* script? or native machine code? */
	ap->is_script =
		som_hdr.ux_shell[0] == '#' && som_hdr.ux_shell[1] == '!';

	if (ap->is_script)
		bcopy((caddr_t)som_hdr.ux_shell, (caddr_t)ap->shell, SHSIZE);
	else {
		ap->shell[0] = '\0';

		ap->sysid = som_hdr.header.system_id;
		ap->magic = som_hdr.header.a_magic;

		switch (ap->magic) {
		case EXEC_MAGIC:
		case SHARE_MAGIC:
		case DEMAND_MAGIC:
			/*
			 * Read in SOM exec aux header.
			 */
			if (error = vn_rdwr(UIO_READ, vp,
					(caddr_t)&u.u_exdata.som_aux,
					sizeof(struct som_exec_auxhdr),
					(int)som_hdr.header.aux_header_location,
					UIOSEG_KERNEL, IO_UNIT, &resid, 0)) {
				return(error);
			}
			if (resid || (u.u_exdata.ux_tsize == 0)) {
				return(ENOEXEC);
			}

			/*
			 * We don't support external millicode.
			 */
			if (u.u_exdata.ux_flags & 0x2) {
				return(ENOEXEC);
			}

			break;
		default:
			return(ENOEXEC);
		}

		/*
		 * Heterogeneous clusters expect EINVAL rather than ENOEXEC
		 * when the system ID does match the expected value(s).
		 * Also, it is important that this test be done *after* the
		 * magic number test.
		 */
                if ((!_PA_RISC_ID(ap->sysid)) || (ap->sysid > cpu_version)) {
			return(EINVAL);
		}
	}

	return(0);	/* ok */
}
#endif /* hp9000s800 */

/*
 * Read in and set up memory for executed file.
 *
 * NOTE:  The "nargc" parameter is not used on Series 300 machines.
 */
/*ARGSUSED*/
getxfile(vp, nargc, uid, gid, ap, vattr)
	struct vnode *vp;
	int nargc;
	uid_t uid;
	gid_t gid;
	aout_info_t *ap;
	struct vattr *vattr;
{
	struct proc *p = u.u_procp;
	u_int textoff, dataoff;
	u_int textsize, datasize, stacksize, bsssize;
	caddr_t textaddr, dataaddr, stackaddr;
	int alignment;		/* original alignment of a.out */
	int addnull;		/* how to add a nullref region, if any */
	int addnull_to_text;	/* same as above but for a HP-PA special case */
	int execself;
	int align_mod;
	extern void (*graphics_exitptr)();
#ifdef	SHAREMAGIC_EXEC
	struct pregion *prp;
#endif	/* SHAREMAGIC_EXEC */

	vm_sema_state;		/* semaphore save state */

	vmemp_lockx();		/* lock down VM empire */

	addnull_to_text = UNDEFINED_CASE;

	/*
	 * This code assumes that VTEXT has been set by
	 * the caller.
	 */
	VASSERT(vp->v_flag & VTEXT);

	/*
	 * See if someone is currently writing on this file.
	 */
	if (openforwrite(vp, 0)) {
		u.u_error = ETXTBSY;
		goto bad;
	}

	/*
	 * For each magic number, the following set of a.out-dependent
	 * values are needed to load the file (unless specified, all
	 * values are in terms of bytes):
 	 *
	 *	textoff		- offset in file of where text is.
	 *	textsize	- size of text in bytes.
	 *	textaddr	- address of text in memory.
	 *	dataoff		- offset in file of where data is.
	 *	datasize	- size of data in bytes.
	 *	dataaddr	- address of data in memory.
	 *	bsssize		- size of bss.
	 *	addnull		- add a null deref page:  (HP-PA only)
	 *	  TRAPNULLREF (-1) -> trap on zero reference (unmap zero page)
	 *	  UNDEFINED    (0) -> do nothing special
	 *	  ZEROFILL     (1) -> map first page to read zeroes
	 *			 
	 *	magic:
	 *		EXEC_MAGIC	- Only a data object is created.
	 *		SHARE_MAGIC	- Create text and data (but do not
	 *				  assume file alignment).
	 *		DEMAND_MAGIC	- Assume both memory and file aligned.
	 *				  Note: If a SHARE_MAGIC is correctly
	 *					aligned, it is treated as a
	 *					a DEMAND_MAGIC.
	 *				  Note: Series 800 a.out's are always
	 *					aligned so they can be loaded
	 *					as either SHARE_MAGIC or
	 *					DEMAND_MAGIC.
	 */


#ifdef __hp9000s300
	switch (ap->magic) {
	case EXEC_MAGIC:
		/* 
		 * Note: text entries are not
		 * really used for EXEC_MAGIC.
		 */
		textoff = 0;
		textsize = 0;
		textaddr = 0;
		dataoff = TEXT_OFFSET(u.u_exdata.Ux_A);
		datasize = u.u_exdata.Ux_A.a_text +
			   u.u_exdata.Ux_A.a_data;
		if (u.u_exdata.ux_system_id == HP98x6_ID)
			dataaddr = (caddr_t)OLD_USER_BASE;
		else
			dataaddr = 0;
		alignment = 1;		/* no special alignment */
		break;

	case SHARE_MAGIC:
	case DEMAND_MAGIC:
		textoff = TEXT_OFFSET(u.u_exdata.Ux_A);
		textsize = u.u_exdata.Ux_A.a_text;
		if (u.u_exdata.ux_system_id == HP98x6_ID)
			textaddr = (caddr_t)OLD_USER_BASE;
		else
			textaddr = 0;
		dataoff = DATA_OFFSET(u.u_exdata.Ux_A);
		datasize = u.u_exdata.Ux_A.a_data;
		dataaddr = (caddr_t)EXEC_ALIGN((int)textaddr+textsize);
		alignment = ((ap->magic == SHARE_MAGIC) ? 1 : NBPG);
		break;
	default:
		panic("getxfile: bad magic number");
		break;
	}
	bsssize = u.u_exdata.Ux_A.a_bss;
	stacksize = u.u_rlimit[RLIMIT_STACK].rlim_cur;
	stackaddr = USRSTACK - stacksize;
	addnull = UNDEFINED_CASE;
#endif /* __hp9000s300 */

#ifdef hp9000s800

#if NBPG == 4096
	/*
	 * A 4kb system supports both 2k- and 4k-aligned executables.
	 * Otherwise assume SOM's with NBPG alignment.  (The following calculations
	 * assume NBPG = 4kb with a minimum SOM alignment of 2kb.
	 * They will not necessarily work for NBPG = 8k, for instance.)
	 */

	alignment = max(poff(u.u_exdata.ux_tloc),poff(u.u_exdata.ux_tmem));
	if (alignment == 0)
		alignment = NBPG;

	/*
	 * We don't have a backwards compatibility problem for EXEC_MAGIC
	 * so let's ensure page alignment.
	 */

	if (ap->magic == EXEC_MAGIC)
	    align_mod = NBPG;
	else
	    align_mod = NBPG_PA83;

	if ((alignment % align_mod) != 0) {
		uprintf("exec(2): invalid alignment in a.out\n");
		u.u_error = EFAULT;
		goto bad;
	}

#else	/* NBPG == 4096 */

#if NBPG == 8192
	THIS AREA NEEDS MORE WORK.
#endif
	/*
	 * Assume SOM's are guaranteed to be of NBPG alignment.
	 */
	alignment = NBPG;
#endif 	/* NBPG == 4096 */

	if (ap->magic == EXEC_MAGIC) {
	    textaddr = 0;
	    textoff  = 0;
	    textsize = 0;
	    dataaddr =  (caddr_t)u.u_exdata.ux_tmem;
	    dataoff  =  u.u_exdata.ux_tloc;
	    datasize =  u.u_exdata.ux_dloc - u.u_exdata.ux_tloc
					   + u.u_exdata.ux_dsize;

	    /*
	     * We only support contiguous TEXT and DATA for EXEC_MAGIC
	     * a.out's because we combine them into one data region.
	     * this is enforced by the following test.
	     */

	    if (    (u.u_exdata.ux_dloc - u.u_exdata.ux_tloc)
		 != (u.u_exdata.ux_dmem - u.u_exdata.ux_tmem) ) {

		    uprintf("exec(2): data must be contigous with text for EXEC_MAGIC\n");
		    u.u_error = EFAULT;
		    goto bad;
	    }
	}
	else {

	    /*
	     * Fix up text memory offset so that it starts
	     * on a page boundary.  Adjust text offset in file accordingly
	     * and make sure it doesn't go negative.
	     */

	    textaddr =  (caddr_t)pagedown(u.u_exdata.ux_tmem);
	    textoff  =  u.u_exdata.ux_tloc - poff(u.u_exdata.ux_tmem);
	    textsize =  u.u_exdata.ux_tsize + poff(u.u_exdata.ux_tmem);
	    if ((int)textoff < 0) {
		    uprintf("exec(2): negative text offset\n");
		    u.u_error = EFAULT;
		    goto bad;
	    }
	    dataaddr =  (caddr_t)u.u_exdata.ux_dmem;
	    dataoff  =  u.u_exdata.ux_dloc;
	    datasize =  u.u_exdata.ux_dsize;
	    /*
	     * We've decided to disallow executables that attach data starting
	     * at an odd 2k address.  To do so would force us to round down
	     * the start of data to a page boundary, thereby assigning
	     * the same physical page to the last page of text and the first page
	     * of data.  A load/store instruction in the last page of text
	     * that accessed the first page of data then would cause the
	     * system to thrash back and forth between the instruction
	     * and data translation of the physical page.  This could be avoided
	     * by ensuring unaligned pageins through the buffer cache
	     * for at least one of text or data, but the current code attempts
	     * to take the optimal pagein path as much as possible.  We've decided
	     * to optimize pagein's to the extent possible, at the cost of disallowing
	     * this rare case.  (Data defaults to address 40000000).
	     */
	    if (poff(dataaddr)) {
		    uprintf("exec(2): non-aligned data segment; please re-link executable\n");
		    u.u_error = EFAULT;
		    goto bad;
	    }
	}

	bsssize = u.u_exdata.ux_bsize;
	stacksize = ptob(SSIZE)+nargc;
	stackaddr = (caddr_t)USRSTACK;

#if NBPG == 4096
	VASSERT(poff(textaddr) == 0);
	VASSERT(poff(dataaddr) == 0);

	addnull = (u.u_exdata.ux_flags & Z_EXEC_FLAG) ? TRAPNULLREF_CASE 
						      : ZEROFILL_CASE ;
	/*
	 * Can't provide any CORRECT nullref semantics if text
	 * really begins at address zero. Also won't support nullref
	 * semantics for EXEC_MAGIC a.out's
	 */

	if ((caddr_t)u.u_exdata.ux_tmem == NULL || ap->magic == EXEC_MAGIC)
		addnull = UNDEFINED_CASE;

	/*
	 * If at this point the beginning of text has been adjusted
	 * to zero, it means the first page of text overlaps a nullref
	 * region.
	 */
	if ((addnull != UNDEFINED_CASE) && (textaddr == NULL)) {
		addnull_to_text = addnull;
		addnull = UNDEFINED_CASE;
	}

#endif /* NBPG == 4096 */

#endif /* hp9000s800 */

	/*
	 * Make a few simple sanity checks on the a.out.
	 */
	if (textoff+textsize > dataoff) {    /* text overlaps data */
		uprintf("exec(2): text overlaps data in a.out\n");
		u.u_error = EFAULT;
		goto bad;
	}

	/* 
	 * We can only check i_blocks with VUFS and VDUX types 
	 */
	if (vp->v_fstype == VUFS || vp->v_fstype == VDUX) {
		if ((u_int)roundup((unsigned)(dataoff+datasize), alignment)
				       	> dbtob(VTOI(vp)->i_blocks)) {
			/* a.out isn't big enough */
			uprintf("exec(2): data extends beyond end of a.out\n");
			u.u_error = EFAULT;
			goto bad;
		}
	}
	if ((dataoff + datasize) > vattr->va_size) {
		uprintf("exec(2): data extends beyond end of a.out\n");
		u.u_error = EFAULT;
		goto bad;
	}

	if ((caddr_t)roundup((unsigned)textaddr, alignment) != textaddr) {
		uprintf("exec(2): incorrectly aligned text in a.out\n");
		u.u_error = EFAULT;
		goto bad;
	}

	if ((caddr_t)roundup((unsigned)dataaddr, alignment) != dataaddr) {
		uprintf("exec(2): incorrectly aligned data in a.out\n");
		u.u_error = EFAULT;
		goto bad;
	}

#ifdef __hp9000s800
	if (ap->magic != EXEC_MAGIC) {
		if (spreg(textaddr) == spreg(dataaddr)) {
			uprintf("exec(2): text and data both in same quadrant\n");
			u.u_error = EFAULT;
			goto bad;
		}

		/*
		 * The above check catches most problems. However, the
		 * following check is necessary for the case where
		 * someone links an a.out with the text in the second
		 * quadrant, and the data in the first.
		 */

		if (spreg(textaddr) != 0) {
			uprintf("exec(2): text not in first quadrant\n");
			u.u_error = EFAULT;
			goto bad;
		}
	}
#endif /* __hp9000s800 */

	SPINLOCK(sched_lock);
	/*
	 * Clear certain flags -
	 *  SSEQL, SUANOM are for vadvise() - currently not supported
	 */
	u.u_procp->p_flag &= ~(SSEQL|SUANOM|SOUSIG);
	SPINUNLOCK(sched_lock);

#ifdef __hp9000s300
	if (u.u_pcb.pcb_dragon_bank != -1) {
		if ((u.u_procp->p_flag & SVFORK) == 0) {
			dragon_bank_free();
			dragon_detach(u.u_procp);
		}
		else
			u.u_pcb.pcb_dragon_bank = -1;
	}

        /*
	 * Make sure this process uses the proper sysent table.
	 */
	if (u.u_exdata.ux_system_id == HP98x6_ID) {
		u.u_pcb.pcb_sysent_ptr = compat_sysent;
		u.u_pcb.pcb_nsysent = ncompat_sysent;
	}
	else {
		u.u_pcb.pcb_sysent_ptr = sysent;
		u.u_pcb.pcb_nsysent = nsysent;
	}
#endif /* __hp9000s300 */

	if ((p->p_flag & SVFORK) == 0) {

	    /*
	     * After the dispreg(), we are committed to the new image!
	     * Release virtual memory resources of the old process,
	     * and initialize the virtual memory of the new process.
	     * If anything is wrong after this call we must issue
	     * a SIGKILL signal to the process.
	     */
	    execself = dispreg(p, vp, PROCATTACHED);
	    u.u_procp->p_vas->va_flags = 0;
	}
	else {
	    execself = 0;

	    p->p_flag |= SKEEP;

	    if (vfork_createU() != 0)
		goto signalit;

	    p->p_flag &= ~(SKEEP|SVFORK);
	}

	/*
	 * Build a special set of pregions and regions for each magic number.
	 */
	switch (ap->magic) {
	case SHARE_MAGIC:
	case DEMAND_MAGIC:
		if (!execself) {
			int off = btop(textoff);
			int count = btorp(dataoff + datasize) - off;

			inctext(vp, USERINITIATED);
			if (mapvnode(vp, off, count)) {
				dectext(vp, 1);
				goto signalit;
			}
			if (addnull_to_text != UNDEFINED_CASE) {
				VASSERT(addnull == UNDEFINED_CASE);
				VASSERT(textaddr == NULL);
#ifdef hp9000s800
				VASSERT(alignment == NBPG_PA83);
#endif
				if (add_text_2k_compat(vp, textoff, textsize,
						   textaddr, addnull_to_text)){
					unmapvnode(vp);
					dectext(vp, 1);
					goto signalit;
				}
			} else {
				if (add_text(vp, textoff, textsize, textaddr)){
					unmapvnode(vp);
					dectext(vp, 1);
					goto signalit;
				}
		        }
			if (addnull > 0) {
				VASSERT(addnull_to_text == 0);
				if (add_nulldref(&u, u.u_procp->p_vas))
					goto signalit; 
			}
		}
		if (add_data(vp, dataoff, datasize, dataaddr, vattr->va_size))
			goto signalit;
#ifdef	SHAREMAGIC_EXEC
		if (ap->magic == SHARE_MAGIC && vp->v_vas == NULL) {
			if ((prp = findpregtype(u.u_procp->p_vas, PT_TEXT)) == NULL)
				panic("we arent having a good day at all");  /* XXX  */
			/* get_share_magic(prp); */
			if ((freemem - prp->p_count) > 512) 
				share_magic_vfdfill(vp, prp, textoff, textaddr, textsize);
			else
				fault_each(u.u_procp->p_vas, PT_TEXT); /* XXXX */
			fault_each(u.u_procp->p_vas, PT_DATA);
		}
#endif	/* SHAREMAGIC_EXEC */
		break;

	case EXEC_MAGIC:
		/*
		 * Create a private text segment that includes
		 * data.  We mark the pregion PT_DATA because
		 * it makes more sense than having a PT_TEXT
		 * being writable.
		 */
		if (create_execmagic(vp, dataoff, datasize, dataaddr))
			goto signalit;
		break;
	default:
		panic("getxfile: unknown magic number");
		break;
	}

	/*
	 * Add bss and stack.
	 */
	if (add_bss((int)bsssize))
		goto signalit;
	if (add_stack(stacksize, stackaddr))
		goto signalit;

	/*
         *  Do exec processing for graphics, if present.
         */
        if (u.u_procp->p_flag2 & SGRAPHICS) {
		VASSERT(graphics_exitptr);
		(*graphics_exitptr)();

         /* Even though graphics_exit() has been called, we still have
          * the potential to be a graphics process since any open
          * framebuffer file descriptors are still valid. Turn the 
          * SGRAPHICS flag back on so graphics_exit() will be called
          * again upon exec/exit to clean up after any subsequent framebuffer
          * operations.
          */
		SPINLOCK(sched_lock);
		u.u_procp->p_flag2 |= SGRAPHICS;
		SPINUNLOCK(sched_lock);
	}

#ifdef __hp9000s300
	/*
	 * Clear signal to send for DIL across an exec.
	 */
	u.u_procp->p_dil_signal = 0;

	/* Set S2DATA_WT and S2STACK_WT appropriately */

	u.u_procp->p_flag2 &= ~(S2DATA_WT|S2STACK_WT);

	if (   (u.u_exdata.ux_miscinfo & M_DATA_WTHRU)
	    || (ap->magic == EXEC_MAGIC) ) {

	    u.u_procp->p_flag2 |= S2DATA_WT;
	}

	if (u.u_exdata.ux_miscinfo & M_STACK_WTHRU)
	    u.u_procp->p_flag2 |= S2STACK_WT;
#endif

	if (u.u_procp->p_flag & STRC) {
		/*
		 * Tracing processes across interruptable NFS mount-points
		 * requires pre-pagein.
		 *
		 * NOTE:  We could make adoptive tracing over interruptable
		 *        NFS mount-points possible by pre-loading *all*
		 *        text segments on the off chance they might later
		 *        be adopted, but the overhead for non-traced
		 *        processes would be much too high.
		 */
		if (vp->v_fstype == VNFS && vtomi(vp)->mi_int) {
			if (pre_demand_load(u.u_procp->p_vas))
				goto signalit;
		}
		psignal(u.u_procp, SIGTRAP);
	} else {
		/*
		 * Set UID/GID protections.
		 */
 		if (u.u_gid != gid) {
			SPINLOCK(sched_lock);
 			u.u_procp->p_flag |= SPRIV;
			SPINUNLOCK(sched_lock);
 		}
		if (uid != u.u_uid || gid != u.u_gid)
			u.u_cred = crcopy(u.u_cred);
		u.u_uid = uid;
		u.u_gid = gid;
		SPINLOCK(PROCESS_LOCK(u.u_procp));
		u.u_procp->p_suid = uid;
		SPINUNLOCK(PROCESS_LOCK(u.u_procp));
		u.u_sgid = gid;
	}

	u.u_prof.pr_scale = 0;		/* profiling */

	SPINLOCK(sched_lock);

	/*
	 * This process has exec'ed successfully.
	 */
	u.u_procp->p_flag2 |= S2EXEC;
	SPINUNLOCK(sched_lock);

	vmemp_unlockx();		/* free up VM empire */
	return(0);

signalit:
	u.u_procp->p_flag &= ~STRC;
	psignal(u.u_procp, SIGKILL);
	/*
	 * Neither the parent or child will notice
	 * this, but it makes the check for u.u_error
	 * below happy.
	 */
	u.u_error = EFAULT;

	/* fall through... */
bad:
	VASSERT(u.u_error);
	vmemp_unlockx();	/* free up VM empire */
	return(1);
}

/*
 * Clear registers (and other things) on exec
 */
/*ARGSUSED*/
exec_cleanup(aout_infop)
	aout_info_t *aout_infop;
{
	register int i, x;
	register struct proc *p = u.u_procp;
	int delay_signal;
#ifdef __hp9000s300
	extern int dragon_present;
	extern int float_present;
	extern char mc68881;
	struct exception_stack *regs = (struct exception_stack *)u.u_ar0;
#define EXEC_DRAGON_MASK	0x40000000
#define EXEC_M68040_MASK        0x20000000
#endif


	/*
	 * Reset caught signals.  Held signals remain held through p_sigmask.
	 */

	delay_signal = 0;
	while (p->p_sigcatch) {
		SPL_REPLACEMENT(PROCESS_LOCK(p),spl6,x);
		i = ffs((long)p->p_sigcatch) - 1;
		p->p_sigcatch &= ~(1 << i);
		u.u_signal[i] = SIG_DFL;

		/*
		 * It is possible that p_cursig may be non-zero
		 * at this point, due to issig getting called during
		 * the exec (one place this happens is in the dm_send
		 * code for DUX). If p_cursig corresponds to a signal
		 * whose default action is to ignore the signal, the
		 * new process will get killed even though it should
		 * not. To solve this problem, we resend the signal
		 * and clear p_cursig.
		 */

		if (p->p_cursig == (i+1) ) {
			delay_signal = p->p_cursig;
			p->p_cursig = 0;
		}

		SPLX_REPLACEMENT(PROCESS_LOCK(p),x);
	}

	if (delay_signal > 0)
		psignal (p, delay_signal);

#ifdef __hp9000s300
	/*
	 * Reset the mc68881 floating point coprocessor.
	 */
	reset_mc68881();

	regs->e_PC = u.u_exdata.ux_entloc & ~1;	/* changed for 68K */
	regs->e_PS = PSL_USERSET;		/* added (from 4.1c sun) */
	regs->e_regs[AR0] = 0;
	regs->e_regs[R2] = u.u_exdata.ux_miscinfo;
	if (dragon_present)
		regs->e_regs[AR0] |= EXEC_DRAGON_MASK;
	if (processor == M68040)
		regs->e_regs[AR0] |= EXEC_M68040_MASK;

	/*
	 * The user's d0 will contain the float_present flag.
	 * This will get interpreted by crt0.o and put into the
	 * user flag 'float_soft' (with no leading underscore).
	 *
	 * Since the float card is mapped in at a different location than in
	 * 2.x systems, the flag is always set to zero for old object files.
	 *
	 * Similarly, the user's d1 will contain the m68881 flag.
	 *
	 * To get it into d0, we use u_rval1, which will get placed into
	 * d0 when our system call returns.  It might get overwritten with
	 * errno if we have an error in the exec, but that's ok.  We use
	 * u_rval2 to get at d1.
	 */
	u.u_rval1 = (aout_infop->sysid == HP98x6_ID) ? 0 : float_present;
	u.u_rval2 = mc68881;

#endif /* __hp9000s300 */

#ifdef hp9000s800
	/*
	 * Set this to show no stack space allocated for any signals.
	 */
	u.u_sigonstack = 0;

	u.u_sstatep->ss_gr31 = u.u_exdata.ux_entloc | PC_PRIV_USER;

	/* clear up FP state. */
	u.u_sstatep->ss_frstat= 0;
	u.u_sstatep->ss_frexcp1 = 0;
	u.u_sstatep->ss_frexcp2 = 0;
	u.u_sstatep->ss_frexcp3 = 0;
	u.u_sstatep->ss_frexcp4 = 0;
	u.u_sstatep->ss_frexcp5 = 0;
	u.u_sstatep->ss_frexcp6 = 0;
	u.u_sstatep->ss_frexcp7 = 0;
#endif /* hp9000s800 */

	/*
	 * Close files marked close-on-exec.
	 */
	for (i = 0; i <= u.u_highestfd; i++) {
		struct file *fp;
		char *pp;
		extern struct file *getf_no_ue();

		if ((fp = getf_no_ue(i)) != NULL) {
			if (getp(i) & UF_EXCLOSE) {
				closef(fp);
				uffree(i);
			} else {
				pp = (char *)getpp(i);
				*pp &= ~UF_MAPPED;
			}
		}
	}
}


/*
 * get_user_str() fetches a string from userland, using copyinstr().
 * It updates the destination pointer and room left count for its caller.
 * Any errors from copyinstr() are propagated up, and ENOENT is translated
 * to E2BIG (as the exec() interface expects).
 */

get_user_str(usrcp, kdstpp, roomp)
	char	*usrcp;
	char	**kdstpp;
	unsigned int	*roomp;
{
	int err;
	unsigned int len;

	err = copyinstr(usrcp, *kdstpp, *roomp, &len);
	if (err) {
		if (err == ENOENT) {
			err = E2BIG;
		}
		return(err);
	}
	*roomp -= len;
	*kdstpp += len;
	return(0);
}
#ifdef	SHAREMAGIC_EXEC

/*
 *	get a share_magic executable
 */
get_share_magic(prp)
preg_t *prp;
{
        reg_t *rp = prp->p_reg;
        size_t index;
        vfd_t *vfd;
        int i;


	reglock(rp);
        index = regindx(prp, 0);

        for (i = 0; i < prp->p_count; i++,index++) {
                if ((vfd = FINDVFD(rp, (int)index)) == (vfd_t *)NULL)
                        panic("bring_in_pages: vfd not found");

                if (virtual_share_magic(prp, ptob(i))) {
			regrele(rp);
                        return(ENOSPC);
		}
        }
	regrele(rp);
        return(0);
}




/*
 *  find us a share_magic page
 */
int
virtual_share_magic(prp, vaddr)
preg_t *prp;
caddr_t vaddr;
{
	reg_t *rp;
	int count;
	int pgindx;	/* index of page faulted on */
	int startindex;	/* index of page where the fault really started */
	vfd_t *vfd;
	dbd_t *dbd;
	extern int xkillcnt;


	/*
	 *  find vfd and dbd as mapped by region
	 */
	rp = prp->p_reg;
	pgindx = regindx(prp, vaddr);
	startindex = pgindx;
	findentry(rp, pgindx, &vfd, &dbd);

	/*
	 *  see what state the page is in.
	 */
	VASSERT(dbd->dbd_type == DBD_FSTORE);
	/*
	 * the purpose of this check for now is to avoid
	 * entering the vop_pagein routines in case
	 * it is invalid to do so (e.g. cleaned up
	 * text vnode as a result of an lmfs crash).
	 * doing this minimizes the number of cases for
	 * which we can encounter an r_zomb failure --
	 * in particular we cannot currently handle
	 * a kernel induced fault on such a region
	 * very gracefully.  See DTS #DSDe404153.
	 *
	 * This assumption will have to change if the
	 * BSTORE pagein routines (for now just devswap_pagein)
	 * decide to use the r_zomb mechanism, and for
	 * coherent MMF's because then we may wish to extend
	 * r_zomb to affect decisions on BSTORE faults/actions.
	 *
	 * XXX MMF_REVISIT XXX
	 * XXX HP_REVISIT XXX
	 */
	if (rp->r_zomb) {
		uprintf("Pid %d killed due to text modification or page I/O error\n",
			u.u_procp->p_pid);
		xkillcnt++;
		/*
		 * -SIGKILL overrides any error previously
		 * returned by hdl_vfault().
		 */
		return(-SIGKILL);
	}

	/*
	 * If the file isn't page-aligned in its view, read
	 * in through the file system and do an unaligned copy
	 * into the user's space.
	 */
	if (rp->r_flags&RF_UNALIGNED) {
		count = unaligned_fault(prp, prp->p_space, vaddr, vfd, dbd);
	} else {
		u.u_procp->p_flag2 |= SANYPAGE;
		count = VOP_PAGEIN(rp->r_fstore, prp, 0,
					prp->p_space, vaddr, &startindex);
		u.u_procp->p_flag2 &= ~SANYPAGE;
                }
	if (count < 0) 
		return count;
	return(0);
}


share_magic_vfdfill(vp, prp, textoff, textaddr, bytes)
	register struct vnode *vp;
	register preg_t	*prp;
	u_int textoff, bytes;
	caddr_t textaddr;
{
	pfd_t *pfd;
	register vfd_t *vfd;
	register int count;
	int start = btop(textaddr);
	struct region *rp = prp->p_reg;
	caddr_t vaddr = textaddr;


printf("share_magic_vfd: vp = 0x%x, prp = 0x%x, textoff = 0x%x, textaddr = 0x%x, bytes = 0x%x\n", 
	vp, prp, textoff, textaddr, bytes);

	VASSERT(rp && start >= 0);
	reglock(rp);

	if (rp->r_zomb) {
		uprintf("Pid %d killed due to text modification or page I/O error\n",
			u.u_procp->p_pid);
		xkillcnt++;
		/*
		 * -SIGKILL overrides any error previously
		 * returned by hdl_vfault().
		 */
		regrele(rp);
		return(-SIGKILL); /* XXX how do we handle errors? */
	}

	memreserve(rp, (unsigned int)prp->p_count);

	for (count = prp->p_count; --count >= 0; start++, vaddr += NBPG) {
		vfd = FINDVFD(rp, start);
		VASSERT((vfd->pgm.pg_v) == 0);
		pfd = allocpfd();

		/*
		 * Insert in vfd
		 */
		/* XXXX gross hack for now... dont write protect the text */
		prp->p_hdl.p_hdlflags &= ~PHDL_RO;
		/* XXX end gross hack */
#ifdef PFDAT32
		vfd->pgm.pg_pfn = pfd - pfdat;
		hdl_addtrans(prp, prp->p_space, vaddr, 0 /* XXX cow*/, pfd - pfdat);
#else
		vfd->pgm.pg_pfn = pfd->pf_pfn;
		hdl_addtrans(prp, prp->p_space, vaddr, 0 /* XXX cow*/, pfd->pf_pfn);
#endif
		vfd->pgm.pg_v = 1;
		rp->r_nvalid++;
		pfdatunlock(pfd);	/*  XXX  is it OK to unlock here?  */
	}
	VASSERT(rp->r_nvalid <= rp->r_pgsz);
	regrele(rp);

	vn_rdwr(UIO_READ, vp, textaddr, bytes, textoff, UIOSEG_USER, IO_UNIT, 0, 0);
}

#endif	/* SHAREMAGIC_EXEC */
