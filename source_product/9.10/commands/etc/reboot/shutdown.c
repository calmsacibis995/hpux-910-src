static char *HPUX_ID = "@(#) $Revision: 72.4 $";
/*
 * shutdown.c:  This is the source file for the shutdown(1m) command.
 * 		The shutdown command was rewritten in 8.0 from a shell
 *		program to a C program which can be setuid root when run
 *		with the -r and -h options (reboot and halt, respectively)
 *		by normally unprivleged users which have been specified
 * 		in an allow file.  See the 8.0 LMFS ERS/IRS for detail on 
 *		these and other changes made to the shutdown command in 8.0.
 *
 *
 *  The basic algorithm for shutdown is as follows:
 *      
 * 1.  Parse options.
 *
 * 2.  Verify authorization to use shutdown.  (You have to be included in a
 *                                             special file as being authorized
 *                                             to shut down a particular system
 *                                             in order to pass this test, or
 *                                             you have to be root and the file
 *                                             must be absent or of 0 length.
 *
 * 3.  Warn users on clients as well as locally that the system will be shut
 *       down.
 *
 * 4.  Execute user-cusomizable scripts in /etc/shutdown.d.  
 *       The intent of the directory is that if users have special needs 
 *       that should occur during the shutdown process, they can be included
 *       in shell script in this directory.  Files are executed using the
 *       system(3) call so use the /bin/sh shell.  They are executed in
 *       machine collated (ASCII) order.
 *
 * 5.  Shut down accounting.
 *
 * 6.  Reboot if the reboot or halt options were chosen and the system is not
 *       a disc mirrored system and the system is not an auxiliary swap server
 *       with a local mounted disc.  (This will cause all clients to reboot by
 *                                    running reboot on each one of them.  In
 *                                    addition, the reboot command kills all
 *                                    processes - using SIGTERM and then 
 *                                    SIGKILL.  Even though file systems are 
 *                                    not unmounted by reboot, the mounttab 
 *                                    file will be updated by syncer to reflect
 *                                    the fact that any local file systems are 
 *                                    no longer available.)
 *
 * If the system is being brought down to a single user state, or if it is a
 * disc mirrored system, or it is an auxiliary swap server with a local
 * mounted file system, Step 6 from above does not occur, but the following
 * things do occur.
 *
 *  6.  All clients are rebooted using /etc/reboot.
 *
 *  7.  The /etc/killall script is run.  (This script runs in 2 passes.  During
 *                                        the first pass, all processes not on
 *                                        a list are sent SIGTERM.  During the
 *                                        second pass, all the processes from
 *                                        the first pass that didn't quit are
 *                                        sent SIGKILL and all processes not
 *                                        on a second list are sent SIGTERM.)
 *
 *  8.  Auditing is shut down.
 *
 *  9.  File systems are unmounted.
 *
 * 10.  The system is rebooted using /etc/reboot if it was a mirrored disc 
 *         or auxiliary swap server with a local mounted disc reboot/halt 
 *         request
 *      OR
 *         init is sent a signal to transition to single user state.
 *
 *
 *  NOTE:  This code has dependencies on the getmount_cnt system call.  It
 *         is used in places that are both #ifdef DISKLESS and not.  These
 *         calls are not inside a #ifdef GETMOUNT.
 */

#include <sys/pstat.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <cluster.h>
#include <sys/nsp.h>
#include <memory.h>
#include <stdio.h>
#include <dirent.h>
#include <string.h>
#include <ctype.h>
#include <pwd.h>
#include <sys/signal.h>
#include <errno.h>
#include <mntent.h>
#include <unistd.h>
#include <limits.h>
#include <sys/getaccess.h>
#include <netdb.h>
#include <utmp.h>
#include <sys/param.h>
#include <netinet/in.h>
#include <arpa/nameser.h>
#include <resolv.h>
#ifdef TRUX
#include <sys/security.h>
#endif
#if defined (TRUX) || defined (AUDIT)
#include <sys/audit.h>
#endif
#include <sys/wait.h>
#include <stdlib.h>
#include <fcntl.h>
#include <termio.h>
#include <sys/ioctl.h>

/*
 * Macros
 */
#define TRUE 	1
#define FALSE 	0

#define PSBURST 10				/* # pstat structs to get    */
#define MAXINPUTSTRING 1024			/* Max size of input_buffer  */
#define MAXOUTPUTSTRING MAXINPUTSTRING+256	/* Max size of output_buffer */
#define TIMEBUFSIZE 1024			/* Buffer size for time str  */
#define LARGEBUFSIZE  4096			/* Buffer size               */
#define UNAMESIZE	20			/* max chars in a user name  */
#define WAIT_BEFORE_SIGNAL 15			/* number of seconds waited  */
				/* before sending signal to processes        */
#define TEN_SECONDS 10				/* time constants */
#define TWENTY_SECONDS 20
#define THIRTY_SECONDS 30

#define DEFAULT_GRACE_PERIOD 60			/* one minute */
#define CMD_LENGTH_PAD	4		        /* pad for additional quotes */
#define MAXLINESIZE 1024

/* 
 *  These are bit masks used in option parsing to tell if an option has
 *  already been processed and put into the argv array that is being built
 *  for the local reboot command.
 */
#define HALT_BIT	1
#define DEVICE_BIT	(1 << 1)
#define FILE_BIT	(1 << 2)
#define DEBUG_BIT	(1 << 3)

static char SHUTDOWN_ALLOW [] = "/etc/shutdown.allow"; /* Allow file name     */
static char REBOOT_CMD [] = "/etc/reboot";    /* Reboot command to use        */
static char END_CMD [] = "' | /etc/wall";     /* End of WALL command          */
static char START_CMD [] = "echo '";          /* Beginning of WALL command    */
static char CMD_MISC [] = " 2>&1 &";          /* Add 0 to 1 and don't wait    */
static char ROOT [] = "root";		      /* User name for rcmd           */

static char REDIRECT_STDERR[] = " 2>&1";      /* Redirecting stderr to stdout */
static char STDOUT_TO_NULL[] = " >/dev/null"; /* Throw away stdout            */
static char STDERR_TO_NULL[] = " 2>/dev/null"; /* Throw away stderr           */
static char MOUNT_CMD[] = "/etc/mount -l";    /* Determine lmfs		      */
static char FUSER_CMD[] = "/etc/fuser ";      /* fuser command to use         */
static char SIGTERM_CMD[] = "/bin/kill -SIGTERM "; /* for sending SIGTERM     */
static char KILL_CMD[] = "/bin/kill -SIGKILL ";    /* for sending SIGKILL     */

static char CUSTOM_SCRIPTS [] = "/etc/shutdown.d"; /* Directory where user    */
						   /* customizable scripts    */
						   /* shutdown will be found  */
int CUSTOM_SCRIPTS_LEN;

/*
 * Option string for getopt -- Note s800 doesn't support -d or -f
 *
 *  New options:  -D<debugLevel>  This is a hidden option, mostly for testing
 *                                purposes that prints out extra information.
 *                
 *                -y              This is a "batch" flag which supresses all
 *                                of the interactive questions.  The default
 *                                is to still print all the interactive 
 *                                questions.  A 'y' option was used instead
 *                                of 'b' since System V.3 uses a 'y'.  In V.3
 *                                'y' answers 'yes' to all questions.  Here
 *                                it just causes all questions to be skipped.
 */
#ifdef hp9000s300
static char OPTIONS [] = "hrd:f:D:y";		/* command line options */
#endif /* hp9000s300 */
#ifdef hp9000s800
static char OPTIONS [] = "hrD:y";		/* command line options */
#endif /* hp9000s800 */

/*
 * Default wall message sent by shutdown.  Also default cwall message that
 * is used if unmounting local discs fails.
 */
static char DEFAULT_WALL_MESSAGE [] = "PLEASE LOG OFF NOW ! ! !\n \
	System maintenance about to begin.\n";

static char START_WARNING [] = "echo 'System ";
static char END_WARNING [] = " is shutting down.\n \
	Its local file systems are no longer available.\n' ";
static char END_KILL_WARNING [] = " is shutting down.\n \
	Its local file systems are no longer available.\n\n \
	Processes accessing those local file systems will be \n \
	terminated in 30 seconds.\n'";

static char PIPE_TO_CWALL [] = " | /etc/cwall 2>&1";
static char PIPE_TO_WALL [] = " | /etc/wall 2>&1";
static char CLUSTER_WARNING [] = "Warning entire cluster of file system going away -- continuing shutdown.\n";
static char START_CANNOT_KILL_PROCESSES [] = "Cannot kill processes on ";
static char END_CANNOT_KILL_PROCESSES [] = " which may be accessing our local mounted\n    file systems -- continuing shutdown.\n";

char graceString [100];
/*
 * Globals used for root server and swap server reboot information
 *   Made global so that the active_clients() routine could use the same
 *   variables, especially mysite, rootserver, and swapserver
 */
cnode_t mysite = 0;			/* Cnode id of my site 		  */
struct  cct_entry *cct_ent;		/* /etc/clusterconf table entry	  */	
int 	rootserver = FALSE;		/* boolean for root server	  */
int 	swapserver = FALSE;		/* boolean for swap server	  */
int 	num_active_clients;		/* # clients of this root/swap svr*/
cnode_t active_clients[MAXCNODE];	/* active client cnode ids 	  */
struct	entry 
	{
	    char *name;			/* name of cnode 		  */
	    int	donot_reboot;		/* true if shouldn't be rebooted  */
	} cnode_array [MAXCNODE];	/* names of cnodes in clusterconf */

char hostname [MAXHOSTNAMELEN + 1]; 	/* hostname of this machine     */
int  NetworkInUse = TRUE;               /* true if networking is in kernel */

/*
 *  Subroutine definitions.
 *
 *  Routines with no parameters first, then "STDC" declarations, then 
 *  "!STDC" declarations.
 */
int 	num_local_mounts ();		  /* count local fs mounts	    */
int	clients_rebooted ();		  /* waits for client reboot 	    */
void 	get_cnode_names ();		  /* initializes global cnode_array */
void	runCustomScripts ();              /* executes custom script files   */
void 	debugHandler ();                  /* ALRM sig handler for debugLevel*/
void 	usage ();			  /* print usage message  	    */
struct  utmp *getutent ();
cnode_t	cnodeid ();
void	sync ();
void	endutent ();

#ifdef __STDC__				  /* ansi c declarations 	    */

int 	allowed (char *, char *, char *); /* allow user setuid?		    */
int 	kill_process (char *);		  /* kill specified process	    */
void	print_active_clients (FILE *);	  /* prints names of active clients */
int	selectFiles (struct dirent *);    /* pick custom script files       */
int	ASCIIcompare (struct dirent **, struct dirent **); /* "strcmp"      */
int     warn_users_of_lmfs (struct servent *, char *);
					  /* warn users with open processes */
					  /* on lmfs 			    */
int  	warn_cnode (char *, char *, struct servent *, int);
					  /* warns cnode of lmfs going away */
void 	signal_processes (char *, char *, struct servent *, char *);	  
					  /* send signals to specified PIDs */

#else /* __STDC__ */			  /* old style c declares	    */

int 	allowed ();			  /* allow user setuid?		    */
int 	kill_process ();		  /* kill specified proc	    */
void	print_active_clients ();	  /* prints names of active clients */
int	selectFiles ();                   /* pick custom script files       */
int	ASCIIcompare ();                  /* does a strcmp with file names  */
int     warn_users_of_lmfs ();  /* warn users with open processes */
					  /* on lmfs 			    */
int  	warn_cnode (); 			  /* warns cnode of lmfs going away */
void 	signal_processes ();	  	  /* send signals to specified PIDs */

#endif /* __STDC__ */

/*  
 *  These are for debugging purposes; they use a hidden option to print out
 *  extra information.
 */
FILE *debugStream = NULL;
int debugLevel = 0;
int quitRead = FALSE;

