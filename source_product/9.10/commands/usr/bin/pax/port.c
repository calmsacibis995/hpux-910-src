/* $Source: /misc/source_product/9.10/commands.rcs/usr/bin/pax/port.c,v $
 *
 * $Revision: 66.1 $
 *
 * port.c - These are routines not available in all environments. 
 *
 * DESCRIPTION
 *
 *	The routines contained in this file are provided for portability to
 *	other versions of UNIX or other operating systems.  Not all systems
 *	have the same functions or the same semantics, these routines
 *	attempt to bridge the gap as much as possible. 
 *
 * AUTHOR
 *
 *	Mark H. Colburn, Open Systems Architects, Inc. (mark@jhereg.mn.org)
 *	John Gilmore (gnu@hoptoad)
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
 * $Log:	port.c,v $
 * Revision 66.1  90/05/11  10:28:15  10:28:15  michas
 * inital checkin
 * 
 * Revision 2.0.0.4  89/12/16  10:35:54  mark
 * Changed the handling of function prototypes and declarations so that they
 * are easier to maintain.  The old ones were a royal pain.
 * 
 * Revision 2.0.0.3  89/10/13  02:35:42  mark
 * Beta Test Freeze
 * 
 */

#ifndef lint
static char        *ident = "$Id: port.c,v 2.0.0.4 89/12/16 10:35:54 mark Exp Locker: mark $";
static char        *copyright = "Copyright (c) 1989 Mark H. Colburn.\nAll rights reserved.\n";
#endif /* ! lint */


/* Headers */

#include "pax.h"


#ifndef MKDIR

/* mkdir - make a directory
 *
 * DESCRIPTION
 *
 * 	Mkdir will make a directory of the name "dpath" with a mode of
 *	"dmode".  This is consistent with the BSD mkdir() function and the
 *	P1003.1 definitions of MKDIR.
 *
 * PARAMETERS
 *
 *	dpath		- name of directory to create
 *	dmode		- mode of the directory
 *
 * RETURNS
 *
 *	Returns 0 if the directory was successfully created, otherwise a
 *	non-zero return value will be passed back to the calling function
 *	and the value of errno should reflect the error.
 */

int
mkdir(dpath, dmode)
    char               *dpath;	/* Directory to make */
    int                 dmode;	/* mode to be used for new directory */
{
    int                 cpid;
    int                 status;
    Stat                statbuf;
    extern int          errno;

    DBUG_ENTER("mkdir");
    if (STAT(dpath, &statbuf) == 0) {
	errno = EEXIST;		/* Stat worked, so it already exists */
	DBUG_RETURN(-1);
    }
    /* If stat fails for a reason other than non-existence, return error */
    if (errno != ENOENT)
	DBUG_RETURN(-1);

    switch (cpid = vfork()) {

    case -1:			/* Error in fork() */
	DBUG_RETURN(-1);	/* Errno is set already */

    case 0:			/* Child process */

	(void) umask(mask | (0777 & ~dmode));	/* Set for mkdir */
	execl("/bin/mkdir", "mkdir", dpath, (char *) 0);
	_exit(-1);		/* Can't exec /bin/mkdir */

    default:			/* Parent process */
	while (cpid != wait(&status)) {
	    /* Wait for child to finish */
	}
    }

    if (TERM_SIGNAL(status) != 0 || TERM_VALUE(status) != 0) {
	errno = EIO;		/* We don't know why, but */
	DBUG_RETURN(-1);	/* /bin/mkdir failed */
    }
    DBUG_RETURN(0);
}

#endif /* !MKDIR */


#ifndef RMDIR

/* rmdir - remove a directory
 *
 * DESCRIPTION
 *
 *	Rmdir will remove the directory specified by "dpath".  It is
 *	consistent with the BSD and POSIX rmdir functions.
 *
 * PARAMETERS
 *
 *	dpath		- name of directory to remove
 *
 * RETURNS
 *
 *	Returns 0 if the directory was successfully deleted, otherwise a
 *	non-zero return value will be passed back to the calling function
 *	and the value of errno should reflect the error.
 */

int
rmdir(dpath)
    char               *dpath;	/* directory to remove */
{
    int                 cpid,
                        status;
    Stat                statbuf;
    extern int          errno;

    DBUG_ENTER("rmdir");
    /* check to see if it exists */
    if (STAT(dpath, &statbuf) == -1) {
	DBUG_RETURN(-1);
    }
    switch (cpid = vfork()) {

    case -1:			/* Error in fork() */
	DBUG_RETURN(-1);	/* Errno is set already */

    case 0:			/* Child process */
	execl("/bin/rmdir", "rmdir", dpath, (char *) 0);
	_exit(-1);		/* Can't exec /bin/rmdir */

    default:			/* Parent process */
	while (cpid != wait(&status)) {
	    /* Wait for child to finish */
	}
    }

    if (TERM_SIGNAL(status) != 0 || TERM_VALUE(status) != 0) {
	errno = EIO;		/* We don't know why, but */
	DBUG_RETURN(-1);	/* /bin/rmdir failed */
    }
    DBUG_RETURN(0);
}

