#ifdef _NAMESPACE_CLEAN
#define fnmatch _fnmatch
#define fflush __fflush
#define printf _printf
#define strchr _strchr
#define strcmp _strcmp
#define strlen _strlen
#define strncpy _strncpy
#   ifdef _ANSIC_CLEAN
#define malloc _malloc
#define realloc _realloc
#define free _free
#   endif /* _ANSIC_CLEAN */
#endif /* _NAMESPACE_CLEAN */

#include <sys/types.h>
#include <sys/param.h>
#include <string.h>
#include <stdio.h>
#include <setlocale.h>
#include <regex.h>
#ifdef NLS
#include <nl_ctype.h>
#endif /* NLS */

#define _XPG4
#include <fnmatch.h>

/* variable used to make end point of match available in csh */
u_char 	*_uae_index;

/**** temporary until unistd.h has right values */
#ifndef FNM_PERIOD
#define FNM_PERIOD _FNM_PERIOD
#endif /* FNM_PERIOD */

/*  Constants and structures used by bracketcomp() */
#define	MATCH_LIST	0100	/* match any char/collation element in list */
#define	NON_MATCH_LIST	0110	/* match any collation element not in list */
#define	END_RE		0010	/* end of RE program	*/

/*  Possible returns from bracketcomp() */
#define MATCH_1		0
#define MATCH_2		1
#define MATCH_1OR2	2

/* Multibyte character test macro */
#define MULTI_BYTE	(_nl_mb_collate)

/* Initial size of RE buffer for bracket expr's. */
#define REG_BUF_SIZ	512

struct logical_char {
	unsigned int	c1,
			c2,
			flag,
			class;
};

static u_char	*ctype_funcs[] = {
				(u_char *) "alpha",
				(u_char *) "upper",
				(u_char *) "lower",
				(u_char *) "digit",
				(u_char *) "xdigit",
				(u_char *) "alnum",
				(u_char *) "space",
				(u_char *) "punct",
				(u_char *) "print",
				(u_char *) "graph",
				(u_char *) "cntrl",
				(u_char *) "ascii"
			};

static int	bufsize;			/* size of RE buffer */
/*  End of bracketcomp() declarations */


static	u_char		*brktxp;	/* storage for bracket exp */
static	u_char		*brktxp_end;
static	u_char		*p,*p2;
static	int		fnm_err;
static	int 	pnm = 0; 
static	int	per = 0;
static	int	noesc = 0;		/* FNM_NOESCAPE is off */

/* global used in conjunction with FNM_PERIOD flag to "remember" if
 * the last character in string was a '/' */
static int    lastslash = 0;
static u_char *next();

#define Malloc malloc
#define Realloc realloc
#define Free free


#ifdef _NAMESPACE_CLEAN
#undef fnmatch
#pragma _HP_SECONDARY_DEF _fnmatch fnmatch
#define fnmatch _fnmatch
#endif /* _NAMESPACE_CLEAN */

int
fnmatch(pattern,string,flag)	/* match pattern to string */
u_char *pattern;
u_char *string;
int flag;
{
	_uae_index = 0;             /* default value */
	per = flag & FNM_PERIOD;
	pnm = flag & FNM_PATHNAME;
        noesc = flag & FNM_NOESCAPE;

	/* Check leading conditions immediately */
	if (pnm && (*string == '/' && *pattern != '/'))
		return(1);
	if (per && *string == '.' && *pattern != '.')
		return(1);
	else
		return(matcher(pattern,string,flag));
}

