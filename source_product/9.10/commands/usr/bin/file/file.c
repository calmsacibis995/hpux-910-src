/* @(#) $Revision: 66.8 $ */
/*
 * determine type of file
 */

#include <stdio.h>
#include <ctype.h>
#include <signal.h>
#include <sys/param.h>
#include <sys/sysmacros.h>
#include <sys/types.h>
#include <sys/stat.h>

#ifdef ACLS
#include <sys/acl.h>
#include <unistd.h>
#endif /* ACLS */

/*
 *	Types
 */

#define BYTE    0
#define SHORT	2
#define LONG	4
#define STR	8

/*
 *	Opcodes
 */
#define EQ          0
#define GT          1
#define GE          2
#define LT          3
#define LE          4
#define STRC        5	/*string compare */
#define ANY         6
#define BITTEST     7
#define BITEQUAL    8
#define SUB      0x80	/* or'ed in */

/*
 *	Misc
 */
#define NENT 	200
#define BSZ 	128
#define FBSZ	5121

/*
 * definitions for entry->flags
 */
#define	TOP_LEVEL	0x80
#define INDIR		0x40

/*
 * Structure of magic file entry
 */
struct entry
{
    unsigned char flags;	/* various flags */
    unsigned char indir_type;   /* type of indirect */
    unsigned char e_type;	/* type of value */
    long e_off;		        /* location of value or indir addr */
    long indir_fudge;		/* fudge to add to indir addr */
    unsigned char e_opcode;	/* type of comparison */
    union
    {
	long num;		/* numeric value to compare against */
	char *str;		/* string value to compare against */
    } e_value;
    char *e_str;		/* message to print */
};

typedef struct entry 	Entry;

Entry 	*mtab;
char	fbuf[FBSZ];
char 	*mfile = "/etc/magic";

char 	*fort[] = {
	"function","subroutine","common","dimension","block","integer",
	"real","double",0};
char	*as200[] = {
	"asciz","dc.b","dc.w","dc.l","ds","ds.b","ds.w","ds.l","equ",
	"link.l", "movm.l", "jsr", "fmovm.x", 0};
char	*as500[] = {
	"idata","ent","ddata","pcls","pshn","zero","ld","pcl","lds",
	"sts", "st","sltspecial","commptr",0};
char	*as800[] = {".subspa",".call",".space",".import",".procend",
	".SUBSPA",".CALL",".SPACE",".IMPORT",".PROCEND",0};

char 	*asc[] = {
	"mov","tst","clr","jmp",0};
char	*sed[] = {
	"p","d","w","g","s","r",0};
char 	*bas[] = {
	"ASSIGN @","ASSIGN #","LINPUT","OUTPUT","PRINTALL IS",
	"PRINTER IS", "DISP","OPTION BASE","CREATE","RANDOMIZE","REAL",
	"INTEGER","DIM", "PRINT","LOAD","ON KEY","COM","GET",0};
char	*pas[] = {
	"const","type","var","begin","longreal","binary","block","eoln",
	"bufferread","bufferwrite","blockread","blockwrite",0};
char 	*c[] = {
	"define","static","include","int","char","float","double",
	"struct", "extern","printf", "union", "ifdef", "define", 0};
char 	*as[] = {
	"globl","byte","even","text","data","bss","comm",0};

extern char *strchr();
extern char *malloc();
extern unsigned long strtoul();
extern long strtul();

long 	atolo();
int 	i = 0;
int	init = 0;

/*
 * Data associated with the current file.  We keep the access and
 * modification times of the current file (if we open it) since we
 * reset it right before we close it.
 */
int 	ifd;
char	*file;
struct mmbuf {
	time_t acctime;
	time_t modtime;
} mmbuf;
long	file_size;	/* actual size of file */
long 	fbsz;		/* bytes to examine for non-magic tests */

void	print_mtab();

#define prf(x) 	printf("%s:%s",x,strlen(x)>6? "\t" : "\t\t");

main(argc, argv)
int argc;
char **argv;
{
	register int	ch;
	register int	cflg = 0, eflg = 0, fflg = 0;
	register FILE	*fl;
	auto	 char	ap[128];
	extern 	 int	optind;
	extern	 char	*optarg;

	fl = 0;
	while ((ch = getopt(argc, argv, "cf:m:")) != EOF)
		switch (ch) {
		case 'c':
			cflg++;
			break;
		case 'f':
			fflg++;
			if ((fl = fopen(optarg, "r")) == NULL) {
				fprintf(stderr, "cannot open %s\n",
				    optarg);
				goto use;
			}
			break;
		case 'm':
			mfile = optarg;
			break;
		case '?':
			eflg++;
			break;
		}

	if (!cflg && !fflg && (eflg || optind == argc)) {
use:
		fputs("usage: file [-c] [-f ffile] [-m mfile] file...\n",
		    stderr);
		exit(2);
	}

	if (cflg)
	{
	    print_mtab();
	    exit(0);
	}

	for (; fflg || optind < argc; optind += !fflg) {
		register int l;

		if (fflg) {
			if ((file = fgets(ap, 128, fl)) == NULL) {
				fflg = 0;
				optind--;
				continue;
			}
			l = strlen(file);
			if (l > 0)
				file[l - 1] = '\0';
		} else
			file = argv[optind];
		if (file == NULL || *file == '\0') {
			fputs("that's all!\n", stderr);
			exit(1);
		}       /* for s500 silly core dump with big arg list */
		type();
		if (ifd != -1)
		{
			close(ifd);
			utime(file, &mmbuf);
		}
	}
	return 0;
}