/**********************************************************************/
main (argc, argv)
int argc;
char **argv;
/**********************************************************************/
{

    /* 
     * General use variables
     */
    char input_buffer [MAXINPUTSTRING + 1]; /* used to get input from user */
    struct termio ttyStruct;            /* to figure out EOF character     */
    char eofChar;                       /* stores real EOF char to print   */
    char *remoteWallCmd;		/* pointer to real wall command    */
    char yesNoAns;                      /* used to get Y/N input           */
    int index;				/* index to active_cnodes array    */
    int umountCount;                    /* loop counter for umount on root */

    struct passwd *passwd_ptr;		/* points to a passwd entry 	   */
    char *loginName;			/* points to user login name       */
    struct stat slash_statbuf; 		/* stat buffer for "/"		   */
    struct stat cwd_statbuf;		/* stat buffer for "."		   */

    char *errptr;			/* return value from strol 	   */

    time_t time_buf;			/* buffer to get system time	   */
    char time_str_buf [TIMEBUFSIZE];	/* buf for string trans of time    */

    FILE *writep;			/* write file pointer for popen    */
    int read_size;			/* size of data read from fread    */

    int num_users = 0;			/* count # of users logged in	   */
    int num_lmfs;			/* number of local mounts- mnt tbl */

    int batchFlag = FALSE;              /* if TRUE, skip questions         */

    int saveId;                         /* used to save real uid           */

#if defined (DISKLESS) && defined (AUDIT)

    int	audit_on = FALSE;		/* TRUE if auditing is "on" */
    int	auditserver = FALSE;		/* TRUE if local audit files */
    char caf[1024], naf[1024];		/* current and next audit log files */
    cnode_t caf_cnode=0, naf_cnode=0;	/* cnodes where current and next */
					/* audit log files are mounted */

    int audit_off_early = FALSE;	/* indicates auditing is to be     */
					/* shutdown earlier than normal */

    int bad_audit_info = FALSE;		/* indicates audit_info failed */
					/* to retrieve audit log info */
#else
#ifdef TRUX

    int audit_off_early = FALSE;	/* indicates auditing is to be     */
					/* shutdown earlier than normal */
#endif
#endif /* DISKLESS and AUDIT */

    /*
     * getopt(3) variables
     */
    extern char *optarg;	/* external from getopt - arg to option */
    extern int optind;		/* external from getopt - index into options */

    /*
     * option parsing variables
     */
    int opt;			/* option variable returned from getopt    */
    int halt = FALSE;		/* TRUE if -h option to halt specified 	   */
    int reboot = FALSE;		/* TRUE if -r option to reboot spec'd	   */
    int newdevice = FALSE;	/* TRUE if -d option specified             */
    int newfile = FALSE;	/* TRUE if -f option specified             */
    char debugOpt [10];		/* saves -D<int> for client reboots        */
    char *grace = NULL;		/* grace period until shutdown 		   */
    unsigned int grace_period;	/* integer transl of grace in seconds 	   */
    char *rebootArgv [9];       /* building argv for reboot command        */
    int argvIndex;              /* index into reboot argv array            */
    int argvFlag = 0;           /* flag to tell what options added to argv */

    /*
     * signal handling variables
     */
    struct sigaction action;		/* action struct for signals	*/

    /*
     * utmp structure
     */
    struct utmp *utmp_ptr;

    /*
     *  Variables used to figure out if we're running on a mirrored disc
     *  system.  (Assume we're not, then later we'll figure out if we are.)
     */
    struct stat mirrorStat; 		/* stat buffer for "/etc/mirrortab" */
    int mirroredDiscs = FALSE;          /* boolean for we are/aren't mirror */

    /* 
     * variables for use in rcmd(2) of reboot to root/swap clients
     */
    int rcmd_socket;		        /* socket descr for rcmd */
    struct servent *sock_port;	        /* socket port structure */
    char *ahost;		        /* dummy hostname pointer for rcmd */
    struct sigaction debugAction;	/* action struct for signals	*/

    /* 
     * variables for debugging the rcmd commands
     */
    FILE *rcmdOutErrStream = NULL;
    int rcmdOutErr;

    /*
     *  If a umount of local discs is attempted but fails, a general cwall is
     *  sent to all machines in the cluster to warn them.
     */
    char cwallCommand [200];            /* space for a cwall on umount error */

    /*
     *  Cluster information variables:
     *
     *  standalone if cnodes() == 0
     *  my cnode id for this site comes from cnodeid; don't both if we're
     *  standalone - it was initiailized to 0
     */
    int standalone = ! cnodes ((cnode_t *) NULL); 

    if (! standalone)
      mysite = cnodeid ();	



    /* 
     *  Parse arguments using getopt.  This used to be after checking for
     *  permission to run shutdown, resetting IFS and PATH, and checking 
     *  that we are running on the root volume and sync'ing the discs.
     *  It was moved here for performance reasons in the case of an illegal
     *  shutdown command.
     *
     *  First, initialize the reboot argv with the reboot command in argv[0]
     *  and all the other entries as NULL.  Set an index variable to 1; ready
     *  to fill in argv[1].  The argvFlag will be used to tell what options
     *  have already been put into the argv array.  If the corresponding
     *  bit in the flag is set, then the data is not added to the argv array.
     *  If it isn't set, the string is added, then the bit is set in the flag.
     *  The bits are as follows:
     *
     *     -h  - bit 0 (see HALT_BIT defintion)
     *     -d  - bit 1 (see DEVICE_BIT defintion)
     *     -f  - bit 2 (see FILE_BIT defintion)
     *     -D  - bit 3 (see DEBUG_BIT defintion)
     *
     *  Since all the entries in the argv array were initialized to NULL, the
     *  last one doesn't need to be set at the end of the option parsing.
     */

    rebootArgv [0] = REBOOT_CMD;

    for (argvIndex = 1; argvIndex < 9; argvIndex ++)
      rebootArgv [argvIndex] = NULL;

    argvIndex = 1;

    while ((opt = getopt (argc, argv, OPTIONS)) != EOF) 
      {
	switch (opt)
	  {
	    case 'r':	
		    reboot++;
		    break;
	    case 'h': 
		    halt++;

		    if (! (argvFlag & HALT_BIT))
		      {
		        rebootArgv [argvIndex ++] = "-h";
		        argvFlag |= HALT_BIT;
		      }

		    break;
#ifdef hp9000s300
	    case 'd': 
		    newdevice ++;

		    /*  
		     *  First check for a NULL device file.  This is not
		     *	allowed.
		     */
		    if (*optarg != '/')
		      {
			(void) fprintf (stderr, 
         "The \"-d\" option requires a full path argument to a device file.\n");
			(void) fprintf (stderr, "-- exiting shutdown.\n");
		        usage ();
		        exit (1);
		      }

		    if (! (argvFlag & DEVICE_BIT))
		      {
		        rebootArgv [argvIndex ++] = "-d";
		        rebootArgv [argvIndex ++] = optarg;
		        argvFlag |= DEVICE_BIT;
		      }

		    break;
	   case 'f':
		    newfile ++;

		    /* 
		     *   Copy the argument.
		     *
		     *  In the case of '-f ""', this will cause the 
		     *  reboot program to properly see the argument to the -f
		     *  option as an empty string.
		     */

		    if (! (argvFlag & FILE_BIT))
		      {
		        rebootArgv [argvIndex ++] = "-f";
		        rebootArgv [argvIndex ++] = optarg;
		        argvFlag |= FILE_BIT;
		      }

		    break;
#endif /* hp9000s300 */

 	   case 'y': 
		   batchFlag = TRUE;
		   break;


 	   case 'D': 
                   /*
                    *  This is a hidden option which can be used for
                    *  debugging purposes.  The debugLevel is assumed to
                    *  be an integer.  The log file where debug messages
                    *  are written is put into / since this directory is
                    *  guaranteed not to be on a mounted volume.  If this
                    *  file  cannot be opened, the debugLevel is reset to
                    *  0 so that no attempts are made to write to the log
                    *  file.  The debugLevel is intended to allow various
                    *  modes of debugging.  The higher the number, the
                    *  more information written to the log file.  This
                    *  option is meant as an aid to regression testing,
                    *  but could also be used for debugging purposes in
                    *  the field.
                    *
                    *  The file is opened with no buffering.
                    */
		   debugLevel = atol (optarg);
		   if (! debugStream)
		     {
		       if ((debugStream = fopen ("/shutdownDLog", "a")) == NULL)
		         debugLevel = 0;

		       else
			 (void) setvbuf (debugStream, (char *) NULL, _IONBF, 0);
		     }

		   if (! (argvFlag & DEBUG_BIT))
		     {
		        rebootArgv [argvIndex ++] = "-D";
		        rebootArgv [argvIndex ++] = optarg;
		        argvFlag |= DEBUG_BIT;
			
			/*
			 *  Copy the debug information into a string to be
			 *  used to pass along to client reboots.
			 */
			(void) strcpy (debugOpt, "-D");
			(void) strcat (debugOpt, optarg);
		     }

		    break;

 	   default:  usage ();
		     exit (1);
	 }
     }

    /*
     *  If stdin is not a tty, then if we try to read from it, in
     *  interactive mode, the chances are that the process will block.
     *  This shouldn't occur since shutdown is supposed to be an interactive
     *  program, but it might occur if the user does something like starting
     *  shutdown in the background.  So the check here is that if the
     *  batchFlag is not set, then stdin must be a tty.  
     *
     *  Note that even if the batchFlag is set, if stdout is not a tty, the
     *  process could still be stopped.  That is, shutdown -y >/dev/null is
     *  OK, but shutdown -y & could hang on output to stdout if the tty
     *  driver is set so that background processes are stopped on output
     *  to the terminal.
     */
    
    if ((! batchFlag) && (! isatty (0)))
      {
	(void) fprintf (stderr, 
 "In interactive mode shutdown must be run from an interactive input device\n");
	(void) fprintf (stderr,
	    "Use the -y option for non-interactive mode-- exiting shutdown.\n");
	exit (1);
      }

    /*
     * Check for a grace period specified 
     * Note that optind was incremented during each iteration of 
     * the getopt loop, so it already points to the grace period
     * if there is one.
     */
    if (optind < argc)
     {
	grace = argv [optind];		/* just keep a pointer to it */

	if (optind < (argc - 1))	/* check for extraneous params */
	 {
	    (void) fprintf (stderr, 
			    "Too many parameters-- exiting shutdown.\n");
	    usage ();
       	    exit (1);
	 }
     }

    /*
     * Check grace period for validity, and translate to an integer
     * If no grace period specified, default to 60 seconds
     */
    if (grace) 
      {
	/* 
	 *  check for special now option 
	 */
	if (strcmp (grace, "now") == 0) 
	    grace_period = 0;

	else 
	  {
	    /* 
	     *  By making the base NULL, strtol takes its base from the 
	     *  format of the string; 0<digit> => octal, 0x<digit> => hex,
	     *  <digit other than 0> => decimal.
	     *
	     *  The only hitch is that if strtol thinks it has a hex number,
	     *  but the next digit isn't valid (0xj) then it seems to try it
	     *  as an octal number, then finally as decimal and returns 0.
	     *  In this case errptr points to xj, so we can't tell that it is
	     *  an error.  Similarly, 09 returns 0 as the number and errptr
	     *  points to 9 so we can't tell that an error occurred.
	     */
	    grace_period = strtol (grace, &errptr, NULL);  

	    /* 
	     *  If strtol can't make a number, errptr points to grace and
	     *  0 is returned.  If overflow occurs, LONG_MAX or LONG_MIN
	     *  is returned and errno is set to ERANGE.
	     */
	    if (((errptr == grace) && (grace_period == 0)) 
	          || (((grace_period == LONG_MAX) || (grace_period == LONG_MIN))
		        && (errno == ERANGE)))
	      {
	        (void) fprintf (stderr, 
        "Illegal value specified as grace period-- exiting shutdown.\n", grace);
	        usage ();
	        exit (1);
	      }
	  }
      }

    else 
      {
	/* 
	 *  otherwise, default 1 min 
	 */
	grace_period = DEFAULT_GRACE_PERIOD;	
      }
    
    /*
     *  Save the grace period in a string for use with the wall command.
     */
    (void) sprintf (graceString, 
	     "All processes will be terminated in %d seconds.\n", grace_period);

    /*  
     *  Figure out hostname for use in allowed() and for debugging if that
     *	is chosen.
     */
    if ((gethostname (hostname, (sizeof (hostname) - 1))) == -1)
      hostname [0] = '\0';

    if (debugLevel) {
      (void) fprintf (debugStream, "++++++++++++++++++++++++++++++++++++++\n");
      (void) fprintf (debugStream, "%s: grace_period: %d\n", hostname, 
		      grace_period);
    }

    /*
     * Ensure that we're REALLY root UNLESS we're rebooting or halting
     * 		(can't allow non-root user to get into root shell via 
     *		shutdown even if they're specified in shutdown.allow)
     *
     *  This is a potential problem for B1.  It will have to be changed
     *  to look for the proper authorization for going to single user state
     *  rather than looking for root.
     */
    if (!(reboot || halt) && (getuid () != 0)) 
      {
        (void) fprintf (stderr, 
    "Must be super-user to shutdown system into state S-- exiting shutdown.\n");
        exit (1);
      }

    /*
     * Check for illegal option combinations
     */
    if (reboot && halt)
      {
	(void) fprintf (stderr, 
			"Conflicting options -h and -r-- exiting shutdown.\n");
	usage ();
	exit (1);
      }

#ifdef hp9000s300
    if (!reboot && newdevice)
      {
	(void) fprintf (stderr, 
             "-d option may only be used with -r option-- exiting shutdown.\n");
	usage ();
	exit (1);
      }

    if (!reboot && newfile)
      {
	(void) fprintf (stderr, 
             "-f option may only be used with -r option-- exiting shutdown.\n");
	usage ();
	exit (1);
      }

#endif /* hp9000s300 */

    if (debugLevel)
      (void) fprintf (debugStream, 
	       "%s: reboot: %d, halt: %d, newdevice: %d, newfile: %d\n", 
	       hostname, reboot, halt, newdevice, newfile);

    /*
     * Clear IFS here -- Explicitly reset IFS=space, tab, new-line to prevent
     *			security hole in popen/system calls
     */
    if (putenv ("IFS= \t\n") != 0)
      {
        /* 
	 *  encountered error cleaning up IFS variable 
	 */
        perror ("putenv");
        (void) fprintf (stderr, 
        "Unable to initialize IFS in shell environment-- exiting shutdown.\n");
        exit (1);
      }

    /*
     *  Reset the PATH to /bin and /usr/bin so that any user-supplied shell
     *  scripts will only have these in the PATH unless explicitly reset in
     *  the script.  This will prevent scripts which do no use full path
     *  names from executing commands in unexpected places, based on the
     *  user's PATH.
     */
     if (putenv ("PATH=/bin:/usr/bin") != 0)
       {
        perror ("putenv");
        (void) fprintf (stderr, 
        "Unable to reset PATH in shell environment-- exiting shutdown.\n");
        exit (1);
      }

    /* DSDe412612, SR#: 1653065086
     * using getlogin, an end user could run shutdown as follows,
     *     shutdown -y -r 999999 < /dev/null > dev/console
     * they could perform a shutdown even if not superuser or in the
     * /etc/shutdown.allow file, if the console were logged on as root.
     * Changed code so that getlogin was not used any more as a 1st try.
     */

    /* Try the password file with the real user id.  Note that this will get
     * the first entry in the file with the correct user id.  This also
     * returns a pointer to static memory.  If this also fails, then quit.
     * But if it worked, set loginName to point to the user name.  This pointer
     * can then be used in either case in the call to allowed().
     */

    /* DSDe412612, SR#: 1653065086
    if ((loginName = getlogin ()) == NULL)
      {
     */
        if ((passwd_ptr = getpwuid (getuid ())) == 0)
          {
	    perror ("getpwuid");
	    (void) fprintf (stderr, 
			    "Unable to get username.\n");
	    (void) fprintf (stderr, 
	     "User not allowed to shut down this system-- exiting shutdown.\n");

	    exit (1);
          }
	
	loginName = passwd_ptr -> pw_name;
    /* DSDe412612, SR#: 1653065086
      }
     */

    if (!allowed (loginName, hostname, SHUTDOWN_ALLOW))
      {
	(void) fprintf (stderr, 
       "User %s not allowed to shutdown this system (%s)-- exiting shutdown.\n",
	      loginName, hostname);
	exit (1);
      }

    /*
     * First, verify that our current working directory is on the
     * root volume as other volumes may be umounted...
     *
     *  Stat / and . and check that the devices are the same.  If so, cd to /.
     *  If the devices are different then exit.  Stat wil fail if part of the
     *  path is not a directory, the file doesn't exit, search permission is
     *  denied, the path or buffer points to an invalid address, too many
     *  symbolic links were found, or the path is too long.  None of these
     *  should occur since we're stat'ing / and ., and we're running as root.
     *  So if stat fails there is a real problem with the system.  For this
     *  reason exit.  (Also, if we get a NULL back, then the structures
     *  weren't filled in and we'll get garbage on the next check when we
     *  try to compare the devices.
     */
    if (stat ("/", &slash_statbuf) != 0) 
      {
	(void) fprintf (stderr, 
		       "Unable to stat the root directory-- exiting shutdown.");
	perror ("stat");
	exit (1);
      }

    if (stat (".", &cwd_statbuf) != 0) 
      {
	(void) fprintf (stderr, 
	    "Unable to stat current working directory-- exiting shutdown.\n");
	perror ("stat");
	exit (1);
      }

    /*
     *  If the devices are the same then we are already on the root volume.
     *  In this case change directories to / so that the killall script
     *  will work.  If we aren't on the root volume terminate with an error.
     *
     *  If we can't cd to /, killall will fail, but we may not run it anyway.
     *  (It only gets run if we're going to single user state or have
     *  mirrored discs.)  So go ahead and take this chance.
     */
    if (slash_statbuf.st_dev == cwd_statbuf.st_dev) 
      {
        if (chdir ("/") != 0)
          {
            perror ("chdir");
            (void) fprintf (stderr, 
    "Cannot change directories to the root directory-- continuing shutdown.\n");
          }
      }

    else
      {
         (void) fprintf (stderr, 
   "Shutdown cannot be run from a mounted file system-- exiting shutdown.\n");
         (void) fprintf (stderr, 
    "Change directories to the root volume (\"/\" will work) and try again.\n");
         exit (1);
      }

    /* 
     * sync all file system buffers 
     */
    sync ();

    /*
     *  Set the real and effective user id's to root.  Remember the old
     *  real id in saveuserid.  The downside of this is that the manpage
     *  for getuid says there is no way to get the saved user id (!).
     *
     *  An error should not occur since saveId should be valid and we are
     *  running with euid 0.  However, if an error does occur then we could
     *  have trouble later since access(2) is used to see if we can execute
     *  the scripts in /etc/shutdown.d and it uses the real user id.  (In
     *  addition, access(2) is used to figure out if we can stop accounting
     *  and auditing if they are running.)
     */
     saveId = (int) getuid ();
     if ((setresuid (0, 0, saveId)) != 0)
       {
	 perror ("setresuid");
         (void) fprintf (stderr,
		  "Error changing real id to root-- continuing shutdown.\n");
       }

    /*
     * Get the time and print it with the shutdown message
     */
    (void) time (&time_buf);
    (void) strftime (time_str_buf, TIMEBUFSIZE, "%x %X %Z", 
		     localtime (&time_buf));
    (void) printf ("\nSHUTDOWN PROGRAM\n%s\n", time_str_buf);

    /*
     * Check for number of users --
     *  If we have AT LEAST 2 users, we have to send out warning messages.
     */
    
    if ((utmp_ptr = getutent ()) == NULL)
      {
	(void) fprintf (stderr, 
	   "Warning: Unable to determine number of local users.\n");
	(void) fprintf (stderr, 
		"Shutdown will not to warn local users to logout.\n");
	perror ("/etc/utmp");

	/* 
	 *  Continue assuming no local users.  We should do this because the
	 *  wall will fail anyway if the utmp file can't be opened.
	 */
	num_users = 0;	
      }

    else 
      {
	do
	  {
	    if (utmp_ptr -> ut_type == USER_PROCESS) 
	      {
		num_users ++;
		if (num_users > 1)

		    /* 
		     *  only care if there's > 1 
		     */
		    break;		
	      }
	  } while ((utmp_ptr = getutent ()) != NULL);

	/* 
	 *  close file 
	 */
	endutent ();	
      }

    if (debugLevel)
      (void) fprintf (debugStream, "%s: num_users: %d\n", hostname, num_users);

#ifdef DISKLESS

    /*
     * Are we the root server or a swap server in a cluster?
     *
     *  Note:  A cnode can only swap to one machine, itself or another
     *         swap server (which may be the root server).  If a cnode
     *         is a swap server, it must swap to itself.
     */
    if ((mysite) && ((cct_ent = getcccid (mysite)) != NULL))
      {
	if (cct_ent -> cnode_type == 'r') 
	    rootserver = TRUE;

	else if (cct_ent -> swap_serving_cnode == mysite) 
	    swapserver = TRUE;
      }

#ifdef AUDIT
     /*
      *  Are we the an audit server?
      *
      *  One or two clients can be an audit server
      *  A client audit server is a cnode which has a locally mounted file 
      *  system with the "current" OR "next" audit log file on it.
      *
      *  The rootserver is an audit server only if both the "current"
      *  AND the "next" audit log files are on partitions mounted on it;
      *  assuming of course that there is a "next" file.
      *
      *  Although the "next" audit file isn't currently in use
      *  it is included in the formula because it could be put in use
      *  if the auditing system should switch audit files while in
      *  the course running shutdown.
      */
     if (audit_info(&audit_on, caf, &caf_cnode, naf, &naf_cnode)) {
        bad_audit_info  = TRUE;
	audit_off_early = TRUE;
     } else {
     	if (audit_on) {
        	if (mysite == caf_cnode || mysite == naf_cnode )
	        	auditserver = TRUE;
        	if (rootserver && naf_cnode && (naf_cnode != caf_cnode))
			auditserver = FALSE;
        } else {
		auditserver = FALSE;
		audit_off_early = FALSE;
     	}
     }

     if (debugLevel) {
        (void) fprintf (debugStream, "%s: audit_on: %d caf: %s caf_cnode: %d naf: %s naf_cnode: %d \n", hostname, audit_on, caf, caf_cnode, naf, naf_cnode);
        (void) fprintf (debugStream, "%s: auditserver: %d audit_off_early: %d \n", hostname, auditserver, audit_off_early);
     }

#endif /* AUDIT */
#endif /* DISKLESS */

    if (debugLevel)
      (void) fprintf (debugStream, "%s: rootserver: %d, swapserver: %d\n", 
	       hostname, rootserver, swapserver);

#ifdef DISKLESS

#ifdef AUDIT

    /*
     * If we're shutting down a client and it is the audit server
     * or if the rootserver isn't the system with the audit files:
     * first issue a warning to the user about the consequences of
     * the situation and give them a chance to abort (if we aren't batch)
     */ 
    if (audit_on && !rootserver && auditserver && !bad_audit_info)
      {
	audit_off_early=1;
	(void) fprintf (stderr,"\nSECURITY WARNING: A filesystem locally mounted on this system \ncontains the 'current' or 'next' audit log file. \nThe auditing system will be disabled prior to rebooting this system.\nAUDITING FOR THE ENTIRE CLUSTER WILL BE TURNED OFF.\n\n");
      }

    if (audit_on && rootserver && !auditserver && !bad_audit_info)
      {
	audit_off_early=1;
	(void) fprintf (stderr, "\nSECURITY WARNING: A filesystem locally mounted on one of the clients \nof this system contains the 'current' or 'next' audit log file.  \nThe auditing system will be disabled prior to rebooting this system.\n\n");
       }

    if (audit_on && bad_audit_info)
       {
	(void) fprintf (stderr, "\nSECURITY WARNING: Unable to determine the status of one \nor both audit log files.  The auditing system will be disabled \nprior to rebooting this system.\n\n");
       }

#endif /* AUDIT */

    /*
     * If we're shutting down either the root server or a swap server,
     * first issue warning to user that they're causing clients to reboot,
     * then issue a reboot warning to all users on all client cnodes.
     */
    if (rootserver || swapserver)
      {
	/*
	 * populate cnode_array []
	 */
	get_cnode_names ();

	/*
	 * find all active clients
	 */
	if (rootserver)
	    num_active_clients = cnodes (active_clients);	

	else 
	    num_active_clients = swapclients (active_clients);	

        if (debugLevel)
          (void) fprintf (debugStream, "%s: num_active_clients: %d\n", 
	           hostname, num_active_clients);
    
	/* 
	 * issue warning if there are any active other than ourselves
	 */
	if (num_active_clients > 1) 
	  {
	    /*
	     * Make sure you can get a socket for rcmd
	     */
	    if (sock_port = getservbyname ("shell", "tcp")) 
	      {
	       /* 
		* Got a legit port, go ahead and issue general warning
		*/
	 (void) printf ("The following client cnodes will also be rebooted:\n");
		print_active_clients (stdout);

                /*
                 *  If we are a root server or swap server and we have clients
		 *  we need to warn and we know we'll be able to warn them, 
		 *  then we need extra time for remote rcmd's to work. 
		 *  Previously, the program just always waited another 15
		 *  seconds if the grace_period was specified.  Here we are
                 *  re-setting it only in cases where it is be needed.
                 */
                if (grace_period < 15)
                  grace_period = 15;
	      }

	    else
	      {
	        /* 
	         * Got a NULL port pointer!!!  Issue "can't reboot" 
	         * warning and set num_active_clients to 0 to prevent
	         * rcmd attempt!
	         */
	        num_active_clients = 0;
	         
	        (void) fprintf (stderr, 
	    "Networking error: Unable to warn/reboot the following clients:\n");
	        print_active_clients (stderr);
	        (void) fprintf (stderr, "--continuing shutdown.\n");
              }
	  }

	/*
	 * This continue message will suffice for both cases above.
	 */
	if (! batchFlag)
	  {
	    do 
	      {
		(void) printf ("Do you want to continue? ");
		(void) printf ("(You must respond with 'y' or 'n'.):   ");
	        (void) fflush (stdin);
		yesNoAns = tolower (getchar ());
	      }   while ((yesNoAns != 'y')&&(yesNoAns != 'n'));
	    
	    if (yesNoAns == 'n')
	      exit (0);
	  }
      }	/* end of if swapserver || rootserver */

#endif /* DISKLESS */

    /* 
     * Send out a wall message if we're a server or if
     * there's more than one user
     */
    if (((rootserver || swapserver) && num_active_clients > 1) || num_users > 1)
      {
	/*
	 * Offer to let user send own shutdown message. 
	 */
        if (! batchFlag)
  	  {
	    do
	      {
	        (void) printf ("Do you want to send your own message? ");
	        (void) printf ("(You must respond with 'y' or 'n'.): ");
	        (void) fflush (stdin);
		yesNoAns = tolower (getchar ());
	      }  while ((yesNoAns != 'y') && (yesNoAns != 'n'));

	    if (yesNoAns == 'y')
	      {
	        /*
	         * Prompt for user's message.
		 *  
		 *  First, figure out the EOF character for this setup.
		 *  This will be used in the message.
		 *
		 *  To do this, use ioctl to return information about stdin.
		 *  The VEOF element of the c_cc array in the structure is
		 *  the EOF character.  
		 *
		 *  If this character < 040, then it will be OR'ed with 0100 
		 *  (octal) to get a printable character.  (For example, the 
		 *  default is ^D a 4.  ORing with 0100 gives 0104, which is a
		 *  capital D.)
		 *
		 *  If the ioctl doesn't work, assume the character is a ^D. 
	         */
		 if ((ioctl (0, TCGETA, &ttyStruct)) != -1)
		   eofChar = ttyStruct.c_cc [VEOF];
                 
		 else
		   eofChar = '\004';

	        (void) printf ("Type your message.\n");
                (void) printf (
			    "End with a new-line and EOF character (%s[%c]).\n",
			    ((eofChar < 040) ? "[CTRL]-" : ""),
                            ((eofChar < 040) ? (eofChar | 0100) : eofChar));

	        /*
	         * Read the data from stdin into input_buffer.
	         *
		 * The fread will terminate when no more characters 
		 * will fit, or if an EOF is seen and no characters are read in.
		 * (Note that the latter condition forces the EOF to be on a 
		 * new line.  It appears that 2 EOFs in a row do it as well.
		 * This is because of the way fread is implemented; a while
		 * loop of reads, each of which terminates on an EOF so it
		 * takes no input on the read and an EOF to terminate it.)
	         * 
	         * When the user types EOF, fread terminates.  A NULL is then
		 * added to the end of the string.
	         */
  
	        read_size = fread ((void *) input_buffer, sizeof(char), 
				   (MAXINPUTSTRING - (strlen (graceString))), 
				   stdin);
		input_buffer [read_size] = '\0';
	      }

	    /*
	     * Use the default message which prints the grace period
	     */
	    else
	      (void) sprintf (input_buffer, "%s", DEFAULT_WALL_MESSAGE);
	  }

        /*  
         *  When in batch mode, use the default wall message.
         */
        else
	  (void) sprintf (input_buffer, "%s", DEFAULT_WALL_MESSAGE);
			
	/*
	 *  In all cases, put the information regarding the grace time
	 *  into input_bffer.
	 */
	(void) strcat (input_buffer, graceString);

	/*  The above code has filled in input_buffer with either the user's
	 *  message or the default message plus information regarding the
	 *  grace time.  This section builds a command
	 *  string which will be executed remotely.  The full command looks
	 *  like this:
	 *
	 *           input_buffer 
	 *          |vvvvvvvvvvvv|
	 *          |            |
         *    echo '<user message>' | /etc/wall 2>&- >&- &
         *    |    |              |            |
         *    |^6^^|              |^^^^13^^^^^^|^^^^^^^^^^
         *   START_CMD                END_CMD    CMD_MISC 
         *
	 *  Build it into remoteWallCmd.  If space can't be allocated for it,
	 *  build the command into input_buffer, using the default wall
	 *  command and grace period strings.  Note that in this case, 
	 *  remoteWallCmd is set to point to input_buffer.  This is used later 
	 *  when sending the wall locally.  
	 * 
	 *  The problem with the local wall is that all we want is the actual
	 *  message, not the whole command used in the remote case.  If 
	 *  memory could be allocated for the message, then the real message to
	 *  send to the local wall is in input_buffer.  But if a malloc error
	 *  occurred, then input_buffer has the whole command in it.  In this
	 *  case, input_buffer will be overwritten with the default wall 
	 *  message and grace period strings.
	 */
	remoteWallCmd = malloc (strlen (START_CMD) + strlen (input_buffer) 
			        + strlen (END_CMD) + strlen (CMD_MISC + 1));

	if (remoteWallCmd)
	  (void) sprintf (remoteWallCmd, "%s%s%s%s", START_CMD, input_buffer, 
			  END_CMD, CMD_MISC);

	else
	  {
	    (void) fprintf (stderr, 
		  "Error getting memory for message; using default message.\n");
	    (void) sprintf (input_buffer, "%s%s%s%s%s", START_CMD, 
			    DEFAULT_WALL_MESSAGE, graceString, END_CMD, 
			    CMD_MISC);
	    remoteWallCmd = input_buffer;
	  }

        if (debugLevel)
            (void) fprintf (debugStream, "%s: Wall command: %s\n", hostname, 
			    remoteWallCmd);
    
#ifdef DISKLESS
	/*
	 * If we're rootserver, we have to broadcast to the entire cluster
	 * that we're going to shutdown.  If we're a swap server we only
	 * have to broadcast to our clients.
	 */
	if ((rootserver || swapserver) && sock_port)
	  {
	    (void) printf("Warning clients of upcoming reboot/shutdown.\n");

	    /*
	     * for each client, tell them we're about to shutdown
	     */
	    for (index = 0; index < num_active_clients; index++)
	      {
	        /* 
	         * Don't attempt rcmd(wall) in the following cases:
	         *	- this is our site (don't reboot ourselves)
	         *	- cnode_array.names is NULL for this site
	         *	  (due to malloc or getccent error)
	         */
	        if (active_clients [index] == mysite ||
	            cnode_array [active_clients [index]].name == NULL)
                  {
                    if (debugLevel)
                      (void) fprintf (debugStream, 
				     "%s: NOT sending wall to %s (%d index)\n", 
		                     hostname, 
				     cnode_array [active_clients [index]].name,
				     index);
	            continue;	
                  }

	        /* 
	         * Execute wall on the client cnode 
	         */
                if (debugLevel)
                  (void) fprintf (debugStream, "%s: Sending wall to %s\n", 
		         hostname, cnode_array [active_clients [index]].name);
    
	        ahost = cnode_array [active_clients [index]].name; 
	        rcmd_socket = rcmd (&ahost, sock_port -> s_port, ROOT, ROOT, 
						      remoteWallCmd, (int *) 0);
                if (debugLevel)
		  (void) fprintf (debugStream, "ahost: %s, rcmd_socket:%d\n",
				  ahost, rcmd_socket);

	        if (rcmd_socket == -1) 
	          (void) fprintf (stderr,
	                     "No reboot warning issued on cnode %s...\n", 
		             cnode_array [active_clients [index]].name);

	        else
		  {
		    /*
		     *  Print out the standard output and error from the
		     *  remote command.  The command merged standard output
		     *  and error on the remote before it got back to this
		     *  machine.  The return value from rcmd is a file
		     *  descriptor which can be opened to get this output.
		     */
		    if (debugLevel > 1)
		      {
		        (void) fprintf (debugStream, 
				"\nSTDOUT/ERR FROM rcmd(%s) ON %s:\n", 
				remoteWallCmd, 
				cnode_array [active_clients [index]].name);

		        (void) fprintf (debugStream, 
			       "=============================================");
		        (void) fprintf (debugStream,
					"============================\n");

			/*
			 *  Set an alarm to interrupt the getc if it seems
			 *  to hang.  What can occur is that the socket
			 *  will still be around, but the process on the
			 *  other end is gone.  So the read (getc) will hang
			 *  waiting for input.  This is eventually stopped
			 *  by the networking system, which uses a "keep alive"
			 *  scheme to send ACKs to the other system every so
			 *  often.  If no answer is received, it trys again
			 *  after a shorter time.  This apparently keeps up
			 *  for 2 hours (!) - used to be 45 seconds - and then
			 *  the socket is closed and the read would fail.
			 *
			 *  Note that this will mess up any other SIGALRM
			 *  signal handlers for the duration of the program.
			 *  But since it is only used in debugging mode 
			 *  (and a higher level) that's probably OK.
			 */

		        rcmdOutErrStream = fdopen (rcmd_socket, "r");

			debugAction.sa_handler = debugHandler;
			(void) sigemptyset (&debugAction.sa_mask);
			debugAction.sa_flags = 0;
			(void) sigaction (SIGALRM, &debugAction, 
							(struct sigaction *)0);

			(void) alarm (10);
			quitRead = FALSE;

		        while (((rcmdOutErr = getc (rcmdOutErrStream)) != EOF)
				&& (quitRead == FALSE))
		           (void) fputc (rcmdOutErr, debugStream);

		        (void) fprintf (debugStream, 
			       "\n===========================================");
		        (void) fprintf (debugStream,
					"============================\n");
		      }

	            (void) close (rcmd_socket); 
		  }

	      }	/* end of for each active client */
	  } /* end of if rootserver or swapserver */

#endif /* DISKLESS */

	/*
	 * Now do the wall on our local system, provided num_users >1.
	 */
	if (num_users > 1)
	  {
	    if ((writep = popen ("/etc/wall","w")) == NULL)
	      {
                (void) fprintf (stderr, 
   "Unable to warn local users of upcoming shut down-- continuing shutdown.\n");
	        perror ("popen");
	      }

	    else 
	      {
	        /*  If we had an error malloc'ing space for the remote wall 
	         *  command, then message was just set input_buffer.  Otherwise
	         *  message has a different address.  In the latter case, 
	         *  input_buffer has the real message without all the extra 
	         *  command stuff.  In the former case, the default message is
	         *  the one we want.  Put this into input_buffer along with the
	         *  grace period string before printing it out.
	         */
	        if (remoteWallCmd == input_buffer)
	          (void) sprintf (input_buffer, "%s%s", DEFAULT_WALL_MESSAGE,
			          graceString);

	        (void) fprintf (writep, "%s\n", input_buffer);
	        (void) pclose (writep);
	      }
	  } /* end of the local wall if num_users > 1 */
      } /* end of if rootserver or swapserver or >1 users */

    /*
     * Now sleep for the designated grace period after warning the user that
     * we're pausing.
     */
     if (grace_period > 0)
       {
         (void) printf (
		  "Waiting a grace period of %d seconds for users to logout.\n",
		  grace_period);
         (void) printf (
		"Do not turn off the power or press reset during this time.\n");
         (void) sleep (grace_period);
       }

     /*
      * Send the final wall message - ignore if wall() fails here
      */
     if ((writep = popen ("/etc/wall", "w")) != 0)
       {
	 (void) fprintf (writep, "SYSTEM BEING BROUGHT DOWN NOW ! ! !\n");
	 (void) pclose (writep);
       }

     /*
      * If the grace_period is > 0 and we're in interactive mode, then 
      * verify before proceeding.
      */
     if ((grace_period > 0) && (! batchFlag))
       {
	 do
	   {
	     (void) printf ("Do you want to continue? ");
	     (void) printf ("(You must respond with 'y' or 'n'.):   ");
	     (void) fflush (stdin);
	     yesNoAns = tolower (getchar ());
	   } while ((yesNoAns != 'y') && (yesNoAns != 'n'));
    
	 if (yesNoAns == 'n') 
	   exit (0);
       }

    /*
     *  Turn off signals for SIGHUP, SIGINT, and SIGTERM here.  The idea
     *  is that from here on, shutdown is un-interruptable since the system
     *  is in the process of going down and is at least semi-hosed at any
     *  point beyond here.
     *  
     *  - IGNORE the signal
     *  - Set sig mask to be empty
     *  - No flags			
     *
     *  For SIGHUP, SIGINT, and SIGTERM
     */
    action.sa_handler = SIG_IGN;	
    (void) sigemptyset (&action.sa_mask);
    action.sa_flags = 0;		
    (void) sigaction (SIGHUP, &action, (struct sigaction *) 0);	
    (void) sigaction (SIGINT, &action, (struct sigaction *) 0);	
    (void) sigaction (SIGTERM, &action, (struct sigaction *) 0);	
      
    /*  Run user-supplied shutdown scripts located in /etc/shutdown.d
     *  These scripts will be run in machine (ASCII)-collated order.  This 
     *  mimics the System V algorithm except that there, shutdown apparently 
     *  goes to state 0, then init executes something called /etc/rc0.  This in
     *  turn executes files in /etc/shutdown.d as well as all files which start 
     *  with a 'K' in /etc/rc0.d.  Scripts that start with a dot are skipped
     *  but subdirectories are attempted to be executed.  The HP-UX version
     *  skips subdirectories as well as files that start with a dot.
     */
     runCustomScripts ();

    /*
     * Shutdown accounting.  First check to see that the accounting script
     * exists; if access(2) returns 0 then access is allowed.
     */
    if ((access ("/usr/lib/acct/shutacct", X_OK)) == 0)
      {
	(void) system ("/usr/lib/acct/shutacct");
	(void) printf ("Accounting stopped.\n");
      }


#if defined (DISKLESS) &&  defined (AUDIT)
    /*
     * Shutdown C2 audit subsystem here (early):
     *
     * If system is a rootserver AND 
     * the rootserver isn't the auditserver
     * (the currently active audit log file is on a client LMFS)
     * OR
     * the system is a client AND the client is the auditserver
     * (it has an LMFS containing the currently active audit log file)
     *
     * The Auditing system should be shutdown prior to unmounting the
     * filesystem containing the active audit file.  Otherwise the Auditing
     * system may switch to the "next" audit file or audit records will be
     * thrown away.
     *
     */
    
    if (audit_off_early)
      {
	    if ((access ("/usr/bin/audsys", X_OK)) == 0)
	      {
		(void) printf ("Audit subsystem being turned off.\n");

		/*
		 * Use pstat to nail down the appropriate entry
		 * for audodomon, and kill it if active -- don't complain
		 * if we can't kill it.
		 */
		(void) kill_process ("audomon");

		/*
		 * Turn off auditing; use the command since it does lots
		 * of other stuff besides calling the audctl intrinsic.
		 *
		 */
		(void) system ("/usr/bin/audsys -f");
	      }
      }
#endif /* DISKLESS and AUDIT */
    /*
     *  The program now splits execution paths.  For (hopefully) the majority
     *  of the halting and rebooting cases we can just go ahead and exec
     *  /etc/reboot to finish bringing down the system.  However, if we are
     *  running on a mirrored disc system or a system that has a local mounted
     *  file system then we need to first unmount all the discs, before 
     *  exec'ing reboot.
     *
     *  In addition, if we are going to single-user state, then we need to
     *  kill all processes as well as unmount all the discs before making the
     *  transition to state S.
     *
     *  So there are 3 checks at this point:
     *
     *    1.  Are we running on a swapserver with a local mounted file system?
     *    2.  Are we running on a mirrored disc system?
     *    3.  Are we rebooting or halting?
     *
     *  If the answers are NO, NO, and YES, then we can just exec reboot.
     *
     *  For any other answers, we go down the more complicated path where 
     *  rbootd is killed, all clients are rebooted, all processes are killed,
     *  and all local HFS discs are unmounted.  Then, if we were really 
     *  trying to reboot or halt we exec reboot.  If we were really trying to
     *  go to single-user state we send a signal to init to make the 
     *  transition.
     */
    if (rootserver || standalone)
      num_lmfs = 0;

    else
      num_lmfs = num_local_mounts ();

    /*
     *  We need to see if we are running on a mirrored disc system.  To do
     *  this, check to see if there is an /etc/mirrortab file that has a size
     *  greater than 0.  If so, set mirroredDiscs to TRUE.  It was initialized
     *  to FALSE.
     */
    if ((stat ("/etc/mirrortab", &mirrorStat)) == 0)
      {
	if (mirrorStat.st_size > 0)
	  mirroredDiscs = TRUE;
      }

    if ((reboot || halt) && (! mirroredDiscs) && (num_lmfs <= 0))
      {
	/*
	 *  The argv array was built as we parsed options, so all we have
	 *  to do is an execv.
	 */

        (void) printf ("Executing \"%s %s %s %s %s %s %s %s %s\".\n", 
		       rebootArgv [0], rebootArgv [1], rebootArgv [2],
		       rebootArgv [3], rebootArgv [4], rebootArgv [5],
		       rebootArgv [6], rebootArgv [7], rebootArgv [8]);

        (void) execv (REBOOT_CMD, rebootArgv);

	/*
	 * check for exec failure
	 */

	perror ("Could not execute /etc/reboot-- exiting shutdown");
	exit (1);
      }

    /* 
     * If we got here, then we're shutting down to single user state,
     * or we're a disk mirroring system or we have an LMFS, and we'll reboot
     * after umount.
     */

#ifdef DISKLESS
    /*
     * If I'm a root or swap server reboot all clients because even just
     * going to init s kills all the csps and that makes the server useless
     *
     *  Note that this works since clients reboot from their swap server
     *  (this can be the root server but it doesn't have to be).  Since the
     *  swap server kills rbootd, the clients stay ready to reboot until
     *  rbootd is running again on the swap server.
     *
     * Note that if we were going to reboot (mirrored disc case) then reboot
     * would do all of this stuff.  However, we have to unmount all the discs
     * first in the mirrored situation.  But before we can do all that the
     * system needs to be quiescent.  So we have to reboot all the clients...
     * Repition of code, but it's necessary.
     */
    if (rootserver || swapserver)
      {
        if (debugLevel)
          (void) fprintf (debugStream, 
	        "%s: Going to state S, mirrored, or LMFS: killing rbootd.\n",
	        hostname);
    
	/* 
	 *  Kill the cluster boot server before rebooting clients 
	 */
	if (kill_process ("rbootd") != 0)
	  {
	    (void) fprintf (stderr, 
                "WARNING:  Unable to kill remote boot daemon (/etc/rbootd).\n");
	    (void) fprintf (stderr, 
     "--continuing shutdown.  You may have to manually reboot some clients.\n");
	  }

	/*
	 *  Put all of the rcmd stuff inside an "if we got a socket when we
	 *  were trying to warn the clients that we were shutting down".  If
	 *  we didn't get a socket that time, the rcmd will fail anyway, so
	 *  we can just skip this stuff.
	 */
        
	if (sock_port)
	  {

	    /*
	     *  Construct reboot command; -f and -d do not get passed on to 
	     *  reboot for clients.  Neither does -h since we always want
	     *  to just reboot them.  Note that the /etc/reboot command does
	     *  the same thing.  That is, it does not pass the -h option on
	     *  to client cnodes.
	     */
	    (void) sprintf (input_buffer, "%s %s %s %s", REBOOT_CMD,
                           (debugOpt ? debugOpt : ""),
			   (debugOpt ? "" : STDOUT_TO_NULL),
			   CMD_MISC);

            if (debugLevel)
              (void) fprintf (debugStream, "%s: Client reboot command: %s\n",
	               hostname, input_buffer);
    
	    /*
	     * Now reboot the client cnodes and wait until they have 
	     * either rebooted or we've given up.  
	     */
	    for (index = 0; index < num_active_clients; index++) 
	      {
	        /* 
	         * Don't reboot in the following cases:
	         *	- this is our site (don't reboot ourselves)
	         *	- cnode_array.names is NULL for this site
	         *	  (due to malloc or getccetn error)
	         *	- we're not supposed to reboot this site because
	         *	  we're the root server and this cnode swaps to
	         *	  an auxiliary swap server (donot_reboot == TRUE)
	         */
	        if (active_clients [index] == mysite 
		   || cnode_array [active_clients [index]].name == NULL 
		   || cnode_array [active_clients [index]].donot_reboot == TRUE)
	          {
                    if (debugLevel)
                      (void) fprintf (debugStream, "%s: NOT rebooting %s\n",
	                   hostname, cnode_array [active_clients [index]].name);
        
		    continue;	
	          }
	        /* 
	         * Execute reboot on the client cnode passing 
	         *   along all applicable options
	         */
    
                 if (debugLevel)
                   (void) fprintf (debugStream, "%s: Rebooting %s\n",
	                   hostname, cnode_array [active_clients [index]].name);
        
	        ahost = cnode_array [active_clients [index]].name; 
	        rcmd_socket = rcmd (&ahost, sock_port -> s_port, ROOT, ROOT, 
						       input_buffer, (int *) 0);
                if (debugLevel)
	          (void) fprintf (debugStream, "ahost: %s, rcmd_socket:%d\n",
			          ahost, rcmd_socket);
    
	        if (rcmd_socket == -1) 
	          (void) fprintf (stderr, 
	      "No reboot command executed on cnode %s-- continuing shutdown.\n",
		                  cnode_array [active_clients [index]].name);
    
	        else 
	          {
		    /*
		     *  Print out the standard output and error from the
		     *  remote command.  The command merged standard output
		     *  and error on the remote before it got back to this
		     *  machine.  The return value from rcmd is a file
		     *  descriptor which can be opened to get this output.
		     */
		    if (debugLevel > 1)
		      {
		        (void) fprintf (debugStream, 
				    "\nSTDOUT/ERR FROM rcmd(%s) ON %s:\n", 
				    input_buffer, 
				    cnode_array [active_clients [index]].name);
    
		        (void) fprintf (debugStream, 
			       "=============================================");
		        (void) fprintf (debugStream,
				          "============================\n");
    
		        /*
		         *  Set an alarm to interrupt the getc if it seems
		         *  to hang.  What can occur is that the socket
		         *  will still be around, but the process on the
		         *  other end is gone.  So the read (getc) will hang
		         *  waiting for input.  This is eventually stopped
		         *  by the networking system, which uses a "keep alive"
		         *  scheme to send ACKs to the other system every so
		         *  often.  If no answer is received, it trys again
		         *  after a shorter time.  This apparently keeps up
		         *  for 2 hours (!) - used to be 45 seconds - and then
		         *  the socket is closed and the read would fail.
		         *
		         *  Note that this will mess up any other SIGALRM
		         *  signal handlers for the duration of the program.
		         *  But since it is only used in debugging mode 
		         *  (and a higher level) that's probably OK.
		         */
    
		        rcmdOutErrStream = fdopen (rcmd_socket, "r");
    
		        (void) alarm (10);
		        quitRead = FALSE;
    
		        while (((rcmdOutErr = getc (rcmdOutErrStream)) != EOF) 
				    && (quitRead == FALSE))
		          (void) fputc (rcmdOutErr, debugStream);
    
		        (void) fprintf (debugStream, 
			       "\n===========================================");
		        (void) fprintf (debugStream,
				          "============================\n");
		      }
    
	           (void) close (rcmd_socket); 	
	          }
	      }	/* end of for each active client */
    	
	    /*
	     * Wait for client cnodes to reboot 
	     */
	    if (debugLevel)
	      (void) fprintf (debugStream, "Calling clients_rebooted\n");
    
	    if (! clients_rebooted ())
	      {
		/*
		 *  Not all clients were rebooted.  This could be a real
		 *  problem, but the system is so messed up anyway that 
		 *  letting the user stop here is probably a bad idea.  So
		 *  instead of using a "do you want to continue" message,
		 *  we just warn them and go on.  Note that the 
		 *  clients_rebooted routine prints the names of the clients
		 *  it is waiting for.  It also waits a long maximum time,
		 *  so if one system wouln't go down the operator should have
		 *  plenty of time to go get the problem fixed.  (The routine
		 *  only waits until all the clients have gone down; not the
		 *  full time that it can wait.)
		 */

	        (void) fprintf (stderr,
           "The following clients could not be rebooted-- continuing shutdown");
	        print_active_clients (stderr);
	      }
	  } /* End of if we had a socket. */
      } /*  End of if rootserver or swapserver */


    /*
     * We need to warn all users of our local mounted file systems
     * that they are going away.  We must warn them before the call
     * to csp, or else we don't have access to the pids on the other
     * cnodes.
     *
     * We only warn (wall) the cnodes with users who have opens
     * on our lmfs.  If that fails, then cwall the entire cluster.
     * 
     */


     if ( (num_local_mounts() > 0) && (! (rootserver || standalone)))
       {
           (void) printf ("All processes accessing the local mounted file systems will now be killed.\n");

           if (warn_users_of_lmfs (sock_port, hostname) < 0) 
           {

    	     /*
     	      *  The resulting cwall command should be:
     	      *
     	      *  echo 'System <hostname> is shutting down.\n
     	      *     Its locally mounted file systems are no longer available.\n'
     	      *     | /etc/wall 2>&1
     	      *
     	      *  By not doing this in the background, the cwall should get 
     	      *  going before the program continues and reboots/halts.  But
     	      *  we'll wait a little anyway.
     	      */

	      (void) fprintf (stderr, CLUSTER_WARNING);

     	      (void) sprintf (cwallCommand, "%s%s%s%s", START_WARNING, 
 		    hostname, END_WARNING, PIPE_TO_CWALL);
    	      (void) system (cwallCommand);
 
    	      if (debugLevel)
      	          (void) fprintf (debugStream, "%s: cwall command: %s\n",
		      hostname, cwallCommand);
	    
	      (void) sleep (5);
	   }
       }


#endif /* DISKLESS */

    /*
     * Kill all currently running processes
     */
    (void) printf ("All currently running processes will now be killed.\n");

    /*
     *  Execute the killall script.  Note that auditing (B1 and C2) must NOT
     *  be killed by this script!
     *
     *  The 7.0 version of shutdown NULL'd out the domainname to prevent ps 
     *  from looking for YP and hanging.  This was added to the killall
     *  script for 8.0 and so is removed from shutdown.
     */
    if (debugLevel > 1)
      (void) system ("/etc/killall >>/shutdownDLog 2>&1");
      
    else
      (void) system ("/etc/killall");

    /*
     * If we get here we are headed for single user state.
     *
     * Remove /etc/rcflag file so that rc will know to restart daemons
     * on return to init 2 -- ignore the error
     */
    (void) unlink ("/etc/rcflag");

#if defined (TRUX) || defined (AUDIT)
    /*
     * Shutdown audit subsystem 
     *   (if standalone or if rootserver)
     *   ...and if we haven't already shutdown auditing
     */
    if (!audit_off_early && (rootserver || standalone))
      {

#ifdef TRUX
	/*
	 * Check for SecureWare auditing; note that this command just
	 * switches auditing to the root volume.  It still needs to be 
	 * shut down later.
	 */
	if ((access ("/tcb/bin/auditcmd", X_OK)) == 0)
	  {
	    (void) printf ("Audit subsystem being turned off.\n");
	    (void) system ("/tcb/bin/auditcmd -q -s");
	  }

	else
#endif /* TRUX */
	  {
#ifdef AUDIT
	    if ((access ("/usr/bin/audsys", X_OK)) == 0)
	      {
		(void) printf ("Audit subsystem being turned off.\n");

		/*
		 * Use pstat to nail down the appropriate entry
		 * for audodomon, and kill it if active -- don't complain
		 * if we can't kill it.
		 */
		(void) kill_process ("audomon");

		/*
		 * Turn off auditing; use the command since it does lots
		 * of other stuff besides calling the audctl intrinsic.
		 *
		 */
		(void) system ("/usr/bin/audsys -f");
	      }
#endif /* AUDIT */
	  }
      }
#endif /* TRUX AND/OR AUDIT */

    /*
     *  Unmount file systems 
     *    If we're the root server or standalone,
     *	  then umount ALL mount types (including nfs).  If we're just
     *    a client cnode, then just unmount local mounts (hfs and cdfs).
     */
    if (rootserver || standalone)
      {
	(void) printf ("All file systems will now be unmounted.\n");
	(void) system ("/etc/umount -a");

	/*
	 * Execute getmountcnt syscall to see if we umounted them all
	 *	(Note that ALL non-root-server cluster mounts should
	 *	be long unmounted by this time since we rebooted them!!!)
	 * Subtract one for the root volume, then we can check
	 * below for all volumes unmounted and check against the same
	 * value (0) as we do for non-root cnodes.
	 *
	 *  Since this could take awhile for NFS mounts, loop 5 times to
	 *  give them time to go away.  If they do then break the loop.
	 *  Sleep a little while through each iteration to give it time to
	 *  do some work.
	 */
	for (umountCount = 0; umountCount <= 5; umountCount ++)
	  {
	    num_lmfs = getmount_cnt (&time_buf) - 1;
	    if (num_lmfs == 0)
	      break;

	    (void) sleep (1);
	  }
      }

#ifdef DISKLESS

    else
      {
	/*
	 * Check for local HFS file systems before making unmount announcement
	 * so that diskless users don't see any change here.  
	 * 
	 *    NOTE: Two possible algorithms may be used to count mounts.
	 *		See routines below...
	 */
	num_lmfs = num_local_mounts ();  

        if (debugLevel)
	  (void) fprintf (debugStream, 
			  "%s: num_lmfs: %d\n", hostname, num_lmfs);
	if (num_lmfs != 0)
	  {
	    (void) printf ("All local file systems will now be unmounted.\n");
	    (void) system ("/etc/umount -a -t hfs");
	  }

	/*
	 *  If there were local file system to unmount, see if we got them all.
	 */
	if (num_lmfs != 0)
	  num_lmfs = num_local_mounts ();

        if (debugLevel)
	  (void) fprintf (debugStream, 
			  "%s: num_lmfs: %d\n", hostname, num_lmfs);

      }

#endif DISKLESS

    /*  
     *  Note that num_lmfs is set in all cases by the if-then-else construct
     *  above.  If we are on the rootserver, then getmount_cnt was used, and
     *  the root disc was "subtracted" from the total; hopefully leaving 0.
     *  For swapservers, num_local_mounts was used, which might return -1
     *  if /etc/mnttab couldn't be opened.  So this check needs to be for
     *  the number of local file systems greater than 0.
     */
    if (num_lmfs > 0)
      {
        /*
         * There were extraneous mounts even after umount -- issue warning
         */
        (void) fprintf (stderr, 
			"WARNING:  SOME FILE SYSTEMS WERE NOT UNMOUNTED!\n");

        /*
         * Display remaining mounts --
         *   Do regular mount command for standalone and rootserver
         *   Do mount -l for local mounts only on aux disk servers
         */
        if (standalone || rootserver)
          (void) system ("/etc/mount -u");
     
#ifdef DISKLESS

        else
          (void) system ("/etc/mount -u -l");
	

#endif /* DISKLESS */

        /*
         * Use pstat to nail down the appropriate entry
         * for umount, and kill it if its still around
         *
         * If we fail trying to kill the umount, ignore the failure.
         */
        (void) kill_process ("umount");
     }

#ifdef DISKLESS

    /*
     * Check for clustered system.  If clustered, kill csps (cluster
     * server processes) with csp call.
     */
    if (cnodes((cnode_t *)0) > 0)
    { 

      if (debugLevel > 1)
          (void) fprintf (debugStream, "Clustered system.\n");

      if (csp (NSP_CMD_ABS, 0) == -1)
      {
	  (void) fprintf (stderr, 
       "Could not kill some cluster server processes-- continuing shutdown.\n");
	  perror ("csp");
      }
    }

#endif /* DISKLESS */

    /* 
     * Now that we've umounted all the file systems, it's ok to
     * execute reboot if we're on a disk mirroring system
     */
    if (reboot || halt) 
      {
	/*
	 *  The argv array was built as we parsed options, so all we have
	 *  to do is an execv.
	 */

        (void) printf ("Executing \"%s %s %s %s %s %s %s %s %s\".\n", 
		       rebootArgv [0], rebootArgv [1], rebootArgv [2],
		       rebootArgv [3], rebootArgv [4], rebootArgv [5],
		       rebootArgv [6], rebootArgv [7], rebootArgv [8]);

        (void) execv (REBOOT_CMD, rebootArgv);

	/*
	 * check for exec failure
	 */
	perror ("Could not execute /etc/reboot-- exiting shutdown");
	exit (1);
      }

    /*
     *  Set up signal handler to exit on SIG_HUP
     * 	(note: action struct initialized earlier when we ignored sighup)
     *
     *  Reset to default action (exit).  Do this before the signal is sent to
     *  init so that we are guaranteed to be ready to exit when init sends
     *  a SIGHUP.
     */
    action.sa_handler = SIG_DFL;	
    if ((sigaction (SIGHUP, &action, (struct sigaction *) 0)) != 0)
      (void) fprintf (stderr, 
       "Error resetting signal handler to default: %d-- continuing shutdown.\n",
	              errno);

    /*
     *  Change state to init s by sending init a SIGBUS
     */
    (void) kill ((pid_t)1, SIGBUS);

    /*
     *  Give init 45 seconds to send us SIGHUP.  When we get SIGHUP, exit 0.
     *  If after 45 seconds we still don't get SIGHUP, exit 0 anyway.  Since
     *  this usually takes about 20 seconds, print 20 in the message, but sleep
     *  for 45 just to be on the safe side.
     */
    (void) printf ("\n");
    (void) printf ("Wait for transition to run-level S (allow 20 seconds).\n");

    (void) sleep (45);

    /*
     *  If we get here, we didn't get SIGHUP indicating transition to init s,
     *  so we need to issue a warning that transition didn't occur.
     */
    (void) fprintf (stderr,
	"Warning:  Transition to init state s timed out-- exiting shutdown.\n");
    exit (1);
}					/* END OF MAIN */

