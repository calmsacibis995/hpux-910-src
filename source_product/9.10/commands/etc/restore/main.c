/* @(#)  $Revision: 70.4 $ */

/*
 * Copyright (c) 1983 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */

/*
 *	Modified to recursively extract all files within a subtree
 *	(supressed by the h option) and recreate the heirarchical
 *	structure of that subtree and move extracted files to their
 *	proper homes (supressed by the m option).
 *	Includes the s (skip files) option for use with multiple
 *	dumps on a single tape.
 *	8/29/80		by Mike Litzkow
 *
 *	Modified to work on the new file system and to recover from
 *	tape read errors.
 *	1/19/82		by Kirk McKusick
 *
 *	Full incremental restore running entirely in user code and
 *	interactive tape browser.
 *	1/19/83		by Kirk McKusick
 */

#include "restore.h"
#ifdef	hpux
#  include "dumprestore.h"
#else	hpux
#  include <protocols/dumprestore.h>
#endif	hpux
#include <signal.h>

int	bflag = 0, cvtflag = 0, dflag = 0, vflag = 0, yflag = 0;
int	hflag = 1, mflag = 1;
char	command = '\0';
long	dumpnum = 1;
long	volno = 0;
long	ntrec;
char	*dumpmap;
char	*clrimap;
ino_t	maxino;
time_t	dumptime;
time_t	dumpdate;
FILE 	*terminal;
/*
 *
 * Uncomment the following to enable debug printing.
 *
 * #define DEBUG 1
 *
 */

#ifdef DEBUG
   FILE	*dbgstream;
#endif

