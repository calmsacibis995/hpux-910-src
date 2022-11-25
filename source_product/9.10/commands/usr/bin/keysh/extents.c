/*
static char rcsId[] =
  "@(#) $Header: extents.c,v 66.14 91/01/01 15:16:05 rpt Exp $";
static char copyright[] =
  "Copyright (c) HP, 1989";
*/


/********************************** extents.c ********************************/
/*                                                                           */
/* This module is the heart of keysh.  It is responsible for recognizing     */
/* softkey commands, navigating the softkey hierarchy, and preventing the    */
/* user from entering syntactically incorrect commands.                      */
/*                                                                           */
/* This module maintains a list of "extents", each corresponding to a        */
/* portion of the command-line being entered by the user -- typically        */
/* either a softkey command/option name or some other ksh word (e.g., a      */
/* pipe or semi-colon).                                                      */
/*                                                                           */
/* As the user types and deletes text (or even moves the cursor right or     */
/* left), extents are added and removed from the extent list.  In this       */
/* way, the extent list always exactly describes the command-line up to      */
/* the current cursor position.                                              */
/*                                                                           */
/* Each extent in the extent list also contains softkey information, such    */
/* as which softkeys which softkeys should be displayed at this point in     */
/* the command-line and which softkey, if any, the user actually selected.   */
/* See softkeys(4) for the rules of softkey navigation -- these describe     */
/* the relationship between softkeys selected by the user and those          */
/* subsequently displayed on the softkey labels.                             */
/*                                                                           */
/* Throughout this module, three extents in the extent list are accessed     */
/* most.  These are:                                                         */
/*                                                                           */
/*   globalHeadExtent    - the first extent in the extent list.  If the      */
/*                         command-line is empty, this is equal to the       */
/*                         globalTailExtent.                                 */
/*                                                                           */
/*   globalLastExtent    - the last *non-empty* extent in the extent list.   */
/*                         This corresponds to the word which the user       */
/*                         is currently entering on the command-line.  If    */
/*                         the command-line is empty, this is NULL.          */
/*                                                                           */
/*   globalTailExtent    - the last extent on the extent list.  This         */
/*                         extent is always empty (though never NULL) and    */
/*                         holds a place for the *next* word to be entered   */
/*                         by the user.                                      */
/*                                                                           */
/* In general, all activity occurs at the end of the extent list.            */
/*                                                                           */
/* When the word module indicates that we have to add a new word to the      */
/* command-line, we basically put that word in the globalTailExtent and      */
/* make globalLastExtent point to it.  We then allocate a new (empty)        */
/* globalTailExtent and append it to the end of the extents list.  Then,     */
/* we check the word in the globalLastExtent to see if it is a softkey.      */
/* If it is, we update the info in the globalTailExtent which indicates      */
/* which softkeys should be displayed next.                                  */
/*                                                                           */
/* When the word module indicates that we should update the last word        */
/* in the extent list, we just save the new word in the globalLastExtent     */
/* and repeate the softkey checks (mentioned above) to see if the updated    */
/* last word is now a softkey.                                               */
/*                                                                           */
/* When the word module indicates that we should delete the last word        */
/* from the command-line, we just delete the globalTailExtent and then       */
/* empty the globalLastExtent -- making it the new globalTailExtent.         */
/*                                                                           */
/* In this way, the extent list (which represents the parsing of the         */
/* softkey command) is created and destroyed little by little --             */
/* distributing the (possibly expensive) softkey processing throughout       */
/* the duration of command entry.  Note that if the user hits <Return>       */
/* with his cursor already at the end of the command-line (normally the      */
/* case), the extent list is already complete so no additional parsing       */
/* need occur (though translation still need occur -- see translate.y).      */
/*                                                                           */
/*****************************************************************************/


#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <sys/types.h>

#include "keyshell.h"
#include "global.h"
#include "config.h"
#include "extents.h"
#include "word.h"
#include "edit.h"
#include "keyboard.h"
#include "display.h"
#include "debug.h"

#include "message.h"
#include "quote.h"
#include "string2.h"


/***************************** CheckLastExtent() *****************************/
/*                                                                           */
/* This routine checks if the word in the last (not tail!) extent of the     */
/* extent list corresponds to a valid softkey selection.  If so, it returns  */
/* a pointer to the corresponding GlobalSoftKey structure.  Otherwise, it    */
/* returns NULL.  If the word does not correspond to a valid softkey         */
/* selection but is a (proper) prefix of one, *prefix will be set to TRUE.   */
/* (This allows a calling routine to postpone declaring an error until it    */
/* is sure that the user is "going astray".)                                 */
/*                                                                           */
/* Note that any word entered by the user will correspond to a valid         */
/* string-type softkey (e.g., <file>).                                       */
/*                                                                           */
/*****************************************************************************/

