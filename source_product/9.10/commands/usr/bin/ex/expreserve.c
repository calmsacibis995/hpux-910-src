static char *HPUX_ID = "@(#) $Revision: 70.1 $";
/* Copyright (c) 1981 Regents of the University of California */
#include <sys/param.h>
#include <stdio.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <ndir.h>
#include <pwd.h>
#include <stdlib.h>
#include <unistd.h>
#include "local/uparm.h"
#include "ex.h"
#ifdef TRUX
#include <sys/security.h>
#endif
				/* mjm: "/tmp" --> TMP */

#define TMP	"/tmp"

#ifdef VMUNIX
#define	HBLKS	3
#else
#define HBLKS	2
#endif

char xstr[1];			/* make loader happy */

/*
 * Expreserve - preserve a file in usrpath(preserve)
 * Bill Joy UCB November 13, 1977
 *
 * This routine is very naive - it doesn't remove anything from
 * usrpath(preserve)... this may mean that we  * stuff there... the danger in doing anything with usrpath(preserve)
 * is that the clock may be screwed up and we may get confused.
 *
 * We are called in three ways - first from the editor with no argumentss
 * and the standard input open on the temp file. Second with an argument
 * to preserve the entire contents of /tmp (root only).  Third by a user
 * from a Cnode.  In this case, expreserve will determine if the environment
 * variable TMPDIR is set and change to the appropriate directory.
 *
 * BUG: should do something about preserving Rx... (register contents)
 *      temporaries.
 */

#ifndef VMUNIX
#define	LBLKS	125
#else
#define	LBLKS	900
#endif
#define	FNSIZE	MAXPATHLEN

struct 	header {
	time_t	Time;			/* Time temp file last updated */
	int	Uid;			/* This users identity */
#ifndef VMUNIX
	short	Flines;			/* Number of lines in file */
#else
	int	Flines;
#endif
	char	Savedfile[FNSIZE];	/* The current file name */
	short	Blocks[LBLKS];		/* Blocks where line pointers stashed */
} H;

#ifdef	lint
#define	ignore(a)	Ignore(a)
#define	ignorl(a)	Ignorl(a)
#else
#define	ignore(a)	a
#define	ignorl(a)	a
#endif

struct	passwd *getpwuid();
off_t	lseek();
FILE	*popen();


#define eq(a, b) strcmp(a, b) == 0

#ifndef NONLS8 /* User messages */
# define	NL_SETN	1	/* set number */
# include	<msgbuf.h>
#else NONLS8
# define	nl_msg(i, s)	(s)
#endif NONLS8

main(argc, argv, environ)
	int argc;
	char **argv;
	char **environ;
{
	DIR *tf;
	struct direct *dirent;
	struct stat stbuf;
	char *tmp_dir = NULL;  /* Store the TMPDIR environment variable */
        uid_t   user_id;

#ifdef SecureWare
#ifdef B1
	if(ISB1)
		expreserve_setup(argc, argv);
#endif B1
#else SecureWare
        cleanenv( &environ, "LANG", "LANGOPTS", "NLSPATH", "TMPDIR", 0 );
#endif SecureWare

#ifndef NONLS8 /* User messages */
	nl_catopen("expreserve");
#endif NONLS8



	/*
	 * Within vi, fd0 can be assigned to a regular file and then
	 * expreserve is called to save the stdin.
	 * If only one argument and file descriptor 0 is not a tty, 
	 * then preserve the standard input.
	 */
	if ((argc == 1) && !(isatty(0)))
	{
		if (copyout((char *) 0))
			exit(1);
		close(1);
		sync();
		exit(0);
	}

	user_id = getuid(); /* obtain the UID */

#if defined(SecureWare) && defined(B1)
	if(ISB1)
	    expreserved_check_auths();
	else{
	    if (user_id)  
	        tmp_dir = getenv("TMPDIR");
	}
#else 
       /*
        * if not super user see if the TMPDIR environment variable is set 
	* and obtain a pointer to the passwd structure for this user.
	*/
	if (user_id)  
	    tmp_dir = getenv("TMPDIR");
	
#endif

	/*
	 *  If super-user, preserve and remove all applicable files in /tmp.
	 *  If not super-user, deal with only that users files which may be
	 *  be located in /tmp or in TMPDIR.
	 */

        if (tmp_dir)
	{
	    if (chdir(tmp_dir) < 0)
	    {
		perror(tmp_dir);
		exit(1);
            }		
        }
	else
	    if (chdir(TMP) < 0) {
	    	    perror(TMP);
		    exit(1);
	    }

#if defined(SecureWare) && defined(B1)
	if((ISB1) && (expreserve_full_copy(TMP, "Ex")))
		exit(0);
#endif
	if ((tf = opendir(".")) == NULL) {
		if (tmp_dir)
		    perror(tmp_dir);
		else
		    perror(TMP);
		exit(1);
	}
	while ((dirent = readdir(tf)) != NULL ) {
		if (dirent->d_ino == 0)
			continue;
		/*
		 * Ex temporaries must begin with Ex;
		 * we check that the 10th character of the name is null
		 * so we won't have to worry about non-null terminated names
		 * later on.
		 * (UCSqm00104): Instead of checking the 10th char for non-null,
		 *               check only the terminating char.
		 */
		if (dirent->d_name[0] != 'E' || dirent->d_name[1] != 'x' || dirent->d_name[dirent->d_namlen])
			continue;
		if (stat(dirent->d_name, &stbuf))
			continue;
		if (user_id && (user_id != stbuf.st_uid))
			continue;
		if ((stbuf.st_mode & S_IFMT) != S_IFREG)
			continue;
		/*
		 * Save the bastard.
		 */
	        ignore(copyout(dirent->d_name));
	}
	exit(0);
}

