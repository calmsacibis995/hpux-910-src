/*
static char rcsId[] =
  "@(#) $Header: intrinsics.c,v 66.13 91/01/01 15:19:55 rpt Exp $";
static char copyright[] =
  "Copyright (c) HP, 1989";
*/


/********************************* intrinsics.c ******************************/
/*                                                                           */
/* This module is responsible for executing all "Keysh_config" built in      */
/* commands.  It also reads and writes the .keyshrc file.                    */
/*                                                                           */
/* Typically, the translate module calls IntrinsicsExecute() whenever it     */
/* processes a node of a built-in command.  Then, after all nodes have been  */
/* processed, it calls IntrinsicsEnd() for any clean-up.  This means that    */
/* IntrinsicsExecute() can either do the work associated with a              */
/* particular node right when it is called, or it can set a static variable  */
/* which IntrinsicsEnd() will examine and, if set, do the work later.        */
/* This deferred processing can be used to avoid doing duplicate work        */
/* (e.g., rewriting the rc file after each status-line indicator is changed) */
/* within the IntrinsicsExecute() routine.                                   */
/*                                                                           */
/*****************************************************************************/


/******************************** softkey naming *****************************/
/*                                                                           */
/* SOFTKEY NAMES                                                             */
/* =============                                                             */
/*                                                                           */
/* Each softkey has two names.  One is the official name given it by the     */
/* softkey file writer and the other is a label given it by the user.        */
/*                                                                           */
/* Typically, the official name corresponds to the HP-UX command which       */
/* the softkey implements (e.g., grep).  This name cannot change since it    */
/* is coded into the softkeys file.                                          */
/*                                                                           */
/* The label, on the other hand, is a mnemonic aid chosen by the user        */
/* (e.g., Search_lines) when the softkey is added to the top-level           */
/* softkey menu.  If no label is specified, the default is to just reuse     */
/* official softkey name.                                                    */
/*                                                                           */
/* INVISIBLE SOFTKEY COMMANDS                                                */
/* ==========================                                                */
/*                                                                           */
/* Softkey naming does not apply to invisible softkey commands, since        */
/* invisible commands always use the official softkey names.                 */
/*                                                                           */
/*                                                                           */
/* VISIBLE SOFTKEY COMMANDS                                                  */
/* ========================                                                  */
/*                                                                           */
/* Softkey naming applies to visible softkey commands.  In particular,       */
/* the user must understand the naming mechanism when using, adding, and     */
/* deleting visible softkey commands.                                        */
/*                                                                           */
/* When using visible softkeys, the only reliable mechanism the user has     */
/* to link the softkey labels he sees (on the top-level softkey menu)        */
/* with the corresponding HP-UX commands is the on-line help.  Creative      */
/* users might look in the .keyshrc file to determine this linkage, but      */
/* not usually.  (Note that if I really wanted to, I could modify the        */
/* system configuration file to add the "rm" softkey and label it            */
/* "List_files" -- I probably wouldn't make too many friends in the          */
/* process...)                                                               */
/*                                                                           */
/* When adding visible softkeys, the user must first know the softkey's      */
/* official name -- in general, this will be one of the 50 or so HP-UX       */
/* commands listed in the keysh.1 man-page.  He can then add that softkey    */
/* to his top-level softkey menu, and, if desired, specify an alternate      */
/* softkey label at the same time.                                           */
/*                                                                           */
/* The intent is that the brick be the starting point whenever the user      */
/* wants to add a visible softkey.  The user first figures out which         */
/* HP-UX command he wants to add to his top-level softkey menu.  He then     */
/* adds it using its official name (e.g., the HP-UX command name from the    */
/* brick).  Then, if desired, he can specify an alternate softkey label      */
/* to actually be displayed in place of the official softkey name.           */
/*                                                                           */
/* In this way, users can add existing HP-UX functionality (e.g., du(1))     */
/* to their top-level softkey menu and either label it with the HP-UX        */
/* command name itself (the default) or specify an alternate (possibly       */
/* more readable) name (e.g., Disk_usage).                                   */
/*                                                                           */
/* To delete a visible softkey command, the user may specify either the      */
/* official softkey name or the label assigned to the softkey when it was    */
/* added.                                                                    */
/*                                                                           */
/* (Note that a different softkey label may be chosen each time a softkey    */
/* is added.  And that different users can choose different labels.)         */
/*                                                                           */
/* ROLE OF THE .KEYSHRC FILE                                                 */
/* =========================                                                 */
/*                                                                           */
/* When keysh first powers up, it knows only the Keysh_config softkey.       */
/* It then reads the .keyshrc file which has a list of "softkey add"         */
/* commands.  These commands specify both which HP-UX commands to add,       */
/* and which labels to display them with in the top-level menu.              */
/*                                                                           */
/* WARTS                                                                     */
/* =====                                                                     */
/*                                                                           */
/* The gross part here is that some (I think, 5) of the softkeys in the      */
/* softkey file don't correspond to real HP-UX commands.                     */
/*                                                                           */
/* What I could have done for these "warts" (which, looking back, might      */
/* have been clearer), would have been to *create* pseudo HP-UX commands     */
/* for each one (e.g., make the official softkey name be "print" instead     */
/* of "Print_files").  Then, in the .keyshrc file, I could have assigned     */
/* the labels I really wanted displayed like for the rest of the HP-UX       */
/* commands (e.g., "kc softkey add print with_label Print_files").  This     */
/* would have made these warts look more like the rest of the HP-UX          */
/* commands.  It also would have given them invisible equivalents (e.g.,     */
/* the user could type "print").                                             */
/*                                                                           */
/* What I did instead was to just make the softkey's official name the       */
/* same as what I wanted the label to be (e.g., "Print_files").  This can    */
/* be confusing.  And there can be no invisible equivalents (which is        */
/* actually good in the case of "Copy_files" and "Move_files", since         */
/* keysh can't really implement "cp" or "mv").                               */
/*                                                                           */
/* In the code, softkey names are represented by:                            */
/*                                                                           */
/*   sk->alias           - the official softkey name                         */
/*                         (from the softkey file)                           */
/*   sk->literal         - the literal string representing the softkey label */
/*                         (from the .keyshrc file "with_label" option)      */
/*   sk->label[2]        - the individual lines of the softkey label         */
/*                         (generated automatically from sk->literal)        */
/*                                                                           */
/* Examples:                                                                 */
/*                                                                           */
/*                       visible softkey   invisible softkey                 */
/*                       ---------------   -----------------                 */
/*   sk->alias           "ls"              "cut"                             */
/*   sk->literal         "List_files"      "cut" (not used)                  */
/*   sk->label[0]        "List"            n/a                               */
/*   sk->label[1]        "files"           n/a                               */
/*                                                                           */
/*****************************************************************************/


#include <pwd.h>
#include <stdlib.h>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/param.h>
#include <assert.h>
#include <unistd.h>
#include <string.h>

#include "keyshell.h"
#include "global.h"
#include "code.h"
#include "translate.h"
#include "config.h"
#include "edit.h"
#include "display.h"
#include "debug.h"

