/* where_main.c 1.1 87/03/16 NFSSRC */
#include <stdio.h>
#include <sys/param.h>
#include <sys/stat.h>

/* We do not ship this command so it has not been localized 	*/
/* The nlmsg_fd declaration is needed because we compile this 	*/
/* file with where.c which has been localized since it is used	*/
/* by on.c. 							*/

#ifdef NLS
#define NL_SETN 1	/* set number */
#include <nl_types.h>
nl_catd nlmsg_fd;
#else
#define catgets(i, sn,mn,s) (s)
#endif NLS

extern errno, sys_nerr;
extern char *sys_errlist[];

#define errstr() (errno < sys_nerr ? sys_errlist[errno] : "unknown error")

/*
 * where(pn, host, fsname, within)
 *
 * pn is the pathname we are looking for,
 * host gets the name of the host owning the file system,
 * fsname gets the file system name on the host,
 * within gets whatever is left from the pathname
 *
 * Returns: 0 if ERROR, 1 if OK and -1 is directory is accessed via RFA.
 */
where(pn, host, fsname, within)
	char *pn;
	char *host;
	char *fsname;
	char *within;
{
	struct stat sb;
	char curdir[MAXPATHLEN];
	char qualpn[MAXPATHLEN];
	char *p, *rindex();

	if (stat(pn, &sb) < 0) {
		strcpy(within, errstr());
		return (0);
	}
	/*
	 * first get the working directory,
	 */
	if (getwd(curdir) == 0) {
		sprintf(within, (catgets(nlmsg_fd,NL_SETN,42, "Unable to get working directory (%s)")),
			curdir);
		return (0);
	}
	if (chdir(pn) == 0) {
		getwd(qualpn);
		chdir(curdir);
	} else {
		if (p = rindex(pn, '/')) {
			*p = 0;
			chdir(pn);
			(void) getwd(qualpn);
			chdir(curdir);
			strcat(qualpn, "/");
			strcat(qualpn, p+1);
		} else {
			strcpy(qualpn, curdir);
			strcat(qualpn, "/");
			strcat(qualpn, pn);
		}
	}
	return findmount(qualpn, host, fsname, within);
}

main(argc, argv)
	char **argv;
{
	char host[MAXPATHLEN];
	char fsname[MAXPATHLEN];
	char within[MAXPATHLEN];
	char *pn;
	int many;

	many = argc > 2;
	while (--argc > 0) {
		pn = *++argv;
		where(pn, host, fsname, within);
		if (many)
			printf("%s:\t", pn);
		printf("%s:%s%s\n", host, fsname, within);
	}
}

