/*
 * Copyright (c) 1985,1989 Regents of the University of California.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#ifndef lint
#ifndef BFA
char copyright[] =
"@(#) Copyright (c) 1985,1989 Regents of the University of California.\n\
 All rights reserved.\n";
#endif /* BFA */
#endif /* not lint */

static char rcsid[] = "$Header: main.c,v 1.52.109.4 94/02/28 15:05:26 randyc Exp $";

#ifdef PATCH_STRING
static char *patch_3690="@(#) PATCH_9.0: nslookup $Revision: 1.52.109.4 $ 94/03/09 PHNE_3690";
#endif


/*
 *******************************************************************************
 *  
 *   main.c --
 *  
 *	Main routine and some action routines for the name server lookup
 *	program.
 *
 *	Andrew Cherenson	CS298-26, Fall 1985
 *	Govind Tatachari	Oct 1993 (switch oriented enhancements)
 *  
 *******************************************************************************
 */

#include <sys/param.h>
#include <netdb.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/nameser.h>
#include <arpa/inet.h>
#include <resolv.h>
#include <signal.h>
#include <setjmp.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "res.h"
#include <switch.h>
/*
#ifndef hpux
#include "pathnames.h"
#endif hpux
*/

/*
 *  Default Internet address of the current host.
 */

#if BSD < 43
#define LOCALHOST "127.0.0.1"
#endif

/*
 * Name of a top-level name server. Can be changed with 
 * the "set root" command.
 */

#ifndef ROOT_SERVER
#define		ROOT_SERVER "ns.nic.ddn.mil."
#endif
char		rootServerName[NAME_LEN] = ROOT_SERVER;


/*
 *  Import the state information from the resolver library.
 */

extern struct state _res;


/*
 *  Info about the most recently queried host.
 */

HostInfo	curHostInfo;
int		curHostValid = FALSE;


/*
 *  Info about the default name server.
 */

HostInfo	*defaultPtr[HOSTTABLE+1] = { NULL, NULL, NULL};
char		defaultServer[HOSTTABLE+1][NAME_LEN];
struct in_addr	defaultAddr[HOSTTABLE+1];

int server_specified=0;		/* specified a nameserver on command line */
int nis_unavailable=0;		/* true when NIS is not up, it still gets 
			         * tried with each query though. ASH */


/*
 *  Initial name server query type is Address.
 */

int		queryType = T_A;
int		queryClass = C_IN;

/*
 * Stuff for Interrupt (control-C) signal handler.
 */

extern void	IntrHandler();
FILE		*filePtr;
jmp_buf		env;

static void CvtAddrToPtr();
static void ReadRC();

int             lookup_type = -1;

#ifdef  PROC
char  		module[25] = "nslookup";
#endif

int	nsw_trace;

tr_switch(fmt, ts1, ts2, ts3, ts4)
char  *fmt, *ts1, *ts2, *ts3, *ts4;
{
	if (nsw_trace)
		printf(fmt, ts1, ts2, ts3, ts4);
}


/* PART 0 - main */

static  struct  __nsw_switchconfig    *lnsw_policy = NULL;

/* The p_lkup is up here, because we want DoLookup to know which criteria
   is  being applied to it. This will allow it not to print error
   messages, when the criteria says to continue when that error
   is set for continuing (e.g. if no answer found, do not print
   error if criteria is set for continuing with NOTFOUND)
   Note: if the user wants to know that it continued and wants to
   see the error messages, then they can turn on swtrace, otherwise
   nslookup silently finds the next source.

   p_lkup is generally local to PolicyLookUp
*/
static struct  __nsw_lookup  *	p_lkup;

#define	hlumap(x) (x-SRC_DNS)

int  mapresult (result)
int	result;
{
	switch (result) {
	case SUCCESS:
		return (__NSW_SUCCESS);
	case TIME_OUT:
		return (__NSW_UNAVAIL);
	default:
		return(__NSW_NOTFOUND);
	}
}

