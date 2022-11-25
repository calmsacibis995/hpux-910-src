#ifndef lint
static  char rcsid[] = "@(#)rusers:	$Revision: 1.31.109.1 $	$Date: 91/11/19 14:07:09 $  ";
#endif
/* NFSSRC rusers.c	2.1 86/04/16 */
/*static  char sccsid[] = "rusers.c 1.1 86/02/05 Copyr 1985 Sun Micro";*/

/*
 * Copyright (c) 1985 by Sun Microsystems, Inc.
 */

#ifndef NLS
#define catgets(i, sn,mn,s) (s)
#define nl_cxtime(a, b)	ctime(a)
#else NLS
#define NL_SETN 1	/* set number */
#include <nl_types.h>
#endif NLS

#include <stdio.h>
/* HPNFS utmp.h uses time_t which requires types.h to be included first */
#include <sys/types.h>
#include <utmp.h>
#include <netdb.h>
#include <sys/socket.h>
#include <rpc/rpc.h>
#include <rpc/pmap_clnt.h>
#include <rpcsvc/rusers.h>
/*	HPNFS
**	include the tracing macros ...
*/
#include <arpa/trace.h>
# define	TRACEFILE	"/tmp/rusers.trace"

struct utmp dummy;
#define NMAX sizeof(dummy.ut_name)
#define LMAX sizeof(dummy.ut_line)
#define	HMAX sizeof(dummy.ut_host)

#define MACHINELEN 12		/* length of machine name printed out */
#define NUMENTRIES 200
#define MAXINT 0x7fffffff
#define min(a,b) ((a) < (b) ? (a) : (b))

struct entry {
	int addr;
	int cnt;
	int idle;		/* set to MAXINT if not present */
	char *machine;
	struct utmpidle *users;
} entry[NUMENTRIES];
int curentry;
int hflag;			/* host: sort by machine name */
int iflag;			/* idle: sort by idle time */
int uflag;			/* users: sort by number of users */
int lflag;			/*  print out long form */
int aflag;			/* all: list all machines */
int dflag;			/* debug: list only first n machines */
int debug;
int vers;
int errors = 0;			/* keep a count of how many errors we had */

int hcompare(), icompare(), ucompare();
int collectnames();
#ifdef NLS
nl_catd nlmsg_fd;
#endif NLS
extern char **environ;

