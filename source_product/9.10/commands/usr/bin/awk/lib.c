/* $Revision: 72.2 $ */
/*
Copyright (c) 1984, 19885, 1986, 1987 AT&T
	All Rights Reserved

THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T.

The copyright notice above does not evidence any
actual or intended publication of such source code.
*/

#define DEBUG
#include <stdio.h>
#include <ctype.h>
#include <errno.h>
#include "awk.h"
#include "y.tab.h"
#include <varargs.h>


#if defined NLS || defined NLS16
	extern nl_catd	_nl_fn;		/* catalog descriptor */
#else
	extern int _nl_fn;
#endif


#define	getfval(p)	(((p)->tval & (ARR|FLD|REC|NUM)) == NUM ? (p)->fval : r_getfval(p))
#define	getsval(p)	(((p)->tval & (ARR|FLD|REC|STR)) == STR ? (p)->sval : r_getsval(p,CONVFMT))

extern	Awkfloat r_getfval();
extern	uchar	*r_getsval();

FILE	*infile	= NULL;
uchar	*file	= (uchar*) "";
uchar	recdata[RECSIZE];
uchar	*record	= recdata;
uchar	fields[RECSIZE];

#define	MAXFLD	200
int	donefld;	/* 1 = implies rec broken into fields */
int	donerec;	/* 1 = record is valid (no flds have changed) */

#define	FINIT	{ OCELL, CFLD, NULL, (uchar*) "", 0.0, FLD|STR|DONTFREE }

Cell fldtab[MAXFLD] = {		/* room for fields */
	{ OCELL, CFLD, (uchar*) "$0", recdata, 0.0, REC|STR|DONTFREE},
	FINIT, FINIT, FINIT, FINIT, FINIT, FINIT, FINIT, FINIT, FINIT, FINIT,
	FINIT, FINIT, FINIT, FINIT, FINIT, FINIT, FINIT, FINIT, FINIT, FINIT,
	FINIT, FINIT, FINIT, FINIT, FINIT, FINIT, FINIT, FINIT, FINIT, FINIT,
	FINIT, FINIT, FINIT, FINIT, FINIT, FINIT, FINIT, FINIT, FINIT, FINIT,
	FINIT, FINIT, FINIT, FINIT, FINIT, FINIT, FINIT, FINIT, FINIT, FINIT,
	FINIT, FINIT, FINIT, FINIT, FINIT, FINIT, FINIT, FINIT, FINIT, FINIT,
	FINIT, FINIT, FINIT, FINIT, FINIT, FINIT, FINIT, FINIT, FINIT, FINIT,
	FINIT, FINIT, FINIT, FINIT, FINIT, FINIT, FINIT, FINIT, FINIT, FINIT,
	FINIT, FINIT, FINIT, FINIT, FINIT, FINIT, FINIT, FINIT, FINIT, FINIT,
	FINIT, FINIT, FINIT, FINIT, FINIT, FINIT, FINIT, FINIT, FINIT, FINIT,
	FINIT, FINIT, FINIT, FINIT, FINIT, FINIT, FINIT, FINIT, FINIT, FINIT,
	FINIT, FINIT, FINIT, FINIT, FINIT, FINIT, FINIT, FINIT, FINIT, FINIT,
	FINIT, FINIT, FINIT, FINIT, FINIT, FINIT, FINIT, FINIT, FINIT, FINIT,
	FINIT, FINIT, FINIT, FINIT, FINIT, FINIT, FINIT, FINIT, FINIT, FINIT,
	FINIT, FINIT, FINIT, FINIT, FINIT, FINIT, FINIT, FINIT, FINIT, FINIT,
	FINIT, FINIT, FINIT, FINIT, FINIT, FINIT, FINIT, FINIT, FINIT, FINIT,
	FINIT, FINIT, FINIT, FINIT, FINIT, FINIT, FINIT, FINIT, FINIT, FINIT,
	FINIT, FINIT, FINIT, FINIT, FINIT, FINIT, FINIT, FINIT, FINIT, FINIT,
	FINIT, FINIT, FINIT, FINIT, FINIT, FINIT, FINIT, FINIT, FINIT, FINIT,
	FINIT, FINIT, FINIT, FINIT, FINIT, FINIT, FINIT, FINIT, FINIT,
};
int	maxfld	= 0;	/* last used field */
int	argno	= 1;	/* current input argument number */
extern	Awkfloat *ARGC;
extern	uchar	*getargv();

