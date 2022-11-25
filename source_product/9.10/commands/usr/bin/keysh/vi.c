/*
static char rcsId[] =
  "@(#) $Header: vi.c,v 66.11 91/01/01 15:44:04 rpt Exp $";
static char copyright[] =
  "Copyright (c) HP, 1989";
*/


/********************************* emacs.c ***********************************/
/*                                                                           */
/* This module is responsible for translating vi edit commands received      */
/* from the keyboard module into generic edit commands understood by the     */
/* edit module.                                                              */
/*                                                                           */
/*****************************************************************************/


#include <string.h>
#include <assert.h>
#include <ctype.h>

#include "keyshell.h"
#include "edit.h"
#include "vi.h"
#include "keyboard.h"
#include "display.h"

#include "clist.h"


/*********************************** viEscape ********************************/
/*                                                                           */
/* This boolean variable is set to TRUE when the editor is in command-mode.  */
/* It is set to FALSE when the editor is in input mode.  The editor is       */
/* in input mode by default and an <ESC> puts it into command-mode.  A       */
/* subsequent "i", "a", etc. puts it back into input mode.                   */
/*                                                                           */
/*****************************************************************************/

int viEscape;


/********************************* cList *************************************/
/*                                                                           */
/* This list queues edit commands to be returned to the edit module.  This   */
/* module won't return until there is at least one command in this list.     */
/*                                                                           */
/*****************************************************************************/

static CList *cList;


/********************************** count ************************************/
/*                                                                           */
/* This variable holds the count to prefix the next command with.  It is     */
/* normally set to -1 which means that no count has been specified.  When    */
/* the user types n (a number) in command mode, we set count = n.            */
/*                                                                           */
/*****************************************************************************/

static int count;


/********************************* quoted ************************************/
/*                                                                           */
/* This boolean indicates that the next character to be read was quoted by   */
/* a back-slash.                                                             */
/*                                                                           */
/*****************************************************************************/

static int quoted;


/*********************************** operated ********************************/
/*                                                                           */
/* This boolean variable is set to TRUE to indicate that we just "operated"  */
/* the previous command.  When we recall the next command, we will           */
/* automatically enter command mode so the user can do a string of "o"       */
/* commands.                                                                 */
/*                                                                           */
/*****************************************************************************/

static int operated;


/********************************* extendedCount *****************************/
/*                                                                           */
/* This integer holds the (saved) repeat-count for a command which began an  */
/* extended input.  For example, when the user types the command "3i", we    */
/* save the "3" here, begin the "extended-do", enter his characters, and     */
/* finally, end the extended-do with a count of 3.                           */
/*                                                                           */
/*****************************************************************************/

static int extendedCount;


/******************************* leftInsertMode ******************************/
/*                                                                           */
/* We set this boolean to TRUE when we leave insert mode following a         */
/* keyboardExecute request.  This way, if the keyboardExecute request is     */
/* denied (i.e., there was an error in the command-line), we automatically   */
/* put ourselves back in insert mode.                                        */
/*                                                                           */
/*****************************************************************************/

static int leftInsertMode;


/************************** keepSeparate / skipAdjust ************************/
/*                                                                           */
/* Normally, when ViRead() returns, it will check to see if there are either */
/* more than one command character in the cList or a count specified > 1.    */
/* If either of these are true, ViRead() will automatically glob all         */
/* of the commands together by surrounding them in a do/end-do block.  If    */
/* the "keepSeparate" boolean is set, ViRead() will assume that this         */
/* blocking has already been done -- and not do it.                          */
/*                                                                           */
/* Also, when ViRead() is returns, it usually appends a viAdjust command     */
/* to the end of the cList -- telling the editor to move the cursor back     */
/* one if it is at the end of line.  If the "skipAdjust" boolean is set,     */
/* ViRead() will assume that this is already taken care of -- and not do it. */
/*                                                                           */
/*****************************************************************************/

