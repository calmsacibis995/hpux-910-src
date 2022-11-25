#include	<nl_ctype.h>
#include	<nl_types.h>
#include	<time.h>
#include	<string.h>
#include	<langinfo.h>
#include	<sys/types.h> 

#define		NULL	0
#define		COMMA	','
#define		PERIOD  '.'
#define		RAPPOS  39
#define		LAPPOS  96
#define		HYPHEN  '-'
#define		SLASH	'/'
#define		DOT	".\0"
#define		YEAR	1
#define		MONTH	2
#define		DAY	3
#define		WEEKDAY	4
#define	PDBADARG	-5
#define	PDNONLS		-6
#define	PDNODFMT	-7
#define	PDINVAL		-8
#define	PDAMBIG		-9

typedef struct {
	int	type,num;
} d_item;

typedef struct {
	int	year,mon,mday,wday;
} d_rec;

nl_item abmon[] = {	/* map to abbreviated month strings: Jan, etc */
    ABMON_1, ABMON_2, ABMON_3, ABMON_4, ABMON_5, ABMON_6, ABMON_7,
    ABMON_8, ABMON_9, ABMON_10, ABMON_11, ABMON_12
};

nl_item mon[] = {	/* map to month strings: January, etc */
    MON_1, MON_2, MON_3, MON_4, MON_5, MON_6, MON_7,
    MON_8, MON_9, MON_10, MON_11, MON_12
};

nl_item abday[] = {	/* map to weekday strings: Friday, etc */
    ABDAY_1, ABDAY_2, ABDAY_3, ABDAY_4, ABDAY_5, ABDAY_6, ABDAY_7
};

nl_item day[] = {	/* map to weekday strings: Friday, etc */
    DAY_1, DAY_2, DAY_3, DAY_4, DAY_5, DAY_6, DAY_7
};

char dmark[] = {	/* Puncutation marks */
    COMMA, HYPHEN, PERIOD, SLASH, RAPPOS, LAPPOS
};

int	MDAY[12] =
{
	31,28,31,
	30,31,30,
	31,31,30,
	31,30,31,
};

static int ip,mode,arg_c;