char *GetErrMsg();	/* forward */

initgetrec()
{
        int i;
        uchar *p;

        for (i = 1; i < *ARGC; i++) {
                if (!isclvar(p = getargv(i)))   /* find 1st real filename */
                        return;
                setclvar(p);    /* a commandline assignment before filename */
                argno++;
        }
        infile = stdin;         /* no filenames, so use stdin */
        /* *FILENAME = file = (uchar*) "-"; */
}

getrec(buf)
	uchar *buf;
{
	static int firsttime = 1;

	if (firsttime) {
		firsttime=0;
		initgetrec();
	}
	dprintf("RS=<%s>, FS=<%s>\n", *RecordSep, *FS);
	donefld = 0;
	donerec = 1;
	buf[0] = 0;
	while (argno < *ARGC || infile == stdin) {
		dprintf("argno=%d, file=|%s|\n", argno, file);
		if (infile == NULL) {	/* have to open a new file */
			file = getargv(argno);
			if (*file == '\0') {	/* it's been zapped */
				argno++;
				continue;
			}
			if (isclvar(file)) {	/* a var=value arg */
				setclvar(file);
				argno++;
				continue;
			}
			*FILENAME = file;
			dprintf("opening file %s\n", file);
			if (*file == '-' && *(file+1) == '\0')
				infile = stdin;
#ifndef hpe
			else if ((infile = fopen(file, "r")) == NULL)
#else
			else if ((infile = fopen(file, "r T")) == NULL)
#endif hpe
				error(FATAL,ERR26, file);
			setfval(fnrloc, 0.0);
		}
		
		if (readrec(buf, RECSIZE, infile)) {
			/* we have a record to process */
			if (buf == record) {
				if (!(recloc->tval & DONTFREE))
					xfree(recloc->sval);
				recloc->sval = record;
				recloc->tval = REC | STR | DONTFREE;
				if (isnumber(recloc->sval)) {
					recloc->fval = atof(recloc->sval);
					recloc->tval |= NUM;
				}
			}
			setfval(nrloc, nrloc->fval+1);
			setfval(fnrloc, fnrloc->fval+1);
			return 1;
		}
		/* EOF arrived on this file; set up next */
		if (infile != stdin)
			fclose(infile);
		infile = NULL;
		argno++;
	}
	return 0;	/* true end of file */
}

readrec(buf, bufsize, inf)	/* read one record into buf */
	uchar *buf;
	int bufsize;
	FILE *inf;
{
	register unsigned sep, c;
	register uchar *rr;

	if ((sep = CHARAT(*RecordSep)) == 0) {	/* NLS */
		sep = '\n';
		while ((c=getC(inf)) == '\n' && c != EOF)	/* skip leading \n's NLS */
			;
		if (c != EOF)
			ungetc(c, inf);
	}
	if (_CS_SBYTE) {    /* process single byte characters */
		/* Note: the double for() loop is neccessary */
		/*       to effiently handle the double new   */
		/*       line condition.  Whenever the Record */
		/*       separator is a NULL		      */
	     for ( rr = buf ;;) {
		for ( ; (c=getc(inf)) != sep && c != EOF && rr < buf+bufsize;)
			*(rr)++=c;
		if ( rr >= buf+bufsize)
			error(FATAL,ERR56, buf);	/* this function calls exit() */

		if ( **RecordSep == sep  || c == EOF )
			break;
		if (( c=getc(inf)) == '\n' || c == EOF ) /* two \n in a row */
			break;

		if ( rr >= buf+bufsize-2 )		/* check if we are going to overwrite buf */
			error(FATAL,ERR56, buf);
		*rr++ = '\n';  /* Place the \n character into the rr buffer. */ 
			        /* This could also be "sep".  		  */
		*rr++ = c;	/* Place byte just read into the rr buffer */
	     }
	} else {    /* process multi-byte characters */
	     for ( rr=buf ;; ) {
		for ( ; (c=getC(inf)) != sep && c != EOF && rr < buf+bufsize-1; )
			WCHARADV(c,rr);
		if (rr >= buf+bufsize-1)
			error(FATAL,ERR56, buf);	/* this function calls exit() */

		if ( CHARAT(*RecordSep) == sep  || c == EOF )
			break;
		if (( c=getC(inf)) == '\n' || c == EOF ) /* two \n in a row */
			break;

		if ( rr >= buf+bufsize-3 )	/* check if we are going to  */
						/* overwrite buf */
			error(FATAL,ERR56, buf);
		*rr++ = '\n';  /* Place the \n into buffer.  This could also */
			       /* be "sep".  				     */
		WCHARADV(c,rr); /* Place the byte read into the buffer  */
				/* This byte may be a 2-byte char.	*/
	     }
	}
	*rr = '\0';
	dprintf("readrec saw <%s>, returns %d\n", buf, c == EOF && rr == buf ? 0 : 1);
	return c != EOF || rr != buf; /* if we're not at the end of the file */
				      /* or we have read some data, then we  */
				      /* have a valid record.		     */
}

