/* @(#) $Revision: 70.1 $ */     

# include <stdio.h>
# include <signal.h>
# include <ctype.h>
# include <sys/types.h>	/* needed by <dir.h> */
# include <sys/stat.h>
# include "pass0.h"
# include "header.h"

/*
 *
 *	"pass0.c" is a file containing the source for the setup program
 *	for the  Assembler.  Routines in this file parse the
 *	arguments, determining the source and object file names, invent
 *	names for temporary files, and execute the first pass of the
 *	assembler.
 *
 *	This program can be invoked with the command:
 *
 *			as [flags] [ifile] [-o ofile] [-a listfile]
 *
 *	where [flags] are optional user-specified flags,
 *	"ifile" is the name of the assembly languge source file
 *	and "ofile" is the name of a file where the object program is
 *	to be written.  If "ifile" is not specified on the command line
 *	the default is to read from stdin.  If "ofile" is not specified
 *	on the command line,
 *	it is created from "ifile" by the following algorithm:
 *
 *	     1.	If the name "ifile" ends with the two characters ".s",
 *		these are replaced with the two characters ".o".
 *
 *	     2.	If the name "ifile" does not end with the two
 *		characters ".s" length, the name "ofile" is created by
 *		appending the characters ".o" to the name "ifile".
 *
 *	     3.	For an assembler constructed name, check that the "outfile"
 *		is not going to overwrite the "infile" (because of a short
 *		filename system).
 *
 *	The global array filenames[2] to filenames[19]  entries are used to
 *	store temporary file names.
 *
 */

#define NO		0
#define YES		1

short passnbr = 0;
extern short  nerrors;


extern short tstlookup;

char	 *file = (char *) NULL;		/* assembly input file name */
unsigned int	line;	/* current input file line number */
FILE	*fdin;	/* input source */
extern	FILE	*fdout, *fdrel;		/* .o file */
char *filenames[20];

char	*infile =  (char *) NULL;
char	*outfile = (char *) NULL;
# ifdef LISTER2
char	*listfile = (char *) NULL;
# endif

char	*strcpy(), *realloc(), *malloc();
char	*copy(), *stralloc();
void	ino_clash();

short	sdopt_flag = NO;	/* sdi optimization flag */
short	p1subtract_flag = NO;
short	rflag = NO;	/* unlink pass1's input file */
short	stdinflag = NO;
short	listflag = NO;	/* generate listing */
short	sdispflag = NO;	/* generate short (68010-like) displacements */
static short	macro = NO;
short	Lflag = 0;	/* if 0, no local names go into LST.
			 * if 1, all names go into LST, even locals (set
			 * by -L flag).
			 * if 2, all locals that don't start with 'L' go
			 * into LST (set by -l flag).
			 */
short	ofile_open = NO;	/* This flag used to delete the .o file if
				 * errors detected while in pass2.  But
				 * don't want to delete if the open
				 * failed (eg, a non-writable .o already
				 * exits.
				 */
short	wflag = NO;	/* -w option causes warnings to be suppressed. */

#ifdef PIC
short	pic_flag = NO;		/* generate position independent code */
short	big_got_flag = NO;	/* global object table > 64k bytes    */
short	rpc_flag = NO;		/* RPC's for proc calls, not RPLT's   */
short	shlib_flag = NO;	/* generate sup sym table, and use    */
				/* REXT not SEG REL fixups            */
#endif

char	*tempnam();

char	*name0;

short	argindex = 1,
	flagindex = 0;

extern	long strtol();


/*
 *
 *	"getargs" processes the argument list, using getopt(3C).
 *	It locates flags (identified by a preceding minus sign ('-'))
 *	and initializes any associated flags for the assembler.
 *	If there are any file names in the argument list, then a
 *	pointer to the name is stored in a global variable for
 *	later use.
 *
 */

/* getopt(3C) externs */
extern	int optind;	/* argv index of next argument to be processed */
extern	int opterr;	/* if set to 0, suppresses error message from getopt. */
extern	char *optarg;	/* points to start of option argument */
extern	int optopt;	/* (undocumented) value of the illegal option when
			 * getopt returns a '?'
			 */
extern	int getopt();
#ifdef PIC
char * as_optstr = "imo:nOdwLlAa:N:V:";
#else
char * as_optstr = "mo:nOdwLlAa:N:V:";
#endif


