/*
static char rcsId[] =
  "@(#) $Header: display.c,v 66.16 91/01/01 15:14:00 rpt Exp $";
static char copyright[] =
  "Copyright (c) HP, 1989";
*/

/******************************* display.c ***********************************/
/*                                                                           */
/* This module handles all terminal output.  It also handles all of the      */
/* help display, including forking the pager, formatting the nroff-style     */
/* help text, and piping the result.  All display output is optimized.       */
/*                                                                           */
/*****************************************************************************/


#define SINGLE
#define MINICURSES

#include <curses.h>
#include <termio.h>
#include <term.h>
#include <string.h>
#include <varargs.h>
#include <time.h>
#include <assert.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>

#include "keyshell.h"
#include "global.h"
#include "edit.h"
#include "vi.h"
#include "keyboard.h"
#include "display.h"
#include "debug.h"

#include "kshhooks.h"
#include "string2.h"
#include "buffer.h"
#include "quote.h"

#define HP9000
#include "TSM/include/facetterm.h"

extern char *tparm();


/********************** EDITline / STATUSline / SOFTKEYline ******************/
/*                                                                           */
/* These constants define line number (0 == top line of display) of the      */
/* command-line, status-line.  If simulated softkeys are being displayed,    */
/* They also define the line number of the upper-line of the softkey         */
/* labels.                                                                   */
/*                                                                           */
/*****************************************************************************/

#define EDITline        (lines-((softkeys==SIMULATE)?(4):(2))+(! statusOn))
#define STATUSline      (lines-((softkeys==SIMULATE)?(3):(1)))
#define SOFTKEYline     (lines-2)


/****************************** CENTER... ************************************/
/*                                                                           */
/* If simulated softkeys are being displayed, keysh will place status        */
/* information inbetween the two banks of four softkeys.  These constants    */
/* define the width of the status field as well as the strings to be         */
/* displayed.  These strings should really be part of message.c.             */
/*                                                                           */
/*****************************************************************************/

#define CENTER          5
#define CENTERblank     "     "
#define CENTERemacs     "emacs"
#define CENTERvi        " vi  "
#define CENTERraw       " raw "
#define CENTERoverwrite "ovwrt"
#define CENTERinsert    "insrt"
#define CENTERquote     "quote"
#define CENTERcommand   " cmd "


/************************** WINDOWwidth / WINDOWshift ************************/
/*                                                                           */
/* These constants define the width of the displayed edit-window into the    */
/* command-line.  They also define how much (left or right) the window       */
/* will be shifted when the user moves the cursor such that it would no      */
/* longer be displayed (e.g., typing off the end of the edit-window).        */
/*                                                                           */
/*****************************************************************************/

#define WINDOWwidth     (columns-kshPromptLength-1)
#define WINDOWshift     ((WINDOWwidth>60)?(30-2):(WINDOWwidth/2-2))


/******************************** ILLEGAL ************************************/
/*                                                                           */
/* This constant just defines an illegal value for a pointer to a softkey    */
/* node.  It is stored as the "last known softkey value" when we want to     */
/* force a screen update regardless of any potential optimizations (i.e.,    */
/* normally, if the new value for a softkey is the same as its last known    */
/* value, no screen update will occur).                                      */
/*                                                                           */
/*****************************************************************************/

#define ILLEGAL         ((GlobalSoftKey *)-1)


/********************************** TSMMETA **********************************/
/*                                                                           */
/* This string defines the sequence which we will prefix tsm-bound escape    */
/* sequences with to let it know that keysh is making the request.  In       */
/* particular, screen scrolling requests to tsm are handled this way so that */
/* the tsm screen image is not truncated on top or bottom.                   */
/*                                                                           */
/*****************************************************************************/

#define TSMMETA         "\033"


/***************************** prev / lprev / sprev **************************/
/*                                                                           */
/* These variables are used to index into the "save..." variables.  They     */
/* index the "remembered" states of the variables.  "lprev" indexes into     */
/* saveLine, "sprev" indexes into saveStatus, and "prev" indexes into        */
/* everything else (e.g., saveSoftKeys, saveCenters, and saveEtcetera(s)).   */
/*                                                                           */
/*****************************************************************************/

static int prev;
static int lprev;
static int sprev;


/********************************* save... ***********************************/
/*                                                                           */
/* These variables are used to remember the state of the keysh display.      */
/* When it comes time to update the display, if the new state is the same    */
/* as the "remembered" state, nothing actually happens.  Only when the       */
/* new state is different than the "remembered" state will we output any     */
/* characters to the terminal -- and then, we only output as many characters */
/* as necessary.  Note that when first starting out or when the user         */
/* requests a screen refresh (^L), we invalidate the "remembered" state --   */
/* causing the entire display to be updated.                                 */
/*                                                                           */
/* Note that these variables contain room for both the "remembered" state    */
/* and the current state of the display.  A simple index (prev/lprev) is     */
/* modified to reflect which state is which.  The basic algorithm for        */
/* updating the display is:                                                  */
/*                                                                           */
/*   saveSomething[1-prev] = new state;                                      */
/*   if (saveSomething[1-prev] != saveSomething[prev]) {                     */
/*     (* new state is different from "remembered" state *)                  */
/*     (* so update the display *)                                           */
/*   }                                                                       */
/*   prev = 1-prev;                                                          */
/*                                                                           */
/* In particular, these variables save the state of:                         */
/*                                                                           */
/*   saveLine       - the window being displayed into the command-line       */
/*   saveStatus     - the status-line                                        */
/*   saveSoftKeys   - the 8 displayed softkeys                               */
/*   saveEtcetera   - the first number in "--Etc-- n of n", if present       */
/*   saveEtceteras  - the second number in "--Etc-- n of n", if present      */
/*   saveCenters    - the center area between the simulated softkeys         */
/*                                                                           */
/*****************************************************************************/

static char saveLine[2][DISPLAYlength+1];
static char saveStatus[2][DISPLAYlength+1];
static GlobalSoftKey *saveSoftKeys[2][DISPLAYsoftKeyCount];
static int saveEtcetera[2];
static int saveEtceteras[2];
static char saveCenters[2][DISPLAYsoftKeyLines][CENTER+1];


/******************************* windowOffset ********************************/
/*                                                                           */
/* This variable is the index into the "editLine" of the first character     */
/* being displayed in the edit window.  As the user moves his cursor,        */
/* this variable is automatically updated to ensure that it is always        */
/* visible in the (sliding) edit window.                                     */
/*                                                                           */
/*****************************************************************************/

static int windowOffset;


/************************** rungBell / bellString ****************************/
/*                                                                           */
/* "rungBell" is a boolean which indicates that we have already rung         */
/* the bell on the terminal so we dont need to do it again.  It is reset     */
/* each time we physically write to the display.  "bellString" is the        */
/* escape sequence we use to ring the bell.                                  */
/*                                                                           */
/*****************************************************************************/

static int rungBell;
static char *bellString;


