/**			defs.h				**/

/*
 *  @(#) $Revision: 72.3 $
 *
 *  (c) Copyright Hewlett-Packard Company, 1986, 1987, 1988, 1989
 *  (c) Copyright Yokogawa-Hewlett-Packard Ltd., 1988, 1989
 *
 *  Acknowledgment is made to Dave Taylor for his creation of
 *  the original version of this software.
 *
 *
 *  define file for ELM mail system.
 */

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <strings.h>
#include <ctype.h>
#include <errno.h>

#include "sysdefs.h"	/* system/configurable defines */

#define KLICK		10

#define SLEN		256	    /* long for ensuring no overwrites... */
#define SHORT		5	    /* super short strings!		  */
#define NLEN		20	    /* name length for aliases            */
#define SHORT_SLEN      40
#define STRING		512	/* reasonable string length for most..      */
#define LONG_SLEN	250	/* for mail addresses from remote machines! */
#define LONG_STRING	500	/* even longer string for group expansion   */
#define LONG_FILE_NAME	1024	/* for long filename support		*/
#define VERY_LONG_STRING 2500	/* huge string for group alias expansion    */
#define MAX_LINE_LEN	5120	/* even bigger string for "filter" prog..   */

#ifdef SIGWINCH
#define	SSTACK_SIZE	10	/* maximum number of nested elm states */
#endif

#define BREAK		'\0'  		/* default interrupt    */
#define BACKSPACE	'\b'     	/* backspace character  */
#define TAB		'\t'            /* tab character        */
#define RETURN		'\r'     	/* carriage return char */
#define LINE_FEED	'\n'     	/* line feed character  */
#define FORMFEED	'\f'     	/* form feed (^L) char  */
#define COMMA		','		/* comma character      */
#define SPACE		' '		/* space character      */
#define DOT		'.'		/* period/dot character */
#define BANG		'!'		/* exclaimation mark!   */
#define AT_SIGN		'@'		/* at-sign character    */
#define PERCENT		'%'		/* percent sign char.   */
#define COLON		':'		/* the colon ..		*/
#define BACKQUOTE	'`'		/* backquote character  */
#define CIRCUMFLEX	'^'		/* for alternatives lst */
#ifdef TILDE
# undef TILDE
#endif
#define TILDE		'~'		/* escape character~    */
#define ESCAPE		'\033'		/* the escape		*/
#define DEL_KEY		'\077'		/* ascii DEL key	*/

#define NO_OP_COMMAND	'\0'		/* no-op for timeouts   */

#define STANDARD_INPUT  0		/* file number of stdin */

#ifndef TRUE
#define TRUE		1
#define FALSE		0
#endif

#define NO		0
#define YES		1
#define MAYBE		2		/* a definite define, eh?  */
#define FORM		3		/*      <nevermind>        */
#define PREFORMATTED	4		/* forwarded form...       */

#define PAD		0		/* for printing name of    */
#define FULL		1		/*   the sort we're using  */

#define OUTGOING	0		/* defines for lock file   */
#define INCOMING	1		/* creation..see lock()    */

#define SH		0		/* defines for system_call */
#define USER_SHELL	1		/* to work correctly!      */
#define EX_CMD		2		/* execute only the command*/

#define EXECUTE_ACCESS	01		/* These five are 	   */
#define WRITE_ACCESS	02		/*    for the calls	   */
#define READ_ACCESS	04		/*       to access()       */
#define ACCESS_EXISTS	00		/*           <etc>         */
#define EDIT_ACCESS	06		/*  (this is r+w access)   */

#define NOTIN		0		/* Not in header editor	   */
#define TH		1		/* in To: header	   */
#define BH		2		/* in Bcc: header	   */
#define CH		3		/* in Cc: header           */
#define JH		4		/* in Subject: header 	   */
#define AH		5		/* in Action: header	   */
#define IH		6		/* in In-reply-to: header  */
#define RH		7		/* in Reply-to: header	   */
#define PH		8		/* in Priority: header	   */
#define UH		9		/* in User_defined: header */

#define BIG_NUM		999999		/* big number!             */
#define BIGGER_NUM	9999999 	/* bigger number!          */

#define START_ENCODE	"[encode]"
#define END_ENCODE	"[clear]"

#define DONT_SAVE	"[no save]"
#define DONT_SAVE2	"[nosave]"

#define alias_file	".aliases"
#define group_file	".groups"
#define system_file	".systems"

#define SIZE_INDICATOR	"=size="	/* dummy alias for size checks  */
#define SIZE_COMMENT	"Table Size"	/*	and the comment too	*/

/** some defines for the 'userlevel' variable... **/

