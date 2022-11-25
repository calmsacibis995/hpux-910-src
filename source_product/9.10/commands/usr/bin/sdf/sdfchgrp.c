/* @(#) $Revision: 38.1 $ */     
static char *HPUX_ID = "@(#) $Revision: 38.1 $";

#include "s500defs.h"
#include <stdio.h>
#include <ctype.h>
#include <sys/stat.h>
#include <grp.h>

struct	group	*gr,*getgrnam();
struct	stat	stbuf;
int	gid;
int	status;
char	*pname;

#ifdef	DEBUG
int debug = 0;
#endif	DEBUG

main(argc, argv)
char *argv[];
{
	register c;

	pname = argv[0];
	if(argc < 3) {
		fprintf(stderr,"Usage: %s gid device:file [...]\n", pname);
		exit(4);
	}
	if(isnumber(argv[1])) {
		gid = atoi(argv[1]);
	}
	else {
		if((gr=getgrnam(argv[1])) == NULL) {
			fprintf(stderr,"%s: unknown group: %s\n", pname,argv[1]);
			exit(4);
		}
		gid = gr->gr_gid;
	}
	for(c=2; c<argc; c++) {
		sdfstat(argv[c], &stbuf);
		if(sdfchown(argv[c], stbuf.st_uid, gid) < 0) {
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
