
/* @(#) $Revision: 70.1 $ */    
/*
 *	UNIX shell
 *
 *	Bell Telephone Laboratories
 *
 */

#include	"defs.h"
#include	<sys/types.h>
#include	<sys/stat.h>
#include	<ndir.h>


#ifdef NLS
#define NL_SETN 1
#endif 

#define MAXDIR	64
DIR			*opendir();
struct direct		*readdir();

static tchar		entry[MAXNAMLEN+1];

/*
 * globals (file name generation)
 *
 * "*" in params matches r.e ".*"
 * "?" in params matches r.e. "."
 * "[...]" in params matches character class
 * "[...a-z...]" in params matches a through z.
 *
 */
extern int	addg();
static tchar tslash[] = {'/', 0};

expand(as, rcnt)
	tchar	*as;
	int rcnt;
{
	int	count;
	DIR	*dirp;
	BOOL	dir = 0;
	tchar	*rescan = 0;
	register tchar	*s, *cs;
	struct argnod	*schain = gchain;
	struct stat statb;
	BOOL	slash;
	char *l_s;	 /* local char version of s for NLS */
	int index=0;
#if defined(DUX) || defined(DISKLESS)

	/* FSDlj04942 CDF pattern expansion*/

        char             meta = 0, cdf_plus = 0;   /* flag a '+' in pattern */
        tchar            *cdf_plusp;     /* points to '+' in pattern */
#endif /* DUX */

	if (trapnote & SIGSET)
		return(0);
	s = cs = as;

	/*
	 * check for meta chars
	 */
	{
		register BOOL open;

		slash = 0;
		open = 0;
		do
		{
loop:			switch (*cs++)
			{
			case 0:
				if (rcnt && slash)
					break;
				else
					return(0);

			case '/':
				slash++;
				open = 0;
				continue;

			case '[':
				open++;
				continue;

#if defined(DUX) || defined(DISKLESS)
                	case '+':
                        	/*
                         	*  Check to see if the plus is
                         	*  literal, or a possible CDF.
                         	*  Only a '+' or '+*' at the end
                         	*  of a path component can be a CDF
                         	*/
                               	cdf_plus=0;
				switch(*cs) {
				case 0:
				case '/':
                                	cdf_plus=1;
                                	cdf_plusp = cs - 1;
					break;
				case '*':
                                	cdf_plusp = cs - 1;
					while (*cs == '*') cs++;
					if (*cs == 0 || *cs == '/')
						cdf_plus = 1;
					else
						cs = ++ cdf_plusp;
					break;
				}  /* switch */
				continue;
#endif /* DUX */

			case ']':
				if (open == 0)
					continue;
			case '?':
			case '*':
				meta = 1;
				if (rcnt > slash)  
					continue;
				else
					cs--;
				break;

			default:
				goto loop;
			}
			break;
		} while (TRUE);
	}
	if ( meta && (*(cs+1) == '+')) {
				cdf_plus=0;
				cdf_plusp = (++cs);
				switch(*++cs) {
				case 0:
				case '/':
                                	cdf_plus=1;
					break;
				case '*':
					while (*cs == '*') cs++;
					if (*cs == 0 || *cs == '/')
						cdf_plus = 1;
					else
						cs = -- cdf_plusp;
					break;
				}  /* switch */
	}

	for (;;)
	{
		if (cs == s)
		{
			s = nullstr;
			break;
		}
		else if (*--cs == '/')
		{
			*cs = 0;
			if (s == cs)
				s = tslash;
			break;
		}
	}

#ifdef NLS
	/* 
	   DSDe600672 

	   This code was added to strip out the QUOTE mark from
	   the directory name.  In macro.c QUOTE is added to the tchar
	   thus the character (ch) is 0x80 & ch (rather than just
	   ch).  This cause a bad value to be returned by to_char
	   and later opendir 

	 */

	while (s[index]) {
		if ((s[index] >> 8) == 0x80)
			s[index] &= 0x00FF;
		index++;			
	}
#endif NLS
	l_s = to_char(s);
	l_s = (*l_s ? l_s : "."); /* Used later */
	if (stat(l_s,&statb) == 0) {
		if ((dirp = opendir(l_s)) != 0)
		{
			if (fstat(dirp->dd_fd, &statb) != -1 &&
		    	(statb.st_mode & S_IFMT) == S_IFDIR)
				dir++;
			else
				closedir(dirp);
		}
	}
	count = 0;
	if (*cs == 0)
		*cs++ = QUOTE;
	if (dir)		/* check for rescan */
	{
		register tchar *rs;
		struct direct *e;
#if defined(DUX) || defined(DISKLESS)
		struct stat f;
		char tmp[2048];
		int dlen,plen;
		char *baseptr, *name;

		if ((dlen = length(l_s)) > 2048)
		    error((nl_msg(1101, "sh internal 2K buffer overflow")));

		baseptr = movstr(l_s,tmp);
		*baseptr++ = '/';
#endif

		/* DSDe601376 -
		   The remaining pathname will be scanned for slashes to 
		   determine if it needs to be expanded again.  If a slash
		   is found as the last character of the pathname, it is 
		   replaced with a null character.  For example - ls /tmp/ 
		   is treated as ls /tmp.  
		*/

		rs = cs;
		do
		{
			if (*rs == '/')      /* DSDe601376 */
			   if (rs[1] == 0)
				*rs=0;
			   else {
				rescan = rs;
				*rs = 0;
				gchain = 0;
			   }
		} while (*rs++);

		while ((e = readdir(dirp)) && (trapnote & SIGSET) == 0)
		{
			*(movstrn(to_tchar(e->d_name), entry, MAXNAMLEN)) = 0;

/* skip over "." and ".." in the directory */
			if (entry[0] == '.' && *cs != '.')
#ifndef BOURNE
				continue;
#else
			{
				if (entry[1] == 0)
					continue;
				if (entry[1] == '.' && entry[2] == 0)
					continue;
			}
#endif

                        
			if (gmatch(entry, cs))
			{
#if defined(DUX) || defined(DISKLESS)

				/* Check for >= 2048 to allow room for '+' */
				name = to_char(entry);
				if ((plen = dlen + length(name)) >= 2048)
				    error((nl_msg(1102, "sh internal 2K buffer overflow")));

				movstr(name,baseptr);

				if (stat(tmp,&f) < 0){
				    tmp[plen--] = '\0';
				    tmp[plen] = '+';
				    if ((stat(tmp,&f) >= 0 ) &&
					(f.st_mode & S_ISUID) &&
					((f.st_mode & S_IFMT) == S_IFDIR))
					    continue;
				}
#endif
				addg(s, entry, rescan);
				count++;
			}
#if defined(DUX) || defined(DISKLESS)
			else if (cdf_plus) {
				*cdf_plusp = '\0';
				if (gmatch(entry, cs)) {
				   char *name_ptr;
				   name = to_char(entry);
				   if ((plen = dlen + length(name)) >= 2048)
				    error((nl_msg(1102, "sh internal 2K buffer overflow")));
				   name_ptr = baseptr;
				   movstr(name,baseptr);
				   tmp[plen--] = '\0';
				   tmp[plen] = '+';

				   if ((stat(tmp,&f) == 0 ) &&
					(f.st_mode & S_ISUID) &&
					((f.st_mode & S_IFMT) == S_IFDIR)) {
					     addg(s,to_tchar(name_ptr),rescan);
					     count++;
			 	   }	
				}
				*cdf_plusp = '+';
			}
						
#endif
		}  /* while */

		closedir(dirp);

		if (rescan)
		{
			register struct argnod	*rchain;

			rchain = gchain;
			gchain = schain;
			if (count)
			{
				count = 0;
				while (rchain)
				{
					count += expand(rchain->argval, slash + 1);
					rchain = rchain->argnxt;
				}
			}
			*rescan = '/';
		}
	}

	{
		register tchar	c;

		s = as;
		while (c = *s)
			*s++ = (c & STRIP ? c : '/');
	}
	return(count);
}

