#ifndef lint
static char *HPUX_ID = "@(#) $Revision: 70.7 $";
#endif

/*
 *	PR command (print files in pages and columns, with headings)
 *	2+head+2+page[56]+5
 */

#include <stdio.h>

#ifdef NLS || NLS16
#define NL_SETN 1	/* message set number */
#define MAXLNAME 14	/* maximum length of a language name */
#include <nl_ctype.h>	/* for byte status */
#include <locale.h>
#include <nl_types.h>
nl_catd nlmsg_fd;
nl_catd nlmsg_tfd;
#else
#define catgets(i, sn,mn,s) (s)
#endif NLS || NLS16

#define ESC	'\033'
#define LENGTH	66
#define LINEW	72
#define NUMW	5
#define MARGIN	10
#define DEFTAB	8

FILE *fopen(), *mustopen();
char nulls[] = "";
typedef struct { FILE *f_f; char *f_name; int f_nextc; } FILS;
FILS *Files;
int Multi = 0, Nfiles = 0, Error = 0, onintr();

#include <signal.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/stat.h>
typedef unsigned char CHAR; /* 8-bit: use unsigned char processing */
typedef int ANY;
typedef unsigned UNS;
#define NFILES	10
int Mode;
char *ttyname(), *Ttyout, obuf[BUFSIZ];
#define istty(F)	ttyname(fileno(F))
/* ARGSUSED */
fixtty(argc, argv) char **argv;
{
	struct stat sbuf;

	setbuf(stdout, obuf);
	if (signal(SIGINT, SIG_IGN) != SIG_IGN) signal(SIGINT, onintr);
	if (Ttyout= istty(stdout)) {
		stat(Ttyout, &sbuf);
		Mode = sbuf.st_mode&0777;
		chmod(Ttyout, 0600);
	}
	return (argc);
}

#define done()	if (Ttyout) chmod(Ttyout, Mode)
#define INTREXIT	_exit
char *GETDATE() /* return date file was last modified */
{
#if defined NLS || defined NLS16
	char *nl_cxtime();
#else
	char *ctime();
#endif
	static char *now = NULL;
	static struct stat sbuf, nbuf;

	if (Nfiles > 1 || Files->f_name == nulls) {
		if (now == NULL) { 
			time(&nbuf.st_mtime);
#if defined NLS || defined NLS16
			now = nl_cxtime(&nbuf.st_mtime, (catgets(nlmsg_tfd,NL_SETN,1,"%h %d %H:%M 19%y")));
			if(*(now + strlen(now) - 1) == '\n')
				*(now + strlen(now) - 1) = '\0';
#else
			now = ctime(&nbuf.st_mtime);
#endif
		}
		return (now);
	} else {
		stat(Files->f_name, &sbuf);
#if defined NLS || defined NLS16
		now = nl_cxtime(&sbuf.st_mtime, (catgets(nlmsg_tfd,NL_SETN,2,"%h %d %H:%M 19%y")));
		if(*(now + strlen(now) - 1) == '\n')
			*(now + strlen(now) - 1) = '\0';
		return (now);
#else
		return (ctime(&sbuf.st_mtime));
#endif
	}
}

#define CADDID()
#if defined NLS || defined NLS16
#define HEAD	(catgets(nlmsg_fd,NL_SETN,3, "%s  %s Page %d\n\n\n")), date, head, Page
#else
#define HEAD	"%12.12s %4.4s  %s Page %d\n\n\n", date+4, date+20, head, Page
#endif
#define TOLOWER(c)	(isupper(c) ? tolower(c) : c)	/* ouch! */
#define cerror(S)	fprintf(stderr, "pr: %s", S)

char *ffiler(s) char *s;
{
	static char buf[100];

	sprintf(buf, (catgets(nlmsg_fd,NL_SETN,4,"can't open %s")), s);
	return (buf);
}

