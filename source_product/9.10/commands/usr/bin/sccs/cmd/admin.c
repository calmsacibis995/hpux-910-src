/* @(#) $Revision: 66.1 $ */     
static char *HPUX_ID = "@(#) $Revision: 66.1 $";
#ifdef NLS
#define NL_SETN 1
#include      <msgbuf.h>
#include      <locale.h>
#else
#define nl_msg(i, s) (s)
#endif
# include	<ndir.h>
# include	"../hdr/defines.h"
# include	"../hdr/had.h"


/*
	Program to create new SCCS files and change parameters
	of existing ones. Arguments to the program may appear in
	any order and consist of keyletters, which begin with '-',
	and named files. Named files which do not exist are created
	and their parameters are initialized according to the given
	keyletter arguments, or are given default values if the
	corresponding keyletters were not supplied. Named files which
	do exist have those parameters corresponding to given key-letter
	arguments changed and other parameters are left as is.

	If a directory is given as an argument, each SCCS file within
	the directory is processed as if it had been specifically named.
	If a name of '-' is given, the standard input is read for a list
	of names of SCCS files to be processed.
	Non-SCCS files are ignored.

	Files created are given mode 444.
*/

# define MAXNAMES 9
# define COPY 0
# define NOCOPY 1
struct stat Statbuf;
char Null[1];
char Error[128];

char *ifile, *tfile;
char	*CMFAPPL;						/* CMF MODS */
char *z;	/* for validation program name */
extern FILE *Xiop;
char	*getval();	/* function returning character ptr */
char	*auxf();
char had[NFLAGS], had_flag[NFLAGS], rm_flag[NFLAGS];
char	*Comments, *Mrs;
char Valpgm[]	=	"val";
int irel, fexists, num_files;
int	VFLAG  =  0;
int	Domrs;
struct sid newsid;
extern char *Sflags[];
char *anames[MAXNAMES], *enames[MAXNAMES];
char	*unlock;
char	*locks;
char *flag_p[NFLAGS];
int asub, esub;
int check_id;
int Did_id;

#ifdef NLS
static char b_dir[]  =
	"directory `%1$s' specified as `%2$c' keyletter value";	/*nl_msg 1*/
#else
static char b_dir[]  =  "directory `%s' specified as `%c' keyletter value (ad29)";
#endif

