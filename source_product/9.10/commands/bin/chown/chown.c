static char *HPUX_ID = "@(#) $Revision: 70.2 $";
/*
 * chown [-R] owner[:group] file ...
 */

#include <stdio.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <pwd.h>
#include <grp.h>
#include <string.h>			/* for strchr */
#include "walkfs.h"
int	walkfs_fn();			/* walkfs function */

#ifndef NLS
#   define catgets(i,sn,mn,s) (s)
#else
#   define NL_SETN 1	/* set number */
#   include <nl_types.h>
#   include <nl_ctype.h>
#   include <langinfo.h>
#   include <locale.h>
#endif

struct	passwd	*pwd,*getpwnam();
struct	group	*gr, *getgrnam();

int	Rflag;				/* -R option */
int	status;				/* status of subroutine calls */
int	uid;				/* user id  */
int	gid = -1;			/* group id, default to -1 (which */
					/* means "no change" when given to */
					/* chown() */
extern int errno;

main(argc, argv)
int argc;
char *argv[];
{

	extern	char	*optarg; 	/* getopt */
	extern	int	optind, opterr; /* getopt */
	char	letter;			/* getopt current key letter */
	char	*gid_pos;		/* position of group id if given */
					/* owner:group style argument */

#	ifdef NLS
		nl_catd nlmsg_fd;
#	endif

	register int c;
	struct	walkfs_info *info;

#       ifdef NLS || NLS16
		if (!setlocale(LC_ALL, ""))
		{
			fputs(_errlocale("chown"), stderr);
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
		nlmsg_fd = catopen("chown",0);
#endif

		fprintf(stderr, (catgets(nlmsg_fd,NL_SETN,1,"usage: chown [-R] owner[:group] file ...\n")));
		exit(4);
	}

	if (gid_pos = strchr(argv[optind], ':')) {
		/* if we get in here, that means the user gave us an option
		   which looked like owner:group instead of just owner.  (He
		   is allowed to do this under POSIX.2 */

		*gid_pos++ = '\0';	       /* zap string at owner part */
					       /* but remember where gid_pos */
					       /* is pointing to */
        }

	if(isnumber(argv[optind])) {
		uid = atoi(argv[optind]);
	}
	else {

		if((pwd=getpwnam(argv[optind])) == NULL) {
#ifdef NLS
			nlmsg_fd = catopen("chown",0);
#endif
			fprintf(stderr, (catgets(nlmsg_fd,NL_SETN,2,"chown: unknown user id %s\n")),argv[optind]);
			exit(4);
		}
		uid = pwd->pw_uid;
	}

	/* now parse the group part, if we have one */
	if (gid_pos) {
		if(isnumber(gid_pos)) {
			gid = atoi(gid_pos);
		}
		else {

			if((gr=getgrnam(gid_pos)) == NULL) {
#ifdef NLS
				nlmsg_fd = catopen("chown",0);
#endif
				fprintf(stderr, (catgets(nlmsg_fd,NL_SETN,3,"chown: unknown group id %s\n")),gid_pos);
				exit(4);
			}
			gid = gr->gr_gid;
		}
	}

	for(c=optind+1; c<argc; c++) {
		if (Rflag) {		/* recursively descend */
			walkfs(argv[c], walkfs_fn, _NFILE-3,
			    WALKFS_LSTAT|WALKFS_DOCDF);
		}
		else if(chown(argv[c], uid, gid) < 0) { /* named ones only */
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
		if(chown(info->shortpath, uid, gid) < 0) {
			perror(info->relpath);
			status = 1;
		}
		break;

	}
	return WALKFS_OK;
}