/*
 * allowed () - This function takes as parameters a username and the name
 * 		of a file used as an allow database and returns true if
 *		the username is found in the allow file along with the 
 *		requested hostname. 
 *
 *		If the allow file cannot be opened, and if the user is root,
 *		then access is granted.  If the user cannot be found in the
 *		file, then if the file is of zero length and the user is
 *		root access is granted.  In all other cases, allowed() 
 *		returns FALSE and the shutdown program terminates.
 *
 *		Forcing root to be in the allow file enforces whatever
 *		security policy the administrator wants.  It will also be
 *		easy to convert to a least-privilege type of mechanism.
 *		By allowing root to execute this program if the allow file
 *		is missing or of zero length we still have an escape 
 *		mechanism, and a way to restrict the use of shutdown to root.
 */

struct sigvec sigsys_vec = {SIG_IGN, 0, 0};
/**********************************************************************/
int
allowed (username, hostname, allow_file)
char *username, *hostname, *allow_file;
/**********************************************************************/
{
	FILE *cap;
	struct stat allowStatBuff;
	int allowStatReturn;
	struct hostent *myHost;
        char myHostName [MAXHOSTNAMELEN + 1];
	int userOkForHost;
	int s = -1;

	if ((cap = fopen (allow_file, "r")) == NULL)
	  {
	    /* 
	     *  Can't open the allow file; if the user is root then return
	     *  true; otherwise return FALSE.
	     */
	    if (getuid () == 0) 
	      return TRUE;

	    else
	      return FALSE;
	  }

	/*
	 * DTS: DSDe408038, FSDlj09329 
	 * The orignal use of gethostbyname and SIGSYS handler created two side
	 * effects:
	 * 1) FSDlj09329:
	 *    If the SIGSYS handler is served (ie. system without LAN 
	 *    installed), the stdin is closed as an unknown side effect. 
	 *    Therefore the "Do you want to continue" prompt loops 
	 *    indefinitely. In 9.0, the fix was to re-copy back stdin after 
	 *    the signal handler is served, but this did not fix the second 
	 *    side effect.
	 * 2) DSDe408038:
	 *    If the SIGSYS handler is served, /bin/stty (used in killall) 
	 *    returns "not a tty", instead of /dev/console. This caused the 
	 *    killall script to kill the parent shell. As a result, the 
	 *    "timing out" message shows up.
	 * Because of these two defects, the network testing is done by 
	 * checking the the return value of the socket() system call, with 
	 * the SIGSYS signal ignored.
	 */

	/* ignore SIGSYS signal */
	(void) sigvector (SIGSYS, &sigsys_vec, 0);  

	/* test to see if the system has network installed */
	if ((s=socket(AF_UNIX, SOCK_STREAM, 0)) < 0) 
                NetworkInUse = FALSE;
        else {
                NetworkInUse = TRUE;
                if (s >= 0) close(s);
		/* gethostbyname() works only if networking is in the kernel */
                myHost = gethostbyname (hostname);
        }

        if (NetworkInUse == FALSE)
            strcpy (myHostName, hostname);
        else if (myHost == NULL)
          {
            (void) fprintf (stderr,
                       "Can't get official host name-- continuing shutdown.\n");          }
        else
            strcpy (myHostName, myHost -> h_name);

	userOkForHost = shutdownRuserok (cap, myHostName, username);

	(void) fclose (cap);

	if (userOkForHost)
	  return TRUE;


	/* 
	 *  Never found the username/hostname pair - stat the file to see if
	 *  it is of zero length.  If so, and if the user is root, then allow 
	 *  access.  If the stat fails, return FALSE.
	 */
	allowStatReturn = stat (allow_file, &allowStatBuff);

	if (allowStatReturn == 0)
	  {
	    if ((allowStatBuff.st_size == 0) && (getuid () == 0))
	      return TRUE;
	  }

	return FALSE;	
}					/* END OF ALLOWED */

