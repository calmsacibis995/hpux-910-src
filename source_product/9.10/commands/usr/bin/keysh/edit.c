/*
static char rcsId[] =
  "@(#) $Header: edit.c,v 66.11 90/11/13 13:18:56 rpt Exp $";
static char copyright[] =
  "Copyright (c) HP, 1989";
*/


/********************************** edit.c ***********************************/
/*                                                                           */
/* This is by far the most complex keysh module.  It implements a            */
/* full-function line-editor capable of supporting both emacs and vi         */
/* editing modes, as well as a wealth of others.  Note that this is the      */
/* only module in keysh which actually performs editing functions on         */
/* the command-line -- the emacs and vi modules *only* translate user        */
/* keystrokes into corresponding editing commands.  The editing commands     */
/* are always executed here.                                                 */
/*                                                                           */
/* Basic editing commands understood by this module are represented by       */
/* the "KeyboardChar" enumerated type.  These commands are all either one    */
/* or two integers long, with the second integer being the command argument. */
/* These basic commands can then be grouped into more complex "blocks" --    */
/* each composed basically as follows:                                       */
/*                                                                           */
/*   do {                                                                    */
/*     <basic editing command>                                               */
/*     .                                                                     */
/*     .                                                                     */
/*   } end-do <repeat count>                                                 */
/*                                                                           */
/* Note that these blocks may themselves be nested, allowing for very        */
/* complex operation sequences.  Any *unblocked* commands received by this   */
/* module are effectively executed in their own block with a <repeat count>  */
/* of 1.                                                                     */
/*                                                                           */
/* These blocks have very special meaning to the edit module.  They          */
/* represent the smallest unit of editing which can be "undone" using the    */
/* vi "u" command or emacs "^_" command.  They also represent the unit       */
/* of editing to be repeated by the vi "." command.                          */
/*                                                                           */
/* In general, a "do" block is processed atomically by this module -- no     */
/* intermediate results will be returned until the entire block has been     */
/* read, executed, and repeated the specified number of times.  There is     */
/* one exception to this rule (which makes this module so complex!), the     */
/* "extended-do" block.                                                      */
/*                                                                           */
/* The problem (like all problems in this module...), results from the       */
/* semantics of vi input mode.  When the user presses "i", he enters         */
/* input mode.  He then can type anything he likes, finally ending with      */
/* an <ESC>.  If he then types "u", we undo everything he input.  If he      */
/* types ".", we repeat everything he input.  Effectively, "i" is a          */
/* command just like "dw" or "x", except that the user *does* need           */
/* intermediate feedback (e.g., the display must update, the softkeys must   */
/* update, etc.).  Also, the user can type "3i", just like "3x" and we       */
/* need to repeat the command 3 times!                                       */
/*                                                                           */
/* So, an "extended-do" block is like a "do" block, except that it *can*     */
/* return before the entire block has been processed.  When this module      */
/* is called again, it will continue processing the extended-do block from   */
/* exactly where it left off.  Note that once the "end-do" has been          */
/* read, the remaining itterations (i.e., the repeat count) of the block     */
/* are executed immediately since they will complete in a finite time        */
/* (without ever blocking for user input).                                   */
/*                                                                           */
/* Two examples will probably be useful here:                                */
/*                                                                           */
/* In vi command mode, when the user types "3ihello<ESC>", the following     */
/* occurs:                                                                   */
/*                                                                           */
/*   When the user types "3", the vi module simply saves the repeat count    */
/*   in a local static integer.  When the user types "i", the vi module      */
/*   sends the "extended-do" command to the edit module.  The edit module    */
/*   (realizing that it has nothing to return), attempts to read (and cache) */
/*   the next edit command in the extended-do block.  This causes the        */
/*   edit module to block until the user types the next letter ("h").        */
/*   At this time, the edit module (realizing                                */
/*   that it *has* something to return) temporarily abandons processing      */
/*   the extended-do block, and returns an "h" to the word module.  When     */
/*   the word module eventually calls for another character, the edit        */
/*   module continues processing the extended-do block from where it left    */
/*   off.  Again (since it has nothing to return), the edit module blocks    */
/*   waiting for a character from the vi module.  This process continues     */
/*   until the user presses <ESC>.  At this time, the vi module returns      */
/*   the "end-do" command along with the repeat count (3) saved from         */
/*   before.  The edit module has cached the entire extended-do block,       */
/*   so it immediately processes the remaining 2 itterations (since it       */
/*   already processed 1 itteration in the process of caching) of            */
/*   "ihello<ESC>".  It returns resulting characters on this and the next    */
/*   nine subsequent calls from the word module.                             */
/*                                                                           */
/* In emacs mode, when the user types "^U^F", the following occurs:          */
/*                                                                           */
/*   When the user types "^U", the emacs module saves the count (4) in a     */
/*   local static integer.  When the user subsequently types "^F", the       */
/*   emacs module returns the sequence "do", "move-right",                   */
/*   "end-do 4" -- where "4" is the repeat-count.  The edit module begins    */
/*   reading this "do" block.  It reads (and executes!) commands until       */
/*   it finds the corresponding "end-do".  At that time, it realizes that    */
/*   3 more itterations of the "do" block still need to be performed, so     */
/*   it immediately does so (before returning to the word module).  The      */
/*   four characters moved right over will be returned to the word           */
/*   module on this and the next three subsequent calls to the edit module.  */
/*                                                                           */
/*****************************************************************************/


#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <sys/types.h>

#include "keyshell.h"
#include "global.h"
#include "edit.h"
#include "emacs.h"
#include "vi.h"
#include "keyboard.h"
#include "display.h"
#include "debug.h"

#include "kshhooks.h"
#include "clist.h"
#include "string2.h"


/*************************** editMode / editOverwrite ************************/
/*                                                                           */
/* These variables define the major and minor editing modes, respectively.   */
/* "editMode" can be either EDITemacs, EDITvi, or EDITraw.  "editOverwrite"  */
/* is a boolean which indicates that normally typed text will overwrite      */
/* (replace) text on the command-line, rather than inserting into the middle */
/* of it.                                                                    */
/*                                                                           */
/*****************************************************************************/

EditMode editMode;
int editOverwrite;


/********************** editLine / editIndex / editEOL ***********************/
/*                                                                           */
/* These variables completely define the current state of the command-line.  */
/* "editLine" is the character buffer representing the command-line          */
/* itself.  "editIndex" is the index of the current cursor position in the   */
/* command-line.  "editEOL" is the index of the null-character at the end of */
/* the command-line (i.e., editEOL == strlen(editLine), only quicker).       */
/*                                                                           */
/*****************************************************************************/

char editLine[EDITlength];
int editIndex;
int editEOL;


/*************************** leftMargin / hardLeftMargin *********************/
/*                                                                           */
/* "hardLeftMargin" is the index of the first "real" character in the        */
/* command-line entered by the user.  Ordinarily, this is "0" (e.g.,         */
/* the first char); however, when recalling a history command in vi or       */
/* emacs modes, a "/" or "^R" will be forced into the first char and         */
/* this variable will be set to "1".  The user then enters characters        */
/* starting with the second.  Note that the user cannot backspace over       */
/* the "/" or "^R" (without terminating the history recall) or edit them.    */
/*                                                                           */
/* "leftMargin" can be set and cleared (using the keyboardSetMargin and      */
/* keyboardClearMargin editing commands) to *temporarily* set a left         */
/* margin which the user cannot exceed.  This might be used to implement     */
/* the disgusting vi semantics of not being able to back up in front of      */
/* where you started "i"nserting or "a"ppending text -- unfortunately,       */
/* this makes the keyboard white keys (e.g., <Delete line>) behave           */
/* similarly.                                                                */
/*                                                                           */
/*****************************************************************************/

static int leftMargin;
static int hardLeftMargin;


/************************* undoCList / undoLine / undoCurrent ****************/
/*                                                                           */
/* These variables contain all of the "undo" information for the current     */
/* command-line.  "undoCList" is a list of actions which, if applied in      */
/* sequence, will restore the command-line back to its original (empty)      */
/* state.  Each of these actions represents one character's worth of         */
/* work as performed by the lowest level Insert() and Delete() routines.     */
/* The actions are stored as "Undo" records, each comprised of an            */
/* insert/delete flag, a cursor position, and (if applicable) the character  */
/* to be inserted.  "undoCurrent" is the index of the last "Undo" record     */
/* to be undone (each time the user does a consecutive "u", we keep          */
/* undoing from where he left off; once he does anything else, we move back  */
/* to the end of the endoCList).                                             */
/*                                                                           */
/* "undoLine" is used by the vi-mode "U" command.  Basically it is set to    */
/* the initial input sequence (prior to the first <ESC>) of characters       */
/* entered by the user.                                                      */
/*                                                                           */
/*****************************************************************************/

static CList *undoCList;
static char undoLine[EDITlength];
static int undoCurrent;


/********************* doCList / altDoCList / redoCList **********************/
/*                                                                           */
/* Whenever a "do-block" is read from a lower-level module to be executed,   */
/* it is cached in the "doCList".  If the do-block sebsequently turns out    */
/* to have a <repeat count> greater than one, it will be "reread" from       */
/* the doCList and reexecuted as many times as needed.                       */
/*                                                                           */
/* Whenever a do-block modifies the command-line (i.e., newDo == FALSE),     */
/* the doCList (containing the do-block) and redoCList are swapped.  In      */
/* this way, the redoCList always contains the edit commands for the last    */
/* vi-mode text-modification command.  The vi-mode "." command then simply   */
/* causes the redoCList to be reread.                                        */
/*                                                                           */
/* "altDoCList" is a kludge.  Basically, when the user tries to recall a     */
/* history line (e.g., "/" in vi mode or "^R" in emacs mode), this module    */
/* calls itself recursively!  The original value of "doCList" is saved and   */
/* this variable is used temporarily to allow the user to edit the           */
/* "alternate" command-line.  When the user presses <Return>, we restore     */
/* the original doCList and update the command-line.                         */
/*                                                                           */
/*****************************************************************************/