#define STDINNAME()	nulls
#define TTY	"/dev/tty", "r"
#define PROMPT()	putc('\7', stderr) /* BEL */
#define NO_FILE	nulls
#ifdef NLS16
#define TABS(N,C)	if ((N = intopt(optarg+1, C)) < 0) N = DEFTAB
#define EQTAB(c)  ((c[0] == Etabc[0]) && (c[1] == Etabc[1])) ? 1 : 0
#else  NLS16
#define TABS(N,C)	if ((N = intopt(optarg+1, &C)) < 0) N = DEFTAB
#endif NLS16
#define ETABS	(Inpos % Etabn)
#define ITABS	(Itabn > 0 && Nspace >= (nc = Itabn - Outpos % Itabn))
#define NSEPC	'\t'

ANY *getspace();

extern int optind, opterr, optopt;
extern char *optarg;

main(argc, argv) char *argv[];
{
	FILS fstr[NFILES];
	int nfdone = 0;

#ifdef NLS || NLS16			/* initialize to the current locale */
	unsigned char lctime[5+4*MAXLNAME+4], *pc;
	unsigned char savelang[5+MAXLNAME+1];

	if (!setlocale(LC_ALL, "")) {		/* setlocale fails */
		fputs(_errlocale("pr"), stderr);
		putenv("LANG=");
		nlmsg_fd = (nl_catd)-1;		/* use default messages */
		nlmsg_tfd = (nl_catd)-1;
	} else {				/* setlocale succeeds */
		nlmsg_fd = catopen("pr", 0);	/* use $LANG messages */
		strcpy(lctime, "LANG=");	/* $LC_TIME affects some msgs */
		strcat(lctime, getenv("LC_TIME"));
		if (lctime[5] != '\0') {	/* if $LC_TIME is set */
			strcpy(savelang, "LANG=");	/* save $LANG */
			strcat(savelang, getenv("LANG"));
			if ((pc = strchr(lctime, '@')) != NULL) /*if modifier*/
				*pc = '\0';	/* remove modifer part */
			putenv(lctime);		/* use $LC_TIME for some msgs */
			nlmsg_tfd = catopen("pr", 0);
			putenv(savelang);	/* reset $LANG */
		} else				/* $LC_TIME is not set */
			nlmsg_tfd = nlmsg_fd;	/* use $LANG messages */
	}
#endif NLS || NLS16
	Files = fstr;
	for (argc = findopt(argc, argv), argv=&argv[optind]; argc > 0; --argc, ++argv)
		if (Multi == 'm') {
			if (Nfiles >= NFILES - 1) die((catgets(nlmsg_fd,NL_SETN,5,"too many files")));
			if (mustopen(*argv, &Files[Nfiles++]) == NULL)
				++nfdone; /* suppress printing */
		} else {
			if (print(*argv))
				fclose(Files->f_f);
			++nfdone;
		}

	/*
	 *  Open stdin if the -m option is specified but no files
	 *  are given.  Must do this since, print(NO_FILE) does not
	 *  open stdin when the -m option is specified.
	 */

	if (Multi == 'm'  &&  Nfiles == 0)
		if (mustopen(NO_FILE, &Files[Nfiles++]) == NULL)
			++nfdone; /* suppress printing */

	if (!nfdone) /* no files named, use stdin */
		print(NO_FILE); /* on GCOS, use current file, if any */
	errprint(); /* print accumulated error reports */
	exit(Error);
}

int potential_num (p)
char *p;
{
	while ( *p!='\0' ) 
	    if (!isdigit(*p++))
		return(0);
	return(1);
}

int potential_arg (p)
char *p;
{
	int bst=0;

	bst = BYTE_STATUS(*p, bst);
	if ( bst==ONEBYTE ) {
	   if (!isdigit(*p)) 
		if ( *(++p) == '\0' )
		   return(1);
	   if (!potential_num(p))
		   return(0);
	   return (1);
	}
	else {
	   if ( FIRSTof2(*p) && SECof2(*(++p))) {
		if ( *(++p) == '\0' )
		   return(1);
		if (!potential_num(p))
		   return(0);
		return(1);
	   }
	   return(0);
	}
}

long Lnumb = 0;
FILE *Ttyin = stdin;
int Dblspace = 1, Fpage = 1, Formfeed = 0, 
	Length = LENGTH, Linew = 0, Offset = 0, Ncols = 1, Pause = 0, 
	Colw, Plength, Margin = MARGIN, Numw, Report = 1, 
	Etabn = 0, Itabn = 0; 