static int keepSeparate;
static int skipAdjust;


/******************************* PUSH() **************************************/
/*                                                                           */
/* This macro queues an edit command to be returned to the edit module.      */
/*                                                                           */
/*****************************************************************************/

#define PUSH(c)          CListAdd(cList, (c))


/****************************** COUNT() **************************************/
/*                                                                           */
/* This macro returns the implied count associated with the current edit     */
/* command.                                                                  */
/*                                                                           */
/*****************************************************************************/

#define COUNT(c)         (((c) < 0)?(1):(c))


/*****************************************************************************/
/*****************************************************************************/
/*****************************************************************************/
/*****************************************************************************/
/*****************************************************************************/


/****************************** Control() ************************************/
/*                                                                           */
/* This function attempts to process the specified character as a vi         */
/* control character command.  It returns TRUE if successful and FALSE       */
/* otherwise.                                                                */
/*                                                                           */
/*****************************************************************************/

static int Control(c)
int c;
{
  int peek;

  switch (c) {
    case CNTL('H'): PUSH(keyboardBackspace);
                    break;
    case '\\'     : PUSH(c);
                    quoted = TRUE;
                    break;
    case CNTL('V'): keyboardQuote = TRUE;
                    peek = KeyboardPeek();
                    if (KEYBOARDchar(peek)) {
                      if (quoted && ! isprint(peek)) {
                        PUSH(keyboardBackspace);
                      }
                      PUSH(KeyboardRead());
                    } else {
                      if (! KEYBOARDaction(peek)) {
                        DisplayBell();
                      }
                    }
                    break;
    case CNTL('W'): PUSH(keyboardBackspaceBWord);
                    break;
    default       : return FALSE;
  }
  return TRUE;
}


/******************************* Count() *************************************/
/*                                                                           */
/* This function attempts to process the specified character as a vi         */
/* count prefix.  It returns TRUE if successful and FALSE otherwise.  If     */
/* no character is specified, this routine will peek and check for any       */
/* possible count prefixes, returning values as before.                      */
/*                                                                           */
/*****************************************************************************/

static int Count(c)
int c;
{
  int peaked;
  int newCount;

  peaked = FALSE;
  if (! c) {
    c = KeyboardPeek();
    peaked = TRUE;
  }
  if (c == '0' || ! isdigit(KEYBOARDchar(c))) {
    return FALSE;
  }
  if (peaked) {
    c = KeyboardRead();
  }

  newCount = c - '0';
  while (isdigit(KEYBOARDchar(KeyboardPeek()))) {
    newCount = newCount*10 + KeyboardRead() - '0';
  }

  if (count > 0) {
    count *= newCount;
  } else {
    count = newCount;
  }
  return TRUE;
}


/******************************** Move() *************************************/
/*                                                                           */
/* This routine attempts to process the specified character as a vi          */
/* movement command.  It returns TRUE if successful and FALSE otherwise.     */
/*                                                                           */
/*****************************************************************************/

