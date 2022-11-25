/* @(#) $Revision: 72.3 $ */      

/* Include headers */

# include	"curses.ext"
# include	<ctype.h>
# include	<nl_ctype.h>

/* Define External Variables (global)	*/

extern	short	_oldx;
extern	short	_oldy;
extern	chtype	_oldctrl;
extern	WINDOW	*_oldwin;
extern	int	_poschanged;

/* Define a constant for the character SPACE */

#define SPACE	' '

/* Define UPDATE_FIRST_LAST macro */

/* Update First/Last macro will check the starting 	*/
/* starting position against the win->_firstch field 	*/
/* for the current row, if the value of startpos is 	*/
/* lower, then update the _firstch, then check the final*/
/* position of x for the current row.  If this position */
/* minus 1 is > _lastch then update the lastch field. 	*/
/*							*/
/*  Note: _firstch and _lastch represents the first and */
/*	  last characters changed on a given row.	*/
/*							*/
/* 	   x represents the current position and 	*/
/*	   this position has not yet been changed.	*/
/* 	   The last changed column is represented by	*/
/*	   the value of x-1.			        */
/*							*/

#define UPDATE_FIRST_LAST	\
	if ( win->_firstch[y] != _NOCHANGE ) 				\
	{								\
	   if ( startpos < win->_firstch[y] ) {				\
	      win->_firstch[y] = startpos;				\
	   }								\
	   if ( win->_lastch[y] < (x-1) )  	win->_lastch[y] = x-1;	\
	} else								\
	{								\
	   win->_firstch[y] = startpos;					\
	   win->_lastch[y] = x-1;					\
   	} 

/* Define DO_WRAP macro */

/* Do Wrap Macro:  Perform the checks and  update the  	*/ 
/* appropriate _lastch and _firstch fields for the     	*/
/* Line as wrapping occurs.  This macro should be used 	*/
/* whenever a line-wrap has occurred.			*/

#define DO_WRAP		\
	UPDATE_FIRST_LAST;						\
	x = 0; y++;  /* Do Wrap (x reset to 0 and y move to next line */ \
	startpos = 0;							\
	DO_SCROLLING_CHECK

/* end of DO_WRAP Macro */

/*  DO_SCROLLING_CHECK Macro 	*/

/* Do Scrolling check Macro: Perform the checks appropriate */
/* for vertical scrolling.  This only occurs if the cursor  */
/* was moved beyond the bottom of the screen.  When this    */
/* happens, the values for y must be decremented to rep-    */
/* represent hitting the bottom of the window and then      */
/* a call to wrefresh and _tscroll is made to insure that   */
/* the screen display is what we expect.		    */

#define DO_SCROLLING_CHECK						\
									\
	if ( y > win->_bmarg )						\
	{								\
	   if ( win->_scroll && !(win->_flags &_ISPAD))			\
	   {								\
		wrefresh(win); /* refresh the display to 'win' */	\
		_tscroll(win); /* scroll the display */			\
		--y;							\
	   } else 							\
	   {								\
		return ERR;						\
	   }								\
	}

/* end of DO_SCROLLING_CHECK Macro */
/*
 *	This routine adds a string starting at (_cury,_curx)
 *
 */
