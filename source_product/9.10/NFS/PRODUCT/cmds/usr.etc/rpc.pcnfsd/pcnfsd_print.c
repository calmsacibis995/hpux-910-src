/* RE_SID: @(%)/tmp_mnt/vol/dosnfs/shades_SCCS/unix/pcnfsd/v2/src/SCCS/s.pcnfsd_print.c 1.11 92/11/04 12:06:43 SMI */
#ifndef lint
static  char rcsid[] = "@(#)pcnfsd_print.c:  $Revision: 1.1.109.4 $Date: 86/09/24 $";
#endif
/*
**=====================================================================
** Copyright (c) 1986-1992 by Sun Microsystems, Inc.
**	@(#)pcnfsd_print.c	1.11	11/4/92
**=====================================================================
*/
#include "common.h"
/*
**=====================================================================
**             I N C L U D E   F I L E   S E C T I O N                *
**                                                                    *
** If your port requires different include files, add a suitable      *
** #define in the customization section, and make the inclusion or    *
** exclusion of the files conditional on this.                        *
**=====================================================================
*/
#include "pcnfsd.h"
#include <malloc.h>
#include <stdio.h>
#include <pwd.h>
#include <ctype.h>
#include <sys/file.h>
#include <signal.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <netdb.h>
#include <errno.h>
#include <string.h>

#ifdef ISC_2_0
#include <sys/fcntl.h>
#include <sys/wait.h>
#endif ISC_2_0

#ifdef SHADOW_SUPPORT
#include <shadow.h>
#endif SHADOW_SUPPORT

/*
**---------------------------------------------------------------------
** Other #define's 
**---------------------------------------------------------------------
*/
#ifndef MAXPATHLEN
#define MAXPATHLEN 1024
#endif

#ifndef SPOOLDIR
#define SPOOLDIR        "/usr/spool/pcnfs" /* HPNFS */
#endif SPOOLDIR

/*
** The following defintions give the maximum time allowed for
** an external command to run (in seconds)
*/
#define MAXTIME_FOR_PRINT	10
#define MAXTIME_FOR_QUEUE	10
#define MAXTIME_FOR_CANCEL	10
#define MAXTIME_FOR_STATUS	10

#define QMAX 50

/*
** The following is derived from ucb/lpd/displayq.c
*/
#define SIZECOL 62
#define FILECOL 24

extern void     scramble();
extern void     *grab();
extern int      suspicious();
#ifndef hpux
extern void     run_ps630();
#endif
extern char    *crypt();
extern FILE    *su_popen();
extern int      su_pclose();
extern char    *my_strdup();
int             build_pr_list();
void		mon_printers();
char 	       *map_printer_name();
char	       *expand_alias();
void           *grab();
void            free_pr_list_item();
void            free_pr_queue_item();
pr_list		list_virtual_printers();

/*
**---------------------------------------------------------------------
**                       Misc. variable definitions
**---------------------------------------------------------------------
*/

extern int      errno;
extern int	interrupted;	/* in pcnfsd_misc.c */
struct stat     statbuf;
char            pathname[MAXPATHLEN];
char            new_pathname[MAXPATHLEN];
char            sp_name[MAXPATHLEN] = SPOOLDIR;
char            tempstr[256];
char            delims[] = " \t\r\n:()";

pr_list         printers = NULL;
pr_queue        queue = NULL;



/*
**=====================================================================
**                      C O D E   S E C T I O N                       *
**=====================================================================
**
**=====================================================================
**                    C A V E A T   L E C T O R  ! ! ! !              *
**                                                                    *
** The following code relies upon the definition or non-definition of *
** a set of manifest constants which control whether BSD or SVR4      *
** rules should be adopted. It's not as simple as it might be,        *
** because certain OS's may support a mixture of facilities (or in    *
** some cases a mechanism may be broken).                             *
** This mechanism also allows for the addition of new styles.         *
**                                                                    *
** Within each line, only one symbol should be defined.               *
**                                                                    *
**  SVR4_STYLE_PRINT     BSD_STYLE_PRINT                              *
**  SVR4_STYLE_PR_LIST   BSD_STYLE_PR_LIST                            *
**  SVR4_STYLE_QUEUE     BSD_STYLE_QUEUE                              *
**  SVR4_STYLE_CANCEL    BSD_STYLE_CANCEL                             *
**  SVR4_STYLE_STATUS    BSD_STYLE_STATUS                             *
**  SVR4_STYLE_MONITOR   BSD_STYLE_MONITOR                            *
**                                                                    *
** The actual per-OS definitions are given in common.h, and are       *
** based on the definition of OSVER_xxx in the Makefile             *
**=====================================================================
*/
#ifdef SUNOS_403C

/*
 * This is strstr() as in SysV and later SunOS's
 */
char *strstr(target, pattern)
char *target;
char *pattern;
{
	int n;
	char *cp;
	if(pattern == NULL || target == NULL)
		return NULL;
	n = strlen(pattern);
	for(cp = target; cp++; *cp) {
		if(strlen(cp) < n)
			break;
		if(strncmp(cp, pattern, n) == 0)
			return cp;
	}
	return NULL;
}
	
#endif SUNOS_403C



/*
** valid_pr
**
** true if a printer name is valid
*/

int
valid_pr(pr)
char *pr;
{
char *p;
pr_list curr;

	mon_printers();

	if(printers == NULL)
		build_pr_list();

	if(printers == NULL)
		return(0); /* can't tell - assume it's bad */

	p = map_printer_name(pr);
	if (p == NULL)
		return(1);	/* must be ok if maps to NULL! */
	curr = printers;
	while(curr) {
		if(!strcmp(p, curr->pn))
			return(1);
		curr = curr->pr_next;
	}
		
	return(0);
}


