/* @(#)  $Revision: 70.4 $ */

/*
 * Copyright (c) 1980 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */

#include "dump.h"
#ifdef	hpux
#  include <varargs.h>
#endif	hpux

/*
 *	This is from /usr/include/grp.h
 *	That defined struct group, which conflicts
 *	with the struct group defined in param.h
 */
struct	Group { /* see getgrent(3) */
	char	*gr_name;
	char	*gr_passwd;
	int	gr_gid;
	char	**gr_mem;
};
struct	Group *getgrnam();
/*
 *	Query the operator; This fascist piece of code requires
 *	an exact response.
 *	It is intended to protect dump aborting by inquisitive
 *	people banging on the console terminal to see what is
 *	happening which might cause dump to croak, destroying
 *	a large number of hours of work.
 *
 *	Every 2 minutes we reprint the message, alerting others
 *	that dump needs attention.
 */
int	timeout;
char	*attnmessage;		/* attention message */
query(question)
	char	*question;
{
	char	replybuffer[64];
	int	back;
	FILE	*mytty;

	if ( (mytty = fopen("/dev/tty", "r")) == NULL){
		msg("fopen on /dev/tty fails\n");
		dumpabort();
	}
	attnmessage = question;
	timeout = 0;
	alarmcatch();
	for(;;){
		if ( fgets(replybuffer, 63, mytty) == NULL){
			if (ferror(mytty)){
				clearerr(mytty);
				continue;
			}
		} else if ( (strcmp(replybuffer, "yes\n") == 0) ||
			    (strcmp(replybuffer, "Yes\n") == 0)){
				back = 1;
				goto done;
		} else if ( (strcmp(replybuffer, "no\n") == 0) ||
			    (strcmp(replybuffer, "No\n") == 0)){
				back = 0;
				goto done;
		} else {
			msg("\"Yes\" or \"No\"?\n");
			alarmcatch();
		}
	}
    done:
	/*
	 *	Turn off the alarm, and reset the signal to trap out..
	 */
	alarm(0);
	if (signal(SIGALRM, sigalrm) == SIG_IGN)
		signal(SIGALRM, SIG_IGN);
	fclose(mytty);
	return(back);
}
/*
 *	Alert the console operator, and enable the alarm clock to
 *	sleep for 2 minutes in case nobody comes to satisfy dump
 */
alarmcatch()
{
	if (timeout)
		msgtail("\n");
	msg("NEEDS ATTENTION: %s: (\"yes\" or \"no\") ",
		attnmessage);
	signal(SIGALRM, alarmcatch);
	alarm(120);
	timeout = 1;
}
/*
 *	Here if an inquisitive operator interrupts the dump program
 */
interrupt()
{
	msg("Interrupt received.\n");
	if (query("Do you want to abort dump?"))
		dumpabort();
	signal(SIGINT, interrupt);
}

/*
 *	The following variables and routines manage alerting
 *	operators to the status of dump.
 *	This works much like wall(1) does.
 */
struct	Group *gp;

/*
 *	Get the names from the group entry "operator" to notify.
 */	
set_operators()
{
	if (!notify)		/*not going to notify*/
		return;
	gp = getgrnam(OPGRENT);
	endgrent();
	if (gp == (struct Group *)0){
		msg("No entry in /etc/group for %s.\n",
			OPGRENT);
		notify = 0;
		return;
	}
}

struct tm *localtime();
struct tm *localclock;

/*
 *	We fork a child to do the actual broadcasting, so
 *	that the process control groups are not messed up
 */
broadcast(message)
	char	*message;
{
	time_t		clock;
	FILE	*f_utmp;
	struct	utmp	utmp;
	int	nusers;
	char	**np;
	int	pid, s;

	switch (pid = fork()) {
	case -1:
		return;
	case 0:
		break;
	default:
		while (wait(&s) != pid)
			continue;
		return;
	}

	if (!notify || gp == 0)
		exit(0);
	clock = time(0);
	localclock = localtime(&clock);

	if((f_utmp = fopen("/etc/utmp", "r")) == NULL) {
		msg("Cannot open /etc/utmp\n");
		return;
	}

	nusers = 0;
	while (!feof(f_utmp)){
		if (fread(&utmp, sizeof (struct utmp), 1, f_utmp) != 1)
			break;
		if (utmp.ut_name[0] == 0)
			continue;
		nusers++;
		for (np = gp->gr_mem; *np; np++){
			if (strncmp(*np, utmp.ut_name, sizeof(utmp.ut_name)) != 0)
				continue;
			/*
			 *	Do not send messages to operators on dialups
			 */
			if (strncmp(utmp.ut_line, DIALUP, strlen(DIALUP)) == 0)
				continue;
#ifdef	DEBUG
			msg("Message to %s at %s\n",
				utmp.ut_name, utmp.ut_line);
#endif	DEBUG
			sendmes(utmp.ut_line, message);
		}
	}
	fclose(f_utmp);
	Exit(0);	/* the wait in this same routine will catch this */
	/* NOTREACHED */
}

