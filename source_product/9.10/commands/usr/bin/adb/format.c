/* @(#) $Revision: 66.2 $ */      

/****************************************************************************

	TRIX DEBUGGER

****************************************************************************/
#include "defs.h"

MSG		BADMOD;
MSG		NOFORK;
MSG		ADWRAP;
SYMPTR		symbol;
short		mkfault;		/* MFM */
CHAR		*lp;
int		maxoff;
int		(*sigint)();
int		(*sigqit)();
STRING		errflg;
CHAR		lastc;
long		dot;
int		dotinc;
long		var[];
int		Sig_Digs=6;		/* significant digits for real num */
double		ftod();
extern int	fsym;


scanform(icount,ifp,itype,ptype)
long		icount;
STRING		ifp;
{
	STRING		fp;
	CHAR		modifier;
	int		fcount, init=1;
	long		savdot;

	while (icount)
	{  fp=ifp;
	    if (init==0 && findsym(shorten(dot),ptype)==0 && maxoff)
	       printf("\n%s:%16t",symbol->symc);
	    savdot=dot; init=0;

	    /*now loop over format*/
	    while (*fp && errflg==0)
	    {   if (digit(modifier = *fp))
	        {    fcount=0;
		     while (digit(modifier = *fp++))
		     {  fcount *= 10;
			fcount += modifier-'0';
		     }
		     fp--;
		}
		else fcount=1;

		if (*fp==0) break;
		fp=exform(fcount,fp,itype,ptype);
	    }
	    dotinc=dot-savdot;
	    dot=savdot;

	    if (errflg)
	    {    if (icount<0)
		 { errflg=0;
		   break;
		 }
		 else error(errflg);
	    }
	    if (--icount) dot=inkdot(dotinc);
	    if (mkfault) error(0);
	}
}

STRING	exform(fcount,ifp,itype,ptype)
int		fcount;
STRING		ifp;
{
	/* execute single format item `fcount' times
	 * sets `dotinc' and moves `dot'
	 * returns address of next format item
	 */
	long		savdot;
	STRING		fp;
	CHAR		modifier;
	int		longpr;
	L_REAL		fw;
	register long	pval;
	register char	c;
	char		rbuf[20];
	union {
		long l[2];
		double d;
	} r;

	fp = ifp;
	while (fcount>0)
	{	fp = ifp; c = *fp;
		longpr = ( ( (c >= 'A' && c <= 'Z' && c != 'I' && c != 'B')
			     || c == 'f' || c == 'p'
			   ) ? 1 : 0);
		if (longpr) dotinc = 4; else dotinc = 2;
		if ((itype == NSP) || (*fp == 'a'))
		{
			pval = dot;
		}
		else
		{
			pval = getword(dot,itype);
			if (!longpr)
			{	pval >>= 16;
				pval &= 0xFFFF;
			}
		}
		if (errflg) return(fp);
		if (mkfault) error(0);
		modifier = *fp++;
		var[0] = pval;

		if (charpos()==0 && modifier!='a') printf("%16m");

		switch(modifier) {

		    case SPACE: case TB:
			break;

		    case 't': case 'T':
			printf("%T",fcount); return(fp);

		    case 'r': case 'R':
			printf("%M",fcount); return(fp);

		    case 'a':
			psymoff(dot,ptype,":%16t"); dotinc=0; break;

		    case 'p':
			psymoff(pval,ptype,"%16t"); break;

		    case 'u':
			printf("%-8u",pval); break;

		    case 'U':
			printf("%-16U",pval); break;

		    case 'c': case 'C':
			if (itype == NSP) pval <<= 8;
			if (modifier=='C') printesc((pval>>24)&LOBYTE);
			else printc((pval>>8)&LOBYTE);
			dotinc=1; break;

		    case 'b': 
			printf("%-8x", (pval>>8)&LOBYTE);
			dotinc=1; break;

		    case 'B':
			printf("%-8o", (pval>>8)&LOBYTE);
			dotinc=1; break;

		    case 's': case 'S':
			savdot=dot; dotinc=1;
			while ((c = (get(dot,itype)>>8)&LOBYTE) && errflg==0)
			{  dot=inkdot(1);
			   if (modifier == 'S') printesc(c);
			   else printc(c);
			   endline();
			}
			dotinc=dot-savdot+1; dot=savdot; break;

		    case 'x':
		    case 'w':
			printf("%-8x",pval); break;

		    case 'X':
		    case 'W':
			printf("%-16X", pval); break;

		    case 'Y':
			printf("%-24Y", pval); break;

		    case 'q':
			printf("%-8q", pval); break;

		    case 'Q':
			printf("%-16Q", pval); break;

		    case 'o':
			printf("%-8o", pval); break;

		    case 'O':
			printf("%-16O", pval); break;

		    case 'i':
			printins(0,itype,pval); printc(EOR); break;

		    case 'I':
			printins(1,itype,pval); printc(EOR); break;

		    case 'd':
			printf("%-8d", pval); break;

		    case 'D':
			printf("%-16D", pval); break;

		    case 'f':
			printdoub(ftod(pval),Sig_Digs);
			printc('\n');
			break;

		    case 'F':
			dotinc = 8;
			r.l[0] = pval;
			r.l[1] = getword(dot+4,itype);
			printdoub(r.d,Sig_Digs);
			printc('\n');
			break;

		    case 'n': case 'N':
			printc('\n'); dotinc=0; break;

		    case '"':
			dotinc=0;
			while (*fp != '"' && *fp) printc(*fp++);
			if (*fp) fp++;
			break;

		    case '^':
			dot=inkdot(-dotinc*fcount); return(fp);

		    case '+':
			dot=inkdot(fcount); return(fp);

		    case '-':
			dot=inkdot(-fcount); return(fp);

		    default: error(BADMOD);
		}
		if (itype!=NSP) dot=inkdot(dotinc);
		fcount--; endline();
	}
	return(fp);
}

unox()
{
	int	rc, status, unixpid;
	char	*argp = lp;

	while (lastc != EOR) rdc();
	if ((unixpid = fork()) == 0)
	{
		fcntl(fsym,F_SETFD,1);
		signal(SIGINT, sigint);
		signal(SIGQUIT, sigqit);
		*lp = 0;
		execl("/bin/sh", "sh", "-c", argp, 0);
		exit(16);
	}
	else if (unixpid == -1) error(NOFORK);
	else
	{
		signal(SIGINT, SIG_IGN);
		while(((rc = wait(&status)) != unixpid) && (rc != -1));
		signal(SIGINT, sigint);
		prints("!"); lp--;
	}
}

printesc(c)
{
	c &= STRIP;
	if (c<SPACE || c>'~' || c=='@')
	   printf("@%c",(c=='@' ? '@' : (c&(~(c&0140)))|0140));
	else printc(c);
}

long	inkdot(incr)
{
	return(dot + incr);
}
