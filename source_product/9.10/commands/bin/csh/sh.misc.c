/* @(#) $Revision: 70.2 $ */   
/***************************************************************
 *         C SHELL
 **************************************************************/

#ifndef NLS
#define catgets(i,sn,mn, s) (s)
#else NLS
#define NL_SETN 11	/* set number */
#include <nl_types.h>
nl_catd nlmsg_fd;
#endif NLS

#include "sh.h"
#ifdef NLS16
#include <nl_ctype.h>
#endif

#ifdef NLS
extern	int	_nl_space_alt;
#define	ALT_SP	(_nl_space_alt & TRIM)
#endif

#define MAX_SPARE	10

extern int env_spare;

/*
 * C Shell
 */

letter(c)
	register short int c;
{
#ifndef NONLS
	/* for now consider all kana or kanji (except alt. space) as letters */
	return (c >= 'a' && c <= 'z' || c >= 'A' && c <= 'Z' || c == '_'|| c > 0177 && c != ALT_SP);
#else
	return (c >= 'a' && c <= 'z' || c >= 'A' && c <= 'Z' || c == '_');
#endif
}

digit(c)
	register CHAR c;
{

	return (c >= '0' && c <= '9');
}

alnum(c)
	register CHAR c;
{
	return (letter(c) || digit(c));
}

any(c, s)
	register int c;
	register char *s;
{

	while (*s)
		if (*s++ == c)
			return(1);
	return(0);
}

CHAR *
calloc(i, j)
	register unsigned i;
	unsigned j;
{
	register char *cp, *dp;
#ifdef debug 
	static char *av[2] = {0, 0};
#endif 
	extern char *malloc();

	i *= j;
	cp = malloc(i);
	if (cp == 0) {
		child++;
#ifndef debug
		error((catgets(nlmsg_fd,NL_SETN,1, "Out of memory")));
#else  
		showall(av);
		printf("i=%d, j=%d: ", i/j, j);
		printf("Out of memory\n");
		chdir("/usr/bill/cshcore");
		abort();
#endif  
	}
	dp = cp;
	if (i != 0)
		do
			*dp++ = 0;
		while (--i);
	return ((CHAR *)cp);
}

cfree(p)
	CHAR *p;
{

	free((char *)p);
}

CHAR **
blkend(up)
	register CHAR **up;
{

	while (*up)
		up++;
	return (up);
}
 
blkpr(av)
	register CHAR **av;
{

	for (; *av; av++) {
		printf("%s", to_char(*av));
		if (av[1])
			printf(" ");
	}
}

blklen(av)
	register CHAR **av;
{
	register int i = 0;

	while (*av++)
		i++;
	return (i);
}

CHAR **
blkcpy(oav, bv)
	CHAR **oav;
	register CHAR **bv;
{
	register CHAR **av = oav;

	while (*av++ = *bv++)
		continue;
	return (oav);
}

CHAR **
blkcat(up, vp)
	CHAR **up, **vp;
{

	(void) blkcpy(blkend(up), vp);
	return (up);
}

blkfree(av0)
	CHAR **av0;
{
	register CHAR **av = av0;

	while (*av)
		xfree(*av++);
	xfree((CHAR *)av0);
}

CHAR **
saveblk(v)
	register CHAR **v;
{
	register int len = blklen(v) + 1;
	register CHAR **newv = (CHAR **) calloc((unsigned) len, sizeof (CHAR **));
	CHAR **onewv = newv;

	while (*v)
		*newv++ = savestr(*v++);
	return (onewv);
}

char *
strspl(cp, dp)
	register char *cp, *dp;
{
	register char *ep = (char *)calloc(1, (unsigned) (strlen(cp) + strlen(dp) + 1));

	strcpy(ep, cp);
	strcat(ep, dp);
	return (ep);
}

CHAR **
blkspl(up, vp)
	register CHAR **up, **vp;
{
	register CHAR **wp = (CHAR **) calloc((unsigned) (blklen(up) + blklen(vp) + 1), sizeof (CHAR **));

	(void) blkcpy(wp, up);
	return (blkcat(wp, vp));
}

/*
 * This routine is specifically for operating on the environ/Environ
 * array.  It uses a global variable, env_spare, to keep track of how
 * many spare entries are left in the array.
 */