InitDefServers(argc, argv)
int	argc;
char	**argv;
{
	Boolean		useLocalServer;
	struct hostent	*hp;
	int		ins;
	int		result;
	char		hostName[NAME_LEN];
	static  char	domain[128];

	extern  int	__nsw_src;
	struct  __nsw_lookup	*t_lkup;
	int		ret_status;
	Boolean		found = FALSE;
	int		lustate = -1;
	int		interactive;

	interactive =  !(argc && *argv[0] != '-');
#ifdef  PROC
	char	*proc;
#endif

#ifdef  PROC
	proc = strchr(module, '.');
	if (proc!=NULL)
		*proc = '\0';
	proc = strcat(module, ".InitDefs");
	printf("%s\n", proc);
#endif
	tr_switch("\nInitializing defaults for sources\n");

	/*
     *	The server argument overrides the choice of initial server. 
     */
	
	useLocalServer = FALSE;
	if (argc == 2) {
		int	ia;
		struct in_addr addr;

		 
		/*
	 *	Use an explicit name server. If the hostname lookup fails,
	 *	default to the server(s) in resolv.conf.
	 */
		addr.s_addr = inet_addr(*++argv);
		tr_switch("name server specified=%s\n", *argv);
		if (addr.s_addr != (unsigned long)-1) {
			_res.nscount = 1;
			_res.nsaddr.sin_addr = addr;
		/* 
		 * The server argument does override the resolver
		 * algorithm (i.e. switch), as well as, the choice of
		 * initial DNS server.
		 */
		        server_specified++;
		}
		else {
			hp = gethostbyname(*argv);
			if (hp == NULL) {
				fprintf(stderr, "*** Can't find server address for '%s': ", *argv);
				herror((char *)NULL);
				fputc('\n', stderr);
			}
			else {
				for (ia = 0; ia < MAXNS && hp->h_addr_list[ia] != NULL; ia++) {
					bcopy(hp->h_addr_list[ia], 
					    (char *)&_res.nsaddr_list[ia].sin_addr, hp->h_length);
					tr_switch("%s resolved to %s\n", *argv,
					    inet_ntoa(_res.nsaddr_list[ia].sin_addr));
				}
				_res.nscount = ia;
		        	server_specified++;
			}
		}
			
	}

    /* Find the the default server's name.
     * Otherwise, see if the current host has a server.
     */
	for (t_lkup=lnsw_policy->lookups; t_lkup!=NULL; t_lkup=t_lkup->next) {

		switch(__nsw_inteqv(t_lkup->service_name)) {

		case    SRC_DNS :
			__nsw_src = SRC_DNS;
			lookup_type = -1;
			if (_res.nscount > 1)
				lookup_type = NAMESERVER;
			ret_status = __NSW_NOTFOUND;
			tr_switch("\nlookup source is DNS\n");
			if (_res.nscount == 0 || useLocalServer) {
				LocalServer(defaultPtr[NAMESERVER]);
				ret_status = __NSW_SUCCESS;
			}
			else {
				for (ins = 0; ins < _res.nscount; ins++) {
					if (_res.nsaddr_list[ins].sin_addr.s_addr == INADDR_ANY) {
						LocalServer(defaultPtr[NAMESERVER]);
						ret_status = __NSW_SUCCESS;
						break;
					}
					result = GetHostInfoByAddr(&(_res.nsaddr_list[ins].sin_addr), 
					    &(_res.nsaddr_list[ins].sin_addr), defaultPtr[NAMESERVER]);
					if (result == SUCCESS) {
						ret_status = __NSW_SUCCESS;
						defaultAddr[NAMESERVER] = _res.nsaddr_list[ins].sin_addr;
						tr_switch("DNS Lookup (GetHostInfoBy) Successful\n");
						tr_switch("DNS defaultInfo filled\n");
						tr_switch("defaultAddr set to %s\n", inet_ntoa(defaultAddr[NAMESERVER]));
						break;
					}
					if (lookup_type == NAMESERVER) {
						fprintf(stderr, "*** Can't find server name for address %s: %s\n", 
						    inet_ntoa(_res.nsaddr_list[ins].sin_addr), DecodeError(result));
					}
					tr_switch("DNS Lookup (GetHostInfoByAddr) Unsuccessful\n");
				} /* for */
				/*
		 *  If we have exhausted the list, tell the user about the
		 *  command line argument to specify an address.
		 */
				/*
		 ** It is assumed that we have made a name server query by this
		 ** point. Either lookup_type will be NAMESERVER or it wil be -1.
		 ** If we have run through the list of servers in _res and lookup
		 ** type is NAMESERVER, bail out since no servers are answering.
		 */
				if ((ins == _res.nscount) || (lookup_type == -1)) {
					ret_status = __NSW_UNAVAIL;
					if (lookup_type == NAMESERVER) {
						fprintf(stderr, "*** Default servers are not available\n");
						/* exit(1); */
					}
				}
			}
			if (ret_status != __NSW_UNAVAIL) {
				tr_switch("DNS defaultInfo filled\n");
				strcpy(defaultServer[NAMESERVER], defaultPtr[NAMESERVER]->name);
			}
			break;


		case    SRC_NIS :
			__nsw_src = SRC_NIS;
			lookup_type = hlumap(__nsw_src);
			ret_status = __NSW_NOTFOUND;
			tr_switch("\nlookup source is NIS\n");
			if (yellowup(0, domain)) {
				FILE  *ypwhich;
				char  *cp;

				ypwhich = popen ("/usr/bin/ypwhich", "r");
				fgets(hostName, sizeof(hostName), ypwhich);
				cp = strchr(hostName, '\n');
				if (cp != NULL)
					*cp = '\0';
				pclose(ypwhich);
				result=GetHostInfoByName(&defaultAddr[lookup_type], C_IN, T_A, hostName, defaultPtr[lookup_type],
				    1);
				if (result == SUCCESS)
					ret_status = __NSW_SUCCESS;
				tr_switch("NIS defaultInfo filled\n");
				if (defaultPtr[lookup_type] != NULL)
					strcpy(defaultServer[lookup_type], defaultPtr[lookup_type]->name);
			}
			else {
				nis_unavailable++;	/*ASH*/
				ret_status = __NSW_UNAVAIL;
				tr_switch("NIS - Unavailable\n");
			}
			break ;

		case    SRC_FILES :
			__nsw_src = SRC_FILES;
			lookup_type = hlumap(__nsw_src);
			tr_switch("\nlookup source is FILES\n");
			gethostname(hostName, sizeof(hostName));
			defaultPtr[lookup_type]->name = (char *)Calloc(1, strlen(hostName)+1);
			strcpy(defaultPtr[lookup_type]->name, hostName);
			ret_status = __NSW_SUCCESS;
			strcpy(defaultServer[lookup_type], defaultPtr[lookup_type]->name);
			tr_switch("Files defaultInfo filled\n");
			break ;

		default :
			__nsw_src = SRC_UNKNOWN;
			ret_status = __NSW_UNAVAIL;
			tr_switch("Unknown source - unavailable\n");
			break ;
		} /* end of switch */

		if ( !found && (ret_status == __NSW_SUCCESS) ) {
			found = TRUE;
			lustate = hlumap(__nsw_src);
		}
	} /* for lookup */

	if (!found) {
		/*
		 * We have tried it all. No name service source is responding.
		 * ?? will it help earn a good name ??
		 */
		fprintf(stderr, "No name server/service(s) responding.\n");
		fflush(stderr);
		exit(1);
	}
	if (server_specified)
	{
	   if ((lustate != NAMESERVER) && (interactive))
	   {
	     printf("Specifying a nameserver has overridden the switch policy order.\n");
	     printf("The reset command will reinstate the order specified by the switch policy.\n");
	   }
	    lookup_type =  NAMESERVER;
	}
        else
		lookup_type = lustate; 	/* normal switch policy */
	queryType = T_A;

	if ((lookup_type == NAMESERVER) && (_res.defdname[0] == '\0')){
		fprintf(stderr,"*** Warning - the local domain is not set.\n");
		fprintf(stderr,"*** Either hostname should be a domain name,\n");
		fprintf(stderr,"*** the domain should be specified in /etc/resolv.conf,\n");
		fprintf(stderr,"*** or the shell variable LOCALDOMAIN should be set.\n\n");
	}
}

/*
 *******************************************************************************
 *
 *  main --
 *
 *	Initializes the resolver library and determines the address
 *	of the initial name server.
 *	The yylex routine is used to read and perform commands.
 *
 *******************************************************************************
 */

