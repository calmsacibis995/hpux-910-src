/*
static char rcsId[] =
  "@(#) $Header: global.c,v 66.11 91/01/01 15:16:55 rpt Exp $";
static char copyright[] =
  "Copyright (c) HP, 1989";
*/


/********************************* global.c **********************************/
/*                                                                           */
/* This routine defines most of the global data structures used throughout   */
/* keyshell.  It also defines some common routines which can be used to      */
/* manipulate those data structures.                                         */
/*                                                                           */
/*****************************************************************************/


#define SINGLE
#define MINICURSES

#include <curses.h>
#include <termio.h>
#include <term.h>
#include <stdlib.h>
#include <string.h>

#include "keyshell.h"
#include "global.h"
#include "vi.h"
#include "keyboard.h"

#include "message.h"
#include "string2.h"


/********************************** globalState ******************************/
/*                                                                           */
/* This variable contains either GLOBALstateHPPA or GLOBALstateMotorola,     */
/* depending on our SPU type.  It is used to evaluate the "motorola"         */
/* and "precision" softkey attributes.                                       */
/*                                                                           */
/*****************************************************************************/

int globalState;


/******************** globalInteractive / globalNonInteractive ***************/
/*                                                                           */
/* These two variables indicate whether keysh is reading from the terminal   */
/* or from an rc file.  They are effectively compliments of each other,      */
/* except for a small phase lag.                                             */
/*                                                                           */
/* globalInteractive is set FALSE and globalNonInteractive is set TRUE in    */
/* intrinsics:ReadRCFile().  globalNonInteractive is set FALSE as soon       */
/* as intrinsics:ReadRCChar() reaches the end of the input buffer;           */
/* globalInteractive is subsequently set TRUE in IntrinsicsBegin() (i.e.,    */
/* the next time a keyshell command is starting to be read).                 */
/*                                                                           */
/*****************************************************************************/

int globalInteractive;
int globalNonInteractive = TRUE;


/******************************** globalOptions ******************************/
/*                                                                           */
/* These are the global keysh configuration options which can be changed     */
/* using the "kc options" built-in command.  They are all boolean variables  */
/* and include:                                                              */
/*                                                                           */
/*   enableTranslations      - enable display of translated HP-UX commands   */
/*   enableAccelerators      - enable upper-case selector characters         */
/*   enableHelpKey           - enable "--Help--" softkey on f1               */
/*   enableBackups           - enable the display of backup softkeys         */
/*   enableInvisibles        - enable processing of invisible softkeys       */
/*   enableVisibles          - enable display/processing of visible softkeys */
/*   enablePrompts           - enable the display of prompt messages         */
/*                                                                           */
/*****************************************************************************/

GlobalOptions globalOptions;


/***************************** globalExtents *********************************/
/*                                                                           */
/* globalExtents is a pointer to the base of the extents list.  Each element */
/* in the extents list has the following fields:                             */
/*                                                                           */
/* link            - link to next extent in the extentlist                   */
/* flags.errored   - boolean indicating user has already been warned of      */
/*                   an error in this extent                                 */
/* flags.visible   - boolean indicating user has selected a true softkey     */
/*                   in this command -- so keysh can turn on error checking  */
/* displayed       - pointer to parent of softkeys to be displayed at        */
/*                   this point in the command                               */
/* enabled         - position of first explicitly enabled displayed softkey  */
/* disabled        - position of first disabled displayed softkey            */
/* etcetera        - etcetera bank of displayed softkeys                     */
/* state           - state (e.g., filter or command) associated with this    */
/*                   extent                                                  */
/* word            - actual command-line word associated with this extent    */
/* type            - type (e.g., softkey, pipe) of this word                 */
/* selected        - pointer to softkey selected by this word (if type ==    */
/*                   typeSoftKey                                             */
/* error           - error message associated with this extent               */
/*                                                                           */
/* Three pointers exist to speed access into the extents list.  They are:    */
/*                                                                           */
/*   globalHeadExtent   - pointer to head extent in list                     */
/*   globalTailExtent   - pointer to tail extent in list (always empty)      */
/*   globalLastExtent   - pointer to last non-empty extent in list (or       */
/*                        NULL if the list is empty.                         */
/*                                                                           */
/* Note that there is always an empty (though valid) extent as the tail      */
/* extent in the list.  For an empty command-line, globalHeadExtent ==       */
/* globalTailExtent and globalLastExtent == NULL.                            */
/*                                                                           */
/*****************************************************************************/

GlobalExtent *globalExtents;          /* base */
GlobalExtent *globalHeadExtent;
GlobalExtent *globalLastExtent;
GlobalExtent *globalTailExtent;