CHAR **
blkspl_spare(up, vp)
	register CHAR **up, **vp;
{
	register CHAR **wp;
	CHAR **savewp, **saveup = up;
	unsigned vlen;

	if (env_spare >= (vlen = blklen(vp)))
	{
		env_spare -= vlen;
		return (blkcat(up, vp));
	}
	else 
	{
		savewp = wp = (CHAR **) calloc((unsigned) (blklen(up) + vlen + 
					MAX_SPARE + 1), sizeof (CHAR **));
		env_spare = MAX_SPARE;
		while (*wp = *up++) wp++;
		while (*wp++ = *vp++);
		xfree(saveup);
		return (savewp);
	}
}

lastchr(cp)
	register CHAR *cp;
{

	if (!*cp)
		return (0);
	while (cp[1])
		cp++;
	return (*cp);
}

/*
 * This routine is called after an error to close up
 * any units which may have been left open accidentally.
 */
closem()
{
	register int f;

	for (f=getnumfds()-1; f >= 0; f--)
		if (f != SHIN && f != SHOUT && f != SHDIAG && f != OLDSTD &&
		    f != FSHTTY
#ifdef NLS
		    /* do  not close the message catalog file */
		    && f != nlmsg_fd
#endif
		    )
			close(f);
}

/*
 * Close files before executing a file.
 * We could be MUCH more intelligent, since (on a version 7 system)
 * we need only close files here during a source, the other
 * shell fd's being in units 16-19 which are closed automatically!
 */
closech()
{
	register int f;

	if (didcch)
		return;
	didcch = 1;
	SHIN = 0; SHOUT = 1; SHDIAG = 2; OLDSTD = 0;
	for (f=getnumfds()-1;f>=3;f--)
#ifdef NLS
		/* do not close message catalog file */
		/* it is close on exec() automatically */
		if (f != nlmsg_fd)		/* msgcat */
#endif
			close(f);
}

donefds()
{

	close(0), close(1), close(2);
	didfds = 0;
}

/*
 * Move descriptor i to j.
 * If j is -1 then we just want to get i to a safe place,
 * i.e. to a unit > 2.  This also happens in dcopy.
 */
dmove(i, j)
	register int i, j;
{

	if (i == j || i < 0)
		return (i);
#ifdef V7
	if (j >= 0) {
		dup2(i, j);
		return (j);
	} else
#endif
		j = dcopy(i, j);
	if (j != i)
		close(i);
	return (j);
}

dcopy(i, j)
	register int i, j;
{

	if (i == j || i < 0 || j < 0 && i > 2)
		return (i);
#ifdef V7
	if (j >= 0) {
		dup2(i, j);
		return (j);
	}
#endif
	close(j);
	return (renum(i, j));
}

renum(i, j)
	register int i, j;
{
	register int k = dup(i);

	if (k < 0)
		return (-1);
	if (j == -1 && k > 2)
		return (k);
	if (k != j) {
		j = renum(k, j);
		close(k);
		return (j);
	}
	return (k);
}

copy(to, from, size)
	register CHAR *to, *from;
	register int size;
{

	if (size)
		do
			*to++ = *from++;
		while (--size != 0);
}

/*
 * Left shift a command argument list, discarding
 * the first c arguments.  Used in "shift" commands
 * as well as by commands like "repeat".
 */
lshift(v, c)
	register CHAR **v;
	register int c;
{
	register CHAR **u = v;

	while (*u && --c >= 0)
		xfree(*u++);
	(void) blkcpy(v, u);
}

number(cp)
	CHAR *cp;
{

	if (*cp == '-') {
		cp++;
		if (!digit(*cp++))
			return (0);
	}
	while (*cp && digit(*cp))
		cp++;
	return (*cp == 0);
}

CHAR **
copyblk(v)
	register CHAR **v;
{
	register CHAR **nv = (CHAR **) calloc((unsigned) (blklen(v) + 1), sizeof (CHAR **));

	return (blkcpy(nv, v));
}

CHAR *
strend(cp)
	register CHAR *cp;
{

	while (*cp)
		cp++;
	return (cp);
}

CHAR *
strip(cp)
	CHAR *cp;
{
	register CHAR *dp = cp;

	while (*dp++ &= TRIM)
		continue;
	return (cp);
}