#if defined(SecureWare) && defined(B1)
char    *pattern  =     usrpath(preserve/Exaa`XXXXX);
#else
char	pattern[] =	usrpath(preserve/Exaa`XXXXX);
#endif

/*
 * Copy file name into usrpath(preserve)/...
 * If name is (char *) 0, then do the standard input.
 * We make some checks on the input to make sure it is
 * really an editor temporary, generate a name for the
 * file (this is the slowest thing since we must stat
 * to find a unique name), and finally copy the file.
 */
copyout(name)
	char *name;
{
	int i;
	static int reenter;
	char buf[BUFSIZ];

	/*
	 * The first time we put in the digits of our
	 * process number at the end of the pattern.
	 */
	if (reenter == 0) {
		mkdigits(pattern);
		reenter++;
	}

	/*
	 * If a file name was given, make it the standard
	 * input if possible.
	 */
	if (name != 0) {
		ignore(close(0));
		/*
		 * Need read/write access for arcane reasons
		 * (see below).
		 */
		if (open(name, 2) < 0)
			return (-1);
	}

	/*
	 * Get the header block.
	 */
	ignorl(lseek(0, 0l, 0));
	if (read(0, (char *) &H, sizeof H) != sizeof H) {
format:
		if (name == 0)
			fprintf(stderr, "Buffer format error\t");
		return (-1);
	}

	/*
	 * Consistency checsks so we don't copy out garbage.
	 */
	if (H.Flines < 0) {
#ifdef DEBUG
		fprintf(stderr, "Negative number of lines\n");
#endif
		goto format;
	}
	if (H.Blocks[0] != HBLKS || H.Blocks[1] != HBLKS+1) {
#ifdef DEBUG
		fprintf(stderr, "Blocks %d %d\n", H.Blocks[0], H.Blocks[1]);
#endif
		goto format;
	}
	if (name == 0 && H.Uid != getuid()) {
#ifdef DEBUG
		fprintf(stderr, "Wrong user-id\n");
#endif
		goto format;
	}
	if (lseek(0, 0l, 0)) {
#ifdef DEBUG
		fprintf(stderr, "Negative number of lines\n");
#endif
		goto format;
	}

	/*
	 * If no name was assigned to the file, then give it the name
	 * LOST, by putting this in the header.
	 */
	if (H.Savedfile[0] == 0) {
		strcpy(H.Savedfile, "LOST");
		ignore(write(0, (char *) &H, sizeof H));
		H.Savedfile[0] = 0;
		lseek(0, 0l, 0);
	}

	/*
	 * File is good.  Get a name and create a file for the copy.
	 */
	mknext(pattern);
	ignore(close(1));
#if defined(SecureWare) && defined(B1)
	if((ISB1) ? (expreserve_create_file(name, pattern, H.Uid) < 0) :
	            (creat(pattern, 0600) < 0))
#else
	if (creat(pattern, 0600) < 0)
#endif
	{
		if (name == 0)
			perror(pattern);
		return (1);
	}

	/*
	 * Make the target be owned by the owner of the file.
	 */
	ignore(chown(pattern, H.Uid, 0));

	/*
	 * Copy the file.
	 */
	for (;;) {
		i = read(0, buf, BUFSIZ);
		if (i < 0) {
			if (name)
				perror((nl_msg(2, "Buffer read error")));
			ignore(unlink(pattern));
			return (-1);
		}
		if (i == 0) {
			if (name)
				ignore(unlink(name));
#if defined(SecureWare) && defined(B1)
			if(ISB1)
			    expreserve_notify(H.Uid, H.Savedfile, (int) name);
			else
			    notify(H.Uid, H.Savedfile, (int) name);
#else
			notify(H.Uid, H.Savedfile, (int) name);
#endif
			return (0);
		}
		if (write(1, buf, i) != i) {
			if (name == 0)
				perror(pattern);
			unlink(pattern);
			return (-1);
		}
	}
}

