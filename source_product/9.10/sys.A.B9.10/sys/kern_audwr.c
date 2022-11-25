/* $Source: /source/hpux_source/kernel/sys.SWT68K_800/sys/RCS/kern_audwr.c,v $
 * $Revision: 1.11.83.3 $        $Author: kcs $
 * $State: Exp $        $Locker:  $
 * $Date: 93/09/17 20:05:12 $
 */
/* HPUX_ID: @(#)kern_aud_wr.c	55.1		88/12/23 */

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

#ifdef AUDIT
#include "../h/signal.h"
#include "../h/param.h"
#include "../h/types.h"
#include "../h/user.h"
#include "../h/proc.h"
#include "../h/vnode.h"
#include "../h/uio.h"
#include "../h/audit.h"
#include "../h/kernel.h"
#include "../h/errno.h"
#include "../h/tty.h"
#include "../ufs/fs.h"
#include "../ufs/inode.h"
#include "../h/file.h"
#include "../dux/cct.h"
#include "../dux/dux_hooks.h"
#include "../dux/lookupops.h"
#include "../dux/dm.h"
extern site_t root_site, my_site;

#define KBYTES 1024
#define PAUD    PZERO-1

extern   dev_t   cons_dev;
extern   dev_t   cons_mux_dev;
struct pir_header { /*  pir and  header */
   struct pir_t pir;
   char hdr[20];
};

static kaw_lock  = 0;
static msgcount1 = 0;
static msgcount2 = 0;

/****************************************************************
 *   kern_aud_wr.c   1.0    June 1988     Alex Choy
 *
 *   Exit Status:
 *      If the write was successful, return 0;
 *      else return -1 and u.u_error contains:
 *
 *      ENOSPC - no space to write the record and caller is su.
 ****************************************************************/

kern_aud_wr(args)
struct audit_arg *args;
{
   u_short logoff = 0;
   struct vattr va, va_set;
   uid_t saveuid;
   short i, get_attr_err = 0;
   struct pir_header  ph;
   short pir_stuffed = 0;
   short save_err = 0;
   short save_uerror = 0;
   char  save_file[MAXPATHLEN];

   save_uerror = u.u_error;
   get_lock();

start:
   check_pidwrite(&(args[0]), &ph, &pir_stuffed);

   /* Check the link count to see if the audit file
    * has been removed.
    */
   if (curr_vp == 0 || 
	(get_attr_err=VOP_GETATTR(curr_vp, &va, u.u_cred, VSYNC)) != 0 ||
	 va.va_nlink == 0)
   {
      if (get_attr_err || !curr_vp)
                printf("Cannot access current audit file(getattr1).\n");
       
      if (next_vp) {	/* Is there a next file? */
      	if (VOP_GETATTR(next_vp, &va, u.u_cred, VSYNC) != 0) {
               	printf("Cannot access next audit file(getattr2).\n");
		goto out;
   	}	
	msgcount1 = 0;
      }  /* next_vp */
      else {
             	if (msgcount1==0) {
      			printf("No next (backup) audit file.\n");
			msgcount1++;
		}
               	goto out;
      }
	
      if (va.va_nlink > 0) {   /* back-up file available */

         /* try to switch to the back-up audit file */
        strcpy (save_file, curr_file);
         rel_lock();
         if (my_site != root_site) 
		DUXCALL(SWAUDFILE)(curr_file, next_file);
         else
         	aud_swtch(curr_file,next_file);
         get_lock();
   	printf("The current audit file is switched from %s to %s.\n",
   		save_file, curr_file);
   	printf("Notify the security officer to specify a backup.\n");
	msgcount2 = 0;
        goto start;

      } else  {
             	if (msgcount2==0) {
      	 		printf("The audit files have been removed.\n");
			msgcount2++;
		}
               	goto out;
      }
   } /* (va.va_nlink == 0) */

   save_err = 0;
   for (i = 0; (i < MAX_AUD_ARGS) && (save_err == 0); i++)
      if (args[i].len > 0)
         save_err = vn_rdwr(UIO_WRITE, curr_vp, args[i].data, args[i].len, 
                     0, UIOSEG_KERNEL, IO_UNIT|IO_APPEND, (int *)0, 0);

