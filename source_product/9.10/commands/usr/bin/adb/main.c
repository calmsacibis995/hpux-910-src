/* @(#) $Revision: 66.1 $ */    

/****************************************************************************

	DEBUGGER - main entry loop 

****************************************************************************/
#include "defs.h"
#include <setjmp.h>
#include <signal.h>
#include <sys/utsname.h>

MSG		NOEOR;
short		mkfault;
FILE		*infile = stdin;
FILE		*outfile = stdout;
#ifdef KSH
char		*getenv();
CHAR		*prompt;
#endif /* KSH */
CHAR		*lp;
int		maxoff = 32768;
int		maxpos = 80;
void		(*sigint)();
void		(*sigqit)();
int		wtflag;
long		maxfile = 1L<<24;
long		txtsiz;
long		datsiz;
long		datbas;
long		stksiz;
STRING		errflg;
int		exitflg;
short		magic;
long		entrypt;
CHAR		lastc;
int		eof;
jmp_buf		env;
int		lastcom;
long		var[36];
STRING		symfil;
STRING		corfil;
CHAR		printbuf[];
CHAR		*printptr;
int		argcount;
int		adb_fcntl_flags;
int		sub_fcntl_flags;
/* unneeded ?? */
int		umm_flag;
int		w310_flag;
int		m68020_flag;
/* ^^^^^^^^^^^ */
int		fpa_addr;
int		real_bad();

long
round(a, b)
long	a;				/* MFM */
register b;				/* MFM */
{
	return(((a + b - 1)/b) * b);
}

chkerr()
{
	if (errflg || mkfault) error(errflg);
}

error(n)
char	*n;
{
	errflg = n;
	iclose(); oclose();
	longjmp(env);
}

catch()
{
	signal(SIGILL, SIG_DFL);
	longjmp(env);
}

void fault(sig)
{
	signal(sig, fault);
	fseek(infile, 0L, 2);
	error("\nadb");
}

#pragma OPT_LEVEL 1

main(argc, argv)
int	argc;
char	**argv;
{
	ioctl(0, TCGETA, &adbtty);
	adb_fcntl_flags = fcntl(0, F_GETFL);
	ioctl(0, TCGETA, &subtty);
	sub_fcntl_flags = adb_fcntl_flags;

	while (argc > 1)
		if (strcmp("-w", argv[1])==0)		/* MFM */
		{
			wtflag = 2;
			argc--; argv++;
		}
		else break;
	if (argc > 1) symfil = argv[1];
	if (argc > 2) corfil = argv[2];
	argcount = argc;

	/* determine which kernel is running */
	identify_kernel();
	/* find the address of the U area from kernel symbol table */
	init_uarea_address();

	if (Dragon_flag) {
		asm("	mov.l	&fpa_loc,_fpa_addr");
		var[VARF] = 1;
		var[VARR] = 2;
	};

	setbout();				/* setup a.out file       */
	setcor();				/* setup core file   */

	var[VARB] = datbas;			/* setup variables	  */
	var[VARD] = datsiz;
	var[VARE] = entrypt;
	var[VARM] = magic;
	var[VARS] = stksiz;
	var[VART] = txtsiz;

	printf("ready\n");
	if ((sigint = signal(SIGINT, SIG_IGN)) != SIG_IGN)
	{
		sigint = fault;
		signal(SIGINT, fault);
	}
	sigqit = signal(SIGQUIT, SIG_IGN);
	signal(SIGFPE,real_bad);
	setjmp(env);

#ifdef KSH
	prompt = getenv("ADBPS1");
	if ( !prompt ) prompt = "!% ";
	hist_open();
#endif /* KSH */

	while(1)
	{
		flushbuf();
		if (errflg)
		{
			printf("%s\n", errflg);
			exitflg = (int)errflg;
			errflg = 0;
		}
		if (mkfault)
		{
			mkfault=0; printc(EOR); prints(DBNAME);
		}
#ifdef KSH
		if (infile == stdin)
			pr_prompt(prompt);
#endif /* KSH */
		lp = 0; rdc(); lp--;
		if (eof)
		{
			if (infile != stdin)
			{
				iclose(); eof=0;
				longjmp(env);
			} else done();
		} else exitflg = 0;
		command(0, lastcom);
		if (lp && (lastc != EOR)) error(NOEOR);
	}
}

#pragma OPT_LEVEL 2

done()
{
	endpcs();
	exit(exitflg);
}

	/* determine which kernel is running. 
	 * Check for 68020 first (by trapping on a 20 instruction)
	 * If no 68020, then look at uname.
	 * If the uname is 9000/310 then it is a WOPR 310
	 * else it is an UMM kernel
	 */

#pragma OPT_LEVEL 1

identify_kernel()
{
	static struct utsname name;

	umm_flag = 0;
	w310_flag = 0;

	signal(SIGILL, catch);
	if (setjmp(env))
		mc68020 = FALSE;
	else
	{
		/* trapf - 68020 only instruction */
		asm ("
			tf	# short 0x51FC	#
		    ");
		mc68020 = TRUE;
	}

/* DEAD CODE at 8.0 == remove ?? */

	if (!mc68020)
	{
		if ( uname(&name) < 0 )
		{
			fprintf (stderr, "cannot determine which kernel is running\n");
			exit(1);
		}
		if (strncmp(name.machine, W310KERNEL, W310KERNEL_LEN) == 0)
			w310_flag = 1;
		else
			umm_flag = 1;
	}

/* END OF DEAD CODE */
	
	/* Check presence of float card and 68881 */

	/* float_soft will indicate presence of hardware float card */
	asm ("
		move.w float_soft,_float_soft
	    ");
	
	if (mc68020) 	/* 881 can only be on Bobcat-20 */
	{
		/* flag_68881 will indicate presence of the 881 co-processor */
		asm ("
			move.w flag_68881,_mc68881
	    	");

		/* Dragon_flag will indicate presence of the Dragon floating
		   point accelerator */
		asm ("
			move.w flag_fpa,_Dragon_flag
	    	");
	}
}

#pragma OPT_LEVEL 2

	/* find the address of the U area from kernel symbol table */
init_uarea_address()
{
	static struct nlist nl[] = { { "_u", }, { (char *)0, } };

	nlist ("/hp-ux", nl);

	uarea_address = nl[0].n_value;
	if (uarea_address == 0)
		fprintf (stderr, "cannot read address of U area from /hp-ux\n");
}

