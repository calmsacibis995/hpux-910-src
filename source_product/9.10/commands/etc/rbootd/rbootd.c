/* HPUX_ID: @(#) $Revision: 70.4 $  */
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <time.h>
#include <unistd.h>
#include <sys/netio.h>
#include <sys/param.h>
#include <sys/stat.h>
#include <sys/sysmacros.h>
#include <sys/types.h>
#include <sys/utsname.h>
#include <dirent.h>
#include "rbootd.h"
#include "rmp_proto.h"

int lanfds[MAXLANDEVS];
char *lanfds_name[MAXLANDEVS];
int nlandevs;
FILE *errorfile;

/*
 * These are so we don't get multiple copies of strings.
 */
char DEFAULTDEVICE[]	= "/dev/ieee";
char ERRLOGFILE[]	= "/usr/adm/rbootd.log";
char MALLOC_FAILURE[]	= "Malloc failure.\n";
#ifdef DTC
char DTCTYPE[]		= "HP234";
char DTCMACHINETYPE[]	= "HP2345B";
char DTC16TYPE[]	= "HP2340A";
char DTCMGRDIR[]	= "/usr/dtcmgr/";
char DTC_CONFIG_FILE[]	= "/usr/dtcmgr/map802";
char IPC_NMDRBOOTD[]	= "/usr/dtcmgr/ipc/rbootd";

int dtclanfds[MAXLANDEVS];
#endif /* DTC */

extern int errno;
extern int loglevel;
struct session  *session_blk;

/*
 * MAXSESSIONS defines the maximum number of open sessions allowed.
 * Since each session may use up one file descriptor, and each LAN
 * device also uses one file descriptor (plus one for error logging),
 * MAXSESSIONS should not be greater than (_NFILE - (MAXLANDEVS + 1)).
 */
int MAXSESSIONS;

boot_request *packet_pool;
struct pinfo *packetinfo_pool;

/*
 * forward declarations
 */
#ifdef DTC
void setupdtclan();				/* LAN routine   */
#endif /* DTC */
void setuplan();				/* LAN routine	 */
void sendpacket();				/* LAN routine	 */
void doio();					/* LAN routine	 */
int getpacket();				/* LAN routines  */
void send_boot_reply();				/* send routines */
static char *cluster_lan();			/* diskless lan  */

/*
 * terminate -- signal handler -- dies on SIGTERM OR SIALRM
 */
void
terminate(sig)
int sig;
{
    if (sig != SIGTERM)
	log(EL0, "Received signal %d\n", sig);

    log(EL0, "TERMINATING\n");
    exit(0);
}

/*
 * main body of boot server:
 *     set up signal handling
 *     process arguments
 *     set up error logging
 *     fork to go into background
 *     set up file descriptors
 *     open lan devices
 *     change cwd to "/"
 *     get hostname
 *     initialize session block
 *     build client list
 *     select on lan devices
 *     dorequest() (handle incoming packets)
 */
