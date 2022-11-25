/* @(#) $Revision: 70.1 $ */
/****************************************************************

 The name of this file is utilities.c.

 +--------------------------------------------------------------+
 | (c)  Copyright  Hewlett-Packard  Company  1986.  All  rights |
 | reserved.   No part  of  this  program  may be  photocopied, |
 | reproduced or translated to another program language without |
 | the  prior  written   consent  of  Hewlett-Packard  Company. |
 +--------------------------------------------------------------+

 Changes:
	$Log:	utilities.c,v $
 * Revision 70.1  92/01/29  13:40:46  13:40:46  ssa
 * Author: dickermn@hpucsb2.cup.hp.com
 * Merged changes made for the second frecover patch for 8.0
 * 
 * Revision 66.7  91/10/30  17:49:22  17:49:22  ssa (RCS Manager)
 * Author: dickermn@hpisoe4.cup.hp.com
 * Changed not-on-media message
 * 
 * Revision 66.7  91/10/23  18:41:44  18:41:44  root ()
 * Added tolerance to the checksum-checking routine to accept character-based
 * checksums as well as integer-based sums.  Also cleaned-up error messages
 * 
 * Revision 66.6  91/03/01  08:31:27  08:31:27  ssa
 * Author: danm@hpbblc.bbn.hp.com
 * fixes to MO and DAT extensions
 * 
 * Revision 66.5  91/01/17  15:18:43  15:18:43  danm
 *  changes for DAT and MO support as well as other 8.0 enhancements
 * 
 * Revision 66.4  90/11/15  09:32:36  09:32:36  danm (#Dan Matheson)
 *  there were duplicate message numbers 4310 on different messages
 * 
 * Revision 66.3  90/02/27  15:47:34  15:47:34  danm (#Dan Matheson)
 *  clean up of #includes and switch to new directory(3c) routines.
 * 
 * Revision 66.2  90/02/26  19:51:24  19:51:24  danm (#Dan Matheson)
 *  moved #includes to frecover.h too make changes easier, and fixed things
 *  to use malloc(3x)
 * 
 * Revision 66.1  90/02/23  19:03:30  19:03:30  danm (#Dan Matheson)
 * folded in post 7.0 fixes for shippment stopper defect to main branch
 * 
 * Revision 64.2  89/05/23  02:50:04  02:50:04  kazu
 * fix FSDlj04152
 * frecover can't be run from cron
 * Changed not to open /dev/tty until user interaction needed
 * 
 * Revision 64.1  89/02/02  23:36:45  23:36:45  kazu (Kazuhisa Yokoto)
 * add remote device access feature
 * 
 * Revision 63.2  88/11/09  11:05:32  11:05:32  lkc (Lee Casuto)
 * made changes to reflect tuple to entry change
 *
 * Revision 63.1  88/09/22  15:56:57  15:56:57  lkc ()
 * Added code to handle ACLS
 *
 * Revision 62.3  88/07/26  17:37:34  17:37:34  lkc (Lee Casuto)
 * modified for CDF's. 3.0 and 6.5 should be the same source
 *
 * Revision 62.2  88/07/12  17:13:41  17:13:41  peteru
 * suppress handling of SIGINT if frecover started by SAM
 *
 * Revision 62.1  88/04/06  11:28:34  11:28:34  carolyn (Carolyn Sims)
 * added error numbers to messages for SAM to key off of
 * add capability to read responses from named pipe shared with SAM
 *
 * Revision 60.2  88/03/23  10:48:14  10:48:14  pvs
 * The symbol "SYMLINK" should really be "SYMLINKS". Made appropriate changes.
 *
 * Revision 60.1  88/02/17  11:11:09  11:11:09  sandee
 * Added a '+ sizeof(int)' to the fmalloc routine.
 *
 * Revision 56.1  87/11/04  10:08:20  10:08:20  runyan (Mark Runyan)
 * Complete for first release (by lkc)
 *
 * Revision 51.2  87/11/03  16:58:48  16:58:48  lkc (Lee Casuto)
 * completed for first release
 *

 This file:
	contains utility routins used by frecover.

****************************************************************/

