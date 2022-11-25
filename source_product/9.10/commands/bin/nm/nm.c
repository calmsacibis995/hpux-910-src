static char *HPUX_ID = "@(#) $Revision: 66.4 $";
/*	nm	COMPILE:	cc -O nm.c -s -o nm	*/

/*
**	print symbol tables for
**	object or archive files
**
**	nm [-degnoprsux] [name ...]
*/



#include	<ar.h>
#include	<a.out.h>
#include	<stdio.h>
#include	<ctype.h>
#include	<globaldefs.h>
#include	<ranlib.h>

/*
 * The following option variables must be global, since they are used
 * by readsyms(), alphacomp() and numcomp().
 */
static int	revsort_flg;
static int	undef_flg;
static int	globl_flg;
static int	cflag;
static int	hflag;
static int	size_flg;

int	curpos;		/* current position in archive file */
struct	ar_hdr	arp;
struct	exec	filhdr;
char	*fnam;

#define SYMNAME(s) ((char *)(s) + sizeof (struct nlist_))
#define SYMLEN(s)  (s->n_length)
#define MIN(a, b)  ((a) < (b) ? (a) : (b))

void setsize();
int alphacomp();
int numcomp();
int nextel();
char *trim();

main(argc, argv)
int argc;
char **argv;
{
    extern int optind;
    extern int opterr;

    /*
     * Format strings for various radix possibilities.
     */
    static char hex_format[] = "0x%08X %c%c %-0.*s\n";
    static char dec_format[] = "%10u %c%c %-0.*s\n";
    static char oct_format[] = "0%011o %c%c %-0.*s\n";

    /*
     * The following are declared "static" so that they are
     * automatically initialized when main() is invoked.
     */
    static char *sym_format = hex_format;
    static int numsort_flg;
    static int nosort_flg;
    static int prep_flg;

    MAGIC fmagic;
    register int narg;
    int c;
    char arcmag[SARMAG + 1];

    opterr = 0;
    while ((c = getopt(argc, argv, "ngehurposdx8")) != EOF)
    {
	switch (c)
	{
	case 'n':		/* sort numerically */
	    numsort_flg = 1;
	    break;
	case 'g':		/* globl symbols only */
	case 'e':
	    globl_flg = 1;
	    break;
	case 'h':
	    hflag = 1;
	    break;
	case 'u':		/* undefined symbols only */
	    undef_flg = 1;
	    break;
	case 'r':		/* sort in reverse order */
	    revsort_flg = 1;
	    break;
	case 'p':		/* don't sort -- symbol table order */
	    nosort_flg = 1;
	    break;
	case 'o':		/* prepend a name to each line */
	    prep_flg = 1;
	    break;
	case 's':
	    size_flg = 1;
	    break;
	case 'd':
	    sym_format = dec_format;
	    break;
	case 'x':
	    sym_format = hex_format;
	    break;
	case '8':
	    sym_format = oct_format;
	    break;
	default:		/* oops */
	    fputs("usage: nm [-degnoprsux] [file ...]\n",
		    stderr);
	    exit(2);
	}
    }

    if (size_flg)
    {
	numsort_flg = 1;
	globl_flg = 1;
	undef_flg = 0;
	nosort_flg = 0;
    }

    if (optind >= argc)
    {
	argv[optind] = "a.out";
	argc++;
    }

    for (narg = argc - optind; optind < argc; optind++)
    {
	int arch_flg = 0;
	FILE *fi = fopen(argv[optind], "r");

	if (fi == NULL)
	{
	    fputs("nm: cannot open ", stderr);
	    perror(argv[optind]);
	    continue;
	}

	/* read the magic number */

	fread(arcmag, SARMAG, 1, fi);
	if (strncmp(arcmag, ARMAG, SARMAG) == 0)
	{
	    curpos = SARMAG;
	    arch_flg = 1;
	}
	else
	{			/* try for an a.out magic # */
	    fseek(fi, 0L, 0);
	    fread(&fmagic, sizeof (fmagic), 1, fi);
	    if (fmagic.system_id != HP98x6_ID &&
		    fmagic.system_id != HP9000S200_ID)
	    {
		fprintf(stderr, "nm: %s bad system id\n", argv[optind]);
		fclose(fi);
		continue;
	    }
	    if (fmagic.file_type == AR_MAGIC)
	    {
		fprintf(stderr, "nm: %s is in old archive format: use 'arcv'\n", argv[optind]);
		fclose(fi);
		continue;
	    }

	    if (fmagic.file_type != EXEC_MAGIC &&
		    fmagic.file_type != SHARE_MAGIC &&
		    fmagic.file_type != RELOC_MAGIC &&
		    fmagic.file_type != DEMAND_MAGIC &&
		    fmagic.file_type != SHL_MAGIC &&
		    fmagic.file_type != DL_MAGIC)
	    {
		fprintf(stderr, "nm: %s-- bad magic number\n", argv[optind]);
		fclose(fi);
		continue;
	    }
	    rewind(fi);
	    fread(&filhdr, sizeof (filhdr), 1, fi);
	    rewind(fi);
	}

	if (arch_flg)
	{
	    nextel(fi);
	    if (narg > 1)
		printf("\n%s:\n", argv[optind]);
	}

	do
	{
	    register n;
	    struct nlist_ **symlist = NULL;
	    int symcount;

	    fnam = arch_flg ? trim(arp.ar_name) : argv[optind];
	    fread(&filhdr, 1, sizeof (struct exec), fi);
	    if (N_BADMAG(filhdr))   /* archive element not in  */
		continue;	/* proper format - skip it */
	    symcount = readsyms(fi, &symlist);
	    if (!nosort_flg)
	    {
		qsort(symlist, symcount,
			sizeof (struct nlist_ *), alphacomp);
		if (numsort_flg)
		    qsort(symlist, symcount,
			    sizeof (struct nlist_ *), numcomp);
		if (size_flg)
		{
		    setsize(symlist, symcount);
		    qsort(symlist, symcount,
			    sizeof (struct nlist_ *), numcomp);
		}
	    }

	    if ((arch_flg || narg > 1) && prep_flg == 0)
	    {
		putchar('\n');
		fputs(fnam, stdout);
		fputs(":\n", stdout);
	    }

	    for (n = 0; n < symcount; n++)
	    {
		struct nlist_ *s = symlist[n];
		char type;
		char alch;

		if (prep_flg)
		{
		    if (arch_flg)
		    {
			fputs(argv[optind], stdout);
			putchar(':');
		    }
		    fputs(fnam, stdout);
		    putchar(':');
		}

		switch (s->n_type & 017)
		{
		case UNDEF:
		    type = 'U';
		    break;
		case ABS:
		    type = 'A';
		    break;
		case TEXT:
		    type = 'T';
		    break;
		case DATA:
		    type = 'D';
		    break;
		case BSS:
		    type = 'B';
		    break;
		default:
		    fprintf(stderr,
			    "nm: unrecognized type 0%o on symbol %-0.*s",
			    s->n_type, SYMLEN(s), SYMNAME(s));
		    break;
		}
		if ((s->n_type & EXTERN) == 0)
		    type |= 040;
		if (s->n_type & ALIGN)
		    alch = 'L';
		else if (s->n_type & EXTERN2)
		    alch = 'S';
		else
		    alch = ' ';

		printf(sym_format, (long)s->n_value, type,
			alch, SYMLEN(s), SYMNAME(s));
	    }
	    /* seek to end of a.out file */
	} while (arch_flg && nextel(fi));
	fclose(fi);
    }
    return 0;
}