/*
** pr_init
**
** create a spool directory for a client if necessary and return the info
*/
pirstat
pr_init(sys, pr, sp)
char *sys;
char *pr;
char**sp;
{
int    dir_mode = 0777;
int rc;

	*sp = &pathname[0];
	pathname[0] = '\0';

	if(suspicious(sys) || suspicious(pr))
		return(PI_RES_FAIL);

	/* get pathname of current directory and return to client */

	(void)sprintf(pathname,"%s/%s",sp_name, sys);
	(void)mkdir(sp_name, dir_mode);	/* ignore the return code */
	(void)chmod(sp_name, dir_mode);
	rc = mkdir(pathname, dir_mode);	/* DON'T ignore this return code */
	if((rc < 0 && errno != EEXIST) ||
	   (chmod(pathname, dir_mode) != 0) ||
	   (stat(pathname, &statbuf) != 0) ||
	   !(statbuf.st_mode & S_IFDIR)) {
	   (void)sprintf(tempstr,
		         "rpc.pcnfsd: unable to set up spool directory %s\n",
		 	  pathname);
            msg_out(tempstr);
	    pathname[0] = '\0';	/* null to tell client bad vibes */
	    return(PI_RES_FAIL);
	    }
 	if (!valid_pr(pr)) 
           {
	    pathname[0] = '\0';	/* null to tell client bad vibes */
	    return(PI_RES_NO_SUCH_PRINTER);
	    } 
	return(PI_RES_OK);
}



/*
** pr_start2
**
** start a print job
*/
psrstat
pr_start2(system, pr, user, fname, opts, id)
char *system;
char *pr;
char *user;
char *fname;
char *opts;
char **id;
{
char            snum[20];
static char     req_id[256];
char            cmdbuf[256];
char            resbuf[256];
FILE *fd;
int i;
char *xcmd;
char *cp;
int failed = 0;

	*id = &req_id[0];
	req_id[0] = '\0';


	if(suspicious(system) || 
		suspicious(pr) ||
		suspicious(user) ||
		suspicious(fname))
		return(PS_RES_FAIL);

	(void)sprintf(pathname,"%s/%s/%s",sp_name,
	                         system,
	                         fname);	


	if (stat(pathname, &statbuf)) 
           {
	   /*
           **-----------------------------------------------------------------
	   ** We can't stat the file. Let's try appending '.spl' and
	   ** see if it's already in progress.
           **-----------------------------------------------------------------
	   */

	   (void)strcat(pathname, ".spl");
	   if (stat(pathname, &statbuf)) 
	      {
	      /*
              **----------------------------------------------------------------
	      ** It really doesn't exist.
              **----------------------------------------------------------------
	      */


	      return(PS_RES_NO_FILE);
	      }
	      /*
              **-------------------------------------------------------------
	      ** It is already on the way.
              **-------------------------------------------------------------
	      */


		return(PS_RES_ALREADY);
	     }

	if (statbuf.st_size == 0) 
	   {
	   /*
           **-------------------------------------------------------------
	   ** Null file - don't print it, just kill it.
           **-------------------------------------------------------------
	   */
	   (void)unlink(pathname);

	    return(PS_RES_NULL);
	    }
	 /*
         **-------------------------------------------------------------
	 ** The file is real, has some data, and is not already going out.
	 ** We rename it by appending '.spl' and exec "lpr" to do the
	 ** actual work.
         **-------------------------------------------------------------
	 */
	(void)strcpy(new_pathname, pathname);
	(void)strcat(new_pathname, ".spl");

	/*
        **-------------------------------------------------------------
	** See if the new filename exists so as not to overwrite it.
        **-------------------------------------------------------------
	*/


	if (!stat(new_pathname, &statbuf)) 
	   {
	   (void)strcpy(new_pathname, pathname);  /* rebuild a new name */
	   (void)sprintf(snum, "%d", rand());	  /* get some number */
	   (void)strncat(new_pathname, snum, 3);
	   (void)strcat(new_pathname, ".spl");	  /* new spool file */
	    }
	if (rename(pathname, new_pathname)) 
	   {
	   /*
           **---------------------------------------------------------------
	   ** Should never happen.
           **---------------------------------------------------------------
           */
	   (void)sprintf(tempstr, "rpc.pcnfsd: spool file rename (%s->%s) failed.\n",
			pathname, new_pathname);
                msg_out(tempstr);
		return(PS_RES_FAIL);
	    }

/* HPNFS */
/* hpux doesn't have anything to support the ps630 printer */
/* It doesn't seem readily available on Solaris 2.0 either */
#ifndef hpux
		if (*opts == 'd') 
	           {
		   /*
                   **------------------------------------------------------
		   ** This is a Diablo print stream. Apply the ps630
		   ** filter with the appropriate arguments.
                   **------------------------------------------------------
		   */
		   (void)run_ps630(new_pathname, opts);
		   }
#endif
		/*
		** Try to match to an aliased printer
		*/
		xcmd = expand_alias(pr, new_pathname, user, system);
	        /*
                **----------------------------------------------------------
	        **  Use the copy option so we can remove the orignal spooled
	        **  nfs file from the spool directory.
                **----------------------------------------------------------
	        */
		if(!xcmd) {
#ifdef BSD_STYLE_PRINT
			sprintf(cmdbuf, "/usr/ucb/lpr -P%s %s",
				pr, new_pathname);
#endif BSD_STYLE_PRINT
#ifdef SVR4_STYLE_PRINT
			sprintf(cmdbuf, "/usr/bin/lp -c -d%s %s",
				pr, new_pathname);
#endif SVR4_STYLE_PRINT
			xcmd = cmdbuf;
		}
		if ((fd = su_popen(user, xcmd, MAXTIME_FOR_PRINT)) == NULL) {
			msg_out("rpc.pcnfsd: su_popen failed");
			return(PS_RES_FAIL);
		}
		req_id[0] = '\0';	/* asume failure */
		while(fgets(resbuf, 255, fd) != NULL) {
			i = strlen(resbuf);
			if(i)
				resbuf[i-1] = '\0'; /* trim NL */
			if(!strncmp(resbuf, "request id is ", 14))
				/* New - just the first word is needed */
				strcpy(req_id, strtok(&resbuf[14], delims));
			else if (strembedded("disabled", resbuf))
				failed = 1;
		}
		if(su_pclose(fd) == 255)
			msg_out("rpc.pcnfsd: su_pclose alert");
		(void)unlink(new_pathname);
                if (req_id[0]=='\0') failed ++; /* HPNFS - failure */
		return((failed | interrupted)? PS_RES_FAIL : PS_RES_OK);
}

