/* parsetime.c */
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
#define		COLON   ':'
#define		SLASH	'/'
#define		AMSTR   "am"
#define		PMSTR   "pm"
#define	PTBADARG	-1
#define	PTNONLS		-2
#define	PTINVAL1	-3
#define	PTINVAL2	-4


nl_item ampm[] = {	/* map to abbreviated AM or PM */
	AM_STR, PM_STR
};

unsigned char n_ampm[][3] = {	/* map to abbreviated AM or PM */
	AMSTR, PMSTR
};

nl_item unit[] = {	/* map to XXX_UNIT strings in langinfo */
	HOUR_UNIT, MIN_UNIT, SEC_UNIT
};
char tmark[] = {	/* Puncutation marks */
    COMMA, COLON, PERIOD, RAPPOS, LAPPOS
};

struct time_rec {
	int	hour,min,sec;
};

static int AM=0,PM=0;

unsigned char *
parsetime(timestr, t_rec, status,mode)
unsigned char *timestr;
struct tm *t_rec;
int	*status;
int	mode;
{
	int	d,num,i,j,EOS,EOT,err;
	int	int_arr[2];
	unsigned char	*tp,*ck_UNIT_str(),*ck_ampm_str();
	int  valid_time();
	struct time_rec rec;
	
	EOS=EOT=0;
	j=err=0;
	i=-1;
 	num=0;
	rec.hour=0;
	rec.min=0;
	rec.sec=0;

	if (*timestr == '\0') {
		*status=PTBADARG;
		return(timestr);
	}
	while (*timestr == ' ') timestr++;
	while ((isdigit(d=(int) *timestr)) && (i < (mode ))) {
		d-=48;
		num=num*10+d;
		timestr++;
		j++;
		if (j % 2 == 0 ) {
			int_arr[++i]=num;
			num=0;
		} else if (!isdigit((int)*timestr)) {
				int_arr[++i]=num;
				num=0;
		}
	}
	if (j % 2 == 1 && j != 1 ) {
		*status=PTBADARG;
		return(timestr);
	}
	while (i < (mode-1) && !EOT ) {
		while (*timestr == ' ') timestr++;
		if (isdigit(d=(int)*timestr)) {
			d-=48;
			num=num*10 + d;
			timestr++;
			if (isdigit(d=(int)*timestr)) {
				d-=48;
				num=num*10 + d;
				timestr++;
			}
			int_arr[++i]=num;
			if (i == (mode-1))
				EOT=1;
		} else
			switch (*timestr) {
			case '\0':
				   EOS=EOT=1;
				   break;
			case 'h': 
				   if (i==1)  {
					*status=PTBADARG;
					return(timestr);
			  	   } else timestr++;
				   break;
			default:
				if ((tp=(unsigned char *)strchr(tmark,*timestr)) != 
NULL) {
					timestr++;
			        	while (*timestr == ' ') timestr++;
					if (!isdigit((int) *timestr)) {
						*status=PTBADARG;	
						return(timestr);
				 	} else;
		                } else 
					if ((tp=ck_UNIT_str(timestr,&i)) ==NULL) 
						EOT=1;
					else  {
						if (i < 0 ) {
							*status=PTBADARG;
							return(timestr);
						} else timestr=tp;
					}
	  	} /* switch */
	} /* while */
	while (*timestr == ' ') timestr++;
	if (*timestr == '\0') EOS=1;
	if (!EOS && EOT ) {
		if (FIRSTof2((int) *timestr)) {
			if ((tp=ck_UNIT_str(timestr,&i)) != NULL) 
				if (i < 0) {
					*status=PTBADARG;
					return(timestr);
				} else timestr=tp;
		}
		if ((tp=ck_ampm_str(timestr)) != NULL)
		 	timestr=tp;
	} 

	while (*timestr == ' ') timestr++;

	if (i == 2) 
		rec.sec=int_arr[i--];
	if (i == 1)
		rec.min=int_arr[i--];
	if (i == 0)
		rec.hour=int_arr[i];
	if ((err=valid_time(&rec)) == 0) {
		*status=0;
		t_rec->tm_hour=rec.hour;
		t_rec->tm_min=rec.min;
		t_rec->tm_sec=rec.sec;
		if (EOS) return(NULL);
		else {
			while (*timestr == ' ')
				timestr++;
			return(timestr);
		}
	} else {
		*status=err;
		if (*timestr == '\0')
			return(NULL);
		else return(timestr);
	}
}
     
