/*
**	getpty.c	--	get a master and slave pty pair
****
**    Note:   this does not setpgrp any longer, the pty bug is
**            now fixed!  Or seems to be ...
*/

#ifndef lint
static char RCS_ID[] = "$Header: getpty.c,v 1.2.109.4 94/11/15 11:47:16 mike Exp $";
#endif lint

#include <sys/param.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <dirent.h>
#include <stdio.h>

# define	MAX_PTY_LEN	MAXPATHLEN+1
# define	CLONE_DRV	"/dev/ptym/clone"

/*
**	Ugh!  The following is gross, but there is no other simple way
**	to tell if a file is a master pty without looking at its major
**	device number.  Lets put it in ifdefs so it will fail to compile
**	on other architectures without modification.
*/
#ifdef __hp9000s300
#define PTY_MASTER_MAJOR_NUMBER 16
#endif
#ifdef __hp9000s800
#define PTY_MASTER_MAJOR_NUMBER 16
#endif

struct	stat	stb;
struct	stat	m_stbuf;	/* stat buffer for master pty */
struct	stat	s_stbuf;	/* stat buffer for slave pty */

/*
**	ptymdirs	--	directories to search for master ptys
**	ptysdirs	--	directories to search for slave ptys
**	ptymloc		--	full path to pty master
**	ptysloc		--	full path to pty slave
*/
char	*ptymdirs[] = { "/dev/ptym/", "/dev/", (char *) 0 } ;
char	*ptysdirs[] = { "/dev/pty/", "/dev/", (char *) 0 } ;
char	ptymloc[MAX_PTY_LEN];
char	ptysloc[MAX_PTY_LEN];

/*
**	oltrs	--	legal first char of pty name (letter) (old style)
**	onums	--	legal second char of pty name (number) for namespace 1
**              (old style)
**	ltrs	--	legal first char of pty name (letter) (new style)
**	nums	--	legal second and third char of pty name (number) (new style)
**	nums	--	legal second, third and fourth  char of pty name for names
**              space 3. 
*/
char	oltrs[] = "onmlkjihgfecbazyxwvutsrqp";
char	onums[] = "0123456789abcdef";
char	ltrs[] = "pqrstuvwxyzabcefghijklmno";
char	nums[] = "0123456789";

/*
**	getpty()	--	get a pty pair
**
**	Input	--	none
**	Output	--	zero if pty pair gotten; non-zero if not
**	[value parameter mfd]
**		  mfd	Master FD for the pty pair
**	[optional value parameters -- only set if != NULL]
**		mname	Master pty file name
**		sname	Slave pty file name
**
**	NOTE: This routine does not open the slave pty.  Therefore, if you
**	want to quickly find the slave pty to open, it is recommended that
**	you *not* pass sname as NULL.
*/

/*
**	Modified 3/28/89 by Peter Notess to search the new, expanded naming
**	convention.  The search is intended to proceed efficiently for both
**	large and small configurations, assuming that the ptys are created
**	in a pre-specified order.  This routine will do a last-ditch,
**	brute-force search of all files in the pty directories if the more
**	efficient search fails.  This is intended to make the search robust
**	in the case where the pty configuration is a non-standard one,
**	although it might take longer to find a pty in this case.  The
**	assumed order of creation is:
**		/dev/ptym/pty[p-z][0-f]
**		/dev/ptym/pty[a-ce-o][0-f]
**		/dev/ptym/pty[p-z][0-9][0-9]
**		/dev/ptym/pty[a-ce-o][0-9][0-9]
**	with /dev/ptym/pty[p-r][0-f] linked into /dev.  The search will
**	proceed in an order that leaves the ptys which also appear in /dev
**	searched last so that they remain available for other applications
**	as long as possible.
*/

/*
 * Modified by Glen A. Foster, September 23, 1986 to get around a problem
 * with 4.2BSD job control in the HP-UX (a.k.a. system V) kernel.  Before
 * this fix, getpty() used to find a master pty to use, then open the cor-
 * responding slave side, just to see if it was there (kind of a sanity
 * check), then close the slave side, fork(), and let the child re-open 
 * the slave side in order to get the proper controlling terminal.  This
 * was an excellent solution EXCEPT for the case when another process was
 * already associated with the same slave side before we (telnetd) were
 * exec()ed.  In that case, the controlling tty stuff gets all messed up,
 * and the solution is to NOT open the slave side in the parent (before the
 * fork()), but to let the child be the first to open it after its setpgrp()
 * call.  This works in all cases.  This stuff is black magic, really!
 *
 * This is necessary due to HP's implementation of 4.2BSD job control.
 */

