/*
static char rcsId[] =
  "@(#) $Header: keyboard.c,v 66.15 91/08/27 16:39:44 ssa Exp $";
static char copyright[] =
  "Copyright (c) HP, 1989";
*/

/********************************* keyboard.c ********************************/
/*                                                                           */
/* This module handles all terminal input.  Any processing which doesn't     */
/* directly affect the command-line is performed here.  This includes:       */
/*                                                                           */
/*   - identifying terminal escape sequences                                 */
/*   - identifying interrupts                                                */
/*   - identifying screen scrolling requests                                 */
/*   - identifying erase and kill characters                                 */
/*   - identifying refresh and clear-screen requests                         */
/*   - identifying softkey selector (upper-case accelerator) characters      */
/*   - identifying help and hint requests                                    */
/*   - redirecting keyboard input to come from an input feeder function      */
/*                                                                           */
/* Most importantly, though, this module returns characters typed by the     */
/* user to be processed by the emacs, vi, and edit modules.                  */
/*                                                                           */
/*****************************************************************************/


#define SINGLE
#define MINICURSES

#include <curses.h>
#include <termio.h>
#include <term.h>
#include <time.h>
#include <string.h>
#include <assert.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include "keyshell.h"
#include "global.h"
#include "edit.h"
#include "vi.h"
#include "keyboard.h"
#include "display.h"
#include "debug.h"

#include "message.h"
#include "kshhooks.h"
#include "clist.h"
#include "select.h"
#include "quote.h"
#include "buffer.h"


/******************************* SEQUENCElength ******************************/
/*                                                                           */
/* This constant defines the longest escape character sequence which         */
/* we expect to ever need to scan terminal input for -- and therefore the    */
/* longest escape character sequence we expect to find in a terminfo(4)      */
/* database entry.  Note that we do *not* use the "sgr" capability.          */
/*                                                                           */
/*****************************************************************************/

#define SEQUENCElength     32


/******************************** FAKEHELP ***********************************/
/*                                                                           */
/* This constant defines a pseudo-keyboard character code which we use       */
/* locally when we want to pretend that the <Tab> key is really a "--Help--" */
/* key.  It must not colide with any of the values for the KeyboardChar      */
/* enumeration defined in keyboard.h.                                        */
/*                                                                           */
/*****************************************************************************/

#define FAKEHELP           0x20000


/******************************** keyboardQuote ******************************/
/*                                                                           */
/* This *external* boolean is set to TRUE when the next character to be      */
/* read by the keyboard module has been quoted by a back-slash-type escape   */
/* mechanism.  It is set in this module when a true "\" has been read.  It   */
/* can also be set by either the emacs or vi modules when they read their    */
/* respective quoting characters (^Q or ^V, respectively).  This variable    */
/* is always reset to FALSE (in this module) after a non-"\" character has   */
/* been read.                                                                */
/*                                                                           */
/*****************************************************************************/

int keyboardQuote;


/************************************* kc ************************************/
/*                                                                           */
/* This variable is for debugging purposes only.  Assigning an int to this   */
/* in the debugger allows you to determine a (symbolic) KeyboardChar         */
/* enumeration value.  The real solution here is to change a significant     */
/* number of the "ints" declared throughout the keysh code into              */
/* "KeyboardChar"s.  Sorry.                                                  */
/*                                                                           */
/*****************************************************************************/

enum KeyboardChar kc;


/***************************** specialChars **********************************/
/*                                                                           */
/* This array defines all of the terminfo escape sequences we will scan      */
/* terminal input for, along with a KeyboardChar equivalent value for each.  */
/* When we read an escape sequences from the terminal, we will return the    */
/* KeyboardChar equivalent value rather than the characters which make up    */
/* the sequence.                                                             */
/*                                                                           */
/*****************************************************************************/

static const char *nl = "\n";
static const char *cr = "\r";

