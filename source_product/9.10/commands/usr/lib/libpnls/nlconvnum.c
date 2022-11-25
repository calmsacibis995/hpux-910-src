


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

nlconvnum(langid, instr, leninstr, outstr, pLenoutstr, err, numspec, 
					fmtmask, pDecimals)
unsigned short	err[];
short		langid, leninstr, *pLenoutstr, fmtmask, *pDecimals;
char		*instr, *outstr, *numspec;
{
	char	decSep,			/* decimal separator */
		thouSep,		/* thousands separator */
		minusSign,		/* minus sign */
		digitRange[10],		/* range of digits 0-9 */
		*pNumspec,		/* pointer to numspec */
		*iWrk, *oWrk;		/* temp work fields */

	int	stripthou, 		/* strip thousands seperator? */
		stripdec, 		/* strip decimal seperator? */
		numbersonly;		/* only numbers? */

	extern struct	l_info *getl_info();
	struct	l_info *n;

	err[0] = err[1] = 0; 

/*
 * check for invalid length of strings
 */
	if (leninstr <= 0  ||  *pLenoutstr <= 0){
		err[0] = E_INVALIDLENGTH;
		return;
	}


/*
 * check for invalid fmtmask
 */
	if (fmtmask > (M_STRIPTHOU | M_STRIPDEC | M_NUMBERSONLY)){
		err[0] = E_INVALIDFMTMASK;
		return;
	}


/*
 * parse the directions in fmtmask
 */
	stripthou = (fmtmask & M_STRIPTHOU) ? TRUE : FALSE;
	stripdec = (fmtmask & M_STRIPDEC) ? TRUE : FALSE;
	numbersonly = (fmtmask & M_NUMBERSONLY) ? TRUE : FALSE;


/*
 * if numspec was not given, get it
 */
	if ( (pNumspec = numspec) == NULL){
		if (langid == -1) {
		    err[0] = E_LNOTCONFIG;
		    return;
	        }
		if ((n = getl_info(langid)) == NULL ) {
		    err[0] = E_LNOTCONFIG;
		    return;
	        }
		pNumspec = n->numspec;
	}
	

/*
 * get numeric definitions
 */
	get_numeric_defs(pNumspec, &decSep, &thouSep, 
			 &minusSign, digitRange, err);
	if (err[0])
		return;


/*
 * flush the input to the left
 */
	for (iWrk = instr ; *iWrk == ' ' ; iWrk++)
		;


/*
 * copy instr to outstr
 * skip decimal point and thousands separators depending on
 * fmtmask value
 *
 */
	for (oWrk = outstr ; 
	     oWrk != outstr + *pLenoutstr && iWrk != instr + leninstr ;
             iWrk++, oWrk++)
		if (((stripthou) && (*iWrk == thouSep)) || 
		    ((stripdec ) && (*iWrk == decSep)))
	            oWrk --;	
		else *oWrk = *iWrk ;


/*
 *	check for truncation.
 */
	for (; iWrk != instr + leninstr && *iWrk == ' '; iWrk++)
		;
	if (iWrk != instr + leninstr){
		err[0] = E_TRUNCATION;
		return;
	}


/*
 * fill the output buffer with blanks.
 */
	for ( ; oWrk != outstr + *pLenoutstr; oWrk++)
		*oWrk = ' ' ;



/*
 * if there are decimal or thousands separators in the field,
 *	check that they are properly placed.
 */
	check_decthou(outstr, outstr + *pLenoutstr, err, 
		      thouSep, decSep, pDecimals);
	if (err[0])  {
		err [0] = E_INVALIDNUMBER;
		return;
        }
			

	for (oWrk = outstr + *pLenoutstr - 1; *(oWrk - 1) == ' '; oWrk--)
		;


/*
 * if numbersonly, check that the field is ok.
 */
	if (numbersonly){
		if (!isnum_str(outstr, oWrk, thouSep, decSep)){
			err[0] = E_INVALIDNUMBER;
			return;
		}
		*pDecimals = 0;
	}
			


	sub_in_outstr(outstr, oWrk, &decSep, &thouSep, &minusSign, digitRange, 
				stripthou, stripdec);



/*
 * get the length of the outputted string
 */
	for (oWrk = outstr + *pLenoutstr ;
				*(oWrk - 1) == ' ' && oWrk > outstr ; 
				oWrk--)
		;
	*pLenoutstr = (short)((oWrk > outstr) ? (oWrk - outstr) : 0) ;


}


sub_in_outstr(outstr, eOutstr, pDecSep, pThouSep, pMinusSign,
	pDigitRange, strip_thousands, strip_decimals)
char	*outstr, 		/* output string */
	*eOutstr,		/* end of string */
	*pDecSep, 		/* decimal separator */
	*pThouSep, 		/* thousands separator */
	*pMinusSign, 		/* minus sign */
	*pDigitRange;		/* digit range */
int	strip_thousands,	/* strip the thousands out? */
	strip_decimals;		/* strip the decimals out? */
{
	char	*wrk;

	for (wrk = outstr ; wrk < eOutstr ; wrk++){
		/*
		 * check for thousands or decimal separators,
		 *	or minus sign.
		 */
		if (*wrk == *pThouSep)   *wrk = ','; 
	        else if (*wrk==*pDecSep) *wrk = '.'; 
	             else if (*wrk==*pMinusSign) *wrk = '-'; 
	                  else  if ((*wrk >= *pDigitRange) &&  
				      (*wrk <= *(pDigitRange + 9)))
			           *wrk = '0' + *wrk - *pDigitRange ;
	}


	for (--wrk; wrk > outstr; wrk--)
		if (*wrk == '\0')
			*wrk = ' ';
}

