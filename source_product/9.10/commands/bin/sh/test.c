/* @(#) $Revision: 66.2 $ */      
/*
 *      test expression
 *      [ expression ]
 */

#ifdef NLS
#define NL_SETN 1	/* set number */
#endif NLS

#include	"defs.h"
#include <sys/types.h>
#include <sys/stat.h>

int	ap, ac;
tchar	**av;

test(argn, com)
tchar	*com[];
int	argn;
{
	ac = argn;
	av = com;
	ap = 1;
	if (eqtc(com[0],"["))
	{
		if (!eqtc(com[--ac], "]"))
			failed("test", (nl_msg(901, "] missing")));
	}
	com[ac] = 0;
	if (ac <= 1)
		return(1);
	return(testexp() ? 0 : 1);
}

tchar *
nxtarg(mt)
{
	if (ap >= ac)
	{
		if (mt)
		{
			ap++;
			return(0);
		}
		failed("test", (nl_msg(902, "argument expected")));
	}
	return(av[ap++]);
}

testexp()
{
	int	p1;
	tchar	*p2;

	p1 = e1();
	p2 = nxtarg(1);
	if (p2 != 0)
	{
		if (eqtc(p2, "-o"))
			return(p1 | testexp());

		if (eqtc(p2, "]") && !eqtc(p2, ")")) 
			failed("test", 		      
					nl_msg(605,synmsg));
		/* nl_msg can't be on same line as the quotes */
	}
	ap--;
	return(p1);
}

e1()
{
	int	p1;
	tchar	*p2;

	p1 = e2();
	p2 = nxtarg(1);

	if ((p2 != 0) && eqtc(p2, "-a"))
		return(p1 & e1());
	ap--;
	return(p1);
}

e2()
{
	if (eqtc(nxtarg(0), "!"))
		return(!e3());
	ap--;
	return(e3());
}

e3()
{
	int	p1;
	register tchar	*a;
	tchar	*p2;
	long	atol();
	long	int1, int2;

	a = nxtarg(0);
	if (eqtc(a, "("))
	{
		p1 = testexp();
		if (!eqtc(nxtarg(0), ")"))
			failed("test",(nl_msg(903, ") expected")));
		return(p1);
	}
	p2 = nxtarg(1);
	ap--;
	if ((p2 == 0) || (!eqtc(p2, "=") && !eqtc(p2, "!=")))
	{
		if (eqtc(a, "-r"))
			return(tio(nxtarg(0), 4));
		if (eqtc(a, "-w"))
			return(tio(nxtarg(0), 2));
		if (eqtc(a, "-x"))
			return(tio(nxtarg(0), 1));
		if (eqtc(a, "-d"))
			return(filtyp(nxtarg(0), S_IFDIR));
		if (eqtc(a, "-c"))
			return(filtyp(nxtarg(0), S_IFCHR));
		if (eqtc(a, "-b"))
			return(filtyp(nxtarg(0), S_IFBLK));
		if (eqtc(a, "-f"))
			return(filtyp(nxtarg(0), S_IFREG));
		if (eqtc(a, "-u"))
			return(ftype(nxtarg(0), S_ISUID));
#if defined(DUX) || defined(DISKLESS)
		if (eqtc(a, "-H"))
			return(isHidden(a=nxtarg(0)));
#endif
#ifdef SYMLINKS
		if (eqtc(a, "-h"))
			return(lfiltyp(nxtarg(0), S_IFLNK));
#endif
		if (eqtc(a, "-g"))
			return(ftype(nxtarg(0), S_ISGID));
		if (eqtc(a, "-k"))
			return(ftype(nxtarg(0), S_ISVTX));
		if (eqtc(a, "-p"))
#ifdef S_IFIFO
			return(filtyp(nxtarg(0),S_IFIFO));
#else
			return(filtyp(0,nxtarg(0)));
			/* filtyp goes boom, nxtarg++ */
#endif S_IFIFO
   		if (eqtc(a, "-s"))
			return(fsizep(nxtarg(0)));
		if (eqtc(a, "-t"))
		{
			if (ap >= ac)		/* no args */
				return(isatty(1));
			else if (eqtc((a = nxtarg(0)), "-a") || eqtc(a, "-o"))
          
			{
				ap--;
				return(isatty(1));
			}
			else
				return(isatty(atoi(to_char(a))));
		}
		if (eqtc(a, "-n"))
			return(!eqtc(nxtarg(0), ""));
		if (eqtc(a, "-z"))
			return(eqtc(nxtarg(0), ""));
	}

	p2 = nxtarg(1);
	if (p2 == 0)
		return(!eqtc(a, ""));
	if (eqtc(p2, "-a") || eqtc(p2, "-o"))
	{
		ap--;
		return(!eqtc(a, ""));
	}
	if (eqtc(p2, "="))
		return(eqtt(nxtarg(0), a));
	if (eqtc(p2, "!="))
		return(!eqtt(nxtarg(0), a));
	int1 = atol(to_char(a));
	int2 = atol(to_char(nxtarg(0)));
	if (eqtc(p2, "-eq"))
		return(int1 == int2);
	if (eqtc(p2, "-ne"))
		return(int1 != int2);
	if (eqtc(p2, "-gt"))
		return(int1 > int2);
	if (eqtc(p2, "-lt"))
		return(int1 < int2);
	if (eqtc(p2, "-ge"))
		return(int1 >= int2);
	if (eqtc(p2, "-le"))
		return(int1 <= int2);

	bfailed(btest, nl_msg(636, badop), p2);
/* NOTREACHED */
}


