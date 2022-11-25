/**  			curses.h			**/

/*
 *  @(#) $Revision: 64.2 $
 *
 *  (c) Copyright Hewlett-Packard Company, 1986, 1987, 1988, 1989
 *  (c) Copyright Yokogawa-Hewlett-Packard Ltd., 1988, 1989
 *
 *  Acknowledgment is made to Dave Taylor for his creation of
 *  the original version of this software.
 *
 *
 *  Include file for seperate compilation.
 */


#define OFF		0
#define ON 		1


int  InitScreen(),      /* This must be called before anything else!! */

     ClearScreen(), 	 CleartoEOLN(),

     MoveCursor(),
     CursorUp(),         CursorDown(), 
     CursorLeft(),       CursorRight(), 

     StartInverse(),     EndInverse(),

#ifndef ELM
     StartBold(),        EndBold(), 
     StartUnderline(),   EndUnderline(), 
     StartHalfbright(),  EndHalfbright(),
#endif
	
     transmit_functions(),

     Raw(),		 ReadCh();             

char *return_value_of();
