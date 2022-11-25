/* $Source: /source/hpux_source/kernel/sys.SWT68K_800/sys/RCS/audctl.c,v $
 * $Revision: 1.11.83.3 $	$Author: kcs $
 * $State: Exp $   	$Locker:  $
 * $Date: 93/09/17 20:03:43 $
 */
/* HPUX_ID: @(#)audctl.c	55.1		88/12/23 */

/* 
(c) Copyright 1983, 1984, 1985, 1986, 1987, 1988  Hewlett-Packard Company.
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

/**************************************************************************
          audctl - turn on or turn off the audit system  and  set  and
          get the audit file.

     SYNOPSIS
          #include <sys/audit.h>

          audctl(cmd, cpath, bpath, mode)
          char *cpath, *bpath;
          int cmd, mode;

     DESCRIPTION
          Audctl sets and gets the "current" audit file and the "next"
          audit file, turns on or off the audit system and reports the
          auditing state.  This  call  is  restricted  to  users  with
          appropriate  privilege.   The  paths are used to hold audit
          file names. The mode is the mode for the creation of the audit
	  files.

          AUD_SET        If  the  audit  system  is  on  and  the
                         AUD_SET  is  given,  the "current" audit
                         file is switched to the specified cpath.
                         Use the file specified by the bpath
                         parameter  as the "next" audit file.
                         If the "current" audit file fills up,  the
                         audit system automatically switches to the
                         "next" file. If there is an existing "next"
                         file,  the file specified via the path
                         will replace the existing "next" file.
                         If  there is not a "next" file, the file
                         specified via the path will be the
                         "next" file. Fully qualified paths are 
			 required (the paths must begin with "/").  
			 If the  audit  system  is off  and the
			 AUD_SET is given, no action is performed; -1
			 is returned and errno is set to EALREADY. 

          AUD_GET        If the audit system is  on,  return  the
                         name of the "current" audit file via the
                         cpath parameter and the "next" audit file
			 via the bpath parameter. If the audit system  
			 is off, -1 is returned and errno is set to 
			 EALREADY.  If there is no available "next" 
			 file, -1 is returned and errno is set to 
			 ENOENT.

          AUD_SETNEXT    Use the file specified by the bpath
                         parameter  as the "next" audit file.  If
                         the "current" audit file fills up,  the
                         audit system automatically switches to the
                         "next" file. If there is an existing "next"
                         file,  the file specified via the bpath
                         will replace the existing "next" file.
                         If  there is not a "next" file, the file
                         specified via the bpath will be the
                         "next" file.  The bpath can also be NULL,
                         which means  that  there  is  no  "next"
                         file. This can happen when the privilege
                         user does not want a "next" file and  is
                         canceling   the  existing  "next"  file.
			 The "next" file, when specified, must be
			 fully qualified (begin with a "/").  If 
			 the audit system is off, -1 is returned 
			 and errno is set to EALREADY.  

          AUD_SETCURR    If  the  audit  system  is  on  and  the
                         AUD_SETCURR  is  given,  the "current" audit
                         file is switched to the specified  cpath.
			 A fully qualified cpath is required (the
			 cpath must begin with "/").  If the  audit  
			 system  is off  and the AUD_SETCURR is given, 
			 no action is performed; -1 is returned and 
			 errno is set to EALREADY. 

          AUD_ON         The caller must issue the AUD_ON command
                         and  with the required "current" file in
                         order to turn on the audit  system.   If
                         the  audit  system  is  off, a "current"
                         audit file  must  be  specified  in  the
                         cpath.   An  error  will  result  if  the
                         "current" file is  not  given  with  the
                         AUD_ON  command.  If the audit system is
                         already on, -1 is returned and errno is set
                         to EBUSY.  The "current" file specified 
			 must be fully qualified (begin with a "/").

          AUD_OFF        Turn off the audit system.  If the audit
                         system  is  on, it is turned off and the
                         "current" and  "next"  audit  files  are
                         closed.   If the audit system is already
                         off,  -1 is returned and errno is set to
                         EALREADY.  

          AUD_SWITCH     Switch the current audit file to be the
			 next audit file.  The next audit file
			 will be NULL.

     ERRORS
          Audctl will fail if one of the following conditions are true
          and errno will be set accordingly;

          [EPERM]       The user is not a privileged user, or the
                        the given file is not a regular file and
                        cannot be used.

          [EALREADY]    The cmd AUD_OFF, AUD_SET, AUD_SETNEXT, 
                        AUD_SETCURR or AUD_GET is specified when 
                        audit system is off.

          [EBUSY]       Trying to turn on auditing when it is  already
                        on.

	  [EFAULT]      Bad pointer. One or more of the function
			parameters were not accessable.  This error
			will also occur if the paths reference areas
			too small to hold the returned paths.

          [EINVAL]      The command specified is not a recognized command or
			the path strings specified are not absolute paths.

          [ENOENT]      The given file does not exist, or there is 
                        no available "next" file when the cmd is 
                        AUD_SWITCH.

          [ENAMETOOLONG] The file name specified is greater than 
			 MAXPATHLEN characters long.

          [ETXTBSY]     The given file is a text file  and  cannot  be
                        used.

     RETURN VALUE
          Upon successful completion, a value of 0 is returned.
          Otherwise a -1 is returned.

**************************************************************************/

