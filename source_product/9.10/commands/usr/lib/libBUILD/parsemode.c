/* @(#) $Revision: 70.1 $ */

/*
 * parse_mode() -- parse a symbolic mode of the form:
 *
 *    octal-value
 * or
 *    [ who ] op [ permission ] [op [ permission ] ]
 *
 * multiple modes may be supplied, seperated by commas.
 *
 * who is one of
 *   u (user)
 *   g (group)
 *   o (other)
 *   a (all)
 *
 * op is one of
 *   + (grant permission)
 *   - (revoke permission)
 *   = (set permission)
 *
 * permission is one of
 *   r read
 *   w write
 *   x execute
 *   X selective execute/search
 *   s setuid or setgid
 *   t sticky
 *   H CDF bit (diskless)
 *   u same permission as user
 *   g same permissions as group
 *   o same permissions as other
 *
 * As the mode is parsed, a 'current mode' is calculated.  The 'u',
 * 'g' and 'o' permissions are taken from the 'current mode', not the
 * starting mode.  You could really think of it either way, but the
 * chmod(1) code does it based on the 'current mode'.
 *
 * AW:
 * 22 Jan 1992: Changed for POSIX.2/D11.2 Compliance.
 */

#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>

#define	USRBIT	04		/* user  bit */
#define	GRPBIT	02		/* group bit */
#define	OTHBIT	01		/* other bit */
#define	ALL	07		/* all bits set for ugo */

#define	SBIT	06000		/* set uid */
#if defined(DUX) || defined(DISKLESS)
#define	HBIT	04000		/* cdf */
#endif
#define	TBIT	01000		/* sticky bit */
#define	RBIT	00444		/* read permissions */
#define	WBIT	00222		/* write permissions */
#define	EBIT	00111		/* execute permissions */
#define	XBIT	010000		/* special bit - cannot be used as an
				 * octal mode value. (internal to this command)
				 */
#define ALLBITS	07777		/* all bits */
#define	MAXOPS	10

#if defined(DUX) || defined(DISKLESS)
static char	*permlist = "rwxsXHt";		/* all valid permlist */
#else
static char	*permlist = "rwxsXt";		/* all valid permlist */
#endif
static char	*oplist = "+-=";		/* all valid operators */
static int	intwho = 0;			/* for who list */
static char	obuf[MAXOPS];			/* temporary buffer */
static mode_t	user_mask;			/* umask of user */