uchar *getargv(n)	/* get ARGV[n] */
	int n;
{
	Cell *x;
	uchar *s, temp[10];
	extern Array *ARGVtab;

	sprintf(temp, "%d", n);
	x = setsymtab(temp, "", 0.0, STR, ARGVtab);
	s = getsval(x);
	dprintf("getargv(%d) returns |%s|\n", n, s, NULL);
	return s;
}

setclvar(s)	/* set var=value from s */
uchar *s;
{
	uchar *p;
	Cell *q;

	for (p=s; *p != '='; p++)
		;
	*p++ = 0;
	p = qstring(p, '\0');
	q = setsymtab(s, p, 0.0, STR, symtab);
	setsval(q, p);
	q->tval |= STRCON;
	if (isnumber(q->sval)) {
		q->fval = atof(q->sval);
		q->tval |= NUM;
	}
	dprintf("command line set %s to |%s|\n", s, p, NULL);
}


static int ctest[0400];	/* NLS16: increased from 0200 */

fldbld()
{
	register uchar *r, *fr, sep;
	Cell *p;
	int i;

	if (donefld)
		return;
	if (!(recloc->tval & STR))
		getsval(recloc);
	r = recloc->sval;	/* was record! */
	fr = fields;
	i = 0;	/* number of fields accumulated here */
	if (strlen(*FS) > 1) {	/* it's a regular expression */
		i = refldbld(r, *FS);
	} else if ((sep = CHARAT(*FS)) == ' ') {	/* NLS */
		for (i = 0; ; ) {
			while (*r == ' ' || *r == '\t' || *r == '\n')
				r++;
			if (*r == 0)
				break;
			i++;
			if (i >= MAXFLD)
				break;
			if (!(fldtab[i].tval & DONTFREE))
				xfree(fldtab[i].sval);
			fldtab[i].sval = fr;
			fldtab[i].tval = FLD | STR | STRCON | DONTFREE;
			/* NLS16 - valid 2nd byte will never be one of these */
			ctest[' '] = ctest['\t'] = ctest['\n'] = ctest['\0'] =1;
			do
				*fr++ = *r++;
			/*while (*r != ' ' && *r != '\t' && *r != '\n' && *r != '\0');*/
			while(!ctest[*(unsigned char *)r]);     /* NLS: changed to unsigned char */
				ctest[' '] = ctest['\t'] = ctest['\n'] = ctest['\0'] =0;

			*fr++ = 0;
		}
		*fr = 0;
	} else if (*r != 0) {	/* if 0, it's a null field */
		for (;;) {
			i++;
			if (i >= MAXFLD)
				break;
			if (!(fldtab[i].tval & DONTFREE))
				xfree(fldtab[i].sval);
			fldtab[i].sval = fr;
			fldtab[i].tval = FLD | STR | STRCON | DONTFREE;

#ifndef NLS16
			ctest[sep] = ctest['\n'] = ctest['\0'] = 1;
			while(!ctest[*r])
			/*while (*r != sep && *r != '\n' && *r != '\0')*/
			/* \n always a separator */
			*fr++ = *r++;
			ctest[sep] = ctest['\n'] = ctest['\0'] = 0;
#else
			if (_CS_SBYTE) {
				register c;
                        	while ((c = *r)!=sep && c!='\n' && c!='\0') {
					*(fr)++ = c;
					r++;
				}
			} else {
				register c;
                        	while ((c = CHARAT(r)) != sep && c != '\n' && c != '\0') {
					WCHARADV(c, fr);
					ADVANCE(r);
				}
			}
#endif
			*fr++ = 0;
			if (*r == 0)	/* NLS: changed from *r++ */
				break;
			ADVANCE(r);	/* NLS */
		}
		*fr = 0;
	}
	if (i >= MAXFLD)
		error(FATAL,ERR72, record);
	/* clean out junk from previous record */
	cleanfld(i, maxfld);
	maxfld = i;
	donefld = 1;
	for (p = fldtab+1; p <= fldtab+maxfld; p++) {
		if(isnumber(p->sval)) {
			p->fval = atof(p->sval);
			p->tval |= NUM|NUMCON;
		}
	}
	setfval(nfloc, (Awkfloat) maxfld);
	if (dbg)
		for (p = fldtab; p <= fldtab+maxfld; p++)
			printf("field %d: |%s|\n", p-fldtab, p->sval);
}