struct {
  int c;
  char **string;
} specialChars[] = {
  keyboardForward, &key_right,
  keyboardBackward, &key_left,
  keyboardForwardBWord, &tab,
  keyboardBackwardBWord, &back_tab,
  keyboardToggleInsertOverwrite, &key_ic,
  keyboardDelete, &key_dc,
  keyboardDeleteLine, &key_dl,
  keyboardClearEOL, &key_eol,
  keyboardClear, &key_eos,
  keyboardClear, &key_clear,
  keyboardBeginLine, &key_home,
  keyboardEndLine, &key_ll,
  keyboardPrevious1, &key_up,
  keyboardNext1, &key_down,
  keyboardScrollBackward, &key_sr,
  keyboardScrollForward, &key_sf,
  keyboardPageBackward, &key_ppage,
  keyboardPageForward, &key_npage,
  keyboardTranslate, &key_il,
  keyboardExecute, &cr,
  keyboardExecute, &nl,
  keyboardF1, &key_f1,
  keyboardF2, &key_f2,
  keyboardF3, &key_f3,
  keyboardF4, &key_f4,
  keyboardF5, &key_f5,
  keyboardF6, &key_f6,
  keyboardF7, &key_f7,
  keyboardF8, &key_f8,
  0, NULL
};


/******************************* inputFeeder *********************************/
/*                                                                           */
/* This pointer identifies the function we will call to read a single        */
/* character (or escape sequences converted to KeyboardChar) from the        */
/* terminal.  Normaly it points to DefaultInputFeeder(), but this can        */
/* be overridden by other keysh modules if they wish to "simulate" user      */
/* input by uding the KeyboardInputFeeder() function.                        */
/*                                                                           */
/*****************************************************************************/

static int (*inputFeeder)();


/********************************** wList ************************************/
/*                                                                           */
/* This list contains input characters which were force-fed into the         */
/* keyboard module thru the KeyboardWrite() routine.  These characters       */
/* take priority over normal input characters (from the terminal) and will   */
/* be returned (in order) upon subsequent calls to KeyboardRead().  Only     */
/* once this list is empty will real terminal characters be read again.      */
/*                                                                           */
/*****************************************************************************/

static CList *wList;


/*********************************** peek ************************************/
/*                                                                           */
/* This integer contains the value of the last character peeked by the       */
/* KeyboardPeek() routine or 0 if no characters have been peeked.  If a      */
/* character has been peeked, it will *always* be the next character         */
/* returned by the KeyboardRead() routine.                                   */
/*                                                                           */
/*****************************************************************************/

static int peek;


/*********************************** usec ************************************/
/*                                                                           */
/* This integer holds the number of micro-seconds which we will wait before  */
/* declaring an incomplete prefix of a valid escape sequence (defined by     */
/* specialChars) to *not* be part of a real escape sequence (i.e., typed     */
/* by hand by the user) -- and hence return its characters individually at   */
/* face value.                                                               */
/*                                                                           */
/*****************************************************************************/

static int usec;


/********************************** nonBlock *********************************/
/*                                                                           */
/* This boolean specifies that we are checking availability for an input     */
/* character from the terminal.  If set to TRUE, low-level input routines    */
/* will *never* block, instead, they will return keyboardNull (the no-op     */
/* character) if no characters are available.                                */
/*                                                                           */
/*****************************************************************************/

static int nonBlock;


/************************ defaultLines / defaultColumns **********************/
/*                                                                           */
/* These integers hold the terminfo default values for lines/columns for     */
/* the terminal -- we use them if we find $LINES or $COLUMNS unset.          */
/*                                                                           */
/*****************************************************************************/

static int defaultLines;
static int defaultColumns;


/*****************************************************************************/
/*****************************************************************************/
/*****************************************************************************/
/*****************************************************************************/
/*****************************************************************************/


