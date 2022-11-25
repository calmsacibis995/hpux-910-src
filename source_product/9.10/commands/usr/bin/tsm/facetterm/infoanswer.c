/*****************************************************************************
** Copyright (c) 1986 - 1990 Structured Software Solutions, Inc.            **
** All Rights Reserved.							    **
*****************************************************************************/
/* @(#) $Header: infoanswer.c,v 70.1 92/03/09 16:40:45 ssa Exp $ */
/**************************************************************************
* Support the terminal description file constructs:
*	fct_info_answer=
*	fct_info_question=
* These are used for cases where the application wants to ask for an
* answer as if it was using the fct_info program, but does not want to run
* another program.
* The semantics are that when the program sees a sequence matching that
* specified in "fct_info_question", it send the first parm to the same
* module as * the fct_info program uses.
* It then responds with the string specified in
* "fct_info_answer" instantiated with the returned information.
* The answer is pasted into the window after getting the receiver's
* attention.
**************************************************************************/
#include <stdio.h>
#include "person.h"
#include "infoansw_p.h"
#include "ftproc.h"
#include "ftwindow.h"
#include "decode.h"

		/**********************************************************
		* Standard allocated structure containing pointer to answer.
		**********************************************************/
typedef struct t_fct_info_question_struct T_FCT_INFO_QUESTION;
struct t_fct_info_question_struct
{
	T_FCT_INFO_QUESTION	*next;
	char			*t_fct_info_answer;
};
T_FCT_INFO_QUESTION	*sT_fct_info_question_ptr =
					{ (T_FCT_INFO_QUESTION *) 0 };

/**************************************************************************
* dec_fct_info_question
*	DECODE has seen a question to be answered by the answer in
*	"t_fct_info_question_ptr".
**************************************************************************/
/*ARGSUSED*/
dec_fct_info_question( unused, t_fct_info_question_ptr,
			 parms_valid, parm, string_parm, string_parms_valid )
	int	unused;
	T_FCT_INFO_QUESTION	*t_fct_info_question_ptr;
	int     parms_valid;
	int     parm[];
	UNCHAR  *string_parm[];
	int     string_parms_valid;
{
	UNCHAR	*s;

	if ( string_parms_valid & 1 )
		s = string_parm[ 0 ];
	else
		s = (UNCHAR *) "";
	fct_fct_info_question( t_fct_info_question_ptr, s );
}
/**************************************************************************
* inst_fct_info_question
*	The terminal description contains a fct_info_question "str" to be 
*	answered by the information in "t_fct_info_question_ptr".
**************************************************************************/
inst_fct_info_question( str, t_fct_info_question_ptr )
	char	*str;
	T_FCT_INFO_QUESTION	*t_fct_info_question_ptr;
{
	dec_install( "fct_info_question", (UNCHAR *) str,
			dec_fct_info_question, 
			0, CURSOR_OPTION,
			(char *) t_fct_info_question_ptr );
}
/**************************************************************************
* fct_fct_info_question
*	DECODE has seen a fct_info_question to be answered by the answer in
*	"t_fct_info_question_ptr".
*	Instantiate the answer with the return from get_information
*	and paste the result to the window.
**************************************************************************/
#define MAX_ANSWER 1024
fct_fct_info_question( t_fct_info_question_ptr, input )
	T_FCT_INFO_QUESTION	*t_fct_info_question_ptr;
	char			*input;
{
	int	parm[ 1 ];
	char	*my_tparm();
	char	*string_parm[ 1 ];
	char	*p;
	char	answer_buff[ MAX_ANSWER + 1 ];
	char	out_buff[ MAX_ANSWER + 1 ];

	get_information( Outwin->number, input, out_buff, MAX_ANSWER );
	string_parm[ 0 ] = out_buff;
	p = my_tparm( t_fct_info_question_ptr->t_fct_info_answer,
		      parm, string_parm, Outwin->number );
	strncpy( answer_buff, p, MAX_ANSWER );
	answer_buff[ MAX_ANSWER ] = '\0';
	paste_to_winno( answer_buff, Outwin->number );
}
/****************************************************************************/
/**************************************************************************
* extra_fct_info_answer
*	Inspect the terminal description file line for types relevant
*	to this module.  Install them if found.
**************************************************************************/
#include "linklast.h"
char	*sT_fct_info_answer_ptr = { (char *) 0 };
char	*dec_encode();
/*ARGSUSED*/
extra_fct_info_answer( buff, string, attr_on_string, attr_off_string ) 
	char	*buff;
	char	*string;
	char	*attr_on_string;		/* not used */
	char	*attr_off_string;		/* not used */
{
	char	*encoded;
	T_FCT_INFO_QUESTION	*t_fct_info_question_ptr;
	long	*mymalloc();

	if ( strcmp( buff, "fct_info_question" ) == 0 )
	{
		/**********************************************************
		* Question is answered by the last fct_info_answer that 
		* was seen.
		* Therefore it must not be first.
		* Allocate memory and store answer
		* Encode fct_info_question
		* install in tree. 
		**********************************************************/
		if ( xT_fct_info_answer_ptr == (char *) 0 )
		{
			printf(
		  "fct_info_question without fct_info_answer - ignored\n" );
		}
		else
		{
			t_fct_info_question_ptr = 
				(T_FCT_INFO_QUESTION *)
				mymalloc( sizeof( T_FCT_INFO_QUESTION ), 
						 "fct_info_question" );
			t_fct_info_question_ptr->next =
						(T_FCT_INFO_QUESTION *) 0;
			link_last( (T_STRUCT *) t_fct_info_question_ptr, 
				   (T_STRUCT *) &xT_fct_info_question_ptr );
			encoded = dec_encode( string );
			t_fct_info_question_ptr->t_fct_info_answer =
						xT_fct_info_answer_ptr;
			inst_fct_info_question( encoded,
						t_fct_info_question_ptr );
		}
	}
	else if ( strcmp( buff, "fct_info_answer" ) == 0 )
	{
		/**********************************************************
		* Encode fct_info_answer and save for a following
		* fct_info_question.
		**********************************************************/
		encoded = dec_encode( string );
		xT_fct_info_answer_ptr =
			(char *) mymalloc( strlen( encoded ) + 1,
					   "fct_info_answer" );
		strcpy( xT_fct_info_answer_ptr, encoded );
	}
	else
	{
		return( 0 );		/* no match */
	}
	return( 1 );
}