#ifdef AUDIT
#include "../h/types.h"
#include "../h/buf.h"
#include "../h/param.h"
#include "../h/systm.h"
#include "../h/user.h"
#include "../h/file.h"
#include "../h/vnode.h"
#include "../h/vfs.h"
#include "../ufs/inode.h"
#include "../ufs/fs.h"
#include "../h/kernel.h"
#include "../h/audit.h"
#include "../h/uio.h"
#include "../h/proc.h"
#ifdef __hp9000s300
#define	STACK_300_OVERFLOW_FIX
#include "../h/malloc.h"
#endif	

#ifdef hp9000s800
/* For light-weight system calls macro definitions */
#include "../h/syscall.h"
#endif

#include "../dux/cct.h"
#include "../dux/dux_hooks.h"
#include "../dux/lookupops.h"
#include "../dux/dm.h"
extern site_t root_site, my_site;
int	audctl_want, audctl_lock;

#ifdef hp9000s800
/* light-weight system call disable flag */
extern int lw_syscall_off;
#endif

/*
 * LEADING_SLASH is used to determine whether char *s is a fully
 * qualified path name.
 */
#define	LEADING_SLASH(s)	(*(s) == '/')
static void save_paths();

int
audctl()
{
    int cmd,mode;
#ifndef	STACK_300_OVERFLOW_FIX
    char cname[MAXPATHLEN];
    char bname[MAXPATHLEN];
    char save_file[MAXPATHLEN];
#endif	/* STACK_300_OVERFLOW_FIX */

    char fubyte();

    register struct a {
        int     cmd;        	/* command          */
        char    *cname;     	/* current path     */
        char    *bname;     	/* next path        */
        int     mode;   	/* mode for file create */
    } *uap = (struct a *)u.u_ap;

#ifdef MP
		/* for MP systems keep auditing disabled */
    if (!uniprocessor) {
        u.u_error = EINVAL;
	return;
    }
#endif

    /* cmd and mode are short-hand for uap->cmd and uap->mode */
    cmd = uap->cmd;
    mode = uap->mode;

    /* save the pathname parameters */
    save_paths(uap->cname,uap->bname,cmd);

    /* if not priviledged user - error */
    if (!suser())
        return;


    /* if auditing is off - error except when turning auditing on
     * exit with EALREADY; if trying to turn auditing on when its
     * on already return EBUSY
     */
    if (audit_state_flag) {
        if (cmd==AUD_ON) {
            u.u_error = EBUSY;
            return;
        }
    } else {
	if (cmd!=AUD_ON) {
            u.u_error = EALREADY;
            return;
        }
    }


    switch (cmd) {

    case AUD_GET:

        {

            if (u.u_error =
	      copyout((caddr_t)curr_file, (caddr_t)uap->cname, (currlen + 1)))
              return;

            if (!next_vp) {
		subyte((caddr_t)uap->bname,0);
                u.u_error = ENOENT;
                return;
            }

            if (u.u_error =
	      copyout((caddr_t)next_file,(caddr_t)uap->bname,(nextlen + 1)))
              return;

	    return;

        }


    case AUD_ON:
    case AUD_SET:

        {
#ifdef	STACK_300_OVERFLOW_FIX
	    char *cname = (char *)vapor_malloc(MAXPATHLEN, M_TEMP, M_WAITOK);
#endif	/* STACK_300_OVERFLOW_FIX */

	    /* get the cname verifying its validity
             */
            if (get_audfile(cname,uap->cname))
	       return;

            /* if next_file specified is NULL */
            if (!uap->bname || !fubyte((caddr_t)uap->bname)) {

		/* note we will not check bname */
		/* if the bname is NULL */

                /* if there is a next file */
                if (next_vp) {
	    	    if (my_site != root_site)
			DUXCALL(AUDOFF)(AUD_SETNEXT,root_site);
	    	    else
			set_audit_off(AUD_SETNEXT);
		}

            } else {
            /* if next_file specified isn't NULL */
#ifdef	STACK_300_OVERFLOW_FIX
	        char *bname = (char *)vapor_malloc(MAXPATHLEN, M_TEMP, M_WAITOK);
#endif	/* STACK_300_OVERFLOW_FIX */

	        /* get the bname verifying its validity */
                if (get_audfile(bname,uap->bname))
		    return;

	    	if (my_site != root_site)
		    DUXCALL(AUDCTL)(bname,mode,AUD_SETNEXT,root_site);
	    	else
		    set_audit_file(bname,mode,AUD_SETNEXT);
		if (u.u_error)
		    return;
            }
	    if (my_site != root_site)
		DUXCALL(AUDCTL)(cname,mode,cmd,root_site);
	    else
		set_audit_file(cname,mode,cmd);


            /* set the proc table pid id write bit to zero */
            clear_idwrite();

            return;

        }


    case AUD_SETCURR:

        {
#ifdef	STACK_300_OVERFLOW_FIX
	    char *cname = (char *)vapor_malloc(MAXPATHLEN, M_TEMP, M_WAITOK);
#endif	/* STACK_300_OVERFLOW_FIX */

	    /* get the cname verifying its validity */
            if (get_audfile(cname,uap->cname))
		return;

	    if (my_site != root_site)
		DUXCALL(AUDCTL)(cname,mode,cmd,root_site);
	    else
		set_audit_file(cname,mode,cmd);


            /* set the proc table pid id write bit to zero */
            clear_idwrite();

	    return;

        }


    case AUD_SETNEXT:

        {

            /* if next_file specified is NULL */
#ifdef	STACK_300_OVERFLOW_FIX
	    char *bname = (char *)vapor_malloc(MAXPATHLEN, M_TEMP, M_WAITOK);
#endif	/* STACK_300_OVERFLOW_FIX */

            if (!uap->bname || !fubyte((caddr_t)uap->bname)) {

		/* note we will not check bname */
		/* if the bname is NULL */

                /* if there is not already a next file */
                if (!next_vp) 
                    return;
	    	if (my_site != root_site)
			DUXCALL(AUDOFF)(cmd,root_site);
	    	else
			set_audit_off(cmd);

              	return;

            }
 

	    /* if the next_file specified isn't NULL */

	    /* get the bname verifying its validity */
            if (get_audfile(bname,uap->bname))
	        return;

	    if (my_site != root_site)
		DUXCALL(AUDCTL)(bname,mode,cmd,root_site);
	    else
		set_audit_file(bname,mode,cmd);


            return;

        }


    case AUD_OFF:

        {

	    if (my_site != root_site)
		DUXCALL(AUDOFF)(cmd,root_site);
	    else
		set_audit_off(cmd);


            return;

        }


    case AUD_SWITCH:
	{
  	    /* back-up file available */
      	    if (next_vp)
	    { 
#ifdef	STACK_300_OVERFLOW_FIX
	        char *save_file = (char *)vapor_malloc(MAXPATHLEN, M_TEMP, M_WAITOK);
#endif	/* STACK_300_OVERFLOW_FIX */
         	/* switch to the back-up audit file */
        	strcpy (save_file, curr_file);
	 	if (my_site != root_site)
			DUXCALL(SWAUDFILE)(curr_file,next_file);
	 	else
         	aud_swtch(curr_file,next_file);
   		printf("The current audit file is switched from %s to %s.\n",
   			save_file, curr_file);
   		printf("Notify the security officer to specify a backup.\n");
         	return;

      	    } /* back-up NOT available */

   	    else 
	    {
      	    	/* exceeded user-specified limits & no backup file */
                u.u_error = ENOENT;
		return;
	    }
	}

        /*  The input cmd is illegal    */

    default:

        {
            u.u_error = EINVAL;
            return;
        }


    }

}