/*
** Decide whether to blow away the printers list based
** on a change to a specified file or directory.
**
*/
void
mon_printers()
{
	static struct stat buf;
	static time_t	old_mtime = (time_t)0;
#ifdef SVR4_STYLE_MONITOR
	char *mon_path = "/etc/lp/printers";
#endif
#ifdef BSD_STYLE_MONITOR
	char *mon_path = "/etc/printcap";
#endif
#ifdef HPUX_STYLE_MONITOR
	char *mon_path = "/usr/spool/lp/pstatus";
#endif
	if(stat(mon_path, &buf) == -1) {
		free_pr_list_item(printers); /* HPNFS */
		return;
	}
	if(old_mtime != (time_t)0) {
		if(old_mtime != buf.st_mtime && printers) {
			free_pr_list_item(printers);
			printers = NULL;
		}
	}
	old_mtime = buf.st_mtime;
}


/*
** build_pr_list
**
** build a list of valid printers
*/
#ifdef SVR4_STYLE_PR_LIST

int
build_pr_list()
{
pr_list last = NULL;
pr_list curr = NULL;
char buff[256];
FILE *p;
char *cp;
int saw_system;
int saw_remote; /* HPNFS */
char tmpbuff[256]; /* HPNFS */
int i; /* HPNFS */

/*
 * In SVR4 the command to determine which printers are
 * valid is lpstat -v. The output is something like this:
 *
 * device for lp: /dev/lp0
 * system for pcdslw: hinode
 * system for bletch: hinode (as printer hisname)
 *
 * On SunOS using the SysV compatibility package, the output
 * is more like:
 *
 * device for lp is /dev/lp0
 * device for pcdslw is the remote printer pcdslw on hinode
 * device for bletch is the remote printer hisname on hinode
 *
 * It is fairly simple to create logic that will handle either
 * possibility:
 */
/* HPNFS
 * HP-UX looks like this for a remote printer
 *  device for pj: /dev/null
 *    remote to: pj on hpindsh
 */

	p = popen("lpstat -v", "r");
	if(p == NULL) {
		printers = list_virtual_printers();
		return(1);
	}
	
	while(fgets(buff, 255, p) != NULL) {
		cp = strtok(buff, delims);
		if(!cp)
			continue;
		if(!strcmp(cp, "device")) {
			cp = strtok(NULL, delims);
			if (!strcmp(cp, "for"))
			    saw_system = saw_remote = 0; /* HPNFS */
			else continue;
		}
		else if (!strcmp(cp, "system")) {
			cp = strtok(NULL, delims);
			if (!strcmp(cp, "for"))
			    saw_system = 1;
			else continue;
		}
#ifdef hpux
		else
		     if (!strcmp(cp, "remote")) {
			cp = strtok(NULL, delims);
			if (!strcmp(cp, "to"))
			    saw_remote = 1;
			else continue;
		}
#endif
		else
			continue;

		cp = strtok(NULL, delims);
		if (!cp)	/* HPNFS */
			continue;

		/* HPNFS */
		/* if saw_remote is true then "curr" is already */
                /* partially filled */
		if (saw_remote != 1) {
			curr = (struct pr_list_item *)
				grab(sizeof (struct pr_list_item));

			curr->pn = my_strdup(cp);
			curr->device = NULL;
			curr->remhost = NULL;
			curr->cm = my_strdup("-");
			curr->pr_next = NULL;
			cp = strtok(NULL, delims);

			if(cp && !strcmp(cp, "is")) 
				cp = strtok(NULL, delims);

			if(!cp) {
				free_pr_list_item(curr);
				continue;
			}
	        }

		if(saw_system) {
			/* "system" OR "system (as printer pname)" */ 
			curr->remhost = my_strdup(cp);
			cp = strtok(NULL, delims);
			if(!cp) {
				/* simple format */
				curr->device = my_strdup(curr->pn);
			} else {
				/* "sys (as printer pname)" */
				if (strcmp(cp, "as")) {
					free_pr_list_item(curr);
					continue;
				}
				cp = strtok(NULL, delims);
				if (!cp || strcmp(cp, "printer")) {
					free_pr_list_item(curr);
					continue;
				}
				cp = strtok(NULL, delims);
				if(!cp) {
					free_pr_list_item(curr);
					continue;
				}
				curr->device = my_strdup(cp);
			}
		}
/* HPNFS this section of code deals with the the HP-UX 9.00 
** lpstat output. If lpstat output changes, so must this code
*/
#ifdef hpux 
		else if (saw_remote) {
		  /* "remote to: <rem printer> on <remhost>"  */
                  /* cp should be pointing to "<rem printer>" */
	          curr->device = my_strdup(cp);

                  /* skip the "on" */
	          cp = strtok(NULL,delims);
                  if (!cp || strcmp(cp, "on")) {
		     free_pr_list_item(curr);
                     continue;
	          }
 
                  /* next field is remotehost*/
	          cp = strtok(NULL,delims);
                  if (!cp) {
		     free_pr_list_item(curr);
                     continue;
	          }
                  curr->remhost = my_strdup(cp);

		  /* HPNFS */
		  /* set comment field to host:printer. Nicer for */
		  /* windows "Browse Printers" command            */
		  sprintf (tmpbuff, "%s:%s", 
			   curr->remhost, curr->device);
		  if (curr->cm) 
			  free (curr->cm);
		  curr->cm = my_strdup (tmpbuff);
		}
#endif
		else if(!strcmp(cp, "the")) {
			/* start of "the remote printer foo on bar" */
			cp = strtok(NULL, delims);
			if(!cp || strcmp(cp, "remote")) {
				free_pr_list_item(curr);
				continue;
			}
			cp = strtok(NULL, delims);
			if(!cp || strcmp(cp, "printer")) {
				free_pr_list_item(curr);
				continue;
			}
			cp = strtok(NULL, delims);
			if(!cp) {
				free_pr_list_item(curr);
				continue;
			}
			curr->device = my_strdup(cp);
			cp = strtok(NULL, delims);
			if(!cp || strcmp(cp, "on")) {
				free_pr_list_item(curr);
				continue;
			}
			cp = strtok(NULL, delims);
			if(!cp) {
				free_pr_list_item(curr);
				continue;
			}
			curr->remhost = my_strdup(cp);
		} else {
			/* the local name */
			curr->device = my_strdup(cp);
			curr->remhost = my_strdup("");
		}

		if(last == NULL)
			printers = curr;
		else
			last->pr_next = curr;
		last = curr;

	}
	(void) pclose(p);
/*
** Now add on the virtual printers, if any
*/
	if(last == NULL)
		printers = list_virtual_printers();
	else
		last->pr_next = list_virtual_printers();

	return(1);
}