static GlobalSoftKey *CheckLastExtent(prefix)
int *prefix;
{
  int len;
  char *end;
  GlobalExtent *e;
  int keyshellFound;
  GlobalSoftKey *sk;
  GlobalSoftKey *rsk;
  GlobalSoftKey *ssk;

  ssk = NULL;
  *prefix = FALSE;
  keyshellFound = FALSE;

  /* find the true length of the word just entered by the user. */
  e = globalLastExtent;
  assert(e);
  end = QuoteEnd(e->word);
  assert(end);
  len = end - e->word;

  /* then compare that word to each of the displayed softkeys */
  for (rsk = HierHeadChild(e->displayed); rsk; rsk = HierNextSibling(rsk)) {
    sk = GlobalSoftKeyCheck(e, rsk);

    /* if the softkey was enabled and is an "option" type softkey, then */
    /* check if the word selected it.  If so, return the selected */
    /* softkey structure */
    if (sk && sk->attrib.softKey) {
      if (! keyshellFound) {
        if (strncmp(e->word, sk->literal, (size_t)len) == 0) {
          if (! sk->literal[len]) {
            globalTailExtent->flags.visible = TRUE;
            return sk;
          }
          *prefix = TRUE;
        }
      }
      if (globalOptions.enableInvisibles || sk->attrib.keyshell) {
        if (strncmp(e->word, sk->alias, (size_t)len) == 0) {
          if (! sk->alias[len]) {
            return sk;
          }
          *prefix = TRUE;
        }
      }

    /* otherwise, if the softkey was enabled and is a "string" type */
    /* softkey, then save it for now -- if we can't match the word against */
    /* any other (preferred) "option" type softkeys, we will return */
    /* this one as selected. */
    } else if (sk && sk->attrib.string && ! ssk) {
      ssk = sk;
    }

    /* check if we have reached the "invisible" softkey commands in the */
    /* top-level softkey menu -- alter our search algorithm if so. */
    if (rsk->attrib.keyshell) {
      keyshellFound = TRUE;
    }
  }

  /* this should really be part of word.c -- it tries to impose some */
  /* sanity on words which can be used to select "string" type softkeys */
  /* (e.g., we don't want to allow the user to specify "-a" as a <file>) */
  if ((e->word[0] == '-' || e->word[0] == '+') && StringSig(e->word+1)) {
    return NULL;
  } else if (e->word[0] == '(' || e->word[0] == ')') {
    return NULL;
  }

  /* else the user entered a word which validly selected a "string" type */
  /* softkey */
  return ssk;
}


/****************************** LastTimeDisplayed() **************************/
/*                                                                           */
/* This routine returns a pointer to the most recent extent which caused the */
/* specified softkey to be last displayed.  We use this info so that upon    */
/* returning from a sub-menu, the parent menu's enable, disable, and         */
/* etcetera counts are restored to where they were when the user last saw    */
/* them.                                                                     */
/*                                                                           */
/*****************************************************************************/

static GlobalExtent *LastTimeDisplayed(sk)
GlobalSoftKey *sk;
{
  GlobalExtent *e;

  for (e = globalLastExtent; e && e->type == typeSoftKey; e = LinkPrev(e)) {
    if (e->displayed == sk) {
      return e;
    }
  }
  return NULL;
}


/****************************** IsError() ************************************/
/*                                                                           */
/* This routine checks to see if the combination of the "pe" extent,         */
/* followed by the "e" extent would be an error.  It returns an appropriate  */
/* error message if so; it returns NULL otherwise.  Basically, we            */
/* declare an error if "pe" corresponded to a non-invisible softkey which    */
/* required that a (subsequent) softkey be selected, and "e" was not even    */
/* a softkey.                                                                */
/*                                                                           */
/*****************************************************************************/

static char *IsError(pe, e)
GlobalExtent *pe;
GlobalExtent *e;
{
  GlobalSoftKey *sk;

  sk = pe->selected;
  if (sk && sk->nextRequired && e->flags.visible && e->type != typeSoftKey) {
    if (GlobalSoftKeyCheck(e, sk->nextRequired)) {
      return sk->nextRequired->required;
    }
  }
  return NULL;
}