/************* statusFeeder / status / statusOn / hintVisible ****************/
/*                                                                           */
/* "statusFeeder" is a pointer to a function which is called periodically    */
/* and returns an up-to-date status-line string.  This string is typically   */
/* copied into the "status" buffer where it stays until needed by the        */
/* DisplayUpdate() function.  "statusOn" is non-zero if we have a real       */
/* status-line to display.                                                   */
/*                                                                           */
/* The "status" buffer is also used to display hint messages to the user.    */
/* While a hint is being displayed, the "hintVisible" boolean will be set    */
/* to TRUE, which inhibits the periodic updating the the "status" buffer     */
/* mentioned above.                                                          */
/*                                                                           */
/*****************************************************************************/

static char *(*statusFeeder)();
static char status[DISPLAYlength+1];
static int statusOn;
static int hintVisible;


/****************************** scroll / scrolled ****************************/
/*                                                                           */
/* "scrolled" is a boolean which indicates that the screen is currently      */
/* scrolled.  While this boolean is true, periodic status-line updates are   */
/* inhibited (sice we no longer know where the status-line is).  Also,       */
/* any DisplayClear() requests will only clear to the *end* of screen,       */
/* rather than clearing the entire screen like they normally do.             */
/*                                                                           */
/* "scroll" just indicates the total number of lines "back" which have       */
/* been scrolled -- it is never positive.                                    */
/*                                                                           */
/*****************************************************************************/

static int scroll;
static int scrolled;


/*************************** softkeys / tsmSoftKeys **************************/
/*                                                                           */
/* "softkeys" indicates the current type of softkeys being displayed.  It    */
/* can be either NONE, SIMULATE (for simulated softkeys), LABEL (for         */
/* real softkey labels on HP terminals), or INVIS (when we'd *like* to       */
/* simulate softkeys but the screen is too narrow).                          */
/*                                                                           */
/* "tsmSoftKeys" indicates the current status of the tsm softkeys.  It       */
/* can be either TSMOFF (meaning we *know* they are off), TSMON (meaning     */
/* we *know* they are on), or TSMMAYBE (meaning we don't know didly).        */
/*                                                                           */
/*****************************************************************************/

static int softkeys;

static int tsmSoftKeys;

#define TSMOFF    0
#define TSMON     1
#define TSMMAYBE  2

#define NONE      0
#define SIMULATE  1
#define LABEL     2
#define INVIS     3


/*****************************************************************************/
/*****************************************************************************/
/*****************************************************************************/
/*****************************************************************************/
/*****************************************************************************/


/****************************** cursor tracking ******************************/
/*                                                                           */
/* this module attempts to track the position of the cursor on the           */
/* terminal screen so as to avoid any unnecessary cursor positioning         */
/* escape sequences.  It does this at the lowest level of the display        */
/* output routines.  Specifically:                                           */
/*                                                                           */
/*    - GotoYX() checks to see if the requested cursor position is           */
/*      equal to the current cursor position.  If so, it does nothing.       */
/*      Otherwise, it outputs the appropriate escape sequence to             */
/*      actually move the cursor and updates the current cursor position.    */
/*                                                                           */
/*    - Putp() outputs a terminfo escape sequence.  If this is a SAFE        */
/*      escape sequence, the current cursor position is unchanged.           */
/*      Otherwise the current cursor position is invalidated so that         */
/*      the next call to GotoYX() will be forced to reposition the           */
/*      cursor.                                                              */
/*                                                                           */
/*    - WriteChar() outputs a character to the terminal.  If this            */
/*      character is not printable and is not part of a Putp() call,         */
/*      then the current cursor position is again invalidated (since         */
/*      we don't know what affect the character had on the terminal).        */
/*      Otherwise, the cursor column is simply incremented by one.           */
/*                                                                           */
/*****************************************************************************/


/******************** currentY / currentX / riskYX ***************************/
/*                                                                           */
/* "currentY" and "currentX" hold the current cursor position, or -1 if      */
/* the current cursor position is unknown.                                   */
/*                                                                           */
/* "riskYX" holds the risk factor (to the cursor position) associated with   */
/* the characters being output.  Normally it is UNKNOWN (meaning that if     */
/* the character is printable, it just moves the cursor one to the right;    */
/* otherwise, it invalidates the cursor position).  This variable can also   */
/* be set to HARMFUL (meaning that we know the current cursor position       */
/* will be changed), SAFE (meaning that we know the current cursor position  */
/* will not be changed), or COOKIE (meaning that we have to check            */
/* "magic_cookie_glitch" to see what will happen to the cursor).             */
/*                                                                           */
/*****************************************************************************/

#define UNKNOWN   0
#define HARMFULL  1
#define COOKIE    2
#define SAFE      3

static int currentY = -1;
static int currentX = -1;
static int riskYX = UNKNOWN;


/******************************* WriteChar() *********************************/
/*                                                                           */
/* This routine simply outputs a character to the buffered-i/o module        */
/* talking to stderr.  If the character is a simple printable character      */
/* then we increment the current cursor column; otherwise we invalidate      */
/* the current cursor position.                                              */
/*                                                                           */
/*****************************************************************************/

static void WriteChar(c)
int c;
{
  BufferWriteChar(c);
  if (riskYX == HARMFULL) {
    currentX = -1;
    currentY = -1;
  } else if (riskYX == UNKNOWN) {
    if (isprint(c) && currentX >= 0) {
      currentX++;
    } else {
      currentX = -1;
      currentY = -1;
    }
  }
}


/******************************* Putp() **************************************/
/*                                                                           */
/* This routine outputs the specified terminfo escape sequence.  It affects  */
/* the current cursor position based on the associated risk factor.          */
/*                                                                           */
/*****************************************************************************/

static void Putp(s, risk)
char *s;
{
  riskYX = risk;
  tputs(s, 1, WriteChar);
  riskYX = UNKNOWN;
  if (risk == COOKIE && magic_cookie_glitch > 0) {
    if (currentX >= 0) {
      currentX += magic_cookie_glitch;
    }
  }
}


/******************************* WriteString() *******************************/
/*                                                                           */
/* This routine displays the specified string.  If "hilite" is TRUE, the     */
/* string will be displayed in inverse video.                                */
/*                                                                           */
/*****************************************************************************/

static void WriteString(s, hilite)
char *s;
int hilite;
{
  int n;

  /* if we are hiliting to a terminal, then send it the appropriate */
  /* escape sequences to go into inverse video mode.  Repeat these */
  /* escape sequences if the terminal goes to a new line since most */
  /* terminals will automatically revert back to un-hilited mode. */
  if (hilite && isatty(KshOutputFd())) {
    Putp(enter_standout_mode, COOKIE);
    n = 0;
    while (*s) {
      if (n++ && ! (currentX % columns)) {
        Putp(enter_standout_mode, COOKIE);
      }
      WriteChar(*s);
      s++;
    }
    Putp(exit_standout_mode, COOKIE);

  /* otherwise, if we are hiliting to a pipe (e.g., sending help to a */
  /* pager), then just use overstrikes and let the pager wory about the */
  /* escape sequences.  If we are not hiliting at all, send the string */
  /* as-is. */
  } else {
    while (*s) {
      WriteChar(*s);
      if (hilite) {
        WriteChar('\b');
        WriteChar(*s);
      }
      s++;
    }
  }
}


/******************************* GotoYX() ************************************/
/*                                                                           */
/* This routine just moves the cursor to the specified position on the       */
/* screen.  It does nothing if our tracking mechanism says the cursor is     */
/* already there.  Note that we will try to optimize cursor movements        */
/* within the same line using the cursor_left, right, and column_address     */
/* capabilities.                                                             */
/*                                                                           */
/*****************************************************************************/

