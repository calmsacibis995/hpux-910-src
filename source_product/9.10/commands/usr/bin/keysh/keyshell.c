/*
static char rcsId[] =
  "@(#) $Header: keyshell.c,v 70.1 94/09/29 16:04:18 hmgr Exp $";
static char copyright[] =
  "Copyright (c) HP, 1989";
*/

/********************************* keyshell.c ********************************/
/*                                                                           */
/* This file contains the main keysh entry points from ksh.  In particular,  */
/* ksh calls here whenever it wants to read a line from the terminal.  This  */
/* causes the following (hierarchical) keysh modules to activate:            */
/*                                                                           */
/* [main read(2) routine,  ]       keyshell.c       (global data)            */
/* [environment control,   ]           |               .                     */
/* [tty control            ]           |               .                     */
/* [(return command-line)  ]           |               .                     */
/*                                     |               .                     */
/*                                     |               .                     */
/* [translate softkeys to  ]       translate.y      global.c                 */
/* [HP-UX commands,        ]          / \                                    */
/* [kick off intrinsic     ]         /   \                                   */
/* [commands               ]        /     \                                  */
/* [(return command-line)  ]       /       \                                 */
/*                                /         \                                */
/*                               /           \                               */
/* [THE HEART OF KEYSH.    ] extents.c intrinsics.c [perform all intrinsic ] */
/* [create extent list,    ]    |            |      [kc functions,         ] */
/* [softkey navigation,    ]    |            |      [maintain .keyshrc file] */
/* [command verification   ]    |            |                               */
/* [(return extent list)   ]    |            |                               */
/*                              |            |                               */
/*                              |            |                               */
/* [identify and parse     ] word.c        config.c [read and parse        ] */
/* [special ksh words      ]    |                   [softkey files         ] */
/* [(return virtual words) ]    |                   [(return softkey trees)] */
/*                              |                                            */
/*                              |                                            */
/* [maintain command-line, ] edit.c . . . . . . . . (edit data)              */
/* [function key expansion,]    |                      .                     */
/* [(return virtual chars) ]    |                     .                      */
/*                              |                    .                       */
/*                              |                   .                        */
/* [convert vi/emacs chars ] vi.c/emacs.c          .                         */
/* [to edit commands       ]    |                 .                          */
/* [(return edit commands) ]    |                .                           */
/*                              |               .                            */
/*                              |              .                             */
/* [identify terminal keys ] keyboard.c       .                              */
/* [and accelerator keys,  ]     \           .                               */
/* [input redirection,     ]      \         .                                */
/* [help                   ]       \       .                                 */
/* [(return chars)         ]        \     .                                  */
/*                                   \   .                                   */
/*                                 display.c        [display command-line, ] */
/*                                                  [display status-line,  ] */
/*                                                  [display softkeys,     ] */
/*                                                  [format/display help   ] */
/*                                                                           */
/*                                                                           */
/* THEORY OF OPERATION                                                       */
/* ===================                                                       */
/*                                                                           */
/* The modules on the left branch above form an "input stream".  Input       */
/* characters from the terminal enter at the low-level keyboard module.      */
/* This module does some processing and then returns its "value added"       */
/* result to the module above it.  This happens at each module, in turn,     */
/* until finally, translated HP-UX commands (the "total value added"         */
/* result) emerge from the keyshell module -- ready for execution by ksh.    */
/*                                                                           */
/* Note that the low-level modules may awaken for *each* input character.    */
/* Modules above the edit module, on the other hand, only awaken when the    */
/* cursor moves.  And modules above the word module only awaken after a      */
/* full word (from ksh's perspective) has been parsed or no input            */
/* characters are pending.  This keeps the (more expensive) upper level      */
/* modules from doing unnecessary work.                                      */
/*                                                                           */
/* THE INPUT STREAM                                                          */
/* ================                                                          */
/*                                                                           */
/* Basically, the keyboard module reads characters from the terminal and     */
/* looks for terminfo(4) escape sequences from cursor keys, etc. (e.g.,      */
/* "<esc>C" on an HP terminal moves the cursor right) and translates them    */
/* into the most basic generic editing commands understood by the edit       */
/* module (e.g., keyboardRight).  Any unrecognized character sequences       */
/* are passed thru unchanged.                                                */
/*                                                                           */
/* Note that the keyboard module also has the ability to set up              */
/* "temporary input feeder" functions -- which allow other modules to        */
/* force-feed input characters, just as if they had been typed by the        */
/* user.                                                                     */
/*                                                                           */
/* If either vi or emacs-mode command-line editing has been enabled,         */
/* input characters will filter up thru the vi or emacs module,              */
/* respectively.  These modules simply look for "valid" character            */
/* sequences (e.g., "<esc>d" in emacs-mode deletes the word under the        */
/* cursor) and translates them into the corresponding generic editing        */
/* commands (e.g., keyboardDeleteWord).  Any unrecognized character          */
/* sequences (including any basic generic editing commands returned from     */
/* the keyboard module) are passed thru unchanged.                           */
/*                                                                           */
/* The edit module then receives a combination of characters and generic     */
/* editing commands (as translated by the lower-level modules).  Editing     */
/* commands are decomposed into sequences of the four basic edit             */
/* operations -- insert a character (without moving), delete a character     */
/* (again, without moving), move left one character, and move right one      */
/* character.  These operations are then performed.  Any resulting cursor    */
/* movements are passed on to the word module in terms of "add a             */
/* character" and "delete a character" primitives.                           */
/*                                                                           */
/* Note that from this point on up the hierarchy, modules are not aware      */
/* of any characters *beyond* the cursor position.  When the cursor moves    */
/* left, it is as if the user backspaced over those characters; when it      */
/* subsequently moves right again, it is as if the user *retyped* them.      */
/* This notion is *crucial* to the "incremental parsing" technique used      */
/* in the keysh extents module.  (Note that when the user presses            */
/* <Return> with the cursor in the middle of the line, the edit module       */
/* silently moves the cursor to the end of the line so that the              */
/* higher-level modules can process any remaining characters.)               */
/*                                                                           */
/* The word module receives these "add a character" and "delete a            */
/* character" primitives -- which describe actions occurring at the          */
/* effective end-of-line (i.e., where the cursor is).  When a character      */
/* is added, the word module checks to see whether it begins a new ksh       */
/* word, or continues the previous one; similarly, when a character is       */
/* deleted, the word module checks to see if it was the last remaining       */
/* character of the previous ksh word.  It then returns either "add a        */
/* word", "update a word", or "delete a word" primitives to the extents      */
/* module, as appropriate.  Note that for performance reasons, multiple      */
/* consecutive "update a word" primitives may be collapsed prior to being    */
/* passed to the (expensive) extents module.                                 */
/*                                                                           */
/* The extents module is the heart of keysh.  It creates an "extent list"    */
/* based on the "add a word", "update a word", and "delete a word"           */
/* primitives returned by the word module.  Each element in this list        */
/* corresponds to a ksh word on the command-line.  Cumulatively, the list    */
/* exactly represents the ksh words from the beginning of the                */
/* command-line up to the current cursor position.  Note that, by            */
/* definition, only the last element in this list can change.  The extents   */
/* module checks to see if this (last) element corresponds to one of the     */
/* currently visible softkeys.  If it does, the element is flagged as a      */
/* "softkey" and a new set of softkeys is computed and displayed;            */
/* otherwise, the softkeys are blanked, and the user is on his own.          */
/*                                                                           */
/* The extents module is also responsible for error detection in softkey     */
/* commands.  When the user presses <Return>, a "return request" command     */
/* is propagated up the input stream -- letting all modules know that a      */
/* <Return> *might* occur.  When the extents module finally intercepts       */
/* this request, it checks the extent list for errors (e.g., missing         */
/* softkeys, too many softkeys, etc.).  If any errors exist, the return      */
/* request is discarded, an error message is displayed, and a temporary      */
/* input feeder function is set up which backs up the cursor until the       */
/* error goes away -- thus positioning the cursor on the first character     */
/* of the first error in the command-line.  Otherwise, if no errors          */
/* exist, a "return verification" command is sent to the keyboard module     */
/* -- which then propagates up the input stream, letting all modules know    */
/* that a successful <Return> *has* occurred.                                */
/*                                                                           */
/* Once a successful <Return> has occurred, the translate module awakens     */
/* and walks the extent list -- with the intent of creating a parallel       */
/* "word list" containing the translated HP-UX command.  For each            */
/* "softkey" element in the extent list (as previously flagged by the        */
/* extents module), an awk-like "editrule" is executed.  This editrule can   */
/* manipulate the word list as necessary to create the proper HP-UX          */
/* command or option corresponding to the softkey.  Other elements in the    */
/* extent list (not flagged as softkeys) are copied to the word list         */
/* unchanged.                                                                */
/*                                                                           */
/* The translate module also has the responsibility of checking for          */
/* "intrinsic" softkey nodes (i.e., nodes of the "kc" built-in command)      */
/* while walking the extent list.  Whenever one of these nodes is            */
/* encountered, the translate module calls on the intrinsics module to       */
/* perform the appropriate action.  (Note that the editrules for             */
/* intrinsic softkey nodes usually result in an empty word list.)            */
/*                                                                           */
/* When the translate module has finished walking the extent list,           */
/* performing editrules, and calling out for intrinsic nodes, it             */
/* concatenates the elements in the word list together and returns the       */
/* translated HP-UX command to the keyshell module -- ready for execution    */
/* by ksh.                                                                   */
/*                                                                           */
/* THE KEYSHELL STUB                                                         */
/* =================                                                         */
/*                                                                           */
/* When ksh first powers-up (just after reading the $ENV file, but before    */
/* accepting any user input), it calls the KeyshellInitialize() function.    */
/* This function is responsible for performing any local module              */
/* initialization and then calling the hierarchical chain of                 */
/* *Initialize() function for the lower-level modules in the input           */
/* stream.                                                                   */
/*                                                                           */
/* Then, whenever ksh wants to read a line from the user, it calls the       */
/* KeyshellRead() function.  This function performs global actions such      */
/* as setting the tty modes and ensuring that ksh's (real) environment is    */
/* updated from the pseudo-environment described by its data structures,     */
/* and then calls the TranslateRead() function to do the real work.          */
/*                                                                           */
/* The TranslateRead() function first lets all of the other keysh modules    */
/* know that a line is about to be read by calling the hierarchical chain    */
/* of *Begin() functions, similar to how the *Initialize() functions were    */
/* called earlier.  This allows each module to do any line-specific          */
/* initialization prior to actually reading characters.                      */
/*                                                                           */
/* Once this is done, the TranslateRead() initiates the input stream         */
/* processing (described above) by calling the hierarchical chain of         */
/* *Read() functions.  These functions will not return until a valid         */
/* command has been entered by the user.  Note that the lower-level          */
/* *Read() functions may be called multiple times before a higher-level      */
/* *Read function returns!                                                   */
/*                                                                           */
/* Finally, the TranslateRead() function calls the hierarchical chain of     */
/* *End() functions, to allow each module to do any line-specific clean-up   */
/* prior to returning to the ksh code.                                       */
/*                                                                           */
/* What this all boils down to is each module seeing a call pattern like:    */
/*                                                                           */
/*   *Initialize();                                                          */
/*   for (each line read by ksh) {                                           */
/*     *Begin();                                                             */
/*     for (as needed by the next higher-level module in the hierarchy) {    */
/*       *Read();                                                            */
/*     }                                                                     */
/*     *End();                                                               */
/*   }                                                                       */
/*                                                                           */
/* MAJOR DATA STRUCTURES                                                     */
/* =====================                                                     */
/*                                                                           */
/* Keysh's global data consists of two main structures -- the static         */
/* softkey tree and the dynamic extent list.  These structures are           */
/* organized as follows:                                                     */
/*                                                                           */
/* softkey tree:                                                             */
/*                           <globalSoftKeys>                                */
/*                           .      .      .                                 */
/*                     .            .            .                           */
/*               .                  .                  .                     */
/*         cd                       ls                       rm              */
/*          .                    .  .  .                     ..              */
/*          .                  .    .    .                 .    .            */
/*          .                .      .      .             .        .          */
/*        <dir>           long    sorted   <file>      recurs-    <file>*    */
/*                        format    ..                 ively                 */
/*                                 .  .                                      */
/*                                .    .                                     */
/*                            oldest- newest-*                               */
/*                            newest  oldest                                 */
/*                                                         (* = required)    */
/*                                                                           */
/* extent list:                                                              */
/*                                                                           */
/*   <globalExtents> .. "ls" .. "sorted" .. "oldest-newest" .. "*.c" .. ""   */
/*                                                                           */
/*                                                                           */
/* Each node in the global softkey tree corresponds to a command or          */
/* option which the user can see and select on the softkeys (see             */
/* softkeys(4) for an overview of the softkey navigation mechanism).         */
/*                                                                           */
/* Each softkey node contains information such as whether or not the node    */
/* is "required" (i.e., if it is visible, the user *must* select it or       */
/* disable it before proceeding).  It also contains information such as,     */
/* if the node is selected, how many sibling nodes would be disabled --      */
/* and which node would then be the *next* required node seen by the         */
/* user.  All of this information is used by the extents module in           */
/* navigating the softkey hierarchy.                                         */
/*                                                                           */
/* The softkey tree is never modified during keysh command parsing.  It      */
/* is only modified on request from the user (i.e., thru a "kc softkey"      */
/* command) by the intrinsics module -- and only at the top-level of the     */
/* hierarchy.                                                                */
/*                                                                           */
/* Each link in the global extent list, on the other hand, corresponds to    */
/* a "word" (as defined by the word module) from the current                 */
/* command-line.  The list grows and shrinks as the user types -- it         */
/* represents everything from the beginning of the command-line up to the    */
/* current cursor position (not necessarily the end of the line).            */
/*                                                                           */
/* Each extent link contains information such as the word (from the          */
/* command-line) which it corresponds to, whether or not it was a softkey,   */
/* which softkeys were being displayed when the user started typing it,      */
/* and which softkeys should be displayed next.  This information is         */
/* also used by the extents module in navigating the softkey hierarchy.      */
/*                                                                           */
/*                                                                           */
/* CONVENTIONS                                                               */
/* ===========                                                               */
/*                                                                           */
/* The following conventions have been followed throughout the keysh code:   */
/*                                                                           */
/*   - the name of *all* exported functions, variables, types, etc. is       */
/*     prefixed with the name of the module which defines them (e.g.,        */
/*     DisplayHelp() is defined in the "display" module).                    */
/*                                                                           */
/*   - all words of a function or type name begin with a capital letter.     */
/*                                                                           */
/*   - the first word of a varable or enumeration name is lower case, all    */
/*     subsequent words begin with a capital letter.                         */
/*                                                                           */
/*   - the first word of a #define is all upper-case.  the second word       */
/*     is all lower case.  all subsequent words begin with a capital letter. */
/*                                                                           */
/*****************************************************************************/