tio(a, f)		
tchar	*a;
int	f;
{
	if (access(to_char(a), f) == 0)
		return(1);
	else
		return(0);
}

ftype(f, field)
tchar	*f;
int	field;
{
	struct stat statb;

	if (stat(to_char(f), &statb) < 0)
		return(0);
	if ((statb.st_mode & field) == field)
		return(1);
	return(0);
}

filtyp(f,field)
tchar	*f;
int field;
{
	struct stat statb;

	if (stat(to_char(f), &statb) < 0)
		return(0);
	if ((statb.st_mode & S_IFMT) == field)
		return(1);
	else
		return(0);
}


#ifdef SYMLINKS
lfiltyp(f,field)         /* routine to do a lstat for symbolic links */
tchar	*f;
int field;
{
	struct stat statb;

	if (lstat(to_char(f), &statb) < 0)
		return(0);
	if ((statb.st_mode & S_IFMT) == field)
		return(1);
	else
		return(0);
}
#endif
#if defined(DUX) || defined(DISKLESS)
isHidden(f)
tchar *f;
{
    char tmp[1024];
    struct stat a;
    struct stat b;
    movstr(to_char(f),tmp);
    if (stat(to_char(f),&a) == 0 ){
    	if ((( a.st_mode & S_IFMT ) == S_IFDIR ) &&
        	( a.st_mode & S_ISUID ))
            return(1);
    }
    strcat(tmp,"+");
    if (stat(tmp,&b) < 0 )
         return(0);
    if ((( b.st_mode & S_IFMT ) == S_IFDIR ) &&
        ( b.st_mode & S_ISUID ))
            return(1);
    return(0);
}
#endif

    
    

fsizep(f)
tchar	*f;
{
	struct stat statb;

	if (stat(to_char(f), &statb) < 0)
		return(0);
	return(statb.st_size > 0);
}

/*
 * fake diagnostics to continue to look like original
 * test(1) diagnostics
 */
bfailed(s1, s2, s3) 
char	*s1;
char	*s2;
char	*s3;
{
	prp();
	prs(s1);
	if (s2)
	{
		prs(colon);
		prs(s2);
		prs(s3);
	}
	newline();
	exitsh(ERROR);
}
