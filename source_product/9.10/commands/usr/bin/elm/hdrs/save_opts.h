/**			 save_opts.h 			**/

/*
 *  @(#) $Revision: 64.3 $
 *
 *  (c) Copyright Hewlett-Packard Company, 1986, 1987, 1988, 1989
 *  (c) Copyright Yokogawa-Hewlett-Packard Ltd., 1988, 1989
 *
 *  Acknowledgment is made to Dave Taylor for his creation of
 *  the original version of this software.
 *
 *
 * Some crazy includes for the save-opts part of the Elm program!
 */

#define ALTERNATIVES		0
#define ALWAYSDELETE		1
#define ALWAYSLEAVE		2
#define ARROW			3
#define ASK			4
#define ASKBCC			5
#define ASKCC			6
#define AUTOCOPY		7
#define BOUNCEBACK		8
#define CALENDAR		9
#define COPY			10
#define EDITOR			11
#define EDITOUT			12
#define ESCAPE_CHAR		13
#define EXPAND			14
#define FORMS			15
#define FULLNAME		16
#define KEEP			17
#define KEYPAD			18
#define LOCALSIGNATURE		19
#define MAILBOX			20
#define MAILDIR			21
#define MENU			22
#define MOVEPAGE		23
#define NAMES			24
#define NOHEADER		25
#define PAGER			26
#define POINTNEW		27
#define PREFIX			28
#define PRINT			29
#define REMOTESIGNATURE		30
#define RESOLVE			31
#define SAVEMAIL		32
#define SAVENAME		33
#define SHELL			34
#define SIGNATURE		35
#define SKIPDELETED		36
#define SOFTKEYS		37
#define SORTBY			38
#define TIMEOUT			39
#define TITLES			40
#define USERLEVEL		41
#define WARNINGS		42
#define WEED			43
#define WEEDOUT			44

#define NUMBER_OF_SAVEABLE_OPTIONS	WEEDOUT+1

struct save_info_recs { 
	char 	name[NLEN]; 	/* name of instruction */
	long 	offset;		/* offset into elmrc-info file */
	} save_info[NUMBER_OF_SAVEABLE_OPTIONS] = 
{
/* 0 */   { "alternatives", -1L }, 
/* 1 */   { "alwaysdelete", -1L },  
/* 2 */   { "alwaysleave", -1L },  
/* 3 */   { "arrow", -1L},   
/* 4 */   { "ask", -1L },  
/* 5 */   { "askbcc", -1L },
/* 6 */   { "askcc", -1L },  
/* 7 */   { "autocopy", -1L },  
/* 8 */   { "bounceback", -1L },  
/* 9 */   { "calendar", -1L },  
/* 10 */  { "copy", -1L },  
/* 11 */  { "editor", -1L },  
/* 12 */  { "editout", -1L },  
/* 13 */  { "escape", -1L },  
/* 14 */  { "expand", -1L },
/* 15 */  { "forms", -1L },  
/* 16 */  { "fullname", -1L },  
/* 17 */  { "keep", -1L },  
/* 18 */  { "keypad", -1L },  
/* 19 */  { "localsignature", -1L },  
/* 20 */  { "mailbox", -1L },  
/* 21 */  { "maildir", -1L },  
/* 22 */  { "menu", -1L },  
/* 23 */  { "movepage", -1L },  
/* 24 */  { "names", -1L },  
/* 25 */  { "noheader", -1L },  
/* 26 */  { "pager", -1L },  
/* 27 */  { "pointnew", -1L},   
/* 28 */  { "prefix", -1L },  
/* 29 */  { "print", -1L },  
/* 30 */  { "remotesignature",-1L},    
/* 31 */  { "resolve", -1L },  
/* 32 */  { "savemail", -1L },  
/* 33 */  { "savename", -1L },  
/* 34 */  { "shell", -1L },  
/* 35 */  { "signature", -1L },  
/* 36 */  { "skipdeleted", -1L },
/* 37 */  { "softkeys", -1L },  
/* 38 */  { "sortby", -1L },  
/* 39 */  { "timeout", -1L },  
/* 40 */  { "titles", -1L },  
/* 41 */  { "userlevel", -1L },  
/* 42 */  { "warnings", -1L },  
/* 43 */  { "weed", -1L },  
/* 44 */  { "weedout", -1L },  
};