/*
 * readsyms() --
 *    read symbol table information from an object file 'fi'.
 *    Returns the number of symbols read.
 *    Sets *psymarr to an array of (struct nlist_ *) pointers.
 */
int
readsyms(fi, psymarr)
FILE *fi;
struct nlist_ ***psymarr;
{
#define SYMBOLS_INCR	100
    static struct nlist_ *symbuf = NULL;
    static struct nlist_ **symbols = NULL;
    int symcount;
    struct nlist_ *bufptr;
    struct nlist_ *bufend;
    long maxsymbols;

    /*
     * There are no symbols if the symbol table size is 0.
     */
    if (filhdr.a_lesyms == 0)
    {
	*psymarr = (struct nlist_ **)0;
	return 0;
    }

    /*
     * Free memory used by previous object file.
     */
    if (symbols != NULL)
	free(symbols);
    if (symbuf != NULL)
	free(symbuf);

    /*
     * Allocate a buffer large enough to hold the symbol table.
     * Allocate an array of nlist_ pointers big enough to hold
     * all of the symbols (for the worst case, where each symbol
     * name is only 1 character long).
     */
    maxsymbols = filhdr.a_lesyms / (sizeof (struct nlist_) + 1) + 1;
    symbuf = (struct nlist_ *)malloc(filhdr.a_lesyms);
    symbols = (struct nlist_ **)
			 malloc(maxsymbols * sizeof (struct nlist_ *));

