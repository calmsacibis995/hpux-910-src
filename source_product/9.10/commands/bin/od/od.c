static char *HPUX_ID = "@(#) $Revision: 70.3 $";
/*
 * od -- octal (also hex, decimal, and character) dump
 * xd -- hexadecimal (also octal, decimal, and character) dump
 */

#include <sys/param.h>
#include <sys/stat.h>
#include <stdio.h>
#include <nl_ctype.h>
#include <unistd.h>
#include <signal.h>

#if defined(NLS) || defined(NLS16)
#define	NL_SETN 1		/* set number */
#include <locale.h>
#include <setlocale.h>
nl_catd nlmsg_fd;
#else
#define catgets(i,sn,mn,s) (s)
#endif

/* it is assumed that the INPBLK is a multiple of the least common
 * multiple of the number of bytes transformed by the specific data types
 */
#define	INPBLK	16
#define BBLOCK	512
#define KBLOCK	1024
#define MBLOCK	1048576
#define	MAXOPTS	32
#define CX	3	/* display space for a character - changing this to
			 * anything else will format all others correspondingly
			 */

char *strrchr();
extern int opterr, optind;
extern char *optarg;

union {
	unsigned short  word[8];
	short		sword[INPBLK];
	int		iword[INPBLK];
	long		lword[INPBLK];
	float		fword[INPBLK];
	double		dword[INPBLK];
	long double	bword[INPBLK];
	unsigned char	inbuf[BBLOCK];
} w;
unsigned char	lbuf[BBLOCK];
int     conv;
int     a_base = 8;
int     c_base = 8;
int     c_ndig = 3;
int     xd = 0;
int     max = 0;
long    addr = 0;
char    *program_name;
int	Abase = 8;		/* address base for -A option */
int	Nflag = 0, new = 0;
int	Lbyte = 0;
unsigned long	Nbytes = 0, Sbytes = 0;
unsigned long topts[MAXOPTS];
int nopts = 0, inpblk;
int verbose = 0;
int status = 0;

#define	DOPTC	0x00000001
#define DOPTS	0x00000002
#define	DOPTI	0x00000004
#define DOPTL	0x00000008
#define	OOPTC	0x00000010
#define	OOPTS	0x00000020
#define	OOPTI	0x00000040
#define OOPTL	0x00000080
#define	UOPTC	0x00000100
#define	UOPTS	0x00000200
#define	UOPTI	0x00000400
#define UOPTL	0x00000800
#define	XOPTC	0x00001000
#define	XOPTS	0x00002000
#define	XOPTI	0x00004000
#define XOPTL	0x00008000
#define	FOPTF	0x00020000
#define	FOPTD	0x00040000
#define FOPTL	0x00080000

int bytes_xform[][2] = {{'C',sizeof(char)}, {'S',sizeof(short)},
{'I',sizeof(int)}, {'L',sizeof(long)}, {0,0}, {'F',sizeof(float)},
{'D',sizeof(double)}, {'B',sizeof(long double)}, {0,0}};

