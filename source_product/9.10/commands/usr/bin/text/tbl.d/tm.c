/* @(#) $Revision: 64.1 $ */      
 /* tm.c: split numerical fields */
# include "t..c"
#ifdef NLS16
#include <nl_ctype.h>
#include <langinfo.h>
extern char *nl_langinfo();
#endif
char *maknew(str)
	char *str;
{
	/* make two numerical fields */
	extern char *chspace();
	int dpoint, c;
	char *p, *q, *ba;
#ifdef NLS16
	char *lastp;
#endif
	p = str;
#ifdef NLS16
	for (ba= 0; c = *str; str++) {
		if ( FIRSTof2((unsigned char)(*str)) ) {
			str++;
			if (!(*str)) break;
		} else {
			if (c == '\\' && *(str+1)== '&')
				ba=str;
		}
	}
#else
	for (ba= 0; c = *str; str++)
		if (c == '\\' && *(str+1)== '&')
			ba=str;
#endif
	str=p;
	if (ba==0)
		{
#ifdef NLS16
		for (dpoint=0; *str; str++) {
			if ( FIRSTof2((unsigned char)(*str)) ) {
				str++;
				if ( !(*str) ) break;
			} else {
				if (   *str == *nl_langinfo(RADIXCHAR)
			    	    && !ineqn(str,p) 
			    	    && (str>p && digit(*(str-1)) 
			    	    || digit(*(str+1))) )
					dpoint=(int)str;
			}
		}
#else 
		for (dpoint=0; *str; str++)
			{
			if (*str=='.' && !ineqn(str,p) &&
				(str>p && digit(*(str-1)) ||
				digit(*(str+1))))
					dpoint=(int)str;
			}
#endif
#ifdef NLS16
		if (dpoint==0) {
			lastp=0;
			for (str=p;*str;str++) {
				if ( FIRSTof2((unsigned char)(*str)) ) {
					str++;
					if ( !(*str) ) break;
				} else {
					if (   digit(*str) 
					    && !ineqn(str+1,p)) {
						lastp=str+1;
					}
				}
			}
			if ( lastp ) {
				str= lastp;
			} else {
				str=p;
			}
		}
#else
		if (dpoint==0)
			for(; str>p; str--)
			{
			if (digit( * (str-1) ) && !ineqn(str, p))
				break;
			}
#endif
		if (!dpoint && p==str) /* not numerical, don't split */
			return(0);
		if (dpoint) str=(char *)dpoint;
		}
	else
		str = ba;
	p =str;
	if (exstore ==0 || exstore >exlim)
		{
		exstore = chspace();
		exlim= exstore+MAXCHS;
		}
	q = exstore;
	while (*exstore++ = *str++);
	*p = 0;
	return(q);
	}
ineqn (s, p)
	char *s, *p;
{
/* true if s is in a eqn within p */
int ineq = 0, c;
#ifdef NLS16
while (c = *p) {
	if (s == p)
		return(ineq);
	if ( FIRSTof2((unsigned char)(*p)) ) {
		p++;
		if ( !(*p) ) {
			break;
		} else if ( s==p ) {
			return(ineq);
		}
	} else {
		if ((ineq == 0) && (c == delim1))
			ineq = 1;
		else
		if ((ineq == 1) && (c == delim2))
			ineq = 0;
	}
	p++;
}
return(0);
#else
while (c = *p)
	{
	if (s == p)
		return(ineq);
	p++;
	if ((ineq == 0) && (c == delim1))
		ineq = 1;
	else
	if ((ineq == 1) && (c == delim2))
		ineq = 0;
	}
return(0);
#endif
}