/********************************* ReadChar() ********************************/
/*                                                                           */
/* This routine reads the next character from the terminal.  This routine    */
/* should be called with "partial" set non-zero if a partial escape sequence */
/* (from specialChars) has already been scanned.  Otherwise, it should be    */
/* called with "partial" set to 0.                                           */
/*                                                                           */
/* If a "partial" is non-zero and a character is not available within        */
/* "usec" micro-seconds, this routine will return keyboardTimeout,           */
/* signifying that an escape sequence took too long to complete.             */
/* Otherwise, this routine will wait up to 1 minute for a character          */
/* to come in -- if one does not, we wake up just long enough to update      */
/* the status-line.  If an interrupt is detected keyboardInterrupt is        */
/* returned immediately.                                                     */
/*                                                                           */
/*****************************************************************************/

static int ReadChar(partial)
int partial;
{
  int n;
  char c;
  SelectMask m;
  long available;
  struct timeval tv;

  do {
    /* check to see if ksh caught an interrupt -- abort if so */
    if (KshInterrupt()) {
      keyshellInterrupt = TRUE;
      return keyboardInterrupt;
    }

    /* check to see if any characters are waiting to be read.  if so, */
    /* just update the display of the command-line; otherwise, also */
    /* update the softkeys, status-line, etc. */
    if (ioctl(KshInputFd(), FIONREAD, &available) || available == 0) {
      if (nonBlock) {
        nonBlock = FALSE;
        return keyboardNull;
      }
      DisplayUpdate(TRUE);
    } else {
      DisplayUpdate(FALSE);
    }

    /* check to see if we are in a hurry (i.e., if a partial escape */
    /* sequence has already been read).  if so, set a short timeout. */
    if (partial) {
      tv.tv_sec = 0;
      tv.tv_usec = usec;
    } else {
      tv.tv_sec = 59;
      tv.tv_usec = 0;
    }

    /* then wait for either a character to be received or a ksh */
    /* interrupt to occur. */
    do {
      SelectInit(m);
      SelectInclude(m, KshInputFd());
      n = select(SELECTfiles, m, NULL, NULL, &tv);
    } while (n < 0 && errno == EINTR && ! KshInterrupt() &&
             ! keyshellSigwinch);

    /* if we were interrupted (e.g., ^C by the user) or we timed out, */
    /* in an escape sequence, return here.  otherwise, if we just did a */
    /* one minute time-out, update the status line and go back to sleep. */
    if (n < 0) {
      if (! keyshellSigwinch) {
        keyshellInterrupt = TRUE;
      }
      return keyboardInterrupt;
    } else if (n == 0) {
      if (! partial) {
        DisplayStatus();
        c = '\0';
        continue;
      }
      return keyboardTimeout;
    }

    /* otherwise we have a character to read -- read it.  if an error */
    /* occurs, pretend we got an interrupt (which we *didn't* since */
    /* select(2) said we wouldn't block...).  also check for EOF. */
    n = read(KshInputFd(), &c, (unsigned)1);
    if (n < 0) {
      keyshellInterrupt = TRUE;
      return keyboardInterrupt;
    } else if (n == 0) {
      keyshellEOF = TRUE;
      return keyboardInterrupt;
    }
  } while (! c);

  /* return the character read from the terminal. */
  return c;
}


/**************************** ReadKeyboardChar() *****************************/
/*                                                                           */
/* This function reads characters from the terminal and attempts to identify */
/* any escape sequences from specialChars.  It updates the status-line       */
/* if ReadChar() times out while not in an escape-sequence.                  */
/*                                                                           */
/*****************************************************************************/