#ifdef NLS16
int Sepc  [2] = {0, 0};
int Nsepc [2] = {'\t', 0};
int Etabc [2] = {'\t', 0};
int Itabc [2] = {'\t', 0};
#else NLS16
int	Sepc = 0, Nsepc = NSEPC, Etabc = '\t', Itabc = '\t';
#endif NLS16

char *Head = NULL;
CHAR *Buffer = NULL, *Bufend;
typedef struct { CHAR *c_ptr, *c_ptr0; long c_lno; } *COLP;
COLP Colpts;


/*
 * This routine was siginifcantly changed to have it use getopt. This was 
 * done for POSIX.2 work.  In doing this work, a number of routines were 
 * changed (atoix, intopt, TABS macro), and two routines were added 
 * (potential_num, potential_arg).  AL 1/20/92
 */
findopt(argc, argv) char *argv[];
{
	char **eargv = argv;
	int eargc = 0, c=0;
	unsigned char tabchar;
	int curr_opt, bstatus;

	argc = fixtty(argc, argv);

	opterr=0;
	do {
	   curr_opt = optind;
	   while (c != EOF &&
	         (c=getopt(argc,argv,":abqjmdpfFrte:i:n:w:o:l:h:s:x:")) != EOF ) {
		switch (c) {
		   case 'd': Dblspace = 2; break;
		   case 'e': 
		      bstatus=0;
		      tabchar=*optarg;
		      bstatus = BYTE_STATUS(tabchar, bstatus);
/* 
 * Since the option argument can be potentially missing (there are defaults
 * for the arguments), check for such a case.
 */
		      if (tabchar == '-' || curr_opt<optind-1) {
			   optind--;
			   Etabn = DEFTAB;
			   break;
		      }
		      if (!isdigit(tabchar)) {
			   Etabc[0]=(unsigned)tabchar;
			   if (bstatus == FIRSTOF2)
				Etabc[1]=(unsigned)*(optarg++);
			   optarg++;
		      }
		      if ((Etabn=atoix(optarg))<=0)
			   Etabn = DEFTAB;
		      break;
		   case 'f': /* retained for historical reasons */
		   case 'F': ++Formfeed; break;
		   case 'h': Head = optarg; break;
		   case 'i': 
		      bstatus=0;
		      tabchar=*optarg;
		      bstatus=BYTE_STATUS(tabchar, bstatus);
/* 
 * Since the option argument can be potentially missing (there are defaults
 * for the arguments), check for such a case.
 */
		      if (tabchar == '-' || curr_opt<optind-1) {
			   optind--;
			   Itabn = DEFTAB;
			   break;
		      }
		      if (!isdigit(tabchar)) {
			   Itabc[0]=(unsigned char)tabchar;
			   if (bstatus == FIRSTOF2)
				Itabc[1]=(unsigned char)*(optarg++);
			   optarg++;
		      }
		      if ((Itabn=atoix(optarg))<=0)
			   Itabn = DEFTAB;
		      break;
		   case 'l': 
/* 
 * Since the option argument can be potentially missing (there are defaults
 * for the arguments), check for such a case.
 */
		      if ( *optarg=='-' || !potential_num(optarg) ) {
			   optind--;
			   Length = 0;
			   break;
		      }
		      Length = atoix(optarg); 
		      break;
		   case 'a':
		   case 'm': Multi = c; break;
		   case 'o': 
/* 
 * Since the option argument can be potentially missing (there are defaults
 * for the arguments), check for such a case.
 */
		      if ( *optarg=='-' || !potential_num(optarg) ) {
			   optind--;
			   Offset = 0;
			   break;
		      }
		      Offset = atoix(optarg); 
		      break;
		   case 'p': ++Pause; break;
		   case 'r': Report = 0; break;
		   case 's':
#ifdef NLS16
		      bstatus=0;
		      bstatus=BYTE_STATUS((unsigned char)*optarg, bstatus);
		      if ( *optarg=='-' || (bstatus==FIRSTOF2 && strlen(optarg)>2) || (bstatus!=FIRSTOF2 && strlen(optarg)>1) ) {
			   optind--;
			   Sepc[0] = '\t';
			   break;
		      }
		      if ((Sepc[0] = (unsigned char)*optarg) != '\0') {
		           bstatus=0;
		           bstatus=BYTE_STATUS((unsigned char)Sepc, bstatus);
			   if (bstatus == FIRSTOF2) {
			      Sepc[1] = (unsigned char)*(argv[optind-1]+1);
			   }
		      }
		      else Sepc [0] = '\t';
#else  NLS16
		      if ((Sepc = *optarg) == '\0') 
			   Sepc = '\t';
#endif NLS16
		      break;
		   case 't': Margin = 0; break;
		   case 'w': 
/* 
 * Since the option argument can be potentially missing (there are defaults
 * for the arguments), check for such a case.
 */
		      if ( potential_num(optarg) )
		      	   Linew = atoix(optarg); 
		      else
			   optind--;
		      break;
		   case 'n':
		   case 'x': /* retained for historical reasons */
		      ++Lnumb;
		      tabchar=*optarg;
		      bstatus=0;
		      bstatus=BYTE_STATUS(tabchar, bstatus);
/* 
 * Since the option argument can be potentially missing (there are defaults
 * for the arguments), check for such a case.
 */
		      if ( tabchar=='-' || curr_opt<optind-1) {
			 optind--;
			 Numw = NUMW;
			 break;
		      }
		      if (!isdigit(tabchar)) {
			   Nsepc[0]=tabchar;
			   if (bstatus == FIRSTOF2) 
				Nsepc[1]=*++optarg;
			   optarg++;
		      }
		      if ( (Numw=atoix(optarg)) <= 0 )
			   Numw =  NUMW;
		   case 'b': /* retained for historical reasons */
		   case 'q': /* retained for historical reasons */
		   case 'j': /* ignore GCOS jprint option */
			break;
/* 
   The following was added to handle the corner case of missing option 
   argument, when the option is the last element of argv. Without this
   check, we'd get a bogus "bad option" error (for example, if you use
   "pr -s").  A simple workaround is to add "--" at the end, but since 
   this used to work fine prior to 9.0, we have to add this check for
   no change in "functionality."   Related SR DSDe407277
   AL 7/15/92
*/
		   case ':': 
		      switch (optopt) {
			   case 'e':
				Etabn = DEFTAB;
				break;
			   case 'i':
				Itabn = DEFTAB;
				break;
			   case 'n':
				Numw = NUMW;
				break;
			   case 'w':
				break;
			   case 'o':
				break;
			   case 'l':
				Length = 0;
				break;
			   case 's':
				Sepc[0] = '\t';
			   	break;
		      }
		      break;
		   case '?':  /* check for -number */
		      if ( isdigit(argv[curr_opt][1]) )  {
			   Ncols = atoix(argv[curr_opt]+1);
			   /* curr_opt = optind; */
		      }
		      else 
			   die(catgets(nlmsg_fd,NL_SETN,6,"bad option"));
	      } /* switch (c) */
	      curr_opt=optind;
	   }  /*while (getopt) */

	   if (optind > 0 && strcmp(argv[optind-1], "--") == 0)
		c = EOF;
	   else
		if (argv[optind][0] == '+') {   /* select +number */
		   Fpage = atoix(argv[optind]+1);
		   if ( Fpage<1 ) Fpage=1;
		   c=0;        /* Ensure c != EOF and */
		   ++optind;   /* fake getopt to ignore +number option */
		}

	} while ( c!=EOF && optind<argc);

	eargc=argc-optind;
	if (argv[optind][0]=='-')  /* if file name is -, treat it as stdin */
		argv[optind][0]='\0';

	if (Length == 0) Length = LENGTH;
	if (Length <= Margin) Margin = 0;
	Plength = Length - Margin/2;
	if (Multi == 'm') Ncols = eargc;
	switch (Ncols) {
	case 0:
		Ncols = 1;
	case 1:
		break;
	default:
		if (Etabn == 0) /* respect explicit tab specification */
			Etabn = DEFTAB;
		if (Itabn == 0)
			Itabn = DEFTAB;
	}
#ifdef NLS16
	if (Linew == 0) Linew = Ncols != 1 && Sepc [0] == 0 ? LINEW : 512;
#else  NLS16
	if (Linew == 0) Linew = Ncols != 1 && Sepc == 0 ? LINEW : 512;
#endif NLS16
	if (Lnumb) {
		int numw;

#ifdef NLS16
		if (Nsepc [0] == '\t') {
#else  NLS16
		if (Nsepc == '\t') {
#endif NLS16
			if(Itabn == 0)
				numw = Numw + DEFTAB - (Numw % DEFTAB);
			else
				numw = Numw + Itabn - (Numw % Itabn);
		}else {
#ifdef NLS16
			/* is this a printable kanji ? */
			if (CHARAT (Nsepc) > 255)
				numw = Numw + 2;
			else
				numw = Numw + ((isprint(Nsepc[0])) ? 1 : 0);
#else  NLS16
			numw = Numw + ((isprint(Nsepc)) ? 1 : 0);
#endif NLS16
		}
		Linew -= (Multi == 'm') ? numw : numw * Ncols;
	}
	if ((Colw = (Linew - Ncols + 1)/Ncols) < 1)
		die((catgets(nlmsg_fd,NL_SETN,7,"width too small")));
	if (Ncols != 1 && Multi == 0) {
		UNS buflen = ((UNS)(Plength/Dblspace + 1))*(Linew+1)*sizeof(CHAR);
		Buffer = (CHAR *)getspace(buflen);
		Bufend = &Buffer[buflen];
		Colpts = (COLP)getspace((UNS)((Ncols+1)*sizeof(*Colpts)));
	}
	if (Ttyout && (Pause || Formfeed) && !istty(stdin))
		Ttyin = fopen(TTY);
	return (eargc);
}

intopt(arg, optp) char *arg; int optp[2];
{
	int c;

	if ((c = (*arg & 0377)) != '\0' && !isdigit(c)) { 
		optp[0] = c; 
		if (FIRSTof2 (optp[0])) 
			optp[1] = *(arg+1);
	}
	return ((c = atoix(arg)) != 0 ? c : -1);
}

int Page, C = '\0', Nspace, Inpos;
#ifdef	NLS16
	int Bstatus = ONEBYTE,
	    Sstatus = ONEBYTE;
#endif	NLS16

print(name) char *name;
{
	static int notfirst = 0;
	char *date = NULL, *head = NULL;
	int c;

	if (Multi != 'm' && mustopen(name, &Files[0]) == NULL) return (0);
	if (Buffer) ungetc(Files->f_nextc, Files->f_f);
	if (Lnumb) Lnumb = 1;
	for (Page = 0; ; putpage()) {
		if (C == EOF) break;
		if (Buffer) nexbuf();
		Inpos = 0;
		if (get(0) == EOF) break;
		fflush(stdout);
		if (++Page >= Fpage) {
			if (Ttyout && (Pause || Formfeed && !notfirst++)) {
				PROMPT(); /* prompt with bell and pause */
				while ((c = getc(Ttyin)) != EOF && c != '\n') ;
			}
			if (Margin == 0) continue;
			CADDID();
			if (date == NULL) date = GETDATE();
			if (head == NULL) head = Head != NULL ? Head :
				Nfiles < 2 ? Files->f_name : nulls;
			printf("\n\n");
			Nspace = Offset;
			putspace();
			printf(HEAD);
		}
	}
	C = '\0';
	return (1);
}

int Outpos, Lcolpos, Pcolpos, Line;

putpage()
{
	register int colno;

	for (Line = Margin/2; ; get(0)) {
		for (Nspace = Offset, colno = 0, Outpos = 0; ; ) {
			/* If multiple column format - treat \f like any *
			 * other character. Break out only if single     *
			 * column format 			         */
			if ( Ncols < 2 && C == '\f' )
				break;
			if (Lnumb && C != EOF && ((colno == 0 && Multi == 'm') || Multi != 'm')) {
				if (Page >= Fpage) {
					putspace();
#ifdef NLS16
					printf("%*ld%c", Numw, Buffer ? Colpts[colno].c_lno++ : Lnumb, Nsepc[0]);
					if (FIRSTof2(Nsepc[0]))
						printf("%c", Nsepc [1]);
#else  NLS16
					printf("%*ld%c", Numw, Buffer ?
						Colpts[colno].c_lno++ : Lnumb, Nsepc);
#endif NLS16
				}
				++Lnumb;
			}
#ifdef	NLS16
			/* save the previous byte status 
				before getting the next character */
			for (Lcolpos = 0, Pcolpos = 0;
				C != '\n' && C != EOF;
				Bstatus = BYTE_STATUS((unsigned char)C, Bstatus), get(colno)) {
#else	NLS16
			for (Lcolpos = 0, Pcolpos = 0;
				C != '\n' && C != EOF; get(colno)) {
#endif	NLS16
			/* If multiple column format - treat \f like any *
			 * other character. Break out only if single     *
			 * column format 			         */
				if ( Ncols < 2 && C == '\f' )
					break;
				put(C);
			}
#ifdef	NLS16
			Bstatus = BYTE_STATUS((unsigned char)C, Bstatus);
#endif	NLS16
			if (C == EOF || ++colno == Ncols ||
				C == '\n' && get(colno) == EOF) break;
#ifdef NLS16
			if (Sepc[0] != 0) {
				put (Sepc[0]);
				if (FIRSTof2 (Sepc[0]))
					put (Sepc[1]);
			}
#else  NLS16
			if (Sepc) put(Sepc);
#endif NLS16
			else if ((Nspace += Colw - Lcolpos + 1) < 1) Nspace = 1;
		}
		if (C == EOF) {
			if (Margin != 0) break;
			if (colno != 0) put('\n');
			return;
		}
		/* If multiple column format - treat \f like any *
		 * other character. Break out only if single     *
		 * column format 			         */
		if (C == '\f' && Ncols < 2)
			break;
		put('\n');
		if (Dblspace == 2 && Line < Plength) put('\n');

		/* FIX : if an ODD number of lines, Double spacing, and
		 * Multiple Columns is specified, then PR (1) prints 
		 * one screen and then quits.  That combination causes
		 * a problem, because Double spacing means an EVEN number
		 * of lines and thus PR gets confused and thinks EOF after
		 * the first screen.  Our solution is when screen length 
		 * is odd to pad the bottom of the screen with an
		 * additional blank line. This increments 'Line' and thus
		 * allows it to BREAK out of the FOR loop, instead of calling
		 * GET () where EOF would be detected.  Be aware that the
		 * value of Line and Plength are affected by Margin (which
		 * is affected by Length).
		 */
		if ((Dblspace == 2) && (Line == Plength - 1))
			put ('\n');

		if (Line >= Plength) break;
	}
	if (Formfeed) put('\f');
	else while (Line < Length) put('\n');
}

nexbuf()
{
	register CHAR *s = Buffer;
	register COLP p = Colpts;
	int j, c, bline = 0;
	int fixLength; 			/* temp value for length */

	for ( ; ; ) {
		p->c_ptr0 = p->c_ptr = s;
		if (p == &Colpts[Ncols]) return;
		(p++)->c_lno = Lnumb + bline;

		/* FIX : if (Length == 11, Dblspace, and Ncols > 1)
		 * then this loop won't be entered cause 
		 * 'j' will be 0.  Nexbuf () will loop in the above three lines
		 * until it exits by the RETURN.  This signals EOF.  
		 * Now if Length < 10 (The default Margin value) 
		 * there won't be a problem, because Margin is reset to 0.
		 * For Length > 11 this isn't a problem.
		 */
		fixLength = (Dblspace && Length != 11) ? Length : 12;

		for (j = (fixLength - Margin)/Dblspace; --j >= 0; ++bline)
			for (Inpos = 0; ; ) {
				if ((c = getc(Files->f_f)) == EOF) {
					for (*s = EOF; p <= &Colpts[Ncols]; ++p)
						p->c_ptr0 = p->c_ptr = s;
					balance(bline);
					return;
				}
#ifdef	NLS16
				/* increase if isprint or the first byte of a 2-byte char */
				if (isprint(c) || FIRSTof2(c)) ++Inpos;
#else	NLS16
				if (isprint(c)) ++Inpos;
#endif	NLS16
				if (Inpos <= Colw || c == '\n') {
					*s = c;
					if (++s >= Bufend)
						die((catgets(nlmsg_fd,NL_SETN,8,"page-buffer overflow")));
				}
				if (c == '\n') break;
				switch (c) {
				case '\b': if (Inpos == 0) --s;
				case ESC:  if (Inpos > 0) --Inpos;
				}
			}
	}
}

balance(bline) /* line balancing for last page */
{
	register CHAR *s = Buffer;
	register COLP p = Colpts;
	int colno = 0, j, c, l;

	c = bline % Ncols;
	l = (bline + Ncols - 1)/Ncols;
	bline = 0;
	do {
		for (j = 0; j < l; ++j)
			while (*s++ != '\n') ;
		(++p)->c_lno = Lnumb + (bline += l);
		p->c_ptr0 = p->c_ptr = s;
		if (++colno == c) --l;
	} while (colno < Ncols - 1);
}

get(colno)
{
	static int peekc = 0;
	register COLP p;
	register FILS *q;
	register int c;
#ifdef NLS16
	int	tabs[2];
	int	in_tab;

	in_tab = 0;
#endif NLS16

	if (peekc)
#ifdef NLS16
		{ peekc = 0; 
		tabs[0] = Etabc [0]; 
		tabs[1] = Etabc [1]; 
		in_tab = 1;}
#else  NLS16
		{ peekc = 0; c = Etabc; }
#endif NLS16
	else if (Buffer) {
		p = &Colpts[colno];
		if (p->c_ptr >= (p+1)->c_ptr0) c = EOF;
		else if ((c = *p->c_ptr) != EOF) ++p->c_ptr;
	} else if ((c = 
		(q = &Files[Multi == 'a' ? 0 : colno])->f_nextc) == EOF) {
		for (q = &Files[Nfiles]; --q >= Files && q->f_nextc == EOF; ) ;
		if (q >= Files) c = '\n';
	} else
		q->f_nextc = getc(q->f_f);
#ifdef NLS16
	if (!in_tab) {
		tabs [0] = c;
		if (FIRSTof2 (tabs[0])) {
			(q = &Files[Multi == 'a' ? 0 : colno]);
			tabs [1] = q->f_nextc; 

		} else
			tabs [1] = 0;
		
		/* if the next input char is part of the tab
		 * skip it
		 */
		if ((EQTAB(tabs)) && FIRSTof2(tabs[0]))
			q->f_nextc = getc(q->f_f);
	}
/* disallow the 2nd byte of a 2-byte char to be the input tab char. */
	if (Etabn != 0 && EQTAB(tabs) && Bstatus != FIRSTOF2) {
#else	NLS16
	if (Etabn != 0 && c == Etabc) {
#endif	NLS16
		++Inpos;
		peekc = ETABS;
		c = ' ';
#ifdef	NLS16
	/* increase if isprint or the first byte of a 2-byte char */
	} else if (isprint(c) || FIRSTof2(c))
#else	NLS16
	} else if (isprint(c))
#endif	NLS16
		++Inpos;
	else
		switch (c) {
		case '\b':
		case ESC:
			if (Inpos > 0) --Inpos;
			break;
		case '\f':
			break;
		case '\n':
		case '\r':
			Inpos = 0;
		}
	return (C = c);
}

put(c)
{
	int move;

	switch (c) {
	case ' ':
		if(Ncols < 2 || Lcolpos < Colw) {
			++Nspace;
			++Lcolpos;
		}
		return;
	case '\t':
		if(Itabn == 0)
			break;
		if(Lcolpos < Colw) {
			move = Itabn - ((Lcolpos + Itabn) % Itabn);
			move = (move < Colw-Lcolpos) ? move : Colw-Lcolpos;
			Nspace += move;
			Lcolpos += move;
		}
		return;
	case '\b':
		if (Lcolpos == 0) return;
		if (Nspace > 0) { --Nspace; --Lcolpos; return; }
		if (Lcolpos > Pcolpos) { --Lcolpos; return; }
	case ESC:
		move = -1;
		break;
	case '\n':
		++Line;
	case '\r':
		Pcolpos = 0;
		Lcolpos = 0;
		Nspace = 0;
		Outpos = 0;
	case '\f':
		/* Treat \f special only in single column format */
		if ( Ncols < 2 ) {
			Pcolpos = 0;
			Lcolpos = 0;
			Nspace = 0;
			Outpos = 0;
		}
	default:
#ifdef	NLS16
		/* move if isprint or the first byte of a 2-byte char */
		move = ((isprint(c) || FIRSTof2(c)) != 0);
#else	NLS16
		move = (isprint(c) != 0);
#endif	NLS16
	}
	if (Page < Fpage) return;
	if (Lcolpos > 0 || move > 0) Lcolpos += move;
	putspace();

#ifdef	NLS16
	if (Ncols < 2 || Lcolpos <= Colw) {
	/* 16-bit: check character on the column boundary.  If it is the */
	/*   	   first byte of a 16-bit char, put a space char instead */
		if (Lcolpos == Colw && BYTE_STATUS((unsigned char)c, Sstatus) == FIRSTOF2) {
			putchar(' ');
			Sstatus = ONEBYTE;
		}
		else {
			putchar(c);
			Sstatus = BYTE_STATUS((unsigned char)c, Sstatus);
		}	
		Outpos += move;
		Pcolpos = Lcolpos;
	}

#else	NLS16

	if (Ncols < 2 || Lcolpos <= Colw) {
		putchar(c);
		Outpos += move;
		Pcolpos = Lcolpos;
	}

#endif	NLS16
}

putspace()
{
	int nc;

	for ( ; Nspace > 0; Outpos += nc, Nspace -= nc)
		if (ITABS)
#ifdef NLS16
			{
			putchar(Itabc[0]);
			if (FIRSTof2 (Itabc[0]))
				putchar(Itabc[1]);
			}
#else  NLS16
			putchar(Itabc);
#endif NLS16
		else {
			nc = 1;
			putchar(' ');
		}
}

atoix(p) register char *p;
{
	register int n = 0, c;

	while (isdigit(c = *p++)) n = 10*n + c - '0';
	return (n);
}

/* Defer message about failure to open file to prevent messing up
   alignment of page with tear perforations or form markers.
   Treat empty file as special case and report as diagnostic.
*/

#if defined NLS || defined NLS16
#define EMPTY	100	/* possible length of " -- empty file" message */
#else
#define EMPTY	14	/* length of " -- empty file" */
#endif

typedef struct err { struct err *e_nextp; char *e_mess; } ERR;
ERR *Err = NULL, *Lasterr = (ERR *)&Err;

FILE *mustopen(s, f) char *s; register FILS *f;
{
	if (*s == '\0') {
		f->f_name = STDINNAME();
		f->f_f = stdin;
		if (Multi == 'm') {
			/* if several "-" with multi-column option,
				return data only for the first */
			static int first = 1;
			if (!first) {
				f->f_nextc = EOF;
				return f->f_f;
			}
			first = 0;
		}
	} else if ((f->f_f = fopen(f->f_name = s, "r")) == NULL) {
		s = ffiler(f->f_name);
		s = strcpy((char *)getspace((UNS)(strlen(s) + 1)), s);
	}
	if (f->f_f != NULL) {
		if ((f->f_nextc = getc(f->f_f)) != EOF || Multi == 'm')
			return (f->f_f);
		sprintf(s = (char *)getspace((UNS)(strlen(f->f_name) + 1 + EMPTY)), (catgets(nlmsg_fd,NL_SETN,9,"%s -- empty file")), f->f_name);
		fclose(f->f_f);
	}
	Error = 1;
	if (Report)
		if (Ttyout) { /* accumulate error reports */
			Lasterr = Lasterr->e_nextp = (ERR *)getspace((UNS)sizeof(ERR));
			Lasterr->e_nextp = NULL;
			Lasterr->e_mess = s;
		} else { /* ok to print error report now */
			cerror(s);
			putc('\n', stderr);
		}
	return ((FILE *)NULL);
}

ANY *getspace(n) UNS n;
{
	ANY *t;

	if ((t = (ANY *)malloc(n)) == NULL) die((catgets(nlmsg_fd,NL_SETN,10,"out of space")));
	return (t);
}

die(s) char *s;
{
	++Error;
	errprint();
	cerror(s);
	putc('\n', stderr);
	exit(1);
}

onintr()
{
	++Error;
	errprint();
	INTREXIT(1);
}

errprint() /* print accumulated error reports */
{
	fflush(stdout);
	for ( ; Err != NULL; Err = Err->e_nextp) {
		cerror(Err->e_mess);
		putc('\n', stderr);
	}
	done();
}