    if (symbuf == (struct nlist_ *)0 || symbols == (struct nlist_ **)0)
    {
	fprintf(stderr,"nm: out of memory on %s\n", fnam);
	exit(2);
    }

    /*
     * seek to beginning of symbol table and read it in.
     */
    fseek(fi, LESYMPOS -(sizeof filhdr), SEEK_CUR);
    if (fread(symbuf, filhdr.a_lesyms, 1, fi) != 1)
    {
	fprintf(stderr,"nm: file %s format error, unexpected eof",
	    fnam);
	*psymarr = (struct nlist_ **)0;
	return 0;
    }

    /*
     * Now setup our symbol array to point to the elements that we are
     * interested in.
     */
    symcount = 0;
    bufptr = symbuf;
    bufend = (struct nlist_ *)((char *)bufptr + filhdr.a_lesyms);
    while (bufptr < bufend)
    {
	struct nlist_ *sym = bufptr;

	/*
	 * Advance bufptr by the size of an nlist_ struct plus the
	 * length of the symol name
	 */
	bufptr = (struct nlist_ *)
		    ((char *)bufptr + sizeof (struct nlist_)
				    + sym->n_length);

	if (cflag)
	{
	    char *cp1 = (char *)sym + sizeof (struct nlist_);

	    if (*cp1 == '~' || *cp1 == '_')
	    {
		register char *cp2 = cp1 + 1;
		register int i;

		for (i=sym->n_length; i > 0; --i)
		    *cp1++ = *cp2++;
	    }
	    else
		continue;
	}

	if (globl_flg && !(sym->n_type & N_EXT))
	    continue;
	if (undef_flg && ((sym->n_type & 037) != N_UNDF))
	    continue;
	if (size_flg && ((sym->n_type & 037) == N_UNDF))
	    continue;

	/*
	 * Make sure we aren't running over our buffer (this should
	 * never happen!).
	 */
	if (symcount >= maxsymbols)
	{
	    fprintf(stderr,"nm: too many symbols in %s?\n", fnam);
	    return 0;
	}

	/*
	 * add sym and name index to sym list
	 */
	symbols[symcount++] = sym;
    }
    *psymarr = symbols;
    return symcount;
}

int
numcomp(pp1, pp2)
struct nlist_ **pp1;
struct nlist_ **pp2;
{
    struct nlist_ *p1 = *pp1;
    struct nlist_ *p2 = *pp2;
    register int i = p1->n_value - p2->n_value;

    if (i == 0)
	i = p2->n_type - p1->n_type;
    return revsort_flg ? -i : i;
}

/*
 * alphacomp() --
 *    compare names of two symbols.  Since the symbol names are not
 *    '\0' terminated, we must use the shortest n_length field of
 *    the two strings.
 */
int
alphacomp(pp1,pp2)
struct nlist_ **pp1;
struct nlist_ **pp2;
{
    struct nlist_ *p1 = *pp1;
    struct nlist_ *p2 = *pp2;
    register int l = MIN(p1->n_length, p2->n_length);
    register int i = strncmp(SYMNAME(p1), SYMNAME(p2), l);

    if (i == 0)
    {
	/*
	 * Two strings are equal up to the length of the shortest
	 * string.  The longer of the two strings is '>'.
	 */
	i = p1->n_length - p2->n_length;
    }

    return revsort_flg ? -i : i;
}

void
setsize(symlist, symcount)
struct nlist_ **symlist;
int symcount;
{
    register i, high, low;
    high = filhdr.a_text;

    for (i = symcount - 1; i >= 0; i--)
    {
	low = symlist[i]->n_value;
	symlist[i]->n_value = high - low;
	high = low;
    }
}

int
nextel(af)
FILE *af;
{
    fseek(af, curpos, 0);
    if (fread(&arp, sizeof arp, 1, af) != 1)
	return 0;
    curpos = curpos + sizeof (arp)+((atol(arp.ar_size) + 1) & ~1);
    if (strncmp(arp.ar_name, DIRNAME, strlen(DIRNAME)) == 0)
	nextel(af);
    return 1;
}

/*
 * trim - Removes slash and makes the string asciz.
 */
char *
trim(s)
char *s;
{
    register int i;
    for (i = 0; i < 16; i++)
	if (s[i] == '/')
	    s[i] = '\0';
    return (s);
}
