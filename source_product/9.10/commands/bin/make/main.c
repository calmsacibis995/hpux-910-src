/* @(#) $Revision: 66.2 $ */     

# include "defs"
#ifdef NLS16
#include <nl_ctype.h>
#endif
#ifdef NLS || NLS16
#include <locale.h>
#endif

/*
command make to update programs.
Flags:	'd'  print out debugging comments
	'p'  print out a version of the input graph
	's'  silent mode--don't print out commands
	'f'  the next argument is the name of the description file;
	     makefile is the default
	'i'  ignore error codes from the shell
	'S'  stop after any command fails (normally do parallel work)
	'n'   don't issue, just print, commands
	't'   touch (update time of) files but don't issue command
	'q'   don't do anything, but check if object is up to date;
	      returns exit code 0 if up to date, -1 if not
*/

char makefile[] = "makefile";
unsigned char Nullstr[] = "";
char Makefile[] =	"Makefile";
unsigned char RELEASE[] = "RELEASE";
CHARSTAR badptr = (CHARSTAR)-1;

NAMEBLOCK mainname ;     /* first target name in makefile */
NAMEBLOCK firstname;     /* target name to be compiled */
LINEBLOCK sufflist;     
#ifdef	PATH             /* PATH feature added by kernel for internal */
LINEBLOCK pathlist;      /* use - this is NOT shipped - it allows a   */
#endif	PATH             /* path list of alternate places to find     */
                         /* dependency files                          */
VARBLOCK firstvar;       /* linked list of targets, environment  */
                         /* variables, and macros, including the */
                         /* built-in ones.  Changes dynamically  */
PATTERN firstpat;        
OPENDIR firstod;

/**********************/
/* SET DEFAULT VALUES */
/**********************/

#include <signal.h>
int sigivalue=0;
int sigqvalue=0;
int sigtvalue=0;
int sighvalue=0;
int waitpid=0;

int Mflags=MH_DEP;  /* something about an old command existing? */
int ndocoms=0;
int okdel=YES;

CHARSTAR prompt=(unsigned char *) "\t";	/* other systems -- pick what you want */
unsigned char junkname[20];
unsigned char funny[256];  /*array indexed by all possible character values*/
                           



unsigned char Makeflags[]="MAKEFLAGS";


/**********************/
/* MAIN FUNCTION      */
/**********************/


main(argc,argv)
int argc;
CHARSTAR argv[];

/* The argument string argv is parsed in numerous places.  Whenever   */
/* one of the arguments is parsed, the first character of the arg     */
/* string is set to 0, marking that argument as processed.            */