/*
 * Modified by Byron Deadwiler, March 9, 1992 to add access to the clone
 * driver to get ptys faster.  Also added code to access ptys using
 * a new namespace (naming convention) that increases the number of
 * pty's that can be configured on a system.
 *
 *	Name-Space      It is a union of three name-spaces, the earlier two
 *			being supported for compatibility. The name comprises
 *			a generic name (/dev/pty/tty) followed by an alphabet
 *			followed by one to three numerals. The alphabet is one
 *			of the 25 in alpha[], which has 'p' thro' 'o' excluding
 *			'd'. The numeral is either hex or decimal.
 *	                ------------------------------------
 *	                | minor | name  |    Remarks       |
 *	                |----------------------------------|
 *                  |      0|  ttyp0|  Modulo 16 hex   |
 *	                |      :|      :|  representation. |
 *	                |     15|  ttypf|                  |
 *	                |     16|  ttyq0|                  |
 *	                |      :|      :|                  |
 *	                |    175|  ttyzf|                  |
 *	                |    176|  ttya0|                  |
 *	                |      :|      :|                  |
 *	                |    223|  ttycf|                  |
 *	                |    224|  ttye0|                  |
 *	                |      :|      :|                  |
 *	                |    399|  ttyof|  Total 400       |
 *	                |----------------------------------|
 *	                |    400| ttyp00|  Modulo hundred  |
 *	                |      :|      :|  decimal repr.   |
 *	                |    499| ttyp99|                  |
 *	                |    500| ttyq00|                  |
 *	                |      :|      :|                  |
 *	                |   1499| ttyz99|                  |
 *	                |   1500| ttya00|                  |
 *	                |      :|      :|                  |
 *	                |   1799| ttyc99|                  |
 *	                |   1800| ttye00|                  |
 *	                |      :|      :|                  |
 *	                |   2899| ttyo99|  Total 2500      |
 *	                |----------------------------------|
 *	                |   2900|ttyp000|  Modulo thousand |
 *	                |      :|      :|  decimal repr.   |
 *	                |   3899|ttyp999|                  |
 *	                |   4900|ttyq000|                  |
 *	                |      :|      :|                  |
 *	                |  12899|ttyz999|                  |
 *	                |  13900|ttya000|                  |
 *	                |      :|      :|                  |
 *	                |  16899|ttyc999|                  |
 *	                |  16900|ttye000|                  |
 *	                |      :|      :|                  |
 *	                |  27899|ttyo999|  Total 25000     |
 *	                |  27900|   	|     invalid      |
 *	                ------------------------------------
 */