static CList *doCList;
static CList *altDoCList;
static CList *redoCList;


/*************************** cList / altCList ********************************/
/*                                                                           */
/* Whenever the cursor moves left or right, we add a virtual character to    */
/* the "cList" -- either a backspace or the character just entered.  These   */
/* virtual characters are what will be returned to the word module.  Note    */
/* that as far as this module is concerned, moving the cursor left over a    */
/* character is just like backspacing over it; moving right over a char      */
/* is just like retyping it.  Special "action" characters (e.g., <Return>)   */
/* and special "verified action" characters can also be added to this list.  */
/*                                                                           */
/* "altCList" is a kludge like "altDoCList" -- and is used in the same way.  */
/* Again, these variables exist because we recurse in the edit module when   */
/* the user types a "/" or "^R" command (ksh doesn't allow any editing in    */
/* this case -- we do).                                                      */
/*                                                                           */
/*****************************************************************************/

static CList *cList;
static CList *altCList;


/**************************** newDo / newCopy ********************************/
/*                                                                           */
/* Both of these booleans are set to TRUE at the beginning of each do-block. */
/*                                                                           */
/* Whenever an edit command modifies the command-line, it saves the          */
/* associated "undo" information in the undoCList along with the current     */
/* value of "newDo" and then sets "newDo" to FALSE.  When the user then      */
/* issues a "u" command, we undo (backwards) elements in the undoCList       */
/* until we find one which had newDo == TRUE -- indicating the beginning     */
/* of a do-block.                                                            */
/*                                                                           */
/* Whenever an edit command copies text into one of the yank/put buffers,    */
/* it checks the nalue of newCopy.  If TRUE, the yank/put buffers are        */
/* emptied since this is the first iteration of a do-block.  Then "newCopy"  */
/* is set to FALSE and the associated text is either appended or prepended   */
/* to the yank/put buffer.  In this way, when the user types "37X" in vi     */
/* or "<ESC>37^H" in emacs, all 37 deleted characters will end up in the     */
/* yank/put buffers.                                                         */
/*                                                                           */
/*****************************************************************************/

static int newDo;
static int newCopy;


/************************* abortFlag / ringBell ******************************/
/*                                                                           */
/* Both of these booleans are set to FALSE at the beginning of each          */
/* do-block.  If any errors occur while editing (e.g., moving left beyond    */
/* the beginning of the line, etc.), these flags are set.  "ringBell"        */
/* causes the bell to be rung as soon as possible -- and is then reset       */
/* to FALSE (allowing the bell to ring more than once in an extended-do      */
/* block).  "abortFlag", once set, causes the remaining iterations (as       */
/* specified by the <repeat count>) of the do-block to be foregone.          */
/*                                                                           */
/*****************************************************************************/

static int abortFlag;
static int ringBell;


/************************** recentLine / operate *****************************/
/*                                                                           */
/* "recentLine" is the history line number of the last history command to    */
/* be executed.  It is reset to the "current" history line number whenever   */
/* the user enters a command directly.  It is to identify the next line      */
/* to operate or the first line to recall in the case where the user presses */
/* <down arrow> before pressing <up arrow> to recall a history line.         */
/*                                                                           */
/* "operate" is a boolean which is set TRUE when the user "operates" a line. */
/* This module will automatically perform the <down-arrow> function for      */
/* him (thus recalling the *next* history command in sequence) the next      */
/* time it is called.  This boolean is only set when a "operateVerify"       */
/* action is processed by this module.                                       */
/*                                                                           */
/*****************************************************************************/

static int recentLine;
static int operate;


/******************************** Undo ***************************************/
/*                                                                           */
/* This structure describes "Undo" records stored in the undoCList.  Note    */
/* that sizeof(Undo) must equal sizeof(int)!  The fields in this structure   */
/* are:                                                                      */
/*                                                                           */
/*   begin   - a boolean which indicates that this operation corresponds     */
/*             to the beginning of a do-block (i.e., newDo).                 */
/*                                                                           */
/*   insert  - either '\0' (indicating a delete operation) or the character  */
/*             to be inserted in the command-line to effect the undo         */
/*             operation.                                                    */
/*                                                                           */
/*   index   - the index into the editLine where the undo operation is to    */
/*             be performed.                                                 */
/*                                                                           */
/*****************************************************************************/

typedef union Undo {
  int c;
  struct {
    char begin;
    char insert;
    short index;
  } undo;
} Undo;


/************************* mark / tempMark / altMark *************************/
/*                                                                           */
/* These indexes identify the primary mark, temporary mark, and alternate    */
/* marks in the command-line.  (A mark is just a memembered cursor position  */
/* to which the editor can easily cut or copy/yank text.)                    */
/* The primary mark is available to the user in emacs mode.  All other       */
/* marks are used by the editing modules themselves.                         */
/*                                                                           */
/*****************************************************************************/

static int mark;
static int tempMark;
static int altMark;


/********************************** buffer[] *********************************/
/*                                                                           */
/* These two buffers are used to hold cut/yanked/copied text from the        */
/* command-line.  The main buffer is available to the user thru the "d",     */
/* "y", "p", etc. commands in vi mode and the "^K" and "^Y" commands in      */
/* emacs mode.  The temp buffer is used by the editing modules themselves.   */
/*                                                                           */
/*****************************************************************************/

#define BUFFERmain  0
#define BUFFERtemp  1
static char buffer[2][EDITlength];


/*********************** extendedDo... / extendedVi... ***********************/
/*                                                                           */
/* These variables are used by the DoBlock() routine to save its internal    */
/* state when it returns prematurely in an extended-do block.  When          */
/* subsequently called again, DoBlock() wil restore its state from these     */
/* variables and continue from where it left off.  Specifically, these       */
/* these variables hold:                                                     */
/*                                                                           */
/*   extendedDo         - boolean set to TRUE if we are processing an        */
/*                        extended-do block.  If true, DoBlock() will save   */
/*                        and restore its state when called.                 */
/*                                                                           */
/*   extendedDoNext     - the index of the next edit command in the doCList  */
/*                        to be executed.                                    */
/*                                                                           */
/*   extendedViForward  - boolean set to TRUE if we have seen and processed  */
/*                        the initial viForward command for this do-block.   */
/*                                                                           */
/*   extendedViBackward - boolean set to true if we have seen a terminal     */
/*                        viBackward command for this do-block.              */
/*                                                                           */
/* Note that the viForward and viBackward commands are special of all of     */
/* the editing commands in that they are at most executed *once* (each) per  */
/* do-block, irrespective of the do-block's <repeat count>.                  */
/*                                                                           */
/* In particular, the viForward command is only processed the first time     */
/* it is seen (and is ignored on all other occasions); alternately, the      */
/* viBackward command is only processed the last time it is seen (and again, */
/* is ignored on all other occasions).  This allows commands like "p" and    */
/* "3p" to be processed accurately like:                                     */
/*                                                                           */
/*    p = do { viForward, paste, viBackward } repeat 1                       */
/*                                                                           */
/*      = viForward, paste, viBackward                                       */
/*                                                                           */
/* and:                                                                      */
/*                                                                           */
/*   3p = do { viForward, paste, viBackward } repeat 3                       */
/*                                                                           */
/*      = viForward, paste, paste, paste, viBackward                         */
/*                                                                           */
/*****************************************************************************/

static int extendedDo;
static int extendedDoNext;
static int extendedViForward;
static int extendedViBackward;


/******************************** POINT **************************************/
/*                                                                           */
/* This macro simply refers to the character under the cursor, or '\0' if    */
/* the cursor is on (actually, past) the end of line.                        */
/*                                                                           */
/*****************************************************************************/

#define POINT editLine[editIndex]


/*****************************************************************************/
/*****************************************************************************/
/*****************************************************************************/
/*****************************************************************************/
/*****************************************************************************/


/***************************** editing primitives ****************************/
/*                                                                           */
/* These are the four most primitive editing functions -- into which all     */
/* high-level editing commands are decomposed.                               */
/*                                                                           */
/* These functions are responsible for:                                      */
/*                                                                           */
/*   - updating the command-line                                             */
/*   - maintaining the list of cursor movements (in "cList") to be           */
/*     returned to the word module                                           */
/*   - saving all undo information                                           */
/*   - checking for any editing errors and requesting bell rings             */
/*   - adjusting the position of any marks in the command-line when          */
/*     chars are inserted/deleted                                            */
/*                                                                           */
/* In particular,                                                            */
/*                                                                           */
/*   Right() moves the cursor one character to the right and returns TRUE    */
/*   if successful.                                                          */
/*                                                                           */
/*   Left() moves the cursor one character to the left and returns TRUE      */
/*   if successful.                                                          */
/*                                                                           */
/*   Insert(c) inserts the specified character at the cursor position and    */
/*   then moves the cursor one character to the right.                       */
/*                                                                           */
/*   Delete() deletes the character under the cursor and returns TRUE        */
/*   if successful.                                                          */
/*                                                                           */
/*****************************************************************************/


/********************************* Right() ***********************************/
/*                                                                           */
/* If we're not at the eol, move the cursor right and then add the           */
/* char we just passed over to the cList to be returned to the word          */
/* module.                                                                   */
/*                                                                           */
/*****************************************************************************/

static int Right()
{
  if (POINT) {
    CListAdd(cList, POINT);
    editIndex++;
    return TRUE;
  }
  ringBell = TRUE;
  return FALSE;
}