{
	register NAMEBLOCK p;
	int i, j;
	int descset, nfargs;   /* descset counts how many -f makefiles */
                               /* nfargs counts ?                      */
        TIMETYPE tjunk;
	unsigned char c;
	CHARSTAR s, *e;
#ifdef NLS16
	CHARSTAR getenv();
#endif

#ifdef unix
	int intrupt();



#endif

#ifdef METERFILE
	meter(METERFILE);
#endif

#if defined NLS || defined NLS16	/* initialize to the right language */
    if (!setlocale(LC_ALL, "")) {
        fprintf(stderr,"%s\n", _errlocale(argv[0]));
    }

#endif NLS || NLS16

	descset = 0;


/* set slots in funny which correspond to metacharacters */

	for(s = (unsigned char *) "#|=^();&<>*?[]:$`'\"\\\n" ; *s ; ++s)
		funny[*s] |= META;

/* set slots in funny which correspond to terminal characters */

	for(s = (unsigned char *) "\n\t :=;{}&>|" ; *s ; ++s)
		funny[*s] |= TERMINAL;
	funny['\0'] |= TERMINAL;

	TURNON(INTRULE);	/* Default internal rules, turned on */

/****************************/
/*  Set command line flags  */
/****************************/

	getmflgs();		/* Init $(MAKEFLAGS) variable     */
	setflags(argc, argv);   /* add 'c' option to $(MAKEFLAGS) */
                                /* and handle any 'f' option args */ 

	setvar("$","$");        /* set value of $ in firstvar     */


/****************************/
/*  Read command line "="   */
/*  type args and make them */
/*  readonly.               */
/****************************/

/* the following for loop is loaded - a lot happens in it */

	TURNON(INARGS|EXPORT);
	if(IS_ON(DBUG))printf("Reading \"=\" type args on command line.\n");
	for(i=1; i<argc; ++i)
		if(argv[i]!=0 && argv[i][0]!=MINUS && (eqsign(argv[i]) == YES) )
			argv[i] = 0;
	TURNOFF(INARGS|EXPORT);

/****************************************/
/* Read internal definitions and rules  */
/* rdd1 calls the yyparse routine which */
/* parses the makefile                  */
/****************************************/

	if( IS_ON(INTRULE) )
	{
		if(IS_ON(DBUG))printf("Reading internal rules.\n");
		rdd1(NULL);     
	}
	TURNOFF(INTRULE);

/****************************************/
/* Read environment args.  Let file args*/
/* which follow override unless 'e' in  */
/* MAKEFLAGS variable is set.           */
/****************************************/

	if( any( (varptr(Makeflags))->varval, 'e') )
		TURNON(ENVOVER);
	if(IS_ON(DBUG))printf("Reading environment.\n");
	TURNON(EXPORT);
	readenv();      /* environment variables read into firstvar */
	TURNOFF(EXPORT|ENVOVER);

/**************************************/
/*  Read command line "-f" arguments  */
/**************************************/

	rdmakecomm();  /* only effective if make compiled with PWB */

	for(i = 1; i < argc; i++)
		if( argv[i] && argv[i][0] == MINUS && argv[i][1] == 'f' && argv[i][2] == CNULL)
		{
			argv[i] = 0;
			if(i >= argc-1 || argv[i+1] == NULL)
				fatal("No description argument after -f flag");
			if( rddescf(argv[++i], YES) )
				fatal1("Cannot open %s", argv[i]);
			argv[i] = 0;
			++descset;  /* count how many makefiles to read */
		}

/**************************************/
/* If no command line "-f" args then  */
/* look for some form of "makefile"   */
/* and parse it (parsing embedded in  */
/* rddescf and routines it calls)     */
/**************************************/

	if( !descset )
#ifdef unix

/* Default search order for makefiles is:          */
/* makefile, Makefile, s.makefile, and s.Makefile  */ 

		if( rddescf(makefile, NO))
                   if( rddescf(Makefile, NO))
                      if( rddescf(makefile, YES))
			rddescf(Makefile, YES);

#endif
#ifdef gcos
		rddescf(makefile, NO);
#endif


	if(IS_ON(PRTR)) printdesc(NO);  /* -p print macro option   */
                                        /* doesn't print open dirs */
                                        /* or files to be compiled */


/* turn on flags for special macros in makefile */

	if( srchname(".IGNORE") ) TURNON(IGNERR);
	if( srchname(".SILENT") ) TURNON(SIL);
	if(p=srchname(".SUFFIXES")) sufflist = p->linep;
	if( !sufflist ) fprintf(stderr,"No suffix list.\n");
#ifdef	PATH
	if(p=srchname(".PATH")) pathlist = p->linep;
	if( !pathlist ) fprintf(stderr,"No path list.\n");
#endif	PATH

#ifdef unix
	sigivalue = (int)signal(SIGINT,1) & 01;
	sigqvalue = (int)signal(SIGQUIT,1) & 01;
	sigtvalue = (int)signal(SIGTERM,1) & 01;
	sighvalue = (int)signal(SIGHUP,1) & 01;
	enbint(intrupt);
#endif


/*********************************************/
/* process all remaining arguments from argc */
/* the only things left should be target     */
/* names to make - these are put into the    */
/* hash table list if they are not already   */
/* there and nfargs is incremented           */
/* THIS IS WHERE THE MAKEFILE IS ACTUALLY    */
/* EXECUTED - doname does it                 */ 
/*********************************************/

	nfargs = 0;

	for(i=1; i<argc; ++i)
		if((s=argv[i]) != 0)
		{
			if((p=srchname(s)) == 0)
			{
				p = makename(s);
			}
			++nfargs;
			doname(p, 0, &tjunk);
			if(IS_ON(DBUG)) printdesc(YES);
		}

/*************************************************/
/* If no file arguments  (target names) have been*/
/* encountered, make the first name encountered  */
/* that doesn't start with a dot   (DEFAULT)     */
/*************************************************/

	if(nfargs == 0)
		if(mainname == 0)
			fatal("No arguments or description file");
		else
		{
			doname(mainname, 0, &tjunk);
			if(IS_ON(DBUG)) printdesc(YES);
		}

	exit(0);
}