static int 
matcher(pattern,string,flag)
u_char *pattern, *string;
int flag;
{

	u_char		*s,*q,*tmppat,*tmpstr;
	u_char		onechar[3];
	int		i,good,chr2mtch;
	int 		two_1;
	int		first; /* used in calls to next because of recursion */
	s = string;
	p = pattern;

	/* Check leading conditions immediately */
	if (pnm && (*s == '/' && *p != '/'))
		return(1);
	if (per && *s == '.' && pnm && lastslash && *p != '.')
		return(1);

	while (1) {
		if (*s == '\0' && *p == '\0') 
			return(0);
		if (*p == '\0') 
			if( flag & _FNM_UAE) {
			    _uae_index = s;
			    return(0);
			}
			else
			    return(1);

		switch(*p) {
		case '*':	/* match any string */
			/* walk past consecutive stars */
			while(*++p == '*');
			tmppat = p;
			tmpstr = 0;
			first = 1;
			while((tmpstr = next(s,tmppat,tmpstr,&first)) != (u_char *)0){
				if (matcher(tmppat,tmpstr,flag)==0)
					return(0);
			}
			return(1);
		case '[':	/* match set of characters */
			if(pnm && *s == '/')
				return(1);
			if (per && *s == '.' && (pnm && lastslash ))
				return(1);

			/* return > MATCH_1OR2 indicates a pattern problem */
			if((chr2mtch = bracketcomp()) >MATCH_1OR2)
				return(-1);
			
			if(*s == '/')
			    lastslash = 1;
			else 
			    lastslash = 0;

			onechar[0]= *s;
			onechar[1]= *(s+1);
			onechar[2]= '\0';
			switch(i = bracketexec(onechar)) {
			    
			    /* No match, return fail */
			    case 0: return(1);

			    /*  match 1 or 2, try rest of string
				with 1 */
			    case 3: if (matcher(p,s+1,flag)==0)
				        return(0);

			    /* fell through to here or matched
			       exactly 2 chars, increment s once..  */
			    case 2: s++;
			    
			    /* Matched 1 char or fell through,
			       incr. s (again, if fall-through) */
			    case 1: s++;
#ifdef NLS16
				    if(FIRSTof2(*(s-1)) && SECof2(*s))
					s++;
#endif /* NLS16 */
			}
			break;

		case '?':	/* match any character */
			if(pnm && *s == '/')
				return(1);
			if (per && *s == '.' && (pnm && lastslash))
				return(1);
			p++;
			if(*s == '/')
			    lastslash = 1;
			else 
			    lastslash = 0;
			    
#ifdef NLS
			if (FIRSTof2(*s))
				s++;
#endif /* NLS */
			if(*s)
				s++;
			else
				return(1);
			break;
		case '\\':	/* quoted character */
			if ((!noesc) && (*++p == '\0')) return(1);

                        /* else fall through... */

		default:	/* match character */
			if (*p == *s) {
			    if(*s == '/')
				lastslash = 1;
			    else 
				lastslash = 0;
#ifdef NLS16
			    if (FIRSTof2(*p)) {
				p++;
				s++;
			    }
			    if (*p != *s)
				return(1);
#endif
			    p++;
			    s++;
			} else {
				return(1);
			}
			break;
		}
	}
}