getargs(xargc,xargv)
  int xargc;
  char **xargv;
{ int c;

  opterr = 0;

  /* Process option using getopt(3C).
   * Note that getopt returns EOF when it encounters the first non-option
   * argument.
   */

  while (optind<xargc) {
	switch(c = getopt(xargc, xargv, as_optstr)) {

#ifdef PIC
		case 'i':
			rpc_flag = YES;
			break;
#endif
		case 'm':
			macro = YES;
				break;
		case 'o':
			if (outfile[0])
			   werror("-o %s overwrites earlier -o %s",
				optarg, outfile);
			outfile = optarg;
			break;
		case 'n':
			sdopt_flag = NO;
			break;
		case 'O':
# ifdef SDOPT
			sdopt_flag = YES;
# else
			werror("-O option ignored");
# endif
			break;
		case 'd':
			sdispflag = YES;
			break;
		case 'w':
			wflag = YES;
			break;
		case 'L':
			if (Lflag!=0)
			   werror("-L option overwrites previous LST option");
			Lflag = 1;
			break;
		case 'l':
			if (Lflag!=0)
			   werror("-l option overwrites previous LST option");
			Lflag = 2;
			break;
/* This is Motorola -R option.  We don't currently support it.
/*		case 'R':
/*			rflag = YES;
/*			break;
 */
		case 'A':
# if defined(LISTER1) || defined(LISTER2)
			listflag = YES;
# else
			werror("list (-A) option not supported");
# endif
			break;

#ifdef LISTER2
		case 'a':
			if (listfile[0])
			   werror("-a %s overwrites earlier -a %s",
				optarg, listfile);
			listfile = optarg;
			listflag = YES;
			break;
#endif
		case 'N':
			werror("-Ns option no longer supported, ignored");
			break;
		case 'V':
			/* option to set the version (a_stamp).  Cuurently,
			 * this will override any version pseudo-op in the
			 * file (see action code for 'version' in asgram.y.
			 */
			{ 
			  int new_version = 0;
			  char * tail;	
			  new_version = strtol(optarg, &tail, 10);
			  if ((new_version < 0) || (tail==optarg)){
				aerror("invalid -V option : -V%s",optarg);
				break;
				}
			  if (*tail != '\0')
				werror("characters ignored following -V option: %s", tail);
			  if (vcmd_seen)
				werror("multiple -V commands; previous value overwritten");
			  vcmd_seen = 1;
			  version = new_version;
			}
			break;

		case '?':
			if (strchr(as_optstr, (char)optopt)) {
			   uerror("missing argument after -%c option",
				  optopt);
			   break;
			   }
			werror("unrecognized option (-%c), ignored",optopt);
			break;
		case EOF:
			/* this must be a + option or a file name */
		      { char * t;
			int ic;
			t = xargv[optind++];

			/* handle "+" options -- really only used by the
			 * "as" driver program. Warn if mismatched, otherwise
			 * silently ignore them here.
			 */
			if (*t=='+') {
			   ic = *++t;
			   switch(ic) {
#ifdef PIC
				case 'z':
					pic_flag = YES;
					shlib_flag = YES;
					break;

				case 'Z':

					pic_flag = YES;
					shlib_flag = YES;
					big_got_flag = YES;
					break;

				case 's':
					shlib_flag = YES;
					break;

#endif

				case 'x':
# ifdef M68010
					werror("+x option used with as10, ignored");
# endif
					break;
				case 'X':
# ifdef M68020
					aerror("as10 is no longer supported, use the -d option");
# endif
					break;
				default:
					werror("unrecognized option (+%c), ignored", ic);
					break;
				}
			   break;
			   }  /* if '+' */

			/* else assume it's a file name */
			if (infile != NULL)
				aerror("multiple input files");
			infile = t;
			break;
		      }
		default:
			/* can't happen ? */
			aerror("bad option (-%c)", optopt);
			break;
			break;

		}	/* switch */
	}	/* while getopt */
}

/*
 *
 *	"main" is the main driver for the assembler. It calls "getargs"
 *	to parse the argument list, set flags, and stores pointers
 *	to any file names in the argument list .
 *	If the output file was not specified in the argument list
 *	then the output file name is generated. Next the temporary
 *	file names are generated and the first pass of the assembler
 *	is invoked.
 *
 */

