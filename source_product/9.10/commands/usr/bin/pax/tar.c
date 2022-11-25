/* $Source: /misc/source_product/9.10/commands.rcs/usr/bin/pax/tar.c,v $
 *
 * $Revision: 66.1 $
 *
 * tar.c - tar specific functions for archive handling
 *
 * DESCRIPTION
 *
 *	These routines provide a tar conforming interface to the pax
 *	program.
 *
 * AUTHOR
 *
 *	Mark H. Colburn, Open Systems Architects, Inc. (mark@osa.com)
 *
 * COPYRIGHT
 *
 *	Copyright (c) 1989 Mark H. Colburn.  All rights reserved.
 *
 *	Redistribution and use in source and binary forms are permitted
 *	provided that the above copyright notice and this paragraph are
 *	duplicated in all such forms and that any documentation,
 *	advertising materials, and other materials related to such
 *	distribution and use acknowledge that the software was developed
 *	by Mark H. Colburn.
 *
 *	THIS SOFTWARE IS PROVIDED ``AS IS'' AND WITHOUT ANY EXPRESS OR
 *	IMPLIED WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED
 *	WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 *
 * $Log:	tar.c,v $
 * Revision 66.1  90/05/11  10:38:56  10:38:56  michas
 * inital checkin
 * 
 * Revision 2.0.0.5  89/12/16  10:36:09  mark
 * Changed the handling of function prototypes and declarations so that they
 * are easier to maintain.  The old ones were a royal pain.
 * 
 * Revision 2.0.0.4  89/10/13  02:35:57  mark
 * Beta Test Freeze
 * 
 */

#ifndef lint
static char        *ident = "$Id: tar.c,v 2.0.0.5 89/12/16 10:36:09 mark Exp Locker: mark $";
static char        *copyright = "Copyright (c) 1989 Mark H. Colburn.\nAll rights reserved.";
#endif /* not lint */

/* Headers */

#include "pax.h"


/* Defines */

#define DEF_BLOCKING	20	/* default blocking factor for extract */


/* Function Prototypes */

#ifdef __STDC__
#define P(x)    x
#else
#define P(x)	()
#endif

static int	    taropt P((int, char **, char *));
static void 	    usage P((void));

#undef P


/* do_tar - main routine for tar. 
 *
 * DESCRIPTION
 *
 *	Provides a tar interface to the PAX program.  All tar standard
 *	command line options are supported.
 *
 * PARAMETERS
 *
 *	int argc	- argument count (argc from main) 
 *	char **argv	- argument list (argv from main) 
 *
 * RETURNS
 *
 *	zero
 */

int
do_tar(argc, argv)
    int                 argc;	/* argument count (argc from main) */
    char              **argv;	/* argument list (argv from main) */
{
    int                 c;	/* Option letter */

    DBUG_ENTER("do_tar");
    /* Set default option values */
    names_from_stdin = 0;
    ar_file = getenv("TAPE");	/* From environment, or */
    if (ar_file == NULL) {	/* from Makefile */
	ar_file = DEF_TAR_FILE;
    }
    /*
     * set up the flags to reflect the default pax inteface.  Unfortunately
     * the pax interface has several options which are completely opposite of
     * the tar and/or cpio interfaces... 
     */
    f_unconditional = 1;
    f_mtime = 1;
    f_dir_create = 1;
    blocking = 0;
    ar_interface = TAR;
    ar_format = TAR;
    msgfile = stderr;

    /* Parse options */
    while ((c = taropt(argc, argv, "#:b:cf:hlmortuvwx")) != EOF) {
	switch (c) {

	case '#':
	    DBUG_PUSH(optarg);
	    break;

	case 'b':		/* specify blocking factor */
	    /*
	     * FIXME - we should use a conversion routine that does some kind
	     * of reasonable error checking, but... 
	     */
	    blocking = atoi(optarg);
	    break;

	case 'c':		/* create a new archive */
	    f_create = 1;
	    break;

	case 'f':		/* specify input/output file */
	    ar_file = optarg;
	    break;

	case 'h':
	    f_follow_links = 1;	/* follow symbolic links */
	    break;

	case 'l':		/* report unresolved links */
	    f_unresolved = 1;
	    break;

	case 'm':		/* don't restore modification times */
	    f_modified = 1;
	    break;

	case 'o':		/* take on user's group rather than archives */
	    break;

	case 'r':		/* named files are appended to archive */
	    f_append = 1;
	    break;

	case 't':
	    f_list = 1;		/* list files in archive */
	    break;

	case 'u':		/* named files are added to archive */
	    f_newer = 1;
	    break;

	case 'v':		/* verbose mode */
	    f_verbose = 1;
	    break;

	case 'w':		/* user interactive mode */
	    f_disposition = 1;
	    break;

	case 'x':		/* named files are extracted from archive */
	    f_extract = 1;
	    break;

	case '?':
	    usage();
	    exit(EX_ARGSBAD);
	}
    }

#ifdef MSDOS
    setmode(fileno(msgfile), O_TEXT);
    if (names_from_stdin) {
        setmode(fileno(stdin), O_TEXT);
    }
#endif /* MSDOS */

    /* check command line argument sanity */
    if (f_create + f_extract + f_list + f_append + f_newer != 1) {
	(void) fprintf(stderr,
	"%s: you must specify exactly one of the c, t, r, u or x options\n",
		       myname);
	usage();
	exit(EX_ARGSBAD);
    }
    /* set the blocking factor, if not set by the user */
    if (blocking == 0) {
	if (f_extract || f_list) {
	    blocking = DEF_BLOCKING;
	    fprintf(stderr, "Tar: blocksize = %d\n", blocking);
	} else {
	    blocking = 1;
	}
    }
    blocksize = blocking * BLOCKSIZE;
    buf_allocate((OFFSET) blocksize);

    if (f_create) {
	open_archive(AR_WRITE);
	create_archive();	/* create the archive */
    } else if (f_extract) {
	open_archive(AR_READ);
	read_archive();		/* extract files from archive */
    } else if (f_list) {
	open_archive(AR_READ);
	read_archive();		/* read and list contents of archive */
    } else if (f_append) {
	open_archive(AR_APPEND);
	append_archive();	/* append files to archive */
    }
    if (f_unresolved) {
	linkleft();		/* report any unresolved links */
    }
    DBUG_RETURN(0);
}