/********************************** Left() ***********************************/
/*                                                                           */
/* If we're not at the bol, move the cursor left and then add a              */
/* "delete" command to the cList to be returned to the word module.          */
/*                                                                           */
/* If the user tried to back up over the bol, return an "empty" command      */
/* since he might be trying to back up over the "/" or "^R" of a history     */
/* search command.  (If the Search() routine reads this, it will restore     */
/* the original command line; any other routine will ignore it completely.)  */
/*                                                                           */
/*****************************************************************************/

static int Left()
{
  if (editIndex > leftMargin) {
    CListAdd(cList, editDelete);
    editIndex--;
    return TRUE;
  }
  if (! POINT) {
    CListAdd(cList, editEmpty);
  }
  ringBell = TRUE;
  return FALSE;
}


/*********************************** Insert() ********************************/
/*                                                                           */
/* If we haven't overflowed our input buffer, add a "delete" record to the   */
/* undoCList (to counter the "insert" we are about to perform) and then      */
/* make a space in the command-line for the new character and insert it.     */
/*                                                                           */
/* Then, see if any marks in the command line were to the right of where     */
/* we just inserted the new char -- if so, adjust their indices.             */
/*                                                                           */
/* Finally, move the cursor right over the char we just inserted.            */
/*                                                                           */
/*****************************************************************************/

static void Insert(c)
int c;
{
  Undo u;
  char temp[EDITlength];

  if (editEOL >= sizeof(editLine)-1) {
    ringBell = TRUE;
    return;
  }
  u.undo.begin = newDo;
  u.undo.insert = 0;
  u.undo.index = editIndex;
  CListAdd(undoCList, u.c);
  newDo = FALSE;
  strcpy(temp, editLine+editIndex);
  editLine[editIndex] = c;
  strcpy(editLine+editIndex+1, temp);
  editEOL++;
  if (mark >= editIndex) {
    mark++;
  }
  if (tempMark >= editIndex) {
    tempMark++;
  }
  if (altMark >= editIndex) {
    altMark++;
  }
  (void)Right();
  return;
}


/************************************* Delete() ******************************/
/*                                                                           */
/* If there is a character under the cursor, add an "insert" record to the   */
/* undoCList (to counter the "delete" we are about to perform) and then      */
/* delete the char in the command-line and move up the following chars.      */
/*                                                                           */
/* Then, see if any marks in the command line were to the right of where     */
/* we just deleted the char -- if so, adjust their indices.                  */
/*                                                                           */
/*****************************************************************************/

static int Delete()
{
  Undo u;
  char temp[EDITlength];

  if (! POINT) {
    ringBell = TRUE;
    return FALSE;
  }
  u.undo.begin = newDo;
  u.undo.insert = POINT;
  u.undo.index = editIndex;
  CListAdd(undoCList, u.c);
  newDo = FALSE;
  strcpy(temp, editLine+editIndex+1);
  strcpy(editLine+editIndex, temp);
  editEOL--;
  if (mark > editIndex) {
    mark--;
  }
  if (tempMark > editIndex) {
    tempMark--;
  }
  if (altMark > editIndex) {
    altMark--;
  }
  return TRUE;
}


/*****************************************************************************/
/*****************************************************************************/
/*****************************************************************************/
/*****************************************************************************/
/*****************************************************************************/


/********************************* MoveTo() **********************************/
/*                                                                           */
/* This routine just moves the cursor right or left to get to the            */
/* specified index in the command-line.                                      */
/*                                                                           */
/*****************************************************************************/

static void MoveTo(index)
int index;
{
  while (index > editIndex && Right()) ;
  while (index < editIndex && Left()) ;
  if (index != editIndex) {
    ringBell = TRUE;
  }
}


/******************************* CopyTo **************************************/
/*                                                                           */
/* This routine copies the chars from the cursor position up to (and         */
/* possibly including) the specified position to buffer[n].  If this is      */
/* not the "first copy" of a do-loop and "how" is either APPEND or PREPEND,  */
/* then the chars are appended or prepended to the buffer[n], as             */
/* appropriate.                                                              */
/*                                                                           */
/* Note that if index < editIndex, the char at editLine[index] is included   */
/* in the copy; otherwise it is not.                                         */
/*                                                                           */
/*****************************************************************************/

#define REPLACE  0
#define APPEND   1
#define PREPEND  2

static void CopyTo(index, n, how)
int index;
int n;
int how;
{
  int end;
  int start;
  char copy[EDITlength];

  if (extendedDo) {
    return;
  }
  if (index > editIndex) {
    start = editIndex;
    end = index;
  } else {
    start = index;
    end = editIndex;
  }
  strncpy(copy, editLine+start, (size_t)end-start);
  copy[end-start] = '\0';

  if (newCopy || how == REPLACE) {
    strcpy(buffer[n], copy);
  } else {
    if (strlen(buffer[n])+strlen(copy) < EDITlength) {
      if (how == APPEND) {
        strcpy(buffer[n], StringConcat(buffer[n], copy));
      } else {
        strcpy(buffer[n], StringConcat(copy, buffer[n]));
      }
    } else {
      ringBell = TRUE;
    }
  }
  newCopy = FALSE;
}


/******************************** DeleteTo ***********************************/
/*                                                                           */
/* This routine deletes the chars from the cursor position up to (and        */
/* possibly including) the specified position.  These characters are not     */
/* saved in any buffer!                                                      */
/*                                                                           */
/* Note that if index < editIndex, the char at editLine[index] is included   */
/* in the cut; otherwise it is not.                                          */
/*                                                                           */
/*****************************************************************************/

static void DeleteTo(index)
int index;
{
  while (index > editIndex && Delete()) {
    index--;
  }
  while (index < editIndex && Left()) {
    (void)Delete();
  }
  if (index != editIndex) {
    ringBell = TRUE;
  }
}


/******************************** CutTo **************************************/
/*                                                                           */
/* This routine cuts the chars from the cursor position up to (and           */
/* possibly including) the specified position to buffer[n].  If this is      */
/* not the "first cut" of a do-loop and "how" is either APPEND or PREPEND,   */
/* then the chars are appended or prepended to the buffer[n], as             */
/* appropriate.                                                              */
/*                                                                           */
/* Note that if index < editIndex, the char at editLine[index] is included   */
/* in the cut; otherwise it is not.                                          */
/*                                                                           */
/*****************************************************************************/

static void CutTo(index, n, how)
int index;
int n;
int how;
{
  CopyTo(index, n, how);
  DeleteTo(index);
}


/*****************************************************************************/
/*****************************************************************************/
/*****************************************************************************/
/*****************************************************************************/
/*****************************************************************************/


/******************************* DeleteLine() ********************************/
/*                                                                           */
/* This routine simply cuts the entire command-line to the main yank/put     */
/* buffer.  The cursor is left at position 0 of the empty command-line.      */
/*                                                                           */
/*****************************************************************************/

static void DeleteLine()
{
  MoveTo(leftMargin);
  CutTo(editEOL, BUFFERmain, REPLACE);
}


/******************************* InsertLine() ********************************/
/*                                                                           */
/* This routine simply inserts the specified string at the current cursor    */
/* position in the command-line.  The cursor is left at the end of the       */
/* inserted string.                                                          */
/*                                                                           */
/*****************************************************************************/

static void InsertLine(s)
char *s;
{
  while (*s) {
    (void)Insert(*s++);
  }
}


/**************************** InsertPaddedLine() *****************************/
/*                                                                           */
/* This routine is like InsertLine() except that it also ensures that        */
/* the inserted string is preceeded and followed by a space.  If this is     */
/* not already so, extra spaces are inserted.                                */
/*                                                                           */
/*****************************************************************************/

static void InsertPaddedLine(s)
char *s;
{
  if (editIndex && ! isspace(editLine[editIndex-1])) {
    (void)Insert(' ');
  }
  InsertLine(s);
  if (! isspace(editLine[editIndex])) {
    (void)Insert(' ');
  }
}


/******************************** NewLine() **********************************/
/*                                                                           */
/* This routine begins editing the specified string as a new line.           */
/* Typically, this routine will be called with either an empty string or     */
/* a string recalled from the history file.  It deletes any current          */
/* command-line, inserts the new command-line, and resets all "undo"         */
/* information to prepair for new editing commands from the user.            */
/*                                                                           */
/*****************************************************************************/

static void NewLine(s)
char *s;
{
  leftMargin = hardLeftMargin;
  DeleteLine();
  strcpy(editLine+leftMargin, s);
  editEOL = strlen(editLine);
  strcpy(undoLine, s);
  CListEmpty(undoCList);
  undoCurrent = 0;
}


/*****************************************************************************/
/*****************************************************************************/
/****************************** word movements *******************************/
/*****************************************************************************/
/*****************************************************************************/


/********************************** CType() **********************************/
/*                                                                           */
/* This function returns the type of the specified character -- either       */
/* CTspace (for white-space), CTalnum (for alphanumeric chars or '_'s), or   */
/* CTpuntc (for other printable punctuation).                                */
/*                                                                           */
/*****************************************************************************/

#define CTspace 0
#define CTalnum 1
#define CTpunct 2

static int CType(c)
char c;
{
  if (isalnum(c) || c == '_') {
    return CTalnum;
  } else if (ispunct(c)) {
    return CTpunct;
  } else {
    return CTspace;
  }
}