/*********END OF MAIN FUNCTION************/



#ifdef unix
intrupt(sig)
int sig;
/****************************************************************/
/* Handles residual file removal when an interrupt terminates   */
/* the make before successful completion. The conditions for    */
/* a target file being removed are: deletion is set, the        */
/* makefile was actually executing, touch is off, the current   */
/* file being made exists and is in the hashtable, the actual   */
/* file exists, it has read (or is it write) permission, and    */
/* the isprecious flag is off, and if it is not a directory,    */
/* then the file is removed.  (Lois, check the validity of all  */
/* the criteria.  Some of them are not obvious from the calls   */
/* below.  Also, the interface to access is changed, and the    */
/* number 02 needs to be changed to a mnemonic.  CHANGE.        */
/****************************************************************/
{
	CHARSTAR p;
	NAMEBLOCK q;

	if(okdel && IS_OFF(NOEX) && IS_OFF(TOUCH) &&
	   (p=varptr("@")->varval) && 
	   (q=srchname(p)) && 
	   (exists(q)>0) &&
	   access(p,02) &&
	   !isprecious(p) )
	{
		if(isdir(p))
			fprintf(stderr, "\n*** %s NOT REMOVED.",p);
		else if(unlink(p) == 0)
			fprintf(stderr, "\n***  %s removed.", p);
	}
	if(junkname[0])             /* where does this get set? */
		unlink(junkname);
	fprintf(stderr, "\n");
	kill(getpid(),sig);        /* is this POSIX stuff?  */
	exit(2);
}




isprecious(p)
CHARSTAR p;
/*******************************************************************/
/* Searches the hashtable for the .PRECIOUS name.  If not found    */
/* a NO is returned.  If found, each line (linep) and its related  */
/* dependencies (depp) are examined and their file names compared  */
/* against p.  If p if found in this chain of filenames, YES is    */
/* returned; otherwise, the for loop finishes and a NO is returned */
/* The exact contents of the hashtable, linep, and depp are not    */
/* yet fully understood by this writer (Lois)                      */
/*                                                                 */
/* In effect, if the target p has been designated as precious      */
/* (not to be removed), the routine returns YES (it is precious);  */
/* otherwise it returns NO (it is not precious).                   */
/*******************************************************************/
{
	register NAMEBLOCK np;
	register LINEBLOCK lp;
	register DEPBLOCK dp;

	if(np = srchname(".PRECIOUS"))
	    for(lp = np->linep ; lp ; lp = lp->nextline)
		for(dp = lp->depp ; dp; dp = dp->nextdep)
			if(equal(p, dp->depname->namep))
				return(YES);

	return(NO);
}


enbint(k)
int (*k)();
/*******************************************************************/
/* enbint is mnemonic for "enable interrupt".  It takes one        */
/* parameter, a function with no arguments which returns an        */
/* integer.  Once the workings of signal are better understood     */
/* by the writer (Lois) this comment can be made more meaningful.  */
/* POSIX stuff has been added here.  Perhaps the POSIX editor      */
/* can shed some light on this little mystery.                     */
/*******************************************************************/
{
	if(sigivalue == 0)
		signal(SIGINT,k);
	if(sigqvalue == 0)
		signal(SIGQUIT,k);
	if(sigtvalue == 0)
		signal(SIGTERM,k);
	if(sighvalue == 0)
		signal(SIGHUP,k);
}
#endif