#endif SVR4_STYLE_PR_LIST

#ifdef BSD_STYLE_PR_LIST

int
build_pr_list()
{
pr_list last = NULL;
pr_list curr = NULL;
char buff[256];
FILE *p;
char *cp;
int saw_system;

	p = popen("/usr/etc/lpc status", "r");
	if(p == NULL) {
		printers = list_virtual_printers();
		return(1);
	}
	
	while(fgets(buff, 255, p) != NULL) {
/*
 * This logic presumes that it's sufficient to find lines with
 * a non-space first character. We should actually check for
 * lines that begin "name:" - later
 */
		if(isspace(buff[0]))
			continue;
		cp = strtok(buff, delims);
		if(!cp)
			continue;
		curr = (struct pr_list_item *)
			grab(sizeof (struct pr_list_item));

		curr->pn = my_strdup(cp);
		curr->device = NULL;
		curr->remhost = NULL;
		curr->cm = my_strdup("-");
		curr->pr_next = NULL;
		curr->device = my_strdup(cp);  /* HPNFS */
		curr->remhost = my_strdup(""); /* HPNFS */
		if(last == NULL)
			printers = curr;
		else
			last->pr_next = curr;
		last = curr;

	}
	(void) pclose(p);
/*
** Now add on the virtual printers, if any
*/
	if(last == NULL)
		printers = list_virtual_printers();
	else
		last->pr_next = list_virtual_printers();

	return(1);
}
#endif BSD_STYLE_PR_LIST



/*
** free_pr_list_item
**
** free a list of printers recursively, item by item
*/
void
free_pr_list_item(curr)
pr_list curr;
{
	if(curr->pn)
		free(curr->pn);
	if(curr->device)
		free(curr->device);
	if(curr->remhost)
		free(curr->remhost);
	if(curr->cm)
		free(curr->cm);
	if(curr->pr_next)
		free_pr_list_item(curr->pr_next); /* recurse */
	free(curr);
}

/*
** build_pr_queue
**
** generate a printer queue listing
*/

#ifdef SVR4_STYLE_QUEUE

/*
** Print queue handling.
**
** Note that the first thing we do is to discard any
** existing queue.
*/

pirstat
build_pr_queue(pn, user, just_mine, p_qlen, p_qshown)
printername     pn;
username        user;
int            just_mine;
int            *p_qlen;
int            *p_qshown;
{
pr_queue last = NULL;
pr_queue curr = NULL;
char buff[256];
FILE *p;
char *owner;
char *job;
char *totsize;
char *cp; /* HPNFS */
int  i;   /* HPNFS */
char *name;         /* HPNFS */
char hostname[255]; /* HPNFS */

static char standard_input[] = "standard input"; /* HPNFS */

	if(queue) {
		free_pr_queue_item(queue);
		queue = NULL;
	}
	*p_qlen = 0;
	*p_qshown = 0;

        gethostname (hostname, 255);

	pn = map_printer_name(pn);
	if(pn == NULL || !valid_pr(pn) || suspicious(pn))
		return(PI_RES_NO_SUCH_PRINTER);


/*
** In SVR4 the command to list the print jobs for printer
** lp is "lpstat lp" (or, equivalently, "lpstat -p lp").
** The output looks like this:
** 
** lp-2                    root               939   Jul 10 21:56
** lp-5                    geoff               15   Jul 12 23:23
** lp-6                    geoff               15   Jul 12 23:23
** 
** If the first job is actually printing the first line
** is modified, as follows:
**
** lp-2                    root               939   Jul 10 21:56 on lp
** 
** I don't yet have any info on what it looks like if the printer
** is remote and we're spooling over the net. However for
** the purposes of rpc.pcnfsd we can simply say that field 1 is the
** job ID, field 2 is the submitter, and field 3 is the size.
** We can check for the presence of the string " on " in the
** first record to determine if we should count it as rank 0 or rank 1,
** but it won't hurt if we get it wrong.
**/

/* HPNFS
** HP-UX lpstat looks like this:
** pj-20               root           priority 0  Sep 23 16:58
**      nfs8992.spl                                562 bytes
** The file name could also be "(standard input)"
**/
#ifdef hpux
        /* tjs 1/94 strip off lp from lp-job to fix PC window */
        /* manager print problem SR #5003-154559              */
        sprintf(buff, "/usr/bin/lpstat %s |/bin/sed -e 's/%s-//'", pn, pn);
#else
	sprintf(buff, "/usr/bin/lpstat %s", pn);
#endif
	p = su_popen(user, buff, MAXTIME_FOR_QUEUE);
	if(p == NULL) {
		msg_out("rpc.pcnfsd: unable to popen() lpstat queue query");
		return(PI_RES_FAIL);
	}
	
	while(fgets(buff, 255, p) != NULL) {

#ifdef hpux
		/* HPNFS check for extraneous lines and skip */
		cp = strtok (buff, delims);
		if (!cp)
			continue;
		if (!strcmp(cp, "printer")) /*"printer queue for xxx"*/
			continue;
		if (!strcmp(cp, "no")) /* "no entries" */
			continue;

		/* HPNFS */
		/* check for "systemname: Warning: ...." */
		/* check for "systemname: printername: ..."*/
		if (!strcmp(cp, hostname))
			continue;


		job = my_strdup(cp);
#else
		job = strtok(NULL, delims);

#endif /* hpux */

		owner = strtok(NULL, delims);

		/* HPNFS */
		/* check for "rem_system: Warning: ..." */
		if (!owner || !strcmp(owner,"Warning")) 
			continue;
		/* HPNFS else save a copy of owner */
		else owner = my_strdup (owner); /* HPNFS */

#ifdef hpux
		/* HPNFS */
		cp = strtok(NULL, delims);
		if (!strcmp(cp, "priority")) {

		    /* we don't need any other info on this line*/
		    if (!(fgets(buff, 255, p) != NULL))
		        continue; /* fgets fails */
		    
		    /* the next field is the name */
		    name = strtok(buff, delims);
		    if (!name)
			continue;
	    
		    /* check for "(standard input)" */
		    /* we need to skip the "input)" also */
		    if (!strcmp(name,"standard") ) {

			/* skip over input str */
		        cp = strtok(NULL, delims);
			if (!cp)
			    continue;
		
			/* make sure it is really "(standard input") */
			if (!strcmp(cp,"input")) {
  			   /* assign name to static str */
			   name = standard_input;
  		           /* the next field is the size */
		           totsize = strtok(NULL, delims);
		        }
	            }         
	            else /* the next field is really totsize */
			   totsize = strtok(NULL, delims);
	        }
		else /* then cp != priority */
		    totsize = cp;
#else
		totsize = strtok(NULL, delims);
#endif /* hpux */

		if(!totsize)
			continue;

		*p_qlen += 1;

		if(*p_qshown > QMAX)
			continue;

		if(just_mine && mystrcasecmp(owner, user))
			continue;

		*p_qshown += 1;

		curr = (struct pr_queue_item *)
			grab(sizeof (struct pr_queue_item));

		curr->position = *p_qlen;
#ifdef hpux
		curr->id = job; /* HPNFS */
#else
		curr->id = my_strdup(job);
#endif

		curr->size = my_strdup(totsize);
		curr->status = my_strdup("");
		curr->system = my_strdup("");
#ifdef hpux
		curr->user = owner; /* HPNFS */
#else
		curr->user = my_strdup(owner);
#endif
		curr->file = my_strdup(name); /* HPNFS */
#ifdef hpux
		curr->cm = my_strdup(name); /* HPNFS */
#else
		curr->cm = my_strdup("-");
#endif
		curr->pr_next = NULL;

		if(last == NULL)
			queue = curr;
		else
			last->pr_next = curr;
		last = curr;

	}
	(void) su_pclose(p);
	return(PI_RES_OK);
}

