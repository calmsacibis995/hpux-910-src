#ifndef lint
static	char rcsid[] = "@(#)domainname:	$Revision: 1.30.109.1 $	$Date: 91/11/19 14:01:01 $";
#endif
/* sccsid[] = "(#)domainname.c 1.1 85/05/30 Copyr 1984 Sun Micro"; */
/*
 * domainname -- get (or set domainname)
 */

#ifndef NLS
#define catgets(i, sn,mn,s) (s)
#else NLS
#define NL_SETN 1	/* set number */
#include <nl_types.h>
#endif NLS

#include <stdio.h>
#include <sys/errno.h>
#include <sys/types.h>
#include <rpc/rpc.h>
#include <rpcsvc/yp_prot.h>
#include <sys/signal.h>

char domainname[YPMAXDOMAIN + 1];

extern int errno;
#ifdef NLS
nl_catd nlmsg_fd;
#endif NLS

struct sigvec oldsigvec;

main(argc,argv)
	char *argv[];
{
	int myerrno = 0;
	struct  sigvec  newsigvec;

#ifdef NLS
	nl_init(getenv("LANG"));
	nlmsg_fd = catopen("domainname",0);
#endif NLS

/* HPNFS On the s300 if setdomainname is not configured into the kernel */
/* HPNFS we do not return a SIGSYS, we return a ENOPROTOOPT.  On the	*/
/* HPNFS s800 we get a SIGSYS.  If SIGSYS is ignored, we get a EINVAL   */

	newsigvec.sv_handler = SIG_IGN;
	newsigvec.sv_mask = 0;
	newsigvec.sv_flags = 0;

	sigvector(SIGSYS, &newsigvec, &oldsigvec);

	argc--;
	argv++;
	if (argc) {		/*  Set the domain name.  */
		if (setdomainname(*argv,strlen(*argv))) {
			myerrno = errno;
			if ((errno == ENOPROTOOPT) || (errno == EINVAL))
				fprintf(stderr, (catgets(nlmsg_fd,NL_SETN, 4, 			"domainname: NFS system call is not available.\n            Please configure NFS into your kernel.\n")));
			else
				perror((catgets(nlmsg_fd,NL_SETN,1, "setdomainname")));
		}

	} else {		/*  Simply retrieve the domain name.  */
		if (getdomainname(domainname,sizeof(domainname))) {
			myerrno = errno;
			perror((catgets(nlmsg_fd,NL_SETN,3, "getdomainname")));
		} else 
		        printf((catgets(nlmsg_fd,NL_SETN,2,"%s\n")),domainname);
	}
	exit(myerrno);
}