static int ReadKeyboardChar()
{
  int c;
  int i;
  int len;
  int prefix;
  char *special;
  static char *pending;
  static char string[SEQUENCElength];

  /* "len" is how far we are into a potential escape sequence. */
  len = 0;

  for (;;) {
    /* if we have characters to return from a previously invalid escape */
    /* sequence then return them here. */
    if (pending && *pending) {
      return *pending++;
    }

    /* otherwise start looking for a new escape sequence. */
    pending = NULL;
    c = ReadChar(len);

    /* if the read timed out, then abort the partial escape sequence and */
    /* return the individual characters. */
    if (c == keyboardTimeout) {
      pending = string;

    /* otherwise if we got an interrupt or null char, just return it. */
    } else if (c == keyboardInterrupt || c == keyboardNull) {
      return c;

    /* otherwise append the new input character to the string we are */
    /* building. if that string is equal to one of the escape sequences */
    /* defined by specialChars, then return the corresponding keyboardChar. */
    /* otherwise, if that string is not a *prefix* of one of the */
    /* escape sequences, then return the characters of the string by */
    /* themselves.  otherwise keep reading (in a hope to scan the */
    /* rest of the escape sequence). */
    } else {
      string[len++] = c;
      string[len] = '\0';
      prefix = FALSE;
      for (i = 0; specialChars[i].c; i++) {
        if (! globalSoftKeyBackup || ! KEYBOARDf(specialChars[i].c)) {
          special = *specialChars[i].string;
          if (special) {
            if (strncmp(string, special, (size_t)len) == 0) {
              if (strcmp(string, special) == 0) {
                return specialChars[i].c;
              }
              prefix = TRUE;
            }
          }
        }
      }
      if (! prefix) {
        pending = string;
      }
    }

  }
}


/***************************** DefaultInputFeeder() **************************/
/*                                                                           */
/* This is the default input feeder for the keyboard module.  It reads a     */
/* character (or special key) from the terminal and the performs basic       */
/* input processing, including screen scrolling, erase and kill character    */
/* recognition, selector character recognition, etc.  It returns chars to    */
/* be used by the edit modules.                                              */
/*                                                                           */
/*****************************************************************************/

static int DefaultInputFeeder()
{
  int c;
  int n;
  GlobalSoftKey *sk;

again:
  for (;;) {
    /* get the next character/key from the terminal. */
    c = ReadKeyboardChar();

    /* if it is a screen scrolling key then perform the desired function. */
    /* otherwise, ensure that the screen is unscrolled and break out of */
    /* the loop. */
    if (c == keyboardScrollBackward) {
      DisplayScroll(-1);
    } else if (c == keyboardScrollForward) {
      DisplayScroll(1);
    } else if (c == keyboardPageBackward) {
      DisplayScroll(-lines);
    } else if (c == keyboardPageForward) {
      DisplayScroll(lines);
    } else if (c == keyboardClear) {
      c = keyboardDeleteLine;
      DisplayClear();
      globalHeadExtent->etcetera = 0;
      break;
    } else {
      DisplayScroll(0);
      break;
    }
  }

  /* if the char is an unquoted erase or kill character, then translate */
  /* it into the corresponding basic editing command understood by the */
  /* edit module.  Note that if we are not in "raw" input mode, this char */
  /* *will* pass thru the emacs or vi modules -- which understand CNTL('H'). */
  if (c == KshKillChar() && ! keyboardQuote) {
    c = keyboardDeleteLine;
  } else if (c == KshEraseChar() && ! keyboardQuote) {
    if (editMode == editRaw) {
      c = keyboardBackspace;
    } else {
      c = CNTL('H');
    }


  /* if the char is an unquoted <Tab> character then return either */
  /* keyboardTranslate, FAKEHELP, or <Tab>. */
  } else if (c == '\t' && ! keyboardQuote) {
    if (! key_il || KshGetEnv(KEYSHELLlocal)) {
      c = keyboardTranslate;
    } else if (! KEYBOARDhelp) {
      c = FAKEHELP;
    }

  /* if the char is an unquoted ^L, then refresh the bottom lines of the */
  /* display and reprogram any necessary terminal modes. */
  } else if (c == CNTL('L') && ! keyboardQuote) {
    DisplayRefresh();
    goto again;

  /* if the char is an unquoted EOF char *and* there are no characters on */
  /* the command-line *and* we are at a $PS1, then pretend that an EOF */
  /* occurred. */
  } else if (c == KshEOFChar() && ! keyboardQuote && ! editLine[0] &&
             keyshellPSn == 1) {
    keyshellEOF = TRUE;
    c = keyboardInterrupt;
  }

  /* if keyboard selector chars are enabled and we are displaying normal */
  /* (non-backup) softkeys and the char isn't quoted and *is* upper-case, */
  /* then check to see if it was one of the accelerator chars for a */
  /* displayed softkey.  if so, pretend that the function key was actually */
  /* hit. */
  if (globalAccelerate && ! globalSoftKeyBackup) {
    if (! keyboardQuote && c < 256 && c != tolower(c) &&
        (editMode != editVi || ! viEscape)) {
      for (n = 0; n < DISPLAYsoftKeyCount; n++) {
        sk = GlobalDisplayedSoftKeyN(n);
        if (sk && c == sk->accelerator) {
          c = KEYBOARDnToF(n);
          break;
        }
      }
    }
  }

  /* then check to see if the *next* character read by this module should */
  /* be considered "quoted". */
  if (c == '\\' && ! keyboardQuote) {
    keyboardQuote = TRUE;
  } else {
    keyboardQuote = FALSE;
  }

  /* cya -- if we aren't at a $PS1 and the user wants to try and */
  /* translate the current command-line, then just laugh at him... */
  if (c == keyboardTranslate && keyshellPSn != 1) {
    DisplayBell();
    goto again;
  }

  /* return the character to be sent to the editing modules. */
  return c;
}


