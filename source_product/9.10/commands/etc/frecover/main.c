/* @(#) $Revision: 70.4 $ */
/****************************************************************

 The name of this file is main.c.

 +--------------------------------------------------------------+
 | (c)  Copyright  Hewlett-Packard  Company  1986.  All  rights |
 | reserved.   No part  of  this  program  may be  photocopied, |
 | reproduced or translated to another program language without |
 | the  prior  written   consent  of  Hewlett-Packard  Company. |
 +--------------------------------------------------------------+

 Changes:
	$Log:	main.c,v $
 * Revision 70.4  92/07/09  17:35:29  17:35:29  ssa
 * Author: venky@hpucsb2.cup.hp.com
 * Usage message has been slightly changed to make 
 * it more meaningful. The -i option in the fourth 
 * instance of frecover usage changed to -I
 * 
 * Revision 70.3  92/07/01  11:36:59  11:36:59  ssa (RCS Manager)
 * Author: venky@hpucsb2.cup.hp.com
 * Changed the structure of the expanded usage message for
 * frecover - for internationalization. The usage message is
 * now a single columned, concise collection of details.
 * 
 * Revision 70.2  92/02/10  17:50:10  17:50:10  ssa
 * Author: dickermn@hpucsb2.cup.hp.com
 * Removed extra 'e' from option string that prevented correct -e parsing
 * 
 * Revision 66.19  92/01/28  17:22:59  17:22:59  ssa
 * Author: dickermn@hpucsb2.cup.hp.com
 * Added support for the -D (no fastsearch option)
 * 
 * Revision 66.18  92/01/25  16:47:56  16:47:56  ssa (RCS Manager)
 * Author: dickermn@hpucsb2.cup.hp.com
 * Added support for the -E -B and -b options (undocumented) to override
 * the detected EOF marker expectation and blocksizes (just in case there
 * are still more problems found with that code).
 * 
 * Revision 66.17  91/12/06  17:45:47  17:45:47  ssa
 * Author: dickermn@hpucsb2.cup.hp.com
 * Corrected code which printed messages about incorrect argument strings
 *  so as to indicate what mistake the user made.
 * 
 * Revision 66.16  91/11/20  09:53:22  09:53:22  ssa
 * Author: dickermn@hpucsb2.cup.hp.com
 * Changed -N and -n messages to be consistant with the actual -N option
 * 
 * Revision 66.15  91/11/18  10:50:23  10:50:23  ssa (RCS Manager)
 * Author: dickermn@hpucsb2.cup.hp.com
 * Changed the -n option to -N to avoid confusion with fbackup's -n
 * option (which is used to backup over NFS).
 * 
 * Revision 66.14  91/11/14  17:30:18  17:30:18  ssa (RCS Manager)
 * Author: dickermn@hpucsb2.cup.hp.com
 * Cleaned up catgets calls so findmsg could find them.
 * 
 * Revision 66.13  91/11/11  18:07:18  18:07:18  ssa (RCS Manager)
 * Author: dickermn@hpucsb2.cup.hp.com
 * Changed usage message so that errors and usage will fit on screen together
 * 
 * Revision 66.12  91/11/04  10:10:44  10:10:44  root ()
 * added support for the -m option
 * 
 * Revision 66.11  91/10/30  15:17:18  15:17:18  root ()
 * added more tolerance to the routine that parses the names of included
 * and excluded files, so that all extra slashes are weeded-out.
 * 
 * Revision 66.10  91/10/10  18:19:30  18:19:30  root ()
 * Added parsing and usage message support for -n/nflag "no recovery" option
 * 
 * Revision 66.9  91/10/08  10:12:32  10:12:32  root ()
 * Changed the addnode() routine to strip trailing / characters 
 * from include/exclude pathnames, allowing for simpler matching
 * 
 * Revision 66.8  91/03/01  08:30:59  08:30:59  ssa ((sccs admin))
 * Author: danm@hpbblc.bbn.hp.com
 * fixes to MO and DAT extensions
 * 
 * Revision 66.7  91/01/17  15:24:01  15:24:01  danm
 * changes for DAT and MO and other 8.0 enhancements
 * 
 * Revision 66.6  90/10/01  09:26:07  09:26:07  danm (#Dan Matheson)
 *  merge of 7.0 fixes from branches into the main 8.0 branch
 * 
 * Revision 66.5  90/05/09  08:50:43  08:50:43  danm (#Dan Matheson)
 *  fix for defect FSDdt04504.  A defective graph file could cause a complete
 *  recover. This has destroyed customer data so, the fix is to exit on
 *  detection of a graph file problem.
 * 
 * Revision 66.4  90/05/04  09:50:49  09:50:49  danm (#Dan Matheson)
 *  fixed typing error with -V option stuff
 * 
 * Revision 66.3  90/05/04  08:33:57  08:33:57  danm (#Dan Matheson)
 *  changes for the new -V option
 * 
 * Revision 66.2  90/02/26  19:46:02  19:46:02  danm (#Dan Matheson)
 *  moved #includes to frecover.h too make changes easier, and fixed things
 *  to use malloc(3x)
 * 
 * Revision 66.1  90/02/23  19:00:01  19:00:01  danm (#Dan Matheson)
 * folded in post 7.0 fixes for shippment stopper defect to main branch
 * 
 * Revision 64.1  89/02/18  15:39:27  15:39:27  jh
 * NLS initialization and collation.
 * Replaced nl_init() with setlocale(); replaced nl_strcmp() with strcoll().
 * 
 * Revision 63.1  88/09/22  16:06:44  16:06:44  lkc (Lee Casuto[Ft.Collins])
 * Added code to handle ACLS
 * 
 * Revision 62.3  88/07/12  17:10:15  17:10:15  peteru
 * suppress SIGINT handling if frecover started by SAM
 * 
 * Revision 62.2  88/06/24  18:25:33  18:25:33  jennie (Jennie Tsang)
 * Take out declaration for signal since signal.h includes it already.
 * 
 * Revision 62.1  88/04/06  11:28:29  11:28:29  carolyn (Carolyn Sims)
 * added error numbers to messages for SAM to key off of
 * add capability to read responses from named pipe shared with SAM
 * 
 * Revision 56.1  87/11/04  10:08:16  10:08:16  runyan (Mark Runyan)
 * Complete for first release (by lkc)
 * 
 * Revision 51.2  87/11/03  16:58:14  16:58:14  lkc (Lee Casuto)
 * completed for first release
 * 

 This file:
    	This file parses the run string and initializes all states
	for frecover.


****************************************************************/

