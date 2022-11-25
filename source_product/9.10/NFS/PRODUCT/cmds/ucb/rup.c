#ifndef lint
static  char rcsid[] = "@(#)rup:	$Revision: 1.28.109.2 $	$Date: 92/01/09 14:41:01 $  ";
#endif
/* NFSSRC rup.c	2.1 86/04/16 */
/*static  char sccsid[] = "rup.c 1.1 86/02/05 Copyr 1985 Sun Micro";*/

/*
 * Copyright (c) 1985 by Sun Microsystems, Inc.
 */

/*	HPNFS
**	include types.h and define the FSHIFT and FSCALE macros found
**	on Sun (in sys/types?)
*/
#ifndef NLS
#define catgets(i, sn,mn,s) (s)
#else NLS
#define NL_SETN 1	/* set number */
#include <nl_types.h>
#endif NLS

#include <sys/types.h>
#define FSHIFT	8
#define FSCALE	(1<<FSHIFT)

#include <stdio.h>
#include <netdb.h>
#include <sys/param.h>
#include <sys/stat.h>
#include <time.h>
#include <sys/socket.h>
#include <rpcsvc/rstat.h>
#include <rpc/rpc.h>
#include <rpc/pmap_clnt.h>

/*
**	include the tracing macros ...
*/
#include <arpa/trace.h>
# define	TRACEFILE	"/tmp/rup.trace"

#define MACHINELEN 12		/* length of machine name printed out */
#define AVENSIZE (3*sizeof(long))

int machinecmp();
int loadcmp();
int uptimecmp();
int collectnames();

struct entry {
	int addr;
	char *machine;
	struct timeval boottime;
	time_t curtime;
	long avenrun[3];
} entry[200];

unsigned curentry;		/* number of elements in the entry array */
int vers;			/* which version did the broadcasting */
int lflag;			/* load: sort by load average */
int tflag;			/* time: sort by uptime average */
int hflag;			/* host: sort by machine name */
int dflag;			/* debug: list only first n machines */
int debug;
int errors = 0;			/* keep a count of how many errors we had */
char tempbuf[100];

#ifdef NLS
nl_catd nlmsg_fd;
#endif NLS
extern char **environ;

main(argc, argv)
	char **argv;
{
	struct statstime sw;
	int err;
	int single;
	enum clnt_stat clnt_stat;

	cleanenv(&environ, "LANG", "LANGOPTS", "NLSPATH", "TZ", 0);

#ifdef NLS
	nl_init(getenv("LANG"));
	nlmsg_fd = catopen("rup",0);
#endif NLS
	STARTTRACE(TRACEFILE);
	single = 0;
	/*	HPNFS	jad	87.07.08
	**	sethostent(1) will keep the hosts file open (more efficient)
	*/
	sethostent(1);
	while (argc > 1) {
		if (argv[1][0] != '-') {
			single++;
			TRACE3("main single=%d, argv=%s",single,argv[1]);
			errors += singlehost(argv[1]);
		}
		else {
			switch(argv[1][1]) {
	
			case 'l':
				lflag++;
				TRACE2("main lflag = %d", lflag);
				break;
			case 't':
				tflag++;
				TRACE2("main tflag = %d", tflag);
				break;
			case 'h':
				hflag++;
				TRACE2("main hflag = %d", hflag);
				break;
			case 'd':
				dflag++;
				TRACE2("main dflag = %d", dflag);
				if (argc < 3)
					usage();
				debug = atoi(argv[2]);
				TRACE2("main debug = %d", debug);
				argc--;
				argv++;
				break;
			default:
				usage();
			}
		}
		argv++;
		argc--;
	}
	TRACE2("main out of while(argc), exit if %d=0", single);
	/*	HPNFS	jad	87.07.08
	**	added support for using options along with host names --
	**	if we gave a flag as well as host names, then go to print
	**	the data which has already been collected; if we just gave
	**	a flag and no host names, then broadcast the request; else
	**	if we only gave host names then exit now ... data has been
	**	printed by singlehost already!
	*/
	if (hflag || tflag || lflag) {
		if (single > 0) {
			printnames();
			exit(errors);
		}
		printf((catgets(nlmsg_fd,NL_SETN,1, "collecting responses... ")));
		fflush(stdout);
	} else
		if (single > 0)
			exit(errors);
	/*
	**	do an RPC broadcast to collect TIME statistics from
	**	all active RPC hosts on the network.
	*/
	vers = RSTATVERS_TIME;
	TRACE("main about to clnt_broadcast(RSTAT TIME)");
	clnt_stat = clnt_broadcast(RSTATPROG, RSTATVERS_TIME, RSTATPROC_STATS,
	    xdr_void, NULL, xdr_statstime,  &sw, collectnames);
	/*
	**	collect TIME statistics, then RPC broadcast to collect
	**	SWTCH statistics from active RPC hosts ...
	*/
	vers = RSTATVERS_SWTCH;
	TRACE("main about to clnt_broadcast(RSTAT SWTCH)");
	clnt_stat = clnt_broadcast(RSTATPROG, RSTATVERS_SWTCH, RSTATPROC_STATS,
	    xdr_void, NULL, xdr_statsswtch,  &sw, collectnames);
	/*
	**	collect SWTCH stastics, then print out the info ...
	*/
	if (hflag || tflag || lflag)
		printnames();
	exit(0);
}

