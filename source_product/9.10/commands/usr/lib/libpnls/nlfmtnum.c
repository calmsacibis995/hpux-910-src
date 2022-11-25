


#include <stdio.h>
#include <ctype.h>
#include <fcntl.h>
#include <string.h>
#include <time.h>
#include <langinfo.h>
#include <nlinfo.h>
#include <stdlib.h>

#ifndef TRUE
# define TRUE 1
# define FALSE 0
#endif




/*
 *	ascii values for thousands and decimal sperators
 */
#define ASCTHOUSEP ','
#define ASCDECSEP  '.'



nlfmtnum(langid, instr, leninstr, outstr, pLenoutstr, err, numspec, 
					fmtmask, decimals)
short	langid, leninstr, *pLenoutstr, fmtmask, decimals;
unsigned short	err[];
char	*instr, *outstr, *numspec;
{
	char	*buff, 			/* temp buffer */
		decSep,			/* decimal separator */
		thouSep,		/* thousands separator */
		minusSign,		/* minus sign */
		digitRange[10],		/* range of digits 0-9 */
		*pNumspec;		/* pointer to numspec */

	int	insthou, 		/* insert thousands seperator? */
		insdec, 		/* insert decimal seperator? */
		inscurrency, 		/* insert currency symbol */
		left_justify, 		/* left justify? */
		right_justify, 		/* right justify? */
		retlength,		/* return length of formatted number? */
		digits_to_left,		/* numbers to left of decimal */
		positive_sign,		/* does a positive sign appear */
		negative_number;	/* is number negative? */
	short	decimal_positions,	/* decimal positions in the field */
		langdir;		/* TRUE-right to left */

	err[0] = err[1] = (short) 0;

	/*
	 * the cleanup routine checks if buff is not NULL and if 
	 *	pNumspec and numspec are NULL
	 */
	buff = pNumspec = NULL;

	/*
	 * check for invalid length of strings
	 */
	if (leninstr <= 0  ||  *pLenoutstr <= 0){
		err[0] = E_INVALIDLENGTH;
		return;
	}

	/*
	 * check for invalid number of decimals
	 */
	if (decimals < 0){
		err[0] = E_INVALIDDECIMALS;
		return;
	}

	/*
	 * check for invalid fmtmask
	 */
	if (fmtmask > (M_INSTHOU | M_INSDEC | M_CURRENCY | M_LEFTJUST | M_RIGHTJUST)){
		err[0] = E_INVALIDFMTMASK;
		return;
	}

	/*
	 * parse the directions in fmtmask
	 *
	 * NOTE:
	 *	these instructions can be overridden by the contents of instr
	 *	for example:
	 *
	 *		if the user designates 2 decimal positions and instr
	 *		contains a string with a decimal position in it, then
	 *		that is what the user will get back.
	 */
	insthou = (fmtmask & M_INSTHOU) ? TRUE : FALSE;
	insdec = (fmtmask & M_INSDEC) ? TRUE : FALSE;
	inscurrency = (fmtmask & M_CURRENCY) ? TRUE : FALSE;
	left_justify = (fmtmask & M_LEFTJUST) ? TRUE : FALSE;
	right_justify = (fmtmask & M_RIGHTJUST) ? TRUE : FALSE;
	if (left_justify  &&  right_justify){
		retlength = TRUE;
		right_justify = FALSE;
	}
	else
		retlength = FALSE;


	/*
	 * if insertion is requested, force right justification
	 */
	if (!left_justify  &&  !right_justify  &&  (insthou || insdec || inscurrency))
		left_justify = TRUE;



	/*
	 * if fmtmask != 0
	 * 	strip and copy instr to buff while updating booleans
	 */
	if (fmtmask){
		/*
	 	* allocate buffer space for the condensed number
	 	*/
		if ((buff = calloc((unsigned)(leninstr + 1), 1)) == NULL){
			err[0] = E_5INTERNAL;
			return;
		}



		/*
		 * strip the input string of everything but the digits (0-9)
		 *	and place the result in buff
		 *
		 * this routine also checks for the presence of separators.
		 *
		 */
		decimal_positions = decimals;
		strip_instr(buff, instr, leninstr, &digits_to_left, 
			&negative_number, &positive_sign, &insthou, &insdec, 
			&inscurrency, &decimal_positions, err);

		/*
		 * strip_instr() found an error
		 */
		if (err[0]){
			nlfmt_cleanup(numspec, pNumspec, buff);
			return;
		}

		if (decimal_positions)
			decimals = decimal_positions;

		/*
		 * invalid decimal position?
		 */
		if (digits_to_left < 0){
			err[0] = E_INVALIDDECIMALS;
			nlfmt_cleanup(numspec, pNumspec, buff);
			return;
		}
	}


	/*
	 * if numspec was not given, get it
	 */
	if ( (pNumspec = numspec) == NULL){
		if ((pNumspec = malloc((unsigned)SIZE_NUMSPEC)) == NULL){
			err[0] = E_6INTERNAL;
			nlfmt_cleanup(numspec, pNumspec, buff);
			return;
		}
		nlnumspec(langid, pNumspec, err);
		if (err [0]) {
			nlfmt_cleanup(numspec, pNumspec, buff);
			return;  /* Error in nlnumspec */
		}
	}


	/*
	 * if the language direction is right to left, then justification
	 *	is reversed.
	 */
	memcpy((char *)&langdir, pNumspec, sizeof(short));

	if (langdir){
		if (left_justify){
			left_justify = TRUE;
			right_justify = FALSE;
		}
		else if (right_justify){
			left_justify = FALSE;
			right_justify = TRUE;
		}
	}

	/*
	 * get numeric definitions
	 */
	get_numeric_defs(pNumspec, &decSep, &thouSep, &minusSign, digitRange, 
					err);



	/*
	 * get_numeric_defs found an error in numspec
	 */
	if (err[0]){
		nlfmt_cleanup(numspec, pNumspec, buff);
		return;
	}


	/*
	 * if we're fomatting the string ...
	 */
	if (fmtmask){
		format_outstr(outstr, pLenoutstr, buff, (short)strlen(buff),
			retlength, pNumspec, insdec, insthou, inscurrency,
			left_justify, right_justify, negative_number,
			positive_sign, digits_to_left, decimals, &decSep,
			&thouSep, &minusSign, digitRange, err);
		if (err[0]){
			nlfmt_cleanup(numspec, pNumspec, buff);
			return;
		}
	}
	/*
	 * otherwise we are simply substituting the thousands and decimal
	 *	separators
	 */
	else
		substitute_in_outstr(outstr, *pLenoutstr, instr, leninstr, 
			&decSep, &thouSep, &minusSign, digitRange);

	nlfmt_cleanup(numspec, pNumspec, buff);
}


