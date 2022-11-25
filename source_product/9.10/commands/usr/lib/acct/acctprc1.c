/* @(#) $Revision: 70.1 $ */    
/*
 *	acctprc1 [ctmp]
 *
 *	Acctprc1 reads input in the form described by acct(5), adds 
 *	login names corresponding to user IDs, then writes for each
 *	process an ASCII line giving user ID, login name, prime CPU 
 *	time (tics), non-prime CPU time (tics), and mean memory size (in
 *	memory segment units). If ctmp is given, it is expected to
 *	contain a list of login sessions, in the form described in
 *	acctcon(1M), sorted by user ID and login name. If this file is
 *	not supplied, it obtains login names from the password file.
 *	The information in ctmp helps it distinguish among different
 *	login names that share the same user ID.
 *
 * Modifications:
 *		11/6/91 : AW
 *		Removing users and sessions limitations for 9.0
 *		(5000 users enhancements)
 */

#include <sys/types.h>
#include "acctdef.h"
#include <stdio.h>
#include <pwd.h>
#include <sys/acct.h>
#include "ctmp.h"
#include "ptmp.h"
#include <malloc.h>
#define MYKIND(flag)	((flag & ACCTF) == 0)

struct	acct	ab;
struct	ctmp	cb;
struct	ptmp	pb;

struct srec {				/* 1 for each distinct session */
	dev_t	sr_tty;			/* dev, used to connect with process*/
	time_t	sr_start;		/* start time of session */
	time_t	sr_end;			/* end time of session */
	struct	srec *sr_ptr;		/* forward pointer to linked list */
};
struct urec {				/* 1 for each distinct uid/name */
	uid_t	ur_uid;			/* sorted by uid/name */
	char	ur_name[NSZ];
	struct	urec *ur_ptr;		/* forward pointer to linked list */
	struct	srec *ur_srec;		/* pointer to session records */
};
struct urec *ur[A_USIZE];		/* allocate user record pointers */

char	*getname(), *getnamc();
char	*uidtonam();

main(argc, argv)
char **argv;
{
	long	elaps[2];
	long	etime, stime, mem;
	long	expand(), i;

	/* initialize hash tables */
	for(i = 0; i < A_USIZE; i++)
		ur[i] = NULL;

	while (--argc > 0) {
		if (**++argv == '-')
			switch(*++*argv) {
			}
		else {
			readctmp(*argv);
		}
	}

	while (fread(&ab, sizeof(ab), 1, stdin) == 1) {
		if (!MYKIND(ab.ac_flag))
			continue;
		pb.pt_uid = ab.ac_uid;
		CPYN(pb.pt_name, getname(ab.ac_uid, ab.ac_tty, ab.ac_btime));
		/*
		 * approximate cpu P/NP split as same as elapsed time
		 */
		if ((etime = SECS(expand(ab.ac_etime))) == 0)
			etime = 1;
		stime = expand(ab.ac_stime) + expand(ab.ac_utime);
		mem = expand(ab.ac_mem);
		pnpsplit(ab.ac_btime, etime, elaps);
		pb.pt_cpu[0] = (double)stime * (double)elaps[0] / etime;
		pb.pt_cpu[1] = (stime > pb.pt_cpu[0])? stime - pb.pt_cpu[0] : 0;
		pb.pt_cpu[1] = stime - pb.pt_cpu[0];
		if (stime)
			pb.pt_mem = (mem + stime - 1) / stime;
		else
			pb.pt_mem = 0;	/* unlikely */
		printf("%u\t%.8s\t%lu\t%lu\t%u\n",
			pb.pt_uid,
			pb.pt_name,
			pb.pt_cpu[0], pb.pt_cpu[1],
			pb.pt_mem);
	}
}

/*
 *	return ptr to name corresponding to uid
 *	try ctmp first, then use uidtonam (internal list or passwd file)
 */
char *
getname(uid, tty, start)
uid_t	uid;
dev_t	tty;
time_t	start;
{
	register char *p;

	if ((p = getnamc(uid, tty, start)) != NULL)
		return(p);
	return(uidtonam(uid));
}

/*
 *	read ctmp file, build up urec-srec data structures for
 *	later use by getnamc
 */
readctmp(fname)
char *fname;
{
	FILE *fp;
	struct urec **fup;
	struct srec **fsp;
	register struct urec *up;
	register struct srec *sp;

	if ((fp = fopen(fname, "r")) == NULL) {
		fprintf(stderr, "acctprc1: Cannot open %s\n", fname);
		return;
	}

/*	while (fscanf(fp, "%hd\t%hu\t%s\t%lu\t%lu\t%lu\t%*[^\n]", */
	while (fscanf(fp, "%hd\t%ld\t%s\t%lu\t%lu\t%lu\t%*[^\n]",
		&cb.ct_tty,
		&cb.ct_uid,
		cb.ct_name,
		&cb.ct_con[0],
		&cb.ct_con[1],
		&cb.ct_start) != EOF) {
		fup = &ur[cb.ct_uid % A_USIZE];	/* get hash index */
		while(*fup != NULL) {		/* search linked list */
			if (cb.ct_uid == (*fup)->ur_uid &&
				EQN(cb.ct_name, (*fup)->ur_name))
				break;		/* found entry */
			fup = &((*fup)->ur_ptr);
		}
		if(*fup == NULL) {		/* add an entry */
			/* allocate buffer */
			up = (struct urec *) calloc(1, sizeof(struct urec));
			if(up == NULL) {
				printf("acctprc1: out of memory\n");
				exit(1);
			}
			*fup = up;		/* update entry */
			up->ur_ptr = NULL;
			up->ur_uid = cb.ct_uid;
			CPYN(up->ur_name, cb.ct_name);
			up->ur_srec = NULL;
		}
		/* update sessions record */
		sp = (struct srec *) calloc(1, sizeof(struct srec));
		if (sp == NULL) {
			printf("acctprc1: out of memory\n");
			exit(1);
		}
		sp->sr_ptr = NULL;
		fsp = &(up->ur_srec);
		/* move down the session list till you get a null */
		while(*fsp != NULL) fsp = &((*fsp)->sr_ptr);
		*fsp = sp;		/* update session record */
		sp->sr_tty = cb.ct_tty;
		sp->sr_start = cb.ct_start;
		sp->sr_end = cb.ct_start + cb.ct_con[0] + cb.ct_con[1];
	}
	fclose(fp);
}

/*
 *	using urec-srec data (if any), make best guess at login name
 *	corresponding to uid, return ptr to the name.
 *	must match on tty; use start time to help guess
 *	for any urec having same uid as uid, search array of associated
 *	srecs for those having same tty
 *	if start time of process is within range of session, that's it
 *	if none can be found within range, give it to person of same uid
 *	who last logged off on that terminal
 */
char *
getnamc(uid, tty, start)
register uid_t uid;
dev_t	tty;
time_t	start;
{
	register struct urec *up;
	register struct srec *sp;
	long latest;
	char *guess;

	latest = 0;
	guess = NULL;
	up = ur[uid % A_USIZE];			/* get hash index */
	while(up != NULL) {
		if(uid == up->ur_uid) {		/* check all user records */
			sp = up->ur_srec;
			while(sp != NULL) {	/* check all sessions */
				if(tty == sp->sr_tty) {
					if (start >= sp->sr_start &&
						start <= sp->sr_end)
						return(up->ur_name);
					if (start >= sp->sr_start &&
						sp->sr_end > latest) {
						latest = sp->sr_end;
						guess = up->ur_name;
					}
				}
				sp = sp->sr_ptr;
			}
		}
		up = up->ur_ptr;
	}
	return(guess);
}