/********************************** HelpText() *******************************/
/*                                                                           */
/* This routine returns the help text for the specified softkey node.  If    */
/* the softkey file has changed since the node was read, an error message    */
/* is returned.                                                              */
/*                                                                           */
/*****************************************************************************/

static char *HelpText(sk)
GlobalSoftKey *sk;
{
  int fd;
  int oldFd;
  char *s;
  struct stat statBuf;

  /* open the appropriate softkey file to read the help from */
  fd = open(sk->file, O_RDONLY);
  if (fd < 0 || fstat(fd, &statBuf) < 0 || sk->st_mtime != statBuf.st_mtime) {
    return MESSAGEhelpCorrupt;
  }

  /* then seek to the position of the help */
  (void)lseek(fd, (off_t)sk->help, SEEK_SET);

  /* then read the help string */
  oldFd = BufferReadFd(fd);
  s = QuoteReadWord(BufferPeekChar, BufferReadChar, FALSE, ';');
  (void)BufferReadFd(oldFd);

  /* then close the help file and return the string. */
  close(fd);
  return s;
}


/******************************** Help() *************************************/
/*                                                                           */
/* This routine provides help for the user.  It prompts him to select the    */
/* key which he wants help with and then proceedes to display the help.      */
/* When it returns, everything (e.g., the command-line) is back to where     */
/* it was initially.                                                         */
/*                                                                           */
/*****************************************************************************/