#endif SVR4_STYLE_QUEUE

#ifdef BSD_STYLE_QUEUE
/*
 * BSD style queue listing
 */

pirstat
build_pr_queue(pn, user, just_mine, p_qlen, p_qshown)
printername     pn;
username        user;
int            just_mine;
int            *p_qlen;
int            *p_qshown;
{
pr_queue last = NULL;
pr_queue curr = NULL;
char buff[256];
FILE *p;
char *cp;
int i;
char *rank;
char *owner;
char *job;
char *files;
char *totsize;

	if(queue) {
		free_pr_queue_item(queue);
		queue = NULL;
	}
	*p_qlen = 0;
	*p_qshown = 0;
	pn = map_printer_name(pn);
	if(pn == NULL || suspicious(pn))
		return(PI_RES_NO_SUCH_PRINTER);

	sprintf(buff, "/usr/ucb/lpq -P%s", pn);

	p = su_popen(user, buff, MAXTIME_FOR_QUEUE);
	if(p == NULL) {
		msg_out("rpc.pcnfsd: unable to popen() lpq");
		return(PI_RES_FAIL);
	}
	
	while(fgets(buff, 255, p) != NULL) {
		i = strlen(buff) - 1;
		buff[i] = '\0';		/* zap trailing NL */
		if(i < SIZECOL)
			continue;
		if(!mystrncasecmp(buff, "rank", 4))
			continue;

		if(!mystrncasecmp(buff, "warning", 7))
			continue;

		totsize = &buff[SIZECOL-1];
		files = &buff[FILECOL-1];
		cp = totsize;
		cp--;
		while(cp > files && isspace(*cp))
			*cp-- = '\0';

		buff[FILECOL-2] = '\0';

		cp = strtok(buff, delims);
		if(!cp)
			continue;
		rank = cp;

		cp = strtok(NULL, delims);
		if(!cp)
			continue;
		owner = cp;

		cp = strtok(NULL, delims);
		if(!cp)
			continue;
		job = cp;

		*p_qlen += 1;

		if(*p_qshown > QMAX)
			continue;

		if(just_mine && mystrcasecmp(owner, user))
			continue;

		*p_qshown += 1;

		curr = (struct pr_queue_item *)
			grab(sizeof (struct pr_queue_item));

		curr->position = atoi(rank); /* active -> 0 */
		curr->id = my_strdup(job);
		curr->size = my_strdup(totsize);
		curr->status = my_strdup(rank);
		curr->system = my_strdup("");
		curr->user = my_strdup(owner);
		curr->file = my_strdup(files);
#ifdef hpux
		curr->cm = my_strdup(name); /* HPNFS */
#else
		curr->cm = my_strdup("-");
#endif
		curr->pr_next = NULL;

		if(last == NULL)
			queue = curr;
		else
			last->pr_next = curr;
		last = curr;

	}
	(void) su_pclose(p);
	return(PI_RES_OK);
}
#endif BSD_STYLE_QUEUE


/*
** free_pr_queue_item
**
** recursively free a pr_queue, item by item
*/
void
free_pr_queue_item(curr)
pr_queue curr;
{
	if(curr->id)
		free(curr->id);
	if(curr->size)
		free(curr->size);
	if(curr->status)
		free(curr->status);
	if(curr->system)
		free(curr->system);
	if(curr->user)
		free(curr->user);
	if(curr->file)
		free(curr->file);
	if(curr->cm)
		free(curr->cm);
	if(curr->pr_next)
		free_pr_queue_item(curr->pr_next); /* recurse */
	free(curr);
}




/*
** get_pr_status
**
** generate printer status report
*/
#ifdef SVR4_STYLE_STATUS

