/* 	@(#)where.c	$Revision: 1.14.109.1 $	$Date: 91/11/19 14:16:00 $  */

/* where.c 1.1 87/03/16 NFSSRC */

/*
 *  where.c - get full pathname including host:
 *
 * Copyright (c) 1985 Sun Microsystems, Inc.
 */

/* NOTE: on.c and where.c share a single message catalog (on.cat).	*/
/* For that reason we have allocated messages 1 through 40 for the on.c */
/* file and messages 41 on for the where.c file.  That is why we start 	*/
/* the messages in this file at number 41. If we need more messages in 	*/
/* this file we will need to take into account the message numbers that */
/* are already used by the other files.					*/				
#ifndef NLS
#define catgets(i, sn,mn,s) (s)
#else NLS
#define NL_SETN 1	/* set number */
#include <nl_types.h>
#endif NLS

#include <stdio.h>
#include <mntent.h>
#include <sys/param.h>
#include <sys/stat.h>

#ifdef DUX     /* added 8/31/88 to support rex on diskless: mjk */
/* need these files to handle diskless interactions */
#include <cluster.h>
#include <sys/types.h>
#endif DUX

extern errno, sys_nerr;
extern char *sys_errlist[];
#ifdef NLS
extern nl_catd nlmsg_fd;
char *catgets();
#endif NLS

#define errstr() (errno < sys_nerr ? sys_errlist[errno] : (catgets(nlmsg_fd,NL_SETN,41, "unknown error")))


/*
 * findmount(qualpn, host, fsname, within)
 *
 * Searches the mount table to find the appropriate file system
 * for a given absolute path name.
 * host gets the name of the host owning the file system,
 * fsname gets the file system name on the host,
 * within gets whatever is left from the pathname
 *
 * Returns: 0 on failure, 1 on success and -1 is directory is accessed via RFA.
 */
findmount(qualpn, host, fsname, within)
	char *qualpn;
	char *host;
	char *fsname;
	char *within;
{
	FILE *mfp;
	char bestname[MAXPATHLEN];
	int bestlen = 0, bestnfs = 0;
	struct mntent *mnt;
   	char *endhost;			/* points past the colon in name */
	extern char *index();
	int i, len;
	struct stat stat_info;

	for (i = 0; i < 10; i++) {
		mfp = setmntent(MNT_MNTTAB, "r");
		if (mfp != NULL)
			break;
		sleep(1);
	}
	if (mfp == NULL) {
		sprintf(within, (catgets(nlmsg_fd,NL_SETN,43, "mount table problem")));
		return (0);
	}

	while ((mnt = getmntent(mfp)) != NULL) {
		len = preflen(qualpn, mnt->mnt_dir);
		if ( (len == 1 && qualpn[0] != '/' && qualpn[0] != '\0') ||
		     (len != 1 && qualpn[len] != '/' && qualpn[len] != '\0'))
			continue;
		if (len > bestlen) {
			bestlen = len;
			strcpy(bestname, mnt->mnt_fsname);
		}
	}
	endmntent(mfp);

	if (stat(qualpn, &stat_info) < 0) {
           /* strcpy(within, errstr()); */
           return (0);
	}
	else if (stat_info.st_remote) 
          /* This is a directory accessed via RFA. Return appropriate
             code to on indicating that RFA is not supported.
           */
	  return(-1);

	endhost = index(bestname,':');
	  /*
	   * If the file system was of type NFS, then there should already
	   * be a host name, otherwise, use ours.
	   */
	if (endhost) {
		*endhost++ = 0;
		strcpy(host,bestname);
		strcpy(fsname,endhost);
	} else 
#ifdef DUX    /* added 8/31/88 to support rex on diskless: mjk */
	  { 
	    /* 
	    ** The file system appears to be local but it may be another
	    ** nodes disk if we are in a discless cluster.
	    */

	    /* declare variables needed for diskless */
	    site_t site;
	    struct cct_entry *entry;

	    /* returns site id if diskless cluster otherwise 0 */
	    site = cnodeid();
	    
	    if (site == 0) 
	      {
		/* we are not in a diskless cluster use our hostname */
		gethostname(host, 255);
	      }
	    else
	      {
		/*
		** we are in a diskless cluster, must determine if the 
		** disk is ours or which cluster member has it.
		*/
		if (site == stat_info.st_cnode )
		  {
		    /* we have the disk, use our host name */
		    gethostname(host, 255);
		  }
		else
		  { 
		    /* 
		    ** another cluster member has the disk. figure out
		    ** who has it and use their hostname
		    */
		    entry = getcccid(stat_info.st_cnode);
		    if (entry != NULL)
		      {
			strcpy(host,entry->cnode_name);
		      }
		    else
		      {
			/* something went wrong assume filesystem is 
			 * local
			*/
			gethostname(host, 255);
		      }
		  }
	      }
		strncpy(fsname,qualpn,bestlen);
		fsname[bestlen] = 0;
	  } 
#else DUX
	  {
		gethostname(host, 255);
		strncpy(fsname,qualpn,bestlen);
		fsname[bestlen] = 0;
	  }
#endif DUX
	/*
	 * Special case for when in the root file system.
	 */
	if ( bestlen == 1 )
		strcpy(within, qualpn);
	else
		strcpy(within,qualpn+bestlen);
	return 1;
}	  

/*
 * Returns: length of second argument if it is a prefix of the
 * first argument, otherwise zero.
 */
preflen(str, pref)
	char *str, *pref;
{
	int len; 

	len = strlen(pref);
	if (strncmp(str, pref, len) == 0)
		return (len);
	return (0);
}