main(argc, argv)
int	argc;
char	**argv;
{
	char	*wantedHost = NULL;
	int	ii;
	int	ret_status;
	extern  int	__nsw_debug;
#ifdef  PROC
	char	*proc;
#endif

#ifdef  PROC
	proc = strcat(module, ".main");
	printf("%s\n", proc);
#endif
	/*
     * Initialize the resolver library routines.
     * The resolver initializes addressses of potential name servers.
     */
	if (res_init() == -1) {
		fprintf(stderr,"*** Can't initialize resolver.\n");
		exit(1);
	}

	nsw_trace = 0;
	__nsw_debug = 0;
	tr_switch("\npotential name servers=%d\n", _res.nscount);

	/*
     * Allocate space for the default server's host info.
     */

	for (ii=0; ii <= HOSTTABLE; ii++)
		defaultPtr[ii] = (HostInfo *) Calloc(1, sizeof(HostInfo));

	/*
     * Parse the arguments:
     *  no args =  go into interactive mode, use default host as server
     *	1 arg	=  use as host name to be looked up, default host will be server
     *		   non-interactive mode
     *  2 args	=  1st arg: 
     *		     if it is '-', then 
     *		        ignore but go into interactive mode
     *		     else 
     *		         use as host name to be looked up, 
     *			 go into non-interactive mode
     *		2nd arg: name or inet address of server
     *
     *	"Set" options are specified with a leading - and must come before
     *	any arguments. For example, to find the well-known services for
     *  a host, type "nslookup -query=wks host"
     */

	ReadRC();			/* look for options file */

	++argv;
	--argc;		/* skip prog name */

	while (argc && *argv[0] == '-' && argv[0][1]) {
		(void) SetOption (&(argv[0][1]));
		++argv;
		--argc;
	}
	if (argc > 2)
		Usage();
	if (argc && *argv[0] != '-')
		wantedHost = *argv;	/* name of host to be looked up */
	/* 
	 * allow: 'nslookup -' to state that DNS is desired using the
         * default nameservers... override switch order.
	 */
	 if ((argc < 2) && (*argv[0] == '-'))
	    server_specified++;


    /*
     * First time around, get the switch policy
     */
	if (lnsw_policy == NULL) {
		lnsw_policy = __nsw_getconfig(__NSW_HOSTS_DB, &ret_status);
		if (lnsw_policy == NULL)
			lnsw_policy = __nsw_getdefault(__NSW_HOSTS_DB);
	}
	InitDefServers(argc, argv);

#ifdef DEBUG
#ifdef DEBUG2
	_res.options |= RES_DEBUG2;
#endif
	_res.options |= RES_DEBUG;
	_res.retry    = 2;
#endif DEBUG

	/*
     * If we're in non-interactive mode, look up the wanted host and quit.
     * Otherwise, print the initial server's name and continue with
     * the initialization.
     */
	if (wantedHost != (char *) NULL) {
		tr_switch("\nBatch mode Lookup\n");
		tr_switch("lookup_type = %d\n\n", lookup_type);
		PolicyLookupHost(wantedHost, 0);
		exit(0);
	}
	tr_switch("\nInteractive Lookup\n");
	tr_switch("lookup_type = %d\n\n", lookup_type);
	if(lookup_type == NAMESERVER)
		PrintHostInfo(stdout, "Default Name Server:", defaultPtr[lookup_type]);
	else if (lookup_type == YP)
		PrintHostInfo(stdout, "Default NIS Server:", defaultPtr[lookup_type]);
	else if (lookup_type == HOSTTABLE)
		PrintHostInfo(stdout, "Using /etc/hosts on:", defaultPtr[lookup_type]);

	/*
	 * Setup the environment to allow the interrupt handler to return here.
	 */

	(void) setjmp(env);

	/* 
	 * Return here after a longjmp.
	 */

	signal(SIGINT, IntrHandler);
	signal(SIGPIPE, SIG_IGN);

	/*
	 * Read and evaluate commands. The commands are described in commands.l
	 * Yylex returns 0 when ^D or 'exit' is typed. 
	 */

	printf("> ");
	fflush(stdout);
	while(yylex()) {
		printf("> ");
		fflush(stdout);
	}
	exit(0);
}


/* PART 1 - ReadRC */

/*
 *******************************************************************************
 *
 * ReadRC --
 *
 *	Use the contents of ~/.nslookuprc as options.
 *
 *******************************************************************************
 */

static void
ReadRC()
{
	register FILE *fp;
	register char *cp;
	char buf[NAME_LEN];

	if ((cp = getenv("HOME")) != NULL) {
		(void) strcpy(buf, cp);
		(void) strcat(buf, "/.nslookuprc");

		if ((fp = fopen(buf, "r")) != NULL) {
			while (fgets(buf, sizeof(buf), fp) != NULL) {
				if ((cp = strchr(buf, '\n')) != NULL) {
					*cp = '\0';
				}
				(void) SetOption(buf);
			}
			(void) fclose(fp);
		}
	}
}


/*
 *******************************************************************************
 *  Setswt -- 
 *	This routine is used to change the state information that affect
 *	the trace of source lookups and source switch.
 *
 *  Results:
 *	SUCCESS		the command was parsed correctly.
 *	ERROR		the command was not parsed correctly.
 *******************************************************************************
 */

void Showall();

int
Setswt(option)
register char *option;
{
	while (isspace(*option))
		++option;
	if (strncmp (option, "set ", 4) == 0)
		option += 4;
	while (isspace(*option))
		++option;

	if (*option == 0) {
		fprintf(stderr, "*** Invalid set command\n");
		return(ERROR);
	} else {
		if (strncmp(option, "all", 3) == 0) {
			Showall();
		} else if (strncmp(option, "ALL", 3) == 0) {
			Showall();
		} else if (strncmp(option, "swt", 3) == 0) {
			nsw_trace = 1;
		} else if (strncmp(option, "noswt", 5) == 0) {
			nsw_trace = 0;
		} else {
			fprintf(stderr, "*** Invalid option: %s\n",  option);
			return(ERROR);
		}
	}
	return(SUCCESS);
}

/*
 *******************************************************************************
 *
 *  SetOption -- 
 *
 *	This routine is used to change the state information that affect
 *	the lookups. The command format is
 *	   set keyword[=value]
 *	Most keywords can be abbreviated. Parsing is very simplistic--
 *	A value must not be separated from its keyword by white space.
 *
 *	Valid keywords:		Meaning:
 *	ALL			lists current values of options, including
 *				  hidden options.
 *	all			lists current values of options.
 *	[no]d2			turn on/off extra debugging mode.
 *	[no]debug		turn on/off debugging mode.
 *	[no]defname		use/don't use default domain name.
 *	domain=NAME		set default domain name to NAME.
 *	[no]ignore		ignore/don't ignore trunc. errors.
 *	port			TCP/UDP port to server.
 *	query=value		set default query type to value,
 *				value is one of the query types in RFC883
 *				without the leading T_.	(e.g., A, HINFO)
 *	[no]recurse		use/don't use recursive lookup.
 *	retry=#			set number of retries to #.
 *	root=NAME		change root server to NAME.
 *	[no]search		use/don't use domain search list.
 *	srchl			domain search list.
 *	[no]swdebug		[don't] trace parsing of switch configuration
 *				file
 *	[no]swtrace		[don't] provide detailed trace information
 *				regarding
 *				source lookup and switching
 *	time=#			set timeout length to #.
 *	[no]vc			use/don't use virtual circuit.
 *
 * 	Deprecated:
 *	[no]primary		use/don't use primary server.
 *
 *  Results:
 *	SUCCESS		the command was parsed correctly.
 *	ERROR		the command was not parsed correctly.
 *
 *******************************************************************************
 */

