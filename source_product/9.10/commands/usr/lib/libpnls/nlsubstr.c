


#include <stdio.h>
#include <nlinfo.h>

#define E_INVALIDSOURCELENGTH1		7
#define E_INVALIDSTARTPOSITIONLENGTH	8
#define E_INVALIDLENGTHTOMOVE		9
#define E_RESERVEDNOTZERO		10
#define E_INVALIDFLAGS84		11
#define E_INVALIDFLAGS124		12
#define E_UNDERFLOW			13
#define E_ENDIS1OF2			14

#define NLCHARTYPE(c)	pcharset[(unsigned char)c]

/*
 *	definitions for flags
 */
#define F_RETURNERR	0
#define F_SPP1		1
#define F_SPM1		2
#define F_SPBL		3
#define F_SP		4
#define F_LMP1		1
#define F_LMM1		2
#define F_LMBL		3
#define F_LM		4


extern int _nl_errno;

nlsubstr(string1, length1, string2, plength2, start_pos, length_to_move, 
				langid, flags, err, charset)
unsigned short err[];
short langid, length1, *plength2, start_pos, length_to_move, flags;
char *string1, *string2, *charset;
{
	char	*pcharset, 
		*wrk,				/* working pointer */
		*oWrk;
	struct l_info *n, *getl_info();
	int	fl_1of2;

	err[0] = err[1] = 0;

	if (length1 <= 0) {
		err[0] = E_INVALIDSOURCELENGTH1;
		return;
	}

	if ((start_pos < 0) ||
	    (start_pos >= length1)) {
		err[0] = E_INVALIDSTARTPOSITIONLENGTH;
		return;
	}

	if ((start_pos + length_to_move > length1) ||
	    (length_to_move <= 0)) {
		err[0] = E_INVALIDLENGTHTOMOVE;
		return;
	}

	_nl_errno = 0;
	if (langid != -1) n = getl_info(langid);
	else _nl_errno = 2;
	if (_nl_errno) {
		err [0] = 2;   /* Language not configured */
		return;
        }

	/*
	 *	if 8 bit character set, simply do a memcpy
	 */
	if (n->char_size == 0) {
		memcpy(string2, string1 + start_pos, length_to_move);
		*plength2 = length_to_move;
		return;
	}

	/*
	 * if the user didn't supply the table:
	 *	use the table in l_info
	 */
	if ((pcharset = charset) == NULL)
		pcharset = n->char_set_definition;

	oWrk = string2;
	/*
	 *	for 16 bit characters, check if we are starting on the
	 *	the first of two.
	 */
	for (wrk = string1; wrk < string1 + start_pos; ){
		if (NLCHARTYPE(*wrk) == NLI_FIRSTOF2)
			wrk++;
		wrk++;
	}
	/* 
	 *	we are starting on 2of2 
	 */
	if (wrk > string1 + start_pos) {
		switch (flags & 0xf){
		case F_RETURNERR:
			err[0] = E_UNDERFLOW;
			return;
		case F_SP:
			fl_1of2 = 1;
			wrk = string1 + start_pos;
			break;
		case F_SPBL:
			*oWrk++ = ' ';
		case F_SPP1:
			wrk = string1 + start_pos + 1;
			fl_1of2 = 0;
			break;
		case F_SPM1:
			wrk = string1 + start_pos - 1;
			fl_1of2 = 0;
			break;
		default:
			err[0] = E_INVALIDFLAGS124;
			return;
		}
	}
	else {
		wrk = string1 + start_pos;
		fl_1of2 = 0;
	}


	/*
	 *	our destination is 1 before the end
	 *	this is done because the user may not wish to move the last
	 *	byte if it is the first of two
	 */
	while (wrk < string1 + start_pos + length_to_move - 1) {
		switch (fl_1of2) {
		case 0:
			if (NLCHARTYPE(*wrk) == NLI_FIRSTOF2)
				fl_1of2 = 1;
			break;
		case 1:
			fl_1of2 = 0;
			break;
		}
		*oWrk++ = *wrk++;
	}

	/*
	 *	if fl_1of2 == 1 then the last character was a first of 2
	 */
	if (fl_1of2)
		*oWrk++ = *wrk;
	else if (NLCHARTYPE(*wrk) == NLI_FIRSTOF2){
		switch (flags >> 4){
		case F_RETURNERR:
			err[0] = E_ENDIS1OF2;
			return;
		case F_LM:
			*oWrk++ = *wrk;
			break;
		case F_LMBL:
			*oWrk++ = ' ';
			break;
		case F_LMP1:
			if (wrk == string1 + length1 - 1){
				err[0] = E_ENDIS1OF2;
				return;
			}
			*oWrk++ = *wrk++;
			*oWrk++ = *wrk;
			break;
		case F_LMM1:
			break;
		default:
			err[0] = E_INVALIDFLAGS84;
			return;
		}
	}
	else
		*oWrk++ = *wrk;

	*plength2 = (short)(oWrk - string2);
}