static void GotoYX(y, x)
int y;
int x;
{
  if (x != currentX || y != currentY) {
    if (y == currentY && currentX < columns) {
      if (x == currentX-1 && cursor_left) {
        tputs(cursor_left, 1, BufferWriteChar);
      } else if (x == currentX+1 && cursor_right) {
        tputs(cursor_right, 1, BufferWriteChar);
      } else if (column_address) {
        tputs(tparm(column_address, x), 1, BufferWriteChar);
      } else {
        goto dumb;
      }
    } else {
dumb:
      tputs(tparm(cursor_address, y, x), 1, BufferWriteChar);
    }
    currentX = x;
    currentY = y;
  }
}


/*****************************************************************************/
/*****************************************************************************/
/*****************************************************************************/
/*****************************************************************************/
/*****************************************************************************/


/********************************** helpInput ********************************/
/*                                                                           */
/* This variable points to the help text currently being displayed.  The     */
/* PeekHelp() and ReadHelp() routines access this text in the standard way   */
/* defined in quote.c.                                                       */
/*                                                                           */
/*****************************************************************************/

static char *helpInput;


/*************************** PeekHelp() / ReadHelp() *************************/
/*                                                                           */
/* PeekHelp() returns a preview of the next character to be returned by      */
/* the ReadHelp() routine.  ReadHelp() actually reads the character and      */
/* advances "helpInput".                                                     */
/*                                                                           */
/*****************************************************************************/

static int PeekHelp()
{
  return *helpInput;
}

static int ReadHelp()
{
  if (*helpInput) {
    return *helpInput++;
  }
  return '\0';
}


/******************************** DumbReadWord() *****************************/
/*                                                                           */
/* This routine reads the next white-space separated word from "helpInput"   */
/* using the PeekHelp() and ReadHelp() routines.  For unquoted words, this   */
/* is identical to QuoteReadWord(PeekHelp, ReadHelp).                        */
/*                                                                           */
/*****************************************************************************/

static char *DumbReadWord()
{
  char *bow;
  char *eow;

  if (! *helpInput) {
    return NULL;
  }
  (void)QuoteSkipSpace(PeekHelp, ReadHelp);
  bow = helpInput;
  if (! (eow = strpbrk(helpInput, " \n"))) {
    eow = StringEnd(helpInput);
  }
  helpInput = eow;
  return StringExtract(NULL, bow, eow);
}


/****************************** DumbReadLine() *******************************/
/*                                                                           */
/* This routine reads the next line of text verbatim from "helpInput".       */
/* It is used to read lines following a ".nf" (no fill) macro.               */
/*                                                                           */
/*****************************************************************************/

static char *DumbReadLine()
{
  char *bol;
  char *eol;

  bol = helpInput;
  if (eol = strchr(helpInput, '\n')) {
    helpInput = eol+1;
  } else {
    eol = StringEnd(helpInput);
    helpInput = eol;
  }
  return StringExtract(NULL, bol, eol);
}


/***************************** DumbEatLine() *********************************/
/*                                                                           */
/* This routine discards the help text up-to and including the next          */
/* new-line.  It is used to synchronize to the beginning of a line           */
/* following a ".nf" (no fill) macro.                                        */
/*                                                                           */
/*****************************************************************************/

static void DumbEatLine()
{
  while (*helpInput && *helpInput++ != '\n') {
    /* NULL */
  }
}


/****************************** DumbReadSpace() ******************************/
/*                                                                           */
/* This routine just returns the number of spaces following the word just    */
/* read by DumbReadWord().  This allows us the ability to preserve nroff     */
/* semantics of not distorting white-space *within* a single input line.     */
/*                                                                           */
/*****************************************************************************/

static int DumbReadSpace()
{
  int count;

  count = 0;
  while (*helpInput == ' ') {
    count++;
    helpInput++;
  }
  return count;
}


/*****************************************************************************/
/*****************************************************************************/
/*****************************************************************************/
/*****************************************************************************/
/*****************************************************************************/


/******************************* OpenPager() *********************************/
/*                                                                           */
/* This routine temporarily forks a pager to display keysh help.  Keysh's    */
/* stderr is attached thru a pipe to the pager's stdin.  The pager's stdout  */
/* then goes to the terminal.  Keysh can then display output as usual,       */
/* and everything will automatically be paged until ClosePager() is called.  */
/*                                                                           */
/*****************************************************************************/

static int OpenPager(pid, handler)
pid_t *pid;
void (**handler)();
{
  int ofd;
  int fds[2];
  char *arg1;
  char *arg2;
  char *pager;

  /* get a pipe to talk to the pager with. */
  if (pipe(fds) < 0) {
    return -1;
  }

  /* then save a reference to our stderr file so that we can restore */
  /* it later. */
  if ((ofd = dup(KshOutputFd())) < 0) {
    close(fds[0]);
    close(fds[1]);
    return -1;
  }

  /* then set the tty back to cooked mode so that the pager isn't */
  /* confused. */
  KshTtyCooked();

  /* and then fork a sub-process to run the pager */
  switch (*pid = fork()) {
    case -1 : close(fds[0]);
              close(fds[1]);
              close(ofd);
              KshTtyRaw();
              return -1;

    /* in the child, we move our pipe to stdin and then close any unneeded */
    /* file descriptors.  then we identify the pager referenced by the */
    /* $PAGER shell variable and exec it with (at most) two arguments. */
    /* if that fails, we just exec /usr/bin/more. */
    case  0 : (void)dup2(fds[0], 0);
              close(fds[0]);
              close(fds[1]);
              close(ofd);
              pager = KshGetEnv("PAGER");
              if (pager) {
                (pager = strtok(pager, " \t")) &&
                  (arg1 = strtok(NULL, " \t")) &&
                  (arg2 = strtok(NULL, " \t"));
                if (pager) {
                  (void)execlp(pager, pager, arg1, arg2, NULL);
                }
              }
              (void)execl("/usr/bin/more", "more", NULL);
              (void)execl("/bin/cat", "cat", NULL);
              perror("execl");
              exit(1);

    /* in the parent, we move our end of the pipe to stderr and then close */
    /* any unneeded file descriptors.  we then temporarily ignore any */
    /* SIGPIPEs. */
    default : (void)dup2(fds[1], KshOutputFd());
              close(fds[0]);
              close(fds[1]);
              *handler = signal(SIGPIPE, SIG_IGN);
              return ofd;
  }
}


/***************************** ClosePager() **********************************/
/*                                                                           */
/* After we have finished sending help to the pager, this routine will       */
/* restore ksh back to normal.                                               */
/*                                                                           */
/*****************************************************************************/

static void ClosePager(ofd, pid, handler)
int ofd;
pid_t pid;
void (*handler)();
{
  int exitStatus;

  /* if we had successfully opened a pager, close our end of the pipe */
  /* (so that the pager reads EOF) and restore our old stderr.  Then */
  /* restore the SIGPIPE signal handler and wait for the pager to die. */
  /* Then, tell ksh that *we* waited on its child -- so that it doesn't */
  /* try and do it again -- and then restore the tty back to raw mode */
  /* so we can continue input. */
  if (ofd >= 0) {
    (void)dup2(ofd, KshOutputFd());
    close(ofd);
    (void)signal(SIGPIPE, handler);
    while (waitpid(pid, &exitStatus, 0) < 0 && errno == EINTR) {
      /* NULL */
    }
    KshJobNotWaitSafe();
    KshTtyRaw();
  }
}