static int Move(c)
int c;
{
  switch (c) {
    case ' '      :
    case 'l'      : PUSH(keyboardForward);
                    break;
    case 'w'      : PUSH(keyboardForwardVWord);
                    break;
    case 'W'      : PUSH(keyboardForwardBWord);
                    break;
    case 'e'      : PUSH(keyboardEndVWord);
                    break;
    case 'E'      : PUSH(keyboardEndBWord);
                    break;
    case CNTL('H'):
    case 'h'      : PUSH(keyboardBackward);
                    break;
    case 'b'      : PUSH(keyboardBackwardVWord);
                    break;
    case 'B'      : PUSH(keyboardBackwardBWord);
                    break;
    case 't'      :
    case 'f'      : if (KEYBOARDchar(KeyboardPeek())) {
                      PUSH(keyboardDo);
                      if (c == 'f') {
                        PUSH(keyboardFindForward);
                      } else {
                        PUSH(keyboardFindForwardVi);
                      }
                      PUSH(KeyboardRead());
                      PUSH(keyboardEndDo);
                      PUSH(COUNT(count));
                      PUSH(keyboardFindAdjust);
                    } else {
                      DisplayBell();
                    }
                    count = -1;
                    break;
    case 'T'      :
    case 'F'      : if (KEYBOARDchar(KeyboardPeek())) {
                      PUSH(keyboardDo);
                      if (c == 'F') {
                        PUSH(keyboardFindBackward);
                      } else {
                        PUSH(keyboardFindBackwardVi);
                      }
                      PUSH(KeyboardRead());
                      PUSH(keyboardEndDo);
                      PUSH(COUNT(count));
                      PUSH(keyboardFindAdjust);
                    } else {
                      DisplayBell();
                    }
                    count = -1;
                    break;
    case ';'      : PUSH(keyboardDo);
                    PUSH(keyboardFindAgain);
                    PUSH(keyboardEndDo);
                    PUSH(COUNT(count));
                    PUSH(keyboardFindAdjust);
                    count = -1;
                    break;
    case ','      : PUSH(keyboardDo);
                    PUSH(keyboardFindReverse);
                    PUSH(keyboardEndDo);
                    PUSH(COUNT(count));
                    PUSH(keyboardFindAdjust);
                    count = -1;
                    break;
    case '0'      : PUSH(keyboardBeginLine);
                    count = -1;
                    break;
    case '^'      : PUSH(keyboardBeginBLine);
                    count = -1;
                    break;
    case '$'      : PUSH(keyboardEndLine);
                    count = -1;
                    break;
    default       : return FALSE;
  }
  return TRUE;
}


/************************** MarkAndMoveCount() *******************************/
/*                                                                           */
/* This routine sets a mark at the current cursor position, moves as         */
/* specified by "c" and "count", and then returns.  "mode" can be either     */
/* "c", "d", or "y" -- depending on whether we are intending to change,      */
/* delete, or yank text.  Note the subtle differences in motion semantics    */
/* for the different modes.                                                  */
/*                                                                           */
/*****************************************************************************/

static int MarkAndMoveCount(mode, c)
int mode;
int c;
{
  int ok;

  PUSH(keyboardSetMark);
  PUSH(keyboardDo);
  if (! (ok = Move(c))) {
    count = -1;
    DisplayBell();
  }
  PUSH(keyboardEndDo);
  PUSH(COUNT(count));
  count = -1;

  if (mode == 'c' && strchr("wW", c)) {
    PUSH(keyboardViCWAdjust);
  } else if ((mode == 'd' || mode == 'c') && strchr("eE;,TtFf", c)) {
    PUSH(keyboardViDFAdjust);
  }

  return ok;
}


/*****************************************************************************/
/*****************************************************************************/
/*****************************************************************************/
/*****************************************************************************/
/*****************************************************************************/


/**************************** SwitchToInputMode() ****************************/
/*                                                                           */
/* This routine switches the vi module to input mode with the specified      */
/* extended repeat-count.  By default, we put the edit module in insert      */
/* (not overwrite) mode.                                                     */
/*                                                                           */
/*****************************************************************************/

static void SwitchToInputMode()
{
  extendedCount = COUNT(count);
  count = -1;
  PUSH(keyboardInsert);
  PUSH(keyboardExtendedDo);
  keepSeparate = TRUE;
  viEscape = FALSE;
}


/***************************** ReturnToCommandMode() *************************/
/*                                                                           */
/* This routine returns the vi module to command mode.  It ends the          */
/* input-mode extended-do block which was started by the SwitchToInputMode() */
/* routine.                                                                  */
/*                                                                           */
/*****************************************************************************/

static void ReturnToCommandMode()
{
  if (! viEscape) {
    PUSH(keyboardEndDo);
    PUSH(extendedCount);
    keepSeparate = TRUE;
    viEscape = TRUE;
  }
  skipAdjust = TRUE;
  count = -1;
}


