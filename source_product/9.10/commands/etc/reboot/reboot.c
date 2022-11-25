static char *HPUX_ID = "@(#) $Revision: 70.2 $";

#include <stdio.h>
#include <sys/types.h>
#include <ctype.h>
#include <utmp.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <time.h>
#include <sys/signal.h>
#include <sys/stat.h>
#include <errno.h>
#include <sys/sysmacros.h>
#include <string.h>

#ifdef LOCAL_DISK
#include <netdb.h>
#include <sys/pstat.h>
#endif /* LOCAL_DISK */

#if	defined(DUX) || defined(DISKLESS)
#include <cluster.h>
#include <sys/nsp.h>
#include <sys/param.h>
#endif

#ifdef DEBUG
#include "sys/reboot.h"
#else
#include <sys/reboot.h>
#endif /* DEBUG */
#if defined(SecureWare) && defined(B1)
#include <sys/security.h>
#include <sys/audit.h>
#endif

/*
 *	/etc/reboot
 *
 *	allow super users to reboot the system automatically;
 *	tell and remind users of imminent reboot;
 */
#define	HOURS	*3600
#define MINUTES	*60
#define SECONDS

#ifdef LOCAL_DISK
/*
 * General use defines
 */
#define TRUE 1
#define FALSE 0
#define MAX_WAIT_FOR_RCMD  40      /* in seconds, the max time we will  */
				   /* wait for the rcmd command.  This  */
				   /* should really only take about 5-6 */
				   /* seconds on a healthy system.      */

#define PSBURST 10				/* # pstat structs to get    */

/*
 * Constants used for rcmd execution of reboot commands on clients
 */

#define CMD_LENGTH_PAD	4		        /* pad for additional quotes */
static char  ROOT [] =	"root";			/* user name for rcmd        */
static char REBOOT_CMD [] = "/bin/ksh -c \"/etc/reboot"; 
			/* make sure the reboot cmd is exec under ksh,        */
			/* because csh doesn't understand ksh output redirect */

#if defined (DEBUG1) || defined (DEBUG2)
			  /* if debugging, save output for later perusal */
static char CMD_MISC [] = " > /tmp/reboot.out 2>&1 \" &";
#else 
				/* close stdout, in, err, and don't wait */
static char CMD_MISC [] = " 2>&- >&- <&- \" &"; 
#endif /* DEBUG1 || DEBUG2 */
#endif /* LOCAL_DISK */

#ifdef DEBUG
static char LOGFILE [] = "shutdown.log";
#else
static char LOGFILE [] = "/usr/adm/shutdownlog";
#endif

static char CONSOLE [] = "/dev/console";
static char INIT [] = "/etc/init";

/*
 *   Option parsing has been changed to allow all options regardless of whether
 *   or not they are architecture dependent.  The option parsing code then 
 *   decides what to do based on the architecture.  This avoids any problems
 *   in heterogeneous clusters where reboot can be invoked on a Series 800 
 *   with Series 300-only options.
 *
 *     base options:  -h, -r, -n, -s, -t, -m, -q
 *     S300 only options: -d, -f
 *     S300 and DUX only option: -l, -b
 *
 *   (UCSqm00452): The above comments are not tue. The four options are 
 *   s300/s400 only, and *WILL NOT* pass to their cnode clients (even 
 *   from a s800 cluster server)! Therefore, 4 exit(1) are added
 *   after usage() for the -d, -f, -l and -b in the option parsing code. 
 *
 *   reboot(1M) and shutdown(1M) man pages are updated to clarify about 
 *   the in-validity of these four option for s700/s800.
 */
char *optflgs = "hrnst:m:qd:f:l:b:D:";

struct	utmp utmp;

int	sint;
int	stogo;

int	hflag, rflag, nflag, sflag, qflag;
int	errflg;

#if defined(SecureWare) && defined(B1)
char auditbuf[80];
#endif

#if defined(hp9000s300) && (defined(DUX) || defined(DISKLESS))
char	*server_list = "/etc/bootservers";
int	lflag = 0;
int	bflag = 0;
#endif /* s300 DUX/DISKLESS */

char	tpath[]		= "/dev/";
char	tbuf[BUFSIZ]	= "now";	/* default time value = now */
char	term[sizeof tpath + sizeof utmp.ut_line];

/*  
 *  Add one character to the end for a NULL terminator.
 */
char	hostname[MAXHOSTNAMELEN + 1];

char	*mess;

time_t	nowtime;

struct interval {
	int stogo;
	int sint;
} interval[] = {
	4 HOURS,	1 HOURS,
	2 HOURS,	30 MINUTES,
	1 HOURS,	15 MINUTES,
	30 MINUTES,	10 MINUTES,
	15 MINUTES,	5 MINUTES,
	10 MINUTES,	5 MINUTES,
	5 MINUTES,	3 MINUTES,
	2 MINUTES,	1 MINUTES,
	1 MINUTES,	30 SECONDS,
	0 SECONDS,	0 SECONDS
};

#ifdef LOCAL_DISK
/*
 * Globals used for root server and swap server reboot information
 *   Made global so that the active_clients() routine could use the same
 *   variables, especially mysite, rootserver, and swapserver
 */
cnode_t 	mysite = 0;			/* cnode id for executing site	  */
int 	rootserver = 0;			/* boolean for root server	  */
int 	swapserver = 0;			/* boolean for swap server	  */
int 	num_active_clients;		/* # clients of this root/swap svr*/
cnode_t active_clients[MAXCNODE];	/* active client cnode ids 	  */
struct	entry{
	    char *name;			/* name of cnode 		  */
	    int	donot_reboot;		/* true if shouldn't be rebooted  */
	} cnode_array[MAXCNODE];	/* names of cnodes in clusterconf */

#endif /* LOCAL_DISK */

char	*shutter;
char	s[512];

/*  
 * These are for the hidden -D debugging option.
 */
int debugLevel = 0;
FILE *debugStream = NULL;

/*
 *  Routine declarations.
 */

#ifdef __STDC__

#ifdef LOCAL_DISK

int clients_rebooted();
void get_cnode_names();
int kill_process(char *);
void print_active_clients(FILE *);

#endif /* LOCAL_DISK */

