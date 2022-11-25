


/*
    nlutils - Miscellaneous utilities used by routines.
*/

#include <stdio.h>
#include <ctype.h>
#include <fcntl.h>
#include <string.h>
#include <time.h>
#include <langinfo.h>
#include <nlinfo.h>

#ifndef TRUE
# define TRUE 1
# define FALSE 0
#endif

/*
 * get definitions for numerics
 */
get_numeric_defs(pNumspec, pDecSep, pThouSep, pMinusSign, pDigitRange, err)
char	*pNumspec,
	*pDecSep,
	*pThouSep,
	*pMinusSign,
	*pDigitRange;
short	err[];
{
	short	altDigits;		/* boolean */
	int	i;

	memcpy((char *)&altDigits, pNumspec + NUMSP_ALTDIGITS, sizeof(short));

	if(altDigits == 1){
		*pDecSep = *(pNumspec + NUMSP_ALTDEC);
		*pThouSep = *(pNumspec + NUMSP_ALTTHOU);
		*pDigitRange = *(pNumspec + NUMSP_09RANGE);
		for (i = 0; i < 9; i++, pDigitRange++)
			*(pDigitRange + 1) = *pDigitRange + 1;
		*pMinusSign = *(pNumspec + NUMSP_ALTMINUS);
	}
	else if (altDigits == 0){
		*pDecSep = *(pNumspec + NUMSP_DECSEP);
		*pThouSep = *(pNumspec + NUMSP_THOUSEP);
		*pDigitRange = '0';
		for (i = 0; i < 9; i++, pDigitRange++)
			*(pDigitRange + 1) = *pDigitRange + 1;
		*pMinusSign = '-';
	}
	else  {
		err[0] = E_INVALIDNUMSPEC;
        }
}




/*
 *	right adjust
 */
rjust(s, lenstr, lens)
char	*s;
int	lenstr, lens;
{
	char	*f, *t;

	for (f = s + lenstr - 1, t = s + lens - 1; f >= s; t--, f--)
		*t = *f;
	for ( ; t >= s ; t--)
		*t = ' ';
}


/*
 *	fills a buffer with blanks
 */
fillbuff(s, lenstr, lens)
char	*s;
int	lenstr, lens;
{
	char	*wrk;

	for (wrk = s + lenstr; wrk < s + lens; wrk++)
		*wrk = ' ';
}



/*
 * check that the thousands separators are valid.
 */
check_decthou(s, sEnd, err, thouSep, decSep, pDecimals)
char		*s, *sEnd, thouSep, decSep;
unsigned short	err[];
short		*pDecimals;
{
	char	*sStart, *lastThou;
	int	found_dec;		/* true if a decimal separator 
					   was found */

	found_dec = 0;
	*pDecimals = 0;
	err [0] = err [1] = 0;


	/*
	 * adjust s to point to first number
	 */
	for ( ; *s == '$'  ||  *s == '-' ||  *s == '+'; s++)
		;

	/*
	 * adjust sEnd to point to 1 beyond true end of number
	 */
	while (sEnd > s + 1  &&  *(sEnd - 1) == ' ')
		sEnd--;




	for (sStart = s, lastThou = NULL ; s < sEnd ; s++){
		
		if (*s == thouSep  ||  *s == decSep){
			if (found_dec){
				err[0] = E_INVALIDNUMBER + 1;
				break;
			}
			/*
			 * if this is the first thousands separator 
			 *	(lastThou == NULL) and the last one was
			 *	not 3 positions away,
			 *	or,
			 *	if this is the first separator and the first
			 *	one was not 4 positions away.
			 */
			if ( (lastThou &&  (s - lastThou) != 4) ||
				      (lastThou == NULL  &&  *s == thouSep  &&  (s - sStart) > 3)){
				err[0] = E_INVALIDNUMBER + 2;
				break;
			}
			if (*s == decSep){
				found_dec = 1;
			}
			else
				lastThou = s;
		}
		else if (found_dec &&  *s != ' ')
			*pDecimals += 1;
	}
}




/*
 * check that the field contains only valid numeric data
 */
isnum_str(s, sEnd, thouSep, decSep)
char	*s, *sEnd, thouSep, decSep;
{
	for ( ; s != sEnd; s++){
		switch (*s){
			case '$':
				break;
			default:
				if (*s != thouSep  &&  *s != decSep  &&
						(*s < '0'  ||  *s > '9'))
					return(0);
				break;
		}
	}
	return(1);
}

/*
 *	returns TRUE if alt digits were configured for languge
 *		FALSE if not
 */
isaltdig(alt_digits)
char	*alt_digits;
{
	short sh_val;
	struct l_info *n, *getl_info();

	if (alt_digits == NULL){
		n = getl_info((short)-1);
		memcpy((char *)&sh_val, n->alt_digits, sizeof(short));
	}
	else
		memcpy((char *)&sh_val, alt_digits, sizeof(short));

	return((int)sh_val);
}


/*
 *	converts alt digits to ascii and vice versa
 */
convalt(t, s, sEnd, whichway, n)
char	*t, *s, *sEnd;
int	whichway;
struct l_info *n;
{
	char	altDigitRange[10], *wrk;
	int	i;
	unsigned short	err[2];
	struct l_info *getl_info();


	if (n == NULL)
		n = getl_info((short)-1);

	if (n == NULL) {
		err [0] = 2;
		return;
        }

	/*
	 *	set up a table of the alternate digit range
	 */
	wrk = altDigitRange;
	*wrk = *(n->alt_digits + 2);
	for (i = 0; i < 9; i++, wrk++)
		*(wrk + 1) = *wrk + 1;


	if (whichway == ALTTOASCII){
		for ( ; s < sEnd; t++, s++)
			if (*s >= *altDigitRange &&  *s <= *(altDigitRange + 9))
				*t = '0' + *s - *altDigitRange ;
			else *t = *s;
	}
	else
		for ( ; s < sEnd; t++, s++){
			if (*s == ' ')
				*t = *(n->lang_dir + 2);
			else if (*s >= '0' &&  *s <= '9')
				*t = *altDigitRange + *s - '0';
			     else *t = *s;
		}
}