static int
bracketcomp()
{
	register		c;

	register u_char	*ep;
	register u_char	*endbuf;
	register int	not_multibyte = (__nl_char_size == 1);

	int		i;
	struct logical_char	lchar, pchar;
	int		c2, range, no_prev_range, two_to_1;
	u_char		*psave;
	int		error;
	int		chrmatch = 0;    /* return value */
	int	offset;
	int 	bracket_seen;

#ifdef DEBUG
	printf("\nIn bracketcomp():\n");
	printf("\tp = %s\n",p);
#endif
	lchar.flag = 0;	        /* initialize to single 1 byte character */
	/*
	 * If space for a brktxp has not been malloc'd, do it.
	 */
	if(brktxp == NULL){
	    if ((brktxp = ep = (u_char *)Malloc(REG_BUF_SIZ))==NULL)
		return(REG_ESPACE);
	}
	/*
	 * otherwise, reuse same space.
	 */
	else
	    ep = brktxp;

	bufsize = REG_BUF_SIZ;
	endbuf = ep + REG_BUF_SIZ - 1;


	*ep++ = MATCH_LIST;

	/* eat the opening '['. If that's all there is, return immediately */
	if(*++p == NULL)
	    return(REG_EBRACK);

	lchar.c1 = *p++;
	if (lchar.c1 == '!') {
	        *(ep-1) = NON_MATCH_LIST;
		lchar.c1 = *p++;
	}

	if (ep >= endbuf)
	        return(REG_ESPACE);

	range = 0;

	do {

#ifdef DEBUG
	printf("\tIn loop: lchar.c1 = %c\n",lchar.c1);
#endif
		/* get one logical character */


		if (FIRSTof2(lchar.c1) && SECof2(*p)) {	/* kanji? */
			lchar.c2 = *p++;
			lchar.flag = 1;				/* kanji */
			lchar.class = 1;			/* as itself */
			goto have_logical_char;
		} else

		if (lchar.c1 == '[' && ((c = *p) == '.' || c == ':' || c == '=')) {

			/*
			 * Eat the [.:=] and save the start of the extended syntax
			 * argument.
			 */
			psave = ++p;

			/*
			 * scan for closing expression (".]", ":]", "=]")
			 */
			bracket_seen = 0;
			while (!((c2 = *p++) == '\0' || (c2 == ']' && p-1 != psave))){
				if(c2 == ']')
				    bracket_seen++;

				if (FIRSTof2(c2) && SECof2(*p))
					p++;
			}
			if (c2 == '\0' && !bracket_seen)
				return(REG_EBRACK);	
			
			if(*(p-2) != c || p-1 == psave){
			    p = psave-1;
			    c = 'x';
			}

/*			p++;			 eat the closing ']' */

			/* process each type of new syntax as appropriate */
			lchar.class = 1;	/* presume a collating symbol */
			switch (c) {

				/*
				 * Equivalence Class
				 */
			case '=':				
				/* equivalence class code is = 2 */
				lchar.class++;	

				/*
				 * Collating Symbol
				 */
			case '.':
				/*
				 * both coll sym & equiv class need a coll ele argument
				 * hence the common code
				 */
				lchar.c1 = *psave;
				switch (p - psave - 2) {

				case 0:
					/* found "[..]" or "[==]" */
					return(REG_ECOLLATE);

				case 1:
					/* found a single one-byte character */
					lchar.flag = 0;			
					/* error if not a collating element */
					if (!_collxfrm(lchar.c1, 0, 0, (int *)0))
						return(REG_ECOLLATE);		
					break;

				case 2:
					/* found a 2-to-1 or a kanji or 2 bytes of junk */
					lchar.c2 = *(psave+1);
					/* is 1st char possibly the 1st char of a 2-to-1? */
					if (_nl_map21) {
						/* check if a 2-to-1 collating element */
						if (!_collxfrm(lchar.c1, lchar.c2, 0, &two_to_1) || !two_to_1)
							return(REG_ECOLLATE);	/* not a 2-to-1 collating element */
						else
							chrmatch = MATCH_2;

						lchar.flag = 2;			/* 2-to-1 */
					} else if (FIRSTof2(lchar.c1) && SECof2(lchar.c2)) {
						/* error if not a collating element */
						if (!_collxfrm(lchar.c1<<8 | lchar.c2, 0, 0, (int *)0))
							return(REG_ECOLLATE);	/* not a collating element */
						lchar.flag = 1;			/* kanji */
					} else
						return(REG_ECOLLATE);		/* found "[.ju.]" or "[=ju=]" */
					break;

				default:
					return(REG_ECOLLATE);			/* found "[.junk.]" or "[=junk=]" */
				}
				break;

			/*
			 * Character Class
			 */
			case ':':
				/* character class */
				lchar.class = 3;				

				if (p - psave - 2 == 0)
					/* found "[::]" */
					return(REG_ECTYPE);			

				/*
				 * Match name in RE against supported ctype functions
				 */
				*(p-2) = '\0';					/* make the name a string */
				i = 0;
				while ((i < sizeof(ctype_funcs)/sizeof(u_char *)) && strcmp((u_char *)psave, (u_char *)ctype_funcs[i]))
					i++;
				*(p-2) = ':';					/* restore RE */

				if (i >= sizeof(ctype_funcs)/sizeof(u_char *))
					return(REG_ECTYPE);			/* found "[:garbage:]" */
				else {
					lchar.flag = i;				/* ctype macro */
				}

				break;
			}

			if(c != 'x')
			    goto have_logical_char;

		} else {
			/*
			 * Got just a character by itself
			 */

			/*
			 * For PMN a backslash quotes characters within MATCH_LIST
			 */
			if ((!noesc) && (lchar.c1 == '\\')) lchar.c1 = *p++;

			lchar.flag = 0;			/* single char */
			lchar.class = 1;		/* as itself */
		}

		/*
		 * Now we have one logical character, figure out what
		 * to do with it.
		 */

have_logical_char:
		/* character class? */
		if (lchar.class == 3) {						

			if (range)
				/* ctype can't be range endpoint */
				return(REG_ERANGE);				
			/*
			 * generate compiled subexpression for a ctype class
			 */
			ep += gen_ctype(&lchar, ep, endbuf, &error);
			if (error)
				return(error);
			/* dash following? */
			if (*p == '-') {					
				p++;
				if (*p != ']')
					/* ctype can't be range endpoint */
					return(REG_ERANGE);			
				else {
					/* output dash as single match */
					lchar.c1 = '-';
					lchar.flag = 0;
					lchar.class = 1;
					ep += gen_single_match(&lchar, ep, endbuf, &error);
					if (error)
						return(error);
				}
			}
		}

		else if (range) {

			/*
			 * generate compiled subexpression for a range
			 */
			if (_nl_map21)
				chrmatch = MATCH_1OR2;
			ep += gen_range(&pchar, &lchar, ep, endbuf, &error);
			if (error)
				return(error);
			range = 0;
			if (*p == '-') {					/* dash following? */
				p++;
				pchar = lchar;
				range++;
				no_prev_range = 0;
			}
		}

		else if (*p == '-') {						/* dash following? */
			p++;
			pchar = lchar;
			range++;
			no_prev_range++;
		}

		else {
			/*
			 * generate compiled subexpression for a single match
			 */
			ep += gen_single_match(&lchar, ep, endbuf, &error);
			if (error)
				return(error);
		}

	} while ((lchar.c1 = *p++) != ']' && lchar.c1 != NULL && ep < endbuf);

	if (ep >= endbuf)
	        return(REG_ESPACE);
	if (lchar.c1 == NULL)
		return(REG_EBRACK);

	if (range) {
		if (no_prev_range) {
			/* output last char as single match */
			ep += gen_single_match(&pchar, ep, endbuf, &error);
			if (error)
				return (error);
		}

		/* output dash as single match */
		lchar.c1 = '-';
		lchar.flag = 0;
		lchar.class = 1;
		ep += gen_single_match(&lchar, ep, endbuf, &error);
		if (error)
			return (error);
	}

	*ep++ = END_RE;		/* add final token */
	brktxp_end	= ep;	/* ptr to end of RE + 1 byte */
	return(chrmatch);		/* and return `success' */

}


