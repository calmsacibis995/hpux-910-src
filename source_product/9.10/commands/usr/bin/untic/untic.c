static char *HPUX_ID = "@(#) $Revision: 27.2 $";
/*
 * untic(1M): de-compile a compiled terminfo(5) entry
 */


#include <stdio.h>
#include <unctrl.h>
#include <fcntl.h>

#define termpath(file) "/usr/lib/terminfo/file"
#define MAGNUM 0432	/* tic magic number */

extern char *boolnames[], *numnames[], *strnames[];
char *getenv(), *malloc();
char ttytype[128];



#define getshi()	getsh(ip) ; ip += (sizeof(short)/sizeof(char))

/*
 * "function" to get a short from a pointer.  The short is in a standard
 * format: two bytes, the first is the low order byte, the second is
 * the high order byte (base 256).  The only negative number allowed is
 * -1, which is represented as 255, 255.  This format happens to be the
 * same as the hardware on the pdp-11 and vax, making it fast and
 * convenient and small to do this on a pdp-11.
 *
 * Here is a more portable version, which does not assume byte ordering
 * in shorts, sign extension, etc.
 */
	short
getsh(p)
    register char *p;
{
    register int rv, sv;
    rv = *((unsigned char *) p++);
    sv = *((unsigned char *) p);
    if (rv == 0377)
	if (sv == 0377)
	    return -1;
    return rv + (sv * 256);
}