static void Help()
{
  int c;
  char *s;
  char *label;
  GlobalSoftKey *sk;

  /* Tell the user to press the key he wants help with. */
  DisplayHint(MESSAGEhelpHint, TRUE);

  globalSoftKeyTopics = TRUE;

  /* then wait for him to press a valid key -- let him still use the */
  /* "--Etc--" key to browse the other softkeys. */
  do {
    c = (*inputFeeder)();
    if (c == KEYBOARDetc) {
      GlobalEtcNext();
    }
  } while (c == KEYBOARDetc);

  globalSoftKeyTopics = FALSE;

  /* if he pressed the "--Help--" key again, display a list of general help */
  /* topics -- and wait for him to press another key. */
  if (c == KEYBOARDhelp || c == FAKEHELP) {
    globalSoftKeyHelp = TRUE;
    ConfigAssignAcceleratorsEtc(globalHelpSoftKey);
    DisplayHint(MESSAGEgeneralHelpHint, TRUE);
    c = (*inputFeeder)();
  }

  /* if he presses <Return>, give him help in his current context. */
  /* if that help doesn't exist, look for generic help which can be used */
  /* as a hint -- better than nothing. */
  if (c == keyboardExecute) {
    if (globalLastExtent && globalLastExtent->selected) {
      sk = globalLastExtent->selected;
      label = globalLastExtent->word;
      goto skip;
    } else {
      for (sk = HierHeadChild(globalHelpSoftKey); sk;
           sk = HierNextSibling(sk)) {
        if (sk->hint) {
          label = sk->literal;
          goto skip;
        }
      }
    }
  }

  /* otherwise, if he pressed a function key, display the help for the */
  /* corresponding softkey node.  if no help is available, use the hint */
  /* or required strings as a last resort. */
  if (KEYBOARDf(c)) {
    sk = GlobalDisplayedSoftKeyN(KEYBOARDfToN(c));
    label = sk->literal;
skip:
    globalSoftKeyHelp = FALSE;
    if (sk && ! sk->attrib.empty) {
      DisplayHint(NULL, FALSE);
      if (sk->help) {
        DisplayHelp(label, HelpText(sk));
      } else if (sk->hint) {
        DisplayHelp(label, sk->hint);
      } else if (sk->required && sk->attrib.string) {
        DisplayHelp(label, sk->required);
      } else {
        DisplayHint(MESSAGEnoHelpHint, TRUE);
      }
    } else {
      DisplayHint(MESSAGEdisabledHint, TRUE);
    }

  } else if (c < 256 && toupper(c) == *MESSAGEhelpQuit) {
    globalSoftKeyHelp = FALSE;
    DisplayHint(NULL, FALSE);

  /* otheriwse, the user pressed an invalid key -- tell him no help */
  /* is available. */
  } else {
    globalSoftKeyHelp = FALSE;
    DisplayHint(MESSAGEnoHelpHint, TRUE);
  }
}


/******************************** SetLinesColumns() **************************/
/*                                                                           */
/* This routine just reads the ksh $LINES and $COLUMNS variables and         */
/* makes sure that the curses "lines" and "columns" variables reflect the    */
/* current screen size.  This is needed if the user dynamically (after       */
/* invocation) resizes the size of an hpterm window.                         */
/*                                                                           */
/*****************************************************************************/

static void SetLinesColumns()
{
  char *s;

  if (s = KshGetEnv("LINES")) {
    lines = atoi(s);
  } else {
    lines = defaultLines;
  }
  if (s = KshGetEnv("COLUMNS")) {
    columns = atoi(s);
  } else {
    columns = defaultColumns;
  }
  if (lines < DISPLAYminLines) {
    lines = DISPLAYminLines;
  }
  if (columns > DISPLAYlength) {
    columns = DISPLAYlength;
  }
}


/*****************************************************************************/
/*****************************************************************************/
/****************************** EXTERNAL FUNCTIONS ***************************/
/*****************************************************************************/
/*****************************************************************************/


/******************************* KeyboardInitialize() ************************/
/*                                                                           */
/* This routine initializes the keyboard module.  It is responsible for      */
/* opening the terminfo(4) database and ensuring that the rest of keysh      */
/* will not encounter any (immediate) fatal errors.  If it cannot do this,   */
/* it will simply exec a /bin/ksh so that the user can at least log in.      */
/*                                                                           */
/*****************************************************************************/

