/*
 * Copyright (c) 1980 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */

#ifndef lint
static char sccsid[] = "@(#)$Revision: 70.1.1.1 $";
/* from 5.6 (Berkeley) 11/3/85 */
/* and from 1.12 88/02/08 SMI */
#endif not lint

/*
 * Fix up / report on disc quotas & usage
 */
#include <stdio.h>
#include <ctype.h>
#include <signal.h>
#include <errno.h>
#include <sys/param.h>
#include <sys/time.h>
#include <sys/vnode.h>
#include <sys/inode.h>
#include <sys/ino.h>
#include <sys/fs.h>
#include <sys/quota.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <mntent.h>
#include <pwd.h>

#define MAXMOUNT	50

union {
	struct	fs	sblk;
	char	dummy[MAXBSIZE];
} un;
#define	sblock	un.sblk

#define	ITABSZ	512	/* Max number of inodes in a secondary storage block */
			/* Max block size is 64k, inode size is 128 bytes    */
struct	dinode	itab[ITABSZ];
struct	dinode	*dp;

#define LOGINNAMESIZE	8
struct fileusage {
	struct fileusage *fu_next;
	u_long fu_curfiles;
	u_long fu_curblocks;
	u_short	fu_uid;
	char fu_name[LOGINNAMESIZE + 1];
};
#define FUHASH 997
struct fileusage *fuhead[FUHASH];
struct fileusage *lookup();
struct fileusage *adduid();
int highuid;

int fi;
ino_t ino;
struct	dinode	*ginode();
char *malloc(), *makerawname();

int	vflag;		/* verbose */
int	aflag;		/* all file systems */
int	pflag;		/* fsck like parallel check */
int	Pflag;		/* check only the ones that are dirty */

#define Q_SETQCLEAN 25	/* undocumented quotactl command to mark FS_QCLEAN */
#define QFNAME "quotas"
char *listbuf[MAXMOUNT];
struct dqblk zerodqbuf;
struct fileusage zerofileusage;

