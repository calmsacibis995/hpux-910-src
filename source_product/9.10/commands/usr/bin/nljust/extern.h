/* @(#) $Revision: 64.2 $ */   

/*
**************************************************************************
** External Globals
**************************************************************************
*/

extern UCHAR *InBuf	;	/* points to current input string */
extern UCHAR *OutBuf	;	/* points to current output string */
extern UCHAR *WrapBuf	;	/* points to current wrap string */
extern UCHAR LastChar	;	/* last char in input buffer */

extern JUST Just	;	/* left or right justification */
extern END End		;	/* what to do at end of line */
extern LANG Lang	;	/* what right-to-left language */
extern TAB Tab		;	/* tab information */
extern FILE *Input	;	/* points to input stream */
extern nl_catd Catd	;	/* message catalog file descriptor */
extern nl_order Order	;	/* key or screen order */
extern nl_mode Mode	;	/* latin or non-latin mode */

extern int Mask		;	/* opposite language character mask */
extern int AltSpace	;	/* alternative space character */
extern int Escape	;	/* flags if command line primary font esc seq */
extern int HaveWrap	;	/* flags if there's still stuff to wrap */
extern int NewLine	;	/* flags if lines end with a new-line */
extern int LBlanks	;	/* flags replacement of leading blanks with alt space */
extern int JustFile	;	/* flags if lines will by justified */
extern int Enhanced	;	/* flags if enhanced printer shapes are used */
extern int Width	;	/* printer width (chars on the print line) */
extern int Margin	;	/* wrap margin */
extern int Len		;	/* num of char's in input buffer */

extern char *Cmd	;	/* command name */
extern char **Filename  ;       /* input file names */
extern char *Primary	;	/* primary char set */
extern char *Secondary	;	/* secondary char set */

extern char ARAB_PRI[]	;	/* arabic char set as primary */
extern char HEBR_PRI[]	;	/* hebrew char set as primary */
extern char ROMN_PRI[]	;	/* roman8 char set as primary */
extern char ASCI_SEC[]	;	/* ascii char set as secondary */

extern char DEFAULT[]	;	/* default language */