main(argc,argv)
int argc;
char *argv[];
{
	register int j;
	register char *p;
	char c, f;
	int i, testklt;
	extern admin();
	extern	char *savecmt();
	char *sid_ab();
	char *gf();
	extern int Fcnt;
	struct sid sid;

#ifdef NLS || NLS16			/* initialize to the current locale */
	if (!setlocale(LC_ALL, "")) {
		fputs(_errlocale("admin"), stderr);
		putenv("LANG=");
	}
	nl_catopen("sccs");
#endif NLS || NLS16

	/*
	Set flags for 'fatal' to issue message, call clean-up
	routine and terminate processing.
	*/
	Fflags = FTLMSG | FTLCLN | FTLEXIT;

	testklt = 1;

	/*
	The following loop processes keyletters and arguments.
	Note that these are processed only once for each
	invocation of 'main'.
	*/
	for(j=1; j<argc; j++)
		if(argv[j][0] == '-' && (c = argv[j][1])) {
			p = &argv[j][2];
			switch (c) {

			case 'i':	/* name of file of body */
				ifile = p;
				if (*ifile && exists(ifile))
					if ((Statbuf.st_mode & S_IFMT) ==
						 S_IFDIR) {

#ifdef NLS 
					sprintmsg(Error,nl_msg(1,b_dir),ifile,c);
					strcat(Error," (ad29)");
#else
					sprintf(Error,b_dir,ifile,c);
#endif  
					fatal(Error);
				}
				break;

			case 't':	/* name of file of descriptive text */
				tfile = p;
				if (*tfile && exists(tfile))
					if ((Statbuf.st_mode & S_IFMT) ==
						S_IFDIR) {
#ifdef NLS 
					sprintmsg(Error,(nl_msg(1,b_dir)),tfile,c);
					strcat(Error," (ad29)");
#else
					sprintf(Error,b_dir,tfile,c);
#endif  
					fatal(Error);
				}
				break;
			case 'm':	/* mr flag */
				Mrs = p;
				repl(Mrs,'\n',' ');
				break;
			case 'y':	/* comments flag for entry */
				Comments = p;
				break;

			case 'd':	/* flags to be deleted */
				testklt = 0;
				if (!(f = *p))
				{
					sprintf(Error,"%s (ad1)",nl_msg(2,"d has no argument"));
					fatal(Error);
				}
				p = &argv[j][3];

				switch (f) {

				case IDFLAG:	/* see 'f' keyletter */
				case BRCHFLAG:	/* for meanings of flags */
				case VALFLAG:
				case TYPEFLAG:
				case MODFLAG:
				case QSECTFLAG:
				case NULLFLAG:
				case FLORFLAG:
				case CEILFLAG:
				case DEFTFLAG:
				case JOINTFLAG:
					if (*p) {
						sprintf(Error,
							nl_msg(3,"value after %c flag"),f);
						fatal( strcat(Error, " (ad12)") );
					}
					break;
				case CMFFLAG:	/* option installed by CMF */
					if (*p) {
						sprintf(Error,
							nl_msg(3,"value after %c flag"),f);
						fatal( strcat(Error, " (ad12)") );
					}
					break;
				case LOCKFLAG:
					if (*p) {
						/*
						set pointer to releases
						to be unlocked
						*/
						repl(p,',',' ');
						if (!val_list(p))
						{
							sprintf(Error,"%s (ad27)",nl_msg(5,"bad list format"));
							fatal(Error);
						}
						if (!range(p))
						{
							sprintf(Error,"%s (ad28)",nl_msg(6,"element in list out of range"));
							fatal(Error);
						}
						if (*p != 'a')
							unlock = p;
					}
					break;

				default:
					sprintf(Error,"%s (ad3)",nl_msg(7,"unknown flag" ));
					fatal(Error);
				}

				if (rm_flag[f - 'a']++)
				{
					sprintf(Error,"%s (ad4)",nl_msg(8,"flag twice"));
					fatal(Error);
				}
				break;

			case 'f':	/* flags to be added */
				testklt = 0;
				if (!(f = *p))
				{
					sprintf(Error,"%s (ad5)",nl_msg(9,"f has no argument"));
					fatal(Error);
				}
				p = &argv[j][3];

				switch (f) {

				case BRCHFLAG:	/* branch */
				case NULLFLAG:	/* null deltas */
				case JOINTFLAG:	/* joint edit flag */
					if (*p) {
						sprintf(Error,
							nl_msg(10, "value after %c flag"), f);
							fatal(strcat(Error," (ad13)"));
					}
					break;

				case IDFLAG:	/* id-kwd message (err/warn) */
					break;

				case VALFLAG:	/* mr validation */
					VFLAG++;
					if (*p)
						z = p;
					break;

				case CMFFLAG:			/* CMFNET MODS */
					if (*p)				/* CMFNET MODS */
						CMFAPPL = p;	/* CMFNET MODS */
					else				/* CMFNET MODS */
						fatal ((nl_msg(11,"No application with application flag.")));
					if (gf (CMFAPPL) == (char*) NULL)
						fatal ((nl_msg(12,"No .FRED file exists for this application.")));
					break;				/* END CMFNET MODS */

				case FLORFLAG:	/* floor */
					if ((i = patoi(p)) == -1)
					{
						sprintf(Error,"%s (ad22)",nl_msg(13,"floor not numeric"));
						fatal(Error);
					}
					if ((size(p) > 5) || (i < MINR) ||
							(i > MAXR))
					{
						sprintf(Error,"%s (ad23)",nl_msg(14,"floor out of range"));
						fatal(Error);
					}
					break;

				case CEILFLAG:	/* ceiling */
					if ((i = patoi(p)) == -1)
					{
						sprintf(Error,"%s (ad24)",nl_msg(15,"ceiling not numeric"));
						fatal(Error);
					}
					if ((size(p) > 5) || (i < MINR) ||
							(i > MAXR))
					{
						sprintf(Error,"%s (ad25)",nl_msg(16,"ceiling out of range"));
						fatal(Error);
					}
					break;

				case DEFTFLAG:	/* default sid */
					if (!(*p))
					{
						sprintf(Error,"%s (ad14)",nl_msg(17,"no default sid")); 
						fatal(Error);
					}
					chksid(sid_ab(p,&sid),&sid);
					break;

				case TYPEFLAG:	/* type */
				case MODFLAG:	/* module name */
				case QSECTFLAG:	/* csect name */
					if (!(*p)) {
						sprintf(Error,
							nl_msg(18,"flag %c has no value"), f);
							fatal(strcat(Error," (ad2)"));
					}
					break;
				case LOCKFLAG:	/* release lock */
					if (!(*p))
						/*
						lock all releases
						*/
						p = "a";
					/*
					replace all commas with
					blanks in SCCS file
					*/
					repl(p,',',' ');
					if (!val_list(p))
					{
						sprintf(Error,"%s (ad27)",nl_msg(19,"bad list format"));
						fatal(Error);
					}
					if (!range(p))
					{
						sprintf(Error,"%s (ad28)",nl_msg(20,"element in list out of range"));
						fatal(Error);
					}
					break;

				default:
					sprintf(Error,"%s (ad3)",nl_msg(21,"unknown flag"));
					fatal(Error);
				}

				if (had_flag[f - 'a']++)
				{
					sprintf(Error,"%s (ad4)",nl_msg(22,"flag twice"));
					fatal(Error);
				}
				flag_p[f - 'a'] = p;
				break;

			case 'r':	/* initial release number supplied */
				 chksid(sid_ab(p,&newsid),&newsid);
				 if ((newsid.s_rel < MINR) ||
				     (newsid.s_rel > MAXR))
				     {
					sprintf(Error,"%s (ad7)",nl_msg(23,"r out of range"));
					fatal(Error);
				     }
				 break;

			case 'n':	/* creating new SCCS file */
			case 'h':	/* only check hash of file */
			case 'z':	/* zero the input hash */
				break;

			case 'a':	/* user-name allowed to make deltas */
				testklt = 0;
				if (!(*p))
				{
					sprintf(Error,"%s (ad8)",nl_msg(24,"bad a argument"));
					fatal(Error);
				}
				if (asub > MAXNAMES)
				{
					sprintf(Error,"%s (ad9)",nl_msg(25,"too many 'a' keyletters"));
					fatal(Error);
				}
				anames[asub++] = p;
				break;

			case 'e':	/* user-name to be removed */
				testklt = 0;
				if (!(*p))
				{
					sprintf(Error,"%s (ad10)",nl_msg(26,"bad e argument"));
					fatal(Error);
				}
				if (esub > MAXNAMES)
				{
					sprintf(Error,"%s (ad11)",nl_msg(27,"too many 'e' keyletters"));
					fatal(Error);
				}
				enames[esub++] = p;
				break;

			default:
				sprintf(Error,"%s (cm1)",nl_msg(28,"unknown key letter"));
				fatal(Error);
			}

			if (had[c - 'a']++ && testklt++)
			{
				sprintf(Error,"%s (cm2)",nl_msg(29,"key letter twice"));
				fatal(Error);
			}
			argv[j] = 0;
		}
		else
			num_files++;

	if (num_files == 0)
	{
		sprintf(Error,"%s (cm3)",nl_msg(30,"missing file arg"));
		fatal(Error);
	}

	if ((HADY || HADM) && ! (HADI || HADN))
	{
		sprintf(Error,"%s (ad30)",nl_msg(31,"illegal use of 'y' or 'm' keyletter"));
		fatal(Error);
	}
	if (HADI && num_files > 1) /* only one file allowed with `i' */
	{
		sprintf(Error,"%s (ad15)",nl_msg(32,"more than one file"));
		fatal(Error);
	}
	if ((HADI || HADN) && ! logname())
	{
		sprintf(Error,"%s (cm9)",nl_msg(33,"USER ID not in password file"));
		fatal(Error);
	}
	setsig();

	/*
	Change flags for 'fatal' so that it will return to this
	routine (main) instead of terminating processing.
	*/
	Fflags &= ~FTLEXIT;
	Fflags |= FTLJMP;

	/*
	Call 'admin' routine for each file argument.
	*/
	for (j=1; j<argc; j++)
		if (p = argv[j])
			do_file(p,admin);
	exit(Fcnt ? 1 : 0);

}