/****************************** CheckErrors() ********************************/
/*                                                                           */
/* This routine generates any prompt or error messages related to the        */
/* word just entered by the user.  It can also set the "error" field for     */
/* an extent which a specific error has been detected.                       */
/*                                                                           */
/*****************************************************************************/

static void CheckErrors(notify, prefix)
int notify;
int prefix;
{
  char *s;
  char *p;
  char *end;
  GlobalExtent *e;
  GlobalExtent *ne;
  GlobalExtent *pe;
  GlobalSoftKey *sk;
  GlobalSoftKey *fsk;
  char prompt[EDITlength];

  if (e = globalLastExtent) {
    ne = globalTailExtent;
    pe = LinkPrev(e);

    /* default to no errors in this extent */
    e->error = NULL;

    /* check to see if the user has tried to enter a non-softkey extent */
    /* (e.g., pipe, semi-colon, or plain old garbage) following an */
    /* intrinsic command.  Flag an error if so -- this is not allowed */
    sk = globalHeadExtent->selected;
    if (sk && sk->code && e->type != typeSoftKey) {
      e->error = MESSAGEnotAllowedHere;
      ne->displayed = globalEmptySoftKey;
    }

    /* Then check to see if the user has entered garbage following a */
    /* normal softkey command.  Flag an error if so -- this is not allowed */
    if ((e->type == typeUnknown || e->type == typeIncomplete) &&
        pe && pe->type == typeSoftKey && e->flags.visible) {
      if (e->type == typeIncomplete) {
        e->error = MESSAGEincomplete;
      } else {
        e->error = MESSAGEnotAllowedHere;
      }
    }

    if (notify) {

      /* if prompts are enabled, then check to see if the current command */
      /* would be in error if the user stopped right here.  if so, we */
      /* want to prompt him to keep going (i.e., something is required). */
      if (globalOptions.enablePrompts) {
        s = IsError(e, ne);
        if (s) {

          /* At this point, we know that the user must select (at least) */
          /* another softkey to complete the command.  We first check */
          /* what the type of the required softkey is.  If it is a "string" */
          /* type and he hasn't yet typed a <space> following the last */
          /* word he entered, then we prompt him for one now.  Otherwise, */
          /* we check to see if the user has other softkeys which he */
          /* can select prior to or instead of selecting the required one. */

          /* Based on these checks, we prompt the user with either */
          /* "<required message>", "Select an option or <required message>", */
          /* or "Select any options; then, <required message>" -- just */
          /* to make things look nice. */
          sk = e->selected;
          end = QuoteEnd(e->word);
          (void)GlobalFirstDisplayedSoftKey(&fsk);
          if (end && ! *end && sk->nextRequired->attrib.string) {
            DisplayHint(MESSAGEpressSpace, FALSE);
          } else if (fsk == sk->nextRequired || fsk->required ||
                     sk->nextRequired->attrib.empty) {
            DisplayHint(s, FALSE);
          } else if (fsk->disable >= sk->nextRequired->position ||
                     fsk->enable < sk->nextRequired->position &&
                     sk->nextRequired->attrib.disabled) {
            strcpy(prompt, MESSAGEselectAnOption);
            goto skip;
          } else {
            strcpy(prompt, MESSAGEselectAnyOptions);
          skip:
            p = StringEnd(prompt);
            strcpy(p, s);
            *p = tolower(*p);
            DisplayHint(prompt, FALSE);
          }
        } else {
          DisplayHint(NULL, FALSE);
        }
      }

      /* Finally, check if the user just created an error (e.g., typed */
      /* a semi-colon when he still had a required softkey to select). */
      /* if so, display the error message now */
      if (pe && (s = IsError(pe, e)) || ! prefix && (s = e->error)) {
        if (! e->flags.errored && ! prefix && e->type != typeIncomplete) {
          DisplayHint(s, TRUE);
          e->flags.errored = TRUE;
        } else if (globalOptions.enablePrompts) {
          DisplayHint(s, FALSE);
        }
      }
    }
  }
}


/*****************************************************************************/
/*****************************************************************************/
/*****************************************************************************/
/*****************************************************************************/
/*****************************************************************************/


/****************************** FillTailExtent() *****************************/
/*                                                                           */
/* This routine just ensures that the tail extent of the extent list is      */
/* empty and then figures out which sotfkeys it should display.              */
/*                                                                           */
/*****************************************************************************/