/*
 *  Generate compiled subexpression for a range
 */
static gen_range(bchar, echar, ep, endbuf, error)
const struct logical_char	*bchar, *echar;
register u_char		*ep;
const u_char		*endbuf;
int				*error;
{

	register unsigned int c1, c2;
	register unsigned xfrm1, xfrm2;
	int 	offset;

	*error = 0;

	if (ep >= endbuf-7) {	
		offset = ep - brktxp;
		if((brktxp = (u_char *)Realloc(brktxp,bufsize+REG_BUF_SIZ)) == 0){
			*error = REG_ESPACE;
			return(0);
		}
		ep = brktxp + offset;
		bufsize = bufsize + REG_BUF_SIZ;
	}

	/* store flag for this range expression */
	*ep++ = 0100;

	/* 7/8-bit language with machine collation */
	if (!_nl_collate_on) {
		if (bchar->c1 > echar->c1) {
			*error = REG_ERANGE;	/* 1st endpoint must be <= 2nd endpoint */
			return (0);
		}
		*ep++ = (u_char)(bchar->c1);
		*ep++ = (u_char)(echar->c1);
		return (3);
	}

	/* 8-bit NLS collation or 16-bit machine collation */

	/* get collation transformation of 1st range endpoint */
	c1 = bchar->c1;
	if (bchar->flag == 1)
		c1 = c1 << 8 | bchar->c2;
	c2 = (bchar->flag == 2) ? bchar->c2 : 0;
	xfrm1 = _collxfrm(c1, c2, (bchar->class == 2) ? -1 : 0, (int *)0);
	if (xfrm1 == 0) {
		*error = REG_ECOLLATE;		/* range endpoint non-collating */
		return (0);
	}

	/* store 1st range endpoint */
	*ep++ = (u_char)(xfrm1 >> 16);
	*ep++ = (u_char)(xfrm1 >> 8);
	*ep++ = (u_char)(xfrm1);

	/* get collation transformation of 2nd range endpoint */
	c1 = echar->c1;
	if (echar->flag == 1)
		c1 = c1 << 8 | echar->c2;
	c2 = (echar->flag == 2) ? echar->c2 : 0;
	xfrm2 = _collxfrm(c1, c2, (echar->class == 2) ? 1 : 0, (int *)0);
	if (xfrm2 == 0) {
		*error = REG_ECOLLATE;		/* range endpoint non-collating */
		return (0);
	}

	/* store 2nd range endpoint */
	*ep++ = (u_char)(xfrm2 >> 16);
	*ep++ = (u_char)(xfrm2 >> 8);
	*ep++ = (u_char)(xfrm2);

	if (xfrm1 > xfrm2) {
		*error = REG_ERANGE;		/* 1st endpoint must be <= 2nd endpoint */
		return (0);
	}

	return (7);
}