/*****************************************************************************/
/*****************************************************************************/
/*****************************************************************************/
/*****************************************************************************/
/*****************************************************************************/


/********************************* Escape() **********************************/
/*                                                                           */
/* This routine processes vi-mode command characters.  It returns TRUE if    */
/* successful, FALSE otherwise.                                              */
/*                                                                           */
/*****************************************************************************/

static int Escape(c)
int c;
{
  int cc;
  int mcount;

  if (Move(c)) {
    return TRUE;
  }
  switch (c) {
    case CNTL('P'):
    case '-'      :
    case 'k'      :  PUSH(keyboardPrevious);
                     PUSH(count);
                     count = -1;
                     break;
    case CNTL('N'):
    case '+'      :
    case 'j'      :  PUSH(keyboardNext);
                     PUSH(count);
                     count = -1;
                     break;
    case 'G'      :  PUSH(keyboardGoto);
                     PUSH(count);
                     count = -1;
                     break;
    case '/'      :  PUSH(keyboardSearchBackward);
                     count = -1;
                     SwitchToInputMode();
                     break;
    case '?'      :  PUSH(keyboardSearchForward);
                     count = -1;
                     SwitchToInputMode();
                     break;
    case 'n'      :  PUSH(keyboardSearchAgain);
                     count = -1;
                     break;
    case 'N'      :  PUSH(keyboardSearchReverse);
                     count = -1;
                     break;

    case 'a'      :
    case 'A'      :  SwitchToInputMode();
                     if (c == 'A') {
                       (void)Move('$');
                     } else {
                       PUSH(keyboardViForward);
                     }
                     break;
    case 'i'      :
    case 'I'      :  SwitchToInputMode();
                     if (c == 'I') {
                       (void)Move('0');
                     }
                     break;

    case 'D'      :  cc = '$';
                     goto midd;
    case 'd'      :  (void)Count(0);
                     if (! KEYBOARDchar(KeyboardPeek())) {
                       count = -1;
                       DisplayBell();
                       break;
                     }
                     cc = KeyboardRead();
midd:
                     if (cc == 'd') {
                       PUSH(keyboardDeleteLine);
                     } else if (MarkAndMoveCount('d', cc)) {
                       PUSH(keyboardCut);
                     }
                     count = -1;
                     break;

    case 'C'      :  cc = '$';
                     goto midc;
    case 'S'      :  cc = 'c';
                     goto midc;
    case 'c'      :  (void)Count(0);
                     if (! KEYBOARDchar(KeyboardPeek())) {
                       count = -1;
                       DisplayBell();
                       break;
                     }
                     cc = KeyboardRead();
midc:
                     mcount = count;
                     count = -1;
                     SwitchToInputMode();
                     count = mcount;
                     if (cc == 'c') {
                       PUSH(keyboardDeleteLine);
                     } else if (MarkAndMoveCount('c', cc)) {
                       PUSH(keyboardCut);
                     } else {
                       ReturnToCommandMode();
                     }
                     count = -1;
                     break;

    case 'p'      :  PUSH(keyboardViForward);
                     /* fall thru */
    case 'P'      :  PUSH(keyboardPaste);
                     PUSH(keyboardViBackward);
                     break;

    case 'R'      :  SwitchToInputMode();
                     PUSH(keyboardOverwrite);
                     break;
    case 'r'      :  if (KEYBOARDchar(KeyboardPeek())) {
                       PUSH(keyboardOverwrite);
                       PUSH(KeyboardRead());
                       PUSH(keyboardViBackward);
                     } else {
                       count = -1;
                       DisplayBell();
                     }
                     break;

    case 'x'      :  PUSH(keyboardDelete);
                     break;
    case 'X'      :  PUSH(keyboardBackspace);
                     break;

    case 'v'      :  PUSH(keyboardFC);
                     PUSH(count);
                     count = -1;
                     break;

    case '.'      :  PUSH(keyboardRedo);
                     break;
    case '~'      :  PUSH(keyboardFlipCase);
                     break;
    case '_'      :  PUSH(keyboardForward);
                     PUSH(keyboardParam);
                     PUSH(count);
                     count = -1;
                     SwitchToInputMode();
                     break;

    case '\\'     :
    case ESC      :  PUSH(keyboardExpandOne);
                     count = -1;
                     SwitchToInputMode();
                     break;
    case '*'      :  PUSH(keyboardExpandAll);
                     count = -1;
                     SwitchToInputMode();
                     break;
    case '='      :  PUSH(keyboardExpandList);
                     count = -1;
                     break;

    case 'Y'      :  cc = '$';
                     goto midy;
    case 'y'      :  (void)Count(0);
                     if (! KEYBOARDchar(KeyboardPeek())) {
                       count = -1;
                       DisplayBell();
                       break;
                     }
                     cc = KeyboardRead();
midy:
                     PUSH(keyboardSetTempMark);
                     if (cc == 'y') {
                       (void)Move('0');
                       (void)MarkAndMoveCount('y', '$');
                       PUSH(keyboardCopy);
                       PUSH(keyboardExchangeTemp);
                     } else if (MarkAndMoveCount('y', cc)) {
                       PUSH(keyboardCopy);
                       PUSH(keyboardExchangeTemp);
                     }
                     count = -1;
                     break;

    case 'u'      :  PUSH(keyboardUndo);
                     break;
    case 'U'      :  PUSH(keyboardUndoAll);
                     count = -1;
                     break;

    case '#'      :  PUSH(keyboardBeginLine);
                     PUSH(keyboardInsert);
                     PUSH('#');
                     PUSH(keyboardExecute);
                     count = -1;
                     keepSeparate = TRUE;
                     break;
    case 'o'      :  PUSH(keyboardOperate);
                     count = -1;
                     keepSeparate = TRUE;
                     break;
    case CNTL('V'):  DisplayHint(HPUX_ID, TRUE);
                     count = -1;
                     break;
    default       :  count = -1;
                     DisplayBell();
                     return FALSE;
  }
  return TRUE;
}