void dingdong();
void setalarm();
time_t getsdt(char *);
void warn(time_t, time_t, char *);
void finish();
void do_nothing();
void log_entry (time_t, int);
void set_stderr();
void nbwrite(int);
int notatty(int);
int isapty(int);
void opt_err(char *);
void usage();
void check_dev(char *);
int getbserver(char *, char *, char *);
struct utmp *getutid(struct utmp *);
char *getlogin();

# else /* __STDC__ */

#ifdef LOCAL_DISK

int clients_rebooted();
int get_cnode_names();
int kill_process();
void print_active_clients();

#endif /* LOCAL_DISK */

void dingdong();
void setalarm();
time_t getsdt();
void warn();
void finish();
void do_nothing();
void log_entry ();
void set_stderr();
void nbwrite();
int notatty();
int isapty();
void opt_err();
void usage();
void check_dev();
int getbserver();
struct utmp *getutid();
char *getlogin();

# endif /* __STDC__ */


/**********************************************************************/
main(argc, argv)
int argc;
char **argv;
/**********************************************************************/
{
	int 	flags, h, m, first, i, ufd;
	char	opt;
	char 	*devname = "",
		*filename = "";
	char 	*ts, *f;
	time_t	sdt;
	extern	int optind;
	extern	char *optarg;
	char	*messPtr;
	time_t  *tloc;

#if defined(DUX) || defined(DISKLESS)

#ifdef LOCAL_DISK

int 	index;				/* index into active_clients	  */
	/* 
	 * variables for use in rcmd(2) of reboot to root/swap clients
	 */
	char *reboot_cmd = NULL;	/* reboot command string */
	int rcmd_socket;		/* socket descr for rcmd */
	struct servent *sock_port;	/* socket port structure */
	char *ahost;			/* dummy hostname pointer for rcmd */

#else
	int	retry;
#endif /* LOCAL_DISK */

	struct cct_entry *cct_ent;
#endif /* DUX || DISKLESS */

#if defined(hp9000s300) && (defined(DUX) || defined(DISKLESS))

	char *server_linkaddress;
	char *boot_server;

#endif /* s300 DUX/DISKLESS */
	
	/*  For POSIX signal handling:
	 */
	struct sigaction mainAction;

#ifndef DEBUG

#if defined(SecureWare) && defined(B1)

	if(ISB1){
	    set_auth_parameters(argc, argv);
	    if (!authorized_user("sysadmin")) {
		fprintf(stderr,
		  "reboot: You must have sysadmin subsystem authorization\n");
		exit(1);
	    }
	    initprivs();
	    (void) forcepriv(SEC_ALLOWMACACCESS);
	    (void) forcepriv(SEC_KILL);
	    (void) forcepriv(SEC_ALLOWDACACCESS);
	    (void) forcepriv(SEC_SHUTDOWN);
	    (void) forcepriv(SEC_REMOTE);
	}
	else{
	    if(geteuid()) {
		fprintf(stderr, "reboot: You are not super-user.  Sorry.\n");
		exit(1);
	    }
	}
#else
	if(geteuid()) {
		fprintf(stderr, "reboot: You are not super-user.  Sorry.\n");
		exit(1);
	}
#endif
#endif /* DEBUG */

	shutter = getlogin();

	/*  
	 *  Use the size -1 so that the name will be NULL terminated.
	 */
	gethostname(hostname, (sizeof(hostname) - 1));

#ifdef LOCAL_DISK
	/*
	 * Are we the root server or a swap server in a cluster?
	 */
	if ((mysite = cnodeid()) && (cct_ent = getcccid(mysite)) != NULL) {
	    if (cct_ent->cnode_type == 'r') 
		rootserver = TRUE;
	    else if (cct_ent->swap_serving_cnode == mysite) 
		swapserver = TRUE;
	}

	/*
	 * If we're a swap server or a rootserver, we need to allocate
	 * a string to collect command line arguments which we will later
	 * use to reboot our clients.
	 *
	 * We also need to populate cnode_array[] for use rebooting
	 */
	 if (rootserver || swapserver) {
	    int string_length = 0;
	    for (index = 1; index < argc; index++) {	/* skip argv[0] */
		/*
		 * Heuristic estimate of required string length:
		 *   figure out length of combined options for command line
		 *   plus two extra char for spaces and hyphen between options
		 */
		string_length += strlen(argv[index]) + 2;
	    }
	    /*
	     * Figure length of command line argument as 
	     *		length of combined option strings
	     * 		+ length of reboot command itself 
	     *		+ padding for extra quotes on message (-m option) 
	     *		+ length of stdout redirect and & string (CMD_MISC)
	     * 		+ 1 for the null terminator 
	     */
	    if ((reboot_cmd = (char *) malloc(strlen(REBOOT_CMD) + 
		string_length + CMD_LENGTH_PAD + strlen(CMD_MISC) + 1)) == 0) {
		/*
		 * encountered an error trying to malloc space --
		 * perror and try to continue rebooting system -- 
		 * Reboot will complain latter that it couldn't reboot
		 * all the clients 
		 */
		perror("malloc");
	    }  
	    else {
		/* 
		 * copy reboot command into newly malloc'd string 
		 *
		 *  Note that REBOOT_CMD does not have a space at the
		 *  end, so any other strings that are copied on it must
		 *  begin with a space.
		 */
		(void) strcpy(reboot_cmd, REBOOT_CMD);
	    }
	    /*
	     * populate cnode_array[]
	     */
	    get_cnode_names();
	}
#endif /* LOCAL_DISK */

	/*
	 *  Although the Series 800 does not understand the -l, -b, -d, and -f
	 *  options, they could get passed.  The -l, and -b options can get
	 *  passed from reboot on a Series 300 to a client Series 800.  The
	 *  other two options could get passed from the shutdown program, 
	 *  which passes them on to reboot.
	 */
	flags = 0; 	/*default to sync and reboot from the current disk*/
	while ((opt = getopt(argc, argv, optflgs)) != EOF) {
		switch(opt) {
		case 'h':
			flags |= RB_HALT;
			hflag = 1;
#ifdef LOCAL_DISK
			/* 
			 * This option not passed on to client reboots
			 */
#endif /* LOCAL_DISK */
			break;
		case 'r':
			flags &= ~RB_HALT;
			rflag = 1;
#ifdef LOCAL_DISK
			/* 
			 * Pass on to client reboots
			 */
			 if (reboot_cmd) (void) strcat(reboot_cmd," -r");
#endif /* LOCAL_DISK */
			break;
		case 'n':
			flags |= RB_NOSYNC;
			nflag = 1;
#ifdef LOCAL_DISK
			/* 
			 * This option not passed on to client reboots
			 */
#endif /* LOCAL_DISK */
			break;
		case 's':
			flags &= ~RB_NOSYNC;
			sflag = 1;
#ifdef LOCAL_DISK
			/* 
			 * Pass on to client reboots
			 */
			 if (reboot_cmd) (void) strcat(reboot_cmd," -s");
#endif /* LOCAL_DISK */
			break;
		case 'd':
#ifdef hp9000s300
			flags |= RB_NEWDEVICE;
			devname = optarg;
			check_dev(devname);
#ifdef LOCAL_DISK
			/* 
			 * This option not passed on to client reboots
			 */
#endif /* LOCAL_DISK */
#endif /* hp9000s300 */

#ifdef hp9000s800
			usage ();
			exit(1);
#endif /* hp9000s800 */
			break;
		case 'f':
#ifdef hp9000s300
			flags |= RB_NEWFILE;
			filename = optarg;
#ifdef LOCAL_DISK
			/* 
			 * This option not passed on to client reboots
			 */
#endif /* LOCAL_DISK */
#endif /* hp9000s300 */

#ifdef hp9000s800
			usage ();
			exit(1);
#endif /* hp9000s800 */
			break;
		case 'l':
#if defined(hp9000s300) && (defined(DUX) || defined(DISKLESS))
			lflag++;
			flags |= RB_NEWSERVER;
			server_linkaddress = optarg;
#ifdef LOCAL_DISK
			/* 
			 * Pass on to client reboots
			 */
			 if (reboot_cmd) {
			     (void) strcat(reboot_cmd, " -l");
			     (void) strcat(reboot_cmd, server_linkaddress);
			}
#endif /* LOCAL_DISK */
#endif /* s300 DUX/DISKLESS */

#ifdef hp9000s800
			usage ();
			exit(1);
#endif /* hp9000s800 */
			break;
		case 'b':
#if defined(hp9000s300) && (defined(DUX) || defined(DISKLESS))
			bflag++;
			flags |= RB_NEWSERVER;
			boot_server = optarg;
#ifdef LOCAL_DISK
			/* 
			 * Pass on to client reboots
			 */
			 if (reboot_cmd) {
			     (void) strcat(reboot_cmd, " -b");
			     (void) strcat(reboot_cmd, boot_server);
			}
#endif /* LOCAL_DISK */
			if ((server_linkaddress = (char *) malloc(32)) == NULL)
			  {
			    perror("malloc");
			    exit(1);
			  }

			if ((devname = (char *) malloc(MAXPATHLEN)) == NULL)
			  {
			    perror("malloc");
			    exit(1);
			  }

			switch (getbserver(boot_server, server_linkaddress, devname)) {
			case -1:
				fprintf(stderr, "reboot: Can't open %s.\n", server_list);
				exit(1);
			case 0:
				fprintf(stderr, "reboot: Can't find %s in %s\n.",
					boot_server, server_list);
				exit(1);
			case 3:		/* device specifier found */
				flags |= RB_NEWDEVICE;
				check_dev(devname);
			case 2:
				break;
			case 1:
			default:
				fprintf(stderr, "reboot: Format error in %s.\n", server_list);
				exit(1);
			}

#endif /* s300 DUX/DISKLESS */
#ifdef hp9000s800
			usage ();
			exit(1);
#endif /* hp9000s800 */
			break;
		case 't':
			strcpy(tbuf, optarg);
#ifdef LOCAL_DISK
			/* 
			 * Pass on to client reboots
			 */
			if (reboot_cmd) {
			    (void) strcat(reboot_cmd, " -t");
			    (void) strcat(reboot_cmd, tbuf);
			}
#endif /* LOCAL_DISK */
			break;
		case 'm':
			/*
			 *  If the message has double quotes in it then the
			 *  instances where strcat is used to append it onto
			 *  another string will get confused.  This occurs
			 *  for local discs when creating a reboot command
			 *  to be passed to clients and also further in the
			 *  reboot code when the message to be sent to 
			 *  users on the system is built.  This is documented
			 *  in the man page, but a check is added here so
			 *  that such an illegal message is truncated at the
			 *  first double quote.
			 */
			mess = optarg;

			if ((messPtr = strchr (mess, "\"")) != NULL)
			  {
			    fprintf(stderr, 
				    "Double quote in message; truncating.\n");
			    *messPtr = '\0';
			  }
#ifdef LOCAL_DISK
			/* 
			 * Pass on to client reboots
			 */
			if (reboot_cmd) {
			    /* Have to add quotes for -m options argument */
			    (void) strcat(reboot_cmd, " -m\"");
			    (void) strcat(reboot_cmd, mess);
			    (void) strcat(reboot_cmd, "\"");
			}
#endif /* LOCAL_DISK */
			break;
		case 'q':
			qflag++;
#ifdef LOCAL_DISK
			/* 
			 * This option not passed on to client reboots
			 */
#endif /* LOCAL_DISK */
			break;

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
		case 'D':
			debugLevel = atol(optarg);
			if (! debugStream)
			  {
			    debugStream = fopen("/rebootDebugLog", "a");
			    if (debugStream == 0)
			      debugLevel = 0;
			    
			    (void) setvbuf(debugStream, NULL, _IONBF, 0);
			  }
                        
                        /*  
			 *  Pass on to clients.
			 */     
			if (reboot_cmd)
			  {
			    (void) strcat(reboot_cmd, " -D");
			    (void) strcat(reboot_cmd, optarg);
			  }

			break;

		case '?':
			errflg++;
			break;
		}
	}
	if (errflg || (argc != optind)) {
		usage();
		exit(1);
	}

	if (debugLevel && reboot_cmd)
	  fprintf(debugStream, "%s: Constructed reboot command: %s\n",
		  hostname, reboot_cmd);

#ifdef hp9000s300
	if ((flags & RB_HALT) && (flags & (RB_NEWDEVICE | RB_NEWFILE)))
		opt_err("-h and -d or -f");
#endif /* hp9000s300 */

	if ((nflag && sflag) == 1) 
	  opt_err("-s and -n");

	if ((rflag && hflag) == 1) 
	  opt_err("-r and -h");

#if defined(hp9000s300) && (defined(DUX) || defined(DISKLESS))
	if ( (lflag && bflag) == 1 ) opt_err("-l and -b");
#endif /* s300 DUX/DISKLESS */

	/*  Set up the signal handler, mask, and flags for POSIX
	 *  signals used in this program.
	 */
	mainAction.sa_handler = SIG_IGN;
	(void) sigemptyset(&mainAction.sa_mask);
	mainAction.sa_flags = 0;

	/*
	 * If the -q (quick) or -n (no sync) option is used, then
	 * reboot immediately without generating any file system
	 * activity.
	 * 
	 *  For root servers and swap servers, don't try to reboot
	 *  clients either -- just let them die a horrible death.
 	 */
	if (qflag || nflag) {
		
		/*  Set up HUP, INT, and QUIT to be SIG_IGN.  
		 *  mainAction.sa_handler was set to SIG_IGN above.
		 */
		(void) sigaction(SIGHUP, &mainAction, (struct sigaction *) 0);
		(void) sigaction(SIGINT, &mainAction, (struct sigaction *) 0);
		(void) sigaction(SIGQUIT, &mainAction, (struct sigaction *) 0);

#if	defined(DUX) || defined(DISKLESS)
		csp(NSP_CMD_ABS,0);
#endif

		/*  
		 *  This causes init to idle for 60 seconds or until it 
		 *  receives another signal (SIGUSR2).  This allows the kill 
		 *  to kill everything without init cranking them all up again.
		 */
		kill(1, SIGUSR1);
#if defined(SecureWare) && defined(B1)
		if(ISB1)
		    reboot_shut_daemons();
#endif

		/*  KILL_ALL_OTHERS is essentially -1, which causes the signal 
		 *  to be sent to all processes except system processes if the 
		 *  euid is root.
		 */
		kill(KILL_ALL_OTHERS, SIGKILL);
		sleep(3);

		/*  The reboot (2) system call can fail if the device file is 
		 *  bad.  Some of the potential errors are caught in 
		 *  check_dev(), but not all.  Specifically, ENET (a remote
    		 *  file), ENXIO (the device does not exist), and EPERM (euid 
		 *  not root) are not caught.  Note that the euid check is 
		 *  performed by this program as one of its first actions.
                 *
    		 *  In the case where reboot (2) fails, it is tried again.  If 
		 *  this also fails, init is restarted (by sending it a 
		 *  SIGUSR2) and the program exits.
		 */

#if defined(hp9000s300) && (defined(DUX) || defined(DISKLESS))
		reboot(flags, devname, filename, server_linkaddress);
#else
		reboot(flags, devname, filename);
#endif /* s300 DUX/DISKLESS */
		perror("reboot");
		if (nflag)
			reboot(RB_HALT | RB_NOSYNC, NULL, NULL);
		else
			reboot(RB_HALT, NULL, NULL);
		kill(1, SIGUSR2);
		exit(-1);
	}

#ifdef LOCAL_DISK
	/*
	 * If we're a swap server, but not the root server, we
	 * need to reboot all of our swap clients.
	 * 
	 * If we're the root server, we need to reboot all of our
	 * swap clients, and all cnodes which swap locally.  
	 *
	 * Since the class of "all cnodes which swap locally" 
	 * includes all swap server cnodes, and since all swap
	 * server cnodes reboot their own clients, rebooting all 
	 * the rootservers swap clients, and all locally swapping
	 * cnodes will eventually reboot all cnodes in the cluster
	 * in reverse order of dependency.	
	 * 
	 * Rootserver's are prevented from rebooting other swapserver's
	 * clients via the check of the swap_serving_cnode in the
	 * get_cnode_names() subroutine.
	 */
	if (rootserver || swapserver) {
#ifndef DEBUG
	    /*
	     * Before rebooting clients, kill the cluster boot server,
	     * so clients cannot reboot and new clients cannot boot later.
	     */
	    if (kill_process("rbootd") != 0) {
		fprintf(stderr, "WARNING:  Unable to kill remote boot daemon (/etc/rbootd).\n Continuing anyway...\n");
	    }

	    else if (debugLevel)
	      fprintf(debugStream, "%s: rbootd killed\n", hostname);

#endif /* not DEBUG */

	    /*
	     * Append the ampersand and stdout redirection to command line 
	     */
	    (void) strcat(reboot_cmd, CMD_MISC);

	    /*
	     * find all active clients and reboot the 
	     * appropriate ones
	     */
	    if (rootserver)
		num_active_clients = cnodes(active_clients);	
	    else 
		num_active_clients = swapclients(active_clients);	

	    if (debugLevel)
	      fprintf (debugStream, "%s: num_active_clients: %d\n", hostname,
		       num_active_clients);

	    /* 
	     * issue warning about reboot of clients
	     */
	    if (num_active_clients > 1) {
		/*
		 * Make sure you can get a socket for rcmd
		 */
		if (sock_port = getservbyname("shell","tcp")) {
		   /* 
		    * Got a legit port, go ahead and issue general warning
		    */
		    printf("The following client cnodes will also be rebooted:\n");
		    print_active_clients(stdout);
		}
		else
		{
		    /* 
		     * Got a NULL port pointer!!!  Issue "can't reboot" 
		     * warning and set num_active_clients to 0 to prevent
		     * rcmd attempt!
		     */
		     num_active_clients = 0;
		     
		     fprintf(stderr, "reboot: networking error: shell/tcp: Unknown service\nreboot: CAUTION!!!  Unable to reboot the following clients:\n");
		     print_active_clients(stderr);
		}
		printf("You may abort reboot via INTERRUPT in the next 5 seconds\n");
		sleep(5);	/* sleep so user may interrupt */
	    }

	    /*
	     * Ok, they've been warned -- do the actual client reboot
	     */
	    for (index = 0; index < num_active_clients; index++) {
#ifdef DEBUG
		printf("Cnode_id = %d\tName = %s\tdonot_reboot = %d\n", 
			active_clients[index], 
			cnode_array[active_clients[index]].name, 
			cnode_array[active_clients[index]].donot_reboot);
#endif /* DEBUG */
		/* 
		 * Don't reboot in the following cases:
		 *	- this is our site (don't reboot ourselves)
		 *	- cnode_array.names is NULL for this site
		 *	  (due to malloc() or getccent() error)
		 *	- we're not supposed to reboot this site because
		 *	  we're the root server and this cnode swaps to
		 *	  an auxiliary swap server (donot_reboot == TRUE)
		 */
		if (active_clients[index] == mysite ||
		    cnode_array[active_clients[index]].name == NULL || 
		    cnode_array[active_clients[index]].donot_reboot == TRUE)
		  {
		    if (debugLevel)
	              fprintf (debugStream, "%s: Not rebooting %s\n", hostname,
			       cnode_array [active_clients [index]].name);
		    continue;	
		  }
		/* 
		 * Execute reboot on the client cnode passing 
		 *   along all applicable options
		 */
	       if (debugLevel)
	         fprintf (debugStream, "%s: Rebooting %s\n", hostname,
			  cnode_array [active_clients [index]].name);
#ifdef DEBUG1
		printf("rcmd(%s)\n", reboot_cmd);
#endif /* DEBUG1 */
		ahost = cnode_array[active_clients[index]].name; 

/*  Note that we can only get here if sock_port isn't NULL.
*/

/*  We will set an alarm here just in case the rcmd never comes back.  */
/*  Rcmd might not come back if a cnode is hung;  reboot on the server */
/*  would subsequently hang.  Normally, rcmd comes back after 5-6      */
/*  seconds -- make sure the alarm time is big enough.                 */

		setalarm(MAX_WAIT_FOR_RCMD);
		if ((rcmd_socket = rcmd(&ahost, sock_port->s_port, ROOT, ROOT, 
				reboot_cmd, 0)) == -1)
		{
	            if (debugLevel)
	               fprintf (debugStream, "%s: Alarm call: Could not reboot %s\n", 
		       hostname, cnode_array [active_clients [index]].name);
		}
		alarm(0);
			
#ifdef DEBUG2
		/*
		 * Look at output from client reboot
		 */
		if (rcmd_socket == -1)
		    fprintf(stderr, "rcmd(%s) failed to host %s\n", reboot_cmd,
				cnode_array[active_clients[index]].name);
		else {
		    FILE *fp;
		    char ch;
		    printf("\nSTDOUT/ERR FROM rcmd(%s) ON %s:\n", reboot_cmd,
				cnode_array[active_clients[index]].name);
		    printf("===============================================");
		    printf("==========================\n");
		    fp = fdopen(rcmd_socket, "r");
		    while ((ch = getc(fp)) != EOF) 
			putchar(ch);
		    printf("================================================");
		    printf("=========================\n");
		}
#endif /* DEBUG2 */
		if (rcmd_socket != -1)
		    close(rcmd_socket); 	/* ignore stdout/err */

	    }	/* end of for each active client */
	}   /* end of if rootserver or swapserver */
       
       else if (debugLevel)
	 fprintf (debugStream, "%s: Not killing rbootd\n", hostname);

#endif /* LOCAL_DISK */

	nowtime = time((time_t *)0);
	sdt = getsdt(tbuf);
	i = 0;
	m = ((stogo = sdt - nowtime) + 30)/60;
	h = m/60; 
	m %= 60;
	ts = ctime(&sdt);
	printf("Shutdown at %5.5s (in ", ts+11);
	if (h > 0)
		printf("%d hour%s ", h, h != 1 ? "s" : "");
	printf("%d minute%s) ", m, m != 1 ? "s" : "");

#ifndef DEBUG

	/*  Set up HUP, QUIT, and INT to be SIG_IGN.  (mainAction.sa_handler
	 *  was set up previously to be SIG_IGN.)
	 */
	(void) sigaction(SIGHUP, &mainAction, (struct sigaction *) 0);
	(void) sigaction(SIGQUIT, &mainAction, (struct sigaction *) 0);
	(void) sigaction(SIGINT, &mainAction, (struct sigaction *) 0);
#endif
	/*  Set up TERM to be finish() and ALRM to be do_nothing.
	 */
 	mainAction.sa_handler = finish;
	(void) sigaction(SIGTERM, &mainAction, (struct sigaction *) 0);

 	mainAction.sa_handler = do_nothing;
	(void) sigaction(SIGALRM, &mainAction, (struct sigaction *) 0);

	fflush(stdout);
#ifndef DEBUG

	/*  
	 *  If we are not rebooting immediately, the program forks and the 
	 *  parent quits.  The child changes its process group so that 
	 *  subsequent signals do not get sent to it.  If the fork fails, the
	 *  parent prints an error message.
	 */
 	if (strcmp(tbuf, "now") != 0) {
 		if (i = fork()) {
			if (i == -1)
			  {
			    printf ("Couldn't fork\n");
			    perror ("reboot");
			  }

			else
 			  printf("[pid %d]\n", i);

 			exit(0);
 		} else {
			set_stderr();
			setpgrp();
		}
	}
#else
	putc('\n', stdout);
#endif
	sint = 1 HOURS;
	f = "";
	ufd = open("/etc/utmp",0);
	if (ufd < 0) {
		perror("reboot: /etc/utmp");
		exit(1);
	}
	first = 1;
	for (;;) {
		for (i = 0; stogo <= interval[i].stogo && interval[i].sint; i++)
			sint = interval[i].sint;
		if (stogo > 0 && (stogo-sint) < interval[i].stogo)
			sint = stogo - interval[i].stogo;
		if (sint >= stogo || sint == 0)
			f = "FINAL ";
		nowtime = time((time_t *) 0);
		lseek(ufd, 0L, 0);
		while (read(ufd,&utmp,sizeof utmp)==sizeof utmp)
		  if (utmp.ut_type == USER_PROCESS) {
			strcpy(term, tpath);
			strncat(term, utmp.ut_line, sizeof utmp.ut_line);
#ifdef DEBUG
			strcpy(term, "/dev/tty");
#endif
			strcpy(s, "\n\r\n");
			warn(sdt, nowtime, f);	/* NOTE: warn() appends to "s" */
			if ((first || sdt-nowtime > 1 MINUTES) && (mess != NULL)) {
				strcat(s, "\t... ");
				strcat(s, mess);
				strcat(s, "\n");
			}
			nbwrite(strlen(s));
		  }
		if (stogo <= 0) {
			printf("\n\007\007System shutdown time has arrived\007\007\n");
			if (nflag == 0)
				log_entry(sdt, flags&RB_HALT);

#ifndef DEBUG

#if	defined(DUX) || defined(DISKLESS)
#ifdef LOCAL_DISK
			/* 
			 * If running on the rootserver or on an auxiliary 
			 * swap server, wait for client cnodes to reboot
			 */
			if (swapserver || rootserver) {
			    if (debugLevel)
			      fprintf(debugStream, 
				       "%s: Waiting for clients to reboot\n",
				       hostname);

			    if (!clients_rebooted()) {
			        /* some clients didn't reboot */   
				fprintf(stderr, 
				  "\nreboot: CAUTION!!! The following clients could not be rebooted:\n");
				print_active_clients(stderr);
			    }

			}

			else if (debugLevel)
			  fprintf(debugStream, 
				   "%s: Not waiting for clients to reboot\n",
				   hostname);
#else /* LOCAL_DISK */
			/*
	 		 * If running on the root server of a cluster, 
			 * wait for other cluster nodes to reboot.
	 		 */
			if ((nflag == 0) 
			     && (cct_ent = getcccid(cnodeid())) != NULL) {
				if (cct_ent->cnode_type == 'r' 
					  &&  cnodes(0) > 1) {
					fprintf(stderr, "reboot: Waiting for cluster nodes to reboot...\n");
					for (retry = 20; 
					    cnodes(0) > 1 && retry > 0; --retry)
						sleep(6);
					if (retry <= 0)
						fprintf(stderr, "reboot: CAUTION: some cluster nodes wouldn't reboot\n");
				}	
			}
#endif /* LOCAL_DISK */
#endif	defined(DUX) || defined(DISKLESS)

			/*  
			 *  This prevents reboot from hanging on a write to 
			 *  stderr.  It also ensures that error messages are 
			 *  written to the console in the event reboot is 
			 *  executed remotely.
	 		 */
			if (notatty(fileno(stderr))) {
				fprintf(stderr, "reboot: redirecting error messages to %s\n", CONSOLE);
					set_stderr();
			}

			/* 
			 *  You don't want to kill all your other processes
			 *  if you are super user, this would be a ... 
			 * 
			 *  So set up HUP to be SIG_IGN.
			 */
			mainAction.sa_handler = SIG_IGN;
			(void) sigaction(SIGHUP, &mainAction, 
							(struct sigaction *) 0);
			if (!(flags & RB_NOSYNC)) {
				if (kill(1, SIGUSR1) == -1) {	/* idle init */
					fprintf(stderr, 
						"reboot: can't idle init\n");
					perror("reboot");
				} else
					sleep(2); /* give init time to react to signal*/

				/* terminate cooperating processes */

				/*  This does a kill on all processes since
				 *  KILL_ALL_OTHERS is -1.
				 */
				if (kill(KILL_ALL_OTHERS,SIGTERM)==-1) {
					if (errno != ESRCH)
						perror("reboot: kill");
				} else 	/* give it time to work */
					sleep(10);

#if	defined(DUX) || defined(DISKLESS)
				/* 
				 *  Terminate all cluster server processes
				 *  One limited knsp will still be running
				 */
				csp(NSP_CMD_ABS,0);
#endif	defined(DUX) || defined(DISKLESS)

#if defined(SecureWare) && defined(B1)
				if(ISB1)
				    reboot_shut_daemons();
#endif
				for (i = 1; ; i++) {
					if (kill(KILL_ALL_OTHERS,SIGKILL)==-1) {
						if (errno != ESRCH)
							perror("reboot: kill");
						break;
					}
#ifdef DEBUG
					system ("ps -e");
#endif
					if (i > 5) {
						fprintf(stderr,
							"reboot:  CAUTION: some process(es) wouldn't die\n");
						break;
					}
					setalarm(2 * i);
					pause();
				}
			}
			/* allow init to write tmp files and terminals */
			sleep(3);
#endif /* DEBUG */

#if defined(hp9000s300) && (defined(DUX) || defined(DISKLESS))
			if (reboot(flags, devname, filename, server_linkaddress) == -1)
#else
			if (reboot(flags, devname, filename) == -1)
#endif /* s300 DUX/DISKLESS */
			{
				perror("reboot");
				usage();
				kill(1, SIGUSR2); /* notify init to restart */
#ifndef DEBUG

/*  Previously, this code was used to start another shell in the event that
    the reboot (2) system call failed.  This presented a security hole since
    a normal user can now execute shutdown (1M) which is a setuid root program
    that calls reboot.  If the user gave an option to shutdown which was passed
    to reboot that caused reboot (2) to fail, this ordinary user would then 
    have a root shell.  

    This has been changed so that if the reboot (2) fails, the program exits.
    This means that the user will have to login again from the console.  Note
    that a SIGUSR2 has already been sent to init to start it up again, so this
    second kill (init, USR2) is redundant.

				execl("/bin/sh", "sh", 0);
				fprintf(stderr, 
					"Reboot:  Sorry, exec failed.  Bye!\n");
				perror("reboot");
				kill(1, SIGUSR2); 
*/
#endif /* DEBUG */
			}
			finish();
		}
		stogo = sdt - time((time_t *) 0);
		if (stogo > 0 && sint > 0)
			sleep(sint<stogo ? sint : stogo);
		stogo -= sint;
		first = 0;
	}
}

