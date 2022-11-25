/*
 * Copyright (c) 1980 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */

#ifndef lint
static char sccsid[] = "@(#)$Revision: 66.9 $";	/* from 5.2 (Berkeley) 8/8/85 */
                                                /* and from 1.10 SMI 88/02/08 */
#endif not lint

/*
 * Turn quota on/off for a filesystem.
 */
#include <sys/param.h>
#include <sys/file.h>
#include <sys/quota.h>
#include <stdio.h>
#include <mntent.h>

int	vflag;		/* verbose */
int	aflag;		/* all file systems */

#define QFNAME "quotas"
char	quotafile[MAXPATHLEN + 1];
char	*listbuf[50];
char	*index(), *rindex();
char	*malloc();
char	*whoami;

main(argc, argv)
	int argc;
	char **argv;
{
	register struct mntent *mntp;
	char **listp;
	int listcnt;
	FILE *mtab, *fstab, *tmp;
	char *rindex();
	int offmode = 0;
	int errs = 0;
#ifndef GETMOUNT
	char *tmpname = "/etc/quotaontmpXXXXXX";
#endif
	extern int optind;
	int option;

	whoami = rindex(*argv, '/') + 1;
	if (whoami == (char *)1)
		whoami = *argv;
	if (strcmp(whoami, "quotaoff") == 0)
		offmode++;
	else if (strcmp(whoami, "quotaon") != 0) {
		fprintf(stderr, "Name must be quotaon or quotaoff not %s\n",
			whoami);
		exit(1);
	}
	while ((option = getopt(argc, argv, "av")) != EOF)
		switch (option) {
		case 'a':
			aflag++;
			break;
		case 'v':
			vflag++;
			break;
		default:
			usage();
		}
	while (optind--) {
		argc--;
		argv++;
	}

	/*
	 * If aflag go through fstab and make a list of appropriate
	 * filesystems.
	 */
	if (aflag) {
		listp = listbuf;
		listcnt = 0;
		fstab = setmntent(MNTTAB, "r");
		while (mntp = getmntent(fstab)) {
			if (strcmp(mntp->mnt_type, MNTTYPE_43) != 0 ||
			    !hasmntopt(mntp, MNTOPT_QUOTA) ||
			    hasmntopt(mntp, MNTOPT_RO))
				continue;
			*listp = malloc(strlen(mntp->mnt_dir) + 1);
			if (*listp == NULL) {
			    fprintf(stderr, "%s: ", whoami);
			    perror("malloc");
			    exit(1);
			}
			strcpy(*listp, mntp->mnt_dir);
			listp++;
			listcnt++;
		}
		endmntent(fstab);
		*listp = (char *)0;
		listp = listbuf;
	} else {
		if (argc == 0) usage();
		listp = argv;
		listcnt = argc;
	}

#ifndef GETMOUNT
	/*
	 * Open temporary version of mtab
	 */
	mktemp(tmpname);
	tmp = setmntent(tmpname, "a");
	if (tmp == NULL) {
		perror(tmpname);
		exit(1);
	}
#else
	update_mnttab();
#endif

	/*
	 * Open real mtab
	 */
	mtab = setmntent(MOUNTED, "r");
	if (mtab == NULL) {
		perror(MOUNTED);
		exit(1);
	}

	/*
	 * Loop through mtab calling quotaonoff()
	 */
	while ((mntp = getmntent(mtab)) != NULL) {
		if (strcmp(mntp->mnt_type, MNTTYPE_43) == 0 &&
		    !hasmntopt(mntp, MNTOPT_RO) &&
		    (oneof(mntp->mnt_fsname, listp, listcnt) ||
		     oneof(mntp->mnt_dir, listp, listcnt)) ) {
			errs += quotaonoff(mntp, offmode);
		}
#ifndef GETMOUNT
	 /* Write mount record to temp mtab. */
		addmntent(tmp, mntp);
#endif
	}
	endmntent(mtab);
#ifndef GETMOUNT
	endmntent(tmp);

	/*
	 * Move temp mtab to mtab
	 */
	if (rename(tmpname, MOUNTED) < 0) {
		perror(MOUNTED);
		exit(1);
	}
#else
	update_mnttab();
#endif

	while (listcnt--) {
		if (*listp)
			fprintf(stderr, "Cannot do %s\n", *listp);
		listp++;
	}
	exit(errs);
}

quotaonoff(mntp, offmode)
	register struct mntent *mntp;
	int offmode;
{

	if (offmode) {
		if (quotactl(Q_QUOTAOFF, mntp->mnt_fsname, 0, NULL) < 0)
			goto bad;
		if (vflag)
			printf("%s: quotas turned off\n", mntp->mnt_dir);
	} else {
		(void) sprintf(quotafile, "%s/%s", mntp->mnt_dir, QFNAME);
		if (quotactl(Q_QUOTAON, mntp->mnt_fsname, 0, quotafile) < 0)
			goto bad;
		if (vflag)
			printf("%s: quotas turned on\n", mntp->mnt_dir);
	}
#ifndef GETMOUNT
	/*
	 * update the mount record before writing it.
	 */
	fixmntent(mntp, offmode);
#endif  /* GETMOUNT */
	return (0);
bad:
	fprintf(stderr, "quotactl: ");
	perror(mntp->mnt_fsname);
	return (1);
}

oneof(target, listp, n)
	char *target;
	register char **listp;
	register int n;
{

	while (n--) {
		if (*listp && strcmp(target, *listp) == 0) {
			*listp = (char *)0;
			return (1);
		}
		listp++;
	}
	return (0);
}

#ifndef GETMOUNT
char opts[1024];

fixmntent(mntp, offmode)
	register struct mntent *mntp;
	int offmode;
{
	register char *qst, *qend;

	if (offmode) {
		if (hasmntopt(mntp, MNTOPT_NOQUOTA))
			return;
		qst = hasmntopt(mntp, MNTOPT_QUOTA);
	} else {
		if (hasmntopt(mntp, MNTOPT_QUOTA))
			return;
		qst = hasmntopt(mntp, MNTOPT_NOQUOTA);
	}
	if (qst) {
		qend = index(qst, ',');
		if (qst != mntp->mnt_opts)
			qst--;			/* back up to ',' */
		if (qend == NULL)
			*qst = '\0';
		else
			while (*qst++ = *qend++);
	}
	sprintf(opts, "%s,%s", mntp->mnt_opts,
	    offmode? MNTOPT_NOQUOTA : MNTOPT_QUOTA);
	mntp->mnt_opts = opts;
}
#endif  /* GETMOUNT */

usage()
{
	fprintf(stderr,
		"Usage:\n\t%s [-v] -a\n\t%s [-v] filesys ...\n",
		whoami, whoami);
	exit(1);
}