/* clib strlen() routine
 * Returns the number of
 * non-\0 bytes in string argument.
 */

static int
strlen(s)
register char *s;
{
	register char *s0 = s + 1;

	if (s == (char*)0)
	  return(0);

	while (*s++ != '\0');

	return (s - s0);
}


/* get_audfile() routine to copy the audit file name from
 * user to kernel space and verify its validity.
 */
static int
get_audfile(dest,source)
char *dest, *source;
{

    int status;
    unsigned int length;
    extern int copyinstr();

    /* get the cname verifying its accessability
     */
     if (status=copyinstr(source, dest, MAXPATHLEN, &length)) {
	 if (status==ENOENT)
	     u.u_error=ENAMETOOLONG;
         else
	     u.u_error=EFAULT;
         return -1;
     } 


    /* verify that input filename is fully qualified;
     * begins with '/'
     */
    if (!LEADING_SLASH(dest)) {
	u.u_error = EINVAL;
	return -1;
    }

     return 0;

}


set_audit_file(fnamep,mode,cmd)
char *fnamep;
int mode, cmd;
{
	struct  vnode *tmp_vp;	 /* temporary vnode pointer  */

        /* create the file if it doesn't exist */
	u.u_error =
	    vn_open(fnamep,UIOSEG_KERNEL,FWRITE|FCREAT|NONEXCL,mode,&tmp_vp);

	if (u.u_error)
	    return;


	/* Grab the cluster audctl lock and broadcast the audctl to the 
	 * cluster. When the broadcast has completed, release the cluster 
	 * audctl lock. This will assure a consistent state throughout the 
	 * cluster for the tables in event of "simultaneous" setevent calls.
	 */

	if ((my_site_status & CCT_CLUSTERED) &&
	    (my_site == root_site))
	{
		get_audctl_lock();
	}

	if (cmd == AUD_ON || cmd == AUD_SET || cmd == AUD_SETCURR)
	{
		/* load the kernel audit data structure with the 
		 * validated parameters after getting the write
		 * synchronization lock (you would like to assure 
		 * that writers have finished).
		 */

		get_lock();
		curr_vp=tmp_vp;
		currlen=strlen(fnamep);
		audit_mode = mode;
		strcpy(curr_file,fnamep);
		rel_lock();
	}
	else if (cmd == AUD_SETNEXT)
	{
		/* load the kernel audit data structure with the 
		* validated parameters
		*/
		next_vp=tmp_vp;
		nextlen=strlen(fnamep);
		audit_mode = mode;
		strcpy(next_file,fnamep);
	}


	/* if AUD_ON, set current state audit_state_flag
	 * and marker audit_ever_on
	 */
	if (cmd==AUD_ON) {
		audit_state_flag=1;
		audit_ever_on=1;

#ifdef hp9000s800
		/* disable light-weight system calls */
		aud_disable_lw_syscalls();
#endif
	}

	if ((my_site_status & CCT_CLUSTERED) &&
	    (my_site == root_site))
	{
		/* broadcast the audctl */;

		DUXCALL(AUDCTL)(fnamep,mode,cmd,DM_CLUSTERCAST);

		/* unlock the table */

		rel_audctl_lock();

		/* if we had trouble with the cluster,
		 * stop auditing and print a message on the root console
		 */
		if (u.u_error) {
			int tmp_save;
			tmp_save=u.u_error;
			printf("\nAuditing request to use audit log file %s\ncannot be performed by one or more clients.\nAuditing will be shutdown NOW!!!\n",
			fnamep);
			set_audit_off(AUD_OFF);
			u.u_error=tmp_save;
		}
	}
}