main(argc, argv)
char **argv;
{
        register char c;
        register int rc, f, same;
	int j;
	char *p;
	int stdflg = 0;

	topts[0] = OOPTS;
	inpblk = (sizeof(long double) > INPBLK) ? sizeof(long double)
		 : INPBLK;
        program_name = argv[0];                 /* name or program (xd/od) */
        p = strrchr(program_name, '/');         /* find the last / */
        if (p!=NULL)                            /* if there is indeed one: */
                program_name = p+1;             /* just use last component */

#if defined(NLS) || defined(NLS16)     /* initialize to the current locale */
	if (!setlocale(LC_ALL, "")) {
		fputs(_errlocale("od"), stderr);
		putenv("LANG=");
		nlmsg_fd = (nl_catd)(-1);
	} else
        	nlmsg_fd = catopen("od", 0);
#endif /* NLS || NLS16 */

#ifdef STANDALONE
        if (argv[0][0] == '\0')
                argc = getargv("od", &argv, 0);
#endif

        if (strcmp(program_name, "xd") == 0) {
                xd = 1;
		Abase = 16;
                a_base = 16;
                c_base = 16;
		c_ndig = 2;
        }

	/* Some sytems generate SIGFPE if the
	 * the input bytes cannot be transformed to floating point type */
	signal(SIGFPE, SIG_IGN);	/* ignore the signal */
	new = 0;
	f = 0;
	while ((c = getopt(argc, argv, "odsxhcbvA:j:N:t:")) != EOF)
	{
	   switch(c) {
		case 'o':
			if(new) errmsg(2,1);
			conv |= 001;
			f = 6;
			break;
		case 'd':
			if(new) errmsg(2,1);
			conv |= 002;
			f = 5;
			break;
		case 's':
			if(new) errmsg(2,1);
			conv |= 004;    /* signed dec. */
			f = 2*c_ndig + 1;
			break;
		case 'x':
		case 'h':
			if(new) errmsg(2,1);
			conv |= 010;
			f = 4;
			break;
		case 'c':
			if(new) errmsg(2,1);
			conv |= 020;
			f = 2*c_ndig + 1;
			break;
		case 'b':
			if(new) errmsg(2,1);
			conv |= 040;
			f = 2*c_ndig + 1;
			break;
		case 't':
			new++;
			get_t_options(optarg);
			break;
		case 'A':
			new++;
			switch(*optarg) {
			   case 'x':
				Abase = 16;
				break;
			   case 'o':
				Abase = 8;
				break;
			   case 'd':
				Abase = 10;
				break;
			   case 'n':
				Abase = 0;
				break;
			   default:
				errmsg(3,1);
			}
			break;
		case 'j':
			new++;
			if((Sbytes = strtol(optarg, &p, 0)) <= 0)
				errmsg(4,1);
			switch(*p) {
			   case 'b':
				Sbytes *= BBLOCK;
				break;
			   case 'k':
				Sbytes *= KBLOCK;
				break;
			   case 'm':
				Sbytes *= MBLOCK;
				break;
			}
			break;
		case 'N':
			new++;
			Nflag++;
			if((Nbytes = strtol(optarg, &p, 0)) <= 0)
				errmsg(5,1);
			break;
		case 'v':
			new++;
			verbose++;
			break;
		default:
			errmsg(6,1);
			break;
	   }
	   if(f > max)
		max = f;
	}

        if(!conv && !new) {
                if (xd) {
                        max = 4;
                        conv = 010;
                }
                else {
                        max = 6;
                        conv = 001;
                }
        }
	if(new && nopts == 0) nopts = 1;

	/* check out old version first : od -options file +offset */
	if (!new) {
	    /* If no file specified (ie using stdin), then must have + before
	     * offset to use old format */
	    if (((argc - optind) >= 2) || 
		((argc - optind) == 1) && (*argv[argc-1] == '+')) {
	        if (offset(argv[argc-1])) {
		    argc--;  /* offset if present, so remove it from arg list*/
	        }
	    }
	}

	if(optind >= argc) {
		stdflg = 1;	/* no file present, process stdin */
		argv[optind] = "stdin";
	}

	do {			/* process all files */
	   unsigned long a, b;
	   same = -1;
	   if(!new) addr = 0;	/* else it is cumulative across input files */
	   if(!stdflg)
		if(freopen(argv[optind], "r", stdin) == NULL) {
			status = errmsg(11, 0, argv[optind]);
			goto nextfile;
	   	}

	   /* skip the number of bytes indicated */
	   if(Sbytes) {
	        if(!new) addr = Sbytes;
	        a = Sbytes;
	        while (a > 0) {
		  rc = (a > BBLOCK) ? BBLOCK : a;
		  if (rc != fread(w.inbuf, 1, rc, stdin)) {
			status = errmsg(1, 0, argv[optind]);
		        goto nextfile;
		  }
		  a -= rc;
	        }
	   }

	   if(Nbytes != 0) b = Nbytes;		/* read this many bytes */
	   else b = inpblk;

	   /* dual buffers are used for supporting wide character outputs 
	    * assuming ofcourse that wide chars will not be 16 bytes long.
	    */
	   rc = (b > inpblk) ? inpblk : b;
	   if((j = fread(&w.inbuf[inpblk], 1, rc, stdin)) != rc) {
		/* Error message should not be printed??
		 * for older version compatibility.
		if(j <= 0) {
			status = errmsg(7, 0, argv[optind]);
		        goto nextfile;
		}
		*/
		;	/* do nothing */
	   }
	   while (b > 0) {
	        if(j == 0) break;		/* premature end of file */
	        /* copy second buffer to first buffer */
	        memcpy(w.inbuf, &w.inbuf[inpblk], j);
	        if(Nbytes != 0) b -= j;
	        if(j != inpblk) {
			Lbyte = j;
			rc = 0;
	        }
	        else {	/* read second buffer */
			rc = (b > inpblk) ? inpblk : b;
	        	rc = fread(&w.inbuf[inpblk], 1, rc, stdin);
	       		Lbyte = inpblk + rc;
	        }
		if (same>=0) {
			if(new && verbose) goto notsame;
			for (f=0; f<inpblk; f++)
				if (lbuf[f] != w.inbuf[f])
					goto notsame;
			if (same==0) {
				printf("*\n");
				same = 1;
			}
			addr += j;
			j = rc;		/* for next processing */
			continue;
		}
	    notsame:
		/* Pass along the actual number of bytes as an extra param*/
		if(!new)
		  line(addr, (j+sizeof(w.word[0])-1)
		       /sizeof(w.word[0]), j);
		else
		  xline(addr, j);

	        addr += j;
		j = rc;		/* for next processing */
		same = 0;
		for (f=0; f<inpblk; f++)
			lbuf[f] = w.inbuf[f];
		for (f=0; f<inpblk; f++)
			w.inbuf[f] = 0;
	    }
	    if(new) {
		if(Abase) putn(addr, Abase, 7);
	    } else putn(addr, a_base, 7);
	    putchar('\n');
nextfile:
	    optind++;
	} while (optind < argc);
        exit(status);
}

