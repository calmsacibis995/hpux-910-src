/**			sysdefs.h			**/

/*
 *  @(#) $Revision: 72.6 $
 *
 *  (c) Copyright Hewlett-Packard Company, 1986, 1987, 1988, 1989
 *  (c) Copyright Yokogawa-Hewlett-Packard Ltd., 1988, 1989
 *
 *  Acknowledgment is made to Dave Taylor for his creation of
 *  the original version of this software.
 *
 *
 *  System level, configurable, defines for the ELM mail system.
 */

/*
 *  Define the following if you think that the information in messages
 *  that have "Reply-To:" and/or "From:" fields with addresses will
 *  contain valid addressing information.  If this isn't defined, the
 *  calculated return address will ALWAYS be used instead.  (note that
 *  this doesn't necessarily preclude the use of G)roup replies).
 */

#define USE_EMBEDDED_ADDRESSES


#define FIND_DELTA	10		/* byte region where the binary search
					   on the path alias file is fruitless 
                                           (can't be within this boundary)    */

#define MAX_SALIASES	503	/* number of system aliases allowed      */
#define MAX_UALIASES	1001	/* number of user aliases allowed 	 */

#define MAX_IN_WEEDLIST 150	/* max headers to weed out               */

#define MAX_HOPS	35	/* max hops in return addr to E)veryone  */

#define MAX_ATTEMPTS	6	/* #times to attempt lock file creation */

/*
 *  see leavembox.c to determine if this should be defined or not....The 
 *  default is to NOT have it defined.
 */

/** #define REMOVE_AT_LAST **/

#define DEFAULT_BATCH_SUBJECT  "no subject (file transmission)"

/*
 *  If you want to have the mailer know about valid mailboxes on the
 *  host machine (assumes no delivery agent aliases) then you should
 *  undefine this (the default is to have it defined)...
 */

#define NOCHECK_VALIDNAME

/*
 *  If your machine doesn't have virtual memory (specifically the vfork() 
 *  command) then you should define the following....		
 */

/** #define NO_VM **/

/*
 *  If you're running sendmail, or another transport agent that can 
 *  handle the blind-carbon-copy list define the following
 */

#define ALLOW_BCC

/*
 *  If you'd like to be able to use the E)dit mailbox function from
 *  within the Elm program (many people find it too dangerous and
 *  confusing to include) then define the following:
 */

/** #define ALLOW_MAILBOX_EDITING **/

/** If you have pathalias, can we get to it as a DBM file??? **/

/** #define USE_DBM **/

/*
 *  If you want the mailer to check the pathalias database BEFORE it
 *  looks to see if a specified machine is in the L.sys database (in
 *  some cases routing is preferable to direct lines) then you should
 *  define the following...
 */

/** #define LOOK_CLOSE_AFTER_SEARCH **/


/*
 *  If you'd rather the program automatically used the 'uuname' command
 *  to figure out what machines it talks to (instead of trying to get
 *  it from L.sys first) then define the following...
 */

#define USE_UUNAME

/*
 *  If you want the program to try to optimize return addresses of
 *  aliases as you feed them to the make-alias function, then you
 *  should define this.
 */

#define DONT_TOUCH_ADDRESSES

/*
 *  If you'd like the system to try to optimize the return addresses of
 *  mail when you 'alias current message', then define:
 */

/** #define OPTIMIZE_RETURN **/

/*
 *  If you'd like "newmail" to automatically go into background when you
 *  start it up (instead of the "newmail &" junk with the process id output,
 *  then define the following...
 */

#define AUTO_BACKGROUND

/*
 *  If you'd rather your mail transport agent (ie sendmail) put the From:
 *  line into the message, define the following...
 *
 *  Undefining this on HP-UX systems causes a problem with mail from diskless
 *  nodes.  The 'From' address contains the diskless node name rather than
 *  the server name.  This makes it impossible to reply to these messages.
 *  Don't undefine this without fixing this problem.
 */

#define DONT_ADD_FROM

/*
 *  If your machine prefers the Internet notation of user@host for the
 *  From: line and addresses, define the following...(the default is to 
 *  use this rather than the USENET notation - check your pathalias file!)
 */

#define INTERNET_ADDRESS_FORMAT

/*
 *  If you're on a machine that prefers UUCP to Internet addresses, then
 *  define the following (the basic change is that on a machine that
 *  receives messages of the form <path>!user@<localhost> the displayed
 *  address will be <path>!user instead of user@<localhost>.
 *
 *  In this case, BOGUS_INTERNET will be defined.
 *  BOGUS_INTERNET is the address that your local system appends to 
 *  messages occasionally.  The algorithm is simply to REMOVE the
 *  BOGUS_INTERNET string. 'hostname' is a variable in  elm.h and 
 *  initialized in 'initialize.c.
 */

#define PREFER_UUCP

/*
 *  If you're running ACSNET and/or want to have your domain name
 *  attached to your hostname on outbound mail then you can define
 *  the following (default are not defined)
 */

/** #define USE_DOMAIN **/
/** #define DOMAIN		"<enter your domain here>" **/

/*
 *  If you are going to be running the mailer with setgid mail (or
 *  something similar) you'll need to define the following to ensure
 *  that the users mailbox in the spool directory has the correct
 *  group (NOT the users group)
 */

#define SAVE_GROUP_MAILBOX_ID

/*
 *  If you want a neat feature that enables scanning of the message
 *  body for entries to add to the users ".calendar" (or whatever)
 *  file, define this.
 */

#define ENABLE_CALENDAR
#define dflt_calendar_file	"calendar"	/* in HOME directory */

/*
 *  If you want to implement 'site hiding' in the mail, then you'll need to
 *  uncomment the following lines and set them to reasonable values.  See 
 *  the configuration guide for more details....(actually these are undoc-
 *  umented because they're fairly dangerous to use.  Just ignore 'em and
 *  perhaps one day you'll find out what they do, ok?)
 */

/****************************************************************************

#undef    DONT_ADD_FROM		

#define   SITE_HIDING
#define   HIDDEN_SITE_NAME	"fake-machine-name"
#define   HIDDEN_SITE_USERS	"/usr/mail/lists/hidden_site_users"

****************************************************************************/

/** Do we have the 'gethostname()' call?  If not, define the following **/
/** #define NEED_GETHOSTNAME **/

/** How about the 'cuserid()' call?  If not...well...you get the idea  **/
/** #define NEED_CUSERID **/

/** are you stuck on a machine that has short names?  If so, define the
    following **/

/** #define SHORTNAMES **/

/******************************************************************************

  The next set of #defines are to include or exclude major blocks of the
  Elm code, and the functionality that goes with it.  These are not presented
  as options during the configuration section due to not wanting to overwhelm
  the administrator with options, instead each is defined here...

  1.  How about encryption?  Do you want to support that?

      #define ENCRYPTION_SUPPORTED 

  2.  Would you like users to be able to use Forms Mode?

      #define FORMS_MODE_SUPPORTED

  3.  Since softkeys are specific to HP terminals (alas) you might
      not want that support if you have no HPs around...

      #define HP_SOFTKEYS_SUPPORTED

  4.  Bounceback is a useful feature, but if you're in any sort of
      legitimate system you shouldn't even waste the space having it
      in the code...

      #define BOUNCEBACK_ENABLED

  5.  if you'd like to have users have "ELM_NAMESERVER" as an environment
      variable, or in their elmrc files, then you'll need to include the
      following define:

      #define ENABLE_NAMESERVER

******************************************************************************/

#define FORMS_MODE_SUPPORTED
#define HP_SOFTKEYS_SUPPORTED

/* 
 *  If you want to get NLS support for help file or some messages in elm,
 *  then define NLS.
 */

#ifndef NLS
# define NLS
#endif NLS

/*
 *  If you want to make elm speak native language, you'll need
 *  to uncomment the following line.
 */

#define system_text_file	"/usr/mail/.elm/aliases.text"
#define system_hash_file	"/usr/mail/.elm/aliases.hash"
#define system_data_file	"/usr/mail/.elm/aliases.data"

#define ALIAS_TEXT		".elm/aliases.text"
#define ALIAS_HASH		".elm/aliases.hash"
#define ALIAS_DATA		".elm/aliases.data"

#define pathfile		"/usr/lib/mail/paths"
#define domains			"/usr/lib/domains"

#define Lsys			"/usr/lib/uucp/L.sys"

/** where to put the output of the elm -d command(internal support)... (in home dir) **/

#define DEBUGFILE	"ELM:debug.info"
#define OLDEBUG		"ELM:debug.last"

#define temp_file	"snd."
#define temp_head_file	"sndh."
#define temp_form_file	"form."
#define temp_mbox	"mbox."
#define temp_print      "print."
#define temp_alias	"alias."
#define temp_edit	"elm-edit"
#define temp_uuname	"uuname."
#define mailtime_file	".elm/last_read_mail"
#define readmail_file	".elm/readmail"

#define emacs_editor	"/usr/local/bin/emacs"

#define expand_cmd	"/usr/bin/expand"
#define default_editor	"/usr/bin/vi"
#define mailhome	"/usr/mail/"

#define default_shell	"/bin/sh"
#define default_pager	"builtin"
#define default_tmpdir	"/tmp"

#define sendmail	"/usr/lib/sendmail"
#define smflags		"-oi -oem"	/* ignore dots and mail back errors */
#define mailer		"/bin/rmail"
#define mailx		"/usr/bin/mailx"

#define helphome	"/usr/lib/elm"

#ifdef  NLS
# define nl_helphome	"/usr/lib/nls"
#endif  NLS

#define helpfile	"elm-help"

#define ELMRC_INFO	"/usr/lib/elm/elmrc-info"

#define elmrcfile	".elm/elmrc"
#define old_elmrcfile	".elm/elmrc.old"
#define mailheaders	".elm/elmheaders"
#define dead_letter	"Cancelled.mail"

#define unedited_mail	"emergency.mbox"

#define newalias_cmd	"/usr/bin/elmalias"
#define newalias	"/usr/bin/elmalias  1>/dev/null 2>&1"
#define readmail	"/usr/bin/readmail"

#define remove		"/bin/rm -f"		/* how to remove a file */
#define cat		"/bin/cat"		/* how to display files */
#define uuname		"uuname"		/* how to get a uuname  */