#if defined NLS || defined NLS16
#define NL_SETN 3	/* set number */
#endif

#include "frecover.h"

extern	void	exit();

/********************************************************************/
/*  	    	PUBLIC PART OF MAIN	    	    	    	    */
int	hflag = 0;			/* graph reconstruction (h) */
int	yflag = 0;			/* all prompts are yes (y)  */
int	aflag = 0;	    	    	/* error abort flag (S)     */
int	vflag = 0;			/* verbose flag (v)    	    */
int	cflag = 0;  	    	    	/* CDF flag (H)	    	    */
int	sflag = 0;  	    	    	/* sparse files (s)	    */
int	xflag = 0;  	    	    	/* relative restore (X)	    */
int	fflag = 0;			/* full recovery	    */
int	pflag = 0;			/* partial recovery	    */
int	nflag = 0;			/* No recovery (verify opt) */
int	mflag = 0;			/* print file marker data   */

  /****************************************************************/
  /* Note that the following are not documented options, as all
     of this information should be determined automatically,
     but they can be used on media which need recovery but (because
     of early bugs, yet undescovered) are not in a known format
     combination.  
   */
int	Eflag = 0;			/* toggle expect expect_eof flag*/
int	bflag = 0;			/* manually select block size   */
int	Dflag = 0;			/* prevent any fastsearching    */
  /****************************************************************/

int	flatflag = 0;			/* flat filesys recovery    */
int	oflag = 0;			/* don't change ownership   */
int	graph = 0;			/* g option specified	    */
int	conf = 0;			/* c option specified       */
int	inc = 0;			/* i option specified	    */
int	exc = 0;			/* e option specified	    */
int	volnum= 0;  	    	    	/* current volume number    */
int	overwrite = 0;			/* overwrite existing files */
int	doerror = 0;			/* mark completion of config*/
int	dochgvol = 0;			/* error and chgvol	    */
int	rflag = 0;			/* restart flag		    */
int	synclimit = SYNCDEF;		/* max allowable resyncs    */
int	recovertype = ABSOLUTE;		/* type of recovery	    */
int	residfd;			/* fd for residual file	    */
char	command = '\0';			/* current command 	    */
int     indexonly = 0;		        /* 1 if want index or vol hdr   */
char	*indexfile = "";    	    	/* name of index file	    */
char	*volhdrfile = "";    	    	/* name of volume header file */
char	errfile[MAXPATHLEN] = "";	/* file to execute on errors*/
char	chgvolfile[MAXPATHLEN] = "";	/* execute on volume change */
char	residual[MAXPATHLEN] = "";	/* residual data on errors  */
char	home[MAXPATHLEN+3];		/* home; set by getcwd	    */
char	restartname[MAXPATHLEN];	/* name of restart file	    */
FILE 	*terminal;			/* descriptor for terminal  */
int	Zflag = 0;	                /* response from sampipe    */
struct	passwd	user;			/* set up user verification */
struct	passwd	*u;			/* access to user data	    */
struct	group	guser;			/* set up group verification*/
struct	group	*g;			/* access to group data	    */
LISTNODE	*ilist;			/* start of include list    */
LISTNODE	*elist;			/* start of exclude list    */
LISTNODE	*temp;			/* temp list pointer	    */
OBSCURE		*o_head;		/* head of obscure list	    */