int
SetOption(option)
register char *option;
{
	char	type[NAME_LEN];
	char	*ptr;
	int		tmp;

	while (isspace(*option))
		++option;
	if (strncmp (option, "set ", 4) == 0)
		option += 4;
	while (isspace(*option))
		++option;

	if (*option == 0) {
		fprintf(stderr, "*** Invalid set command\n");
		return(ERROR);
	} else {
		if (strncmp(option, "all", 3) == 0) {
			ShowOptions();
		} else if (strncmp(option, "ALL", 3) == 0) {
			ShowOptions();
		} else if (strncmp(option, "d2", 2) == 0) {	/* d2 (more debug) */
			_res.options |= (RES_DEBUG | RES_DEBUG2);
		} else if (strncmp(option, "nod2", 4) == 0) {
			_res.options &= ~RES_DEBUG2;
			printf("d2 mode disabled; still in debug mode\n");
		} else if (strncmp(option, "deb", 3) == 0) {	/* debug */
			_res.options |= RES_DEBUG;
		} else if (strncmp(option, "nodeb", 5) == 0) {
			_res.options &= ~(RES_DEBUG | RES_DEBUG2);
		} else if (strncmp(option, "def", 3) == 0) {	/* defname */
			_res.options |= RES_DEFNAMES;
		} else if (strncmp(option, "nodef", 5) == 0) {
			_res.options &= ~RES_DEFNAMES;
		} else if (strncmp(option, "do", 2) == 0) {	/* domain */
			ptr = strchr(option, '=');
			if (ptr != NULL) {
				sscanf(++ptr, "%s", _res.defdname);
				res_re_init();
			}
		} else if (strncmp(option, "ig", 2) == 0) {	/* ignore */
			_res.options |= RES_IGNTC;
		} else if (strncmp(option, "noig", 4) == 0) {
			_res.options &= ~RES_IGNTC;
		} else if (strncmp(option, "po", 2) == 0) {	/* port */
			ptr = strchr(option, '=');
			if (ptr != NULL) {
				sscanf(++ptr, "%hu", &nsport);
			}
#ifdef deprecated
		} else if (strncmp(option, "pri", 3) == 0) {	/* primary */
			_res.options |= RES_PRIMARY;
		} else if (strncmp(option, "nopri", 5) == 0) {
			_res.options &= ~RES_PRIMARY;
#endif
		} else if (strncmp(option, "q", 1) == 0 ||	/* querytype */
		strncmp(option, "ty", 2) == 0) {	/* type */
			ptr = strchr(option, '=');
			if (ptr != NULL) {
				sscanf(++ptr, "%s", type);
				queryType = StringToType(type, queryType);
			}
		} else if (strncmp(option, "cl", 2) == 0) {	/* query class */
			ptr = strchr(option, '=');
			if (ptr != NULL) {
				sscanf(++ptr, "%s", type);
				queryClass = StringToClass(type, queryClass);
			}
		} else if (strncmp(option, "rec", 3) == 0) {	/* recurse */
			_res.options |= RES_RECURSE;
		} else if (strncmp(option, "norec", 5) == 0) {
			_res.options &= ~RES_RECURSE;
		} else if (strncmp(option, "ret", 3) == 0) {	/* retry */
			ptr = strchr(option, '=');
			if (ptr != NULL) {
				sscanf(++ptr, "%d", &tmp);
				if (tmp >= 0) {
					_res.retry = tmp;
				}
			}
		} else if (strncmp(option, "ro", 2) == 0) {	/* root */
			ptr = strchr(option, '=');
			if (ptr != NULL) {
				sscanf(++ptr, "%s", rootServerName);
			}
		} else if (strncmp(option, "sea", 3) == 0) {	/* search list */
			_res.options |= RES_DNSRCH;
		} else if (strncmp(option, "nosea", 5) == 0) {
			_res.options &= ~RES_DNSRCH;
		} else if (strncmp(option, "srchl", 5) == 0) {	/* domain search list */
			ptr = strchr(option, '=');
			if (ptr != NULL) {
				res_dnsrch(++ptr);
			}
		} else if (strncmp(option, "swd", 3) == 0) {	/* debug switch configuration */
			__nsw_debug = 1;
		} else if (strncmp(option, "noswd", 5) == 0) {
			__nsw_debug = 0;
		} else if (strncmp(option, "swt", 3) == 0) {	/* trace name service switch */
			nsw_trace = 1;
		} else if (strncmp(option, "noswt", 5) == 0) {
			nsw_trace = 0;
		} else if (strncmp(option, "ti", 2) == 0) {	/* timeout */
			ptr = strchr(option, '=');
			if (ptr != NULL) {
				sscanf(++ptr, "%d", &tmp);
				if (tmp >= 0) {
					_res.retrans = tmp;
				}
			}
		} else if (strncmp(option, "v", 1) == 0) {	/* vc */
			_res.options |= RES_USEVC;
		} else if (strncmp(option, "nov", 3) == 0) {
			_res.options &= ~RES_USEVC;
		} else {
			fprintf(stderr, "*** Invalid option: %s\n",  option);
			return(ERROR);
		}
	}
	return(SUCCESS);
}


#define SRCHLIST_SEP '/'

res_dnsrch(cp)
register char *cp;
{
	register char **pp;
	int n;

	(void)strncpy(_res.defdname, cp, sizeof(_res.defdname) - 1);
	if ((cp = strchr(_res.defdname, '\n')) != NULL)
		*cp = '\0';
	/*
     * Set search list to be blank-separated strings
     * on rest of line.
     */
	cp = _res.defdname;
	pp = _res.dnsrch;
	*pp++ = cp;
	for (n = 0; *cp && pp < _res.dnsrch + MAXDNSRCH; cp++) {
		if (*cp == SRCHLIST_SEP) {
			*cp = '\0';
			n = 1;
		} else if (n) {
			*pp++ = cp;
			n = 0;
		}
	}
	if ((cp = strchr(pp[-1], SRCHLIST_SEP)) != NULL) {
		*cp = '\0';
	}
	*pp = NULL;
}

/*
 *******************************************************************************
 *  Showall --
 *	Prints out the state information used by the sources
 *
 *******************************************************************************
 */

void  PrintPolicy();
void
Showall()
{
	printf("Set options:\n");
	printf("  %sswtrace\t", (nsw_trace) ? "" : "no");
	printf("  querytype=%s\t", p_type(queryType));
	printf("\n\n");
	PrintPolicy();
	printf("\n");
	PrintHostInfo(stdout, "Default Name Server:", defaultPtr[NAMESERVER]);
	PrintHostInfo(stdout, "Default NIS Server:", defaultPtr[YP]);
	PrintHostInfo(stdout, "Default files Server:", defaultPtr[HOSTTABLE]);
	if (curHostValid) {
		PrintHostInfo(stdout, "Host:", &curHostInfo);
	}
}

/*
 *******************************************************************************
 *
 *  ShowOptions --
 *
 *	Prints out the state information used by the resolver
 *	library and other options set by the user.
 *
 *******************************************************************************
 */