int
valid_time(d)
struct time_rec *d;
{
	struct tm	*localtime();

	if (PM) {
		if (d->hour < 1 || d->hour > 12)
			return(PTINVAL1);
		d->hour %= 12;
		d->hour += 12;
	}
	if (AM) {
		if (d->hour < 1 || d->hour > 12)
			return(PTINVAL1);
		d->hour %= 12;
	}

	if (d->hour >=24 || d->min >= 60 || d->sec >= 60)
		return(PTINVAL2);
	return(0);
}
	
unsigned char *
ck_ampm_str(mptr)
unsigned char *mptr;
{
	int	nomatch,i,ch,currlangid();
	unsigned char	*ptr,alpha[80],*aptr;
	
	nomatch=1;
	aptr=alpha;
	if (isalpha((int) *mptr))  {
		while (isalpha((int) *mptr))
			*aptr++=*mptr++;
		*aptr='\0';
 		lower_str(alpha);
		if (currlangid() == 0) {
			for (i=0; ((i < 2) && (nomatch));i++) 
				nomatch=strcmp(alpha,n_ampm[i]); /*if 0, matched*/
		} else {
			for (i=0; ((i < 2) && nomatch);i++) {
				ptr=(unsigned char *)nl_langinfo(ampm[i]);
				if (!FIRSTof2((int)*ptr)) lower_str(ptr);
				/*
				 * the "date" string given by the user 
				 * could be "runtogether".  Thus the "am/pm"
				 * string is a substring. Compare only the
				 * number of characters in the NLS token.
				 */
				nomatch=strncmp(alpha,ptr,strlen(ptr)); /*if 0, matched*/
			} 
			/*
			 *  did we find a match?
			 *  If yes is it necessary to "backup" the input string
			 *  pointer because the user didn't delimit each part 
			 *  of the string?
			 */
			if ((!nomatch) && (strlen(alpha) > strlen(ptr)))
			    mptr -= (strlen(alpha)-strlen(ptr));
		}
	} else if (FIRSTof2((int)*mptr)) {
		while (FIRSTof2((int) *mptr) && nomatch) {
			ch=CHARADV(mptr);
			PCHARADV(ch,aptr);
			*aptr='\0';
			for (i=0; ((i < 2) && (nomatch));i++) {
				ptr=(unsigned char *)nl_langinfo(ampm[i]);
				nomatch=strcmp(alpha,ptr); /*if 0, matched*/
			} 
		}
	}
	if (nomatch) return(NULL); 
	else if (i==1) AM=1;
	else PM=1;
	return(mptr);
}

unsigned char *
ck_UNIT_str(mptr,ind)
unsigned char *mptr;
int *ind;
{
	int	nomatch,i,ch;
	unsigned char	alpha[80];
	unsigned char	*ptr,*aptr;
	
	aptr=alpha;
	nomatch=1;
	if (isalpha((int) *mptr)) {
		while (isalpha((int) *mptr))
			*aptr++=*mptr++;
		*aptr='\0';
		lower_str(alpha);
		for (i=0; ((i < 3) && (nomatch));i++) {
			ptr=(unsigned char *)nl_langinfo(unit[i]);
			if (!FIRSTof2((int)*ptr)) lower_str(ptr);
				nomatch=strcmp(alpha,ptr); /*if 0, matched*/
		}
	} else if  (FIRSTof2((int) *mptr)) {
		while  (FIRSTof2((int) *mptr) && nomatch) {
			ch=CHARADV(mptr);
			PCHARADV(ch,aptr);
			*aptr='\0';
			for (i=0; ((i < 3) && (nomatch));i++) {
				ptr=(unsigned char *)nl_langinfo(unit[i]);
				nomatch=strcmp(alpha,ptr); /*if 0, matched*/
			}
		}
	} else return(NULL);
		
	 
	if (nomatch) 
		return(NULL);
	else { /*check semantics of the unit characters */
		if (*ind != (i-1)) 
			*ind=-1;
		while (*mptr == ' ')
			mptr++;
		return(mptr);
	}
  }