main(argc,argv)
	int argc;
	char **argv;
{
	register short index, count;
	FILE	*fd;

# if BFA
	bfa_setup();
# endif
	
	getargs(argc, argv);
	/* if no input file was specified, input will be read from stdin */
	if ( (infile == (char *)NULL) || (strcmp(infile,"-") == 0) ) {
		infile = "-";	/* for m4 */
		stdinflag = YES;
		}
	/* else "normal" file */
	else  {  

		/* Check to see if input file exits */
		if ((fd = fopen(infile,"r")) != NULL)
		   fclose(fd);
		else {
		   aerror("Input file (%s) nonexistent or not readable\n", infile);
		   }
		}
	
	/* Handle output filename.  
	 *  If -o <outfile> was specified, it must not be of the form
	 *	*.[cs].
	 *  Also it must not be of form [-+]* since this causes confusion
	 *  with options.
	 *  If no -o option, generate a filename. 
	 */
	count = strlen(outfile);
	if ((count >= 2 && outfile[count-2]=='.' && (outfile[count-1]=='s' ||
		outfile[count-1]=='c')) || outfile[0]=='-' || outfile[0]=='+')
		aerror("illegal output filename: '%s'", outfile);

	if (outfile == NULL) {
	   if (stdinflag == YES)
		outfile = "a.out";
	   else {
		for(index=0,count=0;infile[index]!='\0';index++,count++)
			if(infile[index]=='/')
				count = -1;
		outfile = copy(infile+index-count);
		if(count>=2 && outfile[count-2]=='.' &&
		   outfile[count-1]=='s')
			outfile[count-1]='o';
		else {
			outfile = realloc(outfile, count+3);
			strcpy(outfile+count,".o");
			}
		}
	}
	if (stdinflag == NO)
	   ino_clash(infile, outfile);

# ifdef LISTER2
	if (listflag) {
	   if (stdinflag) {
		werror("list option (-A or -a) ignored when input is stdin");
		listflag = NO;
		}
	   /* Check list filename.  
	    *  If -a <listfile> was specified, it must not be of the form
	    *	*.[ocs]. Just for safety: so user doesn't accidentally
	    *	zap their source file, for example.
	    *   Also can't be of form [-+]* since this should be options.
	    */
	   count = strlen(listfile);
	   if ((count >= 2 && listfile[count-2]=='.' && (listfile[count-1]=='s' 
		|| listfile[count-1]=='c' || listfile[count-1]=='o')) ||
		listfile[0]=='-' || listfile[0]=='+')
		aerror("illegal list filename: '%s'", listfile);
	   if (count>0)
		ino_clash(infile, listfile);
	   filenames[19] = (count>0)?listfile : NULL;
	   }
# endif

	filenames[2] = tempnam(TMPDIR,TMPFILE1);	/* text	*/
	filenames[3] = tempnam(TMPDIR,TMPFILE2);	/* data	*/
	filenames[4] = tempnam(TMPDIR,TMPFILE3);	/* gntt	*/
	filenames[5] = tempnam(TMPDIR,TMPFILE4);	/* slt	*/
	filenames[6] = tempnam(TMPDIR,TMPFILE5);        /* vt   */
	filenames[7] = tempnam(TMPDIR,TMPFILE6);	/* text-rel */
	filenames[8] = tempnam(TMPDIR,TMPFILE7);	/* data-rel */
# ifdef LISTER2
	if (listflag) filenames[9] = tempnam(TMPDIR,TMPFILE8);	/* lister info */
# endif
	filenames[10] = tempnam(TMPDIR,TMPFILE9);	/* lntt */
	filenames[11] = tempnam(TMPDIR,TMPFILE10);	/* xt   */
	filenames[12] = tempnam(TMPDIR,TMPFILE11);	/* module info */

	if (macro) {
		/* tell pass1 to unlink its input when through */
		rflag = YES;
		callm4();
	}

	file = copy(infile);
	filenames[0] = infile;
	filenames[1] = outfile;

	passnbr = 1;
	aspass1();
	passnbr = 2;	/* Note currently don't distinguish sdopt pass
			 * and the pass2.  Sdopt needs to act like pass2
			 * in eval_tdiff and that's the only place where
			 * passnbr is currently used.
			 */
# ifdef SDOPT
	if (sdopt_flag==YES && nerrors==0)  sdopass();
# endif
	if (nerrors == 0) aspass2();
	if (ofile_open && nerrors) unlink(filenames[1]);
# ifdef LISTER2
	if (listflag && nerrors==0) lister();
# endif
	if (rflag) unlink(filenames[0]);
	/* These fcloses used to be in the indivual passes, but now
	 * leave the file descriptors open to the end so can reuse
	 * the descriptors when listing, avoiding extra fopen/setvbuf/
	 * fclose sequences.
	 */
	fclose(fdin);
	fclose(fdrel);
	fclose(fdout);
	EXIT(nerrors?1:0);
} /* main */