int blocksize;
int do_fs;

int     n_outfiles = 0;
char   *outfptr;
struct fn_list *outfile_head;
struct fn_list *outfile_temp;

struct index_list *index_head = NULL;    /* head of index list */
struct index_list *index_tail = NULL;    /* tail of index list */

#if defined NLS || defined NLS16
nl_catd nlmsg_fd;			/* message catalog	    */
#endif
#ifdef ACLS
int	aclflag = TRUE;			/* assume recovery of ACLS  */
#endif /* ACLS */
/********************************************************************/

/********************************************************************/
/*		VARIABLES FROM FILES				    */
extern char msg[MAXS];			/* message buffer	    */
extern int  obscure_file;		/* flag for obscure links   */
extern struct index_list *index_short;
/********************************************************************/

/********************************************************************/
/*		VARIABLES FROM Volheaders       		    */
extern VHDRTYPE vol;			/* volume header       	    */
/********************************************************************/

int machine_type;         /* machine type where the tape device is, rmt.c
			     can overwrite */
int outfiletype;
int fd;

main(argc, argv)
int argc;
char *argv[];
{
	char *filelist = "";
	char *configfile = "";
	extern int onintr();
        extern char	*optarg;
        extern int optind;
        int	errflg = FALSE;
	char c;
	extern int 	opterr;
	struct utsname name;
	struct fn_list *t;
	struct fn_list *tail;
	struct fn_list *head;
	int n, old_optind=1;
	char *current_opt;

#ifdef ACLS
	static char *options = "DEbBmNZhosvyAFHOXc:f:S:e:g:i:I:V:xrR:";
#else
	static char *options = "DEbBmNZhosvyFHOXc:f:S:e:g:i:I:V:xrR:";
#endif /* ACLS */

	blocksize = BLOCKSIZE;
	do_fs = FALSE;
	
	t = (struct fn_list *)NULL;
	head = (struct fn_list *)NULL;
	tail = (struct fn_list *)NULL;

	opterr = 0;			/* disable messages */
	if (argc < 2) 
	    command = 'r';
	(void) getcwd(home, MAXPATHLEN+2);
	u = getpwuid((int)geteuid());
	saveuser(u, &user);
	g = getgrgid((int)getegid());
	savegroup(g, &guser);
	ilist = (LISTNODE *)fmalloc(sizeof(LISTNODE));
	ilist->ptr = (LISTNODE *)NULL;
	elist = (LISTNODE *)fmalloc(sizeof(LISTNODE));
	elist->ptr = (LISTNODE *)NULL;
	o_head = (OBSCURE *)fmalloc(sizeof(OBSCURE));
	o_head->o_ptr = (OBSCURE *)NULL;
	obscure_file = FALSE;
	(void) setvbuf(stderr, NULL, BUFSIZ, _IOLBF);

    uname(&name);
    if (strncmp(name.machine, "9000/3", 6) == 0) {
      machine_type = 300;
    }
    else if (strncmp(name.machine, "9000/7", 6) == 0) {
      machine_type = 700;
    } else {
      machine_type = 800;
    }

#ifdef DEBUG_D
    printf("writer: machine_type = %d\n", machine_type);
#endif /* DEBUG_D */


#ifdef NLS || NLS16			/* initialize to the current locale */
	if (!setlocale(LC_ALL, "")) {
		fputs(_errlocale("frecover"), stderr);
		putenv("LANG=");
	}
	nlmsg_fd = catopen("frecover", 0);
#endif NLS || NLS16

        current_opt = argv[1] + 1;     /*The first option parsed by getopt*/

        while ((c = (char) getopt(argc, argv, options)) != EOF) {
	    switch(c) {
		case '-':
			break;
                case 'm':
                        mflag++;
                        break;
                case 'N':
                        nflag++;
                        recovertype = NONE;
                        break;
                case 'E':
                        Eflag++;
                        break;
                case 'B':
                        bflag = 1024;  /* Larger, newer block size    */
                        break;
                case 'b':
                        bflag = 512;   /* Smaller, original block size*/
                        break;
                case 'D':
                        Dflag++;
                        break;
                case 'Z':
			Zflag++;
			break;
#ifdef ACLS
		case 'A':
			aclflag = FALSE;
			break;
#endif /* ACLS */
		case 'h':
			hflag++;
			break;
		case 'S':
			(void) strcpy(residual, optarg);
			if((residfd = open(residual, O_RDWR | O_CREAT)) < 0) {
			    (void) sprintf(msg, (catgets(nlmsg_fd,NL_SETN,203, "(3203): unable to open residual file %s")), residual);
			    warn(msg);
			    warn((catgets(nlmsg_fd,NL_SETN,204, "(3204): proceeding without residual file")));
			    break;
			}
			aflag++;
			break;
	        case 'F':
			flatflag++;
			recovertype = FLAT;
			break;
		case 'O':
			oflag++;
			break;
		case 'o':
			overwrite++;
			break;
		case 'H':
		    	cflag++;
			break;
		case 's':
		    	sflag++;
			break;
		case 'v':
			vflag++;
			break;
		case 'y':
			yflag++;
			break;
		case 'X':
		    	xflag++;
			recovertype = RELATIVE;
			break;
		case 'f':
			t = (struct fn_list *)malloc(sizeof(struct fn_list));
			strcpy(t->name, optarg);
			t->next = (struct fn_list *)NULL;
			if (head == NULL) {
			  head = t;
			  tail = t;
			}
			else {
			  tail->next = t;
			  tail = t;
			}
			n_outfiles++;
			break;
		case 'R':
			(void) strcpy(restartname, optarg); /* fall through */
		case 'r':
		case 'x':
		case 'I':
			if (command != '\0')
			    panic((catgets(nlmsg_fd,NL_SETN,205, "(3205): specified options are mutually exclusive")), USAGE);
			command = (char)c;
			if(command == 'I')
			    indexfile = optarg;
			break;
		case 'V':
			if (command != '\0')
			    panic((catgets(nlmsg_fd,NL_SETN,237, "(3238): specified options are mutually exclusive")), USAGE);
			command = (char)c;
			if(command == 'V')
			    volhdrfile = optarg;
			break;
		case 'g':
			graph++;
			filelist = optarg;
			addlist(filelist);
			break;
		case 'c':
			conf++;
			configfile = optarg;
			doconfig(configfile);
			break;
		case 'i':
			inc++;
			addnode(ilist, optarg);
			break;
		case 'e':
			exc++;
			addnode(elist, optarg);
			break;
		case '?':
			/* Unrecognized option:
			   check to see if the current option is found in 
			   the options string.  If it is, then getopt is 
			   complaining because of a lack of argument, if
			   not, then it's a problem with an invalid argument
			 */
			if(strchr(options, *current_opt) != NULL) {
			    (void) sprintf(msg, (catgets(nlmsg_fd,NL_SETN,206, "(3206): option '%c' expects an argument")), *current_opt);
			    warn(msg);
			}
			else {
			    (void) sprintf(msg, (catgets(nlmsg_fd,NL_SETN,245, "(3245): option '%c' is unrecognized")), *current_opt);
			    warn(msg);
			}
			errflg++;
			break;
	    }

	    if (optind != old_optind) {       /*We have changed argv's*/
		current_opt = argv[optind] + 1;
                old_optind = optind;
	    } else {                          /*Keep the current_opt accurate*/
		current_opt++;
	    }

        }

	if (n_outfiles == 0) {
	  t = (struct fn_list *)malloc(sizeof(struct fn_list));
	  strcpy(t->name, DEFAULTINPUT);
	  t->next = (struct fn_list *)NULL;
	  head = t;
	  tail = t;
	  n_outfiles++;
	}

	outfile_head = (struct fn_list *)NULL;
	if ((n = expand_fn(head)) < 0) {
	  errflg = TRUE;
	  if (n == -3) {
	    (void) sprintf(msg, ((catgets(nlmsg_fd,NL_SETN,251, "(3251): number of output files cannot be more than MAXOUTFILES \n"))));
	    warn(msg);
	  }
	  else {
	    (void) sprintf(msg, ((catgets(nlmsg_fd,NL_SETN,252, "(3252): error in expanding output file pattern\n"))));
	    warn(msg);
	  }
	}
	
	outfile_temp = outfile_head;
	outfptr = outfile_head->name;
	index_short = NULL;

	if(errflg)
	    panic((char *)NULL, USAGE);
	errflg = optind;
	for(; optind < argc; optind++) {
	    (void) sprintf(msg, (catgets(nlmsg_fd,NL_SETN,207, "(3207): additional argument: %s specified")),argv[optind]);
	    warn(msg);
	}
	if(errflg != optind)
	    panic("",USAGE);
	if (command == '\0') 
	    panic((catgets(nlmsg_fd,NL_SETN,208, "(3208): must specify at least one of V, I, x, r, or R")), USAGE);
	if(flatflag && xflag) {
	    warn((catgets(nlmsg_fd,NL_SETN,209, "(3209): specified both X and F keys; F assumed")));
	    recovertype = FLAT;
	}

        if (nflag && (flatflag || xflag)) {
            sprintf (msg, catgets(nlmsg_fd,NL_SETN,211, "(3211): N option specified.  Ignoring %s"),
                         (flatflag && xflag) ? "F and X" :
                         (flatflag) ? "F" :
                         (xflag) ? "X" : "");
            warn(msg);
            recovertype = NONE;
        }

	setinput();
	/* ignore SIGINT if SAM special option is used */
	if (!Zflag)
	{
		if (signal(SIGINT, onintr) == SIG_IGN)
			(void) signal(SIGINT, SIG_IGN);
	}
	else
		(void) signal(SIGINT, SIG_IGN);
	if (signal(SIGTERM, onintr) == SIG_IGN)
		(void) signal(SIGTERM, SIG_IGN);

	switch (command) {
	/*
	 * Incremental restoration of a file system.
	 */
	case 'r':
	        if((ilist->ptr != (LISTNODE *)NULL) ||
		   (elist->ptr != (LISTNODE *)NULL))
		    panic((catgets(nlmsg_fd,NL_SETN,210, "(3210): illegal option (-i/-e) specified with r option")), USAGE);
	        fflag++;
		setup(1);
		files();
		done(0);
	/*
	 * Resume an incremental file system restoration.
	 */
	case 'R':
		rflag++;
		restorestatus();
		setup(1);
		posfile();
		files();
		done(0);
	/*
	 * Batch extraction of tape contents.
	 */
	case 'x':
		pflag++;
		ilist->val = fmalloc(sizeof("NULL") + sizeof(int));
		if(ilist->ptr == (LISTNODE *)NULL) {
		    (void) strcpy(ilist->val, "NULL");
		    ilist->aflag = RECOVER;
		}
		else {
		    (void) strcpy(ilist->val, "MORE");
		}
		setup(1);
		somefiles();
		done(0);
    	/* 
    	 * Extract the index
	 */
	case 'I':
		if(hflag || aflag || flatflag || oflag || overwrite ||
		   cflag || sflag || xflag || graph || inc || exc)
		    warn((catgets(nlmsg_fd,NL_SETN,246, "(3246): additional inapplicable options specified on index extraction")));
		indexonly = 1;
		setup(0);
		frindex(0);
		done(0);
    	/* 
    	 * Extract the volume header
	 */
	case 'V':
		if(hflag || aflag || flatflag || oflag || overwrite ||
		   cflag || sflag || xflag || graph || inc || exc)
		    warn((catgets(nlmsg_fd,NL_SETN,247, "(3247): additional inapplicable options specified on volume header extraction")));
		indexonly = 1;
		setup(0);
		volheader_print();
		done(0);
        }

	return(0);                      /* never executed. shut up lint */
}