udvar(name)
	CHAR *name;
{

	setname(to_char(name));
	bferr((catgets(nlmsg_fd,NL_SETN,2, "Undefined variable")));
}

prefix(sub, str)
	register CHAR *sub, *str;
{

	for (;;) {
		if (*sub == 0)
			return (1);
		if (*str == 0)
			return (0);
		if (*sub++ != *str++)
			return (0);
	}
}

#ifndef NONLS
/* The routines below are for 8-16 bit processing.  They are mostly analogous */
/* copies of existing or library routines. They begin w/capital letter  */

Strlen(s)
CHAR *s;
{
	register CHAR *s0 = s + 1;

	if (s == NOSTR) return (0);
	while (*s++ != '\0')
		;
	return (s - s0);
}

CHAR *Strcpy(s1, s2)
CHAR *s1, *s2;
{
	register CHAR *os1;
	
	if (s2 == NOSTR) {
		*s1 = '\0';
		return(s1);
	}
	os1 = s1;
	while (*s1++ = *s2++)
		;
	return(os1);
}

CHAR *Strncpy(s1, s2, n)
CHAR *s1, *s2;
int n;
{
	register CHAR *os1 = s1;
	
	if (s2 == NOSTR) {
		while (--n >= 0)
		    *s1++ = '\0';
	}
	else {
		while (--n >= 0)
			if ((*s1++ = *s2++) == '\0')
			    while (--n >= 0)
				*s1++ = '\0';
	}
	return(os1);
}

CHAR *Strcat(s1, s2)
CHAR *s1, *s2;
{
	register CHAR *os1;

	if (s2 == NOSTR) return(s1);
	os1 = s1;
	while(*s1++)
		;
	-- s1;
	while(*s1++ = *s2++)
		;
	return(os1);
}

Strcmp(s1, s2)
CHAR *s1, *s2;
{
	if (s1 == s2) return(0);
	if (s1 == NOSTR) {
		if (s2 == NOSTR)
			return(0);
		else
			return(-*s2);
	}
	else if (s2 == NOSTR)
		return (*s1);
	
	while (*s1 == *s2++)
		if (*s1++ == '\0') return(0);
	return(*s1 - *--s2);
}

CHAR *Strchr(sp,c)
CHAR *sp, c;
{
	if (sp == NOSTR) return (0);
	do {
		if (*sp == c)
			return(sp);
	} while(*sp++);
	return (0);
}

CHAR *Strrchr(sp,c)
CHAR *sp, c;
{
	CHAR *r;

	if (sp == NOSTR) return (NOSTR);
	r = NOSTR;
	do {
		if (*sp == c)
			r = sp;
	} while(*sp++);
	return (r);
}

eq(s1, s2)
CHAR *s1;
char *s2;
{
	if (s1 == (CHAR *)0 || s2 == (char *)0)
		return(0);
	while (*s1 && *s2 && *s1 == *s2)
		s1++, s2++;
	return (*s1 == 0 && *s2 == 0);
}

Eq(s1, s2)
CHAR *s1;
CHAR *s2;
{
	if (s1 == (CHAR *)0 || s2 == (CHAR *)0)
		return(0);
	while (*s1 && *s2 && *s1 == *s2)
		s1++, s2++;
	return (*s1 == 0 && *s2 == 0);
}

               /* MAXCH increased from 1024 --> NCARGS; 
		                                     DSDe415086
						     DSDe415398
						     DSDe415879  */

#define MAXCH 	NCARGS 	/* size of the static array for to_char to_short*/
			/* the string to be converted cannot exceed NCARGS bytes */

CHAR *to_short(s)
register char *s;
{
	static CHAR buf[MAXCH+1];  /* buffer for converted CHAR string */
				  /* we re-use this space, so caller has */
				  /* to explicitly save the string */
	register CHAR *n;

	if (s) {
		n = buf;
#ifndef NLS16
		while (*n++ = *s++ & 0377)
		;
#else
		/* properly convert kanji strings */
#ifdef EUC
		while (*n++ = _CHARADV(s) & TRIM)
		;
#else  EUC
		while (*n++ = CHARADV(s) & TRIM)
		;
#endif EUC

#endif
		if (n > &buf[MAXCH+1])
			error((catgets(nlmsg_fd,NL_SETN,3, "to_short ran out of space")));
		return(buf);
	}
	else return ((CHAR *) 0);
}


