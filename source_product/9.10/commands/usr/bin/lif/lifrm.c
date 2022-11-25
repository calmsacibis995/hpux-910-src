/* @(#) $Revision: 66.1 $ */   

#include "lifdef.h"
#include "global.h"
#include <stdio.h>

extern char *Fatal[];
extern int DEBUG;

main(argc,argv)
register int argc;
register char *argv[];
{
	register char *ptr;
	register int argcount;
	register int n,filedis,errflag;
	register int flag = 0;
	char dirpath[MAXDIRPATH], filename[MAXFILENAME +1];

	if (argc>1 && strcmp(argv[1],"-D")==0) {
		DEBUG = TRUE;
		argc--;
		argv++;
	}

	if (argc<2)
		goto usage;
	for(argcount = 1; argcount < argc; argcount++){
		errflag = 0;
		ptr = argv[argcount];
		for (n = 0; (n <MAXDIRPATH && *ptr != ':' && !errflag); ptr++, n++){
			if (*ptr == '\0'){
				fprintf(stderr,"Use `rm' to remove HP-UX file %s.\n",argv[argcount]);
				flag = -1;
				errflag = 1;
			}
			dirpath[n] = *ptr;
		}
		if (errflag) continue;
		dirpath[n] = '\0';
		ptr++;
		for (n = 0; n < MAXFILENAME && *ptr != '\0'; ptr++, n++)
			filename[n] = *ptr;
		while (n < MAXFILENAME)
			filename[n++] = ' ';
		filename[n] = '\0';
		if ((filedis = open(dirpath,2)) < 0) {
			perror("lifrm(open)");
			flag = -1;
			continue;
		}
		n = lifpurge(dirpath, filename, filedis);
		if (n != TRUE){
			fprintf(stderr,"lifrm : Can not remove %s; %s\n",filename,Fatal[n] );
			flag = -1;
			continue;
		}

	}
	/* call closeit only if the file descriptor is valid */
	if (filedis < 0 || closeit(filedis) < 0)
		exit(-1);
	
	exit (flag);

usage :
	fprintf(stderr,"USAGE: lifrm dir:file1 ... dir:fileN \n");
	exit(-1);
}