usage()
{

    (void) fprintf(stderr,(catgets(nlmsg_fd,NL_SETN,253, "frecover -r [-hosvymFNOX] [-c config] [-f device] [-S skip]\n")));

    (void) fprintf(stderr,(catgets(nlmsg_fd,NL_SETN,254,"frecover -R path [-f device]\n")));	   
    
    (void) fprintf(stderr,(catgets(nlmsg_fd,NL_SETN,255,"frecover -x [-hosvymFNOX] [-c config] [-e path] [-f device]\n            [-g graph] [-i path] [-S skip]\n")));
	    
    (void) fprintf(stderr,(catgets(nlmsg_fd,NL_SETN,256,"frecover [-I | -V] path [-vy] [-f device] [-c config]\n")));	     

    (void) fprintf(stderr,(catgets(nlmsg_fd,NL_SETN,257,"-{r,x} - full/partial recovery\n")));

    (void) fprintf(stderr,(catgets(nlmsg_fd,NL_SETN,258,"-{I,V} - write index/volume-header to path\n")));   

    (void) fprintf(stderr,(catgets(nlmsg_fd,NL_SETN,259,"-{F,X} - flat/relative recovery\n")));

    (void) fprintf(stderr,(catgets(nlmsg_fd,NL_SETN,260,"-{i,e} - specify a path to include/exclude\n")));

    (void) fprintf(stderr,(catgets(nlmsg_fd,NL_SETN,261,"-{g,c} - specify a graph/configuration file\n")));         

    (void) fprintf(stderr,(catgets(nlmsg_fd,NL_SETN,262,"-{N,h} - No files recovered-Only verify/graph information\n")));    

    (void) fprintf(stderr,(catgets(nlmsg_fd,NL_SETN,263,"-R     - continue an interrupted session\n")));  

    (void) fprintf(stderr,(catgets(nlmsg_fd,NL_SETN,264,"-f     - specify an input device\n")));

    (void) fprintf(stderr,(catgets(nlmsg_fd,NL_SETN,265,"-o     - overwrite existing files\n")));     
 
    (void) fprintf(stderr,(catgets(nlmsg_fd,NL_SETN,266,"-v     - verbose mode\n")));                 
    (void) fprintf(stderr,(catgets(nlmsg_fd,NL_SETN,267,"-y     - answer yes to all questions\n")));
 
    (void) fprintf(stderr,(catgets(nlmsg_fd,NL_SETN,268,"-S     - specify a file for skipped blocks\n"))); 
    (void) fprintf(stderr,(catgets(nlmsg_fd,NL_SETN,269,"-s     - sparse file recovery\n")));

    (void) fprintf(stderr,(catgets(nlmsg_fd,NL_SETN,270,"-O     - set ownership of files to the user running the recovery\n")));

    (void) fprintf(stderr,(catgets(nlmsg_fd,NL_SETN,271,"-m     - Print marker information (checksums, filemarks, setmarks)\n")));
 
}  /* end usage() */