#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/param.h>
#include <assert.h>
#include <locale.h>

#include "keyshell.h"
#include "global.h"
#include "translate.h"
#include "intrinsics.h"
#include "config.h"
#include "extents.h"
#include "word.h"
#include "edit.h"
#include "keyboard.h"
#include "display.h"
#include "debug.h"

#include "message.h"
#include "kshhooks.h"
#include "string2.h"

#define HP9000
#include "TSM/include/facetterm.h"


/********** keyshellEOF / keyshellInterrupt / keyshellSigwinch ***************/
/*                                                                           */
/* Both of these booleans are set to FALSE at the beginning of each          */
/* command-line.  If an EOF is encountered when reading from the terminal,   */
/* keyshellEOF is then set.  Similarly, if an interrupt is encountered,      */
/* keyshellInterrupt is then set.  keyshellSigwinch is set by the ksh        */
/* fault handler when a SIGWINCH occurs (by calling KeyshellSigwinch()).     */
/*                                                                           */
/*****************************************************************************/

int keyshellEOF;
int keyshellInterrupt;
int keyshellSigwinch;


/********************************** keyshellPSn ******************************/
/*                                                                           */
/* This integer is set by the KshEdSetup() routine to indicate the prompt    */
/* type just issued by ksh (i.e., what we are reading input for).  For a     */
/* $PS1, this integer is set to 1.  For a $PS2 or $PS3, it is set to 0.      */
/* We currently only do softkey processing for $PS1's.                       */
/*                                                                           */
/*****************************************************************************/

