static char *HPUX_ID = "@(#) $Revision: 27.1 $";
/*
 * Remove directory
 *
 * (BELL FILE SYSTEM VERSION)
 */

#include <sys/types.h>
#include "dir.h"
#include <sys/stat.h>
#include <stdio.h>

int	Errors = 0;
char	*strchr();
char	*strrchr();
char	*strcat();
char	*strcpy();
char	*bfstail();

int debug=0;
char *pname;

main(argc,argv)
int argc;
char **argv;
{

	pname = argv[0];

	if(argc < 2) {
		fprintf(stderr, "usage: %s dirname ...\n", pname);
		exit(2);
	}
	while(--argc)
		rmdir(*++argv);
	exit(Errors?2:0);
}

extern char * normalize();

rmdir(d)
char *d;
{
	int	fd;
	char	*np, name[500];
	struct	stat	st, cst;
	struct	direct	dir;

	d = normalize(d);
	strcpy(name, d);
	np = bfstail(name);		/* last component of name */

	if(bfsstat(name,&st) < 0) {
		fprintf(stderr, "%s: %s non-existent\n", pname, name);
		++Errors;
		return;
	}
	if((st.st_mode & S_IFMT) != S_IFDIR) {
		fprintf(stderr, "%s: %s not a directory\n", pname, name);
		++Errors;
		return;
	}
	if((fd = bfsopen(name,0)) < 0) {
		fprintf(stderr, "%s: %s unreadable\n", pname, name);
		++Errors;
		return;
	}
	if (!bfsfile(fd)) {
		fprintf(stderr, "%s: use rmdir to process %s\n",
			pname, name);
		bfsclose(fd);
		++Errors;
		return;
	}
	while(bfsread(fd, (char *)&dir, sizeof dir) == sizeof dir) {
		if(dir.d_ino == 0) continue;
		if(!strcmp(dir.d_name, ".") || !strcmp(dir.d_name, ".."))
			continue;
		fprintf(stderr, "%s: %s not empty\n", pname, name);
		++Errors;
		bfsclose(fd);
		return;
	}
	bfsclose(fd);
	if(!strcmp(np, ".") || !strcmp(np, "..")) {
		fprintf(stderr, "%s: cannot remove . or ..\n", pname);
		++Errors;
		return;
	}
		strcat(name, "/.");
		bfsunlink(name);			/* unlink name/. */
		strcat(name, ".");
		bfsunlink(name);			/* unlink name/.. */

		name[strlen(name)-3] = '\0';		/* Strip trailing /.. */
		if (bfsunlink(name) < 0) {
			fprintf(stderr, "%s: %s not removed\n", pname, name);
			++Errors;
		}
}