/* cleanpath is to be used to copy pathnames from the passed include/exclude
 * lists to their internal storage forms, copying s2 into s1, removing any
 * redundant or trailing slashes from the pathnames.  It is up to the calling
 * function to be sure that s1 has at least as much space as s2.
 */

void
cleanpath(s1, s2)
  char *s1, *s2;
{ 
  int pos1=0, pos2, slash = FALSE;
 
  for (pos2=0; s2[pos2] != '\0'; pos2++) {
    if (s2[pos2] == '/') {
      if (slash != TRUE) {
	s1[pos1++] = s2[pos2];
      }
      slash = TRUE;
    } else {
      slash = FALSE;
      s1[pos1++] = s2[pos2];
    }
  }

  if (pos1 == 0) {
    *s1 = '\0';
    return;
  }

  if (pos1 == 1 || s1[pos1-1] != '/')
    s1[pos1] = '\0';
  else 
    s1[pos1-1]='\0';
}


addnode(list, path)
LISTNODE *list;
char *path;
{
    LISTNODE *t1, *t2;		/* temporary pointers */
    int pathlen;                /* size of the part of the string we want */

    t1 = list->ptr;
    t2 = list;
    while(t1 != (LISTNODE *)NULL) {		/* sort list as we go */
	if(strcmp(path, t1->val) == 0)		/* don't add duplicates */
	    return;
	if(mystrcmp(t1->val, path) > 0)
	    break;				/* put new node here */
	t2 = t1;
	t1 = t1->ptr;
    }

    pathlen = strlen(path);

    temp = (LISTNODE *)fmalloc(sizeof(LISTNODE));
    temp->val = fmalloc(strlen (path) + 1);
    
    cleanpath (temp->val, path);

    temp->aflag = FALSE;
    temp->ftype = FALSE;
    t2->ptr = temp;
    temp->ptr = t1;
}  /* end addnode() */


