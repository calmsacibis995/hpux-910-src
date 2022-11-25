/* @(#) $Revision: 70.3 $ */
/* $Header: pdt.c,v 70.3 92/07/06 10:17:07 ssa Exp $ (Hewlett-Packard) */

#include <nl_ctype.h>
#include <sys/types.h>
#include <stdio.h>
#include <ctype.h>
#include <time.h>
#include <nl_types.h>

#define	PTBADARG	-1
#define	PTNONLS		-2
#define	PTINVAL1	-3
#define	PTINVAL2	-4
#define	PDBADARG	-5
#define	PDNONLS		-6
#define	PDNODFMT	-7
#define	PDINVAL		-8
#define	PDAMBIG		-9

#define         ZULU    "zulu"
#define         UTC     "utc"

#define         NOON    	1
#define         MIDNIGHT 	2
#define         NOW     	3
#define		TODAY		4
#define		TOMORROW	5
#define		MINUTE		6
#define		MINUTES		7
#define		HOUR		8
#define		HOURS		9
#define		DAY		10
#define		DAYS		11
#define		WEEK		12
#define		WEEKS		13
#define		MONTH		14
#define		MONTHS		15
#define		YEAR		16
#define		YEARS		17
#define		NEXT		18

#define         NL_SETN    3
#define BADDATE		(catgets(nlmsg_fd,NL_SETN,21, "bad date specification"))   
#define BADFIRST	(catgets(nlmsg_fd,NL_SETN,22, "bad time specification"))  
#define BADHOURS	(catgets(nlmsg_fd,NL_SETN,23, "hours field is invalid")) 
#define BADMD		(catgets(nlmsg_fd,NL_SETN,24, "bad date"))	
#define BADMINUTES	(catgets(nlmsg_fd,NL_SETN,25, "bad time"))  
#define BADNLSENV	(catgets(nlmsg_fd,NL_SETN,26, "bad NLS environment"))	
#define AMBIGDATE	(catgets(nlmsg_fd,NL_SETN,27, "the date argument is ambiguous")) 
#define BADCATFILE      (catgets(nlmsg_fd,NL_SETN,28, "bad catalogue file"))	

extern unsigned char argpbuf[80];
extern struct tm at,*tp,rt;
extern int gmtflag;
extern unsigned char *parsedate();
extern unsigned char *parsetime();

unsigned char *argp = NULL;
char *  INCREMENT[] = {
	"noon",		/* catgets 	1 */
	"midnight",	/* catgets	2 */
	"now",		/* catgets	3 */
	"today",	/* catgets	4 */
	"tomorrow",	/* catgets	5 */
	"minute",	/* catgets	6 */
	"minutes",	/* catgets	7 */
	"hour",		/* catgets	8 */
	"hours",	/* catgets	9 */
	"day",		/* catgets	10 */
	"days",		/* catgets	11 */
	"week",		/* catgets	12 */
	"weeks",	/* catgets	13 */
	"month",	/* catgets	14 */
	"months",	/* catgets	15 */
	"year",		/* catgets	16 */
	"years",	/* catgets	17 */
	"next"		/* catgets	18 */
};

extern nl_catd	nlmsg_fd;
extern nl_catd	nltime_fd;

