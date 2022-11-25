/**			headers.h			**/

/*
 *  @(#) $Revision: 70.1 $
 *
 *  (c) Copyright Hewlett-Packard Company, 1986, 1987, 1988, 1989
 *  (c) Copyright Yokogawa-Hewlett-Packard Ltd., 1988, 1989
 *
 *  Acknowledgment is made to Dave Taylor for his creation of
 *  the original version of this software.
 *
 *
 *  This is the header file for ELM mail system.
 */

#include <stdio.h>
#include <fcntl.h>

#include "curses.h"
#include "defs.h"


#ifdef NLS
# include <nl_types.h>
#endif NLS

/******** global variables accessable by all pieces of the program *******/

extern int current;			/* current message number  */
extern int header_page;         	/* current header page     */
extern int last_header_page;		/* last header page        */
extern int message_count;		/* max message number      */
extern int headers_per_page;		/* number of headers/page  */
extern char VERSION[SHORT_SLEN];	/* version number          */
extern char infile[LONG_FILE_NAME];	/* name of current mailbox */
extern char hostname[SLEN];		/* name of machine we're on*/
extern char username[SLEN];		/* return address name!    */
extern char dflt_full_name[SLEN];	/* Full name in passwd     */
extern char full_username[SLEN];	/* Full username - gecos   */
extern char home[SLEN];			/* home directory of user  */
extern char folders[LONG_FILE_NAME];	/* folder home directory   */
extern char mailbox[LONG_FILE_NAME];	/* mailbox name if defined */
extern char editor[LONG_FILE_NAME];	/* default editor for mail */
extern char alternative_editor[LONG_FILE_NAME];/* the 'other' editor */
extern char visual_editor[LONG_FILE_NAME];/* editor of $VISUAL       */
extern char printout[LONG_FILE_NAME];	/* how to print messages   */
extern char savefile[LONG_FILE_NAME];	/* name of file to save to */
extern char calendar_file[LONG_FILE_NAME];/* name of file for clndr  */
extern char prefixchars[SLEN];		/* prefix char(s) for msgs */
extern char shell[LONG_FILE_NAME];	/* default system shell    */
extern char pager[LONG_FILE_NAME];	/* what pager to use...    */
extern char tmpdir[LONG_FILE_NAME];	/* default TMPDIR */
extern char batch_subject[SLEN];	/* subject buffer for batchmail */
extern char local_signature[LONG_FILE_NAME];/* local msg signature file   */
extern char remote_signature[LONG_FILE_NAME];/* remote msg signature file */
extern char nameserver[LONG_FILE_NAME];	/* the optional username server */

extern char backspace,			/* the current backspace char  */
	    eof_char,			/* the current eof char        */
	    escape_char,		/* '~' or something else...    */
	    kill_line;			/* the current kill_line char  */

extern char up[SHORT], 
	    down[SHORT];		/* cursor control seq's    */
extern char revision_id[NLEN];		/* keep revision of code   */
extern int  cursor_control;		/* cursor control avail?   */
extern int  has_transmit;		/* has transmit function   */
extern int  hp_device;			/* has 2622 ESC sequence? */

extern char start_highlight[SHORT],
	    end_highlight[SHORT];	/* standout mode... */

extern int  has_highlighting;		/* highlighting available? */

/** the following two are for arbitrary weedout lists.. **/

extern char *weedlist[MAX_IN_WEEDLIST];
extern int  weedcount;		/* how many headers to check?        */

