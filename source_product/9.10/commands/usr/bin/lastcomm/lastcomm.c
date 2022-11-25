
#ifndef lint
static char *HPUX_ID = "@(#) $Revision: 63.1 $";
#endif

/*
 * last command
 */
#include <sys/param.h>
#include <sys/acct.h>
#include <sys/file.h>
#include <fcntl.h>

#include <stdio.h>
#include <pwd.h>
#include <sys/stat.h>
#include <utmp.h>
#include <ctype.h>

struct	acct buf[DEV_BSIZE / sizeof (struct acct)];

#define bufsize sizeof(buf)
/*
 * access to information relating to the fields of a structure
 */
#define	fldoff(str, fld)	((int)&(((struct str *)0)->fld))
#define	fldsiz(str, fld)	(sizeof(((struct str *)0)->fld))
#define	strbase(str, ptr, fld)	((struct str *)((char *)(ptr)-fldoff(str, fld)))

#ifdef hpux
#define ECORE	0200	/*  bit set in exit status if core was dumped	*/
#define ETERM	 017	/*  bit set if termination was due to SIGTERM	*/
#endif

time_t	expand();
char	*flagbits();
char	*getname();
char	*getdev();

main(argc, argv)
	char *argv[];
{
	register int bn, cc;
	register struct acct *acp;
	int fd;
	struct stat sb;
	int bytes;

	fd = open("/usr/adm/pacct", O_RDONLY);
	if (fd < 0) {
		perror("/usr/adm/pacct");
		exit(1);
	}
	fstat(fd, &sb);
	bytes = sb.st_size;
	for ( bn = bytes/bufsize; bn >= 0; bn-- ) {
		lseek(fd, bn*bufsize, L_SET);
		cc = read(fd, buf, bufsize);
		if (cc < 0) {
			perror("read");
			break;
		}
		acp = buf + (cc / sizeof (buf[0])) - 1;
		for (; acp >= buf; acp--) {
			register char *cp;
			time_t x;

			if (acp->ac_comm[0] == '\0')
				strcpy(acp->ac_comm, "?");
			for (cp = &acp->ac_comm[0];
			     cp < &acp->ac_comm[fldsiz(acct, ac_comm)] && *cp;
			     cp++)
				if (!isascii(*cp) || iscntrl(*cp))
					*cp = '?';
			if (argc > 1 && !ok(argc, argv, acp))
				continue;
			x = expand(acp->ac_utime) + expand(acp->ac_stime);
			if (x == (time_t)0) {
				x = (time_t)1;
			}
			printf("%-*.*s %s %-*s %-*s %6.2f secs %.16s\n",
				fldsiz(acct, ac_comm), fldsiz(acct, ac_comm),
				acp->ac_comm,
				flagbits(acp->ac_flag, acp->ac_stat),
				fldsiz(utmp, ut_name), getname(acp->ac_uid),
				fldsiz(utmp, ut_line), getdev(acp->ac_tty),
				x / (double)HZ, ctime(&acp->ac_btime));
		}
	}
}

time_t
expand (t)
	unsigned t;
{
	register time_t nt;
	register e;

	nt = t & 017777;
	e = (t>>13) & 07;
	while (e--) {
		nt <<= 3;
	}
	return (nt);
}

char *
flagbits(f, s)
	register char f, s;
{
	register int i = 0;
	static char flags[20];

#define BIT(flag, ch)	flags[i++] = (f & flag) ? ch : ' '
	BIT(ASU, 'S');
	BIT(AFORK, 'F');
#ifdef hpux
/*  Look at the exit status.  Check for core dump or termination by
 *  SIGTERM.
 */
#define ESTAT(flag, ch)	flags[i++] = ((s & flag) == flag) ? ch : ' '
	ESTAT(ECORE, 'D');
	ESTAT(ETERM, 'X');
#else
	BIT(ACOMPAT, 'C');
	BIT(ACORE, 'D');
	BIT(AXSIG, 'X');
#endif
	flags[i] = '\0';
	return (flags);
}

ok(argc, argv, acp)
	register int argc;
	register char *argv[];
	register struct acct *acp;
{
	register int j;

	for (j = 1; j < argc; j++)
		if (strcmp(getname(acp->ac_uid), argv[j]) &&
		    strcmp(getdev(acp->ac_tty), argv[j]) &&
		    strncmp(acp->ac_comm, argv[j], fldsiz(acct, ac_comm)))
			break;
	return (j==argc) ;
}

/* should be done with nameserver or database */

struct	utmp utmp;

#define NUID	2048
#define	NMAX	(sizeof (utmp.ut_name))

char	names[NUID][NMAX+1];
char	outrangename[NMAX+1];
int	outrangeuid = -1;