/*
 * free allocated space
 */
nlfmt_cleanup(numspec, pNumspec, buff)
char *numspec, *pNumspec, *buff;
{
	if (buff != NULL)
		free(buff);
	if (pNumspec != NULL  &&  numspec == NULL)
		free(pNumspec);
}



/*
 * format the outputted string
 */
format_outstr(outstr, pLenoutstr, instr, leninstr, retlength, numspec, insdec, 
	insthou, inscurrency, left_justify, right_justify, negative_number, 
	positive_sign, digits_to_left, decimals, pDecSep, pThouSep, pMinusSign, 
	pDigitRange, err)
char	*outstr, 		/* output string */
	*instr, 		/* input string */
	numspec[],
	*pDecSep, 		/* decimal separator */
	*pThouSep, 		/* thousands separator */
	*pMinusSign, 		/* minus sign */
	*pDigitRange;		/* digit range */
short	*pLenoutstr, 		/* length of output string */
	leninstr, 		/* length of input string */
	decimals,		/* decimal positions */
	err[];			/* return error in first element */
int	insdec,			/* insert decimal separator? */
	insthou, 		/* insert thousands separator */
	inscurrency, 		/* insert currency symbol? */
	retlength,		/* retlength */
	left_justify,		/* left justify? */
	right_justify,		/* right justify? */
	negative_number, 	/* negative number? */
	positive_sign, 		/* should we include a positive sign? */
	digits_to_left;		/* digits to left of decimal */
{
	char	*eOutstr, *eInstr, *startOutstr;
	short	curr_place, curr_bytes;
	int	i;

	eInstr = instr + leninstr;
	eOutstr = outstr + *pLenoutstr;
	startOutstr = outstr;

	memcpy((char *)&curr_place, numspec + NUMSP_CURRENCYPLACE, 
					sizeof(short));
	if (curr_place < 0  ||  curr_place > 2){
		err[0] = E_INVALIDNUMSPEC;
		return;
	}

	memcpy((char *)&curr_bytes, numspec + NUMSP_BYTESCURRENCY, 
					sizeof(short));
	if ((curr_bytes < 0)  ||  (curr_bytes > LENCURRENCYSYMBOL) ||
	    (outstr+curr_bytes > eOutstr)){
		err[0] = E_INVALIDNUMSPEC;
		return;
	}


	/*
	 * put the sign
	 */
	if (negative_number)  *outstr++ = *pMinusSign;
	if (positive_sign)    *outstr++ = '+';


	/*
	 * if the currency symbol precedes the number ...
	 */
	if (inscurrency  &&  curr_place == CURRENCY_PRECEDES){
	    if (outstr + curr_bytes < eOutstr) {
		memcpy(outstr, numspec + NUMSP_CURRENCY, (int)curr_bytes);
		outstr += curr_bytes;
	    } else {
		err [0] = E_TRUNCATION;
		return;
            }
	}



	if (insdec  &&  digits_to_left == 0){
            if (outstr+2 >= eOutstr) {
		err [0] = E_TRUNCATION;
		return;
	    } else {
		*outstr++ = '0';
		*outstr++ = *pDecSep;

		/*
		 * the input field needs to be padded with zeros
		 */
		if (decimals > eInstr - instr){
			for (i = decimals - (int)(eInstr - instr);
			     ((i!=0) && (outstr < eOutstr)); i--)
				*outstr++ = '0';
		}
	    } 
	}

	/*
	 * format the number with decimal and thousands separators
	 */
	for ( ; instr < eInstr  &&  outstr < eOutstr; instr++, outstr++){
		*outstr = *(pDigitRange + *instr - '0') ;
		if (digits_to_left && (insdec  ||  insthou)){
			if (--digits_to_left){
				if (insthou && (digits_to_left % 3) == 0)
					*++outstr = *pThouSep;
			}
			else{
				if (insdec){
					outstr++;
					/* 
					 * symbol replaces radix char 
					 */
					if (curr_place == 2)	
						*outstr = *(numspec + NUMSP_CURRENCY);
					else
						*outstr = *pDecSep;
				}
			}
		}
	}


	/*
	 * check for truncation
	 */
	if (instr != eInstr  &&  outstr == eOutstr){
		err[0] = E_TRUNCATION;
		return;
	}




	/*
	 * if the currency symbol succeedes the number
	 */
	if (inscurrency  &&  curr_place == CURRENCY_SUCCEEDES){
	    if (outstr + curr_bytes < eOutstr) {
		memcpy(outstr, numspec + NUMSP_CURRENCY, (int)curr_bytes);
		outstr += curr_bytes;
	    } else {
		err [0] = E_TRUNCATION;
		return;
            }
	}


	/*
	 * check to see if we want to justify the number
	 */
	if (left_justify)
		fillbuff(startOutstr, (int)(outstr - startOutstr), *pLenoutstr);
	else  if (right_justify)
		rjust(startOutstr, (int)(outstr - startOutstr), *pLenoutstr);



	/*
	 * if the user wants the length returned.
	 */
	if (retlength)
		*pLenoutstr = (short)(outstr - startOutstr);
}




