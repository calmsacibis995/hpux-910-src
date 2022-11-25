/*****************************************************************************
** Copyright (c) 1986 - 1990 Structured Software Solutions, Inc.            **
** All Rights Reserved.							    **
*****************************************************************************/
/* @(#) $Header: answer.c,v 70.1 92/03/09 15:40:18 ssa Exp $ */
/**************************************************************************
* Support the terminal description file constructs:
*	answer=
*	question=
* These are used for cases where the application asks the terminal
* questions such as for its identifier or the cursor location.
* The semantics are that when the program sees a sequence matching that
* specified in "question", it responds with the string specified in
* "answer" possibly instantiated with the row and column of the cursor
* position for that window.
* The answer is pasted into the window after getting the receiver's
* attention.
**************************************************************************/
#include <stdio.h>
#include "person.h"
#include "answer_p.h"
#include "ftproc.h"
#include "ftwindow.h"
#include "decode.h"

		/**********************************************************
		* Standard allocated structure containing pointer to answer.
		**********************************************************/
typedef struct t_question_struct T_QUESTION;
struct t_question_struct
{
	T_QUESTION	*next;
	char		*t_answer;
};
T_QUESTION	*sT_question_ptr = { (T_QUESTION *) 0 };

/**************************************************************************
* dec_question
*	DECODE has seen a question to be answered by the answer in
*	"t_question_ptr".
**************************************************************************/
/*ARGSUSED*/
dec_question( unused, t_question_ptr )
	int	unused;
	T_QUESTION	*t_question_ptr;
{
	fct_question( t_question_ptr );
}
/**************************************************************************
* inst_question
*	The terminal description contains a question "str" to be 
*	answered by the information in "t_question_ptr".
**************************************************************************/
inst_question( str, t_question_ptr )
	char	*str;
	T_QUESTION	*t_question_ptr;
{
	dec_install( "question", (UNCHAR *) str, dec_question, 
			0, CURSOR_OPTION,
			(char *) t_question_ptr );
}
/**************************************************************************
* fct_question
*	DECODE has seen a question to be answered by the answer in
*	"t_question_ptr".
*	Instantiate the answer with the cursor position if necessary
*	and paste the result to the window.
**************************************************************************/
#define MAX_ANSWER 1024
fct_question( t_question_ptr )
	T_QUESTION	*t_question_ptr;
{
	int	parm[ 2 ];
	char	*my_tparm();
	char	*string_parm[ 1 ];
	char	*p;
	char	answer_buff[ MAX_ANSWER + 1 ];

	parm[ 0 ] = Outwin->row;
	parm[ 1 ] = Outwin->col;
	p = my_tparm( t_question_ptr->t_answer, parm, string_parm,
							Outwin->number );
	strncpy( answer_buff, p, MAX_ANSWER );
	answer_buff[ MAX_ANSWER ] = '\0';
	paste_to_winno( answer_buff, Outwin->number );
}
/****************************************************************************/
/**************************************************************************
* extra_answer
*	Inspect the terminal description file line for types relevant
*	to this module.  Install them if found.
**************************************************************************/
#include "linklast.h"
char	*sT_answer_ptr = { (char *) 0 };
char	*dec_encode();
/*ARGSUSED*/
extra_answer( buff, string, attr_on_string, attr_off_string ) 
	char	*buff;
	char	*string;
	char	*attr_on_string;		/* not used */
	char	*attr_off_string;		/* not used */
{
	char	*encoded;
	T_QUESTION	*t_question_ptr;
	long	*mymalloc();

	if ( strcmp( buff, "question" ) == 0 )
	{
		/**********************************************************
		* Question is answered by the last answer that was seen.
		* Therefore it must not be first.
		* Allocate memory and store answer - encode question - 
		* install in tree. 
		**********************************************************/
		if ( xT_answer_ptr == (char *) 0 )
		{
			printf( "question without answer - ignored\n" );
		}
		else
		{
			t_question_ptr = 
				(T_QUESTION *) mymalloc( sizeof( T_QUESTION ), 
						         "question" );
			t_question_ptr->next = (T_QUESTION *) 0;
			link_last( (T_STRUCT *) t_question_ptr, 
				   (T_STRUCT *) &xT_question_ptr );
			encoded = dec_encode( string );
			t_question_ptr->t_answer = xT_answer_ptr;
			inst_question( encoded, t_question_ptr );
		}
	}
	else if ( strcmp( buff, "answer" ) == 0 )
	{
		/**********************************************************
		* Encode answer and save for a following question.
		**********************************************************/
		encoded = dec_encode( string );
		xT_answer_ptr = (char *) mymalloc( strlen( encoded ) + 1,
						  "answer" );
		strcpy( xT_answer_ptr, encoded );
	}
	else
	{
		return( 0 );		/* no match */
	}
	return( 1 );
}
/**************************************************************************
* paste_to_winno
*	Paste the string in "buff" into window number "winno".
*	This is called to paste an "answer" into the window in response 
*	to an application's "question".
*	The inputs are stored in externals and a "send_packet_signal"
*	is sent to the receiver.  The receiver's response triggers a 
*	call to check_paste_to_winno_pending (below) that does the actual
*	paste.
*	The inputs are not copied and must not be disturbed before the
*	paste is complete.
**************************************************************************/
char	*Paste_to_winno_ptr = (char *) 0;
int	Paste_to_winno_winno = -1;
paste_to_winno( buff, winno )
	char	*buff;
	int	winno;
{
	Paste_to_winno_winno = winno;
	Paste_to_winno_ptr = buff;
	send_packet_signal();
	while( Paste_to_winno_winno >= 0 )
		fsend_get_acks_only();
}
/**************************************************************************
* check_paste_to_winno_pending
*	Are we waiting to paste an "answer" into a window in response to
*	a "question" from the application.  This is called when the 
*	receiver answers a "send_packet_signal" to see if "paste_to_winno"
*	sent the "send_packet_signal".
* Returns:
*	0 = was not "paste_to_winno"
*	1 = was "paste_to_winno" and "answer" was pasted.
**************************************************************************/
check_paste_to_winno_pending()
{
	if ( Paste_to_winno_winno < 0 )
		return( 0 );
	paste_string_to_winno( Paste_to_winno_winno, Paste_to_winno_ptr );
	fct_window_mode_ans_curwin();
	Paste_to_winno_winno = -1;
	return( 1 );
}