sendmes(tty, message)
	char *tty, *message;
{
	char t[50], buf[BUFSIZ];
	register char *cp;
	register int c, ch;
	int	msize;
	FILE *f_tty;

	msize = strlen(message);
	strcpy(t, "/dev/");
	strcat(t, tty);

	if((f_tty = fopen(t, "w")) != NULL) {
		setbuf(f_tty, buf);
		fprintf(f_tty, "\nMessage from the dump program to all operators at %d:%02d ...\r\n\n"
		       ,localclock->tm_hour
		       ,localclock->tm_min);
		for (cp = message, c = msize; c-- > 0; cp++) {
			ch = *cp;
			if (ch == '\n')
				putc('\r', f_tty);
			putc(ch, f_tty);
		}
		fclose(f_tty);
	}
}

/*
 *	print out an estimate of the amount of time left to do the dump
 */

time_t	tschedule = 0;

timeest()
{
	float percent_done;
        int finished_hr, finished_min;

	time_t	tnow, deltat;

	time (&tnow);
	if (tnow >= tschedule){
		tschedule = tnow + 300;
		if (blockswritten < 500)
			return;	
		deltat = tstart_writing - tnow +
			(((1.0*(tnow - tstart_writing))/blockswritten) * esize);
                percent_done = (blockswritten*100.0)/esize;
		finished_hr = deltat/3600; 
                finished_min = (deltat%3600)/60;
                /* These messages modified to fix defect DSDe416645. */
	  	if (percent_done < 100.00)
	 	   msg("%3.2f%% done, finished in %d:%02d\n",
	                percent_done, finished_hr, finished_min);
		else 
           msg("Files have grown, currently at %3.2f%% of original estimate.\n",
                   percent_done);
	}
}

int blocksontape()
{
	/*
	 *	esize: total number of blocks estimated over all reels
	 *	blockswritten:	blocks actually written, over all reels
	 *	etapes:	estimated number of tapes to write
	 *
	 *	tsize:	blocks can write on this reel
	 *	asize:	blocks written on this reel
	 *	tapeno:	number of tapes written so far
	 */
	if (tapeno == etapes)
		return(esize - (etapes - 1)*tsize);
	return(tsize);
}

#ifdef	hpux

#if defined(TRUX) && defined(B1)
msg_b1(va_alist)
    va_dcl
{
  if (b1debug){
    va_list ap;
    char *fmt;

    va_start(ap);
    fmt = va_arg(ap, char *);
    fprintf(stderr,"  DUMP: ");
#ifdef	TDEBUG
    fprintf(stderr,"pid=%d ", getpid());
#endif	TDEBUG
    vfprintf(stderr, fmt, ap);
    fflush(stdout);
    fflush(stderr);
    va_end(ap);
  }
}
#endif /* TRUX && B1 */

msg(va_alist)
    va_dcl
{
    va_list ap;
    char *fmt;

    va_start(ap);
    fmt = va_arg(ap, char *);
    fprintf(stderr,"  DUMP: ");
#ifdef	TDEBUG
    fprintf(stderr,"pid=%d ", getpid());
#endif	TDEBUG
    vfprintf(stderr, fmt, ap);
    fflush(stdout);
    fflush(stderr);
    va_end(ap);
}

msgtail(va_alist)
    va_dcl
{
    va_list ap;
    char *fmt;

    va_start(ap);
    fmt = va_arg(ap, char *);
    vfprintf(stderr, fmt, ap);
    va_end(ap);
}
#else	hpux
	/* VARARGS1 */
	/* ARGSUSED */
msg(fmt, a1, a2, a3, a4, a5)
	char	*fmt;
	int	a1, a2, a3, a4, a5;
{
	fprintf(stderr,"  DUMP: ");
#ifdef	TDEBUG
	fprintf(stderr,"pid=%d ", getpid());
#endif	TDEBUG
	fprintf(stderr, fmt, a1, a2, a3, a4, a5);
	fflush(stdout);
	fflush(stderr);
}

	/* VARARGS1 */
	/* ARGSUSED */