unsigned char *
parsedate(datestr, drec, status, m)
unsigned char *datestr;
struct tm *drec;
int	*status;
int	m;
{
	d_item *f_tok,*s_tok,*t_tok;
	d_rec	date;
	int	ymd,md,st,valid_date();
	unsigned char *next,*get_token();
	char *fmt;
	
/* Initialization */
	mode=m;
	*status=PDBADARG;
	st=0;
	arg_c=0;
	ymd=md=0;
	date.year=date.mon=date.mday=date.wday=0;

	fmt=(char *)nl_langinfo(D_T_FMT);
	ip=scan_dfmt(fmt);

/* Get First Token */
	f_tok=(d_item *)malloc(sizeof(d_item));
	f_tok->type=f_tok->num=0;
	while (*datestr == ' ')
		datestr++;
	if (!isdigit((int)*datestr)) {
		/* parse weekday name */
		if ((next=get_token(datestr,f_tok,1)) != NULL) {
			date.wday=f_tok->num;
			valid_date(&date,1);
			drec->tm_year=date.year;
			drec->tm_mon=date.mon;
			drec->tm_mday=date.mday;
			drec->tm_wday=date.wday;
			*status=0;
			return(next);
		} 
	}
	if ((next=get_token(datestr,f_tok,0)) == NULL)
		return(datestr);
	arg_c++; 
	if (mode == 0) {
		switch (f_tok->type) {
		case 1:
			ymd=1;
			date.year=f_tok->num;
			break;
		case 2:
			ymd=2;
			date.mon=f_tok->num-1;
			break;
		case 3:
			ymd=3;
			date.mday=f_tok->num;
			break;
		}
	}
	datestr=next;
	if (*datestr == '\0') return(datestr);

/* Get Second Token */
	s_tok=(d_item *)malloc(sizeof(d_item));
	s_tok->type=s_tok->num=0;
	if ((next=get_token(datestr,s_tok,0)) == NULL) 
		return(datestr);
	arg_c++;
	if (mode == 0) {
		md=s_tok->type;
		if ((ymd==md) && ymd) return(datestr); 
		switch (ymd) {
		case	0:
				switch (md) {
				case 1:
					date.year=s_tok->num;
					break;
				case 2:
					date.mon=s_tok->num-1;
					date.mday=f_tok->num;
					break;
				case 3:
					date.mday=s_tok->num;
					date.mon=f_tok->num-1;
					break;
				}
				break;
		case	1:
				if (md == 2)
					date.mon=s_tok->num-1;
				else if (md == 3) 
					date.mday=s_tok->num;
				break;
		case	2:
				if (md == 1) 
					date.year=s_tok->num;
				else if (md == 3 || md == 0) 
					date.mday=s_tok->num;
				break;
		case	3:
				if (md == 1) 
					date.year=s_tok->num;
				else if (md == 2 || md == 0)
					date.mon=s_tok->num-1;
				break;
		}
	}
	if (*next== '\0' || *next == '+') {  /* Only two arguments for the date */
		if (mode == 1) 
		    st=update(&date,f_tok->num,s_tok->num,0,0);
		else if (ymd==0  && md==0) { 
			if (f_tok->num > 12 && s_tok->num <= 12) {
				date.mday=f_tok->num;
				date.mon=s_tok->num-1;
			} else 
			if (f_tok->num <= 12 && s_tok->num > 12) {
				date.mday=s_tok->num;
				date.mon=f_tok->num-1;
			} else st=update(&date,f_tok->num,s_tok->num,0,1);
	            }   	
		else {
			if (ymd == 1 || md == 1) return(datestr); 
			switch (ymd) {
			case 	0:
					if (md == 2)
						date.mday=f_tok->num;
					else  if (md==3) 
						date.mon=f_tok->num-1;
					break;
			case	2:
					if (md == 0)
						date.mday=s_tok->num;
					break;
			case	3:	
					if (md == 0)
						date.mon=s_tok->num-1;
					break;
			default:
					return(datestr);
			}
		}
		*status=valid_date(&date,0);
		if (*status == 0) {
			drec->tm_year=date.year;
			drec->tm_mon=date.mon;
			drec->tm_mday=date.mday;
			return(next);
		} else return(datestr);
		
	} else {

/* Get Third Token */
		datestr=next;
		t_tok=(d_item *)malloc(sizeof(d_item));
		t_tok->type=t_tok->num=0;
		if ((next=get_token(datestr,t_tok,0)) == NULL) 
			return(datestr);
		arg_c++;
		if (mode == 1)
			 st=update(&date,f_tok->num,s_tok->num,t_tok->num,0);
		else {
		switch (ymd) {
		case 0:
			switch (md) {
			case 0:	
				st=update(&date,f_tok->num,s_tok->num,
					t_tok->num,t_tok->type);
				break;
			case 1:	
				switch (t_tok->type) {
				case 0:           
					st=update(&date,f_tok->num,
					   t_tok->num,s_tok->num,1);
					break;
				case 2:
					date.mon=t_tok->num-1;
					date.mday=f_tok->num;
					break;
				case 3:
					date.mday=t_tok->num;
					date.mon=f_tok->num-1;
					break;
				default:
					return(datestr);
				}
				break;
			case 2:
				switch(t_tok->type) {
				case 0:
					st=update(&date,f_tok->num,
					   t_tok->num,0,2);
					break;
				case 1:
					date.year=t_tok->num;
					date.mday=f_tok->num;
					break;
				case 3:
					date.mday=t_tok->num;
					date.year=f_tok->num;
					break;
				default:
					return(datestr);
				}
				break;
			case 3:
				switch(t_tok->type) {
				case 0:
					st=update(&date,f_tok->num,
					   t_tok->num,s_tok->num,3);
					break;
				case 1: 
					date.year=t_tok->num;
					date.mon=f_tok->num-1;
					break;
				case 2:
					date.mon=t_tok->num-1;
					date.year=f_tok->num;
					break;
				default:
					return(datestr);
				}
				break;
			}
			break;
		case 1:
			switch (md) {
			case 0:	
				switch (t_tok->type) {
				case 0:
					st=update(&date,s_tok->num,t_tok->num,
					0,1);
					break;
				case 2:
					date.mon=t_tok->num-1;
					date.mday=s_tok->num;
					break;
				case 3:
					date.mday=t_tok->num;
					date.mon=s_tok->num-1;
					break;
				default:
					return(datestr);
				}
				break;
			case 2:
				switch(t_tok->type) {
				case 0:
				case 3:
					date.mday=t_tok->num;
					break;
				default:
					return(datestr);
				}
				break;
			case 3:
				switch (t_tok->type) {
				case 0:
				case 2:
					date.mon=t_tok->num-1;
					break;
				default:
					return(datestr);
				}
				break;
			default:
				return(datestr);
			}
			break;
		case 2:
			switch (md) {
			case 0:	
				switch (t_tok->type) {
				case 0:
					st=update(&date,s_tok->num,t_tok->num,
					0,2);
					break;
				case 1:
					date.year=t_tok->num;
					date.mday=s_tok->num;
					break;
				case 3:
					date.mday=t_tok->num;
					date.year=s_tok->num;
					break;
				default:
					return(datestr);
				}
				break;
			case 1:
				switch(t_tok->type) {
				case 0:
				case 3:
					date.mday=t_tok->num;
					break;
				default:
					return(datestr);
				}
				break;
			case 3:
				switch (t_tok->type) {
				case 0:
				case 1:
					date.year=t_tok->num;
					break;
				default:
					return(datestr);
				}
				break;
			default:
				return(datestr);
			}
			break;
		case 3:
			switch (md) {
			case 0:	
				switch (t_tok->type) {
				case 0:
					st=update(&date,s_tok->num,t_tok->num,
					0,3);
					break;
				case 1:
					date.year=t_tok->num;
					date.mon=s_tok->num-1;
					break;
				case 2:
					date.mon=t_tok->num-1;
					date.year=s_tok->num;
					break;
				default:
					return(datestr);
				}
				break;
			case 1:
				switch(t_tok->type) {
				case 0:
				case 2:
					date.mon=t_tok->num-1;
					break;
				default:
					return(datestr);
				}
				break;
			case 2:
				switch (t_tok->type) {
				case 0:
				case 1:
					date.year=t_tok->num;
					break;
				default:
					return(datestr);
				}
				break;
			default:
				return(datestr);
			}
			break;
		default:
			return(datestr);
		}
		} /*else*/
	}

	if (st < 0) {
		*status=st;
		return(datestr);
	}
	*status=valid_date(&date,0);
	if (*status == 0) {
		drec->tm_year=date.year;
		drec->tm_mon=date.mon;
		drec->tm_mday=date.mday;
		return(next);
	} else return(datestr);
	
} /* parsedate */
     