/*********************** ForwardVWord() / BackwardVWord() ********************/
/*                                                                           */
/* These functions move the cursor forward or backward until they are        */
/* either on (bool == TRUE) or not on (bool == FALSE) a valid "vi word"      */
/* in the command-line.  If bell is TRUE and the specified condition         */
/* cannot be satisfied, the bell will be rung.  Note that if the specified   */
/* condition is initially satisfied, the cursor will not be moved.           */
/*                                                                           */
/* These functions return TRUE if successful.                                */
/*                                                                           */
/* Note that the sequence Forward...(FALSE, TRUE), Forward...(TRUE, TRUE)    */
/* moves the cursor to the beginning of the *next* word (i.e., move to the   */
/* end of the current word, then move to the beginning of the next one).     */
/*                                                                           */
/*****************************************************************************/

static int ForwardVWord(bool, bell)
int bool;
int bell;
{
  int cType;

  if (bool) {
    while (POINT && ! CType(POINT)) {
      (void)Right();
    }
  } else {
    cType = CType(POINT);
    while (POINT && CType(POINT) == cType) {
      (void)Right();
    }
  }
  if (! POINT) {
    ringBell |= bell;
    return FALSE;
  }
  return TRUE;
}


static int BackwardVWord(bool, bell)
int bool;
int bell;
{
  int ok;
  int cType;

  if (bool) {
    while (editIndex > leftMargin && ! CType(POINT)) {
      (void)Left();
    }
    ok = CType(POINT);
  } else {
    cType = CType(POINT);
    while (editIndex > leftMargin && CType(POINT) == cType) {
      (void)Left();
    }
    ok = CType(POINT) != cType;
  }
  if (! ok) {
    ringBell |= bell;
    return FALSE;
  }
  return TRUE;
}


/************************** ForwardWord() / BackwardWord() *******************/
/*                                                                           */
/* These functions move the cursor forward or backward until they are        */
/* either on (bool == TRUE) or not on (bool == FALSE) a valid "emacs word"   */
/* in the command-line.  If bell is TRUE and the specified condition         */
/* cannot be satisfied, the bell will be rung.  Note that if the specified   */
/* condition is initially satisfied, the cursor will not be moved.           */
/*                                                                           */
/* These functions return TRUE if successful.                                */
/*                                                                           */
/* Note that the sequence Forward...(FALSE, TRUE), Forward...(TRUE, TRUE)    */
/* moves the cursor to the beginning of the *next* word (i.e., move to the   */
/* end of the current word, then move to the beginning of the next one).     */
/*                                                                           */
/*****************************************************************************/

static int ForwardWord(bool, bell)
int bool;
int bell;
{
  while (POINT && (! isalnum(POINT)) == bool) {
    (void)Right();
  }
  if (! POINT) {
    ringBell |= bell;
    return FALSE;
  }
  return TRUE;
}


static int BackwardWord(bool, bell)
int bool;
int bell;
{
  while (editIndex > leftMargin && (! isalnum(POINT)) == bool) {
    (void)Left();
  }
  if ((! isalnum(POINT)) == bool) {
    ringBell |= bell;
    return FALSE;
  }
  return TRUE;
}


/*********************** ForwardBWord() / BackwardBWord() ********************/
/*                                                                           */
/* These functions move the cursor forward or backward until they are        */
/* either on (bool == TRUE) or not on (bool == FALSE) a valid "vi bigword"   */
/* in the command-line.  If bell is TRUE and the specified condition         */
/* cannot be satisfied, the bell will be rung.  Note that if the specified   */
/* condition is initially satisfied, the cursor will not be moved.           */
/*                                                                           */
/* These functions return TRUE if successful.                                */
/*                                                                           */
/* Note that the sequence Forward...(FALSE, TRUE), Forward...(TRUE, TRUE)    */
/* moves the cursor to the beginning of the *next* word (i.e., move to the   */
/* end of the current word, then move to the beginning of the next one).     */
/*                                                                           */
/*****************************************************************************/

static int ForwardBWord(bool, bell)
int bool;
int bell;
{
  while (POINT && (! isspace(POINT)) != bool) {
    (void)Right();
  }
  if (! POINT) {
    ringBell |= bell;
    return FALSE;
  }
  return TRUE;
}


static int BackwardBWord(bool, bell)
int bool;
int bell;
{
  while (editIndex > leftMargin && (! isspace(POINT)) != bool) {
    (void)Left();
  }
  if ((! isspace(POINT)) != bool) {
    ringBell |= bell;
    return FALSE;
  }
  return TRUE;
}


/************************* ForwardBlank() / BackwardBlank() ******************/
/*                                                                           */
/* These routines move the cursor forward or backward over all adjacent      */
/* whitespace.  The cursor is left on the first non-white character.         */
/*                                                                           */
/*****************************************************************************/

#ifndef KEYSHELL
static void ForwardBlank()
{
  while (isspace(POINT)) {
    (void)Right();
  }
}

static void BackwardBlank()
{
  while (editIndex > leftMargin && isspace(editLine[editIndex-1])) {
    (void)Left();
  }
}
#endif


/*****************************************************************************/
/*****************************************************************************/
/************************* character finds ***********************************/
/*****************************************************************************/
/*****************************************************************************/


/******************************** Find() *************************************/
/*                                                                           */
/* This routine finds the first occurrance of the specified character        */
/* searching in the specified direction.                                     */
/*                                                                           */
/* An initial search can be specified with direction FORWARD, BACKWARD,      */
/* FORWARDVI, or BACKWARDVI.  The *VI variants of the direction only         */
/* affect subsequent ADJUST calls.                                           */
/*                                                                           */
/* Specifying AGAIN or REVERSE causes the last search to be repeated --      */
/* "c" is not used.                                                          */
/*                                                                           */
/* Specifying ADJUST (again, with "c" not used) will have no affect if       */
/* the initial search was specified with direction FORWARD or BACKWARD.      */
/* Otherwise, if the initial search was specified with direction             */
/* FORWARDVI or BACKWARDVI, specifying ADJUST will cause the cursor to       */
/* move one character in the *opposite* direction of the search.  This       */
/* is used to process the vi "t" and "T" commands (and also ";" and ","      */
/* commands following "t" and "T" commands).                                 */
/*                                                                           */
/*****************************************************************************/

#define FORWARD    0
#define BACKWARD   1
#define AGAIN      2
#define REVERSE    3
#define FORWARDVI  4
#define BACKWARDVI 5
#define ADJUST     6

static void Find(c, how)
{
  int index;
  int direction;
  static int saveC;
  static int saveAdjust;
  static int saveDirection = 1;

  switch (how) {
    case FORWARD    : direction = saveDirection = 1;
                      saveAdjust = 0;
                      break;
    case BACKWARD   : direction = saveDirection = -1;
                      saveAdjust = 0;
                      break;
    case FORWARDVI  : direction = saveDirection = 1;
                      saveAdjust = -direction;
                      break;
    case BACKWARDVI : direction = saveDirection = -1;
                      saveAdjust = -direction;
                      break;
    case AGAIN      : direction = saveDirection;
                      if (saveAdjust) {
                        saveAdjust = -direction;
                      }
                      break;
    case REVERSE    : direction = -saveDirection;
                      if (saveAdjust) {
                        saveAdjust = -direction;
                      }
                      break;
    case ADJUST     : if (saveAdjust > 0) {
                        (void)Right();
                      } else if (saveAdjust < 0) {
                        (void)Left();
                      }
                      return;
  }
  if (c) {
    saveC = c;
  }
  index = editIndex+direction;
  while (index >= leftMargin && index < editEOL) {
    if (editLine[index] == saveC) {
      MoveTo(index);
      return;
    }
    index += direction;
  }
  ringBell = TRUE;
}


/*****************************************************************************/
/*****************************************************************************/
/*****************************************************************************/
/*****************************************************************************/
/*****************************************************************************/


/******************************* UpperCase() *********************************/
/*                                                                           */
/* This routine transforms the character under the cursor to upper case and  */
/* then moves the cursor right one position.                                 */
/*                                                                           */
/*****************************************************************************/

static void UpperCase()
{
  int c;

  if (c = POINT) {
    (void)Delete();
    (void)Insert(toupper(c));
  }
}


/******************************* LowerCase() *********************************/
/*                                                                           */
/* This routine transforms the character under the cursor to lower case and  */
/* then moves the cursor right one position.                                 */
/*                                                                           */
/*****************************************************************************/

static void LowerCase()
{
  int c;

  if (c = POINT) {
    (void)Delete();
    (void)Insert(tolower(c));
  }
}


/******************************* FlipCase() **********************************/
/*                                                                           */
/* This routine transforms the case of the character under the cursor and    */
/* then moves the cursor right one position.                                 */
/*                                                                           */
/*****************************************************************************/

static void FlipCase()
{
  if (islower(POINT)) {
    UpperCase();
  } else if (isupper(POINT)) {
    LowerCase();
  } else {
    (void)Right();
  }
}


/*****************************************************************************/
/*****************************************************************************/
/***************************** history functions *****************************/
/*****************************************************************************/
/*****************************************************************************/


/****************************** partialLine **********************************/
/*                                                                           */
/* This buffer holds a partial input line which *might* be the next line     */
/* entered into history.  The intent is that if the user enters part of a    */
/* line and then (accidentally) hits the up-arrow key, he should be able     */
/* to get back to where he was by hitting the down-arrow key.  Any time      */
/* that this buffer is empty and the editLine is not, we copy the editLine   */
/* here.  Then, any time that the user down-arrows past the end of the       */
/* history file, we display this line.                                       */
/*                                                                           */
/*****************************************************************************/

static char partialLine[EDITlength];


/*********************************** FC() ************************************/
/*                                                                           */
/* This function performs the vi-mode "v" command and the emacs-mode         */
/* "<ESC>v" command.  Basically, if the user didn't enter a prefix, we save  */
/* the current editLine in the history buffer and then pretend that the      */
/* user entered the 'fc -e "${VISUAL:-${EDITOR:-vi}}" nnn', where nnn is     */
/* the line number of the command we just wrote to the history file.         */
/* Otherwise, if the user did enter a prefix, we edit that line from the     */
/* history file.                                                             */
/*                                                                           */
/*****************************************************************************/