/*****************************************************************************/
/*****************************************************************************/
/*****************************************************************************/
/*****************************************************************************/
/*****************************************************************************/


/********************************* SoftColumnN() *****************************/
/*                                                                           */
/* This routine returns the screen column where the simulated label for      */
/* the specified softkey begins.  In the special case where n < 0, it        */
/* returns the column where the "center indicators" begin.  Note that        */
/* this routine automatically adjusts for magic cookies.                     */
/*                                                                           */
/*****************************************************************************/

static int SoftColumnN(n)
int n;
{
  int column;
  int adjust;

  adjust = 0;
  if (magic_cookie_glitch > 0) {
    adjust = 1;
  }
  if (n < 0) {
    return DISPLAYsoftKeyCount/2 * (DISPLAYsoftKeyLength+1) + adjust;
  }
  column = n * (DISPLAYsoftKeyLength+1);
  if (n >= DISPLAYsoftKeyCount/2) {
    column += CENTER+1;
  }
  return column;
}


/********************************* Center() **********************************/
/*                                                                           */
/* This routine returns the specified line of the "center indicators"        */
/* as based on the current editing mode.  These are only displayed when      */
/* simulating softkey labels on the bottom two lines of the screen.          */
/*                                                                           */
/*****************************************************************************/

static char *Center(skLine)
int skLine;
{
  if (skLine == 0) {
    if (editMode == editEmacs) {
      return CENTERemacs;
    } else if (editMode == editVi) {
      return CENTERvi;
    } else {
      return CENTERraw;
    }
  } else {
    if (editMode == editVi && viEscape) {
      return CENTERcommand;
    } else if (keyboardQuote) {
      return CENTERquote;
    } else if (editOverwrite) {
      return CENTERoverwrite;
    } else {
      return CENTERinsert;
    }
  }
}


/********************************* keys **************************************/
/*                                                                           */
/* This array contains pointers to (pointers to) the default function key    */
/* escape sequences.  If possible, we always program the defaults before     */
/* presenting the user with softkeys to choose from (in case some other      */
/* unfriendly application left them in a non-default state).  Note that      */
/* the keyboard module will only recognize function keys if they generate    */
/* these (default) escape sequences.                                         */
/*                                                                           */
/*****************************************************************************/

static char **keys[] = {
  &key_f1,
  &key_f2,
  &key_f3,
  &key_f4,
  &key_f5,
  &key_f6,
  &key_f7,
  &key_f8
};


/****************************** SoftKeyUpdate() ******************************/
/*                                                                           */
/* This routine updates the softkey labels and function key definitions.     */
/* If real (HP) softkey labels exist, they will be used.  Otherwise, labels  */
/* will be simulated using the bottom two lines of the screen.  Magic        */
/* cookies are handled correctly (well, at least the case where              */
/* abs(magic_cookie_glitch) <= 1...).                                        */
/*                                                                           */
/*****************************************************************************/

static void SoftKeyUpdate(displayEnd)
int displayEnd;
{
  int n;
  int new;
  int first;
  int length;
  int skLine;
  char *label;
  char *center;
  GlobalSoftKey *sk;
  GlobalSoftKey *psk;
  int changed[DISPLAYsoftKeyCount];
  char tsmBuffer[FIOC_BUFFER_SIZE];

  /* if the user doesn't want any softkeys at all, just return. */
  if (! softkeys) {
    return;
  }

  /* if tsm is running and we would otherwise be displaying the backup */
  /* softkeys *and* we are not simulating softkey labels, then let */
  /* tsm display its softkeys (instead of us displaying our backup */
  /* softkeys) and clear our knowledge of softkey state. */
  if (keyshellTsmSoftKeys && softkeys != SIMULATE && globalSoftKeyBackup) {
    for (n = 0; n < DISPLAYsoftKeyCount; n++) {
      saveSoftKeys[1-prev][n] = ILLEGAL;
    }
    if (tsmSoftKeys != TSMON) {
      tsmSoftKeys = TSMON;
      strcpy(tsmBuffer, "k+\r");
      (void)ioctl(0, FIOC_WINDOW_MODE, tsmBuffer);
    }
    return;
  }

  /* set this to TRUE if we have any changes to make to the softkeys */
  new = FALSE;

  /* then figure out which softkeys changed.  get the current key from */
  /* GlobalDisplayedSoftKeyN() and compare it to the "remembered" key */
  /* in saveSoftKeys[prev].  The only trick here is that when dealing */
  /* with the "--Etc--" softkey, we also have to check saveEtcetera[] */
  /* and saveEtceteras[] to see if they changed. */
  for (n = 0; n < DISPLAYsoftKeyCount; n++) {
    sk = GlobalDisplayedSoftKeyN(n);
    if (! sk) {
      sk = globalEmptySoftKey;
    }

    changed[n] = 0;
    saveSoftKeys[1-prev][n] = sk;
    saveEtcetera[1-prev] = globalTailExtent->etcetera;
    saveEtceteras[1-prev] = globalTailExtent->displayed->etceteras;
    psk = saveSoftKeys[prev][n];

    for (skLine = 0; skLine < DISPLAYsoftKeyLines; skLine++) {
      label = sk->label[skLine];
      strcpy(label, StringPad(label, DISPLAYsoftKeyLength));
      if (sk != psk) {
        changed[n] |= 1<<skLine;
        new = TRUE;
      } else if (sk->attrib.etcetera && skLine == 1 &&
                 (saveEtcetera[1-prev] != saveEtcetera[prev] ||
                  saveEtceteras[1-prev] != saveEtceteras[prev])) {
        changed[n] |= 1<<skLine;
        new = TRUE;
      }
    }
  }

  if (new) {
    /* if we have to make any changes to the softkeys, check to see if */
    /* we have the capability to reprogram them.  if so then program them */
    /* back to defaults (or to backup values).  note that if we had */
    /* already programmed them back to defaults, we don't need to do */
    /* it again. */
    if (pkey_key) {
      tsmSoftKeys = TSMMAYBE;
      for (n = 0; n < DISPLAYsoftKeyCount; n++) {
        if (changed[n]) {
          sk = saveSoftKeys[1-prev][n];
          psk = saveSoftKeys[prev][n];
          if (sk->attrib.backup && sk->literal[0]) {
            Putp(tparm(pkey_key, n+1, sk->literal), SAFE);
          } else if ((psk == ILLEGAL || psk->attrib.backup) && *keys[n]) {
            Putp(tparm(pkey_key, n+1, *keys[n]), SAFE);
          }
        }
      }

    /* otherwise if tsm is running and we're not using hardware softkey */
    /* labels, then let tsm program the function keys back to defaults */
    /* and make sure the labels (if any) are off. */
    } else if (softkeys != LABEL && keyshellTsm && tsmSoftKeys != TSMOFF) {
      tsmSoftKeys = TSMOFF;
      strcpy(tsmBuffer, "k-\r");
      (void)ioctl(0, FIOC_WINDOW_MODE, tsmBuffer);
    }
  }

  /* then, if we are using hardware softkey labels, simply reprogram */
  /* all of the ones which changed -- and then make sure the labels */
  /* are on. */
  if (softkeys == LABEL) {
    if (new) {
      for (n = 0; n < DISPLAYsoftKeyCount; n++) {
        if (changed[n]) {
          label = StringConcat(saveSoftKeys[1-prev][n]->label[0],
                               saveSoftKeys[1-prev][n]->label[1]);
          *StringEndSig(label) = '\0';
          if (! label[0]) {
            strcpy(label, " ");
          }
          Putp(tparm(plab_norm, n+1, label), SAFE);
        }
      }
      Putp(label_on, SAFE);
    }

  /* otherwise, if we are simulating labels, go thru the bottom two */
  /* lines of the screen and redisplay the labels of any softkeys which */
  /* have changed. */
  } else if (softkeys == SIMULATE && ! displayEnd) {
    if (new) {
      Putp(label_off, SAFE);
    }
    for (skLine = 0; skLine < DISPLAYsoftKeyLines; skLine++) {
      if (new) {
        for (n = 0; n < DISPLAYsoftKeyCount; n++) {
          if (changed[n] & 1<<skLine) {

            /* if we don't have magic cookies on this terminal, then we */
            /* can enter and exit standout mode before and after each */
            /* softkey label.  if we have the ceol_standout_glitch, we */
            /* only need to do this the first time we draw the labels -- */
            /* the attributes won't change on subsequent draws (until we */
            /* clear the screen). */
            if (magic_cookie_glitch <= 0) {
              if (n == 0 || n == DISPLAYsoftKeyCount/2) {
                GotoYX(SOFTKEYline+skLine, SoftColumnN(n));
              } else if (changed[n-1] & 1<<skLine) {
                DisplayPrintf(" ");
              } else {
                GotoYX(SOFTKEYline+skLine, SoftColumnN(n));
              }
              psk = saveSoftKeys[prev][n];
              if (psk == ILLEGAL || ! ceol_standout_glitch) {
                Putp(enter_standout_mode, COOKIE);
              }
              DisplayPrintf("%s", saveSoftKeys[1-prev][n]->label[skLine]);
              if (psk == ILLEGAL || ! ceol_standout_glitch) {
                Putp(exit_standout_mode, COOKIE);
              }

            /* if we do have magic cookies, we only enter standout mode at */
            /* the beginning of each bank of four softkeys -- and we use "|" */
            /* characters to separate the individual softkey labels. */
            } else {
              if (n == 0 || n == DISPLAYsoftKeyCount/2) {
                GotoYX(SOFTKEYline+skLine, SoftColumnN(n));
                Putp(enter_standout_mode, COOKIE);
              } else if (changed[n-1] & 1<<skLine) {
                DisplayPrintf("|");
              } else {
                GotoYX(SOFTKEYline+skLine, SoftColumnN(n));
                DisplayPrintf("|");
              }
              DisplayPrintf("%s", saveSoftKeys[1-prev][n]->label[skLine]);
              if (n == DISPLAYsoftKeyCount/2-1 || n == DISPLAYsoftKeyCount-1) {
                Putp(exit_standout_mode, COOKIE);
              }
            }
          }
        }
      }

      /* then update the center edit-mode indicators (both lines) */
      center = saveCenters[1-prev][skLine];
      strcpy(center, Center(skLine));
      first = StringUpdate(center, saveCenters[prev][skLine], &length);
      if (length) {
        GotoYX(SOFTKEYline+skLine, SoftColumnN(-1)+first);
        DisplayPrintf("%s", StringPad(center+first, length));
      }
    }
  }
}