#include "message.h"
#include "kshhooks.h"
#include "chunk.h"
#include "string2.h"
#include "buffer.h"
#include "clist.h"

#define HP9000
#include "TSM/include/facetterm.h"


/********************************** statusLine *******************************/
/*                                                                           */
/* This structure defines a boolean for each indicator in the status-line.   */
/* The boolean is set to TRUE if the indicator is visible, FALSE otherwise.  */
/*                                                                           */
/*****************************************************************************/

static struct {
  int host;
  int user;
  int dir;
  int time;
  int date;
  int mail;
} statusLine;


/***************************** initial / INITIALS ****************************/
/*                                                                           */
/* This structure defines the initial value for keyshell global options      */
/* and status-line indicators.  It also defines the "codes" corresponding    */
/* to the nodes of the "Keysh_config" command which manipulate these         */
/* options.  Its fields are defined as follows:                              */
/*                                                                           */
/*   parameter  - a pointer to an int which holds the global option value    */
/*   value      - the default global option value (set on power-up)          */
/*   code       - the code of the "Keysh_config" node whose command path is  */
/*                written to the .keyshrc file when the user has changed     */
/*                the option from its default value                          */
/*   pcode      - the code of the "Keysh_config" node which should have      */
/*                an asterisk displayed in its lower-right corner if the     */
/*                option is enabled -- this is typically the parent node     */
/*                of the one referenced by "code".                           */
/*                                                                           */
/* INITIALS is the number of entries in the "initial" structure which need   */
/* to be initialized on power-up and checked when rewriting the .keyshrc     */
/* file.                                                                     */
/*                                                                           */
/*****************************************************************************/

static struct Initial {
  int *parameter;
  int value;
  int code;
  int pcode;
} initial[] = {
  &globalOptions.enableAccelerators, FALSE, CODEoptAcceleratorsOn,
    CODEoptAccelerators,
  &globalOptions.enableHelpKey, TRUE, CODEoptHelpOff, CODEoptHelp,
  &globalOptions.enableBackups, TRUE, CODEoptBackupsOff, CODEoptBackups,
  &globalOptions.enableInvisibles, TRUE, CODEoptInvisiblesOff,
    CODEoptInvisibles,
  &globalOptions.enablePrompts, TRUE, CODEoptPromptsOff, CODEoptPrompts,
  &globalOptions.enableTranslations, TRUE, CODEoptTranslationOff,
    CODEoptTranslation,
  &globalOptions.enableVisibles, TRUE, CODEoptVisiblesOff, CODEoptVisibles,

  &statusLine.host, TRUE, CODEstatusHostOff, CODEstatusHost,
  &statusLine.user, FALSE, CODEstatusUserOn, CODEstatusUser,
  &statusLine.dir, TRUE, CODEstatusCurrentDirOff, CODEstatusCurrentDir,
  &statusLine.mail, TRUE, CODEstatusMailOff, CODEstatusMail,
  &statusLine.date, FALSE, CODEstatusDateOn, CODEstatusDate,
  &statusLine.time, TRUE, CODEstatusTimeOff, CODEstatusTime,
};

#define INITIALS  (sizeof(initial)/sizeof(struct Initial))


/********************************* keyshell **********************************/
/*                                                                           */
/* This structure is used to store intermediate values resulting from        */
/* calls to the IntrinsicsExecute() routine.  Typically, this routine is     */
/* called for each "Keysh_config" command noded visited by the translate     */
/* module.  Each of these calls saves enough state so that the final         */
/* call to IntrinsicsEnd() can carry out the actual command specified        */
/* by the user.                                                              */
/*                                                                           */
/* For example, when the user executes the command "kc softkey add paste",   */
/* where "paste" is an HP-UX command, the translate module calls             */
/* IntrinsicsExecute() 4 times -- once for each word on the command-line.    */
/* The first two calls are ignored, the third call sets the keyshell.add     */
/* boolean to TRUE, and the last call sets the keyshell.softkey pointer      */
/* to "paste".  Finally, the IntrinsicsEnd() routine is called which         */
/* examines this structure and actually adds the "paste" softkey to the      */
/* top-level softkey menu.                                                   */
/*                                                                           */
/* In particular, these fields are:                                          */
/*                                                                           */
/*   add          - set to TRUE if this is a "kc add" command                */
/*   delete       - set to TRUE if this is a "kc delete" command             */
/*   place        - set to TRUE if this is a "kc move" command               */
/*   first        - set to TRUE for place/move "to_first_softkey"            */
/*   invisibles   - set to TRUE for "kc add invisibles"                      */
/*   backups      - set to TRUE for "kc add backups"                         */
/*   softKey      - set to the name of the softkey to add/delete/move        */
/*   withLabel    - set to the name of the label to add the softkey with     */
/*   fromUser     - set to the name of the user whose .softkeys file should  */
/*                  be read                                                  */
/*   fromFile     - set to the name of the softkey file which should be read */
/*   before       - set to the name of the softkey to move "softKey" before  */
/*                                                                           */
/*****************************************************************************/

static struct {
  int add;
  int delete;
  int place;
  int first;
  int invisibles;
  int backups;
  char *softKey;
  char *withLabel;
  char *fromUser;
  char *fromFile;
  char *before;
} keyshell;


/********************************** tsmBuffer ********************************/
/*                                                                           */
/* This buffer is used to issue the "get window number" ioctl to tsm.        */
/*                                                                           */
/*****************************************************************************/

static char tsmBuffer[FIOC_BUFFER_SIZE];


/********************************** mtime ************************************/
/*                                                                           */
/* This variable holds the last known modify-time of the .keyshrc file.      */
/*                                                                           */
/*****************************************************************************/

static time_t mtime;


/********************************** iList ************************************/
/*                                                                           */
/* When rewriting the .keyshrc file, this list holds the names of all        */
/* files which we have already added invisible softkeys from (i.e., written  */
/* "kc softkey add invisibles from_file <file>" to the .keyshrc for).  We    */
/* keep this info so that we never write a duplicate line to the .keyshrc    */
/* file.                                                                     */
/*                                                                           */
/*****************************************************************************/

static CList *iList;


/*********************************** rcAction ********************************/
/*                                                                           */
/* This variable identifies the .keyshrc action which should be              */
/* performed when IntrinsicsEnd() is next called.  It can be:                */
/*                                                                           */
/*   NONE       - perform no action                                          */
/*   RESTART    - clear the current configuration and then reread .keyshrc   */
/*   RESTORE    - rewrite .keyshrc's original contents and then RESTART      */
/*   FLUSH      - just rewrite .keyshrc with the current configuration       */
/*   DEFAULT    - remove .keyshrc and then RESTART                           */
/*                                                                           */
/* Note that in the "kc" command, RESTORE has been renamed to "undo" and     */
/* FLUSH has been renamed to "write".                                        */
/*                                                                           */
/*****************************************************************************/

static int rcAction;

#define NONE     0
#define RESTART  1
#define RESTORE  2
#define FLUSH    3
#define DEFAULT  4