cleanfld(n1, n2)	/* clean out fields n1..n2 inclusive */
{
	static uchar *nullstat = (uchar *) "";
	register Cell *p, *q;

	for (p = &fldtab[n2], q = &fldtab[n1]; p > q; p--) {
		if (!(p->tval & DONTFREE))
			xfree(p->sval);
		p->tval = FLD | STR | DONTFREE;
		p->sval = nullstat;
	}
}

newfld(n)	/* add field n (after end) */
{
	if (n >= MAXFLD)
		error(FATAL,ERR29, record );
	cleanfld(maxfld, n);
	maxfld = n;
	setfval(nfloc, (Awkfloat) n);
}

refldbld(rec, fs)	/* build fields from reg expr in FS */
	uchar *rec, *fs;
{
	fa *makedfa();
	uchar *fr;
	int i, tempstat;
	fa *pfa;

	fr = fields;
	*fr = '\0';
	if (*rec == '\0')
		return 0;
	pfa = makedfa(fs);
	dprintf("into refldbld, rec = <%s>, pat = <%s>\n", rec, fs);
	for (i = 1; i < MAXFLD; i++) {
		if (!(fldtab[i].tval & DONTFREE))
			xfree(fldtab[i].sval);
		fldtab[i].tval = FLD | STR | STRCON | DONTFREE;
		fldtab[i].sval = fr;
		dprintf("refldbld: i=%d\n", i);
		if (nematch(pfa, rec)) {
			dprintf("match %s (%d chars)\n", patbeg, patlen);
			strncpy(fr, rec, patbeg-rec);
			fr += patbeg - rec + 1;
			*(fr-1) = '\0';
			rec = patbeg + patlen;
		} else {
			dprintf("no match %s\n", rec);
			strcpy(fr, rec);
			break;
		}
	}
	return i;		
}

recbld()
{
	int i;	/* NLS */
	register uchar *r, *p, *c;
	static uchar rec[RECSIZE];

	if (donerec == 1)
		return;
	r = rec;
	for (i = 1; i <= *NF; i++) {
		p = getsval(&fldtab[i]);
		while (*r = *p++)
			r++;
		if (i < *NF) {		/* NLS */

			c = *OFS;
			if (_CS_SBYTE) {
				while (1) {
					*(r)++ = *c;
					if (*c++ == 0) {
						break;
					}
				}
			} else {
				while (1) {
					WCHARADV(*c,r);
					if (*c++ == 0) {
						break;
					}
				}
			}
			r--;
		}
	}
	*r = '\0';
	dprintf("in recbld FS=%o, recloc=%o\n", **FS, recloc, NULL);
	recloc->tval = REC | STR | DONTFREE;
	recloc->sval = record = rec;
	dprintf("in recbld FS=%o, recloc=%o\n", **FS, recloc, NULL);
	if (r > record + RECSIZE)
		error(FATAL,ERR22, record);
	dprintf("recbld = |%s|\n", record, NULL, NULL);
	donerec = 1;
}

Cell *fieldadr(n)
{
	if (n < 0 || n >= MAXFLD)
		error(FATAL,ERR81, n);
	return(&fldtab[n]);
}

