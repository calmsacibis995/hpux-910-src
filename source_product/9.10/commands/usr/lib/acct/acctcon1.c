/* @(#) $Revision: 70.3 $ */      
/*
 *	acctcon1 [options]
 *
 *	Acctcon1 converts a sequence of login/logoff records read from
 *	its standard input to a sequence of records, one per login
 *	session. Its input should normally be redirected from 
 *	/etc/wtmp. Its output is ASCII, giving device, user ID,
 *	login name, prime connect time (seconds), non-prime connect
 *	time (seconds), session starting time (numeric), and starting
 *	date and time. The options are:
 *
 *	-p	Print input only, showing line name, login name, and
 *		time (in both numeric and date/time formats).
 *
 *	-t	Acctcon1 maintains a list of lines on which users are
 *		logged in. When it reaches the end of its input, it
 *		emits a session record for each line that still appears
 *		to be active. It normally assumes that its input is a
 *		current file, so that it uses the current time as the
 *		ending time for each session still in progress. The
 *		-t flag causes it to use, instead, the last time found
 *		in the input, thus assuring reasonable and repeatable
 *		numbers for non-current files.
 *
 *	-l file	File is created to contain a summary of line usage
 *		showing line name, number of minutes used, percentage
 *		of total elapsed time used, number of sessions charged,
 *		number of logins, and number of logoffs. This file helps
 *		track line usage, identify bad lines, and find software
 *		and hardware oddities. Both hang-up and termination of
 *		the login shell generate a logoff record, so that the
 *		number of logoffs is often twice the number of sessions.
 *
 *	-o file	File is filled with an overall record for the accounting
 *		period, giving starting time, ending time, number of
 *		reboots, and number of date changes.
 *
 * Modifications: AW: 11/5/91  --- 5000 Users modifications
 *		Remove A_TSIZE limitations for number of terminal buffers
 * 		Remove static allocations (tbuf).
 *========================================================================
 */

#include <sys/types.h>
#include "acctdef.h"
#include <stdio.h>
#include <ctype.h>
#include <time.h>
#include <utmp.h>
#include "ctmp.h"
#include <malloc.h>

struct  utmp	wb;	/* record structure read into */
struct	ctmp	cb;	/* record structure written out of */

struct tbuf {
	char	tline[LSZ];	/* /dev/* */
	char	tname[NSZ];	/* user name */
	time_t	ttime;		/* start time */
	dev_t	tdev;		/* device */
	int	tlsess;		/* # complete sessions */
	int	tlon;		/* # times on (ut_type of 7) */
	int	tloff;		/* # times off (ut_type != 7) */
	long	ttotal;		/* total time used on this line */
	struct	tbuf *fwptr;	/* pointer to linked structures */
};
struct tbuf *ttylist[A_TSIZE];	/* Allocate array of pointers only */

#define NSYS	100
int	nsys;
struct sys {
	char	sname[LSZ];	/* reasons for ACCOUNTING records */
	char	snum;		/* number of times encountered */
} sy[NSYS];

time_t	datetime;	/* old time if date changed, otherwise 0 */
time_t	firstime;
time_t	lastime;
int	ndates;		/* number of times date changed */
int	exitcode;
char	*report	= NULL;
char	*replin = NULL;
int	printonly;
int	tflag;

char	*ctime();
long	ftell();
uid_t	namtouid();
dev_t	lintodev();
struct tbuf *iline();

main(argc, argv) 
char **argv;
{
int i;
	while (--argc > 0 && **++argv == '-')
		switch(*++*argv) {
		case 'l':
			if (--argc > 0)
				replin = *++argv;
			continue;
		case 'o':
			if (--argc > 0)
				report = *++argv;
			continue;
		case 'p':
			printonly++;
			continue;
		case 't':
			tflag++;
			continue;
		}

	for (i = 0; i < A_TSIZE; i++)
		ttylist[i] = NULL;		/* initialize hash table */

	if (printonly) {
		while (wread()) {
			if (valid()) {
				printf("%.12s\t%.8s\t%lu",
					wb.ut_line,
					wb.ut_name,
					wb.ut_time);
				printf("\t%s", ctime(&wb.ut_time));
			} else
				fixup(stdout);
			
		}
		exit(exitcode);
	}

	while (wread()) {
		if (firstime == 0)
			firstime = wb.ut_time;
		if (valid())
			loop();
		else
			fixup(stderr);
	}
	if (!tflag)
		time(&wb.ut_time);
	upall();			/* Log off anyone still on */
	wb.ut_name[0] = '\0';
	strcpy(wb.ut_line, "acctcon1");
	wb.ut_type = ACCOUNTING;
	loop();
	if (report != NULL)
		printrep();
	if (replin != NULL)
		printlin();
	exit(exitcode);
}