type()
{
	struct stat mbuf;
	ifd = -1;
	if (stat(file, &mbuf) < 0) {
		prf(file);
		puts("cannot open");
		return;
	}

	/*
	 * copy time values from stat struct into struct utime() will
	 * recognize.
	 */
	mmbuf.acctime = mbuf.st_atime;
	mmbuf.modtime = mbuf.st_mtime;
	file_size = mbuf.st_size;

	switch (mbuf.st_mode & S_IFMT) {
	case S_IFCHR:
		prf(file);
		printf("character special (%d/%d)\n",
		    major(mbuf.st_rdev), minor(mbuf.st_rdev));
		return;
	case S_IFDIR:
		prf(file);
		puts("directory");
		return;
	case S_IFIFO:
		prf(file);
		puts("fifo");
		return;
	case S_IFBLK:
		prf(file);
		printf("block special (%d/%d)\n",
		    major(mbuf.st_rdev), minor(mbuf.st_rdev));
		return;
	case S_IFSOCK:
		prf(file);
		puts("socket");
		return;
	}

	if ((ifd = open(file, 0)) < 0) {
		prf(file);
		puts("cannot open for reading");
		return;
	}

	fbuf[FBSZ - 1] = '\0';
	if ((fbsz = read(ifd, fbuf, FBSZ-1)) == 0) {
		prf(file);
		puts("empty");
		return;
	}

	if (fbsz != FBSZ - 1)
		memset(fbuf + fbsz, '\0', (FBSZ-1) - fbsz);

	if (ckmtab()) {
		return;
	}

	/*
	 * Do special checks that are too complicated for /etc/magic
	 * processing.  Always do the magic-like and binary file checks
	 * first.
	 */
	if (sharpbangck() ||  /* special kind of magic */
	    sccsck()	||    /* binary file checks */
	    comprck()	||
	    tarck()	||
	    ckc()	||    /* ascii file checks */
	    fortck()	||
	    asck() 	||
	    pasck()	||
	    basck()	||
	    lexck()	||
	    awkck()	||
	    sedck()	||
	    troffck()) {
		putchar('\n');
		return;
	}

	if (mbuf.st_mode&((S_IEXEC)|(S_IEXEC>>3)|(S_IEXEC>>6)))
		fputs("commands text", stdout);
#ifdef ACLS
	else
		/*
		 * If the file has optional entries in its access
		 * control list, they must be checked also for execute
		 * permission.
		 */
		if (aclexec(file))
			fputs("commands text", stdout);
#endif /* ACLS */
	else
		if (english(fbuf, fbsz))
			fputs("English text", stdout);
	else
		fputs("ascii text", stdout);

	if (i < fbsz)
		if ((fbuf[i++]&0377) > 127)
			fputs(" with garbage", stdout);
	putchar('\n');
}

/*
 * atolo() --
 *    Convert a string to a number, ensuring that the string was
 *    properly terminated with a tab, blank, newline or '\0'.
 */
long
atolo(s, cflg)
register char *s;
int cflg;
{
    register long val;
    register int negative = 0;
    char *p;

    while (*s == ' ' || *s == '\t')
	s++;

    if (*s == '-')
    {
	negative = 1;
	s++;
    }

    val = (long)strtoul(s, &p, 0);

    if (*p != '\t' && *p != ' ' && *p != '\n' && *p != '\0')
    {
	if (cflg)
	    fprintf(stderr,
		"fmt error, bad value \"%s\" in magic file\n", s);
	return 0;
    }

    return negative ? 0 - val : val;
}

/*
 * parse_indir() --
 *    parse an indirect offset specification.  The specification is of
 *    the form:
 *       (nnnnn[.bwl][+-mmmm])
 *
 *    returns 0 if ok, non-zero if there is a format error.
 */