int	errorflag	= 0;
char	errbuf[200];

yyerror(s)
uchar *s;
{
	extern uchar *cmdname, *curfname;
	static int been_here = 0;

	if (been_here++ > 2)
		return;
	fprintf(stderr,"%s: %s",cmdname,s);
	fprintf(stderr,GetErrMsg(YYERR1), lineno);
	if (curfname != NULL)
		fprintf(stderr,GetErrMsg(YYERR2), curfname);
	fprintf(stderr, "\n");
	errorflag = 2;
	eprint();
}

fpecatch()
{
	error(FATAL,ERR39);
}

extern int bracecnt, brackcnt, parencnt;

bracecheck()
{
	int c;
	static int beenhere = 0;

	if (beenhere++)
		return;
	while ((c = input()) != EOF && c != '\0')
		bclass(c);
	bcheck2(bracecnt, '{', '}');
	bcheck2(brackcnt, '[', ']');
	bcheck2(parencnt, '(', ')');
}

bcheck2(n, c1, c2)
{
	if (n == 1)
		fprintf(stderr, GetErrMsg(ERR97), c2);
	else if (n > 1)
		fprintf(stderr, GetErrMsg(ERR98), n, c2);
	else if (n == -1)
		fprintf(stderr, GetErrMsg(ERR99), c2);
	else if (n < -1)
		fprintf(stderr, GetErrMsg(ERR100), -n, c2);
}