/******************************** ViewLine() *********************************/
/*                                                                           */
/* This function takes the editLine and copies a portion of its chars        */
/* (corresponding to the screen window currently surrounding the cursor) to  */
/* a view window ("dest") -- making control characters visible as            */
/* appropriate.  It returns the index of the cursor in this view window.     */
/*                                                                           */
/* It uses a slow trial-and error method to find the appropriate window.     */
/*                                                                           */
/*****************************************************************************/

static int ViewLine(dest)
char *dest;
{
  char *source;
  char *point;
  char *cursor;

  /* while cursor position is left of the start of the view window, */
  /* move the view window left. */
  while (editIndex < windowOffset) {
    windowOffset -= WINDOWshift;
  }

  /* then, until we actually see the cursor in the view window, move */
  /* the view window right.  Note that the physical number of characters */
  /* which can be represented in the view window can vary from the window */
  /* length divided by two (in the case where they are all control */
  /* characters) up to the window length itself. */
  do {
    point = editLine+editIndex;
    source = editLine+windowOffset;
    cursor = StringVisual(dest, source, WINDOWwidth, point);
    dest[WINDOWwidth] = '\0';
    if (! cursor) {
      windowOffset += WINDOWshift;
    }
  } while (! cursor);

  /* then return the index of the cursor into the view window. */
  return cursor-dest;
}


/*****************************************************************************/
/*****************************************************************************/
/*****************************************************************************/
/*****************************************************************************/
/*****************************************************************************/


/******************************* DisplayInitialize() *************************/
/*                                                                           */
/* This routine initializes the display module.                              */
/*                                                                           */
/*****************************************************************************/

void DisplayInitialize()
{
  tsmSoftKeys = TSMMAYBE;

  (void)BufferWriteFd(KshOutputFd());
  Putp(clr_eos, SAFE);
}


/****************************** DisplayStatusFeeder() ************************/
/*                                                                           */
/* This routine specifies the function which should be called whenever       */
/* the display moduleneeds to update the status-line.  The function should   */
/* return a character pointer to the new status-line.                        */
/*                                                                           */
/*****************************************************************************/

void DisplayStatusFeeder(feeder)
char *(*feeder)();
{
  statusFeeder = feeder;
}


/******************************** DisplayBegin() *****************************/
/*                                                                           */
/* This routine performs the pre-line initialization for the display         */
/* module.  Note that no display output should ever occur while keysh is     */
/* not interactive (e.g., while reading the keyshrc file).                   */
/*                                                                           */
/*****************************************************************************/

void DisplayBegin()
{
  if (! globalInteractive) {
    return;
  }

  /* invalidate our current cursor position */
  currentX = -1;
  currentY = -1;
  riskYX = UNKNOWN;

  /* begin displaying the beginning of the command-line and no hints */
  windowOffset = 0;
  hintVisible = FALSE;

  /* check to see if the user wants us to display softkeys.  we can */
  /* only use hardwars softkey labels if they are 2 lines high.  we */
  /* can only simulate softkey labels if the screen is at least 80 */
  /* characters wide. */
  if (! globalOptions.enableInvisibles && ! globalOptions.enableVisibles &&
      ! globalOptions.enableBackups || KshGetEnv(KEYSHELLksh)) {
    softkeys = NONE;
  } else if (plab_norm && label_height == 2 && ! KshGetEnv(KEYSHELLsimulate)) {
    softkeys = LABEL;
  } else {
    softkeys = SIMULATE;
  }
  if (columns < DISPLAYminColumns) {
    softkeys = INVIS;
  }

  /* find out the string to be sent to the terminal to ring the bell */
  bellString = KshGetEnv(KEYSHELLbell);

  /* then check out if we should even display a status-line */
  statusOn = ! KshGetEnv(KEYSHELLksh);

  /* then draw the initial keysh command-line, status-line, etc. */
  DisplayResume();
}