addlist(filelist)
char *filelist;
{
    FILE *fp;				/* access to name */
    int i;				/* local counter  */

    if((fp = fopen(filelist, "r")) == NULL) {
	(void) sprintf(msg, (catgets(nlmsg_fd,NL_SETN,249, "(3249): unable to open graph file %s")),filelist);
	warn(msg);
	exit(1);
    }

    while(fgets(msg, MAXS, fp) != NULL) {
	i = 1;
	if((msg[0] != 'e') && (msg[0] != 'i')) {
	  (void) sprintf(msg, (catgets(nlmsg_fd,NL_SETN,248, "(3248): missing i or e in graph file %s")),filelist);
	  warn(msg);
	  exit(1);  /* corrupt graph file exiting is safest thing to do */
	}
	while(isspace((int)msg[i]))
	    i++;
	msg[strlen(msg)-1] = '\0';
	if(msg[0] == 'e')
	    addnode(elist, &msg[i]);
	else
	    addnode(ilist, &msg[i]);
    }

    (void) fclose(fp);
}    


#define MAXCONFLINE (MAXPATHLEN+32)
int
doconfig(configfile)
char *configfile;
{
    FILE *configfp;			/* access to config file */
    char line[MAXCONFLINE];		/* line from config file */
    char label[32];			/* action identifier */
    char value[MAXPATHLEN];		/* file name to execute */
    int  n, cnt = 0;

    if ((configfp = fopen(configfile, "r")) == NULL) {
	(void) sprintf(msg,(catgets(nlmsg_fd,NL_SETN,241, "(3241): unable to open configuration file %s")), configfile);
	warn(msg);
	return;
    }
    while (fgets(line, MAXCONFLINE, configfp) != NULL) {
	cnt++;
	n = sscanf(line, "%s%s\n", label, value);
	switch (n) {
	case -1:
	    break;
	case 2: 
	    if (!strcmp(label, "sync"))
		synclimit = atoi(value);
	    else if (!strcmp(label, "chgvol")) {
		(void) strcpy(chgvolfile, value);
		dochgvol++;
	    }
	    else if (!strcmp(label, "error")) {
		(void) strcpy(errfile, value);
		doerror++;
	    }
	    else {
		(void) sprintf(msg,(catgets(nlmsg_fd,NL_SETN,242, "(3242): illegal configuration file label (%s) ignored")),
			label);
		warn(msg);
	    }
	    break;
	default:
	    (void) sprintf(msg,(catgets(nlmsg_fd,NL_SETN,243, "(3243): configuration file error at line %d")), cnt);
	    warn(msg);
	    (void) sprintf(msg,(catgets(nlmsg_fd,NL_SETN,244, "(3244): bad line:\n%s")), line);
	    warn(msg);
	    break;
	}
    }
    (void) fclose(configfp);
}