#define REUSE   0
#define CLEAR   1
#define BEGIN   2

static void FillTailExtent(how)
int how;
{
  GlobalExtent *e;
  GlobalExtent *last;
  GlobalSoftKey *sk;

  if (globalTailExtent->word) {
    free((void *)globalTailExtent->word);
  }
  globalTailExtent->flags.errored = FALSE;
  globalTailExtent->word = NULL;
  globalTailExtent->type = typeUnknown;
  globalTailExtent->selected = NULL;
  globalTailExtent->error = NULL;
  if (how == REUSE) {
    return;
  }
  if (how != BEGIN) {
    globalTailExtent->etcetera = 0;
  }
  globalTailExtent->displayed = globalEmptySoftKey;
  globalTailExtent->enabled = -1;
  globalTailExtent->disabled = -1;
  if (e = globalLastExtent) {
    if (e->selected) {

      /* if the last extent selected a valid softkey and that softkey had */
      /* children, then display its children next.  We are moving down */
      /* the softkey hierarchy. */
      if (HierChildCount(e->selected)) {
        globalTailExtent->displayed = e->selected;
        sk = HierHeadChild(e->selected);
        if (sk->required && ((sk->states & e->state) != e->state)) {
          globalTailExtent->enabled = sk->enable;
        }

      /* otherwise, if the last extent selected a softkey without */
      /* children, then we have to move up the softkey hierarchy to find */
      /* which softkeys should be displayed next.  See softkeys(4) for */
      /* the algorithm. */
      } else {
        sk = e->selected;
        while (sk = HierParent(sk)) {
          last = LastTimeDisplayed(sk);
          assert(last);
          if (HierChildCount(sk) > last->selected->disable+1) {
            globalTailExtent->displayed = sk;
            globalTailExtent->enabled = last->selected->enable;
            globalTailExtent->disabled = last->selected->disable;
            globalTailExtent->etcetera = last->etcetera;
            if (sk->etceteras) {
              GlobalEtcThis();
            }
            break;
          }
        }
      }
    }

  /* if we are at the beginning of the command-line on a $PS1 prompt then */
  /* display the top-level softkey command menu.  otherwise, just blank */
  /* the softkeys -- we don't dare help the user on a $PS2... */
  } else if (keyshellPSn == 1) {
    globalTailExtent->displayed = globalSoftKeys;
    globalTailExtent->state = GLOBALstateCommand | globalState;
    globalTailExtent->flags.visible = FALSE;
  } else {
    globalTailExtent->state = 0;
    globalTailExtent->flags.visible = FALSE;
  }

  /* make sure that the softkey accelerator characters and etcetera */
  /* counts we are going to display are up-to-date. */
  ConfigAssignAcceleratorsEtc(globalTailExtent->displayed);
}


/***************************** AddTailExtent() *******************************/
/*                                                                           */
/* This routine just adds an empty (uninitialized) extent to the end of      */
/* the extent list.                                                          */
/*                                                                           */
/*****************************************************************************/

static void AddTailExtent()
{
  GlobalExtent *e;

  e = (GlobalExtent *)malloc(sizeof(*e));
  memset((void *)e, '\0', sizeof(*e));
  LinkAddTail(globalExtents, e);
  globalTailExtent = LinkTail(globalExtents);
  globalLastExtent = LinkPrev(globalTailExtent);
  globalHeadExtent = LinkHead(globalExtents);
}


/***************************** UpdateLastExtent() ****************************/
/*                                                                           */
/* This routine updates the last (not tail!) extent in the extent list to    */
/* correspond to the newly updated word (of type "type") returned by the     */
/* word module.  It will set *prefix to TRUE if that word is a proper prefix */
/* of a softkey.                                                             */
/*                                                                           */
/*****************************************************************************/