/*****************************************************************************/
/*****************************************************************************/
/***************************** status line ***********************************/
/*****************************************************************************/
/*****************************************************************************/


/****************************** eol / line ***********************************/
/*                                                                           */
/* "line" is the current status line built by the StatusLine() routine.      */
/* "eol" is the index of the end of "line" (e.g., index of '\0' char).       */
/*                                                                           */
/*****************************************************************************/

static int eol;
static char line[DISPLAYlength];


/******************************* PREFIX* / POSTFIX ***************************/
/*                                                                           */
/* These constants define the character(s) used to separate individual       */
/* status-line indicators on the status-line.  They should probably come     */
/* from the message catalog...                                               */
/*                                                                           */
/*****************************************************************************/

#define PREFIXC  '='
#define PREFIX   "=== "
#define POSTFIX  " "


/**************************** StatusAppend() *********************************/
/*                                                                           */
/* This routine appends the specified status-line indicator to the current   */
/* status-line in "line" along with the appropriate prefix and postfix       */
/* strings.  It will not overflow the "line" buffer.                         */
/*                                                                           */
/*****************************************************************************/

static void StatusAppend(s)
char *s;
{
  int n;

  n = sizeof(PREFIX)-1 + strlen(s) + sizeof(POSTFIX)-1;
  if (eol+n < sizeof(line)) {
    strcpy(line+eol, PREFIX);
    strcat(line+eol, s);
    strcat(line+eol, POSTFIX);
    eol += n;
  }
}


/****************************** StatusLine() *********************************/
/*                                                                           */
/* This routine creates an up-to-date status-line and returns a pointer      */
/* to it.  This is typically called by the display module as its             */
/* (*statusFeeder)() function.                                               */
/*                                                                           */
/*****************************************************************************/

static char *StatusLine()
{
  char *s;
  char *l;
  struct tm *t;
  char host[MAXHOSTNAMELEN+1];
  char buffer[DISPLAYlength];
  extern char *logname();
  struct stat thisMail;
  static struct stat baseMail;
  time_t thisTime;
  static time_t baseTime;

  eol = 0;
  line[eol] = '\0';

  /* if the user set the $KEYSH variable then include it first on the */
  /* status-line */
  if (kshStatus[0]) {
    StatusAppend(kshStatus);
  }


  /* if tsm is running, then include the tsm window number next on the */
  /* status-line */
  if (keyshellTsm) {
    strcpy(buffer, MESSAGEtsmWindow);
    strcat(buffer, tsmBuffer);
    StatusAppend(buffer);
  }

  /* if the user selected that both user name and hostname should be */
  /* displayed, then display them as "user@host" -- otherwise if he */
  /* selected either of them to be displayed individually, then do so. */
  if (statusLine.user) {
    buffer[0] = '\0';
    if (l = logname()) {
      strncpy(buffer, l, sizeof(buffer)-1);
      buffer[sizeof(buffer)-1] = '\0';
    }
    host[0] = '\0';
    if (statusLine.host && gethostname(host, sizeof(host)) >= 0) {
      host[MAXHOSTNAMELEN] = '\0';
      strncat(buffer, "@", sizeof(buffer)-strlen(buffer)-1);
      strncat(buffer, host, sizeof(buffer)-strlen(buffer)-1);
    }
    StatusAppend(buffer);
  } else if (statusLine.host && gethostname(host, sizeof(host)) >= 0) {
    host[MAXHOSTNAMELEN] = '\0';
    StatusAppend(host);
  }

  /* if the user selected the current directory to be displayed, then */
  /* displayit nextin the status-line. */
  if (statusLine.dir && (s = KshGetEnv("PWD"))) {
    StatusAppend(s);
  }

  /* save the current time */
  thisTime = time(NULL);

  /* this is really complex.  we are trying to figure out if we want to */
  /* display "No mail", "You have mail", or "You have new mail".  We */
  /* use the same algorithm as ksh using the size, mtime, and atime. */
  /* This allows us to detect and display "You have new mail" immediately. */
  if (statusLine.mail && (s = KshGetEnv("MAIL"))) {
    if (stat(s, &thisMail)) {
      thisMail.st_size = 0;
    }
    if (thisMail.st_size > 48) {
      if (thisMail.st_mtime >= baseTime &&
          thisMail.st_atime <= thisMail.st_mtime &&
          (thisMail.st_size > baseMail.st_size ||
           thisMail.st_ino != baseMail.st_ino ||
           thisMail.st_dev != baseMail.st_dev)) {
        StatusAppend(MESSAGEnewMail);
      } else {
        StatusAppend(MESSAGEoldMail);
        baseTime = thisTime;
        baseMail = thisMail;
      }
    } else {
      StatusAppend(MESSAGEnoMail);
    }
  }

  /* then, if the user asked for both the time and date to be displayed, */
  /* concatenate them together and display them.  otherwise, if he asked */
  /* for either one individually, display it. */
  if (statusLine.time || statusLine.date) {
    t = localtime(&thisTime);
    if (statusLine.time && statusLine.date) {
      strftime(buffer, sizeof(buffer), "%x %X", t);
    } else if (statusLine.date) {
      strftime(buffer, sizeof(buffer), "%x", t);
    } else {
      strftime(buffer, sizeof(buffer), "%X", t);
    }
    StatusAppend(buffer);
  }

  /* then fill up the rest of the status-line with "===..." */
  strcpy(line+eol, StringRepeat(PREFIXC, (int)sizeof(line)-eol-1));

  return line;
}

/*****************************************************************************/
/*****************************************************************************/
/*****************************************************************************/
/*****************************************************************************/
/*****************************************************************************/


/******************************* UpdateKeyshell() ****************************/
/*                                                                           */
/* This routine just cleans up the top-level softkey menu any time a new     */
/* softkey is added or deleted.  It makes sure that the "Keysh_config"       */
/* softkey is the last softkey of the last etcetera bank and then assigns    */
/* top-level keyboard accelerator chars (which can change whenever a         */
/* softkey is added/deleted) and sets the top-level etcetera count.          */
/*                                                                           */
/*****************************************************************************/

static void UpdateKeyshell()
{
  int n;
  int count;
  GlobalSoftKey *sk;
  GlobalSoftKey *nsk;
  static GlobalSoftKey empty[DISPLAYsoftKeyCount];

  if (globalNonInteractive) {
    return;
  }

  /* remove any empty softkeys in the top-level menu which were padding */
  /* the "keysh_config softkey */
  count = 0;
  for (sk = HierHeadChild(globalSoftKeys); sk; sk = nsk) {
    nsk = HierNextSibling(sk);
    if (sk->attrib.empty && ! sk->literal) {
      (void)HierRemove(sk);
    } else {
      count++;
      if (sk->attrib.keyshell) {
        break;
      }
    }
  }

  /* then add enough back so that the "Keysh_config" softkey is in */
  /* the last softkeyif the last bank of etceteras. */
  if (count <= KEYBOARDsoftKeyCount) {
    n = KEYBOARDsoftKeyCount - count;
  } else {
    n = KEYBOARDsoftKeyCount-1 - (((count-1) % (KEYBOARDsoftKeyCount-1)) + 1);
  }
  while (n--) {
    empty[n].attrib.empty = TRUE;
    HierAddBeforeSibling(globalKeyshell, &empty[n], FALSE);
  }

  /* then assign top-level accelerator chars and the top-level etcetera */
  /* count */
  ConfigAssignAcceleratorsEtc(globalSoftKeys);
}