/*
 * num_local_mounts () --
 *   The use getmount_cnt (2) and getmntent (3x) to determine the number
 *   of locally mounted file systems -- returns number of local HFS mounts.
 */
/**********************************************************************/
int
num_local_mounts ()
/**********************************************************************/
{ 
    time_t time_buf;		/* time buffer for getmount_cnt */
    int num_mounts;		/* mounted file system count	*/
    struct mntent *mnt;		/* getmntent entry structure	*/
    FILE *mnttab;		/* stream ptr to /etc/mnttab	*/


    /*
     * if num_mounts > 1, then there are other mounts in 
     * the cluster besides the roots mount, so we need
     * to see if any of them belong to us.
     */
    if ((num_mounts = getmount_cnt (&time_buf)) > 1)
      {
	/*
	 * Count how many mounts are local to this cnode
	 */
	num_mounts = 0;	
	if (! (mnttab = setmntent (MNT_MNTTAB, "r"))) 
	  {
	    (void) fprintf (stderr,
		   "WARNING: Cannot open /etc/mnttab-- continuing shutdown.\n");
	    perror ("setmntent");

	    /* 
	     *  The system is pretty messed up if we can't open /etc/mmttab.
	     *  
	     *  This routine is called in 2 places.  In the first, we're just
	     *  trying to decide if we need to go through the umount path or
	     *  if we can just exec reboot.  In that case, if we can't open
	     *  /etc/mnttab, we should probably just go ahead and reboot.
	     *  Syncer should eventually fix the mnttab problem.
	     *
	     *  The second case where this is called is when we're trying to
	     *  unmount file systems.  In this case, we should probably go
	     *  ahead and try to unmount file systems since this also should
	     *  fix the /etc/mnttab file.
	     *
	     *  By returning -1 here, the first use of the routine can choose
	     *  to continue and exec reboot just like it would if this routine
	     *  returned a 0 (no local HFS discs), and the second use can 
	     *  continue with the umount attempt, whereas if a 0 is returned 
	     *  (no local HFS discs) then it won't try the umount.
	     */
	    return -1; 		
	  } 

	else
	  {
	    while (mnt = getmntent (mnttab))
	      {
		if (mnt -> mnt_cnode == mysite) 
		    num_mounts ++;
		
		if (debugLevel)
		  (void) fprintf (debugStream, 
			 "%s: file system name: %s, path: %s, cnode: %hu\n",
			 hostname, mnt -> mnt_fsname, mnt -> mnt_dir,
			 mnt -> mnt_cnode);
	      }
	    
	    /*
	     *  Close the mount table file.
	     */
	    (void) endmntent (mnttab);
	  }
      } else
	  num_mounts = 0;

    return num_mounts;
}					/* END OF NUM_LOCAL_MOUNTS */


