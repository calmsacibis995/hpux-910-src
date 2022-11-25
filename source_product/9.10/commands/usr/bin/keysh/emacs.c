/*
static char rcsId[] =
  "@(#) $Header: emacs.c,v 66.10 91/01/01 15:15:11 rpt Exp $";
static char copyright[] =
  "Copyright (c) HP, 1989";
*/


/********************************* emacs.c ***********************************/
/*                                                                           */
/* This module is responsible for translating emacs edit commands received   */
/* from the keyboard module into generic edit commands understood by the     */
/* edit module.                                                              */
/*                                                                           */
/*****************************************************************************/


#include <assert.h>
#include <ctype.h>
#include <signal.h>

#include "keyshell.h"
#include "edit.h"
#include "emacs.h"
#include "keyboard.h"
#include "display.h"

#include "clist.h"


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
/* the user types <ESC>n, (where n is a number), we set count = n.           */
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


/******************************* PUSH() **************************************/
/*                                                                           */
/* This macro queues an edit command to be returned to the edit module.      */
/*                                                                           */
/*****************************************************************************/

#define PUSH(c)      CListAdd(cList, (c))


/****************************** COUNT() **************************************/
/*                                                                           */
/* This macro returns the implied count associated with the current edit     */
/* command.                                                                  */
/*                                                                           */
/*****************************************************************************/

#define COUNT(c)     (((c) < 0)?(1):(c))


/*****************************************************************************/
/*****************************************************************************/
/*****************************************************************************/
/*****************************************************************************/
/*****************************************************************************/


/****************************** Control() ************************************/
/*                                                                           */
/* This function attempts to process the specified character as en emacs     */
/* control character command.  It returns TRUE if successful and FALSE       */
/* otherwise.                                                                */
/*                                                                           */
/*****************************************************************************/

static int Control(c)
int c;
{
  int peek;

  switch (c) {
    case CNTL('A'): PUSH(keyboardBeginLine);
                    count = -1;
                    break;
    case CNTL('B'): PUSH(keyboardBackward);
                    break;
    case CNTL('C'): PUSH(keyboardUpperCase);
                    break;
    case CNTL('D'): PUSH(keyboardSetMark);
                    PUSH(keyboardDelete);
                    break;
    case CNTL('E'): PUSH(keyboardEndLine);
                    count = -1;
                    break;
    case CNTL('F'): PUSH(keyboardForward);
                    break;
    case CNTL('G'): raise(SIGINT);
                    count = -1;
                    break;
    case CNTL('H'): PUSH(keyboardBackspace);
                    PUSH(keyboardSetMark);
                    break;
    case CNTL('K'): PUSH(keyboardSetMark);
                    PUSH(keyboardClearEOL);
                    count = -1;
                    break;
    case CNTL('N'): PUSH(keyboardNext);
                    PUSH(count);
                    count = -1;
                    break;
    case CNTL('O'): PUSH(keyboardOperate);
                    count = -1;
                    break;
    case CNTL('P'): PUSH(keyboardPrevious);
                    PUSH(count);
                    count = -1;
                    break;
    case '\\'     : PUSH(c);
                    quoted = TRUE;
                    count = -1;
                    break;
    case CNTL('Q'): keyboardQuote = TRUE;
                    peek = KeyboardPeek();
                    if (KEYBOARDchar(peek)) {
                      if (quoted && ! isprint(peek)) {
                        PUSH(keyboardBackspace);
                      }
                      PUSH(KeyboardRead());
                    } else {
                      count = -1;
                      if (! KEYBOARDaction(peek)) {
                        DisplayBell();
                      }
                    }
                    break;
    case CNTL('R'): PUSH(keyboardSearchBackward);
                    count = -1;
                    break;
    case CNTL('S'): PUSH(keyboardSearchForward);
                    count = -1;
                    break;
    case CNTL('T'): PUSH(keyboardForward);
                    PUSH(keyboardSetTempMark);
                    PUSH(keyboardBackward);
                    PUSH(keyboardCutTemp);
                    PUSH(keyboardBackward);
                    PUSH(keyboardPasteTemp);
                    PUSH(keyboardForward);
                    break;
    case CNTL('U'): if (count < 0) {
                      count = 4;
                    } else {
                      count *= 4;
                    }
                    break;
    case CNTL('V'): DisplayHint(HPUX_ID, TRUE);
                    count = -1;
                    break;
    case CNTL('W'): PUSH(keyboardCut);
                    count = -1;
                    break;
    case CNTL('Y'): PUSH(keyboardSetMark);
                    PUSH(keyboardPaste);
                    break;
    case CNTL(']'): if (KEYBOARDchar(KeyboardPeek())) {
                      PUSH(keyboardFindForward);
                      PUSH(KeyboardRead());
                    } else {
                      count = -1;
                      DisplayBell();
                    }
                    break;
    case CNTL('_'): PUSH(keyboardUndo);
                    break;
    case DEL:       PUSH(keyboardBackspace);
                    PUSH(keyboardSetMark);
                    break;
    default:        return FALSE;
  }
  return TRUE;
}