/* S E T _ A U D I T _ O F F
 *
 * Turn off auditing, for the whole cluster if necessary, and for both
 * audit files if the cmd is AUD_OFF, or just for the next file if
 * this came for an AUD_SETNEXT to NULL.
 */

set_audit_off(cmd)
{

	/* Grab the cluster audctl lock and broadcast the audctl to the 
	 * cluster. When the broadcast has completed, release the cluster 
	 * audctl lock. This will assure a consistent state throughout the 
	 * cluster for the tables in event of "simultaneous" setevent calls.
	 */

	if ((my_site_status & CCT_CLUSTERED) &&
	    (my_site == root_site))
	{
		get_audctl_lock();
	}

	/* reset audit flag and release current file's vnode */

	if (cmd == AUD_OFF)
	{
		get_lock();
		audit_state_flag = 0;

#ifdef hp9000s800
		/* enable light-weight system calls */
		aud_enable_lw_syscalls();
#endif

		curr_file[0] = '\0';
		currlen = 0;
		if (curr_vp) { /* curr_vp should never be zero */
			vn_close(curr_vp,FWRITE);
			VN_RELE(curr_vp);
			curr_vp = NULL;
		}
		rel_lock();
	}

	/* if there is designated next file, release vnode */

	if (next_vp) {
		next_file[0] = '\0';
		nextlen = 0;
		vn_close(next_vp,FWRITE);
		VN_RELE(next_vp);
		next_vp = NULL;
	}

	if ((my_site_status & CCT_CLUSTERED) &&
	    (my_site == root_site))
	{
		/* broadcast the audoff */;

		DUXCALL(AUDOFF)(cmd,DM_CLUSTERCAST);

		/* unlock the table */

		rel_audctl_lock();
	}
}

