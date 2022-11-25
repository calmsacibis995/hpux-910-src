static char *HPUX_ID = "@(#) $Revision: 27.1 $";

/*
 * chown uid file ...
 * (BELL FILE SYSTEM VERSION)
 */

#include <stdio.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <pwd.h>

struct	passwd	*pwd,*getpwnam();
struct	stat	stbuf;
int	uid;
int	status;
char 	*pname;
int	debug;

extern char * normalize();

main(argc, argv)
char *argv[];
{
	register c;

	pname = argv[0];

	if(argc < 3) {
		fprintf(stderr, "usage: %s uid file ...\n", pname);
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
		argv[c] = normalize(argv[c]);
		bfsstat(argv[c], &stbuf);
		if(bfschown(argv[c], uid, stbuf.st_gid) < 0) {
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
