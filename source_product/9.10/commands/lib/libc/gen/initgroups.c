/* @(#) $Revision: 66.1 $ */

/*
 * initgroups
 */

/*  Lines added to clean up ANSI/POSIX namespace */
#ifdef _NAMESPACE_CLEAN
#define fputs	   _fputs
#define fgetgrent  _fgetgrent
#define strcmp	   _strcmp
#define setgroups  _setgroups
#define fopen	   _fopen
#define fclose	   _fclose
#define perror	   _perror
#define rewind	   _rewind
#define initgroups _initgroups
#define setgrent   _setgrent
#define getgrent   _getgrent
#define endgrent   _endgrent
#define stat	   _stat
#endif

#include <stdio.h>
#include <sys/param.h>
#include <grp.h>
#include <sys/types.h>
#include <sys/stat.h>

extern int fclose();
extern FILE *fopen();
extern void rewind();

static char GROUP[] = "/etc/logingroup";
static char YPGROUP[] = "/etc/group";
static FILE *grf = NULL;

static void
fsetgrent()
{
	if (grf == NULL)
		grf = fopen(GROUP, "r");
	else
		rewind(grf);
}

static void
fendgrent()
{
	if (grf != NULL) {
		(void) fclose(grf);
		grf = NULL;
	}
}

static int
grplink()
{
    struct stat gbuf, ypgbuf;

    if (stat(GROUP, &gbuf) == -1 || stat(YPGROUP, &ypgbuf) == -1)
	return 0;
    return gbuf.st_ino == ypgbuf.st_ino && gbuf.st_dev == ypgbuf.st_dev;
}

#ifdef _NAMESPACE_CLEAN
#undef initgroups
#pragma _HP_SECONDARY_DEF _initgroups initgroups
#define initgroups _initgroups
#endif /* _NAMESPACE_CLEAN */

initgroups(uname, agroup)
	char *uname;
	int agroup;
{
	static char name[] = "initgroups: ";
	static char msg[] = " is in too many groups\n";
	extern struct group *fgetgrent();
	extern struct group *getgrent();

	register struct group *grp;
	register int i;
	int ngroups = 0;
	int groups[NGROUPS];

	if (agroup >= 0)
		groups[ngroups++] = agroup;
	if (grplink()) {
		/* use /etc/group for access to yp database */
		setgrent();
		while ((grp = getgrent()) != NULL) {
			if (grp->gr_gid == agroup)
				continue;
			for (i = 0; grp->gr_mem[i]; i++)
				if (!strcmp(grp->gr_mem[i], uname)) {
					if (ngroups == NGROUPS) {
						fputs(name, stderr);
						fputs(uname, stderr);
						fputs(msg, stderr);
						goto toomanyyp;
					}
					groups[ngroups++] = grp->gr_gid;
					break;
				}
		}
toomanyyp:
		endgrent();
	}
	else {
		fsetgrent();
		if (grf) {
			while ((grp = fgetgrent(grf)) != NULL) {
				if (grp->gr_gid == agroup)
					continue;
				for (i = 0; grp->gr_mem[i]; i++)
					if (!strcmp(grp->gr_mem[i], uname)) {
						if (ngroups == NGROUPS) {
						    fputs(name, stderr);
						    fputs(uname, stderr);
						    fputs(msg, stderr);
						    goto toomany;
						}
						groups[ngroups++] = grp->gr_gid;
						break;
					}
			}
		}
toomany:
		fendgrent();
	}
	if (setgroups(ngroups, groups) < 0) {
		perror("setgroups");
		return (-1);
	}
	return (0);
}
