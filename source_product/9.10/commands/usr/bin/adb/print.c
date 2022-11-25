/* @(#) $Revision: 66.1 $ */   

/****************************************************************************

	DEBUGGER

****************************************************************************/
#include "defs.h"
#define MAXSIG 16
extern printfloats();

MSG		LONGFIL;
MSG		NOTOPEN;
MSG		A68BAD;
MSG		A68LNK;
MSG		BADMOD;
MAP		txtmap;
MAP		datmap;
SYMPTR		symbol;
int		lastframe;
int		callpc;
FILE		*infile;
FILE		*outfile;
int		fcor;
CHAR		*lp;
int		maxoff;
int		maxpos;
short		hexa = FALSE;		/* 0-noadjust, TRUE-adjust */
int		Sig_Digs;
int		ibase;			/* default base for input */
COREMAP		*cmaps;
char		coremapped;

/* breakpoints */
BKPTR		bkpthead;

REGLIST reglist[] =
{
		"d0", R0, 0,
		"d1", R1, 0,
		"d2", R2, 0,
		"d3", R3, 0,
		"d4", R4, 0,
		"d5", R5, 0,
		"d6", R6, 0,
		"d7", R7, 0,
		"a0", AR0, 0,
		"a1", AR1, 0,
		"a2", AR2, 0,
		"a3", AR3, 0,
		"a4", AR4, 0,
		"a5", AR5, 0,
		"a6", AR6, 0,
		"sp", SP, 0,
		"ps", PS, 0,
		"pc", PC, 0,
};

char		lastc;
int		fcor;
STRING		errflg;
long		dot;
long		var[];
STRING		symfil;
STRING		corfil;
int		pid;
long		adrval;
int		adrflg;
long		cntval;
int		cntflg;
int		signum;
STRING		signals[] = 
{ "","hangup","interrupt","quit","illegal instruction","trace trap","IOT",
  "EMT","floating exception","killed","bus error","segmentation violation",
  "bad syscall argument","broken pipe","alarm","terminated","user signal 1",
  "user signal 2","sigchld","sigpwr","virtual time alarm","SIG21","SIG22",
  "SIG23","SIG24","SIG25","SIG26","SIG27","SIG28","SIG29","SIG30","SIG31",
  "SIG32" };

printtrace(modif)
{
	int		stat, name, limit;
	short		narg, i;		/* MFM */
	POS		dynam;
	register  BKPTR	bkptr;
	CHAR		hi, lo;
	int		word;
	STRING		comptr;
	register SYMPTR		symp;		/* MFM */

	if (cntflg==0) cntval = -1;

	switch (modif)
	{
	    case '<':
	    case '>':
 	    {
		CHAR	file[64];
		int	index;

		index=0;
		if (modif=='<') iclose();
		else oclose();
		if (rdc()!=EOR)
		{	do
			{ file[index++]=lastc;
			  if (index>=63) error(LONGFIL);
			} while (readchar()!=EOR);
			file[index]=0;
			if (modif=='<')
			{	infile=fopen(file,"r");
				if (infile == NULL) error(NOTOPEN);
			}
			else
			{	outfile=fopen(file,"w");
				if (outfile == NULL) error(NOTOPEN);
				else fseek(outfile,0L,2);
			}

		}
		lp--;
		}
		break;

	    case 'x':
		ibase = 16; break;

#ifdef help
	    case 'h':
		system("more /usr/lib/adb.help");
		break;	
#endif help

	    case 'H':
		hexa = TRUE; break;	

	    case 'd':
		ibase = 10; break;

	    case 'o':
		ibase = 8; break;

	    case 'n':
		Sig_Digs = (adrflg ? ((adrval > MAXSIG) ? MAXSIG : adrval) :
				MAXSIG);
		break;

	    case 'q': case 'Q': case '%':
		done();

	    case 'w': case 'W':
		maxpos = (adrflg ? adrval : MAXPOS);
		break;

	    case 's': case 'S':
		maxoff = (adrflg ? adrval : 32768);
		break;

	    case 'v': case 'V':
		prints("variables\n");
		for (i=0;i<=35;i++)
		  if (var[i])
		  {     printc((i<=9 ? '0' : 'a'-10) + i);
			printf(" = %X\n",var[i]);
		  }
		break;

	    case 'm': case 'M':
		printmap("? map",&txtmap);
		if (cmaps && coremapped) printcmap("/ map");
		printmap(coremapped?"/ map (inactive)":"/ map",&datmap);
		if (cmaps && !coremapped) printcmap("/ map (inactive)");
		break;

	    case 0: case '?':
		if (pid) printf("sub-process id = %d\n", pid);
		prints(signals[signum]);

	    case 'r': case 'R':
		if (pid || (fcor != -1)) printregs();
		else prints("no process or core image\n");
		return;

	    case 'f': case 'F':
		if (pid || (fcor != -1)) printfloats();
		else prints("no process or core image\n");
		return;

	    case 'c': case 'C':
		/* add support for $C later ?? */
		if (pid) backtr(adrflg?adrval:getreg(pid, reglist[a6].roffs), cntval);
		else if (fcor != -1) backtr(adrflg?adrval:reglist[a6].rval, cntval);
		else prints("no process or core image\n");
		break;

	    /*print externals*/
	    case 'e': case 'E':
		symset();
		while (symp = symget())
		if ((symp->symf == 043) || (symp->symf == 044))
			printf("%s:\t%X\n",symp->symc,getword(symp->vals,DSP));
		break;

	    /*print breakpoints*/
	    case 'b': case 'B':
		printf("breakpoints\ncount%8tbkpt%24tcommand\n");
		for (bkptr = bkpthead; bkptr; bkptr=bkptr->nxtbkpt)
		if (bkptr->flag)
		{
			printf("%-8.8X", bkptr->count);
			psymoff(leng(bkptr->loc), ISYM, "%24t");
			comptr = bkptr->comm;
			while (*comptr) printc(*comptr++);
		}
		break;

	    default:
		error(BADMOD);
	}
}