main(argc, argv)
    int argc;
    char *argv[];
{
    char hostname[MAXHOSTNAMELEN]; /* hostname of server */
    register int i;
    int ret;
    int lanfd;
    int nullfd;
#ifdef DEBUG
    int forkflag = TRUE;
#endif
    char *logfile = ERRLOGFILE;

    char *logfilemode;
    char *cluster_lan_name;
    extern char *optarg;
    extern int optind;
    extern int opterr;

    /*
     * set up signal handling.
     * We ignore hangup, interrupt, pipe and quit.
     * We catch SIGTERM so that we gracefully terminate.
     */
    (void)signal(SIGHUP,  SIG_IGN);
    (void)signal(SIGINT,  SIG_IGN);
    (void)signal(SIGQUIT, SIG_IGN);
    (void)signal(SIGPIPE, SIG_IGN);
    (void)signal(SIGTERM, terminate);

    /*
     * Figure out how many file descriptors we may have open at once.
     * We allow 1 extra for the log file, plus all of the ones that we
     * have open right now.
     */
#ifdef GT_64_FDS
    MAXSESSIONS = sysconf(_SC_OPEN_MAX) - getnumfds() - 1;
#else
    MAXSESSIONS = _NFILE - 3 - 1;
#endif

    /*
     * process arguments
     */
    opterr = 0;
    logfilemode = "w";
#ifdef DEBUG
    while ((ret = getopt(argc, argv, "adl:L:")) != EOF)
#else
    while ((ret = getopt(argc, argv, "al:L:")) != EOF)
#endif
    {
	switch(ret)
	{
	case 'a':
	    logfilemode = "a+";
	    break;
#ifdef DEBUG
	case 'd':
	    forkflag = FALSE;
	    loglevel = EL5;
	    (void)signal(SIGINT,  SIG_DFL);
	    (void)signal(SIGQUIT, SIG_DFL);
	    break;
#endif /* DEBUG */

	case 'l':
	    i = atoi(optarg);
#ifdef DEBUG
	    if (i < EL0 || i > EL9 || *optarg < '0' || *optarg > '9')
#else
	    if (i < EL0 || i > EL3 || *optarg < '0' || *optarg > '9')
#endif
	    {
		fprintf(stderr,
		    "rbootd: illegal argument for -l option.\n");
		exit(1);
	    }

	    loglevel = i;
	    break;

	case 'L':
	    logfile = optarg;
	    break;

	default:
	    fputs("Usage: rbootd [-a] [-l <logging level>] [-L <logfile>] [<landevice>]\n", stderr);
	    exit(1);
	}
    }

    if (argc - optind > MAXLANDEVS)
    {
	fprintf(stderr,
	    "rbootd: too many lan devices (maximum allowed is %d)\n",
	    MAXLANDEVS);
	exit(1);
    }

    /*
     * Open error logging file.  Don't buffer output to the log
     * file.
     */
    if ((errorfile = fopen(logfile, logfilemode)) == (FILE *)0)
    {
	fprintf(stderr, "Could not open error logging file %s\".\n",
	    logfile);
	exit(1);
    }
    (void)setbuf(errorfile, (char *)0);

    /*
     * Open /dev/null
     */
    if ((nullfd = open("/dev/null", O_RDWR)) < 0)
	errexit("Could not open /dev/null.\n");

    /*
     * Fork so we put ourselves in background
     */
#ifdef DEBUG
    if (forkflag == TRUE)
#endif
    {
	if ((ret = fork()) == -1)
	    errexit("Could not fork.\n");

	/* Exit if we are parent */

	if (ret != 0)
	    _exit(0);
    }

    log(EL0, "STARTUP\n");

    /*
     * Now close all file descriptors except our nullfd and error
     * file.
     * We then dup the null file to stdin, stdout and stderr.
     */
#ifdef GT_64_FDS
    for (i=getnumfds(); i >= 0; i--)
#else
    for (i=_NFILE; i >= 0; i--)
#endif
	if (i != nullfd && i != fileno(errorfile))
	    (void)close(i);

    for (i = 0; i < 3; i++)
	if (fcntl(nullfd, F_DUPFD, i) != i)
	    errexit("File descriptor manipulation error.\n");

    (void)close(nullfd);

    /*
     * Make ourselves a process group leader
     */
    (void)setpgrp();

    /*
     * Open LAN devices, set up SSAPs, etc...
     */
    nlandevs = 0;
    while (optind < argc)
    {
	if ((lanfd = open(argv[optind], O_RDWR|O_NDELAY)) < 0)
	    errexit("Could not open lan device %s\n", argv[optind]);

	setuplan(lanfd);
	lanfds_name[nlandevs] = argv[optind];
	lanfds[nlandevs++] = lanfd;
	optind++;

	/*
	 * Since we just used up a file descriptor for a lan device,
	 * we can support one less simultaneous session than before.
	 */
	MAXSESSIONS--;
    }

    /*
     * Ensure that we open the diskless lan device (if not already
     * open).  If we aren't diskless, we make sure that we have
     * opened at least one device (DEFAULTDEVICE).
     */
    if ((cluster_lan_name = cluster_lan()) != (char *)0)
    {
	if ((lanfd = open(cluster_lan_name, O_RDWR|O_NDELAY)) < 0)
	    errexit("Could not open lan device %s\n", cluster_lan_name);

	setuplan(lanfd);
	lanfds_name[nlandevs] = cluster_lan_name;
	lanfds[nlandevs++] = lanfd;

	/*
	 * Since we just used up a file descriptor for a lan device,
	 * we can support one less simultaneous session than before.
	 */
	MAXSESSIONS--;
    }

#ifdef DTC
{
    struct stat statb;

    if (stat(DTCMGRDIR, &statb) != -1)
    {
	/*
	 * Another set of file descriptors are got as for the DTC case,
	 * we need to set dxsap and sxsap to be the same as the DTC
	 * accepts only packets which have dxsap=sxsap. When sending a
	 * packet to a DTC the write is performed on file descriptor
	 * "dtclanfd".
	 */
	int dtclanfd;
	int i;

	/*
	 * The DTC code needs a spare file descriptor when setting up a
	 * session.  This is only used temporarily, so we only need one
	 * extra.  Decrement MAXSESSIONS so that we will always have a
	 * spare file descriptor available.
	 */
	MAXSESSIONS--;

	/*
	 * Open LAN devices, set up XSSAP=XDSAP by calling
	 * setupdtclan().
	 */
	for (i = 0; i < nlandevs; i++)
	{
	    if ((dtclanfd = open(lanfds_name[i], O_RDWR|O_NDELAY)) < 0)
		errexit("Could not open lan device %s (DTC)\n",
		    lanfds_name[i]);

	    setupdtclan(dtclanfd);
	    dtclanfds[i] = dtclanfd;

	    /*
	     * Since we just used up a file descriptor for a lan device,
	     * we can support one less simultaneous session than before.
	     */
	    MAXSESSIONS--;
	}
    }
}
#endif /* DTC */

    /*
     * Change directory to "/" so we know where we are (in case we
     * core  dump!).
     * We do this *after* we have opened any files that were specified
     * on the command line.
     */
    if (chdir("/") != 0)
	 errexit("Could not change directory to /.\n");

    /*
     * Get hostname for probe packets.  Since the hostname can be
     * kind-of long, we just use the first part of it (i.e. we change
     * "foo.hp.com" to "foo".  If "foo" is too long to display, it is
     * up to the client to handle it, so we don't worry about that.
     */
    hostname[0] = '\0';
    if (gethostname(hostname, sizeof(hostname)) < 0)
	errexit("Can't get hostname.");
    else
    {
	extern char *strchr();
	char *s;

	if ((s = strchr(hostname, '.')) != NULL)
	    *s = '\0';
    }

    if (hostname[0] == '\0' || strcmp(hostname, "unknown") == 0)
	errexit("Hostname not set.");

    /*
     * Now that we know the right value for MAXSESSIONS, allocate the
     * various data structures that are sized by it.
     */
    session_blk =
	(struct session *)calloc(sizeof(struct session), MAXSESSIONS+1);
    if ((int)session_blk == 0)
	errexit(MALLOC_FAILURE);

    packet_pool =
	(boot_request *)calloc(sizeof(boot_request), MAXSESSIONS);
    if ((int)packet_pool == 0)
	errexit(MALLOC_FAILURE);

    packetinfo_pool =
	(struct pinfo *)calloc(sizeof(struct pinfo), MAXSESSIONS);
    if ((int)packetinfo_pool == 0)
	errexit(MALLOC_FAILURE);

    /*
     * Set up client and session block data structures
     * (We don't use location zero, since sessions are numbered 1..n)
     */
    for (i=1; i <= MAXSESSIONS; i++)
    {
	session_blk[i].client = NULL;
	session_blk[i].bfd = -1;
    }

    config();

    log(EL0, "INITIALIZATION COMPLETE\n");

    doio(hostname);

    /* NOTREACHED */
}

