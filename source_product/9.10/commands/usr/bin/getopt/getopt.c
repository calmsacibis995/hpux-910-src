static char *HPUX_ID = "@(#) $Revision: 70.2 $";

#ifndef NLS  		/* NLS must be defined */
#define catgets(i,sn,mn,s) (s)
#else NLS
#define NL_SETN 1
#include <nl_types.h>
#endif NLS

#include	<stdio.h>
#include	<sys/param.h>
char	*strchr();

main(argc, argv)
int argc;
char **argv;
{
	extern	int optind;
	extern	char *optarg;
	register int	c;
	int	errflg = 0;
	char	tmpstr[4];
	char	outstr[NCARGS];
	char	*goarg;
#ifdef NLS
	nl_catd nlmsg_fd;

	nlmsg_fd = catopen( "getopt" );
#endif NLS

	if(argc < 2) {
		fputs((catgets(nlmsg_fd,NL_SETN,1,"usage: getopt legal-args $*\n")), stderr);
		exit(2);
	}

	goarg = argv[1];
	argv[1] = argv[0];
	argv++;
	argc--;
	outstr[0] = '\0';   /* initialize outstr. This is AT&T V.3 code!! */

	while((c=getopt(argc, argv, goarg)) != EOF) {
		if(c=='?') {
			errflg++;
			continue;
		}

		tmpstr[0] = '-';
		tmpstr[1] = c;
		tmpstr[2] = ' ';
		tmpstr[3] = '\0';

		strcat(outstr, tmpstr);

		if(*(strchr(goarg, c)+1) == ':') {
			strcat(outstr, optarg);
			strcat(outstr, " ");
		}
	}

	if(errflg) {
		exit(2);
	}

	strcat(outstr, "-- ");
	while(optind < argc) {
		strcat(outstr, argv[optind++]);
		strcat(outstr, " ");
	}

	fputs(outstr, stdout);
	fputc('\n', stdout);
	exit(0);
}