int keyshellPSn;


/****************************** keyshellInitialized **************************/
/*                                                                           */
/* This boolean is set to TRUE if terminfo initialization is successful.     */
/* It is set to FALSE otherwise.  Note that if this variable is set to       */
/* FALSE, keysh reverts to cannonical input processing (like /bin/sh).       */
/*                                                                           */
/*****************************************************************************/

int keyshellInitialized;


/************************* keyshellTsm / keyshellTsmSoftKeys *****************/
/*                                                                           */
/* keyshellTsm is set to TRUE if we are running under tsm.  It is set        */
/* to FALSE otherwise.  keyshellTsmSoftKeys is set to TRUE if keyshellTsm    */
/* is TRUE and the $KEYTSM environment variable is not set -- indicating     */
/* that tsm softkeys should be displayed instead of backup softkeys          */
/* where appropriate.                                                        */
/*                                                                           */
/*****************************************************************************/

int keyshellTsmSoftKeys;
int keyshellTsm;


/*****************************************************************************/
/*****************************************************************************/
/******************************** environment ********************************/
/*****************************************************************************/
/*****************************************************************************/


/******************* LENGTHnameValue / NUMBERnameValue ***********************/
/*                                                                           */
/* LENGTHnameValue is the maximum length of an environment variable which    */
/* we can transfer into keysh's environment space.  NUMBERnameValues is      */
/* the maximum number of environment variables which we can transfer into    */
/* keysh's environment space.                                                */
/*                                                                           */
/*****************************************************************************/

