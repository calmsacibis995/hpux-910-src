/**			elm.h				**/

/*
 *  @(#) $Revision: 70.1.1.1 $
 *
 *  (c) Copyright Hewlett-Packard Company, 1986, 1987, 1988, 1989
 *  (c) Copyright Yokogawa-Hewlett-Packard Ltd., 1988, 1989
 *
 *  Acknowledgment is made to Dave Taylor for his creation of
 *  the original version of this software.
 *
 *
 *  Main header file for ELM mail system.
 */

#include <stdio.h>
#include <fcntl.h>

#include "curses.h"
#include "defs.h"


#ifdef NLS
# include <nl_types.h>
#endif NLS


/******** global variables accessable by all pieces of the program *******/

int current = 0;		/* current message number  */
int header_page = 0;     	/* current header page     */
int last_header_page = -1;     	/* last header page        */
int message_count = 0;		/* max message number      */
int headers_per_page;		/* number of headers/page  */

#ifdef SIGWINCH
int resized = 0;		/* window resize occurred */
#endif

char VERSION[SHORT_SLEN];	/* version number          */
char infile[LONG_FILE_NAME];	/* name of current mailbox */
char hostname[SLEN];		/* name of machine we're on*/
char username[SLEN];		/* return address name!    */
char dflt_full_name[SLEN];	/* Full name in passwd     */
char full_username[SLEN];	/* Full username - gecos   */
char home[SLEN];		/* home directory of user  */
char folders[LONG_FILE_NAME];	/* folder home directory   */
char mailbox[LONG_FILE_NAME];	/* mailbox name if defined */
char editor[LONG_FILE_NAME];	/* editor for outgoing mail*/
char alternative_editor[LONG_FILE_NAME];/* alternative editor...   */
char visual_editor[LONG_FILE_NAME];/* editor name of $VISUAL  */
char printout[LONG_FILE_NAME];	/* how to print messages   */
char savefile[LONG_FILE_NAME];	/* name of file to save to */
char calendar_file[LONG_FILE_NAME];/* name of file for clndr  */
char prefixchars[SLEN];		/* prefix char(s) for msgs */
char shell[LONG_FILE_NAME];	/* current system shell    */
char pager[LONG_FILE_NAME];	/* what pager to use       */
char tmpdir[LONG_FILE_NAME];	/* default TMPDIR	   */
char batch_subject[SLEN];	/* subject buffer for batchmail */
char local_signature[LONG_FILE_NAME];/* local msg signature file     */
char remote_signature[LONG_FILE_NAME];/* remote msg signature file    */
char nameserver[LONG_FILE_NAME];/* the optional user nameserver */

#ifdef SIGWINCH
int  state_stack[SSTACK_SIZE];	/* elm states for SIGWINCH screen refresh */
#endif

char backspace,			/* the current backspace char */
     eof_char,			/* the current eof char       */
     escape_char = TILDE,	/* '~' or something else..    */
     kill_line;			/* the current kill-line char */

char up[SHORT], down[SHORT];	/* cursor control seq's    */
char revision_id[STRING];	/* keep the revision of code */
int  cursor_control = FALSE;	/* cursor control avail?   */
int  has_transmit = FALSE;	/* has transmit function   */
int  hp_device = FALSE;		/* has 2622 ESC sequence? */

char start_highlight[SHORT],
     end_highlight[SHORT];	/* stand out mode...       */

int  has_highlighting = FALSE;	/* highlighting available? */

#ifdef SIGWINCH
int  sstack_p;			/* points to top of state_stack */
#endif

char *weedlist[MAX_IN_WEEDLIST];
int  weedcount = 0;