main(argc,argv)
    int argc;
    char *argv[];
{
    char tiebuf[4096];	/* buffer for compiled terminfo file */
    int n,		/* number of bytes read from terminfo file */
	tfd;		/* terminfo-file file descriptor */
    short magic, snames, nbools, nints, nstrs, sstrtab;
    char *strtab,
	 **strptrs,	/* pointers to string capability values */
	 *term;		/* name of terminal type */
    register char *ip;
    register char *cp;


    switch (argc) {
	case 1:
	    term = getenv("TERM");
	    if (term == (char *)0) {
		fprintf(stderr,"untic: no value for the TERM variable\n");
		exit(1);
	    }
	    tfd = getfd(term);
	    break;

	case 2:
	    tfd = getfd(argv[1]);
	    break;

	case 3:
	    if ( strcmp(argv[1],"-f") != 0 ) {
		fprintf(stderr,"Usage: untic [term] [ -f file ]\n");
		exit(1);
	    }
	    tfd = open(argv[2], O_RDONLY);
	    if ( tfd < 0 ) {
		perror("untic");
		exit(1);
	    }
	    break;

	default:
	    fprintf(stderr,"Usage: untic [term] [ -f file ]\n");
	    exit(1);
    }

    n = read(tfd, tiebuf, sizeof tiebuf);
    close(tfd);

    if (n <= 0) {
	fprintf(stderr,"untic: corrupted term entry\n");
	exit(1);
    }

    if (n == sizeof tiebuf) {
	fprintf(stderr,"untic: term entry too long\n");
	exit(1);
    }

    ip = tiebuf;

    /*	The compiled terminfo entry has the following structure:
     *
     * 1.	Header: (length is 12 bytes)
     *		-- terminfo magic number
     *		-- # bytes for terminal name(s) (snames)
     *		-- # boolean entries (nbools)
     *		-- # numeric entries (nints)
     *		-- # string entries (nstrs)
     *		-- # length of all string entries combined (sstrtab)
     *
     * 2.	Terminal names: (length is snames)
     *
     * 3.	Boolean entries: (length is nbools)
     *
     * 4.	Numeric entries: (length is nints*2)
     *
     * 5.	String entries: (length is nstrs*2)
     *		-- each entry gives the byte offset (index) of the
     *		   string capability in the next section
     *
     * 6.	Actual string entries: (length is sstrtab)
     *		-- indexed by offsets given in previous section
     */
	
    /* Pick up header (1): */
    magic = getshi();
    if (magic != MAGNUM) { /* check the magic number */
	fprintf(stderr,"untic: corrupted term entry\n");
	exit(1);
    }
    snames = getshi();	/* # bytes for terminal name(s) */
    nbools = getshi();	/* # boolean entries */
    nints = getshi();	/* # of numeric entries */
    nstrs = getshi();	/* # of string entries */
    sstrtab = getshi();	/* total # bytes in string entries provided */

    /* Process terminal name(s) (2): */
    for(cp=ttytype; snames--;)
		*cp++ = *ip++;
    *cp = '\0';
    printf("%s,\n", ttytype);


    /* Process boolean capabilities (3): */
    {
	int i;

	printf("\t");
	for (i=0; i < nbools; i++) {
	    if (*ip++ != (char)0) /* if capability is true */
		printf("%s, ",boolnames[i]);
	}
	printf("\n");
    }

    /* Force proper alignment */
    if (((unsigned int) ip) & 1)
	ip++;

    /* Process numeric capabilities (4): */
    {
	short i,s;

	printf("\t");
	for (i=0; i < nints; i++) {
	    s = getshi();
	    if (s != -1) /* if capability exists */
		printf("%s#%d, ",numnames[i], s);
	}
	printf("\n");
    }


    /* Read offsets of string capabilities (5):
     * (The offset is an index into the strtab string, where the string
     *  value will be. Therefore, strtab+index is the pointer to the string
     *  value and this is put in strptrs[i], for the ith string capability.
     *  If the string capability does not exist, strptr[i] will contain
     *  a NULL pointer instead.)
     */
    {
	short i,n;

	strptrs = (char **) malloc (sizeof(char **)*nstrs);
	strtab = (char *) malloc(sstrtab);
	for (i=0; i < nstrs; i++) {
	    n = getshi();
	    if (n == -1)
		strptrs[i] = (char *)0;	
	    else /* put pointer of strtab in strptrs */
		strptrs[i] = strtab + n;
	}
    }



    /* Read in actual values of string capabilities (6):
     * (read the values into strtab string, this
     *  automatically updates the strings pointed
     *  to in strptrs)
     */
    for (cp=strtab; sstrtab--; )
	*cp++ = *ip++;


    /* Print string capabilities: */
    {
	int i,j,pcount;
	char *string;	/* string capability value */

	printf("\t");
	for (i=0, pcount=0; i < nstrs; i++)
	    if (strptrs[i] != (char *)0) {
		pcount++; /* update print count */
		printf("%s=",strnames[i]);

		/* Print value (escape is special): */
		string = strptrs[i]; /* get value */
		for (j=0; string[j]; j++) {
		    switch(string[j]) {
			case 033:
			    printf("\\E"); continue;
			case '\n':
			    printf("\\n"); continue;
			case '\r':
			    printf("\\r"); continue;
			case '\t':
			    printf("\\t"); continue;
			case '\b':
			    printf("\\b"); continue;
			case '\f':
			    printf("\\f"); continue;
			case '^':
			    printf("\\^"); continue;
			case '\\':
			    printf("\\\\"); continue;
			case ',':
			    printf("\\,"); continue;
			case ':':
			    printf("\\:"); continue;
			case '\0':
			    printf("\\0"); continue;
			default:
			    break;
		    }

		    if ( (unsigned)(string[j]) < 0177 )
			printf("%s", unctrl(string[j]));
		    else
			printf("\\%3o", (int)(string[j] & 0377));
		}

		printf(", ");

		/* Print four string capabilities per line: */
		if ((pcount % 4) == 0)
		    printf("\n\t");
	    }
	printf("\n");
    }

    exit(0); /* Alles ging goed: Hiep Hiep Hoeraa! */
}


getfd(term)
    char *term; /* name of terminal */
{
    char fname[128];	/* file name of compiled terminfo file */
    register char *cp;
    int tfd = -1;

    /* Try finding file using the TERMINFO variable: */
    if (cp=getenv("TERMINFO")) {
	strcpy(fname, cp);
	cp = fname + strlen(fname);
	*cp++ = '/';
	*cp++ = *term;
	*cp++ = '/';
	strcpy(cp, term);
	tfd = open(fname, O_RDONLY);
    }

    /* If that didn't work, try finding it in /usr/lib/terminfo: */
    if (tfd < 0) {
	strcpy(fname, termpath(a/));
	cp = fname + strlen(fname);
	cp[-2] = *term;
	strcpy(cp, term);
	tfd = open(fname, O_RDONLY);
    }

    /* Does it exist at all? */
    if( tfd < 0 ) {
	if (access(termpath(.),0)) {
	    perror(termpath(.));
	    exit(1);
	} else
	    fprintf(stderr,"untic: no such terminal: %s\n",term);
	exit(1);
    }
    return tfd;
}