#define LENGTHnameValue    256
#define NUMBERnameValues   (sizeof(nameValues)/sizeof(nameValues[0]))


/******************************** Environment() ******************************/
/*                                                                           */
/* This routine copies selected shell variables from ksh's internal          */
/* data structures into a new argv-style array which will form keysh's       */
/* true environment space.  This new environment space is updated regularly  */
/* so that keysh's own environment will reflect what the user sees.          */
/*                                                                           */
/* Note that when a user logs is, ksh's environment normally only contains   */
/* 6 variables or so -- ksh then crafts each child process's environment     */
/* prior to each exec, rather than passing on its own.  Ksh's crippled       */
/* environment doesn't cause problems because it never                       */
/* calls libc routine which rely on true environment variables being set --  */
/* not so for keysh.                                                         */
/*                                                                           */
/*****************************************************************************/

static void Environment()
{
  int e;
  int n;
  char *s;
  char *equal;
  char *value;
  char *nameValue;
  static char nameValues[][LENGTHnameValue] = {
    "_=",
    "TZ=",
    "LANG=",
    "TERM=",
    "PATH=",
    "HOME=",
    "LINES=",
    "SHELL=",
    "COLUMNS=",
    "LOGNAME=",
    "TERMINFO="
  };
  static char *environment[NUMBERnameValues+1];
  extern char **environ;

  /* create a new argv-style array of name/value pairs in the form */
  /* "NAME=VALUE" and then make our global environ pointer point to it. */
  e = 0;
  for (n = 0; n < NUMBERnameValues; n++) {
    nameValue = nameValues[n];
    equal = strchr(nameValue, '=');
    *equal = '\0';
    value = KshGetEnv(nameValue);
    *equal = '=';
    if (value) {
      strncpy(equal+1, value, (size_t)LENGTHnameValue-(equal+1-nameValue)-1);
      environment[e++] = nameValue;
    }
  }
  environment[e++] = NULL;
  environ = environment;

  /* then make sure our nls routines have the locales set properly */
  if ((! (s = KshGetEnv("LC_TIME")) || ! s[0]) &&
      (! (s = KshGetEnv("LANG")) || ! s[0])) {
    s = "american";
  }
  setlocale(LC_TIME, s);
}