void
ShowOptions()
{
	register char **cp;

	PrintHostInfo(stdout, "Default Name Server:", defaultPtr[lookup_type]);
	if (curHostValid) {
		PrintHostInfo(stdout, "Host:", &curHostInfo);
	}

	printf("Set options:\n");
	printf("  %sd2\t\t", (_res.options & RES_DEBUG2) ? "" : "no");
	printf("  %sdebug  \t", (_res.options & RES_DEBUG) ? "" : "no");
	printf("  %sdefname\t", (_res.options & RES_DEFNAMES) ? "" : "no");
	printf("  %signoretc\t", (_res.options & RES_IGNTC) ? "" : "no");
	printf("  %srecurse\n", (_res.options & RES_RECURSE) ? "" : "no");
	printf("  %ssearch\t", (_res.options & RES_DNSRCH) ? "" : "no");
	printf("  %sswtrace\t", (nsw_trace) ? "" : "no");
	printf("  %svc\t\t", (_res.options & RES_USEVC) ? "" : "no");

	printf("  port=%u\n", nsport);
	printf("  querytype=%s\t", p_type(queryType));
	printf("  class=%s\t", p_class(queryClass));
	printf("  timeout=%d\t", _res.retrans);
	printf("  retry=%d\n", _res.retry);
	printf("  root=%s\n", rootServerName);
	printf("  domain=%s\n", _res.defdname);

	if (cp = _res.dnsrch) {
		printf("  srchlist=%s", *cp);
		for (cp++; *cp; cp++) {
			printf("%c%s", SRCHLIST_SEP, *cp);
		}
		putchar('\n');
	}
	putchar('\n');
	if (nsw_trace)
		PrintPolicy();
}
#undef SRCHLIST_SEP


/*
 * Fake a reinitialization when the domain is changed.
 */
res_re_init()
{
	register char *cp, **pp;
	int n;

	/* find components of local domain that might be searched */
	pp = _res.dnsrch;
	*pp++ = _res.defdname;
	for (cp = _res.defdname, n = 0; *cp; cp++)
		if (*cp == '.')
			n++;
	cp = _res.defdname;
	for (; n >= LOCALDOMAINPARTS && pp < _res.dnsrch + MAXDFLSRCH; n--) {
		cp = strchr(cp, '.');
		*pp++ = ++cp;
	}
	*pp = 0;
	_res.options |= RES_INIT;
}


/*
 *******************************************************************************
 *
 *  Usage --
 *
 *	Lists the proper methods to run the program and exits.
 *
 *******************************************************************************
 */

Usage()
{
	fprintf(stderr, "Usage:\n");
	fprintf(stderr,
	    "   nslookup [-opt ...]             # interactive mode using default server\n");
	fprintf(stderr,
	    "   nslookup [-opt ...] - server    # interactive mode using 'server'\n");
	fprintf(stderr,
	    "   nslookup [-opt ...] host        # just look up 'host' using default server\n");
	fprintf(stderr,
	    "   nslookup [-opt ...] host server # just look up 'host' using 'server'\n");
	exit(1);
}


/* PART 2 - LocalServer */

LocalServer(defaultPtr)
HostInfo *defaultPtr;
{
	char	hostName[NAME_LEN];
	int		result;
#ifdef  PROC
	char  	*proc;
#endif

#ifdef  PROC
	proc = strchr(module, '.');
	if (proc!=NULL)
		*proc = '\0';
	proc = strcat(module,".LocalServer");
	printf("%s: Entered\n", proc);
#endif
	gethostname(hostName, sizeof(hostName));
	defaultAddr[lookup_type].s_addr = htonl(INADDR_ANY);
	/*
  ** Look up the name to force a name server query to happen. (Later
  ** checks depend on this.) Turn off the search so a fully qualified
  ** hostname(1) only causes 1 query.
  **
  ** If the BSD code is used with "0.0.0.0" as the name, no query will
  ** happen since this is a valid IP address.
  */
	_res.options &= ~RES_DNSRCH;
	result = GetHostInfoByName(&defaultAddr[lookup_type], C_IN, T_A, hostName, defaultPtr,1);
	_res.options |= RES_DNSRCH;
	if ((result != SUCCESS) && (lookup_type == NAMESERVER)) {
		struct in_addr *x;
		fprintf(stderr, "*** Can't find address for server %s: %s\n", 
		    hostName, DecodeError(result));
		/*
    ** There is a server running. It may not be able to give us the
    ** address of this host because it's database is corrupted. Fill in
    ** the address of THISHOST and the hostname and continue. At least
    ** the user will be able to see the failure packets. If the server
    ** is just not responding, the user can at least get into interactive
    ** mode and switch servers.
    */
		defaultPtr->name = (char *)Calloc(1, strlen(hostName)+1);
		strcpy(defaultPtr->name, hostName);
		defaultPtr->addrList = (char **) Calloc(2, sizeof(char *));
		x = (struct in_addr *) malloc(sizeof(struct in_addr));
		x->s_addr = inet_addr("0.0.0.0");
		defaultPtr->addrList[0] = (char *)x;
		defaultPtr->addrList[1] = NULL;
	}
}


/*
 *******************************************************************************
 *
 * IsAddr --
 *
 *	Returns TRUE if the string looks like an Internet address.
 *	A string with a trailing dot is not an address, even if it looks
 *	like one.
 *
 *	XXX doesn't treat 255.255.255.255 as an address.
 *
 *******************************************************************************
 */

Boolean
IsAddr(host, addrPtr)
char *host;
unsigned long *addrPtr;	/* If return TRUE, contains IP address */
{
	register char *cp;
	unsigned long addr;

	if (isdigit(host[0])) {
		/* Make sure it has only digits and dots. */
		for (cp = host; *cp; ++cp) {
			if (!isdigit(*cp) && *cp != '.')
				return FALSE;
		}
		/* If it has a trailing dot, don't treat it as an address. */
		if (*--cp != '.') {
			if ((addr = inet_addr(host)) != (unsigned long) -1) {
				*addrPtr = addr;
				return TRUE;
#if 0
			} else {
				/* XXX Check for 255.255.255.255 case */
#endif
			}
		}
	}
	return FALSE;
}

/* PART 3 - LookupHost */

/*
 *******************************************************************************
 *
 *  PolicyLookupHost --
 *
 *	Asks the name servers for information about the specified host or domain.
 *	The information is printed if the lookup was successful.
 *
 *  Results:
 *	ERROR		- the output file could not be opened.
 *	+ results of DoLookup
 *
 *******************************************************************************
 */

