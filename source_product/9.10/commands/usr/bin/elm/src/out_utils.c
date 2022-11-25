/**			out_utils.c				**/

/*
 *  @(#) $Revision: 66.1 $
 *
 *  (c) Copyright Hewlett-Packard Company, 1986, 1987, 1988, 1989
 *  (c) Copyright Yokogawa-Hewlett-Packard Ltd., 1988, 1989
 *
 *  Acknowledgment is made to Dave Taylor for his creation of
 *  the original version of this software.
 *
 *
 *  This file contains routines used for output in the ELM program.
 */


#include "headers.h"


static char err_buffer[SLEN];			/* store last error message */

static char central_message_buffer[SLEN];

char 		*strcpy(),
		*strncpy();


int 
show_last_error()

{
	/*
	 *  rewrite last error message! 
	 */

	error( err_buffer );
}


int
clear_error()

{
	MoveCursor( LINES,0 );
	CleartoEOLN();
	err_buffer[0] = '\0';
}


int
set_error( s )

	char 	*s;

{
	if ( strlen(s) < SLEN )
		strcpy( err_buffer, s );
	else {
		strncpy( err_buffer, s, SLEN-1 );
		err_buffer[SLEN-1] = '\0';
	}
}


int
set_error1( s, a )

	char 	*s, 
		*a;

{



	int	s_len, a_len;
	char	buffer[SLEN],
		buffer1[SLEN];


	s_len = strlen( s );
	a_len = strlen( a );

	if ( s_len + a_len < SLEN )
		sprintf( buffer, s, a );
	else {
		strncpy( buffer1, a, SLEN-s_len-1 );
		buffer1[SLEN - s_len] = '\0';
		sprintf( buffer, s, buffer1 );
	}
		
	strcpy( err_buffer, buffer );
}


int
set_error2( s, a1, a2 )

	char	*s, 
		*a1,
		*a2;

{
	/*
	 *  same as set_error, but with two 'printf' arguments 
	 */


	int  s_len, a1_len, a2_len;
	char buffer[SLEN],
	     buffer1[SLEN];


	s_len = strlen( s );
	a1_len = strlen( a1 );
	a2_len = strlen( a2 );

	if ( s_len+a1_len+a2_len < SLEN )

		sprintf( buffer,s, a1, a2 );

	else if ( s_len+a1_len > SLEN ) {

		strncpy( buffer1, a1, SLEN - s_len - 1 );
		buffer1[SLEN - s_len] = '\0';
		sprintf( buffer, s, buffer1, "" );

	} else {

		strncpy( buffer1, a2, SLEN - s_len - a1_len - 1 );
		buffer1[SLEN - s_len - a1_len] = '\0';
		sprintf( buffer, s, a1, buffer1 );

	}

	strcpy( err_buffer, buffer );
}


int
error( s )

	char 	*s;

{
	/*
	 *  outputs error 's' to screen at line 22, centered! 
	 */

	MoveCursor( LINES,0 );
	CleartoEOLN();
	PutLine0( LINES,(COLUMNS-strlen(s))/2,s );
	fflush( stdout );
	strcpy( err_buffer, s );		/* save it too! */
}


/*VARARGS1*/

int
error1( s, a )

	char 	*s, 
		*a;

{
	/*
	 *  same as error, but with a 'printf' argument 
	 */


	int	s_len, a_len;
	char	buffer[SLEN],
		buffer1[SLEN];


	s_len = strlen( s );
	a_len = strlen( a );

	if ( s_len + a_len < SLEN )
		sprintf( buffer, s, a );
	else {
		strncpy( buffer1, a, SLEN-s_len-1 );
		buffer1[SLEN - s_len] = '\0';
		sprintf( buffer, s, buffer1 );
	}
		
	error( buffer );
}


/*VARARGS1*/

int
error2( s, a1, a2 )

	char	*s, 
		*a1,
		*a2;

{
	/*
	 *  same as error, but with two 'printf' arguments 
	 */


	int  s_len, a1_len, a2_len;
	char buffer[SLEN],
	     buffer1[SLEN];


	s_len = strlen( s );
	a1_len = strlen( a1 );
	a2_len = strlen( a2 );

	if ( s_len + a1_len + a2_len < SLEN )

		sprintf( buffer, s, a1, a2 );

	else if ( s_len + a1_len > SLEN ) {

		strncpy( buffer1, a1, SLEN - s_len - 1 );
		buffer1[SLEN - s_len] = '\0';
		sprintf( buffer, s, buffer1, "" );

	} else {

		strncpy( buffer1, a2, SLEN - s_len - a1_len - 1 );
		buffer1[SLEN - s_len - a1_len] = '\0';
		sprintf( buffer, s, a1, buffer1 );

	}

	error( buffer );
}


/*VARARGS1*/

int
error3( s, a1, a2, a3 )

	char 	*s, 
		*a1, 
		*a2,
		*a3;

{
	/*
	 *  same as error, but with three 'printf' arguments 
	 */


	int  s_len, a1_len, a2_len, a3_len;
	char buffer[SLEN],
	     buffer1[SLEN];


	s_len = strlen( s );
	a1_len = strlen( a1 );
	a2_len = strlen( a2 );
	a3_len = strlen( a3 );

	if ( s_len+a1_len+a2_len+a3_len < SLEN )

		sprintf( buffer,s, a1, a2, a3 );

	else if ( s_len+a1_len > SLEN ) {

		strncpy( buffer1, a1, SLEN - s_len - 1 );
		buffer1[SLEN - s_len] = '\0';
		sprintf( buffer, s, buffer1, "", "" );

	} else if ( s_len+a1_len+a2_len > SLEN ) {

		strncpy( buffer1, a2, SLEN - s_len - a1_len - 1 );
		buffer1[SLEN - s_len - a1_len] = '\0';
		sprintf( buffer, s, a1, buffer1, "" );

	} else {

		strncpy( buffer1, a3, SLEN-s_len-a1_len-a2_len-1 );
		buffer1[SLEN - s_len - a1_len - a2_len] = '\0';
		sprintf( buffer, s, a1, a2, buffer1 );

	}

	error( buffer );
}


int
lower_prompt( s )

	char 	*s;

{
	/*
	 *  prompt user for input on LINES-1 line, left justified 
	 */

	SetCursor( LINES-1, 0 );	/* force current position for cursor.c */
	PutLine0( LINES-1,0,s );
	CleartoEOLN();
}


int
prompt( s )

	char 	*s;

{
	/*
	 *  prompt user for input on LINES-3 line, left justified 
	 */

	SetCursor( LINES-3, 0 );	/* force current position for cursor.c */
	PutLine0( LINES-3,0,s );
	CleartoEOLN();
}


int
set_central_message( string, arg )

	char	*string, 
		*arg;

{
	/*
	 *  set up the given message to be displayed in the center of
	 *  the current window 
	 */

	sprintf( central_message_buffer, string, arg );
}


int
display_central_message()

{
	/*
	 *  display the message if set... 
	 */

	if ( central_message_buffer[0] != '\0' ) {
	  	ClearLine( LINES-15 );
	  	Centerline( LINES-15, central_message_buffer );
	  	fflush( stdout );
	}
}


int
clear_central_message()

{
	/*
	 *  clear the central message buffer 
	 */

	central_message_buffer[0] = '\0';
}
