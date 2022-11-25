static char *HPUX_ID = "@(#) $Revision: 72.2 $";

/*
 * cat (Concatenate files).
 */

#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#ifdef NLS
#   define NL_SETN 1
#   include <nl_ctype.h>
#   include <locale.h>
#   include <nl_types.h>
    nl_catd nl_fn;
#else
#   define catgets(i,sn,mn,s) (s)
#endif

#ifndef _DBUFSIZ
#   define _DBUFSIZ 8192
#endif

char	buffer[_DBUFSIZ];

int	eflag		= 0;
int	silent		= 0;
int	tflag		= 0;
int	vflag		= 0;
int     unbuffered	= 0;

char    close_err[]  = /* catgets 6 */ "cat: close error -- ";
char    output_err[] = /* catgets 7 */ "cat: output error -- ";
char    read_err[]   = /* catgets 8 */ "cat: read error -- ";

main(argc, argv)
int argc;
char **argv;
{
    register FILE *fi;
    register int c;
    extern int optind;
    int errflg = 0;
    int stdinflg = 0;
    int status = 0;
    int dev, ino = -1;
    struct stat statb;

#ifdef NLS		/* initialize to the correct language */
    if (!setlocale(LC_ALL, ""))
    {
	fputs(_errlocale("cat"), stderr);
	nl_fn = (nl_catd)-1;
    }
    else
	nl_fn = catopen("cat", 0);
#endif /* NLS */

    while ((c = getopt(argc, argv, "estuv")) != EOF)
    {
	switch (c)
	{
	case 'e': 
	    eflag = 1;
	    continue;
	case 's': 
	    silent = 1;
	    continue;
	case 't': 
	    tflag = 1;
	    continue;
	case 'u': 
	    unbuffered = 1;
	    continue;
	case 'v': 
	    vflag = 1;
	    continue;
	case '?': 
	    errflg = 1;
	    break;
	}
	break;
    }

    if (errflg)
    {
	if (!silent)
	    fputs(catgets(nl_fn, NL_SETN, 1, "usage: cat [-su] [-v[-t][-e]] [-] [file] ...\n"), stderr);
	return 2;
    }

    if (fstat(fileno(stdout), &statb) < 0)
    {
	if (!silent)
	    fputs(catgets(nl_fn, NL_SETN, 2, "cat: Cannot stat stdout\n"), stderr);
	return 2;
    }

    statb.st_mode &= S_IFMT;
    if (statb.st_mode != S_IFCHR && statb.st_mode != S_IFBLK)
    {
	dev = statb.st_dev;
	ino = statb.st_ino;
    }

    if (optind == argc)
    {
	argc++;
	stdinflg++;
    }

    for (argv = &argv[optind];
	    optind < argc && !ferror(stdout); optind++, argv++)
    {
	if (stdinflg)
	    fi = stdin;
	else if ((*argv)[0] == '-' && (*argv)[1] == '\0')
	    fi = stdin;
	else
	{
	    if ((fi = fopen(*argv, "r")) == NULL)
	    {
		if (!silent)
		{
		    fputs(catgets(nl_fn, NL_SETN, 3, "cat: cannot open "), stderr);
		    perror(*argv);
		}
		status = 2;
		continue;
	    }
	}

	if (fstat(fileno(fi), &statb) < 0)
	{
	    if (!silent)
	    {
		fputs(catgets(nl_fn, NL_SETN, 4, "cat: cannot stat "), stderr);
		perror(*argv ? *argv : "-");
	    }
	    status = 2;
	    continue;
	}

	if (statb.st_dev == dev && statb.st_ino == ino)
	{
	    if (!silent)
		fprintf(stderr, (catgets(nl_fn, NL_SETN, 5, "cat: input %s is output\n")),
			stdinflg ? "-" : *argv);
	    if (fclose(fi) != 0)
		perror(catgets(nl_fn, NL_SETN, 6, close_err));
	    status = 2;
	    continue;
	}
	if (vflag)
	{
	    if(!status)
	    	status = vcat(fi);
	    else			
	    	(void) vcat(fi);
	}
	else
	{
	    if(!status)
	    	status = cat(fi);
	    else			
	    	(void) cat(fi);
	}
	if (fi != stdin)
	{
	    fflush(stdout);
	    if (fclose(fi) != 0)
	        if (!silent)
		    perror(catgets(nl_fn, NL_SETN, 6, close_err));
	}
    }

    fflush(stdout);
    if (ferror(stdout))
    {
	if (!silent)
	    perror(catgets(nl_fn, NL_SETN, 7, output_err));
	status = 2;
    }
    return status;
}