int
PolicyLookupHost(string, putToFile)
char	*string;
Boolean	putToFile;
{
	char	host[NAME_LEN];
	char	file[NAME_LEN];
	int		result;
	/* HP_NSSWITCH */
#ifdef notdef
	/* see comments in declarations above */
	struct  __nsw_lookup  *	p_lkup;
#endif
	int		ret_status;
	int	lustate;
#ifdef  PROC
	char  	*proc;
#endif

#ifdef  PROC
	proc = strchr(module, '.');
	if (proc!=NULL)
		*proc = '\0';
	proc = strcat(module, ".PolicyLookupHost");
	printf("%s\n", proc);
#endif
	lustate = lookup_type;
    /*
     *  Invalidate the current host information to prevent Finger 
     *  from using bogus info.
     */

	curHostValid = FALSE;

	/*
     *	 Parse the command string into the host and
     *	 optional output file name.
     *
     */

	sscanf(string, " %s", host);	/* removes white space */
	if (!putToFile) {
		filePtr = stdout;
	} else {
		filePtr = OpenFile(string, file);
		if (filePtr == NULL) {
			fprintf(stderr, "*** Can't open %s for writing\n", file);
			lookup_type = lustate;
			return(ERROR);
		}
		fprintf(filePtr,"> %s\n", string);
	}

	p_lkup=lnsw_policy->lookups;
	while (p_lkup!=NULL) {
		if (lookup_type == hlumap(__nsw_inteqv(p_lkup->service_name)))
			break;
		else 
			p_lkup=p_lkup->next;
	}
	if(lookup_type == NAMESERVER)
		PrintHostInfo(filePtr, "Name Server:", defaultPtr[lookup_type]);
	else if (lookup_type == YP)
		PrintHostInfo(filePtr, "Default NIS Server:", defaultPtr[lookup_type]);
	else if (lookup_type == HOSTTABLE)
		PrintHostInfo(filePtr, "Using /etc/hosts on:", defaultPtr[lookup_type]);
        result=0;

	while (p_lkup!=NULL) {
		switch(__nsw_inteqv(p_lkup->service_name)) {
		case    SRC_DNS :
			__nsw_src = SRC_DNS;
			lookup_type = hlumap(__nsw_src);
			ret_status = __NSW_NOTFOUND;
			tr_switch("lookup source is DNS\n");
			if (defaultPtr[lookup_type] != NULL) {
				if (nsw_trace)
					PrintHostInfo(filePtr, "Name Server:", defaultPtr[lookup_type]);
				ret_status = mapresult((result=DoLookup(host, defaultPtr[lookup_type],
				    defaultServer[lookup_type])));
			}
			break ;

		case    SRC_NIS :

			__nsw_src = SRC_NIS;
			lookup_type = hlumap(__nsw_src);
			ret_status = __NSW_NOTFOUND;
			tr_switch("lookup source is NIS\n");
			if (defaultPtr[lookup_type] != NULL) {
				if (nsw_trace)
					PrintHostInfo(filePtr, "Default NIS Server:", defaultPtr[lookup_type]);
				ret_status = mapresult((result=DoLookup(host, defaultPtr[lookup_type],
				    defaultServer[lookup_type])));
			}
			break ;

		case    SRC_FILES :
			__nsw_src = SRC_FILES;
			lookup_type = hlumap(__nsw_src);
			tr_switch("lookup source is FILES\n");
			if (defaultPtr[lookup_type] != NULL) {
				if (nsw_trace)
					PrintHostInfo(filePtr, "Using /etc/hosts on:", defaultPtr[lookup_type]);
				ret_status = mapresult((result=DoLookup(host, defaultPtr[lookup_type],
				    defaultServer[lookup_type])));
			}
			break ;

		default :
			__nsw_src = SRC_UNKNOWN;
			ret_status = __NSW_UNAVAIL;
			tr_switch("Unknown source - unavailable\n");
			result=0;

			break ;
		} /* end of switch */

		if (p_lkup->action[ret_status] == __NSW_RETURN)
			break;
	        p_lkup=p_lkup->next;
		if (p_lkup != NULL && nsw_trace)
			printf("\nSwitching to next source in the policy\n");
	} /* for lookup */

	if (putToFile) {
		fclose(filePtr);
		filePtr = NULL;
	}
	lookup_type = lustate;
	return(result);
}


/*
 *******************************************************************************
 *
 *  LookupHost --
 *
 *	Asks the default name server for information about the
 *	specified host or domain. The information is printed
 *	if the lookup was successful.
 *
 *  Results:
 *	ERROR		- the output file could not be opened.
 *	+ results of DoLookup
 *
 *******************************************************************************
 */

int
LookupHost(string, putToFile)
char	*string;
Boolean	putToFile;
{
	char	host[NAME_LEN];
	char	file[NAME_LEN];
	int		result;


	/*
     *  Invalidate the current host information to prevent Finger 
     *  from using bogus info.
     */

	curHostValid = FALSE;

	/*
     *	 Parse the command string into the host and
     *	 optional output file name.
     *
     */

	sscanf(string, " %s", host);	/* removes white space */
	if (!putToFile) {
		filePtr = stdout;
	} else {
		filePtr = OpenFile(string, file);
		if (filePtr == NULL) {
			fprintf(stderr, "*** Can't open %s for writing\n", file);
			return(ERROR);
		}
		fprintf(filePtr,"> %s\n", string);
	}

#ifdef hpux
	if(lookup_type == NAMESERVER)
		PrintHostInfo(filePtr, "Name Server:", defaultPtr[lookup_type]);
	else if (lookup_type == YP)
		PrintHostInfo(filePtr, "Default NIS Server:", defaultPtr[lookup_type]);
	else if (lookup_type == HOSTTABLE)
		PrintHostInfo(filePtr, "Using /etc/hosts on:", defaultPtr[lookup_type]);
#else
	PrintHostInfo(filePtr, "Server:", defaultPtr);
#endif

	result = DoLookup(host, defaultPtr[lookup_type], defaultServer[lookup_type]);

	if (putToFile) {
		fclose(filePtr);
		filePtr = NULL;
	}
	return(result);
}


/*
 *******************************************************************************
 *
 * DoLoookup --
 *
 *	Common subroutine for LookupHost and LookupHostWithServer.
 *
 *  Results:
 *	SUCCESS		- the lookup was successful.
 *	Misc. Errors	- an error message is printed if the lookup failed.
 *
 *******************************************************************************
 */

