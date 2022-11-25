/* @(#) $Revision: 64.1 $ */    
#ifndef NLS
#define nl_msg(i, s) (s)
#else NLS
#define NL_SETN 1	/* set number */
#include <msgbuf.h>
#include <limits.h>
#endif NLS
 /* te.c: error message control, input line count */
# include "t..c"
#ifdef NLS16
#include <nl_ctype.h>
#endif
error(s)
	char *s;
{
#ifdef NLS
	char Nlsbuf[NL_TEXTMAX]; /* To avoid duplicate */	
	strcpy(Nlsbuf, s);
	s=Nlsbuf;
#endif
fprintmsg(stderr, (nl_msg(31, "\n%1$s: line %2$d: %3$s\n")), ifile, iline, s);
# ifdef unix
fprintf(stderr, (nl_msg(32, "tbl quits\n")));
exit(1);
# endif
# ifdef gcos
fprintf(stderr, (nl_msg(33, "run terminated due to error condition detected by tbl preprocessor\n")));
exit(0);
# endif
}
char *gets1(s)
	char *s;
{
char *p;
int nbl = 0;
iline++;
p=fgets(s,512,tabin);
while (p==0)
	{
	if (swapin()==0)
		return(0);
	p = fgets(s,512,tabin);
	}

while (*s) s++;
s--;
if (*s == '\n') *s-- =0;
#ifdef NLS16
for(nbl=0; (*s == '\\' && !(FIRSTof2((unsigned char)(*(s-1))))) && s>p; s--)
#else
for(nbl=0; *s == '\\' && s>p; s--)
#endif
	nbl++;

if (linstart && nbl % 2) /* fold escaped nl if in table */
	gets1(s+1);

return(p);
}
# define BACKMAX 1600
char backup[BACKMAX];
char *backp = backup;
un1getc(c)
{
if (c=='\n')
	iline--;
*backp++ = c;
if (backp >= backup+BACKMAX)
	error((nl_msg(34, "too much backup")));
}
get1char()
{
int c;
if (backp>backup)
	c = *--backp;
else
	c=getc(tabin);
if (c== EOF) /* EOF */
	{
	if (swapin() ==0)
		error((nl_msg(35, "unexpected EOF")));
	c = getc(tabin);
	}
if (c== '\n')
	iline++;
return(c&0377);
}