/*
**	NOTE:	this routine should be put into a library somewhere, since
**	both rlogin and telnet need it!  also, other programs might want to
**	call it some day to get a pty pair ...
*/
getpty(mfd, mname, sname)
int	*mfd;
char	*mname, *sname;
{
    int loc, ltr, num, num2;
    register int mlen, slen;
	char *s, *s_path;

    if ((*mfd = open(CLONE_DRV, O_RDWR))  != -1) {
        s_path = ptsname(*mfd);
	if (s_path == NULL)
		close(*mfd);
	else
	{
		strcpy(sname,s_path);
        	(void) chmod (sname, 0622);
        	/* get master path name */
        	s = strrchr(sname, '/') + 2; /* Must succeed since slave_pty */
                                     	     /* begins with /dev/            */
        	sprintf(mname,"%s%s%s",ptymdirs[0],"p",s);
        	return 0;
	}
    }
    

    for (loc=0; ptymdirs[loc] != (char *) 0; loc++) {
	if (stat(ptymdirs[loc], &stb))			/* no directory ... */
	    continue;					/*  so try next one */

	/*	first, try the 3rd name space ptyp000-ptyo999  */
	/*	generate the master pty path	*/
	if (namesp3(mfd,mname,sname,loc) == 0)
             return 0;

	/*	second, try the 2nd name space ptyp00-ptyo99  */
	/*	generate the master pty path	*/
	(void) strcpy(ptymloc, ptymdirs[loc]);
	(void) strcat(ptymloc, "ptyLNN");
	mlen = strlen(ptymloc);

	/*	generate the slave pty path	*/
	(void) strcpy(ptysloc, ptysdirs[loc]);
	(void) strcat(ptysloc, "ttyLNN");
	slen = strlen(ptysloc);

	for (ltr=0; ltrs[ltr] != '\0'; ltr++) {
	    ptymloc[mlen - 3] = ltrs[ltr];
	    ptymloc[mlen - 2] = '0';
	    ptymloc[mlen - 1] = '0';
	    if (stat(ptymloc, &stb))			/* no ptyL00 ... */
		break;				/* go try old style names */

	    for (num=0; nums[num] != '\0'; num++)
	      for (num2=0; nums[num2] != '\0'; num2++) {
		ptymloc[mlen - 2] = nums[num];
		ptymloc[mlen - 1] = nums[num2];
		if ((*mfd=open(ptymloc,O_RDWR)) < 0)	/* no master	*/
		    continue;				/* try next num	*/
		
		ptysloc[slen - 3] = ltrs[ltr];
		ptysloc[slen - 2] = nums[num];
		ptysloc[slen - 1] = nums[num2];

		/*
		**	NOTE:	changed to only stat the slave device; see
		**	comments all over the place about job control ...
		*/
		if (fstat(*mfd, &m_stbuf) < 0 || stat(ptysloc, &s_stbuf) < 0) {
		    close(*mfd);
		    continue;
		}
		/*
		**	sanity check: are the minor numbers the same??
		*/
		if (minor(m_stbuf.st_rdev) != minor(s_stbuf.st_rdev)) {
		    close(*mfd);  
		    continue;				/* try next num	*/
		}

		/*	else we got both a master and a slave pty	*/
got_one:	(void) chmod (ptysloc, 0622);		/* not readable */
		if (mname != (char *) 0)
		    (void) strcpy(mname, ptymloc);
		if (sname != (char *) 0)
		    (void) strcpy(sname, ptysloc);
		return 0;				/* return OK	*/
	    }
	}

	/*	now, check old-style names	*/
	/*  the 1st name-space ptyp0-ptyof */
	/*	generate the master pty path	*/
	(void) strcpy(ptymloc, ptymdirs[loc]);
	(void) strcat(ptymloc, "ptyLN");
	mlen = strlen(ptymloc);

	/*	generate the slave pty path	*/
	(void) strcpy(ptysloc, ptysdirs[loc]);
	(void) strcat(ptysloc, "ttyLN");
	slen = strlen(ptysloc);

	for (ltr=0; oltrs[ltr] != '\0'; ltr++) {
	    ptymloc[mlen - 2] = oltrs[ltr];
	    ptymloc[mlen - 1] = '0';
	    if (stat(ptymloc, &stb))			/* no ptyL0 ... */
		continue;				/* try next ltr */

	    for (num=0; onums[num] != '\0'; num++) {
		ptymloc[mlen - 1] = onums[num];
		if ((*mfd=open(ptymloc,O_RDWR)) < 0)	/* no master	*/
		    continue;				/* try next num	*/
		
		ptysloc[slen - 2] = oltrs[ltr];
		ptysloc[slen - 1] = onums[num];

		/*
		**	NOTE:	changed to only stat the slave device; see
		**	comments all over the place about job control ...
		*/
		if (fstat(*mfd, &m_stbuf) < 0 || stat(ptysloc, &s_stbuf) < 0) {
		    close(*mfd);
		    continue;
		}
		/*
		**	sanity check: are the minor numbers the same??
		*/
		if (minor(m_stbuf.st_rdev) != minor(s_stbuf.st_rdev)) {
		    close(*mfd);  
		    continue;				/* try next num	*/
		}

		/*	else we got both a master and a slave pty	*/
		goto got_one;
	    }
	}
    }

    /* we failed in our search--we now try the slow brute-force method */
    for (loc=0; ptymdirs[loc] != (char *) 0; loc++) {
	DIR *dirp;
	struct dirent *dp;

	if ((dirp=opendir(ptymdirs[loc])) == NULL)	/* no directory ... */
	    continue;					/*  so try next one */

	(void) strcpy(ptymloc, ptymdirs[loc]);
	mlen = strlen(ptymloc);
	(void) strcpy(ptysloc, ptysdirs[loc]);
	slen = strlen(ptysloc);

	while ((dp=readdir(dirp)) != NULL) {
	    /* stat, open, go for it, else continue */
	    ptymloc[mlen] = '\0';
	    (void) strcat(ptymloc, dp->d_name);

	    if (stat(ptymloc, &m_stbuf) ||
	       (m_stbuf.st_mode & S_IFMT) != S_IFCHR ||
	       major(m_stbuf.st_rdev) != PTY_MASTER_MAJOR_NUMBER)
		continue;

	    if ((*mfd=open(ptymloc,O_RDWR)) < 0)	/* busy master	*/
	        continue;				/* try next entry */

	    ptysloc[slen] = '\0';       /* guess at corresponding slave name */
	    (void) strcat(ptysloc, dp->d_name);
	    if (ptysloc[slen] == 'p')
		ptysloc[slen] = 't';

	    if (stat(ptysloc, &s_stbuf) < 0 ||
		minor(m_stbuf.st_rdev) != minor(s_stbuf.st_rdev)) {
	        close(*mfd);
	        continue;
	    }

	    goto got_one;
	}

	closedir(dirp);
    }

    /*	we were not able to get the master/slave pty pair	*/
    return -1;		
}