int
volheader_print()
{

  FILE *fd = 0;
  
  if(strcmp(volhdrfile, "-") == 0) {
    volhdrfile = "/dev/tty";
  }
  if( (fd = fopen(volhdrfile, "w")) == 0) {
    (void) sprintf(msg,(catgets(nlmsg_fd,NL_SETN,250, "(3250): unable to open volume header file %s")), volhdrfile);
    warn(msg);
    return;
  }

  fprintf(fd, "Magic Field:%s\n", vol.magic);
  fprintf(fd, "Machine Identification:%s\n", vol.machine);
  fprintf(fd, "System Identification:%s\n", vol.sysname);
  fprintf(fd, "Release Identification:%s\n", vol.release);
  fprintf(fd, "Node Identification:%s\n", vol.nodename);
  fprintf(fd, "User Identification:%s\n", vol.username);
  fprintf(fd, "Record Size:%d\n", atoi(vol.recsize));
  fprintf(fd, "Time:%s\n", vol.time);
  fprintf(fd, "Media Use:%d\n", atoi(vol.mediause));
  fprintf(fd, "Volume Number:%d\n", atoi(vol.volno));
  fprintf(fd, "Checkpoint Frequency:%d\n", atoi(vol.check));
  fprintf(fd, "Fast Search Mark Frequency:%d\n", atoi(vol.fsmfreq));
  fprintf(fd, "Index Size:%d\n", atoi(vol.indexsize));
  fprintf(fd, "Backup Identification Tag:%d %d\n", atoi(vol.backupid.ppid),  atoi(vol.backupid.time));
  fprintf(fd, "Language:%s\n", vol.lang);

  (void) fclose(fd)  ;
}



int expand_fn(tmp)
     struct fn_list *tmp;
{
  struct fn_list *t;
  struct fn_list *r;
  struct fn_list *tail;  
  char dirname[MAXPATHLEN+1];
  char fn[MAXPATHLEN+1];
  struct fn_list *res;
  int match;

  DIR *dirp;
  struct dirent *dp;
  

  /* loop through the tmp list expanding each name and
     puuting the result in the res list.
  */
  
  t = tmp;
  res = (struct fn_list *)NULL;
  tail = (struct fn_list *)NULL;
  