/*****************************************************************************/
/*****************************************************************************/
/***************************** intrinisc command strings *********************/
/*****************************************************************************/
/*****************************************************************************/


/******************************* SearchCode() ********************************/
/*                                                                           */
/* This routine returns a pointer to the first softkey node in the           */
/* specified softkey hierarchy with the specified intrinsics command-code.   */
/* It returns NULL if no such softkey node can be found.                     */
/*                                                                           */
/*****************************************************************************/

static GlobalSoftKey *SearchCode(root, code)
GlobalSoftKey *root;
int code;
{
  GlobalSoftKey *sk;
  GlobalSoftKey *sc;

  if (root->code == code) {
    return root;
  }
  for (sk = HierHeadChild(root); sk; sk = HierNextSibling(sk)) {
    if (sc = SearchCode(sk, code)) {
      return sc;
    }
  }
  return NULL;
}


/******************************* CommandPath() *******************************/
/*                                                                           */
/* The purpose of this routine is to extract command substrings from the     */
/* Keysh_config softkey stored in the hierarchy at globalKeyshell.  We do    */
/* this so that we can write Keysh_config commands to the .keyshrc file      */
/* without ever having to hard code within the keyshell code.                */
/*                                                                           */
/* Basically, when passed the "codes" associated with two nodes in the       */
/* globalKeyshell hierarchy, this routine will return the command string     */
/* which the user would have to type to get from "rootCode" (not inclusive)  */
/* to "leafCode" (inclusive).                                                */
/*                                                                           */
/* If "rootCode" is -1, the entire Keysh_config command string leading       */
/* to "leafCode" will be returned, including the Keysh_config name itself.   */
/*                                                                           */
/*****************************************************************************/

static char *CommandPath(rootCode, leafCode)
int rootCode;
int leafCode;
{
  int i;
  int j;
  int levels;
  GlobalSoftKey *root;
  GlobalSoftKey *leaf;
  GlobalSoftKey *sk;
  static char command[EDITlength];

  /* Search the softkey hierarchy for a node with code "rootCode" -- this */
  /* is where we will start searching for a node with code "leafCode" from. */
  /* Note that if rootCode is -1, we will prepend the "kc" command */
  /* name itself to the command string we are building (and then search */
  /* the entire hierarchy for a node with code "leafCode"). */
  root = globalKeyshell;
  if (rootCode >= 0) {
    root = SearchCode(root, rootCode);
    command[0] = '\0';
  } else {
    strcpy(command, globalKeyshell->alias);
    strcat(command, " ");
  }

  if (root) {

    /* then search for a node with code "leafCode" within the pseudo-root */
    /* identified above. */
    leaf = SearchCode(root, leafCode);
    if (leaf) {

      /* figure out how many levels "leaf" is below "root" in the node */
      /* hierarchy. */
      levels = 0;
      for (sk = leaf; sk != root; sk = HierParent(sk)) {
        levels++;
      }

      /* then append the node names between "root" (not inclusive) and */
      /* "leaf" (inclusive) to the command string we are building.  then */
      /* return the complete string. */
      for (i = levels; i > 0; i--) {
        sk = leaf;
        for (j = 0; j < i-1; j++) {
          sk = HierParent(sk);
        }
        strcat(command, sk->literal);
        strcat(command, " ");
      }
      return command;
    }
  }

  return "";
}

/*****************************************************************************/
/*****************************************************************************/
/*****************************************************************************/
/*****************************************************************************/
/*****************************************************************************/


/**************************** LabelSoftKey() *********************************/
/*                                                                           */
/* This routine just labels the specified softkey with the user specified    */
/* name.                                                                     */
/*                                                                           */
/*****************************************************************************/

static void LabelSoftKey(sk, literal)
GlobalSoftKey *sk;
char *literal;
{
  sk->literal = ChunkString(sk->chunk, literal);
  (void)StringCapitalize(sk->literal);
  ConfigDefaultLabel(sk);
}


/***************************** AddBackupSoftKeys() ***************************/
/*                                                                           */
/* This routine adds all of the backup softkeys from the specified file.     */
/*                                                                           */
/*****************************************************************************/

static void AddBackupSoftKeys(file)
char *file;
{
  GlobalSoftKey *bu;

  /* give any currently loaded backup softkeys back to the config module */
  while (bu = HierTailChild(globalBackupSoftKeys)) {
    ConfigPutSoftKeyBack((GlobalSoftKey *)HierRemove(bu));
  }

  /* then load the new ones */
  while (bu = ConfigGetOtherSoftKey(file, CONFIGbackup)) {
    HierAddTailChild(globalBackupSoftKeys, bu, FALSE);
  }
}


/****************************** UpdateSoftKeys() *****************************/
/*                                                                           */
/* This routine adds any remaining softkeys from the specified file from     */
/* the config module to the global softkey hierarchy.  If a softkey          */
/* already existed in the global softkey hierarchy, then we assume that the  */
/* config module had a newer version, so we update the one we had.           */
/* Otherwise, if we got a softkey which we haven't seen before, just add     */
/* it as an invisible softkey (i.e., after the Keysh_config softkey).        */
/*                                                                           */
/*****************************************************************************/

static void UpdateSoftKeys(file)
char *file;
{
  GlobalSoftKey *sk;
  GlobalSoftKey *osk;

  if (file) {

    /* while the config module still has new softkeys for us to read... */
    while (sk = ConfigGetOtherSoftKey(file, CONFIGinvisible)) {
      osk = GlobalFindSoftKey(globalSoftKeys, sk->literal);

      /* if we alread loaded this softkey, replace the old one with the */
      /* new one. */
      if (osk && strcmp(sk->file, osk->file) == 0) {
        LabelSoftKey(sk, osk->literal);
        HierAddAfterSibling(osk, sk, TRUE);
        ConfigThrowSoftKeyAway((GlobalSoftKey *)HierRemove(osk));

      /* otherwise, just load this as an invisible softkey command */
      } else {
        LabelSoftKey(sk, sk->alias);
        HierAddAfterSibling(globalKeyshell, sk, TRUE);
      }
    }
  }
}


/*****************************************************************************/
/*****************************************************************************/
/*****************************************************************************/
/*****************************************************************************/
/*****************************************************************************/


/****************************** DeleteSoftKey() ******************************/
/*                                                                           */
/* This routine deletes the softkey keyshell.softKey from the top-level      */
/* softkey menu.  It returns the name of the file which the softkey came     */
/* from.  Note that if keyshell.softKey doesn't exist and keyshell.withLabel */
/* isn't NULL, that softkey will be deleted instead.                         */
/*                                                                           */
/*****************************************************************************/