/*
 * doio: handles all incoming packets.
 *
 *      read packet (getpacket()) from link level
 *      switch on packet type (sid & seqno) to
 *		rcv_server_identify
 *		rcv_filelist_request
 *		rcv_boot_request
 *		rcv_read_request
 *		rcv_boot_complete
 *		rcv_indication_request (DTC case)
 */
void
doio(hostname)
char *hostname;
{
    register int i;
    boot_request reqpkt;
    struct timeval stimeout;
    struct timeval *stimeptr;
    long seqno;
    int lanfd;
    char src[ADDRSIZE];	  /* source link address (internal)	      */
    int nfds;		  /* file descriptor counter for select mask  */
    int lanfdmask;	  /* Mask indicates open lan file descriptors */
    int readfds;	  /* read mask for select		      */

    /*
     * Initialize stimeout to 0 for pollselect
     */
    stimeout.tv_sec = 0;
    stimeout.tv_usec = 0;

    /*
     * Compute nfds and lanfdmask
     */
    nfds = 0;
    lanfdmask = 0;
    for (i = 0; i < nlandevs; i++)
    {
	lanfd = lanfds[i];
	lanfdmask |= (1 << lanfd);
	if (lanfd >= nfds)
	    nfds = lanfd + 1;
    }

    /*
     * Wait for request to come in on LAN device
     */
    for (;;)
    {
#ifdef DEBUG
	log(EL5, "Waiting for a packet\n");
#endif

	/*
	 * set select timeout to infinite or zero depending on
	 * whether or not the packet queue is empty.
	 */
	if (qempty())
	    stimeptr = (struct timeval *)0; /* block indefinitely */
	else
	    stimeptr = &stimeout;	    /* non-blocking       */

	readfds = lanfdmask;
	if (select(nfds, &readfds, (int *)0, (int *)0, stimeptr) < 0)
	{
	    if (errno == EINTR)
		continue;
	    else
	    {
		errexit("Select failed.\n");
		exit(1);
	    }
	}

	if (readfds != 0)
	{
	    /*
	     * Find out which lan devices have data
	     */
	    for (i = 0; i < nlandevs; i++)
	    {
		lanfd = lanfds[i];
		if ((readfds & (1 << lanfd)) != 0)
		{
		    /*
		     * buffer up all packets found on lan device
		     */
		    readpackets(lanfd, lanfds_name[i]);
		}
	    }
	}

#ifdef DEBUG
	log(EL5, "Calling getpacket\n");
#endif

	/*
	 * pop a packet off of the packet queue
	 */
	if ((lanfd = getpacket(&reqpkt, src, sizeof reqpkt)) < 0)
	    continue;

#ifdef DEBUG
	log(EL5, "Successful return from getpacket\n");
#endif

	/*
	 * Got a good packet from getpacket:  Switch on type to
	 * handle each kind
	 */
	switch (reqpkt.type)
	{
	case BOOT_REQUEST:
	    memcpy((char *)&seqno, reqpkt.seqno, 4);
	    if (reqpkt.sid == PROBESID)
	    {
		if (seqno == 0)
		    rcv_server_identify(lanfd, src, &reqpkt, hostname);
		else
		    rcv_filelist_request(lanfd, src, seqno, &reqpkt);
	    }
	    else
		rcv_boot_request(lanfd, src, seqno, &reqpkt);
	    break;
	case READ_REQUEST:
	    rcv_read_request(lanfd, &reqpkt, src);
	    break;
	case BOOT_COMPLETE:
	    rcv_boot_complete(&reqpkt, src);
	    break;
#ifdef DTC
	case INDICATION_REQUEST:
	    rcv_indication_request(lanfd, &reqpkt, src);
	    break;
#endif /* DTC */
	}
    }
}