#if defined NLS || defined NLS16
#define NL_SETN 4	/* set number */
#endif

#include "frecover.h"

extern void free();
extern void exit();
extern void doobscure();

/********************************************************************/
/*  	    PUBLIC PART OF UTILITIES	    	    	    	    */
int checkpoints;    	    	    /* checkpoint freq.	    	    */
int fsmfreq;            	    /* set in volume header read    */
int datarecsize;    	    	    /* set from volhdr	    	    */
int maxsize;	    	    	    /* maximum record size  	    */
RESTART p;		   	    /* restart data structure	    */
/********************************************************************/

/********************************************************************/
/*  	    VARIABLES FROM IO	    	    	    	    	    */
extern int fd;	    	    	    /* file descriptor for device   */
extern char buf[];  	    	    /* buffer to hold file data	    */
extern char *gbp;   	    	    /* pointer to current buf loc.  */
/********************************************************************/

/********************************************************************/
/*  	    VARIABLES FROM MAIN	    	    	    	    	    */
extern char errfile[MAXPATHLEN];    /* file to execute on errors    */
extern char restartname[MAXPATHLEN];/* file for saving status	    */
extern FILE *terminal;	    	    /* device for responses	    */
extern int Zflag;	            /* response from sampipe not tty*/
extern int yflag;		    /* answer yes to all prompts    */
extern int pflag;		    /* partial recovery flag	    */
#ifdef ACLS
extern int aclflag;		    /* recover optional entry  flag */
#endif /* ACLS */
extern int doerror;		    /* config file complete         */
extern LISTNODE *ilist;		    /* list of files to recover	    */
extern int hflag;		    /* used for restart only	    */
extern int aflag;		    /* used for restart only	    */
extern int vflag;		    /* used for restart only	    */
extern int sflag;		    /* used for restart only	    */
extern int cflag;		    /* used for restart only	    */
extern int xflag;		    /* used for restart only	    */
extern int fflag;		    /* used for restart only	    */
extern int flatflag;		    /* used for restart only	    */
extern int oflag;		    /* used for restart only	    */
extern int volnum;		    /* used for restart only	    */
extern int overwrite;		    /* used for restart only	    */
extern int recovertype;		    /* used for restart only	    */
extern int dochgvol;		    /* used for restart only	    */
extern int synclimit;		    /* used for restart only	    */
extern char residual[MAXPATHLEN];   /* used for restart only	    */
extern char home[MAXPATHLEN+3];	    /* used for restart only	    */
extern char chgvolfile[MAXPATHLEN]; /* used for restart only	    */
extern OBSCURE *o_head;		    /* used for restart only	    */
extern int blocksize;

extern int     outfiletype;
extern char   *outfptr;

#if defined NLS || defined NLS16
extern nl_catd nlmsg_fd;	    /* used by catgets		    */
#endif
/********************************************************************/

/********************************************************************/
/*	    VARIABLES FROM FILES				    */
extern char recovername[MAXPATHLEN];	/* temp recover name	    */
extern char msg[MAXS];			/* warn message buffer	    */
extern struct stat statbuf;		/* used here for cleanup    */
extern int filenum;			/* used for restart only    */
extern int newfd;			/* used to close onintr	    */
/********************************************************************/

/********************************************************************/
/*	    VARIABLES FROM VOLHEADERS				    */
extern int prevvol;			/* used for restart only    */
extern VHDRTYPE vol;			/* used for restart only    */
/********************************************************************/


cpbytes(s1, s2, start1, start2, more)
char *s1, *s2;
int *start1, *start2, *more;
{
    while(TRUE) {
    	if(*start1 == blocksize)
	    return;
	if(*start2 == blocksize)
	    return;
	if(s2[*start2] == '\0')
	    break;
    	s1[*start1] = s2[*start2];
	*start1 += 1;
	*start2 += 1;
    }
    s1[*start1] = s2[*start2];
    *start2 += 1;

    *more = 0;
    while(s2[*start2] == '\0') {
    	if(*start2 == blocksize)
	    return;
    	*start2 += 1;
    }
    return;

}



