/* @(#) $Revision: 37.3 $ */    
#ifdef NLS
#define NL_SETN 12
#include	<msgbuf.h>
#else
#define nl_msg(i, s) (s)
#endif
#ifdef NLS16
#include	<nl_ctype.h>
#endif NLS16
# include	"../../hdr/defines.h"
# include	"../../hdr/had.h"
# include	<sgtty.h>


extern char *Mrs;
extern int Domrs;

char	Cstr[RESPSIZE];
char	Mstr[RESPSIZE];
char	*savecmt();	/* function returning character ptr */

dohist(file)
char *file;
{
	char line[BUFSIZ];
	struct sgttyb tty;
	int doprmt;
	register char *p;
	FILE *in, *fdfopen();
	extern char *Comments, *getresp();

	in = xfopen(file,0);
	while ((p = fgets(line,sizeof(line),in)) != NULL)
		if (line[0] == CTLCHAR && line[1] == EUSERNAM)
			break;
	if (p != NULL) {
		while ((p = fgets(line,sizeof(line),in)) != NULL)
			if (line[3] == VALFLAG && line[1] == FLAG && line[0] == CTLCHAR)
				break;
			else if (line[1] == BUSERTXT && line[0] == CTLCHAR)
				break;
		if (p != NULL && line[1] == FLAG) {
			Domrs++;
		}
	}
	fclose(in);
	doprmt = 0;
	if (isatty(0) == 1)
		doprmt++;
	if (Domrs && !Mrs) {
		if (doprmt)
			printf((nl_msg(121,"MRs? ")));
		Mrs = getresp(" ",Mstr);
	}
	if (Domrs)
		mrfixup();
	if (!Comments) {
		if (doprmt)
			printf((nl_msg(122,"comments? ")));
		sprintf(line,"\n");
		Comments = getresp(line,Cstr);
	}
}


char *
getresp(repstr,result)
char *repstr;
char *result;
{
	char line[BUFSIZ], *strcat();
	register int done, sz;
	register char *p, *q;
	extern char	had_standinp;
	extern char	had[26];

	result[0] = 0;
	done = 0;
	/*
	save old fatal flag values and change to
	values inside ()
	*/
	FSAVE(FTLEXIT | FTLMSG | FTLCLN);
	if ((had_standinp && (!HADY || (Domrs && !HADM)))) {
		Ffile = 0;
		sprintf(Error,"%s (de16)",(nl_msg(123,"standard input specified w/o -y and/or -m keyletter")));
		fatal(Error);
	}
	/*
	restore the old flag values and process if above
	conditions were not met
	*/
	FRSTR();
	sz = sizeof(line) - size(repstr);
	while (!done && fgets(line,sz,stdin) != NULL) {
		++done;							/* assume this is the last line */
		p = strend(line);
		if (*--p == '\n') {					/* has to end with a new-line unless too much text */
			p = line;
			while (*p != '\0') {				/* scan down the line looking for a '\' quoting something */
				if (*p == '\\')
					if (*(p+1) == '\n') {		/* if it was the new-line that was quoted, */
						copy(repstr,p);		/* then replace it with the proper separator */
						--done;			/* and indicate that still more is to come */
					}
					else {				/* otherwise, eat the backslash by */
						q = p;			/* sliding the remainder of the string down over it */
						while (*(q-1) = *++q);
					}
#ifdef NLS16
				ADVANCE(p);				/* and continue with the remaining characters */
#else NLS16
				p++;					/* same only this won't work for multiple byte char's */
#endif NLS16
			}
			if (done)					/* if this is the last line */
				*--p = 0;				/* then kill the final new-line */
		}
		else
		{
			sprintf(Error,"%s (co18)",(nl_msg(124,"line too long")));
			fatal(Error);
		}
		if ((size(line) + size(result)) > RESPSIZE)
		{
			sprintf(Error,"%s (co19)",(nl_msg(125,"response too long")));
			fatal(Error);
		}
		strcat(result,line);
	}
	return(result);
}


char	*Qarg[NVARGS];
char	**Varg = Qarg;

valmrs(pkt,pgm)
struct packet *pkt;
char *pgm;
{
	extern char *Sflags[];
	register int i;
	int st;
	register char *p;
	char *auxf();

	Varg[0] = pgm;
	Varg[1] = auxf(pkt->p_file,'g');
	if (p = Sflags[TYPEFLAG - 'a'])
		Varg[2] = p;
	else
		Varg[2] = Null;
	if ((i = fork()) < 0) {
		sprintf(Error,"%s (co20)",(nl_msg(126,"cannot fork; try again")));
		fatal(Error);
	}
	else if (i == 0) {
		for (i = 4; i < 15; i++)
			close(i);
		execvp(pgm,Varg);
		exit(1);
	}
	else {
		wait(&st);
		return(st);
	}
}

# define	LENMR	60

mrfixup()
{
	register char **argv, *p, c;
	char *ap, *stalloc();
	int len;

	argv = &Varg[VSTART];
	p = Mrs;
	NONBLANK(p);
	for (ap = p; *p; p++) {
		if (*p == ' ' || *p == '\t') {
			if (argv >= &Varg[(NVARGS - 1)])
			{
				sprintf(Error,"%s (co21)",(nl_msg(127,"too many MRs")));
				fatal(Error);
			}
			c = *p;
			*p = 0;
			if ((len = size(ap)) > LENMR)
			{
				sprintf(Error,"%s (co24)",nl_msg(128,"MR number too long"));
				fatal(Error);
			}
			*argv = stalloc(len);
			copy(ap,*argv);
			*p = c;
			argv++;
			NONBLANK(p);
			ap = p;
		}
	}
	--p;
	if (*p != ' ' && *p != '\t') {
		if ((len = size(ap)) > LENMR)
		{
			sprintf(Error,"%s (co24)", (nl_msg(129,"MR number too long")));
			fatal(Error);
		}
		copy(ap,*argv++ = stalloc(len));
	}
	*argv = 0;
}


# define STBUFSZ	500

char *
stalloc(n)
register int n;
{
	static char stbuf[STBUFSZ];
	static int stind = 0;
	register char *p;

	p = &stbuf[stind];
	if (&p[n] >= &stbuf[STBUFSZ])
	{
		sprintf(Error,"%s (co22)",(nl_msg(130,"out of space")));
		fatal(Error);
	}
	stind += n;
	return(p);
}


char *savecmt(p)
register char *p;
{
	register char	*p1, *p2;
	int	ssize, nlcnt;
	char *fmalloc();

	nlcnt = 0;
	for (p1 = p; *p1; p1++)
		if (*p1 == '\n')
			nlcnt++;
/*
 *	ssize is length of line plus mush plus number of newlines
 *	times number of control characters per newline.
*/
	ssize = (strlen(p) + 4 + (nlcnt * 3)) & (~1);
	p1 = fmalloc(ssize);
	p2 = p1;
	while (1) {
		while(*p && *p != '\n')
			*p1++ = *p++;
		if (*p == '\0') {
			*p1 = '\0';
			return(p2);
		}
		else {
			p++;
			*p1++ = '\n';
			*p1++ = CTLCHAR;
			*p1++ = COMMENTS;
			*p1++ = ' ';
		}
	}
}