main(argc, argv)
	int argc;
	char *argv[];
{
	register char *cp;
	ino_t ino;
#ifdef	hpux
	char *inputdev = "/dev/rmt/0m";
	char *symtbl = "./restoresymtab";
#else	hpux
	char *inputdev = "/dev/rmt8";
	char *symtbl = "./restoresymtable";
#endif	hpux
	char name[MAXPATHLEN];
#ifdef  hpux
	void (*signal())();
	extern void onintr();
#else   hpux
	int (*signal())();
	extern int onintr();
#endif  hpux

#if defined(TRUX) && defined(B1)
	it_is_b1fs = 1;
	ie_init(argc, argv, 1); 
/* ie_init routine does some enablepriv, but I want forcepriv instead So here:*/

	if(!authorized_user("backup")) 	audit_rec(MSG_NOAUTH); 

	force_all_privs();
	disable_some_privs();

	if ((ir = mand_alloc_ir()) == (char *) 0)
		panic("error on mandatory ir_buffer allocate\n");
#endif /*(TRUX) && (B1) */

#ifdef DEBUG
/* Infinite loop */
	char forever;
	forever=TRUE;
	while(forever);
/* End of infinite loop */

	/* Open up debug output file */
	dbgstream= fopen("/testdsk/rest.debug","a+");
	
#endif

	if (signal(SIGINT, onintr) == SIG_IGN)
		(void) signal(SIGINT, SIG_IGN);
	if (signal(SIGTERM, onintr) == SIG_IGN)
		(void) signal(SIGTERM, SIG_IGN);
#ifdef	hpux
	setvbuf(stderr, NULL, _IOLBF, 0);
#else	hpux
	setlinebuf(stderr);
#endif	hpux
	if (argc < 2) {
usage:
		fprintf(stderr, "Usage:\n%s%s%s%s%s",
			"\trestore tfhsvy [file file ...]\n",
			"\trestore xfhmsvy [file file ...]\n",
			"\trestore ifhmsvy\n",
			"\trestore rfsvy\n",
			"\trestore Rfsvy\n");
		done(1);
	}
	argv++;
	argc -= 2;
	command = '\0';
	for (cp = *argv++; *cp; cp++) {
		switch (*cp) {
		case '-':
			break;
		case 'c':
#if defined(TRUX) && defined(B1)
	/* the conversion is not supported for the B1 version. 
	 * Currently this option is not even in the regular HPUX man pages!!
	 */
              	cvtflag = 0;
		fprintf(stderr, "Bad key character %c. Not supported on BLS\n", *cp);
		goto usage;
#else /* (TRUX) && (B1) */
			cvtflag++;
#endif /* (TRUX) && (B1) */
			break;
		case 'd':
			dflag++;
			break;
		case 'h':
			hflag = 0;
			break;
		case 'm':
			mflag = 0;
			break;
		case 'v':
			vflag++;
			break;
		case 'y':
			yflag++;
			break;
		case 'f':
			if (argc < 1) {
				fprintf(stderr, "missing device specifier\n");
				done(1);
			}
			inputdev = *argv++;
			argc--;
			break;
		case 'b':
			/*
			 * change default tape blocksize
			 */
			bflag++;
			if (argc < 1) {
				fprintf(stderr, "missing block size\n");
				done(1);
			}
			ntrec = atoi(*argv++);
			if (ntrec <= 0) {
				fprintf(stderr, "Block size must be a positive integer\n");
				done(1);
			}
			argc--;
			break;
		case 's':
			/*
			 * dumpnum (skip to) for multifile dump tapes
			 */
			if (argc < 1) {
				fprintf(stderr, "missing dump number\n");
				done(1);
			}
			dumpnum = atoi(*argv++);
			if (dumpnum <= 0) {
				fprintf(stderr, "Dump number must be a positive integer\n");
				done(1);
			}
			argc--;
			break;
		case 't':
		case 'R':
		case 'r':
		case 'x':
		case 'i':
			if (command != '\0') {
				fprintf(stderr,
					"%c and %c are mutually exclusive\n",
					*cp, command);
				goto usage;
			}
			command = *cp;
			break;
		default:
			fprintf(stderr, "Bad key character %c\n", *cp);
			goto usage;
		}
	}
	if (command == '\0') {
		fprintf(stderr, "must specify i, t, r, R, or x\n");
		goto usage;
	}
#if defined(TRUX) && defined(B1)
	fprintf(stderr,"\n*************************************************\n");
	fprintf(stderr,"This is an HP BLS (B Level security System)\n");
	fprintf(stderr,"*************************************************\n\n");
	(void) fflush(stderr);
#endif /* (TRUX) && (B1) */

	setinput(inputdev); /* set up an input source */
	if (argc == 0) {
		argc = 1;
		*--argv = ".";
	}
	switch (command) {
	/*
	 * Interactive mode.
	 */
	case 'i':
		setup(); /*Verifies access to tape and that it's a dump tape */
		extractdirs(1); /*Extract dir content. If 1, save mod/own/time*/
		initsymtable((char *)0); /*Init.a symbol table from file.0here*/
		runcmdshell(); /*Read and execute commands from the terminal*/
		done(0);
	/*
	 * Incremental restoration of a file system.
	 */
	case 'r':
		setup();
		if (dumptime > 0) {
			/*
			 * This is an incremental dump tape.
			 */
			vprintf(stdout, "Begin incremental restore\n");
			initsymtable(symtbl);
			extractdirs(1);
			removeoldleaves(); 
			vprintf(stdout, "Calculate node updates.\n");
			treescan(".", ROOTINO, nodeupdates);
			findunreflinks();
			removeoldnodes();
		} else {
			/*
			 * This is a level zero dump tape.
			 */
			vprintf(stdout, "Begin level 0 restore\n");
			initsymtable((char *)0);
			extractdirs(1);
			vprintf(stdout, "Calculate extraction list.\n");
			treescan(".", ROOTINO, nodeupdates);
		}
		createleaves(symtbl);
		createlinks();
		setdirmodes();
		checkrestore();
		if (dflag) {
			vprintf(stdout, "Verify the directory structure\n");
			treescan(".", ROOTINO, verifyfile);
		}
		dumpsymtable(symtbl, (long)1); /*dump snapshot of symbol table*/
		done(0); 		       /* Clean up and exit */
	/*
	 * Resume an incremental file system restoration.
	 */
	case 'R':
		initsymtable(symtbl);
		skipmaps();
		skipdirs(); /* calls skipfile which calls getfile */
		createleaves(symtbl);
		createlinks();
		setdirmodes();
		checkrestore();
		dumpsymtable(symtbl, (long)1);
		done(0);
	/*
	 * List contents of tape.
	 */
	case 't':
		setup();
		extractdirs(0);
		initsymtable((char *)0);
		while (argc--) {
			canon(*argv++, name);
			ino = dirlookup(name);
			if (ino == 0)
				continue;
			treescan(name, ino, listfile);
		}
		done(0);
	/*
	 * Batch extraction of tape contents.
	 */
	case 'x':
		setup();
		extractdirs(1);
		initsymtable((char *)0); 
		while (argc--) {
			canon(*argv++, name); /* Make name start with ./ &etc */
			ino = dirlookup(name);/* Check & see if name's on tape*/
			if (ino == 0)
				continue;
			if (mflag)
				pathcheck(name);/*Allof pathname elemnt exist?*/
			treescan(name, ino, addfile);
		}
		createfiles();
		createlinks();
		setdirmodes();
		if (dflag)
			checkrestore();
		done(0);
	}
}