/**********************************************************************/
void
dingdong()
/**********************************************************************/
{
	/* RRRIIINNNGGG RRRIIINNNGGG */
}

/**********************************************************************/
void
setalarm(n)
/**********************************************************************/
{
	struct sigaction alrmAction;

	alrmAction.sa_handler = dingdong;
	(void) sigemptyset(&alrmAction.sa_mask);
	alrmAction.sa_flags = 0;

	(void) sigaction(SIGALRM, &alrmAction, (struct sigaction *) 0);
	alarm(n);
}

#ifdef DEBUG

/* when debug, use local reboot to check the return condition */

/**********************************************************************/
int
reboot(a, b, c, d)
int a;
char *b, *c, *d;
/**********************************************************************/
{
	printf("flags = %x, devname = %s, filename = %s\n", a, b, c);
#if defined(hp9000s300) && (defined(DUX) || defined(DISKLESS))
	if (a & RB_NEWSERVER)
		printf("link_address = %s\n", d);
#endif /* s300 DUX/DISKLESS */
	if(!strcmp("xxx",  b)) {
		return(EOF);
	}
	return(0);
}
#endif /* DEBUG */
	
/**********************************************************************/
time_t
getsdt(s)
	register char *s;
/**********************************************************************/
{
	time_t t, t1, tim;
	struct tm *lt;

	if (strcmp(s, "now") == 0)
		return(nowtime);
	if (*s == '+') {
		++s; 

		/*  If the next character isn't a digit, then this isn't a
		 *  proper number.  Otherwise, get the number from the
		 *  string.
		 */
		if (!isdigit(*s))
			goto badform;

		t = atol(s);

		if (t <= 0)
			t = 5;
		t *= 60;
		tim = time((time_t *) 0) + t;
		return(tim);
	}
	t = 0;
	if ( !isdigit(*s) )
		goto badform;
	while ( isdigit(*s) )
		t = t * 10 + *s++ - '0';
	if (t > 23 || *s != ':')
		goto badform;
	else
		s++;
	tim = t*60;
	t = 0;
	while (isdigit(*s))
		t = t * 10 + *s++ - '0';
	if (t > 59 || *s != '\0')
		goto badform;
	tim += t; 
	tim *= 60;
	t1 = time((time_t *) 0);
	lt = localtime(&t1);
	t = lt->tm_sec + lt->tm_min*60 + lt->tm_hour*3600;
	if ( tim < t )
		tim += 24 HOURS;
	if ( tim -t > 24 HOURS ) {
		/* more that 24 hours away */
		printf("Sorry, internal calculation wrong\n");
		finish();
	}
	return (t1 + tim - t);
badform:
	printf("Bad time format\n");
	finish();
}