/*
 * Blast the last 5 characters of cp to be the process number.
 */
mkdigits(cp)
	char *cp;
{
	register int i, j;

	for (i = getpid(), j = 5, cp += strlen(cp); j > 0; i /= 10, j--)
		*--cp = i % 10 | '0';
}

/*
 * Make the name in cp be unique by clobbering up to
 * three alphabetic characters into a sequence of the form 'aab', 'aac', etc.
 * Mktemp gets weird names too quickly to be useful here.
 */
mknext(cp)
	char *cp;
{
	char *dcp;
	struct stat stb;

	dcp = cp + strlen(cp) - 1;
	while ((*dcp >= IS_MACRO_LOW_BOUND) && isdigit(*dcp))
		dcp--;
whoops:
	if (dcp[0] == 'z') {
		dcp[0] = 'a';
		if (dcp[-1] == 'z') {
			dcp[-1] = 'a';
			if (dcp[-2] == 'z')
				fprintf(stderr, (nl_msg(3, "Can't find a name\t")));
			dcp[-2]++;
		} else
			dcp[-1]++;
	} else
		dcp[0]++;
	if (stat(cp, &stb) == 0)
		goto whoops;
}

/*
 * Notify user uid that his file fname has been saved.
 */
notify(uid, fname, flag)
	int uid;
	char *fname;
{
	struct passwd *pp = getpwuid(uid);
	register FILE *mf;
	char cmd[BUFSIZ];

	if (pp == NULL)
		return;

	/*
	** close security hole - if IFS contains a "/" then we will execute
	** "bin" instead of "/bin/mail"
	*/
	putenv("IFS= \t\n");

	sprintf(cmd, "/bin/mail %s", pp->pw_name);
	mf = popen(cmd, "w");
	if (mf == NULL) {
		/* just in case */
		sprintf(cmd, "/usr/bin/mail %s", pp->pw_name);
		mf = popen(cmd, "w");
		if (mf == NULL)
			return;
	}
	setbuf(mf, cmd);

	if (fname[0] == 0) {
#ifndef NONLS8 /* User messages */
		if (flag)
			fprintf(mf, (nl_msg(4, "A copy of an editor buffer of yours was saved when the system went down.\n")));
		else
			fprintf(mf, (nl_msg(5, "A copy of an editor buffer of yours was saved when the editor was killed.\n")));
		fprintf(mf, (nl_msg(6, "No name was associated with this buffer so it has been named \"LOST\".\n")));
	} else {
		if (flag)
			fprintf(mf, (nl_msg(7, "A copy of an editor buffer of your file \"%s\"\nwas saved when the system went down.\n")), fname);
		else
			fprintf(mf, (nl_msg(8, "A copy of an editor buffer of your file \"%s\"\nwas saved when the editor was killed.\n")), fname);
	}
#else NONLS8
		fprintf(mf,
"A copy of an editor buffer of yours was saved when %s.\n",
		flag ? "the system went down" : "the editor was killed");
		fprintf(mf,
"No name was associated with this buffer so it has been named \"LOST\".\n");
	} else
		fprintf(mf,
"A copy of an editor buffer of your file \"%s\"\nwas saved when %s.\n", fname,
		/*
		 * "the editor was killed" is perhaps still not an ideal
		 * error message.  Usually, either it was forcably terminated
		 * or the phone was hung up, but we don't know which.
		 */
		flag ? "the system went down" : "the editor was killed");
#endif NONLS8

	fprintf(mf,
(nl_msg(9, "This buffer can be retrieved using the \"recover\" command of the editor.\n")));
	fprintf(mf,
(nl_msg(10, "An easy way to do this is to give the command \"ex -r %s\".\n")),
fname[0]? fname: "LOST");
	fprintf(mf,
(nl_msg(11, "This works for \"edit\" and \"vi\" also.\n")));
	pclose(mf);
}

/*
 *	people making love
 *	never exactly the same
 *	just like a snowflake 
 */

#ifdef lint
Ignore(a)
	int a;
{

	a = a;
}

Ignorl(a)
	long a;
{

	a = a;
}
#endif