pirstat
get_pr_status(pn, avail, printing, qlen, needs_operator, status)
printername   pn;
bool_t       *avail;
bool_t       *printing;
int          *qlen;
bool_t       *needs_operator;
char         *status;
{
char buff[256];
char cmd[64];
FILE *p;
int n;
pirstat stat = PI_RES_NO_SUCH_PRINTER;

/*
** New - SVR4 printer status handling.
**
** The command we'll use for checking the status of printer "lp"
** is "lpstat -a lp -p lp". Here are some sample outputs:
**
** 
** lp accepting requests since Wed Jul 10 21:49:25 EDT 1991
** printer lp disabled since Thu Feb 21 22:52:36 EST 1991. available.
** 	new printer
** ---
** pcdslw not accepting requests since Fri Jul 12 22:30:00 EDT 1991 -
** 	unknown reason
** printer pcdslw disabled since Fri Jul 12 22:15:37 EDT 1991. available.
** 	new printer
** ---
** lp accepting requests since Wed Jul 10 21:49:25 EDT 1991
** printer lp now printing lp-2. enabled since Sat Jul 13 12:02:17 EDT 1991. available.
** ---
** lp accepting requests since Wed Jul 10 21:49:25 EDT 1991
** printer lp now printing lp-2. enabled since Sat Jul 13 12:02:17 EDT 1991. available.
** ---
** lp accepting requests since Wed Jul 10 21:49:25 EDT 1991
** printer lp disabled since Sat Jul 13 12:05:20 EDT 1991. available.
** 	unknown reason
** ---
** pcdslw not accepting requests since Fri Jul 12 22:30:00 EDT 1991 -
** 	unknown reason
** printer pcdslw is idle. enabled since Sat Jul 13 12:05:28 EDT 1991. available.
**
** Note that these are actual outputs. The format (which is totally
** different from the lpstat in SunOS) seems to break down as
** follows:
** (1) The first line has the form "printername [not] accepting requests,,,"
**    This is trivial to decode.
** (2) The second line has several forms, all beginning "printer printername":
** (2.1) "... disabled"
** (2.2) "... is idle"
** (2.3) "... now printing jobid"
** The "available" comment seems to be meaningless. The next line
** is the "reason" code which the operator can supply when issuing
** a "disable" or "reject" command.
** Note that there is no way to check the number of entries in the
** queue except to ask for the queue and count them.
*/
	/* assume the worst */
	*avail = FALSE;
	*printing = FALSE;
	*needs_operator = FALSE;
	*qlen = 0;
	*status = '\0';

	pn = map_printer_name(pn);
	if(pn == NULL || !valid_pr(pn) || suspicious(pn))
		return(PI_RES_NO_SUCH_PRINTER);
	n = strlen(pn);

#ifdef hpux
	sprintf(cmd, "/usr/bin/lpstat -a%s -p%s", pn, pn);
#else
	sprintf(cmd, "/usr/bin/lpstat -a %s -p %s", pn, pn);
#endif

	p = popen(cmd, "r");
	if(p == NULL) {
		msg_out("rpc.pcnfsd: unable to popen() lp status");
		return(PI_RES_FAIL);
	}
	
	stat = PI_RES_OK;

	while(fgets(buff, 255, p) != NULL) {
		if(!strncmp(buff, pn, n)) {
			if(!strstr(buff, "not accepting"))
			*avail = TRUE;
			continue;
		}
		if(!strncmp(buff, "printer ", 8)) {
			if(!strstr(buff, "disabled"))
				*printing = TRUE;
			if(strstr(buff, "printing"))
				strcpy(status, "printing");
			else if (strstr(buff, "idle"))
				strcpy(status, "idle");
			continue;
		}
		if(!strncmp(buff, "UX:", 3)) {
			stat = PI_RES_NO_SUCH_PRINTER;
		}
	}
	(void) pclose(p);
	return(stat);
}
#endif SVR4_STYLE_STATUS

#ifdef BSD_STYLE_STATUS

pirstat
get_pr_status(pn, avail, printing, qlen, needs_operator, status)
printername   pn;
bool_t       *avail;
bool_t       *printing;
int          *qlen;
bool_t       *needs_operator;
char         *status;
{
char cmd[128];
char buff[256];
char buff2[256];
char pname[64];
FILE *p;
char *cp;
char *cp1;
char *cp2;
int n;
pirstat stat = PI_RES_NO_SUCH_PRINTER;

	/* assume the worst */
	*avail = FALSE;
	*printing = FALSE;
	*needs_operator = FALSE;
	*qlen = 0;
	*status = '\0';

	pn = map_printer_name(pn);
	if(pn == NULL || suspicious(pn))
		return(PI_RES_NO_SUCH_PRINTER);

	sprintf(pname, "%s:", pn);
	n = strlen(pname);

	sprintf(cmd, "/usr/etc/lpc status %s", pn);
	p = popen(cmd, "r");
	if(p == NULL) {
		msg_out("rpc.pcnfsd: unable to popen() lpc status");
		return(PI_RES_FAIL);
	}
	
	while(fgets(buff, 255, p) != NULL) {
		if(strncmp(buff, pname, n))
			continue;
/*
** We have a match. The only failure now is PI_RES_FAIL if
** lpstat output cannot be decoded
*/
		stat = PI_RES_FAIL;
/*
** The next four lines are usually if the form
**
**     queuing is [enabled|disabled]
**     printing is [enabled|disabled]
**     [no entries | N entr[y|ies] in spool area]
**     <status message, may include the word "attention">
*/
		while(fgets(buff, 255, p) != NULL && isspace(buff[0])) {
			cp = buff;
			while(isspace(*cp))
				cp++;
			if(*cp == '\0')
				break;
			cp1 = cp;
			cp2 = buff2;
			while(*cp1 && *cp1 != '\n') {
				*cp2++ = 
					(isupper(*cp1) ? tolower(*cp1) : *cp1);
				cp1++;
			}
			*cp1 = '\0';
			*cp2 = '\0';
/*
** Now buff2 has a lower-cased copy and cp points at the original;
** both are null terminated without any newline
*/			
			if(!strncmp(buff2, "queuing", 7)) {
				*avail = (strstr(buff2, "enabled") != NULL);
				continue;
			}
			if(!strncmp(buff2, "printing", 8)) {
				*printing = (strstr(buff2, "enabled") != NULL);
				continue;
			}
			if(isdigit(buff2[0]) && (strstr(buff2, "entr") !=NULL)) {

				*qlen = atoi(buff2);
				continue;
			}
			if(strstr(buff2, "attention") != NULL ||
			   strstr(buff2, "error") != NULL)
				*needs_operator = TRUE;
			if(*needs_operator || strstr(buff2, "waiting") != NULL)
				strcpy(status, cp);
		}
		stat = PI_RES_OK;
		break;
	}
	(void) pclose(p);
	return(stat);
}
#endif BSD_STYLE_STATUS