/*
	Routine that actually does admin's work on SCCS files.
	Existing s-files are copied, with changes being made, to a
	temporary file (x-file). The name of the x-file is the same as the
	name of the s-file, with the 's.' replaced by 'x.'.
	s-files which are to be created are processed in a similar
	manner, except that a dummy s-file is first created with
	mode 444.
	At end of processing, the x-file is renamed with the name of s-file
	and the old s-file is removed.
*/

struct packet gpkt;	/* see file defines.h */
char	Zhold[BUFSIZ];	/* temporary z-file name */

admin(afile)
char	*afile;
{
	struct deltab dt;	/* see file defines.h */
	struct stats stats;	/* see file defines.h */
	struct stat sbuf;
	FILE	*iptr, *fdfopen();
	register int k;
	register char *cp, *q;
	char	nline[BUFSIZ];
	char	*getline();
	char	*p_lval, *tval, *lval;
	char	f;		/* character holder for flag character */
	char	line[BUFSIZ];
	char	*del_ba(), *fmalloc();
	int	i;		/* used in forking procedure */
	int	ck_it  =  0;	/* used for lockflag checking */
	int	status;		/* used for status return from fork */
	long	time();
	extern	nfiles;
	extern	char had_dir;
	if (setjmp(Fjmp))	/* set up to return here from 'fatal' */
		return;		/* and return to caller of admin */

	lval = 0;		/* ensure we start out pointing at nothing */

	if ( !sccsfile(afile) )
	  {			/* Don't continue if not a good file */
	    sprintf(Error,"%s (co1)",nl_msg(212,"not an SCCS file") );
	    fatal(Error);
	  }
	zero(&stats,sizeof(stats));

	if (HADI && had_dir) /* directory not allowed with `i' keyletter */
	{
		sprintf(Error,"%s (ad26)",nl_msg(34,"directory named with `i' keyletter"));
		fatal(Error);
	}

	fexists = exists(afile);

	if (HADI)
		HADN = 1;
	if (HADI || HADN) {
			if (VFLAG && had_flag[CMFFLAG - 'a'])
			fatal ((nl_msg(35,"Can't have two verification routines.")));

		if (HADM && !VFLAG && !had_flag[CMFFLAG - 'a'])
		{
			sprintf(Error,"%s (de8)",nl_msg(36,"MRs not allowed"));
			fatal(Error);
		}

		if (VFLAG && !HADM)
		{
			sprintf(Error,"%s (de10)",nl_msg(37,"MRs required"));
			fatal(Error);
		}

	}

	if (!HADI && HADR)
	{
		sprintf(Error,"%s (ad16)",nl_msg(38,"r only allowed with i"));
		fatal(Error);
	}

	if (HADN && HADT && !(*tfile))
	{
		sprintf(Error,"%s (ad17)",nl_msg(39,"t has no argument"));
		fatal(Error);
	}

	if (HADN && HADD)
	{
		sprintf(Error,"%s (ad18)",nl_msg(40,"d not allowed with n"));
		fatal(Error);
	}

	if (HADN && fexists) {
		sprintf(Error,nl_msg(41,"file %s exists"),afile);
		fatal(strcat(Error," (ad19)"));
	}

	if (!HADN && !fexists) {
		sprintf(Error,nl_msg(42,"file %s does not exist"),afile);
		fatal(strcat(Error," (ad20)"));
	}
	if (HADH) {
		/*
		   fork here so 'admin' can execute 'val' to
		   check for a corrupted file.
		*/
		if ((i = fork()) < 0)
			fatal((nl_msg(43,"cannot fork, try again")));
		if (i == 0) {		/* child */
			/*
			   perform 'val' with appropriate keyletters
			*/
			execlp(Valpgm, Valpgm, "-s", afile, 0);
			sprintf(Error,(nl_msg(44,"cannot execute '%s'")),Valpgm);
			fatal(Error);
		}
		else {
			wait(&status);	   /* wait on status from 'execlp' */
			if (status)
			{
				sprintf(Error,"%s (co6)",nl_msg(45,"corrupted file"));
				fatal(Error);
			}
			return;		/* return to caller of 'admin' */
		}
	}

	/*
	Lock out any other user who may be trying to process
	the same file.
	*/
	if (!HADH && lockit(copy(auxf(afile,'z'),Zhold),2,getpid()))
	{
		sprintf(Error,"%s (cm4)",nl_msg(46,"cannot create lock file"));
		fatal(Error);
	}

	if (fexists)
		sinit(&gpkt,afile,1);	/* init pkt & open s-file */
	else {
		if (toolong (afile)) {
			sprintf(Error, nl_msg(47,"file name too long"));
			fatal(Error);
		}
		xfcreat(afile,0444);	/* create dummy s-file */
		sinit(&gpkt,afile,0);	/* and init pkt */
	}

	if (!HADH)
		/*
		   set the flag for 'putline' routine to open
		   the 'x-file' and allow writing on it.
		*/
		gpkt.p_upd = 1;

	if (HADZ) {
		gpkt.do_chksum = 0;	/* ignore checksum processing */
		gpkt.p_ihash = 0;
	}

	/*
	Get statistics of latest delta in old file.
	*/
	if (!HADN) {
		stats_ab(&gpkt,&stats);
		gpkt.p_wrttn++;
		newstats(&gpkt,line,"0");
	}

	if (HADN) {		/*   N E W   F I L E   */

		/*
		Beginning of SCCS file.
		*/
		sprintf(line,"%c%c%s\n",CTLCHAR,HEAD,"00000");
		putline(&gpkt,line);

		/*
		Statistics.
		*/
		newstats(&gpkt,line,"0");

		dt.d_type = 'D';	/* type of delta */

		/*
		Set initial release, level, branch and
		sequence values.
		*/
		if (HADR)
			{
			 dt.d_sid.s_rel = newsid.s_rel;
			 dt.d_sid.s_lev = newsid.s_lev;
			 dt.d_sid.s_br  = newsid.s_br ;
			 dt.d_sid.s_seq = newsid.s_seq;
			 if (dt.d_sid.s_lev == 0) dt.d_sid.s_lev = 1;
			 if ((dt.d_sid.s_br) && ( ! dt.d_sid.s_seq))
				dt.d_sid.s_seq = 1;
			}
		else
			{
			 dt.d_sid.s_rel = 1;
			 dt.d_sid.s_lev = 1;
			 dt.d_sid.s_br = dt.d_sid.s_seq = 0;
			}
		time(&dt.d_datetime);		/* get time and date */

		copy(logname(),dt.d_pgmr);	/* get user's name */

		dt.d_serial = 1;
		dt.d_pred = 0;

		del_ba(&dt,line);	/* form and write */
		putline(&gpkt,line);	/* delta-table entry */

		/*
		If -m flag, enter MR numbers
		*/

		if (Mrs) {
			if (had_flag[CMFFLAG - 'a']) {	/* CMF check routine */
				if (cmrcheck (Mrs, CMFAPPL) != 0) {	/* check them */
					fatal ((nl_msg(48,"Bad CMR number(s).")));
					}
				}
			mrfixup();
			if (z && valmrs(&gpkt,z))
			{
				sprintf(Error,"%s (de9)",nl_msg(49,"invalid MRs"));
				fatal(Error);
			}
			putmrs(&gpkt);
		}

		/*
		Enter comment line for `chghist'
		*/

		if (HADY) {
			sprintf(line,"%c%c ",CTLCHAR,COMMENTS);
			putline(&gpkt,line);
			sprintf(line,"%s",savecmt(Comments));
			putline(&gpkt,line);
			putline(&gpkt,"\n");
		}
		else {
			/*
			insert date/time and pgmr into comment line
			*/
			cmt_ba(&dt,line);
			putline(&gpkt,line);
		}
		/*
		End of delta-table.
		*/
		sprintf(line,CTLSTR,CTLCHAR,EDELTAB);
		putline(&gpkt,line);

		/*
		Beginning of user-name section.
		*/
		sprintf(line,CTLSTR,CTLCHAR,BUSERNAM);
		putline(&gpkt,line);
	}
	else
		/*
		For old file, copy to x-file until user-name section
		is found.
		*/
		flushto(&gpkt,BUSERNAM,COPY);

	/*
	Write user-names to be added to list of those
	allowed to make deltas.
	*/
	if (HADA)
		for (k = 0; k < asub; k++) {
			sprintf(line,"%s\n",anames[k]);
			putline(&gpkt,line);
		}

	/*
	Do not copy those user-names which are to be erased.
	*/
	if (HADE && !HADN)
		while ((cp = getline(&gpkt)) &&
				!(*cp++ == CTLCHAR && *cp == EUSERNAM)) {
			for (k = 0; k < esub; k++) {
				cp = gpkt.p_line;
				while (*cp)	/* find and */
					cp++;	/* zero newline */
				*--cp = '\0';	/* character */

				if (equal(enames[k],gpkt.p_line)) {
					/*
					Tell getline not to output
					previously read line.
					*/
					gpkt.p_wrttn = 1;
					break;
				}
				else
					*cp = '\n';	/* restore newline */
			}
		}

	if (HADN) {		/*   N E W  F I L E   */

		/*
		End of user-name section.
		*/
		sprintf(line,CTLSTR,CTLCHAR,EUSERNAM);
		putline(&gpkt,line);
	}
	else
		/*
		For old file, copy to x-file until end of
		user-names section is found.
		*/
		if (!HADE)
			flushto(&gpkt,EUSERNAM,COPY);

	/*
	For old file, read flags and their values (if any), and
	store them. Check to see if the flag read is one that
	should be deleted.
	*/
	if (!HADN)
		while ((cp = getline(&gpkt)) &&
				(*cp++ == CTLCHAR && *cp == FLAG)) {

			gpkt.p_wrttn = 1;	/* don't write previous line */

			cp += 2;	/* point to flag character */
			k = *cp - 'a';
			f = *cp;
			if (f == LOCKFLAG) {
				p_lval = cp;
				++p_lval;
				tval = fmalloc(size(gpkt.p_line)-5);
				copy(++p_lval,tval);
				lval = tval;
				while(*tval)
					++tval;
				*--tval = '\0';
			}

			if (!had_flag[k] && !rm_flag[k]) {
				had_flag[k] = 2;	/* indicate flag is */
							/* from file, not */
							/* from arg list */

				if (*++cp != '\n') {	/* get flag value */
					q = fmalloc(size(gpkt.p_line)-5);
					copy(++cp,q);
					flag_p[k] = q;
					while (*q)	/* find and */
						q++;	/* zero newline */
					*--q = '\0';	/* character */
				}
			}
			if (rm_flag[k])
				if (f == LOCKFLAG) {
					if (unlock) {
						if ((adjust(lval)) && !had_flag[k])
							ck_it = had_flag[k] = 1;
					}
					else had_flag[k] = 0;
				}
				else had_flag[k] = 0;
		}


	/*
	Write out flags.
	*/
	/* test to see if the CMFFLAG is safe */
	if (had_flag[CMFFLAG - 'a']) {
		if (had_flag[VALFLAG - 'a'] && !rm_flag[VALFLAG - 'a'])
			fatal ((nl_msg(50,"Can't use -fz with -fv.")));
		}
	for (k = 0; k < NFLAGS; k++)
		if (had_flag[k]) {
			if (flag_p[k] || lval) {
				if (('a' + k) == LOCKFLAG && had_flag[k] == 1) {
					if ((flag_p[k] && (*flag_p[k] == 'a')) || (lval && (*lval == 'a')))
						locks = "a";
					else if (lval && flag_p[k])
						locks =
						cat(nline,lval," ",flag_p[k],0);
					else if (lval)
						locks = lval;
					else locks = flag_p[k];
					sprintf(line,"%c%c %c %s\n",
						CTLCHAR,FLAG,'a' + k,locks);
					locks = 0;
					if (lval) {
						ffree(lval);
						tval = lval = 0;
					}
					if (ck_it)
						had_flag[k] = ck_it = 0;
				}
				else sprintf(line,"%c%c %c %s\n",
					CTLCHAR,FLAG,'a'+k,flag_p[k]);
			}
			else
				sprintf(line,"%c%c %c\n",
					CTLCHAR,FLAG,'a'+k);

			/* flush imbeded newlines from flag value */
			i = 4;
			if (line[i] == ' ')
				for (i++; line[i+1]; i++)
					if (line[i] == '\n')
						line[i] = ' ';
			putline(&gpkt,line);

			if (had_flag[k] == 2) {	/* flag was taken from file */
				had_flag[k] = 0;
				if (flag_p[k]) {
					ffree(flag_p[k]);
					flag_p[k] = 0;
				}
			}
		}

	if (HADN) {
		/*
		Beginning of descriptive (user) text.
		*/
		sprintf(line,CTLSTR,CTLCHAR,BUSERTXT);
		putline(&gpkt,line);
	}
	else {
		/*
		Write out BUSERTXT record which was read in
		above loop that processes flags.
		*/
		gpkt.p_wrttn = 0;
		putline(&gpkt,(char *) 0);
	}

	/*
	Get user description, copy to x-file.
	*/
	if (HADT) {
		if (*tfile) {
			iptr = xfopen(tfile,0);
			fgetchk(line,BUFSIZ,iptr,tfile,&gpkt);
			fclose(iptr);
		}

		/*
		If old file, ignore any previously supplied
		commentary. (i.e., don't copy it to x-file.)
		*/
		if (!HADN)
			flushto(&gpkt,EUSERTXT,NOCOPY);
	}

	if (HADN) {		/*   N E W  F I L E   */

		/*
		End of user description.
		*/
		sprintf(line,CTLSTR,CTLCHAR,EUSERTXT);
		putline(&gpkt,line);

		/*
		Beginning of body (text) of first delta.
		*/
		sprintf(line,"%c%c %u\n",CTLCHAR,INS,1);
		putline(&gpkt,line);

		if (HADI) {		/* get body */

			/*
			Set indicator to check lines of body of file for
			keyword definitions.
			If no keywords are found, a warning
			will be produced.
			*/
			check_id = 1;
			/*
			Set indicator that tells whether there
			were any keywords to 'no'.
			*/
			Did_id = 0;
			if (*ifile)
				iptr = xfopen(ifile,0);	/* from a file */
			else
				iptr = stdin;	/* from standard input */

			/*
			Read and copy to x-file, while checking
			first character of each line to see that it
			is not the control character (octal 1).
			Also, count lines read, and set statistics'
			structure appropriately.
			The 'fgetchk' routine will check for keywords.
			*/
			stats.s_ins = fgetchk(line,BUFSIZ,iptr,ifile,&gpkt);
			stats.s_del = stats.s_unc = 0;

			/*
			If no keywords were found, issue warning.
			*/
			if (!Did_id) {
				if (had_flag[IDFLAG - 'a'])
					if(!(*flag_p[IDFLAG -'a']))
					{
						sprintf(Error,"%s (cm6)",nl_msg(51,"no id keywords"));
						fatal(Error);
					}
					else
					{
						sprintf(Error,"%s (cm10)",nl_msg(52,"invalid id keywords"));
						fatal(Error);
					}
				else
					fprintf(stderr,"%s (cm7)\n",(nl_msg(53,"No id keywords")));
			}

			check_id = 0;
			Did_id = 0;
		}

		/*
		End of body of first delta.
		*/
		sprintf(line,"%c%c %u\n",CTLCHAR,END,1);
		putline(&gpkt,line);
	}
	else {
		/*
		Indicate that EOF at this point is ok, and
		flush rest of (old) s-file to x-file.
		*/
		gpkt.p_chkeof = 1;
		while (getline(&gpkt)) ;
	}

	/*
	Flush the buffer, take care of rewinding to insert
	checksum and statistics in file, and close.
	*/
	flushline(&gpkt,&stats);

	/*
	Change x-file name to s-file, and delete old file.
	Unlock file before returning.
	*/
	if (!HADH) {
		if (!HADN) stat(gpkt.p_file,&sbuf);
		rename(auxf(gpkt.p_file,'x'),&gpkt);
		if (!HADN) {
			chmod(gpkt.p_file,sbuf.st_mode);
			chown(gpkt.p_file,sbuf.st_uid,sbuf.st_gid);
		}
		xrm(&gpkt);
		unlockit(auxf(afile,'z'),getpid());
	}
}