int
valid_date(d,tag)
d_rec 	*d;
int	tag;
{
	time_t	now;
	struct tm	*tp,*localtime();
	int 	rt;

	time(&now);
	tp=localtime(&now);
	if (d->year == 0 ) {
		d->year=tp->tm_year;
		if (d->mon < tp->tm_mon)
			d->year++;
	}
	MDAY[1] = 28 + leap_year(d->year);

/* date = weekday */
	if (tag == 1) {
		d->year=tp->tm_year;
		d->mon=tp->tm_mon;
		d->mday=tp->tm_mday;
		if (d->wday < 7) {
			rt=d->wday - tp->tm_wday;
			if (rt < 0 )
				d->mday = d->mday + rt + 7;
			else d->mday += rt;
		}
	}
	if (d->mon >=12 || d->mday > MDAY[d->mon])
		return(PDINVAL);
	if (d->mon < 0 || d->mday <= 0)
		return(PDINVAL);
	if (d->year >= 100 )
		d->year -= 1900;
	if (d->year < 70 || d->year >= 100)
		return(PDINVAL);
	return(0);
}
	
leap_year(year)
{
	return (year % 4 == 0);
}

		
unsigned char *
get_token(iptr,sptr,tag)
unsigned char *iptr;
d_item *sptr;
int	tag;
{
	int	ch,num,n,c;
	unsigned char	alpha[80],*aptr,*tptr;
	unsigned char 	*parse_general();

	while (*iptr == ' ')
		iptr++;
	ch=(int) *iptr;
	if (isdigit(ch)) {
		num=0;
		while (isdigit(n=(int) *iptr))  {
			n-=48;
			num=num*10 + n;
			iptr++;
		}
		/*if (num > 12 && num <= 31) sptr->type=DAY;
		else  */
		if (num > 31) sptr->type =YEAR;
		sptr->num = num;
	} else   
	if (isalpha(ch) || FIRSTof2(ch)) {
		aptr=alpha;
		if (isalpha(ch))  {
			while (isalpha((int) *iptr))
				*aptr++=*iptr++;
		} else 
			while (FIRSTof2((int) *iptr)) {
				c=CHARADV(iptr);
				PCHARADV(c,aptr);
			}

		*aptr='\0';
		sptr->num=ck_lang_tab(alpha,tag);
		if (sptr->num < 0) 
			return(NULL);
		if (tag == 1) {
			sptr->type=WEEKDAY;
			while (*iptr == ' ')
				iptr++;
			return(iptr);
		} else 
			sptr->type=MONTH;
	} else
	if (*iptr == '\0') 
		return(iptr);
	else 
		return(NULL);
	tptr=parse_general(iptr,sptr);
	return(tptr);
}