/*
 *  Generate compiled subexpression for a single match
 */
static gen_single_match(lchar, ep, endbuf, error)
const struct logical_char	*lchar;
register u_char		*ep;
const u_char		*endbuf;
int				*error;
{
	u_char *orig_ep = ep;
	int orig_off = ep - brktxp;
	int	offset;

	*error = 0;

	if (lchar->class == 2) {
		/* output equiv class as a range */
		return gen_range(lchar, lchar, ep, endbuf, error);
	} else {
		/* output: char */
		if (ep >= endbuf-3) {
		    offset = ep - brktxp;
		    if((brktxp = (u_char *)Realloc(brktxp,bufsize+REG_BUF_SIZ)) == 0){
			*error = REG_ESPACE;
		 	    return(0);
		    }
		    ep = brktxp + offset;
		    bufsize = bufsize + REG_BUF_SIZ;
		    orig_ep = brktxp + orig_off;
		}

		*ep++ = lchar->flag | 040;
		*ep++ = lchar->c1;
		if (lchar->flag) 		/* if kanji or 2-to-1 */
			*ep++ = lchar->c2;
	}
	return (ep - orig_ep);
}



/*
 *  Generate compiled subexpression for a ctype class
 */
static gen_ctype(lchar, ep, endbuf, error)
const struct logical_char	*lchar;
register u_char			*ep;
const u_char			*endbuf;
int				*error;
{
	int	offset;
	*error = 0;

	if (ep >= endbuf-1) {
		    offset = ep - brktxp;
		    if((brktxp = (u_char *)Realloc(brktxp,bufsize+REG_BUF_SIZ)) == 0){
			*error = REG_ESPACE;
		 	return(0);
		    }
		    ep = brktxp + offset;
		    bufsize = bufsize + REG_BUF_SIZ;
	}

	*ep = lchar->flag | 020;
	return (1);
}
/************************************
 *  next()
 *  Returns a pointer to the next substring to try to match with a '*'.
 *  The previous end point is fed back as an argument to provide an
 *  end-point.
 *  Unless the next character in the pattern is an opening bracket or
 *  a '?' or a backslash, we can save recursive calls by only returning
 *  substrings that start with the same character as the next one in the
 *  pattern.
 *  A complication is that in a multi-byte language environment, we
 *  can't reliably walk backwards through a string, so all processing
 *  has to proceed from the beginning.
 *
 *  Jim Stratton, SR#5003-009811 (DTS#FSDlj09458) 7 Oct 91:
 *  A check for backslash, indicating an escaped character, was included
 *  in the decision to scan the substring backward.  The test for a match
 *  while backing up wasn't taking into consideration escaped characters
 *  that were preceded by a `*'.
 ***************************************/