msgtail(fmt, a1, a2, a3, a4, a5)
	char	*fmt;
	int	a1, a2, a3, a4, a5;
{
	fprintf(stderr, fmt, a1, a2, a3, a4, a5);
}
#endif

/*
 *	Tell the operator what has to be done;
 *	we don't actually do it
 */
#ifdef	hpux
struct mntent *
allocfsent(fs)
	register struct mntent *fs;
{
	register struct mntent *new;
	register char *cp;
	char *malloc();

	/* get new mntent struct */
	new = (struct mntent *)malloc(sizeof (*fs));

	/* copy fsname field */
	cp = malloc(strlen(fs->mnt_fsname) + 1);
	strcpy(cp, fs->mnt_fsname);
	new->mnt_fsname = cp;

	/* copy dir field */
	cp = malloc(strlen(fs->mnt_dir) + 1);
	strcpy(cp, fs->mnt_dir);
	new->mnt_dir = cp;

	/* copy type field */
	cp = malloc(strlen(fs->mnt_type) + 1);
	strcpy(cp, fs->mnt_type);
	new->mnt_type = cp;

	/* copy opts field */
	cp = malloc(strlen(fs->mnt_opts) + 1);
	strcpy(cp, fs->mnt_opts);
	new->mnt_opts = cp;


	/* copy all the simple fields */
	new->mnt_freq = fs->mnt_freq;
	new->mnt_passno = fs->mnt_passno;
	new->mnt_time = fs->mnt_time;

	return (new);
}

struct	pfstab {
	struct	pfstab *pf_next;
	struct	mntent	*pf_mntent;
};

#else	hpux

struct fstab *
allocfsent(fs)
	register struct fstab *fs;
{
	register struct fstab *new;
	register char *cp;
	char *malloc();

	new = (struct fstab *)malloc(sizeof (*fs));
	cp = malloc(strlen(fs->fs_file) + 1);
	strcpy(cp, fs->fs_file);
	new->fs_file = cp;
	cp = malloc(strlen(fs->fs_type) + 1);
	strcpy(cp, fs->fs_type);
	new->fs_type = cp;
	cp = malloc(strlen(fs->fs_spec) + 1);
	strcpy(cp, fs->fs_spec);
	new->fs_spec = cp;
	new->fs_passno = fs->fs_passno;
	new->fs_freq = fs->fs_freq;
	return (new);
}

struct	pfstab {
	struct	pfstab *pf_next;
	struct	fstab *pf_fstab;
};

#endif	hpux

static	struct pfstab *table = NULL;