extern CHARSTAR builtin[];  /*don't know what these are yet */


CHARSTAR *linesptr=builtin;

FILE * fin;
int firstrd=0;


rdmakecomm()
{
/************************************************************/
/* this becomes meaningful only if Programmers Workbench is */
/* defined when the make code is compiled.                  */ 
/************************************************************/
#ifdef PWB
	register CHARSTAR nlog;
	char s[256];
	CHARSTAR concat(), getenv();

	if(rddescf( concat((nlog=getenv("HOME")),"/makecomm",s), NO))
		rddescf( concat(nlog,"/Makecomm",s), NO);

	if(rddescf("makecomm", NO))
		rddescf("Makecomm", NO);
#endif
}

                         /* the two variables below used by rdd1     */
extern int yylineno;     /* counter for the parsing routine, yyparse */
extern CHARSTAR zznextc; /* counter for the parsing routine, yyparse */
rddescf(descfile, flg)
CHARSTAR descfile;
int flg;			/* if YES try s.descfile */
/******************************************************************/
/* rddescf is mnemonic for "read description file" which is the   */
/* makefile being parsed.  This procedure determines if standard  */
/* in or a file is to be used for input and deals with opening    */
/* the appropriate input source  (file opening checking, etc).    */
/* This is where the sccs files are found and opened as well.     */
/* Once this is done, rdd1 is called with the correct input       */
/* source. rdd1 actually does the parsing of the makefile         */
/******************************************************************/
{
	FILE * k;

/* standard in specified as input */

	if(equal(descfile, "-"))
		return( rdd1(stdin) );

/* a file is specified as input */
retry:
	if( (k = fopen(descfile,"r")) != NULL)
	{
		if(IS_ON(DBUG))printf("Reading %s\n", descfile);
		return( rdd1(k) );
	}

/* the regular file was not found */

	if(flg == NO)        /* don't look for a s.file */
		return(1);

/* look for an SCCS s.file */
/* not sure how the following line works yet */
	if(get(descfile, CD, varptr(RELEASE)->varval) == NO)
		return(1);
	flg = NO;
	goto retry;   /* not sure why this is a goto loop */

}




rdd1(k)
FILE * k;
/****************************************************************/
/* This is a deceptively simple looking function.  When it is   */
/* finished, however, the makefile ( * k) will have been read   */
/* and parsed into all the internal data structures in make     */
/* so that it can be executed.  yyparse does all this dirty     */
/* work.  yyparse is a routine compiled into the make code      */
/* by yacc from the file gram.y.  See gram.y for all the lurid  */
/* details of how the makefile gets parsed.                     */
/*                                                              */
/* This function initializes some extern variables for yyparse  */
/* and then calls yyparse.  It then closes the input file if it */
/* is not stdin.                                                */
/* A 0 returns if all is well; fatal indicates failure          */
/****************************************************************/
{
	fin = k;            /*global EXTERN variable for yyparse*/
	yylineno = 0;       /*misc.c extern variable for yyparse*/
	zznextc = 0;        /*main.c extern variable for yyparse*/

	if( yyparse() )
		fatal("Description file error");

	if ( fin != NULL && fin != stdin )
		fclose(fin);

	return(0);
}