/**********************************************************************/
void
warn(sdt, now, type)
	time_t sdt, now;
	char *type;
/**********************************************************************/
{
	char *ts;
	register delay = sdt - now;
	char t[80];

	if (delay > 8)
		while (delay % 5)
			delay++;

	if (shutter) 
		sprintf(t,
	    "\007\007\t*** %sSystem shutdown message from %s@%s ***\r\n\n",
		    type, shutter, hostname);
	else 
		sprintf(t,
		    "\007\007\t*** %sSystem shutdown message (%s) ***\r\n\n",
		    type, hostname);
	
	strcat(s,t);

	ts = ctime(&sdt);
	if (delay > 10 MINUTES) {
		sprintf(t, "System going down at %5.5s\r\n", ts+11);
#if defined(SecureWare) && defined(B1)
		if (ISB1)
                	audit_subsystem(t, (char *) 0, ET_BOOT_DOWN);
#endif
	} else if (delay > 95 SECONDS) {
		sprintf(t, "System going down in %d minute%s\r\n",
		    (delay+30)/60, (delay+30)/60 != 1 ? "s" : "");
#if defined(SecureWare) && defined(B1)
		if (ISB1)
                	audit_subsystem(t, (char *) 0, ET_BOOT_DOWN);
#endif
	} else if (delay > 0) {
		sprintf(t, "System going down in %d second%s\r\n",
		    delay, delay != 1 ? "s" : "");
#if defined(SecureWare) && defined(B1)
		if (ISB1)
                	audit_subsystem(t, (char *) 0, ET_BOOT_DOWN);
#endif
	} else {
		strcpy(t, "System going down IMMEDIATELY\r\n");
#if defined(SecureWare) && defined(B1)
		if (ISB1)
                	audit_subsystem(t, (char *) 0, ET_BOOT_DOWN);
#endif
	}

	strcat(s,t);
}

