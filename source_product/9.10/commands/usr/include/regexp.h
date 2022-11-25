/* @(#) $Revision: 70.6 $ */    
#ifndef _REGEXP_INCLUDED
#define _REGEXP_INCLUDED

#ifndef _SYS_STDSYMS_INCLUDED
#  include <sys/stdsyms.h>
#endif /* _SYS_STDSYMS_INCLUDED */

#ifdef __cplusplus
extern "C" {
#endif

#ifdef _INCLUDE_XOPEN_SOURCE

#define _GETC()		(GETC() & 0377)				/* 8-bit: insure against */
#define _PEEKC()	(PEEKC() & 0377)			/*        sign extension */
#define	NBRA		9					/* allowed number of paired parentheses */
#define RE_BUF_SIZE	1024					/* max size of incoming RE */

#include <nl_ctype.h>
#include <regex.h>

	regex_t		*__preg;
	char		*braslist[NBRA];
	char		*braelist[NBRA];
	int		nbra;
	char		*loc1, *loc2, *locs;
	int		sed;
	int		nodelim;
	int		circf;

#if defined(__STDC__) || defined(__cplusplus)
char *compile(register char *instring, register char *ep,
	      const char *endbuf, int seof)
#else /* __STDC__ || __cplusplus */
char *compile(instring, ep, endbuf, seof)
register char	*ep;
register char	*instring;
char		*endbuf;
int		seof;
#endif /* __STDC__ || __cplusplus */
{
	INIT							/* Dependent declarations and initializations */

	register		c;
	register		eof = seof & 0377;		/* 8 bit */
	unsigned char		re_buf[RE_BUF_SIZE+2];		/* place to assemble incoming regular expression */
	register unsigned char	*re = re_buf;
	int			err_num;

	if ( ((c = _GETC()) == eof) || (c == '\n') ) {
		if(c == '\n') {
			UNGETC(c);				/* don't eat the newline */
			nodelim = 1;				/* set flag for ed(1) */
		}
		/*
		 * A few commands (ed & more) set *ep to zero to indicate an empty
		 * RE string should be returned as an error.  Other commands just
		 * want a zero length compiled RE returned.
		 */
		if (*ep == 0 && !sed) {
			ERROR(REG_ENOSEARCH);
		} else {
			RETURN(ep);
		}
	}

	circf = nbra = 0;

	if (c == '^')						/* anchor at the beginning of line? */
		circf++;
	else
		UNGETC(c);


	/* copy remainder of RE input to RE buffer */
	while ( ((c = _GETC()) != eof) && (c != '\n') ) {

		*re++ = c;
		if (c == '\\') {
			c = _GETC();
			if (c == 'n')				/* '\' 'n' is converted to '\n' */
				*(re-1) = '\n';
			else if (c == '\n') {			/* '\' '\n' is not allowed */
				ERROR(REG_ENEWLINE);
			} else
				*re++ = c;
		}

		if (!c || re > &re_buf[RE_BUF_SIZE]) {  /* traditional to handle no delimiter as */
			ERROR(REG_ESPACE);		/* out of space error */
		}

		if (FIRSTof2(c) && SECof2(_PEEKC()))
			*re++ = _GETC();
	}
	*re = '\0';
	
	/* at the end of the RE */

	if(c == '\n') {
		UNGETC(c);					/* don't eat the newline */
		nodelim = 1;					/* set flag for ed(1) */

		if (sed) {					/* sed doesn't like RE's that end with newline */
			ERROR(REG_ENEWLINE);
		}
	}
	
#ifdef __hp9000s800
	__preg = (regex_t *)((int)(ep+7)&~3);
#else /* __hp9000s800 */
	__preg = (regex_t *)(ep+4);
#endif /* __hp9000s800 */
	__preg->__c_re	= (unsigned char *)__preg+sizeof(regex_t);
	__preg->__c_buf_end = (unsigned char *)endbuf;

#if defined(__STDC__) || defined(__cplusplus)
	if ( err_num = regcomp(__preg, (const char *)re_buf, _REG_NOALLOC|_REG_EXP) )
#else /* defined(__STDC__) || defined(__cplusplus) */
	if ( err_num = regcomp(__preg, re_buf, _REG_NOALLOC|_REG_EXP) )
#endif /* defined(__STDC__) || defined(__cplusplus) */
	{
		ERROR(err_num);
	}

	nbra = __preg->__re_nsub;					/* count of subexpressions */

	*ep=1;	/* so that we don't get an ERROR when compiling null string */

	RETURN((char *)__preg->__c_re_end);			/* return ptr to end of compiled RE + 1 */
}


#if  defined(__STDC__) || defined(__cplusplus)
step(const char *string, const char *expbuf)
#else /* __STDC__ || __cplusplus */
step(string, expbuf)
char *string, *expbuf;
#endif /* __STDC__ || __cplusplus */
{
int		i;
regmatch_t	pmatch[NBRA+1];

#ifdef __hp9000s800
	__preg = (regex_t *)((int)(expbuf+7)&~3);
#else /* __hp9000s800 */
	__preg = (regex_t *)(expbuf+4);
#endif /* __hp9000s800 */
	__preg->__c_re	= (unsigned char *)__preg+sizeof(regex_t);
	__preg->__anchor	= circf;

	do {
		if (regexec(__preg, string, NBRA+1, pmatch, 0))
			return(0);				/* no match found */

		loc1 = string+pmatch[0].__rm_so;		/* endpoints of matched string */
		loc2 = string+pmatch[0].__rm_eo;
	
	} while (locs && loc2 <= locs && _CHARADV(string));	/* find a match past locs if set */

	for (i=NBRA; i; i--) {
		braslist[i-1] = string+pmatch[i].__rm_so;		/* copy subexpression endpoints */
		braelist[i-1] = string+pmatch[i].__rm_eo;
	}

	return (1);						/* success */

}


#if defined(__STDC__) || defined(__cplusplus)
advance(const char *string, const char *expbuf)
#else /* __STDC__ || __cplusplus */
advance(string, expbuf)
char *string, *expbuf;
#endif /* __STDC__ || __cplusplus */
{
	int		i;
	regmatch_t	pmatch[NBRA+1];

#ifdef __hp9000s800
	__preg = (regex_t *)((int)(expbuf+7)&~3);
#else /* __hp9000s800 */
	__preg = (regex_t *)(expbuf+4);
#endif /* __hp9000s800 */
	__preg->__c_re	= (unsigned char *)__preg+sizeof(regex_t);
	__preg->__anchor	= 1;					/* advance() is always anchored */

	if (regexec(__preg, string, NBRA+1, pmatch, 0))
		return(0);					/* no match found */

	loc1 = string+pmatch[0].__rm_so;			/* endpoints of matched string */
	loc2 = string+pmatch[0].__rm_eo;

	if ((locs && loc2 <= locs))
		return(0);					/* match doesn't count if not past locs */

	for (i=NBRA; i; i--) {
		braslist[i-1] = string+pmatch[i].__rm_so;		/* copy subexpression endpoints */
		braelist[i-1] = string+pmatch[i].__rm_eo;
	}

	return (1);						/* success */

}

#endif /* _INCLUDE_XOPEN_SOURCE */

#ifdef __cplusplus
}
#endif

#endif /* _REGEXP_INCLUDED */