/******************************* Count() *************************************/
/*                                                                           */
/* This function attempts to process the specified character as an emacs     */
/* count specification (of the form <ESC>n).  It returns TRUE if successful  */
/* and FALSE otherwise.                                                      */
/*                                                                           */
/*****************************************************************************/

static int Count(c)
int c;
{
  if (c != ESC || ! isdigit(KEYBOARDchar(KeyboardPeek()))) {
    return FALSE;
  }
  do {
    c = KeyboardRead();
    if (count <= 0) {
      count = c - '0';
    } else {
      count = count*10 + c - '0';
    }
  } while (isdigit(KEYBOARDchar(KeyboardPeek())));
  return TRUE;
}


/******************************* Escape() ************************************/
/*                                                                           */
/* This function attempts to process the specified character (and the one    */
/* which follows) as an emacs escape command.  It returns TRUE if successful */
/* and FALSE otherwise.                                                      */
/*                                                                           */
/*****************************************************************************/

static int Escape(c)
int c;
{
  if (c != ESC) {
    return FALSE;
  }
  if (! KEYBOARDchar(KeyboardPeek())) {
    count = -1;
    DisplayBell();
    return TRUE;
  }
  c = KeyboardRead();
  switch (c) {
    case CNTL(']'): if (KEYBOARDchar(KeyboardPeek())) {
                      PUSH(keyboardFindBackward);
                      PUSH(KeyboardRead());
                    } else {
                      count = -1;
                      DisplayBell();
                    }
                    break;

    case '\\'     :
    case ESC      : PUSH(keyboardSetMark);
                    PUSH(keyboardExpandOne);
                    count = -1;
                    break;
    case '*'      : PUSH(keyboardSetMark);
                    PUSH(keyboardExpandAll);
                    count = -1;
                    break;
    case '='      : PUSH(keyboardSetMark);
                    PUSH(keyboardExpandList);
                    count = -1;
                    break;

    case '#'      : PUSH(keyboardBeginLine);
                    PUSH(keyboardInsert);
                    PUSH('#');
                    PUSH(keyboardExecute);
                    count = -1;
                    break;

    case '<'      : PUSH(keyboardFirst);
                    count = -1;
                    break;
    case '>'      : PUSH(keyboardLast);
                    count = -1;
                    break;

    case '_'      :
    case '.'      : PUSH(keyboardSetMark);
                    PUSH(keyboardParam);
                    PUSH(count);
                    count = -1;
                    break;

    case ' '      : PUSH(keyboardSetMark);
                    count = -1;
                    break;

    case 'b'      : PUSH(keyboardBackwardWord);
                    break;
    case 'c'      : PUSH(keyboardCapitalWord);
                    break;
    case 'd'      : PUSH(keyboardSetMark);
                    PUSH(keyboardDeleteWord);
                    break;
    case 'f'      : PUSH(keyboardEndWord);
                    PUSH(keyboardForward);
                    break;
    case DEL      :
    case 'h'      :
    case CNTL('H'): PUSH(keyboardBackspaceWord);
                    PUSH(keyboardSetMark);
                    break;
    case 'l'      : PUSH(keyboardLowerCaseWord);
                    break;
    case 'p'      :
    case 'w'      : PUSH(keyboardCopy);
                    count = -1;
                    break;
    case 'u'      : PUSH(keyboardUpperCaseWord);
                    break;

    case 'v'      : PUSH(keyboardFC);
                    PUSH(count);
                    count = -1;
                    break;

    default       : DisplayBell();
                    count = -1;
                    break;
  }
  return TRUE;
}


