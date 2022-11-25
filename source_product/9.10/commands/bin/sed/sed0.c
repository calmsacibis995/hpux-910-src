/* @(#) $Revision: 70.3 $ */     


#include <stdio.h>
#include <locale.h>
#ifdef		NLS16
#include <nl_ctype.h>
#endif
#include "sed.h"

FILE	*fin;
FILE    *fcode[12];
char    *lastre;
char    sseof;
char    *reend;
char    *hend;
union reptr     *ptrend;
int     eflag;
int	fflag;
extern  nbra;
char    linebuf[LBSIZE+1];
int     gflag;
int     nlno;
char    fname[12][40];
int     nfiles;
union reptr ptrspace[PTRSIZE];
union reptr *rep;
char    *cp;
char    respace[RESIZE];
struct label ltab[LABSIZE];
struct label    *lab;
struct label    *labend;
int     depth;
int     eargc;
char    **eargv;
union reptr     **cmpend[PTRSIZE];
char    *badp;
char    bad;

#define CCEOF	22

#ifdef NLS16
#define MRK	0177	/* mark substitution points and */
			/* end of translation buffer for */
			/* 16-bit algorithm */
#endif NLS16

struct label    *labtab = ltab;

char    CGMES[]		= "sed: command garbled: %s\n";
char    TMMES[]		= "Too much text: %s\n";
char    LTL[]  		= "Label too long: %s\n";
char    AD0MES[]	= "No addresses allowed: %s\n";
char    AD1MES[]	= "Only one address allowed: %s\n";
		/* Changed from 512 to LINE_MAX (2048) for POSIX.2 */
char	TOOBIG[]	= "Suffix too large - 2048 max: %s\n";

extern	sed;	  /* IMPORTANT flag !!! */
extern char *comple();

extern char *optarg;		/* for getopt */
extern int optind, opterr;
char letter;			/* current keyletter */

char	*optparm;		/* option parameter for fcomp */

