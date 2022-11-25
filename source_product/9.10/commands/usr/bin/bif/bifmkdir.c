static char *HPUX_ID = "@(#) $Revision: 27.2 $";
/*
** make directory
** (BELL FILE SYSTEM VERSION)
*/

/* #include	<signal.h> */
#include	<stdio.h>

int	Errors = 0;
char	*strcat();
char	*strcpy();
char	*strchr();
char	*strrchr();

int debug=0;
char *pname;
char *bfstail();

extern char * normalize();

main(argc, argv)
char *argv[];
{

/*	signal(SIGHUP, SIG_IGN);
	signal(SIGINT, SIG_IGN);
	signal(SIGQUIT, SIG_IGN);
	signal(SIGPIPE, SIG_IGN);
	signal(SIGTERM, SIG_IGN); */

	pname = argv[0];

	if(argc < 2) {
		fprintf(stderr, "%s: arg count\n", pname);
		exit(2);
	}
	while(--argc)
		mkdir(*++argv);
	exit(Errors? 2: 0);
}

mkdir(dir)
register char *dir;
{
	char papa[128], dot[128], dotdot[128];

	dir = normalize(dir);
	if (!bfspath(dir)) {
		fprintf(stderr, "%s: use mkdir to make %s\n", pname, dir);
		++Errors;
		return;
	}

	/* Create the directory itself. */
	if ((bfsmknod(dir, 040777, 0)) < 0) {
		perror(pname);
		fprintf(stderr,"%s: cannot make directory %s\n", pname, dir);
		++Errors;
		return;
	}
	bfschown(dir, getuid(), getgid());

	/* Link the directory to dir/. */
	strcpy(dot, dir);
	strcat(dot, "/.");
	if ((bfslink(dir, dot)) < 0) {
		perror(pname);
		fprintf(stderr, "%s: cannot link %s to %s\n",
			pname, dir, dot);
		bfsunlink(dir);
		++Errors;
		return;
	}

	/* Construct name of parent directory */
	strcpy(papa, dir);			/* copy in entire name */
	strcpy(bfstail(papa), ".");		/* replace tail with . */

	/* Link dir/.. to papa */
	strcpy(dotdot, dir);
	strcat(dotdot, "/..");
	if((bfslink(papa, dotdot)) < 0) {
		perror(pname);
		fprintf(stderr, "%s: cannot link %s to %s\n",
			pname, papa, dotdot);
		bfsunlink(dot);		/* remove dir/. */
		bfsunlink(dir);		/* remove dir itself */
		++Errors;
	}
}
