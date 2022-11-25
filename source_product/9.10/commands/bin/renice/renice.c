/*
 *                             renice(1)
 * Change the priority (nice value) of processes or groups of processes which 
 * are already running.   The code is based on the BSD source; but since 
 * the main routine was essentially rewritten to make the command POSIX.2a/D8
 * compliant, the BSD copyright notice was removed (this source doesn't look
 * much like the BSD  source anymore, and certainly behaves differently.  
 * These changes were mainly resulted for using getopt(3) for option parsing 
 * (the BSD original source did NOT use getopt).  The use of getopt 
 * fundamentally changed how the command now works.  The runstring is now:
 *
 *       renice [-n offset] [-g | -p | -u] ID...
 *
 * Defaults are: offset=10  and  ID interpreted as pid.
 * Alexander Leontiev  3/12/92
 */

static char *HPUX_ID = "@(#) $Revision: 70.2 $";

#include <sys/time.h>
#include <sys/resource.h>
#include <stdio.h>
#include <pwd.h>

extern int errno;
extern char *sys_errlist[];
extern char *optarg;
extern int opterr, optind;

#define NICE_DEFAULT 10  /* This value is based on nice default. If that
			    value changes for nice(1), update this. */

void usage()
{
	fprintf(stderr, "usage: renice [-n offset] [ -g | -p | -u ] ID...\n");
	exit(1);
}
/*
 * Change the priority (nice) of processes
 * or groups of processes which are already
 * running.
 */
main(argc, argv)
	int argc;
	char **argv;
{
	int which = -100, c;
	int who = 0, offset=NICE_DEFAULT, errs = 0;
	char *list;

	opterr = 0; 
	while ((c=getopt(argc,argv,"gn:pu"))!=EOF) {
		switch (c) {
		  case 'g' :
			if (which==PRIO_PROCESS || which==PRIO_USER) 
				usage();
			which = PRIO_PGRP;
			break;
		  case 'n' : 
			errno = 0;
			offset = atoi(optarg);
			if ( errno!=0 )
				usage();
			if (offset > PRIO_MAX)  offset = PRIO_MAX;
			if (offset < PRIO_MIN)  offset = PRIO_MIN;
			break;
		  case 'p' :
			if (which==PRIO_PGRP || which==PRIO_USER) 
				usage();
			which = PRIO_PROCESS;
			break;
		  case 'u' :
			if (which==PRIO_PGRP || which==PRIO_PROCESS) 
				usage();
			which = PRIO_USER;
			break;
		  case '?' :
			usage();
		}
	}

/* 
 * We at least need one parameter, i.e., a pocess id. If one not there,
 * print the usage error messsage.
*/
	if (optind == argc)  usage();

	if (which == -100) which = PRIO_PROCESS;

	for (; optind < argc; optind++) {
	   if (which == PRIO_PROCESS || which==PRIO_PGRP) {
		errno = 0;
		who = atoi(argv[optind]);
		if (who<=0 && errno!=0) {
		    printf("renice: bad %s ID %s\n",(which==PRIO_PROCESS ?
		    "process":"group"), argv[optind]);
		    break;
		}
#ifdef DEBUG
		printf("about to renice process or group %s to %d\n",argv[optind], offset);
#endif

		errs += donice (which, who, offset);
		continue;
	   }
	   if ( (who=atoi(argv[optind])) <= 0 ) {
	     	struct passwd *pwd = getpwnam(argv[optind]);
	     	if (pwd == NULL) {
		   fprintf(stderr,"renice: bad user ID %s\n",argv[optind]);
		   break;
	     	}
	     	who = pwd->pw_uid;
	   } 
#ifdef DEBUG
	   printf("about to renice user %s to %d\n",argv[optind], offset);
#endif
	   errs += donice (which, who, offset);
	}

	exit(errs != 0);
}

donice(which, who, offset)
	int which, who, offset;
{
	int oldoffset;

	errno = 0, oldoffset = getpriority(which, who);
	if (oldoffset == -1 && errno) {
		fprintf(stderr, "renice: %d: ", who);
		perror("getpriority");
		return (1);
	}
	if (setpriority(which, who, offset) < 0) {
		fprintf(stderr, "renice: %d: ", who);
		perror("setpriority");
		return (1);
	}
	printf("%d: old priority %d, new priority %d\n", who, oldoffset, offset);
	return (0);
}