singlehost(host)
	char *host;
{
	enum clnt_stat err;
	struct statstime sw;
	time_t now;
	size_t hostlen;
	struct hostent *hp;
	char *dot_ptr;
	char my_name[255];
	char *my_domain;

    
	TRACE("singlehost SOP, about to callrpc TIME");
	err = (enum clnt_stat)callrpc(host, RSTATPROG, RSTATVERS_TIME,
	    RSTATPROC_STATS, xdr_void, 0, xdr_statstime, &sw);
	TRACE2("singlehost callrpc returns %d", err);
	if (err == RPC_SUCCESS)
		now = sw.curtime.tv_sec;
	else if (err == RPC_PROGVERSMISMATCH) {
		TRACE("singlehost PROGMISMATCH, try SWTCH");
		if (err = (enum clnt_stat)callrpc(host, RSTATPROG,
		    RSTATVERS_SWTCH, RSTATPROC_STATS, xdr_void, 0,
		    xdr_statsswtch, &sw)) {
			TRACE2("singlehost callrpc returned %d", err);
			hostlen = strlen(host);
			fprintf(stderr,
			    (catgets(nlmsg_fd,NL_SETN,2, "%*.*s: ")), hostlen, hostlen, host);
			clnt_perrno(err);
			fprintf(stderr, (catgets(nlmsg_fd,NL_SETN,3, "\n")));
			return(1);
		}
		time (&now);
	} else {
		TRACE("singlehost aborting on error");
		hostlen = strlen(host);
		fprintf(stderr, (catgets(nlmsg_fd,NL_SETN,4, "%*.*s: ")), hostlen, hostlen, host);
		clnt_perrno(err);
		fprintf(stderr, (catgets(nlmsg_fd,NL_SETN,5, "\n")));
		return(1);
	}
	TRACE3("singlehost got current time as %ld (%.19s)", now, ctime(&now));
	
	/* hack to get hostname in a form that we would see if we were to 
	   use the broadcast case */

	gethostname(my_name, 255);
	hp = gethostbyname(host);
        /* deleted by rb 1/9/92 - not needed and fails on /etc/hosts
	hp = gethostbyaddr(hp->h_addr, sizeof(int), AF_INET);
        */
	my_domain = (char *) strchr(my_name,'.');
	dot_ptr = (char *) strchr(hp->h_name,'.');
	if ((dot_ptr != NULL) && (strcmp(my_domain,dot_ptr)==0))
		*dot_ptr = '\0';

	/*	HPNFS	jad	87.07.08
	**	Modified to know if we're supposed to sort, and if so just
	**	collect the information for now; only dump it at the end
	*/

	if (lflag || hflag || tflag) {
	    TRACE2("singlehost got info, adding it to entry[%d]", curentry);
	    entry[curentry].addr = 0;		/* DON'T waste time finding! */
	    entry[curentry].machine = (char *)malloc(MACHINELEN+1);
	    strcpy(entry[curentry].machine, host);	/* use name instead! */
	    entry[curentry].curtime = now;
	    entry[curentry].boottime = sw.boottime;
	    memcpy(entry[curentry].avenrun, sw.avenrun, AVENSIZE);
	    curentry++;
	} else {
	    TRACE("singlehost got information, sending to stdout");
	    printf((catgets(nlmsg_fd,NL_SETN,6, "%*.*s  ")), MACHINELEN, MACHINELEN, hp->h_name);
	    putline(now, sw.boottime, sw.avenrun);
	}
	return(0);
}

