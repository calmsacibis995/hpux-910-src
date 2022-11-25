/* @(#) $Revision: 51.1 $ */   

#include	"arab_def.h"

/* alpha_shape :	examines current character and returns font	*/
/*			shape of the alphabetic character (which does 	*/
/*			not include numeric, special, or diacritic	*/
/*			marks).						*/
/* input :		pointer to current character & font array	*/
/* output :		code of alphabetic character font		*/

alpha_shape (ch,pa)
unsigned char *ch,*pa;
{

	unsigned char	char_shape;

	if (process_laam_alif (ch,&char_shape))
	{
		return (*(pa + LAAM_ALIF + char_shape));
	}
	else
	{
		return (*(pa + ARAB_MAP + ((*ch - OFF_AR)*4 + cont_shape (ch))));
	}
}

/* cont_shape :examines current alphabetic character and returns the	*/
/*		context shape (isolated, initial, medial, final) of the */
/*		character.						*/
/* input :	pointer to current character				*/
/* output :	context shape of character				*/

cont_shape (ch)
unsigned char *ch;
{

	if (left_connected (ch))
	{
		if (right_connected (ch))
		{
			return (MEDIAL);
		}
		else
		{
			return (INITIAL);
		}
	}
	else if (right_connected (ch))
	{
		return (FINAL);
	}
	else
	{
		return (ISOLATED);
	}
}

/* process_laam_alif :	if previous character is a LAAM and the current	*/
/*			character belongs to the "Laam Alif Group" then	*/
/*			special character font shapes are used.		*/
/* input :		pointer to current character & char shape	*/
/* output :		true if previous is LAAM and current belongs to	*/
/*			"Laam Alif Group"; otherwise, false.		*/
/*			if true, char_shape returns with font code;	*/
/*			otherwise, char_shape undefined.		*/

process_laam_alif (ch,cs)
unsigned char *ch,*cs;
{
	if (*(ch+1) == LAAM_U)
	{
		switch (*ch)
		{
			case	ALF_MADDA_U:
			{
				*cs = ALF_MAD_LAM;
				return (TRUE);
			}
			case	ALF_HAMZA_U:
			{
				*cs = ALF_HAM_LAM;
				return (TRUE);
			}
			case	HAMZA_U_ALIF_U:
			{
				*cs = HAMZA_ALIF_LAM;
				return (TRUE);
			}
			case	ALF_U:
			{
				*cs = ALF_AFTER_LAM;
				return (TRUE);
			}
			default:
			{
				return (FALSE);
			}
		}
	}
	else
	{
		return (FALSE);
	}
}

/* space_shape :	examines previous character in order to return	*/
/*			1 of 3 different space fonts: baa_queue family	*/
/*			space, seen_queue family space, or space.	*/
/* input :		pointer to current character			*/
/*			pointer to font array				*/
/* output :		code of space character font			*/

space_shape (ch,pa)
unsigned char *ch,*pa;
{
	if (baa_q_family (ch+1) || *(ch+1) == TAMDEED_U)
	{
		return (*(pa + BA_Q));
	}
	else if (seen_q_family (ch+1))
	{
		return (*(pa + SEEN_Q));
	}
	else
	{
		return (*(pa + SPACE_CHAR));
	}
}

/* spec_shape :examines current character and returns font codes of	*/
/*		special characters.  three tables of special characters	*/
/*		contain the possible font shapes.  if current char is	*/
/*		not in one of these three tables, then a space is 	*/
/*		returned.						*/
/* assumption: spec_shape is the last function called by               */
/*		context_analysis.					*/
/* input :	pointer to current character & font table		*/
/* output :	code of special character font				*/

spec_shape (ch,pa)
unsigned char *ch,*pa;
{
	if (*ch <= COMMERCIAL_AT_U)
	{
		return (*(pa + MISC_CHAR + (*ch - OFF_MS)));
	}
	else if (*ch <= UNDERLINE_U)
	{
		return (*(pa + SPEC_1 + (*ch - OFF_S1)));
	}
	else if (*ch <= OVERLINE_U && *ch >= OPEN_BRACE_U)
	{
		return (*(pa + SPEC_2 + (*ch - OFF_S2)));
	}
	else
	{
		return (*(pa + SPACE_CHAR));
	}
}

/* diac_shape :examines current diacritic character and returns	*/
/*		font according to rules 11, 14 - 20.			*/
/* input :	pointer to current diacritic character & font table	*/
/* output :	diacritic font code					*/

diac_shape (ch,pa)
unsigned char *ch,*pa;
{
	int	prior_shape;

	if (*(ch+1) == HAMZA_U || alphabetic (ch+1))
	{      
		if (*ch == TAN_FATHA_U)
		{
			switch (*(ch+1))
			{
				case	ALF_MADDA_U:
				case	ALF_HAMZA_U:
				case	ALF_U:
				case	ALF_MAKSURA_U:
				case	WAW_HAMZA_U:
				case	TAA_MARBUTA_U:
				case	HAMZA_U_ALIF_U:
				{
					return (*(pa + DIAC_NORMAL + (*ch - OFF_DN)));
				}
				case	HAMZA_U:
				case	DAAL_U:
				case	THAAL_U:
				case	RAA_U:
				case	ZAA_U:
				case	WAW_U:
				{
					return (*(pa + ALF_TANWEEN));
				}
				default:
				{
					return (*(pa + TAN_FAT_BQ));
				}
			}
		}
		else
		{
			if (non_connectable (ch+1))
			{
				return (*(pa + DIAC_NORMAL + (*ch - OFF_DN)));
			}
			else
			{
				prior_shape = cont_shape (ch+1);
				if (prior_shape == FINAL || prior_shape == ISOLATED)
				{
					if (baa_q_family (ch+1) || *(ch+1) == TAMDEED_U)
					{
						return (*(pa + DIAC_BAA_Q + (*ch - OFF_DN)));
					}
					else if (seen_q_family (ch+1))
					{
						return (*(pa + DIAC_SEEN_Q + (*ch - OFF_DN)));
					}
					else
					{
						return (*(pa + DIAC_NORMAL + (*ch - OFF_DN)));
					}
				}
				else
				{
					if (*ch == TAN_KASRA_U || *ch == TAN_DHAMMA_U)
					{
						return (*(pa + BUG_CHAR));
					}
					else
					{
						return (*(pa + DIAC_MEDIAL + (*ch - OFF_DM)));
					}
				}
			}
		}
	}
	else
	{
		return (*(pa + DIAC_NORMAL + (*ch - OFF_DN)));
        } 
}