#endif /* !RMDIR */


#ifndef STRERROR

/* strerror - return pointer to appropriate system error message
 *
 * DESCRIPTION
 *
 *	Get an error message string which is appropriate for the setting
 *	of the errno variable.
 *
 * RETURNS
 *
 *	Returns a pointer to a string which has an appropriate error
 *	message for the present value of errno.  The error message
 *	strings are taken from sys_errlist[] where appropriate.  If an
 *	appropriate message is not available in sys_errlist, then a
 *	pointer to the string "Unknown error (errno <errvalue>)" is 
 *	returned instead.
 */

char *
strerror(errnum)
    int			errnum;
{
    static char         msg[40];/* used for "Unknown error" messages */

    DBUG_ENTER("strerror");
    if (errnum > 0 && errnum < sys_nerr) {
	DBUG_RETURN(sys_errlist[errnum]);
    }
    sprintf(msg, "Unknown error (errno %d)", errnum);
    DBUG_RETURN(msg);
}

#endif /* !STRERROR */


#ifndef GETOPT

/*
 * getopt - parse command line options
 *
 * DESCRIPTION
 *
 *	This is a slightly modified version of the AT&T Public Domain
 *	getopt().  It is included for those systems that do not already
 *	have getopt() available in their C libraries.
 */

#define ERR(s, c)	if(opterr){\
	extern int write();\
	char errbuf[2];\
	errbuf[0] = c; errbuf[1] = '\n';\
	(void) write(2, argv[0], (unsigned)strlen(argv[0]));\
	(void) write(2, s, (unsigned)strlen(s));\
	(void) write(2, errbuf, 2);}

int                 opterr = 1;
int                 optind = 1;
int                 optopt;
char               *optarg;

int
getopt(argc, argv, opts)
    int                 argc;
    char              **argv;
    char               *opts;
{
    static int          sp = 1;
    register int        c;
    register char      *cp;

    DBUG_ENTER("getopt");
    if (sp == 1)
	if (optind >= argc ||
	    argv[optind][0] != '-' || argv[optind][1] == '\0')
	    DBUG_RETURN(EOF);
	else if (strcmp(argv[optind], "--") == NULL) {
	    optind++;
	    DBUG_RETURN(EOF);
	}
    optopt = c = argv[optind][sp];
    if (c == ':' || (cp = index(opts, c)) == NULL) {
	ERR(": illegal option -- ", c);
	if (argv[optind][++sp] == '\0') {
	    optind++;
	    sp = 1;
	}
	DBUG_RETURN('?');
    }
    if (*++cp == ':') {
	if (argv[optind][sp + 1] != '\0')
	    optarg = &argv[optind++][sp + 1];
	else if (++optind >= argc) {
	    ERR(": option requires an argument -- ", c);
	    sp = 1;
	    DBUG_RETURN('?');
	} else
	    optarg = argv[optind++];
	sp = 1;
    } else {
	if (argv[optind][++sp] == '\0') {
	    sp = 1;
	    optind++;
	}
	optarg = NULL;
    }
    DBUG_RETURN(c);
}

#endif /* !GETOPT */


#ifndef MEMCPY

/*
 * memcpy - copy a block of memory from one place to another
 *
 * DESCRIPTION
 *
 *	This is an implementation of the System V/ANSI memcpy() function.
 *	Although Berkeley has bcopy which could be used here, use of this
 *	function does make the source more ANSI compliant.  This function
 *	is only used if it is not in the C library.
 */

char *
memcpy(s1, s2, n)
    char               *s1;	/* destination block */
    char               *s2;	/* source block */
    unsigned int        n;	/* number of bytes to copy */
{
    char               *p;

    DBUG_ENTER("memcpy");
    p = s1;
    while (n--) {
	*(unsigned char) s1++ = *(unsigned char) s2++;
    }
    DBUG_RETURN(p);
}

#endif /* !MEMCPY */


#ifndef MEMSET
/*
 * memset - set a block of memory to a particular value
 *
 * DESCRIPTION
 *
 *	This is an implementation of the System V/ANSI memset function.
 *	Although Berkeley has the bzero() function, which is often what
 *	memset() is used for, the memset function is more general.  This
 *	function is only used if it is not in the C library.
 */

char *
memset(s1, c, n)
    char               *s1;	/* pointer to buffer to fill */
    int                 c;	/* character to fill buffer with */
    unsigned int        n;	/* number of characters to fill */
{
    char               *p;

    DBUG_ENTER("memset");
    p = s1;
    while (n--) {
	*(unsigned char) s1++ = (unsigned char) c;
    }
    DBUG_RETURN(p);
}

#endif /* !MEMSET */