parse_indir(p, ep, lcnt, cflg)
char *p;
Entry *ep;
int lcnt;   /* line number for error messages */
int cflg;   /* line number for error messages */
{
    char *sb;
    unsigned long offset;

    /*
     * Set the flag to indicate that this is an "indirect" entry.
     * Then, get the offset value and initialize offset type and
     * fudge to defaults.
     */
    ep->flags |= INDIR;
    ep->e_off = strtoul(p+1, &sb, 0);
    ep->indir_type = LONG;
    ep->indir_fudge = 0;

    while (*sb == ' ' || *sb == '\t')
	sb++;

    if (*sb == '.')
    {
	sb++;
	switch (*sb)
	{
	case 'b':
	case 'B':
	    ep->indir_type = BYTE;
	    break;
	case 'w':
	case 'W':
	    ep->indir_type = SHORT;
	    break;
	case 'l':
	case 'L':
	    ep->indir_type = LONG;
	    break;
	default:
	    if (cflg)
		fprintf(stderr,
		   "fmt error, bad size specifier %c on line %d\n",
		   *sb, lcnt);
	    return 1;
	}
	sb++;
    }

    while (*sb == ' ' || *sb == '\t')
	sb++;

    if (*sb == '+' || *sb == '-')
	ep->indir_fudge = strtol(sb, &sb, 0);

    while (*sb == ' ' || *sb == '\t')
	sb++;

    if (*sb == ')')
	return 0;

    if (cflg)
	fprintf(stderr,
	   "fmt error, bad indirect specifier %s on line %d\n",
	   p, lcnt);
    return 1;
}

/*
 * make a copy of the magic file specified.  If none is explicitly
 * specified use the one in /etc
 */
mkmtab(cflg)
register int cflg;
{
    register Entry *ep;
    register FILE *fp;
    register int lcnt = 0;
    auto char buf[BSZ];
    auto Entry *mend;

    ep = (Entry *)calloc(sizeof (Entry), NENT);
    if (ep == NULL)
    {
	fputs("no memory for magic table\n", stderr);
	exit(2);
    }
    mtab = ep;
    mend = &mtab[NENT];
    fp = fopen(mfile, "r");
    if (fp == NULL)
    {
	fprintf(stderr, "cannot open magic file <%s>.\n", mfile);
	exit(2);
    }

    while (fgets(buf, BSZ, fp) != NULL)
    {
	register char *p = buf;
	register char *p2;
	register char opc;

	if (*p == '\n' || *p == '#')
	    continue;
	lcnt++;

	/*
	 * LEVEL
	 */
	if (*p == '>')
	{
	    ep->flags = 0;		/* clear all bits */
	    p++;
	}
	else
	    ep->flags = TOP_LEVEL;	/* clear other bits */

	/*
	 * OFFSET
	 */
	p2 = strchr(p, '\t');
	if (p2 == NULL)
	{
	    if (cflg)
		fprintf(stderr,
		    "fmt error, no tab after %son line %d\n", p, lcnt);
	    continue;
	}
	*p2++ = NULL;
	if (*p == '(')
	{
	    if (parse_indir(p, ep, lcnt, cflg) != 0)
		continue;
	}
	else
	    ep->e_off = atolo(p, cflg);
	while (*p2 == '\t')
	    p2++;

	/*
	 * TYPE
	 */
	p = p2;
	p2 = strchr(p, '\t');
	if (p2 == NULL)
	{
	    if (cflg)
		fprintf(stderr,
		   "fmt error, no tab after %son line %d\n", p, lcnt);
	    continue;
	}
	*p2++ = NULL;
	if (*p == 's')
	{
	    if (*(p + 1) == 'h')
		ep->e_type = SHORT;
	    else
		ep->e_type = STR;
	}
	else if (*p == 'l')
	    ep->e_type = LONG;
	while (*p2 == '\t')
	    p2++;

	/*
	 * OP_VALUE
	 */
	p = p2;
	p2 = strchr(p, '\t');
	if (p2 == NULL)
	{
	    if (cflg)
		fprintf(stderr,
		    "fmt error, no tab after %son line %d\n", p, lcnt);
	    continue;
	}
	*p2++ = NULL;
	if (ep->e_type != STR)
	{
	    opc = *p++;
	    switch (opc)
	    {
	    case '=':		/* defalts to this */
		if (*p == '=')	/* allow "==" too */
		    p++;
		ep->e_opcode = EQ;
		break;
	    case '>':
		if (*p == '=')
		{
		    p++;
		    ep->e_opcode = GE;
		}
		else
		    ep->e_opcode = GT;
		break;
	    case '<':
		if (*p == '=')
		{
		    p++;
		    ep->e_opcode = LE;
		}
		else
		    ep->e_opcode = LT;
		break;
	    case '&':
		if (*p == '=')
		{
		    p++;
		    ep->e_opcode = BITEQUAL;
		}
		else
		    ep->e_opcode = BITTEST;
		break;
	    case 'x':
		ep->e_opcode = ANY;
		break;
	    default:
		p--;
	    }
	}
	if (ep->e_opcode != ANY)
	{
	    if (ep->e_type != STR)
		ep->e_value.num = atolo(p, cflg);
	    else
	    {
		ep->e_value.str = malloc(strlen(p) + 1);
		strcpy(ep->e_value.str, p);
	    }
	}
	while (*p2 == '\t')
	    p2++;

	/*
	 * STRING
	 */
	ep->e_str = malloc(strlen(p2) + 1);
	p = ep->e_str;
	while (*p2 != '\n')
	{
	    if (*p2 == '%')
		ep->e_opcode |= SUB;
	    *p++ = *p2++;
	}
	*p = NULL;
	ep++;
	if (ep >= mend)
	{
	    fputs(
		"file: magic tab overflow - increase NENT in file.c.\n",
		stderr);
	    exit(2);
	}
    }
    ep->e_off = -1L;
    if (fp)
	fclose(fp);
}

