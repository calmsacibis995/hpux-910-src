/* @(#) $Revision: 49.1 $ */   
/*
 *	rename file specified by from to file specified by to
 */

/*	Modified:	31 January 1987 - Lee Casuto - ITG/ISO
 *			prrserve uid, gid, and mode of from file
 */
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
int
rename (from, to)
char *from, *to;
{
	struct stat buf;
	int i;
	static char opasswd[]="/etc/opasswd";

	i = stat(to, &buf);
	if(i != 0) {
		fprintf(stderr, "cannot stat %s\n",to);
		return(-1);
	}
	if (unlink(opasswd) && access(opasswd, 0) == 0) 
	{
		fprintf(stderr, "cannot unlink %s\n", opasswd);
		return(-1);
	}

        if (link(to, opasswd))
	{
		fprintf(stderr, "cannot link %s to %s\n", to, opasswd);
		return(-1);
	}

	if (unlink(to))
	{
		fprintf(stderr, "cannot unlink %s\n", to);
		return(-1);
	}

	if (link(from, to))
	{
		fprintf(stderr, "cannot link %s to %s\n", from, to);
		if (link(opasswd, to))
		{
			fprintf(stderr, "cannot recover %s\n", to);
			exit(1);
		}
		return(-1);
	}

	if (unlink(from))
	{
		fprintf(stderr,"cannot unlink %s\n", from);
		return(-1);
	}
	
	if(chown(to, buf.st_uid, buf.st_gid)) {
		fprintf(stderr,"cannot chown %s\n",to);
		return(-1);
	}

	if(chmod(to, buf.st_mode)) {
		fprintf(stderr,"cannot chmod %s\n",to);
		return(-1);
	}
	
}