/*****************************************************************************/
/*****************************************************************************/
/*****************************************************************************/
/*****************************************************************************/
/*****************************************************************************/


/***************************** ViInitialize() ********************************/
/*                                                                           */
/* This routine performs local initialization of the vi module.              */
/*                                                                           */
/*****************************************************************************/

void ViInitialize()
{
  cList = CListCreate();
  /* emacs will initialize keyboard module */
}


/***************************** ViBegin() *************************************/
/*                                                                           */
/* This routine performs pre-line initialization for the vi module.          */
/*                                                                           */
/*****************************************************************************/

void ViBegin()
{
  assert(CListCount(cList) == 0);

  count = -1;
  quoted = FALSE;
  leftInsertMode = FALSE;
  SwitchToInputMode();
  KeyboardBegin();
}


/********************************* ViRead() **********************************/
/*                                                                           */
/* This routine reads characters from the keyboard module and translates     */
/* any vi-mode commands it sees into generic editing commands understood     */
/* by the edit module.  It returns as soon as an editing command is          */
/* available.  Extra editing commands are queued in cList until this         */
/* routine is called again.                                                  */
/*                                                                           */
/*****************************************************************************/

int ViRead()
{
  int c;
  int action;

  /* if any editing commands were already queued, return the next one */
  if (c = CListRemove(cList)) {
    return c;
  }

  /* then default to a count of -1 (which is really a count of 1...) */
  count = -1;

  /* by default we will adjust the cursor following every vi command we */
  /* send to the edit module.  we'll also group multiple edit commands */
  /* into a common "do" block. */
  skipAdjust = FALSE;
  keepSeparate = FALSE;


  /* then keep processing keyboard characters until we have an editing */
  /* command to return.  for the character following a "\", pretend */
  /* that the user preceeded it with a ^V to quote it just like any other */
  /* vi quoted char. */
  do {
    if (quoted) {
      (void)Control(CNTL('V'));
      quoted = FALSE;
    } else {
      c = KeyboardRead();

      /* if we left insert mode because the user pressed <Return>, */
      /* but the return was not verified (i.e., syntax error occurred), */
      /* then quietly return us to insert mode so the user can continue */
      /* to type. */
      if (leftInsertMode && ! KEYBOARDverify(c)) {
        leftInsertMode = FALSE;
        (void)Escape('i');

      /* otherwise if we "operated" the previous command, then return */
      /* usd to command mode so that the user can type a string of */
      /* "o"s and operate a sequence of history lines. */
      } else if (operated) {
        ReturnToCommandMode();
        operated = FALSE;
      }

      /* if we have a special character (e.g., not ascii), then check */
      /* if it is a <Return> request.  If so, leave command-mode for */
      /* now -- but be prepaired to return back if the command translation */
      /* fails.  Otherwise, if we just verified an "operate" operation, */
      /* set a flag so that the recalled command will jump immediately */
      /* to command-mode.  then pass the special char to the edit module */
      /* to be processed. */
      if (! KEYBOARDchar(c)) {
        count = -1;
        if (KEYBOARDaction(c)) {
          leftInsertMode = ! viEscape;
          ReturnToCommandMode();
        } else if (KEYBOARDverify(c)) {
          skipAdjust = TRUE;
          if (c == keyboardOperateVerify) {
            operated = TRUE;
          }
        } else if (c == keyboardEndInputMode) {
          ReturnToCommandMode();
        }
        PUSH(c);

      /* otherwise, if we are in command mode, process the command char */
      } else if (viEscape) {
        if (! Count(c) && ! Escape(c)) {
          DisplayBell();
        }

      /* otherwise, we are in input mode.  if the user pressed <ESC>, */
      /* then enter command-mode and bump the cursor back one. */
      } else if (c == ESC) {
        PUSH(keyboardViBackward);
        ReturnToCommandMode();

      /* otherwise, if this is not a special insert-mode control char, */
      /* just send the char to the edit module as-is to be inserted into */
      /* the command-line. */
      } else if (! Control(c)) {
        PUSH(c);
      }
    }
  } while (! CListPeek(cList));

  /* then check to see if the user specified a repeat-count for the */
  /* command and the keepSeparate flag is not set -- if so, surround */
  /* the command in a do-enddo block with the specified count. */
  /* otherwise, return the command by itself. */
  if (CListPeek(cList) == keyboardUndo || CListPeek(cList) == keyboardRedo) {
    PUSH(COUNT(count));
    action = CListRemove(cList);
  } else if (! keepSeparate && (count > 1 || CListCount(cList) > 1)) {
    PUSH(keyboardEndDo);
    PUSH(COUNT(count));
    action = keyboardDo;
  } else {
    action = CListRemove(cList);
  }

  /* then make sure the cursor is not past the eol in command-mode */
  if (viEscape && ! skipAdjust) {
    PUSH(keyboardViAdjust);
  }
  return action;
}


/********************************* ViAvail() *********************************/
/*                                                                           */
/* This routine returns non-zero if a keyboard character is pending to       */
/* be read.  Otherwise it returns 0.                                         */
/*                                                                           */
/*****************************************************************************/

int ViAvail()
{
  if (! viEscape) {
    return KeyboardAvail();
  }
  return FALSE;
}


/******************************** ViEnd() ************************************/
/*                                                                           */
/* This module performs post-line clean-up for the vi module.                */
/*                                                                           */
/*****************************************************************************/

void ViEnd()
{
  KeyboardEnd();
}


/*****************************************************************************/