static char *DeleteSoftKey()
{
  GlobalSoftKey *sk;
  static char file[MAXPATHLEN];

  /* find the softkey to delete */
  sk = GlobalFindSoftKey(globalSoftKeys, keyshell.softKey);
  if (! sk && keyshell.withLabel) {
    sk = GlobalFindSoftKey(globalSoftKeys, keyshell.withLabel);
  }

  /* don't allow the user to delete the Keysh_config softkey */
  if (! sk || sk->attrib.keyshell) {
    return NULL;
  }

  /* save the name of the file which the softkey came from */
  strcpy(file, sk->file);

  /* then give the softkey back to the config module (to be saved or */
  /* deallocated, as appropriate). */
  ConfigPutSoftKeyBack((GlobalSoftKey *)HierRemove(sk));

  UpdateKeyshell();

  /* return the name of the file we should add invisible softkeys from */
  return file;
}


/******************************** PlaceSoftKey() *****************************/
/*                                                                           */
/* This routine places the softkey keyshell.softkey either first (if         */
/* keyshell.first is set), before the softkey keyshell.before (if            */
/* keyshell.before is non-NULL), or last (otherwise) in the top-level        */
/* softkey menu.                                                             */
/*                                                                           */
/*****************************************************************************/

static void PlaceSoftKey()
{
  GlobalSoftKey *sk;
  GlobalSoftKey *exist;

  /* find the softkey to move */
  sk = GlobalFindSoftKey(globalSoftKeys, keyshell.softKey);
  if (! sk) {
    DisplayPrintf(MESSAGEcannotFindSoftKey, keyshell.softKey);
    DisplayFlush();
    return;
  }

  /* then find the softkey to put it in front of */
  exist = NULL;
  if (keyshell.first) {
    exist = HierHeadChild(globalSoftKeys);
  } else if (keyshell.before) {
    exist = GlobalFindSoftKey(globalSoftKeys, keyshell.before);
    if (! exist) {
      DisplayPrintf(MESSAGEcannotFindSoftKey, keyshell.before);
      DisplayFlush();
    }
  }
  if (! exist) {
    exist = globalKeyshell;
  }

  /* and then move it */
  HierAddBeforeSibling(exist, HierRemove(sk), TRUE);

  UpdateKeyshell();
}


/*************************** AddSoftKey() ************************************/
/*                                                                           */
/* This routine adds visible, invisible, and backup softkeys to keysh.       */
/*                                                                           */
/*****************************************************************************/

static char *AddSoftKey()
{
  char *logDir;
  GlobalSoftKey *sk;
  static char file[MAXPATHLEN];

  /* figure out which file we are adding softkeys from.  The default for */
  /* adding backups is the user's own ".softkeys" file. */
  if (keyshell.backups && ! keyshell.fromFile && ! keyshell.fromUser) {
    strcpy(file, "~");
    strcat(file, MESSAGEuserSoftKeysFile);

  /* if we are adding softkeys from another user's ".softkeys" file, */
  /* then look up the real path to the file. */
  } else if (keyshell.fromUser) {
    logDir = KshLogDir(keyshell.fromUser);
    if (! logDir) {
      DisplayPrintf(MESSAGEunknownUser, keyshell.fromUser);
      DisplayFlush();
      return NULL;
    }
    strcpy(file, logDir);
    strcat(file, MESSAGEuserSoftKeysFile);

  /* otherwise check if the the user specified a realpath name to the */
  /* softkey file */
  } else if (keyshell.fromFile) {
    strcpy(file, keyshell.fromFile);

  /* otherwise use the system default softkey file */
  } else {
    strcpy(file, MESSAGEsoftKeysFile);
  }

  /* if the user only wants to add backup softkeys, then get them and */
  /* return. */
  if (keyshell.backups) {
    AddBackupSoftKeys(file);
    return NULL;
  }

  /* otherwise if the user is adding a visible softkey... */
  if (! keyshell.invisibles) {

    /* then delete any other visible softkeys which might have the same */
    /* name or label */
    while (DeleteSoftKey()) {
      /* NULL */
    }

    /* then get the softkey to add from the config module */
    sk = ConfigGetSoftKey(file, keyshell.softKey);
    if (! sk) {
      DisplayPrintf(MESSAGEcannotFindSoftKeyInFile, keyshell.softKey, file);
      DisplayFlush();
      return NULL;
    }

    /* if the user specified an alternate label, set it; otherwise, just */
    /* usethe softkey name itself as the label. */
    if (keyshell.withLabel) {
      LabelSoftKey(sk, keyshell.withLabel);
    } else {
      LabelSoftKey(sk, sk->alias);
    }

    /* then add the softkey after the last one in the global softkey */
    /* hierarchy -- then, if the user didn't really want it there, move */
    /* it to where he really wanted it. */
    HierAddBeforeSibling(globalKeyshell, sk, TRUE);
    if (keyshell.first || keyshell.before) {
      PlaceSoftKey();
    }

    UpdateKeyshell();
  }

  /* return the name of the file we should add invisible softkeys from */
  return file;
}

/*****************************************************************************/
/*****************************************************************************/
/****************************** rc file i/o **********************************/
/*****************************************************************************/
/*****************************************************************************/


/**************************** input / firstInput / output ********************/
/*                                                                           */
/* "input" is a pointer to a buffer holding the entire contents of the rc    */
/* file being read.  "output" is the current position within that buffer     */
/* from which the next character will be read.  "firstInput" is a pointer    */
/* to the saved "input" buffer from when we first powered-up -- it is used   */
/* if the user enters the "kc undo" command.                                 */
/*                                                                           */
/*****************************************************************************/

static char *input;
static char *firstInput;
static char *output;


/******************************* OpenRCFile() ********************************/
/*                                                                           */
/* This routine opens the rc file for stating (i.e., checking the mtime),    */
/* reading, writing, or deleting (i.e., unlinking it).                       */
/*                                                                           */
/*****************************************************************************/

#define STATING   0
#define READING   1
#define WRITING   2
#define DELETING  3