main(argc, argv)
int     argc;
char    *argv[];
{ /* main */
	/*opterr = 0;		/* turns off getopt error reporting */

	sed = 1;
	eargc = argc;
	eargv = argv;

	badp = &bad;
	aptr = abuf;
	lab = labtab + 1;       /* 0 reserved for end-pointer */
	rep = ptrspace;
	rep->r1.ad1 = respace;
	lbend = &linebuf[LBSIZE];
	hend = &holdsp[LBSIZE];
	lcomend = &genbuf[71];
	ptrend = &ptrspace[PTRSIZE];
	reend = &respace[RESIZE];
	labend = &labtab[LABSIZE];
	lnum = 0;
	pending = 0;
	depth = 0;
	spend = linebuf;
	hspend = holdsp;	/* Avoid "bus error" under "H" cmd. */
	fcode[0] = stdout;
	nfiles = 1;

#ifdef NLS
	/*				not needed until msg cat support added
	nl_catd		catd;
	*/

	if (!setlocale(LC_ALL,"")) {
		fprintf(stderr, _errlocale("sed"));
		putenv("LANG=");
		/*			not needed until msg cat support added
		catd = (nl_catd)-1;
		*/
	}
		/*			not needed until msg cat support added
	else
		catd = catopen("sed", 0);
		*/
#endif NLS

	if(argc == 1)
		exit(0);

	while ((letter = getopt(argc, argv, "e:f:gn")) != EOF)
		switch(letter) {

		case 'n':
			nflag++;
			/* exit if there is nothing following the flag */
			if (optind == eargc) /* nothing to do? */
				exit(0);
			else
				continue;

		case 'f':
			if(optarg[0] == '\0')	exit(2);

#ifndef hpe
			if((fin = fopen(optarg, "r")) == NULL) {
#else
			/* Open with trim of trailing blanks */
			if((fin = fopen(optarg, "r T")) == NULL) { /* } */
#endif hpe
				fprintf(stderr, "Cannot open pattern-file: %s\n", optarg);
				exit(2);
			}

			optparm = '\0';
			fcomp();
			fclose(fin);
			fflag++;
			continue;

		case 'e':
			eflag++;
			optparm = optarg;
			fcomp();
			eflag = 0;
			continue;

		case 'g':
			gflag++;
			continue;

		default:
		case '?':
			/*
			** exit if nothing following the bad flag,
			** otherwise try to go on and do something useful
			*/
			if (optind == argc)
				exit(2);
			else
				continue;
		}


	if(rep == ptrspace && fflag == 0) {
		eflag++;
		optparm = argv[optind++];
		fcomp();
		eflag = 0;
	}

	if(depth) {
		fprintf(stderr, "Too many {'s\n"); /* } */
		exit(2);
	}

	labtab->address = rep;

	dechain();

	if(optind >= argc)	/* if(eargc <= 0) */
		execute((char *)NULL, hend);
	else while(optind < argc) {	/* continue thru the filenames */
		execute(argv[optind++], hend);
	}
	fclose(stdout);
	exit(0);
} /* main */

fcomp()
{

	register char   *p, *op, *tp;
	char    *address();
	union reptr     *pt, *pt1;
	int     i, ii;
	struct label    *lpt;

	op = lastre;

	if(rline(linebuf) < 0)  return;
	if(*linebuf == '#') {
		if(linebuf[1] == 'n')
			nflag = 1;
	}
	else {
		cp = linebuf;
		goto comploop;
	}

	for(;;) {
		if(rline(linebuf) < 0)  break;

		cp = linebuf;

comploop:
	/* fprintf(stdout, "cp: %s\n", cp);	/*DEBUG*/
		while(*cp == ' ' || *cp == '\t')	cp++;
		if(*cp == '\0' || *cp == '#')	 continue;
		if(*cp == ';') {
			cp++;
			goto comploop;
		}

		p = address(rep->r1.ad1);
		if(p == badp) {
			fprintf(stderr, CGMES, linebuf);
			exit(2);
		}

		if(p == rep->r1.ad1) {
			if(op)
				rep->r1.ad1 = op;
			else {
				fprintf(stderr, "First RE may not be null\n");
				exit(2);
			}
		} else if(p == 0) {
			p = rep->r1.ad1;
			rep->r1.ad1 = 0;
		} else {
			op = rep->r1.ad1;
			if(*cp == ',' || *cp == ';') {
				cp++;
				if((rep->r1.ad2 = p) > reend) {
					fprintf(stderr, TMMES, linebuf);
					exit(2);
				}
				p = address(rep->r1.ad2);
				if(p == badp || p == 0) {
					fprintf(stderr, CGMES, linebuf);
					exit(2);
				}
				if(p == rep->r1.ad2)
					rep->r1.ad2 = op;
				else
					op = rep->r1.ad2;

			} else
				rep->r1.ad2 = 0;
		}

		if(p > reend) {
			fprintf(stderr, "Too much text: %s\n", linebuf);
			exit(2);
		}

		while(*cp == ' ' || *cp == '\t')	cp++;

swit:
		switch(*cp++) {

			default:
				fprintf(stderr, "Unrecognized command: %s\n", linebuf);
				exit(2);

			case '!':
				rep->r1.negfl = 1;
				goto swit;

			case '{':	/* } */
				rep->r1.command = BCOM;
				rep->r1.negfl = !(rep->r1.negfl);
				cmpend[depth++] = &rep->r2.lb1;
				if(++rep >= ptrend) {
					fprintf(stderr, "Too many commands: %s\n", linebuf);
					exit(2);
				}
				rep->r1.ad1 = p;
				if(*cp == '\0') continue;

				goto comploop;

						/* { */
			case '}':
				if(rep->r1.ad1) {
					fprintf(stderr, AD0MES, linebuf);
					exit(2);
				}

				if(--depth < 0) {	/* { */
					fprintf(stderr, "Too many }'s\n");
					exit(2);
				}
				*cmpend[depth] = rep;

				rep->r1.ad1 = p;
				continue;

			case '=':
				rep->r1.command = EQCOM;
				if(rep->r1.ad2) {
					fprintf(stderr, AD1MES, linebuf);
					exit(2);
				}
				break;

			case ':':
				if(rep->r1.ad1) {
					fprintf(stderr, AD0MES, linebuf);
					exit(2);
				}

				while(*cp++ == ' ');
				cp--;


				tp = lab->asc;
				while((*tp++ = *cp++))
					if(tp > &(lab->asc[8])) {
						fprintf(stderr, LTL, linebuf);
						exit(2);
					}
				*--tp = '\0';

				if(lpt = search(lab)) {
					if(lpt->address) {
						fprintf(stderr, "Duplicate labels: %s\n", linebuf);
						exit(2);
					}
				} else {
					lab->chain = 0;
					lpt = lab;
					if(++lab >= labend) {
						fprintf(stderr, "Too many labels: %s\n", linebuf);
						exit(2);
					}
				}
				lpt->address = rep;
				rep->r1.ad1 = p;

				continue;

			case 'a':
				rep->r1.command = ACOM;
				if(rep->r1.ad2) {
					fprintf(stderr, AD1MES, linebuf);
					exit(2);
				}
				if(*cp == '\\') cp++;
				if(*cp++ != '\n') {
					fprintf(stderr, CGMES, linebuf);
					exit(2);
				}
				rep->r1.re1 = p;
				p = text(rep->r1.re1);
				break;
			case 'c':
				rep->r1.command = CCOM;
				if(*cp == '\\') cp++;
				if(*cp++ != ('\n')) {
					fprintf(stderr, CGMES, linebuf);
					exit(2);
				}
				rep->r1.re1 = p;
				p = text(rep->r1.re1);
				break;
			case 'i':
				rep->r1.command = ICOM;
				if(rep->r1.ad2) {
					fprintf(stderr, AD1MES, linebuf);
					exit(2);
				}
				if(*cp == '\\') cp++;
				if(*cp++ != ('\n')) {
					fprintf(stderr, CGMES, linebuf);
					exit(2);
				}
				rep->r1.re1 = p;
				p = text(rep->r1.re1);
				break;

			case 'g':
				rep->r1.command = GCOM;
				break;

			case 'G':
				rep->r1.command = CGCOM;
				break;

			case 'h':
				rep->r1.command = HCOM;
				break;

			case 'H':
				rep->r1.command = CHCOM;
				break;

			case 't':
				rep->r1.command = TCOM;
				goto jtcommon;

			case 'b':
				rep->r1.command = BCOM;
jtcommon:
				while(*cp++ == ' ');
				cp--;

				if(*cp == '\0') {
					if(pt = labtab->chain) {
						while(pt1 = pt->r2.lb1)
							pt = pt1;
						pt->r2.lb1 = rep;
					} else
						labtab->chain = rep;
					break;
				}
				tp = lab->asc;
				while((*tp++ = *cp++))
					if(tp > &(lab->asc[8])) {
						fprintf(stderr, LTL, linebuf);
						exit(2);
					}
				cp--;
				*--tp = '\0';

				if(lpt = search(lab)) {
					if(lpt->address) {
						rep->r2.lb1 = lpt->address;
					} else {
						pt = lpt->chain;
						while(pt1 = pt->r2.lb1)
							pt = pt1;
						pt->r2.lb1 = rep;
					}
				} else {
					lab->chain = rep;
					lab->address = 0;
					if(++lab >= labend) {
						fprintf(stderr, "Too many labels: %s\n", linebuf);
						exit(2);
					}
				}
				break;

			case 'n':
				rep->r1.command = NCOM;
				break;

			case 'N':
				rep->r1.command = CNCOM;
				break;

			case 'p':
				rep->r1.command = PCOM;
				break;

			case 'P':
				rep->r1.command = CPCOM;
				break;

			case 'r':
				rep->r1.command = RCOM;
				if(rep->r1.ad2) {
					fprintf(stderr, AD1MES, linebuf);
					exit(2);
				}
				if(*cp++ != ' ') {
					fprintf(stderr, CGMES, linebuf);
					exit(2);
				}
				/* skip blank spaces before filename */
				while (*cp == ' ') cp++;
				rep->r1.re1 = p;
				p = text(rep->r1.re1);
				break;

			case 'd':
				rep->r1.command = DCOM;
				break;

			case 'D':
				rep->r1.command = CDCOM;
				rep->r2.lb1 = ptrspace;
				break;

			case 'q':
				rep->r1.command = QCOM;
				if(rep->r1.ad2) {
					fprintf(stderr, AD1MES, linebuf);
					exit(2);
				}
				break;

			case 'l':
				rep->r1.command = LCOM;
				break;

			case 's':
				rep->r1.command = SCOM;
				sseof = *cp++;
				rep->r1.re1 = p;
				p = comple((char *) 0, rep->r1.re1, reend, sseof);
				if(p == badp) {
					fprintf(stderr, CGMES, linebuf);
					exit(2);
				}
				if(p == rep->r1.re1) {
					rep->r1.re1 = op;
				} else {
					op = rep->r1.re1;
				}

				if((rep->r1.rhs = p) > reend) {
					fprintf(stderr, TMMES, linebuf);
					exit(2);
				}

				if((p = compsub(rep->r1.rhs)) == badp) {
					fprintf(stderr, CGMES, linebuf);
					exit(2);
				}

				if(*cp == 'g') {
					cp++;
		/* POSIX.2/D11.2 wants the limit to be LINE_MAX */
		/*			rep->r1.gfl = 999;	*/
					rep->r1.gfl = GOCCUR;
				} else if(gflag)
		/*			rep->r1.gfl = 999;	*/
					rep->r1.gfl = GOCCUR;

				if(*cp >= '1' && *cp <= '9')
					{i = *cp - '0';
					cp++;
					while(1)
						{ii = *cp;
						if(ii < '0' || ii > '9') break;
						i = i*10 + ii - '0';
						if(i > LINE_MAX) 
		/* Changed from 512 to LINE_MAX for POSIX.2 */
							{fprintf(stderr, TOOBIG, linebuf);
							exit(2);
							}
						cp++;
						}
					rep->r1.gfl = i;
					}

				if(*cp == 'p') {
					cp++;
					rep->r1.pfl = 1;
				}

				if(*cp == 'P') {
					cp++;
					rep->r1.pfl = 2;
				}

				if(*cp == 'w') {
					cp++;
					if(*cp++ !=  ' ') {
						fprintf(stderr, CGMES, linebuf);
						exit(2);
					}
					if(nfiles >= 10) {
						fprintf(stderr, "Too many files in w commands\n");
						exit(2);
					}

					text(fname[nfiles]);
					for(i = nfiles - 1; i >= 0; i--)
						if(cmp(fname[nfiles],fname[i]) == 0) {
							rep->r1.fcode = fcode[i];
							goto done;
						}
					if((rep->r1.fcode = fopen(fname[nfiles], "w")) == NULL) {
						fprintf(stderr, "cannot open %s\n", fname[nfiles]);
						exit(2);
					}
					fcode[nfiles++] = rep->r1.fcode;
				}
				break;

			case 'w':
				rep->r1.command = WCOM;
				if(*cp++ != ' ') {
					fprintf(stderr, CGMES, linebuf);
					exit(2);
				}
				/* skip blank spaces before fiiename */
				while (*cp == ' ') cp++;
				text(fname[nfiles]);
				for(i = nfiles - 1; i >= 0; i--)
					if(cmp(fname[nfiles], fname[i]) == 0) {
						rep->r1.fcode = fcode[i];
						goto done;
					}

				if(nfiles > 10){
					fprintf(stderr, "Too many files in w commands\n");
					exit(2);
				}

				if((rep->r1.fcode = fopen(fname[nfiles], "w")) == NULL) {
					fprintf(stderr, "Cannot create %s\n", fname[nfiles]);
					exit(2);
				}
				fcode[nfiles++] = rep->r1.fcode;
				break;

			case 'x':
				rep->r1.command = XCOM;
				break;

			case 'y':
				rep->r1.command = YCOM;
				sseof = *cp++;
				rep->r1.re1 = p;
				p = ycomp(rep->r1.re1);
				if(p == badp) {
					fprintf(stderr, CGMES, linebuf);
					exit(2);
				}
				if(p > reend) {
					fprintf(stderr, TMMES, linebuf);
					exit(2);
				}
				break;

		}
done:
		if(++rep >= ptrend) {
			fprintf(stderr, "Too many commands, last: %s\n", linebuf);
			exit(2);
		}

		rep->r1.ad1 = p;

		/* strip off trailing white space from the last command: */
		while(*cp == ' ' || *cp == '\t')	cp++;

		if(*cp++ != '\0') {
			if(cp[-1] == ';')
				goto comploop;
			fprintf(stderr, CGMES, linebuf);
			exit(2);
		}

	}
	rep->r1.command = 0;
	lastre = op;
} /* fcomp */

#ifdef NLS16
char    *compsub(rhsbuf)
char    *rhsbuf;
{
	register char   *p, *q;
	int c;		/* may be 2 bytes */

	p = rhsbuf;
	q = cp;
	for(;;) {
		if((c = CHARADV(q)) == '\\') {
			c = CHARADV(q);
			if(c > nbra + '0' && c <= '9')
				return(badp);
			*p++ = MRK;
			PCHARADV(c, p);
			continue;
		}
		if(c == sseof) {
			*p++ = '\0';
			cp = q;
			return(p);
		}
		if(c == '\0') {
			return(badp);
		}
		PCHARADV(c, p);
	}
} /* compsub */

#else 		/* 8-bit algorithm */
char    *compsub(rhsbuf)
char    *rhsbuf;
{
	register char   *p, *q;

	p = rhsbuf;
	q = cp;
	for(;;) {
		if((*p = *q++) == '\\') {
			*p = *q++;
			if(*p > nbra + '0' && *p <= '9')
				return(badp);
			if(*p >= '1' && *p < nbra + '1')    /* modified for */
				*p = *p - '0' + 0200;       /* 8 bit handling */
			p++;
			continue;
		}
		if(*p == sseof) {
			*p++ = '\0';
			cp = q;
			return(p);
		}
		if(*p++ == '\0') {
			return(badp);
		}
	}
} /* compsub */
#endif NLS16

rline(lbuf)
char    *lbuf;
{
	register char   *p, *q;
	register	t;
	static char     *saveq;

	p = lbuf - 1;			/* point to char before lbuf */

	if(eflag) {
		if(eflag > 0) {
			eflag = -1;
			if(optparm == '\0')
				exit(2);
			q = optparm;
			while(*++p = *q++) {
				if(*p == '\\') {
					if((*++p = *q++) == '\0') {
						saveq = 0;
						return(-1);
					} else
						continue;
				}
				if(*p == '\n') {
					*p = '\0';
					saveq = q;
					return(1);
				}
			}
			saveq = 0;
			return(1);
		}
		if((q = saveq) == 0)    return(-1);

		while(*++p = *q++) {
			if(*p == '\\') {
				if((*++p = *q++) == '0') {
					saveq = 0;
					return(-1);
				} else
					continue;
			}
			if(*p == '\n') {
				*p = '\0';
				saveq = q;
				return(1);
			}
		}
		saveq = 0;
		return(1);
	}

	while((t = getc(fin)) != EOF) {
		*++p = t;
		if(*p == '\\') {
			t = getc(fin);
			*++p = t;
		}
		else if(*p == '\n') {
			*p = '\0';
			return(1);
		}
	}
	return(-1);
} /* rline */

char    *address(expbuf)
char    *expbuf;
{
	register char   *rcp;
	long    lno;

	if(*cp == '$') {
		cp++;
		*expbuf++ = CEND;
		*expbuf++ = CCEOF;
		return(expbuf);
	}
	if (*cp == '/' || *cp == '\\' ) {
		if ( *cp == '\\' )
			cp++;
		sseof = *cp++;
		return(comple((char *) 0, expbuf, reend, sseof));
	}

	rcp = cp;
	lno = 0;

	while(*rcp >= '0' && *rcp <= '9')
		lno = lno*10 + *rcp++ - '0';

	if(rcp > cp) {
		*expbuf++ = CLNUM;
		*expbuf++ = nlno;
		tlno[nlno++] = lno;
		if(nlno >= NLINES) {
			fprintf(stderr, "Too many line numbers\n");
			exit(2);
		}
		*expbuf++ = CCEOF;
		cp = rcp;
		return(expbuf);
	}
	return(0);
} /* address */

cmp(a, b)
char    *a,*b;
{
	register char   *ra, *rb;

	ra = a - 1;
	rb = b - 1;

	while(*++ra == *++rb)
		if(*ra == '\0') return(0);
	return(1);
} /* cmp */

#ifdef NLS16	/* problem is that '\\' may be 2nd byte of kanji */
char    *text(textbuf)
char    *textbuf;
{
	register char   *p, *q;
	int c;

	p = textbuf;
	q = cp;
	for(;;) {

		if((c = CHARADV(q)) == '\\')
			c = CHARADV(q);
		if(c == '\0') {
			*p = 0;
			cp = --q;
			return(++p);
		}
		PCHARADV(c, p);
	}
} /* text */

#else /* 8-bit algorithm */
char    *text(textbuf)
char    *textbuf;
{
	register char   *p, *q;

	p = textbuf;
	q = cp;
	for(;;) {

		if((*p = *q++) == '\\')
			*p = *q++;
		if(*p == '\0') {
			cp = --q;
			return(++p);
		}
		p++;
	}
} /* text */
#endif NLS16

struct label    *search(ptr)
struct label    *ptr;
{
	struct label    *rp;

	rp = labtab;
	while(rp < ptr) {
		if(cmp(rp->asc, ptr->asc) == 0)
			return(rp);
		rp++;
	}

	return(0);
} /* search */

dechain()
{
	struct label    *lptr;
	union reptr     *rptr, *trptr;

	for(lptr = labtab; lptr < lab; lptr++) {

		if(lptr->address == 0) {
			fprintf(stderr, "Undefined label: %s\n", lptr->asc);
			exit(2);
		}

		if(lptr->chain) {
			rptr = lptr->chain;
			while(trptr = rptr->r2.lb1) {
				rptr->r2.lb1 = lptr->address;
				rptr = trptr;
			}
			rptr->r2.lb1 = lptr->address;
		}
	}
} /* dechain */

#ifdef NLS16	/* character sets too big to map into arrays */
char *ycomp(expbuf)
char    *expbuf;
{
	register char *ep, *tsp;	
	int c, d;
	char    *sp;

	ep = expbuf;
	sp = cp;
	for(tsp = cp; (c = CHARADV(tsp)) != sseof;  ) {
		if(c == '\\')
			tsp++;
		if(c == '\n' || c == NULL)
			return(badp);
	}
	while((c = CHARADV(sp)) != sseof) {
		if(c == '\\' && *sp == 'n') {
			sp++;
			c = '\n';
		}
		if((d = CHARADV(tsp)) == '\\' && *tsp == 'n') {
			d = '\n';
			tsp++;
		}
		if(d == sseof || d == '\0')
			return(badp);
		*ep++ = (c>>8) & 0377;
		*ep++ = c & 0377;
		*ep++ = (d>>8) &0377;
		*ep++ = d & 0377;
	}
	*ep++ = MRK;
	if(*tsp != sseof)
		return(badp);
	cp = ++tsp;
	return(ep);
} /* ycomp */
#else /* 8-bit algorithm */
char *ycomp(expbuf)
char    *expbuf;
{
	register char *ep, *tsp;	
	unsigned register char c;	/* modified for 8 bit */
	int i;
	char    *sp;

	ep = expbuf;
	sp = cp;
	for(tsp = cp; *tsp != sseof; tsp++) {
		if(*tsp == '\\')
			tsp++;
		if(*tsp == '\n' || *tsp == NULL)
			return(badp);
	}
	tsp++;
	while((c = *sp++ & 0377) != sseof) {	/* modified for 8 bit handling */
		if(c == '\\' && *sp == 'n') {
			sp++;
			c = '\n';
		}
		if((ep[c] = *tsp++) == '\\' && *tsp == 'n') {
			ep[c] = '\n';
			tsp++;
		}
		if(ep[c] == sseof || ep[c] == '\0')
			return(badp);
	}
	if(*tsp != sseof)
		return(badp);
	cp = ++tsp;

	for(i = 0; i < 0400; i++) {	/* modified for 8 bit handling */
		c = (unsigned char)i;
		if(ep[c] == 0)
			ep[c] = c;
	}

	return(ep + 0400);	/* modified for 8 bit handling */
} /* ycomp */
#endif NLS16