static void FC(n)
int n;
{
  if (n < 0) {
    if (! StringSig(editLine)) {
      ringBell = TRUE;
      return;
    }
    n = KshGetHistCurrent();
    KshHistWrite(editLine);
  }
  NewLine(StringConcat(KshFCCommand(), ltoa((long)n)));

  KeyboardWrite(keyboardExecute);
}


/********************************* Goto() ************************************/
/*                                                                           */
/* This function just reads the specified line from the history file and     */
/* starts editing it.                                                        */
/*                                                                           */
/*****************************************************************************/

static void Goto(n)
int n;
{
  char temp[EDITlength];

  if (! partialLine[0]) {
    strcpy(partialLine, editLine);
  }
  if (n < 0) {
    n = KshHistMin();
  }
  recentLine = n;
  KshSetHistCurrent(n);
  KshHistCopy(temp, n);
  NewLine(temp);
}


/********************************* Previous() ********************************/
/*                                                                           */
/* This routine just recalls the previous line from the history file and     */
/* starts editing it.                                                        */
/*                                                                           */
/*****************************************************************************/

static void Previous(n)
int n;
{
  int new;

  if (n < 0) {
    n = 1;
  }
  new = KshGetHistCurrent() - n;
  if (new < KshHistMin()) {
    ringBell = TRUE;
    return;
  }
  Goto(new);
}


/******************************** Next() *************************************/
/*                                                                           */
/* This routine just recalls the next line from the history file and         */
/* starts editing it.  If we have reached the end of the history file,       */
/* the "partialLine" is used.                                                */
/*                                                                           */
/*****************************************************************************/

static void Next(n)
int n;
{
  int new;

  if (n < 0) {
    n = 1;
  }
  if (KshGetHistCurrent() == KshHistMax() && recentLine) {
    KshSetHistCurrent(recentLine);
  }
  new = KshGetHistCurrent() + n;
  if (new == KshHistMax() && partialLine[0]) {
    KshSetHistCurrent(new);
    NewLine(partialLine);
    partialLine[0] = '\0';
    return;
  } else if (new >= KshHistMax()) {
    ringBell = TRUE;
    return;
  }
  Goto(new);
}


/********************************** Search() *********************************/
/*                                                                           */
/* This is the most complex routine in the module.  Edit it with caution.    */
/*                                                                           */
/* This routine performs a history search in the specified direction.  If    */
/* "how" is AGAIN or REVERSE, the user will not be queried for a search      */
/* string, instead, the previous search string and direction (or opposite    */
/* direction, in the case of REVERSE) will be used.                          */
/*                                                                           */
/* If FORWARD or BACKWARD is specified, the user will be prompted to         */
/* enter a new string to search for.  If the user enters a null-string,      */
/* the previous search string will again be used.  If the user backspaces    */
/* past the beginning of the line (containing the search prompt char,        */
/* such as "/" or "^R"), the search will be aborted and the original         */
/* command-line restored.                                                    */
/*                                                                           */
/*****************************************************************************/

static void Search(how)
int how;
{
  int line;
  int blank;
  int prompt;
  int action;
  int restore;
  int leftIndex;
  int direction;
  static int active;
  CList *saveCList;
  CList *saveDoCList;
  char dummy[EDITlength];
  char origLine[EDITlength];
  static int saveDirection = -1;
  static char saveString[EDITlength];

  /* don't allow us to recurse in this routine */
  if (active) {
    return;
  }

  /* figure out which direction in the history file we are searching */
  switch (how) {
    case FORWARD  : direction = saveDirection = 1;
                    break;
    case BACKWARD : direction = saveDirection = -1;
                    break;
    case AGAIN    : direction = saveDirection;
                    break;
    case REVERSE  : direction = -saveDirection;
                    break;
  }

  /* if we need to, prompt the user for a new string and enter it */
  if (how == FORWARD || how == BACKWARD) {
    switch (editMode) {
      case editVi    : prompt = ((direction>0)?('?'):('/'));
                       break;
      case editEmacs : prompt = ((direction>0)?(CNTL('S')):(CNTL('R')));
                       break;
      default        : prompt = ((direction>0)?('+'):('-'));
                       break;
    }

    /* delete anything in the current command-line */
    strcpy(origLine, editLine);
    DeleteLine();

    /************************** critical section begin ***********************/
                                                                          /* */
    /* save any work in progress in the current command-line and */       /* */
    /* then set up to read an "alternate" command-line to get the */      /* */
    /* search string */                                                   /* */
    saveCList = cList;                                                    /* */
    saveDoCList = doCList;                                                /* */
    cList = altCList;                                                     /* */
    doCList = altDoCList;                                                 /* */
                                                                          /* */
    /* then make sure the softkeys are blanked while the user enters */   /* */
    /* the search string */                                               /* */
    blank = globalSoftKeyBlank;                                           /* */
    globalSoftKeyBlank = TRUE;                                            /* */
                                                                          /* */
    /* then add the prompt char (e.g., "/" or "^R") to the command */     /* */
    /* line and set the input margin just after it */                     /* */
    (void)Insert(prompt);                                                 /* */
    leftIndex = editIndex;                                                /* */
    leftMargin = editIndex;                                               /* */
    hardLeftMargin = editIndex;                                           /* */
                                                                          /* */
    /* then read characters until the user presses <Return> or he */      /* */
    /* tries to backspace before the beginning of the line -- fake */     /* */
    /* a return after setting a flag for the latter case. */              /* */
    active = TRUE;                                                        /* */
    restore = FALSE;                                                      /* */
    do {                                                                  /* */
      action = EditRead(dummy);                                           /* */
      if (action == editEmpty) {                                          /* */
        restore = TRUE;                                                   /* */
        KeyboardWrite(keyboardExecute);                                   /* */
      }                                                                   /* */
    } while (! KEYBOARDaction(action));                                   /* */
    active = FALSE;                                                       /* */
                                                                          /* */
    /* then reset the input margin to the true beginning of the */        /* */
    /* command-line */                                                    /* */
    leftMargin = 0;                                                       /* */
    hardLeftMargin = 0;                                                   /* */
                                                                          /* */
    /* then, if the user entered a real string (greater than 0 */         /* */
    /* length), save the string as what we will search for. */            /* */
    if (editLine[leftIndex]) {                                            /* */
      strcpy(saveString, editLine+leftIndex);                             /* */
    }                                                                     /* */
                                                                          /* */
    /* then delete anything left in the alternate command-line */         /* */
    DeleteLine();                                                         /* */
                                                                          /* */
    /* and restore the state of the softkeys */                           /* */
    globalSoftKeyBlank = blank;                                           /* */
                                                                          /* */
    /* and return to the work in progress in the current command-line */  /* */
    /* (Note that the doCList and redoCList might have been swapped */    /* */
    /* behind our backs!) */                                              /* */
    cList = saveCList;                                                    /* */
    if (doCList == altDoCList) {                                          /* */
      doCList = saveDoCList;                                              /* */
    } else {                                                              /* */
      redoCList = saveDoCList;                                            /* */
    }                                                                     /* */
                                                                          /* */
    /************************** critical section end *************************/

    /* make sure the input modules know we are done screwing around */
    KeyboardWrite(keyboardEndInputMode);

    /* if the user aborted the search, just restore his original */
    /* command-line and return */
    if (restore) {
      NewLine(origLine);
      return;
    }
  }

  /* then search for the requested history line -- if none is found, */
  /* just ring the bell; otherwise go there. */
  line = KshHistFind(saveString, KshGetHistCurrent(), direction);
  if (line < 0) {
    ringBell = TRUE;
    return;
  }
  Goto(line);
}


/************************************ Param() ********************************/
/*                                                                           */
/* This routine just recalls the nth (or last, if n == -1) parameter from    */
/* the previous history line and inserts it at the cursor position.          */
/*                                                                           */
/*****************************************************************************/

static void Param(n)
int n;
{
  char *p;
  char temp[EDITlength];

  p = KshHistWord(temp, n);
  InsertPaddedLine(p);
}


/*****************************************************************************/
/*****************************************************************************/
/*****************************************************************************/
/*****************************************************************************/
/*****************************************************************************/


/********************************* Expand() **********************************/
/*                                                                           */
/* This routine performs the specified expansion on the current word         */
/* in the command-line.  if c is '=', a file name list is printed; if        */
/* c is '*', the file name is expanded in place; if c is <ESC>, the file     */
/* name is completed in place.                                               */
/*                                                                           */
/*****************************************************************************/

static void Expand(c)
int c;
{
  int first;
  int insert;
  int delete;
  int tempEOL;
  int tempIndex;
  char temp[EDITlength];

  /* if displaying a filename list, temporarily move the command-line */
  /* out of the way. */
  if (c == '=') {
    DisplayAside(TRUE);
  }

  /* then create a scratch buffer copy of the command-line that the */
  /* ksh routines can modify. */
  tempEOL = editEOL;
  tempIndex = editIndex;
  strcpy(temp, editLine);

  /* if the expansion routine fails, refresh the display and ring the */
  /* bell as appropriate */
  if (KshEdExpand(temp, &tempIndex, &tempEOL, c) < 0) {
    if (c == '=') {
      DisplayRefresh();
    } else {
      KeyboardWrite(keyboardEndInputMode);
    }
    ringBell = TRUE;
    return;

  /* otherwise, if we just printed a filename list, redraw a new copy */
  /* of the command-line where the user can continue editing, then return */
  } else if (c == '=') {
    DisplayResume();
    return;
  }

  /* otherwise, if we expanded the file name, find out exactly which */
  /* characters in the command-line changed and update them officially. */
  if ((first = StringDiff(temp, editLine, &insert, &delete)) >= 0) {
    MoveTo(first);
    while (delete > 0 && Delete()) {
      delete--;
    }
    while (insert > 0) {
      (void)Insert(temp[first++]);
      insert--;
    }
  }
  MoveTo(tempIndex);
}


