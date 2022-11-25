/* @(#) $Revision: 64.3 $ */

/*
**	1.1 Data Order Conversion
**
**	The strord routine converts the order of characters in a
**	string from screen to keyboard or vice versa.  If the string to
**	be converted is a Latin mode string, then all non-Latin
**	sub-strings are reversed.  Similarly, if the string to be
**	converted is a non-Latin mode string, then all Latin sub-strings
**	are reversed.  Hindi numbers always have a left-to-right
**	orientation.  Hindi numbers can represent both integers and
**	floating-point numbers.  The alternative radix point is part of
**	a Hindi floating-point number.
**
**	1.2 Latin and non-Latin Sub-Strings
**
**	Latin and non-Latin sub-strings are identified by the most
**	significant bit of their characters.  The most significant
**	bit is off for a Latin character and on for a non-Latin
**	character.
**
**	Space characters are part of (and do not delimit) Latin and
**	non-Latin strings.  Space characters include Latin spaces
**	(0x20) and non-Latin spaces (0xa0).
**
**	Regular Ascii control codes (0x00 - 0x1f) may terminate
**	Latin and non-Latin sub-strings.  Latin strings are delimited by
**	non-Latin characters or Ascii control characters.  Non-Latin
**	strings are delimited Latin characters or Ascii control
**	characters.
**
**	1.3 Data Conversion Example
**
**	Assume the following code:
**
**	1. Ai : Arabic letters.
**	2. Hi : Hindi digits.
**	3. Li : Latin letters.
**	4. Di : Latin digits.
**	5. t : tab (0x09).
**	6. b : Latin space (0x20).
**	7. B : Non-Latin space (0xa0).
**
**	The following examples summarize the data conversion:
**
**	1. Latin Mode: Phonetic to Screen
**
**	File Contents:
**
**	A1 A2 A3 B B H1 H2 H3 t A4 A5 A6 t t L1 L2 L3 t L4 L5 L6 b D1 D2 D3
**
**	Result of Conversion:
**
**	H1 H2 H3 B B A3 A2 A1 t A6 A5 A4 t t L1 L2 L3 t L4 L5 L6 b D1 D2 D3
**
**	Screen to Phonetic conversion results in the opposite mapping.
**
**	2. Non-Latin Mode: Phonetic to Screen
**
**	File Contents:
**
**	A1 A2 A3 B B H1 H2 H3 t A4 A5 A6 L1 L2 L3 t t L4 L5 L6 b D1 D2 D3 t
**
**	Result of Conversion:
**
**	A1 A2 A3 B B H3 H2 H1 t A4 A5 A6 L3 L2 L1 t t D3 D2 D1 b L6 L5 L4 t
**
**	Screen to Phonetic conversion results in the opposite mapping.
**
**	1.4 Routine Arguments
**
**	The routine has the following parameters:
**
**	1. an output string (null terminated array of characters)
**	2. an input string (null terminated array of characters)
**	3. mode information (of enum type nl_mode: NL_LATIN or NL_NONLATIN)
**
**	The routine does not check for output string overflow.
**	The routine also assumes a succesful prior call to nl_init(3c).
**
*/
#ifdef _NAMESPACE_CLEAN
#define strord _strord
#endif


#include	<ctype.h>
#include	<nl_types.h>

#define NULL		'\0'
#define OPP_LANG(c)	(mode == NL_LATIN ? ((unsigned int)c >= 128) : ((unsigned int)c < 128))
#define IS_HINDI(c)	((unsigned char)c == 0247 || ((unsigned char)c >= 0260 && (unsigned char)c <= 0271))
#define REV(start, finish, tmp) 	while ( start < finish)		\
					{  				\
						tmp = *start;		\
						*start = * finish; 	\
						* finish =  tmp; 	\
						start++;  finish--; 	\
					}
#ifdef _NAMESPACE_CLEAN
#undef strord
#pragma _HP_SECONDARY_DEF _strord strord
#define strord _strord
#endif

char *
strord(str_out, str_in, mode)
char *str_out;
char *str_in;
nl_mode mode;
{
	extern unsigned char *_nl_dgt_alt;	/* alternative digit string */

	register char 	*pstr_out = str_out;
	register char 	*pstr_in  = str_in;
	register char 	*tpptr;
	register char 	*endptr;
	register int    swapint;

	while(*str_in) {

		/* copy control characters */

		while (*str_in && iscntrl((int)(unsigned char)*str_in)) {
			*pstr_out++ = *str_in++;
		}

		/* copy primary language characters up to opp lang */

		while (	(!OPP_LANG(*str_in)) 
			&& (*str_in) 
			&& !(iscntrl((int)(unsigned char)*str_in)))
				*pstr_out++ = *str_in++;
		pstr_in = str_in;

		/* find end of opposite language string */

		while 	(OPP_LANG(*pstr_in) 
			&& (*pstr_in) 
			&& (!(iscntrl((int)(unsigned char)*pstr_in))))
				pstr_in++;	
		tpptr = pstr_in;

		/* flip opposite language characters */

		while (pstr_in != str_in)
			*pstr_out++ = *--pstr_in;
		str_in = tpptr;
	}
				
	*pstr_out = NULL;

	/* if we have hindi digits, make another pass and flip them */

 	if (*_nl_dgt_alt)
	{
		pstr_out = str_out;

		while (*pstr_out){
			while (*pstr_out && !IS_HINDI(*pstr_out))
				pstr_out++;
			if (IS_HINDI(*pstr_out))
			{
				tpptr = pstr_out++;
				while (IS_HINDI(*pstr_out))
					pstr_out++;

				endptr = pstr_out;  /* save end of string */

				pstr_out--; /* we passed end of HINDI by one*/

				REV(tpptr, pstr_out, swapint);
				pstr_out = endptr;
			}
		}

	}
	return(str_out);
}
