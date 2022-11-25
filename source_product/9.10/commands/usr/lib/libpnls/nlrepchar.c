


#include <stdio.h>
#include <nlinfo.h>

#define NLCHARTYPE(c)	pcharset[(unsigned char)c]

nlrepchar(instr, outstr, length, repchar, langid, err, charset)
unsigned char	repchar;
char *instr, *outstr, *charset;
unsigned short	err[];
short		length, langid;
{
	char holdChar, *pcharset, *eoutstr;
	struct l_info *getl_info(), *n;

	err[0] = err[1] = 0;

	if (langid == -1) {
	    err [0] = E_LNOTCONFIG;
	    return;
        } 

	if ((n = getl_info(langid))== NULL) {
	    err [0] = E_LNOTCONFIG;
	    return;
        } 

	/*
	 * if the user didn't supply the table:
	 *	allocate space for it,
	 *	and load it with nlinfo().
	 */
	pcharset = charset;
	if (pcharset == NULL)
		pcharset = n->char_set_definition;


	/*
	 * temporarily place the def for the space in holdChar
	 *	and replace it with NLI_LOWER
	 */
	holdChar = pcharset[32];
	pcharset[32] = NLI_LOWER;

	if (NLCHARTYPE(repchar) == NLI_FIRSTOF2) {
	        err [0] = 3;
	        return;
        }

	if (length <= 0) {
	        err [0] = 4;
	        return;
        }


	if ((outstr >= instr)  && (outstr < instr + length)
	||  (instr >= outstr  && instr < outstr + length)){
		err[0] = 8;   /* instring overwrites outstring */
		return;
	}
	
	/*
	 * if character is not printable, substitue the repchar for it
	 *
	 *	SPECIAL for 16 bit languages - do not replace the 2nd of
	 *		two with repchar
	 */
	eoutstr = outstr + length ; 
	if (n->char_size == 0)
		for ( ; outstr < eoutstr ; instr++, outstr++)
			*outstr = (NLCHARTYPE(*instr) == NLI_GRAPH || 
				   NLCHARTYPE(*instr) == NLI_CONTROL)
				   ? repchar : *instr;
	else
		for ( ; outstr < eoutstr ; ) {
			*outstr = (NLCHARTYPE(*instr) == NLI_GRAPH || 
				   NLCHARTYPE(*instr) == NLI_CONTROL)
				   ? repchar : *instr;
			if (NLCHARTYPE(*instr) == NLI_FIRSTOF2) {
			        instr++;
			        outstr++;
			        if (outstr == eoutstr) err [0] = (short) 10;
				else  *outstr = *instr;
                        }
			instr++;
			outstr++;
		}

	pcharset[32] = holdChar;
}