/*
 * simply substitute the thousands and decimal separators
 */
substitute_in_outstr(outstr, lenoutstr, instr, leninstr,
	pDecSep, pThouSep, pMinusSign, pDigitRange)
char	*outstr, 		/* output string */
	*instr, 		/* input string */
	*pDecSep, 		/* decimal separator */
	*pThouSep, 		/* thousands separator */
	*pMinusSign, 		/* minus sign */
	*pDigitRange;		/* digit range */
short	lenoutstr, 		/* length of output string */
	leninstr; 		/* length of input string */
{
	char	*eOutstr, *eInstr;
	int	in_a_number;

	eInstr = instr + leninstr;
	eOutstr = outstr + lenoutstr;
	in_a_number = FALSE;

	for ( ; instr != eInstr  &&  outstr != eOutstr; instr++, outstr++){
		/*
		 * if not in a number previously,
		 *	check if that changed.
		 */
		if (!in_a_number){
			/*
			 * check if a digit
			 * then check if a minus sign
			 */
			if (isascdigit(*instr)){
				*outstr = *(pDigitRange + *instr - '0') ;
				in_a_number = TRUE;
			}
			else if (*instr == '-'){
				*outstr = *pMinusSign;
				in_a_number = TRUE;
			}
			else{
				*outstr = *instr;
			}
		}
		else{
			/*
			 * check for thousands or decimal separators,
			 *	then check if we're still in a number
			 */
			switch (*instr){
				case ASCTHOUSEP:
					*outstr = *pThouSep;
					break;
				case ASCDECSEP:
					*outstr = *pDecSep;
					break;
				default:
					if (isascdigit(*instr))
						*outstr = *(pDigitRange + *instr - '0') ;
					else{
						*outstr = *instr;
						in_a_number = FALSE;
					}
					break;
			}
		}
	}
}