static u_char *
next(str,pat,previous,first)
u_char 	*str;
u_char 	*pat;
u_char 	*previous;
int	*first;
{
	static u_char	*tmpptr, *p;
	u_char		*poss_match;
	static int 	check_all = 0;

        /*  the whole substring has been tried or str is empty */
	if(previous == str || (*str == NULL && *pat != NULL)) 	
	    return(0);

	if(*first){  		/* First call */
	    *first = 0;
	    /* if the next char in the pattern is '\', '[' or '?', all
	     * possibilities have to be checked.  If check_all has
	     * been set in a previous call, leave it set. */
	    if( check_all      || ((!noesc) && (*pat == '\\')) ||
                (*pat == '[') || (*pat == '?'))
		check_all = 1;
	    else
		check_all = 0;

	    /* find end as far as possible down the string 
	     * This is only to the next '/' if PNM_PATHNAME is set */
	    if(MULTI_BYTE){
		p = str;
		if(pnm){
		    /* if no '/', advance to end */
		    while((CHARADV(p)) && *p != '/');
		    tmpptr = p;
		    if(!tmpptr){
			/* if PERIOD flag set and first character in str
			 * is a '.' AND last character was a '/', don't
			 * move down str at all. */
			if(per && *str == '.' && lastslash)
			    tmpptr = str;
		        else
			    tmpptr = str + strlen(str);
		    }
		}
		else
		    tmpptr = str + strlen(str);
	    }
	    else{ 	/* not MULTI_BYTE */
	        if (pnm){
		    if((tmpptr = (u_char *)strchr(str,'/')) == 0){ 
		        if (per && *str == '.' && lastslash)
			    tmpptr = str;
	                else
		            tmpptr = str + strlen(str);
		    }
	        }
		else 
		    tmpptr = str + strlen(str);
	    }
	    previous = tmpptr;
	    if(check_all || *pat == *previous)
		return(previous);
	}

	poss_match = 0;
	if(!check_all){
	    if(MULTI_BYTE){  	/* may be 2-byte chars */
		for(tmpptr = str; tmpptr < previous;){
		    if(CHARAT(tmpptr) == CHARAT(pat))
			poss_match = tmpptr;
		    CHARADV(tmpptr);
		}
	    }
	     /* no multi-byte chars , can go backwards, faster */
	    else{ 	/* no multi-byte chars */
		--previous;
		for(tmpptr = previous; tmpptr >= str;){
		    if(*tmpptr == *pat){
			poss_match = tmpptr;
			break;
		    }
		    --tmpptr;
		}
	    }
	}        /*  check_all, must consider every possible substr. */
	else{
	    if(MULTI_BYTE){
		for(tmpptr = str; tmpptr < previous-2;)
		    CHARADV(tmpptr);
		if(FIRSTof2(*tmpptr))
		    poss_match = tmpptr;
		else{
		    /* Special check needed in case str is one byte */
		    if(previous - tmpptr > 1)
			poss_match = ++tmpptr;
		    else
			poss_match = tmpptr;
		}
	    }
	    else 	/* no MB, back up one byte from last */
		poss_match = --previous;

	}
	/* make sure we haven't backed out to the beginning */
	if(poss_match < str)
	    poss_match = 0;
	
	return(poss_match);
}