parsedatetime()
{

        int status, i, nomatch;
	int num,d;
	unsigned char *np,*msg,*strmatch();

        argp=argpbuf;

/* parse time field 
 */
        if (isdigit((int) *argp) ) {
                np=parsetime(argp,&at,&status,2);
                if (status != 0) {
                        switch (status) {
                        case PTBADARG:
                                        atabort(BADFIRST);
                                        break;
                        case PTNONLS:
                                        atabort(BADNLSENV);
                                        break;
                        case PTINVAL1:
                                        atabort(BADHOURS);
                                        break;
                        case PTINVAL2:
                                        atabort(BADMINUTES);
                                        break;
                        }
                } else argp=np;
        } else {
                nomatch=1;
                for (i=1;(nomatch && i < 4);i++) {
                        msg=(unsigned char *)catgets(nltime_fd,NL_SETN,i,
				INCREMENT[i-1]);
			if (msg == NULL)
                                atabort(BADCATFILE);
                        if ((np=strmatch(argp,msg)) != argp)
                                nomatch=0;
		}
		if (!nomatch) {
			switch (i-1) {
			case NOON:
					at.tm_hour=12;
					break;
			case MIDNIGHT:
					at.tm_hour=0;
					break;
			case NOW:		
					/*
					 *  copy the entire structure.  We need
					 *  more than just the hour and
					 *  minute.
					 */
					at=*tp;
					break;
			}
			argp=np;
		} else atabort(BADFIRST);
	}
	while (*argp == ' ')
		argp++;
	if (*np == '\0' || np == NULL) {
		nodate();
		return;
	} else {
		argp=np;
		if (isalpha((int) *argp)) {
			if (((np=strmatch(argp,ZULU)) != argp) ||
			    ((np=strmatch(argp,UTC)) != argp)) {
				if (at.tm_hour == 24 && at.tm_min != 0)
					atabort(BADHOURS);
				at.tm_hour %= 24;
				gmtflag =1;  
				if (*np == '\0') {
					nodate();
					return;
				} else argp=np;
			}
		}
	}

/* parse date field, if its not increment (+) or the NEXT keyword 
 */
	if ((*argp != '+') &&
	    (strncmp(argp, catgets(nltime_fd,NL_SETN, NEXT, "next") , 4))) {
        	nomatch=1;
        	if (!isdigit((int) *argp)) {
                	for (i=TODAY;(nomatch && i < TOMORROW+1);i++) {
                        	msg=(unsigned char *)catgets(nltime_fd,NL_SETN,i,
				    INCREMENT[i-1]);
                        	if (msg==NULL )
                                	atabort(BADCATFILE);
                        	if ((np=strmatch(argp,msg)) != argp)
                                	nomatch=0;
			}
			if (!nomatch) {
				at.tm_mon=tp->tm_mon;
				at.tm_mday=tp->tm_mday;
				at.tm_year=tp->tm_year;
				at.tm_isdst=tp->tm_isdst;
				if ((i-1) == TOMORROW)
					rt.tm_mday +=1;
				argp=np;
			}
		}
		if (nomatch) {
			at.tm_isdst=tp->tm_isdst;
        		np=parsedate(argp,&at,&status,0);
        		if (status != 0) {
                		switch (status) {
                  		case PDBADARG:
                	           		atabort(BADDATE);
                                   		break;
                  		case PDNONLS:
                                   		atabort(BADNLSENV);
                                   		break;
                  		case PDNODFMT:
	   	  		case PDAMBIG:
                                   		atabort(AMBIGDATE);
                                   		break;
		  		case PDINVAL:
			           		atabort(BADMD);
				   		break;
               			}
                	} else  
				argp=np;
		}
	} else nodate();
	if (*np == '\0' || np == NULL) 
		return;
	else {

/* now parse either the increment or NEXT keyword field 
 */
		if (*argp == '+') {
			while (*++argp == ' ');
			if (isdigit((int) *argp)) {
				num=0;
				while (isdigit(d=(int) *argp)) {
					d-=48;
					num=num*10+d;
					argp++;
				}
			} else atabort(BADDATE);
			while (*argp == ' ') argp++;
			nomatch=1;
                	for (i=MINUTE;(nomatch && i < YEARS+1);i++) {
                        	msg=(unsigned char *)catgets(nltime_fd,NL_SETN,i,
				     INCREMENT[i-1]);
                        	if (msg==NULL )
                                	atabort(BADCATFILE);
                        	if ((np=strmatch(argp,msg)) != argp)
                                	nomatch=0;
			}
			if (!nomatch) {
				switch (i-1) {
				case MINUTE:
				case MINUTES:
						rt.tm_min += num;
						break;
				case HOUR:
				case HOURS:
						rt.tm_hour += num;
						break;
				case DAY:		
				case DAYS:		
						rt.tm_mday += num;
						break;
				case WEEK:
				case WEEKS:
						rt.tm_mday += num * 7;
						break;
				case MONTH:
				case MONTHS:
						rt.tm_mon += num;
						break;
				case YEAR:
				case YEARS:	
						rt.tm_year += num;
						break;
				default:
						atabort(BADDATE);
				}
				argp=np;
			} else atabort(BADDATE);

		} else if(strncmp(argp, 
			   catgets(nltime_fd,NL_SETN, NEXT, "next") , 4) == 0) {

			/* Move forward until white space is encountered.
			 * Then move till non-white space is encountered.
			 */
			while (*++argp != ' ');	
			while (*++argp == ' ');	
			
			/* the next argument must be alpha
			 */
			if (!isalpha((int) *argp)) 
				atabort(BADDATE);

			nomatch=1;
                	for (i=MINUTE;(nomatch && i < YEARS+1);i++) {

                        	msg=(unsigned char *)catgets(nltime_fd,NL_SETN,i,
				     INCREMENT[i-1]);
                        	if (msg==NULL)
                                	atabort(BADCATFILE);
                        	if ((np=strmatch(argp,msg)) != argp)
                                	nomatch=0;
			}
			if (!nomatch) {
				switch (i-1) {
					case MINUTE:
					case MINUTES:
						rt.tm_min += 1;
						break;
					case HOUR:
					case HOURS:
						rt.tm_hour += 1;
						break;
					case DAY:		
					case DAYS:		
						rt.tm_mday += 1;
						break;
					case WEEK:
					case WEEKS:
						rt.tm_mday += 7;
						break;
					case MONTH:
					case MONTHS:
						rt.tm_mon += 1;
						break;
					case YEAR:
					case YEARS:	
						rt.tm_year += 1;
						break;
					default:
						atabort(BADDATE);
				}
				argp=np;
			} else
				atabort(BADDATE);
		} else 
			atabort(BADDATE);
	}
}