static int
DoLookup(host, servPtr, serverName)
char	*host;
HostInfo	*servPtr;
char	*serverName;
{
	int result;
	struct in_addr *servAddrPtr;
	struct in_addr addr;
    	int nf_continue, su_continue, ua_continue, ta_continue;

	/* Skip escape character */
	if (host[0] == '\\')
		host++;

	/*
     *  If the user gives us an address for an address query, 
     *  silently treat it as a PTR query. If the query type is already
     *  PTR, then convert the address into the in-addr.arpa format.
     *
     *  Use the address of the server if it exists, otherwise use the
     *	address of a server who knows about this domain.
     *  XXX For now, just use the first address in the list.
     */

#ifdef hpux
	if(lookup_type == HOSTTABLE)
		servAddrPtr = NULL;
	else {
#endif
		if (servPtr->addrList != NULL) {
			servAddrPtr = (struct in_addr *) servPtr->addrList[0];
		} else {
			servAddrPtr = (struct in_addr *) servPtr->servers[0]->addrList[0];
		}
#ifdef hpux
	}
#endif

    nf_continue =  (p_lkup->action[__NSW_NOTFOUND] == __NSW_CONTINUE);
    su_continue =  (p_lkup->action[__NSW_SUCCESS] == __NSW_CONTINUE);
    ua_continue =  (p_lkup->action[__NSW_UNAVAIL] == __NSW_CONTINUE);
    ta_continue =  (p_lkup->action[__NSW_TRYAGAIN] == __NSW_CONTINUE);

	/* 
     * RFC1123 says we "SHOULD check the string syntactically for a 
     * dotted-decimal number before looking it up [...]" (p. 13).
     */
	if (queryType == T_A && IsAddr(host, &addr.s_addr)) {
		result = GetHostInfoByAddr(servAddrPtr, &addr, &curHostInfo);
	} else {
		if (queryType == T_PTR) {
			CvtAddrToPtr(host);
		}
		result = GetHostInfoByName(servAddrPtr, queryClass, queryType, host, 
		    &curHostInfo, 0);
	}

	switch (result) {
	case SUCCESS:
		/*
	     *  If the query was for an address, then the &curHostInfo
	     *  variable can be used by Finger.
	     *  There's no need to print anything for other query types
	     *  because the info has already been printed.
	     */
		if ((queryType == T_A) && (nsw_trace || (!su_continue)))  {
			curHostValid = TRUE;
			PrintHostInfo(filePtr, "Name:", &curHostInfo);
		}
		break;

		/*
	 * No Authoritative answer was available but we got names
	 * of servers who know about the host.
	 */
	case NONAUTH:				/* Nameserver result only */
	    if ((!su_continue) || nsw_trace)
		PrintHostInfo(filePtr, "Name:", &curHostInfo);
	    break;

	case NO_INFO:		/* returned for both UNAVAIL & NOT_FOUND. */
				/* UNAVAIL is a strange duck, it is not   */
				/* checked. If it is UNAVAIL then the inital */
				/* finding of name sources at nslookup start */
				/* would have removed them from the list */
				/* nslookup is only as good as the status of */
				/* the sources at start up (e.g. resolv.conf */
				/* & nsswitch.conf changes are not seen again*/
				/* AMMENDED: set a flag if NIS was found */
				/* unavailable, this allows a more accurate */
				/* return value, and makes the switch work */
#ifdef hpux
		if (( lookup_type == YP) && (nis_unavailable))	/*ASH*/
	   	{	
		    result = TIME_OUT;	/* this will cause UNAVAIL */
		    if (nsw_trace || !ua_continue)
			fprintf(stderr, 
			    "*** No %s information is available for \"%s\"\n", 
			    (inet_addr(host)==(unsigned long)-1) ? "address" : 				    "hostname", host);
		    break;
		}

		if ((lookup_type != NAMESERVER) && (nsw_trace || !nf_continue))
		{

			fprintf(stderr, 
			    "*** No %s information is available for \"%s\"\n", 
			    (inet_addr(host)==(unsigned long)-1) ? "address" : "hostname",
			    host);
		}
		else
		    if (nsw_trace || !nf_continue)
#endif
			fprintf(stderr, "*** No %s (%s) records available for %s\n", 
			    DecodeType(queryType), p_type(queryType), host);
	    break;

	case TIME_OUT:				/* Nameserver result only */
	    if (nsw_trace || !ta_continue)
		fprintf(stderr, "*** Request to %s timed-out\n", serverName);
	    break;

	default:				/* Nameserver result only */
	    if (nsw_trace || !nf_continue)
			/* occurs only if NAMESERVER & NOTFOUND -ASH*/
		fprintf(stderr, "*** %s can't find %s: %s\n", serverName, host,
		    DecodeError(result));
	}
	return result;
}


/*
 *******************************************************************************
 *
 * CvtAddrToPtr --
 *
 *	Convert a dotted-decimal Internet address into the standard
 *	PTR format (reversed address with .in-arpa. suffix).
 *
 *	Assumes the argument buffer is large enougth to hold the result.
 *
 *******************************************************************************
 */

static void
CvtAddrToPtr(name)
char *name;
{
	char *p;
	int ip[4];
	struct in_addr addr;

	if (IsAddr(name, &addr.s_addr)) {
		p = inet_ntoa(addr);
		if (sscanf(p, "%d.%d.%d.%d", &ip[0], &ip[1], &ip[2], &ip[3]) == 4) {
			sprintf(name, "%d.%d.%d.%d.in-addr.arpa.", 
			    ip[3], ip[2], ip[1], ip[0]);
		}
	}
}

/* PART 4 - yylex */

/*
 *******************************************************************************
 *
 *  PrintHelp --
 *
 *	Prints out the help file.
 *	(Code taken from Mail.)
 *
 *******************************************************************************
 */

#ifndef hpux
void
PrintHelp()
{
	register int c;
	register FILE *helpFilePtr;

	if ((helpFilePtr = fopen(_PATH_HELPFILE, "r")) == NULL) {
		perror(_PATH_HELPFILE);
		return;
	}
	while ((c = getc(helpFilePtr)) != EOF) {
		putchar((char) c);
	}
	fclose(helpFilePtr);
}
#else
void
PrintHelp()
{
	printf("Commands: 	(identifiers are shown in uppercase, [] ");
	printf("means optional)\n");
	printf("NAME		- print info about the host/domain NAME ");
	printf("using default server\n");
	printf("NAME1 NAME2	- as above, but use NAME2 as server\n");
	printf("exit            - exit the program, ^D also exits\n");
	printf("finger [USER]   - finger the optional NAME at the current ");
	printf("default host\n");
	printf("help or ?	- print info on common commands; ");
	printf("see nslookup(1) for details\n");
	printf("ls [opt] DOMAIN [> FILE] - list addresses in DOMAIN ");
	printf("(optional: output to FILE)\n");
	printf("    -a          -  list canonical names and aliases\n");
	printf("    -h          -  list HINFO (CPU type and operating system)\n");
	printf("    -s          -  list well-known services\n");
	printf("    -d          -  list all records\n");
	printf("    -t TYPE     -  list records of the given type ");
	printf("(e.g., A,CNAME,MX, etc.)\n");
	printf("policy		- print switch policy information\n");
	printf("root		- set current default server to the root\n");
	printf("server NAME	- set default server to NAME, using current ");
	printf("default server\n");
	printf("lserver NAME	- set default server to NAME, using initial ");
	printf("server\n");
	printf("reset     	- lookups use the switch policy; resets ");
	printf("DNS servers\n");
	printf("set OPTION	- set an option\n");
	printf("    all		-  print options, current server and host\n");
	printf("    [no]debug	-  print debugging information\n");
	printf("    [no]d2	-  print exhaustive debugging information\n");
	printf("    [no]defname	-  append domain name to each query \n");
	printf("    [no]swtrace	-  print source lookup and source switch messages\n");
	printf("    [no]recurse	-  ask for recursive answer to query\n");
	printf("    [no]vc	-  always use a virtual circuit\n");
	printf("    domain=NAME	-  set default domain name to NAME\n");
	printf("    srchlist=N1[/N2/.../N6] -  set domain to N1 and search ");
	printf("list to N1,N2, etc.\n");
	printf("    root=NAME	-  set root server to NAME\n");
	printf("    retry=X	-  set number of retries to X\n");
	printf("    timeout=X	-  set time-out interval to X\n");
	printf("    querytype=X	-  set query type, e.g., A,ANY,CNAME,HINFO,");
	printf("MX,NS,PTR,SOA,WKS\n");
	printf("    type=X	-  synonym for querytype\n");
	printf("    class=X     -  set query class to one of IN (Internet), ");
	printf("CHAOS, HESIOD or ANY\n");
	printf("view FILE	- sort an 'ls' output file and view it with ");
	printf("more\n");
}