int waddstr(win,str)
register WINDOW	*win; 
register unsigned char	*str;
{
   register 	chtype	ch;
   register	int	x,y,startpos;
		char	*uctr;
		int	rightmostpos=_NOCHANGE;


# ifdef DEBUG
	if(outf)
	{
		if( win == stdscr )
		{
			fprintf(outf, "WADDSTR(stdscr, ");
		}
		else
		{
			fprintf(outf, "WADDSTR(%o, ", win);
		}
		fprintf(outf, "\"%s\")\n", str);
	}
# endif	DEBUG

/* 
   Determine if special processing needs to be conducted base on the
   value of _oldctrl.  If _oldctrl is set then check if one of the 
   following conditions exists:

	* If user is adding to a different window than the one that is 
	  associated with _oldctrl, then do special processing (_setctrl).

	* If _oldctrl position moved by more than 1 character to the right
	  and did not wrap to the next line, then do special processing
	  (due to a move call or similar occurring).  

   Note:
	_oldctrl will normally contain either a 0 or the first byte of a 
	potential two-byte sequence.  The intended use for _oldctrl is to
	be able to identify two byte character sequences ( in 16 bit languages)
	and to be able to recall the value of the first byte of the two byte
	sequence.  The assumptions associated with _oldctrl are that the
	two byte sequence being added/inputted will always occur one right
	after the other for any specific window.

*/

   if (_oldctrl)
   {
	if ( (win != _oldwin) ||
	     (!(_oldy==win->_cury && _oldx==win->_curx-1) && 
	      !(_oldx==win->_maxx-1 && _oldy==win->_cury-1)) )
	{
		_setctrl();
	}
   }

   /* Reset _poschanged flag, since it's already been handled 	*/
   /* by the above checks.					*/


   _poschanged = 0;

/* ------------------------------------------------------------ */
/* Set the x,y positions to the current virtual position   	*/
/* as determined by the window specified (win).			*/

   x = win->_curx;
   y = win->_cury;

/* Begin Processing the String					*/
/* Processing will be based on the following:			*/
/* 	If the current language is 16-bit then use the 16-bit	*/
/* 	processing loop, otherwise, use the 8-bit processing	*/
/* 	loop.							*/

/* The basic differences between the two loops is that of 	*/
/* additional checks.  The 16-bit loop will have to check for	*/
/* more conditions than the 8-bit loop.				*/


   if ( ! _CS_SBYTE )   
   { 
      /* If the current character at the current position contains */
      /* a character that is the second of two bytes and the input */
      /* stream is at least 1 byte in size, then we must blank out */
      /* the previous position.	  And update the _firstch field	   */
      /* accordingly.					 	   */

      if ( win->_y[y][x] & A_SECOF2 && *str ) 
      {  
	 win->_y[y][x-1] = (win->_y[y][x-1] & A_ATTRIBUTES ) | SPACE;
	 if ( (x-1) < win->_firstch[y] ) win->_firstch[y] = x-1;
      }
   }

	startpos=x;

	while ( *str ) /* While there are characters in the input string */
	{
	   /* There are five possible actions, base on the input character.*/
	   /*     (1) Tab character, (2) New line character,		   */
	   /*	  (3) Carriage Return, (4) Backspace  and (5) All others.  */
	   /* Note: See description below for specific treatments.	   */

	   ch = (unsigned int) *str;

	   if (ch != '\t' ) {
	     if ( ch != '\n' ) {
		if ( ch != '\r' ) {
  		   if ( ch != '\b' ) {

/* ---------------------------------------------------------------------*/
/*	Code block to process default condition of input character	*/
/* ---------------------------------------------------------------------*/
/*  Process ALL other characters. In this category, it has been 	*/
/*  determined that the current character is not one of the above 	*/
/*  special characters.  In this case, we will then simply place the 	*/
/*  character into the appropriate place (as defined by the current x,y)*/
/*  on the screen along with the current attributes.  As part of this   */
/*  we will also check for line-wraps and scrolling (vertical wrap).	*/

		if (  _CS_SBYTE ||   /* If  1-byte char code or 2-bytes */
		    ! ( FIRSTof2((unsigned int)ch) && SECof2((unsigned int) *(str+1))) )  
		{
		   if ( !isprint((unsigned int)ch) && !(0240 <= ch <= 0376) ) {
		      /* If the character is a control character then, */
		      /* apply the unctrl function to generate a two   */
		      /* character sequence for this control character.*/
		      /* Then place the first of the two character     */
		      /* sequence into the window display area, and    */
		      /* Follow on by setting the current character    */
		      /* to the second character, thereby allowing the */
		      /* normal processing to finish the job.	    */

		      uctr = unctrl(ch);
		      win->_y[y][x++] = *uctr | win->_attrs;
		      if ( win->_maxx <= x ) {
		         DO_WRAP;
		      }
		     ch = (unsigned int) uctr[1];
		   }
		} else {
		   /* We have a two-byte character.  insert the */
		   /* first byte into the current position, if  */
		   /* there is room on the current line for both*/
		   /* else, write a blank space in and perform a*/
		   /* wrap to the next line and insert both     */
		   /* bytes there.			   	    */
		   if ( win->_maxx-1 <= x ) 
		   {
		      win->_y[y][x++] = SPACE | win->_attrs ;
		      DO_WRAP;
		   }
		   win->_y[y][x++] = ch | win->_attrs | A_FIRSTOF2;
		   ch = *(++str) | A_SECOF2;
		}



		win->_y[y][x++] = ch | win->_attrs;
		if ( win->_maxx <= x  ) {
		   DO_WRAP;
		}
/* ---------------------------------------------------------------------*/
/*	End Code block to process default condition of input character	*/
/* ---------------------------------------------------------------------*/
		   } else {
/* ---------------------------------------------------------------------*/
/*	Code block to process backspace 				*/
/* ---------------------------------------------------------------------*/
/* Process a backspace character.  If the current character is a 	*/
/* backspace character then move the current position to the left by 1.	*/
/* If the character is already on the far right, then leave the 	*/
/* character at the beginning of line (far right position).	    	*/ 
			if ( rightmostpos < x ) rightmostpos=x;
			if ( --x < 0 ) x=0;

			/* Update startpos if x has been moved beyond it */
			/* to the left.					 */
			if ( x < startpos ) startpos = x;
		   } /* End of != '\b' else */
/* ---------------------------------------------------------------------*/
/*	End Code block to process backspace character			*/
/* ---------------------------------------------------------------------*/
	        } else {
/* ---------------------------------------------------------------------*/
/*	Code block to process carriage return character 		*/
/* ---------------------------------------------------------------------*/
/* Process a Carriage Return character 					*/
/*  If the current character is a Carriage return then the following 	*/
/*  steps are to be performed.   					*/
/*	(1)  Move the cursor to the beginning of the current line.	*/
/*	(2)  Set the beginning of the line to 0.  Since we know where	*/
/*	     the beginning should be.  For performance reasons, we 	*/
/*	     don't bother checking if it is already set, otherwise	*/
/*	     we'll spend additional cycles to test and branch 		*/
/*  	(3)  Set if the position of x is > than right most position then */
/*	     set the position to right accordingly.			*/
			startpos = x = 0;
			if ( rightmostpos < x ) rightmostpos = x;
		  } /* End of != '\r' else */
/* ---------------------------------------------------------------------*/
/*	End Code block to process carriage return character		*/
/* ---------------------------------------------------------------------*/
	     } else {
/* ---------------------------------------------------------------------*/
/*	Code block to process newline character 			*/
/* ---------------------------------------------------------------------*/
/*  If the current character is a new-line character   			*/
/*  then do the following steps:                       			*/
/*      (1)  Clear the to the end-of-line of the       			*/
/*           current line.			      			*/
/*      (2)  Move the cursor to the beginning of the   			*/
/*	    next line.				      			*/
/* Clear to end of line 						*/

		       win->_curx = x;
		       win->_cury = y;
		       wclrtoeol(win);
		       DO_WRAP;
		       DO_SCROLLING_CHECK;
		} /* End of != '\n' else */
/* ---------------------------------------------------------------------*/
/*	End Code block to process newline character			*/
/* ---------------------------------------------------------------------*/
	   } else {
/* ---------------------------------------------------------------------*/
/*	Code block to process tab character 				*/
/* ---------------------------------------------------------------------*/
/*  If the current character is a tab character then expand the tab to  */
/*  next tab stop (assumed that tab stops are 8-character positions 	*/
/*  apart.)	      							*/

		       register int newx;	/* Final position for x */
		       chtype	blank= SPACE;

		       blank |= win->_attrs; 	/* Set sttribute */

		       /* newx= current_position + 8-(current_position mod 8 */
		       newx = x+ (8 - (x & 07)); 

		       while ( x < newx ) 
		       {
			   /* Set character to blank then increment x value */
			   win->_y[y][x++] = blank ;  
		
			   /* If x is at the end of the current line  */
			   /* then break out of the loop and leave x  */
			   /*  at the last character of the current   */
			   /* Line.			     */
			   if ( win->_maxx <= x  ) {
			       x--; 
			       break;
			   } /* End of if */
		       } /* End of while */
	      }  /* End of != '\t' else */
/* ---------------------------------------------------------------------*/
/*	End Code block to process tab character				*/
/* ---------------------------------------------------------------------*/
/*	END OF CASCADED IFs						*/
/* ---------------------------------------------------------------------*/


	str++; /* Increment to next character in string */
	} /* End of While */

	/* The following (updating for overwritten two-byte and */
	/* updating first and last character positions only	*/
	/* needs to be done if any characters where actually    */
	/* written.  This can be determined by checking the 	*/
	/* win->_firstch[y] value.  If this value is still set  */
	/* to _NOCHANGE, then nothing on this line has been 	*/
	/* changed. 						*/
	
	if ( (startpos != x) || (rightmostpos != _NOCHANGE) )
	{

	   /* Now to check if we have overwritten the first of a two */
	   /* byte character.  Do this ONLY if this is a 16-bit 	  */
	   /* language 						  */

	   if ( !_CS_SBYTE && 	/* Not Single byte code scheme */
	     ( win->_y[y][x] & A_SECOF2 ) )
    	   {
	      /* Yes, there was a two-byte sequence at this location */
	      /* and we've already written over the first half of    */
	      /* the two byte sequence.  We now must overwrite the   */
	      /* current position with a space character. 	     */
	      /* We will also maintain the OLD attributes for this   */
	      /* character position rather than use the current      */
	      /* attributes.					     */
	      /*						     */
	      /* Note: When replacing the character in the current   */
	      /*       position with a blank we will not be moving   */
	      /*       the cursor, however, the win->_lastch field  */
	      /*       must still be set to the current position     */
	      /*       rather than the usual current positoin - 1    */

	      win->_y[y][x] = ( win->_y[y][x] & A_ATTRIBUTES ) | SPACE ;
	      if ( win->_lastch[y] < x ) win->_lastch[y]=x;
	   } /* End of Second_of_2 check */

	   /* Now to update the firstch and lastch fields 	*/
	   UPDATE_FIRST_LAST;
	   if ( win->_lastch[y] < rightmostpos ) win->_lastch[y]=rightmostpos;
	} /* End of if ( !startpos == x)) */

   /* Update the _curx and _cury fields. */
   win->_curx = x;
   win->_cury = y;

   return OK;
}
