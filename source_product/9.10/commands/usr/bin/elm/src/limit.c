/**			limit.c				**/

/*
 *  @(#) $Revision: 70.2 $
 *
 *  (c) Copyright Hewlett-Packard Company, 1986, 1987, 1988, 1989
 *  (c) Copyright Yokogawa-Hewlett-Packard Ltd., 1988, 1989
 *
 *  Acknowledgment is made to Dave Taylor for his creation of
 *  the original version of this software.
 *
 *
 *  This stuff is used to 'select' a subset of the existing mail in
 *  the folder based on one of a number of criteria.  The basic 
 *  tricks are pretty easy - we have as status of VISIBLE associated
 *  with each header stored in the mind of the computer (!) and 
 *  simply modify the commands to check that flag...the global 
 *  variable `selected' is set to the number of messages currently
 *  selected, or ZERO if no select.
 */


#include "headers.h"


#define TO		1
#define FROM		2


#ifdef NLS
# define NL_SETN  24
#else NLS
# define catgets(i,sn,mn,s) (s)
#endif NLS


char           *shift_lower();
unsigned long   sleep();

extern int	num_char;

int
limit()

{
	/*
	 *  returns non-zero if we did enough to redraw the screen 
	 */


	char            criteria[SLEN], 
			first[SLEN], 
			rest[SLEN];
	int             last_current,
			last_selected;


	last_selected = selected;	/* for cancel operation */

	if ( selected ) {
		PutLine1( LINES - 2, 0,
		    catgets(nl_fd,NL_SETN,1,"Already have selection criteria - add more? (y/n) n%c"),
		    BACKSPACE );

		if ( hp_terminal )
			define_softkeys( YESNO );

		criteria[0] = want_to( (char *) NULL, 'n', TRUE );

		if ( criteria[0] == 'y' )
			PutLine0( LINES - 3, COLUMNS - 30, 
				  catgets(nl_fd,NL_SETN,2,"Adding criteria...") );
		else if ( criteria[0] == 'n' ) {
			selected = 0;

			/*
			 * Next line is for future enhancement.
			 * PutLine0(LINES - 3, COLUMNS - 30, catgets(nl_fd,NL_SETN,3,"Use '?' for help"));
			 */
		} else {

			if ( selected != last_selected )
				selected = last_selected;

			return ( 0 );
		}
	}

	PutLine1( LINES - 2, 0, catgets(nl_fd,NL_SETN,4,"Enter criteria: ") );
	CleartoEOLN();

	if ( hp_terminal )
		define_softkeys( LIMIT );

	criteria[0] = '\0';

	num_char = strlen(catgets(nl_fd,NL_SETN,4,"Enter criteria: ") );

	if ( optionally_enter( criteria, LINES - 2, num_char, FALSE ) ) {

		if ( selected != last_selected )
			selected = last_selected;

		return ( 0 );
	}

	if ( strlen(criteria) == 0 ) {

		if ( selected != last_selected )
			selected = last_selected;

		return ( 0 );
	}

	/*
	 * Next part is for future enhancement.
	 * if ( strlen( criteria ) == 1 && criteria[0] == '?' ) {
	 *	help_criteria();
	 *      return( TRUE );
	 * }
	 */

	split_word( criteria, first, rest );

	if ( equal(first, "all") ) {
		selected = 0;
		ClearLine( LINES - 2 );
		return ( TRUE );
	}

	last_current = current;
	current = -1;

	if ( equal(first, "subj") || equal(first, "subject") )
		selected = limit_selection( SUBJECT, rest, selected );
	else if ( equal(first, "to") )
		selected = limit_selection( TO, rest, selected );
	else if ( equal(first, "from") )
		selected = limit_selection( FROM, rest, selected );
	else {
		selected = 0;
		error1( catgets(nl_fd,NL_SETN,5,"Don't understand \"%s\" as a selection criteria!"), first );
		(void) sleep( 2 );
	}

	if ( !selected )
		current = last_current;
	else
		current = visible_to_index(1) + 1;	/* map it and shift up 1 */

	if ( !selected )
		set_error( catgets(nl_fd,NL_SETN,6,"no items selected") );
	else {
		sprintf( first, catgets(nl_fd,NL_SETN,7,"%d item(s) selected"), selected );
		set_error( first );
		ClearLine( LINES - 2 );
	}

	return ( selected );
}


int
limit_selection( based_on, pattern, additional_criteria )

	int             based_on, 
			additional_criteria;
	char            *pattern;