/*
 * Clean up and exit
 */
done(exitcode)
	int exitcode;
{
    LISTNODE *t1;			/* temp pointer */

    if(exitcode != 0) {
	if(statcall(recovername, &statbuf) == 0) {
	    (void) close(newfd);
            (void) unlink(recovername);	/* get rid of file */
	}
    }

    if(pflag) {
	t1 = ilist->ptr;
	while(t1 != (LISTNODE *)NULL) {
	    if(t1->aflag < SUBSET) {
		(void) sprintf(msg, (catgets(nlmsg_fd,NL_SETN,301, "(4301): %s not recovered from backup media")), t1->val);
	        warn(msg);
	    }
	    t1 = t1->ptr;
	}
    }

    if(exitcode == 0)
        doobscure();

    if (outfiletype == VDI_REMOTE) {
      rmt_close(fd);
    } else {
      vdi_close(outfiletype, outfptr, &fd, 0);
    }

    exit(exitcode);
}

/*
 * respond to interrupts
 */
onintr()
{
        int ysave;

	ysave = yflag;
	yflag = 0;			/* turn off auto answer */
	if (reply((catgets(nlmsg_fd,NL_SETN,302, "(4302): frecover interrupted, continue"))) == FAIL) {
	    if(reply((catgets(nlmsg_fd,NL_SETN,303, "(4303): do you wish to save status for restart"))) == FAIL) {
		done(1);
	    }
	    else {
		savestatus();
		done(1);
	    }
	}
	yflag = ysave;			/* reset yes flag	*/
	/* ignore SIGINT if SAM special option is used */
	if (!Zflag)
	{
		if (signal(SIGINT, onintr) == SIG_IGN)
			(void) signal(SIGINT, SIG_IGN);
	}
	else
		(void) signal (SIGINT, SIG_IGN);
	if (signal(SIGTERM, onintr) == SIG_IGN)
		(void) signal(SIGTERM, SIG_IGN);
}

/*
 * Elicit a reply.
 */
#define MAXANSLEN 32
#define MAXQUESLEN 256
reply(inputstr)
	char *inputstr;
{
	char question[MAXQUESLEN];
	char ans[MAXANSLEN];
	char nostr[16];
	char yesstr[16];

#if defined NLS || defined NLS16
	(void) strcpy(yesstr, nl_langinfo(YESSTR));
	(void) strcpy(nostr, nl_langinfo(NOSTR));
#else
	(void) strcpy(yesstr, "yes");
	(void) strcpy(nostr, "no");
#endif /* NLS */

	(void) strncpy(question, inputstr, MAXQUESLEN);
	(void) fprintf(stderr, "frecover%s? ", question);
        (void) fflush(stderr);
	if(yflag) {
	    (void) fprintf(stderr, (catgets(nlmsg_fd,NL_SETN,304, "(4304): automatic '%s'\n")), yesstr);
            (void) fflush(stderr);
	    return(GOOD);
	}

	for(;;) {
	    (void) fprintf(stderr, "[%s %s] ",yesstr, nostr);
	    if(Zflag)			/* SAM expects newline */
	        (void) fprintf(stderr, "\n");
	    (void) fflush(stderr);
	    (void) openterminal();	/* open /dev/tty for interaction */
	    (void) fgets(ans, MAXANSLEN, terminal);
	    ans[strlen(ans)-1] = '\0';	/* strip off '\n' */
	    if(!strcmp(yesstr, ans))
	        return(GOOD);
	    if(!strcmp(nostr, ans))
	        return(FAIL);
	}
}



extern int usage();			/* display usage 	*/
extern int done();			/* exit and cleanup	*/