  while (t != (struct fn_list *)NULL) {

#ifdef DEBUG
fprintf(stderr, "expand_fn pattern: %s\n", t->name);
#endif
    if (((strcmp(t->name, "-")) == 0) || ((strchr(t->name, ':')) != NULL)) {  
      /*  stdin or stdout, entry done add to end of res list */
      /* or have a remote name, entry done add to end of res list */
      if ((r = malloc((unsigned) sizeof(struct fn_list))) == NULL) {
	return(-1);
      }
      strcpy(r->name, t->name);
      r->next = (struct fn_list *)NULL;
      if (res == (struct fn_list *)NULL) {
	res = r;
	tail = r;
	tail->next = res;
      }
      else {
	tail->next = r;
	tail = r;
	tail->next = res;
      }
      t = t->next;
      continue;
    }  /* end if (- || :) */
    
    if (((strchr(t->name, '/')) == NULL)) {
      /* check cwd for match */
      if ((dirp = opendir(".")) == NULL) {
	return(-2);
      }
      while ((dp = readdir(dirp)) != NULL) {
	match = 0;
	if ((fnmatch(t->name, dp->d_name, FNM_PATHNAME)) == 0) {
	  if ((r = malloc((unsigned) sizeof(struct fn_list))) == NULL) {
	    return(-1);
	  }
	  match++;
	  strcpy(r->name, dp->d_name);
	  r->next = (struct fn_list *)NULL;
	  if (res == (struct fn_list *)NULL) {
	    res = r;
	    tail = r;
	    tail->next = res;
	  }
	  else {
	    tail->next = r;
	    tail = r;
	    tail->next = res;
	  }
	}
      }
      if (match == 0) { 
	/* probable output to file that does not yet exist */
	if ((r = malloc((unsigned) sizeof(struct fn_list))) == NULL) {
	  return(-1);
	}
#ifdef DEBUG
fprintf(stderr, "expand_fn: zero match: %s\n", t->name);
#endif
	strcpy(r->name, t->name);
	r->next = (struct fn_list *)NULL;
	if (res == (struct fn_list *)NULL) {
	  res = r;
	  tail = r;
	  tail->next = res;
	}
	else {
	  tail->next = r;
	  tail = r;
	  tail->next = res;
	}
      }
      if ((closedir(dirp)) == -1) {
	return(-2);
      }
      t = t->next;
    } /* end if cwd */
    else { 
      /* get directory prefix and look */
      strcpy(dirname, t->name);
      strcpy(fn, strrchr(t->name,'/')+1);
      dirname[strlen(dirname) - strlen(fn) -1] = '\0';

#ifdef DEBUG
fprintf(stderr, "expand_fn: dirname: %s\n", dirname);
fprintf(stderr, "expand_fn: fn: %s\n", fn);
#endif
      
      if ((dirp = opendir(dirname)) == NULL) {
#ifdef DEBUG
perror("expand_fn: opendir error");
#endif
	return(-2);
      }
      match = 0;
      while ((dp = readdir(dirp)) != NULL) {
	if ((fnmatch(fn, dp->d_name, FNM_PATHNAME)) == 0) {
	  if ((r = malloc((unsigned) sizeof(struct fn_list))) == NULL) {
	    return(-1);
	  }
	  match++;
	  strcpy(r->name, dirname);
	  strcat(r->name, "/");
	  strcat(r->name, dp->d_name);
	  r->next = (struct fn_list *)NULL;
	  if (res == (struct fn_list *)NULL) {
	    res = r;
	    tail = r;
	    tail->next = res;
	  }
	  else {
	    tail->next = r;
	    tail = r;
	    tail->next = res;
	  }
	}
      }
      if (match == 0) { 
	/* probable output to file that does not yet exist */
	if ((r = malloc((unsigned) sizeof(struct fn_list))) == NULL) {
	  return(-1);
	}
	strcpy(r->name, t->name);
	r->next = (struct fn_list *)NULL;
	if (res == (struct fn_list *)NULL) {
	  res = r;
	  tail = r;
	  tail->next = res;
	}
	else {
	  tail->next = r;
	  tail = r;
	  tail->next = res;
	}
      }
      if ((closedir(dirp)) == -1) {
	return(-2);
      }
      t = t->next;
    }  /* end else directory prefix */
  }  /* end while (t) */

  n_outfiles = 1;
  t = res;
#ifdef DEBUG
fprintf(stderr, "expand_fn: res output file: %s\n", res->name);
#endif
  while (t->next !=res) {
    n_outfiles++;
    t = t->next;
#ifdef DEBUG
fprintf(stderr, "expand_fn: output file: %s\n", t->name);
#endif
  }
  if (n_outfiles > MAXOUTFILES) {
    return(-3);
  }
  outfile_head = res;
  
  return(0);
  
}  /* end expand_fn() */