fgetchk(strp,len,inptr,file,pkt)
register char *strp;
register int len;
FILE *inptr;
register char *file;
register struct packet *pkt;
{
	register int k;
	register int ll;

	for (k = 1; fgets(strp,len,inptr); k++) {
		if (*strp == CTLCHAR) {
#ifdef NLS
			sprintmsg(Error,(nl_msg(54,"%1$s illegal data on line %2$d")), file, k);
			strcat(Error," (ad21)");
#else
			sprintf(Error,"%s illegal data on line %d (ad21)",
				file,k);
#endif
			fatal(Error);
		}

		if (check_id)
			chkid(strp,flag_p['i'-'a']);

		ll = strlen( strp ) - 1;
		if ( strp[ ll ] != '\n' )
		  {
		  strp[ ll+1 ] = '\n';
		  strp[ ll+2 ] = '\0';
		  }
		putline(pkt,strp);
	}
	return(k - 1);
}


clean_up()
{
	xrm(&gpkt);
	if (Xiop)
		fclose(Xiop);
	if(gpkt.p_file[0])
		unlink(auxf(gpkt.p_file,'x'));
	if (HADN)
		unlink(gpkt.p_file);
	if (!HADH)
		unlockit(Zhold,getpid());
}


cmt_ba(dt,str)
register struct deltab *dt;
char *str;
{
	register char *p;
	char *date_ba();

	p = str;
	*p++ = CTLCHAR;
	*p++ = COMMENTS;
	*p++ = ' ';
	copy((nl_msg(55,"date and time created")),p);
	while (*p++)
		;
	--p;
	*p++ = ' ';
	date_ba(&dt->d_datetime,p);
	while (*p++)
		;
	--p;
	*p++ = ' ';
	copy((nl_msg(56,"by")),p);
	while (*p++)
		;
	--p;
	*p++ = ' ';
	copy(dt->d_pgmr,p);
	while (*p++)
		;
	--p;
	*p++ = '\n';
	*p = 0;
}