main(argc, argv)
	int argc;
	char **argv;
{
	register struct mntent *mntp;
	register struct fileusage *fup;
	char **listp;
	int listcnt;
	char quotafile[MAXPATHLEN + 1];
	FILE *mtab, *fstab;
	int errs = 0;
	struct passwd *pw;
	extern int optind;
	int option;

	while ((option = getopt(argc, argv, "vPpa")) != EOF)
		switch (option) {
		case 'v':
			vflag++;
			break;
		case 'P':	/* check only the dirty ones */
			Pflag++;
		case 'p':
			pflag++;
			break;
		case 'a':
			aflag++;
			break;
		default:
			usage();
		}
	while (optind--) {
		argc--;
		argv++;
	}

	(void) setpwent();
	while ((pw = getpwent()) != 0) {
		fup = lookup((u_short)pw->pw_uid);
		if (fup == 0) {
			fup = adduid((u_short)pw->pw_uid);
			strncpy(fup->fu_name, pw->pw_name,
				sizeof(fup->fu_name));
		}
	}
	(void) endpwent();

	if (quotactl(Q_SYNC, NULL, 0, NULL) < 0 && errno == EINVAL && vflag)
		printf( "Warning: Quotas are not compiled into this kernel\n");
	sync();

	if (aflag) {
		/*
		 * Go through fstab and make a list of appropriate
		 * filesystems.
		 */
		listp = listbuf;
		listcnt = 0;
		if ((fstab = setmntent(MNTTAB, "r")) == NULL) {
			fprintf(stderr, "Can't open ");
			perror(MNTTAB);
			exit(8);
		}
		while (mntp = getmntent(fstab)) {
			if (strcmp(mntp->mnt_type, MNTTYPE_43) != 0 ||
			    !hasmntopt(mntp, MNTOPT_QUOTA) ||
			    hasmntopt(mntp, MNTOPT_RO))
				continue;
			*listp = malloc(strlen(mntp->mnt_dir) + 1);
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
	if (pflag) {
		errs = preen(listcnt, listp);
	} else {
		if ((mtab = setmntent(MOUNTED, "r")) == NULL) {
			fprintf(stderr, "Can't open ");
			perror(MOUNTED);
			exit(8);
		}
		while (mntp = getmntent(mtab)) {
			if (strcmp(mntp->mnt_type, MNTTYPE_43) == 0 &&
			    !hasmntopt(mntp, MNTOPT_RO) &&
			    (oneof(mntp->mnt_fsname, listp, listcnt) ||
			     oneof(mntp->mnt_dir, listp, listcnt)) ) {
				sprintf(quotafile,
				    "%s/%s", mntp->mnt_dir, QFNAME);
				errs +=
				    chkquota(mntp->mnt_fsname,
					mntp->mnt_dir, quotafile);
			}
		}
		endmntent(mtab);
	}
	while (listcnt--) {
		if (*listp)
			fprintf(stderr, "Cannot check %s\n", *listp);
	}
	exit(errs == 0 ? 0 : 2);
}

preen(listcnt, listp)
	int listcnt;
	char **listp;
{
	register struct mntent *mntp;
	register int passno, anygtr;
	register int errs;
	FILE *mtab;
	union wait status;
	char quotafile[MAXPATHLEN + 1];

	passno = 1;
	errs = 0;
	if ((mtab = setmntent(MOUNTED, "r")) == NULL) {
		fprintf(stderr, "Can't open ");
		perror(MOUNTED);
		exit(8);
	}
	do {
		rewind(mtab);
		anygtr = 0;

		while (mntp = getmntent(mtab)) {
			if (mntp->mnt_passno > passno)
				anygtr = 1;

			if (mntp->mnt_passno != passno)
				continue;

			if (strcmp(mntp->mnt_type, MNTTYPE_43) != 0 ||
			    hasmntopt(mntp, MNTOPT_RO) ||
			    (!oneof(mntp->mnt_fsname, listp, listcnt) &&
			     !oneof(mntp->mnt_dir, listp, listcnt)) )
				continue;

			switch (fork()) {
			case -1:
				perror("fork");
				exit(8);
				break;

			case 0:
				sprintf(quotafile, "%s/%s",
					mntp->mnt_dir, QFNAME);
				exit(chkquota(mntp->mnt_fsname,
					mntp->mnt_dir, quotafile));
			}
		}

		while (wait(&status) != -1) 
			errs += status.w_retcode;

		passno++;
	} while (anygtr);
	endmntent(mtab);

	return (errs);
}

chkquota(fsdev, fsfile, qffile)
	char *fsdev;
	char *fsfile;
	char *qffile;
{
	register struct fileusage *fup;
	dev_t quotadev;
	FILE *qf;
	register u_short uid;
	register u_short highquota;
	int cg, i;
	char *rawdisk;
	struct stat statb;
	struct dqblk dqbuf;
	extern int errno;
	int retval;

	if (Pflag) {
	    char fsclean[MAXPATHLEN + 32];
	    sprintf(fsclean, "/etc/fsclean -q%s %s", vflag ? "v" : "", fsdev);
	    if (system(fsclean) == 0)
		return 0;
	}
	rawdisk = makerawname(fsdev);
	if (vflag)
		printf("*** Checking quotas for %s (%s)\n", rawdisk, fsfile);
	fi = open(rawdisk, 0);
	if (fi < 0) {
		perror(rawdisk);
		return (1);
	}
	qf = fopen(qffile, "r+");
	if (qf == NULL) {
		perror(qffile);
		close(fi);
		return (1);
	}
	if (fstat(fileno(qf), &statb) < 0) {
		perror(qffile);
		fclose(qf);
		close(fi);
		return (1);
	}
	quotadev = statb.st_dev;
	if (stat(fsdev, &statb) < 0) {
		perror(fsdev);
		fclose(qf);
		close(fi);
		return (1);
	}
	if (quotadev != statb.st_rdev) {
		fprintf(stderr, "%s dev (0x%x) mismatch %s dev (0x%x)\n",
			qffile, quotadev, fsdev, statb.st_rdev);
		fclose(qf);
		close(fi);
		return (1);
	}
	bread(SBLOCK, (char *)&sblock, SBSIZE);
	ino = 0;
	for (cg = 0; cg < sblock.fs_ncg; cg++) {
		dp = NULL;
		for (i = 0; i < sblock.fs_ipg; i++)
			acct(ginode());
	}
	highquota = 0;
	for (uid = 0; uid <= highuid; uid++) {
		(void) fseek(qf, (long)dqoff(uid), 0);
		(void) fread(&dqbuf, sizeof(struct dqblk), 1, qf);
		if (feof(qf))
			break;
		fup = lookup(uid);
		if (fup == 0)
			fup = &zerofileusage;
		if (dqbuf.dqb_bhardlimit || dqbuf.dqb_bsoftlimit ||
		    dqbuf.dqb_fhardlimit || dqbuf.dqb_fsoftlimit) {
			highquota = uid;
		} else {
			fup->fu_curfiles = 0;
			fup->fu_curblocks = 0;
		}
		if (dqbuf.dqb_curfiles == fup->fu_curfiles &&
		    dqbuf.dqb_curblocks == fup->fu_curblocks) {
			fup->fu_curfiles = 0;
			fup->fu_curblocks = 0;
			continue;
		}
		if (vflag) {
			if (pflag || aflag)
				printf("%s: ", rawdisk);
			if (fup->fu_name[0] != '\0')
				printf("%-10s fixed:", fup->fu_name);
			else
				printf("#%-9d fixed:", uid);
			if (dqbuf.dqb_curfiles != fup->fu_curfiles)
				printf("  files %d -> %d",
				    dqbuf.dqb_curfiles, fup->fu_curfiles);
			if (dqbuf.dqb_curblocks != fup->fu_curblocks)
				printf("  blocks %d -> %d",
				    dqbuf.dqb_curblocks, fup->fu_curblocks);
			printf("\n");
		}
		dqbuf.dqb_curfiles = fup->fu_curfiles;
		dqbuf.dqb_curblocks = fup->fu_curblocks;
		(void) fseek(qf, (long)dqoff(uid), 0);
		(void) fwrite(&dqbuf, sizeof(struct dqblk), 1, qf);
		(void) quotactl(Q_SETQUOTA, fsdev, uid, &dqbuf);
		fup->fu_curfiles = 0;
		fup->fu_curblocks = 0;
	}
	(void) fflush(qf);
	(void) ftruncate(fileno(qf), (highquota + 1) * sizeof(struct dqblk));
	fclose(qf);
	close(fi);
	if (retval = quotactl(Q_SETQCLEAN, fsdev, 0, 0)) {
	    fprintf(stderr,
		    "quotacheck: quotactl(Q_SETQCLEAN,%s,0,0) returns %d: ",
		    fsdev, retval);
	    perror("");
	    return 1;
	}
	return (0);
}

acct(ip)
	register struct dinode *ip;
{
	register struct fileusage *fup;

	if (ip == NULL)
		return;
	if (ip->di_mode == 0)
		return;
#ifdef ACLS
	/* acct continuation inodes with the parent,
	    not when the continuation is encountered,
	    since ip->di_uid isn't meaningful
	    for a contin inode. */
	if ((ip->di_mode & IFMT) == IFCONT)
		return;
#endif
	fup = adduid((u_short)ip->di_uid);
	fup->fu_curfiles++;
#ifdef ACLS
	if (ip->di_contin)
	    fup->fu_curfiles++;
#endif
	if ((ip->di_mode & IFMT) == IFCHR || (ip->di_mode & IFMT) == IFBLK)
		return;
	fup->fu_curblocks += ip->di_blocks;
}

oneof(target, listp, n)
	char *target;
	register char **listp;
	register int n;
{
	char *cdfname;

	while (n--) {
		cdfname = getcdf(*listp,NULL,BUFSIZ);
		if (*listp && strcmp(target, cdfname) == 0) {
			*listp = (char *)0;
			free(cdfname);
			return (1);
		}
		free(cdfname);
		listp++;
	}
	return (0);
}

struct dinode *
ginode()
{
	register unsigned long iblk;

	if (dp == NULL || ++dp >= &itab[ITABSZ]) {
		iblk = itod(&sblock, ino);
		bread((u_long)fsbtodb(&sblock, iblk),
		    (char *)itab, sizeof itab);
		dp = &itab[ino % INOPB(&sblock)];
	/* The value of dp = &itab[0] 99.99% of the time, but the above code */
	/* is more general!! */
	}
	if (ino++ < ROOTINO)
		return(NULL);
	return(dp);
}

bread(bno, buf, cnt)
	long unsigned bno;
	char *buf;
{
	extern off_t lseek();
	register off_t pos;

	pos = (off_t)dbtob(bno);
	if (lseek(fi, pos, 0) != pos) {
		perror("lseek");
		exit(1);
	}

	(void) lseek(fi, (long)dbtob(bno), 0);
	if (read(fi, buf, cnt) != cnt) {
		perror("read");
		exit(1);
	}
}

struct fileusage *
lookup(uid)
	register u_short uid;
{
	register struct fileusage *fup;

	for (fup = fuhead[uid % FUHASH]; fup != 0; fup = fup->fu_next)
		if (fup->fu_uid == uid)
			return (fup);
	return ((struct fileusage *)0);
}

struct fileusage *
adduid(uid)
	register u_short uid;
{
	struct fileusage *fup, **fhp;
	extern char *calloc();

	fup = lookup(uid);
	if (fup != 0)
		return (fup);
	fup = (struct fileusage *)calloc(1, sizeof(struct fileusage));
	if (fup == 0) {
		fprintf(stderr, "out of memory for fileusage structures\n");
		exit(1);
	}
	fhp = &fuhead[uid % FUHASH];
	fup->fu_next = *fhp;
	*fhp = fup;
	fup->fu_uid = uid;
	if (uid > highuid)
		highuid = uid;
	return (fup);
}

char *
makerawname(name)
	char *name;
{
	register char *ptr1, *ptr2;
	static char rawname[MAXPATHLEN];

	if (strncmp(name, "/dev/", strlen("/dev/")) != 0) {
	    if (strncmp(name, "/dev+/", strlen("/dev+/")) != 0) {
		return name;
	    } else {
		/* this is a cdf */
		strcpy(rawname, "/dev+/");
		ptr1 = name + strlen("/dev+/");
		ptr2 = rawname + strlen("/dev/+");
		while (*ptr1 && (*ptr1 != '/')) {
		    /* copy cnode name to rawname */
		    *ptr2++ = *ptr1++;
		}
		if (*ptr1++ == (char)0) {
		    return name;
		} else {
		    strcpy(ptr2, "/r");
		    strcat(ptr2, ptr1);
		}
		return rawname;
	    }
	} else {
	    strcpy(rawname, "/dev/r");
	    ptr1 = name + strlen("/dev/");
	    strcat(rawname, ptr1);
	    return (rawname);
	}
}

usage()
{
	fprintf(stderr, "Usage:\n\t%s\n\t%s\n",
		"quotacheck [-v] [-p] [-P] -a",
		"quotacheck [-v] [-p] [-P] filesys ...");
	exit(1);
}
