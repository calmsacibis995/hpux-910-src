


#include <stdio.h>
#include <fcntl.h>
#include <ctype.h>
#include <langinfo.h>
#include <nlinfo.h>

#ifndef TRUE
# define TRUE 1
# define FALSE 0
#endif

/*
 *	the following code is used extensively, and was therefore put into
 *	a macro in order to make the code more readable.
 */
#define NLCHARTYPE(c) pcharset[(unsigned char)c]

short int nlscanmove(instr, outstr, flags, length, langid, err, charset, shift)
unsigned char	*instr, 	/* input buffer */
		*outstr;	/* output buffer */
short	flags;		/* see man page for details */
short   length;		/* length of input buffer */
short	langid;		/* language id */
unsigned short	err[];	/* error return */
char	*charset,	/* character set table */
	*shift;		/* shift table upper/lower */
{
	int	f_upper, f_lower, f_both, f_numeric, f_special, 
		f_onebyte, f_twobyte, f_all,
		f_while, f_until, f_up, f_low, f_scan, f_move;
	char	*pcharset,
		*pshift;
	int	numchar;
	struct l_info *getl_info(), *n;

	err[0] = err[1] = 0;
	n = NULL;

	if (langid == -1){
		err[0] = E_LNOTCONFIG;
		return((short) 0);
	}

	if (flags & M_OTHER){
		err[0] = SME_SCANSHIFT;
		return((short) 0);
	}

	if (length <= 0){
		err[0] = SME_LENGTH;
		return((short) 0);
	}

	f_scan = (instr == outstr) ? TRUE : FALSE;
	if (f_move = (f_scan) ? FALSE : TRUE)
		if ((outstr >= instr  && outstr < instr + length)
		||  (instr >= outstr  && instr < outstr + length)){
			err[0] = SME_OVERLAP;
			return((short) 0);
		}
	
	f_low     = (flags & M_DS)           ? TRUE : FALSE;
	f_up      = (flags & M_US)           ? TRUE : FALSE;

	if (f_low  &&  f_up){
		err[0] = SME_UPDOWN;
		return((short) 0);
	}

	f_upper   = (flags & M_U)            ? TRUE : FALSE;
	f_lower   = (flags & M_L)            ? TRUE : FALSE;
	f_both    = (f_upper  &&  f_lower)   ? TRUE : FALSE;
	f_numeric = (flags & M_N)            ? TRUE : FALSE;
	f_special = (flags & M_S)            ? TRUE : FALSE;
	f_onebyte = (flags & M_OB)           ? TRUE : FALSE;
	f_twobyte = (flags & M_TB)           ? TRUE : FALSE;
	f_all     = (f_both    && f_numeric &&  f_special && 
		     f_onebyte && f_twobyte) ? TRUE : FALSE;
	if (f_all)
		f_while = f_until = FALSE;
	else{
		f_until   = (flags & M_WU)           ? TRUE : FALSE;
		f_while   = (f_until)                ? FALSE : TRUE;
	}



	/*
	 * if needed, get the shift tables.
	 */
	if (f_up  || f_low){
		if ((pshift = shift) == NULL) {
			if ((n = getl_info(langid))==NULL) {
			    err [0] = SME_NOTCONF;
			    return ((short) 0);
                        }
			pshift =  (f_up) ? n->upshift : n->downshift;
		}
	}


	/*
	 * if needed, get the character set table.
	 */
	if (f_upper || f_lower || f_both || f_numeric || f_special ||
	    f_onebyte  || f_twobyte){
		if ((pcharset = charset) == NULL) {
			if (n == NULL)
			         if ((n = getl_info(langid))==NULL) {
			             err [0] = SME_NOTCONF;
			             return ((short) 0);
                                 }
			pcharset =  n->char_set_definition;
		}
	}


	for (numchar = 0 ; numchar < length ; numchar++, instr++, outstr++){
               if (f_while  ||  f_until){
	           if (f_upper || f_lower || f_both || 
		       f_numeric || f_special ||
	               f_onebyte  || f_twobyte){
                              if ((NLCHARTYPE(*instr) > NLI_FIRSTOF2) ||
                                  (NLCHARTYPE(*instr) < NLI_NUMERIC)) {
			               err [0] = SME_INVALIDENTRY;
			               return ((short) 0);
                              }  
		   }
	           if(((f_upper || f_both) && NLCHARTYPE(*instr) == NLI_UPPER)
			|| ((f_lower||f_both)&&NLCHARTYPE(*instr)== NLI_LOWER)
			|| (f_numeric &&  NLCHARTYPE(*instr) == NLI_NUMERIC)
			|| (f_special && (NLCHARTYPE(*instr) == NLI_GRAPH ||
					  NLCHARTYPE(*instr) == NLI_SPECIAL ||
					  NLCHARTYPE(*instr) == NLI_CONTROL))
                        || (f_onebyte && (NLCHARTYPE(*instr) == NLI_UPPER ||
					  NLCHARTYPE(*instr) == NLI_LOWER ||
					  NLCHARTYPE(*instr) == NLI_NUMERIC ||
			                  NLCHARTYPE(*instr) == NLI_GRAPH ||
					  NLCHARTYPE(*instr) == NLI_SPECIAL ||
					  NLCHARTYPE(*instr) == NLI_CONTROL)) 
                        || (f_twobyte &&  NLCHARTYPE(*instr) == NLI_FIRSTOF2)){
				if (f_until)
					break;
			}
			else{
				if (f_while)
					break;
			}
		}
		if (f_move)
			*outstr = ((f_up || f_low) && 
				   (NLCHARTYPE(*instr) != NLI_FIRSTOF2)) ? 
				   pshift[(unsigned char)*instr] : *instr ;
		if (NLCHARTYPE(*instr) == NLI_FIRSTOF2) {
			if (numchar == length -1) {
                           err[0] = SME_OUTOFRANGE;
                           return(numchar);
                           } 
			else {
			       instr++;  /* Go to next character */
			       numchar++;
			       if ((numchar < length) && (f_move))
				    *++outstr = *instr;
                        }
                }
		
	
	}

	return(numchar);
}