/**********************************************************************/
void
finish()
/**********************************************************************/
{
	struct sigaction termAction;

	termAction.sa_handler = SIG_IGN;
	(void) sigemptyset(&termAction.sa_mask);
	termAction.sa_flags = 0;

	(void) sigaction(SIGTERM, &termAction, (struct sigaction *) 0);
	exit(0);
}

/**********************************************************************/
void
do_nothing()
/**********************************************************************/
{
}

/*
 * make an entry in the reboot log
 */

char *days[] = {
	"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"
};

char *months[] = {
	"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep",
	"Oct", "Nov", "Dec"
};

/**********************************************************************/
void
log_entry(now, how)
	time_t now;
	int how;
/**********************************************************************/
{
	register FILE *fp;
	struct tm *tm, *localtime();

	tm = localtime(&now);
	fp = fopen(LOGFILE, "r+");
	if (fp == NULL)
		return;
	fseek(fp, 0L, 2);
#if defined(SecureWare) && defined(B1)
	if (ISB1) {
        	sprintf(auditbuf, "shutdown time: %02d:%02d  %s %s %2d, %4d",
                  tm->tm_hour,tm->tm_min, days[tm->tm_wday], months[tm->tm_mon],
                  tm->tm_mday, tm->tm_year + 1900);
        	audit_subsystem(auditbuf, "system shutdown at this time",
                  ET_BOOT_DOWN);
	}
#endif
	fprintf(fp, "%02d:%02d  %s %s %2d, %4d.", tm->tm_hour,
		tm->tm_min, days[tm->tm_wday], months[tm->tm_mon],
		tm->tm_mday, tm->tm_year + 1900);
	fprintf(fp, "  %s: %s", (how) ? "Halt" : "Reboot", mess);
	if (shutter)
		fprintf(fp, " (by %s!%s)", hostname, shutter);
	fputc('\n', fp);
	fclose(fp);
}