static void UpdateLastExtent(word, type, prefix)
char *word;
GlobalType type;
int *prefix;
{
  GlobalExtent *e;

  e = globalLastExtent;
  assert(e);
  *prefix = FALSE;
  globalSoftKeyBlank = TRUE;

  /* update the word in the last extent of the extent list */
  if (e->word) {
    free((void *)e->word);
  }
  e->word = (char *)malloc(strlen(word)+1);
  strcpy(e->word, word);
  e->type = type;
  e->selected = NULL;

  /* propagate any state info to the tail extent */
  globalTailExtent->state = e->state;
  globalTailExtent->flags.visible = e->flags.visible;

  /* if we still don't know the type of the word (i.e., not ksh-special) */
  /* then check if it might be a valid softkey selection. */
  if (type == typeUnknown) {
    e->selected = CheckLastExtent(prefix);
    if (e->selected) {
      e->type = typeSoftKey;
      type = e->type;
    }
  }

  /* then check to see if the word might have changed our state (e.g., */
  /* a pipe or semi-colon).  If so, update the state and put us back */
  /* to the top-level softkey menu. */
  if (type == typePipe && e->state) {
    e->selected = globalSoftKeys;
    globalTailExtent->state = GLOBALstateFilter | globalState;
    globalTailExtent->flags.visible = FALSE;
  } else if (type == typeSeparator && e->state) {
    e->selected = globalSoftKeys;
    globalTailExtent->state = GLOBALstateCommand | globalState;
    globalTailExtent->flags.visible = FALSE;
  } else if (type == typeComment) {
    globalTailExtent->state = 0;
    globalTailExtent->flags.visible = FALSE;
  }

  /* only turn the softkeys on if the user really selected one */
  if (e->selected) {
    globalSoftKeyBlank = FALSE;
  }
}


/**************************** DeleteTailExtent() *****************************/
/*                                                                           */
/* This routine just removes the tail extent from the extent list and        */
/* discards it.                                                              */
/*                                                                           */
/*****************************************************************************/

static void DeleteTailExtent()
{
  free((void *)LinkRemove(globalTailExtent));
  globalTailExtent = LinkTail(globalExtents);
  assert(globalTailExtent);
  globalLastExtent = LinkPrev(globalTailExtent);
  globalHeadExtent = LinkHead(globalExtents);
  if (! globalLastExtent || globalLastExtent->selected) {
    globalSoftKeyBlank = FALSE;
  }
}


/*****************************************************************************/
/*****************************************************************************/
/*****************************************************************************/
/*****************************************************************************/
/*****************************************************************************/


/**************************** CheckBackups() *********************************/
/*                                                                           */
/* This routine checks if backup softkeys should be displayed at this time   */
/* (i.e., if we are blanking the softkeys or we would otherwise display the  */
/* top-level softkey menu and the display of visible softkeys is disabled).  */
/* It sets globalSoftKeyBackup to TRUE if so.                                */
/*                                                                           */
/*****************************************************************************/

static void CheckBackups()
{
  GlobalSoftKey *sk;

  globalSoftKeyBackup = FALSE;
  sk = globalTailExtent->displayed;
  if (sk == globalEmptySoftKey ||
      sk == globalSoftKeys && ! globalOptions.enableVisibles) {
    globalSoftKeyBackup = TRUE;
  }
}


/****************************** FirstError() *********************************/
/*                                                                           */
/* This routine scans the entire extent list for the first error where       */
/* either a required softkey was missing or some alternate error had been    */
/* previously flagged.  It returns a pointer to the extent in error and      */
/* sets *errorString to the appropriate error message.                       */
/*                                                                           */
/*****************************************************************************/

static GlobalExtent *FirstError(errorString)
char **errorString;
{
  GlobalExtent *e;
  GlobalExtent *pe;

  pe = globalHeadExtent;
  while (e = LinkNext(pe)) {
    if ((*errorString = IsError(pe, e)) || (*errorString = e->error)) {
      return e;
    }
    pe = e;
  }
  return NULL;
}


/******************************** Backward() *********************************/
/*                                                                           */
/* This routine is used as a temporary keyboard input feeder function        */
/* following the detection of a command-line error after the user presses    */
/* <Return>.  It causes the cursor to go backward until it reaches the       */
/* first detected error (at which time, this routine is uninstated).         */
/*                                                                           */
/*****************************************************************************/

static int Backward()
{
  return keyboardBackward;
}


/*****************************************************************************/
/*****************************************************************************/
/*************************** external functions ******************************/
/*****************************************************************************/
/*****************************************************************************/


/******************************* ExtentsInitialize() *************************/
/*                                                                           */
/* This routine performs local initialization for the extents module.  It    */
/* simply creates an empty extents list.                                     */
/*                                                                           */
/*****************************************************************************/

void ExtentsInitialize()
{
  static GlobalExtent base;

  LinkCreateBase(&base);
  globalExtents = &base;
  AddTailExtent();
  FillTailExtent(CLEAR);
  WordInitialize();
}


