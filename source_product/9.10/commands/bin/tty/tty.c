static char *HPUX_ID = "@(#) $Revision: 66.2 $";
/*
 * Type tty or pty name
 */

#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/sysmacros.h>      /* for major/minor macro's for pty check */

/* macros for isapty() */
#ifdef hp9000s500
#define SPTYMAJOR 29            /* major number for slave  pty's */
#define PTYSC     0xfe          /* select code for pty's         */
#define select_code(x) (((x) & 0xff0000) >> 16)
#endif
#if defined(hp9000s200) || defined(hp9000s800)
#define SPTYMAJOR 17            /* major number for slave  pty's */
#define PTYSC     0x00          /* select code for pty's         */
#define select_code(x) (((x) & 0xff0000) >> 16)
#endif

extern char *ttyname(), *strstr();
extern void exit();
char    *ptyname();

extern  char    *optarg;                /* for getopt */
extern  int     optind, opterr;         /* for getopt */

main(argc, argv)
char **argv;
{
	register char *p, *p1, *p2;
	register int ptycmd, sflag=0;
	register int c;			/* option character for getopt */
/* we will do pty as long as the basename is pty */
        p2 = p1 = argv[0];
	while (*p1) {
	    if (*p1++ == '/')
                     p2 = p1;
        }
/* Use 7.0 routine strstr to replace strcmp */
	ptycmd = strstr(p2, "pty") != NULL;
	if (ptycmd)
	    p = ptyname(0);
	else
	    p = ttyname(0);

/*
 * Parse options, using getopt.
 */
	while ((c=getopt(argc,argv, "s"))!=EOF) {
		switch(c) {
			case 's': sflag++; break;
			case '?': exit(2);	/* invalid option */
		}
	} /* while (c=getopt(argc,argv,)) */
	if (optind != argc)		/* unknown arguments? */
		exit(2);

	if (!sflag)
		puts(p ? p :
			  ptycmd ? "not a pty" : "not a tty");
	return p ? 0 : 1;
}

char *ptyname(fd)
int fd;
{
    struct stat sbuf;
    char *name;

    name = ttyname(fd);
    if (  (name != (char *) 0) &&
	  (fstat(fd, &sbuf) == 0) &&
	  ((sbuf.st_mode & S_IFMT) == S_IFCHR) &&
	  (major(sbuf.st_rdev) == SPTYMAJOR) &&
	  (select_code(sbuf.st_rdev) == PTYSC)    )
	return(name);               /* passed all tests */
    else
	return( (char *) 0 );       /* failed */
}
