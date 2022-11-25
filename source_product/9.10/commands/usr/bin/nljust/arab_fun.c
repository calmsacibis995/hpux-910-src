/* @(#) $Revision: 51.1 $ */   

#include	"arab_def.h"

/* alphabetic : examines current input char to determine if it is	*/
/*		an arabic alphabetic character				*/
/* input :	pointer to current character				*/
/* output :	true if current character is arabic alphabetic;		*/
/*		otherwise, false					*/
/* note :	the character HAMZA is a special case.  it is never	*/
/*		connected on the right or the left.  so, it is not	*/
/*		included in the alphabetic check.  see function		*/
/*		context_analysis for special check.			*/

alphabetic (ch)
unsigned char *ch;
{
	if (*ch >= ALF_MADDA_U  &&  *ch <= GHAYN_U
			        ||
	    *ch >= TAMDEED_U    &&  *ch <= YA_U)
	{
		return (TRUE);
	}
	else
	{
		return (FALSE);
	}

}

/* diacritic :  examines current input char to determine if it is	*/
/*		an arabic diacritic character				*/
/* input :	pointer to current character				*/
/* output :	true if current character is arabic diacritic;		*/
/*		otherwise, false					*/

diacritic (ch)
unsigned char *ch;
{
	if (*ch >= TAN_FATHA_U   &&  *ch <= SUKUN_U
			         ||
	    *ch >= SHADDA_FAT_U  &&  *ch <= SHADDA_KAS_U)
	{
		return (TRUE);
	}
	else
	{
		return (FALSE);
	}
}


/* baa_q_family :	examines current character to see if it belongs	*/
/*			to the baa queue family group.  this group of	*/
/*			characters have a particular type of tail which	*/
/*			is appended to the character under certain	*/
/*			conditions.  see context analysis rule 13.	*/
/* input :		pointer to current character			*/
/* output :		true if current character one of the baa queue	*/
/*			family group; otherwise, false.  other code	*/
/*			makes the decision as to whether to add the	*/
/*			tail to the character.				*/

baa_q_family (ch)
unsigned char *ch;
{
	switch (*ch)
	{
		case	BAA_U:
		case	TAA_U:
		case	THAA_U:
		case	FAA_U:
		{
			return (TRUE);
		}
		default:
		{
			return (FALSE);
		}
	}
}

/* seen_q_family :	examines current character to see if it belongs	*/
/*			to the seen queue family group.  this group of	*/
/*			characters have a particular type of tail which	*/
/*			is appended to the character under certain	*/
/*			conditions.  see context analysis rule 14.	*/
/* input :		pointer to current character			*/
/* output :		true if current character one of the seen queue	*/
/*			family group; otherwise, false.  other code	*/
/*			makes the decision as to whether to add the	*/
/*			tail to the character.				*/

seen_q_family (ch)
unsigned char *ch;
{
	if (*ch >= SEEN_U  &&  *ch <= DHAAD_U)
	{
		return (TRUE);
	}
	else
	{
		return (FALSE);
	}
}

/* non_connectable :	examines current character to see if it can be	*/
/*			connected to the left (or next) character.	*/
/*			the character can be connected to the left if	*/
/*			it is alphabetic and not one of 13 characters	*/
/*			which cannot be connected to the left.		*/
/* input :		pointer to current character			*/
/* output :		true if current character CANNOT be connected	*/
/*			to the left; otherwise, false.			*/

non_connectable (ch)
unsigned char *ch;
{
	if (alphabetic (ch))
	{
		switch (*ch)
		{
			case	ALF_MADDA_U:
			case	ALF_HAMZA_U:
			case	WAW_HAMZA_U:
			case	HAMZA_U_ALIF_U:
			case	ALF_U:
			case	TAA_MARBUTA_U:
			case	DAAL_U:
			case	THAAL_U:
			case	RAA_U:
			case	ZAA_U:
			case	WAW_U:
			case	ALF_MAKSURA_U:
			{
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
		return (TRUE);
	}
}

/* left_connected :	examines current character to see if it can be	*/
/*			connected to the left (or next) character.	*/
/*			unlike function non_connectable, this function	*/
/*			looks at the context of the current character.	*/
/*			if current char is connectable, the prior	*/
/*			characters are looked at.			*/
/* input :		pointer to current character			*/
/* output :		true if current character can be connected to 	*/
/*			the next character; otherwise, false.		*/

left_connected (ch)
unsigned char *ch;
{
	if (non_connectable (ch))
	{
		return (FALSE);
	}
	else if (alphabetic (ch-1) || *(ch-1) == TAN_FATHA_U)
	{
		return (TRUE);
	}
	else if (diacritic (ch-1) && *(ch-1) > TAN_KASRA_U && *(ch) != HAMZA_C_U)
	{
		if (alphabetic (ch-2))
		{
			return (TRUE);
		}
		else
		{
			return (FALSE);
		}
	}
	else
	{
		return (FALSE);
	}
}

/* right_connected :	examines current character to see if it can be	*/
/*			connected to the right (or previous) character.	*/
/*			note that HAMZA is a special case -- it is not	*/
/*			connected to the left or right.  the previous	*/
/*			char is examined.  if it belongs to the group	*/
/*			of chars in the 1st part of the case statement,	*/
/*			then the current char cannot be right connected.*/
/*			otherwise, if the previous char is connectable	*/
/*			to the left, then the current char is connectabe*/
/*			to the right.  finally, if the previous char is	*/
/*			a diacritic, then the previous previous char 	*/
/*			must be connectable to the left in order for	*/
/*			the current char to the right connected.	*/
/* input :		pointer to the current character		*/
/* output :		true if the current character can be connected	*/
/*			to the right (or previous) char; otherwise,	*/
/*			false.						*/

right_connected (ch)
unsigned char *ch;
{
	if (*ch == HAMZA_U)
	{
		return (FALSE);
	}
	else if (alphabetic (ch+1))
	{
		if (non_connectable (ch+1))
		{
			return (FALSE);
		}
		else
		{
			return (TRUE);
		}
	}
	else if (diacritic (ch+1) && *(ch+1) > TAN_KASRA_U)
	{
		if (alphabetic (ch+2))
		{
			if (non_connectable (ch+2))
			{
				return (FALSE);
			}
			else
			{
				return (TRUE);
			}
		}
		else
		{
			return (FALSE);
		}
	}
	else
	{
		return (FALSE);
        } 
}