line(a, n, byte_count)
long a;
int n, byte_count;
{
register unsigned short *wp = w.word;
register i, f, c;

        f = 1;
        for(c=1; c; c<<=1) {
                if((c&conv) == 0)
                        continue;
                if(f) {
                        putn(a, a_base, 7);
                        putchar(' ');
                        f = 0;
                } else
                        putchar('\t');
                for (i=0; i<n; i++) {
                        /*
                         * If we end up with an odd number of bytes,
                         * then let putx() know that we only have
                         * half a word to work with.
                         */
                        if ((i*2+1) == byte_count)
                                putx(wp[i], c, 0);
                        else
                                putx(wp[i], c, 1);
                        putchar(i==n-1? '\n': ' ');
                }
        }
}

putx(n, c, full_word)
unsigned n;
{

        switch(c) {
        case 001:
                pre(6);
                putn((long)n, 8, 6);
                break;
        case 002:
                pre(5);
                putn((long)n, 10, 5);
                break;
        case 004:
                if(n > 32767){pre(6); putchar('-'); n = (~n + 1) & 0177777;}
                else pre(5);
                putn((long)n,10,5);
                break;
        case 010:
                pre(4);
                putn((long)n, 16, 4);
                break;
        case 020:
                pre(2*c_ndig + 1);
                {
                        unsigned short sn = n;
                        cput(*(char *)&sn);
                        /* Display second byte only if full word given */
                        if (full_word)
                        {
                                putchar(' ');
                                cput(*((char *)&sn + 1));
                        }
                        break;
                }
        case 040:
                pre(2*c_ndig + 1);
                {
                        unsigned short sn = n;
                        putn((long)(*(char *)&sn)&0377, c_base, c_ndig);
                        /* Display second byte only if full word given */
                        if (full_word)
                        {
                                putchar(' ');
                                putn((long)(*((char *)&sn + 1))&0377, c_base, c_ndig);
                        }
                        break;
                }
        }
}