putline(now, boottime, avenrun)
	time_t now;
	struct timeval boottime;
	long avenrun[];
{
	int uptime, days, hrs, mins, i;
	
	TRACE("putline SOP");
	uptime = now - boottime.tv_sec;
	uptime += 30;
	if (uptime < 0)		/* unsynchronized clocks */
		uptime = 0;
	TRACE2("putline uptime = %d", uptime);
	days = uptime / (60*60*24);
	uptime %= (60*60*24);
	hrs = uptime / (60*60);
	uptime %= (60*60);
	mins = uptime / 60;

	TRACE("putline sending uptime data to stdout");
	printf((catgets(nlmsg_fd,NL_SETN,7, "  up")));
	if (days > 0)
	{
		if (days > 1)
			strcpy(tempbuf,(catgets(nlmsg_fd,NL_SETN,9, "days,")));
		else 
			strcpy(tempbuf,(catgets(nlmsg_fd,NL_SETN,10,"day, ")));
		nl_printf((catgets(nlmsg_fd,NL_SETN,8, " %1$2d %2$s")), days, 
			   tempbuf);
	}
	else
		printf((catgets(nlmsg_fd,NL_SETN,11, "         ")));
	if (hrs > 0)
		nl_printf((catgets(nlmsg_fd,NL_SETN,12, " %1$2d:%2$02d,  ")), hrs, mins);
	else
		nl_printf((catgets(nlmsg_fd,NL_SETN,13, " %1$2d %2$s")), mins, mins>1?(catgets(nlmsg_fd,NL_SETN,14, "mins,")):(catgets(nlmsg_fd,NL_SETN,15, "min, ")));

	/*
	 * Print 1, 5, and 15 minute load averages.
	 * (Found by looking in kernel for avenrun).
	 */
	TRACE("putline sending load average data to stdout");
	printf((catgets(nlmsg_fd,NL_SETN,16, "  load average:")));
	for (i = 0; i < (AVENSIZE/sizeof(avenrun[0])); i++) {
		if (i > 0)
			printf((catgets(nlmsg_fd,NL_SETN,17, ",")));
		printf((catgets(nlmsg_fd,NL_SETN,18, " %.2f")), (double)avenrun[i]/(double)FSCALE);
	}
	printf((catgets(nlmsg_fd,NL_SETN,19, "\n")));
	TRACE("putline returns");
}

collectnames(resultsp, raddrp)
	char *resultsp;
	struct sockaddr_in *raddrp;
{
	struct hostent *hp;
	static int debugcnt;
	int i, cnt;
	register int addr;
	register struct entry *entryp, *lim;
	struct statstime *sw;
	time_t now;
	char *dot_ptr;
        char my_name[255];
        char *my_domain;

	TRACE("collectnames SOP");
	/* 
	 * weed out duplicates
	 */
	addr = raddrp->sin_addr.s_addr;
	lim = entry + curentry;
	for (entryp = entry; entryp < lim; entryp++) {
		TRACE3("checking addr, entrpy->addr, return if equal: %d %d",
				 addr, entryp->addr);
		if (addr == entryp->addr)
			return (0);
	}
	sw = (struct statstime *)resultsp;
	debugcnt++;
	/*	HPNFS	jad	87.07.08
	**	fill in the address and NULL out the name; this way
	**	printnames() knows to look up the name (hostbyaddr)
	*/
	entry[curentry].addr = addr;
	entry[curentry].machine = NULL;
	TRACE("collectnames have a new address ... added the entry");

