/* @(#) $Revision: 38.1 $ */     
static char *HPUX_ID = "@(#) $Revision: 38.1 $";
#include "s500defs.h"
#include <stdio.h>
#include <ctype.h>
#include <sys/stat.h>
#include <pwd.h>

struct	passwd	*pwd,*getpwnam();
struct	stat	stbuf;
int	uid;
int	status;
char 	*pname;

#ifdef	DEBUG
int debug = 0;
#endif	DEBUG

main(argc, argv)
char *argv[];
{
	register c;

	pname = argv[0];

	if(argc < 3) {
		fprintf(stderr, "usage: %s uid device:file [...]\n", pname);
		exit(4);
	}
	if(isnumber(argv[1])) {
		uid = atoi(argv[1]);
		goto cho;
	}
	if((pwd=getpwnam(argv[1])) == NULL) {
		fprintf(stderr, "%s: unknown user id %s\n", pname,argv[1]);
		exit(4);
	}
	uid = pwd->pw_uid;

cho:
	for(c=2; c<argc; c++) {
		sdfstat(argv[c], &stbuf);
		if(sdfchown(argv[c], uid, stbuf.st_gid) < 0) {
			perror(argv[c]);
			status = 1;
		}
	}
	exit(status);
}

isnumber(s)
char *s;
{
	register c;

	while(c = *s++)
		if(!isdigit(c))
			return(0);
	return(1);
}