main(argc, argv)
	char **argv;
{
	struct utmpidlearr utmpidlearr;
	enum clnt_stat clnt_stat;
	int single;

	cleanenv(&environ, "LANG", "LANGOPTS", "NLSPATH", "TZ", 0);

#ifdef NLS
	nl_init(getenv("LANG"));
	nlmsg_fd = catopen("rusers",0);
#endif NLS
	STARTTRACE(TRACEFILE);
	single = 0;
	while (argc > 1) {
		if (argv[1][0] != '-') {
			single++;
			TRACE3("main single=%d, host=%s", single, argv[1]);
			errors += singlehost(argv[1]);
		}
		else {
			switch(argv[1][1]) {
	
			case 'h':
				hflag++;
				TRACE2("main hflag=%d", hflag);
				break;
			case 'a':
				aflag++;
				TRACE2("main aflag=%d", aflag);
				break;
			case 'i':
				iflag++;
				TRACE2("main iflag=%d", iflag);
				break;
			case 'l':
				lflag++;
				TRACE2("main lflag=%d", lflag);
				break;
			case 'u':
				uflag++;
				TRACE2("main uflag=%d", uflag);
				break;
			case 'd':
				dflag++;
				TRACE2("main dflag=%d", dflag);
				if (argc < 3)
					usage();
				debug = atoi(argv[2]);
				TRACE2("main debug=%d", debug);
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
	TRACE("main out of while(argc) loop");
	if (iflag + hflag + uflag > 1)
		usage();
	if (single > 0) {
		TRACE("main single > 0, call printnames if flag set");
		if (iflag || hflag || uflag)
			printnames();
		exit(errors);
	}
	if (iflag || hflag || uflag) {
		TRACE("main single = 0, flags set, collecting responses");
		printf((catgets(nlmsg_fd,NL_SETN,1, "collecting responses... ")));
		fflush(stdout);
	}
	vers = RUSERSVERS_IDLE;
	TRACE("main about to clnt_broadcast(RUSERSVERS_IDLE)");
	clnt_stat = clnt_broadcast(RUSERSPROG, RUSERSVERS_IDLE,
	    RUSERSPROC_NAMES, xdr_void, NULL, xdr_utmpidlearr,
	    &utmpidlearr, collectnames);
	vers = RUSERSVERS_ORIG;
	TRACE("main about to clnt_broadcast(RUSERSVERS_ORIG)");
	clnt_stat = clnt_broadcast(RUSERSPROG, RUSERSVERS_ORIG,
	    RUSERSPROC_NAMES, xdr_void, NULL, xdr_utmparr, &utmpidlearr,
	    collectnames);
	TRACE("main all done with broadcasting, printing names if flag set");
	if (iflag || hflag || uflag)
		printnames();
	/*
	**	if we make it here, all is OK, so exit successfully
	*/
	exit(0);
}

singlehost(name)
	char *name;
{
	struct hostent *hp;
	enum clnt_stat err;
	struct sockaddr_in addr;
	struct utmpidlearr utmpidlearr;

	TRACE("singlehost SOP");
	vers = RUSERSVERS_ORIG;
	utmpidlearr.uia_arr = NULL;
	TRACE("singlehost about to callrpc(RUSERSVERS_IDLE)");
	err = (enum clnt_stat)callrpc(name, RUSERSPROG, RUSERSVERS_IDLE,
	    RUSERSPROC_NAMES, xdr_void, 0, xdr_utmpidlearr, &utmpidlearr);
	if (err == RPC_PROGVERSMISMATCH) {
		TRACE("singlehost PROGVERSMISMATCH, try callrpc(ORIG)");
		if (err = (enum clnt_stat)callrpc(name, RUSERSPROG,
		    RUSERSVERS_ORIG, RUSERSPROC_NAMES, xdr_void, 0,
		    xdr_utmparr, &utmpidlearr)) {
			TRACE("singlehost ORIG failed too, we quit trying");
			fprintf(stderr, (catgets(nlmsg_fd,NL_SETN,2, "%*s: ")), name);
			clnt_perrno(err);
			fprintf(stderr, (catgets(nlmsg_fd,NL_SETN,3, "\n")));
			return(1);
		}
	}
	else if (err == RPC_SUCCESS) {
		TRACE("singlehost RPC_SUCCESS, set vers=RUSERSVERS_IDLE");
		vers = RUSERSVERS_IDLE;
	} else {
		TRACE("singlehost some other error, print message and quit");
		fprintf(stderr, (catgets(nlmsg_fd,NL_SETN,4, "%s: ")), name);
		clnt_perrno(err);
		fprintf(stderr, (catgets(nlmsg_fd,NL_SETN,5, "\n")));
		return(1);
	}
	/* 
	 * simulate calling from clnt_broadcast
	 */
	hp = gethostbyname(name);
	TRACE3("singlehost gethostbyname(%s) returns 0x%x", name, hp);
	if (hp == NULL) {
		TRACE2("couldn't find address for %s", name);
		fprintf(stderr, (catgets(nlmsg_fd,NL_SETN,6, "rusers: unknown host name %s\n")), name);
		return(1);
	}
	addr.sin_addr.s_addr = *(int *)hp->h_addr;
	TRACE2("singlehost address = 0x%lx", hp->h_addr);
	collectnames(&utmpidlearr, &addr);
	return(0);
}

collectnames(resultsp, raddrp)
	char *resultsp;
	struct sockaddr_in *raddrp;
{
	struct utmpidlearr utmpidlearr;
	struct utmpidle *uip;
	struct utmp *up;
	static int debugcnt;
	int i, cnt, minidle;
	register int addr;
	register struct entry *entryp, *lim;
	struct utmpidle *p, *q;
	struct hostent *hp;
	char host[MACHINELEN + 1];
        char *dot_ptr;
	char my_name[255];
	char *my_domain;

	TRACE("collectnames SOP");
	utmpidlearr = *(struct utmpidlearr *)resultsp;
	TRACE2("collectnames utmpidlearr.uia_cnt = %d", utmpidlearr.uia_cnt);
	if ((cnt = utmpidlearr.uia_cnt) < 1 && !aflag)
		return(0);
	/* 
	 * weed out duplicates
	 */
	addr = raddrp->sin_addr.s_addr;
	lim = entry + curentry;
	TRACE3("collectnames addr = 0x%x, lim = %d", addr, lim);
	for (entryp = entry; entryp < lim; entryp++)
		if (addr == entryp->addr) {
			TRACE("collectnames address matched, return");
			return (0);
		}
	debugcnt++;
	entry[curentry].addr = addr;
	hp = gethostbyaddr(&raddrp->sin_addr.s_addr,
	    sizeof(int),AF_INET);
	TRACE2("collectnames gethostbyaddr returns 0x%lx", hp);
	if (hp == NULL)
		sprintf(host, (catgets(nlmsg_fd,NL_SETN,7, "0x%08.8x")), addr);
	else {
		gethostname(my_name, 255);
		my_domain = (char *) strchr(my_name,'.');
		dot_ptr = (char *) strchr(hp->h_name,'.');
		if ((dot_ptr != NULL) && (strcmp(my_domain,dot_ptr)==0))
			*dot_ptr = '\0';
		sprintf(host, (catgets(nlmsg_fd,NL_SETN,8, "%.*s")), MACHINELEN, hp->h_name);
	}
	TRACE2("collectnames host name is %s", host);

	/*
	 * if raw, print this entry out immediately
	 * otherwise store for later sorting
	 */
	if (!iflag && !hflag && !uflag) {
		TRACE("collectnames no flags, printing data now");
		if (lflag)
			for (i = 0; i < cnt; i++)
				putline(host, utmpidlearr.uia_arr[i], vers);
		else {
			printf((catgets(nlmsg_fd,NL_SETN,9, "%-*.*s")), MACHINELEN, MACHINELEN, host);
			for (i = 0; i < cnt; i++)
				printf((catgets(nlmsg_fd,NL_SETN,10, " %.*s")), NMAX, 
				    utmpidlearr.uia_arr[i]->ui_utmp.ut_name);
			printf((catgets(nlmsg_fd,NL_SETN,11, "\n")));
		}
	}
	else {
		TRACE2("collectnames flag set, adding as entry %d", curentry);
		entry[curentry].cnt = cnt;
		TRACE2("collectnames doing malloc for %d elements", cnt);
		q = (struct utmpidle *)
		    malloc(cnt*sizeof(struct utmpidle));
		TRACE2("collectnames malloc done, got 0x%x", q);
		p = q;
		minidle = MAXINT;
		for (i = 0; i < cnt; i++) {
			TRACE2("collectnames copying element %d", i);
			memcpy(q, utmpidlearr.uia_arr[i],
			    sizeof(struct utmpidle));
			if (vers == RUSERSVERS_IDLE)
				minidle = min(minidle, q->ui_idle);
			TRACE2("collectnames minidle = %d", minidle);
			q++;
		}
		entry[curentry].users = p;
		entry[curentry].idle = minidle;
	}
	if (curentry >= NUMENTRIES) {
		TRACE2("collectnames curentry exceeds %d, abort!", NUMENTRIES);
		fprintf(stderr, (catgets(nlmsg_fd,NL_SETN,12, "too many hosts on network\n")));
		exit(1);
	}
	curentry++;
	TRACE2("collectnames curentry = %d", curentry);
	if (dflag && debugcnt >= debug)
		return (1);
	TRACE("collectnames returns normally");
	return(0);
}

printnames()
{
	char buf[MACHINELEN+1];
	struct hostent *hp;
	int i, j, v;
	int (*compare)();
	char *dot_ptr;

	TRACE("printnames SOP");
	for (i = 0; i < curentry; i++) {
		TRACE2("printnames getting host entry %d", i);
		hp = gethostbyaddr(&entry[i].addr,sizeof(int),AF_INET);
		TRACE2("printnames gethostbyaddr returns 0x%x", hp);
		if (hp == NULL)
			sprintf(buf, (catgets(nlmsg_fd,NL_SETN,13, "0x%08.8x")), entry[i].addr);
		else {
			dot_ptr = (char *) strchr(hp->h_name,'.');
			if (dot_ptr != NULL)
				*dot_ptr = '\0';
			sprintf(buf, (catgets(nlmsg_fd,NL_SETN,14, "%.*s")), MACHINELEN, hp->h_name);
                }
		TRACE2("printnames host name is %s", buf);
		entry[i].machine = (char *)malloc(MACHINELEN+1);
		strcpy(entry[i].machine, buf);
	}
	if (iflag)
		compare = icompare;
	else if (hflag)
		compare = hcompare;
	else
		compare = ucompare;
	TRACE("printnames calling qsort on entry[]");
	qsort(entry, curentry, sizeof(struct entry), compare);
	TRACE("printnames qsort done, now printing data out");
	printf((catgets(nlmsg_fd,NL_SETN,15, "\n")));
	for (i = 0; i < curentry; i++) {
		if (!lflag) {
			TRACE("printnames lflag ...");
			printf((catgets(nlmsg_fd,NL_SETN,16, "%-*.*s")), MACHINELEN,
			    MACHINELEN, entry[i].machine);
			for (j = 0; j < entry[i].cnt; j++)
				printf((catgets(nlmsg_fd,NL_SETN,17, " %.*s")), NMAX,
				    entry[i].users[j].ui_utmp.ut_name);
			printf((catgets(nlmsg_fd,NL_SETN,18, "\n")));
		}
		else {
			TRACE("printnames else not lflag");
			if (entry[i].idle == MAXINT)
				v = RUSERSVERS_ORIG;
			else
				v = RUSERSVERS_IDLE;
			for (j = 0; j < entry[i].cnt; j++)
				putline(entry[i].machine,
				    &entry[i].users[j], v);
		}
	}
}

hcompare(a,b)
	struct entry *a, *b;
{
	return (strcmp(a->machine, b->machine));
}

ucompare(a,b)
	struct entry *a, *b;
{
	return (b->cnt - a->cnt);
}

icompare(a,b)
	struct entry *a, *b;
{
	return (a->idle - b->idle);
}

putline(host, uip, vers)
	char *host;
	struct utmpidle *uip;
	int vers;
{
	register char *cbuf;
	struct utmp *up;
	struct hostent *hp;
	char buf[100];

	TRACE("putline SOP");
	up = &uip->ui_utmp;
	TRACE3("putline up = 0x%x, name = %s", up, up->ut_name);
	printf((catgets(nlmsg_fd,NL_SETN,19, "%-*.*s ")), NMAX, NMAX, up->ut_name);

	strcpy(buf, host);
	strcat(buf, (catgets(nlmsg_fd,NL_SETN,20, ":")));
	strncat(buf, up->ut_line, LMAX);
	TRACE2("putline buf is %s", buf);
	printf((catgets(nlmsg_fd,NL_SETN,21, "%-*.*s")), MACHINELEN+LMAX, MACHINELEN+LMAX, buf);

	cbuf = (char *)nl_cxtime(&up->ut_time, "");
	printf((catgets(nlmsg_fd,NL_SETN,22, "  %.12s  ")), cbuf+4);
	if (vers == RUSERSVERS_IDLE) {
		prttime(uip->ui_idle, (catgets(nlmsg_fd,NL_SETN,23, "")));
	}
	else
		printf((catgets(nlmsg_fd,NL_SETN,24, "    ??")));
	/*
	**	HP-UX does not define a ut_host field in utmp
	*/
	if (up->ut_host[0])
		printf((catgets(nlmsg_fd,NL_SETN,25, " (%.*s)")), HMAX, up->ut_host);
	putchar('\n');
	TRACE("putline returning normally");
}

/*
 * prttime prints a time in hours and minutes.
**	Should have used the same routine used by "rwho" ... groan
 */
prttime(tim)
	time_t tim;
{
	TRACE2("prttime SOP, tim = %d", tim);
	if (tim == 0)	return;

	if (tim >= 60)	printf((catgets(nlmsg_fd,NL_SETN,26, "%3d:")), tim/60);
	else		printf((catgets(nlmsg_fd,NL_SETN,27, "   :")));
	printf((catgets(nlmsg_fd,NL_SETN,28, "%02d")), tim%60);

	TRACE("prttime returns normally");
}

#ifdef NOTDEF
/* 
 * for debugging, never called anywhere in rusers
 */
printit(i)
{
	int j, v;
	
	TRACE2("printit SOP, i = %d", i);
	printf((catgets(nlmsg_fd,NL_SETN,29, "%12.12s: ")), entry[i].machine);
	if (entry[i].cnt) {
		if (entry[i].idle == MAXINT)
			v = RUSERSVERS_ORIG;
		else
			v = RUSERSVERS_IDLE;
		/*
		**	Sun 3.0 forgot the first argument ...
		*/
		putline(entry[i].machine, &entry[i].users[0], v);
		for (j = 1; j < entry[i].cnt; j++) {
			printf((catgets(nlmsg_fd,NL_SETN,30, "\t")));
			putline(entry[i].machine, &entry[i].users[j], vers);
		}
	}
	else
		printf((catgets(nlmsg_fd,NL_SETN,31, "\n")));
}
#endif NOTDEF

usage()
{
	fprintf(stderr, (catgets(nlmsg_fd,NL_SETN,32, "Usage: rusers [-a] [-h] [-i] [-l] [-u] [host ...]\n")));
	exit(1);
}
