
#ifndef lint
static char *HPUX_ID = "@(#) $Revision: 66.2 $";
#endif
/* /usr/lib/calprog produces an egrep -f file
   that will select today's and tomorrow's
   calendar entries, with special weekend provisions

   used by calendar command
*/

#include <sys/types.h>
#include <stdio.h>
#include <ctype.h>
#include <locale.h>
#include <time.h>
#include <nl_types.h>
#include <langinfo.h>

#define	CALENDAR "calendar"
#define NL_SETN		1
#define	DAY	(3600*24L)

nl_item abmon[] = {	/* map to abbreviated month strings: Jan, etc */
    ABMON_1, ABMON_2, ABMON_3, ABMON_4, ABMON_5, ABMON_6, ABMON_7,
    ABMON_8, ABMON_9, ABMON_10, ABMON_11, ABMON_12
};

nl_item mon[] = {	/* map to abbreviated month strings: Jan, etc */
    MON_1, MON_2, MON_3, MON_4, MON_5, MON_6, MON_7,
    MON_8, MON_9, MON_10, MON_11, MON_12
};
char lang_var[80]="C";
int	months[4], days[4];

main(argc,argv)
int argc;
char **argv;
{
	unsigned char *lptr,*argp,argbuf[80];
	long	t;
	int	i;
	struct	tm *tm;


	if (argc > 1) {
		argp=argbuf;
		strcat(argp,argv[1]);
		get_LANG(argp);
	} else 
		get_LANG(CALENDAR);

#if defined NLS || defined NLS16	/* initialize to the right language */

	if (!(setlocale(LC_ALL,lang_var))) 
		fputs(_errlocale("calendar"),stderr);

#endif NLS || NLS16

	/*
	 * Build an array of months and days to be egrep'd for.
	 * Normally today and tomorrow, but extra days if today
	 * is Friday or Saturday.
	 */
	time(&t);

	/* Extract today's month and day */
	tm = localtime(&t);
	months[0] = tm->tm_mon+1;
	days[0] = tm->tm_mday;

	i = 1;
	switch(tm->tm_wday) {
	case 5:
		/* Friday gets 3 extra (Sat - Mon) */
		t += DAY;
		tm = localtime(&t);
		months[i] = tm->tm_mon+1;
		days[i++] = tm->tm_mday;
	case 6:
		/* Saturday gets 2 extra (Sun - Mon) */
		t += DAY;
		tm = localtime(&t);
		months[i] = tm->tm_mon+1;
		days[i++] = tm->tm_mday;
	default:
		/* and all other days just get tomorrow */
		t += DAY;
		tm = localtime(&t);
		months[i] = tm->tm_mon+1;
		days[i++] = tm->tm_mday;
	}

	/*
	 * Build and output the egrep pattern(s) for the days selected.
	 */
	tprint();

	exit(0);
}

get_LANG(p)
char *p;
{
	FILE	*fp;
	char *line,tmp[40],lang_buf[80],*tp;
	unsigned char *lptr;
	int	i,ch;
	
	if ((fp=fopen(p,"r")) == NULL) {
		return(0);
	}
	line=lang_buf;
	if (fgets(line,80,fp) != NULL) 
	{
		while (*line == ' ')
			line++;
		for (i=0; i<4; i++) {
			ch=toupper((int)*line++);
			tmp[i]=(char) ch;
		}
		if (strncmp(tmp, "LANG", 4) == 0 ) {
			while (*line == ' ')
				line++;
			if (*line == '=') {
	 			line++; /*skip over '=' */
				while (*line == ' ')
					line++;
				i=0;
				while (isgraph((int) *line))
					tmp[i++]=*line++;
				tmp[i]='\0';
				if (strcmp(tmp,""))
					strcpy(lang_var,tmp);
			}
		} 
		else

	/* if no LANG= string in calendar file, then call setlocale with
	   an empty string.  Therefore calendar will be sensitive to LANG
	   and LC_TIME environment variable */

			strcpy(lang_var,"");	
	}
	fclose(fp);
}

