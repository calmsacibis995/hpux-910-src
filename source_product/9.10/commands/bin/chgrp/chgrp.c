static char *HPUX_ID = "@(#) $Revision: 70.2 $";
/*
 * chgrp [-R] group file ...
 */

#include <stdio.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <grp.h>
#include "walkfs.h"
int     walkfs_fn();                    /* walkfs function */

#ifndef NLS
#   define catgets(i,sn,mn,s) (s)
#else
#   define NL_SETN 1	/* set number */
#   include <nl_types.h>
#   include <nl_ctype.h>
#   include <langinfo.h>
#   include <locale.h>
#endif

struct	group	*gr,*getgrnam();
struct	stat	stbuf;

int	Rflag;				/* -R option */
int	status;				/* status of subroutine calls */
int	gid;				/* group id */

main(argc, argv)
int argc;
char *argv[];
{

	extern	char	*optarg; 	/* getopt */
	extern	int	optind, opterr; /* getopt */
	char	letter;			/* getopt current key letter */
#	ifdef NLS
		nl_catd nlmsg_fd;
#	endif

	register int c;

        /* call setlocale - catopen is called later on */

#       ifdef NLS || NLS16
		if (!setlocale(LC_ALL, ""))
		{
			fputs(_errlocale("chgrp"), stderr);
			putenv("LANG=");
			nlmsg_fd = (nl_catd)-1;
		}
#       endif

	while ((letter = getopt(argc, argv, "R")) != EOF) {
		switch(letter) {

		case 'R':
			if(!Rflag) 	/* recursively descend directories */
				Rflag = 1;
			break;
		}
	}

	if(argc < optind + 2) {
#ifdef NLS
		nlmsg_fd = catopen("chgrp",0);
#endif
		fprintf(stderr,(catgets(nlmsg_fd,NL_SETN,1,"usage: chgrp [-R] group file ...\n")));
		exit(4);
	}

	if(isnumber(argv[optind])) {
		gid = atoi(argv[optind]);
	}
	else {
#ifdef NLS
		nlmsg_fd = catopen("chgrp",0);
#endif
		if((gr=getgrnam(argv[optind])) == NULL) {
			fprintf(stderr,(catgets(nlmsg_fd,NL_SETN,2,"chgrp: unknown group: %s\n")),argv[optind]);
			exit(4);
		}
		gid = gr->gr_gid;
	}

	for(c=optind+1; c<argc; c++) {
		if (Rflag) {		/* recursively descend */
			walkfs(argv[c], walkfs_fn, _NFILE-3,
			    WALKFS_LSTAT|WALKFS_DOCDF);
		}
		if(chown(argv[c], -1, gid) < 0) { /* named ones only */
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
		if(!isdigit((unsigned char)c))
			return(0);
	return(1);
}

walkfs_fn(info, flag)
	struct walkfs_info *info;
	unsigned int flag;
{
	switch(flag) {

	case WALKFS_NONDIR:		/* falls thru to WALKFS_DIR */
	case WALKFS_DIR:
		if(chown(info->shortpath, -1, gid) < 0) {
			perror(info->relpath);
			status = 1;
		}
		break;

	}
	return WALKFS_OK;
}
