static char *HPUX_ID = "@(#) $Revision: 66.9 $";
/*$Header: date.c,v 66.9 90/04/18 15:41:39 ec Exp $ (Hewlett-Packard) */

/*
**	date - with format capabilities
*/

#include	<sys/types.h>
#include	<utmp.h>
#include        <stdio.h>
#include	<time.h>
#include	<unistd.h>

#ifdef NLS16
#include	<nl_ctype.h>
#else NLS16
#define		WCHARADV(c,p)		*(p)++ = (c)
#endif NLS16

#ifdef NLS
#include	<nl_types.h>
#include	<locale.h>
#define NL_SETN  1
#else NLS
#define		catgets(i, j, k, s)	(s)
#endif NLS

#define	dysize(A) (((A)%4)? 365: 366)
#define USAGEMSG "Usage: date [-u] [ mmddhhmm[yy] ] [ +format ]\n"

short mon_sum[] = {0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334};

char	*cbp;
char	*tzn;
time_t	timbuf;

extern int daylight;
extern char __nl_derror;
#ifdef NLS
extern int __nl_langid[];
#endif NLS

struct	utmp	wtmp[2] = { {"","",OTIME_MSG,0,OLD_TIME,0,0,0},
			    {"","",NTIME_MSG,0,NEW_TIME,0,0,0} };

main(argc, argv)
int	argc;
int	**argv;
{
	int		tfailed, wf;
	char		date_buf[BUFSIZ];
	char		*dptr;
	register char	*format;
	char		c;
	extern char	*optarg;
	extern int	optind,opterr;
#ifdef NLS
	nl_catd		catd;

	if (!setlocale(LC_ALL,"")) {
		fputs(_errlocale("date"), stderr);
		putenv("LANG=");
		catd = (nl_catd)-1;
	} else
		catd = catopen("date", 0);
#endif NLS

        /*
         *  Use the kernel's id of timezone info when date is
         *  being run as a login shell.  This will use the
         *  default rules for changing to/from Daylight Savings
         *  Time (which won't always be correct for all users
         *  but is the best we can do without modifying the
         *  kernel to return a real TZ string).
         */
        if (!strcmp("-date", argv[0])) {
                struct timeval tp;
                struct timezone tzp;
                char buf[20];
                (void) gettimeofday(&tp, &tzp);
                if (tzp.tz_dsttime)
                        (void) sprintf(buf, "TZ=<NO TZ NAME>%d<NO TZ NAME>", tzp.tz_minuteswest/60);
                else
                        (void) sprintf(buf, "TZ=<NO TZ NAME>%d", tzp.tz_minuteswest/60);
                putenv(buf);
        }

	tfailed = 0;
	while ((c=getopt(argc,argv,"u"))!=EOF)
		switch(c) {
			case 'u':
				putenv("TZ=UTC0");
				break;
			case '?':
				(void) fputs(catgets(catd, NL_SETN, 2, USAGEMSG), stderr);
				exit(2);
		}
	if(argv[optind]) {
		cbp = (char *)argv[optind];

		if(*cbp == '+')	{
			(void) time(&timbuf);
			format = (char *)argv[optind];
			format++;
			if (!*format)
				(void) putchar('\n');
			else {
				if (!strftime(date_buf, BUFSIZ, format, localtime(&timbuf)))
					date_buf[0] = '\0';		/* if no formatted date returned */
				if (__nl_derror == '\0') {
					(void) fputs(date_buf, stdout);
					(void) putchar('\n');
				} else {
					dptr = date_buf;
					WCHARADV(__nl_derror, dptr);
					*dptr = '\0';
					(void) fputs(catgets(catd, NL_SETN, 1, "date: bad format character - "), stderr);
					(void) fputs(date_buf, stderr);
					(void) putc('\n', stderr);
					exit(2);
				}
			}
			exit(0);
		}

		if(*cbp == '-') {
			(void) fputs(catgets(catd, NL_SETN, 2, USAGEMSG), stderr);
			exit(2);
		}

	/* convert the input time to numeric value,  assume Greenwich time */
		if(gtime()) {
			(void) fputs(catgets(catd, NL_SETN, 3, "date: bad conversion\n"), stderr);
			exit(2);
		}

	/* convert to Greenwich time, on assumption of Standard time. */
		timbuf += timezone;

	/* Now fix up to local daylight time. */
		if (localtime(&timbuf)->tm_isdst)
			timbuf += -1*60*60;

		(void) time(&wtmp[0].ut_time);

		/* make sure the user wants to run time backwards */
		if (timbuf < wtmp[0].ut_time) {
			 char resp[40];
			 (void) fputs(catgets(catd, NL_SETN, 4, "date: do you really want to run time backwards?[yes/no]"), stdout);
			 (void) fflush(stdout);
			 (void) gets(resp);
			 if (strcmp(resp,(catgets(catd, NL_SETN, 5, "yes"))) != 0) {
			    (void) fputs(catgets(catd, NL_SETN, 6, "Only \"yes\" will make it take!\n"), stdout);
			    exit(1);
		    	 }
		}

		if(stime(&timbuf) < 0) {
			tfailed++;
			(void) fputs(catgets(catd, NL_SETN, 7, "date: no permission\n"), stderr);
			exit(tfailed?2:0);
		}
		else {
			(void) time(&wtmp[1].ut_time);

/*	Attempt to write entries to the utmp file and to the wtmp file.	*/

			pututline(&wtmp[0]) ;
			pututline(&wtmp[1]) ;

			if ((wf = open(WTMP_FILE, 1)) >= 0) {
				(void) lseek(wf, 0L, 2);
				(void) write(wf, (char *)wtmp, sizeof(wtmp));
			}
		}
	}

	/*
	 *  Default:  display time
	 */
        if (tfailed==0)
                (void) time(&timbuf);
        /* added Nov '84 native language support */
#ifndef NONLS
        if (__nl_langid[LC_TIME] != C_LANGID && __nl_langid[LC_TIME] != NC_LANGID){
                (void) printf("%s\n", nl_cxtime(&timbuf, ""));
                exit(tfailed?2:0);
        }
#endif NONLS

        cbp = ctime(&timbuf);
        (void) write(1, cbp, 20);
        tzn = tzname[localtime(&timbuf)->tm_isdst];
        if (tzn)
                (void) write(1, tzn, strlen(tzn));
        (void) write(1, cbp+19, 6);
        exit(tfailed?2:0);
}