cput(c)
{
        char *s;

        c &= 0377;
#ifdef NLS
        if(isprint(c)) {
#else /* NLS */
        if(c>037 && c<0177) {
#endif /* NLS */
                s = "  ";
                s[1] = c;
        }
        else {
                switch(c) {
                case '\0':
                        s = "\\0";
                        break;
                case '\b':
                        s = "\\b";
                        break;
                case '\f':
                        s = "\\f";
                        break;
                case '\n':
                        s = "\\n";
                        break;
                case '\r':
                        s = "\\r";
                        break;
                case '\t':
                        s = "\\t";
                        break;
                default:
                        putn((long)c, c_base, c_ndig);
                        return;
                }
        }

        if (c_ndig == 3)
                printf(" ");
        printf("%s",s);
}

putn(n, b, c)
long n;
{
        register d;

        if(!c)
                return;
        putn(n/b, b, c-1);
        d = n%b;
        if (d > 9)
                putchar(d-10+'a');
        else
                putchar(d+'0');
}

pre(n)
{
        int i;

        for(i=n; i<max; i++)
                putchar(' ');
}

offset(s)
register char *s;
{
        register char *p;
        register int d;
	char *possible_path;
	int block_specified = 0;
	int base_specified = 0;
	int new_base = a_base;   /* keep new base separate until we decide
			          * that we really have a new base */
	struct stat statbuf;

	possible_path = s;
        if (*s == '+') s++;
        if (*s=='x') {
                s++;
		new_base = 16;
		base_specified = 1;
        } else if (*s=='0' && (s[1]=='x' || s[1]=='X')) {
                s += 2;
		new_base = 16;
		base_specified = 1;
        } else if (*s == '0') {
	        new_base = 8;
		base_specified = 1;
	}
        p = s;
        while(*p) {
                if (*p++=='.') {
		        new_base = 10;
			base_specified = 1;
		}
        }
        for (Sbytes=0; *s; s++) {
                d = *s;
                if(d>='0' && d<='9')
                        Sbytes = Sbytes*new_base + d - '0';
                else if (d>='a' && d<='f' && new_base==16)
                        Sbytes = Sbytes*new_base + d + 10 - 'a';
		else if(d == '.')
			continue;
		else if((d == 'b' || d == 'B') && *(s+1) == '\0') {
			Sbytes *= BBLOCK;
			block_specified = 1;
		}
                else {
			Sbytes = 0;		/* error in offset */
                        return(0);
		}
        }

	/* was there anything in the string that indicates the old
	 * style offset string
	*/
	if (!base_specified && !block_specified && !Sbytes)
	      return(0);   /* no, so return no */

	/* if we got valid offset, return okay */
	if (Sbytes) 
	{
	      a_base = new_base;
	      return(1);
	}

	/* sbytes is 0, but the string did contain a new base or block 
	 * indicator.  But it could just be a 2nd filename that has
	 * a "." or "B" or "b" or "0x" etc.
	*/
	if (stat(possible_path, &statbuf) < 0)
	{
	      /* not a valid file, so we will accept it */
	      a_base = new_base;
	      return(1);
	}
	return(0);
}
/* ------------------------------------------------------------------- */
/*
 * POSIX conformance additions
 *
 */
#define	DOUX	0x0000ffff	/* d, o, u, x options */
#define	DOPT	0x0000000f
#define	OOPT	0x000000f0
#define	UOPT	0x00000f00
#define	XOPT	0x0000f000
#define	FOPT	0x000f0000
#define	AOPT	0x00100000
#define	COPT	0x00200000

int
get_t_options(targ)
char *targ;
{
	while(*targ) {
	    switch(*targ++) {
		case 'a':
			topts[nopts++] = AOPT;
			break;
		case 'c':
			topts[nopts++] = COPT;
			break;
		case 'd':
			check_modifier(DOPT, &targ, 0);
			break;
		case 'o':
			check_modifier(OOPT, &targ, 0);
			break;
		case 'u':
			check_modifier(UOPT, &targ, 0);
			break;
		case 'x':
			check_modifier(XOPT, &targ, 0);
			break;
		case 'f':
			if(*targ == 'L') *targ = 'B';  /* 'L' is ambiguous */
			check_modifier(FOPT, &targ, 5);
			break;
		default:
			errmsg(8,1);
			break;
	    }
	}
}

check_modifier(option, targp, indx)
unsigned long option;
register char **targp;
int indx;
{
int num = 0, i;

	while(**targp >= '0' && **targp <= '9')
		num = num*10 + *((*targp)++) - '0';
	if(num) {
	   for(i = indx; bytes_xform[i][0]; i++)
	   	if(num == bytes_xform[i][1]) {
		     *(--(*targp)) = bytes_xform[i][0];
		     break;
		}
	   if(bytes_xform[i][0] == 0) errmsg(9,1);
	}

	if(option == FOPT) i = 0x00010000;
	else i = 0x00001111;		/* C modifier flag */

	switch(**targp) {
		case 'L': i <<= 1;	/* 0x8888 is L flag */
		case 'I': i <<= 1;	/* 0x4444 is I flag */
		case 'S': i <<= 1;	/* 0x2222 is S flag */
		case 'C':
			if(!(option & DOUX)) errmsg(10,1);
			break;
		case 'B': i <<= 1;	/* 0x80000 is B flag */
		case 'D': i <<= 1;	/* 0x40000 is D flag */
		case 'F': i <<= 1;	/* 0x20000 is F flag */
			if(option != FOPT) errmsg(10,1);
			break;
		default:
			/* if no modifier is present assume defaults
			 * Int is default for d, o, u and x
			 * Double is default for f
			 */
			i = 0x44444;	/* all defaults */
			(*targp)--;	/* go back to the specifier */
			break;
	}
	topts[nopts++] = option & i;
	(*targp)++;
}

char *errstr[] = {
"",
"%s: Error reading file %s\n"	,			/* catgets 1 */
"%s: Obsolescent forms and new forms cannot be combined\n", /* catgets 2 */
"%s: Illegal argument for -A option\n",			/* catgets 3 */
"%s: Illegal value for -j option\n",			/* catgets 4 */
"%s: Illegal value for -N option\n",			/* catgets 5 */
"Usage: %s [-v] [-A base] [-j skip] [-N count] [-t type_string]... [file...]\n", 							/* catgets 6 */
"%s: End of file reached for input: %s\n",		/* catgets 7 */
"%s: Invalid argument for -t option\n",			/* catgets 8 */
"%s: Invalid numeric modifier\n",			/* catgets 9 */
"%s: Invalid type modifier used\n",			/* catgets 10 */
"%s: cannot open %s\n"					/* catgets 11 */
};

/* VARARGS */
int
errmsg(indx, flag, parm)
int indx, flag;
char *parm;
{
	fprintf(stderr, (catgets(nlmsg_fd, NL_SETN, indx, errstr[indx])),
		program_name, parm);
	if(flag) exit(indx);
	return(indx);
}

xline(a, n)
long a;
int n;
{
register f, c;

        f = 1;
        for(c=0; c<nopts; c++) {
                if(f && Abase) {
                        putn(a, Abase, 7);
                        putchar(' ');
                        f = 0;
                } else
                        putchar('\t');

		xputx(topts[c], n);
		putchar('\n');
        }
}

char *named_char[] =  {     "nul", "soh", "stx", "etx", "eot", "enq", "ack",
"bel", " bs", " ht", " nl", " vt", " ff", " cr", " so", " si", "dle", "dcl",
"dc2", "dc3", "dc4", "nak", "syn", "etb", "can", " em", "sub", "esc", " fs",
" gs", " rs", " us", " sp", "del" };

union {
	short		sword;
	int		iword;
	long		lword;
	float		fword;
	double		dword;
	long double	bword;
	unsigned char	bufx[sizeof(long double)];
} fw;

xputx(c, n)
unsigned long c;
int n;
{
register int i, f;
register char ch;

    /* space formatting is done based on the no. of bytes in the
     * smallest displayable field (char). The screen space allocated 
     * for char is CX. This may be changed to any value (#defined). It is
     * currently set to 3 (octal display of unprintable chars) + blank for
     * delimiters. sizeof(char) is assumed to be 1 byte implicitly.
     * the var f contains the number of characters that can be displayed
     * for that field size.
     */
    switch(c) {
	case AOPT:	/* named character */
		for(i=0; i<n; i++) {
		    ch = w.inbuf[i] & 0x7f;	/* ignore MSB */
		    if(ch < 33) printf(" %*.*s",CX,CX, named_char[ch]);
		    else if(ch == 0177) printf(" %*.*s",CX,CX, named_char[33]);
		    else printf(" %*c",CX, ch);
		}
		break;
	case COPT:
		process_c(n);
		break;
	case DOPTC:
		f = CX;
		for(i=0; i<n; i++)
			printf(" %*d", f, w.inbuf[i]);
		break;
	case DOPTS:
		f = sizeof(short)*CX + sizeof(short)-1;
		for(i=0; i<n/sizeof(short); i++)
			printf(" %*hd", f, w.sword[i]);
		if(fillin(sizeof(short),n))
			printf(" %*hd", f, fw.sword);
		break;
	case DOPTI:
		f = sizeof(int)*CX + sizeof(int)-1;
		for(i=0; i<n/sizeof(int); i++)
			printf(" %*d", f, w.iword[i]);
		if(fillin(sizeof(int),n))
			printf(" %*d", f, fw.iword);
		break;
	case DOPTL:
		f = sizeof(long)*CX + sizeof(long)-1;
		for(i=0; i<n/sizeof(long); i++)
			printf(" %*ld", f, w.lword[i]);
		if(fillin(sizeof(long),n))
			printf(" %*ld", f, fw.lword);
		break;
	case OOPTC:
		f = CX;
		for(i=0; i<n; i++)
			printf(" %0*o", f, w.inbuf[i]);
		break;
	case OOPTS:
		f = sizeof(short)*CX + sizeof(short)-1;
		for(i=0; i<n/sizeof(short); i++)
			printf(" %0*ho", f, w.sword[i]);
		if(fillin(sizeof(short),n))
			printf(" %0*ho", f, fw.sword);
		break;
	case OOPTI:
		f = sizeof(int)*CX + sizeof(int)-1;
		for(i=0; i<n/sizeof(int); i++)
			printf(" %0*o", f, w.iword[i]);
		if(fillin(sizeof(int),n))
			printf(" %0*o", f, fw.iword);
		break;
	case OOPTL:
		f = sizeof(long)*CX + sizeof(long)-1;
		for(i=0; i<n/sizeof(long); i++)
			printf(" %0*lo", f, w.lword[i]);
		if(fillin(sizeof(long),n))
			printf(" %0*lo", f, fw.lword);
		break;
	case UOPTC:
		f = CX;
		for(i=0; i<n; i++)
			printf(" %*u", f, w.inbuf[i]);
		break;
	case UOPTS:
		f = sizeof(short)*CX + sizeof(short)-1;
		for(i=0; i<n/sizeof(short); i++)
			printf(" %*hu", f, w.sword[i]);
		if(fillin(sizeof(short),n))
			printf(" %*hu", f, fw.sword);
		break;
	case UOPTI:
		f = sizeof(int)*CX + sizeof(int)-1;
		for(i=0; i<n/sizeof(int); i++)
			printf(" %*u", f, w.iword[i]);
		if(fillin(sizeof(int),n))
			printf(" %*u", f, fw.iword);
		break;
	case UOPTL:
		f = sizeof(long)*CX + sizeof(long)-1;
		for(i=0; i<n/sizeof(long); i++)
			printf(" %*lu", f, w.lword[i]);
		if(fillin(sizeof(long),n))
			printf(" %*lu", f, fw.lword);
		break;
	case XOPTC:
		f = CX;
		for(i=0; i<n; i++)
			printf(" %*x", f, w.inbuf[i]);
		break;
	case XOPTS:
		f = sizeof(short)*CX + sizeof(short)-1;
		for(i=0; i<n/sizeof(short); i++)
			printf(" %*hx", f, w.sword[i]);
		if(fillin(sizeof(short),n))
			printf(" %*hx", f, fw.sword);
		break;
	case XOPTI:
		f = sizeof(int)*CX + sizeof(int)-1;
		for(i=0; i<n/sizeof(int); i++)
			printf(" %*x", f, w.iword[i]);
		if(fillin(sizeof(int),n))
			printf(" %*x", f, fw.iword);
		break;
	case XOPTL:
		f = sizeof(long)*CX + sizeof(long)-1;
		for(i=0; i<n/sizeof(long); i++)
			printf(" %*lx", f, w.lword[i]);
		if(fillin(sizeof(long),n))
			printf(" %*lx", f, fw.lword);
		break;
	case FOPTF:
		f = sizeof(float)*CX + sizeof(float)-1;
		for(i=0; i<n/sizeof(float); i++)
			printf(" %*.8e", f, w.fword[i]);
		if(fillin(sizeof(float),n))
			printf(" %*.8e", f, fw.fword);
		break;
	case FOPTD:
		f = sizeof(double)*CX + sizeof(double)-1;
		for(i=0; i<n/sizeof(double); i++)
			printf(" %*.16e", f, w.dword[i]);
		if(fillin(sizeof(double),n))
			printf(" %*.16e", f, fw.dword);
		break;
	case FOPTL:
		f = sizeof(long double)*CX + sizeof(long double)-1;
		for(i=0; i<n/sizeof(long double); i++)
			printf(" %*.34Le", f, w.bword[i]);
		if(fillin(sizeof(long double),n))
			printf(" %*.34Le", f, fw.bword);
		break;
	}
}

int
fillin(s, n)
register int s, n;
{
register int p, i;

	p = n%s;		/* any remainder? */
	if (p == 0) return(0);
	for(i=0; i<p; i++)
	   fw.bufx[i] = w.inbuf[n-p+i];
				/* extend remaining with nulls. */
	for(i=p; i<s; i++)
	   fw.bufx[i] = 0;
	return(1);
}

int cflag = 0;
process_c(n)
int n;
{
register int i, j;
register unsigned char *cptr, c;
char *s=" \\ ";

	cptr = &w.inbuf[cflag];		/* cflag remembers wide chars 
					 * already displayed */
	j = 0;
	for(i=0; cflag; i++, cflag--)
		printf(" %*.*s",CX,CX, " **");
		
	for(; i<n; i++) {
#if defined(NLS) || defined(NLS16)
		j = mblen(cptr, (Lbyte - i));	/* get char length */
#endif
		c = 0;
		if(j > 1) {			/* multibyte characters */
		   c = cptr[j];			/* save this byte */
		   cptr[j] = '\0';		/* null terminate it */
		   printf(" %*.*s",CX,CX,cptr);	/* display it */
		   cptr += j;
		   *cptr = c;			/* restore saved byte */
		   cflag = j - 1;		/* no. of asterisk fills */
		   for(; (i<n-1 && cflag); i++,cflag--)
			printf(" %*.*s",CX,CX," **");
		   continue;
		}
		else {				/* single byte chars */
		   if(isprint(*cptr))
			printf(" %*c",CX, *cptr);
		   else if(*cptr == '\0')
			printf(" %.*s",CX, " \\0");
		   else {
			switch(*cptr) {
			    case '\\':  c = '\\';
					break;
			    case '\a':  c = 'a';
					break;
			    case '\b':  c = 'b';
					break;
			    case '\f':  c = 'f';
					break;
			    case '\n':  c = 'n';
					break;
			    case '\r':  c = 'r';
					break;
			    case '\t':  c = 't';
					break;
			    case '\v':  c = 'v';
					break;
			    default:		/* octal values */
					printf(" %0*o",CX, *cptr);
					break;
			}
			if (c) {
				s[2] = c;
				printf(" %*.*s",CX,CX, s);
			}
		   }
		}
		cptr++;
	}

}