static char *err_mess[] = {
	" at source line %d",				/* catgets 01 */
	" context is\n\t",				/* catgets 02 */
	" in function %s",				/* catgets 03 */
	" input record number %g",			/* catgets 04 */
	" source line number %d\n",			/* catgets 05 */
	"%s argument out of domain",			/* catgets 06 */
	"%s is a function, not an array",		/* catgets 07 */
	"%s is an array, not a function",		/* catgets 08 */
	"%s is not an array",				/* catgets 09 */
	"%s makes too many open files",			/* catgets 10 */
	"%s result out of range",			/* catgets 11 */
	", file %s",					/* catgets 12 */
	"Usage: %s [-f source | 'cmds'] [files]",	/* catgets 13 */
	"\t%d extra %c's\n",				/* catgets 14 */
	"\t%d missing %c's\n",				/* catgets 15 */
	"\textra %c\n",					/* catgets 16 */
	"\tmissing %c\n",				/* catgets 17 */
	"`%s' is an array name and a function name",	/* catgets 18 */
	"argument #%d of function %s was not supplied",	/* catgets 19 */
	"bad format string in printf(%s)",		/* catgets 20 */
	"bailing out",					/* catgets 21 */
	"built giant record `%.20s...'",		/* catgets 22 */
	"calling undefined function %s",		/* catgets 23 */
	"can't %s %s; it's a function.",		/* catgets 24 */
	"can't %s %s; it's an array name.",		/* catgets 25 */
	"can't open %s",				/* catgets 26 */
	"can't open file %s",				/* catgets 27 */
	"can't use function %s as argument",		/* catgets 28 */
	"creating too many fields",			/* catgets 29 */
	"division by zero in %=",			/* catgets 30 */
	"division by zero in /=",			/* catgets 31 */
	"division by zero in mod",			/* catgets 32 */
	"division by zero",				/* catgets 33 */
	"empty regular expression",			/* catgets 34 */
	"empty restore file\n",				/* catgets 35 */
	"extra )",					/* catgets 36 */
	"extra ]",					/* catgets 37 */
	"extra }",					/* catgets 38 */
	"floating point exception",			/* catgets 39 */
	"format item %.20s... too long",		/* catgets 40 */
	"out of space for stack frames in %s",		/* catgets 41 */
	"funny variable %o: n=%s s=\"%s\" f=%g t=%o",	/* catgets 42 */
	"gsub() result %.20s too big",			/* catgets 43 */
	"illegal arithmetic operator %d",		/* catgets 44 */
	"illegal assignment operator %d",		/* catgets 45 */
	"illegal field $(%s)",				/* catgets 46 */
	"illegal function type %d",			/* catgets 47 */
	"illegal jump type %d",				/* catgets 48 */
	"illegal nested function",			/* catgets 49 */
	"illegal primary in regular expression %s at %s",/* catgets 50 */
	"illegal redirection",				/* catgets 51 */
	"illegal reference to array %s",		/* catgets 52 */
	"illegal statement",				/* catgets 53 */
	"illegal type of split()",			/* catgets 54 */
	"index() doesn't permit regular expressions",	/* catgets 55 */
	"input record `%.20s...' too long",		/* catgets 56 */
	"newline in character class %s...",		/* catgets 57 */
	"newline in regular expression %.10s...",	/* catgets 58 */
	"newline in string %.10s...",			/* catgets 59 */
	"next is illegal inside a function",		/* catgets 60 */
	"no space for temporaries",			/* catgets 61 */
	"nonterminated character class %s",		/* catgets 62 */
	"not enough args in printf(%s)",		/* catgets 63 */
	"not enough arguments in printf(%s)",		/* catgets 64 */
	"not restored\n",				/* catgets 65 */
	"not saved\n",					/* catgets 66 */
	"null file name in print or getline",		/* catgets 67 */
	"out of space in makesymtab",			/* catgets 68 */
	"out of space in nodealloc",			/* catgets 69 */
	"out of space in rehash",			/* catgets 70 */
	"out of space in tostring on %s",		/* catgets 71 */
	"record `%.20s...' has too many fields",	/* catgets 72 */
	"regular expression too big: %s",		/* catgets 73 */
	"return not in function",			/* catgets 74 */
	"string %.10s... too long",			/* catgets 75 */
	"string too long",				/* catgets 76 */
	"string/reg expr %.10s... too long",		/* catgets 77 */
	"sub() result %.20s too big",			/* catgets 78 */
	"symbol table overflow at %s",			/* catgets 79 */
	"syntax error in regular expression %s at %s",	/* catgets 80 */
	"trying to access field %d",			/* catgets 81 */
	"unexpected break or next",			/* catgets 82 */
	"unexpected break, continue or next",		/* catgets 83 */
	"unknown boolean operator %d",			/* catgets 84 */
	"unknown relational operator %d",		/* catgets 85 */
	"unknown type %d in cfoll",			/* catgets 86 */
	"unknown type %d in first\n",			/* catgets 87 */
	"unknown type %d in freetr",			/* catgets 88 */
	"unknown type %d in penter\n",			/* catgets 89 */
	"you can't define function %s more than once",	/* catgets 90 */
	" input record number %g",			/* catgets 91 ERRREC */
	", file %s",					/* catgets 92 ERRFILE */
	" source line number %d\n",			/* catgets 93 ERRLINE */
	" context is\n\t",				/* catgets 94 ERRCNTX */
	" at source line %d",				/* catgets 95 YYERR1 */
	" in function %s",				/* catgets 96 YYERR2 */
	"\tmissing %c\n",				/* catgets 97 */
	"\t%d missing %c's\n",				/* catgets 98 */
	"\textra %c\n",					/* catgets 99 */
	"\t%d extra %c's\n",				/* catgets 100 */
	"invalid collation element referenced\n",	/* catgets 101 RERR101*/
	"trailing \\ in pattern\n",			/* catgets 102 RERR102*/
	"\\n found before end of pattern\n",		/* catgets 103 RERR103*/
	"nesting level too deep\n",			/* catgets 104 RERR104*/
	"number in \\digit invalid or in error\n",	/* catgets 105 RERR105*/
	"[ ] imbalance\n",				/* catgets 106 RERR106*/
	"\\( \\) or ( ) imbalance\n",			/* catgets 107 RERR107*/
	"\\{ \\} imbalance\n",				/* catgets 108 RERR108*/
	"invalid endpoint in range statement\n",	/* catgets 109 RERR109*/
	"out of memory for compiled pattern\n",		/* catgets 110 RERR110*/
	"number too large in \\{ \\} construct\n",	/* catgets 111 RERR111*/
	"invalid number in \\{ \\} construct\n",	/* catgets 112 RERR112*/
	"more than two numbers in \\{ \\} construct\n",	/* catgets 113 RERR113*/
	"first number exceeds second in \\{ \\} construct\n", /* catgets 114 RERR114*/
	"invalid character class type named\n",		/* catgets 115 RERR115*/
	"no remembered search string\n",		/* catgets 116 RERR116*/
	"duplication operator in illegal position\n",	/* catgets 117 RERR117*/
	"no expression within ( ) or on one side of an |\n", /* catgets 118 RERR118*/
	"you can only delete array[element]\n",		/* catgets 119 ERR119 */
	"no program given\n",				/* catgets 120 ERR120 */
	"out of space for stack frames calling %s\n",	/* catgets 121 ERR121 */
	"function %s called with %d args, uses only %d\n",/* catgets 122 ERR122 */
	"function %s has %d arguments, limit %d\n",	/* catgets 123 ERR123 */
	"tempcell list is curdled\n",			/* catgets 124 ERR124 */
	"printf string %.40s... too long\n",		/* catgets 125 ERR125 */
	"out of space concatenating %.15s and %.15s\n",	/* catgets 126 ERR126 */
	"atan2 requires two arguments; returning 1.0\n",/* catgets 127 ERR127 */
	"warning: function has too many arguments\n",	/* catgets 128 ERR128 */
	"i/o error occurred on %s\n",			/* catgets 129 ERR129 */
	"i/o error occurred closing %s\n",		/* catgets 130 ERR130 */
	"too many program files: Ignoring: %s\n",	/* catgets 131 ERR131 */
};


