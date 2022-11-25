static char *HPUX_ID = "@(#) $Revision: 70.6 $";


#include <stdio.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <pwd.h>
#include <time.h>
#include <nl_types.h>
#define NL_SETN 1

#define	dysize(A) (((A)%4)? 365: 366)
#define ONE_YEAR 31536000	/* number of seconds in 1970 */

#define DOUBLE_DASH  ((strcmp(argv[optind-1], "--")==0))


struct	stat	stbuf;
struct	stat	refbuf;
int	status;
int dmsize[12]={31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};

char	*cbp;
char *timestr;
time_t	timbuf, time();
extern time_t timezone;


main(argc, argv)
char *argv[];
{
	register c;
	struct utbuf { time_t actime, modtime; } times;
	int	tflag = 0, ref = 0;
	int	timelength;     

	int mflg=1, aflg=1, cflg=0, nflg=0, errflg=0, optc, fd;
	int ret;
	extern char *optarg;
	extern int optind, opterr;
	nl_catd catd;

	catd = catopen("touch",0);

	while ((optc=getopt(argc, argv, "amcr:t:")) != EOF)
		switch(optc) {
		case 'm':
			mflg++;
			aflg--;
			break;
		case 'a':
			aflg++;
			mflg--;
			break;
		case 'c':
			cflg++;
			break;
		case 't':
			timestr = optarg;
			ret = gtime(1);
			if(ret > 0) {
				fputs(catgets(catd,NL_SETN,1,"date: bad conversion\n"), stderr);
				exit(2);
			}
		/* POSIX.2 specifically disallows any date before the
		    Epoch  */
			if(ret < 0) {
			    fputs(catgets(catd,NL_SETN,2,"date: before Jan 1, 1970, GMT\n"),stderr);
			    exit(2);
			}
			tflag++;
			break;
		case 'r':
			if(stat(optarg, &refbuf)) {
				fprintf(stderr,catgets(catd,NL_SETN,3,"touch: %s cannot stat\n"),argv[optind]);
				exit(2);
			}
			ref++;
			break;

		case '?':
			errflg++;
		}

	if((tflag && ref) || (ref > 1) || (tflag > 1) || 
	    ((argc-optind) < 1) || (errflg) )
	{
		fputs(catgets(catd,NL_SETN,4,"usage: touch [-amc] [-t [[CC]YY]MMDDhhmm[.SS]] [-r ref_file] file ...\n"),stderr);
		exit(2);
	}
	status = 0;
	if(!tflag && !ref) {   /* for backward compatibility	*/
		if(DOUBLE_DASH || !isnumber(argv[optind]) || ((argc-optind) == 1) ||
		   ((timelength = strlen(argv[optind])) != 8 && timelength != 10) )
			if((aflg <= 0) || (mflg <= 0))
				timbuf = time((long *) 0);
			else
				nflg++;
		else {
			timestr = (char *)argv[optind++];
			ret = gtime(0);
			if(ret > 0) {
				fputs(catgets(catd,NL_SETN,5,"date: bad conversion\n"), stderr);
				exit(2);
			}
		        /*disallow any date before the Epoch  */
			if(ret < 0) {
			    fputs(catgets(catd,NL_SETN,6,"date: before Jan 1, 1970, GMT\n"),stderr);
			    exit(2);
			}
		}
	}
	for(c=optind; c<argc; c++) {
		if(stat(argv[c], &stbuf)) {
			if(cflg) {
/*
 * POSIX.2 change: exit status should be 0 when -c option is specified and
 * the file does not exist. (assertion 16 in P1003.3.2/D7)   AL 2/10/92
				status++; */
				continue;
			}
			else if ((fd = creat (argv[c], 0666)) < 0) {
				fprintf(stderr,catgets(catd,NL_SETN,7,"touch: %s cannot create\n"),argv[c]);
				status++;
				continue;
			}
			else {
				(void) close(fd);
				if(stat(argv[c], &stbuf)) {
					fprintf(stderr,catgets(catd,NL_SETN,8,"touch: %s cannot stat\n"), argv[c]);
					status++;
					continue;
				}
			}
		}
		if(ref)
		{
			times.modtime = refbuf.st_mtime;
			times.actime = refbuf.st_atime;
		}
		else
			times.actime = times.modtime = timbuf;
		if (mflg <= 0)
			times.modtime = stbuf.st_mtime;
		if (aflg <= 0)
			times.actime = stbuf.st_atime;
		if(utime(argv[c], (struct utbuf *)(nflg? 0: &times))) {
			fprintf(stderr,catgets(catd,NL_SETN,9,"touch: cannot change times on %s\n"), argv[c]);
			status++;
			continue;
		}
	}
	if (status != 0)
	    status = (status % 255) ? status : 1; /* prevent returning 0 */
	exit(status);                         /* if status == 255 */
}