/*
 *  If reboot was called with a time delay, close stdin and stdout.
 *  Use the console for stderr output.
 */
/**********************************************************************/
void
set_stderr()
/**********************************************************************/
{
	if ( freopen(CONSOLE, "w", stderr) == NULL ) {
		freopen( "/dev/tty", "w", stderr );
	}
	fclose(stdin); 
	fclose(stdout);
}

/*
 *  This function is called when sending the "Shutdown" message
 *  to users on the system.   It guarantees non-blocking writes
 *  so reboot() won't hang.  This hang problem has been observed
 *  when writing to a pty of a defunct process
 *
 *  Note that this routine uses the globals term and s, which are
 *  set in main().
 */
/**********************************************************************/
void
nbwrite(len)
int len;
/**********************************************************************/
{
	int fd;
#ifdef hp9000s800
	int tmp;
#endif

#ifdef hp9000s800
	setalarm(3);
	if ( (fd = open(term, O_WRONLY)) == -1 ) {
		alarm(0);
		return ;
	}
	alarm(0);
	tmp=1;
	if (ioctl(fd,FIOSNBIO,&tmp) != -1) {
		write(fd, s, len);
		tmp=0;
		ioctl(fd,FIOSNBIO,&tmp);
		close(fd);
	}
#else
	/*  Non-S800 systems do not support non-blocking I/O on terminals.
	 *  To compensate, the write will be done by a child process.
	 *  Note that if the child process hangs, we won't be
	 *  able to cleanly reboot.  Better than reboot hanging...
	 */
	setalarm(5);
	if ( ((fd = open(term, O_WRONLY)) == -1) ||
		((write(fd, s, len) == -1) && (errno == EINTR)) )
			fprintf(stderr, "reboot: Couldn't write to /etc/utmp entry: %s\n", term);
	alarm(0);
	return;
#endif
}