#if defined (DISKLESS) && defined (AUDIT)
/*
 *   audit_info
 *   routine that uses audctl(2) and stat(2) to determine
 *   whether auditing is on and the cnode where the audit log files
 *   are.
 */
/**********************************************************************/
int
audit_info (audit_on,c_file,c_node,x_file,x_node)
int *audit_on;
char *c_file,*x_file;
cnode_t *c_node,*x_node;
/**********************************************************************/
{ 
	int retval=0;
	extern int errno;
	struct stat stat_buf;

	*audit_on=1;

	if (audctl(AUD_GET,c_file,x_file,0))
	  switch (errno) { 
		case ENOENT:
			*x_file=0;
			break;
		case EALREADY:
			*audit_on=0;
			break;
		default:
			fprintf(stderr,"Unable to determine audit files: errno=%d\n",errno);
			*audit_on=0;
			retval= -1;
			break;
	  }	

	  if (*audit_on) {
		if (stat (c_file, &stat_buf)) {
		   fprintf (stderr,"Unable to stat current audit file: errno=%d\n", errno);
                   *c_node=0;
		   retval= -1;
		} else {
		   *c_node=stat_buf.st_cnode;
		}
	        if (*x_file) { 
		   if (stat (x_file, &stat_buf)) {
		     fprintf (stderr,"Unable to stat next audit file: errno=%d\n", errno);
                     *x_node=0;
		     retval= -1;
                   } else {
		     *x_node=stat_buf.st_cnode;
		   }
	         } 
	  } else {
		*c_node=0;
		*x_node=0;
	  }
	return retval;
}
#endif  /* DISKLESS && AUDIT */