panic(str, use)
char *str;
int use;
{
    if(strcmp(str, (char *)NULL)) {
	(void) fprintf(stderr, "frecover%s\n",str);
	(void) fflush(stderr);
    }
    if(doerror)
        (void) system(errfile);
    if(use)
        usage();
    done(1);
}


int
warn(str)
char *str;
{
    (void) fprintf(stderr, "frecover%s\n", str);
    (void) fflush(stderr);
    if(doerror)
        (void) system(errfile);
    return;
}


char *
fmalloc(val)
unsigned val;
{
    char *p;

    if((p = malloc(val + sizeof(int))) == NULL)
        panic((catgets(nlmsg_fd,NL_SETN,305, "(4305): not enough memory for allocate")), !USAGE);
    return(p);
}


saveuser(u, user)
struct passwd *u, *user;
{
  user->pw_name = fmalloc((unsigned)strlen(u->pw_name));
  (void) strcpy(user->pw_name, u->pw_name);
  user->pw_uid = u->pw_uid;
  user->pw_gid = u->pw_gid;
}

savegroup(g, guser)
struct group *g, *guser;
{
    guser->gr_name = fmalloc((unsigned)strlen(g->gr_name));
    (void) strcpy(guser->gr_name, g->gr_name);
    guser->gr_gid = g->gr_gid;
}


char *
dirname(path)
char *path;
{
	register	char	*p1, *p2, *savep, *nextc;
	static		char	*p3;
	extern char 	*fmalloc();
	static int	first = TRUE;

	if(first)
	    first = FALSE;
	else
	    (void) free(p3);

	p3 = fmalloc(strlen(path)+sizeof(int));
	(void) strcpy(p3, path);
	nextc = savep = p1 = p2 = p3;
	while (*p1) {
#ifdef NLS16
	    ADVANCE(nextc);
#else
	    nextc = (p1 + 1);
#endif
	    if(((*p1 == '/') && (*nextc != '/')) && (*nextc != '\0'))
                savep = p1;
#ifdef NLS16
	    ADVANCE(p1);
#else
	    p1++;
#endif
	}
	if (savep == p2) {
		if (*savep != '/')
			*savep = '.';
		savep++;
	}
	*savep = '\0';
	return(p2);
}


char *
basename(p)
char *p;
{
    register char *s;

    if(s = strrchr(p, '/'))
    	s++;
    else
    	s = p;

    return(s);
}


#ifdef NLS16
#define CNULL (char *)0

char *
strrchr(sp, c)
register char *sp;
register unsigned    c;
{
	register char *r;

	if (sp == CNULL)
		return(CNULL);

	r = CNULL;
	do {
		if(CHARAT(sp) == c)
			r = sp;
	} while(CHARADV(sp));
	return(r);
}
#endif NLS16



OBSCURE *t1, *t2;			/* used by save and restore status */

