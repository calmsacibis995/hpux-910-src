


#include <stdio.h>
#include <time.h>
#include <nlinfo.h>

#define	dysize(A) (((A)%4==0 && (A)%100!=0 || (A)%400==0)? 366: 365)

static short dmsize[12]={31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
static short dmleap[12]={31, 29, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};

#ifndef TRUE
# define TRUE	1
# define FALSE	0
#endif

/*
 *	get the julian date
 *		the year, month, and day are supplied.
 */
getjulian(xtime)
struct tm	*xtime;
{
	register int m;

	/*
	**	get the month and day of month
	*/
	xtime->tm_yday = xtime->tm_mday - 1;
	if (dysize(xtime->tm_year + 1900) == 366)
		for ( m = 0 ; m < xtime->tm_mon; m++)
			xtime->tm_yday += dmleap[m];
	else
		for ( m = 0 ; m < xtime->tm_mon; m++)
			xtime->tm_yday += dmsize[m];
}


isvalidjulian(xtime)
struct tm	*xtime;
{
	if (xtime->tm_yday < 1)
		return(FALSE);
	else if (dysize(xtime->tm_year + 1900) == 366) {
		if (xtime->tm_yday > 366)
			return(FALSE);
	}
	else if (xtime->tm_yday > 365)
		return(FALSE);

	return(TRUE);
}




isvalidgregorian(xtime)
struct tm	*xtime;
{

	/*
	 * 0 <= month < 12
	 * 0 <= year < 100
	 * day of month > 0
	 */
	if (xtime->tm_mon < 0 || xtime->tm_mon > 11 || 
			xtime->tm_year < 0 || xtime->tm_year > 99 ||
			xtime->tm_mday < 1)
		return(FALSE);

	/*
	 * check that day of month is not beyond limit
	 */
	if (dysize(xtime->tm_year + 1900) == 366) {
		if (dmleap[xtime->tm_mon] < xtime->tm_mday)
			return(FALSE);
	}
	else {
		if (dmsize[xtime->tm_mon] < xtime->tm_mday)
			return(FALSE);
	}

	return(TRUE);
}




/*
 *	fill in a tm structure
 *		the year and julian date are supplied.
 *		this function fills in the rest.
 *		the time is set to zero.
 */
unsigned short nlcrtm(xtime)
struct tm	*xtime;
{
	register int d1, j;
	long day;


	/*
	 * check for valid year
	 */
	if (xtime->tm_year < 0  || xtime->tm_year > 99  ||
			!isvalidjulian(xtime))
		return(E_DAYOFYEAROUTOFRANGE);


	/*
	**	get the month and day of month
	*/
	xtime->tm_mon = xtime->tm_mday = 0; 
	j = xtime->tm_yday;
	if (dysize(xtime->tm_year + 1900) == 366)
		for ( ; j > dmleap[xtime->tm_mon]; xtime->tm_mon++)
			j -= dmleap[xtime->tm_mon];
	else
		for ( ; j > dmsize[xtime->tm_mon]; xtime->tm_mon++)
			j -= dmsize[xtime->tm_mon];
	xtime->tm_mday = j;





	/*
	 *	get the number of days since UN*X start date
	 */
	day = xtime->tm_yday;
	if (xtime->tm_year >= 70)
		for (d1 = 70; d1 < xtime->tm_year; d1++)
			day += dysize(d1 + 1900);
	else
		for (d1 = 70; d1 > xtime->tm_year; d1--)
			day -= dysize(d1 - 1 + 1900);



	/*
	** Day of the week
	*/
	xtime->tm_wday = ((day + 7340035L) % 7);


	/*
	 * initialize tm_isdst to 0
	 */
	xtime->tm_isdst = 0;

	return((unsigned short)0);

}