static int
bracketexec(string)
u_char	*string;
{
	u_char	*ep = brktxp;
	int	not_multibyte = (__nl_char_size == 1);
	int	negate = 0;
	u_char	c1,c2;
	unsigned int x1, x2, e1, e2, flag;
	int match_1_char, match_2_to_1, two_to_1;
	

#ifdef DEBUG
	printf("\nIn bracketexec:\n");
	printf("\tstring = %s,  first token = %o\n",string,*ep);
#endif
	if(*ep++ == NON_MATCH_LIST)
		negate = 1;

	c1 = string[0];
	c2 = string[1];

	if (c1 == 0)
	        return 0;
	
	x1 = match_1_char = match_2_to_1 = 0;
	if (_nl_map21 && (_pritab[c1] & 0x40))
		x2 = _collxfrm(c1, c2, 0, &two_to_1);
	else
		x2 = two_to_1 = 0;
	
	while (*ep != END_RE && *ep != 0 && !(match_1_char && 
	       (!two_to_1 || match_2_to_1))) {
	
		/* extract subexpression to be matched */
		flag = *ep++;
	
		/* if subexpression type is single */
		if (flag & 040) {
			e1 = *ep++;
			if (!(flag & 3))
				match_1_char = (c1 == e1);		/* single char match wanted */
			else if (flag & 1) {
				e1 = (e1 << 8) | *ep++;			/* kanji match wanted */
				match_1_char = (((c1 << 8) | c2) == e1);
			} else {
				e2 = *ep++;				/* 2-to-1 match wanted */
				match_2_to_1 = (c1 == e1 && c2 == e2);
			}
		}
	
		/* else if subexpression type is ctype */
		else if (flag & 020) {
			if (c1 < 256)					/* no ctype for kanji */
			    switch (flag & 017) {

				case 0:    match_1_char = isalpha(c1); break;
				case 1:    match_1_char = isupper(c1); break;
				case 2:    match_1_char = islower(c1); break;
				case 3:    match_1_char = isdigit(c1); break;
				case 4:    match_1_char = isxdigit(c1); break;
				case 5:    match_1_char = isalnum(c1); break;
				case 6:    match_1_char = isspace(c1); break;
				case 7:    match_1_char = ispunct(c1); break;
				case 8:    match_1_char = isprint(c1); break;
				case 9:    match_1_char = isgraph(c1); break;
				case 10:    match_1_char = iscntrl(c1); break;
				case 11:    match_1_char = isascii(c1); break;
			    }
		}
	
		/* else if subexpression type is range */
		else {
			/* 7/8-bit language with machine collation */
			if (!_nl_collate_on) {
				e1 = *ep++;
				e2 = *ep++;
				match_1_char = (c1 >= e1) && (c1 <= e2);
			} else {
			/* 8-bit NLS collation or 16-bit machine collation */
			/* on first range subexpression get collation transform */
				if (!x1)
					x1 = _collxfrm(c1, 0, 0, (int *)0);
#ifdef NLS16
				if (FIRSTof2(c1) && SECof2(c2)) 
				    x1 = (c1 << 8) | c2;
#endif /* NLS16 */
				e1 = *ep++ << 16;
				e1 += *ep++ << 8;
				e1 += *ep++;
				e2 = *ep++ << 16;
				e2 += *ep++ << 8;
				e2 += *ep++;
				match_1_char = (x1 >= e1) && (x1 <= e2);
				match_2_to_1 = two_to_1 && (x2 >= e1) && (x2 <= e2);
			}
		}
	}
	
	/* negate matches as appropriate */
	if (negate) {
		match_1_char = !match_1_char;
		match_2_to_1 = !match_2_to_1 && two_to_1;
	}
	
	return ((match_1_char ? 1 : 0) | (match_2_to_1 ? 2 : 0));
}
	
	