/***************************** globalSoftKeys ********************************/
/*                                                                           */
/* This variable is a pointer to the root node of the entire softkey         */
/* hierarchy.  Below this are the individual softkey commands themselves,    */
/* below which are the corresponding softkey options.  Each node in the      */
/* hierarchy has the following fields:                                       */
/*                                                                           */
/* hier             - node into the global softkey hierarchy                 */
/* attrib.softKey   - boolean indicating this is a "option" type softkey     */
/* attrib.string    - boolean indicating this is a "string" type softkey     */
/* attrib.empty     - boolean indicating this is an empty softkey            */
/* attrib.etcetera  - special case boolean indicating the --Etc-- softkey    */
/* attrib.keyshell  - special case boolean indicating the Keysh_config       */
/*                    softkey                                                */
/* attrib.disabled  - boolean indicating this softkey is disabled by default */
/* attrib.backup    - boolean indicating this is a backup softkey            */
/* attrib.automatic - boolean indicating this is an automatic softkey        */
/* states           - set of states to which this softkey applies            */
/* disable          - position of first softkey disabled by this one         */
/* enable           - position of first softkey enabled by this one          */
/* position         - position of this softkey (relative to siblings)        */
/* etceteras        - number of etceteras needed for this softkey's children */
/* label[2]         - two lines of the label to display for this softkey     */
/* accelerator      - the accelerator character to use for this softkey      */
/* literal          - the literal command-line string for this softkey       */
/* alias            - the official softkey name from the softkey file        */
/* editRule         - the editrule for this softkey (see softkeys(4))        */
/* cleanUpRule      - the clean-up editrule for this softkey (see softkeys(4)*/
/* help             - the helptext for this softkey (see softkeys(4))        */
/* hint             - the hint message for this softkey (string types only)  */
/* required         - the required error message to print if this softkey    */
/*                    is visible but not selected                            */
/* nextRequired     - a pointer to the next required softkey which the user  */
/*                    will see                                               */
/* code             - the intrinsics command code for this node              */
/* file             - the file name this softkey was read from               */
/* st_mtime         - the modify time of the file this softkey was read from */
/* chunk            - the chunk used to allocate all memory for this softkey */
/*                                                                           */
/* Within this hierarchy are (fromleft to right), all visible softkeys,      */
/* the Keysh_config softkey, and then all invisible softkeys.                */
/*                                                                           */
/*****************************************************************************/

GlobalSoftKey *globalSoftKeys;        /* root */


/******************************* globalKeyshell ******************************/
/*                                                                           */
/* This variable is a pointer to the root of the Keysh_config command node   */
/* hierarchy.                                                                */
/*                                                                           */
/*****************************************************************************/

GlobalSoftKey *globalKeyshell;        /* root */


/************************* globalBackupSoftKeys ******************************/
/*                                                                           */
/* This is a pointer to the root of backup softkey hierarchy.  At most this  */
/* hierarchy may have eight children, none of which have children of their   */
/* own.  These softkeys are displayed when keysh has no other softkeys to    */
/* display.                                                                  */
/*                                                                           */
/*****************************************************************************/

GlobalSoftKey *globalBackupSoftKeys;  /* root */


/******************************* globalHelpSoftKey ***************************/
/*                                                                           */
/* This is a pointer to the root of the help softkey hierarchy.  These       */
/* softkeys are displayed as help topics when the user presses "--Help--"    */
/* followed by a <Return>.  If the user subsequently picks one of these      */
/* softkeys, its associated help is displayed.  Again, this hierarchy can    */
/* have at most eight children, none of which can have children of their     */
/* own.                                                                      */
/*                                                                           */
/*****************************************************************************/

GlobalSoftKey *globalHelpSoftKey;     /* root */


/******************************** globalEmptySoftKey *************************/
/*                                                                           */
/* This is just a global empty softkey.  It can be displayed to blank        */
/* the softkeys (since it has no children).                                  */
/*                                                                           */
/*****************************************************************************/

GlobalSoftKey *globalEmptySoftKey;


/********* globalSoftKeyBlank / globalSoftKeyBackup / globalSoftKeyHelp ******/
/*                                                                           */
/* The following are three boolean variables.  If "globalSoftKeyBlank" is    */
/* TRUE, the softkeys will be blanked no matter what else.  If               */
/* "globalSoftKeyBackup" is true, the backup softkeys (from                  */
/* globalBackupSoftKeys) will be displayed instead of the normal keysh       */
/* softkeys.  If "globalSoftKeyHelp" is TRUE, the help softkeys (from        */
/* globalHelpSoftKey) will be displayed instead.                             */
/*                                                                           */
/*****************************************************************************/

int globalSoftKeyBlank;
int globalSoftKeyBackup;
int globalSoftKeyHelp;


/****************************** globalSoftKeyTopics **************************/
/*                                                                           */
/* This boolean is set TRUE when we the --Help-- softkey can be used to      */
/* call up a set of help topics.  It basically just changes the second line  */
/* of the help softkey label to include the string "(topics)".               */
/*                                                                           */
/*****************************************************************************/