printmap(s, amap)
STRING	s;
register MAP	*amap;			/* MFM */
{
	int file = amap->ufd;
	printf("%s\t`%s'\n", s,
		(file < 0 ? "-" : (file == fcor ? corfil : symfil)));
	printf("b1 = %X\t", amap->b1);
	printf("e1 = %X\t", amap->e1);
	printf("f1 = %X\n", amap->f1);
	printf("b2 = %X\t", amap->b2);
	printf("e2 = %X\t", amap->e2);
	printf("f2 = %X\n", amap->f2);
}

printregs()
{
	register short	i;		/* MFM */
	register long	v;

	printf("%s\t%x\n", "ps", (reglist[ps].rval & 0xFFFF));
	printf("%s\t%X\t", "pc", reglist[pc].rval);
	printpc(); printf("\n");
	printf("%s  %X\n\n", "sp", reglist[sp].rval);

	for (i = 0; i <= 7; i++) 	/* KERNEL_TUNE_DLB */
	{
		printf("%s  %X", reglist[i].rname, reglist[i].rval);
		while (charpos() % 22) printc(' ');
		printf("%s  %X\n", reglist[i+8].rname, reglist[i+8].rval);
	}
}

getroffs(regnam)
{
	register  REGPTR	p;
	register  STRING	regptr;
	CHAR		regnxt;

	regnxt = readchar();
	for (p = reglist; p <= &reglist[pc]; p++)
	{
		regptr = p->rname;
		if ((regnam == *regptr++) && (regnxt == *regptr))
			return(p->roffs);
	}
	lp--;

	return(-1);
}

printpc()
{
	dot = reglist[pc].rval;
	psymoff(dot, ISYM, ":");
	printins(0, ISP, chkget(dot, ISP));
	printc(EOR);
}


printcmap(s)
char *s;
{ COREMAP *thismap;
    printf("%s\t`%s'\n",s,corfil);
    for (thismap = cmaps; thismap; thismap = thismap->next_map)
    {   switch (thismap->type)
	{   case CORE_FORMAT:	printf("Core:\t");       break;
	    case CORE_KERNEL:	printf("Kernel:\t");     break;
	    case CORE_PROC:	printf("Registers:\t");  break;
	    case CORE_TEXT:	printf("Text:\t");       break;
	    case CORE_DATA:	printf("Data:\t");       break;
	    case CORE_STACK:	printf("Stack:\t");      break;
	    case CORE_SHM:	printf("SHM:\t");        break;
	    case CORE_MMF:	printf("MMF:\t");        break;
	    case CORE_EXEC:	printf("Exec:\t");       break;
	    default:		printf("???:\t");
	}
	printf("b = %X\t",thismap->b);
	printf("e = %X\t",thismap->e);
	printf("f = %X\n",thismap->f);
    }
}