char *
getname(uid)
{
	register struct passwd *pw;
	static init;
	struct passwd *getpwent();

	if (uid >= 0 && uid < NUID && names[uid][0])
		return (&names[uid][0]);
	if (uid >= 0 && uid == outrangeuid)
		return (outrangename);
	if (init == 2) {
		if (uid < NUID)
			return (0);
		setpwent();
		while (pw = getpwent()) {
			if (pw->pw_uid != uid)
				continue;
			outrangeuid = pw->pw_uid;
			strncpy(outrangename, pw->pw_name, NMAX);
			endpwent();
			return (outrangename);
		}
		endpwent();
		return (0);
	}
	if (init == 0)
		setpwent(), init = 1;
	while (pw = getpwent()) {
		if (pw->pw_uid < 0 || pw->pw_uid >= NUID) {
			if (pw->pw_uid == uid) {
				outrangeuid = pw->pw_uid;
				strncpy(outrangename, pw->pw_name, NMAX);
				return (outrangename);
			}
			continue;
		}
		if (names[pw->pw_uid][0])
			continue;
		strncpy(names[pw->pw_uid], pw->pw_name, NMAX);
		if (pw->pw_uid == uid)
			return (&names[uid][0]);
	}
	init = 2;
	endpwent();
	return (0);
}

#include <sys/dir.h>

#define N_DEVS		43		/* hash value for device names */
#define NDEVS		500		/* max number of file names in /dev */

struct	devhash {
	dev_t	dev_dev;
	char	dev_name [fldsiz(utmp, ut_line) + 1];
	struct	devhash * dev_nxt;
};
struct	devhash *dev_hash[N_DEVS];
struct	devhash *dev_chain;
#define HASH(d)	(((int) d) % N_DEVS)

/* sub directories of /dev to search for pty's/tty's */
char *dev_prefix[] = { "/", "/pty/", (char *) 0 }; 

setupdevs()
{
	register DIR * fd;
	register struct devhash * hashtab;
	register ndevs = NDEVS;
	struct direct * dp;
	int loc;
	char dir_name[20];

	hashtab = (struct devhash *)malloc(NDEVS * sizeof(struct devhash));
	if (hashtab == (struct devhash *)0) {
		fprintf(stderr, "No mem for dev table\n");
		return;
	}
	for (loc=0; dev_prefix[loc] != (char *) 0; loc++) {
		strcpy(dir_name, "/dev");
		strcat(dir_name, dev_prefix[loc]);
		if ((fd = opendir(dir_name)) == NULL)
			continue;   /* this dir not on machine */

		while (dp = readdir(fd)) {
			if (dp->d_ino == 0)
				continue;
			if (dp->d_name[0] != 't' && strcmp(dp->d_name, "console"))
				continue;
			strcpy(hashtab->dev_name, dev_prefix[loc]);
			strncat(hashtab->dev_name, dp->d_name, fldsiz(utmp, ut_line));
			/*strncpy(hashtab->dev_name, dp->d_name, fldsiz(utmp, ut_line));*/
			hashtab->dev_name[fldsiz(utmp, ut_line)] = 0;
			hashtab->dev_nxt = dev_chain;
			dev_chain = hashtab;
			hashtab++;
			if (--ndevs <= 0)
				break;
		}
		closedir(fd);
	}
}

char *
getdev(dev)
	dev_t dev;
{
	register struct devhash *hp, *nhp;
	struct stat statb;
	char name[fldsiz(devhash, dev_name) + 6];
	static dev_t lastdev = (dev_t) -1;
	static char *lastname;
	static int init = 0;

	if (dev == NODEV)
		return ("__");
	if (dev == lastdev)
		return (lastname);
	if (!init) {
		setupdevs();
		init++;
	}
	for (hp = dev_hash[HASH(dev)]; hp; hp = hp->dev_nxt)
		if (hp->dev_dev == dev) {
			lastdev = dev;
			return (lastname = strrchr(hp->dev_name,'/') + 1);
		}
	for (hp = dev_chain; hp; hp = nhp) {
		nhp = hp->dev_nxt;
		strcpy(name, "/dev/");
		strcat(name, hp->dev_name);
		if (stat(name, &statb) < 0)	/* name truncated usually */
			continue;
		if ((statb.st_mode & S_IFMT) != S_IFCHR)
			continue;
		hp->dev_dev = statb.st_rdev;
		hp->dev_nxt = dev_hash[HASH(hp->dev_dev)];
		dev_hash[HASH(hp->dev_dev)] = hp;
		if (hp->dev_dev == dev) {
			dev_chain = nhp;
			lastdev = dev;
			return (lastname = strrchr(hp->dev_name,'/') + 1);
		}
	}
	dev_chain = (struct devhash *) 0;
	return ("??");
}