/*
 * print_mtab() --
 *    dump out the "magic" table for debugging.
 */
void
print_mtab()
{
    register Entry *ep;
    char offset_buf[80];

    mkmtab(1);
    puts("level\toff\t\ttype\topcode\tvalue\t\tstring");
    for (ep = mtab; ep->e_off != -1L; ep++)
    {
	printf("%d\t", ep->flags & TOP_LEVEL ? 0 : 1);
	if (ep->flags & INDIR)
	{
	    sprintf(offset_buf, "(%d.%c", ep->e_off,
		    ep->indir_type == LONG ? 'l' :
		    ep->indir_type == SHORT ? 'w' : 'b');
	    if (ep->indir_fudge == 0)
		strcat(offset_buf, ")");
	    else
	    {
		if (ep->indir_fudge > 0)
		    strcat(offset_buf, "+");
		sprintf(offset_buf + strlen(offset_buf), "%d)",
		    ep->indir_fudge);
	    }
	}
	else
	{
	    sprintf(offset_buf, "%d", ep->e_off);
	}
	printf("%-15s\t%d\t%d\t", offset_buf, ep->e_type, ep->e_opcode);
	if (ep->e_type == STR)
	{
	    fputs(ep->e_value.str, stdout);
	    putchar('\t');
	}
	else
	    printf("%-15d\t", ep->e_value.num);
	fputs(ep->e_str, stdout);
	if (ep->e_opcode & SUB)
	    fputs("\tsubst", stdout);
	putchar('\n');
    }
}

/*
 * data_at() --
 *    return a pointer 'size' bytes of data from the input file
 *    starting at 'offset'.
 */
char *
data_at(offset, size)
long offset;
int size;
{
    static char *buf;
    static int bufsiz;

    if (offset < 0 || offset + size > file_size)
	return (char *)0;

    /*
     * If the data is all in the first FBSZ bytes, we can just
     * return a pointer into our file buffer.
     */
    if (offset + size < FBSZ)
	return (char *)&fbuf[offset];

    /*
     * The data lies outside the first FBSZ bytes.  We must read
     * the data from the file into a local buffer.
     *
     * First, try to seek to where the data should be.
     */
    if (lseek(ifd, offset, SEEK_SET) == -1)
	return (char *)0;

    /*
     * Next, we must make sure that we have a buffer large enough
     * for the data that we will read.
     */
    if (buf != (char *)0 && size > bufsiz)
    {
	free(buf);
	buf = (char *)0;
    }

    if (buf == (char *)0)
    {
	if ((buf = malloc(bufsiz = size<32 ? 32 : size)) == (char *)0)
	{
	    bufsiz = 0;
	    return (char *)0;
	}
    }

    if (read(ifd, buf, size) != size)
	return (char *)0;
    return buf;
}

/*
 * data_for() --
 *    fetch the data that an Entry "ep" needs, returning a pointer
 *    to the data.
 */
char *
data_for(ep)
Entry *ep;
{
    int offset;
    char *addr;
    int size;

    /*
     * If this is an indirect entry, we must get the real offset that
     * we want to look at by looking at the file at the offset that is
     * specified by ep->e_off plus ep->e_fudge.
     */
    if (ep->flags & INDIR)
    {
	size = ep->indir_type == LONG  ? 4 :
	       ep->indir_type == SHORT ? 2 : 1;
	if ((addr = data_at(ep->e_off, size)) == (char *)0)
	    return (char *)0;

	switch (size)
	{
	case 1:
	    offset = (long)(*(unsigned char *)addr);
	    break;
	case 2:
	    offset = (long)(*(unsigned short *)addr);
	    break;
	case 4:
	    offset = (long)(*(unsigned long *)addr);
	    break;
	}
	offset += ep->indir_fudge;
    }
    else
	offset = ep->e_off;

    if (ep->e_type == STR)
	size = strlen(ep->e_value.str);
    else
	size = ep->e_type == LONG ? 4 : ep->e_type == SHORT ? 2 : 1;

    return data_at(offset, size);
}

/*
 * ckmtab() --
 *     check the magic table for the numbers in the header file
 */