/******************************* DislayResume() ******************************/
/*                                                                           */
/* This routine makes room on the bottom of the screen for the keysh         */
/* command-line, status-line, softkeys, etc. by scrolling up an empty area.  */
/* It then redisplays the keysh command-line, status-line, softkeys, etc.    */
/*                                                                           */
/*****************************************************************************/

void DisplayResume()
{
  int i;

  for (i = EDITline; i < lines-1+kshPromptLines; i++) {
    DisplayPrintf("\n");
  }

  DisplayRefresh();
}


/******************************** DisplayRefresh() ***************************/
/*                                                                           */
/* This routine redraws the keysh command-line, status-line, softkeys, etc.  */
/* on the bottom of the screen.  It does this by invalidating all of the     */
/* "remembered" state information (so that the next call to DisplauUpdate()  */
/* will redraw everything).  This routine also redraws the keysh prompt,     */
/* resets any necessary terminal modes, and updates the status-line.         */
/*                                                                           */
/*****************************************************************************/

void DisplayRefresh()
{
  int i;
  int skLine;

  prev = 0;
  lprev = 0;
  sprev = 0;
  saveLine[lprev][0] = '\0';
  saveStatus[sprev][0] = '\0';
  for (skLine = 0; skLine < DISPLAYsoftKeyLines; skLine++) {
    for (i = 0; i < DISPLAYsoftKeyCount; i++) {
      saveSoftKeys[prev][i] = ILLEGAL;
    }
    saveCenters[prev][skLine][0] = '\0';
  }
  saveEtcetera[prev] = 0;
  saveEtceteras[prev] = 0;

  GotoYX(EDITline-kshPromptLines, 0);
  DisplayPrintf("%s", kshPrompt);
  Putp(clr_eos, SAFE);

  if (! KshGetEnv(KEYSHELLlocal)) {
    Putp(keypad_xmit, SAFE);
  }
  Putp(exit_insert_mode, SAFE);

  DisplayStatus();
}


/****************************** DisplayUpdate() ******************************/
/*                                                                           */
/* This is the heart of the display module.  It is typically called by       */
/* the keyboard module whenever it is about to block.  This causes the       */
/* terminal display to be updated to reflect keysh's internal state.         */
/* Note that we never update the display while we are not interactive        */
/* (e.g., reading the keyshrc file) or while the display is scrolled.        */
/* Calling this routine with full == FALSE will cause *only* the             */
/* display of the command-line to be updated -- rather than the entire       */
/* keysh display.                                                            */
/*                                                                           */
/*****************************************************************************/

void DisplayUpdate(full)
int full;
{
  int i;
  int clr;
  int lost;
  int begin;
  int insert;
  int delete;
  char *oline;
  char *nline;
  int first;
  int length;
  int cursorIndex;

  if (! globalInteractive) {
    return;
  }

  if (! scrolled) {
    /* compute the new view window into the command-line */
    nline = saveLine[1-lprev];
    oline = saveLine[lprev];
    cursorIndex = ViewLine(nline);

    /* then try to update the command-line display.  first try a dumb */
    /* approach where we update the shortest range of changed chars -- */
    /* being smart about clearing to the eol if the capability exists. */
    /* then try to see how many character insertions/deletions it */
    /* would take to do the same thing.  if the insertions/deletions */
    /* are easier, then do them; otherwise revert to the dumb method. */
    first = StringUpdate(nline, oline, &length);
    if (length) {
      clr = FALSE;
      if (clr_eol && first+length > strlen(nline)) {
        length = strlen(nline+first);
        clr = TRUE;
      }
      if (clr_eol && enter_insert_mode && exit_insert_mode &&
          delete_character) {
        begin = StringDiff6(nline, oline, WINDOWshift+1, WINDOWwidth,
                            &insert, &delete);
        if (begin < 0 || delete+insert >= length) {
          goto dumb;
        }
        GotoYX(EDITline, kshPromptLength+begin);
        lost = delete-insert;
        while (delete > insert) {
          Putp(delete_character, SAFE);
          delete--;
        }
        if (insert > delete) {
          Putp(enter_insert_mode, SAFE);
          DisplayPrintf("%s", StringPad(nline+begin, insert-delete));
          Putp(exit_insert_mode, SAFE);
          begin += insert-delete;
        }
        if (delete > 0) {
          DisplayPrintf("%s", StringPad(nline+begin, delete));
        }
        if (strlen(nline) > WINDOWwidth-lost) {
          GotoYX(EDITline, kshPromptLength+WINDOWwidth-lost);
          DisplayPrintf("%s", StringPad(nline+WINDOWwidth-lost, lost));
          Putp(clr_eol, SAFE);
        }
      } else {
dumb:
        GotoYX(EDITline, kshPromptLength+first);
        DisplayPrintf("%s", StringPad(nline+first, length));
        if (clr) {
          Putp(clr_eol, SAFE);
        }
      }
    }
    lprev = 1-lprev;

    if (full) {
      /* update the softkey display */
      SoftKeyUpdate(FALSE);
      prev = 1-prev;

      /* then check and see if the status-line is on and has changed -- */
      /* if so, redraw just the changed characters. */
      if (statusOn) {
        strcpy(saveStatus[1-sprev], status);
        first = StringUpdate(saveStatus[1-sprev], saveStatus[sprev], &length);
        if (length) {
          GotoYX(STATUSline, first);
          DisplayPrintf("%s", StringPad(saveStatus[1-sprev]+first, length));
        }
      }
      sprev = 1-sprev;
    }

    /* then make sure the cursor is in the right position on the */
    /* command-line. */
    GotoYX(EDITline, kshPromptLength+cursorIndex);
  }

  DisplayFlush();
}


/******************************* DisplayBell() *******************************/
/*                                                                           */
/* This routine just rings the bell using either the user specified          */
/* character sequence (if defined) or the terminfo default.  It sets a flag  */
/* so that it does this at most once for each call to DisplayFlush().        */
/*                                                                           */
/*****************************************************************************/

void DisplayBell()
{
  if (! rungBell) {
    if (bellString) {
      DisplayPrintf("%s", bellString);
    } else {
      Putp(bell, SAFE);
    }
    rungBell = TRUE;
  }
}


/***************************** DisplayStatus() *******************************/
/*                                                                           */
/* This routine just updates the "status" buffer with the newly computed     */
/* status-line.  This "status" buffer will be displayed on the terminal the  */
/* next time DisplayUpdate() is called.  Note that if a hint is being        */
/* displayed on the status-line, this routine does nothing.                  */
/*                                                                           */
/*****************************************************************************/

void DisplayStatus()
{
  if (! hintVisible) {
    if (statusFeeder && statusOn) {
      strncpy(status, (*statusFeeder)(), (size_t)columns-1);
      status[columns-1] = '\0';
    } else {
      status[0] = '\0';
    }
  }
}