putmrs(pkt)
struct packet *pkt;
{
	register char **argv;
	char str[64];
	extern char **Varg;

	for (argv = &Varg[VSTART]; *argv; argv++) {
		sprintf(str,"%c%c %s\n",CTLCHAR,MRNUM,*argv);
		putline(pkt,str);
	}
}


/*
** This was rewritten cause the original code built the new list in a string
** local to this procedure and then returned a pointer to the string (which
** of course disappeared when the procedure exited).
*/
adjust(line)
char	*line;
{
	register int k;
	register int i;
	char	*t_unlock;
	char	rel[5];
	int	sz, l;

	t_unlock = unlock;
	while(*t_unlock) {
		NONBLANK(t_unlock);
		t_unlock = getval(t_unlock,rel);
		while ((k = pos_ser(line,rel)) != -1) {
			sz = size(rel);					/* size of rel + space */
			l = strlen(line) - sz;				/* len of new line */
			if (l < 0)
				line[0] = '\0';				/* deleting last rel in list */
			else
				for(i = (k>0 ? k-1 : 0); i <= l; i++)	/* have to delete space unless 1st rel */
					line[i] = line[i+sz];		/* copy rest of line down on top of deleted rel */
		}
	}
	return (strlen(line));
}


char	*getval(sourcep,destp)
register char	*sourcep;
register char	*destp;
{
	while (*sourcep != ' ' && *sourcep != '\t' && *sourcep != '\0')
		*destp++ = *sourcep++;
	*destp = 0;
	return(sourcep);
}


val_list(list)
register char *list;
{
	register int i;

	if (list[0] == 'a')
		return(1);
	else for(i = 0; list[i] != '\0'; i++)
		if (list[i] == ' ' || numeric(list[i]))
			continue;
		else if (list[i] == 'a') {
			list[0] = 'a';
			list[1] = '\0';
			return(1);
		}
		else return(0);
	return(1);
}


pos_ser(s1,s2)
char	*s1;
char	*s2;
{
	register int offset;
	register char *p;
	char	num[5];

	p = s1;
	offset = 0;

	while(*p) {
		NONBLANK(p);
		p = getval(p,num);
		if (equal(num,s2)) {
			return(offset);
}
		offset = offset + size(num);
	}
	return(-1);
}


range(line)
register char *line;
{
	register char *p;
	char	rel[BUFSIZ];

	p = line;
	while(*p) {
		NONBLANK(p);
		p = getval(p,rel);
		if (size(rel) > 5)
			return(0);
	}
	return(1);
}