/*
 * usage () -- Routine to print shutdown usage message.
 */
/**********************************************************************/
void
usage ()
/**********************************************************************/
{
#ifdef hp9000s300

    (void) fprintf (stderr,
   "Usage: shutdown [ -h | -r [-d device ] [-f lif_file ] ] [-y ] [ grace ]\n");

#else /* hp9000s300 */

    (void) fprintf (stderr, "Usage: shutdown [ -h | -r ] [ -y ] [ grace ]\n");

#endif /* else !hp9000s300 */
}					/* END OF USAGE */

/*  This routine executes user-supplied scripts which are located in the
 *  directory /etc/shutdown.d.  It uses scandir() to build an array of
 *  pointers to dirent structures.  Only the entries chosen by selectFiles ()
 *  will be in the array.  The array will be sorted by using strcmp () on the
 *  directory entry names.  This will result in a ASCII ordering, which is the
 *  way that System V.3 does it.  Files are executed using the system () call,
 *  so they are expected to be shell scripts or programs.
 */

/**********************************************************************/
void
runCustomScripts ()
/**********************************************************************/
{
  struct dirent **nameList;
  struct dirent **fileList;
  int numFiles;
  char exePath [MAXNAMLEN + 1];

  CUSTOM_SCRIPTS_LEN = strlen (CUSTOM_SCRIPTS);

  /*  After lots of discussion, it was decided that the sorting of file
   *  names should be in ASCII order, and fully documented.  This gets
   *  rid of potential problems when users run shutdown using different
   *  LANG variables which would thereby potentially cause things to occur
   *  in a different order from user to user.
   */
  numFiles = scandir (CUSTOM_SCRIPTS, &nameList, selectFiles, ASCIIcompare);
  if (numFiles <= 0)
    return;
  
  fileList = nameList;

  while (numFiles-- > 0)
    {
      /* Compose the complete path name for the qualified scripts */
      (void) sprintf (exePath, "%s/%s", CUSTOM_SCRIPTS, (*fileList)->d_name);
      if (system(exePath) != 0)
	perror ("system");

      free ((void *) *fileList);
      fileList ++;
    }

  free ((void *) nameList);
}

/*  This routine determines which entries in a directory should be saved
 *  to be executed as part of the user's custom shutdown scripts.  This
 *  is basically determined by whether or not the file is executable.
 *  Note that this will get directories too, which really shouldn't be
 *  part of the files being executed. 
 */
/**********************************************************************/
selectFiles (dirEntryPtr)
  struct dirent *dirEntryPtr;
/**********************************************************************/
{
  char pathName [MAXNAMLEN + 1];
  int statReturn;
  struct stat statBuff;

  if (strlen (dirEntryPtr -> d_name) + CUSTOM_SCRIPTS_LEN > MAXNAMLEN)
    return (0);

  (void) sprintf (pathName, "%s/%s", CUSTOM_SCRIPTS, dirEntryPtr -> d_name);

  /*  If the file can be executed, make sure it doesn't start with a "."
   *  and isn't a directory.  This is a slight variation on system V.  In
   *  that system, files that start with a "." are ignored, but directories
   *  are executed (!).  This leads to some very odd output on the screen.
   *
   *  So if the file name starts with a "." then skip it.  If the file 
   *  cannot be stat'd, assume that it should be executed.  If the file
   *  can be stat'd and is a directory skip it.  If it is just a regular
   *  file then it will be executed.
   *
   *  If the file is OK, we need the full pathname to execute it.  So copy it.
   *  This is a calculated risk.  I'm assuming that MAXNAMLEN (255) is long
   *  enough for the full path name of the user's custom script.  The first
   *  element of the path is CUSTOM_SCRIPTS ("/etc/shutdown.d") so that 
   *  leaves lots of room for the file name part.  There is a 255 character
   *  limit since we're putting the full path name back into the dirent
   *  structure.  This structure has a fixed character buffer of 256 bytes for
   *  the file name in the directory.  But we need the full path for both 
   *  the getaccess call and for the system call that will later be used to
   *  execute the shell script.  Note that all this is necessary since the
   *  scandir(3) routine which is building a list of files to execute (using
   *  this routine to pick the files) is really building an array of dirent
   *  structures.
   *
   *  Note that this whole algorithm doesn't address subdirectories in 
   *  /etc/shutdown.d.
   *
   *  Note:  We changed the real user id to root in the main program before
   *         we got to this point, so it's OK to use access(2).
   */
  if ((access (pathName, X_OK)) == 0)
    {
      if (*(dirEntryPtr -> d_name) == '.')
	return (0);

      else 
	{
          statReturn = stat (pathName, &statBuff);

	  if (statReturn == 0)
	    {
	      if (S_ISDIR(statBuff.st_mode))
	        return (0);
	    }
	}

      return (1);
    }
  
  /*
   *  If we get to this point, then the file wasn't executable
   *  so return 0.
   */
  return (0);
}