   if (save_err) {
      /*
       * Since we could not finish the write completely, we will back off
       * and reset the file to the original size to preserve atomicity
       * of audit records.
       */
      vattr_null(&va_set); /* initialize for setting */
      va_set.va_size = va.va_size; /* set it to the original size */
      saveuid = u.u_uid; /* ensure permission to restore the file */
      u.u_uid = 0;
      if (VOP_SETATTR(curr_vp, &va_set, u.u_cred, 0) != 0) {
	u.u_uid = saveuid;
    	printf("Cannot access next audit file(setattr).\n");
	goto out;
      }	
      u.u_uid = saveuid;

      if (next_vp) {	/* Is there a next file? */
      	if (VOP_GETATTR(next_vp, &va, u.u_cred, VSYNC) != 0) {
      		printf("Cannot access next audit file(getattr3).\n");
		goto out;
   	}	

      	if (va.va_nlink > 0) {   /* back-up file available */
         	/* switch to the back-up audit file */
        	strcpy (save_file, curr_file);
        	rel_lock();
		if (my_site != root_site) 
			DUXCALL(SWAUDFILE)(curr_file, next_file);
		else
         		aud_swtch(curr_file,next_file);
        	get_lock();
   		printf("The current audit file is switched from %s to %s.\n",
   				save_file, curr_file);
   		printf("Notify the security officer to specify a backup.\n");
	
        	goto start;
	} /* (va.va_nlink > 0) */
      } /* next_vp */

      /* back-up NOT available */
      if (suser()) {
            rel_lock();
            u.u_error = ENOSPC;
            return (-1);
      } else {
            if (!on_console()) { /* not on the console */
               rel_lock();
               sleep((caddr_t)&lbolt, PAUD);
               get_lock();
               goto start;
            } else {   /* on console - should log off after trying another
                        * write as the super user.
                        */
               saveuid = u.u_uid;
               u.u_uid = 0;
               save_err = 0;
               for (i = 0; (i < MAX_AUD_ARGS) && (save_err == 0); i++)
                  if (args[i].len > 0)
                     save_err = vn_rdwr(UIO_WRITE, curr_vp, args[i].data,
                                 args[i].len, 0, UIOSEG_KERNEL,
                                 IO_UNIT|IO_APPEND, (int *)0, 0);
               u.u_uid = saveuid;
	       if (save_err) {
	       	    printf("Cannot audit, even as superuser - filesystem full.\n");
                    vattr_null(&va_set);
                    va_set.va_size = va.va_size;
                    saveuid = u.u_uid;
		    u.u_uid = 0;
                    if (VOP_SETATTR(curr_vp, &va_set, u.u_cred, 0) != 0) {
                        printf("Cannot access next audit file(setattr).\n");
                    }
		    u.u_uid = saveuid;
               }
               logoff++;
            }
      } /* if su ... else ... */
   }; /* if save_err */

out: rel_lock();

   if (logoff) {
      /* try to free up the console */
      free_console();
   }
   
   u.u_error = save_uerror;
   return(0);
}  /* end of kern_aud_wr */


/**********************************************************************
 * free_console sends a SIGHUP to the controlling process of the terminal,
 *              and disassociates the terminal from the controlling process.
 *              The behavior is similar to a modem disconnect.
 *********************************************************************/
free_console()
{
   register struct tty *tp = u.u_procp->p_ttyp;

   if ((tp != NULL) &&
       (tp->t_cproc !=NULL) &&
       (tp->t_cproc->p_uid != 0)) {
          psignal(tp->t_cproc,SIGHUP);
          tp->t_pgrp = 0;
          tp->t_cproc = NULL;
   }
} /* free_console */


/**********************************************************************
 * on_console returns 1 if it has
 *            a ttyd equal to cons_mux_dev or cons_dev; else it
 *            returns a 0.
 *********************************************************************/