rxerror(i)
int i;
{
	switch(i) {
		case REG_ECOLLATE: error(FATAL,RERR101); break;
		case REG_EESCAPE: error(FATAL,RERR102); break;
		case REG_ENEWLINE: error(FATAL,RERR103); break;
		case REG_ENSUB: error(FATAL,RERR104); break;
		case REG_ESUBREG: error(FATAL,RERR105); break;
		case REG_EBRACK: error(FATAL,RERR106); break;
		case REG_EPAREN: error(FATAL,RERR107); break;
		case REG_EBRACE: error(FATAL,RERR108); break;
		case REG_ERANGE: error(FATAL,RERR109); break;
		case REG_ESPACE: error(FATAL,RERR110); break;
		/*
		  REG_[A-Z]BRACE is subsituted by REG_BADBR
		  because of regcomp change
		*/
		case REG_BADBR: error(FATAL,RERR112); break;
		case REG_ECTYPE: error(FATAL,RERR115); break;
		case REG_ENOSEARCH: error(FATAL,RERR116); break;
		case REG_EDUPOPER: error(FATAL,RERR117); break;
		case REG_ENOEXPR: error(FATAL,RERR118); break;
	}
}


char *GetErrMsg(num)
int num;
{
	char *fmt;

	if (*(fmt = catgets( _nl_fn, NL_SETN, num, err_mess[num-1])) == '\0') {
		fmt = err_mess[num-1];
	}
	return(fmt);
}


error(type,num,va_alist)
int type;		/* Syntax, Warning, or Fatal */
int num;		/* message number */
va_dcl			/* optional arguments */
{
	char *fmt;	/* ptr to format string */
	va_list	args;	/* optional arg list */
	extern Node *curnode;
	extern uchar *cmdname;
	uchar	s[80];

	if (type==SYNTAX) {
		fmt = GetErrMsg(num);
		va_start( args);
		vsprintf(s,fmt,args);
		va_end( args);
		yyerror(s);
		return;
	}
	/* get the errror message format string */
	fmt = GetErrMsg(num);

	/* set up the optional argument list */
	va_start( args);

	/* print the error message on stderr */
	fprintf( stderr, "%s: ",cmdname);
	vfprintf( stderr, fmt, args);
	fprintf( stderr, "\n");

	/* close down the optional argument list */
	va_end( args);

	if (type != VERYFATAL) {
		/* print the record number on stderr */
		if (NR && *NR > 0) {
			fprintf(stderr,GetErrMsg(ERRREC), *FNR);
			if (**FILENAME && strcmp(*FILENAME, "-") != 0)
				fprintf(stderr,GetErrMsg(ERRFILE), *FILENAME);
			fprintf(stderr, "\n");
		}
		if (curnode)
			fprintf(stderr,GetErrMsg(ERRLINE), curnode->lineno);
		else if (lineno)
			fprintf(stderr,GetErrMsg(ERRLINE), lineno);
		eprint();
	}

	if (type==FATAL || type==VERYFATAL) {
		if (dbg)
			abort();
		exit(2);
	}
}