wread()
{
	return( fread(&wb, sizeof(wb), 1, stdin) == 1 );
	
}

/*
 * valid: check input wtmp record, return 1 if looks OK
 */
valid()
{
	register i, c;

	for (i = 0; i < NSZ; i++) {
		c = wb.ut_name[i];
		/* "." is a legal entry in the passowrd file,  the
		 * check for "." was missing.  This check was added
		 * to fix Bug #: FSDlj02847
		 */
		/*
		if (isalnum(c) || c == '$' || c == ' ' || c == '.')
		*/
		/* DSDe406933:
		 * Futher generalize the previous fix, change to follow
		 * current behavior of login(1), to allow all printing
		 * characters in the user name.
		 */
		if (isgraph(c))
			continue;
		
		else if (c == '\0')
			break;
		else
			return(0);
	}

	if((wb.ut_type >= EMPTY) && (wb.ut_type <= UTMAXTYPE))
		return(1);

	return(0);
}

/*
 *	fixup assumes that V6 wtmp (16 bytes long) is mixed in with
 *	V7 records (20 bytes each)
 *
 *	Starting with Release 5.0 of UNIX, this routine will no
 *	longer reset the read pointer.  This has a snowball effect
 *	On the following records until the offset corrects itself.
 *	If a message is printed from here, it should be regarded as
 *	a bad record and not as a V6 record.
 */
fixup(stream)
register FILE *stream;
{
	fprintf(stream, "bad wtmp: offset %lu.\n", ftell(stdin)-sizeof(wb));
	fprintf(stream, "bad record is:  %.12s\t%.8s\t%lu",
		wb.ut_line,
		wb.ut_name,
		wb.ut_time);
	fprintf( stream, "\t%s\n", ctime(&wb.ut_time));
	exitcode = 1;
}

loop()
{
	register timediff;
	register struct tbuf *tp;
	int i;

	if( wb.ut_line[0] == '\0' )	/* It's an init admin process */
		return;			/* no connect accounting data here */
	switch(wb.ut_type) {
	case OLD_TIME:
		datetime = wb.ut_time;
		return;
	case NEW_TIME:
		if(datetime == 0)
			return;
		timediff = wb.ut_time - datetime;
		/* update ttime for all entries */
		for (i = 0; i < A_TSIZE; i++) {
			tp = ttylist[i];
			while(tp != NULL) {
				tp->ttime += timediff;
				tp = tp->fwptr;
			}
		}
		datetime = 0;
		ndates++;
		return;
	case BOOT_TIME:
		upall();
	case ACCOUNTING:
	case RUN_LVL:
		lastime = wb.ut_time;
		bootshut();
		return;
	case USER_PROCESS:
	/* DSDe407380: switch between LOGIN_PROCESS and DEAD_PROCESS 
	   so that the connect time is calculated as the time from 
	   USER_PROCESS to DEAD_PROCESS, instead of from USER_PROCESS 
	   to LOGIN_PROCESS.
	*/
	case DEAD_PROCESS:
	case INIT_PROCESS:
		update(iline());	/* iline now returns pointer */
		return;
	case LOGIN_PROCESS:
	case EMPTY:
		return;
	default:
		fprintf(stderr, "acctcon1: Invalid type %d for %s %s %s\n",
			wb.ut_type,
			wb.ut_name,
			wb.ut_line,
			ctime(&wb.ut_time));
	}
}

/*
 * bootshut: record reboot (or shutdown)
 * bump count, looking up wb.ut_line in sy table
 */
bootshut()
{
	register i;

	for (i = 0; i < nsys && !EQN(wb.ut_line, sy[i].sname); i++)
		;
	if (i >= nsys) {
		if (++nsys > NSYS) {
			fprintf(stderr, "acctcon1: Reason table overflow\n");
			nsys = NSYS;
			return;
		}
		CPYN(sy[i].sname, wb.ut_line);
	}
	sy[i].snum++;
}

/*
 * iline: look up/enter current line name in tbuf, return pointer
 * (used to avoid system dependencies on naming)
 */
struct tbuf *iline()
{
	register struct tbuf **ftb;
	register struct tbuf *tb;
	int i, len, entry;

	/* Get hash table index from string name (tty name), by getting
	 * the weighted sum of the last four characters in the name */
	len = strlen(wb.ut_line);	/* get entry in the hash table */
	entry = 0;
	i = 4;
	while(len && i) entry += wb.ut_line[--len] * i--;