/*
 *  Returns true if the file descriptor is attached to a pipe or a pty.
 */
/**********************************************************************/
int
notatty(fd)
int fd;
/**********************************************************************/
{
	struct stat sbuf;

	if ( isapty(fd) ||
	     (fstat(fd, &sbuf) == 0)  &&
	     ((sbuf.st_mode & S_IFMT) == S_IFIFO) )
		return 1;
	else
		return 0;
}


/* 
 *  NOTE: The function isapty() should be removed from here if and
 *  when isapty() becomes a libc routine.
 */

/* macros for isapty() */
#ifdef hp9000s500
#define SPTYMAJOR 29            /* major number for slave  pty's */
#define PTYSC     0xfe          /* select code for pty's         */
#define select_code(x) (((x) & 0xff0000) >> 16)
#endif
#if defined(hp9000s300) || defined(hp9000s800)
#define SPTYMAJOR 17            /* major number for slave  pty's */
#define PTYSC     0x00          /* select code for pty's         */
#define select_code(x) (((x) & 0xff0000) >> 16)
#endif

/**********************************************************************/
int
isapty(fd)
int fd;
/**********************************************************************/
{
    struct stat sbuf;

    if (  isatty(fd) &&
	  (fstat(fd, &sbuf) == 0) &&
	  ((sbuf.st_mode & S_IFMT) == S_IFCHR) &&
	  (major(sbuf.st_rdev) == SPTYMAJOR) &&
	  (select_code(sbuf.st_rdev) == PTYSC)    )
	return 1;		/* passed all tests */
    else
	return 0;		/* failed */
}