eprint()	/* try to print context around error */
{
	uchar *p, *q;
	int c;
	static int been_here = 0;
	extern uchar ebuf[300], *ep;

	if (compile_time == 2 || compile_time == 0 || been_here++ > 0)
		return;
	p = ep - 1;
	if (p > ebuf && *p == '\n')
		p--;
	for ( ; p > ebuf && *p != '\n' && *p != '\0'; p--)
		;
	while (*p == '\n')
		p++;
	fprintf(stderr,GetErrMsg(ERRCNTX));
	for (q=ep-1; q>=p && *q!=' ' && *q!='\t' && *q!='\n'; q--)
		;
        for ( ; p < q; p++)
                if (*p)
                        putc(*p, stderr);
	fprintf(stderr, " >>> ");
        for ( ; p < ep; p++)
                if (*p)
                        putc(*p, stderr);
	fprintf(stderr, " <<< ");
	if (*ep)
		while ((c = input()) != '\n' && c != '\0' && c != EOF) {
			putc(c, stderr);
			bclass(c);
		}
	putc('\n', stderr);
	ep = ebuf;
}

bclass(c)
{
	switch (c) {
	case '{': bracecnt++; break;
	case '}': bracecnt--; break;
	case '[': brackcnt++; break;
	case ']': brackcnt--; break;
	case '(': parencnt++; break;
	case ')': parencnt--; break;
	}
}

double errcheck(x, s)
	double x;
	uchar *s;
{
	extern int errno;

	if (errno == EDOM) {
		errno = 0;
		error(WARNING,ERR06, s);
		x = 1;
	} else if (errno == ERANGE) {
		errno = 0;
		error(WARNING,ERR11, s);
		x = 1;
	}
	return x;
}

PUTS(s) uchar *s; {
	dprintf("%s\n", s, NULL, NULL);
}

isclvar(s)	/* is s of form var=something? */
	uchar *s;
{
	uchar *os = s;

	for ( ; *s; s++)
		if (!(isalnum(*s) || *s == '_'))
			break;
	return *s == '=' && s > os && *(s+1) != '=';
}

#define	MAXEXPON	38	/* maximum exponent for fp number */

isnumber(s)
register uchar *s;
{
	register d1, d2;
	int point;
	uchar *es;

	d1 = d2 = point = 0;
	while (*s == ' ' || *s == '\t' || *s == '\n')
		s++;
	if (*s == '\0')
		return(0);	/* empty stuff isn't number */
	if (*s == '+' || *s == '-')
		s++;
	if (!isdigit(*s) && *s != '.')
		return(0);
	if (isdigit(*s)) {
		do {
			d1++;
			s++;
		} while (isdigit(*s));
	}
	if(d1 >= MAXEXPON)
		return(0);	/* too many digits to convert */
	if (*s == '.') {
		point++;
		s++;
	}
	if (isdigit(*s)) {
		d2++;
		do {
			s++;
		} while (isdigit(*s));
	}
	if (!(d1 || point && d2))
		return(0);
	if (*s == 'e' || *s == 'E') {
		s++;
		if (*s == '+' || *s == '-')
			s++;
		if (!isdigit(*s))
			return(0);
		es = s;
		do {
			s++;
		} while (isdigit(*s));
		if (s - es > 2)
			return(0);
		else if (s - es == 2 && (int)(10 * (*es-'0') + *(es+1)-'0') >= MAXEXPON)
			return(0);
	}
	while (*s == ' ' || *s == '\t' || *s == '\n')
		s++;
	if (*s == '\0')
		return(1);
	else
		return(0);
}

#ifdef NLS16
getC(f) FILE *f;        /* retrieve a character, not necessarily < 255 */
{
	static peekc = 0;

	if (_CS_SBYTE)
		return(getc(f));
	else {
		int c;

		if ((c = peekc) == 0)
			c = getc(f);
		peekc = 0;
		if (c == EOF)
			return EOF;
		if (FIRSTof2(c)){
			peekc = getc(f);
			if (peekc == EOF)
				return EOF;
			if (!iscntrl(peekc) && !isspace(peekc)){
				c = (c << 8) | peekc;
				peekc = 0;
			}
		}
		return c;
	}
}
#endif