/*****************************************************************************/
/*****************************************************************************/
/*****************************************************************************/
/*****************************************************************************/
/*****************************************************************************/


/*********************************** SoftKey() *******************************/
/*                                                                           */
/* This function expands the specified softkey into the command-line.  If    */
/* the softkey has the "automatic" attribute set, a <Return> is sent to      */
/* the keyboard module.                                                      */
/*                                                                           */
/*****************************************************************************/

static void SoftKey(c)
int c;
{
  GlobalSoftKey *sk;

  sk = GlobalDisplayedSoftKeyN(KEYBOARDfToN(c));
  if (sk) {
    InsertPaddedLine(sk->literal);
    if (sk->attrib.automatic) {
      KeyboardWrite(keyboardExecute);
    }
  }
}


/********************************** Char() ***********************************/
/*                                                                           */
/* This function overwrites or inserts the specified character into the      */
/* command-line, depending on the state of the editOverwrite boolean.  The   */
/* cursor is left to the right of the new character.                         */
/*                                                                           */
/*****************************************************************************/

static void Char(c)
int c;
{
  assert(KEYBOARDchar(c));

  if (editOverwrite && POINT) {
    (void)Delete();
  }
  (void)Insert(c);
}


/*****************************************************************************/
/*****************************************************************************/
/*****************************************************************************/
/*****************************************************************************/
/*****************************************************************************/


/******************************** GenericRead() ******************************/
/*                                                                           */
/* This function reads the next input character or command from the vi,      */
/* emacs, or edit module as appropriate.                                     */
/*                                                                           */
/*****************************************************************************/


static int GenericRead()
{
  switch (editMode) {
    case editVi    :  return ViRead();
    case editEmacs :  return EmacsRead();
    default        :  return KeyboardRead();
  }
}


/******************************* GenericAvail() ******************************/
/*                                                                           */
/* This function checks for the immediate availability of a character or     */
/* command from the vi, emacs, or edit module as appropriate.                */
/*                                                                           */
/*****************************************************************************/

static int GenericAvail()
{
  switch (editMode) {
    case editVi    :  return ViAvail();
    case editEmacs :  return EmacsAvail();
    default        :  return KeyboardAvail();
  }
}


/*****************************************************************************/
/*****************************************************************************/
/*****************************************************************************/
/*****************************************************************************/
/*****************************************************************************/


/********************************** Do2() ************************************/
/*                                                                           */
/* This function returns TRUE if the specified edit command is a 2-integer   */
/* command (i.e., if it has an associated parameter).                        */
/*                                                                           */
/*****************************************************************************/

static int Do2(c)
int c;
{
  switch (c) {
    case keyboardFindForward           :
    case keyboardFindBackward          :
    case keyboardFindForwardVi         :
    case keyboardFindBackwardVi        :
    case keyboardNext                  :
    case keyboardPrevious              :
    case keyboardGoto                  :
    case keyboardParam                 :
    case keyboardFC                    : return TRUE;
  }
  return FALSE;
}


/************************************ Do() ***********************************/
/*                                                                           */
/* This function processes the specified edit command.  If the edit          */
/* command has an associated parameter, it must be passed as the second      */
/* argument to this function.                                                */
/*                                                                           */
/* All command-line editing work originates fromthis routine.                */
/*                                                                           */
/*****************************************************************************/

static void Do(c, nextC)
int c;
int nextC;
{
  int temp;

  switch (c) {
  /* MISC */
    case keyboardNull                  : break;
    case keyboardFlush                 : DisplayHint(NULL, FALSE);
                                         break;
  /* INSERT */
    case keyboardInsert                : editOverwrite = FALSE;
                                         break;
    case keyboardOverwrite             : editOverwrite = TRUE;
                                         break;
    case keyboardToggleInsertOverwrite : editOverwrite = ! editOverwrite;
                                         break;
  /* MOVE */
    case keyboardForward               : (void)Right();
                                         break;
#ifndef KEYSHELL
    case keyboardForwardWord           : (void)ForwardWord(FALSE, TRUE);
                                         (void)ForwardWord(TRUE, TRUE);
                                         break;
#endif
    case keyboardForwardBWord          : (void)ForwardBWord(FALSE, TRUE);
                                         (void)ForwardBWord(TRUE, TRUE);
                                         break;
    case keyboardForwardVWord          : (void)ForwardVWord(FALSE, TRUE);
                                         (void)ForwardVWord(TRUE, TRUE);
                                         break;
#ifndef KEYSHELL
    case keyboardForwardBlank          : ForwardBlank();
                                         break;
#endif
    case keyboardBackward              : (void)Left();
                                         break;
    case keyboardBackwardWord          : (void)Left();
                                         (void)BackwardWord(TRUE, TRUE);
                                         if (BackwardWord(FALSE, FALSE)) {
                                           (void)Right();
                                         }
                                         break;
    case keyboardBackwardBWord         : (void)Left();
                                         (void)BackwardBWord(TRUE, TRUE);
                                         if (BackwardBWord(FALSE, FALSE)) {
                                           (void)Right();
                                         }
                                         break;
    case keyboardBackwardVWord         : (void)Left();
                                         (void)BackwardVWord(TRUE, TRUE);
                                         if (BackwardVWord(FALSE, FALSE)) {
                                           (void)Right();
                                         }
                                         break;
#ifndef KEYSHELL
    case keyboardBackwardBlank         : BackwardBlank();
                                         break;
    case keyboardBeginWord             : (void)BackwardWord(TRUE, TRUE);
                                         if (BackwardWord(FALSE, FALSE)) {
                                           (void)Right();
                                         }
                                         break;
    case keyboardBeginBWord            : (void)BackwardBWord(TRUE, TRUE);
                                         if (BackwardBWord(FALSE, FALSE)) {
                                           (void)Right();
                                         }
                                         break;
    case keyboardBeginVWord            : (void)BackwardVWord(TRUE, TRUE);
                                         if (BackwardVWord(FALSE, FALSE)) {
                                           (void)Right();
                                         }
                                         break;
#endif
    case keyboardEndWord               : (void)ForwardWord(TRUE, TRUE);
                                         if (ForwardWord(FALSE, FALSE)) {
                                           (void)Left();
                                         }
                                         break;
    case keyboardEndBWord              : (void)Right();
                                         (void)ForwardBWord(TRUE, TRUE);
                                         if (ForwardBWord(FALSE, FALSE)) {
                                           (void)Left();
                                         }
                                         break;
    case keyboardEndVWord              : (void)Right();
                                         (void)ForwardVWord(TRUE, TRUE);
                                         if (ForwardVWord(FALSE, FALSE)) {
                                           (void)Left();
                                         }
                                         break;
    case keyboardBeginLine             : MoveTo(leftMargin);
                                         break;
    case keyboardBeginBLine            : MoveTo(leftMargin);
                                         (void)ForwardBWord(TRUE, FALSE);
                                         break;
    case keyboardEndLine               : MoveTo(editEOL);
                                         break;
#ifndef KEYSHELL
    case keyboardEndBLine              : MoveTo(editEOL);
                                         (void)BackwardBWord(TRUE, FALSE);
                                         break;
#endif
    case keyboardViAdjust              : if (editIndex == editEOL &&
                                             editEOL > leftMargin) {
                                           (void)Left();
                                         }
                                         break;
    case keyboardViForward             : if (POINT) {
                                           (void)Right();
                                         }
                                         break;
    case keyboardViBackward            : if (editIndex > leftMargin) {
                                           (void)Left();
                                         }
                                         break;
    case keyboardViDFAdjust            : if (POINT && editIndex >= mark) {
                                           (void)Right();
                                         }
                                         break;
    case keyboardViCWAdjust            : while (editIndex > leftMargin &&
                                                editIndex > mark &&
                                              isspace(editLine[editIndex-1])) {
                                           (void)Left();
                                         }
                                         if (POINT && editIndex == mark) {
                                           (void)Right();
                                         }
                                         break;
#ifndef KEYSHELL
    case keyboardSetMargin             : leftMargin = editIndex;
                                         break;
    case keyboardClearMargin           : leftMargin = hardLeftMargin;
                                         break;
#endif
  /* FIND */
    case keyboardFindForward           : Find(nextC, FORWARD);
                                         break;
    case keyboardFindBackward          : Find(nextC, BACKWARD);
                                         break;
    case keyboardFindForwardVi         : Find(nextC, FORWARDVI);
                                         break;
    case keyboardFindBackwardVi        : Find(nextC, BACKWARDVI);
                                         break;
    case keyboardFindAgain             : Find(0, AGAIN);
                                         break;
    case keyboardFindReverse           : Find(0, REVERSE);
                                         break;
    case keyboardFindAdjust            : Find(0, ADJUST);
                                         break;
  /* MARKS */
    case keyboardSetMark               : mark = editIndex;
                                         break;
    case keyboardSetTempMark           : tempMark = editIndex;
                                         break;
    case keyboardExchange              : temp = editIndex;
                                         MoveTo(mark);
                                         mark = temp;
                                         break;
    case keyboardExchangeTemp          : temp = editIndex;
                                         MoveTo(tempMark);
                                         tempMark = temp;
                                         break;
  /* CUT */
    case keyboardCut                   : CutTo(mark, BUFFERmain, REPLACE);
                                         break;
    case keyboardCutTemp               : CutTo(tempMark, BUFFERtemp, REPLACE);
                                         break;
  /* COPY */
    case keyboardCopy                  : CopyTo(mark, BUFFERmain, REPLACE);
                                         break;
    case keyboardCopyTemp              : CopyTo(tempMark, BUFFERtemp, REPLACE);
                                         break;
  /* BUFFER */
#ifndef KEYSHELL
    case keyboardCopyTempBuffer        : strcpy(buffer[BUFFERmain],
                                                buffer[BUFFERtemp]);
                                         break;
#endif
  /* COMPOUND MOVE/CUT */
    case keyboardDelete                : altMark = editIndex;
                                         (void)Right();
                                         CutTo(altMark, BUFFERmain, APPEND);
                                         break;
    case keyboardDeleteWord            : altMark = editIndex;
                                         Do(keyboardEndWord, 0);
                                         Do(keyboardForward, 0);
                                         CutTo(altMark, BUFFERmain, APPEND);
                                         break;
#ifndef KEYSHELL
    case keyboardDeleteBWord           : altMark = editIndex;
                                         Do(keyboardForwardBWord, 0);
                                         CutTo(altMark, BUFFERmain, APPEND);
                                         break;
    case keyboardDeleteVWord           : altMark = editIndex;
                                         Do(keyboardForwardVWord, 0);
                                         CutTo(altMark, BUFFERmain, APPEND);
                                         break;
#endif
    case keyboardBackspace             : altMark = editIndex;
                                         (void)Left();
                                         CutTo(altMark, BUFFERmain, PREPEND);
                                         break;
    case keyboardBackspaceWord         : altMark = editIndex;
                                         Do(keyboardBackwardWord, 0);
                                         CutTo(altMark, BUFFERmain, PREPEND);
                                         break;
    case keyboardBackspaceBWord        : altMark = editIndex;
                                         Do(keyboardBackwardBWord, 0);
                                         CutTo(altMark, BUFFERmain, PREPEND);
                                         break;
#ifndef KEYSHELL
    case keyboardBackspaceVWord        : altMark = editIndex;
                                         Do(keyboardBackwardVWord, 0);
                                         CutTo(altMark, BUFFERmain, PREPEND);
                                         break;
#endif
    case keyboardDeleteLine            : DeleteLine();
                                         break;
    case keyboardClearEOL              : CutTo(editEOL, BUFFERmain, REPLACE);
                                         break;
  /* PASTE */
    case keyboardPaste                 : InsertLine(buffer[BUFFERmain]);
                                         break;
    case keyboardPasteTemp             : InsertLine(buffer[BUFFERtemp]);
                                         break;
  /* CASE */
    case keyboardFlipCase              : FlipCase();
                                         break;
    case keyboardUpperCase             : UpperCase();
                                         break;
    case keyboardLowerCase             : LowerCase();
                                         break;
    case keyboardUpperCaseWord         : (void)ForwardWord(TRUE, TRUE);
                                         while (isalnum(POINT)) {
                                           UpperCase();
                                         }
                                         break;
    case keyboardLowerCaseWord         : (void)ForwardWord(TRUE, TRUE);
                                         while (isalnum(POINT)) {
                                           LowerCase();
                                         }
                                         break;
    case keyboardCapitalWord           : (void)ForwardWord(TRUE, TRUE);
                                         UpperCase();
                                         (void)ForwardWord(FALSE, TRUE);
                                         break;
  /* HISTORY */
    case keyboardPrevious              : Previous(nextC);
                                         break;
    case keyboardNext                  : Next(nextC);
                                         break;
    case keyboardPrevious1             : Previous(1);
                                         break;
    case keyboardNext1                 : Next(1);
                                         break;
    case keyboardFirst                 : Goto(KshHistMin());
                                         break;
    case keyboardLast                  : Goto(KshHistMax()-1);
                                         break;
    case keyboardGoto                  : Goto(nextC);
                                         break;
    case keyboardSearchBackward        : Search(BACKWARD);
                                         break;
    case keyboardSearchForward         : Search(FORWARD);
                                         break;
    case keyboardSearchAgain           : Search(AGAIN);
                                         break;
    case keyboardSearchReverse         : Search(REVERSE);
                                         break;
    case keyboardParam                 : Param(nextC);
                                         break;
    case keyboardFC                    : FC(nextC);
                                         break;
  /* EXPANSION */
    case keyboardExpandOne             : Expand('\\');
                                         break;
    case keyboardExpandAll             : Expand('*');
                                         break;
    case keyboardExpandList            : Expand('=');
                                         break;
    case keyboardEndInputMode          : break;
  /* ACTION */
    case keyboardInterrupt             : DeleteLine();
                                         c = keyboardExecute;
                                         goto skip;
    case keyboardTranslate             :
    case keyboardOperate               :
    case keyboardExecute               : DisplayUpdate(TRUE);
                                         /* fall thru */
skip:
    case keyboardTranslateVerify       :
    case keyboardOperateVerify         :
    case keyboardExecuteVerify         : MoveTo(editEOL);
                                         CListAdd(cList, c);
                                         if (c == keyboardOperateVerify) {
                                           operate = TRUE;
                                         }
                                         break;
  /* SOFTKEYS */
    case keyboardF1                    :
    case keyboardF2                    :
    case keyboardF3                    :
    case keyboardF4                    :
    case keyboardF5                    :
    case keyboardF6                    :
    case keyboardF7                    :
    case keyboardF8                    : SoftKey(c);
                                         break;
 /* CHARS */
    default                            : Char(c);
                                         break;
  }
}


