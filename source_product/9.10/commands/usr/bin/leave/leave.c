static char *HPUX_ID = "@(#) $Revision: 70.1 $";
#include <stdio.h>
#include <signal.h>
#include <time.h>

/*
 * leave [hhmm]
 *
 * Reminds you when you have to leave.
 * Leave prompts for input and goes away if you hit return.
 * It nags you like a mother hen.
 */

extern char *strcpy();
extern void exit();
extern unsigned long sleep();
extern pid_t fork();

char origlogin[20], thislogin[20];
char *getlogin();
char *whenleave;
char *ctime();
char buff[100];

main(argc,argv)
char **argv;
{
	long when, tod, now, diff, hours, minutes;
	struct tm *nv;
	int atoi();
	int i;

	if (argc < 2) {
		fputs("When do you have to leave? ", stdout);
		fflush(stdout);
		gets(buff);
	} else {
		strcpy(buff,argv[1]);
	}

	if (buff[0] == '\0')
		exit(0);
	if (buff[0] == '+') {
		diff = atoi(buff+1);
		strcpy(origlogin,getlogin());
		doalarm(diff);
	}
        for (i=0; buff[i]; i++)
	    if (buff[i] < '0' || buff[i] > '9') {
	    Usage:
		fputs("usage: ", stdout);
		fputs(argv[0], stdout);
		fputs(" [hhmm]\n", stdout);
		exit(1);
	    }
	strcpy(origlogin,getlogin());

	tod = atoi(buff);
	hours = tod / 100;
	if (hours > 12)
		hours -= 12;
	if (hours == 12)
		hours = 0;
	minutes = tod % 100;

	if (hours < 0 || hours > 12 || minutes < 0 || minutes > 59)
		goto Usage;
	time(&now);
	nv = localtime(&now);
	when = 60*hours+minutes;
	if (nv->tm_hour > 12) nv->tm_hour -= 12;	/* do am/pm bit */
	now = 60*nv->tm_hour + nv->tm_min;
	diff = when - now;
        /*printf(" ",stdout); */
        fputs("", stdout);
	while(diff < 0)
		diff += 12*60;
	doalarm(diff);
	return 0;
}


doalarm(nmins)
long nmins;
{
	char *msg1, *msg2, *msg3, *msg4;
	int slp1, slp2, slp3, slp4;
	int seconds, gseconds;
	long daytime;

	seconds = 60 * nmins;
	if (seconds <= 0)
		seconds = 1;
	gseconds = seconds;

	msg1 = "You have to leave in 5 minutes";
	if (seconds <= 60*5) {
		slp1 = 0;
	} else {
		slp1 = seconds - 60*5;
		seconds = 60*5;
	}

	msg2 = "Just one more minute!";
	if (seconds <= 60) {
		slp2 = 0;
	} else {
		slp2 = seconds - 60;
		seconds = 60;
	}

	msg3 = "Time to leave!";
	slp3 = seconds;

	msg4 = "You're going to be late!";
	slp4 = 60;

	time(&daytime);
	daytime += gseconds;
	whenleave = ctime(&daytime);

	fputs("Alarm set for ", stdout);
	fputs(whenleave, stdout);
	fputc('\n', stdout);

	if (fork())
		exit(0);
	signal(SIGINT, SIG_IGN);
	signal(SIGQUIT, SIG_IGN);
	signal(SIGTERM, SIG_IGN);
#ifdef	SIGTSTP
	/*
	 * Avoid silent death on write on job-control systems
	 * since we're init's orphan.
	 */
	signal(SIGTTOU, SIG_IGN);
#endif	SIGTSTP

	if (slp1)
		bother(slp1,msg1);
	if (slp2)
		bother(slp2,msg2);
	bother(slp3,msg3);
	for (;;) {
		bother(slp4,msg4);
	}
}

bother(slp,msg)
int slp;
char *msg;
{

	delay(slp);
	fputs("\7\7\7", stdout);
	fputs(msg, stdout);
	fputc('\n', stdout);
}

/*
 * delay is like sleep but does it in 100 sec pieces and
 * knows what zero means.
 */
delay(secs) int secs; {
	int n;

	while(secs>0) {
		n = 100;
		secs = secs - 100;
		if (secs < 0) {
			n = n + secs;
		}
		if (n > 0)
			sleep(n);
		strcpy(thislogin,getlogin());
		if (strcmp(origlogin, thislogin))
			exit(0);
	}
}

#ifdef V6
char *getlogin() {
#include <utmp.h>

	static struct utmp ubuf;
	int ufd;

	ufd = open("/etc/utmp",0);
	seek(ufd, ttyn(0)*sizeof(ubuf), 0);
	read(ufd, &ubuf, sizeof(ubuf));
	ubuf.ut_name[sizeof(ubuf.ut_name)] = 0;
	return(&ubuf.ut_name);
}
#endif