/*
 *  This routine compares two directory entry file names using strcmp.
 */
/**********************************************************************/
ASCIIcompare (dirEntry1, dirEntry2)
  struct dirent **dirEntry1, **dirEntry2;
/**********************************************************************/
{
  return (strcmp ((*dirEntry1) -> d_name, (*dirEntry2) -> d_name));
}

/*
 *  This code was lifted from ruserok.c (libc/bsdipc).  In that file,
 *  it checks a remote host, remote and local user for access using the
 *  hosts.equiv and .rhosts files.  It also has and option to deny access
 *  to a particular host or user.
 *
 *  For the purpose of access to the shutdown program, several changes
 *  had to be made.
 *
 *    1.  Only one file instead of potentially 2 have to be searched.
 *        (/etc/shutdown.allow versus /etc/hosts.equiv and maybe .rhosts)
 *        This simplified ruserok() since the loop that checks the file
 *        (reading it one line at a time) only has to be executed once.
 *        Also, code after the loop which basically opened a second file
 *        for checking could be deleted.
 *
 *    2.  Checks for denial to a host or user don't make sense in the
 *        case of shutdown authorization.  So this code was also deleted.
 *
 *    3.  The @ (netgroup) wildcard doesn't make sense since all the
 *        machines in the shutdown.allow file are in the same cluster.
 *        So this code was deleted.
 *
 *    4.  In the _checkhost routine, checking for a domain name was only
 *        done at the end of the routine since it would be run a second
 *        time, for the second file.  This was removed entirely since it was 
 *        used in the case where the host in the file did not have a domain
 *        listed.  In our case, this should match.
 *
 *    5.  The routines ruserok(), _checkhost(), and _incluster() were
 *        renamed so that conflicts with the libc.a routines would not occur.
 *        Although _incluster did not need any changes, it is declared as
 *        a static in libc.a so the code had to be duplicated anyway.
 *
 *           ruserok    => shutdownRuserok
 *           _checkhost => shutdownCheckhost
 *           _incluster => shutdownIncluster
 *
 *    6.  The file stream is passed in since the file was already opened
 *        in the allowed() routine.
 *
 *    7.  Lines containing a single hostname and no username are not allowed.
 *        In the .rhosts matching, if such a line is found, then a match
 *        is attempted between the local and remote user names.  In our 
 *        case, there is only the user running the shutdown program.
 *
 *    8.  In _incluster, _checkhost is called with the length of the cnode
 *        name string.  This was changed in the shutdown version of these
 *        routines to use the length of the first part of the official hostname
 *        of the system where shutdown is running.  (baselen in shutdownRuserok
 *        is passed to shutdownInclude and then to shutdownCheckhost.)
 *
 *    9.  Calling parameters for shutdownRuserok were changed to remove
 *        superuser and luser.  A FILE pointer to the allow file was added
 *        to the calling parameters.
 */
/**********************************************************************/
shutdownRuserok (hostf, officialHostname, user)
	FILE *hostf;
	char *officialHostname;
	char *user;
/**********************************************************************/
{
	char line[BUFSIZ];
	char *ahost, *fileUser, *cp;
	int hostmatch, usermatch;
	int baselen = -1;
	char *p;

	/*
	 *  This looks for the first occurrance of a dot in the 
	 *  officialHostname.  If one occurs, then baselen is the length of the
	 *  officialHostname without domain information.  If there isn't a dot,
	 *  then baselen is initialized to -1.
	 */
	if ((p = strchr (officialHostname, '.')) != NULL)
	  baselen = p - officialHostname;

	while (cp = fgets (line, sizeof (line), hostf)) 
	  {
            hostmatch = usermatch = 0;

	    /*
	     *  Skip comment lines.
	     */
            if (*cp == '#')
              continue;

	    /*
	     *  Use strtok() to get a pointer to the officialHostname and put a
	     *  '\0' at the first separator, which must be a space, tab, or
	     *  newline.  If the result is a NULL string, then the line
	     *  began with white space and needs to be skipped.
	     */
            ahost = strtok (cp," \t\n");		
            if (ahost == NULL)
              continue;	

	    /*
	     *  Use strtok() to get a pointer to the user name and put a
	     *  '\0' at the first separator, which must be a space, tab, or
	     *  newline.
	     */
            fileUser = strtok ((char *) NULL," \t\n");

#ifdef DISKLESS
	    /*
	     *  If the host is "%" then this should match any host in the
	     *  cluster.  Use shutdownIncluster to determine if this host
	     *  is in the cluster.
	     */
            if (ahost [0] == '%' && ahost [1] == 0)
              hostmatch = shutdownIncluster (officialHostname, baselen);
	    else

#endif /* DISKLESS */

	    /*
	     *  If the host is "+" then this matches all hosts.
	     */
            if (ahost [0] == '+' && ahost [1] == 0)
              hostmatch = 1;

            else
              hostmatch = shutdownCheckhost (officialHostname, ahost, baselen);

	    if (hostmatch && fileUser) 
	      {
                if (fileUser [0] == '+' && fileUser [1] == 0)
                  usermatch = 1;

                else
                  usermatch = !strcmp (user, fileUser);
              }

            if (hostmatch && usermatch) 
                return TRUE;
	  }

	return FALSE;
}



/**********************************************************************/
static int
shutdownCheckhost (fullHostname, fileHost, len)
	char *fullHostname, *fileHost;
	int len;
/**********************************************************************/
{
	/*
	 *  First, if there is no domain part, the names have to match
	 *  exactly.
	 */
	if (len == -1)
	  return (!strcasecmp (fullHostname, fileHost));

	/*
	 *  There is a domain part to the fullHostName.
	 *  Check to see if the first part matches.
	 */
	if (strncasecmp (fullHostname, fileHost, (size_t) len))
	  return (FALSE);

	/*
	 *  There is a domain part to the fullHostName, and the first parts 
	 *  matched, so try the whole thing.  If it matches then return TRUE.
	 */
	if (! strcasecmp (fullHostname, fileHost))
	  return (TRUE);

	/*
	 *  There is a domain part to the fullHostName, and the first parts
	 *  match, but the second parts don't.  Either the fileHost doesn't
	 *  have a second part, or it does and the second parts don't match.
	 *
	 *  If the fileHost does have a second part, then they just didn't 
	 *  match so return FALSE.  (FileHost was read out of the allow file.)
	 */
	if (*(fileHost + len) != '\0')
	  return (FALSE);

	/*
	 *  We got here since the first parts matched, but the name in the
	 *  allow file didn't have a second part and the fullHostname did.
	 *
	 *  So allow this to be a match.
	 */
	return (TRUE);
}


/**********************************************************************/
static int
shutdownIncluster (thisHost, baselen)
	char *thisHost;
	int baselen;
/**********************************************************************/
{
  cnode_t buf [MAX_CNODE]; 
  int n, i;
  struct cct_entry *getcccid (), *cct;
  struct hostent *nextHost;

  if ((n = cnodes (buf)) <= 0)
    {
      /*
       *  < 0 means error;
       *  = 0 means standalone system;
       *  in either case, fail
       */
      return (FALSE);
    }

  else
    {
      for (i = 0; i < n; i++)
        {
          /* 
           *  assume all cnodes in cluster in local domain 
           */
          if (cct = getcccid (buf [i]))
	    {
	      nextHost = gethostbyname (cct -> cnode_name);
	      if (nextHost == NULL)
		{
	          if (shutdownCheckhost (thisHost, cct -> cnode_name, baselen))
                    return (TRUE);
		}
              
	      else
		{
	          if (shutdownCheckhost (thisHost, nextHost -> h_name, baselen))
                    return (TRUE);
		}
	    }
        }

      return (FALSE);
    }
}

/*
 *  This is a signal handler for when debugging is turned on and we are
 *  waiting for output from the rcmd routine.  In this case, if the rcmd
 *  was executing a reboot, the client died, but the socket is still
 *  around and we are still tyring to read from it.  The network will
 *  eventually kill the socket (in 2 hours), so we use SIGALRM to get
 *  out of the problem.  This routine will set a global that breaks out
 *  of the read loop.
 */
/**********************************************************************/
void
debugHandler ()
/**********************************************************************/
{
  quitRead = TRUE;
  if (debugLevel)
      (void) fprintf (debugStream, "\n*** Caught a SIGALRM signal ***\n");
}


#ifdef DISKLESS
/*
 *  This routine tries to warn a cnode which has users who have open 
 *  processes on this hostname's local mounted file systems.  This way, 
 *  we won't have to bother all users in a cluster with a cwall 
 *  everytime a cnode does a shutdown.
 *  
 *  The general algorithm for this routine:
 *       1. Determine the local file systems 
 *          (/etc/mount -l)
 *       2. For each cnode do
 *               2a. Find pids used by that cnode on local mounts.
 *                   If none, continue;
 *               2b. Send wall message to cnode.
 *                   If fails, wall to entire cluster;
 *               2c. Send SIGTERM, followed by a SIGKILL signal to all 
 *		     open processes on local mounts.
 *
 *  If we have any problems when trying to do the above algorithm, we 
 *  try to warn the cnode involved.  If that fails, then we
 *  will just bail out and cwall the entire cluster.  (Do this by
 *  returning a -1 from this routine.)
 */

/**********************************************************************/
int
warn_users_of_lmfs (sock_port, hostname)
	struct servent *sock_port;
	char *hostname;
/**********************************************************************/