int globalSoftKeyTopics;


/********************************* globalAccelerate **************************/
/*                                                                           */
/* This boolean is set to TRUE if either the user has enabled the            */
/* global "accelerators" option (actually, called "selectors" in the "kc"    */
/* command) or if the terminal doesn't have at least eight function keys.    */
/* When true, upper-case selector characters are recognized as an alternate  */
/* for the function keys.                                                    */
/*                                                                           */
/*****************************************************************************/

int globalAccelerate;


/*****************************************************************************/
/*****************************************************************************/
/******************************** external routines **************************/
/*****************************************************************************/
/*****************************************************************************/


/********************************* GlobalInitialize() ************************/
/*                                                                           */
/* This routine performs initialization for the global module.               */
/*                                                                           */
/*****************************************************************************/

void GlobalInitialize()
{
  char context[1024];
  static GlobalSoftKey empty;
  static GlobalSoftKey help;
  static GlobalSoftKey backups;

  /* initialize all of the softkey hierarchies */
  HierCreateRoot(&empty);
  globalEmptySoftKey = &empty;

  HierCreateRoot(&backups);
  globalBackupSoftKeys = &backups;

  HierCreateRoot(&help);
  globalHelpSoftKey = &help;

  /* then figure out if we are a motorola or precision SPU */
  (void)getcontext(context, sizeof(context));
  if (strstr(context, "HP-PA")) {
    globalState = GLOBALstateHPPA;
  } else {
    globalState = GLOBALstateMotorola;
  }
}


/***************************** GlobalBegin() *********************************/
/*                                                                           */
/* This module performs pre-line initialization for the global module        */
/*                                                                           */
/*****************************************************************************/

void GlobalBegin()
{

  /* check if we should enable the upper-case selector characters */
  globalAccelerate = FALSE;
  if (globalOptions.enableAccelerators || ! key_f1 || ! key_f8) {
    globalAccelerate = TRUE;
  }
}


/****************************** GlobalSoftKeyCheck() *************************/
/*                                                                           */
/* This routine checks if the specified softkey should be displayed for      */
/* the specified extent.  It returns the softkey if so, and NULL if not.     */
/*                                                                           */
/*****************************************************************************/

GlobalSoftKey *GlobalSoftKeyCheck(e, sk)
GlobalExtent *e;
GlobalSoftKey *sk;
{
  if (sk) {

    if (sk->attrib.empty && ! sk->required) {
      return NULL;
    } else if (sk->attrib.keyshell && e != globalHeadExtent) {
      return NULL;
    } else if ((sk->states & e->state) != e->state) {
      return NULL;
    } else if (HierParent(sk) == e->displayed) {
      if (e->disabled >= sk->position) {
        return NULL;
      } else if (sk->attrib.disabled && e->enabled < sk->position) {
        return NULL;
      }
    }
  }
  return sk;
}


/*************************** GlobalDisplayedSoftKeyN() ***********************/
/*                                                                           */
/* This routine returns a pointer to the softkey which should be displayed   */
/* for the nth softkey (where "n" can range from 0 to 7).  If that           */
/* softkey should be blanked, NULL is returned.                              */
/*                                                                           */
/*****************************************************************************/