gtime()
{
	register int i;
	register int y, t;
	int d, h, m;
	time_t nt;

	tzset();
	t = gpair();
	if(t<1 || t>12)
		return(1);
	d = gpair();
	if(d<1 || d>31)
		return(1);
	h = gpair();
	if(h == 24) {
		h = 0;
		d++;
	}
	m = gpair();
	if(m<0 || m>59)
		return(1);
	y = gpair();
	if (y<0) {
		(void) time(&nt);
		y = localtime(&nt)->tm_year;
	}
	if (*cbp == 'p')
		h += 12;
	if (h<0 || h>23)
		return(1);
	timbuf = 0;
	y += 1900;
	if (y<1970)
		y+=100;			/* allow for years past 1999 */
	for(i=1970; i<y; i++)
		timbuf += dysize(i);
	/* Leap year */
	if (dysize(y)==366 && t >= 3)
		timbuf += 1;
	timbuf += mon_sum[t-1];
	timbuf += (d-1);
	timbuf *= 24;
	timbuf += h;
	timbuf *= 60;
	timbuf += m;
	timbuf *= 60;
	return(0);
}


gpair()
{
	register int c, d;
	register char *cp;

	cp = cbp;
	if(*cp == 0)
		return(-1);
	c = (*cp++ - '0') * 10;
	if (c<0 || c>100)
		return(-1);
	if(*cp == 0)
		return(-1);
	if ((d = *cp++ - '0') < 0 || d > 9)
		return(-1);
	cbp = cp;
	return (c+d);
}