namesp3(mfd,mname,sname,loc)
int     *mfd, loc;
char    *mname, *sname;
{
	int ltr, num, num2, num3;
    	register int mlen, slen;

	/*	first, try the new naming convention	*/
	/*	generate the master pty path	*/
	(void) strcpy(ptymloc, ptymdirs[loc]);
	(void) strcat(ptymloc, "ptyLNNN");
	mlen = strlen(ptymloc);

	/*	generate the slave pty path	*/
	(void) strcpy(ptysloc, ptysdirs[loc]);
	(void) strcat(ptysloc, "ttyLNNN");
	slen = strlen(ptysloc);

	for (ltr=0; ltrs[ltr] != '\0'; ltr++) {
	    ptymloc[mlen - 4] = ltrs[ltr];
	    ptymloc[mlen - 3] = '0';
	    ptymloc[mlen - 2] = '0';
	    ptymloc[mlen - 1] = '0';
	    if (stat(ptymloc, &stb))			/* no ptyL00 ... */
		break;				/* go try old style names */

	    for (num=0; nums[num] != '\0'; num++)
	      for (num2=0; nums[num2] != '\0'; num2++) 
	        for (num3=0; nums[num3] != '\0'; num3++) {
		ptymloc[mlen - 3] = nums[num];
		ptymloc[mlen - 2] = nums[num2];
		ptymloc[mlen - 1] = nums[num3];
		if ((*mfd=open(ptymloc,O_RDWR)) < 0)	/* no master	*/
		    continue;				/* try next num	*/
		
		ptysloc[slen - 4] = ltrs[ltr];
		ptysloc[slen - 3] = nums[num];
		ptysloc[slen - 2] = nums[num2];
		ptysloc[slen - 1] = nums[num3];

		/*
		**	NOTE:	changed to only stat the slave device; see
		**	comments all over the place about job control ...
		*/
		if (fstat(*mfd, &m_stbuf) < 0 || stat(ptysloc, &s_stbuf) < 0) {
		    close(*mfd);
		    continue;
		}
		/*
		**	sanity check: are the minor numbers the same??
		*/
		if (minor(m_stbuf.st_rdev) != minor(s_stbuf.st_rdev)) {
		    close(*mfd);  
		    continue;				/* try next num	*/
		}

		/*	else we got both a master and a slave pty	*/
		(void) chmod (ptysloc, 0622);		/* not readable */
		if (mname != (char *) 0)
		    (void) strcpy(mname, ptymloc);
		if (sname != (char *) 0)
		    (void) strcpy(sname, ptysloc);
		return 0;				/* return OK	*/
	    }
	}
	return -1;					/* error return */
}