/**********************************************************************/
void
opt_err(options)
char *options;
/**********************************************************************/
{
	fprintf(stderr, "reboot: cannot use %s options together.\n", options);
	usage();
	exit(1);
}

/**********************************************************************/
void
usage()
/**********************************************************************/
{
	fprintf(stderr, "Usage: reboot [-h | -r] [-n | -s] [-q] [-t time] [-m message]\n");
#ifdef hp9000s300
	fprintf(stderr,"\t\t[-d device_name] [-f lif_filename]\n");
#endif
#if defined(hp9000s300) && (defined(DUX) || defined(DISKLESS))
	fprintf(stderr,"\t\t[-l server_linkaddress] [-b boot_server]\n");
#endif /* s300 DUX/DISKLESS */
}

#ifdef hp9000s300
/*  This routine will catch several potential errors regarding the device
 *  file.  Specifically, ENOTDIR (part of the path is not a directory),
 *  ENOENT (the file does not exist), EFAULT (the path points to an invalid
 *  address), and ENAMETOOLONG (part of the path is too long) (in addition
 *  to some other errors) are all caught by stat (2).  Any of these errors
 *  would cause the reboot (2) intrinsic to fail.
 */
/**********************************************************************/
void
check_dev(device)
char *device;
/**********************************************************************/
{
	struct stat	statbuf;
	if (stat(device, &statbuf) == EOF) {
		fprintf(stderr, "reboot: Can't stat boot device %s\n", device);
		exit(1);
	}
	
	if ((! S_ISCHR(statbuf.st_mode)) && (! S_ISBLK(statbuf.st_mode)))
	  {
		fprintf(stderr, "reboot: Not a character or block device %s\n",
			device);
		exit(1);
	  }
}
#endif

#if defined(hp9000s300) && (defined(DUX) || defined(DISKLESS))
/* find the server entry in the server_list file */

/**********************************************************************/
int
getbserver(server, address, dev)
char *server, *address, *dev;
/**********************************************************************/
{
	FILE *sfile;
	char sys[32], *comment, garbage[BUFSIZ];
	char line[BUFSIZ];
	int count;

#ifdef DEBUG
	fprintf(stderr, "reboot, getbserver() DEBUG:\n");
	fprintf(stderr, "looking for %s\n", server);
#endif
	if ((sfile=fopen(server_list, "r")) == NULL) return -1;
	while (fgets(line, BUFSIZ, sfile) != NULL) {
		if ((comment = strchr(line, '#')) != NULL) {
			*comment = '\0';
			comment++;
		}
		count=sscanf(line, "%s %s %s %s", sys, address, dev, garbage);
#ifdef DEBUG
		fprintf(stderr, "\tsys: %s, addr: %s, dev: %s\n",
			sys, address, dev);
		fprintf(stderr, "\tcomment: %s\n", comment);
		fprintf(stderr, "\tgarbage: %s\n", garbage);
#endif
		if (count <= 0)
			continue;
		if (strcmp(server, sys) == 0) {
			if (count < 2 || count > 3)
				count = 1;
			break;
		}
	}
	fclose(sfile);
#ifdef DEBUG
	fprintf(stderr, "\treturn'ing %d.\n", count);
#endif
	return count;	
}
#endif /* s300 DUX/DISKLESS */