GlobalSoftKey *GlobalDisplayedSoftKeyN(n)
int n;
{
  int len;
  int selected;
  GlobalSoftKey *sk;
  GlobalSoftKey *rsk;
  static GlobalSoftKey etc;
  static GlobalSoftKey help;
  static GlobalSoftKey helpt;

  /* check if we should be displaying the help topic softkeys -- return */
  /* the appropriate one if so. */
  if (globalSoftKeyHelp) {
    return HierNthChild(globalHelpSoftKey, n);

  /* otherwise check if we should be displaying the backup softkeys -- */
  /* return the appropriate one if so. */
  } else if (globalSoftKeyBackup) {
    if (globalOptions.enableBackups) {
      return HierNthChild(globalBackupSoftKeys, n);
    }
    return NULL;

  /* otherwise, check if we should blank the softkeys -- return NULL if so */
  } else if (globalSoftKeyBlank) {
    return NULL;

  /* else check if this is the "--Help--" softkey -- format it and return */
  /* it if so. */
  } else if (KEYBOARDnToF(n) == KEYBOARDhelp) {
    if (! globalSoftKeyTopics) {
      strcpy(help.label[0], MESSAGEhelpLabel);
      help.accelerator = *MESSAGEhelpAccelerator;
      return &help;
    } else {
      strcpy(helpt.label[0], MESSAGEhelpLabel);
      strcpy(helpt.label[1], MESSAGEhelpTopicsLabel);
      helpt.accelerator = *MESSAGEhelpAccelerator;
      return &helpt;
    }

  /* else check if this is the "--More--" softkey -- format it and return */
  /* it if so. */
  } else if (KEYBOARDnToF(n) == KEYBOARDetc) {
    strcpy(etc.label[0], MESSAGEetcLabel);
    strcpy(etc.label[1], ltoa((long)(globalTailExtent->etcetera+1)));
    strcat(etc.label[1], MESSAGEetcOfLabel);
    len = strlen(etc.label[1]);
    strncat(etc.label[1], ltoa((long)globalTailExtent->displayed->etceteras),
            (size_t)(DISPLAYsoftKeyLength-len));
    etc.accelerator = *MESSAGEetcAccelerator;
    etc.attrib.etcetera = TRUE;
    return &etc;
  }

  /* otherwise we just have a real child node to display -- adjust its */
  /* count by one if we have a "--Help--" softkey in the f1 position. */
  if (globalOptions.enableHelpKey) {
    n--;
  }

  /* then get the appropriate child softkey from the point in the */
  /* global hierarchy we are currently displaying. */
  n += globalTailExtent->etcetera*(KEYBOARDsoftKeyCount-1);
  rsk = HierNthChild(globalTailExtent->displayed, n);

  /* then check if the softkey is enabled, etc. -- return NULL if not. */
  sk = GlobalSoftKeyCheck(globalTailExtent, rsk);
  if (! sk) {
    return NULL;
  }

  /* then check if we need to put a "*" or " " in the lower right corner */
  /* of the softkey to indicate it is selected or not -- do it if so. */
  selected = IntrinsicsSelected(sk);
  if (selected != -1) {
    strcpy(sk->label[1], StringPad(sk->label[1], DISPLAYsoftKeyLength));
    sk->label[1][DISPLAYsoftKeyLength-1] = ((selected)?('*'):(' '));
  }

  return sk;
}


/************************** GlobalFirstDisplayedSoftKey() ********************/
/*                                                                           */
/* This routine returns a pointer to the first displayed softkey (from       */
/* left to right) which the user can currently see.  It also returns the     */
/* integer position of that softkey (relative to its siblings).              */
/*                                                                           */
/*****************************************************************************/

int GlobalFirstDisplayedSoftKey(fsk)
GlobalSoftKey **fsk;
{
  GlobalExtent *e;
  GlobalSoftKey *sk;
  GlobalSoftKey *rsk;

  *fsk = NULL;
  e = globalTailExtent;
  for (rsk = HierHeadChild(e->displayed); rsk; rsk = HierNextSibling(rsk)) {
    sk = GlobalSoftKeyCheck(e, rsk);
    if (sk) {
      *fsk = sk;
      return sk->position;
    }
    if (rsk->attrib.keyshell) {
      return -1;
    }
  }
  return -1;
}


/******************************* GlobalFindSoftKey() *************************/
/*                                                                           */
/* This routine returns a pointer to the immediate child softkey of          */
/* "root" whose name matches "literal".  It returns NULL if none is found.   */
/*                                                                           */
/*****************************************************************************/

GlobalSoftKey *GlobalFindSoftKey(root, literal)
GlobalSoftKey *root;
char *literal;
{
  GlobalSoftKey *sk;

  for (sk = HierHeadChild(root); sk; sk = HierNextSibling(sk)) {
    if (! sk->attrib.backup && ! sk->attrib.empty) {
      if (StringCompareLower(literal, sk->literal) == 0) {
        return sk;
      }
      if (sk->alias && StringCompareLower(literal, sk->alias) == 0) {
        return sk;
      }
    }
  }
  return NULL;
}


/******************************* GlobalEtcThis() *****************************/
/*                                                                           */
/* This routine just auto-etceteras the softkeys so that the first           */
/* enabled softkey is showing in the current etcetera bank.                  */
/*                                                                           */
/*****************************************************************************/

void GlobalEtcThis()
{
  int n;
  GlobalSoftKey *dummy;

  n = GlobalFirstDisplayedSoftKey(&dummy);
  globalTailExtent->etcetera = n/(KEYBOARDsoftKeyCount-1);
}


/****************************** GlobalEtcNext() ******************************/
/*                                                                           */
/* This routine just increments the etcetra bakn, wrapping around to the     */
/* beginning when the last one is reached.                                   */
/*                                                                           */
/*****************************************************************************/

void GlobalEtcNext()
{
  globalTailExtent->etcetera++;
  globalTailExtent->etcetera %= globalTailExtent->displayed->etceteras;
}


/******************************* GlobalEnd() *********************************/
/*                                                                           */
/* This routine performs post-line clean-up for the global module.           */
/*                                                                           */
/*****************************************************************************/

void GlobalEnd()
{
}


/*****************************************************************************/