	ftb = &ttylist[entry % A_TSIZE];
	while(*ftb != NULL) {		/* search linked list */
		if (EQN(wb.ut_line, (*ftb)->tline))
			break;		/* found entry */
		ftb = &((*ftb)->fwptr);
	}
	if(*ftb == NULL) {		/* add new entry */
		/* allocate space */
    		tb = (struct tbuf *) calloc(1, sizeof(struct tbuf));
		if(tb == NULL) {
			fprintf(stderr, "acctcon1: Out of memory\n");
			exit(2);
		}
		*ftb = tb;
		tb->fwptr = NULL;
		CPYN(tb->tline, wb.ut_line);
		tb->tdev = lintodev(wb.ut_line);
		return(tb);
	} else				/* found entry */
		return(*ftb);
}


upall()
{
	register struct tbuf *tp;
	int i;
	wb.ut_type = INIT_PROCESS;	/* fudge a logoff for reboot record */
	for (i = 0; i < A_TSIZE; i++) {
		tp = ttylist[i];
		while(tp != NULL) {
			update(tp);	/* update for all entries */
			tp = tp->fwptr;
		}
	}
}

/*
 * update tbuf with new time, write ctmp record for end of session
 */
update(tp)
struct tbuf *tp;
{
	long	told,	/* last time for tbuf record */
		tnew;	/* time of this record */
			/* Difference is connect time */

	told = tp->ttime;
	tnew = wb.ut_time;
	if (told > tnew) {
		fprintf(stderr, "acctcon1: Bad times: old: %s", ctime(&told));
		fprintf(stderr, "new: %s\n", ctime(&tnew));
		exitcode = 1;
		tp->ttime = tnew;
		return;
	}
	tp->ttime = tnew;
	switch(wb.ut_type) {
	case USER_PROCESS:
		tp->tlsess++;
		if(tp->tname[0] != '\0') { /* Someone logged in without */
					   /* logging off. Put out record. */
			cb.ct_tty = tp->tdev;
			CPYN(cb.ct_name, tp->tname);
			cb.ct_uid = namtouid(cb.ct_name);
			cb.ct_start = told;
			pnpsplit(cb.ct_start, tnew-told, cb.ct_con);
			prctmp(&cb);
			tp->ttotal += tnew-told;
		}
		else	/* Someone just logged in */
			tp->tlon++;
		CPYN(tp->tname, wb.ut_name);
		break;
	case INIT_PROCESS:
	case DEAD_PROCESS:
		tp->tloff++;
		if(tp->tname[0] != '\0') { /* Someone logged off */
			/* Set up and print ctmp record */
			cb.ct_tty = tp->tdev;
			CPYN(cb.ct_name, tp->tname);
			cb.ct_uid = namtouid(cb.ct_name);
			cb.ct_start = told;
			pnpsplit(cb.ct_start, tnew-told, cb.ct_con);
			prctmp(&cb);
			tp->ttotal += tnew-told;
			tp->tname[0] = '\0';
		}
	}
}

printrep()
{
	register i;

	freopen(report, "w", stdout);
	printf("from %s", ctime(&firstime));
	printf("to   %s", ctime(&lastime));
	if (ndates)
		printf("%d\tdate change%c\n",ndates,(ndates>1 ? 's' : '\0'));
	for (i = 0; i < nsys; i++)
		printf("%d\t%.12s\n", sy[i].snum, sy[i].sname);
}

/*
 *	print summary of line usage
 *	accuracy only guaranteed for wtmp file started fresh
 */
printlin()
{
	register struct tbuf *tp;
	double timet, timei;
	double ttime;
	int tsess, ton, toff;
	int i;

	freopen(replin, "w", stdout);
	ttime = 0.0;
	tsess = ton = toff = 0;
	timet = MINS(lastime-firstime);
	printf("TOTAL DURATION IS %.0f MINUTES\n", timet);
	printf("LINE\t\tMINUTES\tPERCENT\t# SESS\t# ON\t# OFF\n");
	for (i = 0; i < A_TSIZE; i++) {
		tp = ttylist[i];
		while(tp != NULL) {	/* print all entries */
			timei = MINS(tp->ttotal);
			ttime += timei;
			tsess += tp->tlsess;
			ton += tp->tlon;
			toff += tp->tloff;
			printf("%-12s\t%-3.0f\t%-3.0f\t%d\t%d\t%d\n",
				tp->tline,
				timei,
				(timet > 0.)? 100*timei/timet : 0.,
				tp->tlsess,
				tp->tlon,
				tp->tloff);
			tp = tp->fwptr;
		}
	}
	printf("TOTALS\t\t%.0f\t--\t%d\t%d\t%d\n", ttime, tsess, ton, toff);
}

prctmp(t)
register struct ctmp *t;
{

	printf("%u\t%u\t%.8s\t%lu\t%lu\t%lu",
		t->ct_tty,
		t->ct_uid,
		t->ct_name,
		t->ct_con[0],
		t->ct_con[1],
		t->ct_start);
	printf("\t%s", ctime(&t->ct_start));
}