/****************************** DisplayHint() ********************************/
/*                                                                           */
/* This routine displays the specified hint message on the status-line,      */
/* optionally accompanied with a bell ring.  If hint == NULL, any hints      */
/* are cleared and the real status-line is redisplayed.                      */
/*                                                                           */
/*****************************************************************************/

void DisplayHint(hint, ring)
char *hint;
int ring;
{
  if (hint) {
    if (ring) {
      DisplayBell();
    }
    strncpy(status, hint, (size_t)columns-1);
    status[columns-1] = '\0';
    hintVisible = TRUE;
  } else if (hintVisible) {
    hintVisible = FALSE;
    DisplayStatus();
  }
}


/******************************* DisplayHelp() *******************************/
/*                                                                           */
/* This routine formats and displays the specified help along with the       */
/* specified label.  If possible, a pager will be forked to allow the user   */
/* to browse the (potentially multi-screen) help.                            */
/*                                                                           */
/* The help macro language resembles a subset of the nroff man(5) macros.    */
/*                                                                           */
/*****************************************************************************/

#define DOTnf     1
#define DOTbr     2
#define DOTsp     3
#define DOTp      4
#define DOTip     5
#define DOTil     6
#define DOTin     7
#define DOTti     8

#define HELPWIDTH (columns*95/100-1)

void DisplayHelp(label, help)
char *label;
char *help;
{
  int ti;
  int ofd;
  pid_t pid;
  int dot;
  int len;
  int text;
  int left;
  int blank;
  int spaces;
  int cursor;
  int margin;
  char *tag;
  char *word;
  char *line;
  char *indent;
  void (*handler)();
  char savedWord[1024];
  char savedHelp[16384];

  /* temporarily put the command-line the user is working on aside */
  blank = globalSoftKeyBlank;
  globalSoftKeyBlank = TRUE;
  SoftKeyUpdate(TRUE);
  prev = 1-prev;
  DisplayAside(FALSE);

  /* then fork a pager to feed the formatted help thru. */
  ofd = OpenPager(&pid, &handler);

  /* then print the help label in inverse video */
  DisplayPrintf("%S \n", label);

  /* and then start reading macros and words from the help text */
  ti = 0;
  left = 0;
  cursor = 0;
  margin = 0;
  text = FALSE;

  /* save the help text in a temporary buffer since it might be in the */
  /* static data area of the quote module (which we will reuse below) */
  strncpy(savedHelp, help, sizeof(savedHelp));
  savedHelp[sizeof(savedHelp)-1] = '\0';
  helpInput = savedHelp;

  /* get the next word from the help text */
  while (word = DumbReadWord()) {

    /* check to see if it is a dot-macro  -- if not, save the word for */
    /* later (because it was returned in a static data area) */
    if (StringCompareLower(word, ".nf") == 0) {
      dot = DOTnf;
    } else if (StringCompareLower(word, ".br") == 0) {
      dot = DOTbr;
    } else if (StringCompareLower(word, ".sp") == 0) {
      dot = DOTsp;
    } else if (StringCompareLower(word, ".p") == 0) {
      dot = DOTp;
    } else if (StringCompareLower(word, ".ip") == 0) {
      dot = DOTip;
    } else if (StringCompareLower(word, ".il") == 0) {
      dot = DOTil;
    } else if (StringCompareLower(word, ".in") == 0) {
      dot = DOTin;
    } else if (StringCompareLower(word, ".ti") == 0) {
      dot = DOTti;
    } else {
      dot = 0;
      strncpy(savedWord, word, sizeof(savedWord)-1);
      savedWord[sizeof(savedWord)-1] = '\0';
    }

    /* if we got a dot-macto then break the current output line. */
    if (dot && cursor) {
      cursor = 0;
      DisplayPrintf("\n");
    }

    switch (dot) {
      /* if we got a ".nf", then discard chars up to the end of the */
      /* current line, then read a line at a time and print it as-is */
      /* until we get to a ".fi" line. */
      case DOTnf : DumbEatLine();
                   while (*helpInput) {
                     line = DumbReadLine();
                     *StringEndSig(line) = '\0';
                     if (strcmp(line, ".fi") == 0) {
                       break;
                     }
                     DisplayPrintf("%s\n", line);
                   }
                   text = TRUE;
                   break;

      /* if we got a ".br", we already did all we have to. */
      case DOTbr : break;

      /* if we got a ".sp" then print an extra blank line and reset */
      /* the flag that tells us we have (non-white) text on the previous */
      /* line. */
      case DOTsp : DisplayPrintf("\n");
                   text = FALSE;
                   break;

      /* if we got a ".p", ".ip", or ".il" then we're starting a new */
      /* paragraph -- if there was texton the previous line and this */
      /* isn't a ".il", then force a blank line preceeding the paragraph. */
      case DOTp  :
      case DOTip :
      case DOTil : if (text) {
                     text = FALSE;
                     if (dot != DOTil) {
                       DisplayPrintf("\n");
                     }
                   }

                   /* if this is a ".p", then set our paragraph margin to */
                   /* 0.  otherwise, read the tag and indent from the */
                   /* help text, print the tag, and set our paragraph margin */
                   /* as specified by the indent. */
                   margin = 0;
                   if (dot != DOTp) {
                     tag = QuoteReadWord(PeekHelp, ReadHelp, FALSE, '\0');
                     if (tag) {
                       DisplayPrintf("%s%s ", StringRepeat(' ', ti+left), tag);
                       cursor = left+strlen(tag)+1;
                       ti = 0;
                     }

                     indent = QuoteReadWord(PeekHelp, ReadHelp, FALSE, '\0');
                     if (indent) {
                       margin = atoi(indent);
                     }
                   }
                   break;

      /* set our global indent */
      case DOTin : indent = QuoteReadWord(PeekHelp, ReadHelp, FALSE, '\0');
                   if (indent) {
                     left += atoi(indent);
                     if (left < 0) {
                       left = 0;
                     }
                   }
                   break;

      /* set our temporary indent */
      case DOTti : indent = QuoteReadWord(PeekHelp, ReadHelp, FALSE, '\0');
                   if (indent) {
                     ti = atoi(indent);
                   }
                   break;

      /* otherwise we have a normal word to print. */
      default    : word = savedWord;
                   len = strlen(word);

                   /* find out how many spaces followed the word in the */
                   /* help text -- if none, then we'l just guess and follow */
                   /* the word with one space, unless it ends in a ".", in */
                   /* which case we'll assume it's the end of a sentence and */
                   /* end it in two. */
                   spaces = DumbReadSpace();
                   if (! spaces) {
                     if (len && (word[len-1] == '.' || word[len-1] == '!' ||
                                 word[len-1] == '?')) {
                       spaces = 2;
                     } else {
                       spaces = 1;
                     }
                   }

                   /* if the word won't fit on the current line, then */
                   /* start a new one. */
                   if (cursor > ti+left+margin && cursor+len >= HELPWIDTH) {
                     DisplayPrintf("\n");
                     cursor = 0;
                   }

                   /* if necessary, tab out to the paragraph margin */
                   if (cursor < ti+left+margin) {
                     DisplayPrintf("%s",
                                   StringRepeat(' ', ti+left+margin-cursor));
                     cursor = left+margin;
                     ti = 0;
                   }

                   /* then print the word.  if we have room, follow it */
                   /* with the appropriate number of spaces. */
                   DisplayPrintf("%s", word);
                   cursor += len;
                   if (cursor+spaces < HELPWIDTH) {
                     DisplayPrintf("%s", StringRepeat(' ', spaces));
                     cursor += spaces;
                   }

                   /* and set the flagindicating that the previous line */
                   /* contains non-white text */
                   text = TRUE;
                   break;
    }
  }
  if (cursor) {
    DisplayPrintf("\n");
  }
  DisplayFlush();

  /* then close the pager */
  ClosePager(ofd, pid, handler);

  /* and redraw the original command-line, etc. */
  globalSoftKeyBlank = blank;
  DisplayResume();
}