/*
 * strip the separators, the plus or minus signs
 */
strip_instr(t, s, leninstr, pLeft, pNegative, pPositive, pInsthou, 
				pInsdec, pInscurrency, pDecimals, err)
char	*t, 				/* output string */
	*s;				/* input string */
short	leninstr,			/* length of instr */
	*pDecimals,			/* digits to right of decimal */
	err[];				/* error condition */
int	*pLeft, 			/* digits to left of decimal */
	*pNegative, 			/* negative number? */
	*pPositive, 			/* positive sign? */
	*pInsthou, 			/* insert thousands? */
	*pInsdec, 			/* insert decimals? */
	*pInscurrency; 			/* insert currency symbol? */
{
	char	*sEnd, *sStart;
	int	decimal_present;
	short	digits_to_right;

	/*
	 * if there are thousands separtors in the field,
	 *	check that they are properly placed.
	 */
	check_decthou(s, s + leninstr, err, ASCTHOUSEP, ASCDECSEP, &digits_to_right);
	if (err[0])
		return;



	*pNegative = *pPositive = FALSE;
	decimal_present = *pLeft = 0;
	digits_to_right = 0;



	/*
	 * go through the string and get all the digits (0-9)
	 */
	for (sStart = s, sEnd = s + leninstr; s != sEnd; s++){
		if (decimal_present){
			if (isascdigit(*s))
				*t++ = *s;
		}
		else{
			if (isascdigit(*s)){
				*t++ = *s;
				*pLeft += 1;
			}
			else if (*s == ASCDECSEP)
				decimal_present = TRUE;
			else if (*s == '-'){
				/*
				 * check that the minus sign appears either in
				 *	the first position, or right after the
				 *	dollar sign.
				 */
				if (s > sStart + 1  ||  
					(s == sStart + 1 &&  *(s - 1) != '$')){
					err[0] = E_INVALIDNUMBER;
					break;
				}
				*pNegative = TRUE;
			}
			else if (*s == '+'){
				/*
				 * check that the positive sign appears either 
				 *	in the first position, or right after 
				 *	the dollar sign.
				 */
				if (s > sStart + 1  ||  
					(s == sStart + 1 &&  *(s - 1) != '$')){
					err[0] = E_INVALIDNUMBER;
					break;
				}
				*pPositive = TRUE;
			}
			else if (*s == ASCTHOUSEP)
				*pInsthou = TRUE;
			else if (*s == '$'){
				if ( s != sStart){
					if (s - 1 != sStart  || 
						(s - 1 == sStart  &&  *(s - 1) != '-'  &&  *(s - 1) != '+') ){
						err[0] = E_INVALIDNUMBER;
						break;
					}
				}
				*pInscurrency = TRUE;
			}
		}
	}


	/*
	 * calculate the decimal positions
	 */
	if (err[0] == 0){
		if (decimal_present){
			*pDecimals = digits_to_right;
			*pInsdec  = TRUE;
		}
		else{
			*pLeft = *pLeft - *pDecimals;
			if (*pLeft < 0)
				*pLeft = 0;
		}
	}
}
