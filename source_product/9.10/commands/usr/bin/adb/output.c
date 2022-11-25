/* @(#) $Revision: 66.1 $ */      

/****************************************************************************

	DEBUGGER - output routines

****************************************************************************/
#include "defs.h"

short		mkfault;		/* MFM */
FILE		*infile;
FILE		*outfile;
int		maxpos;
int		hexa;

CHAR		printbuf[MAXLIN];
CHAR		*printptr = printbuf;
CHAR		*digitptr;
CHAR		*endofprint = &printbuf[MAXLIN-1];

printc(c)
CHAR	c;
{
	register CHAR	d;			/* MFM */
	register STRING	q;			/* MFM */
	register int	tabs, posn, p;		/* MFM */

	if (mkfault) return;
	else if ((*printptr=c)==EOR || printptr == endofprint)
	{    tabs=0; posn=0; q=printbuf;
	     for (p=0; p<printptr-printbuf; p++)
	     {  d=printbuf[p];
		if ((p&7)==0 && posn)
		{ tabs++;
		  posn=0;
		}
		if (d==SPACE) posn++;
		else
		{    while (tabs>0)
		     { *q++=TB;
		       tabs--;
		     }
		     while (posn>0)
		     { *q++=SPACE;
		       posn--;
		     }
		     *q++=d;
		}
	     }
	     *q++=c;
	     fwrite(printbuf, 1, q-printbuf, outfile);
	     printptr=printbuf;
	}
	else if (c==TB)
	{ *printptr++=SPACE;
	     while ((printptr-printbuf)&7) *printptr++=SPACE;
	}
	else if (c) printptr++;
}

charpos()
{	return(printptr-printbuf);
}

flushbuf()
{	if (printptr!=printbuf) printc(EOR);
}

printf(fmat, a1)
STRING	fmat;
STRING	*a1;
{
	STRING		fptr, s;
	int		*vptr;
	long		*dptr;
	int		width, prec;
	CHAR		c, adj;
	int		x, decpt, n;
	long		lx;
	CHAR		digits[64];

	fptr = fmat; vptr = (int *)&a1;
	while (c = *fptr++)
	{   if (c!='%') printc(c);
	    else
	    { if (*fptr=='-')
	      	{  adj='l';
	 	   fptr++;
	      	}
		else adj='r';
		width=convert(&fptr);
		if (*fptr=='.')
		{   fptr++;
		    prec=convert(&fptr);
		}
		else prec = -1;
		digitptr = digits;
		dptr = (long *)&a1; lx = *dptr; x = *vptr++; s = (STRING)0;
		switch (c = *fptr++)
		{
		   case 'd':
		   case 'u':
		   case 'D':
		   case 'U':
			printnum(x, c, 10); break;
		   case 'o':
		   case 'O':
			printoct(x, 0); break;
		   case 'q':
		   case 'Q':
			printoct(x, -1); break;
		   case 'h':
			if (x<0) printc('-');
			x = abs(x);
			c = 'x';
		   case 'x':
		   case 'X':
			printnum(x, c, 16); break;
		   case 'Y':
			printdate(lx); break;
		   case 'c':
			printc(x); break;
		   case 's':
			s = (STRING)x; break;
		   case 'm':
			vptr--; break;
		   case 'M':
			width = x; break;
		   case 'T':
		   case 't':
			if (c=='T') width = x; else vptr--;
			if (width) width -= charpos() % width;
			break;
		   default:
			printc(c); vptr--;
		}
		if (s == NULL)
		{   *digitptr = NULL;
		    s = digits;
		}
		n = strlen(s);
		n = (prec<n && prec>=0 ? prec : n);
		width -= n;
		if (adj=='r') while (width-- > 0) printc(SPACE);
		while (n--) printc(*s++);
		while (width-- > 0) printc(SPACE);
		digitptr = digits;
	    }
	}
}

printdate(tvec)
long	tvec;
{
	register  short	i;
	register STRING	timeptr;
	timeptr = (STRING)ctime(&tvec);
	for (i=20; i<24; i++) *digitptr++ = *(timeptr+i);
	for (i= 3; i<19; i++) *digitptr++ = *(timeptr+i);
}

prints(s)
char	*s;
{
	printf("%s", s);
}

newline()
{
	printc(EOR);
}

convert(cp)
register STRING	*cp;
{
	register CHAR	c;
	int		n;
	n=0;
	while (((c = *(*cp)++)>='0') && (c<='9')) n=n*10+c-'0';
	(*cp)--;
	return(n);
}

printnum(n, fmat, base)
register int	n;
char		fmat;
{
	register char	k;
	register int	*dptr;
	int		digs[15], dnum = 0, i;

	dptr = digs;
	if (hexa) dnum = ((base==10) ? 0 : ((fmat=='x') ? 4 : 8));

	if (base == 16) {*digitptr++ = '0'; *digitptr++ ='x';}
	else if ((n < 0) && (fmat=='d' || fmat=='D'))
	{
		n = -n;
		*digitptr++ = '-';
	}
	i = (dnum ? dnum : n);
	while (i)
	{
		*dptr++ = ((unsigned)n) % base;
		n = ((unsigned)n)/base;
		if (dnum) i--; else i = n;
	}
	if (dptr == digs) *dptr++ = NULL;
	while (dptr != digs)
	{
		k = *--dptr;
		*digitptr++ = (k+ ((k <= 9) ? '0' : ('A' - 10)));
	}
}

printoct(o, s)
long	o;
int	s;
{
	register short	i;		/* MFM */
	long		po = o;
	CHAR		digs[12];

	if (s)
	{ if (po<0)
	     { po = -po;
	       *digitptr++='-';
	     }
	     else if (s>0)
	       *digitptr++='+';
	}
	for (i=0;i<=11;i++)
	{ digs[i] = po&7;
	  po >>= 3;
	}
	digs[10] &= 03; digs[11]=0;
	for (i=11;i>=0;i--) if (digs[i]) break; 
	for (i++;i>=0;i--) *digitptr++=digs[i]+'0';
}

printdoub(val,sd)
double val;
long sd;
{	register char *p;
	int decpt, sign;
	char *ecvt();
	
	p = ecvt(val,sd,&decpt,&sign);
	if (*p!='N' && *p!='I')
	{
		if (sign) printc('-');
		else printc(' ');
		printc('.');
		while (*p) printc(*p++);
		printc('e');
		if (decpt>=0) printc('+');
		printf("%d",decpt);
	}
	else	while (*p) printc(*p++);

}

iclose()
{
	if (infile!=stdin)
	{	fclose(infile);
		infile=stdin;
	}
}

oclose()
{
	if (outfile!=stdout)
	{	flushbuf();
		fclose(outfile);
		outfile=stdout;
	}
}

endline()
{
	if (charpos()>=maxpos) printf("\n");
}