{
  
  /* 
   * numClients is the number of active clients in cluster; active_clients
   * is a list of those client cnode numbers.  If we are a swap server, 
   * active_clients may have already been set -- but only with the clients
   * the swapserver is responsible for rebooting.  Now we need ALL active
   * clients in the entire cluster, so call cnodes to do get ALL active nodes.
   */
  int numClients = cnodes (active_clients);   
  
  register char *cnodeName;     /* stores cnode that we are examining   */
  
  FILE *fp;			/* points to input                      */
  
  char tmp[LARGEBUFSIZE+1]; 	/* used to store input read in          */
  char tmp2[MAXPATHLEN + 1];	/* temp storage for device name         */
  char pid_string[LARGEBUFSIZE+1]; /* stores a list of process ids (separated */
				/* by spaces) as a string of characters */

  char *ptr;			/* points to beginning of buffer	*/
  char *endptr;			/* points to end of buffer		*/
  char *tmpptr;			/* temp pointer into buffers  		*/
  char *pidptr;			/* points to beginning of pid_string    */
  char *endpidptr;		/* points to end of pid_string          */
  char *remoteFuserCmd;		/* remote command for fuser		*/
  char *ahost;			/* for rcmd; points to cnode name	*/
  char *parentPID;		/* points to my parent process id       */
  char *devNames;		/* points to the lmfs device names      */
  
  int i;			/* index				*/
  int myCnode;			/* boolean to indicate this cnode       */
  int got_pids;			/* boolean indicating if there are      */
				/* processes which have opens on the lmfs */
  int rcmd_socket;		/* needed for socket and rcmd commands  */
  int size;			/* indicates bytes needed for malloc    */
  
  /*
   * If numClients = 1, that means we are the only active cnode on this
   * cluster, so skip the rest of this, as we already know our file systems
   * are going away, and we already warned all users on our cnode that
   * we are shutting down.
   */
  if (numClients < 0)
      return(-1);
  else if (numClients <= 1)
      return(0);
  
  /*
   * Determine which files sytems are locally mounted.
   */
  
  if ((fp = popen (MOUNT_CMD, "r")) == NULL) 
  {
      if (debugLevel)
          (void) fprintf (debugStream, "%s: Mount command failed\n", hostname);
      return(-1);
  }

  /*
   * Create a list of all file systems which could not be unmounted.  
   * We use this information when we call the fuser command.
   * Keep growing devNames until we have all the device file names 
   * for the locally mounted file systems.
   */
  devNames = NULL;
  
  while (fgets (tmp, MAXINPUTSTRING, fp) != NULL) 
  {
      sscanf(tmp, "%*s%*s%s%*s", tmp2);
  
     /*
      * Malloc an extra byte to store a space.
      */
      size = strlen(devNames) + strlen(tmp2) + 2;
  
     /*
      * The first time, malloc space so realloc works okay
      * on the next iteration thru the loop.
      */

      if (devNames == NULL)
      {
          devNames = malloc(strlen(tmp2) + 2);
  	  if (devNames)
  	  {
  	      strcpy(devNames, tmp2);
  	      strcat(devNames, " ");
          }
  	  else
  	  {
  	      if (debugLevel)
      	          (void) fprintf (debugStream, "%s: Malloc failed on devNames array\n", hostname);
              pclose(fp);
	      (void) free (devNames);
  	      return(-1);
          }
      } 
      else
      {
          if (realloc (devNames, size) != NULL)
          {
  	      strcat(devNames, tmp2);
  	      strcat(devNames, " ");
          } 
          else 
          {
  	      if (debugLevel)
  	      {
      	          (void) fprintf (debugStream, 
  		         "%s: Realloc failed on devNames array\n", hostname);
      	          (void) fprintf (debugStream, "%s: devicefile = %s\n"
  	                 "      devNames = %s\n", hostname, tmp2, devNames);
  	      }
              pclose(fp);
  	      return(-1);
          }
      }
   
  }  /* end "while fgets" */
  
  pclose(fp);
  
  
  if (debugLevel)
      (void) fprintf (debugStream, "%s: LMFS device names = *%s*\n", 
		      hostname, devNames);
  
  
 /*
  * Build the remote fuser command
  */
  
  remoteFuserCmd = malloc (strlen (FUSER_CMD) + strlen (devNames) + strlen (STDERR_TO_NULL) + 1);
   
  if (remoteFuserCmd)
      (void) sprintf (remoteFuserCmd, "%s%s%s", FUSER_CMD, devNames, 
		      STDERR_TO_NULL);
  else 
  {
      if (debugLevel)
          (void) fprintf (debugStream, "%s: Malloc for fuser command failed\n", hostname);
      (void) free (devNames);
      return(-1);
  }
  
      
  (void) free (devNames);
  
 /*
  *  Test to see if we already have an open socket; if we do,
  *  use it, otherwise try to open one.
  */
  if (! sock_port) 
  {
      if (debugLevel)
  	  (void) fprintf (debugStream, "%s: Get a socket\n", hostname);
      if (! (sock_port = getservbyname ("shell", "tcp"))) 
      {
  	  (void) fprintf (stderr, "Networking error: Cannot open socket -- continuing shutdown.\n");
	  (void) free (remoteFuserCmd);
  	  return(-1);
      }
  }
  
  
 /*
  *  If we are not the rootserver or swapserver, the cnode_array never 
  *  gets populated.  We need that information here, so populate the 
  *  cnode array.
  */

  if (! (rootserver || swapserver))
      get_cnode_names ();
  
  if (debugLevel)
  {
      (void) fprintf (debugStream, "%s: Active cnodes : ", hostname);

      for (i=0, cnodeName = cnode_array[active_clients[i]].name ; 
	i < numClients; i++, cnodeName = cnode_array[active_clients[i]].name) 
      {     
	    (void) fprintf (debugStream, "%s ", cnodeName);
      }

      (void) fprintf (debugStream, "\n");
  }
	      
  
 /*
  *  2. For each cnode do
  *     2a. Find pids used by that cnode on local mounts.
  *         If none, continue;
  *     2b. Send wall message to cnode.
  *         If fails, wall to entire cluster;
  *     2c. Send SIGTERM, followed by a SIGKILL signal to all 
  *         open processes on local mounts.
  *
  *  If we are inside this for "each cnode" loop, and there is 
  *  a problem warning the ttys, try only to warning that cnode first.
  *  If that fails, we bail out and warn the entire cluster.
  *
  *  For each cnode, execute fuser to see if they have any
  *  processes with opens on our locally mounted file systems.
  */
  
  for (i=0, cnodeName = cnode_array[active_clients[i]].name ; i < numClients; 
                         i++, cnodeName = cnode_array[active_clients[i]].name) 
  {
  
      if (cnodeName == NULL)
  	  continue;
  
     /*
      * Determine if this is my cnode that is shutting down.
      */
      myCnode = (strcmp(cnodeName, hostname)) ? FALSE : TRUE;
  
  
     /*
      * Execute fuser command, saving output.
      */
      if (myCnode)
  	  fp = popen(remoteFuserCmd, "r");
      else
      {
  	  ahost = cnodeName;
          if ( (rcmd_socket = rcmd (&ahost, sock_port -> s_port, ROOT, ROOT,
                                    remoteFuserCmd, (int *) 0)) < 0)  
  	  {
  	      if (debugLevel) 
  	      {
  	          (void) fprintf (debugStream, 
  	                 "%s: Cannot do rcmd to %s with remoteFuserCmd: %s\n", 
  		          hostname, cnodeName, remoteFuserCmd);
  		  (void) fprintf (debugStream, 
  		         "%s:     Return from rcmd = %d\n", hostname, 
			  rcmd_socket);
  	      }

      	      (void) fprintf (stderr, "%s%s%s", START_CANNOT_KILL_PROCESSES,
				  cnodeName, END_CANNOT_KILL_PROCESSES);

	      if (warn_cnode (cnodeName, hostname, sock_port, 0) < 0)
	      {
		  (void) free (remoteFuserCmd);
                  return(-1);
	      }
	      else
		  continue;
          }
  	  else
              fp = fdopen(rcmd_socket, "r");
      }
  
      if (fp == NULL) 
      {
      	  (void) fprintf (stderr, "%s%s%s", START_CANNOT_KILL_PROCESSES,
				  cnodeName, END_CANNOT_KILL_PROCESSES);

  	  if (debugLevel) 
  	  {
  	      (void) fprintf (debugStream, 
  	             "%s: File pointer == NULL for remoteFuserCmd output from: %s\n", 
  	             hostname, cnodeName);
	  }

	  if (warn_cnode (cnodeName, hostname, sock_port, 0) < 0)
	  {
	      (void) free (remoteFuserCmd);
              return(-1);
	  }
	  else
	      continue;
      }
  
     /*
      * Grab the fuser output, which contains the pids for all
      * processes with open files on our unmounted file systems.
      *
      */
      pid_string[0] = '\0';
      got_pids = FALSE;
  
      if (fgets (pid_string, LARGEBUFSIZE, fp) != NULL) 
      {
          if ((tmpptr = strrchr(pid_string, '\n')) !=  NULL)
              *tmpptr = '\0';

          if (strlen(pid_string) != 0)
              got_pids = TRUE;
      }

      if (myCnode)
  	  pclose(fp);
      else
          fclose (fp);
 
     /*
      * First, check the validity of the fuser output.  Fuser in 8.0
      * was rather flakey, so make sure the fuser output only contains
      * digits and spaces.  If it's anything else, then immediately
      * warn the cnode.
      */
      if (got_pids)
      {
	  ptr = pid_string;
	  endptr = strrchr(pid_string, '\0');
	  while (ptr < endptr && ((*ptr == ' ') || isdigit(*ptr))) 
	      ptr++;
	  if (ptr < endptr)
	     /*
	      * We have some invalid fuser data.  Warn the 
	      * cnode about the local mounted file systems going away.
	      * We won't try and kill any processes because our
	      * fuser output is bad.
	      */

  	      if (debugLevel) 
  	      {
  	          (void) fprintf (debugStream, 
  	             "%s: Bad data from remoteFuserCmd on: %s\n", 
  	             hostname, cnodeName);
	      }

      	      (void) fprintf (stderr, "%s%s%s", START_CANNOT_KILL_PROCESSES,
				  cnodeName, END_CANNOT_KILL_PROCESSES);
	      if (warn_cnode (cnodeName, hostname, sock_port, 0) < 0)
	      {
		  (void) free (remoteFuserCmd);
                  return(-1);
              }
	      else
		  continue;
  
          if (debugLevel)
  	      (void) fprintf (debugStream, 
  	              "%s: PIDs with opens on lmfs on cnode %s:\n     *%s*\n", 
  	               hostname, cnodeName, pid_string);
  
         /*
          * For this cnode, we have all the PIDS which have opens
          * on our local mount file systems.  Warn that cnode
	  * that our lmfs are going away.  Then, kill those processes
	  * access our lmfs.
          * Don't bother warning our cnode -- they already know we are
          * shutting down.
          */
  
	  if (! myCnode)
	  {
	      if (warn_cnode (cnodeName, hostname, sock_port, 1) < 0)
	      {
      		  (void) fprintf (stderr, "%s%s%s", START_CANNOT_KILL_PROCESSES,
				  cnodeName, END_CANNOT_KILL_PROCESSES);
		  (void) free (remoteFuserCmd);
                  return(-1);
              }
	      else
	      {
	          (void) signal_processes (pid_string, cnodeName, 
					       sock_port, hostname);
		  continue;
	      }
	  }
	  else   /* myCnode is true */
          {
	     /* 
	      * Deal with users home directory mounted as a
	      * locally mounted file system, the user running ksh or
	      * csh, and then running shutdown.  The shell keeps the
	      * history file open on the LMFS, and will not allow
	      * the file system to be unmounted.  
	      *
	      * First, find the parent PID, and remove it from 
	      * the array list if it is present.  This way, we 
	      * won't kill our own shell.  If the parent PID is
	      * the only PID in the pid_string, then don't try
	      * sending any signals.
	      */

              parentPID = ltoa((long) getppid());

              if (pidptr = strstr(pid_string, parentPID)) 
  	      {
  	          endpidptr = pid_string + (strlen (pid_string));
                  while (*pidptr != ' ' && pidptr < endpidptr)
                      *pidptr++ = ' ';

  		  pidptr = pid_string;

  		  while (*pidptr == ' ' && pidptr < endpidptr)
  		      pidptr++;
  	      }
	      else
  	          pidptr = pid_string;


  	      if (strlen(pidptr)) 
		  (void) signal_processes (pid_string, cnodeName, sock_port, hostname);

          } 

    
     }  /* end "if (got_pids)" */
  
  }  /* end "for each cnode" */
 
  (void) free (remoteFuserCmd);

}  /* end warn_users_of_lmfs */

#endif  /* DISKLESS */


/*
 *
 *  This routine warns all users on a cnode that had open processes
 *  on our local mounted file systems.  If this routine has problems,
 *  we will (eventually) return back to the main program, and cwall
 *  the entire cluster.
 *
 */
#ifdef DISKLESS

/******************************************************************/
int
warn_cnode (cnodeName, hostname, sock_port, kill_warning)
	char *cnodeName;
	char *hostname;
	struct servent *sock_port;
	int kill_warning;
/******************************************************************/
{
  char wallCommand [250];    /* Don't want to depend on malloc for space.  */
  char *ahost;
  int rcmd_socket;
  
  ahost = cnodeName;
  
 /*
  *  Depending on where "warn_cnode" is called, we may be able to kill
  *  user processes which are accessing our local mounted file systems.
  *  The boolean "kill_warning" indicates when we should warn users
  *  that their processes will be terminated in 30 seconds.
  */

  if (kill_warning)
      (void) sprintf (wallCommand, "%s%s%s%s", START_WARNING, hostname, 
		      END_KILL_WARNING, PIPE_TO_WALL);
  else
      (void) sprintf (wallCommand, "%s%s%s%s", START_WARNING, hostname, 
		      END_WARNING, PIPE_TO_WALL);


  if ((rcmd_socket = rcmd (&ahost, sock_port -> s_port, ROOT, ROOT,
                     wallCommand, (int *)0)) < 0) 
  {
       (void) fprintf (debugStream,
                    "%s: Could not warn users on %s", hostname, cnodeName);
       return(-1);
  } 
  else
  
       if (debugLevel)
           (void) fprintf (debugStream, "%s: Warned cnode: %s\n", hostname, cnodeName);
  
  return(0);

}  /* end warn_cnode */

#endif  /* DISKLESS */

/*
 *
 *  This routine is called to send a SIGTERM followed by a SIGKILL
 *  to the process ids specified in pid_string.
 *
 */

#ifdef DISKLESS 
/******************************************************************/
void 	
signal_processes (pid_string, cnodeName, sock_port, hostname) 
	char *pid_string;
	char *cnodeName;
	struct servent *sock_port;
	char *hostname;
/******************************************************************/
{

  char *remoteKillCmd;             /* stores kill command                   */
  char *ahost;			   /* points to cnode name for rcmd         */
  int   myCnode;		   /* boolean determining if its my system  */
  int   rcmd_socket;               /* for rcmd return value                 */


 /*
  * Am I trying to kill processes on my own cnode??
  */
  myCnode = (strcmp(cnodeName, hostname)) ? FALSE : TRUE;


 /*
  * Build the remote kill command, sending SIGTERM signal.
  */
  remoteKillCmd = malloc (strlen (SIGTERM_CMD) + strlen (pid_string) + 
		  strlen(STDERR_TO_NULL) + 1);
  
  if (! remoteKillCmd) 
  {
      (void) fprintf (stderr, "%s%s%s", START_CANNOT_KILL_PROCESSES,
				  cnodeName, END_CANNOT_KILL_PROCESSES);
      if (debugLevel)
 	  (void) fprintf (debugStream, "%s: Error with malloc for kill cmd;"
		    "cannot kill processes on %s\n", hostname, cnodeName);
      return;
  }

 
  (void) sprintf (remoteKillCmd, "%s%s%s", SIGTERM_CMD, pid_string, STDERR_TO_NULL);

  ahost = cnodeName;

 /*
  * We will just wait a bit before sending a SIGTERM to
  * all the processes.
  */
  sleep (WAIT_BEFORE_SIGNAL + WAIT_BEFORE_SIGNAL);

  if (myCnode)
  {
      (void) system (remoteKillCmd);
      (void) sprintf (remoteKillCmd, "%s%s%s", KILL_CMD, pid_string, STDERR_TO_NULL);
      sleep (WAIT_BEFORE_SIGNAL);
      (void) system (remoteKillCmd);
  } 
  else
  {
      if ( (rcmd_socket = rcmd (&ahost, sock_port -> s_port, 
                            ROOT, ROOT, remoteKillCmd, (int *) 0)) < 0)  
      {
      	  (void) fprintf (stderr, "%s%s%s", START_CANNOT_KILL_PROCESSES,
				  cnodeName, END_CANNOT_KILL_PROCESSES);
          if (debugLevel) 
  	      (void) fprintf (debugStream, 
  	             "%s: Remote 'kill -SIGTERM' command failed on %s -- %s\n", 
		     hostname, cnodeName, remoteKillCmd);
          (void) free (remoteKillCmd);
          return;
      } 
      else 
      {  
         /*
          *  Really kill the processes this time  
  	  */
  
  
          (void) sprintf (remoteKillCmd, "%s%s%s", KILL_CMD, pid_string, STDERR_TO_NULL);
          ahost = cnodeName;
  
  	  sleep (WAIT_BEFORE_SIGNAL);

          if ( (rcmd_socket = rcmd (&ahost, sock_port -> s_port, 
  	                           ROOT, ROOT, remoteKillCmd, (int *) 0)) < 0)
	  {
      	      (void) fprintf (stderr, "%s%s%s", START_CANNOT_KILL_PROCESSES,
				  cnodeName, END_CANNOT_KILL_PROCESSES);
  	      if (debugLevel) 
  	          (void) fprintf (debugStream, 
  	              "%s: Remote 'kill -SIGKILL' command failed on %s -- %s\n",
  	               hostname, cnodeName, remoteKillCmd);
              (void) free (remoteKillCmd);
 	      return;
           }

  
      }
  }

(void) free (remoteKillCmd);

}  /* end signal_processes */

#endif  /* DISKLESS */