int allow_forms = NO;		/* flag: are AT&T Mail forms okay?  */
int file_changed = 0;		/* flag: true if infile changed     */
int mini_menu = 1;		/* flag: menu specified?	    */
int mbox_specified = 0;		/* flag: specified alternate mbox?  */
int check_first = 1;		/* flag: verify mail to be sent!    */
int check_size = 0;		/* flag: -z option is specified ?   */
int auto_copy = 0;		/* flag: automatically copy source? */
int filter = 1;			/* flag: weed out header lines?	    */
int resolve_mode = 1;		/* flag: delete saved mail?	    */
int auto_cc = 0;		/* flag: mail copy to user?	    */
int noheader = 1;		/* flag: copy + header to file?     */
int title_messages = 1;		/* flag: title message display?     */
int forwarding = 0;		/* flag: are we forwarding the msg? */
int hp_terminal = 1;		/* flag: are we on HP term?	    */
int hp_softkeys = 1;		/* flag: are there softkeys?        */
int save_by_name = 1;		/* flag: save mail by login name?   */
int mail_only = 0;		/* flag: send mail then leave?      */
int check_only = 0;		/* flag: check aliases then leave?  */
int move_when_paged = 0;	/* flag: move when '+' or '-' used? */
int point_to_new = 1;		/* flag: start pointing at new msg? */
int bounceback = 0;		/* flag: bounce copy off remote?    */
int signature = 0;		/* flag: include $home/.signature?  */
int always_leave = 1;		/* flag: always leave msgs pending? */
int always_del = 1;		/* flag: always delete marked msgs? */
int arrow_cursor = 0;		/* flag: use "->" cursor regardless?*/
int debug = 0; 			/* flag: default is no debug!       */
int read_in_aliases = 0;	/* flag: have we read in aliases??  */
int warnings = 1;		/* flag: output connection warnings?*/
int user_level = 0;		/* flag: how good is the user?      */
int selected = 0;		/* flag: used for select stuff      */
int names_only = 0;		/* flag: display user names only?   */
int question_me = 1;		/* flag: ask questions as we leave? */
int keep_empty_files = 0;	/* flag: leave empty mailbox files? */
int clear_pages = 0;		/* flag: act like "page" (more -c)? */
int prompt_for_cc = 1;		/* flag: ask user for "cc:" value?  */
int prompt_for_bcc = 0;         /* flag: ask user for "bcc:" value? */
int expand_tabs = 0;		/* flag: expand tabs in message?    */
int skip_deleted = 0;		/* flag: skip deleted message?      */
int no_skip_deleted = 0;	/* flag: no skip deleted message?   */
int in_header_editor = NOTIN;	/* flag: in header editor?	    */
int no_to_header = 0;		/* flag: not empty to header?	    */

int sortby = RECEIVED_DATE;	/* how to sort incoming mail...     */

long timeout = 600L;		/* timeout (secs) on main prompt    */

int mailbox_defined = 0;	/** mailbox specified?    **/

/** set up some default values for a 'typical' terminal *snicker* **/

int LINES=23;			/** line number in screen      **/
int COLUMNS=80;			/** columns per page      **/

long size_of_pathfd;		/** size of pathfile, 0 if none **/

FILE *mailfile;			/* current mailbox file     */
FILE *debugfile;		/* file for debug output    */
FILE *pathfd;			/* path alias file          */
FILE *domainfd;			/* domain file		    */

long mailfile_size = 0L;	/* size of current mailfile */

int   max_headers;		/* number of headers allocated */

struct header_rec *header_table;

struct alias_rec user_hash_table[ MAX_UALIASES ];
struct alias_rec system_hash_table[ MAX_SALIASES ];

struct date_rec last_read_mail; /* last time we read mailbox  */

struct lsys_rec *talk_to_sys;   /* what machines do we talk to? */

struct addr_rec *alternative_addresses;	/* how else do we get mail? */

int system_files = 0;		/* do we have system aliases? */
int user_files = 0;		/* do we have user aliases?   */

int system_data;		/* fileno of system data file */
int user_data;			/* fileno of user data file   */

int userid;			/* uid for current user	      */
int groupid;			/* groupid for current user   */
int egroupid;			/* effective groupid 'mail'   */

int deleted_at_cancel;		/* deleted msg at change cmd  */
int saved_at_cancel;		/* saved msg at change cmd    */
int keep_in_incoming;		/* keep msg in incomingmbox   */


#ifdef PREFER_UUCP
char BOGUS_INTERNET[SLEN];	/* to strip domain of host    */
#endif


#ifdef NLS
nl_catd nl_fd;			/* message catalogue file     */
char    lang[SLEN];		/* language name for NLS      */
char 	*catgets();
#endif NLS



/* These buffers are working area for some functions */

#define		MAX_RECURSION	20

char 	expanded_group[VERY_LONG_STRING];	/* expand_group()	*/
char	alias_address[VERY_LONG_STRING];	/* get_alias_address()	*/
char	bounce_address[LONG_STRING];		/* bounce_off_remote()	*/
char	escape_sequence[20];			/* return_value_of()	*/
char 	arpa_date[SLEN];			/* get_arpa_date()	*/
char	ctime_date[SLEN];			/* get_ctime_date()	*/
char	err_name_buf[50];			/* error_name()		*/
char	err_desc_buf[50];			/* error_description()	*/
char	formatted_buf[VERY_LONG_STRING];	/* fomat_long()		*/
char	stripped_buf[VERY_LONG_STRING];		/* strip_parens()	*/
char	tail_comp_buf[SLEN];			/* tail_of_string()	*/
char	lowered_buf[LONG_SLEN];			/* shift_lower()	*/
char	*token_buf[MAX_RECURSION];		/* get_token()		*/
char	last_cmp[SLEN];				/* get_last() 		*/