static int OpenRCFile(how)
int how;
{
  int ok;
  int fd;
  char *s;
  int mode;
  char file[MAXPATHLEN];
  static struct stat st;

  /* get the path name of the user's own .keyshrc file */
  if (s = KshGetEnv(KEYSHELLenviron)) {
    strcpy(file, s);
  } else {
    file[0] = '\0';
    if (s = KshGetEnv("HOME")) {
      strcpy(file, s);
    }
    strcat(file, MESSAGEuserRCFile);
  }

  /* then stat it */
  ok = stat(file, &st);

  /* if we should delete the file, then make sure it is a regular file */
  /* first.  then remove it and return. */
  if (how == DELETING) {
    if (ok == 0) {
      if (! S_ISREG(st.st_mode) || unlink(file) < 0) {
        DisplayPrintf(MESSAGEcannotUnlink, file);
        DisplayFlush();
        return -1;
      }
    }
    return 0;
  }

  /* if we should just stat the file, then save its modify time and return */
  if (how == STATING) {
    if (ok == 0) {
      mtime = st.st_mtime;
    }
    return -1;

  /* otherwise prepair to read or write the file */
  } else if (how == READING) {
    mode = O_RDONLY;
  } else {
    mode = O_WRONLY | O_CREAT | O_TRUNC;
  }

  /* open the file with the appropriate mode */
  fd = open(file, mode, 0666);

  /* if the open failed and we're just trying to read our configuration, */
  /* then try opening the system default file, instead. */
  if (how == READING) {
    if (fd >= 0) {
      mtime = st.st_mtime;
    } else {
      fd = open(MESSAGEdefaultRCFile, mode);
    }
  }

  /* if we couldn't successfully open the rc file, print an error and */
  /* return */
  if (fd < 0) {
    if (how == READING) {
      DisplayPrintf(MESSAGEcannotReadConfig, file, MESSAGEdefaultRCFile);
    } else {
      DisplayPrintf(MESSAGEcannotWriteConfig, file);
    }
    DisplayFlush();
    return -1;
  }

  /* otherwise, we successfully opened the file -- lock it for exclusive */
  /* access */
  lockf(fd, F_LOCK, 0);

  /* if we're about to write the file and the modify time had changed */
  /* since the last time we wrote it, warn the user that other changes */
  /* written to the file may be lost... */
  if (how == WRITING) {
    if (ok == 0 && st.st_mtime != mtime) {
      DisplayPrintf(MESSAGEconflictWritingConfig, file);
      DisplayFlush();
    }
  }

  /* return the open file descriptor */
  return fd;
}


/*********************************** ReadRCChar() ****************************/
/*                                                                           */
/* This routine reads the next character from the rc file buffer pointed     */
/* to by "input" and "output".  It is used as an input feeder to the         */
/* keyboard module.                                                          */
/*                                                                           */
/*****************************************************************************/

static int ReadRCChar()
{
  int c;

  /* read the next character from the buffer */
  c = *output;

  /* if we haven't reached the end of the buffer, just advance our pointer */
  /* and return the char. */
  if (c) {
    output++;

  /* otherwise, we need to switch the keyboard module back to reading */
  /* from the terminal since we've finished reading the rc file. */
  /* Note that if the buffer we just finished reading from is the */
  /* first one we ever read, then we don't want to free it because the */
  /* user may want to revert to it later using the "kc undo" command. */
  } else {
    if (input != firstInput) {
      free((void *)input);
    }
    KeyboardInputFeeder(NULL);
    globalNonInteractive = FALSE;
    UpdateKeyshell();
    c = '\n';
  }

  if (c == '\n') {
    return keyboardExecute;
  }

  return c;
}


/********************************* ReadRCFile() ******************************/
/*                                                                           */
/* This routine opens the rc file, reads it into a buffer, and then          */
/* sets up the keyboard input feeder to read it a char at a time.  No        */
/* chars will be read from the real terminal until the entrire rc file       */
/* has been read and processed.                                              */
/*                                                                           */
/*****************************************************************************/

static void ReadRCFile()
{
  int fd;
  int len;
  struct stat st;

  fd = OpenRCFile(READING);
  if (fd < 0) {
    globalNonInteractive = FALSE;
    return;
  }

  (void)fstat(fd, &st);
  input = (char *)malloc((size_t)st.st_size+1);
  if (! firstInput) {
    firstInput = input;
  }

  len = read(fd, input, (unsigned)st.st_size);
  if (len < 0) {
    len = 0;
  }
  input[len] = '\0';

  close(fd);

  output = input;
  globalInteractive = FALSE;
  globalNonInteractive = TRUE;

  KeyboardInputFeeder(ReadRCChar);
}


/******************************* ReReadRCFile() ******************************/
/*                                                                           */
/* This routine just resets keysh to its power-up state and then rereads     */
/* the configuration from the rc file.                                       */
/*                                                                           */
/*****************************************************************************/

static void ReReadRCFile()
{
  int n;
  GlobalSoftKey *sk;
  GlobalSoftKey *nsk;

  /* set all globaloptions to power-up values */
  for (n = 0; n < INITIALS; n++) {
    *initial[n].parameter = initial[n].value;
  }


  /* then give all top-level softkeys back to the config module */
  for (sk = HierHeadChild(globalSoftKeys); sk; sk = nsk) {
    nsk = HierNextSibling(sk);
    if (! sk->attrib.keyshell) {
      (void)HierRemove(sk);
      if (! sk->attrib.empty || sk->literal) {
        ConfigPutSoftKeyBack(sk);
      }
    }
  }

  /* ditto for any backup softkeys */
  while (sk = HierTailChild(globalBackupSoftKeys)) {
    ConfigPutSoftKeyBack((GlobalSoftKey *)HierRemove(sk));
  }

  /* then, assuming this is an interactive keysh, reread the rc file to */
  /* reset any options and load any softkeys.  Note that for a */
  /* non-interactive keysh (e.g., executing a script), we never need */
  /* any of the keysh stuff, so we can speed power-up by skipping all */
  /* of the rc file initializations. */
  if (KshInteractive() && ! KshGetEnv(KEYSHELLksh)) {
    ReadRCFile();
  } else {
    globalNonInteractive = FALSE;
  }

  UpdateKeyshell();
}


/****************************** RestoreRCFile() ******************************/
/*                                                                           */
/* This routine just rewrites the rc file with the contents we first         */
/* read from it on power-up -- or the contents read the last time this       */
/* routine was called (i.e., the last "kc undo").                            */
/*                                                                           */
/*****************************************************************************/

static void RestoreRCFile()
{
  int fd;
  int oldFd;
  char *lastInput;

  /* read the current contents of the rc file -- this is what we will */
  /* undo back to if the user calls us again */
  lastInput = firstInput;
  firstInput = NULL;
  ReadRCFile();

  fd = OpenRCFile(WRITING);
  if (fd < 0) {
    return;
  }

  /* then rewrite the saved rc file contents */
  oldFd = BufferWriteFd(fd);
  BufferWriteString(lastInput);
  BufferWriteFlush();
  (void)BufferWriteFd(oldFd);

  close(fd);

  if (lastInput) {
    free(lastInput);
  }

  ReReadRCFile();
}


/********************************** InvisibleList() **************************/
/*                                                                           */
/* This routine returns TRUE if the specified file is already in the iList.  */
/* Otherwise, it adds the file to the iList and returns FALSE.  In this      */
/* way, TRUE is only returned once for each file passed to this routine.     */
/*                                                                           */
/*****************************************************************************/

static int InvisibleList(file)
char *file;
{
  int i;

  for (i = 0; i < CListCount(iList); i++) {
    if (strcmp((char *)CListLook(iList, i), file) == 0) {
      return TRUE;
    }
  }
  CListAdd(iList, (int)file);
  return FALSE;
}