	/*
	 * if raw, print this entry out immediately
	 * otherwise store for later sorting
	 */
	if (!hflag && !lflag && !tflag) {
		TRACE("collectnames raw printout of address to stdout");
		hp = gethostbyaddr(&raddrp->sin_addr.s_addr,
		    sizeof(int),AF_INET);
		if (hp == NULL)
			printf((catgets(nlmsg_fd,NL_SETN,20, "  0x%08.8x: ")), addr);
		else {
			gethostname(my_name, 255);
			my_domain = (char *) strchr(my_name,'.');
			dot_ptr = (char *) strchr(hp->h_name,'.');
                        if ((dot_ptr != NULL) && (strcmp(my_domain,dot_ptr)==0))
				*dot_ptr = '\0';
			printf((catgets(nlmsg_fd,NL_SETN,21, "%*.*s  ")), MACHINELEN, MACHINELEN, hp->h_name);
		}
		if (vers == RSTATVERS_TIME)
			now = sw->curtime.tv_sec;
		else
			time (&now);
		putline(now, sw->boottime, sw->avenrun);
	}
	else {
		TRACE("collectnames saving info in entry array");
		entry[curentry].boottime = sw->boottime;
		if (vers == RSTATVERS_TIME)
			entry[curentry].curtime = sw->curtime.tv_sec;
		else
			time(&entry[curentry].curtime);
		memcpy(entry[curentry].avenrun, sw->avenrun, AVENSIZE);
	}
	curentry++;
	TRACE2("collectnames curentry = %d, returning", curentry);
	if (dflag && debugcnt >= debug)
		return (1);
	return(0);
}

printnames()
{
	char buf[MACHINELEN+1];
	struct hostent *hp;
	int i, j;
	char *dot_ptr;

	TRACE("printnames SOP");
	for (i = 0; i < curentry; i++) {
		/*	HPNFS	jad	87.07.08
		**	check to see if we have already filled it in!!
		**	if we use single host names this has been done.
		*/
		if ( entry[i].machine != NULL )
			continue;

		hp = gethostbyaddr(&entry[i].addr,sizeof(int),AF_INET);
		if (hp == NULL)
			sprintf(buf, (catgets(nlmsg_fd,NL_SETN,22, "0x%08.8x")), entry[i].addr);
		else {
			dot_ptr = (char *) strchr(hp->h_name,'.');
			if (dot_ptr != NULL)
                                *dot_ptr = '\0';
			sprintf(buf, (catgets(nlmsg_fd,NL_SETN,23, "%.*s")), MACHINELEN, hp->h_name);
		}
		entry[i].machine = (char *)malloc(MACHINELEN+1);
		strcpy(entry[i].machine, buf);
		TRACE3("printnames on entry %d, buf = %s", curentry, buf);
	}
	TRACE("printnames about to qsort data ...");
	if (hflag)
		qsort((char *)entry,curentry,sizeof(*entry),machinecmp);
	else if (lflag)
		qsort((char *)entry,curentry,sizeof(*entry),loadcmp);
	else
		qsort((char *)entry,curentry,sizeof(*entry),uptimecmp);

	TRACE("printnames qsort done, now printing to stdout ...");
	printf((catgets(nlmsg_fd,NL_SETN,24, "\n")));
	for (i = 0; i < curentry; i++) {
		printf((catgets(nlmsg_fd,NL_SETN,25, "%12.12s  ")), entry[i].machine);
		putline(entry[i].curtime, entry[i].boottime, entry[i].avenrun);
	}
	TRACE("printnames returning normally");
}

machinecmp(a,b)
	struct entry *a, *b;
{
	return (strcmp(a->machine, b->machine));
}

uptimecmp(a,b)
	struct entry *a, *b;
{
	return (a->boottime.tv_sec - b->boottime.tv_sec);
}

loadcmp(a,b)
	struct entry *a, *b;
{
	return (a->avenrun[0] - b->avenrun[0]);
}

usage()
{
	fprintf(stderr, (catgets(nlmsg_fd,NL_SETN,26, "Usage: rup [-h] [-l] [-t] [host ...]\n")));
	exit(1);
}