int
ck_lang_tab(mptr,tag)
unsigned char *mptr;
int	tag;
{
	int	nomatch,i,p,m;
	unsigned char	*ptr;
	
	if (isalpha((int)*mptr)) 
		lower_str(mptr);
	nomatch=1;
/* match weekday name */
	if (tag == 1) {
		for (i=0; ((i < 7) && (nomatch));i++) {
			ptr=(unsigned char *)nl_langinfo(abday[i]);
			if (!FIRSTof2(*ptr)) 
				lower_str(ptr);
			if (strlen(ptr) == strlen(mptr)) 
				nomatch=strcmp(mptr,ptr); /*if 0, matched*/
		} 
		for (i=0; ((i < 7) && (nomatch));i++) {
			ptr=(unsigned char *)nl_langinfo(day[i]);
			if (!FIRSTof2(*ptr)) 
				lower_str(ptr);
			if (strlen(ptr) == strlen(mptr)) 
				nomatch=strcmp(mptr,ptr); /*if 0, matched*/
		} 
		if (nomatch)
			return(-1);
		else return (i-1);
	}

/* match abbreviated month name */
	for (i=0; ((i < 12) && (nomatch));i++) {
		ptr=(unsigned char *)nl_langinfo(abmon[i]);
		if (!FIRSTof2(*ptr)) 
			lower_str(ptr);
		if (strlen(ptr) == strlen(mptr)) 
			nomatch=strcmp(mptr,ptr); /*if 0, matched*/
	} 

/* match fully spelled out month name */
	if (nomatch) {
		for (i=0;((i < 12) && nomatch);i++) {
	 		ptr=(unsigned char *)nl_langinfo(mon[i]);
			if (!FIRSTof2(*ptr)) lower_str(ptr);
			if (strlen(ptr) == strlen(mptr)) 
				nomatch=strcmp(mptr,ptr); /*if 0, matched*/
		}
	}

/* match abbreviated month name appended with a dot */
	if (nomatch) {
		for (i=0; ((i < 12) && (nomatch));i++) {
			ptr=(unsigned char *)nl_langinfo(abmon[i]);
			if (!FIRSTof2(*ptr)) lower_str(ptr);
			if ((p=strlen(ptr)) == (m=strlen(mptr))) 
				nomatch=strcmp(mptr,ptr); /*if 0, matched*/
			else 
			if ((m+1) == p)
				if ((*(ptr+p-1)) == '.') {
					strcat(mptr,DOT);
					nomatch=strcmp(mptr,ptr);
		        	}
		} 
		if (nomatch) return(-1); 
	}
	return(i);
}

lower_str(ptr)
char *ptr;
{
	char * cptr;
	int	c;

	cptr=ptr;
	while (*ptr != '\0') {
		c=tolower((int) *ptr++);
		*cptr++=(char) c;
	}
}

unsigned char *
parse_general(iptr,sptr)
unsigned char *iptr;
d_item *sptr; 
{
	unsigned char  	alpha[80],*aptr,*tmp,*ptr;
	int c;

	if (*iptr == '\0') return(iptr);
	while (*iptr == ' ') 
		iptr++;
	if (FIRSTof2((int) *iptr)) {
		aptr=alpha;
		tmp=iptr;
		while (FIRSTof2((int) *iptr)) {
			c=CHARADV(iptr);
			PCHARADV(c,aptr);
		}
		*aptr='\0';
		aptr=alpha;
		ptr=(unsigned char *)nl_langinfo(YEAR_UNIT);
		if (nl_strcmp(aptr,ptr) == 0) sptr->type=YEAR;
		else { 
			ptr=(unsigned char *)nl_langinfo(MON_UNIT);
		        if (nl_strcmp(aptr,ptr) == 0) sptr->type=MONTH;
		        else { 
				ptr=(unsigned char *)nl_langinfo(DAY_UNIT);
			      	if (nl_strcmp(aptr,ptr) == 0) sptr->type=DAY;
			        else return(tmp);
 			     }
		}
	}
	if (strchr(dmark,(int)*iptr) != NULL)
		iptr++;
	while (*iptr == ' ')
		iptr++;
	return(iptr);
}