#ifdef NLS || NLS16

tchar savep[1024];
char  stemp[1024];

gmatch(s, p)
register tchar	*s, *p;	
{
	register tchar *t;
	register tchar buf[1024];
	register int i = 0;

	t = s;
	while (*t) {
		if ((*t &= STRIP) == 0)
			*t = QUOTE;
		t++;
	}

	if (cf(savep, p) != 0) {		/* new pattern */
		tmovstr(p, savep);		/* save the new pattern */

		t = p;				/* special handling for QUOTE */
		while (*t) {
			if (*t & QUOTE)
				buf[i++] = '\\';
			buf[i++] = *t & STRIP;
			t++;
		}
		buf[i] = '\0';

		strcpy(stemp,to_char(buf));
	}
	if (fnmatch(stemp,to_char(s),0))
		return(0);
	else					/* matched */
		return(1);
}

#else

gmatch(s, p)
register tchar	*s, *p;	
{
	register int	scc;
	tchar		c;

	if (scc = *s++)
	{
		if ((scc &= STRIP) == 0)
			scc = QUOTE;
	}
	switch (c = *p++)
	{
	case '[':
		{
			BOOL ok;
			int lc;
			int notflag = 0;

			ok = 0;
			lc = 077777;
			if (*p == '!')
			{
				notflag = 1;
				p++;
			}
			while (c = *p++)
			{
				if (c == ']')
					return(ok ? gmatch(s, p) : 0);
				else if (c == MINUS)
				{
					if (notflag)
					{
						if (scc < lc || scc > *(p++))
							ok++;
						else
							return(0);
					}
					else
					{
						if (lc <= scc && scc <= (*p++))
							ok++;
					}
				}
				else
				{
					lc = c & STRIP;
					if (notflag)
					{
						if (scc && scc != lc)
							ok++;
						else
							return(0);
					}
					else
					{
						if (scc == lc)
							ok++;
					}
				}
			}
			return(0);
		}

	default:
		if ((c & STRIP) != scc)
			return(0);

	case '?':
		return(scc ? gmatch(s, p) : 0);

	case '*':
		while (*p == '*')
			p++;

		if (*p == 0) 
			return(1);
		--s;
		while (*s)
		{
			if (gmatch(s++, p))
				return(1);
		}
		return(0);

	case 0:
		return(scc == 0);
	}
}

#endif NLS || NLS16

static int
addg(as1, as2, as3)
tchar	*as1, *as2, *as3;
{
	register tchar	*s1, *s2;
	register int	c;

	s2 = (tchar *)(Rcheat(locstak()) + BYTESPERWORD);
	sizechk(s2 + 2*CHARSIZE + tlength(as1) + tlength(as2) + tlength(as3));
	s1 = as1;
	while (c = *s1++)
	{
		if ((c &= STRIP) == 0)
		{
			*s2++ = '/';
			break;
		}
		*s2++ = c;
	}
	s1 = as2;
	while (*s2 = *s1++)
		s2++;
	if (s1 = as3)
	{
		*s2++ = '/';
		while (*s2++ = *++s1);
	}
	makearg(endstak(s2));
}

makearg(args)
	register struct argnod *args;
{
	args->argnxt = gchain;
	gchain = args;
}