savestatus()
{
    int rfd;

    while(TRUE) {
	(void) fprintf(stderr, (catgets(nlmsg_fd,NL_SETN,310, "frecover(4310): Enter the absolute path name of a file for saving status: ")));
	(void) fflush(stderr);
	(void) openterminal();	/* open /dev/tty for interaction */
	(void) fgets(restartname, MAXPATHLEN, terminal);
	restartname[strlen(restartname) -1] = '\0';	/* strip cr */
	(void) fflush(terminal);
	if((rfd=creat(restartname, 0644)) == NULL)
	    warn((catgets(nlmsg_fd,NL_SETN,306, "(4306): can't create restart file, try again")));
	break;
    }

    p.filenum = filenum;
    p.hflag = hflag;
    p.yflag = yflag;
    p.aflag = aflag;
    p.vflag = vflag;
    p.sflag = sflag;
    p.cflag = cflag;
    p.xflag = xflag;
    p.fflag = fflag;
    p.pflag = pflag;
#ifdef ACLS
    p.aclflag = aclflag;
#endif /* ACLS */
    p.flatflag = flatflag;
    p.oflag = oflag;
    p.volnum = volnum;
    p.overwrite = overwrite;
    p.recovertype = recovertype;
    p.doerror = doerror;
    p.dochgvol = dochgvol;
    p.prevvol = prevvol;
    p.synclimit = synclimit;
    p.vol = vol;
    (void) strcpy(p.residual, residual);
    (void) strcpy(p.home, home);
    (void) strcpy(p.errfile, errfile);
    (void) strcpy(p.chgvolfile, chgvolfile);

    if(write(rfd, (char *)&p, sizeof(RESTART)) != sizeof(RESTART)) {
	warn((catgets(nlmsg_fd,NL_SETN,307, "(4307): can't write restart file")));
	(void) close(rfd);
	(void) unlink(restartname);
	return;
    }

    t1 = o_head->o_ptr;
    while(t1 != (OBSCURE *)NULL) {
	if(write(rfd, (char *)t1, sizeof(OBSCURE)) != sizeof(OBSCURE)) {
	    warn((catgets(nlmsg_fd,NL_SETN,312, "(4312): can't save obscure node")));
	    (void) close(rfd);
	    (void) unlink(restartname);
	    return;
	}
	t1 = t1->o_ptr;
    }

    (void) close(rfd);
    return;
}

restorestatus()
{
    int rfd;
    int res;

    if((rfd=open(restartname, O_RDONLY)) == NULL) {
	panic((catgets(nlmsg_fd,NL_SETN,308, "(4308): can't open restart file")), !USAGE);
    }

    if(read(rfd, (char *)&p, sizeof(RESTART)) != sizeof(RESTART)) {
	panic((catgets(nlmsg_fd,NL_SETN,309, "(4309): can't read restart file")), !USAGE);
    }

    t2 = o_head;
    t1 = (OBSCURE *)fmalloc(sizeof(OBSCURE));
    while(TRUE) {
	res = read(rfd, (char *)t1, sizeof(OBSCURE));
	if(res == 0) {
	    free((char *)t1);		/* don't need this node */
	    break;			/* EOF detected */
	}
	if(res < 0)
	    panic((catgets(nlmsg_fd,NL_SETN,311, "(4311): can't read OBSCURE node")), !USAGE);
	t2->o_ptr = t1;
	t2 = t1;
	t1->o_ptr = (OBSCURE *)NULL;
	t1 = (OBSCURE *)fmalloc(sizeof(OBSCURE));
    }

    filenum = p.filenum;;
    hflag = p.hflag;
    yflag = p.yflag;
    aflag = p.aflag;
    vflag = p.vflag;
    sflag = p.sflag;
    cflag = p.cflag;
    xflag = p.xflag;
    fflag = p.fflag;
    pflag = p.pflag;
    flatflag = p.flatflag;
    oflag = p.oflag;
    volnum = p.volnum;
    overwrite = p.overwrite ;
    recovertype = p.recovertype;
    doerror = p.doerror;
    dochgvol = p.dochgvol;
    prevvol = p.prevvol;
    synclimit = p.synclimit;
    vol = p.vol;
    (void) strcpy(residual, p.residual);
    (void) strcpy(home, p.home);
    (void) strcpy(errfile, p.errfile);
    (void) strcpy(chgvolfile, p.chgvolfile);

    (void) close(rfd);
    return;
}


#ifndef SYMLINKS
/* @(#) $Revision: 70.1 $ */
/*
 *	rename file specified by from to file specified by to
 */

/*	Modified:	31 January 1987 - Lee Casuto - ITG/ISO
 *			prrserve uid, gid, and mode of from file
 */

int
rename (from, to)
char *from, *to;
{
	(void) unlink(to);
	if (link(from, to)) {
		(void) fprintf(stderr, "cannot link %s to %s\n", from, to);
		return(-1);
	}

	if (unlink(from)) {
		(void) fprintf(stderr,"cannot unlink %s\n", from);
		return(-1);
	}
	return(0);
}
#endif /* SYMLINKS */