char *to_char(s)
register CHAR *s;
{
	static char buf[MAXCH*2+1];	/* buffer for converted char string */
	register char *n;

	if (s) {
		n = buf;
#ifndef NLS16
		while (*n++ = *s++ ) 
			;
#else
		/* properly convert kanji strings */
		while (*s) {
			if ((*s&TRIM) > 0377) {
				*n++ = (*s >> 8) | 0200; 
				*n++ = *s++ & 0377; 
			}
			else *n++ = *s++ & TRIM;
		}
#endif
		*n = 0;		/* null terminate */
		if (n > &buf[MAXCH*2])
			error ((catgets(nlmsg_fd,NL_SETN,4, "Exceeded to_char space\n")));

#ifdef STRING_DEBUG
/*  Debug:  Used to trace values.
*/
  printf ("to_char (1): pid: %d, Translated string: %s\n", getpid (), buf);
#endif
		return(buf);
	}
	else return ((char *) 0);
}

char *
savebyte(s)
register char *s;
{
	register char *n;

	if (s == 0)
		return((char *) 0);
	strcpy(n = (char *)calloc(1, (unsigned)((strlen(s)+1))), s);
	return(n);
}

CHAR **blk_to_short(c)
register char **c;
{
	register int len = 0;
	register CHAR **newv;
	CHAR **onewv;
	char **c1;

	c1 = c;
	while (*c1++) len++;
	newv = (CHAR **) calloc((unsigned) (len + 1), sizeof (CHAR **));
	onewv = newv;
	while (*c) {
		*newv++ = savestr(to_short(*c));
		c++;
	}
	return (onewv);
}

char **blk_to_char(c)
register CHAR **c;
{
	register int len = blklen(c) + 1;
	register char **newv;
	char **onewv;

	newv = (char **) calloc((unsigned)len, sizeof (char **));
	onewv = newv;
	while (*c) {
		*newv++ = savebyte(to_char(*c));
		c++;
	}
	return (onewv);
}

Atoi(p)
register CHAR *p;
{
	register n, c, neg = 0;

	if (!digit(c = *p)) {
		while (c == ' ' || c == ALT_SP)
			c = *++p;
		switch (c) {
		case '-':
			neg++;
		case '+':
			c = *++p;
		}
		if (!digit(c))
			return(0);
	}
	for (n = '0' - c; digit(c = *++p) ;) {
		n *= 10;
		n += '0' - c;
	}
	return (neg ? n : -n);
}

Any(c, s)
	register int c;
	register CHAR *s;
{

	while (*s)
          {

#ifdef TRACE_DEBUG
/*  Debug:  Used to trace variables.
*/
  printf ("Any (1): pid: %d, Pointer: %c\n", getpid (), *s);
#endif
		if (*s++ == c)
			return(1);
	  }
	return(0);
}

char **					/* NLS: copy blocks of bytes */
b_blkcpy(oav, bv)
	char **oav;
	register char **bv;
{
	register char **av = oav;

	while (*av++ = *bv++)
		continue;
	return (oav);
}

CHAR *
Strspl(cp, dp)
	register CHAR *cp, *dp;
{
	register CHAR *ep = calloc(1, (unsigned)((Strlen(cp)+Strlen(dp)+1)*sizeof(CHAR)));

	Strcpy(ep, cp);
	Strcat(ep, dp);
	return (ep);
}

b_copy(to, from, size)			/* NLS: copies bytes */
	register char *to, *from;
	register int size;
{

	if (size)
		do
			*to++ = *from++;
		while (--size != 0);
}

/*
 * This routine is like blkspl_spare, but is used when it is known
 * that there are no spare entries in the array of pointers.
 */
char **
blkspl_spare2(up, vp)
	register char **up, **vp;
{
	register char **wp;
	char **savewp, **saveup = up;

	savewp = wp = (char **) calloc((unsigned) (blklen(up) + blklen(vp) 
					+ MAX_SPARE + 1), sizeof (char **));
	while (*wp = *up++) wp++;
	while (*wp++ = *vp++);
	xfree(saveup);
	return (savewp);
}
#endif
