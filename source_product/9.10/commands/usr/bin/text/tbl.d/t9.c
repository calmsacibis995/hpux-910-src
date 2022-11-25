/* @(#) $Revision: 49.1 $ */    
#ifndef NLS
#define nl_msg(i, s) (s)
#else NLS
#define NL_SETN 1	/* set number */
#include <msgbuf.h>
#endif NLS
 /* t9.c: write lines for tables over 200 lines */
# include "t..c"
static useln;
extern char *maknew();
yetmore()
{
for(useln=0; useln<MAXLIN && table[useln]==0; useln++);
if (useln>=MAXLIN)
	error((nl_msg(25, "Wierd.  No data in table.")));
table[0]=table[useln];
for(useln=nlin-1; useln>=0 && (fullbot[useln] || instead[useln]); useln--);
if (useln<0)
	error((nl_msg(26, "Wierd.  No real lines in table.")));
domore(leftover);
while (gets1(cstore=cspace) && domore(cstore))
	;
last =cstore;
return;
}
domore(dataln)
	char *dataln;
{
	int icol, ch;
if (prefix(".TE", dataln))
	return(0);
if (dataln[0] == '.' && !isdigit(dataln[1]))
	{
	puts(dataln);
	return(1);
	}
fullbot[0]=0;
instead[0]=(char *)0;
if (dataln[1]==0)
switch(dataln[0])
	{
	case '_': fullbot[0]= '-'; putline(useln,0);  return(1);
	case '=': fullbot[0]= '='; putline(useln, 0); return(1);
	}
for (icol = 0; icol <ncol; icol++)
	{
	table[0][icol].col = dataln;
	table[0][icol].rcol=0;
	for(; (ch= *dataln&0377) != '\0' && ch != tab; dataln++)
			;
	*dataln++ = '\0';
	switch(ctype(useln,icol))
		{
		case 'n':
			table[0][icol].rcol = maknew(table[0][icol].col);
			break;
		case 'a':
			table[0][icol].rcol = table[0][icol].col;
			table[0][icol].col= "";
			break;
		}
	while (ctype(useln,icol+1)== 's') /* spanning */
		table[0][++icol].col = "";
	if (ch == '\0') break;
	}
while (++icol <ncol)
	{
	table[0][icol].col = "";
	table[0][icol].rcol = 0;
	}
putline(useln,0);
return(1);
}