/******************************* ExtentsBegin() ******************************/
/*                                                                           */
/* This routine performs pre-line initialization for the extents module.     */
/* It simply empties the extents list.                                       */
/*                                                                           */
/* (Note that the extent list cannot be freed during the previous call       */
/* to ExtentdEnd() since it subsequently used by the translate module.)      */
/*                                                                           */
/*****************************************************************************/

void ExtentsBegin()
{
  while (globalLastExtent) {
    DeleteTailExtent();
    FillTailExtent(REUSE);
  }

  if (KshGetEnv(KEYSHELLetcetera)) {
    FillTailExtent(CLEAR);
  } else {
    FillTailExtent(BEGIN);
  }

  WordBegin();
}


/******************************* ExtentsRead() *******************************/
/*                                                                           */
/* This routine reads a command-line from the user.  It displays the         */
/* appropriate softkey menus and checks for subsequent softkey selections    */
/* and navigates the softkey hierarchy accordingly.                          */
/*                                                                           */
/* This routine will not return until it has parsed an entire valid          */
/* command-line.  It's return value is either keyboardTranslateVerify or     */
/* keyboardExecuteVerify, depending on whether the user pressed the          */
/* <Insert line> or <Return> key to enter the command.                       */
/*                                                                           */
/*****************************************************************************/

int ExtentsRead()
{
  char *word;
  int prefix;
  int action;
  int notify;
  GlobalType newType;
  char *errorString;
  char newWord[EDITlength];
  GlobalExtent *firstError;

  firstError = NULL;
  do {

    /* if we are backing up the cursor to the first error in the */
    /* command-line and we have reached it, then display the appropriate */
    /* error message and let the user have another try to fix the command. */
    if (firstError && firstError == globalTailExtent) {
      firstError = NULL;
      KeyboardInputFeeder(NULL);
      KeyboardWrite(keyboardNull);
      DisplayHint(errorString, TRUE);
    }

    CheckBackups();

    /* then get the next word (or updated word) for the command-line */
    word = "";
    if (globalLastExtent) {
      word = globalLastExtent->word;
    }
    action = WordRead(word, newWord, &newType);

    prefix = FALSE;

    /* if the user just deleted the last word on the command-line then */
    /* remove the tail extent in the extent list */
    if (action == wordDelete) {
      DeleteTailExtent();
      FillTailExtent(REUSE);

    /* otherwise, if the user added a new word to the command-line, then */
    /* add a new extent to the extent list and then put the word into */
    /* the last extent and check if it is a softkey, etc.  if the user */
    /* just changed the last word in the command-line, then just update the */
    /* last extent and re-check if it is a softkey, etc. */
    } else if (action == wordUpdate || action == wordAdd) {
      if (action == wordAdd) {
        AddTailExtent();
      }
      UpdateLastExtent(newWord, newType, &prefix);
      FillTailExtent(CLEAR);
    }

    /* then check for errors in the command-line and let the user */
    /* know if any are found.  Note that if the cursor isn't actually */
    /* stopping here (e.g., the user is moving the cursor to the */
    /* beginning or end of the line), we won't notify him about the error */
    /* until he hits <Return>. */
    notify = FALSE;
    if (! firstError && ! WordAvail()) {
      notify = TRUE;
    }
    CheckErrors(notify, prefix);

    /* if the user hit <Return>, check if there are any errors in the */
    /* command-line.  If so, throw out the <Return> and start the */
    /* cursor moving backwards.  We'll stop the cursor when it reaches */
    /* the error.  Otherwise, if no errors are found, then verify the */
    /* <Return> for the rest of the modules. */
    if (KEYBOARDaction(action)) {
      if (firstError = FirstError(&errorString)) {
        if (! globalInteractive) {
          DisplayPrintf(MESSAGEautoConfigError);
          DisplayPrintf("%S \n", editLine);
          DisplayFlush();
          firstError = NULL;
          KeyboardWrite(keyboardInterrupt);
        } else {
          KeyboardInputFeeder(Backward);
        }
      } else {
        KeyboardWrite(KEYBOARDaToV(action));
      }
    }
  } while (! KEYBOARDverify(action));
  return action;
}


/********************************* ExtentsEnd() ******************************/
/*                                                                           */
/* This routine performs post-line clean-up for the extents module.          */
/*                                                                           */
/*****************************************************************************/

void ExtentsEnd()
{
  globalSoftKeyBackup = TRUE;

  WordEnd();
}


/*****************************************************************************/