#define RANK_AMATEUR	0
#define AMATEUR		1
#define OKAY_AT_IT	2
#define GOOD_AT_IT	3
#define EXPERT		4
#define SUPER_AT_IT	5

/** some defines for the "status" field of the header record **/

#define ACTION		1		/* bit masks, of course */
#define CONFIDENTIAL	2
#define DELETED		4
#define EXPIRED		8
#define FORM_LETTER	16
#define NEW		32
#define PRIVATE		64
#define TAGGED		128
#define URGENT		256
#define VISIBLE		512

#define UNDELETE	0		/* purely for ^U function... */

/** some months... **/

#define JANUARY		0			/* months of the year */
#define FEBRUARY	1
#define MARCH		2
#define APRIL		3
#define MAY		4
#define JUNE		5
#define JULY		6
#define AUGUST		7
#define SEPTEMBER	8
#define OCTOBER		9
#define NOVEMBER	10
#define DECEMBER	11

#define equal(s,w)	(strcmp(s,w) == 0)
#define min(a,b)	a < b? a : b
#define ctrl(c)	        c - 'A' + 1	/* control character mapping */
#define lastch(s)	s[strlen(s)-1]
#define plural(n)	n == 1 ? "" : "s"

#define movement_command(c)	(c == 'j' || c == 'k' || c == ' ' || 	      \
				 c == BACKSPACE || c == ESCAPE || c == '*' || \
				 c == '-' || c == '+' || c == '=' ||          \
				 c == '#' || c == '@' || c == 'x' || 	      \
				 c == 'a' || c == 'q')

#define no_ret(s)	{ register int xyz; /* varname is for lint */	      \
		          for (xyz=strlen(s)-1; xyz >= 0 && 		      \
				(s[xyz] == '\r' || s[xyz] == '\n'); )	      \
			     s[xyz--] = '\0';                                 \
			}
			  
#define ClearLine(n)	MoveCursor(n,0); CleartoEOLN()
#define whitespace(c)	(c == ' ' || c == '\t')
#define ok_char(c)	(isalnum(c) || c == '-' || c == '_')
#define quote(c)	(c == '"' || c == '\'') 
#define onoff(n)	(n == 0 ? "OFF" : "ON")

/** The circumflex character is used in the alternatives list as shorthand
    for the regular expression '[, \t\r\n]' (e.g. "^taylor") and the following
    macro defines what characters it will match.
**/

#define circumflex_match_char(c)  (whitespace(c) || c == '\n' || c == '\r' || \
				   c == ',')

/** The next function is so certain commands can be processed from the showmsg
    routine without rewriting the main menu in between... **/

#define special(c)	(c == 'j' || c == 'k')

/** and a couple for dealing with status flags... **/

#define ison(n,mask)	(n & mask)
#define isoff(n,mask)	(~ison(n, mask))

#define setit(n,mask)		n |= mask
#define clearit(n, mask)	n &= ~mask

/** a few for the usage of function keys... **/

#define f1	1
#define f2	2
#define f3	3
#define f4	4
#define f5	5
#define f6	6
#define f7	7
#define f8	8

#define MAIN	0
#define ALIAS   1
#define YESNO	2
#define CHANGE  3
#define READ	4
#define CANCEL  5
#define SEND	6
#define CLEAR	7
#define SAVE	8
#define LIMIT	9

#define MAIN_HELP    0
#define ALIAS_HELP   1
#define OPTIONS_HELP 2

/** some possible sort styles... **/

#define REVERSE		-		/* for reverse sorting           */
#define SENT_DATE	1		/* the date message was sent     */
#define RECEIVED_DATE	2		/* the date message was received */
#define SENDER		3		/* the name/address of sender    */
#define SIZE		4		/* the # of lines of the message */
#define SUBJECT		5		/* the subject of the message    */
#define STATUS		6		/* the status (deleted, etc)     */
#define MAILBOX_ORDER	7		/* the order it is in the file   */

#ifdef SIGWINCH
/* actions to be taken in SIGWINCH signal handler */

#define WA_NOT	1			/* take no special action */
#define	WA_MHM	2			/* main help menu */
#define	WA_AHM	3			/* alias help menu */
#define	WA_OHM	4			/* options help menu */
#endif

/* some stuff for our own malloc call - pmalloc */

#define PMALLOC_THRESHOLD	256	/* if greater, then just use malloc */
#define PMALLOC_BUFFER_SIZE    2048	/* internal [memory] buffer size... */

# ifdef DEBUG
#  define   dprint(n,x)		{ 				\
				   if (debug >= n)  {		\
				     fprintf x ; 		\
				     fflush(debugfile);         \
				   }				\
				}