/********************************* CacheBlock() ******************************/
/*                                                                           */
/* This function reads and caches the nth edit command in the specified      */
/* do-block.  (If the nth command has been cached previously, this           */
/* routine returns immediately with its value.)                              */
/*                                                                           */
/* Typically, this routine is used to initially read the commands            */
/* comprising a do-block from the lower level input modules.  Once the       */
/* do-block has been read in its entirety, this routine is used to *reread*  */
/* the commands of the do-block if the <repeat count> is greater than 1.     */
/*                                                                           */
/*****************************************************************************/

static int CacheBlock(toDoCList, n)
CList *toDoCList;
int n;
{
  int c;

  while (n >= CListCount(toDoCList)) {
    c = GenericRead();
    CListAdd(toDoCList, c);
  }
  c = CListLook(toDoCList, n);
  return c;
}

/******************************* DoBlock() ***********************************/
/*                                                                           */
/* This routine executes a do-block in the specified clist.  "count" is      */
/* the initial guess at the number of times that the block should be         */
/* repeated (either 0 or 1), "first" is the index of the first edit command  */
/* in the block, and "next" is a pointer to an integer which will be set     */
/* to the index of the next command which still needs to be executed in      */
/* the clist.                                                                */
/*                                                                           */
/* This routine is recursive.  If it encounters a nested do-block, it will   */
/* call itself with appropriate parameters to execute just the sub-block.    */
/* Upon return, execution of the outer block will resume.                    */
/*                                                                           */
/* Extended-do blocks are handled here -- see the discussion on this earlier */
/* in the file on static storage values maintained between invocations       */
/* of this routine (the "extendedDo" and associated variables).              */
/*                                                                           */
/*****************************************************************************/

static void DoBlock(toDoCList, count, first, next)
CList *toDoCList;
int count;
int first;
int *next;
{
  int c;
  int this;
  int gotCount;
  int viForward;
  int viBackward;

  /* initialize our local variables.  if we are continuing processing */
  /* of an extended-do block, read the previously saved values from */
  /* static storage. */
  gotCount = FALSE;
  if (extendedDo && ! next) {
    viForward = extendedViForward;
    viBackward = extendedViBackward;
  } else {
    viForward = FALSE;
    viBackward = FALSE;
  }

  /* then repeat the do-block until the <repeat count> is satisfied */
  /* or an error occurs which makes us abort. */
  do {

    /* again, if we are continuing an extended-do, read the index of */
    /* the next edit command to execute from static storage; otherwise */
    /* believe our parameter. */
    if (extendedDo && ! next) {
      this = extendedDoNext;
    } else {
      this = first;
    }

    /* then repeat until we reach the end of this do-block */
    while ((c = CacheBlock(toDoCList, this++)) != keyboardEndDo) {

      /* if we have a nested do-block, then recurse here */
      if (c == keyboardDo) {
        DoBlock(toDoCList, ((count)?(1):(0)), this, &this);

      /* otherwise check if we really plan on doing anything here */
      /* (as opposed to skipping the block because its count is 0) */
      } else if (count) {

        /* if so, then look for the special viForward and viBackward */
        /* commands.  if found, set flags so that we only execute them */
        /* once per do-block.  otherwise, execute a normal (one or two */
        /* integer long) edit command. */
        if (c == keyboardViForward) {
          if (! viForward) {
            Do(c, 0);
            viForward = TRUE;
          }
        } else if (c == keyboardViBackward) {
          viBackward = TRUE;
        } else if (Do2(c)) {
          Do(c, CacheBlock(toDoCList, this++));
        } else {
          Do(c, 0);
        }

        /* if the edit command failed, then set a flag so we abort */
        /* without waiting for the do-block <repeat count> to expire. */
        abortFlag |= ringBell;
        if (! next && ringBell) {
          DisplayBell();
          ringBell = FALSE;
        }
      }

      /* then check if this is a top-level extended-do routine which */
      /* we are going to return prematurely from (i.e., which has data */
      /* to return).  if so, save out local state in static variables */
      /* and return -- we will resume from this point when called again */
      if (extendedDo && ! next && CListPeek(cList)) {
        extendedDoNext = this;
        extendedViForward = viForward;
        extendedViBackward = viBackward;
        return;
      }
    }

    /* we have read the "end-do" command so now read the <repeat count>. */
    /* if we had a <repeat count> already, multiply them.  at this time */
    /* we can terminate the extended-do block since any repeating which */
    /* needs to be done will complete without needing to call any lower */
    /* level input modules -- and hence will not block. */
    c = CacheBlock(toDoCList, this++);
    if (! gotCount) {
      count *= c;
      gotCount = TRUE;
      if (! next) {
        extendedDo = FALSE;
      }
    }

    /* then repeat the (cached!) do-block as many times as necessary */
    count--;
  } while (count > 0 && ! abortFlag);

  /* update out callers variable telling them how many edit commands */
  /* were processed */
  if (next) {
    *next = this;
  }

  /* then execute a single viBackward command for this block if one */
  /* was scheduled */
  if (viBackward) {
    Do(keyboardViBackward, 0);
  }
}