/******************************* DisplayPrintf() *****************************/
/*                                                                           */
/* This routine is similar to the printf(3) routine.  It understands the     */
/* following data types:                                                     */
/*                                                                           */
/*   %%   - print a '%'                                                      */
/*   %c   - print a character                                                */
/*   %d   - print a signed integer                                           */
/*   %D   - print a hilited signed integer                                   */
/*   %q   - print a string with double-quotes around it                      */
/*   %s   - print a string                                                   */
/*   %S   - print a hilited string                                           */
/*                                                                           */
/*****************************************************************************/

/*VARARGS*/
void DisplayPrintf(va_alist)
va_dcl
{
  int c;
  int d;
  char *p;
  char *q;
  char *s;
  int hilite;
  va_list ap;

  va_start(ap);
  p = va_arg(ap, char *);
  while (*p) {
    if (*p == '%' && *(p+1)) {
      p++;
      hilite = FALSE;
      switch (*p) {
        case '%'  : WriteChar('%');
                    break;
        case 'c'  : c = va_arg(ap, int);
                    WriteChar(c);
                    break;
        case 'D'  : hilite = TRUE;
        case 'd'  : d = va_arg(ap, int);
                    WriteString(ltoa((long)d), hilite);
                    break;
        case 'q'  : q = va_arg(ap, char *);
                    if (q) {
                      WriteChar('"');
                      WriteString(q, hilite);
                      WriteChar('"');
                    } else {
                      WriteString("NULL", hilite);
                    }
                    break;
        case 'S'  : hilite = TRUE;
        case 's'  : s = va_arg(ap, char *);
                    if (s) {
                      WriteString(s, hilite);
                    } else {
                      WriteString("NULL", hilite);
                    }
                    break;
        default   : assert(FALSE);
      }
    } else {
      WriteChar(*p);
    }
    p++;
  }
  va_end(ap);
}


/********************************* DisplayClear() ****************************/
/*                                                                           */
/* This routine clears the display.  If the display is currently scrolled,   */
/* it clears from the cursor position to the end of scrolling memory;        */
/* otherwise it clears the whole thing.                                      */
/*                                                                           */
/*****************************************************************************/

void DisplayClear()
{
  if (! scrolled) {
    Putp(key_home, HARMFULL);
    Putp(clear_screen, HARMFULL);
    DisplayRefresh();
  } else {
    GotoYX(EDITline-kshPromptLines, 0);
    Putp(clr_eos, SAFE);
    DisplayScroll(0);
  }
}


/******************************* DisplayFlush() ******************************/
/*                                                                           */
/* This routine flushes the output buffer used by the display module (for    */
/* performance reasons), causing all of the buffered data to be written      */
/* to the terminal.                                                          */
/*                                                                           */
/*****************************************************************************/

void DisplayFlush()
{
  rungBell = FALSE;
  BufferWriteFlush();
}


/****************************** DisplayScroll() ******************************/
/*                                                                           */
/* This routine scrolls the display the specified number of lines in the     */
/* specified direction (negative == backward; positive == forward).  If      */
/* 0 is specified as the scroll count, this routine "unscrolls" the display  */
/* as if the user had hit <Home down>.                                       */
/*                                                                           */
/*****************************************************************************/

void DisplayScroll(n)
int n;
{
  if (n < 0) {
    /* if we weren't already scrolled, cear the keysh command-line, etc. */
    /* from the bottom of the screen. */
    if (! scrolled) {
      GotoYX(EDITline-kshPromptLines, 0);
      Putp(clr_eos, SAFE);
      scrolled = TRUE;
    }

    /* then scroll back as many pages and lines as necessary */
    while (n <= -lines) {
      Putp(key_ppage, HARMFULL);
      n += lines;
      scroll -= lines;
    }
    while (n < 0) {
      if (keyshellTsm) {
        Putp(TSMMETA, SAFE);
      }
      Putp(key_sr, HARMFULL);
      n++;
      scroll--;
    }

    /* and put the cursor where the user expects it to be. */
    GotoYX(EDITline-kshPromptLines, 0);
    n = -1;
  }

  if (n + scroll > 0) {
    n = 0;
  }

  if (n > 0) {
    /* scroll forward as many pages andlines as necessary */
    while (scroll <= -lines && n >= lines) {
      Putp(key_npage, HARMFULL);
      n -= lines;
      scroll += lines;
    }
    while (scroll < 0 && n > 0) {
      if (keyshellTsm) {
        Putp(TSMMETA, SAFE);
      }
      Putp(key_sf, HARMFULL);
      n--;
      scroll++;
    }

    /* and put the cursor where the user expects it to be */
    if (scroll) {
      GotoYX(EDITline-kshPromptLines, 0);
    }
    n = 1;
  }

  /* if we have unscrolled all the way, then move the cursor to */
  /* the end of display memory and then redraw the keysh command-line, etc. */
  if (scrolled && (n == 0 || n > 0 && scroll == 0)) {
    if (keyshellTsm) {
      Putp(TSMMETA, SAFE);
    }
    Putp(key_ll, HARMFULL);
    scrolled = FALSE;
    scroll = 0;
    DisplayResume();
  }
}


/********************************** DisplayAside() ***************************/
/*                                                                           */
/* This routine temporarily puts the command-line aside so that other        */
/* information (e.g., help or file-name completion list) can be displayed.   */
/* Once this is done, DisplayResume() can be called to restore the command-  */
/* line to its original state.                                               */
/*                                                                           */
/*****************************************************************************/

void DisplayAside(noNL)
int noNL;
{
  GotoYX(EDITline, columns-1);
  Putp(clr_eos, SAFE);
  DisplayPrintf("\n");

  if (noNL && EDITline < lines-1) {
    GotoYX(EDITline, 0);
  } else if (noNL) {
    GotoYX(EDITline-1, 0);
  }
  DisplayFlush();
}


/******************************* DisplayEnd() ********************************/
/*                                                                           */
/* this routine perform post-line clean-up for the display module.  It       */
/* updates the softkeys one last time (in case backup softkeys need to be    */
/* programmed), puts the keypad back into local mode (where ksh leaves it),  */
/* and puts the command-line display aside so that a command can run.        */
/*                                                                           */
/*****************************************************************************/

void DisplayEnd()
{
  if (! globalInteractive) {
    return;
  }

  SoftKeyUpdate(TRUE);
  prev = 1-prev;
  Putp(keypad_local, SAFE);

  DisplayAside(keyshellInterrupt || keyshellEOF);

  tsmSoftKeys = TSMMAYBE;
}


/*****************************************************************************/