/*
 * Help screen for YP or /etc/hosts
 */
void
PrintHostTableHelp()
{
	printf("NAME		- print address information about NAME\n");
	printf("IP-ADDRESS	- print hostname information about IP-ADDRESS\n");
	printf("policy		- print switch policy information\n");
	printf("server NAME	- set default server to NAME, using current ");
	printf("default server\n");
	printf("lserver NAME	- set default server to NAME, using initial ");
	printf("server\n");
	printf("set OPTION	- sets the OPTION \n");
	printf("    all		-  print options, current server and host\n");
	printf("    [no]swtrace	-  print lookup result and lookup switch messages\n");
}

void
PrintPolicy()
{
	int	savedflag;

	savedflag = __nsw_debug;
	__nsw_debug = 1;
	__nsw_dumpconfig(lnsw_policy);
	__nsw_debug = savedflag;
}
#endif


/*
 *******************************************************************************
 *
 *  SetDefaultServer --
 *
 *	Changes the default name server to the one specified by the first
 *	argument. The command "server name" uses the current default server
 *	to lookup the info for "name". The command "lserver name" uses the
 *	original server to lookup "name".
 *
 *  Side effects:
 *	This routine will cause a core dump if the allocation requests fail.
 *
 *  Results:
 *	SUCCESS		The default server was changed successfully.
 *	NONAUTH		The server was changed but addresses of
 *			other servers who know about the requested server
 *			were returned.
 *	Errors		No info about the new server was found or
 *			requests to the current server timed-out.
 *
 *******************************************************************************
 */

int
SetDefaultServer(string, local)
char	*string;
Boolean	local;
{
	register HostInfo	*newDefPtr;
	struct in_addr	*servAddrPtr;
	struct in_addr	addr;
	char		newServer[NAME_LEN];
	int			result;
	int			i;

	/*
     *  Parse the command line. It maybe of the form "server name",
     *  "lserver name" or just "name".
     */

	if (local) {
		i = sscanf(string, " lserver %s", newServer);
	} else {
		i = sscanf(string, " server %s", newServer);
	}
	if (i != 1) {
		i = sscanf(string, " %s", newServer);
		if (i != 1) {
			fprintf(stderr,"SetDefaultServer: invalid name: %s\n",  string);
			return(ERROR);
		}
	}

	/*
     * Allocate space for a HostInfo variable for the new server. Don't
     * overwrite the old HostInfo struct because info about the new server
     * might not be found and we need to have valid default server info.
     */

	newDefPtr = (HostInfo *) Calloc(1, sizeof(HostInfo));


	/*
     *	A 'local' lookup uses the original server that the program was
     *  initialized with.
     *
     *  Check to see if we have the address of the server or the
     *  address of a server who knows about this domain.
     *  XXX For now, just use the first address in the list.
     */
	if (lookup_type != NAMESERVER)
	{
	     printf("Specifying a server has overridden the switch policy order.\n");
	     printf("The reset command will reinstate the order specified by the switch policy.\n");
	    lookup_type = NAMESERVER;
	    server_specified++;
	}

	if (local) {
		servAddrPtr = &defaultAddr[lookup_type];
	} else if (defaultPtr[lookup_type]->addrList != NULL) {
		servAddrPtr = (struct in_addr *) defaultPtr[lookup_type]->addrList[0];
	} else {
		servAddrPtr = (struct in_addr *) defaultPtr[lookup_type]->servers[0]->addrList[0];
	}

	result = ERROR;
	if (IsAddr(newServer, &addr.s_addr)) {
		result = GetHostInfoByAddr(servAddrPtr, &addr, newDefPtr);
		/* If we can't get the name, fall through... */
	}
	if (result != SUCCESS && result != NONAUTH) {
		result = GetHostInfoByName(servAddrPtr, C_IN, T_A, 
		    newServer, newDefPtr, 1);
	}

	if (result == SUCCESS || result == NONAUTH) {
		/*
	     *  Found info about the new server. Free the resources for
	     *  the old server.
	     */

		FreeHostInfoPtr(defaultPtr[lookup_type]);
		free((char *)defaultPtr[lookup_type]);
		defaultPtr[lookup_type] = newDefPtr;
		strcpy(defaultServer[lookup_type], defaultPtr[lookup_type]->name);
#ifdef hpux
		PrintHostInfo(stdout, "Default Name Server:", defaultPtr[lookup_type]);
#else
		PrintHostInfo(stdout, "Default Server:", defaultPtr);
#endif
		return(SUCCESS);
	} else {
		fprintf(stderr, "*** Can't find address for server %s: %s\n",
		    newServer, DecodeError(result));
		free((char *)newDefPtr);

		return(result);
	}
}


/*
 *******************************************************************************
 *
 *  LookupHostWithServer --
 *
 *	Asks the name server specified in the second argument for 
 *	information about the host or domain specified in the first
 *	argument. The information is printed if the lookup was successful.
 *
 *	Address info about the requested name server is obtained
 *	from the default name server. This routine will return an
 *	error if the default server doesn't have info about the 
 *	requested server. Thus an error return status might not
 *	mean the requested name server doesn't have info about the
 *	requested host.
 *
 *	Comments from LookupHost apply here, too.
 *
 *  Results:
 *	ERROR		- the output file could not be opened.
 *	+ results of DoLookup
 *
 *******************************************************************************
 */

int
LookupHostWithServer(string, putToFile)
char	*string;
Boolean	putToFile;
{
	char	file[NAME_LEN];
	char	host[NAME_LEN];
	char	server[NAME_LEN];
	int		result;
	static HostInfo serverInfo;

	curHostValid = FALSE;

	sscanf(string, " %s %s", host, server);
	if (!putToFile) {
		filePtr = stdout;
	} else {
		filePtr = OpenFile(string, file);
		if (filePtr == NULL) {
			fprintf(stderr, "*** Can't open %s for writing\n", file);
			return(ERROR);
		}
		fprintf(filePtr,"> %s\n", string);
	}

	result = GetHostInfoByName(
	    defaultPtr[lookup_type]->addrList ?
	    (struct in_addr *) defaultPtr[lookup_type]->addrList[0] :
	    (struct in_addr *) defaultPtr[lookup_type]->servers[0]->addrList[0], 
	    C_IN, T_A, server, &serverInfo, 1);

	if (result != SUCCESS) {
		fprintf(stderr,"*** Can't find address for server %s: %s\n", server,
		    DecodeError(result));
	} else {
#ifdef hpux
		PrintHostInfo(filePtr, "Name Server:", &serverInfo);
#else
		PrintHostInfo(filePtr, "Server:", &serverInfo);
#endif

		result = DoLookup(host, &serverInfo, server);
	}
	if (putToFile) {
		fclose(filePtr);
		filePtr = NULL;
	}
	return(result);
}
void Reset()
{

    server_specified=0;
    _res.options = _res.options & (~RES_INIT);
    res_init();
    InitDefServers(0,NULL);
    printf("Now using this host's switch policy.\n");
}