printdesc(prntflag)
int prntflag;
/******************************************************************/
/* This function is called if the -p flag is set, and prints out  */
/* all the macro values in the environment of the make invocation */
/******************************************************************/
{
	NAMEBLOCK p;
	DEPBLOCK dp;
	VARBLOCK vp;
	OPENDIR od;
	SHBLOCK sp;
	LINEBLOCK lp;

#ifdef unix
	if(prntflag)
	{
		fprintf(stderr,"Open directories:\n");
		for(od=firstod; od!=0; od = od->nextopendir)
			fprintf(stderr,"\t%d: %s\n", od->dirp->dd_fd, od->dirn);
/*                                        ^^         ^^^^^^^^^^^^^^^
 * For some reason, we output the file descriptor of each open directory.
 * We have this hard coded from the DIR structure definition in <ndir.h>
 * If make won't compile due to this, change the od->dirp-><whatever>
 * to the "new", "right" thing.
 */
	}
#endif

	if(firstvar != 0) fprintf(stderr,"Macros:\n");
	for(vp=firstvar; vp!=0; vp=vp->nextvar)
		if(vp->v_aflg == NO)
			printf("%s = %s\n" , vp->varname , vp->varval);
		else
		{
			CHAIN pch;

			fprintf(stderr,"Lookup chain: %s\n\t", vp->varname);
			for(pch = (CHAIN)vp->varval; pch; pch = pch->nextchain)
				fprintf(stderr," %s",
					((NAMEBLOCK)pch->datap)->namep);
			fprintf(stderr,"\n");
		}

	for(p=firstname; p!=0; p = p->nextname)
		prname(p, prntflag);
	printf("\n");
	fflush(stdout);
}

prname(p, prntflag)
register NAMEBLOCK p;
{
	register LINEBLOCK lp;
	register DEPBLOCK dp;
	register SHBLOCK sp;

	if(p->linep != 0)
		printf("\n\n%s:",p->namep);
	else
		fprintf(stderr, "\n\n%s", p->namep);
	if(prntflag)
	{
		fprintf(stderr,"  done=%d",p->done);
	}
	if(p==mainname) fprintf(stderr,"  (MAIN NAME)");
	for(lp = p->linep ; lp!=0 ; lp = lp->nextline)
	{
		if( dp = lp->depp )
		{
			fprintf(stderr,"\n depends on:");
			for(; dp!=0 ; dp = dp->nextdep)
				if(dp->depname != 0)
				{
					printf(" %s", dp->depname->namep);
					printf(" ");
				}
		}
		if(sp = lp->shp)
		{
			printf("\n");
			fprintf(stderr," commands:\n");
			for( ; sp!=0 ; sp = sp->nextsh)
				printf("\t%s\n", sp->shbp);
		}
	}
}


setflags(ac, av)
int ac;                /* argc from the main function */ 
CHARSTAR *av;          /* argv from the main function */
/*****************************************************************/
/* This function parses all the "-*" options specified in argv   */
/* and feeds them one at a time to optswitch, which sets         */
/* appropriate flags and adds the option characters to the       */
/* Makeflags value in firstvar.  -f options are marked for       */
/* further processing elsewhere.                                 */
/*****************************************************************/
{
	register int i, j;              /* loop counters */
	register unsigned char c;       /* option character */
	int flflg=0;			/* flag to note `-f' option. */

	for(i=1; i<ac; ++i)       /* start with first argument */
	{
		if(flflg)         /* reset -f flag */
		{
			flflg = 0;
			continue;
		}
		if(av[i]!=0 && av[i][0]==MINUS)
		{
			if(any(av[i], 'f'))
				flflg++;
			for(j=1 ; (c=av[i][j])!=CNULL ; ++j)
				optswitch(c);
			if(flflg)
				av[i] = (unsigned char *) "-f";
			else
				av[i] = 0;
		}
	}
}