tprint()
{
	unsigned char	s1[12], *s2;
	/* unsigned char s3[20]; */
	int	ch;
	int	i, month, first;
	int	multibyte;
	
	/*
	 * Asian (i.e., "multibyte") language?
	 */
	multibyte = (*(unsigned char *)nl_langinfo(BYTES_CHAR) - '1' > 0);

	/*
	 * Get the date format string and from that determine
	 * if we should check for month/day or day/month formats.
	 */
	s2=(unsigned char *)nl_langinfo(D_FMT);
	ch=det_fmt(s2);

	/*
	 * Process each date, but only print one format for any
	 * given month.  For example, instead of looking for
	 *       May 19
	 *       May 20
	 * look for
	 *       May (19|20)
	 */
	i = 0;
	while (i < 4 && months[i]) {
		/* current month */
		month = months[i];

		/* get current month's abbreviated name */
		s2=(unsigned char *)nl_langinfo(abmon[month-1]);
		s1[0]='\0';
		catstr(s1,s2);

		/*
		 * I guess someone decided they didn't want the following:
		 *
		s2=(unsigned char *)nl_langinfo(mon[month-1]);
		s3[0]='\0';
		catstr(s3,s2);
		 */

		/*
		 * If the date is Feb 25th, don't match "12/25".
		 *
		 * The explicit list of punctuation used for the non-Asian
		 * languages is much faster, but does not work for Asian
		 * dates where the month number is usually preceded by a
		 * kanji character instead of some sort of punctuation.
		 */ 
		if (multibyte)
			printf("(^|[^0-9])");		/* for Asian dates */
		else
			printf("(^|[- \t(,;\"[/])");	/* for non-Asian dates */

		/*
		 * Pattern appearing before the day for the month/day
		 * else the day/month format.
		 */
		if (ch == 1)
			printf("((%s[^ ]* *|0*%d[-/]|\\*/)0*", s1, month);
		else
			printf("(0*");
		/*
		 * Produce an OR list of this month's day numbers.
		 */
		first = 1;
		while (i < 4 && month == months[i]) {
			printf("%s%d", first ? "(" : "|", days[i++]);
			first = 0;
		}

		/*
		 * Finish up the month/day
		 * else the day/month formats.
		 */
		if (ch == 1)
			printf("))([^0-9]|$)\n");
		else
			printf("))((.? *%s)|([-/]0*%d[^0-9])|/\\*)\n",
				s1, month);
	}
}

catstr(a1,a2)
unsigned char *a1;
unsigned char *a2;
{
	char	c[3];
	int	ch;
	
	ch=(int) *a2;
        if (isalpha(ch)) {
		strcat(a1,"[");
		c[0]=(char)toupper(ch);
		c[1]=(char)tolower(ch);
		c[2]='\0';
		strcat(a1,c);
		strcat(a1,"]");
		strcat(a1,++a2);
	} else strcpy(a1,a2);
}

int
det_fmt(fmt)
unsigned char *fmt;
{

	char ch;
	int	path;

	path=0;
	for (;;) {
		if ((ch=*fmt++) == '\0') {
			if (!path) return(1);
			else {
				return(path%2);
			}
		};
		while ((ch != '%') && (ch != '\0')) 
			ch= *fmt++;

		if (( ch== '%' ) && (*fmt == '%')) continue;
		
		if (ch== '%')   {
			while  ( !(isalpha((int) ch)) && (ch != '\0'))
				ch = *fmt++;
			switch (ch) {
				case 'm':
				case 'h':	/* obsolete -- %b is the new equivalent */
				case 'F':	/* obsolete -- %B is the new equivalent */
				case 'b':
				case 'B': path=path*10 + 2;
					  break;
				case 'd': path=path*10 + 3;
					  break;
				case 'D': path=231;
					  break;
				default:
					  ;
			}
		}
	}
}