/*	gtime calculates the time in seconds from the given date
**	date string.  
**
**	Returns >0 if the time cannot be calculated because any of the
**	components are out of range.
**	Returns <0 if the calculated time is earlier than the Epoch,
**	(with appropriate allowances made for timezone.
*/
gtime(t)
int t;
{
	register int i, y;
	long nt, diffrnce;
	char *time_end;
	int secs=0, mins=0, hours=0, days=0, mons=0, yrs=100, cent=0;
	int inlength;
	int roll = 0;

	tzset();

#ifdef DEBUG
	printf("timestr = %s,  t = %d\n",timestr,t);
#endif
	
	/* set a pointer to the end of the string, then back up 2 */
	inlength = strlen(timestr);
	if (inlength < 2) return(1);
	time_end = timestr + inlength;
	time_end -= 2;

	/* if the -t option was used, the last two chars are seconds if
	 * they follow a '.' */
	if (t) {
	    if(*(time_end - 1) == '.') {
		time_end--;
		*time_end = NULL;
	secs = atoi(time_end + 1);
#ifdef DEBUG
	printf("Last two chars: %s\n",(time_end +1));
	printf("secs = %d\n",secs);
#endif
	    if (secs < 0 || secs > 61)
		return(1);
	    time_end -= 2;
	    }
	}
	else { /* old */
	    /*  If the old-style syntax was used, the last 2 chars are
	     *  years if the length of the string is greater than 8 */
 	    if (inlength == 10) {
	        yrs = atoi(time_end);
	        *time_end = NULL;
	        time_end -= 2;
	    }
	}
	
	mins = atoi(time_end);
	if (mins < 0 || mins > 59)
	    return(1);
	*time_end = NULL;
	time_end -=2;

	hours = atoi(time_end);
	if (hours < 0 || hours > (t ? 23 : 24))
	    return(1);
	*time_end = NULL;
	time_end -= 2;

	days += atoi(time_end);
	if (days < 1 || days > 31)
	    return(1);
	*time_end = NULL;
	time_end -= 2;

	mons = atoi(time_end);
	if (mons < 1 || mons > 12)
	    return(1);

	/* If there are more chars, the next two back are the year */
	if (time_end > timestr) {
	    *time_end = NULL;
	    time_end -= 2;
	    yrs = atoi(time_end);
	    /*  If there are more chars yet, they are the century */
	    if (time_end > timestr) {
	        *time_end = NULL;
		time_end -= 2;
		cent = atoi(time_end);
		if (cent < 19 || cent > 20)
		    return(1);
	    }
	}
	/* if not back to the beginning, something is wrong */
	if (time_end != timestr)
	    return(1);

	/* if no yrs value was extracted from timestr, use current year */
	if (yrs == 100) {
	    (void) time(&nt);
	    yrs = localtime(&nt)->tm_year;
	}

	if (!t) 
	    cent = 19;
	else {   	/*  -t option in effect */
	    if(!cent)
	        if(yrs >= 69)	/*  If no century given w/ -t, key off */
		    cent = 19;	/*  year; if earlier than '69, assume */
	        else		/*  roll-around to 21st century */
		    cent = 20;
	}
	

	y = yrs + cent*100;

	/* leap second(s) */
	if(secs > 59){
	    mins++;
	    secs = 0;
	}

	/* hours of 24 allowed in old syntax, not in new */
	if(hours == 24) {
	    days++;
	    hours = 0;
	}

	timbuf = 0;

	for(i=1970; i<y; i++)
		timbuf += dysize(i);
	/* Leap year */
	if (dysize(y)==366 && mons >= 3)
		timbuf += 1;
	while(--mons)
		timbuf += dmsize[mons-1];
	timbuf += (days-1);
	timbuf *= 24;
	timbuf += hours;
	timbuf *= 60;
	timbuf += mins;
	timbuf *= 60;
	timbuf += secs;

	/* timezone adjusted times late on Dec 31, 1969 may be legal 
	 * If POSIX -t syntax is used, report an error, otherwise let
	 * it slide with year adjusted to 1970. 
	 */
	diffrnce = ONE_YEAR - timbuf;
	if(y <= 1969 && (diffrnce > timezone || diffrnce <= 0) || y < 1969)
	    if(t)
	        return(-1);
	if(y == 1969 && diffrnce < timezone)
	    timbuf = diffrnce;
	else {
	    timbuf += timezone;
	    if(localtime(&timbuf)->tm_isdst)
		timbuf -= 3600;		/* Subtract an hour if DST in effect */
	}
	    
	return(0);
}
isnumber(s)
char *s;
{
	register c;

	while(c = *s++)
		if(!isdigit(c&0377))
			return(0);
	return(1);
}