optswitch(c)
register unsigned char c;
/*****************************************************************/
/* Handles one option letter fed to it from the parsing of the   */
/* argv values.  These are the documented options on the make    */
/* man page.  For each option, certain global flags are set and  */
/* calls setmflags which actually adds the character to the      */
/* Makeflags variable in firstvar.                               */
/*****************************************************************/
{

	switch(c)
	{

	case 'e':                 /* environment override flag */
		setmflgs(c);
		break;

	case 'd':                 /* debug flag */
		TURNON(DBUG);
		setmflgs(c);
		break;

	case 'p':                 /* print description */
		TURNON(PRTR);
		break;

	case 's':                 /* silent flag */
		TURNON(SIL);
		setmflgs(c);
		break;

	case 'i':                 /* ignore errors */
		TURNON(IGNERR);
		setmflgs(c);
		break;

	case 'S':                 /* default - quit and don't proceed */
		TURNOFF(KEEPGO);  /* if work on current entry fails   */
		setmflgs(c);
		break;

	case 'k':                 /* abandon work on current entry if */
		TURNON(KEEPGO);   /* it fails, but continue work on   */
		setmflgs(c);      /* other entries                    */
		break;

	case 'n':                 /* do not exec any commands, just print */
		TURNON(NOEX);
		setmflgs(c);
		break;

	case 'r':                 /* turn off internal rules */
		TURNOFF(INTRULE);
		break;

	case 't':                 /* touch target files only */
		TURNON(TOUCH);
		setmflgs(c);
		break;

	case 'q':                /* return indication of whether target */ 
		TURNON(QUEST);   /* file is up-to-date                  */
		setmflgs(c);
		break;

	case 'g':               /* turn default $(GET) of files not found */
		TURNON(GET);    /* not standard or documented option      */
		setmflgs(c);
		break;

	case 'm':               /* print memory map */
		TURNON(MEMMAP); /* nonstandard undocumented option */
		setmflgs(c);    /* don't know what it does */
		break;

	case 'b':               /* use MH version of test for cmd existence*/
		TURNON(MH_DEP); /* nonstandard option (Version 7)          */
		setmflgs(c);
		break;
	case 'B':               /* turn off -b flag */
		TURNOFF(MH_DEP); /* nonstandard, undocumented option */
		setmflgs(c);
		break;

	case 'f':	/* Named makefile; already handled by setflags(). */
		break;

	default:
		fatal1("Unknown flag argument %c", c);
	}
}

 


getmflgs()
/***************************************************************/
/* getmflgs() sets the cmd line flags into an EXPORTED variable*/
/* for future invocations of make to read.                     */
/* (This comment is questionable - I have not deciphered this  */
/* code yet - Lois)                                            */
/***************************************************************/
{
	register VARBLOCK vpr;        /*pointer to Makeflags varblock*/
	register CHARSTAR *pe;        /*counter pointer for environ string*/
	register CHARSTAR p;

	vpr = varptr(Makeflags);        /*get pointer to Makeflags block */
	setvar(Makeflags, "ZZZZZZZZZZZZZZZZ");  /*beats me why*/
	vpr->varval[0] = CNULL;                 /*ditto*/
	vpr->envflg = YES;                   /*evidently flag for export*/
	vpr->noreset = YES;                  /*not sure*/
	optswitch('b');             /*force compatibility with old makefiles*/

/* environ is a global C variable inherited by all command invocations */
/* it contains all environment variable assignments from the environ-  */
/* in which the make was invoked                                       */

	for(pe = environ; *pe; pe++)    /*look at every environment value*/
	{
		if(sindex(*pe, "MAKEFLAGS=") == 0)   /*not figured out yet*/
		{
			for(p = (*pe)+sizeof Makeflags; *p; p++)
				optswitch(*p);
			return;
		}
	}
}


setmflgs(c)
register unsigned char c;
/*************************************************************/
/*  setmflgs(c) sets up the cmd line input flags for EXPORT. */
/*  It adds the given option letter (c) to the Makeflags     */
/*  variable in firstvar if it isn't already there.          */ 
/*************************************************************/
{
	register VARBLOCK vpr;
	register CHARSTAR p;

	vpr = varptr(Makeflags);       /*get the Makeflags pointer*/
	for(p = vpr->varval; *p; p++)
	{
		if(*p == c)            /*if option letter already there*/
			return;        /* don't do anything */ 
	}
	*p++ = c;                      /* add the option character */
	*p = CNULL;                    /* and null terminate the string */
}