/******************************* ControlX() **********************************/
/*                                                                           */
/* This function attempts to process the specified character (and the one    */
/* which follows) as an emacs control-X command.  It returns TRUE if         */
/* successful and FALSE otherwise.                                           */
/*                                                                           */
/*****************************************************************************/

static int ControlX(c)
int c;
{
  if (c != CNTL('X')) {
    return FALSE;
  }
  if (! KEYBOARDchar(KeyboardPeek())) {
    count = -1;
    DisplayBell();
    return TRUE;
  }
  c = KeyboardRead();
  switch (c) {
    case CNTL('X'): PUSH(keyboardExchange);
                    count = -1;
                    break;
    case 'u'      : PUSH(keyboardUndo);
                    break;
    default       : DisplayBell();
                    count = -1;
                    break;
  }
  return TRUE;
}


/*****************************************************************************/
/*****************************************************************************/
/*****************************************************************************/
/*****************************************************************************/
/*****************************************************************************/


/**************************** EmacsInitialize() ******************************/
/*                                                                           */
/* This routine performs local initialization of the emacs module.           */
/*                                                                           */
/*****************************************************************************/

void EmacsInitialize()
{
  cList = CListCreate();
  KeyboardInitialize();
}


/**************************** EmacsBegin() ***********************************/
/*                                                                           */
/* This routine performs pre-line initialization for the emacs module.       */
/*                                                                           */
/*****************************************************************************/

void EmacsBegin()
{
  assert(CListCount(cList) == 0);

  quoted = FALSE;
  KeyboardBegin();
}


/****************************** EmacsRead() **********************************/
/*                                                                           */
/* This routine reads characters from the keyboard module and translates     */
/* any emacs-mode commands it sees into generic editing commands understood  */
/* by the edit module.  It returns as soon as an editing command is          */
/* available.  Extra editing commands are queued in cList until this         */
/* routine is called again.                                                  */
/*                                                                           */
/*****************************************************************************/

int EmacsRead()
{
  int c;

  /* if any editing commands were already queued, return the next one */
  if (c = CListRemove(cList)) {
    return c;
  }

  /* then default to a count of -1 (which is really a count of 1...) */
  count = -1;

  /* then keep processing keyboard characters until we have an editing */
  /* command to return.  for the character following a "\", pretend */
  /* that the user preceeded it with a ^Q to quote it just like any other */
  /* emacs quoted char. */
  do {
    if (quoted) {
      (void)Control(CNTL('Q'));
      quoted = FALSE;
    } else {
      c = KeyboardRead();

      /* if this is a special (non-ascii) char, then send it directly to */
      /* the edit module.  otherwise, try to process it as an emacs command */
      /* char.  if it isn't an emacs command char, then send it to the */
      /* edit module to be inserted into the command-line. */
      if (! KEYBOARDchar(c)) {
        count = -1;
        PUSH(c);
      } else if (! Count(c) && ! Control(c) && ! Escape(c) && ! ControlX(c)) {
        PUSH(c);
      }
    }
  } while (! CListPeek(cList));

  /* then check to see if the user specified a repeat-count for the */
  /* command -- if so, surround the command in a do-enddo block with the */
  /* specified count.  otherwise, return the command by itself. */
  if (CListPeek(cList) == keyboardUndo) {
    PUSH(COUNT(count));
  } else if (count > 1 || CListCount(cList) > 1) {
    PUSH(keyboardEndDo);
    PUSH(COUNT(count));
    return keyboardDo;
  }
  return CListRemove(cList);
}


/******************************** EmacsAvail() *******************************/
/*                                                                           */
/* This routine returns non-zero if a keyboard character is pending to       */
/* be read.  Otherwise it returns 0.                                         */
/*                                                                           */
/*****************************************************************************/

int EmacsAvail()
{
  return KeyboardAvail();
}


/******************************** EmacsEnd() *********************************/
/*                                                                           */
/* This module performs post-line clean-up for the emacs module.             */
/*                                                                           */
/*****************************************************************************/

void EmacsEnd()
{
  KeyboardEnd();
}


/*****************************************************************************/