void KeyboardInitialize()
{
  int i;
  int fd;
  char *s;
  static char *null;
  struct termio attr;

  /* initialize the display module in case we need to display any errors */
  DisplayInitialize();

  /* open up the terminfo(4) database for the terminal type specified in */
  /* the $TERM ksh variable.  Note that while we do this, we */
  /* clear TAB3 (expand tabs to spaces) for the tty, since otherwise */
  /* terminfo will (believe it or not) NULL the "tab" and "back_tab" */
  /* capabilities!  If the terminal can't address the cursor, we */
  /* pretend that the terminfo initialization failed. */
  if ((s = KshGetEnv("TERM")) && s[0]) {
    fd = KshOutputFd();
    if (ioctl(fd, TCGETA, &attr) == 0 && (attr.c_oflag & TABDLY) == TAB3) {
      attr.c_oflag &= ~TABDLY;
      (void)ioctl(fd, TCSETAW, &attr);
    }
    setupterm(s, fd, &keyshellInitialized);
    if (keyshellInitialized) {
      resetterm();
    }
    if (! cursor_address) {
      keyshellInitialized = FALSE;
    }
  }

  /* if the terminfo initialization failed then try to exec a /bin/ksh. */
  /* if we were a login shell (e.g., "-keysh"), pass that info on to ksh. */
  /* if the exec fails, we will return and continue with keysh, but */
  /* we will only do cannonical reads from the terminal (like /bin/sh). */
  if (! keyshellInitialized && KshInteractive()) {
    DisplayPrintf(MESSAGEterminfoFailed, KshArgv0());
    DisplayPrintf(MESSAGEexecingKsh, KshArgv0());
    DisplayFlush();
    if (*KshArgv0() == '-') {
      (void)execl("/bin/ksh", "-ksh", NULL);
    } else {
      (void)execl("/bin/ksh", "ksh", NULL);
    }
    perror("execl");
    DisplayPrintf(MESSAGEcontinuing, KshArgv0());
    DisplayFlush();
  }

  /* if there are any terminfo capabilities which correspond to single */
  /* ascii characters, wipe them out.  This gives normal ascii chars */
  /* precedence over any terminfo capability (so that if left arrow is */
  /* defined as ^H, we pitch the left arrow definition and continue to */
  /* process back-spaces normally). */
  for (i = 0; specialChars[i].c; i++) {
    s = *specialChars[i].string;
    if (specialChars[i].c != keyboardExecute && s && s[0] && ! s[1]) {
      specialChars[i].string = &null;
    }
  }

  /* if there is no capability to program a function key "key" sequence, */
  /* but there *is* a capability to program a function key "xmit" */
  /* sequence, then use that capability instead (as a last resort...). */
  if (pkey_xmit && ! pkey_key) {
    pkey_key = pkey_xmit;
  }

  /* save the terminfo default lines/columns values */
  defaultLines = lines;
  defaultColumns = columns;

  /* by default, read input characters from the terminal. */
  KeyboardInputFeeder(NULL);

  /* create (and empty) the list of hi priority KeyboardWrite() chars */
  wList = CListCreate();
}


/*************************** KeyboardInputFeeder() ***************************/
/*                                                                           */
/* This routine specifies a function which this module should call to        */
/* read input from.  By default (or if called with feeder == NULL), the      */
/* DefaultInputFeeder() function will be called which reads input from       */
/* the terminal attached to stdin.                                           */
/*                                                                           */
/*****************************************************************************/

void KeyboardInputFeeder(feeder)
int (*feeder)();
{
  if (feeder) {
    inputFeeder = feeder;
  } else {
    inputFeeder = DefaultInputFeeder;
  }
}


/***************************** KeyboardWrite() *******************************/
/*                                                                           */
/* This function writes the specified character into a private queue.        */
/* Characters from this queue take priority over normal input characters and */
/* are returned in order upon sebsequent calls to KeyboardRead().            */
/*                                                                           */
/*****************************************************************************/

void KeyboardWrite(c)
int c;
{
  CListAdd(wList, c);
}


/****************************** KeyboardBegin() ******************************/
/*                                                                           */
/* This function performs the pre-line initialization functions for this     */
/* module.  In particular, it checks for any shell pertinant shell variables */
/* have changed since the last time called.                                  */
/*                                                                           */
/*****************************************************************************/