{
	/*
	 *  Given the type of criteria, and the pattern, mark all
	 *  non-matching headers as ! VISIBLE.  If additional_criteria,
	 *  don't mark as visible something that isn't currently!
	 */


	register int    index, count = 0;
	register char	buffer[VERY_LONG_STRING];


	dprint( 2, (debugfile, "\n\n\n**limit on %d - '%s' - (%s) **\n\n",
	        based_on, pattern, additional_criteria ? "add'tl" : "base") );

	if ( based_on == SUBJECT ) {
		for ( index = 0; index < message_count; index++ )
			if ( !in_string(shift_lower(header_table[index].subject), pattern) )
				header_table[index].status &= ~VISIBLE;
			else if ( additional_criteria &&
				  !(header_table[index].status & VISIBLE) )
				header_table[index].status &= ~VISIBLE;	/* shut down! */
			else {					/* mark it as readable */
				header_table[index].status |= VISIBLE;
				count++;
				dprint( 5, (debugfile,
					"  Message %d (%s from %s) marked as visible\n",
					index, header_table[index].subject,
					header_table[index].from) );
			}

	} else if ( based_on == FROM ) {
		for ( index = 0; index < message_count; index++ )
			if ( !in_string(shift_lower(header_table[index].from), pattern) )
				header_table[index].status &= ~VISIBLE;
			else if ( additional_criteria &&
				  !(header_table[index].status & VISIBLE) )
				header_table[index].status &= ~VISIBLE;	/* shut down! */
			else {					/* mark it as readable */
				header_table[index].status |= VISIBLE;
				count++;
				dprint( 5, (debugfile,
					"  Message %d (%s from %s) marked as visible\n",
					index, header_table[index].subject,
					header_table[index].from) );
			}

	} else if ( based_on == TO ) {
		for ( index = 0; index < message_count; index++ ) {
			current = index + 1;
			get_existing_address( buffer );
			if ( !in_string(shift_lower(buffer), pattern) )
				header_table[index].status &= ~VISIBLE;

			else if ( additional_criteria &&
				  !(header_table[index].status & VISIBLE) )
				header_table[index].status &= ~VISIBLE;	/* shut down! */
			else {					/* mark it as readable */
				header_table[index].status |= VISIBLE;
				count++;
				dprint( 5, (debugfile,
					"  Message %d (%s from %s) marked as visible\n",
					index, header_table[index].subject,
					header_table[index].from) );
			}
		}
	}

	dprint( 4, (debugfile, "\n** returning %d selected **\n\n\n", count) );

	return ( count );
}


int
next_visible( index )

	int             index;
	
{
	/*
	 *  Given 'index', this routine will return the actual index into the
	 *  array of the NEXT visible message, or '-1' if none are visible 
	 */


#ifdef DEBUG
	register int    remember_for_debug;
	remember_for_debug = index;
#endif


	index--;		/* shift from 'current' to actual index  */
	index++;		/* make sure we don't bump into ourself! */

	while ( index < message_count ) {
		if ( header_table[index].status & VISIBLE ) {
			dprint( 9, (debugfile, "[Next visible: given %d returning %d]\n",
				remember_for_debug, index + 1) );
			return ( index + 1 );
		}
		index++;
	}

	return ( -1 );
}


int
previous_visible( index )

	int             index;

{
	/*
	 *  Just like 'next-visible', but backwards FIRST... 
	 */


#ifdef DEBUG
	register int    remember_for_debug;
	remember_for_debug = index;
#endif


	index -= 2;		/* shift from 'current' to actual index, and
				 * skip us! */

	while ( index > -1 ) {
		if ( header_table[index].status & VISIBLE ) {
			dprint( 9, (debugfile, "[previous visible: given %d returning %d]",
				remember_for_debug, index + 1) );
			return ( index + 1 );
		}
		index--;
	}

	return ( -1 );
}


int
compute_visible( message )

	int             message;

{
	/*
	 *  return the 'virtual' index of the specified message in the
	 *  set of messages - that is, if we have the 25th message as
	 *  the current one, but it's #2 based on our limit criteria,
	 *  this routine, given 25, will return 2.
	 */


	register int    index, 
			count = 0;


	if ( !selected )
		return ( message );

	if ( message < 0 )
		message = 0;				/* normalize */

	for ( index = 0; index <= message; index++ )
		if ( header_table[index].status & VISIBLE )
			count++;

	dprint( 4, (debugfile,
		"[compute-visible: displayed message %d is actually %d]\n",
		count, message) );

	return ( count );
}


int
visible_to_index( message )

	int             message;

{
	/*
	 *  Given a 'virtual' index, return a real one.  This is the
	 *  flip-side of the routine above, and returns (message_count+1)
	 *  if it cannot map the virtual index requested (too big) 
	 */


	register int    index = 0, 
			count = 0;


	for ( index = 0; index < message_count; index++ ) {
		if ( header_table[index].status & VISIBLE )
			count++;
		if ( count == message ) {
			dprint( 4, (debugfile,
				"visible-to-index: (up) index %d is displayed as %d\n",
				message, index) );
			return ( index );
		}
	}

	dprint( 4, (debugfile, "index %d is NOT displayed!\n", message) );

	return ( message_count + 1 );
}