on_console()
{
#ifdef hp9000s800
      if ( (u.u_procp->p_ttyd == cons_mux_dev) ||
           (u.u_procp->p_ttyd == cons_dev) ) {
#else
      if (u.u_procp->p_ttyd == cons_dev) {
#endif
         return(1);
      } else {
         return(0);
      }
}      


/*************************************************************
 * clear_idwrite turns off all the p_idwrite bits in each active
 * process's proc table slot. This is done at audit file switch
 * time to make sure a pid identification record is written for
 * each auditing process in the next audit file.
 *************************************************************/
clear_idwrite()
{
   short   phx;
   struct proc *p;

   /* follow the active list headed by proc[0] and turn
    * all the p_idwrite off.
    */

   phx = proc->p_fandx;
   while (phx != 0) {
      p = &proc[phx];
      p->p_idwrite = 0;
      phx = p->p_fandx;
   }
} /* clear_idwrite() */


/*************************************************************
 * aud_swtch switches to the backup file, which is assumed available
 * when this routine is called, and print out a message.
 *************************************************************/
aud_swtch(called_curr_file,called_next_file)
register char * called_curr_file;
register char * called_next_file;
{
   struct vnode *savevp;
   
   clear_idwrite();


   if ((my_site_status & CCT_CLUSTERED) && (my_site == root_site))
   {
		get_audctl_lock();

		/* Are the auditing files at the time this routine was
		 * called the same as the current auditing files? If they 
		 * are not then a switch or audctl has been performed 
		 * elsewhere and we do not need to do this switch
		 */

		if (strcmp(called_curr_file,curr_file) != 0 ||
		    strcmp(called_next_file,next_file) != 0 )
		{
			rel_audctl_lock();
			return;
		}
   }
   get_lock();

   savevp = curr_vp;
   curr_vp = next_vp;
   currlen = nextlen;
   strcpy (curr_file, next_file);
   next_vp = NULL;
   nextlen = 0;
   strcpy (next_file, "");
   if (savevp) {
	vn_close(savevp, FWRITE);
	VN_RELE(savevp);
   }

   rel_lock();
   if ((my_site_status & CCT_CLUSTERED) && (my_site == root_site))
   {
		/* broadcast the aud_swtch */;

		DUXCALL(CL_SWAUDFILE)();
		rel_audctl_lock();

		/* if the entire cluster doesn't switch properly
		 * print a message and shutdown auditing.
		 */
		if (u.u_error) {
			int tmp_save;
			tmp_save=u.u_error;
			printf("\nAuditing request to switch audit files\ncannot be performed by one or more clients.\nAuditing will be shutdown NOW!!!\n");
			set_audit_off(AUD_OFF);
			u.u_error=tmp_save;
		}
   }
} /* aud_swtch */


/**********************************************************************
 * check_pidwrite checks the p_idwrite of the calling process and, if
 *                the flag is 0, inserts a pid ident. record in front
 *                of the header.
 *********************************************************************/
check_pidwrite(header, php, pir_stuffed)
struct audit_arg *header;
struct pir_header *php;
short *pir_stuffed;
{
   struct proc *p;
   short i;

   p = u.u_procp;
   if (p->p_idwrite == 0) {
      if (*pir_stuffed == 0) {
         /* pir header */
         php->pir.aud_head.ah_pid = p->p_pid;
         php->pir.aud_head.ah_event = EN_PIDWRITE;
         php->pir.aud_head.ah_len = sizeof(struct pir_body);
         /* pir body */
         php->pir.pid_info.aid = u.u_aid;
         php->pir.pid_info.ruid = u.u_ruid;
         php->pir.pid_info.rgid = u.u_rgid;
         php->pir.pid_info.euid = u.u_uid;
         php->pir.pid_info.egid = u.u_gid;
         php->pir.pid_info.tty = p->p_ttyd;
         php->pir.pid_info.ppid = p->p_ppid;
         /* audit record header in args[0] */
         for (i=0; i < header->len; i++)
            php->hdr[i] = header->data[i];
         /* put this back into header */
         header->data = (char *)php;
         header->len = sizeof(struct pir_t) + sizeof(struct audit_hdr);
         /* a pir has been stuffed */
         (*pir_stuffed)++;
      }
      /* turn the flag on */
      p->p_idwrite = 1;
   } /* if p_idwrite == 0 */
} /* check_pidwrite */
      

/**********************************************************************
 * get_lock tries to acquire the kaw_lock. If the lock is not available,
 *          the process will sleep until someone else does a rel_lock which
 *          releases the lock and wakes up all processes sleeping on the
 *          lock.
 *********************************************************************/
get_lock()
{
   while (kaw_lock != 0)
      sleep ((caddr_t) &kaw_lock, PAUD);
   kaw_lock++;
} /* get_lock */


/**********************************************************************
 * rel_lock releases the kaw_lock and wakes up all processes sleeping on
 *          the lock.
 *********************************************************************/
rel_lock()
{
   kaw_lock--;
   wakeup ((caddr_t) &kaw_lock);
} /* rel_lock */
#endif AUDIT