int
cat(fi)
FILE *fi;
{
    register int fd = fileno(fi);
    register int nitems;
    register int nread;
    register int i, j;

    do {
	if (unbuffered)
	    nitems = nread = read(fd, buffer, _DBUFSIZ);
	else
	{
	    nitems = 0;
	    do
	    {
		if ((nread = read(fd, buffer + nitems,
			    _DBUFSIZ - nitems)) > 0)
		    nitems += nread;
		else
		    break;
	    } while (nitems < _DBUFSIZ);
	}

	if (nread == -1)   /* report read error, and quit */
	{
	    if (!silent)
		perror(catgets(nl_fn, NL_SETN, 8, read_err));
	    return 2;
	}
	else               /* looks good so far, write buffer out */
	if (nitems > 0 && write(1, buffer, (unsigned)nitems) != nitems)
	{
	    if (!silent)
		perror(catgets(nl_fn, NL_SETN, 7, output_err));
	    return 2;
	}

    } while (nread > 0);

    return 0;
}

/* character type table */

#define PLAIN	0
#define CONTRL	1
#define TAB	2
#define BACKSP	3
#define NEWLIN	4

char ctype[128] = {
	CONTRL,	CONTRL,	CONTRL,	CONTRL,	CONTRL,	CONTRL,	CONTRL,	CONTRL,
	CONTRL, TAB,	NEWLIN,	CONTRL,	TAB,	CONTRL,	CONTRL,	CONTRL,
	CONTRL,	CONTRL,	CONTRL,	CONTRL,	CONTRL,	CONTRL,	CONTRL,	CONTRL,
	CONTRL,	CONTRL,	CONTRL,	CONTRL,	CONTRL,	CONTRL,	CONTRL,	CONTRL,
	PLAIN,	PLAIN,	PLAIN,	PLAIN,	PLAIN,	PLAIN,	PLAIN,	PLAIN,
	PLAIN,	PLAIN,	PLAIN,	PLAIN,	PLAIN,	PLAIN,	PLAIN,	PLAIN,
	PLAIN,	PLAIN,	PLAIN,	PLAIN,	PLAIN,	PLAIN,	PLAIN,	PLAIN,
	PLAIN,	PLAIN,	PLAIN,	PLAIN,	PLAIN,	PLAIN,	PLAIN,	PLAIN,
	PLAIN,	PLAIN,	PLAIN,	PLAIN,	PLAIN,	PLAIN,	PLAIN,	PLAIN,
	PLAIN,	PLAIN,	PLAIN,	PLAIN,	PLAIN,	PLAIN,	PLAIN,	PLAIN,
	PLAIN,	PLAIN,	PLAIN,	PLAIN,	PLAIN,	PLAIN,	PLAIN,	PLAIN,
	PLAIN,	PLAIN,	PLAIN,	PLAIN,	PLAIN,	PLAIN,	PLAIN,	PLAIN,
	PLAIN,	PLAIN,	PLAIN,	PLAIN,	PLAIN,	PLAIN,	PLAIN,	PLAIN,
	PLAIN,	PLAIN,	PLAIN,	PLAIN,	PLAIN,	PLAIN,	PLAIN,	PLAIN,
	PLAIN,	PLAIN,	PLAIN,	PLAIN,	PLAIN,	PLAIN,	PLAIN,	PLAIN,
	PLAIN,	PLAIN,	PLAIN,	PLAIN,	PLAIN,	PLAIN,	PLAIN,	CONTRL,
};

int
vcat(fi)
FILE *fi;
{
    register int c;
#if defined NLS && defined EUC
    register int cc;
#endif

    while ((c = getc(fi)) != EOF)
    {
	if (c & 0200)
	{
#ifdef NLS

#ifdef EUC
	/*  Check the valid character  */

			if (FIRSTof2(c)) {
				cc=getc(fi);
				if (SECof2(cc)) {
					putchar(c);
					putchar(cc);
					continue;
				}
				else {
					ungetc(cc,fi);
					putchar('M');
					putchar('-');
					c -= 0200;
				}
			}
			else {
				if (isprint(c)) {
					putchar(c);
					continue;
				}
				else {
					putchar('M');
					putchar('-');
					c -= 0200;
				}
			}
#else  /* EUC */
	    if (isprint(c) || FIRSTof2(c))
	    {
		putchar(c);
		continue;
	    }
	    else
	    {
		putchar('M');
		putchar('-');
		c -= 0200;
	    }
#endif /* EUC */

#else
	    putchar('M');
	    putchar('-');
	    c -= 0200;
#endif /* NLS */
	}

	switch (ctype[c])
	{
	case PLAIN: 
	    putchar(c);
	    break;
	case TAB: 
	    if (!tflag)
		putchar(c);
	    else
	    {
		putchar('^');
		putchar(c ^ 0100);
	    }
	    break;
	case CONTRL: 
	    putchar('^');
	    putchar(c ^ 0100);
	    break;
	case NEWLIN: 
	    if (eflag)
		putchar('$');
	    putchar(c);
	    break;
	}
    }
    return 0;
}
