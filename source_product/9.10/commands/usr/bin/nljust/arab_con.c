/* @(#) $Revision: 62.1 $ */   

#include "arab_def.h"

/* Main driver program for arabic context analysis routine.	*/
/* Input : ch 	points to current character to be analyzed.	*/
/*		The 4 next and 4 previous char surround the	*/
/*		current char.  The input characters are		*/
/*		arabic8 codar_u codes (not character fonts).	*/
/*	   pa	points to start of the character font array.	*/
/* Output :	Character fonts (not arabic8 codar_u codes)	*/
/*		for the current, previous, previous previous,	*/
/*		next and next next character			*/
/*		are returned in the array pointed to by ch.	*/

arabic (ch,pa)
unsigned char *ch,*pa;
{
	unsigned char	cur, prev, prev_prev, next, next_next;

	cur = context_analysis (ch,pa,1);
	prev = context_analysis (ch+1,pa,1);
	prev_prev = context_analysis (ch+2,pa,1);
	next = context_analysis (ch-1,pa,1);
	next_next = context_analysis (ch-2,pa,1);

	*ch = cur;
	*(ch+1) = prev;
	*(ch+2) = prev_prev;
	*(ch-1) = next;
	*(ch-2) = next_next;
}

/* Routine breaks the context analysis down into six cases: latin,	*/
/* alphabetic, space, numbers, diacritics and special characters.	*/
/* Input :	pointers to the character to be analysized and the	*/
/*		font array.						*/
/* Output :	function returns the character font from the array	*/
/*		pointed to by pa of the arabic8 codar_u code		*/
/*		pointed to by ch.					*/

context_analysis (ch,pa,off)
unsigned char *ch,*pa;
int off;
{
	if      (*ch < LATIN_CHAR_BOUNDARY)
	{    
		return (*ch);
	}
        else if (!off)
	{
        	return (off_context(*ch));
	}
	else if (*ch == HAMZA_U || alphabetic (ch))
	{
		return (alpha_shape (ch,pa));
	}
	else if (*ch == ARAB_SPACE_U)
	{
		return (space_shape (ch,pa));
	}
	else if (*ch >= INDIA_ZERO_U && *ch <= INDIA_NINE_U)
	{
		return (*(pa + MISC_CHAR + (*ch - OFF_MS)));
	}
	else if (diacritic (ch))
	{
		return (diac_shape (ch,pa));
	}
	else
	{
        	return (spec_shape (ch,pa));
        } 
}


unsigned char initial [] =

{
 32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 
 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63, 64, 80, 206, 205,
 199, 207, 197, 208, 222, 114, 223, 224, 235, 236, 237, 200, 201, 202,
 203, 244, 245, 246, 247, 213, 214, 226, 227, 91, 92, 93, 94, 95, 126,
 225, 242, 238, 239, 240, 241, 228, 204, 198, 196, 97, 98, 99, 103, 104,
 105, 107, 106, 40, 40, 40, 100, 101, 102, 40, 40, 123, 124, 125, 126
};

off_context(ch)
unsigned char ch;
{
	ch = ch - ARAB_SPACE_U;
	return (initial [ch]);
}

/* initial and medial enhanced laam-alif shapes */

unsigned char n_alfmad_lam[]	= { I_ALF_MAD_LAM,	M_ALF_MAD_LAM };
unsigned char n_alfham_lam[]	= { I_ALF_HAM_LAM,	M_ALF_HAM_LAM };
unsigned char n_hamza_alif_lam[]= { I_HAMZA_ALIF_LAM,	M_HAMZA_ALIF_LAM };
unsigned char n_alfafter_lam[]	= { I_ALF_AFTER_LAM,	M_ALF_AFTER_LAM };

/*
** LAAM macro:  Input parameter (l) is one of the above LAAM-ALIF arrays.
** If ALIF shape is preceeded by a medial LAAM, get medial LAAM-ALIF.
** If ALIF shape is preceeded by an initial LAAM, get initial LAAM-ALIF.
** Otherwise, just copy the shape from input (p2) to output (p1).
*/

#define	HAVE_LAAM(l)			i = 0; \
					switch (*(p2+1)) { \
					case M_LAAM: i++; \
					case I_LAAM: p2++; *p1 = l[i]; break; \
					default: *p1 = *p2; } \

/*
** Scan input string (s2) for an old ALIF following LAAM font.
** If the preceeding font is a new enhanced initial or medial LAAM,
** substitute the appropriate single LAAM-ALIF shape for the old two
** font combination.  Return a pointer to the output string (s1).
** This routine assumes that the context analysis has already
** been done.  It deals with fonts not codes.  It is used in a
** second pass over the input string (the context analysis was the
** first pass).
*/

unsigned char *
laam_alif(s1,s2)
unsigned char *s1;			/* output */
unsigned char *s2;			/* input */
{
	register unsigned char *p1;
	register unsigned char *p2;
	register int i;

	for (p1=s1, p2=s2 ; *p2 ; p1++, p2++) {
		switch (*p2) {
		case F_ALF_MAD_LAM:
			HAVE_LAAM(n_alfmad_lam);
			break;
		case F_ALF_HAM_LAM:
			HAVE_LAAM(n_alfham_lam);
			break;
		case F_HAMZA_ALIF_LAM:
			HAVE_LAAM(n_hamza_alif_lam);
			break;
		case F_ALF_AFTER_LAM:
			HAVE_LAAM(n_alfafter_lam);
			break;
		default:
			*p1 = *p2;
		}
	}
	*p1 = '\0';
	return s1;
}
