/*
**	@(#)check_exit.c	$Revision: 1.20.109.1 $	$Date: 91/11/19 14:10:34 $  
**	check_exit.c	--	check if rpc.daemons should exit or live
**	contains functions:
**		check_exit()	--	checks Exitopt and exits or sets alarm
**		alarmer()	--	handles SIGALRM, checks with portmap
****
**	globals:
**	    Exitopt	--	sleep interval, 0 to cause immediate exit
**	    My_prog	--	the program number registered with portmap
**	    My_vers	--	a version number registered with portmap
**	    My_prot	--	the protocol registered with portmap
**	    My_port	--	the port number I am listening upon
*/

#ifndef NLS
#define catgets(i, sn,mn,s) (s)
#else NLS
#define NL_SETN 1	/* set number */
#include <nl_types.h>
#endif NLS

#include <stdio.h>
#include <sys/types.h>
#include <rpc/rpc.h>
#include <sys/socket.h>
#include <signal.h>

#ifdef	TRACEON
/*
**	make sure we get the LIBTRACE code, since we're going to be
**	compiled in with another module!
*/
#undef		TRACEON
#define	LIBTRACE
#endif	TRACEON
extern int errno;
#include <arpa/trace.h>

/*
**	we will always sleep (at least) this long between alarms
*/
#define	MIN_INTERVAL	600

/*
**	these variables all come from the module we link with; they
**	are used by check_exit and alarmer.  The value of Exitopt
**	is especially important, since it controls whether we will
**	exit immediately or check with the portmapper every so often
*/
extern	long	Exitopt;
extern	int	My_prog, My_vers, My_prot, My_port;
extern	char	LogFile[];

/*
**	getopt options
*/
extern	char	*optarg;


/*
**	check_exit()	--	if opt is zero, exit now, else set alarm
*/
void
check_exit()
{
    TRACE2("check_exit SOP, Exitopt = %ld", Exitopt);
    if (Exitopt == 0)
	exit(0);
    else
	(void) alarm(Exitopt);
    TRACE("check_exit returns, alarm is set");
}



/*
**	alarmer()	--	handle SIGALRM, check with the portmapper
**		checks to see if PROG,VERS registered with portmapper is
**		listening on My_port -- if not I've been remapped, so die
*/
alarmer(sig)
int	sig;
{
    u_short	port;
    struct sockaddr_in	myaddr;
    /*
    */
    TRACE("alarmer SOP");
    get_myaddress(&myaddr);
	/* 8.0 change */
	/* 
	 * myaddr.sin_family = AF_INET;
	 * myaddr.sin_addr.s_addr = INADDR_LOOPBACK;
	 */	
    TRACE2("alarmer myaddr.sin_addr = 0x%lx", myaddr.sin_addr);
    port = pmap_getport(&myaddr, My_prog, My_vers, My_prot);
    TRACE3("alarmer pmap_getport = %d, My_port = %d", port, My_port);
    if (port != My_port) {
	TRACE("alarmer found ports not the same exit!");
	exit(1);
    } else if (Exitopt > 0) {
	if (Exitopt < MIN_INTERVAL)
	    Exitopt = MIN_INTERVAL;
	TRACE2("alarmer same ports, setting alarm for %ld", Exitopt);
	(void) signal(SIGALRM, alarmer);
	(void) alarm(Exitopt);
    }
}



/*
**	argparse()	--	parse command line options for daemon
*/
void
argparse(argc, argv)
int	argc;
char	**argv;
{
    int c;
    char *invo_name = *argv;
#ifdef NLS
    nl_catd nlmsg_fd;
    nl_init(getenv("LANG"));
    nlmsg_fd = catopen("check_exit",0);
#endif NLS
    /*
    **	parse all arguments passed in to the function and set the
    **	Exitopt and LogFile variables as required
    */
    while ((c=getopt(argc, argv, "enl:")) != EOF) {
	TRACE2("argparse getopt returned option %c", c);
	switch(c) {
	    case 'e':
		    /*
		    **	exit after running once -- Exitopt == 0
		    */
		    Exitopt = 0l;
		    TRACE3("argparse option %c, set Exitopt = %d", c, Exitopt);
		    break;
	    case 'n':
		    /*
		    **	default is to check with portmapper every hour
		    */
		    Exitopt = MIN_INTERVAL;
		    TRACE3("argparse option %c, set Exitopt = %d", c, Exitopt);
		    break;
	    case 'l':
		    /*
		    **	log file name ...
		    */
		    strcpy(LogFile, optarg);
		    break;
	    default:
		    TRACE2("argparse option %c is illegal", c);
		    fprintf(stderr,(catgets(nlmsg_fd,NL_SETN,1, "%s: illegal option\n")), invo_name);
		    break;
	}
    }
    /*
    **	set up the signal handler and set an alarm; note that if
    **	Exitopt is zero, this will just turn off the alarm ...
    */
    (void) signal(SIGALRM, alarmer);
    (void) alarm(Exitopt);
    TRACE2("argparse set alarm for %d seconds", Exitopt);
}