nodate()
{
	at.tm_mday=tp->tm_mday;
	at.tm_mon=tp->tm_mon;
	at.tm_year=tp->tm_year;
	at.tm_wday=tp->tm_wday;
	at.tm_yday=tp->tm_yday;
	at.tm_isdst=tp->tm_isdst;
	if ((at.tm_hour < tp->tm_hour)
		|| ((at.tm_hour == tp->tm_hour) && (at.tm_min < tp->tm_min)))
		rt.tm_mday++;
}

unsigned char *
strmatch(s1,s2)
unsigned char *s1;
unsigned char *s2;
{
	unsigned char tmpbuf[80],*tptr,*sp;
	int c;

	tptr=tmpbuf;
	sp=s1;
	while (*s1 == ' ') s1++;
	if (isalpha((int)*s1)) {
		while (isalpha((int)*s1)) {
			*tptr++=*s1++;
		}
		*tptr='\0';
		lower_str(tmpbuf);
	} else if (FIRSTof2((int)*s1)) {
		while (FIRSTof2((int) *s1)) {
			c=CHARADV(s1);
			PCHARADV(c,tptr);
		}
		*tptr='\0';
	}
	while (*s2 == ' ') s2++;
	if (isalpha((int)*s2))
		lower_str(s2);
	/*
	 * the "date" string given by the user could be "runtogether".  
	 * Thus string s2 could be a substring of s1. Compare only the
	 * number of characters in the s2.
	 */
	if (strncmp(tmpbuf,s2,strlen(s2)) == 0) 
	{
		/*
		 *  Is it necessary to "backup" s1 because the user
		 *  didn't delimit each part of the string?
		 */
		if (strlen(tmpbuf) > strlen(s2))
		    s1 -= (strlen(tmpbuf)-strlen(s2));
		while (*s1 == ' ')
			s1++;
		return(s1);
	} else return(sp);
}