extern int  allow_forms;	/* flag: are AT&T Mail forms okay?    */
extern int  file_changed;	/* flag: true iff infile changed      */
extern int  mini_menu;		/* flag: display menu?     	      */
extern int  mbox_specified;     /* flag: specified alternate mailbox? */
extern int  check_first;	/* flag: verify mail to be sent!      */
extern int  check_size; 	/* flag: -z option is specified?      */
extern int  auto_copy;		/* flag: auto copy source into reply? */
extern int  filter;		/* flag: weed out header lines?	      */
extern int  resolve_mode;	/* flag: resolve before moving mode?  */
extern int  auto_cc;		/* flag: mail copy to yourself?       */
extern int  noheader;		/* flag: copy + header to file?       */
extern int  title_messages;	/* flag: title message display?       */
extern int  forwarding;		/* flag: are we forwarding the msg?   */
extern int  hp_terminal;	/* flag: are we on an hp terminal?    */
extern int  hp_softkeys;	/* flag: are there softkeys?          */
extern int  save_by_name;  	/* flag: save mail by login name?     */
extern int  mail_only;		/* flag: send mail then leave?        */
extern int  check_only;		/* flag: check aliases and leave?     */
extern int  move_when_paged;	/* flag: move when '+' or '-' used?   */
extern int  point_to_new;	/* flag: start pointing at new msgs?  */
extern int  bounceback;		/* flag: bounce copy off remote?      */
extern int  signature;		/* flag: include $home/.signature?    */
extern int  always_leave;	/* flag: always leave mail pending?   */
extern int  always_del;		/* flag: always delete marked msgs?   */
extern int  arrow_cursor;	/* flag: use "->" regardless?	      */
extern int  debug;		/* flag: debugging mode on?           */
extern int  read_in_aliases;	/* flag: have we read in aliases yet? */
extern int  warnings;		/* flag: output connection warnings?  */
extern int  user_level;		/* flag: how knowledgable is user?    */
extern int  selected;		/* flag: used for select stuff        */
extern int  names_only;		/* flag: display names but no addrs?  */
extern int  question_me;	/* flag: ask questions as we leave?   */
extern int  keep_empty_files;	/* flag: keep empty files??	      */
extern int  clear_pages;	/* flag: clear screen w/ builtin pgr? */
extern int  prompt_for_cc;	/* flag: prompt user for 'cc' value?  */
extern int  prompt_for_bcc;     /* flag: prompt user for 'bcc' value? */
extern int  expand_tabs;	/* flag: expand tabs in message?      */
extern int  skip_deleted;	/* flag: skip deleted messages?       */
extern int  no_skip_deleted;	/* flag: no skip deleted messages?    */

extern int  sortby;		/* how to sort mailboxes	      */

extern long timeout;		/* seconds for main level timeout     */

extern int mailbox_defined;	/** specified mailbox?  **/

extern int LINES;		/** lines per screen    **/
extern int COLUMNS;		/** columns per line    **/

extern long size_of_pathfd;	/** size of pathfile, 0 if none **/

extern FILE *mailfile;		/* current mailbox file     */
extern FILE *debugfile;		/* file for debut output    */
extern FILE *pathfd;		/* path alias file          */
extern FILE *domainfd;		/* domains file 	    */

extern long mailfile_size;	/* size of current mailfile */

extern int  max_headers;	/* number of headers currently allocated */

extern struct header_rec *header_table;

extern struct alias_rec user_hash_table  [MAX_UALIASES];
extern struct alias_rec system_hash_table[MAX_SALIASES];

extern struct date_rec last_read_mail;

extern struct lsys_rec *talk_to_sys;	/* who do we talk to? */

extern struct addr_rec *alternative_addresses;	/* how else do we get mail? */

extern int system_files;	/* do we have system aliases? */
extern int user_files;		/* do we have user aliases?   */

extern int system_data;		/* fileno of system data file */
extern int user_data;		/* fileno of user data file   */

extern uid_t userid;		/* uid for current user	      */
extern gid_t groupid;		/* groupid for current user   */
extern gid_t egroupid;		/* effective groupid 'mail'   */

extern int deleted_at_cancel;	/* deleted msg at change cmd  */
extern int saved_at_cancel;	/* saved msg at change cmd    */
extern int keep_in_incoming;	/* keep msg in incomingmbox   */


#ifdef PREFER_UUCP 
extern char BOGUS_INTERNET[SLEN];/* to strip domain of host    */
#endif


#ifdef NLS
extern nl_catd nl_fd;		/* message catalogue file     */
extern char    lang[SLEN];	/* language name for NLS      */
char	*catgets();
#endif NLS


/* These buffers are working area for some functions */

#define		MAX_RECURSION	20

extern char 	expanded_group[VERY_LONG_STRING];	/* expand_group()	*/
extern char	alias_address[VERY_LONG_STRING];	/* get_alias_address()	*/
extern char	bounce_address[LONG_STRING];		/* bounce_off_remote()	*/
extern char	escape_sequence[20];			/* return_value_of()	*/
extern char 	arpa_date[SLEN];			/* get_arpa_date()	*/
extern char	ctime_date[SLEN];			/* get_ctime_date()	*/
extern char	err_name_buf[50];			/* error_name()		*/
extern char	err_desc_buf[50];			/* error_description()	*/
extern char	formatted_buf[VERY_LONG_STRING];	/* fomat_long()		*/
extern char	stripped_buf[VERY_LONG_STRING];		/* strip_parens()	*/
extern char	tail_comp_buf[SLEN];			/* tail_of_string()	*/
extern char	lowered_buf[LONG_SLEN];			/* shift_lower()	*/
extern char	*token_buf[MAX_RECURSION];		/* get_token()		*/
extern char	last_cmp[SLEN];				/* get_last() 		*/