/* taropt -  tar specific getopt
 *
 * DESCRIPTION
 *
 * 	Plug-compatible replacement for getopt() for parsing tar-like
 * 	arguments.  If the first argument begins with "-", it uses getopt;
 * 	otherwise, it uses the old rules used by tar, dump, and ps.
 *
 * PARAMETERS
 *
 *	int argc	- argument count (argc from main) 
 *	char **argv	- argument list (argv from main) 
 *	char *optstring	- sring which describes allowable options
 *
 * RETURNS
 *
 *	Returns the next option character in the option string(s).  If the
 *	option requires an argument and an argument was given, the argument
 *	is pointed to by "optarg".  If no option character was found,
 *	returns an EOF.
 *
 */

static int
taropt(argc, argv, optstring)
    int                 argc;	/* argument count from cmd line */
    char              **argv;	/* argument vector from cmd line */
    char               *optstring;	/* option string ala getopt */
{
    extern char        *optarg;	/* Points to next arg */
    extern int          optind;	/* Global argv index */
    static char        *key;	/* Points to next keyletter */
    static char         use_getopt;	/* !=0 if argv[1][0] was '-' */
    char                c;
    char               *place;

    DBUG_ENTER("taropt");
    optarg = (char *) NULL;

    if (key == (char *) NULL) {	/* First time */
	if (argc < 2) {
	    DBUG_RETURN(EOF);
	}
	key = argv[1];
	if (*key == '-') {
	    use_getopt++;
	} else {
	    optind = 2;
	}
	
    }
    if (use_getopt) {
	DBUG_RETURN(getopt(argc, argv, optstring));
    }
    c = *key++;
    if (c == '\0') {
	key--;
	DBUG_RETURN(EOF);
    }
    place = index(optstring, c);

    if (place == (char *) NULL || c == ':') {
	fprintf(stderr, "%s: unknown option %c\n", argv[0], c);
	DBUG_RETURN('?');
    }
    place++;
    if (*place == ':') {
	if (optind < argc) {
	    optarg = argv[optind];
	    optind++;
	} else {
	    fprintf(stderr, "%s: %c argument missing\n", argv[0], c);
	    DBUG_RETURN('?');
	}
    }
    DBUG_RETURN(c);
}


/* usage - print a helpful message and exit
 *
 * DESCRIPTION
 *
 *	Usage prints out the usage message for the TAR interface and then
 *	exits with a non-zero termination status.  This is used when a user
 *	has provided non-existant or incompatible command line arguments.
 *
 * RETURNS
 *
 *	Returns an exit status of 1 to the parent process.
 *
 */

static void
usage()
{
    DBUG_ENTER("usage");
    fprintf(stderr, "Usage: %s -c[bfvw] device block filename..\n", myname);
    fprintf(stderr, "       %s -r[bvw] device block [filename...]\n", myname);
    fprintf(stderr, "       %s -t[vf] device\n", myname);
    fprintf(stderr, "       %s -u[bvw] device block [filename...]\n", myname);
    fprintf(stderr, "       %s -x[flmovw] device [filename...]\n", myname);
    exit(1);
}