/******************************* WriteRCFile() *******************************/
/*                                                                           */
/* This routine writes the rc file with keysh's current configuration.       */
/*                                                                           */
/*****************************************************************************/

static void WriteRCFile()
{
  int n;
  int fd;
  int oldFd;
  GlobalSoftKey *sk;

  /*open the rc file for writing */
  fd = OpenRCFile(WRITING);
  if (fd < 0) {
    return;
  }

  oldFd = BufferWriteFd(fd);

  /* write a (currently empty string) preamble to the rc file */
  BufferWriteString(MESSAGEpreamble);

  /* then, for each global option which isn't in its default state, */
  /* write the command which would need to be issued to set the option. */
  for (n = 0; n < INITIALS; n++) {
    if (*initial[n].parameter != initial[n].value) {
      BufferWriteString(CommandPath(-1, initial[n].code));
      BufferWriteString("\n");
    }
  }

  /* then empty the list of files which we've read invisible softkey */
  /* commands from. */
  CListEmpty(iList);

  /* then, for each visible softkey, write the command which would */
  /* need to be executed to add the softkey as-is.  Note that if the */
  /* softkey doesn't have the default label or if it doesn't come */
  /* from the default softkey file, then we need to append "with_label" */
  /* and/or "from_file" options to the command, respectively. */
  for (sk = HierHeadChild(globalSoftKeys); ! sk->attrib.keyshell;
       sk = HierNextSibling(sk)) {
    if (! sk->attrib.empty || sk->literal) {
      BufferWriteString(CommandPath(-1, CODEsoftKeyAdd));
      BufferWriteString(sk->alias);
      if (StringCompareLower(sk->alias, sk->literal)) {
        BufferWriteString(" ");
        BufferWriteString(CommandPath(CODEsoftKeyASoftKey, CODEsoftKeyWith));
        BufferWriteString(sk->literal);
      }
      BufferWriteString(" ");
      if (strcmp(sk->file, MESSAGEsoftKeysFile)) {
        BufferWriteString(CommandPath(CODEsoftKeyASoftKey, CODEsoftKeyFrom));
        BufferWriteString(sk->file);
      }
      BufferWriteString("\n");
      (void)InvisibleList(sk->file);
    }
  }

  /* then, for each invisible softkey whose file we haven't already */
  /* added, write the command to add that file. */
  for (sk = HierTailChild(globalSoftKeys); ! sk->attrib.keyshell;
       sk = HierPrevSibling(sk)) {
    if (! InvisibleList(sk->file)) {
      BufferWriteString(CommandPath(-1, CODEsoftKeyInvisibles));
      if (strcmp(sk->file, MESSAGEsoftKeysFile)) {
        BufferWriteString(CommandPath(CODEsoftKeyInvisibles,
                                      CODEsoftKeyIFrom));
        BufferWriteString(sk->file);
      }
      BufferWriteString("\n");
    }
  }

  /* then write the command to read the backup softkeys we have loaded */
  if (sk = HierHeadChild(globalBackupSoftKeys)) {
    BufferWriteString(CommandPath(-1, CODEsoftKeyBFrom));
    BufferWriteString(sk->file);
    BufferWriteString("\n");
  }

  BufferWriteFlush();
  (void)BufferWriteFd(oldFd);

  close(fd);
  (void)OpenRCFile(STATING);
}


/*****************************************************************************/
/*****************************************************************************/
/****************************** external routines ****************************/
/*****************************************************************************/
/*****************************************************************************/


/******************************* IntrinsicsInitialize() **********************/
/*                                                                           */
/* This routine performs local initialization for the intrinsics module.     */
/*                                                                           */
/*****************************************************************************/

void IntrinsicsInitialize()
{
  static GlobalSoftKey softkeys;
  static GlobalSoftKey dummy;

  assert(sizeof(char *) == sizeof(int));

  /* if tsm is running, save the tsm window number */
  if (keyshellTsm) {
    strcpy(tsmBuffer, "window_number");
    (void)ioctl(0, FIOC_GET_INFORMATION, tsmBuffer);
  }

  iList = CListCreate();

  globalSoftKeys = &softkeys;
  HierCreateRoot(globalSoftKeys);

  DisplayStatusFeeder(StatusLine);

  ConfigInitialize();

  /* thenload the "Keysh_config" softkey */
  globalKeyshell = ConfigGetSoftKey(MESSAGEintrinsicsFile,
                                    MESSAGEkeyshellIntrinsic);

  /* if we couldn't load the "Keysh_config" softkey then try to limp */
  /* along without it.  otherwise, add it to the top-level softkey */
  /* menu */
  if (! globalKeyshell) {
    globalKeyshell = &dummy;
    HierCreateRoot(globalKeyshell);
    globalKeyshell->attrib.keyshell = TRUE;
    globalKeyshell->literal = MESSAGEkeyshellIntrinsic;
    globalKeyshell->alias = MESSAGEkeyshellIntrinsicAlias;
    HierAddTailChild(globalSoftKeys, globalKeyshell, FALSE);
    KshPutEnv(StringConcat(KEYSHELLksh, "="), FALSE, FALSE);
  } else {
    globalKeyshell->attrib.keyshell = TRUE;
    globalKeyshell->alias = MESSAGEkeyshellIntrinsicAlias;
    HierAddTailChild(globalSoftKeys, globalKeyshell, TRUE);
  }

  /* Then perform auto-configuration from the .keyshrc file */
  ReReadRCFile();
}


/******************************* IntrinsicsExecute() *************************/
/*                                                                           */
/* This routine performs the processing associated with each built-in        */
/* Keysh_config command node.  It is called by the translate module for      */
/* each node as it is processed.                                             */
/*                                                                           */
/* Some of the cases in this routine affect global options directly and      */
/* then set a flag indicating that the keysh configuration has changed       */
/* (and needs to be rewritten when IntrinsicsEnd() is finally called).       */
/* Others just set static variables which IntrinsicsEnd() will examine       */
/* to figure out what work still needs to be done.                           */
/*                                                                           */
/*****************************************************************************/