/*****************************************************************************/
/*****************************************************************************/
/****************************** external routines ****************************/
/*****************************************************************************/
/*****************************************************************************/


/****************************** KeyshellInitialize() *************************/
/*                                                                           */
/* This is a main ksh entry point.  It performs all module initializations   */
/* for keysh.                                                                */
/*                                                                           */
/*****************************************************************************/

void KeyshellInitialize()
{
  int fd;
  int len;
  struct stat st;
  extern uid_t geteuid();
  GlobalSoftKey *sk;
  static GlobalSoftKey empty;
  char tsmBuffer[FIOC_BUFFER_SIZE];

  /* check if tsm is running */
  strcpy(tsmBuffer, "hotkey");
  keyshellTsm = ioctl(0, FIOC_GET_INFORMATION, tsmBuffer) != -1 &&
                strcmp(tsmBuffer, "hotkey");
  keyshellTsmSoftKeys = keyshellTsm && ! KshGetEnv(KEYSHELLtsm);

  /* then update keysh's real environment to reflect changes made to */
  /* ksh's local data structures. */
  Environment();

  /* initialize the other keysh modules */
  MessageInitialize();
  TranslateInitialize();

  /* then read the global help topics file */
  sk = ConfigGetSoftKey(MESSAGEhelpFile, MESSAGEhelpSoftKey);
  if (sk) {
    globalHelpSoftKey = sk;
  }

  /* then force keysh's PS1, PS2, and PS3 variables to known states */
  /* so that we can perform relaible softkey processing. */
  if (! KshGetEnv(KEYSHELLprompt) && ! KshGetEnv(KEYSHELLksh)) {
    if (geteuid() == 0) {
      KshPutEnv("PS1=\n# ", TRUE, FALSE);
    } else {
      KshPutEnv("PS1=\n$ ", TRUE, FALSE);
    }
    KshPutEnv("PS2=> ", TRUE, FALSE);
    KshPutEnv("PS3=#? ", TRUE, FALSE);
  }
}


