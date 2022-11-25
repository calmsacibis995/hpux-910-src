static char *HPUX_ID = "@(#) $Revision: 27.1 $";
/*
 * chgrp gid file ...
 * (BELL FILE SYSTEM VERSION)
 */

#include <stdio.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <grp.h>

struct	group	*gr,*getgrnam();
struct	stat	stbuf;
int	gid;
int	status;
int	debug=0;
char	*pname;

extern char * normalize();

main(argc, argv)
char *argv[];
{
	register c;

	pname = argv[0];
	if(argc < 3) {
		fprintf(stderr,"%s: usage: %s gid file ...\n", pname);
		exit(4);
	}
	if(isnumber(argv[1])) {
		gid = atoi(argv[1]);
	} else {
		if((gr=getgrnam(argv[1])) == NULL) {
			fprintf(stderr,"%s: unknown group: %s\n", pname,argv[1]);
			exit(4);
		}
		gid = gr->gr_gid;
	}
	for(c=2; c<argc; c++) {
		argv[c] = normalize(argv[c]);
		bfsstat(argv[c], &stbuf);
		if(bfschown(argv[c], stbuf.st_uid, gid) < 0) {
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