int
parse_mode(pstr, start_mode)
char *pstr;
int start_mode;
{
char	c;
int	i, j, k;
int	result = 0;
int	tmode = 0, twho;	/* temp storage for mode and who */
int	intperm = 0;		/* temp storage for permissions */
int	state = 0;
int	use_mask;		/* use umask ? */
int	cdf_allow = 1;		/* assume 'H' is allowed */

#if defined(DUX) || defined(DISKLESS)
	/* if it is a file and not a directory, disallow cdf setting.
	 * This logic is required to ensure that parse_mode(m,0) passes through
	 * We set cdf_allow to:
	 *	  0 -- 'H' may not be turned on.
	 *	  1 -- they may turn on 'H'
	 */
	if((start_mode & S_IFMT) != 0 && (start_mode & S_IFDIR) != S_IFDIR)
		cdf_allow = 0;
#endif

	result = start_mode & ALLBITS;		/* start from this mode */

	/* check for absolute mode  (in octal) */
	errno = 0;
	if(*pstr >= '0' && *pstr <= '7') {
		char *p;
		if(!(result = strtol(pstr, &p, 8)))
			if(errno) return(-1);

		/* comma or null can follow an octal mode */
		if(*p != '\0' && *p != ',') {
			errno = EINVAL;
			return(-1);
		}

		/* check whether number is in range */
		if((unsigned)result > ALLBITS) {
			errno = ERANGE;
			return(-1);
		}

		/* if there is something following the mode (eg: 555,u+s)
		 * continue operation, else return here
		 */
		result |= start_mode & ~ALLBITS;
		if(*p == '\0') return(result);
		pstr = p+1;
	}

	/* get umask, this should be used instead of ALL (ugo)
	 * when wholist is not defined 
	 * -- POSIX.2 */
	user_mask = umask(0);		/* get previous mask */
	umask(user_mask);		/* set it back */

	/* parse remaining arguments */
	state = 0;			/* initial state */
	while(state != 3) {
		switch(state) {
			case 0:	/* get wholist */
				intwho = ALL;
				use_mask = user_mask;	/*'who' not specified */
				j = 1;
				c = *pstr++;		/* get wholist */
				if(c == 0) {		/* if completed, exit */
					state = 3;
					break;
				}
				while(!valid(c, oplist)) { /* do until op */
					if (j) intwho = 0; /* initialize */
					j = 0;
					if(!who(c)) return(-1);	/* wholist? */
					c = *pstr++;	/* get next */
					use_mask = 0;	/* 'who' is specified */
				}
				state = 1;		/* get ops */
				break;
			case 1:	/* get operation - must be present */
				i = 0;		/* number of operations */
				while(valid(c, oplist)) {
				   /* special case '=u|g|o' */
				   if(c == '=' && valid(*pstr, "ugo")) {
					/* extract bits to be copied */
					switch(*pstr++) {
						case 'u': /* user bits 0700 */
						tmode = (result & S_IRWXU) >> 6;
						break;
						case 'g': /* group bits 0070 */
						tmode = (result & S_IRWXG) >> 3;
						break;
						case 'o': /* other bits 0007 */
						tmode = (result & S_IRWXO);
						break;
					}
					/* now copy bits depending on wholist */
					twho = intwho;
					k = S_IRWXO;	/* 07 */
					for(j=0; j<3; j++) {
					   /* copy bits for that "who" alone */
					   if(twho & 01)
						result = (result & ~k) | tmode;
					   tmode <<= 3;
					   twho >>= 1;
					   k <<=3;
					}
				   } else {
					obuf[i++] = c; /* just add the ops */
				   	if(i == MAXOPS)
						return(-1);/* max allowed */
				   }
				   c = *pstr++;
				}
				if(who(c)) return(-1);	/* no more "who"
							 * possible */
				state = 2;		/* get action */
				break;
			case 2:	/* get action */
				/* collect all permissions */
				intperm = 0;
				while(valid(c, permlist)) {
					j = perm(c, intwho);

					/* check if it is a directory or if
					 * atleast one exec/search permission
					 * is present if 'X' option is used
					 */
					if(j & XBIT && (start_mode & S_IFDIR
					   || start_mode & EBIT))
						j = perm('x', intwho);
#if defined(DUX) || defined(DISKLESS)
					/* if 'H' option: check if file is a
					 * directory
					 */
					if(c == 'H' && !cdf_allow)
						return(-1);
#endif
					intperm |= j;
					c = *pstr++;
				}
				intperm &= ~use_mask;	/* if who not specified,
							 * apply umask
							 */
				/* for each operation defined do... */
				for(j = 0; j < i; j++) {
				   switch(obuf[j]) {
				   	case '=': /* first clear who bits */
						result &= ~(orbits(ALL,intwho));
						result |= intperm;
						break;
					case '-': /* remove permissions */
						result &= ~intperm;
						break;
					case '+': /* add permissions */
						result |= intperm;
						break;
				   }
				}
				state = 1;
				if(c == ',') state = 0;		/* continue */
				else if(c == '\0') state = 3;	/* completed */
				break;
		}
	}
	return(result);
}

static
valid(c, s)
char c;
char *s;
{
	/* check if it is a valid character belonging to "s" */
	while(*s) if(c == *s++) return(1);
	return(0);
}

static
who(c)
char c;
{
	/* check if it is a valid "wholist" */
	switch(c) {
		case 'u': intwho |= USRBIT;
			  return(1);
		case 'g': intwho |= GRPBIT;
			  return(1);
		case 'o': intwho |= OTHBIT;
			  return(1);
		case 'a': intwho = ALL;
			  return(1);
		default:
			return(0);
	}
}

static
perm(c, who)
char c;
int who;
{
int s = 0;
	/* return permission bits corresponding to perm */
	switch(c) {
		case 'r': return(orbits(RBIT, who));
		case 'w': return(orbits(WBIT, who));
		case 'x': return(orbits(EBIT, who));
		case 'X': return(XBIT);
		case 's': if(who & USRBIT) s = S_ISUID;
			  if(who & GRPBIT) s |= S_ISGID;
			  return(s);
		case 't': return(TBIT);
#if defined(DUX) || defined(DISKLESS)
		case 'H': return(HBIT);
#endif
		default:  return(0);
	}

}

static
orbits(mode, who)
int mode, who;
{
int res = 0, i;
	/* OR permission bits to relevant "who" area */
	mode &= ALL;		/* only 3 rwx bits are valid */
	who &= ALL;
	for(i=0; i<3; i++) {
	   if(who & 01) res |= mode;
	   mode <<= 3;
	   who >>= 1;
	}
	return(res);
}

