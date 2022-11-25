#ifndef lint
static char rcsid[] = "@(#)biod:	$Revision: 1.25.109.1 $	$Date: 91/11/19 14:01:26 $";
#endif

/*  static  char sccsid[] = "biod.c 1.1 85/05/30 Copyr 1983 Sun Micro"; */

#ifndef NLS
#define catgets(i, sn,mn,s) (s)
#else NLS
#define NL_SETN 1	/* set number */
#include <nl_types.h>
#endif NLS
#include <stdio.h>
#include <sys/types.h>
#include <sys/errno.h>
#include <sys/file.h>
#include <sys/ioctl.h>
#include <sys/signal.h>


/*
 * This is the NFS asynchronous block I/O daemon
 */

/*
 * If this command is run on a secure system, it assumes that it is
 * started by the "epa" command and will be given the default
 * privileges.  This is all that needs to be done to make this
 * program run on a secure HP-UX system
 */

#ifdef NLS
	nl_catd nlmsg_fd;
#endif NLS

struct sigvec oldsigvec;

main(argc, argv)
	int argc;
	char *argv[];
{
	extern int errno;
	int pid;
	int count;
	struct sigvec newsigvec;

#ifdef NLS
	nl_init(getenv("LANG"));
	nlmsg_fd = catopen("biod",0);
#endif NLS


/* HPNFS On the s300 if async_daemon is not configured into the kernel   */
/* HPNFS we do not return a SIGSYS, we get an errno ENOPROTOOPT.  On the */
/* HPNFS s800 we get a SYGSYS, which when ignored returns a EINVAL.      */

	newsigvec.sv_handler = SIG_IGN;
	newsigvec.sv_mask = 0;
	newsigvec.sv_flags = 0;

	sigvector(SIGSYS, &newsigvec, &oldsigvec);

	if (argc > 2) {
		usage(argv[0]);
	}

	if (argc == 2) {
		count = atoi(argv[1]);
		if (count < 0) {
			usage(argv[0]);
		}
	} else {
		count = 1;
	}

	setpgrp();
	while (count--) {
		pid = fork();
		if (pid == 0) {
/*
 *  HPNFS
 *
 *  The setpgrp seems to be required when using C shell.  The effect of
 *  job control, possibly?  Without the setpgrp call by the child, the
 *  child dies when the parent dies.  Dave Erickson 10-31-86.
 *
 *  HPNFS
 */
			setpgrp();
#ifdef BFA
			_UpdateBFA();
#endif BFA
			async_daemon();		/* Should never return */

/* HPNFS ENOPROTOOPT is the error the s300 returns if the system call */
/* HPNFS is not configured in.					      */
			if (count == 1)
				if ((errno == ENOPROTOOPT) || (errno == EINVAL))
					fprintf(stderr, (catgets(nlmsg_fd,NL_SETN, 4, "biod: NFS system call is not available.\n      Please configure NFS into your kernel.\n")));
				else
				{
					fprintf(stderr, (catgets(nlmsg_fd,NL_SETN,1, "%s: async_daemon ")), argv[0]);
					perror("");
				}
			exit(1);
		}
		if (pid < 0) {
			fprintf(stderr, (catgets(nlmsg_fd,NL_SETN,2, "%s: cannot fork")), argv[0]);
			perror("");
			exit(1);
		}
	}
/*
 *  HPNFS
 *
 *  The following sleep call has to be made, again under C shell, so the
 *  parent will not die before each of the children has a chance to do the
 *  setpgrp.  Without calling sleep, the parent might exit before some or
 *  all of the children become "unattached" (via setpgrp).  As a result,
 *  those children still "attached" die.  Dave Erickson, 10-31-86.
 *
 *  HPNFS
 */
	sleep (1);
}

usage(name)
	char	*name;
{

/*
 *  HPNFS
 *
 *  The "(count > 0)" is printed to be more informative.
 *  Dave Erickson, 10-31-86.
 *
 *  HPNFS
 */
	fprintf(stderr, (catgets(nlmsg_fd,NL_SETN,3, "usage: %s [<count>] (count >= 1)\n")), name);
	exit(1);
}