/****************************** KeyshellRead() *******************************/
/*                                                                           */
/* This is a main ksh entry point.  It reads a command-line just like        */
/* read(2) in cannonical mode.                                               */
/*                                                                           */
/*****************************************************************************/

int KeyshellRead(fd, buffer, length)
int fd;
char *buffer;
int length;
{
  int len;
  int ofd;

  assert(length+2 == EDITlength);    /* Modified for DSDe409947 */

  /* recompute the keyshellTsmSoftKeys value in case the user set/unset */
  /* $KEYTSM since the last time we were called */
  keyshellTsmSoftKeys = keyshellTsm && ! KshGetEnv(KEYSHELLtsm);

  /* then update keysh's real environment to reflect changes made to */
  /* ksh's local data structures. */
  Environment();

  /* set the user's edit mode selection from ksh's option variables */
  if (KshViMode()) {
    editMode = editVi;
  } else if (KshEmacsMode()) {
    editMode = editEmacs;
  } else {
    editMode = editRaw;
  }

  /* clear the global vars which indicate we've seen an EOF or an interrupt */
  keyshellEOF = 0;
  keyshellInterrupt = 0;
  keyshellSigwinch = 0;

  /* then do the normal edit mode processing common to the old ksh vi */
  /* and emacs modules */
  keyshellPSn = KshEdSetup(fd);
  ofd = KshOutputFd();

  /* if either we couldn't open terminfo or we can't place the tty into */
  /* raw mode, then just do a cannonical read and then return. */
  if (! keyshellInitialized || KshBuiltin() || KshTtyRaw() < 0) {
    write(ofd, kshPrompt, strlen(kshPrompt));
    return read(fd, buffer, (unsigned)length);
  }

  /* otherwise we want to read a real keysh input line -- keep processing */
  /* until we get a real line from the user (as opposed to an rc file). */
  do {
    len = TranslateRead(buffer);
  } while (globalNonInteractive);

  /* then restore the tty state */
  KshTtyCooked();

  /* and return the translated command-line to ksh for execution */
  if (keyshellEOF) {
    return 0;
  } else if (keyshellInterrupt) {
    return -1;
  }
  buffer[len] = '\n';
  return len+1;
}


/******************************* KeyshellSigwinch() **************************/
/*                                                                           */
/* This function handles SIGWINCH for keyshell.  It simply sets a flag       */
/* which will cause keyshell to refresh the display when the select(2)       */
/* statement in keyboard.c returns.                                          */
/*                                                                           */
/*****************************************************************************/

void KeyshellSigwinch()
{
  keyshellSigwinch = 1;
}


/*****************************************************************************/