void IntrinsicsExecute(word, code)
char *word;
int code;
{
  /* these intrinsic command codes don't force us to rewrite the */
  /* .keyshrc file */
  switch (code) {
    case CODEopt                 :
    case CODEstatus              :
    case CODEnop                 : return;

    case CODEsoftKeyWith         :
    case CODEsoftKeyIFrom        :
    case CODEsoftKeyBFrom        :
    case CODEsoftKeyFrom         : return;

    case CODEoptTranslation      :
    case CODEoptInvisibles       :
    case CODEoptBackups          :
    case CODEoptAccelerators     :
    case CODEoptHelp             :
    case CODEoptPrompts          :
    case CODEoptVisibles         : return;

    case CODEstatusCurrentDir    :
    case CODEstatusTime          :
    case CODEstatusDate          :
    case CODEstatusMail          :
    case CODEstatusHost          :
    case CODEstatusUser          : return;

    case CODErestore             : rcAction = RESTORE;
                                   return;
    case CODEreRead              : rcAction = RESTART;
                                   return;
    case CODEflush               : rcAction = FLUSH;
                                   return;
    case CODEdefault             : rcAction = DEFAULT;
                                   return;
  }

  /* all codes below force us to rewrite the .keyshrc file when */
  /* IntrinsicsEnd() is called */
  if (globalInteractive) {
    rcAction = FLUSH;
  }
  switch (code) {

    case CODEoptTranslationOn    : globalOptions.enableTranslations = TRUE;
                                   break;
    case CODEoptTranslationOff   : globalOptions.enableTranslations = FALSE;
                                   break;
    case CODEoptBackupsOn        : globalOptions.enableBackups = TRUE;
                                   break;
    case CODEoptBackupsOff       : globalOptions.enableBackups = FALSE;
                                   break;
    case CODEoptInvisiblesOn     : globalOptions.enableInvisibles = TRUE;
                                   break;
    case CODEoptInvisiblesOff    : globalOptions.enableInvisibles = FALSE;
                                   break;
    case CODEoptPromptsOn        : globalOptions.enablePrompts = TRUE;
                                   break;
    case CODEoptPromptsOff       : globalOptions.enablePrompts = FALSE;
                                   break;
    case CODEoptAcceleratorsOn   : globalOptions.enableAccelerators = TRUE;
                                   ConfigAssignAcceleratorsEtc(globalSoftKeys);
                                   break;
    case CODEoptAcceleratorsOff  : globalOptions.enableAccelerators = FALSE;
                                   ConfigAssignAcceleratorsEtc(globalSoftKeys);
                                   break;
    case CODEoptVisiblesOn       : globalOptions.enableVisibles = TRUE;
                                   break;
    case CODEoptVisiblesOff      : globalOptions.enableVisibles = FALSE;
                                   break;
    case CODEoptHelpOn           : globalOptions.enableHelpKey = TRUE;
                                   UpdateKeyshell();
                                   break;
    case CODEoptHelpOff          : globalOptions.enableHelpKey = FALSE;
                                   UpdateKeyshell();
                                   break;

    case CODEstatusCurrentDirOn  : statusLine.dir = TRUE;
                                   break;
    case CODEstatusCurrentDirOff : statusLine.dir = FALSE;
                                   break;
    case CODEstatusTimeOn        : statusLine.time = TRUE;
                                   break;
    case CODEstatusTimeOff       : statusLine.time = FALSE;
                                   break;
    case CODEstatusDateOn        : statusLine.date = TRUE;
                                   break;
    case CODEstatusDateOff       : statusLine.date = FALSE;
                                   break;
    case CODEstatusMailOn        : statusLine.mail = TRUE;
                                   break;
    case CODEstatusMailOff       : statusLine.mail = FALSE;
                                   break;
    case CODEstatusHostOn        : statusLine.host = TRUE;
                                   break;
    case CODEstatusHostOff       : statusLine.host = FALSE;
                                   break;
    case CODEstatusUserOn        : statusLine.user = TRUE;
                                   break;
    case CODEstatusUserOff       : statusLine.user = FALSE;
                                   break;

    case CODEsoftKeyAdd          : keyshell.add = TRUE;
                                   break;
    case CODEsoftKeyLabel        : keyshell.withLabel = word;
                                   break;
    case CODEsoftKeyInvisibles   : keyshell.invisibles = TRUE;
                                   break;
    case CODEsoftKeyBackups      : keyshell.backups = TRUE;
                                   break;
    case CODEsoftKeyPSoftKey     :
    case CODEsoftKeyDSoftKey     :
    case CODEsoftKeyASoftKey     : keyshell.softKey = word;
                                   break;
    case CODEsoftKeyUser         : keyshell.fromUser = word;
                                   break;
    case CODEsoftKeyFile         : keyshell.fromFile = word;
                                   break;
    case CODEsoftKeyFirst        : keyshell.first = TRUE;
                                   break;
    case CODEsoftKeyBefore       : keyshell.before = word;
                                   break;
    case CODEsoftKeyDelete       : keyshell.delete = TRUE;
                                   break;
    case CODEsoftKeyPlace        : keyshell.place = TRUE;
                                   break;

    default                      : assert(FALSE);
  }
}


/******************************** IntrinsicsSelected() ***********************/
/*                                                                           */
/* This routine returns 0 if the specified softkey has a global option       */
/* flag associated with it which is FALSE or 1 if the specified softkey      */
/* has a global option flag associated with it which is TRUE.  If the        */
/* specified softkey does not have a global option flag associated with      */
/* it, -1 is returned.                                                       */
/*                                                                           */
/* This routine is used to determine whether to put a "*", " ", or nothing   */
/* in the lower right corner of the displayed softkey label.                 */
/*                                                                           */
/*****************************************************************************/

int IntrinsicsSelected(sk)
GlobalSoftKey *sk;
{
  int n;

  for (n = 0; n < INITIALS; n++) {
    if (sk->code == initial[n].pcode) {
      return *initial[n].parameter;
    }
  }
  return -1;
}


/******************************** IntrinsicsBegin() **************************/
/*                                                                           */
/* This routine performs pre-line initialization for the intrinsics module.  */
/*                                                                           */
/*****************************************************************************/

void IntrinsicsBegin()
{
  /* reset all static variables which might be set by IntrinsicsExecute() */
  /* back to their default values. */
  keyshell.add = FALSE;
  keyshell.delete = FALSE;
  keyshell.place = FALSE;
  keyshell.first = FALSE;
  keyshell.invisibles = FALSE;
  keyshell.backups = FALSE;
  keyshell.softKey = NULL;
  keyshell.withLabel = NULL;
  keyshell.fromUser = NULL;
  keyshell.fromFile = NULL;
  keyshell.before = NULL;

  if (! globalNonInteractive) {
    globalInteractive = TRUE;
  }
}


/***************************** IntrinsicsEnd() *******************************/
/*                                                                           */
/* This routine performs post-line clean-up for the intrinsics module.       */
/*                                                                           */
/*****************************************************************************/

void IntrinsicsEnd()
{
  /* if IntrinsicsExecute() was called to add, move, or delete a softkey, */
  /* then do the real work now. */
  if (keyshell.add) {
    UpdateSoftKeys(AddSoftKey());
  } else if (keyshell.place) {
    PlaceSoftKey();
  } else if (keyshell.delete) {
    UpdateSoftKeys(DeleteSoftKey());
  }

  /* if we had a restart, restart default, or undo request, then do it now */
  if (rcAction == RESTART) {
    ReReadRCFile();
    KshHistWrite(editLine);
  } else if (rcAction == DEFAULT) {
    if (OpenRCFile(DELETING) == 0) {
      ReReadRCFile();
    }
    KshHistWrite(editLine);
  } else if (rcAction == RESTORE) {
    RestoreRCFile();
    KshHistWrite(editLine);

  /* otherwise, if our configuration changed or we got a "write" request, */
  /* then rewrite the .keyshrc file */
  } else if (rcAction == FLUSH) {
    WriteRCFile();
  }
  rcAction = NONE;
}


/*****************************************************************************/