/*********************************** UndoBlock() *****************************/
/*                                                                           */
/* This routine undoes the specified number of do-block's worth of edit      */
/* actions.  It starts at the current position in the undoCList and works    */
/* backward, undoing all primitive actions until it finds "count"            */
/* actions flagged as the "beginning" of do-blocks.  Note that the           */
/* "current" position in the undoCList is left at the last undone action --  */
/* so that an immediately following call will continue where this one        */
/* left off.                                                                 */
/*                                                                           */
/*****************************************************************************/

static void UndoBlock(count)
int count;
{
  Undo u;

  /* until we have undone the specified number of do-blocks... */
  while (count-- > 0 && ! abortFlag) {

    /* if we haven't reached the beginning of the undoCList... */
    if (undoCurrent) {

      /* undo a single do-block, including all undo actions back to */
      /* the last one flagged as the beginning of a do-block. */
      do {
        u.c = CListLook(undoCList, --undoCurrent);
        MoveTo(u.undo.index);
        if (u.undo.insert) {
          (void)Insert(u.undo.insert);
        } else {
          (void)Delete();
        }
      } while (undoCurrent && ! u.undo.begin);

    /* else we have an error -- can't go back any further */
    } else {
      ringBell = TRUE;
    }

    /* abort the <repeat count> if we have any errors. */
    abortFlag |= ringBell;
    if (ringBell) {
      DisplayBell();
      ringBell = FALSE;
    }
  }
}

/******************************* DoEdit() ************************************/
/*                                                                           */
/* This is the high-level editing routine.  It reads the next edit           */
/* command to process from the lower-level modules and dispatches the        */
/* proper routine to perform the action.  It is responsible for              */
/* interrupting and resuming the processing of extended-do blocks.           */
/*                                                                           */
/*****************************************************************************/

static void DoEdit()
{
  int c;
  int count;
  CList *temp;

  newCopy = TRUE;

  /* if we had previously interrupted an extended-do block, then resume */
  /* it now. */
  if (extendedDo) {
    goto skip;
  }

  /* reset our state for the beginning of a new do-block */
  newDo = TRUE;
  abortFlag = FALSE;
  ringBell = FALSE;

  /* if we have a characters in the command-line and none in the */
  /* "undoLine" used for the vi mode "U" command, then save a copy of */
  /* the current command line -- this is as far back as we can undo. */
  if (editLine[0] && ! undoLine[0]) {
    strcpy(undoLine, editLine);
  }

  /* if we "operated" the last command-line, then recall the next one */
  /* automatically for the user; otherwise, let the user drive -- read */
  /* the next editing command from the lower level input modules. */
  if (operate) {
    c = keyboardNext1;
    operate = FALSE;
  } else {
    c = GenericRead();
  }

  /* if we got a redo request (i.e., vi-mode "." command), just re */
  /* execute the last do-block which modified the command-line. */
  if (c == keyboardRedo) {
    count = GenericRead();
    DoBlock(redoCList, count, 0, NULL);

  /* otherwise if we got a kludge command which shouldn't affect anything */
  /* else, just execute it by hand and return */
  } else if (c == keyboardNull || c == keyboardViAdjust) {
    Do(c, 0);
    return;

  /* otherwise, if we got an undo request, get the repeat count and */
  /* perform the undo function -- then return before updating the */
  /* "current" index in the undoCList (so that the user can keep going */
  /* back in time by requesting subsequent "undos". */
  } else if (c == keyboardUndo) {
    count = GenericRead();
    UndoBlock(count);
    return;

  /* otherwise, if we got an "undo all" request, just restore the */
  /* most recently saved command-line */
  } else if (c == keyboardUndoAll) {
    NewLine(undoLine);

  /* otherwise we have a real edit command to execute -- begin with a */
  /* fresh doCList */
  } else {
    CListEmpty(doCList);

    /* if this is an extended-do, set up the static state variables which */
    /* will be used by the DoBlock() routine -- and put us into */
    /* extended-do mode */
    if (c == keyboardExtendedDo) {
      extendedDo = TRUE;
      extendedDoNext = 0;
      extendedViForward = FALSE;
      extendedViBackward = FALSE;

    /* otherwise, if this is not a do-block, just treat it as a single */
    /* edit command and wrap it into a dummy do-list with a <repeat */
    /* count> of 1. */
    } else if (c != keyboardDo) {
      CListAdd(doCList, c);
      if (Do2(c)) {
        CListAdd(doCList, GenericRead());
      }
      CListAdd(doCList, keyboardEndDo);
      CListAdd(doCList, 1);
    }

    /* then execute the do-block in its entirety, or the extended-do */
    /* block until data is available to be returned to the word module. */
skip:
    DoBlock(doCList, 1, 0, NULL);

    /* if we are still in the middle of an extended-do, just return -- */
    /* data to return to the word module is in the cList. */
    if (extendedDo) {
      return;
    }

    /* Otherwise, if we just finished a real do-block, check to see if */
    /* it modified the command-line.  If it did, save it as the */
    /* redoCList so that we can repeat it if the user executes the vi-mode */
    /* "." command */
    if (! newDo) {
      temp = redoCList;
      redoCList = doCList;
      doCList = temp;
    }
  }

  /* then update our "current" index into the undoCList so that the */
  /* action we just did will be the next to be undone. */
  undoCurrent = CListCount(undoCList);
}

/*****************************************************************************/
/*****************************************************************************/
/*************************** external routines *******************************/
/*****************************************************************************/
/*****************************************************************************/

/***************************** EditInitialize() ******************************/
/*                                                                           */
/* This routine performs local initialization for the edit module            */
/*                                                                           */
/*****************************************************************************/


void EditInitialize()
{
  assert(sizeof(Undo) == sizeof(int));

  cList = CListCreate();
  altCList = CListCreate();
  undoCList = CListCreate();
  doCList = CListCreate();
  altDoCList = CListCreate();
  redoCList = CListCreate();

  ViInitialize();
  EmacsInitialize();
}

/***************************** EditBegin() ***********************************/
/*                                                                           */
/* This routine performs pre-line initialization for the edit module.  It    */
/* basically just starts the user out with an empty editLine.                */
/*                                                                           */
/*****************************************************************************/

void EditBegin()
{
  assert(CListCount(cList) == 0);

  editIndex = 0;
  editLine[editIndex] = '\0';
  editEOL = 0;
  leftMargin = 0;
  hardLeftMargin = 0;
  editOverwrite = 0;
  partialLine[0] = '\0';

  NewLine("");
  switch (editMode) {
    case editEmacs : EmacsBegin();
                     break;
    case editVi    : ViBegin();
                     break;
    default        : KeyboardBegin();
                     break;
  }
}

/****************************** EditRead() ***********************************/
/*                                                                           */
/* This routine reads a virtual character from the edit module.  This is     */
/* either an "editDelete" (implying that the cursor moved backward), an      */
/* "editAdd" (implying that the cursor moved forward over the "editWord"     */
/* string), or an action of some kind (e.g., <Return>).                      */
/*                                                                           */
/* This routine identifies the main interface in the keysh code between      */
/* dumb character processing and the smarter softkey processing.             */
/*                                                                           */
/*****************************************************************************/

int EditRead(editWord)
char *editWord;
{
  int c;
  int index;

  /* continue processing editing commands until we get something to return */
  while(! (c = CListPeek(cList))) {
    DoEdit();
  }

  /* then (for performance reasons), attempt to merge consecutive */
  /* added characters into a single "editAdd" result (with "editWord" */
  /* being the string of chars merged). */
  if (KEYBOARDchar(c)) {
    index = 0;
    do {
      c = CListRemove(cList);
      editWord[index++] = c;
      if (KEYBOARDavailChar(c) && ! CListPeek(cList) && GenericAvail()) {
        DoEdit();
      }
    } while (KEYBOARDavailChar(c) && KEYBOARDavailChar(CListPeek(cList)));
    editWord[index] = '\0';
    return editAdd;

  /* return "editDelete"s and other actions here */
  } else {
    return CListRemove(cList);
  }
}

/****************************** EditAvail() **********************************/
/*                                                                           */
/* This routine returns non-zero if a normal character is immediately        */
/* available for reading from the EditRead() routine -- it is used to        */
/* avoid unnecessary duplicate work in the word module.                      */
/*                                                                           */
/*****************************************************************************/

int EditAvail()
{
  int c;

  c = CListPeek(cList);
  if (KEYBOARDchar(c)) {
    return editAdd;
  } else if (c == editDelete) {
    return c;
  }
  return NULL;
}

/****************************** EditEnd() ************************************/
/*                                                                           */
/* This routine performs post-line clean-up for the edit module.             */
/*                                                                           */
/*****************************************************************************/

void EditEnd()
{
  switch (editMode) {
    case editEmacs : EmacsEnd();
                     break;
    case editVi    : ViEnd();
                     break;
    default        : KeyboardEnd();
                     break;
  }
}

/*****************************************************************************/
