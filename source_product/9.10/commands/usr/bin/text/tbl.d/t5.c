/* @(#) $Revision: 42.1 $ */    
#ifndef NLS
#define nl_msg(i, s) (s)
#else NLS
#define NL_SETN 1	/* set number */
#include <msgbuf.h>
#endif NLS
 /* t5.c: read data for table */
# include "t..c"
#ifdef NLS16
#include <nl_ctype.h>	/* Must be included to define FIRSTof2() macro */
#endif
gettbl()
{
extern char *chspace();
extern char *maknew();
int icol, ch;
cstore=cspace= chspace();
textflg=0;
for (nlin=nslin=0; gets1(cstore); nlin++)
	{
	stynum[nlin]=nslin;
	if (prefix(".TE", cstore))
		{
		leftover=0;
		break;
		}
	if (prefix(".TC", cstore) || prefix(".T&", cstore))
		{
		readspec();
		nslin++;
		}
	if (nlin>=MAXLIN)
		{
		leftover=(int)cstore;
		break;
		}
	fullbot[nlin]=0;
	if (cstore[0] == '.' && !isdigit(cstore[1]))
		{
		instead[nlin] = cstore;
		while (*cstore++);
		continue;
		}
	else instead[nlin] = 0;
	if (nodata(nlin))
		{
		if (ch = oneh(nlin))
			fullbot[nlin]= ch;
		nlin++;
		nslin++;
		fullbot[nlin] = 0;
		instead[nlin] = (char *) 0;
		}
	table[nlin] = (struct colstr *) alocv((ncol+2)*sizeof(table[0][0]));
	if (cstore[1]==0)
	switch(cstore[0])
		{
		case '_': fullbot[nlin]= '-'; continue;
		case '=': fullbot[nlin]= '='; continue;
		}
	stynum[nlin] = nslin;
	nslin = min(nslin+1, nclin-1);
	for (icol = 0; icol <ncol; icol++)
		{
		table[nlin][icol].col = cstore;
		table[nlin][icol].rcol=0;
		ch=1;
		if (match(cstore, "T{")) /* text follows */
			table[nlin][icol].col =
				(char *)gettext(cstore, nlin, icol,
					font[icol][stynum[nlin]],
					csize[icol][stynum[nlin]]);
		else
			{
#ifdef NLS16
			for(; ch=(int)(unsigned char)(*cstore);cstore++){
				if ( FIRSTof2(ch&0377) ) {
					/* Skip HP15 or illeagal code */
					cstore++;
					if (!(*cstore)) break;
				} else if ( ch==tab ) {
					break;
				}
			}
#else
			for(; (ch= *cstore&0377) != '\0' && ch != tab; cstore++)
					;
#endif
			*cstore++ = '\0';
			switch(ctype(nlin,icol)) /* numerical or alpha, subcol */
				{
				case 'n':
					table[nlin][icol].rcol = maknew(table[nlin][icol].col);
					break;
				case 'a':
					table[nlin][icol].rcol = table[nlin][icol].col;
					table[nlin][icol].col = "";
					break;
				}
			}
		while (ctype(nlin,icol+1)== 's') /* spanning */
			table[nlin][++icol].col = "";
		if (ch == '\0') break;
		}
	while (++icol <ncol+2)
		{
		table[nlin][icol].col = "";
		table [nlin][icol].rcol=0;
		}
	while (*cstore != '\0')
		 cstore++;
	if (cstore-cspace > MAXCHS)
		cstore = cspace = chspace();
	}
last = cstore;
permute();
if (textflg) untext();
return;
}
nodata(il)
{
int c;
for (c=0; c<ncol;c++)
	{
	switch(ctype(il,c))
		{
		case 'c': case 'n': case 'r': case 'l': case 's': case 'a':
			return(0);
		}
	}
return(1);
}
oneh(lin)
{
int k, icol;
k = ctype(lin,0);
for(icol=1; icol<ncol; icol++)
	{
	if (k != ctype(lin,icol))
		return(0);
	}
return(k);
}
# define SPAN "\\^"
permute()
{
int irow, jcol, is;
char *start, *strig;
for(jcol=0; jcol<ncol; jcol++)
	{
	for(irow=1; irow<nlin; irow++)
		{
		if (vspand(irow,jcol,0))
			{
			is = prev(irow);
			if (is<0)
				error((nl_msg(22, "Vertical spanning in first row not allowed")));
			start = table[is][jcol].col;
			strig = table[is][jcol].rcol;
			while (irow<nlin &&vspand(irow,jcol,0))
				irow++;
			table[--irow][jcol].col = start;
			table[irow][jcol].rcol = strig;
			while (is<irow)
				{
				table[is][jcol].rcol =0;
				table[is][jcol].col= SPAN;
				is = next(is);
				}
			}
		}
	}
}
vspand(ir,ij,ifform)
{
if (ir<0) return(0);
if (ir>=nlin)return(0);
if (instead[ir]) return(0);
if (ifform==0 && ctype(ir,ij)=='^') return(1);
if (table[ir][ij].rcol!=0) return(0);
if (fullbot[ir]) return(0);
return(vspen(table[ir][ij].col));
}
vspen(s)
	char *s;
{
if (s==0) return(0);
if (!point(s)) return(0);
return(match(s, SPAN));
}