get_audctl_lock()
{
	while (audctl_lock) {
		audctl_want = 1;
		sleep((caddr_t)&curr_vp, PSWP+1);
	}
	audctl_lock = 1;
}

rel_audctl_lock()
{
	audctl_lock = 0;
	if (audctl_want)
	{
		audctl_want = 0;
		wakeup((caddr_t)&curr_vp);
	}
}

#ifdef hp9000s800
aud_disable_lw_syscalls()
{
	register int s;

	/* disable light-weight system calls */

	s = spl7();
	lw_syscall_off |= LW_SYSCALL_OFF_AUDIT;
	splx(s);
}

aud_enable_lw_syscalls()
{
   register int s;

   /* enable light-weight system calls */

   s = spl7();
   lw_syscall_off &= (~LW_SYSCALL_OFF_AUDIT);
   splx(s);
}
#endif

/* Save_paths saves the unvalidated audit file pathnames.
 * "Dummy" values are saved when it doesn't make sense to save the real
 * ones.
 */
static void
save_paths(cpath,bpath,cmd)
char *cpath, *bpath;
int cmd;
{
extern struct aud_type aud_syscall[];


    /* check to see if this information needs to be saved 
     * there's no reason to do anything if this routine is not being audited,
     * or if auditing is off.
     */
    if (!AUDITON() || aud_syscall[u.u_syscall].logit == NONE)
      return;

    switch (cmd) {

      case AUD_GET:
	  /* When the AUD_GET command is specified the paths are output 
	   * parameters.  Their contents upon entering audctl is 
	   * potentially garbage; so they are not saved.
           */
          save_pn_info(0);
          save_pn_info(0);
      break;

      case AUD_SETCURR:
          /* If the command is to set the current file, save the current file
           * path and a dummy next path as a placeholder.
           */
          save_pn_info(cpath);
          save_pn_info(0);
      break;

      case AUD_SETNEXT:
          /* If the command is to set the next file, save a dummy current file
           * path as a placeholder and the next file path.  If the next
	   * path specified is NULL substitute an empty string for it.
           */
          save_pn_info(0);
          save_pn_info(bpath);
      break;

      case AUD_SET:
          /* If the command is to set both files, or if the command is
           * to set both files, both paths are saved.
	   * If the next path specified is NULL substitute an empty string
	   * for it.
           */
          save_pn_info(cpath);
          save_pn_info(bpath);
      break;

      case AUD_OFF:
      case AUD_SWITCH:
      default:
          /* Save dummy path info if the command is:
           * 1. to shutdown auditing
           * 2. to switch audit files
           * 3. unrecognized
           * The paths provided are not used in these instances.
           */
          save_pn_info(0);
          save_pn_info(0);
    }
}

#endif AUDIT