void KeyboardBegin()
{
  char *s;

  /* default to "not reading a quoted character" */
  keyboardQuote = FALSE;

  /* set the terminal size based on the LINES and columns variables */
  SetLinesColumns();

  /* set the escape sequence inter-character timeout */
  if (! (s = KshGetEnv(KEYSHELLtimeOut)) || ! (usec = atoi(s)*1000)) {
    usec = 350000;
  }

  DisplayBegin();
}


/******************************* KeyboardRead() ******************************/
/*                                                                           */
/* This routine is the main interface to the keyboard module.  It returns    */
/* the next character read from the keyboard.  "--Etc--", "--Help--", and    */
/* hint requests are handled here.                                           */
/*                                                                           */
/*****************************************************************************/

int KeyboardRead()
{
  int c;
  GlobalSoftKey *sk;

  /* if someone previously KeyboardPeek()'d a character, then return it */
  /* now so that we guarantee that a peeked character is always the next */
  /* one to be read. */
  if (peek) {
    c = peek;
    peek = 0;
    return c;
  }

  /* if someone previously KeyboardWrite()'d a character, then return it */
  /* now so that we guarantee that a written character will always be */
  /* read before a new character is actually input from the terminal. */
  if (c = CListRemove(wList)) {
    return c;
  }

  for (;;) {
    /* otherwise, get a new input character to process. */
    c = (*inputFeeder)();

    /* if it is the "--Etc--" softkey then just advance the etcetera bank */
    if (c == KEYBOARDetc) {
      GlobalEtcNext();

    /* if it is the "--Help--" softkey then process it. */
    } else if (c == KEYBOARDhelp || c == FAKEHELP) {
      Help();

    /* otherwise, if it is a function key, then check if it is either */
    /* disabled or a string type softkey with a hint.  if the softkey */
    /* is disabled then just ring the bell.  otherwise, if the user is */
    /* requesting a hint, then display the associated hint message. */
    } else if (KEYBOARDf(c)) {
      sk = GlobalDisplayedSoftKeyN(KEYBOARDfToN(c));
      if (! sk || sk->attrib.empty) {
        DisplayBell();
      } else if (! sk->attrib.softKey) {
        if (sk->hint) {
          DisplayHint(sk->hint, TRUE);
        } else if (sk->required) {
          DisplayHint(sk->required, TRUE);
        } else {
          DisplayHint(MESSAGEnoHintHint, TRUE);
        }
      } else {
        break;
      }
    } else {
      break;
    }
  }

  /* any time we return a real character, we clear the previous hint */
  /* message on the status-line -- restoring its original contents. */
  if (c != keyboardNull) {
    DisplayHint(NULL, FALSE);
  }

  return c;
}


/******************************** KeyboardPeek() *****************************/
/*                                                                           */
/* This routine returns a peek at the *next* character which will be         */
/* read by the KeyboardRead() routine.                                       */
/*                                                                           */
/*****************************************************************************/

int KeyboardPeek()
{
  /* if we haven't already peeked a character, read one and save it so */
  /* that the KeyboardRead() routine will return it next time it is */
  /* called. */
  if (! peek) {
    peek = KeyboardRead();
  }
  return peek;
}


/***************************** KeyboardAvail() *******************************/
/*                                                                           */
/* This routine returns non-zero if there is a character immediately         */
/* waiting to be read *and* it is an alphanumeric-like character.            */
/*                                                                           */
/*****************************************************************************/

int KeyboardAvail()
{
  nonBlock = TRUE;
  return KEYBOARDavailChar(KeyboardPeek());
}


/******************************** KeyboardEnd() ******************************/
/*                                                                           */
/* This routine does post-line cleanup for the keyboard module.              */
/*                                                                           */
/*****************************************************************************/

void KeyboardEnd()
{
  DisplayEnd();
}


/*****************************************************************************/