int
update(d,a1,a2,a3,tag)
d_rec *d;
int a1;
int a2;
int a3;
int tag;
{
	if (a1 == 0 || a2 == 0 )
		return(PDBADARG);
	if (!ip) {
		if (mode == 1) 
			return(PDNODFMT);
		if (tag==1 ) { 
			d->year=a3;
			if (((a1 > 12 || a1 == a2)) && a2 <= 12) {
				d->mday=a1;
				d->mon=a2-1;
			}
			else if (a2 > 12 &&  a1 <= 12) {
				d->mday=a2;
				d->mon=a1-1;
			} else return(PDAMBIG);
			return(0);
		} else return(PDAMBIG);
	}

	switch (tag)  {
	case 0:
		if (mode == 1) {
			if ((ip >= 1 && ip <= 6 && arg_c != 3) ||
				((ip==8 || ip==7) && arg_c !=2))
				return(PDBADARG); 
		}
		switch (ip) {
		case 1:	d->year=a1;
			d->mon=a2-1;
			d->mday=a3;
			break;
		case 2: d->year=a1;
			d->mday=a2;
			d->mon=a3-1;
			break;
		case 3: d->mon=a1-1;
			d->mday=a2;
			d->year=a3;
			break;
		case 4: d->mday=a1;
			d->mon=a2-1;
			d->year=a3;
			break;
		case 5: d->mon=a1-1;
			d->year=a2;
			d->mday=a3;
			break;
		case 6: d->mday=a1;
			d->year=a2;
			d->mon=a3-1;
			return(PDAMBIG);
		}
		break;
	case 1:
		if (a3 !=0 ) 
			d->year=a3;
		if  (a1 <= 12 && a2 >12 ) {
			d->mon=a1-1;
			d->mday=a2;
		} else if (a1 > 12 && a2 <= 12) {
			d->mon=a2-1;
			d->mday=a1;
		} else
		if (ip==1 || ip==3 || ip==5 || ip==7) {
			d->mon=a1-1;
			d->mday=a2;
		} else {
			d->mon=a2-1;
			d->mday=a1;
		}
		break;
	case 2:
		if (a3 != 0) d->mon=a3-1;
		if (ip==1 || ip==2 || ip==5) {
			d->year=a1;
			d->mday=a2;
		} else if (ip==3 || ip==4 || ip==6) {
			d->year=a2;
			d->mday=a1;
		}
		break;
	case 3:
		if (a3 != 0 )
			 d->mday=a3;
		if (ip==1 || ip==2 || ip==6) {
			d->year=a1;
			d->mon=a2-1;
		} else if (ip==3 || ip==4 || ip==5) {
			d->mon=a1-1;
			d->year=a2;
		}
		break;
	} /* switch */
	return(0);
}

/*
 *
 * This routine looks at the D_T_FMT string provided in the D.<language>
 * file.  This string contains a description of how to represent the current
 * date & time in that language.  We parse the string to find out what
 * order this language likes to say month, day, and year in.  Is it
 * "Jan 1, 1990" or "1990 Jan 1"?  We use this to disambiguate input
 * strings like "1/2", which could mean Jan 2nd, or Feb 1st.
 */

int
scan_dfmt(fmt)
char *fmt;
{

	char ch;
	int	path;

	path=0;
	for (;;) {
		/* Get the next % or end-of-string */
		while ((ch=*fmt++) && ch!='%')
			;

		if (ch=='\0')		/* normal end-of-string */
			break;

		/* We have a %.  What's after it? */
		while ((ch=*fmt++) && ch!='%' && !isalpha((int) ch))
			;

		if (ch=='\0')		/* Strings ends with %--error! */
			break;		/* but what else can we do? */

		/*
		 * See the man page for date(1) for a description
		 * of these format letters.
		 */
		switch (ch) {
			/* month */
			case 'b': case 'B': case 'm':
			case 'h': case 'F':
				path=path*10 + 2;	break;

			/* day of month */
			case 'd':
				path=path*10 + 3;	break;

			/* year */
			case 'y': case 'Y': case 'E':
				path=path*10 + 1;	break;

			/* Funny combined case %m/%d/%y */
			case 'D': path=231;		break;
		}
	}
	switch (path) {
	case 123:  return 1;
	case 132:  return 2;
	case 231:  return 3;
	case 321:  return 4;
	case 213:  return 5;
	case 312:  return 6;
	case 23:   return 7;
	case 32:   return 8;
	default:   return 0;
	}
}
