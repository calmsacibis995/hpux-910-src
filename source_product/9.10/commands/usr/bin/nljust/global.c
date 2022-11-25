/* @(#) $Revision: 64.2 $ */   

/*
**************************************************************************
** Include Files
**************************************************************************
*/

#include <nl_types.h>
#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include "justify.h"

/*
**************************************************************************
** Font Escape Sequences
**************************************************************************
*/

char ARAB_PRI[] = "\033(8V";	/* arabic char set as primary */
char HEBR_PRI[] = "\033(8H";	/* hebrew char set as primary */
char ROMN_PRI[] = "\033(8U";	/* roman8 char set as primary */
char ASCI_SEC[] = "\033)8U";	/* ascii char set as secondary */

/*
**************************************************************************
** Default Language
**************************************************************************
*/

char DEFAULT[] = "C";

/*
**************************************************************************
** Globals: (all globals start with capital letter)
**************************************************************************
*/

UCHAR *InBuf	;		/* points to current input string */
UCHAR *OutBuf	;		/* points to current output string */
UCHAR *WrapBuf	;		/* points to current wrap string */
UCHAR LastChar	;		/* last char in the input buffer */

JUST Just	= RIGHT	;	/* left or right justification */
END End		= WRAP	;	/* what to do at end of line */
LANG Lang	= ARABIC;	/* what right-to-left language */
TAB Tab		= {8,'\011'};	/* tab information: stop & char */
FILE *Input	= stdin;	/* points to input stream */
nl_order Order	= NL_KEY;	/* key or screen order */
nl_mode Mode	= NL_NONLATIN;	/* latin or non-latin mode */
nl_catd Catd	;		/* message catalog file descriptor */

int Mask	= NL_MASK;	/* opposite language character mask */
int AltSpace	= NL_SPACE;	/* alternative space character */
int Escape	= FALSE	;	/* flags if command line primary font esc seq */
int HaveWrap	= FALSE	;	/* flags if there's still stuff to wrap */
int NewLine	= TRUE	;	/* flags if lines end with new-line */
int LBlanks	= FALSE	;	/* flags replacement of leading blanks with alt space */
int JustFile	= TRUE	;	/* flags if lines will by justified */
int Enhanced	= FALSE	;	/* flags if enhanced printer shapes are used */
int Width	= MAX_WIDTH;	/* printer width (max char's on the print line) */
int Margin	= MAX_WIDTH;	/* wrap margin */
int Len		;		/* num of char's in the input buffer */

char *Cmd	;		/* command name */
char **Filename	;		/* input file names */
char *Primary	;		/* primary char set */
char *Secondary	= ASCI_SEC;	/* secondary char set */