ckmtab()
{
    register Entry *ep;
    register char *p;
    register int lev1 = 0;
    register long val = 0L;

    if (!init)
    {
	mkmtab(0);
	init = 1;
    }

    prf(file);
    for (ep = mtab; ep->e_off != -1L; ep++)
    {
	if (lev1)
	{
	    if (ep->flags & TOP_LEVEL)
		break;
	}
	else if (!(ep->flags & TOP_LEVEL))
	    continue;

	if ((p = data_for(ep)) == (char *)0)
	    continue;

	switch (ep->e_type)
	{
	case STR:
	    {
		if (strncmp(p, ep->e_value.str, strlen(ep->e_value.str)))
		    continue;
		if (lev1)
		    putchar(' ');
		if (ep->e_opcode & SUB)
		    printf(ep->e_str, ep->e_value.str);
		else
		    fputs(ep->e_str, stdout);
		lev1 = 1;
		continue;
	    }
	case BYTE:
	    val = (long)(*(unsigned char *)p);
	    break;
	case SHORT:
	    val = (long)(*(unsigned short *)p);
	    break;
	case LONG:
	    val = (*(long *)p);
	    break;
	}

	switch (ep->e_opcode & ~SUB)
	{
	case EQ:
	    if (!(val == ep->e_value.num))
		continue;
	    break;
	case GT:
	    if (!(val > ep->e_value.num))
		continue;
	    break;
	case GE:
	    if (!(val >= ep->e_value.num))
		continue;
	    break;
	case LT:
	    if (!(val < ep->e_value.num))
		continue;
	    break;
	case LE:
	    if (!(val <= ep->e_value.num))
		continue;
	    break;
	case BITTEST:
	    if ((val & ep->e_value.num) == 0)
		continue;
	    break;
	case BITEQUAL:
	    if ((val & ep->e_value.num) != ep->e_value.num)
		continue;
	    break;
	}

	if (lev1)
	    putchar(' ');
	if (ep->e_opcode & SUB)
	    printf(ep->e_str, val);
	else
	    fputs(ep->e_str, stdout);
	lev1 = 1;
    }
    if (lev1)
    {
	putchar('\n');
	return 1;
    }
    return 0;
}

/*
 * check the buffer for keywords listed in each array of keywords, for
 * example lookup(as) will check the buffer for occurrances of
 * assembler keywords
 */
lookup(tab)
register char **tab;
{
	register char r;
	register int k,j,l;

	while (fbuf[i] == ' ' || fbuf[i] == '\t' || fbuf[i] == '\n')
		i++;
	for (j=0; tab[j] != 0; j++) {
		l=0;
		for (k=i; ((r=tab[j][l++]) == fbuf[k] && r != '\0');k++)
			continue;
		if (r == '\0')
			if (fbuf[k] == ' ' || fbuf[k] == '\n' ||
			    fbuf[k] == '\t' || fbuf[k] == '{' ||
			    fbuf[k] == '/' || fbuf[k] == '(') {
				i=k;
				return 1;
			}
	}
	return 0;
}

/*
 * look for a C program comment
 */
ccom()
{
	register char cc;
	while ((cc = fbuf[i]) == ' ' || cc == '\t' || cc == '\n')
		if (i++ >= fbsz)
			return 0;
	if (fbuf[i] == '/' && fbuf[i+1] == '*') {
		i += 2;
		while (fbuf[i] != '*' || fbuf[i+1] != '/') {
			if (fbuf[i] == '\\')
				i += 2;
			else
				i++;
			if (i >= fbsz)
				return 0;
		}
		if ((i += 2) >= fbsz)
			return 0;
	}
	if (fbuf[i] == '\n')
		if (ccom() == 0)
			return 0;
	return 1;
}

/*
 * look for an assembler comment
 */