getfstab()
{

#ifdef	hpux

	register struct mntent *fs;
	register struct pfstab *pf;
	FILE *chklst;

	if ((chklst = setmntent(MNT_CHECKLIST, "r")) == NULL) {
		msg("Can't open %s for dump table information.\n", MNT_CHECKLIST);
		return;
	}
	while (fs = getmntent(chklst)) {
		/* only interested in local file systems */
		if (strcmp(fs->mnt_type, MNTTYPE_HFS))
			continue;
#if defined(TRUX) && defined(B1)
		if (!strcmp(fs->mnt_dir, disk) || !strcmp(fs->mnt_fsname, disk))
			found_in_chklst++;
#endif /* (TRUX) && defined(B1) */

#else	hpux

	register struct fstab *fs;
	register struct pfstab *pf;

	if (setfsent() == 0) {
		msg("Can't open %s for dump table information.\n", FSTAB);
		return;
	}
	while (fs = getfsent()) {
		if (strcmp(fs->fs_type, FSTAB_RW) &&
		    strcmp(fs->fs_type, FSTAB_RO) &&
		    strcmp(fs->fs_type, FSTAB_RQ))
			continue;

#endif	hpux

		fs = allocfsent(fs);
		pf = (struct pfstab *)malloc(sizeof (*pf));
		pf->pf_mntent = fs;
		pf->pf_next = table;
		table = pf;
	}

#ifdef	hpux
	endmntent(chklst);
#else	hpux
	endfsent();
#endif	hpux
}

/*
 * Search in the fstab for a file name.
 * This file name can be either the special or the path file name.
 *
 * The entries in the fstab are the BLOCK special names, not the
 * character special names.
 * The caller of fstabsearch assures that the character device
 * is dumped (that is much faster)
 *
 * The file name can omit the leading '/'.
 */

#ifdef	hpux

struct mntent *
fstabsearch(key)
	char *key;
{
	register struct pfstab *pf;
	register struct mntent *fs;
	char *rawname();

	if (table == NULL)
		return ((struct mntent *)0);
	for (pf = table; pf; pf = pf->pf_next) {
		fs = pf->pf_mntent;
		if (strcmp(fs->mnt_dir, key) == 0)
			return (fs);
		if (strcmp(fs->mnt_fsname, key) == 0)
			return (fs);
		if (strcmp(rawname(fs->mnt_fsname), key) == 0)
			return (fs);
		if (key[0] != '/'){
			if (*fs->mnt_fsname == '/' &&
			    strcmp(fs->mnt_fsname + 1, key) == 0)
				return (fs);
			if (*fs->mnt_dir == '/' &&
			    strcmp(fs->mnt_dir + 1, key) == 0)
				return (fs);
		}
	}
	return (0);
}

#else	hpux

struct fstab *
fstabsearch(key)
	char *key;
{
	register struct pfstab *pf;
	register struct fstab *fs;
	char *rawname();

	if (table == NULL)
		return ((struct fstab *)0);
	for (pf = table; pf; pf = pf->pf_next) {
		fs = pf->pf_fstab;
		if (strcmp(fs->fs_file, key) == 0)
			return (fs);
		if (strcmp(fs->fs_spec, key) == 0)
			return (fs);
		if (strcmp(rawname(fs->fs_spec), key) == 0)
			return (fs);
		if (key[0] != '/'){
			if (*fs->fs_spec == '/' &&
			    strcmp(fs->fs_spec + 1, key) == 0)
				return (fs);
			if (*fs->fs_file == '/' &&
			    strcmp(fs->fs_file + 1, key) == 0)
				return (fs);
		}
	}
	return (0);
}

#endif	hpux

/*
 *	Tell the operator what to do
 */
lastdump(arg)
	char	arg;		/* w ==> just what to do; W ==> most recent dumps */
{
			char	*lastname;
			char	*date;
	register	int	i;
			time_t	tnow;
#ifdef	hpux
	register	struct	mntent	*dt;
#else	hpux
	register	struct	fstab	*dt;
#endif	hpux
			int	dumpme;
	register	struct	idates	*itwalk;

	int	idatesort();

	time(&tnow);
	getfstab();		/* /etc/fstab input */
	inititimes();		/* /etc/dumpdates input */
	qsort(idatev, nidates, sizeof(struct idates *), idatesort);

	if (arg == 'w')
		fprintf(stdout, "Dump these file systems:\n");
	else
		fprintf(stdout, "Last dump(s) done (Dump '>' file systems):\n");
	lastname = "??";
	ITITERATE(i, itwalk){
		if (strncmp(lastname, itwalk->id_name, sizeof(itwalk->id_name)) == 0)
			continue;
		date = (char *)ctime(&itwalk->id_ddate);
		date[16] = '\0';		/* blast away seconds and year */
		lastname = itwalk->id_name;
		dt = fstabsearch(itwalk->id_name);
		dumpme = (  (dt != 0)
#ifdef	hpux
			 && (dt->mnt_freq != 0)
			 && (itwalk->id_ddate < tnow - (dt->mnt_freq*DAY)));
#else	hpux
			 && (dt->fs_freq != 0)
			 && (itwalk->id_ddate < tnow - (dt->fs_freq*DAY)));
#endif	hpux
		if ( (arg != 'w') || dumpme)
		  fprintf(stdout,"%c %8s\t(%6s) Last dump: Level %c, Date %s\n",
			dumpme && (arg != 'w') ? '>' : ' ',
			itwalk->id_name,
#ifdef	hpux
			dt ? dt->mnt_dir : "",
#else	hpux
			dt ? dt->fs_file : "",
#endif	hpux
			itwalk->id_incno,
			date
		    );
	}
}

int	
idatesort(p1, p2)
	struct	idates	**p1, **p2;
{
	int	diff;

	diff = strncmp((*p1)->id_name, (*p2)->id_name, sizeof((*p1)->id_name));
	if (diff == 0)
		return ((*p2)->id_ddate - (*p1)->id_ddate);
	else
		return (diff);
}

int 
max(a,b)
	int a, b;
{
	return(a>b?a:b);
}
int 
min(a,b)
	int a, b;
{
	return(a<b?a:b);
}