/*
 * cluster_lan() --
 *     return a name of a device file in /dev that matches the lan
 *     device used for diskless communication.
 *     Returns DEFAULTDEVICE if this isn't a diskless node (server
 *     or client).
 *     If this is a diskless node, and no matching device file can be
 *     found, an error message is printed to the logfile (at EL1).
 *     DEFAULTDEVICE is returned in this case as well.
 *
 *     If one of the lan file descriptors in the lanfds[] array
 *     already references the lan device for diskless communication,
 *     NULL is returned.
 */
static char *
cluster_lan()
{
    cnode_t me = cnodeid();
    DIR *dir;
    struct dirent *dp;
    int i;
    struct cct_entry *cnode;
    struct fis req;
    char pbuf[MAXNAMLEN + 6];

    /*
     * If we aren't a member of a cluster, we see if there were any
     * devices specified on the command line.  If there were, we
     * return NULL.  If no lan devices were specified on the command
     * line, we return DEFAULTDEVICE.
     */
    if (me <= 0 || (cnode = getcccid(me)) == (struct cct_entry *)0)
    {
	if (nlandevs == 0)
	{
	    log(EL3, "using default lan device %s\n", DEFAULTDEVICE);
	    return DEFAULTDEVICE;
	}
	return (char *)0;
    }

    /*
     * We are a member of a cluster.  First, search lanfds[] for a
     * lan card that matches our diskless link level address.
     */
    for (i = 0; i < nlandevs; i++)
    {
	req.reqtype = LOCAL_ADDRESS;
	req.vtype   = sizeof req.value.s;

	if (ioctl(lanfds[i], NETSTAT, &req) == -1)
	    continue;

	if (memcmp(cnode->machine_id, req.value.s, M_IDLEN) == 0)
	{
	    /*
	     * found a match
	     */
	    log(EL3, "specified device %s matches the diskless lan\n",
		lanfds_name[i]);
	    return (char *)0;
	}
    }

    /*
     * Drat, didn't find a match.  Now we have to do it the hard
     * way.  We search /dev (top level only) for device files with
     * the right major number and minor number (if applicable) for an
     * "ieee" lan device.
     *
     * For each ieee lan device we find, we open the device and get
     * its link level address.  If we find a match, we return the
     * name of that device file (in a malloc-ed buffer).
     */
    if ((dir = opendir("/dev")) == (DIR *)0)
    {
	log(EL1, "can't open /dev!  Using default device %s\n",
	    DEFAULTDEVICE);
	return DEFAULTDEVICE;
    }

    strcpy(pbuf, "/dev/");
    while ((dp = readdir(dir)) != (struct dirent *)0)
    {
	struct stat st;

	/*
	 * Ignore "." and "..", what we are looking for can't be
	 * one of these.
	 */
	if (dp->d_name[0] == '.' &&
	    (dp->d_name[1] == '\0' || (dp->d_name[1] == '.' &&
				      dp->d_name[2] == '\0')))
	    continue;

	/*
	 * Build the full path name to this file.
	 */
	strncpy(pbuf+5, dp->d_name, dp->d_namlen);
	pbuf[dp->d_namlen + 5] = '\0';

	/*
	 * Only look at character special device files.
	 */
	if (stat(pbuf, &st) == -1 || !S_ISCHR(st.st_mode))
	    continue;

	/*
	 * Only look at devices with the right major number.
	 */
#ifdef __hp9000s300
	/*
	 * The s300 has major 18 for the ieee lan device.  Major 19
	 * is the ether lan device.
	 */
	if (major(st.st_rdev) == 18)
#else
	/*
	 * PA-RISC machines use:
	 *    major 50 -- CIO  (s800)
	 *    major 51 -- NIO  (s800)
	 *    major 52 -- WSIO (s700)
	 * The same major number is used for both ieee and ether
	 * lan devices.  The low-order bit in the minor number is
	 * 0 for ieee, 1 for ether.
	 */
	if (major(st.st_rdev) >= 50 && major(st.st_rdev) <= 52 &&
	    (minor(st.st_rdev) & 0x01) == 0)
#endif /* s700 or s800 */
	{
	    int fd;

	    /*
	     * This device is a candidate.  Open it and see if its
	     * link-level address is the one we want.
	     *
	     * NOTE: we really only need to open this O_RDONLY, but
	     *       the caller will want to open it O_RDWR, so we
	     *       open it that way too (so we skip files for which
	     *       we don't have write permission).
	     */
	    if ((fd = open(pbuf, O_RDWR)) == -1)
		continue;

	    req.reqtype = LOCAL_ADDRESS;
	    req.vtype   = sizeof req.value.s;
	    if (ioctl(fd, NETSTAT, &req) == -1)
	    {
		close(fd);
		continue;
	    }
	    close(fd);

	    if (memcmp(cnode->machine_id, req.value.s, M_IDLEN) == 0)
	    {
		char *s = (char *)malloc(strlen(pbuf) + 1);

		/*
		 * Found a match.
		 */
		if (s == (char *)0)
		    errexit(MALLOC_FAILURE);

		closedir(dir);
		strcpy(s, pbuf);
		log(EL3, "using %s for diskless boot requests\n", s);
		return s;
	    }
	}
    }
    closedir(dir);

    log(EL1, "didn't find a lan device for diskless boot requests!\n");
    return DEFAULTDEVICE;
}