/*
** pr_cancel
**
** cancel job id on printer pr on behalf of user
*/

#ifdef SVR4_STYLE_CANCEL

pcrstat pr_cancel(pr, user, id)
char *pr;
char *user;
char *id;
{
char            cmdbuf[256];
char            resbuf[256];
FILE *fd;
pcrstat stat = PC_RES_NO_SUCH_JOB;

	pr = map_printer_name(pr);
	if(pr == NULL || suspicious(pr))
		return(PC_RES_NO_SUCH_PRINTER);
	if(suspicious(id))
		return(PC_RES_NO_SUCH_JOB);

#ifdef hpux
        /* tjs 1/94 add printer name prefix to job received from PC */
        /* to form the hp-ux job name lp-job,  SR # 5003-154559     */
	sprintf(cmdbuf, "/usr/bin/cancel %s-%s", pr, id);
#else
	sprintf(cmdbuf, "/usr/bin/cancel %s", id);
#endif
	if ((fd = su_popen(user, cmdbuf, MAXTIME_FOR_CANCEL)) == NULL) {
		msg_out("rpc.pcnfsd: su_popen failed");
		return(PC_RES_FAIL);
	}
/*
** For SVR4 we have to be prepared for the following kinds of output:
** 
** # cancel lp-6
** request "lp-6" cancelled
** # cancel lp-33
** UX:cancel: WARNING: Request "lp-33" doesn't exist.
** # cancel foo-88
** UX:cancel: WARNING: Request "foo-88" doesn't exist.
** # cancel foo
** UX:cancel: WARNING: "foo" is not a request id or a printer.
**             TO FIX: Cancel requests by id or by
**                     name of printer where printing.
** # su geoff
** $ cancel lp-2
** UX:cancel: WARNING: Can't cancel request "lp-2".
**             TO FIX: You are not allowed to cancel
**                     another's request.
**
** There are probably other variations for remote printers.
** Basically, if the reply begins with the string
**          "UX:cancel: WARNING: "
** we can strip this off and look for one of the following
** (1) 'R' - should be part of "Request "xxxx" doesn't exist."
** (2) '"' - should be start of ""foo" is not a request id or..."
** (3) 'C' - should be start of "Can't cancel request..."
**
** The fly in the ointment: all of this can change if these
** messages are localized..... :-(
*/

/* HPNFS
** HP-UX on 9.00 is a bit different the SVR4.
** The possible error messages are:
**
** General errors:
**  "Error %d occured.\n"
**  "Maximum number of remote requests exceeded. Request \"%s\" not cancelled."
**  "Unable to execute the remote cancel command %s."
**  "You must have root capability to use this option"
**  "cannot restart class"
**  "printer \"%s\" has disappeared!"

** Not Owner:
**  "request \"%s-%d\" not cancelled: not owner"

** OK - cancel succeeded
**  "request \"%s-%d\" cancelled"
**  "your printer request %s-%d was cancelled by %s."

** No Such Job
**  "\"%s\" is not a printer or a class"
**  "\"%s\" is not a request id or a printer"
**  "printer \"%s\" was not busy"
**  "request \"%s-%d\" non-existent"
**  "spool directory non-existent" ( well it sort of fits here :) )
*/

#ifdef hpux
	if(fgets(resbuf, 255, fd) == NULL) 
		stat = PC_RES_FAIL;
        else if (strstr(resbuf, "not owner")) /* not cancelled: */
		stat = PC_RES_NOT_OWNER;
        else if (strstr(resbuf, "cancelled"))
		stat = PC_RES_OK;
        else if (strstr(resbuf, "not a"))
		stat = PC_RES_NO_SUCH_JOB;
        else if (strstr(resbuf, "was not busy"))
		stat = PC_RES_NO_SUCH_JOB;
        else if (strstr(resbuf, "non-existent"))
		stat = PC_RES_NO_SUCH_JOB;
        else 	stat = PC_RES_FAIL;

#else
	if(fgets(resbuf, 255, fd) == NULL) 
		stat = PC_RES_FAIL;
	else if(!strstr(resbuf, "UX:"))
		stat = PC_RES_OK;
	else if(strstr(resbuf, "doesn't exist"))
		stat = PC_RES_NO_SUCH_JOB;
	else if(strstr(resbuf, "not a request id"))
		stat = PC_RES_NO_SUCH_JOB;
	else if(strstr(resbuf, "Can't cancel request"))
		stat = PC_RES_NOT_OWNER;
	else	stat = PC_RES_FAIL;
#endif

	if(su_pclose(fd) == 255)
		msg_out("rpc.pcnfsd: su_pclose alert");
	return(stat);
}

#endif SVR4_STYLE_CANCEL

#ifdef BSD_STYLE_CANCEL

