/* @(#) $Revision: 70.1 $ */    

#include <sys/types.h>
#include <ctype.h>
#include <nl_types.h>
#include <stdio.h>

#define	NL_SETN		2

extern nl_catd nlmsg_fd;

/*****************/
time_t num(ptr)
/*****************/
char **ptr;
{
	time_t n=0;
	while (isdigit(**ptr)) {
		n = n*10 + (**ptr - '0');
		*ptr += 1; }
	return(n);
}


int dom[12]={31,28,31,30,31,30,31,31,30,31,30,31};

/*****************/
days_btwn(m1,d1,y1,m2,d2,y2)
/*****************/
int m1,d1,y1,m2,d2,y2;
{
	/* calculate the number of "full" days in between m1/d1/y1 and m2/d2/y2.
	   NOTE: there should not be more than a year separation in the dates.
		 also, m should be in 0 to 11, and d should be in 1 to 31.    */
	
	int days,m;

	if ((m1==m2) && (d1==d2) && (y1==y2)) return(0);
	if ((m1==m2) && (d1<d2)) return(d2-d1-1);
	/* the remaining dates are on different months */
	days = (days_in_mon(m1,y1)-d1) + (d2-1);
	m = (m1 + 1) % 12;
	while (m != m2) {
		if (m==0) y1++;
		days += days_in_mon(m,y1);
		m = (m + 1) % 12; }
	return(days);
}


/*****************/
days_in_mon(m,y)
/*****************/
int m,y;
{
	/* returns the number of days in month m of year y
	   NOTE: m should be in the range 0 to 11	*/

	return( dom[m] + (((m==1)&&((y%4)==0)) ? 1:0 ));
}

/*****************/
char *xmalloc(size)
/*****************/
unsigned int size;
{
	char *p;
	char *malloc();

	if((p=malloc(size)) == NULL) {
		printf((catgets(nlmsg_fd,NL_SETN,1, "cannot allocate %d byte of space\n")),size);
		fflush(stdout);
		exit(55);
	}
	return p;
}