callm4()
{
	static char
		*av[] = { "m4", 0, 0, 0};


	av[1] = infile;
	name0 = tempnam(TMPDIR,TMPFILE0); 		/* m4 output file */
	if (callsys(M4, av, name0) != 0) {
		unlink(name0);
		fprintf(stderr,"M4 call failed\n");
		EXIT(100);
	}
	infile = name0;
	return;
} /* callm4 */

callsys(f,v,o)
	char f[], *v[];
	char *o;	/* file name, if any, for redirecting stdout */
{
	int t, status;

	if ((t=fork())==0) {
#ifdef  BBA
#pragma BBA_IGNORE
#endif
		if (o != NULL) {	/* redirect stdout */
			if (freopen(o, "w", stdout) == NULL) {
				fprintf(stderr,"Can't open %s\n", o);
				EXIT(100);
			}
		}
		execv(f, v);
		fprintf(stderr,"Can't find %s\n", f);
		EXIT(100);
	} else
		if (t == -1) {
#ifdef  BBA
#pragma BBA_IGNORE
#endif
			fprintf(stderr,"Fork of new process failed, Try again\n");
			return(100);
		}
	while(t!=wait(&status));
	if ((t=(status&0377)) != 0) {
		if (t!=SIGINT)		/* interrupt */
#ifdef  BBA
#pragma BBA_IGNORE
#endif
			{
			fprintf(stderr,"status %o\n",status);
			fprintf(stderr,"Fatal error in %s\n", f);
			}
		EXIT(100);
	}
	return((status>>8) & 0377);
} /* callsys */

char *
copy(s)
register char *s;
{
	register char *ns;

	ns = stralloc(strlen(s));
	return(strcpy(ns, s));
}


char *
stralloc(n)
int	n;
{
	char *malloc();
	register char *s;

	s = malloc(n+1);
	if (s==NULL) {
#ifdef  BBA
#pragma BBA_IGNORE
#endif
		aerror("out of space", (char *)NULL);
	}
	return(s);
}



/* The following routine is called to check that the 'src' and 'obj'
 * do not point to the same file.
 * This code is taken from cc.c.
 */

void ino_clash(src,obj)
char *src;
char *obj;
{

    ino_t        src_ino, obj_ino;
    struct stat  r,s;

    if (stat (src, &r)) return;
    src_ino = r.st_ino;
    if (stat (obj, &s)) return;
    obj_ino = s.st_ino;
    if (src_ino == obj_ino)
    {
	/*
	 * the inode number are the same, but are they on the
	 * same device?
	 */
	if (r.st_dev == s.st_dev)
	 {
	     /*
	      * hmm, the device seems to be the same, but are the
	      * files on the same node?
	      */
	     if (!(s.st_remote ^ r.st_remote))
	      {
		  /*
		   * either both remote or both local.
		   * if remote and network special files
		   * are the same return error
		   * if local look at the cluster ID of the files
		   * machine in case this is Diskless
		   */
		  if(r.st_remote)
		   {
		       if ( (s.st_netdev == r.st_netdev) &&
			   (s.st_netino && r.st_netino))
				{
			    aerror("%s will over-write existing %s",obj,src);
			}
		   }
		  else
		   {
		       /* maybe we're running DISKLESS ? */
		       if (r.st_cnode == s.st_cnode &&
			   r.st_realdev == s.st_realdev)
			{
			    aerror("%s will over-write existing %s",obj,src);
			}
		   }
	      }
	 }
    }
}


# if BFA
char bfastring[150];
int getpid();

bfa_setup()
{
  sprintf(bfastring, "-r%s/%s.%d %s", BFADIR, BFAPREFIX, getpid(), BFAPREFIX);
}
# endif