pcrstat pr_cancel(pr, user, id)
char *pr;
char *user;
char *id;
{
char            cmdbuf[256];
char            resbuf[256];
FILE *fd;
int i;
pcrstat stat = PC_RES_NO_SUCH_JOB;

	pr = map_printer_name(pr);
	if(pr == NULL || suspicious(pr))
		return(PC_RES_NO_SUCH_PRINTER);
	if(suspicious(id))
		return(PC_RES_NO_SUCH_JOB);

		sprintf(cmdbuf, "/usr/ucb/lprm -P%s %s", pr, id);
		if ((fd = su_popen(user, cmdbuf, MAXTIME_FOR_CANCEL)) == NULL) {
			msg_out("rpc.pcnfsd: su_popen failed");
			return(PC_RES_FAIL);
		}
		while(fgets(resbuf, 255, fd) != NULL) {
			i = strlen(resbuf);
			if(i)
				resbuf[i-1] = '\0'; /* trim NL */
			if(strstr(resbuf, "dequeued") != NULL)
				stat = PC_RES_OK;
			if(strstr(resbuf, "unknown printer") != NULL)
				stat = PC_RES_NO_SUCH_PRINTER;
			if(strstr(resbuf, "Permission denied") != NULL)
				stat = PC_RES_NOT_OWNER;
		}
		if(su_pclose(fd) == 255)
			msg_out("rpc.pcnfsd: su_pclose alert");
		return(stat);
}
#endif BSD_STYLE_CANCEL

/*
** New subsystem here. We allow the administrator to define
** up to NPRINTERDEFS aliases for printer names. This is done
** using the "/etc/pcnfsd.conf" file, which is read at startup.
** There are three entry points to this subsystem
**
** void add_printer_alias(char *printer, char *alias_for, char *command)
**
** This is invoked from "config_from_file()" for each
** "printer" line. "printer" is the name of a printer; note that
** it is possible to redefine an existing printer. "alias_for"
** is the name of the underlying printer, used for queue listing
** and other control functions. If it is "-", there is no
** underlying printer, or the administrative functions are
** not applicable to this printer. "command"
** is the command which should be run (via "su_popen()") if a
** job is printed on this printer. The following tokens may be
** embedded in the command, and are substituted as follows:
**
** $FILE	-	path to the file containing the print data
** $USER	-	login of user
** $HOST	-	hostname from which job originated
**
** Tokens may occur multiple times. If The command includes no
** $FILE token, the string " $FILE" is silently appended.
**
** pr_list list_virtual_printers()
**
** This is invoked from build_pr_list to generate a list of aliased
** printers, so that the client that asks for a list of valid printers
** will see these ones.
**
** char *map_printer_name(char *printer)
**
** If "printer" identifies an aliased printer, this function returns
** the "alias_for" name, or NULL if the "alias_for" was given as "-".
** Otherwise it returns its argument.
**
** char *expand_alias(char *printer, char *file, char *user, char *host)
**
** If "printer" is an aliased printer, this function returns a
** pointer to a static string in which the corresponding command
** has been expanded. Otherwise ot returns NULL.
*/
#define NPRINTERDEFS	256
int num_aliases = 0;
struct {
	char *a_printer;
	char *a_alias_for;
	char *a_command;
} alias [NPRINTERDEFS];


#ifdef SVR4_STYLE_PRINT
char default_cmd[] = "lp $FILE";
#endif SVR4_STYLE_PRINT
#ifdef BSD_STYLE_PRINT
char default_cmd[] = "lpr $FILE";
#endif BSD_STYLE_PRINT

void
add_printer_alias(printer, alias_for, command)
char *printer;
char *alias_for;
char *command;
{
/*
 * Add a little bullet-proofing here
 */
	if(alias_for == NULL || strlen(alias_for) == 0)
		alias_for = "lp";
	if(command == NULL || strlen(command) == 0)
		command = default_cmd;	/* see above */
	if(num_aliases < NPRINTERDEFS) {
		alias[num_aliases].a_printer = my_strdup(printer);
		alias[num_aliases].a_alias_for =
			(strcmp(alias_for,  "-") ? my_strdup(alias_for) : NULL);
		if(strstr(command, "$FILE"))
			alias[num_aliases].a_command = my_strdup(command);
		else {
			alias[num_aliases].a_command =
				(char *)grab(strlen(command) + 8);
			strcpy(alias[num_aliases].a_command, command);
			strcat(alias[num_aliases].a_command, " $FILE");
		}
		num_aliases++;
	}
}


/*
** list_virtual_printers
**
** build a pr_list of all virtual printers (if any)
*/
pr_list list_virtual_printers()
{
pr_list first = NULL;
pr_list last = NULL;
pr_list curr = NULL;
int i;


	if(num_aliases == 0)
		return(NULL);

	for (i = 0; i < num_aliases; i++) {
		curr = (struct pr_list_item *)
			grab(sizeof (struct pr_list_item));

		curr->pn = my_strdup(alias[i].a_printer);
		if(alias[i].a_alias_for == NULL)
			curr->device = my_strdup("");
		else
			curr->device = my_strdup(alias[i].a_alias_for);
		curr->remhost = my_strdup("");
		curr->cm = my_strdup("(alias)");
		curr->pr_next = NULL;
		if(last == NULL)
			first = curr;
		else
			last->pr_next = curr;
		last = curr;

	}
	return(first);
}



/*
** map_printer_name
**
** if a printer name is actually an alias, return the name of
** the printer for which it is an alias; otherwise return the name
*/
char *
map_printer_name(printer)
char *printer;
{
int i;
	for (i = 0; i < num_aliases; i++){
		if(!strcmp(printer, alias[i].a_printer))
			return(alias[i].a_alias_for);
	}
	return(printer);
}


/*
** substitute
**
** replace a token in a string as often as it occurs
*/
static void
substitute(string, token, data)
char *string;
char *token;
char *data;
{
char temp[512];
char *c;

	while(c = strstr(string, token)) {
		*c = '\0';
		strcpy(temp, string);
		strcat(temp, data);
		c += strlen(token);
		strcat(temp, c);
		strcpy(string, temp);
	}
}


/*
** expand_alias
**
** expand an aliased printer command by substituting file/user/host
*/
char *
expand_alias(printer, file, user, host)
char *printer;
char *file;
char *user;
char *host;
{
static char expansion[512];
int i;
	for (i = 0; i < num_aliases; i++){
		if(!strcmp(printer, alias[i].a_printer)) {
			strcpy(expansion, alias[i].a_command);
			substitute(expansion, "$FILE", file);
			substitute(expansion, "$USER", user);
			substitute(expansion, "$HOST", host);
			return(expansion);
		}
	}
	return(NULL);
}