# else
#  define   dprint(n,x)
# endif

/* some random structs... */

struct date_rec {
	int  month;		/** this record stores a **/
	int  day;		/**   specific date and  **/
	int  year;		/**     time...		 **/
	int  hour;
	int  minute;
       };

struct header_rec {
	int  lines;		/** # of lines in the message  **/
	int  status;		/** Urgent, Deleted, Expired?  **/
	int  status2;		/** Read or Not?               **/
	int  index_number;	/** relative loc in file...    **/
	long offset;		/** offset in bytes of message **/
	struct date_rec received; /** when elm received here   **/
	char from[VERY_LONG_STRING];/** who sent the message?  **/
	char to[VERY_LONG_STRING];/** who it was sent to       **/
	char messageid[STRING];	/** the Message-ID: value      **/
	char dayname[8];	/**  when the                  **/
	char month[10];		/**        message             **/
	char day[3];		/**          was 	       **/
	char year[5];		/**            sent            **/
	char time[NLEN];	/**              to you!       **/
	char subject[STRING];   /** The subject of the mail    **/
       };

struct alias_rec {
	char   name[NLEN];	/* alias name 			     */
	long   byte;		/* offset into data file for address */
       };

struct lsys_rec {
	char   name[NLEN];	/* name of machine connected to      */
	struct lsys_rec *next;	/* linked list pointer to next       */
       };

struct addr_rec {
	 char   address[NLEN];	/* machine!user you get mail as      */
	 struct addr_rec *next;	/* linked list pointer to next       */
	};

#ifdef SHORTNAMES	/* map long names to shorter ones */
# include <shortnames.h>
#endif

/*
 *  Let's make sure that we're not going to have any annoying problems
 *  with int pointer sizes versus char pointer sizes by guaranteeing 
 *  that everything vital is predefined...
 */

#ifdef ELM
    /* addr_utils.c */

    /* alias.c */
    void a_help_menu(), o_help_menu(), mm_help_menu();

    /* aliasdb.c */
    char *find_path_to();

    /* aliaslib.c */
    char *get_alias_address();
    char *expand_system();
    char *expand_group();

    /* args.c */

    /* bounceback.c */
#   ifdef BOUNCEBACK_ENABLED
	char *bounce_off_remote();
#   endif

    /* builtin.c */
    /* calendar.c */
    /* checkname.c */
    /* connect_to.c */
    /* curses.c */

    /* date.c */
    char *get_arpa_date();
#   ifdef SITE_HIDING
	char *get_ctime_date();
#   endif

    /* del_alias.c */
    /* delete.c */

    /* domains.c */
    char *expand_domain();
    char *match_and_expand_domain();

    /* edit.c */
    /* editmsg.c */
    /* elm.c */
    /* encode.c */

    /* errno.c */
    char *error_description();
    char *error_name();

    /* expires.c */
    /* file.c */

    /* file_utils.c */
    long bytes();
    FILE *user_fopen();
    void force_final_newline();

    /* fileio.c */
    /* forms.c */
    /* getopt.c */
    /* hdrconfg.c */
    /* hdrconfg_b.c */
    /* help.c */
    /* hpux_rel.c */
    /* in_utils.c */

    /* initialize.c */
    char *expand_logname();

    /* leavembox.c */
    /* limit.c */
    /* mailmsg1.c */
    /* mailmsg2.c */
    /* mailtime.c */
    /* mkhdrs.c */
    /* msdos.c */
    /* newmbox.c */
    /* opt_utils.c */

    /* options.c */
    char *level_name();

    /* out_utils.c */
    /* pattern.c */

    /* pmalloc.c */
    void *my_malloc(), *my_realloc();

    /* quit.c */
    /* read_rc.c */
    /* remail.c */
    /* reply.c */
    /* returnaddr.c */
    /* save_opts.c */
    /* savecopy.c */
    /* screen.c */
    /* showmsg.c */
    /* showmsgcmd.c */

    /* signals.c */
    void setsize();

    /* softkeys.c */

    /* sort.c */
    char *sort_name();

    /* string2.c */

    /* strings.c */
    char *get_last();
    char *format_long();
    char *get_token();
    char *strip_commas();
    char *strip_parens();
    char *strip_tabs();
    char *shift_lower();
    char *tail_of_string();

    /* syscall.c */

    /* utils.c */
    void move_old_files_to_new();
    void show_mailfile_stats();
    void emergency_exit();
    void leave();
    void silently_exit();
    void leave_locked();
    char *nameof();
#   ifdef SIGWINCH
	void init_sstack(), push_state(), pop_state();
#   endif

    /* validname.c */

    /* version.c */
    void show_version_id();



#endif