ascom()
{
#ifdef hp9000s800
	while (fbuf[i] == ';') {
#else
	while (fbuf[i] == '/') {
#endif
		i++;
		while (fbuf[i++] != '\n')
			if (i >= fbsz)
				return 0;
		while (fbuf[i] == '\n')
			if (i++ >= fbsz)
				return 0;
	}
	return 1;
}

/*
 * look for English text
 */
english(bp, n)
char *bp;
{
# define NASC 128
	register int j, vow, freq, rare;
	register int badpun = 0, punct = 0;
	auto	 int ct[NASC];

	if (n < 50)
		return 0; /* no point in statistics on squibs */
	for (j=0; j < NASC; j++)
		ct[j]=0;
	for (j=0; j < n; j++) {
		if (bp[j] < NASC)
			ct[bp[j]|040]++;
		switch (bp[j]) {
		case '.':
		case ',':
		case ')':
		case '%':
		case ';':
		case ':':
		case '?':
			punct++;
			if (j < n-1 &&
			    bp[j+1] != ' ' && bp[j+1] != '\n')
				badpun++;
		}
	}
	if (badpun*5 > punct)
		return 0;
	vow = ct['a'] + ct['e'] + ct['i'] + ct['o'] + ct['u'];
	freq = ct['e'] + ct['t'] + ct['a'] + ct['i'] + ct['o'] + ct['n'];
	rare = ct['v'] + ct['j'] + ct['k'] + ct['q'] + ct['x'] + ct['z'];
	if (2*ct[';'] > ct['e'])
		return 0;
	if ((ct['>']+ct['<']+ct['/'])>ct['e'])
		return 0; /* shell file test */
	return (vow*5 >= n-ct[' '] && freq >= 10*rare);
}

/*
 * Note: none of the following type-checking functions require '\n' to
 * complete their printf format strings.  The type() function (which
 * calls these type-checking functions) provides a linefeed upon return
 * to it!
 */

/*
 * Code to check for SCCS files.  These files are always distinguished
 * by an octal one in the first byte followed by the character 'h'
 * followed by five or more digits
 */
sccsck()
{
#define OCTONE	'\001'	/*set up a constant for checking SCCS files*/
	register int i = 0;

	if (fbuf[i] == OCTONE && fbuf[++i] == 'h') {
		for (i=2; i<=6; i++)
			if (!(isdigit(fbuf[i])))
				return 0;
		fputs("sccs file", stdout);
		return 1;
	}
	else
		return 0;
}

/*
 * check for fortran program
 */
fortck()
{
	char savebuf[FBSZ];

	i = 0;
	while (savebuf[i] = fbuf[i])
		i++;

	/*
	 * Since fortran isn't case sensiteve, convert to lower case
	 * before checking.
	 */
	for (i = 0; i <= fbsz; i++)
		if (fbuf[i] >= 'A' && fbuf[i] <= 'Z')
			fbuf[i] = tolower(fbuf[i]);

	i = 0;
	while (fbuf[i] == 'c' || fbuf[i] == '#') {
		while (fbuf[i++] != '\n')
			if (i >= fbsz) {
				i = 0;
				while (fbuf[i] = savebuf[i])
				        i++; /*restore fbuf*/
			       return 0;
			}
	}
	while (!lookup(fort)) {
		while (fbuf[i++] != '\n')
			if (i >= fbsz) {
				i = 0;
				while (fbuf[i] = savebuf[i])
				        i++; /*restore fbuf*/
				return 0;
			}
	}
	fputs("fortran program text", stdout);
	return 1;
}

/*
 * check for assembler text- this check includes checks for s200 and
 * s500 assembler text
 */
asck()
{
	int j;
	i = 0;
	if (ascom()) {
		j = i-1;
		if (fbuf[i] == '.') {
			i++;
			if (lookup(as)) {
				fputs("assembler program text", stdout);
				return 1;
			}
			else if (j != -1 && fbuf[j] == '\n' &&
				isalpha(fbuf[j+2])) {
				fputs("[nt]roff, tbl, or eqn input text", stdout);
				return 1;
			}
		}
		while (!lookup(asc)) {
			if (!ascom())
				goto notdecas;
			while (fbuf[i] != '\n' && fbuf[(i++)] != ':')
				if (i >= fbsz)
					goto notdecas;
			while (fbuf[i] == '\n' || fbuf[i] == ' ' ||
			       fbuf[i] == '\t')
				if (i++ >= fbsz)
					goto notdecas;
			j = i-1;
			if (fbuf[i] == '.') {
				i++;
				if (lookup(as)) {
					fputs("assembler program text",
					    stdout);
					return 1;
				}
				else if (fbuf[j] == '\n' && isalpha(fbuf[j+2])) {
					fputs("[nt]roff, tbl, or eqn input text", stdout);
					return 1;
				}
			}
		}
		fputs("assembler program text", stdout);
		return 1;
	}
notdecas:
	if (ck800() || ck200as() || ck500as())
		return 1;
	else
		return 0;
}

ck200as()
{
	i = 0;
	while (!lookup(as200))
		while (fbuf[i] != ' ' && fbuf[i] != '\t' &&
		       fbuf[i] != '\n')
			if (i++ >= fbsz)
				return 0;

	fputs("s200 assembler program text", stdout);
	return 1;
}

ck500as()
{
	i = 0;
	while (!lookup(as500))
		while (fbuf[i] != ' ' && fbuf[i] != '\t' &&
		       fbuf[i] != '\n')
			if (i++ >= fbsz)
				return 0;
	fputs("s500 assembler program text", stdout);
	return 1;
}

ck800()
{
	i = 0;
	while (!lookup(as800))
		while (fbuf[i] != ' ' && fbuf[i] != '\t' &&
		       fbuf[i] != '\n')
			if (i++ >= fbsz)
				return 0;
	fputs("s800 assembler program text", stdout);
	return 1;
}

troffck()
{
	for (i=0; i < fbsz; i++)
		if (fbuf[i]&0200) {
			if (fbuf[0]=='\100' && fbuf[1]=='\357') {
				fputs("troff output", stdout);
				return 1;
			}
			fputs("data", stdout);
			return 1;
		}
	return 0;
}

/*check for C program text */

ckc()
{
	char ch;

	i = 0;
	if (!ccom())
		return 0;
lbck:
	while (fbuf[i++] != '#')
		if (i >= fbsz) {
			i = 0;
			goto check;
		}
	while (fbuf[i] == '\t' || fbuf[i] == '\n' || fbuf[i] == ' ')
		if (i >= fbsz) {
			fputs("data", stdout);
			return 1;
		}
		else
			i++;

	/*takes care of # vars */

	if (lookup(c)) {
		if (!lexck())
			fputs("c program text", stdout);
		return 1;
	}
	else
		if (i < fbsz)
			goto lbck;

	i=0;
check:
	while (fbuf[i] == ' ' || fbuf[i] == '\t' || fbuf[i] == '\n')
		if (i++ >= fbsz)
			return 0;

	if (lookup(c)) {
		while ((ch = fbuf[i++]) != ';' && ch != '{')
			if (i >= fbsz)
				return 0;
		if (!lexck())
			fputs("c program text", stdout);
		return 1;
	}

	while (fbuf[i] != '(') {
		if (fbuf[i] <= 0)
			if (ck200as() || ck500as())
				return 1;
			else
				return 0;
		if (fbuf[i] == ';') {
			i++;
			goto check;
		}
		if (fbuf[i++] == '\n')
			if (lookup(c)) {
				fputs("c program text", stdout);
				return 1;
			}
		if (i >= fbsz)
			return 0;
	}

	while (fbuf[i] != ')') {
		if (fbuf[i++] == '\n')
			if (lookup(c)) {
				fputs("c program text", stdout);
				return 1;
			}
		if (i >= fbsz)
			return 0;
	}

	while (fbuf[i] != '{') {
		if (fbuf[i++] == '\n')
			if (lookup(c)) {
				fputs("c program text", stdout);
				return 1;
			}
		if (i >= fbsz)
			return 0;
	}

	/*
	 * add a table look up to be more sure it's a C program
	 */
	i++;
	if (lookup(c)) {
		if (!lexck())
			fputs("c program text", stdout);
		return 1;
	}
	else {
		if (!lexck())
			fputs("ascii text", stdout);
		return 1;
	}
}

lexck()
{
	i = 0;
	while (fbuf[i] != '%'  && fbuf[i+1] != '%')
		while (fbuf[i++] != '\n')
			if (i >= fbsz)
				return 0;

	i += 2;
	while (fbuf[i++] != '\n')
			if (i >= fbsz)
				return 0;

	if (!yacck(i))
		fputs("lex command text", stdout);

	return 1;
}

sedck()
{
	char ch;

	i = 0;
	if (fbuf[i] == '/') {
		i++;
		while (fbuf[i++] != '/')
			if (i >= fbsz)
				return 0;
		if (lookup(sed)) {
			fputs("sed program text", stdout);
			return 1;
		}
		else
			return 0;
	}

	if (fbuf[i] <= '9' && fbuf[i] >= '0')
		while (fbuf[i] != '\n')
			if (i++ >= fbsz)
				return 0;

	if ((fbuf[i] == 'c' || fbuf[i] == 'a' || fbuf[i] == 'i') &&
	     fbuf[i+1] == '\\') {
		fputs("sed program text", stdout);
		return 1;
	}

	if (fbuf[i] == 's') {
		ch = fbuf[i+1];
		while (fbuf[i] != ch)
			if (i++ >= fbsz)
				return 0;
		while (fbuf[i] != ch)
			if (i++ >= fbsz)
				return 0;
		fputs("sed program text", stdout);
		return 1;
	}
	return 0;
}

awkck()
{
	i = 0;
	if (fbuf[i] == '/') {
		i++;
		while (fbuf[i] != '/') {
			i++;
			if (i >= fbsz)
				return 0;
		}
		fputs("awk program text", stdout);
		return 1;
	}

	while (fbuf[i] != '{') {
		i++;
		if (i >= fbsz)
			return 0;
	}

	while (fbuf[i] != '}') {
		i++;
		if (i >= fbsz)
			return 0;
	}

	fputs("awk program text", stdout);
	return 1;
}

#define STRMATCH(p, q) strncmp((p), (q), sizeof (q)-1)

/*
 * check for a line starting with '#!'; if found look for csh, ksh, sh,
 * awk or nawk
 */
sharpbangck()
{
	if (fbuf[0] == '#' && fbuf[1] == '!') {
		i = 2;
		while (fbuf[i] == ' ' || fbuf[i] == '\t')
			i++;
		if (STRMATCH(&fbuf[i], "/bin/csh") == 0 ||
		    STRMATCH(&fbuf[i], "/bin/ksh") == 0 ||
		    STRMATCH(&fbuf[i], "/bin/sh") == 0) {
			fputs("commands text", stdout);
			return 1;
		}
		if (STRMATCH(&fbuf[i], "/usr/bin/awk") == 0 ||
		    STRMATCH(&fbuf[i], "/usr/bin/nawk") == 0) {
			fputs("awk program text", stdout);
			return 1;
		}
	}
	return 0;
}

yacck(i)
int i;
{
	if (i >= fbsz)
		return 0;
	while (fbuf[i] == '\n' || fbuf[i] == ' ' || fbuf[i] == '\t')
		if (i++ >= fbsz)
			return 0;

	if (fbuf[i] == '%' && fbuf[i+1] != '{') {
		fputs("yacc program text", stdout);
		return 1;
	}

	while (fbuf[i++] != ':')
		if (i >= fbsz)
			return 0;

	while (fbuf[i++] != ';')
		if (i >= fbsz)
			return 0;
	return 0;
}

/*check for pascal program text*/

pasck()
{
	char savebuf[FBSZ];

	i = 0;
	while (savebuf[i] = fbuf[i])
		i++;

	/*
	 * Since Pascal isn't case sensitive, convert to all lower
	 * case.
	 */
	for (i = 0; i <= fbsz; i++)
		if (fbuf[i] >= 'A' && fbuf[i] <= 'Z')
			fbuf[i] = tolower(fbuf[i]);

	i = 0;
	while (!(lookup(pas))) {
		while ((isdigit(fbuf[i])) || fbuf[i] == ' ' ||
		       fbuf[i] == '\t' || fbuf[i] == '-')
			if (i++ >= fbsz) {
				i = 0;
				while (fbuf[i] = savebuf[i])
				        i++; /*restore fbuf*/
				return 0;
			}
		if (lookup(pas))
			break;
		else
			while (fbuf[i++] != '\n')
				if (i >= fbsz) {
					i = 0;
					while (fbuf[i] = savebuf[i])
					        i++; /*restore fbuf*/
					return 0;
				}
	}
	fputs("pascal program text", stdout);
	return 1;
}

/*check for basic program text*/

basck()
{
	i = 0;
	while (!lookup(bas))
		while (fbuf[i++] != '\n')
			if (i >= fbsz)
				return 0;
	fputs("basic program text", stdout);
	return 1;
}

/*
 * comprck() -- classify a file as the output of compress(CONTRIB)
 */
comprck()
{
	if (fbuf[0] == '\037' && fbuf[1] == '\235') {
		fputs("compressed data, ", stdout);
		if (fbuf[2]&0x80)
			fputs("block compressed, ", stdout);
		printf("%d bits", fbuf[2]&0x1f);
		return 1;
	}
	return 0;
}

/*
 * Determine whether the file is a tar file.
 * 25-Oct-83 FLB
 * Recoded 2 Jul 85 ACT
 */
tarck()
{
#define TBLOCK        512     /* This stuff is copied from tar.c. */
#define NAMSIZ        100
	register struct header {
		char name[NAMSIZ];
		char mode[8];
		char uid[8];
		char gid[8];
		char size[12];
		char mtime[12];
		char chksum[8];
		char linkflag;
		char linkname[NAMSIZ];
	} *bp;
	register int comp_chksum;
	register char *cp, *cp2;
	register int header_chksum;
	char buf[TBLOCK];

	cp2 = fbuf;
	for (cp = buf; cp <= &buf[TBLOCK-1];)
		*cp++ = *cp2++;     /* copy fbuf so as not to mung it */

	bp = (struct header *) buf;

	{
	    extern unsigned long strtoul();
	    char savec = *(bp->chksum + 8);

	    *(bp->chksum + 8) = '\0';
	    header_chksum = (int)strtoul(bp->chksum, NULL, 8);
	    *(bp->chksum + 8) = savec;
	}

	for (cp = bp->chksum; cp < &bp->chksum[8]; cp++)
		*cp = ' ';

	comp_chksum = 0;
	for (cp = buf; cp <= &buf[TBLOCK-1]; cp++)
		comp_chksum += *cp;

	if (comp_chksum == header_chksum) {
		fputs("tar file", stdout);
		return 1;
	}
	else
		return 0;
}

#ifdef ACLS
/*
 * aclexec-
 * return 1 if an execute bit is turned on in one of a file's entries.
 * return 0 if no executes or no entries.
 */
aclexec(file)
char *file;
{
	struct acl_entry acl[NACLENTRIES];
	int nentries;
	int count;

	if (nentries = getacl(file, NACLENTRIES, acl))
		for (count=0; count<nentries; count++) {
			if (acl[count].mode & X_OK)
				return 1;
		}
	return 0;
}
#endif /* ACLS */
